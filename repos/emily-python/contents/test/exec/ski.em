# Test turing completeness by way of combinators

# Expect:
# 1729

let i = function (x) (x)

let k = function (x, y) (x)

let s = function (x, y, z) (x z (y z))

let counter = 0

let newline = function (x)   # Mimic Unlambda "r"
    println counter
    counter = 0
    x                # Act as identity

let star = function (x)      # Mimic Unlambda ".*"
    counter =
    	+ counter 1
    x                        # Act as identity


# "1729 stars" sample from Unlambda manual
# Original author: David Madore <david.madore@ens.fr>

(((s (k newline)) ((s ((s i) (k star))) (k i))) (((s ((s (k ((s i) (k (s ((s (k s)) k)))))) ((s ((s (k s)) k)) ((s ((s (k s)) k)) i)))) ((s (k ((s ((s (k s)) k)) ((s ((s (k s)) k)) i)))) (s ((s (k s)) k)))) (((s ((s (k s)) k)) i) ((s ((s (k s)) k)) ((s ((s (k s)) k)) i)))))
