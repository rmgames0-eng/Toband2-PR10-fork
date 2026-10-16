# Build and play location

The user requires one stable play folder. Run `python build-mingw.py` to update
`build/party/TOband.exe` and its shipped `lib` data. Do not create task-specific
play folders such as `build/brunhild`. Keep saves and personal settings intact.
The C:/Users/rmgam/Documents/Codex junction points to H:/Codex, so both paths
refer to the same checkout and executable. Explicit temporary build directories
remain available for isolated packaging or tests, not as a new play location.
