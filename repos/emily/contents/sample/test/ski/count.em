# Gratuitously implement a SKI test using the Unlambda-style backtick operator. Expected output: Eternally growing pyramid
# NOT PART OF REGRESSION TESTS -- DOES NOT TERMINATE

# Identity combinator
let .i ^x( x )

# Kancel combinator
let .k ^x( ^y( x ) )

# Substitution combinator
let .s ^x( ^y ( ^z ( x z (y z) ) ) )

# Print-asterisk combinator
let .P ^x( print "*"; x )

# Print-return combinator
let .R ^x( println " "; x )

# "count.unl" sample from Comprehensive Unlambda Network
# Original author: David Madore <david.madore@ens.fr>

` ` ` s ` ` s ` k s ` ` s ` k ` s i ` ` s ` k k ` ` s ` k ` s ` ` s ` k s k ` ` s ` k ` ` s ` ` s ` ` s i ` k P ` k R i i ` k i ` k i ` ` s ` ` s ` k s ` ` s ` k ` s i ` ` s ` k k ` ` s ` k ` s ` ` s ` k s k ` ` s ` k ` ` s ` ` s ` ` s i ` k P ` k R i i ` k i
