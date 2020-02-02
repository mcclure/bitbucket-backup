These are the headers from release 3.9-r259341 of LLVM from MacPorts (this is a prerelease version). I am converting the headers piecemeal into .lua files containing ffi.cdef[[ ]] sections. The .h files are unused, but are left here in the repo for comparison. I have not investigated the licensing implications of this.

Conversion consists of:
- Remove #includes and header guards from top and bottom of file
- Wrap file in ffi.cdef
- If any preprocessor directives remain at this point, run gcc -E -P -C -xc-header (run preprocessor, do not emit file markers, do not remove comments, do not complain about file extensions)
	- The preprocessor directives in Target.lua are inherently machine-specific, so this becomes a build step. See top-level README.
