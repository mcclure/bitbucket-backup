# Test a return statement as a weird sort of GOTO.
# Expect:
# 5.
# 4.
# 3.
# 2.
# 1.

goto ^y = y y

counter = 5
loop = do ^@(return)
println: counter
counter = counter - 1
counter > 0 ? goto loop : null