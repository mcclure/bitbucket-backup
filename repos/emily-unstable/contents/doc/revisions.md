# Release history

## Version 0.2 -- April 11, 2015

Changes:

- Interactive mode
- Now possible to load code from disk, using `package` (stdlib), `project` (local) or `directory` (file-relative) objects. Stdlib directory is currently empty by default. Command line/environment variable changes to support this.
- `private` object visible in loaded package files or inside object literals.
- Build uses `install/` usr-alike tree instead of flat `package/` directory.
- Tests use organized directory tree instead of flat `sample/` directory.
- Error messages reported more sensibly all-around.
- `regression.py` improvements: `Arg:`, `Env:`, `Omit file:` test directives; command line options for standalone use; script now returns error code.
- `relocate.py` (script for easy reorganization of test directories) added.
- More builtin objects support `.eq` properly.
- `.and`, `.or`, `.xor` removed, added `add`, `or`, `xor` short-circuiting functions, `&&`, `||`, `%%` grouping operators also added.
- Illegal close-parenthesis usage is now properly disallowed.

## Version 0.1 -- January 18, 2015

Initial release.

*Received two documentation updates.*