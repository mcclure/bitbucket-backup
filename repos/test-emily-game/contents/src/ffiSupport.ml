open Ctypes
open Foreign

type foreignWrap = {
    mutable name : string option;
    mutable args : string list;
    mutable returning: string;
}

type valueToCFn = ValueToCFn : ('a Ctypes.typ * (Value.value -> 'a)) -> valueToCFn

type cToValueFn = CToValueFn : ('a Ctypes.typ * ('a -> Value.value)) -> cToValueFn

let valueToCFor = function
    | "void" -> ValueToCFn ( void, function _ -> () )
    | "int" -> ValueToCFn ( int, function
            | Value.FloatValue f -> int_of_float f
            | _ -> failwith "Expected number"
        )
    | "double" -> ValueToCFn ( double, function
            | Value.FloatValue f -> f
            | _ -> failwith "Expected number"
        )
    | "string" -> ValueToCFn ( string, function
            | Value.StringValue s -> s
            | _ -> failwith "Expected string"
        )
    | s -> failwith @@ "Unsupported type name:" ^ s

let cToValueFor = function
    | "void"   -> CToValueFn ( void,   fun _ -> Value.Null )
    | "int"    -> CToValueFn ( int,    fun x -> Value.FloatValue (float_of_int x) )
    | "double" -> CToValueFn ( double, fun x -> Value.FloatValue x )
    | "string" -> CToValueFn ( string, fun x -> Value.StringValue x )
    | s -> failwith @@ "Unsupported type name for return:" ^ s

let valueForeignUnary name argTypeName retType retConvert =
  let ValueToCFn (argType, argConvert) = valueToCFor argTypeName in
  let ffi_binding = foreign name (argType @-> returning retType) in
  Value.BuiltinFunctionValue(
    fun arg -> retConvert (ffi_binding (argConvert arg))
  )

