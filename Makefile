# ==============================================================================
# libturtle: Cross-platform C Turtle Graphics Library
# Supports Raylib (desktop) and canvas-wasi (HTML Canvas WebAssembly)
# ==============================================================================

BACKEND ?= raylib

BUILD_DIR = build/$(BACKEND)
LIB_TARGET = $(BUILD_DIR)/libturtle_$(BACKEND).a

# Upstream canvas-wasi header URLs
CANVAS_WASI_URL = https://raw.githubusercontent.com/breeze833/canvas-wasi/main/include
CANVAS_HEADERS = backend/canvas-wasi/include/graphics.h backend/canvas-wasi/include/graphics_canvas.h

# ------------------------------------------------------------------------------
# Backend Configuration
# ------------------------------------------------------------------------------
ifeq ($(BACKEND),raylib)
    CC ?= gcc
    AR ?= ar
    CFLAGS ?= -O2 -Wall -Wextra
    INCLUDES = -Iinclude -Ibackend/raylib/include $(shell pkg-config --cflags raylib 2>/dev/null || echo "-I/usr/local/include")
    LDFLAGS = $(shell pkg-config --libs raylib 2>/dev/null || echo "-L/usr/local/lib -lraylib") -lm -lpthread -ldl -lrt -lX11
    HAL_SRC = backend/raylib/hal_raylib.c
    DEMO_SRC = examples/raylib_demo.c
    DEMO_TARGET = $(BUILD_DIR)/demo
else ifneq ($(filter $(BACKEND),canvas-wasi htmlcanvas),)
    CC = clang
    WASM_TARGET ?= wasm32-wasi
    AR = $(shell which llvm-ar llvm-ar-19 ar 2>/dev/null | head -n 1)
    CFLAGS ?= -O2 -Wall -Wextra
    INCLUDES = --target=$(WASM_TARGET) -Iinclude -Ibackend/canvas-wasi/include
    LDFLAGS = --target=$(WASM_TARGET) -nodefaultlibs -lc -lm -Wl,--allow-undefined
    HAL_SRC = backend/canvas-wasi/hal_canvas_wasi.c
    DEMO_SRC = examples/wasm_demo.c
    DEMO_TARGET = $(BUILD_DIR)/demo.wasm
else
    $(error Unknown BACKEND '$(BACKEND)'. Valid choices: raylib, canvas-wasi)
endif

CORE_SRC = src/turtle.c
CORE_OBJ = $(BUILD_DIR)/turtle.o
HAL_OBJ  = $(BUILD_DIR)/hal.o

# ------------------------------------------------------------------------------
# Targets
# ------------------------------------------------------------------------------
.PHONY: all clean demo fetch-canvas-headers help

all: $(LIB_TARGET)

# Ensure canvas-wasi headers exist before building canvas-wasi
ifneq ($(filter $(BACKEND),canvas-wasi htmlcanvas),)
$(HAL_OBJ): $(CANVAS_HEADERS)
endif

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(CORE_OBJ): $(CORE_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(HAL_OBJ): $(HAL_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(LIB_TARGET): $(CORE_OBJ) $(HAL_OBJ)
	$(AR) rcs $@ $^

demo: $(LIB_TARGET) $(DEMO_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) $(DEMO_SRC) $(LIB_TARGET) $(LDFLAGS) -o $(DEMO_TARGET)
ifneq ($(filter $(BACKEND),canvas-wasi htmlcanvas),)
	@cp -f $(DEMO_TARGET) $(BUILD_DIR)/demo 2>/dev/null || true
endif
	@echo "Built demo: $(DEMO_TARGET)"

run: demo
ifeq ($(BACKEND),raylib)
	./build/raylib/demo
else
	@echo "Serving canvas-wasi demo on http://localhost:8000/examples/ ..."
	@echo "Open your browser to: http://localhost:8000/examples/"
	@python3 -m http.server 8000
endif

serve:
	@echo "Serving canvas-wasi demo on http://localhost:8000/examples/ ..."
	@echo "Open your browser to: http://localhost:8000/examples/"
	@python3 -m http.server 8000

fetch-canvas-headers:
	@mkdir -p backend/canvas-wasi/include
	@if [ -n "$(CANVAS_WASI_DIR)" ] && [ -d "$(CANVAS_WASI_DIR)/include" ]; then \
		echo "Copying canvas-wasi headers from $(CANVAS_WASI_DIR)/include..."; \
		cp "$(CANVAS_WASI_DIR)/include/graphics.h" backend/canvas-wasi/include/; \
		cp "$(CANVAS_WASI_DIR)/include/graphics_canvas.h" backend/canvas-wasi/include/; \
	else \
		echo "Downloading latest canvas-wasi headers from GitHub..."; \
		curl -fsSL $(CANVAS_WASI_URL)/graphics.h -o backend/canvas-wasi/include/graphics.h || wget -qO backend/canvas-wasi/include/graphics.h $(CANVAS_WASI_URL)/graphics.h; \
		curl -fsSL $(CANVAS_WASI_URL)/graphics_canvas.h -o backend/canvas-wasi/include/graphics_canvas.h || wget -qO backend/canvas-wasi/include/graphics_canvas.h $(CANVAS_WASI_URL)/graphics_canvas.h; \
	fi
	@echo "canvas-wasi headers updated."

$(CANVAS_HEADERS):
	@$(MAKE) fetch-canvas-headers

clean:
	rm -rf build

help:
	@echo "Available targets:"
	@echo "  make BACKEND=raylib           Build libturtle_raylib.a"
	@echo "  make demo BACKEND=raylib      Build native desktop demo executable"
	@echo "  make run BACKEND=raylib       Build and run native desktop demo"
	@echo "  make BACKEND=canvas-wasi      Build libturtle_canvas-wasi.a for WASI"
	@echo "  make demo BACKEND=canvas-wasi Build WebAssembly WASI demo module"
	@echo "  make run BACKEND=canvas-wasi  Build demo and start local web server"
	@echo "  make serve                    Start local web server for browser demo"
	@echo "  make fetch-canvas-headers     Download latest canvas-wasi headers"
	@echo "  make clean                    Remove build directory"
