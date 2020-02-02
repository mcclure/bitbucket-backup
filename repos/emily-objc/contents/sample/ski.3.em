# Test turing completeness by way of combinators (3). Expected output: The fibbonachi sequence, as numbers.

# Identity combinator
set .i ^x( x )

# Kancel combinator
set .k ^x( ^y( x ) )

# Substitution combinator
set .s ^x( ^y ( ^z ( x z (y z) ) ) )

set .counter 0

set .newline ^x(	# Mimic Unlambda "r"
	print counter
	print "\n"
	set .counter 0
	x				# Act as identity
)

set .star ^x(		# Mimic Unlambda ".*"
	set .counter (counter .plus 1)
	x				# Act as identity
)

# "fibo.unl" sample from Comprehensive Unlambda Archive Network
(((s ((s ((s i) i)) ((s (k k)) (k i)))) (k i)) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s (k k)) (k k)))) ((s (k k)) (k newline)))))) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s (k k)) (k k)))) (k i))))) ((s ((s (k s)) ((s (k k)) (k k)))) ((s (k k)) (k star))))))) ((s ((s (k s)) ((s (k k)) (k k)))) ((s (k k)) i))))))) ((s ((s (k s)) ((s (k k)) (k k)))) ((s (k k)) i)))))) ((s (k k)) (k i)))))) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s (k k)) (k k)))) ((s (k k)) (k ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s (k k)) (k k)))) ((s (k k)) (k s)))))) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s (k k)) (k k)))) ((s (k k)) (k s)))))) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s (k k)) (k k)))) ((s (k k)) (k k)))))) ((s ((s (k s)) ((s (k k)) (k k)))) ((s (k k)) i))))))) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s (k k)) (k k)))) ((s (k k)) (k k)))))) ((s (k k)) (k i)))))))) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s (k k)) (k k)))) ((s (k k)) (k s)))))) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s (k k)) (k k)))) ((s (k k)) (k s)))))) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s (k k)) (k k)))) ((s (k k)) (k k)))))) ((s ((s (k s)) ((s (k k)) (k k)))) (k i))))))) ((s ((s (k s)) ((s ((s (k s)) ((s (k k)) (k s)))) ((s ((s (k s)) ((s (k k)) (k k)))) ((s (k k)) (k k)))))) ((s (k k)) (k i)))))))) ((s ((s (k s)) ((s (k k)) (k k)))) ((s (k k)) (k i))))))))))) ((s ((s (k s)) ((s (k k)) (k k)))) (k i)))))) ((s (k k)) (k i)))))