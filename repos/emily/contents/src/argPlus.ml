(* This is a reimplementation of Arg, with the following differences:

    - Arguments may be specified with an =, like, --key=value
    - Rule functions may throw a Complete to succeed while terminating further parsing
    - `Help of string` complemented by `Help of int` (exit code)
    - Provisions for environment variables

Not all features of Arg are implemented, notably most spec types are not supported. *)

(* I use this to turn a single set of rules into simultaneously environment and argument parse rules. See options.ml *)
let keyMutate f = List.map @@ function ((a, b, c) : (Arg.key list * Arg.spec * Arg.doc)) -> (f a, b, c)

let argPlusLimitations who = failwith @@ "Internal error: Called "^who^" with an arg spec it was not designed to handle."

(* Rule methods can raise Arg.Bad, Arg.Help, ArgPlus.Help or ArgPlus.Complete *)
exception Complete    (* Success, stop processing arguments *)
exception Help of int (* Argument is exit code *)

(* Takes the rule list normally given as first argument to Arg.parse and parses env vars against it. *)
let envParse =
    List.iter @@ function
        ( (key, spec, _) : (Arg.key * Arg.spec * Arg.doc) ) ->
            try
                (* Rather than iterating env, iterate the rule list and check for each env we recognize *)
                let value = Unix.getenv key in (* May fail *)
                match spec with
                    (* Discard argument-- does this ever even make sense? *)
                    | Arg.Unit f -> f ()

                    (* String argument *)
                    | Arg.String f -> f value

                    (* Incorrect use of ArgParse *)
                    | _ -> argPlusLimitations "envParse"
            with
                Not_found -> () (* Unix.getenv failed, which means the env var wasn't present. Move on *)

(* Notice one additional argument vs Arg.parse, called after successful completion with unprocessed part of arg list (possibly empty) *)
let argParse rules fallback usage onComplete =
    (* Store all rules keyed on parameter *)
    let lookup : (Arg.key, Arg.spec) Hashtbl.t = Hashtbl.create(1) in
    List.iter (function (key, spec, _) -> Hashtbl.replace lookup key spec) rules;

    (* Function to imperatively stream in each argument, one at a time *)
    let rest : string list ref = ref @@ Array.to_list Sys.argv in
    let consume () = match !rest with | [] -> None | next::more -> (rest := more; Some next) in

    (* Inner loop *)
    let rec proceed () =
        match consume () with
            (* Entire arg list consumed *)
            | None -> ()

            (* Argument found *)
            | Some key -> (match (CCHashtbl.get lookup key) with
                (* This is a known argument and it has no arguments *)
                | Some Arg.Unit f -> f ()

                (* This is a known argument and it has one argument, a string *)
                | Some Arg.String f -> (match consume () with
                        | None -> raise @@ Arg.Bad ("option '"^key^"' needs an argument.")
                        | Some arg -> f arg)

                (* Incorrect use of ArgPlus *)
                | Some _ -> argPlusLimitations "argParse"

                (* Not a known argument key *)
                | None ->
                    (* Interpret key string to see just what this is *)
                    let keyLen = String.length key in

                    (* It starts with a - *)
                    if (keyLen > 0 && String.get key 0 == '-') then
                        let eqAt = try Some (String.index key '=') with Not_found -> None in
                        match eqAt with
                            (* It's a --a=b, which is why we didn't find the key in the lookup table... *)
                            | Some splitAt ->
                                (* Split out the key and value implied by the = and take a pass at lookup *)
                                let subKey = String.sub key 0 splitAt in
                                let subValue = String.sub key (splitAt+1) (keyLen-splitAt-1) in
                                (match (CCHashtbl.get lookup subKey) with
                                    (* The argument is recognized, but can't be used with = *)
                                    | Some Arg.Unit _ -> raise @@ Arg.Bad ("option '"^subKey^"' does not take an argument.")

                                    (* The argument is recognized and we can work with it *)
                                    | Some Arg.String f -> f subValue

                                    (* Incorrect use of ArgPlus *)
                                    | Some _ -> argPlusLimitations "argParse"

                                    (* Despite pulling out the =, it's still unrecognized *)
                                    | None -> raise @@ Arg.Bad ("unknown option '"^subKey^"'"))

                            (* No gimmicks, it's just plain not recognized *)
                            | None -> raise @@ Arg.Bad ("unknown option '"^key^"'")

                    (* This doesn't start with a -, so it's an anonymous argument. Let the user handle it *)
                    else
                        fallback key
            );
            proceed()

    (* Error/exceptional situation handling *)
    in let name = match consume() with Some s -> s (* argv[0] is executable name *)
        | None -> "(INTERNAL ERROR)" (* None implies argc==0, so we won't ever be displaying this anyway? *)
    in try
        (try
            proceed()
        with
            (* An arg rule requested the help be shown using the Arg interface. *)
            | Arg.Help _ -> raise @@ Help 0 (* FIXME: What is the argument to Arg.Help for? It isn't documented. *)

            (* An arg rule requested a premature halt to processing. *)
            | Complete -> ());

        (* We ended without failing, so call the complete handler *)
        onComplete !rest
    with
        (* Something requested the help be shown *)
        | Help i -> Arg.usage rules usage; exit i

        (* Something requested we flag failure to the user *)
        | Arg.Bad s -> prerr_endline @@ name^": "^s; Arg.usage rules usage; exit 1
