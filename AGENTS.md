# What

New engine: renderer backend/RHI, to be voxel engine with physics etc in the future

## Principles

- Extensible, modular code, no shortcuts
- Each feature needs to be well thought out and clean code

## Structure

src/ -> contains source code
resources/ -> shaders, models, shader_cache, etc.

## Agent checks

- Run `./build_run.sh --frames 200` for a bounded smoke run.
- Run `./scripts/format.sh <files>` and `./scripts/tidy.sh <files>` on edited C/C++ files.
- Install optional pre-commit hooks with `./scripts/install-hooks.sh`; stage formatter changes before committing.
