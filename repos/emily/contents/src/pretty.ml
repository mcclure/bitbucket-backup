(* Pretty-printers for types from various other files *)

(* --- Code printers --- *)

(* "Disassemble" a token tree into a human-readable string (specializable) *)
let rec dumpCodeTreeGeneral groupPrinter token =
    match token.Token.contents with
    (* For a simple (nongrouping) token, return a string for just the item *)
    | Token.Word x | Token.Symbol x -> x
    | Token.String x -> "\"" ^ x ^ "\""
    | Token.Atom x -> "." ^ x
    | Token.Number x -> string_of_float x
    | Token.Group {Token.kind=kind; closure=closure; items=items} ->
        let l, r = match kind with
            | Token.Plain -> "(", ")"
            | Token.Scoped -> "{", "}"
            | Token.Box _ -> "[", "]"
        in let l = (match closure with
            | Token.NonClosure -> ""
            | Token.ClosureWithBinding (_,[]) -> "^"
            | Token.ClosureWithBinding (_,binding) -> "^" ^ (String.concat " " binding)) ^ l
        (* GroupPrinter is an argument function which takes the left group symbol, right group
           symbol, and group contents, and decides how to format them all. *)
        in groupPrinter token l r items

(* "Disassemble" a token tree into a human-readable string (specialized for looking like code) *)
let dumpCodeTreeTerse token =
    let rec groupPrinter token l r items =
        l ^ ( String.concat "; " (
                    let eachline x = String.concat " " ( List.map (dumpCodeTreeGeneral groupPrinter) x )
                    in List.map eachline items;
        ) ) ^ r
    in dumpCodeTreeGeneral groupPrinter token

(* "Disassemble" a token tree into a human-readable string (specialized to show token positions) *)
let dumpCodeTreeDense token =
    let rec oneToken x = Printf.sprintf "%s %s" (Token.positionString x.Token.at) (dumpCodeTreeGeneral groupPrinter x)
    and groupPrinter token l r items =
        l ^ "\n" ^ ( String.concat "\n" (
                    let eachline x = String.concat "\n" ( List.map oneToken x )
                    in List.map eachline items;
        ) ) ^ "\n" ^ r
    in dumpCodeTreeGeneral groupPrinter token

(* --- Value printers --- *)

