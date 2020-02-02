(* Set up environment and invoke Execute *)

(* File handling utilities *)

(* Convert a filename to an atom key for a loader *)
(* FIXME: Refuse to process "unspeakable" atoms, like "file*name"? *)
let nameAtom filename = Value.AtomValue (try
        Filename.chop_extension filename      (* If there is an extension remove it *)
    with
        Invalid_argument _ -> filename)       (* If there is no extension do nothing *)

(* See also path.ml *)
let defaultPackagePath =
    let envPath = [%getenv "BUILD_PACKAGE_DIR"] in
    Path.executableRelativePath envPath
let packageRootPath () =
    match Options.(run.packagePath) with Some s -> s | _ -> defaultPackagePath

(* What should the target of this particular loader be? *)
type loaderSource =
    | NoSource                  (* I don't want the loader *)
    | SelfSource                (* I want the loader inferred from context *)
    | Source of Value.value     (* I want it to load from a specific path *)

(* From what source should the project/directory loaders for this execution come? *)
type loadLocation =
    | Cwd             (* From the current working directory *)
    | Path of string  (* From a known location *)

(* Given a loaderSource and a known context path, eliminate the SelfSource case *)
let selfFilter self source = match source with SelfSource -> Source self | _ -> source

(* Given a pre-selfFiltered loaderSource, convert to an option. *)
(* FIXME: Should the error state (SelfSource) instead be an allowed case? *)
let knownFilter source = match source with
    | NoSource -> None
    | Source x -> Some x
    | _ -> failwith "Internal error: Package loader attempted to load a file as if it were a directory"

(* There is one "base" starter plus one substarter for each file executed.
   The base starter lacks a project/directory, the others have it. *)

(* Given a starter, make a new starter with a unique scope that is the child of the old starter's scope.
   Return both the starter and the new scope. *)
(* THIS COMMENT IS WRONG, FIX IT *)
let subStarterWith starter table =
    {starter with Value.rootScope=Value.TableValue table}
let subStarterPair ?kind:(kind=Value.NoLet) starter =
    let table = ValueUtil.tableInheriting kind starter.Value.rootScope in
    table,subStarterWith starter @@ table

let boxSubStarter starter kind =
    subStarterWith starter @@ ValueUtil.boxBlank kind starter.Value.rootScope

(* Given a starter, make a new starter with a subscope and the requested project/directory. *)
let starterForExecute starter (project:Value.value option) (directory:Value.value option) =
    let table,subStarter = subStarterPair starter in
    Value.tableSetOption table Value.projectKey   project;
    Value.tableSet table Value.directoryKey
        (match directory with Some d -> d | None -> Value.TableValue(ValueUtil.tableBlank Value.NoSet));
    subStarter

(* Loader is invoking execute internally to load a package from a file. *)
let executePackage starter (project:Value.value option) (directory:Value.value option) buf =
    Execute.execute (starterForExecute starter project directory) buf

let packagesLoaded = Hashtbl.create(1)

(* Create a package loader object. Will recursively call itself in a lazy way on field access.
   Directory and projectSource will be replaced as needed on field access, will not. *)
(* TODO: Consider a guard on multi-loading? *)
(* TODO: Here "lazy" means if you access a field corresponding to a file, the file is loaded.
         Maybe loading should be even lazier, such that load when a field is loaded *from* a file, load occurs?
         This would make prototype loading way easier. *)
(* FIXME: Couldn't kind be NewScope and the starter impose the box? *)
let loadFile starter (projectSource:loaderSource) (directory:loaderSource) path =
    (* FIXME: What if knownFilter is NoSource here? This is the "file where expected a directory" case. *)
    let buf = Tokenize.tokenizeChannel (Token.File path) (open_in path)
    in executePackage starter (knownFilter projectSource) (knownFilter directory) buf
let rec loadPackageDir starter (projectSource:loaderSource) path =
    let directoryTable = ValueUtil.tableBlank Value.NoSet in
    let directoryObject = Value.ObjectValue directoryTable in
    let directoryFilter = selfFilter directoryObject in
    let proceed = loadPackage starter (directoryFilter projectSource) (Source directoryObject) in
    Array.iter (fun name ->
        ValueUtil.tableSetLazy directoryTable (nameAtom name)
            (fun _ -> proceed (Filename.concat path name))
    ) (Sys.readdir path); directoryObject
and loadPackage starter (projectSource:loaderSource) (directory:loaderSource) path =
    (* This is gonna do bad things on case-insensitive filesystems *)
    match CCHashtbl.get packagesLoaded path with
        | Some v -> v
        | None ->
            let v =
                try
                    (* COMMENT ME!!! This is not good enough. *)
                    if String.length path == 0 then
                        raise @@ Sys_error "Empty path"
                    else if Sys.is_directory path then
                        loadPackageDir starter projectSource path
                    else
                        let packageScope = Value.TableValue(ValueUtil.tableBlank Value.NoSet) in
                        ignore @@ loadFile (boxSubStarter starter @@ ValueUtil.(Populating(Package,packageScope)))
                            projectSource directory path;
                        packageScope
                with Sys_error s ->
                    Value.TableValue( ValueUtil.tableBlank Value.NoSet )
            in Hashtbl.replace packagesLoaded path v;
            v

(* Return the value for the project loader. Needs to know "where" the project is. *)
let projectPathForLocation location =
    match Options.(run.projectPath) with
        | Some s -> s
        | _ -> (match location with Cwd -> Path.bootPath | Path str -> str)
let projectForLocation starter defaultLocation =
    loadPackage starter SelfSource NoSource @@ projectPathForLocation defaultLocation

(* For external use: Given a file, get the loadLocation it would be executed within. *)
let locationAround path =
    Path (Filename.dirname path)

(* External entry point: Build a starter *)
let completeStarter withProjectLocation =
    let rootScope = ValueUtil.tableBlank Value.NoSet in
    let nv() = Value.TableValue(ValueUtil.tableBlank Value.NoSet) in (* "New value" *)
    let packageStarter = Value.{rootScope=Value.TableValue rootScope;context={
        nullProto=nv(); trueProto=nv(); floatProto=nv();
        stringProto=nv(); atomProto=nv(); objectProto=nv()}} in
    let packagePath = packageRootPath () in
    let package = loadPackage packageStarter NoSource NoSource packagePath
    in
    let populateProto proto pathKey =
        (* TODO convert path to either path or value to load from  *)
        (* TODO find some way to make this not assume path loaded from disk *)
        let path = List.fold_left FilePath.concat packagePath ["emily";"core";"prototype";pathKey ^ ".em"] in
        let enclosing = loadPackageDir packageStarter NoSource @@ Filename.dirname path in
        ignore @@ loadFile
            (boxSubStarter packageStarter @@ ValueUtil.(Populating(Package,proto)))
            NoSource (Source enclosing) path
    in
    Value.tableSet rootScope Value.internalKey InternalPackage.internalValue;
    Value.tableSet rootScope Value.packageKey package;
    populateProto Value.(packageStarter.rootScope)           "scope";
    populateProto Value.(packageStarter.context.nullProto)   "null";
    populateProto Value.(packageStarter.context.trueProto)   "true";
    populateProto Value.(packageStarter.context.floatProto)  "number";
    populateProto Value.(packageStarter.context.stringProto) "string";
    populateProto Value.(packageStarter.context.atomProto)   "atom";
    populateProto Value.(packageStarter.context.objectProto) "object";
    let project = projectForLocation packageStarter withProjectLocation in
    let scope,starter = subStarterPair ~kind:Value.WithLet packageStarter in
    Value.tableSet scope Value.projectKey   project;
    Value.tableSet scope Value.directoryKey project;
    starter

(* External entry point: Given a starter and a buffer, execute it *)
let executeProgramFrom location buf =
    Execute.execute (completeStarter location) buf
