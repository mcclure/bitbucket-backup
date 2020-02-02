# Ensure import ASTs turn out as expected

# Arg: --ast2
# Expect: [Sequence(Scoped) [Let Scope [AtomLiteral b] [Apply [Var a] [AtomLiteral b]]] [Let Scope [AtomLiteral d] [Apply [Var c] [AtomLiteral d]]] [Let Scope [AtomLiteral g] [Apply [Apply [Var e] [AtomLiteral f]] [AtomLiteral g]]] [Sequence [Let Scope [AtomLiteral i] [Apply [Var h] [AtomLiteral i]]]] [Sequence [Let Scope [AtomLiteral k] [Apply [Var j] [AtomLiteral k]]]] [Sequence [Let Scope [AtomLiteral m] [Apply [Var l] [AtomLiteral m]]] [Let Scope [AtomLiteral o] [Apply [Var n] [AtomLiteral o]]]] [Sequence [Let Scope [AtomLiteral q] [Apply [Var p] [AtomLiteral q]]] [Let Scope [AtomLiteral s] [Apply [Apply [Var p] [AtomLiteral r]] [AtomLiteral s]]]]]

import a.b
from c import d
from e import f.g
import (h.i)
from j import (k)
import
	l.m
	n.o
from p import 
	q
	r.s
