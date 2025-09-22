## Makefile helpers para construir GP2040-CE con Docker
## Variables:
##   BOARD=<ConfigName>     (por defecto: Pico)
##   SDK_VER=<versión>      (por defecto: 2.1.1; puedes probar 1.5.1 si ajustas CMake)
##   SKIP_WEBBUILD=1        (omite npm ci/build del web UI)
##   IMAGE=gp2040ce-build   (nombre de imagen)

.PHONY: docker-build docker-shell docker-configure docker-build-project docker-clean clean docker-www docker-configure-release

BOARD ?= Pico
SDK_VER ?= 2.1.1
IMAGE ?= gp2040ce-build
CM_EXTRAS ?= -D SKIP_PICO_SDK_VERSION_CHECK=ON

# Build Docker image with specified SDK version
docker-build:
	docker build --build-arg PICO_SDK_VERSION=$(SDK_VER) -t $(IMAGE) .

# Open interactive shell in container
docker-shell:
	docker run --rm -it \
		-e GP2040_BOARDCONFIG=$(BOARD) \
		-e SKIP_WEBBUILD=$(SKIP_WEBBUILD) \
		-v "$(CURDIR):/workspace" -w "/workspace" $(IMAGE) bash

# Configure CMake (default: Debug unless overridden by CM_EXTRAS)
docker-configure:
	docker run --rm -it \
		-e GP2040_BOARDCONFIG=$(BOARD) \
		-e SKIP_WEBBUILD=$(SKIP_WEBBUILD) \
		-v "$(CURDIR):/workspace" -w "/workspace" $(IMAGE) \
		bash -lc 'git config --global --add safe.directory "*" || true; \
			git submodule sync --recursive || true; \
			git submodule update --init --recursive || true; \
			rm -rf build && cmake -S . -B build $(CM_EXTRAS)'

# Build the project
docker-build-project:
	docker run --rm -it \
		-e GP2040_BOARDCONFIG=$(BOARD) \
		-e SKIP_WEBBUILD=$(SKIP_WEBBUILD) \
		-v "$(CURDIR):/workspace" -w "/workspace" $(IMAGE) \
		bash -lc 'git config --global --add safe.directory "*" || true; cmake --build build -j && cp build/*.uf2 . || true'

# Copy UF2 artifacts from build/ into repo root without rebuilding
.PHONY: docker-copy-uf2
docker-copy-uf2:
	docker run --rm -it -v "$(CURDIR):/workspace" -w "/workspace" $(IMAGE) \
		bash -lc 'cp build/*.uf2 . || true && ls -lh ./*.uf2'

# Clean build cache artifacts only
docker-clean:
	docker run --rm -it -v "$(CURDIR):/workspace" -w "/workspace" $(IMAGE) bash -lc 'rm -rf build/*.ninja build/CMakeCache.txt || true'

# Remove full build directory
clean:
	rm -rf build

# === CI-like helpers ===
# Build web UI first (mirrors node.js.yml)
docker-www:
	docker run --rm -it \
		-v "$(CURDIR):/workspace" -w "/workspace/www" $(IMAGE) \
		bash -lc 'npm ci && CI=false npm run build'

# Configure Release build with SKIP_WEBBUILD=TRUE (mirrors cmake.yml)
docker-configure-release:
	docker run --rm -it \
		-e GP2040_BOARDCONFIG=$(BOARD) \
		-e SKIP_WEBBUILD=TRUE \
		-v "$(CURDIR):/workspace" -w "/workspace" $(IMAGE) \
		bash -lc 'git config --global --add safe.directory "*" || true; \
			git submodule sync --recursive || true; \
			git submodule update --init --recursive || true; \
			rm -rf build && cmake -S . -B build -D CMAKE_BUILD_TYPE=Release -D SKIP_PICO_SDK_VERSION_CHECK=ON'
