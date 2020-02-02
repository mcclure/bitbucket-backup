(* This file contains an routine that runs an emily repl off stdin. Once entered, it runs until quit. *)
(* In future, this could possibly be moved into its own standalone executable. *)

(* Predefined Emily code used by the REPL *)
let replHelpString = Options.fullVersion^{|, interactive mode
Type "help" for help, "quit" to quit
Type "last" to get the previous line's value|}

(* Check if the string 's' ends with a backslash. *)
(* FIXME: This is inadequate, the tokenizer should report this itself. *)
let isContinued s =
    let n = String.length s in
    n > 0 && (String.get s (n - 1)) == '\\'

let lastKeyString = "last"

(* Runs the REPL.

This is the high-level entry point that Main uses to run the REPL.
It needs to set up a global scope to use, then execute the
files provided as arguments (if any) and then start reading
input from the user.

Control-D (EOF) exits the REPL, as does evaluating the word `quit`. *)

let repl target =

    (* This is our global mutable REPL scope. Prepopulate it with some stuff: *)
    let starter = Loader.completeStarter Loader.Cwd in
    let scopeTable = Value.(tableFrom starter.rootScope) in

    (* Line and lines are used to read and execute user input *)
    let line = ref "" in
    let lines = ref [] in

    (* Display a prompt, then read a line from the user *)
    let promptAndReadLine s =
        print_string s;
        flush stdout;
        line := input_line stdin;
        lines := !line :: !lines in

    (* Tokenize and execute the given file target *)
    let runFile f =
        let buf = (Tokenize.tokenizeChannel (Token.File f) (open_in f)) in
        Execute.execute starter buf in

    let runString s =
        let buf = Tokenize.tokenizeString Token.Cmdline s in
        Execute.execute starter buf in

    (* Run file or -e target *)
    let runTargetFile t =
        match t with
            | Options.File f -> ignore @@ runFile f
            | Options.Literal s -> ignore @@ runString s
            | Options.Stdin -> failwith "Can't take input from stdin and run in interactive mode at once."
    in

    (* Load any files provided by the user, before launching REPL *)
    let runUserFiles () =
        match target with None -> () | Some t -> runTargetFile t in

    let rec promptAndReadBuffer () = (
        (* Print a prompt, take a line of text, push to "lines" stack *)
        promptAndReadLine (match !lines with [] -> ">>> " | _ -> "..> ");

        (* Turn the line stack into a "program" *)
        let combinedString = (String.concat "\n" (List.rev !lines)) in

        try
            (* Attempt to parse the program and return an AST. *)
            Tokenize.tokenizeString Token.Cmdline combinedString
        with
            (* The program is invalid, but could be valid with more text. Read another line. *)
            Token.CompilationError(Token.IncompleteError,_,_) -> promptAndReadBuffer() ) in

    (* First, set up the repl environment. *)

    (* This function will be run if the user evaluates the symbol `help` *)
    Value.tableSetString scopeTable "help" (Value.BuiltinUnaryMethodValue (fun _ ->
        print_endline replHelpString;
        raise Sys.Break (* Stop executing code. Is this too aggressive? *)
    ));
    (* This function will be run if the user evaluates the symbol `quit` *)
    Value.tableSetString scopeTable "quit" (Value.BuiltinUnaryMethodValue (fun _ ->
        raise End_of_file (* ANY attempt to read the "quit" variable quits immediately *)
    ));
    Value.tableSetString scopeTable lastKeyString Value.Null;

    (* Next print initial help scroll *)
    print_endline (replHelpString ^ "\n");

    (* Then run any files provided as arguments *)
    runUserFiles ();

    (* Intercept Control-C so it doesn't kill the REPL. *)
    Sys.catch_break true;

    try
        (* As long as the user hasn't sent EOF (Control-D), read input *)
        while true do
            (try
                (* Read a full program (throws if program invalid) *)
                let buf = promptAndReadBuffer() in

                (* Evaluate program in repl-shared scope *)
                let result = Execute.execute starter buf in

                (* Store final result for next time *)
                Value.tableSetString scopeTable lastKeyString result;

                (* Pretty-print final result *)
                print_endline @@ (Pretty.replDisplay result true)

            with
                | Sys.Break -> (* Control-C should clear the line, draw a new prompt *)
                    print_endline ""
                | Token.CompilationError e ->
                    print_endline @@ "Error parsing:\n" ^ (Token.errorString e)
                | Failure e ->
                    print_endline @@ "Error executing:\n" ^ e
            );

            (* Flush stdout so any output is immediately visible *)
            flush stdout;

            (* Empty lines, since they have all been executed, and repeat *)
            lines := [];
        done

    with End_of_file ->
        (* Time to exit the REPL *)
        print_endline "Done"
