# Dependency patches

`tools/fetch_deps.sh` applies these to the pinned dependencies in `lib/`.

- `N64ModernRuntime-0001-unload-touching-sections.patch`: `unload_overlays()` treated a
  loaded section that starts exactly where the unloaded range ends as overlapping it,
  then aborted with "Cannot partially unload section". ISS64 packs overlays back to back
  in RAM, so this happened on almost every overlay swap.
