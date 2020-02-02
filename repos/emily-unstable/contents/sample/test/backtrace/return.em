# Test that backtraces on error are useful. #3: Use a continuation
# NOT PART OF REGRESSION TESTS

a ^b = (
    b 3
)

b ^c = (
    4 (a return)
    2 4
)

8 (b 4) # b will return 3, and therefore fail here