(* Macro processing *)

let failAt at mesg = raise (Token.CompilationError(Token.MacroError, at, mesg))
let failToken at = failAt at.Token.at

(* Last thing we always do is make sure no symbols survive after macro processing. *)
let verifySymbols l =
    List.iter (function
        | {Token.contents=Token.Symbol s;Token.at=at} -> Token.failAt at @@ "Unrecognized symbol "^ s
        | _ -> ()
    ) l;
    l

(* Types for macro processing. *)
type macroPriority = L of float | R of float (* See builtinMacros comment *)
type singleLine = Token.token list

(* Note what a single macro does:
    The macro processor sweeps over a line, keeping a persistent state consisting of
    "past" (tokens behind cursor, reverse order) "present" (token at cursor) and
    "future" (tokens ahead of cursor). A macro replaces all 3 with a new line. *)
type macroFunction = singleLine -> Token.token -> singleLine -> singleLine
type macroSpec  = { priority : macroPriority ; specFunction : macroFunction }
type macroMatch = {
    matchFunction: macroFunction ;
    past : singleLine; present : Token.token; future : singleLine
}

(* The set of loaded macros lives here. *)
let macroTable = Hashtbl.create(1)

(* All manufactured tokens should be made through clone, so that position information is retained *)
let cloneAtom at s = Token.clone at @@ Token.Atom s
let cloneWord at s = Token.clone at @@ Token.Word s
let cloneGroup at = Token.cloneGroup at Token.NonClosure Token.Plain []
(* Note: makes no-return closures *)
let cloneClosure at = Token.cloneGroup at (Token.ClosureWithBinding (false,[])) Token.Plain []

(* Debug method gets to use this. *)
let nullToken = Token.(makeToken {fileName=Unknown;lineNumber=0;lineOffset=0} (Symbol "_"))

