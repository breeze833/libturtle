// Web Worker executing demo.wasm with canvas-wasi host bindings

let wasmMemory = null;
let offscreen = null;
let ctx = null;
let controlArray = null;
let shouldClose = 0;
const keysDown = new Uint8Array(256);

self.onmessage = (e) => {
  if (e.data.type === "init_control") {
    controlArray = new Int32Array(e.data.buffer);
  } else if (e.data.type === "close") {
    shouldClose = 1;
  } else if (e.data.type === "keydown") {
    if (e.data.keyCode >= 0 && e.data.keyCode < 256) {
      keysDown[e.data.keyCode] = 1;
    }
  } else if (e.data.type === "keyup") {
    if (e.data.keyCode >= 0 && e.data.keyCode < 256) {
      keysDown[e.data.keyCode] = 0;
    }
  }
};

function readString(ptr) {
  if (!wasmMemory) return "";
  const bytes = new Uint8Array(wasmMemory.buffer);
  let end = ptr;
  while (bytes[end] !== 0 && end < bytes.length) {
    end++;
  }
  return new TextDecoder().decode(bytes.subarray(ptr, end));
}

// Minimal WASI Preview 1 host imports
const wasi_snapshot_preview1 = {
  proc_exit: (code) => {
    console.log("[WASI] Process exited with code:", code);
  },
  fd_write: (fd, iovs, iovs_len, nwritten_ptr) => {
    const view = new DataView(wasmMemory.buffer);
    let totalWritten = 0;
    for (let i = 0; i < iovs_len; i++) {
      const ptr = view.getUint32(iovs + i * 8, true);
      const len = view.getUint32(iovs + i * 8 + 4, true);
      totalWritten += len;
      const str = new TextDecoder().decode(new Uint8Array(wasmMemory.buffer, ptr, len));
      if (fd === 1) console.log(str);
      else if (fd === 2) console.error(str);
    }
    if (nwritten_ptr) {
      view.setUint32(nwritten_ptr, totalWritten, true);
    }
    return 0;
  },
  fd_close: () => 0,
  fd_seek: () => 0
};

// canvas-wasi host graphics bindings
const env = {
  graphics_init: (w, h, titlePtr) => {
    const title = readString(titlePtr);
    offscreen = new OffscreenCanvas(w, h);
    ctx = offscreen.getContext("2d");
    self.postMessage({ type: "init", width: w, height: h, title });
  },
  canvas_save: () => {
    if (ctx) ctx.save();
  },
  canvas_restore: () => {
    if (ctx) ctx.restore();
  },
  canvas_set_fill_style: (ptr) => {
    if (ctx) ctx.fillStyle = readString(ptr);
  },
  canvas_fill_rect: (x, y, w, h) => {
    if (ctx) ctx.fillRect(x, y, w, h);
  },
  canvas_begin_path: () => {
    if (ctx) ctx.beginPath();
  },
  canvas_close_path: () => {
    if (ctx) ctx.closePath();
  },
  canvas_set_stroke_style: (ptr) => {
    if (ctx) ctx.strokeStyle = readString(ptr);
  },
  canvas_set_line_width: (w) => {
    if (ctx) ctx.lineWidth = w;
  },
  canvas_set_line_cap: (ptr) => {
    if (ctx) ctx.lineCap = readString(ptr);
  },
  canvas_move_to: (x, y) => {
    if (ctx) ctx.moveTo(x, y);
  },
  canvas_line_to: (x, y) => {
    if (ctx) ctx.lineTo(x, y);
  },
  canvas_arc: (x, y, r, sa, ea, ccw) => {
    if (ctx) ctx.arc(x, y, r, sa, ea, Boolean(ccw));
  },
  canvas_scale: (sx, sy) => {
    if (ctx) ctx.scale(sx, sy);
  },
  canvas_translate: (x, y) => {
    if (ctx) ctx.translate(x, y);
  },
  canvas_rotate: (rad) => {
    if (ctx) ctx.rotate(rad);
  },
  canvas_stroke: () => {
    if (ctx) ctx.stroke();
  },
  canvas_fill: () => {
    if (ctx) ctx.fill();
  },
  canvas_get_image_data: (sx, sy, sw, sh, outPtr) => {
    if (!ctx || !wasmMemory || sw <= 0 || sh <= 0) return;
    const imgData = ctx.getImageData(sx, sy, sw, sh);
    const dest = new Uint8Array(wasmMemory.buffer, outPtr, sw * sh * 4);
    dest.set(imgData.data);
  },
  canvas_put_image_data: (dx, dy, dw, dh, inPtr) => {
    if (!ctx || !wasmMemory || dw <= 0 || dh <= 0) return;
    const src = new Uint8Array(wasmMemory.buffer, inPtr, dw * dh * 4);
    const clamped = new Uint8ClampedArray(dw * dh * 4);
    clamped.set(src);
    const imgData = new ImageData(clamped, dw, dh);
    ctx.putImageData(imgData, dx, dy);
  },
  canvas_present: () => {
    if (!offscreen) return;
    /* Create a temporary OffscreenCanvas so transferToImageBitmap does not wipe persistent offscreen */
    const temp = new OffscreenCanvas(offscreen.width, offscreen.height);
    const tctx = temp.getContext("2d");
    tctx.drawImage(offscreen, 0, 0);
    const bitmap = temp.transferToImageBitmap();
    self.postMessage({ type: "frame", bitmap }, [bitmap]);
  },
  graphics_wait_frame: (targetFps) => {
    const waitMs = Math.max(1, Math.floor(1000 / (targetFps || 60)));
    const start = performance.now();
    while (performance.now() - start < waitMs) {
      /* Synchronous frame delay in Web Worker */
    }
  },
  graphics_close: () => {
    self.postMessage({ type: "close" });
  },
  graphics_should_close: () => {
    if (controlArray && controlArray[0]) return 1;
    return shouldClose ? 1 : 0;
  },
  graphics_is_key_down: (key) => {
    if (controlArray && key >= 0 && key < controlArray.length) {
      return controlArray[key] ? 1 : 0;
    }
    return (key >= 0 && key < 256) ? (keysDown[key] ? 1 : 0) : 0;
  }
};

async function run() {
  try {
    const response = await fetch("../build/canvas-wasi/demo.wasm");
    const wasmBytes = await response.arrayBuffer();

    const module = await WebAssembly.instantiate(wasmBytes, {
      env,
      wasi_snapshot_preview1
    });

    wasmMemory = module.instance.exports.memory;
    module.instance.exports._start();
  } catch (err) {
    console.error("Failed to run WebAssembly demo:", err);
  }
}

run();
