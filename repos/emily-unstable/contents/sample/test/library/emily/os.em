# Test OS interaction points

# These odd instructions are so we can test command line arguments
# Omit file
# Arg: sample/test/library/emily/os.em
# Arg: one
# Arg: --two
# Arg: three

arg = package.emily.os.args

# Expect:
# 3.
# --two
# one
# --two
# three
println: arg.count
println: arg 1
arg.each println
