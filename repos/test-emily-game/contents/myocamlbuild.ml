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