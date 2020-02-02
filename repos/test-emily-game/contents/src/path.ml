(* Simple path tools *)

(* TODO: This should be normalized. Strongly consider using extunix.realpath instead *)
let readlink path = FileUtil.readlink path
let bootPath = readlink @@ Sys.getcwd()
let executableRelativePath path =
    (* This will work as long as the executable was not executed from $PATH. *)
    if FilePath.is_relative path then
        let invokePath = Filename.dirname @@ Array.get Sys.argv 0 in
        let exePath = readlink @@ if FilePath.is_relative invokePath then
            Filename.concat (Sys.getcwd()) invokePath else invokePath
        in Filename.concat exePath path
    else path
