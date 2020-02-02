# Test game library failure cases: imgFree needs an arg
# Expect failure

e = package.emily.game # E for Engine
e.init []
e.imgFree[1,] # Only 0 is valid currently
e.finish []
