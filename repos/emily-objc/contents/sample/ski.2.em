# Test turing completeness by way of combinators (2). Expected output: 1,729 asterisks.

set .i ^x( x )

set .k ^x( ^y( x ) )

set .s ^x( ^y ( ^z ( x z (y z) ) ) )

set .newline ^(print "\n")

set .star ^(print "*")

# "1729 stars" sample from Unlambda manual
(((s (k newline)) ((s ((s i) (k star))) (k i))) (((s ((s (k ((s i) (k (s ((s (k s)) k)))))) ((s ((s (k s)) k)) ((s ((s (k s)) k)) i)))) ((s (k ((s ((s (k s)) k)) ((s ((s (k s)) k)) i)))) (s ((s (k s)) k)))) (((s ((s (k s)) k)) i) ((s ((s (k s)) k)) ((s ((s (k s)) k)) i)))))