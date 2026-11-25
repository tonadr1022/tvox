#!/usr/bin/env bash
export MTL_DEBUG_LAYER=1
export MTL_SHADER_VALIDATION=1
export MTL_SHADER_VALIDATION=1
export MTL_LOG_TO_STDERR=1
export MTL_HUD_ENABLED=1
export MTL_SHADER_VALIDATION_GLOBAL_MEMORY=1
set -euo pipefail

cmake --build build
./build/src/tvox "$@"