let valueForeign name argTypeNames retTypeName =
  let CToValueFn (retType, retConvert) = cToValueFor retTypeName in
  match argTypeNames with
    | [] ->  valueForeignUnary name "void" retType retConvert
    | [a] -> valueForeignUnary name a      retType retConvert
    | [a;b] -> (* "Arg type 0, arg convert 0..." *)
        let ValueToCFn (at0, ac0) = valueToCFor a in
        let ValueToCFn (at1, ac1) = valueToCFor b in
        let ffi_binding = foreign name (at0 @-> at1 @-> returning retType) in
        Value.BuiltinFunctionValue( fun a0 ->
        Value.BuiltinFunctionValue( fun a1 ->
            retConvert (ffi_binding (ac0 a0) (ac1 a1))
        ) )
    | [a;b;c] -> (* Each arity needs its own implementation currently *)
        let ValueToCFn (at0, ac0) = valueToCFor a in
        let ValueToCFn (at1, ac1) = valueToCFor b in
        let ValueToCFn (at2, ac2) = valueToCFor c in
        let ffi_binding = foreign name (at0 @-> at1 @-> at2 @-> returning retType) in
        Value.BuiltinFunctionValue( fun a0 ->
        Value.BuiltinFunctionValue( fun a1 ->
        Value.BuiltinFunctionValue( fun a2 ->
            retConvert (ffi_binding (ac0 a0) (ac1 a1) (ac2 a2))
        ) ) )
    | [a;b;c;d] -> (* Each arity needs its own implementation currently *)
        let ValueToCFn (at0, ac0) = valueToCFor a in
        let ValueToCFn (at1, ac1) = valueToCFor b in
        let ValueToCFn (at2, ac2) = valueToCFor c in
        let ValueToCFn (at3, ac3) = valueToCFor d in
        let ffi_binding = foreign name (at0 @-> at1 @-> at2 @-> at3 @-> returning retType) in
        Value.BuiltinFunctionValue( fun a0 ->
        Value.BuiltinFunctionValue( fun a1 ->
        Value.BuiltinFunctionValue( fun a2 ->
        Value.BuiltinFunctionValue( fun a3 ->
            retConvert (ffi_binding (ac0 a0) (ac1 a1) (ac2 a2) (ac3 a3))
        ) ) ) )
    | [a;b;c;d;e] -> (* Each arity needs its own implementation currently *)
        let ValueToCFn (at0, ac0) = valueToCFor a in
        let ValueToCFn (at1, ac1) = valueToCFor b in
        let ValueToCFn (at2, ac2) = valueToCFor c in
        let ValueToCFn (at3, ac3) = valueToCFor d in
        let ValueToCFn (at4, ac4) = valueToCFor e in
        let ffi_binding = foreign name (at0 @-> at1 @-> at2 @-> at3 @-> at4 @-> returning retType) in
        Value.BuiltinFunctionValue( fun a0 ->
        Value.BuiltinFunctionValue( fun a1 ->
        Value.BuiltinFunctionValue( fun a2 ->
        Value.BuiltinFunctionValue( fun a3 ->
        Value.BuiltinFunctionValue( fun a4 ->
            retConvert (ffi_binding (ac0 a0) (ac1 a1) (ac2 a2) (ac3 a3) (ac4 a4))
        ) ) ) ) )
    | [a;b;c;d;e;f] -> (* Each arity needs its own implementation currently *)
        let ValueToCFn (at0, ac0) = valueToCFor a in
        let ValueToCFn (at1, ac1) = valueToCFor b in
        let ValueToCFn (at2, ac2) = valueToCFor c in
        let ValueToCFn (at3, ac3) = valueToCFor d in
        let ValueToCFn (at4, ac4) = valueToCFor e in
        let ValueToCFn (at5, ac5) = valueToCFor f in
        let ffi_binding = foreign name (at0 @-> at1 @-> at2 @-> at3 @-> at4 @-> at5 @-> returning retType) in
        Value.BuiltinFunctionValue( fun a0 ->
        Value.BuiltinFunctionValue( fun a1 ->
        Value.BuiltinFunctionValue( fun a2 ->
        Value.BuiltinFunctionValue( fun a3 ->
        Value.BuiltinFunctionValue( fun a4 ->
        Value.BuiltinFunctionValue( fun a5 ->
            retConvert (ffi_binding (ac0 a0) (ac1 a1) (ac2 a2) (ac3 a3) (ac4 a4) (ac5 a5))
        ) ) ) ) ) )
    | [a;b;c;d;e;f;g] -> (* Each arity needs its own implementation currently *)
        let ValueToCFn (at0, ac0) = valueToCFor a in
        let ValueToCFn (at1, ac1) = valueToCFor b in
        let ValueToCFn (at2, ac2) = valueToCFor c in
        let ValueToCFn (at3, ac3) = valueToCFor d in
        let ValueToCFn (at4, ac4) = valueToCFor e in
        let ValueToCFn (at5, ac5) = valueToCFor f in
        let ValueToCFn (at6, ac6) = valueToCFor g in
        let ffi_binding = foreign name (at0 @-> at1 @-> at2 @-> at3 @-> at4 @-> at5 @-> at6 @-> returning retType) in
        Value.BuiltinFunctionValue( fun a0 ->
        Value.BuiltinFunctionValue( fun a1 ->
        Value.BuiltinFunctionValue( fun a2 ->
        Value.BuiltinFunctionValue( fun a3 ->
        Value.BuiltinFunctionValue( fun a4 ->
        Value.BuiltinFunctionValue( fun a5 ->
        Value.BuiltinFunctionValue( fun a6 ->
            retConvert (ffi_binding (ac0 a0) (ac1 a1) (ac2 a2) (ac3 a3) (ac4 a4) (ac5 a5) (ac6 a6))
        ) ) ) ) ) ) )
    | [a;b;c;d;e;f;g;h] -> (* Each arity needs its own implementation currently *)
        let ValueToCFn (at0, ac0) = valueToCFor a in
        let ValueToCFn (at1, ac1) = valueToCFor b in
        let ValueToCFn (at2, ac2) = valueToCFor c in
        let ValueToCFn (at3, ac3) = valueToCFor d in
        let ValueToCFn (at4, ac4) = valueToCFor e in
        let ValueToCFn (at5, ac5) = valueToCFor f in
        let ValueToCFn (at6, ac6) = valueToCFor g in
        let ValueToCFn (at7, ac7) = valueToCFor h in
        let ffi_binding = foreign name (at0 @-> at1 @-> at2 @-> at3 @-> at4 @-> at5 @-> at6 @->at7 @-> returning retType) in
        Value.BuiltinFunctionValue( fun a0 ->
        Value.BuiltinFunctionValue( fun a1 ->
        Value.BuiltinFunctionValue( fun a2 ->
        Value.BuiltinFunctionValue( fun a3 ->
        Value.BuiltinFunctionValue( fun a4 ->
        Value.BuiltinFunctionValue( fun a5 ->
        Value.BuiltinFunctionValue( fun a6 ->
        Value.BuiltinFunctionValue( fun a7 ->
            retConvert (ffi_binding (ac0 a0) (ac1 a1) (ac2 a2) (ac3 a3) (ac4 a4) (ac5 a5) (ac6 a6) (ac7 a7))
        ) ) ) ) ) ) ) )
    | _ -> failwith "Max ffi arguments currently 8"
