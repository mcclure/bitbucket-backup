# Test turing completeness by way of combinators (3). Expected output: The fibbonachi sequence, as numbers.
# NOT PART OF REGRESSION TESTS -- DOES NOT TERMINATE

# Identity combinator
let .i ^x( x )

# Kancel combinator
let .k ^x( ^y( x ) )

# Substitution combinator
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

# "fibo.unl" sample from Comprehensive Unlambda Archive Network
# Original author: David Madore <david.madore@ens.fr>

(((s ((s ((s i) i)) ((s (k k)) (k i)))) (k i)) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s (k k)) (k k)))) ((s (k k)) (k newline)))))) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s (k k)) (k k)))) (k i))))) ((s ((s (k s)) ((s (k k)) (k k)))) ((s (k k)) (k star))))))) ((s ((s (k s)) ((s (k k)) (k k)))) ((s (k k)) i))))))) ((s ((s (k s)) ((s (k k)) (k k)))) ((s (k k)) i)))))) ((s (k k)) (k i)))))) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s (k k)) (k k)))) ((s (k k)) (k ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s (k k)) (k k)))) ((s (k k)) (k s)))))) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s (k k)) (k k)))) ((s (k k)) (k s)))))) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s (k k)) (k k)))) ((s (k k)) (k k)))))) ((s ((s (k s)) ((s (k k)) (k k)))) ((s (k k)) i))))))) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s (k k)) (k k)))) ((s (k k)) (k k)))))) ((s (k k)) (k i)))))))) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s (k k)) (k k)))) ((s (k k)) (k s)))))) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s (k k)) (k k)))) ((s (k k)) (k s)))))) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s (k k)) (k k)))) ((s (k k)) (k k)))))) ((s ((s (k s)) ((s (k k)) (k k)))) (k i))))))) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s (k k)) (k k)))) ((s (k k)) (k k)))))) ((s (k k)) (k i)))))))) ((s ((s (k s)) ((s (k k)) (k k)))) ((s (k k)) (k i))))))))))) ((s ((s (k s)) ((s (k k)) (k k)))) (k i)))))) ((s (k k)) (k i)))))