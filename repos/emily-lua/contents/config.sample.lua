config = {
	-- Which version of LLVM are you using? (Use underscores instead of periods.)
	llvmVersion = "3_8",

	-- What is the path in which the libLLVM dynamic library is installed?
	-- Not needed if libLLVM is in the library path.
	-- llvmLibPath = "/opt/local/libexec/llvm-3.8/lib/libLLVM.dylib",

	-- For what platform should the program be compiled?
	-- Leave blank for default, or try typing `clang -version` on your system to get this
	-- compileTriple = "x86_64-apple-darwin",

    -- What microarchitecture should the compiler target? Suggest leave blank for default.
	-- compileCpu = "core-i7",

	-- What compiler features should be enabled? Suggest leave blank for default.
	-- compileFeatures = "",
}
