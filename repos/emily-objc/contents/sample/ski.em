# Test turing completeness by way of combinators (1). Expected output: ** [newline] *

set .i ^x( x )

set .k ^x( ^y( x ) )

set .s ^x( ^y ( ^z ( x z (y z) ) ) )

set .newline ^(print "\n")

set .star ^(print "*")

star i
star i
newline i
star i