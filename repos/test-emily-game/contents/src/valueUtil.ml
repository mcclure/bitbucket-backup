(* This file contains support methods for creating values with certain properties, split out from Value for module recursion reasons. *)
open Value

(* Misc failure throw methods *)
let badArg desired name var = failwith @@ "Bad argument to "^name^": Need "^desired^", got " ^ Pretty.dumpValue(var)
let badArgTable = badArg "table"
let badArgClosure = badArg "closure"
let impossibleArg name = failwith @@ "Internal failure: Impossible argument to "^name

let misapplyString a b = "Runtime failure: "^(Pretty.dumpValue a)^" can't respond to "^(Pretty.dumpValue b)
let rawMisapplyArg a b = failwith @@ misapplyString a b

(* Tools *)
let boolCast v = if v then True else Null
let ignoreFirst _ x = x

(* Create a closure from an ocaml function *)
let snippetClosure argCount exec =
    ClosureValue({ exec = ClosureExecBuiltin(exec); needArgs = argCount;
        bound = []; this = ThisNever; })

(* For debugging, call this after creating a hashtable set to become a Value *)
let sealTable t =
    if Options.(run.trackObjects) then
        (idGenerator := !idGenerator +. 1.0; tableSet t idKey (FloatValue !idGenerator))

(* Same as calling tableBlank TrueBlank. We need a separate version because
   tableBlank relies on some of the functions that require tableTrueBlank
   to themselves be defined, and let rec gets awkward w/mutual recursion. *)
let tableTrueBlank () =
    let t = Hashtbl.create(1) in sealTable t; t
let tableTrueBlankInheriting v =
    let t = tableTrueBlank () in tableSet t parentKey v;
        t

(* Makes a scope to be used in a snippetTextClosure *)
let snippetScope bindings =
    let scopeTable = tableTrueBlank () in
    List.iter (fun x -> match x with (k,v) -> tableSet scopeTable (AtomValue k) v) bindings;
    TableValue(scopeTable)

(* Define an ad hoc function using a literal string inside the interpreter. *)
let snippetTextClosureAbstract source thisKind context keys text =
    ClosureValue({ exec = ClosureExecUser({body = Tokenize.snippet source text;
        envScope=snippetScope context;
        scoped = false; key = keys; return = false;
    }); needArgs = List.length keys;
        bound = []; this = thisKind; })

(* Define an ad hoc function using a literal string inside the interpreter. *)
let snippetTextClosure source = snippetTextClosureAbstract source ThisNever
let snippetTextMethod  source = snippetTextClosureAbstract source ThisBlank

let snippetApply closure value =
    match closure with
        | ClosureValue ({bound;needArgs} as cv) ->
            if needArgs > 1 then
                ClosureValue {cv with bound=value::bound;needArgs=needArgs-1}
            else
                failwith "Internal error"
        | _ -> failwith "Internal error"

(* These first three snippet closures are relied on by the later ones *)

(* Ternary function without short-circuiting... *)
(* internal.tern exposes this *)
let rawTern = snippetClosure 3 (function
    | [Null;_;v] -> v
    | [_;v;_] -> v
    | _ -> impossibleArg "rawTern")

(* ...used to define the ternary function with short-circuiting: *)
(* This is used by snippets that require tern, but tern in scopePrototype is separate. *)
let tern = snippetTextClosure (Token.Internal "tern")
    ["rawTern", rawTern; "null", Null]
    ["pred"; "a"; "b"]
    "(rawTern pred a b) null"

(* This handles what occurs when you assign to a table while defining a new object literal.
   It takes newborn functions and assigns a this to them. (Old functions just freeze.) *)
let rawRethisAssignObjectDefinition (obj:value) v = match v with
    | ClosureValue({this=ThisBlank} as c) ->
        ClosureValue({c with this=CurrentThis(obj,obj)})
    | ClosureValue({this=CurrentThis(current,this)} as c) -> ClosureValue({c with this=FrozenThis(current,this)})
    | _ -> v

(* This handles what occurs when you assign to a table at any other time:
   The "newborn" quality that makes it possible to assign a `this` is lost. *)
let rawRethisAssignObject v = match v with
    | ClosureValue({this=ThisBlank} as c) -> ClosureValue({c with this=ThisNever})
    | ClosureValue({this=CurrentThis(current,this)} as c) -> ClosureValue({c with this=FrozenThis(current,this)})
    | _ -> v

