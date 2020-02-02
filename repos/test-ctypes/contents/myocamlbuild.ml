open Ocamlbuild_plugin
open Ocamlbuild_pack

let args =
  S[A"-output-complete-obj"]

let () =
  dispatch (function
    | After_rules ->
      flag ["ocaml"; "link"; "native"; "output_obj"] args;
      flag ["ocaml"; "link"; "byte"; "output_obj"]   args;
      dep  ["link";"ocaml";"use_cpart"] ["src/libcpart.a"]
    | Before_rules ->
      let native_link_gen linker =
        Ocaml_compiler.link_gen "cmx" "cmxa"
          !Options.ext_lib [!Options.ext_obj; "cmi"] linker
      in
      let native_output_obj x = native_link_gen Ocaml_compiler.ocamlopt_link_prog
        (fun tags -> tags++"ocaml"++"link"++"native"++"output_obj") x
      in
      rule "ocaml: cmx & o -> native.(so|dll|dylib)"
        ~prod:("%.native"-.-(!Options.ext_dll))
        ~deps:["%.cmx"; "%.o"]
        (native_output_obj "%.cmx" ("%.native"-.-(!Options.ext_dll)));
    | _ -> ())
