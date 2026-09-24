/*
	AudioWorklet: DrSnuggles
*/

import libopenmptPromise from './libopenmpt.worklet.js'

// consts
const OPENMPT_MODULE_RENDER_STEREOSEPARATION_PERCENT = 2
const OPENMPT_MODULE_RENDER_INTERPOLATIONFILTER_LENGTH = 3

// vars
let libopenmpt

// init
libopenmptPromise()
.then(res => {
	libopenmpt = res

	if (!libopenmpt.stackSave) return
	// set libopenmpt version to display later
	let stack = libopenmpt.stackSave()
	libopenmpt.version = libopenmpt.UTF8ToString(libopenmpt._openmpt_get_string(asciiToStack('library_version')))
	libopenmpt.build = libopenmpt.UTF8ToString(libopenmpt._openmpt_get_string(asciiToStack('build')))
	libopenmpt.stackRestore(stack)
})
.catch(e => console.error(e))

//
// Helpers
//
function asciiToStack(str) {
	const stackStr = libopenmpt.stackAlloc(str.length + 1)			// DrS: needed to export in emscripten
	writeAsciiToMemory(str, stackStr)					// no longer in Emscripten, see below
	return stackStr
}
// Scope stack strings: stackAlloc without stackRestore overflows the WASM stack.
function ctlSet(modulePtr, key, value) {
	const sp = libopenmpt.stackSave()
	try { libopenmpt._openmpt_module_ctl_set(modulePtr, asciiToStack(key), asciiToStack(value)) }
	finally { libopenmpt.stackRestore(sp) }
}
// libopenmpt returns caller-owned strings; free each after copying it out.
function takeString(ptr) {
	const str = libopenmpt.UTF8ToString(ptr)
	libopenmpt._openmpt_free_string(ptr)
	return str
}
function writeAsciiToMemory(str,buffer,dontAddNull){for(let i=0;i<str.length;++i){libopenmpt.HEAP8[buffer++>>0]=str.charCodeAt(i)}if(!dontAddNull)libopenmpt.HEAP8[buffer>>0]=0}


//
// Processor
//
class MPT extends AudioWorkletProcessor {
	constructor() {
		super()
		this.port.onmessage = this.handleMessage_.bind(this)
		this.paused = false
		this.config = {
			repeatCount: -1,		// -1 = play endless, 0 = play once, do not repeat
			stereoSeparation: 100,	// percents
			interpolationFilter: 0,	// https://lib.openmpt.org/doc/group__openmpt__module__render__param.html
		}
		this.channels = 0
	}

	process(inputList, outputList, parameters) {
		if (!this.modulePtr || !this.leftPtr || !this.rightPtr || this.paused) return true	//silence

		const left = outputList[0][0]
		const right = outputList[0][1]

		const actualFramesPerChunk = libopenmpt._openmpt_module_read_float_stereo(this.modulePtr, sampleRate, left.length, this.leftPtr, this.rightPtr)
		if (actualFramesPerChunk == 0) {
			//this.paused = true
			// modulePtr will be 0 on openmpt: error: openmpt_module_read_float_stereo: ERROR: module * not valid or other openmpt error
			const error = !this.modulePtr
			if (error) {
				this.port.postMessage({cmd:'err',val:'Process'})
			} else {
				this.port.postMessage({cmd:'end'})
			}
			this.paused = true	// report once, not every render quantum; play/unpause restarts
			return true
		}

		left.set(libopenmpt.HEAPF32.subarray(this.leftPtr / 4, this.leftPtr / 4 + actualFramesPerChunk))
		right.set(libopenmpt.HEAPF32.subarray(this.rightPtr / 4, this.rightPtr / 4 + actualFramesPerChunk))

		// post progress
		// 	openmpt_module_get_current_order

		let msg = {
			cmd: 'pos',
			pos: libopenmpt._openmpt_module_get_position_seconds(this.modulePtr),
			// pos in song
			order: libopenmpt._openmpt_module_get_current_order(this.modulePtr),
			pattern: libopenmpt._openmpt_module_get_current_pattern(this.modulePtr),
			row: libopenmpt._openmpt_module_get_current_row(this.modulePtr),
			vu: [], // per-channel {left, right}
		}
		for (let i = 0; i < this.channels; i++) {
			msg.vu.push( {
				left: libopenmpt._openmpt_module_get_current_channel_vu_left(this.modulePtr, i),
				right: libopenmpt._openmpt_module_get_current_channel_vu_right(this.modulePtr, i),
			})
		}

		this.port.postMessage( msg )

		return true // def. needed for Chrome
	}

