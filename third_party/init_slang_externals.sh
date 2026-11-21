#!/usr/bin/env bash
# Initialize only the Slang nested externals required for tvox's slim Metal build.
# Do NOT run `git submodule update --init --recursive` on third_party/slang.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SLANG="$ROOT/third_party/slang"

if [[ ! -f "$SLANG/CMakeLists.txt" ]]; then
  echo "error: third_party/slang is missing; add the slang submodule first" >&2
  exit 1
fi

required=(
  external/unordered_dense
  external/miniz
  external/lz4
  external/cmark
  external/vulkan
  external/metal-cpp
  external/fast_float
  external/lua
  external/spirv-headers
)

cd "$SLANG"
git submodule update --init --depth 1 "${required[@]}"
echo "Initialized ${#required[@]} slang externals under third_party/slang/external/"