(* Emily versions of the above two *)
let rethisAssignObjectDefinition = snippetClosure 2 (function
    | [obj;a] -> rawRethisAssignObjectDefinition obj a
    | _ -> impossibleArg "rethisAssignObjectDefinition")

let rethisAssignObject = snippetClosure 1 (function
    | [a] -> rawRethisAssignObject a
    | _ -> impossibleArg "rethisAssignObject")

(* This could have been done in-place with a k combinator *)
let rethisAssignObjectInsideLet _ x = rawRethisAssignObject x

(* This next batch is the functions required to create a blank user table *)

(* Setup for filter-based functions *)
(* FIXME: Remove need for target *)
let actTableSet (target:value) (t:tableValue) key value =
    tableSet t key value;
    if Options.(run.traceSet) then print_endline @@ "Set update "^Pretty.dumpValueNewTable target

let actTableSetWith (modifier:value->value->value) (target:value) (t:tableValue) key value =
    actTableSet target t key (modifier target value)

let actPairTableSet t1v t1 t2v t2 key value =
    actTableSet t1v t1 key value;
    actTableSet t2v t2 key value

(* Most tables need to be preopulated with a "has". Here's the has tester for a singular table: *)
let rawHas = snippetClosure 2 (function
    | [TableValue t;key] | [ObjectValue t;key] -> boolCast (tableHas t key)
    | [v;_] -> badArgTable "rawHas" v
    | _ -> impossibleArg "rawTern")

(* A curried one which knows how to check the super class: *)
let hasConstruct = snippetTextClosure (Token.Internal "hasConstruct")
    ["rawHas",rawHas;"tern",tern;"true",Value.True;"null",Value.Null]
    ["obj";"key"]
    "tern (rawHas obj key) ^(true) ^(
         tern (rawHas obj .parent) ^(obj.parent.has key) ^(null)
     )"

(* ...And a factory for one with a preset object: *)
let makeHas obj = snippetApply hasConstruct obj

(* Most tables need to be preopulated with a "set". Here's the setter for a singular table: *)
let rawSet = snippetClosure 3 (function (* TODO: Unify with makeLet? *)
    | [TableValue t as tv;key;value] | [ObjectValue t as tv;key;value] ->
        actTableSet tv t key value;
        Null
    | [v;_;_] -> badArgTable "rawSet" v
    | _ -> impossibleArg "rawSet")

(* ...And a factory for a curried one that knows how to check the super class: *)
let setConstruct = snippetTextClosure (Token.Internal "setConstruct")
    ["rawHas",rawHas;"rawSet",rawSet;"tern",tern;"true",Value.True;"null",Value.Null]
    ["obj";"key"; "value"]
    "tern (rawHas obj key) ^(rawSet obj key value) ^(
         obj.parent.set key value                # Note: Fails in an inelegant way if no parent
     )"

let makeSet obj = snippetApply setConstruct obj

(* Same thing, but for an ObjectValue instead of a TableValue.
   The difference lies in how "this" is treated *)
let objectSetConstruct = snippetTextClosure (Token.Internal "objectSetConstruct")
    ["rawHas",rawHas;"rawSet",rawSet;"tern",tern;"true",Value.True;"null",Value.Null;"modifier",rethisAssignObject]
    ["obj";"key"; "value"]
    "tern (rawHas obj key) ^(rawSet obj key (modifier value)) ^(
         obj.parent.set key (modifier value) # Note: Fails in an inelegant way if no parent
     )"

let makeObjectSet obj = snippetApply objectSetConstruct obj

