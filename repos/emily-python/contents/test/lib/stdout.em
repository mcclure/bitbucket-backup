# Test stdout/stderr

# Expect:
# 1 2 3 4 5
# Next writing
# 678

print 1 2
stdout.print 3 4
stderr.print 10 11 12 ln # FIXME: Can this be confirmed?
print 5 ln
stdout.print "Next writing\n"
stdout.write 6 7 8 ln
stdout.flush # FIXME: Is this testable?

# Test unicode
# Expect: ⚧
print "⚧"
