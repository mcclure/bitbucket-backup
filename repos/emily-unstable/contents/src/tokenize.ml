(* Functions to create ASTs from strings:

   This is Emily's "parser", which I'm calling a tokenizer since it doesn't do much.
   It scans left to right, categorizing tokens, pushing on left parens and popping on right.
   The parts we'd think of a "parser" as usually doing will be handled in a second,
   currently unimplemented, macro-processing step. *)

(* Tokenize uses sedlex which is inherently stateful, so tokenize for a single source string is stateful.
   This is the basic state for a file parse-- it basically just records the position of the last seen newline. *)
type tokenizeState = {
    mutable lineStart: int;
    mutable line: int
}

type groupCloseToken = Eof | Char of string
type groupCloseRecord = groupCloseToken*Token.codePosition

let groupCloseHumanReadable kind = match kind with
    | Eof -> "end of file"
    | Char s -> "\""^s^"\""

(* Entry point to tokenize, takes a filename and a lexbuf *)
(* TODO: Somehow strip blank lines? *)
let tokenize enclosingKind name buf : Token.token =
    (* -- Helper regexps -- *)
    let digit = [%sedlex.regexp? '0'..'9'] in
    let number = [%sedlex.regexp? Plus digit] in
    let octalDigit = [%sedlex.regexp? '0'..'7'] in
    let octalNumber = [%sedlex.regexp? "0o", Plus octalDigit] in
    let hexDigit = [%sedlex.regexp? '0'..'9'|'a'..'f'|'A'..'F'] in
    let hexNumber = [%sedlex.regexp? "0x", Plus hexDigit] in
    let binaryNumber = [%sedlex.regexp? "0b", Plus ('0'|'1') ] in
    let letterPattern = [%sedlex.regexp? 'a'..'z'|'A'..'Z'] in (* TODO: should be "alphabetic" *)
    let wordPattern = [%sedlex.regexp? letterPattern, Star ('A'..'Z' | 'a'..'z' | digit) ] in
    let sciNotation = [%sedlex.regexp? ('e'|'E'), Opt('+'|'-'), number ] in
    let floatPattern = [%sedlex.regexp? '.',number | number, Opt('.', number), Opt sciNotation] in
    let numberPattern = [%sedlex.regexp? hexNumber | octalNumber | floatPattern | binaryNumber] in

    (* Helper function: We treat a list as a stack by appending elements to the beginning,
       but this means we have to do a reverse operation to seal the stack at the end. *)
    let cleanup = List.rev in

    (* Individual lines get a more special cleanup so macro processing can occur *)
    let completeLine l =
        if Options.(run.stepMacro) then print_endline @@ "-- Macro processing for "^(Token.fileNameString name);
        Macro.process @@ cleanup l in

    (* -- State management machinery -- *)
    (* Current tokenizer state *)
    let state = {lineStart=0; line=1} in
    (* Call when the current selected sedlex match is a newline. Mutates tokenizer state. *)
    let stateNewline () = state.lineStart <- Sedlexing.lexeme_end buf; state.line <- state.line + 1 in
    (* Use tokenizer state to translate sedlex position into a codePosition *)
    let currentPosition () = Token.{fileName=name; lineNumber=state.line; lineOffset = Sedlexing.lexeme_end buf-state.lineStart} in
    (* Parse failure. Include current position string. *)
    let parseFail mesg = Token.failAt (currentPosition()) mesg in
    let incompleteFail mesg = Token.incompleteAt (currentPosition()) mesg in

    (* -- Parsers -- *)

    (* Sub-parser: double-quoted strings. Call after seeing opening quote mark *)
    let rec quotedString () =
        (* This parser works by statefully adding chars to a string buffer *)
        let accum = Buffer.create 1 in
        (* Helper function adds a literal string to the buffer *)
        let add = Buffer.add_string accum in
        (* Helper function adds a sedlex match to the buffer *)
        let addBuf() = add (Sedlexing.Utf8.lexeme buf) in
        (* Operate *)
        let rec proceed () =
            (* Call after seeing a backslash. Matches one character, returns string escape corresponds to. *)
            let escapedChar () =
                match%sedlex buf with
                    | '\\' -> "\\"
                    | '"' -> "\""
                    | 'n'  -> "\n"
                    | _ -> parseFail "Unrecognized escape sequence" (* TODO: devour newlines *)
            (* Chew through until quoted string ends *)
            in match%sedlex buf with
                (* Treat a newline like any other char, but since sedlex doesn't track lines, we have to record it *)
                | '\n' -> stateNewline(); addBuf(); proceed()
                (* Backslash found, trigger escape handler *)
                | '\\' -> add (escapedChar()); proceed()
                (* Unescaped quote found, we are done. Return the data from the buffer. *)
                | '"'  -> Buffer.contents accum
                (* User probably did not intend for string to run to EOF. *)
                | eof -> incompleteFail "Reached end of file inside string. Missing quote?"
                (* Any normal character add to the buffer and proceed. *)
                | any  -> add (Sedlexing.Utf8.lexeme buf); proceed()
                | _ -> parseFail "Internal failure: Reached impossible place"
        in proceed()

    (* Sub-parser: backslash. Eat up to, and possibly including, newline *)
    in let rec escape seenText =
        let backtrack() = let _ = Sedlexing.backtrack buf in ()
        in match%sedlex buf with
            (* Have reached newline. We're done. If no command was issued, eat newline. *)
            | '\n' -> if seenText then backtrack() else stateNewline()
            (* Skip over white space until newline is reached *)
            | white_space -> escape seenText
            (* A second backslash? Okay, back out and let the main loop handle it *)
            | '\\' -> backtrack()
            (* Comments are allowed after a backslash, and ignored as normal. *)
            | '#', Star (Compl '\n') -> escape seenText
            (* User probably did not intend to concatenate with blank line. *)
            | eof -> if seenText then backtrack() else
                incompleteFail "Found EOF immediately after backslash, expected token or new line."
            (* TODO: Ignore rather than error *)
            | any -> parseFail "Did not recognize text after backslash."
            | _ -> parseFail "Internal failure: Reached impossible place"

    (* Main loop. *)
    (* Takes a constructor that's prepped with all the properties for the enclosing group, and
       needs only the final list of lines to produce a token. Returns a completed group. *)
    in let rec proceed groupClose (groupSeed : Token.token list -> Token.codeSequence -> Token.token) groupInitializer lines line =
        (* Constructor for a token with the current preserved codeposition. *)
        let makeTokenHere = Token.makeToken (currentPosition()) in

        (* Right now all group closers are treated as equivalent. TODO: Don't do it like this. *)
        let closePattern = [%sedlex.regexp? '}' | ')' | ']' | eof] in

        (* Recurse with the same groupSeed we started with. *)
        let proceedWithInitializer = proceed groupClose groupSeed in

        (* Recurse with the same groupSeed and initializer we started with. *)
        let proceedWithLines = proceedWithInitializer groupInitializer in

        (* Recurse with the same groupSeed, initializer and lines we started with. *)
        let proceedWithLine =  proceedWithLines lines in

        (* Recurse with all the same arguments we started with. *)
        let skip () = proceedWithLine line in

        (* Helper function: Get current sedlex match *)
        let matchedLexemes () = Sedlexing.Utf8.lexeme(buf) in

        (* Complete current line & push onto current codeSequence *)
        let linesPlusLine () = completeLine line :: lines in

        (* Recurse with the groupSeed and lines we started with, & the argument pushed onto the current line *)
        (* Notice stateNewLine and addToLineProceed are using ndifferent notions of a "line". *)
        let addToLineProceed x = proceedWithLine (x :: line) in

        (* Recurse with the groupSeed we started with, the current line pushed onto the codeSequence, & a new blank line *)
        let newLineProceed x = proceedWithLines (linesPlusLine()) [] in

        (* Complete processing the current group by completing the current codeSequence & feeding it to the groupSeed. *)
        let closeGroup () = groupSeed groupInitializer ( cleanup (linesPlusLine()) ) in

        (* Helper: Given a string->tokenContents mapping, make the token, add it to the line and recurse *)
        let addSingle constructor = addToLineProceed(makeTokenHere(constructor(matchedLexemes()))) in

        (* Helper: Function-ized Symbol constructor *)
        let makeSymbol x = Token.Symbol x in

        (* Helper: See if lines contains anything substantial *)
        let rec anyNonblank = function
            | [] -> false
            | []::more -> anyNonblank more
            | _ -> true
        in

        let groupCloseMakeFrom str =
            let char = match str with
                | "{" -> "}"
                | "(" -> ")"
                | "[" -> "]"
                | _ -> parseFail "Internal failure: interpreter is confused about parenthesis"
            in (Char char, currentPosition())

        in let groupCloseUnderToken () = match matchedLexemes () with | "" -> Eof | s -> Char s

        (* Recurse with blank code, and a new groupSeed described by the arguments *)
        in let rec openGroup closureKind groupKind =
            proceed (groupCloseMakeFrom @@ matchedLexemes())
                (Token.makeGroup (currentPosition()) closureKind groupKind) [] [] []

        (* Variant assuming non-closure *)
        in let openOrdinaryGroup = openGroup Token.NonClosure

        (* Now finally here's the actual grammar... *)
        in match%sedlex buf with
            (* Ignore comments *)
            | '#', Star (Compl '\n') -> skip ()

            (* On ANY group-close symbol, we end the current group *)
            | closePattern -> (
                (* However, we have to check to make sure that the symbol matches *)
                let candidateClose = groupCloseUnderToken() in
                (* The expected group close comes packed with the position of the opening symbol *)
                let groupClose,groupCloseAt = groupClose in
                (* This is a matching close *)
                if candidateClose = groupClose then closeGroup ()
                (* This is not a matching close *)
                else match candidateClose with
                    (* No close before EOF: Failure is positioned at the opening symbol *)
                    | Eof -> Token.incompleteAt groupCloseAt
                        ("Did not find matching "^(groupCloseHumanReadable groupClose)^" anywhere before end of file. Opening symbol:")
                    (* Close present, but wrong: Failure is positioned at closing symbol *)
                    | Char _ -> parseFail @@
                        "Expected closing "^(groupCloseHumanReadable groupClose)^" but instead found "^(groupCloseHumanReadable candidateClose)
                )

            (* Quoted string *)
            | '"' -> addToLineProceed(makeTokenHere(Token.String(quotedString())))

            (* Floating point number
               More complicated because float_of_string doesn't handle octal or binary *)
            | numberPattern -> addSingle (fun x ->
                               if (String.length x) < 3 then
                                 Token.Number(float_of_string x)
                               else match (String.sub x 0 2) with
                                      "0b" | "0o" -> Token.Number(float_of_int (int_of_string x))
                                      | _         -> Token.Number(float_of_string x))


            (* Local scope variable *)
            | wordPattern -> addSingle (fun x -> Token.Word x)

            (* Line demarcator *)
            | ';' -> newLineProceed()

            (* Line demarcator (but remember, we have to track newlines) *)
            | '\n' -> stateNewline(); newLineProceed()

            (* Reader instructions.
               TODO: A more general system for reader instructions; allow tab after \version *)
            | "\\version 0.1"  -> escape true; skip() (* Ignore to end of line, don't consume *)
            | "\\version 0.2"  -> escape true; skip() (* Ignore to end of line, don't consume *)
            | "\\version 0.3b"  -> escape true; skip() (* Ignore to end of line, don't consume *)
            | '\\' -> escape false; skip()            (* Ignore to end of line and consume it *)

            (* Ignore whitespace *)
            | white_space -> skip ()

            (* On groups or closures, open a new parser (NO TCO AT THIS TIME) and add its result token to the current line *)
            | '(' -> addToLineProceed( openOrdinaryGroup Token.Plain )
            | '{' -> addToLineProceed( openOrdinaryGroup Token.Scoped )
            | '[' -> addToLineProceed( openOrdinaryGroup @@ Token.Box Token.NewObject )

            (* If a | is seen, this demarcates the initializer *)
            | '|' -> if anyNonblank lines
                then addSingle makeSymbol (* If we've passed our chance for an initializer, this is just a pipe *)
                else proceedWithInitializer ( (* Otherwise it's an initializer *)
                    if Options.(run.stepMacro) then print_endline @@ "-- Macro processing for initializer in "^(Token.fileNameString name);
                    Macro.process @@ cleanup line
                ) lines [] (* Otherwise swap line into the initializer spot *)

            | Plus(Compl(Chars "#()[]{}\\;\""|digit|letterPattern|white_space))
                -> addSingle makeSymbol
            | _ -> parseFail "Unexpected character" (* Probably not possible? *)

    (* When first entering the parser, treat the entire program as implicitly being surrounded by parenthesis *)
    in proceed (Eof,currentPosition()) (Token.makeGroup (currentPosition()) Token.NonClosure enclosingKind) [] [] []

(* Tokenize entry point typed to channel *)
let tokenizeChannel source channel =
    let lexbuf = Sedlexing.Utf8.from_channel channel in
    tokenize Token.Plain source lexbuf

(* Tokenize entry point typed to string *)
let tokenizeString source str =
    let lexbuf = Sedlexing.Utf8.from_string str in
    tokenize Token.Plain source lexbuf

let unwrap token = match token.Token.contents with
    | Token.Group g -> g.Token.items
    | _ -> failwith(Printf.sprintf "%s %s" "Internal error: Object in wrong place" (Token.positionString token.Token.at))

let snippet source str =
    try
        unwrap @@ tokenizeString source str
    with
        Token.CompilationError e -> failwith @@
            "Internal error: Interpreter-internal code is invalid:" ^
            (Token.errorString e)

