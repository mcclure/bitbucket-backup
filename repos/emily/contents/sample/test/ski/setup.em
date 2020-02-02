# Test turing completeness by way of combinators (1). Expected output: ** [newline] *
# NOT PART OF REGRESSION TESTS

let .i ^x( x )

let .k ^x( ^y( x ) )

let .s ^x( ^y ( ^z ( x z (y z) ) ) )

let .newline ^(print "\n")

let .star ^(print "*")

star i
star i
newline i
star i