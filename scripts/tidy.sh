#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

if ! command -v clang-tidy >/dev/null 2>&1; then
  echo "clang-tidy is required" >&2
  exit 127
fi

if [[ ! -f build/compile_commands.json ]]; then
  echo "build/compile_commands.json is missing; configure the project first" >&2
  exit 1
fi

files=()
has_header=false
for file in "$@"; do
  case "$file" in
    *.h|*.hh|*.hpp|*.hxx) has_header=true ;;
    *.c|*.cc|*.cpp|*.cxx|*.m|*.mm) files+=("$file") ;;
    *) echo "unsupported clang-tidy input: $file" >&2; exit 2 ;;
  esac
done

if ((${#files[@]} == 0)) || [[ "$has_header" == true ]]; then
  files=()
  while IFS= read -r -d '' file; do
    files+=("$file")
  done < <(find src -type f \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' -o -name '*.cxx' \
    -o -name '*.m' -o -name '*.mm' \) -print0)
fi

if ((${#files[@]} == 0)); then
  echo "no translation units found" >&2
  exit 1
fi

clang-tidy -p build \
  --checks='-*,modernize-use-constexpr,readability-identifier-naming' \
  --header-filter="^$repo_root/src/.*" "${files[@]}"
