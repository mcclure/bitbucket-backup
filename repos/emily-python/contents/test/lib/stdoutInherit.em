# Verify that a weird behavior is stable:
# Because print does not automatically blackhole, the + will be applied once and not afterward.
# Expect: 2 2 3 100

# Selfhost works different
# Tags: broken-meta

let deformedStdout = inherit stdout
	method print = function(x)
		super.print (+ x 1)

deformedStdout.print 1.0 2.0 3.0
stdout.print 100.0 ln
