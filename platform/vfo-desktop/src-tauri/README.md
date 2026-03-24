# src-tauri

Planned Tauri runtime layer for vfo desktop.

Responsibilities:

- spawn `vfo` as a local process with explicit command templates
- stream stdout/stderr lines to the UI in near real time
- expose typed IPC calls only (no free-form shell execution)
- map exit codes into structured UI error envelopes

Non-goals:

- no media-processing rules in Rust
- no profile/scenario decisioning in desktop runtime
