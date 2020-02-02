# Test that backtraces on error are useful. #1: Normal expression
# NOT PART OF REGRESSION TESTS

a ^b = (
    3 4 # Fail here
)

b ^c = (
    a 5
)

b 6

7 8 # Line is unreachable, but pads out AST