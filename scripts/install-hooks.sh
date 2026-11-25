#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
if ! command -v pre-commit >/dev/null 2>&1; then
  echo "pre-commit is required; install it to activate the repository hooks" >&2
  exit 127
fi

cd "$repo_root"
pre-commit install
