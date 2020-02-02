# Test *setting* the parent field on scopes.
# FIXME: This really shouldn't be allowed.
# Expect:
# f
# s
# 4.
# 5.

let .x 5
set .parent println

f
s 4 x
