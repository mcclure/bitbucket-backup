# Test dict ops

# Expect:
# false
# false
# false
# true
# 5
# true
# word
# 3 ok
# false
# false

let d = new Dict

println
	d.iter.more

println
	d.has "ok"
	d.has 3

d.set "ok" 5
d.set 3 "word"

println
	d.has "ok"
	d.get "ok"
	d.has 3
	d.get 3

# Warning: This assumes the keys are printed sorted, which is not guaranteed
let i = d.iter
while (i.more)
	print (i.next)
print ln

d.del "ok"
d.del 3

println
	d.has "ok"
	d.has 3
