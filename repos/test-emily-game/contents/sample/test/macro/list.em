# Test comma/append operator.
# Expect:
# 1.
# 2.
# 3.
# <null>
# 2.
# 3.

# Note empty sections ignored
[,1,2, ,3,].each println

# This is awful, and eventually the comma operator will be replaced just to prevent it:
# Inner comma appends arguments to spooky (!) to object,
# Outer comma appends result of =, which is a null, to object.
x = [ spooky ^x = (x,), ]

[2,3].each: x.spooky

x.each println
