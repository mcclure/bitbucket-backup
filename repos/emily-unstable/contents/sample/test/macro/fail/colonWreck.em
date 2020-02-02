# Make sure that ?: and ^: collide in a reasonable way (i.e. an error)
# Expect failure

println: 3 ? ^x: 4 : 5