	handleMessage_(msg) {
		//console.log('[Processor:Received]',msg.data)
		const v = msg.data.val
		switch (msg.data.cmd) {
			case 'config':
				this.config = v
				break
			case 'play':
				this.play(v, false)
				break
			case 'pause':
				this.paused = true
				break
			case 'unpause':
				this.paused = false
				break
			case 'togglePause':
				this.paused = !this.paused
				break
			case 'stop':
				this.stop()
				break
			case 'meta':
				this.meta()
				break
			case 'repeatCount':
				this.config.repeatCount = v
				if (!this.modulePtr) return
				libopenmpt._openmpt_module_set_repeat_count(this.modulePtr, this.config.repeatCount)
				break
			case 'setPitch':
				if (!libopenmpt.stackSave || !this.modulePtr) return
				ctlSet(this.modulePtr, 'play.pitch_factor', v.toString())
				break
			case 'setTempo':
				if (!libopenmpt.stackSave || !this.modulePtr) return
				ctlSet(this.modulePtr, 'play.tempo_factor', v.toString())
				break
			case 'selectSubsong':
				if (!this.modulePtr) return
				libopenmpt._openmpt_module_select_subsong(this.modulePtr, v)
				//this.meta()
				break
			case 'setPos':
				if (!this.modulePtr) return
				libopenmpt._openmpt_module_set_position_seconds(this.modulePtr, v)
				break
			case 'setOrderRow':
				if (!this.modulePtr) return
				libopenmpt._openmpt_module_set_position_order_row(this.modulePtr, v.o, v.r)
				break
			/*
			case 'toggleMute'
				// openmpt_module_ext_get_interface(mod_ext, interface_id, interface, interface_size)
				// openmpt_module_ext_interface_interactive
				// set_channel_mute_status
				// https://lib.openmpt.org/doc/group__libopenmpt__ext__c.html#ga0275a35da407cd092232a20d3535c9e4
				if (!this.modulePtr) return
				//const extPtr = libopenmpt.openmpt_module_ext_get_interface(mod_ext, interface_id, interface, interface_size)
				break
			*/
			case 'mute':
				if (this.setChannelMuteFn && this.extPtr) this.setChannelMuteFn(this.extPtr, v.ch, v.mute ? 1 : 0)
				break
			case 'decodeAll':
				this.decodeAll(v)
				break
			default:
				console.log('Received unknown message',msg.data)
		}
	} // handleMessage_

	decodeAll(buffer) {
		this.play(buffer, true)
		
		// now build the full audioData
		const left = [], right = []
		const maxFramesPerChunk = 128
		let actualFramesPerChunk = libopenmpt._openmpt_module_read_float_stereo(this.modulePtr, sampleRate, maxFramesPerChunk, this.leftPtr, this.rightPtr)
		while (actualFramesPerChunk != 0) {
			left.push( ...libopenmpt.HEAPF32.subarray(this.leftPtr / 4, this.leftPtr / 4 + actualFramesPerChunk) )
			right.push( ...libopenmpt.HEAPF32.subarray(this.rightPtr / 4, this.rightPtr / 4 + actualFramesPerChunk) )
			actualFramesPerChunk = libopenmpt._openmpt_module_read_float_stereo(this.modulePtr, sampleRate, maxFramesPerChunk, this.leftPtr, this.rightPtr)
		}

		// post final data
		let msg = {
			cmd: 'fullAudioData',
			meta: this.getMeta(),
			data: [left,right]
		}
		this.port.postMessage( msg )

		this.stop()
	}

