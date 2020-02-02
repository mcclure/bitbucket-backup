open Ast_mapper
open Ast_helper
open Asttypes
open Parsetree
open Longident

let const_mapper argv =
  (* Our const_mapper only overrides the handling of expressions in the default mapper. *)
  { default_mapper with expr =
    (* Create a recursive function and then immediately return it *)
    let rec process mapper expr =
      (* Shared error handler used by multiple cases below *)
      let didnt_find_if loc =
        raise (Location.Error (
                  Location.error ~loc "[%const] accepts an if statement, e.g. if%const true then 1"))
      in
      match expr with
      (* Is this an extension node? *)
      | { pexp_desc = Pexp_extension ({ txt = "const"; loc }, pstr)} ->
        begin match pstr with
        | PStr [{ pstr_desc = Pstr_eval (exp,_) }] ->
          (* Unpack expression, then recurse to handle internal if%matches and match on result *)
          begin match process mapper exp with
            (* Currently only match with ifthenelse *)
            | { pexp_loc  = loc;
                pexp_desc = Pexp_ifthenelse( {pexp_loc=cond_loc;pexp_desc=cond_desc},
                  then_clause, else_option) } ->
              (* Used by = and <> *)
              let pairTest x y op = 
                match x,y with
                    | Pexp_constant x, Pexp_constant y -> op x y
                    | _ ->
                      raise (Location.Error (
                        Location.error ~loc:cond_loc "[%const if...] does not know how to compare these two expressions"))
              in
              (* Evaluate conditional *)
              let which = match cond_desc with
                | Pexp_construct ({txt=Lident "true"},None) -> true
                | Pexp_construct ({txt=Lident "false"},None) -> false
                | Pexp_apply( {pexp_desc=Pexp_ident({txt=Lident "=" })}, [_,{pexp_desc=x};_,{pexp_desc=y}] ) -> pairTest x y (=)
                | Pexp_apply( {pexp_desc=Pexp_ident({txt=Lident "<>"})}, [_,{pexp_desc=x};_,{pexp_desc=y}] ) -> pairTest x y (<>)
                | _ ->
                  raise (Location.Error (
                      Location.error ~loc:cond_loc "[%const if...] does not know how to interpret this kind of expression"))
              in
              (* Depending on value of conditional, replace self extension node with either the then or else clause contents *)
              if which then then_clause else (match else_option with Some x -> x | _ ->
                (* Or, if the else clause is selected but is not specified, a () *)
                Ast_helper.with_default_loc loc (fun _ -> Ast_convenience.unit ()))
          (* Failed to match Pexp_ifthenelse, so fail *)
          | _ -> didnt_find_if loc
          end
        (* Failed to match Pstr, so fail *)
        | _ -> didnt_find_if loc
        end
      (* Failed to match Pexp_extension, so hand this off to the default mapper. *)
      | x -> default_mapper.expr mapper x;
    in
    process
  }

let () = register "const" const_mapper
