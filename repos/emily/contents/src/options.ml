(* Parse and validate command line arguments. *)

let version = "0.3b"
let fullVersion = ("Emily language interpreter: Version " ^ version)

type executionTarget = Stdin | File of string | Literal of string

type optionSpec = {
    (* Execution args *)
    mutable target : executionTarget option;
    mutable args : string list;
    mutable repl : bool;
    mutable stepMacro : bool;
    mutable trace : bool;
    mutable trackObjects : bool;
    mutable traceSet : bool;
    mutable packagePath : string option;
    mutable projectPath : string option;
    mutable dontNeedTargets : bool; (* Set indirectly by several options *)

    (* Things to do instead of execution *)
    mutable disassemble : bool;
    mutable disassembleVerbose : bool;
    mutable printPackage : bool;
    mutable printProject : bool;
    mutable printVersion : bool;
    mutable printMachineVersion : bool;
}

let run = {
    target=None; args=[];
    repl=false;
    stepMacro=false; trace=false; trackObjects=false; traceSet=false;
    packagePath=None;projectPath=None;
    dontNeedTargets=false;
    disassemble=false; disassembleVerbose=false;
    printPackage=false; printProject=false; printVersion=false; printMachineVersion=false;
}

let keyMutateArgument    = ArgPlus.keyMutate @@ fun l -> "--" ^ (String.concat "-" l)
let keyMutateEnvironment = ArgPlus.keyMutate @@ fun l -> "EMILY_" ^ (String.concat "_" @@ List.map String.uppercase l)

let buildPathSetSpec name action whatIs =
    (name, Arg.String(action), "Directory root for packages loaded from \"" ^ whatIs ^ "\"")

let () =
    let usage = (fullVersion ^ {|

Sample usage:
    emily filename.em     # Execute program
    emily -               # Execute from stdin
    emily -e "println 3"  # Execute from command line|}

(* Only include this bit if REPL enabled *)
^ (if%const [%getenv "BUILD_INCLUDE_REPL"] <> "" then {|
    emily -i              # Run in interactive mode (REPL)
    emily -i filename.em  # ...after executing this program|}
else "")^{|

Options:|})

    in let versionSpec key = (key, Arg.Unit(fun () -> run.printVersion <- true), {|Print interpreter version|})

    in let executeArgs = [ (* Basic arguments *)
        ("-", Arg.Unit(fun () -> (* Arg's parser means the magic - argument must be passed in this way. *)
            run.target <- Some Stdin;
            raise ArgPlus.Complete
        ), ""); (* No summary, this shouldn't be listed with options. *)

        (* Args *)
        ("-e", Arg.String(fun f ->
            run.target <- Some(Literal f);
            raise ArgPlus.Complete
        ), "Execute code inline");

    ] @ (if%const [%getenv "BUILD_INCLUDE_REPL"] <> "" then [

        (* Only include if Makefile requested REPL *)
        ("-i", Arg.Unit(fun f ->
            run.repl <- true; run.dontNeedTargets <- true;
        ), "Enter interactive mode (REPL)");

    ] else []) @ [ (* Normal arguments continue *)

        versionSpec "-v";
        versionSpec "--version";

        ("--machine-version", Arg.Unit(fun () -> run.printMachineVersion <- true), {|Print interpreter version (number only) and quit|});
    ]

    in let environmentArgs = [ (* "Config" arguments which can be also set with env vars *)
        buildPathSetSpec ["package";"path"]          (fun a -> run.packagePath <- Some a)  "package";
        buildPathSetSpec ["project";"path"]          (fun a -> run.projectPath <- Some a)  "project";
    ]

    in let debugArgs = [ (* For supporting Emily development itself-- separate out to sort last in help *)
        ("--debug-dis",   Arg.Unit(fun () -> run.disassemble <- true),        {|Print "disassembled" code and exit|});
        ("--debug-disv",  Arg.Unit(fun () -> run.disassembleVerbose <- true), {|Print "disassembled" code with position data and exit|});
        ("--debug-macro", Arg.Unit(fun () -> run.stepMacro <- true),          {|Print results of each individual macro evaluation|});
        ("--debug-trace", Arg.Unit(fun () -> run.trace <- true),              "When executing, print interpreter state");
        ("--debug-track", Arg.Unit(fun () -> run.trackObjects <- true),       {|When executing, give all objects a unique "!id" member|});
        ("--debug-set",   Arg.Unit(fun () -> run.traceSet <- true),           {|When executing, print object contents on each set|});
        ("--debug-run",   Arg.Unit(fun () ->
            run.trace <- true;
            run.trackObjects <- true;
            run.traceSet <- true
        ),  {|When executing, set all runtime trace type options|});
        ("--debug-print-package-path", Arg.Unit(fun () -> run.printPackage <- true; run.dontNeedTargets <- true),  {|Print package loader path and quit|});
        ("--debug-print-project-path", Arg.Unit(fun () -> run.printProject <- true; run.dontNeedTargets <- true),  {|Print project loader path and quit|});
    ]

    in let args = executeArgs @ (keyMutateArgument environmentArgs) @ debugArgs

    in let targetParse t = (run.target <- Some(File t); raise ArgPlus.Complete)

    in
    ArgPlus.envParse (keyMutateEnvironment environmentArgs);
    ArgPlus.argParse args targetParse usage (fun progArgs ->
        (* Arguments are parsed; either short-circuit with an informational message, or store targets *)
        (* FIXME: Is "withholding" run.targets in this way really the most elegant way to do this? *)
        if run.printMachineVersion then print_endline version else
        if run.printVersion then print_endline fullVersion else (
            run.args <- progArgs;
            if (run.dontNeedTargets) then () else match run.target with
                | None -> raise @@ ArgPlus.Help 1 (* No targets! Fail and print help. *)
                | _    -> ()
        )
    )
