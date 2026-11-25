#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

if ! command -v clang-format >/dev/null 2>&1; then
  echo "clang-format is required" >&2
  exit 127
fi

files=("$@")
if ((${#files[@]} == 0)); then
  while IFS= read -r -d '' file; do
    files+=("$file")
  done < <(find src -type f \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' -o -name '*.cxx' \
    -o -name '*.h' -o -name '*.hh' -o -name '*.hpp' -o -name '*.hxx' -o -name '*.m' \
    -o -name '*.mm' \) -print0)
fi

if ((${#files[@]} == 0)); then
  echo "no C/C++ source files found" >&2
  exit 1
fi

clang-format -i -- "${files[@]}"
