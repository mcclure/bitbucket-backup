# Test emily.functional module

p = package.emily.functional
c = p.combinator
u = p.util

# Test s, k, i

# Copypaste from ski/stars.em
# Expect: 1729.
{
    # FIXME: I need an import
    s = c.s; k = c.k; i = c.i
    counter = 0

    newline ^x = (       # Mimic Unlambda "r"
        println counter
        counter = 0
        x                # Act as identity
    )

    star ^x = (          # Mimic Unlambda ".*"
        counter = counter + 1
        x                # Act as identity
    )

    # "1729 stars" sample from Unlambda manual
    # Original author: David Madore <david.madore@ens.fr>
    (((s (k newline)) ((s ((s i) (k star))) (k i))) (((s ((s (k ((s i) (k (s ((s (k s)) k)))))) ((s ((s (k s)) k)) ((s ((s (k s)) k)) i)))) ((s (k ((s ((s (k s)) k)) ((s ((s (k s)) k)) i)))) (s ((s (k s)) k)))) (((s ((s (k s)) k)) i) ((s ((s (k s)) k)) ((s ((s (k s)) k)) i)))))
}

# Test void just by ensuring this doesn't error or do anything else noticeable
(u.void) println 3 "OK" 5 3 1

# Expect:
# 3.
# 3.
# 7.
# 7.

# Test apply
[c.a, u.apply].each ^a (a 3 println)

# Test compose, identity
[c.o, u.compose].each ^o (
    (o (u.identity): \
       o println: 3 .plus) 4
)

# Test map

# Expect:
# 2.
# 3.
# 4.

u.map ^x(x+1) [1,2,3] .each println