(* Macro processing, based on whatever builtinMacros contains *)
let rec process l =
    if Options.(run.stepMacro) then print_endline @@ Pretty.dumpCodeTreeTerse @@ cloneGroup nullToken [l];

    (* Search for macro to process next. Priority None/line None means none known yet. *)
    let rec findIdeal (bestPriority:macroPriority option) bestLine (past:singleLine) present future : macroMatch option =
        (* Iterate cursor *)
        let proceed priority line =
            match future with
                (* Future is empty, so future has iterated to end of line *)
                | [] -> line

                (* Future is nonempty, so move the cursor forward. *)
                | nextToken :: nextFuture -> findIdeal priority line (present::past) nextToken nextFuture

        (* Iterate cursor leaving priority and line unchanged *)
        in let skip() =
            proceed bestPriority bestLine

        (* Investigate token under cursor *)
        in match present.Token.contents with
            (* Words or symbols can currently be triggers for macros. *)
            | Token.Word s | Token.Symbol s ->
                (* Is the current word/symbol a thing in the macro table? *)
                let v = CCHashtbl.get macroTable s in
                (match v with
                    (* It's in the table; now to decide if it's an ideal match. *)
                    | Some {priority;specFunction} ->
                        (* True if this macro is better fit than the current candidate. *)
                        let better = (match bestPriority,priority with
                            (* No matches yet, automatic win. *)
                            | None,_ -> true

                            (* If associativity varies, we can determine winner based on that alone: *)
                            (* Prefer higher priority, but break ties toward left-first macros over right-first ones. *)
                            | Some L(left),R(right) -> left < right
                            | Some R(left),L(right) -> left <= right

                            (* "Process leftmost first": Prefer higher priority, break ties to the left. *)
                            | Some L(left),L(right) -> left < right

                            (* "Process rightmost first": Prefer higher priority, break ties to the right. *)
                            | Some R(left),R(right) -> left <= right
                        ) in
                        if better then
                            proceed (Some priority) (Some {past; present; future; matchFunction=specFunction})

                        (* It's a worse match than the current best guess. *)
                        else skip()
                    (* It's not in the table. *)
                    | _ -> skip() )
            (* It's not even a candidate to trigger a macro. *)
            | _ -> skip()

    (* Actually process macro *)
    in match l with
        (* Special case: Line is empty, do nothing. *)
        | [] -> l

        (* Split out first item to use as the findideal "present" cursor. *)
        | present::future ->
            (* Repeatedly run findIdeal until there are no more macros in the line. *)
            (match findIdeal None None [] present future with
                (* No macros triggered! Sanitize the line and return it. *)
                | None -> verifySymbols l

                (* A macro was found. Run the macro, then re-process the adjusted line. *)
                | Some {matchFunction; past; present; future} ->
                    if Options.(run.stepMacro) then print_endline @@ "    ...becomes:";
                    process (matchFunction past present future) )

(* The macros themselves *)

(* Support functions for macros *)

let newFuture at f        = cloneGroup at [process f]        (* Insert a forward-time group *)
let newPast at p          = newFuture at (List.rev p)        (* Insert a reverse-time group *)
let newFutureClosure at f = cloneClosure at [process f]      (* Insert a forward-time closure *)
let newPastClosure at p   = newFutureClosure at (List.rev p) (* Insert a reverse-time closure *)

(* A recurring pattern in the current macros is to insert a new single token
   into "the middle" of an established past and future *)
(* FIXME: Inferring position from "present" will work  *)
let arrangeToken at past present future =
    [ newFuture at @@ List.concat [List.rev past; [present]; future] ]
let arrange at past present future =
    arrangeToken at past (newFuture at present) future

(* Constructors that return working macros *)

(* Given argument "op", make a macro to turn `a b … OP d e …` into `(a b …) .op (d e …)` *)
let makeSplitter atomString : macroFunction = (fun past at future ->
    [ newPast at past ; cloneAtom at atomString ; newFuture at future]
)

(* Given argument "op", make a macro to turn `OP a` into `((a) .op)` *)
let makeUnary atomString : macroFunction = (fun past at future ->
    match future with
        | a :: farFuture ->
            arrange at past [a; cloneAtom at atomString] farFuture
        | _ -> Token.failToken at @@ (Pretty.dumpCodeTreeTerse at) ^ " must be followed by something"
)

(* Given argument "op", make a macro to turn `OP a` into `(op (a))` *)
let makePrefixUnary wordString : macroFunction = (fun past at future ->
    match future with
        | a :: farFuture ->
            arrange at past [cloneWord at wordString; a] farFuture
        | _ -> Token.failToken at @@ (Pretty.dumpCodeTreeTerse at) ^ " must be followed by something"
)

(* Given argument "op", make a macro to turn `a b … OP d e …` into `(op ^(a b …) ^(d e …)` *)
let makeShortCircuit wordString : macroFunction = (fun past at future ->
    [ cloneWord at wordString ; newPastClosure at past ; newFutureClosure at future ]
)

let makeSplitterInvert atomString : macroFunction = (fun past at future ->
    [ cloneWord at "not" ; newFuture at
        [ newPast at past ; cloneAtom at atomString ; newFuture at future]
    ]
)

(* One-off macros *)

(* Ridiculous thing that only for testing the macro system itself. *)
(* Prints what's happening, then deletes itself. *)
let debugOp (past:singleLine) (present:Token.token) (future:singleLine) =
    print_endline @@ "Debug macro:";
    print_endline @@ "\tPast:    " ^ (Pretty.dumpCodeTreeTerse @@ cloneGroup nullToken [List.rev past]);
    print_endline @@ "\tPresent: " ^ (Pretty.dumpCodeTreeTerse @@ present);
    print_endline @@ "\tFuture:  " ^ (Pretty.dumpCodeTreeTerse @@ cloneGroup nullToken [future]);
    List.concat [List.rev past; future]

(* Apply operator-- Works like ocaml @@ or haskell $ *)
let applyRight past at future =
    [ newPast at @@ newFuture at future :: past ]

(* "Apply pair"; works like unlambda backtick *)
let backtick past at future =
    match future with
        | a :: b :: farFuture ->
            arrange at past [a;b] farFuture
        | _ -> Token.failToken at "` must be followed by two symbols"

(* Works like ocaml @@ or haskell $ *)
let question past at future =
    let result colonAt cond a b =
        [cloneWord at "tern";
            newFuture at cond; newFutureClosure at a; newFutureClosure colonAt b]
    in let rec scan a rest =
        match rest with
            | ({Token.contents=Token.Symbol ":"} as colonAt)::moreRest ->
                result colonAt (List.rev past) (List.rev a) moreRest
            | {Token.contents=Token.Symbol "?"}::moreRest ->
                Token.failToken at "Nesting like ? ? : : is not allowed."
            | token::moreRest ->
                scan (token::a) moreRest
            | [] -> Token.failToken at ": expected somewhere to right of ?"
    in scan [] future

(* Works like Perl // *)
let ifndef past at future =
    let target, key =
        match past with
            | ({Token.contents=Token.Word name} as token)::[] -> cloneWord at "scope", cloneAtom token name
            | left::[] -> Token.failToken left "Either a variable name or a field access is expected to left of //"
            | token::moreTokens -> newPast at moreTokens, token
            | [] -> Token.failToken at "Nothing found to left of // operator"
    in [cloneWord at "check"; target; key; newFutureClosure at future]

(* Assignment operator-- semantics are relatively complex. See manual.md. *)
let assignment past at future =
    (* The final parsed assignment will consist of a list of normal assignments
       and a list of ^ variables for a function. Perform that assignment here: *)
    let result lookups bindings =
        (* The token to be eventually assigned is easy to compute early, so do that. *)
        let rightside = match bindings with
            (* No bindings; this is a normal assignment. *)
            | None -> newFuture at future

            (* Bindings exist: This is a function definition. *)
            | Some bindings  -> Token.cloneGroup at (Token.ClosureWithBinding (true,(List.rev bindings)))
                    Token.Plain [] [process future]

        (* Recurse to try again with a different command. *)
        (* TODO: This is all wrong... set should be default, let should be the extension.
           However this will require... something to allow [] to work right. *)
        in let rec resultForCommand cmdAt cmd lookups =

            (* Done with bindings now, just have to figure out what we're assigning to *)
            match (List.rev lookups),cmd with
                (* ...Nothing? *)
                | [],_ -> Token.failToken at "Found a =, but nothing to assign to."

                (* Sorta awkward, detect the "nonlocal" prefix and swap out let. This should be generalized. *)
                | ({Token.contents=Token.Word "nonlocal"} as cmdToken)::moreLookups,"let" -> resultForCommand cmdToken Value.setKeyString moreLookups

                (* Looks like a = b *)
                | [token],_ ->
                    [cloneWord cmdAt cmd;
                        (match token with
                            | {Token.contents=Token.Word name} -> cloneAtom token name
                            | token -> token); (* FIXME: Nothing now prevents assigning to a number in a plain scope? *)
                        rightside]

                (* Looks like a b ... = c *)
                | firstToken::moreLookups,_ ->
                    (match (List.rev moreLookups) with
                        (* Note what's happening here: We're slicing off the FINAL element, first in the reversed list. *)
                        | finalToken::middleLookups ->
                            List.concat [[firstToken]; List.rev middleLookups; [cloneAtom cmdAt cmd; finalToken; rightside]]

                        (* Excluded by [{Token.word}] case above *)
                        | _ -> Token.failToken at "Internal failure: Reached impossible place" )

        in resultForCommand at "let" lookups

    (* Parsing loop, build the lookups and bindings list *)
    in let rec processLeft remainingLeft lookups bindings =
        match remainingLeft,bindings with
            (* If we see a ^, switch to loading bindings *)
            | {Token.contents=Token.Symbol "^"}::moreLeft,None ->
                processLeft moreLeft lookups (Some [])

            (* If we're already loading bindings, just skip it *)
            | {Token.contents=Token.Symbol "^"}::moreLeft,_ ->
                processLeft moreLeft lookups bindings

            (* Sanitize any symbols that aren't cleared for the left side of an = *)
            | {Token.contents=Token.Symbol x} :: _,_ -> failwith @@ "Unexpected symbol "^x^" to left of ="

            (* We're adding bindings *)
            | {Token.contents=Token.Word b} :: restPast,Some bindings ->
                processLeft restPast lookups (Some (b::bindings))

            (* We're adding lookups *)
            | l :: restPast,None ->
                processLeft restPast (l::lookups) None

            (* There is no more past, Jump to result. *)
            | [],_ -> result lookups bindings

            (* Apparently did something like 3 = *)
            | token::_,_ -> Token.failToken token @@ "Don't know what to do with "^(Pretty.dumpCodeTreeTerse token)^" to left of ="

    in processLeft (List.rev past) [] None

(* Constructor for closure constructor, depending on whether return wanted. See manual.md *)
let closureConstruct withReturn =
    fun past at future ->
        (* Scan line picking up bindings until group reached. *)
        let rec openClosure bindings future =
            match future with
                (* If redundant ^s seen, skip them. *)
                | {Token.contents=Token.Symbol "^"} :: moreFuture ->
                    openClosure bindings moreFuture

                (* This is a binding, add to list. *)
                | {Token.contents=Token.Word b} :: moreFuture ->
                    openClosure (b::bindings) moreFuture

                (* This is a group, we are done now. *)
                | {Token.contents=Token.Group {Token.closure=Token.NonClosure;Token.kind;Token.groupInitializer;Token.items}} :: moreFuture ->
                    (match kind with
                        (* They asked for ^[], unsupported. *)
                        | Token.Box _ -> Token.failToken at @@ "Can't use object literal with ^"
                        (* Supported group *)
                        | _ -> arrangeToken at past (Token.cloneGroup at (Token.ClosureWithBinding(withReturn,(List.rev bindings))) kind groupInitializer items) moreFuture
                    )

                (* Found a :, so act like it's a group *)
                (* FIXME: Merge with above? *)
                | {Token.contents=Token.Symbol ":"} :: moreFuture ->
                    arrangeToken at past (Token.cloneGroup at (Token.ClosureWithBinding(withReturn,(List.rev bindings))) Token.Plain [] [process moreFuture]) []

                (* Reached end of line *)
                | [] -> Token.failToken at @@ "Body missing for closure"

                (* Any other symbol *)
                | _ ->  Token.failToken at @@ "Unexpected symbol after ^"

        in openClosure [] future

(* Commas in statement *)
let comma past at future =
    (* Split statement into comma-delimited sections. Generates a reverse list of token reverse-lists *)
    let rec gather accumulateLine accumulateAll future =
        (* A new value for accumulateLine *)
        let pushLine() = (accumulateLine)::accumulateAll in
        match future with
            (* Finished *)
            | [] -> pushLine()
            (* Another comma was found, open a new section *)
            | {Token.contents=Token.Symbol ","}::moreFuture -> gather [] (pushLine()) moreFuture
            (* Add token to current section *)
            | t::moreFuture -> gather (t::accumulateLine) accumulateAll moreFuture
    (* Given reverse list of reverse-list-of-tokens, create a list of this.append statements *)
    in let rec emit accumulateFinal subLines =
        match subLines with
            (* Finished *)
            | [] -> accumulateFinal
            (* Blank line-- ignore it *)
            | []::moreLines -> emit accumulateFinal moreLines
            (* Nonempty line-- wrap TOKENS into this.append(TOKENS); *)
            | (firstToken::moreTokens as tokens)::moreLines -> emit (
                [cloneWord firstToken "this"; cloneAtom firstToken "append"; cloneGroup firstToken [process @@ List.rev tokens]]
            ::accumulateFinal) moreLines
    (* Pull apart past with gather, stitch back together with emit, we now have a list of statements we can turn into a group. *)
    in [cloneGroup at @@ emit [] @@ gather [] [past] future]

(* Atom *)
let atom past at future =
    match future with
        (* Look at next token and nothing else. *)
        | {Token.contents=Token.Word a} :: moreFuture ->
            arrangeToken at past (cloneAtom at a) moreFuture
        | _ -> Token.failToken at "Expected identifier after ."

(* Splitter which performs an unrelated unary operation if nothing to the left. *)
let makeDualModeSplitter unaryAtom binaryAtom : macroFunction = (fun past at future ->
    let prefixUnary = makeUnary unaryAtom in
    let splitter = makeSplitter binaryAtom in
    match past with
        | []
        | {Token.contents=Token.Symbol "*"}::_ (* Because this is intended for unary -, special-case arithmetic. *)
        | {Token.contents=Token.Symbol "/"}::_ (* I don't much like this solution? *)
        | {Token.contents=Token.Symbol "%"}::_
        | {Token.contents=Token.Symbol "-"}::_
        | {Token.contents=Token.Symbol "+"}::_
            -> prefixUnary past at future
        | _  -> splitter past at future
)

(* Just to be as explicit as possible:

   Each macro has a priority number and a direction preference.
   If the priority number is high, the macro is high priority and it gets interpreted first.
   If sweep direction is L, macros are evaluated "leftmost first"  (moving left to right)
   If sweep direction is R, macros are evaluated "rightmost first" (moving right to left)
   If there are multiple macros of the same priority, all the L macros (prefer-left)
   are interpreted first, and all of the R macros (prefer-right) after.
   (I recommend against mixing L and R macros on a single priority.)

   Notice how priority and sweep direction differ from "precedence" and "associativity".
   They're essentially opposites. The later a splitter macro evaluates, the higher
   "precedence" the associated operator will appear to have, because splitter macros
   wrap parenthesis around *everything else*, not around themselves.
   For similar reasons, right-preference likes like left-associativity and vice versa.

   So this table goes:
    - From lowest priority to highest priority.
    - From lowest-magnitude priority number to highest-magnitude priority number.
    - From last-evaluated to earliest-evaluated macro.
    - From closest-binding operators to loosest-binding operators
      (In C language: From "high precedence" to "low precedence" operators)
*)

let builtinMacros = [
    (* Weird grouping *)

    R(20.), "`", backtick;

    (* More boolean *)
    R(30.), "!", makePrefixUnary "not";

    (* Math *)
    R(30.), "~", makeUnary    "negate";
    R(40.), "/", makeSplitter "divide";
    R(40.), "*", makeSplitter "times";
    R(40.), "%", makeSplitter "mod";
    R(50.), "-", makeDualModeSplitter "negate" "minus";
    R(50.), "+", makeSplitter "plus";

    (* Comparators *)
    R(60.), "<", makeSplitter "lt";
    R(60.), "<=", makeSplitter "lte";
    R(60.), ">", makeSplitter "gt";
    R(60.), ">=", makeSplitter "gte";

    R(65.), "==", makeSplitter "eq";
    R(65.), "!=", makeSplitterInvert "eq";

    (* Orelse *)
    L(67.), "//", ifndef;

    (* Boolean *)
    R(70.), "&&", makeShortCircuit "and";
    R(75.), "||", makeShortCircuit "or";
    R(77.), "%%", makeShortCircuit "xor";

    (* Grouping *) (* FIXME: Would these make more sense after assignment? *)
    L(90.), ":", applyRight;
    L(90.), "?", question;

    (* Core *)
    L(100.), "^",  closureConstruct false;
    L(100.), "^@", closureConstruct true;
    L(105.), "=",  assignment;
    L(110.), ".",  atom;

    (* Pseudo-statement *)
    L(150.), ",", comma;
]

(* Populate macro table from builtinMacros. *)
let () =
    List.iter (function
        (priority, key, specFunction) -> Hashtbl.replace macroTable key {priority; specFunction}
    ) builtinMacros
