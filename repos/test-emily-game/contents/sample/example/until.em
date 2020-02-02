# "Until" example from tutorial.md -- implement the opposite of "while", three ways
# Expect:
# 5.4.3.2.1.0.
# 5.4.3.2.1.0.
# 5.4.3.2.1.0.

compose ^a ^b ^c = a (b c)
test ^flowcontrol = {
    x = 5
    flowcontrol ^(x < 0) ^(print x; x = x - 1)
    println ""
}

# Normal
until ^condition ^block = while ^(!( do condition )) block

test until

# Curried
until2 ^condition = while (compose not condition)

test until2

# "Point-free"
until3 = compose while (compose not)

test until3