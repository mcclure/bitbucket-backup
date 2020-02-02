# A selection of ways a single token can be written.

# Arg: --ast
# Expect: (x .y #1 #2. #3.4 #0.5 "6" "7\n8" "\"9\"" x1 .y2 #3 z exx .eyy)

# Backslashes in quotes are not printed sensibly in Python (otherwise works)
# Tags: broken-default

x .y 1 2. 3.4 .5 "6" "7
8" "\"9\"" x1 .y2 3z exx .eyy