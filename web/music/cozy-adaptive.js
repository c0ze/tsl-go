/**
 * CozyAdaptive — the drop-in adaptive-music runtime for the web.
 *
 * Pairs a cozy-generated module (.it) with its manifest (.cozy.json):
 *
 *   const music = await CozyAdaptive.create('level1.it', 'level1.cozy.json');
 *   music.setIntensity(0.7);      // mutes layers above the threshold
 *   music.transitionTo('combat'); // jumps at the next pattern boundary
 *   music.transitionTo('explore', { via: 'bridge' }); // musical transition
 *   music.transitionTo('combat', { at: 'bar' }); // jumps at the next barline
 *   await music.load('level2.it', 'level2.cozy.json'); // swap modules, same audio graph
 *
 * Manifest shape:
 *   { layers:   [{ name, channels: [..], above: 0.0 }, ...],   // intensity-ordered
 *     sections: { name: [firstOrder, lastOrder], ... },
 *     bars:     { name: rowsPerBar, ... },                      // optional, for at: 'bar'
 *     loop: "explore" }
 *
 * Sections loop themselves until a transition is requested. Layer control is
 * real engine-level channel muting (libopenmpt ext interactive interface),
 * so intensity changes are sample-accurate and free.
 */
import { ChiptuneJsPlayer } from './vendor/chiptune3/chiptune3.js';

export class CozyAdaptive {
  constructor(player, manifest) {
    this.player = player;
    this.manifest = manifest;
    this.intensity = 1;
    this.section = null;
    this._queue = [];        // pending section transitions
    this._nextBoundary = false; // start a new request at the next pattern
    this._atBar = false;     // ...or at the next barline of the current section
    this._settling = null;   // section we just jumped to, until observed
    this._lastOrder = -1;
    this._lastRow = -1;
    this._sectionCbs = [];
    this._progressCbs = [];
    this.playing = false;

    player.onProgress((d) => this._onProgress(d));
  }

  /**
   * Load module + manifest and start. Rejects (instead of hanging) when the
   * worklet, manifest or module cannot load, or after opts.timeout ms
   * (default 15000). A failed create closes the AudioContext it opened.
   */
  static async create(moduleUrl, manifest, opts = {}) {
    const g = globalThis;
    if (!opts.createPlayer && (!g.isSecureContext || !('audioWorklet' in (g.AudioContext?.prototype ?? {})))) {
      throw new Error(`audio needs HTTPS — open https://${g.location?.host}${g.location?.pathname}`);
    }
    const timeout = opts.timeout ?? 15000;
    // Construct before any await so the AudioContext is created inside the user gesture.
    const player = opts.createPlayer
      ? opts.createPlayer()
      : new ChiptuneJsPlayer({ repeatCount: -1, context: opts.context });
    const ownsContext = !opts.context;
    try {
      const initialized = waitFor(player, 'onInitialized', timeout, 'audio worklet');
      if (typeof manifest === 'string') manifest = await fetchChecked(manifest, (r) => r.json());
      validateManifest(manifest);
      const buf = await fetchChecked(moduleUrl, (r) => r.arrayBuffer());
      await initialized;
      const ready = waitFor(player, 'onMetadata', timeout, 'module');
      player.play(buf);
      await ready;
    } catch (e) {
      disposePlayer(player, ownsContext);
      throw e;
    }
    const ca = new CozyAdaptive(player, manifest);
    ca._ownsContext = ownsContext;
    ca.playing = true;
    ca._jump(manifest.loop || Object.keys(manifest.sections)[0]);
    ca.setIntensity(opts.intensity ?? 1);
    return ca;
  }

  /**
   * Replace the module and manifest on the same player (and AudioContext),
   * starting opts.section or the new manifest's loop section. When loads
   * overlap, only the latest one starts; earlier ones resolve to false.
   */
  async load(moduleUrl, manifest, opts = {}) {
    const seq = (this._loadSeq = (this._loadSeq ?? 0) + 1);
    const [m, buf] = await Promise.all([
      typeof manifest === 'string' ? fetchChecked(manifest, (r) => r.json()) : manifest,
      fetchChecked(moduleUrl, (r) => r.arrayBuffer()),
    ]);
    validateManifest(m);
    if (seq !== this._loadSeq) return false;
    this.playing = false; // ignore progress from the outgoing module
    const ready = waitFor(this.player, 'onMetadata', opts.timeout ?? 15000, 'module');
    this.player.play(buf);
    await ready;
    if (seq !== this._loadSeq) return false;
    this.manifest = m;
    this._queue = [];
    this._nextBoundary = false;
    this._atBar = false;
    this.playing = true;
    this._jump(opts.section ?? m.loop ?? Object.keys(m.sections)[0]);
    this.setIntensity(this.intensity);
    return true;
  }

  /** 0..1 — layers with `above` greater than this are muted. */
  setIntensity(x) {
    this.intensity = Math.max(0, Math.min(1, x));
    for (const layer of this.manifest.layers || []) {
      const active = this.intensity >= (layer.above ?? 0);
      for (const ch of layer.channels) this.player.setChannelMute(ch, !active);
    }
    return this;
  }

  /** Names of layers currently sounding at this intensity. */
  activeLayers() {
    return (this.manifest.layers || [])
      .filter((l) => this.intensity >= (l.above ?? 0))
      .map((l) => l.name);
  }

