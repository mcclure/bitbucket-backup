# String prototype methods

# Expect:
# a
# ⚧
# c

"a⚧c".char.each println

# Expect: 北

println: do: ^@( "北京市".char.each ^x( return x ) )

# Expect:
# 97.
# 9895.
# 99.

atc = "a⚧c"
atc.codepoint.each println

# Expect:
# 3. <true> <null> ⚧
# 3. <true> <null> 9895.

printsp (atc.char.count) (atc.char.has 1) (atc.char.has 4) (atc.char 1) ln \
        (atc.codepoint.count) (atc.codepoint.has 1) \
        (atc.codepoint.has 4) (atc.codepoint 1) ln

# Expect: abcd
println: "ab" + "cd"

# String conversion from other types

# Expect: ab3.de
println: "ab" + (3.toString) + "d" + (.e.toString)