(* Re-escape string according to the Emily reader's rules *)
let escapeString s =
    let sb = Buffer.create (String.length s + 2) in
    let escapeChar c = match c with
        | '"' | '\\' -> Buffer.add_char sb '\\'; Buffer.add_char sb c
        | '\n' -> Buffer.add_string sb "\\n"
        | _ -> Buffer.add_char sb c in
    Buffer.add_char sb '"';
    String.iter escapeChar s;
    Buffer.add_char sb '"';
    Buffer.contents sb

let angleWrap s = "<" ^ s ^ ">"

let idStringForTable t =
    match Value.tableGet t Value.idKey with
        | None -> "UNKNOWN"
        | Some Value.FloatValue v -> string_of_int @@ int_of_float v
        | _ -> "INVALID" (* Should be impossible *)
let idStringForValue v = match v with
    | Value.TableValue t | Value.ObjectValue t -> idStringForTable t
    | _ -> "UNTABLE"

let dumpValueTreeGeneral wrapper v =
    match v with
        | Value.Null -> "<null>"
        | Value.True -> "<true>"
        | Value.FloatValue v -> string_of_float v
        | Value.StringValue s -> escapeString s
        | Value.AtomValue s -> "." ^ s
        | Value.UserMethodValue _ -> "<object-method>"
        | Value.BuiltinFunctionValue _ -> "<builtin>"
        | Value.BuiltinMethodValue _ -> "<object-builtin>"
        | Value.BuiltinUnaryMethodValue _ -> "<property-builtin>"
        | Value.BuiltinHandoffValue _ -> "<special-builtin>"
        | Value.ClosureValue {Value.exec=e; Value.needArgs=n} ->
            let tag = match e with Value.ClosureExecUser _ -> "closure" | Value.ClosureExecBuiltin _ -> "closure-builtin" in
             "<" ^ tag ^ "/" ^ string_of_int(n) ^">"
        | Value.TableValue     _ -> wrapper "scope" v (* From the user's perspective, a table is a scope *)
        | Value.ObjectValue    _ -> wrapper "object" v
        | Value.ContinuationValue _ -> "<return>"

let simpleWrapper label obj = angleWrap label

let labelWrapper label obj = match obj with
    | Value.TableValue t | Value.ObjectValue t -> angleWrap @@ label ^ ":" ^ (idStringForTable t)
    | _ -> angleWrap label

let dumpValue v =
    let wrapper = if Options.(run.trackObjects) then labelWrapper else simpleWrapper in
    dumpValueTreeGeneral wrapper v

(* FIXME: The formatting here is not even a little bit generalized. *)
let dumpValueUnwrappedTable t = " = [\n            " ^
    (String.concat "\n            " (List.map (function
        (v1, v2) -> dumpValue(v1) ^ " = " ^ dumpValue(v2)
    ) (CCHashtbl.to_list t) ) ) ^ "\n        ]"

let dumpValueTable v =
    dumpValue (v) ^ match v with
        | Value.TableValue t | Value.ObjectValue t -> dumpValueUnwrappedTable t
        | _ -> ""

let dumpValueNewTable v =
    (if Options.(run.traceSet) then dumpValueTable else dumpValue) v

(* Normal "print" uses this *)
let dumpValueForUser v =
    match v with
        | Value.StringValue s -> s
        | Value.AtomValue s -> s
        | _ -> dumpValue v

(* --- repl.ml helper printers --- *)

(* FIXME: Can all these various display functions be condensed at all? *)
(* Also, shouldn't more of this be exposed to code? *)

(* display numbers: 4.0 -> "4", 4.1 -> "4.1", etc. *)
let displayNumber n =
    let s = string_of_float n in
    let n = String.length s in
    match s.[n - 1] with
    | '.' -> String.sub s 0 (n - 1)
    | _ -> s

(* Should the REPL should show a key/value pair? if not hide it. *)
let shouldShowItem (k, _) =
    not @@ List.exists (fun x -> x = k)
        [Value.parentKey; Value.hasKey; Value.setKey; Value.letKey]

(* Sort items in objects/tables by key name *)
let sortItems (k1, v1) (k2, v2) =
    match (k1, k2) with
    | (Value.AtomValue s1, Value.AtomValue s2) -> String.compare s1 s2
    | (Value.AtomValue _, _) -> 1
    | (_, Value.AtomValue _) -> -1
    | (Value.FloatValue n1, Value.FloatValue n2) ->
       if n1 < n2 then -1 else if n1 > n2 then 1 else 0
    | _ -> 0

(* Display the key atom -- special-cased to avoid using a dot, since defns don't use them *)
let displayKey k =
    match k with
    | Value.AtomValue s -> s
    | Value.FloatValue n -> "<" ^ displayNumber(n) ^ ">"
    | _ -> "<error>"

(* (Optionally) truncate a string and append a suffix *)
let truncate s limitAt reduceTo suffix =
    if (String.length s) > limitAt then (String.sub s 0 reduceTo) ^ suffix else s

(* Provide a compact view of tables/objects for the REPL *)
let rec displayTable t =
    let items = List.sort sortItems (CCHashtbl.to_list t) in
    let ordered = List.filter shouldShowItem items in
    let f (k, v) = (displayKey k) ^ " = " ^ (replDisplay v false) in
    let out = match (List.map f ordered) with
        | [] -> "[...]"
        | toks -> "[" ^ (String.concat "; " toks) ^ "; ...]" in
    truncate out 74 72 "...]"

(* Create a string representation of a value for the REPL *)
and replDisplay value recurse =
    match value with
        | Value.Null -> "null"
        | Value.True -> "true"
        | Value.FloatValue n -> displayNumber n
        | Value.StringValue s -> escapeString s
        | Value.AtomValue s -> "." ^ s
        | Value.TableValue t | Value.ObjectValue t ->
            if recurse then displayTable t else "<object>"
        | _ -> dumpValueForUser value
