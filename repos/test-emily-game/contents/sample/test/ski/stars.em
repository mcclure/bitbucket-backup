# Test turing completeness by way of combinators (2). Expected output is 1,729 asterisks.

# Expect:
# 1729.

let .i ^x( x )

let .k ^x( ^y( x ) )

let .s ^x( ^y ( ^z ( x z (y z) ) ) )

let .counter 0

let .newline ^x(    # Mimic Unlambda "r"
    println counter
    set .counter 0
    x                # Act as identity
)

let .star ^x(        # Mimic Unlambda ".*"
    set .counter (counter .plus 1)
    x                # Act as identity
)

# "1729 stars" sample from Unlambda manual
# Original author: David Madore <david.madore@ens.fr>

(((s (k newline)) ((s ((s i) (k star))) (k i))) (((s ((s (k ((s i) (k (s ((s (k s)) k)))))) ((s ((s (k s)) k)) ((s ((s (k s)) k)) i)))) ((s (k ((s ((s (k s)) k)) ((s ((s (k s)) k)) i)))) (s ((s (k s)) k)))) (((s ((s (k s)) k)) i) ((s ((s (k s)) k)) ((s ((s (k s)) k)) i)))))