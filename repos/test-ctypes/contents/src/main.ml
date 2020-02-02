open Ctypes
open Foreign

let () =
    let increment3 = foreign "increment3" (int @-> returning int) in
    print_endline @@ string_of_int @@ increment3 3