(* Many tables need to be prepopulated with a "let". Here's the let setter for a singular table: *)
(* TODO: Don't 'make' like this? *)
let makeLet (action:value->value->unit) = snippetClosure 2 (function
    | [key;value] ->
        action key value;
        Null
    | _ -> impossibleArg "makeLet")

(* Helpers for tableBlank *)
let populateWithHas t =
    tableSetString t Value.hasKeyString (makeHas (TableValue t)) (* FIXME: Should this ever be ObjectValue...? *)
let populateWithSet t =
    populateWithHas t;
    tableSetString t Value.setKeyString (makeSet (TableValue t))

(* Not unified with tableBlank because it returns a value not an object *)
let objectBlank context =
    let obj = tableTrueBlank() in
    let objValue = ObjectValue obj in
    populateWithHas obj;
    tableSetString obj Value.setKeyString (makeObjectSet objValue);
    tableSetString obj Value.letKeyString
        (makeLet (actTableSetWith rethisAssignObjectInsideLet objValue obj));
    tableSetString obj Value.parentKeyString (context.Value.objectProto);
    objValue

(* FIXME: Once actTableSet no longer takes a table value, the dummy TableValue will not be needed *)
let populateLetForScope storeIn writeTo =
    tableSetString storeIn Value.letKeyString (makeLet (actTableSet (TableValue writeTo) writeTo))

(* Give me a simple table of the requested type, prepopulate with basics. *)
let rec tableBlank kind : tableValue =
    let t = tableTrueBlank() in (match kind with
        | TrueBlank -> ()
        | NoSet -> populateWithHas t
        | NoLet -> populateWithSet t
        | WithLet ->
            populateWithSet t;
            populateLetForScope t t
    );
    t

type boxTarget = Package | Object
type boxSpec   = Populating of boxTarget*value

let boxBlank boxKind boxParent =
    let Populating(targetType,targetValue) = boxKind in
    let t = tableBlank NoLet in
    let privateTable = tableBlank NoLet in
    let privateValue = TableValue privateTable in
    let targetTable = tableFrom targetValue in
    tableSet privateTable Value.letKey (makeLet @@ (* Another fallacious value usage *)
        actPairTableSet privateValue privateTable (TableValue t) t
    );
    tableSet t Value.letKey (makeLet @@ (* See objects.md *)
        if targetType=Package then
            actPairTableSet targetValue targetTable (TableValue t) t
        else
            actTableSetWith rawRethisAssignObjectDefinition targetValue targetTable
    );
    if targetType=Package then
        tableSet t exportLetKey (makeLet @@ actTableSet targetValue targetTable);
    tableSet t thisKey   targetValue;
    tableSet t parentKey boxParent;
    tableSet t currentKey targetValue;
    (* Access to a private value: *)
    tableSet t privateKey privateValue;
    t

let tableInheriting tableKind v =
    let t = tableBlank tableKind in
        tableSet t parentKey v;
        t

(* Not used by interpreter, but present for user *)
let rawRethisTransplant obj = match obj with
    | ClosureValue c -> ClosureValue({c with this=ThisBlank})
    | _ -> obj

let rethisTransplant = snippetClosure 1 (function
    | [obj] -> rawRethisTransplant obj
    | _ -> impossibleArg "rethisSuperFrom")

(* Helpers for super function *)
let rawRethisSuperFrom obj v = match v with
    | ClosureValue({this=CurrentThis(current,_)} as c) -> ClosureValue({c with this=CurrentThis(current,obj)})
    | _ -> v

let rethisSuperFrom = snippetClosure 2 (function
    | [obj;a] -> rawRethisSuperFrom obj a
    | _ -> impossibleArg "rethisSuperFrom")

let misapplyArg = snippetClosure 2 (function
    | [a;b] -> rawMisapplyArg a b
    | _ -> impossibleArg "misapplyArg")

(* Factory for super functions *)
let superConstruct = snippetTextClosure (Token.Internal "superConstruct")
    ["rethis",rethisSuperFrom;"rawHas",rawHas;"tern",tern;"misapplyArg",misapplyArg]
    ["callCurrent";"obj";"arg"]
    "tern (rawHas callCurrent .parent) ^(rethis obj (callCurrent.parent arg)) ^(misapplyArg obj arg)"

let makeSuper current this = snippetApply (snippetApply superConstruct current) this

let stackString stack =
    let rec stackStringImpl stack accum =
        match stack with
            | [] -> accum
            | frame::moreFrames ->
                stackStringImpl moreFrames @@ accum ^ "\n\t" ^ (
                    match frame with
                        | {register=LineStart _; code=({Token.at}::_)::_} -> Token.positionString at
                        | {register=FirstValue (_,_,at)} -> Token.positionString at
                        | {register=PairValue (_,_,_,at)} -> Token.positionString at
                        | {code=[]} -> "<empty file>"
                        | {code=[]::_} -> "<lost place>")
    in stackStringImpl stack "Stack:"

let rawMisapplyStack stack a b = failwith @@ (misapplyString a b) ^ "\n" ^ (stackString stack)

let makeLazy table key func =
    let lock _ = (* Later maybe pass on this to func? *)
        let result = func () in
            ( tableSet table key result ; result )
    in BuiltinUnaryMethodValue ( lock )

let tableSetLazy table key func =
    tableSet table key (makeLazy table key func)

