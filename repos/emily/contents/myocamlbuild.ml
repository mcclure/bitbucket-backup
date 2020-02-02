(* This is required to work around bugs when using ocamlbuild with the C embed feature. *)

open Ocamlbuild_plugin
open Ocamlbuild_pack

let args =
  S[A"-output-complete-obj"]

let () =
  dispatch (function
    | After_rules ->
      flag ["ocaml"; "link"; "native"; "output_obj"] args;
      flag ["ocaml"; "link"; "byte"; "output_obj"]   args
    | _ -> ())