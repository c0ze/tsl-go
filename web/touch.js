// On-screen controls for touch devices. Every button (and every tappable menu
// entry) just synthesises the keydown the keyboard would send, so the wasm's
// single key path handles phones and keyboards alike. Movement buttons repeat
// while held, like a held key.
(function () {
  "use strict";
  var pad = document.getElementById("touchpad");
  var overlay = document.getElementById("overlay");
  if (!pad) return;

  function send(key) {
    document.dispatchEvent(new KeyboardEvent("keydown", { key: key, bubbles: true }));
  }

  // Hold-to-repeat for the direction pad: one step on press, then a steady
  // walk after a short pause — released, cancelled, or dragged off, it stops.
  var timer = null;
  function stop() {
    if (timer) { clearTimeout(timer); clearInterval(timer); timer = null; }
  }
  pad.addEventListener("pointerdown", function (e) {
    var b = e.target.closest("button[data-key]");
    if (!b) return;
    e.preventDefault(); // no focus ring, no synthetic mouse events
    var key = b.getAttribute("data-key");
    // While a menu is up, only the arrows and OK/Esc make sense: a diagonal
    // or the centre would be read as an item letter (y/n even answer a
    // Yes/No prompt).
    if (!overlay.hidden && b.hasAttribute("data-map-only")) return;
    send(key);
    stop();
    if (b.hasAttribute("data-repeat")) {
      timer = setTimeout(function () { timer = setInterval(function () { send(key); }, 140); }, 350);
    }
  });
  ["pointerup", "pointercancel", "pointerout"].forEach(function (ev) {
    pad.addEventListener(ev, stop);
  });
  pad.addEventListener("contextmenu", function (e) { e.preventDefault(); }); // long-press menu

  // Tapping a menu entry picks it; tapping the rest of an open menu does
  // nothing (the Cancel button closes it).
  overlay.addEventListener("click", function (e) {
    var item = e.target.closest("[data-key]");
    if (item) send(item.getAttribute("data-key"));
  });
})();