  /**
   * Move to a named section at the next pattern boundary.
   * opts.via: play another section (e.g. a bridge) once on the way.
   * opts.now: jump immediately instead of waiting for the boundary.
   * opts.at: 'bar' — start at the next barline instead, using the current
   *   section's manifest.bars entry (falls back to the pattern boundary).
   */
  transitionTo(name, opts = {}) {
    if (!this.manifest.sections?.[name]) throw new Error(`unknown section: ${name}`);
    if (opts.via && !this.manifest.sections[opts.via]) throw new Error(`unknown section: ${opts.via}`);
    this._queue = opts.via ? [opts.via, name] : [name];
    this._nextBoundary = true;
    this._atBar = opts.at === 'bar';
    if (opts.now || !this.playing) this._advance();
    return this;
  }

  onSection(cb) { this._sectionCbs.push(cb); return this; }
  onProgress(cb) { this._progressCbs.push(cb); return this; }

  pause() { this.player.pause(); this.playing = false; return this; }
  resume() { this.player.unpause(); this.playing = true; return this; }
  setVolume(v) { this.player.setVol(v); return this; }
  stop() { this.player.stop(); this.playing = false; return this; }
  /** Stop and release audio resources; closes the AudioContext create() opened. */
  dispose() {
    this.playing = false;
    this._queue = [];
    this._sectionCbs = [];
    this._progressCbs = [];
    disposePlayer(this.player, this._ownsContext);
  }

  // --- internals -------------------------------------------------------------
  _range(name) { return this.manifest.sections[name]; }

  _jump(name) {
    this.section = name;
    this._settling = name;
    this._lastOrder = -1; // force re-evaluation: the jump may target the order we're already on
    this._lastRow = -1;
    this.player.setOrderRow(this._range(name)[0], 0);
    for (const cb of this._sectionCbs) cb(name);
  }

  _advance() {
    if (!this._queue.length) return;
    // After entering a via section, play its complete range before advancing.
    this._nextBoundary = false;
    this._atBar = false;
    this._jump(this._queue.shift());
  }

  _onProgress(d) {
    if (!this.playing) return;
    for (const cb of this._progressCbs) cb(d);
    const previousOrder = this._lastOrder;
    // A one-order module wraps without changing order. Internal SBx loops to
    // row zero are indistinguishable here; adaptive sections use linear patterns.
    const rowWrapped = d.order === previousOrder && d.row === 0 && this._lastRow > 0;
    const boundary = d.order !== previousOrder || rowWrapped;
    const newRow = d.order !== previousOrder || d.row !== this._lastRow;
    this._lastOrder = d.order;
    this._lastRow = d.row;
    const bar = this._atBar && this.manifest.bars?.[this.section];
    if (!boundary) {
      // a bar-quantized request starts on the first row of the next bar
      if (bar && newRow && !this._settling && this._queue.length && this._nextBoundary && d.row % bar === 0) this._advance();
      return;
    }
    const [s, e] = this.section ? this._range(this.section) : [0, Infinity];

    if (this._settling) {
      // ignore stale progress until we land inside the section we jumped to
      if (d.order >= s && d.order <= e) this._settling = null;
      return;
    }
    const sectionEnded = d.order > e || d.order < s ||
      (previousOrder === e && (d.order < previousOrder || rowWrapped));
    if (this._queue.length && this._nextBoundary) {
      this._advance();
    } else if (sectionEnded) {
      // Finish the whole bridge, or loop the section with no pending request.
      if (this._queue.length) this._advance();
      else this._jump(this.section); // loop the section
    }
  }
}

function validateManifest(m) {
  const names = Object.keys(m?.sections ?? {});
  if (!names.length) throw new Error('manifest has no sections');
  for (const name of names) {
    const r = m.sections[name];
    if (!Array.isArray(r) || r.length !== 2 || !r.every(Number.isInteger) || r[0] < 0 || r[1] < r[0]) {
      throw new Error(`manifest section ${name} must be [firstOrder, lastOrder]`);
    }
  }
  if (m.loop != null && !m.sections[m.loop]) throw new Error(`manifest loop names unknown section: ${m.loop}`);
  for (const [name, rows] of Object.entries(m.bars ?? {})) {
    if (!m.sections[name] || !Number.isInteger(rows) || rows < 1) throw new Error(`manifest bars.${name} must be a positive row count for a known section`);
  }
  for (const layer of m.layers ?? []) {
    if (!Array.isArray(layer.channels) || !layer.channels.every((c) => Number.isInteger(c) && c >= 0)) {
      throw new Error(`manifest layer ${layer.name} needs integer channels`);
    }
  }
}

async function fetchChecked(url, read) {
  const res = await fetch(url);
  if (!res.ok) throw new Error(`${url}: HTTP ${res.status}`);
  return read(res);
}

// chiptune3 handlers cannot be removed, so each waiter ignores late events.
function waitFor(player, event, ms, what) {
  return new Promise((resolve, reject) => {
    let done = false;
    const settle = (fn, v) => { if (!done) { done = true; clearTimeout(timer); fn(v); } };
    const timer = setTimeout(() => settle(reject, new Error(`${what} did not load within ${ms} ms`)), ms);
    player[event]((v) => settle(resolve, v));
    player.onError?.((e) => settle(reject, new Error(`${what} failed to load (${e?.type ?? 'error'})`)));
  });
}

function disposePlayer(player, closeContext) {
  try { player.stop?.(); } catch { /* already gone */ }
  try { player.processNode?.disconnect(); } catch { /* not connected */ }
  if (closeContext && player.context?.state !== 'closed') player.context?.close?.().catch(() => {});
}
