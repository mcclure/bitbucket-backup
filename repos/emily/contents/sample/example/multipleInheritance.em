# "Multiple inheritance" sample from intro.md
# Expect:
# 3.

# A function that returns a function-- it generates the fake "parent"
dualInherit ^parent1 ^parent2 = ^key {
    thisUpdate this: \
        (parent1.has key ? parent1 : parent2) key
}

hasAddOne = [
    addOne ^ = this.value = this.value + 1
]
hasAddTwo = [
    addTwo ^ = this.value = this.value + 2
]

child = [
    parent = dualInherit hasAddOne hasAddTwo
    value  = 0
]

do: child.addOne
do: child.addTwo
println: child.value