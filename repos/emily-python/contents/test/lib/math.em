# Test arithmetic stuff in library

let x = 1
let y = "okay"

print

# Expect: true true false false true true false false false true true false false false
	== 1 1
	== 1 x
	== 2 1
	== 2 x
	== "okay" "okay"
	== "okay" y
	== "bad" "okay"
	== "bad" y
	== 3 "3"
	== null null
	== true true
	== false null
	== false 0
	== true 1
	ln

# Expect: false false true true false false true true true
	!= 1 1
	!= 1 x
	!= 2 1
	!= 2 x
	!= "okay" "okay"
	!= "okay" y
	!= "bad" "okay"
	!= "bad" y
	!= 3 "3"
	ln

# Expect: true false false
	boolean "true"
	boolean 0
	boolean null
	ln

# Expect: false true true
	not "true"
	not 0
	not null
	ln

# Expect: 4 true 1 6 2 2 -2
	+ 3 1
	== 4 (+ 3 1)
	- 3 2
	* 3 2
	/ 4 2
	% 5 3
	neg 2.0
	ln

# Expect: okayokay true
	+ y y
	== "okayokay" (+ y y)
	ln

# Expect: true false false false true true false true
	> 3 1
	> 3 3
	< 3 1
	< 3 3
	>= 3 1
	>= 3 3
	<= 3 1
	<= 3 3
	ln

# Expect: false false true false false true
	and null null
	and null 1
	and 1 1
	and false false
	and false true
	and true true
	ln

# Expect: false true true false true true
	or null null
	or null 1
	or 1 1
	or false false
	or false true
	or true true
	ln

# Expect: false true false false true false
	xor null null
	xor null 1
	xor 1 1
	xor false false
	xor false true
	xor true true
	ln

# Expect: null
	nullfn 3
	ln
