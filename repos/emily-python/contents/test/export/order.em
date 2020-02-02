# Test side effects when loading from project loader

# Expect:
# Running 'talkyInclude'
# Running 'talkyInclude2'
# 5 5 3

print
	project.talkyInclude2.y
	project.talkyInclude2.y
	project.talkyInclude.x
	ln
