# Test game library. Awkwardly this does mean opening a window...

e = package.emily.game # E for Engine

e.init []

# Expect:
# <true>
# <null>
# 1.
println (e.imgValid[0,]) (e.imgValid[1,]) (e.imgCount[])

# Expect:
# 1.
# <true>
# 2.
img = e.imgPush [32,32]
println (img) (e.imgValid[img,]) (e.imgCount[])

# Expect:
# <null>
# 2.
e.imgFree[img,]
println (e.imgValid[img,]) (e.imgCount[])

e.finish []
