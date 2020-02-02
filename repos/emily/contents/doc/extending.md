# Extending Emily

If you want to try to make your own changes to Emily: All the source files are in `src/`. You should probably read [style.md](style.md).

All the files under `src/` have a comment at the top explaining what they do. The main thrust of implementing the language however passes through the files `tokenize.ml`, `macro.ml`, `execute.ml`. The command line interface is provided by `options.ml` and `main.ml`.

At the moment, if you add something to the language, you probably want to do it by adding things to one of the base prototypes, probably the scope prototype. Notice all the files with names starting with "builtin", notably `builtinScope.ml`. If you want to add something, and it's not just a pure value, most often you will want that thing to be constructed with `ValueUtil.snippetClosure` (which makes a function value out of an ocaml function) or `ValueUtil.snippetTextClosure` (which makes a function value out of a string containing an Emily program). If you want to add a "method" to a prototype (i.e., a value which on access or execution has awareness of the object from which it was extracted) see `Value.BuiltinMethodValue` or `BuiltinUnaryMethodValue`.

If you want to add a new operator to the language, you will want to do this by adding to the big `builtinMacros` table at the end of `macro.ml`. The manual explains some how the macros work.