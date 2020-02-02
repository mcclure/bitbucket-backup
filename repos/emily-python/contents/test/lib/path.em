# Path interactions

let myDir = file.path.join "test" "lib"
let myFile = file.path.join myDir "path.em"

# Expect: true false false true

print
	file.path.isDir  myDir
	file.path.isDir  myFile
	file.path.isFile myDir
	file.path.isFile myFile
	ln

let normalizeMyDir  = file.path.normalize myDir
let normalizeMyFile = file.path.normalize myFile

let weirdDir = file.path.join myDir ".."
weirdDir = file.path.join weirdDir "lib"

# Expect: false true true true true

print
	== normalizeMyDir ""
	== normalizeMyDir (file.path.normalize weirdDir)
	== normalizeMyDir (file.path.dir normalizeMyFile)
	== (file.path.file myFile) "path.em"
	== (file.path.file (file.path.entryFile)) "path.em"
	ln
