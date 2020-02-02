# Demonstrate timing of evaluation for a curried closure.
# Expect:
# a
# b

let .a ^x y ( println "b" )
let .b (a 3)

println "a"
b 4
