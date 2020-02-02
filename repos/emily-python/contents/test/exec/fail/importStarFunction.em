# Test for sensible failure on import * from strange thing
# Expect failure

let a = function (x) (x)

from a import *