	play(buffer, paused = false) {
		this.stop()

		const maxFramesPerChunk = 128	// thats what worklet is using
		const byteArray = new Int8Array(buffer)
		const ptrToFile = libopenmpt._malloc(byteArray.byteLength)
		libopenmpt.HEAPU8.set(byteArray, ptrToFile)
		// ext module so we can reach the interactive interface (channel mute etc.)
		try {
			this.extPtr = libopenmpt._openmpt_module_ext_create_from_memory(ptrToFile, byteArray.byteLength, 0, 0, 0, 0, 0, 0, 0)
		} finally {
			// libopenmpt owns its decoded module; the input copy is temporary.
			libopenmpt._free(ptrToFile)
		}
		this.modulePtr = this.extPtr ? libopenmpt._openmpt_module_ext_get_module(this.extPtr) : 0

		if(this.modulePtr === 0) {
			// could not create module
			this.port.postMessage({cmd:'err',val:'ptr'})
			return
		}

		// interactive interface: struct of function pointers; slot 10 = set_channel_mute_status
		this.setChannelMuteFn = null
		const ifaceStack = libopenmpt.stackSave?.()
		if (ifaceStack !== undefined) try {
			const ifaceBuf = libopenmpt.stackAlloc(64)
			if (libopenmpt._openmpt_module_ext_get_interface(this.extPtr, asciiToStack('interactive'), ifaceBuf, 64)) {
				const dv = new DataView(libopenmpt.HEAPU8.buffer, ifaceBuf, 64)
				this.setChannelMuteFn = libopenmpt.wasmTable.get(dv.getUint32(10 * 4, true))
			}
		} catch (e) { /* mute unavailable in this build */ }
		finally { libopenmpt.stackRestore(ifaceStack) }

		if (libopenmpt.stackSave) {
			const stack = libopenmpt.stackSave()
			libopenmpt._openmpt_module_ctl_set(this.modulePtr, asciiToStack('render.resampler.emulate_amiga'), asciiToStack('1'))
			libopenmpt._openmpt_module_ctl_set(this.modulePtr, asciiToStack('render.resampler.emulate_amiga_type'), asciiToStack('a1200'))
			//libopenmpt._openmpt_module_ctl_set('play.pitch_factor', e.target.value.toString());

			libopenmpt.stackRestore(stack)
		}
		
		this.paused = paused
		this.channels = libopenmpt._openmpt_module_get_num_channels(this.modulePtr)
		this.leftPtr = libopenmpt._malloc(4 * maxFramesPerChunk)	// 4x = float
		this.rightPtr = libopenmpt._malloc(4 * maxFramesPerChunk)

		// set config options on module
		libopenmpt._openmpt_module_set_repeat_count(this.modulePtr, this.config.repeatCount)
		libopenmpt._openmpt_module_set_render_param(this.modulePtr, OPENMPT_MODULE_RENDER_STEREOSEPARATION_PERCENT, this.config.stereoSeparation)
		libopenmpt._openmpt_module_set_render_param(this.modulePtr, OPENMPT_MODULE_RENDER_INTERPOLATIONFILTER_LENGTH, this.config.interpolationFilter)

		// post back tracks metadata
		if (!paused) this.meta()
	}
	stop() {
		if (this.extPtr) {
			libopenmpt._openmpt_module_ext_destroy(this.extPtr)
			this.extPtr = 0
			this.modulePtr = 0
		} else if (this.modulePtr) {
			libopenmpt._openmpt_module_destroy(this.modulePtr)
			this.modulePtr = 0
		}
		if (this.leftPtr) {
			libopenmpt._free(this.leftPtr)
			this.leftPtr = 0
		}
		if (this.rightPtr) {
			libopenmpt._free(this.rightPtr)
			this.rightPtr = 0
		}
		this.setChannelMuteFn = null
		this.channels = 0
	}
	meta() {
		this.port.postMessage({cmd: 'meta', meta: this.getMeta()})
	}
	getSong() {
		if (!libopenmpt.UTF8ToString || !this.modulePtr) return false

		// https://lib.openmpt.org/doc/
		let song = {
			channels: [],
			instruments: [],
			samples: [],
			orders: [],
			numSubsongs: libopenmpt._openmpt_module_get_num_subsongs(this.modulePtr),
			patterns: [],
		}
		// channels
		const chNum = libopenmpt._openmpt_module_get_num_channels(this.modulePtr)
		this.channel = chNum
		for (let i = 0; i < chNum; i++) {
			song.channels.push( takeString(libopenmpt._openmpt_module_get_channel_name(this.modulePtr, i)) )
		}
		// instruments
		for (let i = 0, e = libopenmpt._openmpt_module_get_num_instruments(this.modulePtr); i < e; i++) {
			song.instruments.push( takeString(libopenmpt._openmpt_module_get_instrument_name(this.modulePtr, i)) )
		}
		// samples
		for (let i = 0, e = libopenmpt._openmpt_module_get_num_samples(this.modulePtr); i < e; i++) {
			song.samples.push( takeString(libopenmpt._openmpt_module_get_sample_name(this.modulePtr, i)) )
		}
		// orders
		for (let i = 0, e = libopenmpt._openmpt_module_get_num_orders(this.modulePtr); i < e; i++) {
			song.orders.push( {
				name: takeString(libopenmpt._openmpt_module_get_order_name(this.modulePtr, i)),
				pat: libopenmpt._openmpt_module_get_order_pattern(this.modulePtr, i),
			})
		}
		// patterns — cells as formatted tracker strings "C-5 01 v40 J37" (13 wide)
		for (let patIdx = 0, patNum = libopenmpt._openmpt_module_get_num_patterns(this.modulePtr); patIdx < patNum; patIdx++) {
			const pattern = {
				name: takeString(libopenmpt._openmpt_module_get_pattern_name(this.modulePtr, patIdx)),
				rows: [],
			}
			// rows
			for(let rowIdx = 0, rowNum = libopenmpt._openmpt_module_get_pattern_num_rows(this.modulePtr, patIdx); rowIdx < rowNum; rowIdx++) {
				const row = []
				// channels
				for (let chIdx = 0; chIdx < chNum; chIdx++) {
					const ptr = libopenmpt._openmpt_module_format_pattern_row_channel(this.modulePtr, patIdx, rowIdx, chIdx, 13, 1)
					row.push( libopenmpt.UTF8ToString(ptr) )
					libopenmpt._openmpt_free_string(ptr)
				}
				pattern.rows.push( row )
			}
			song.patterns.push( pattern )
		}


		return song
	}
	getMeta() {
		if (!libopenmpt.UTF8ToString || !this.modulePtr) return false

		const data = {}
		data.dur = libopenmpt._openmpt_module_get_duration_seconds(this.modulePtr)
		if (data.dur == 0) {
			// looks like an error occured reading the mod
			this.port.postMessage({cmd:'err',val:'dur'})
		}
		const keys = takeString(libopenmpt._openmpt_module_get_metadata_keys(this.modulePtr)).split(';')
		for (let i = 0; i < keys.length; i++) {
			const keyNameBuffer = libopenmpt._malloc(keys[i].length + 1)
			writeAsciiToMemory(keys[i], keyNameBuffer)
			data[keys[i]] = takeString(libopenmpt._openmpt_module_get_metadata(this.modulePtr, keyNameBuffer))
			libopenmpt._free(keyNameBuffer)
		}
		data.song = this.getSong()
		data.totalOrders = data.song.orders.length	// libopenmpt._openmpt_module_get_num_orders(this.modulePtr)
		data.totalPatterns = data.song.patterns.length// libopenmpt._openmpt_module_get_num_patterns(this.modulePtr)
		data.songs = this.getSongs()
		data.libopenmptVersion = libopenmpt.version
		data.libopenmptBuild = libopenmpt.build
		return data
	}
	getSongs() {
		if (!libopenmpt.UTF8ToString) return ''	// todo: ?? why string here

		const subsongs = libopenmpt._openmpt_module_get_num_subsongs(this.modulePtr)
		const names = []
		for (let i = 0; i < subsongs; i++) {
			const namePtr = libopenmpt._openmpt_module_get_subsong_name(this.modulePtr, i)
			const name = libopenmpt.UTF8ToString(namePtr)
			if(name != "") {
				names.push(name)
			} else {
				names.push("Subsong " + (i + 1))
			}
			libopenmpt._openmpt_free_string(namePtr)
		}
		return names
	}

}

registerProcessor('libopenmpt-processor', MPT)
