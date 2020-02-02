(* Loads a program and runs it, based on contents of Options. *)

let () =
    let processOne target =
        let buf = match target with
            | Options.File f -> Tokenize.tokenizeChannel (Token.File f) (open_in f)
            | Options.Stdin -> Tokenize.tokenizeChannel Token.Stdin stdin
            | Options.Literal s -> Tokenize.tokenizeString Token.Cmdline s
        in let location = match target with
            | Options.File f -> Loader.locationAround f
            | _ -> Loader.Cwd
        in
        (*  *)
        if Options.(run.disassemble) then print_endline (Pretty.dumpCodeTreeTerse buf) else
        if Options.(run.disassembleVerbose) then print_endline (Pretty.dumpCodeTreeDense buf) else
        if Options.(run.printPackage) then print_endline @@ Loader.packageRootPath () else
        if Options.(run.printProject) then print_endline @@ Loader.projectPathForLocation location else
        ignore @@ Loader.executeProgramFrom location buf
    in
    if Options.(run.repl) then
        ( if%const [%getenv "BUILD_INCLUDE_REPL"] <> "" then Repl.repl Options.(run.target) )
    else
        try
            processOne
                (* FIXME: This is maybe awkward? It is here so printPackage can work without a target. *)
                (* It works by assuming an implicit -e '', which is only safe if we assume *)
                (* option.ml would have failed already if that weren't ok. *)
                (match Options.(run.target) with None -> Options.Literal "" | Some t -> t);

            (* In the standalone version, it appears this happens automatically on exit. *)
            (* In the C-embed version, it does *not*, so call it here. *)
            flush_all ()
        with
            | Token.CompilationError e ->
                prerr_endline @@ Token.errorString e; exit 1
            | Failure e ->
                prerr_endline @@ e; exit 1
