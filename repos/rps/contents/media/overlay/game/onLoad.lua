-- On Load

pull(gm, {r=Router():insert(), })

gm.a = Player({is="a", base="one", notebase=0}):insert()
gm.b = Player({is="b", base="two", notebase=3}):insert()
gm.ref = Ref():connect(gm.a,gm.b):insert()