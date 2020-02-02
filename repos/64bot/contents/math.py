# Super minimal set of math functions, it would be better to just depend on numpy or something

from math import sqrt, sin, cos, pi

def vector_add(a, b):
	return [a[0]+b[0], a[1]+b[1], a[2]+b[2]]

def vector_sub(a, b):
	return [a[0]-b[0], a[1]-b[1], a[2]-b[2]]

# Scalar multiply
def vector_mul(a, b):
	return [x*b for x in a]

def vector_len(a):
	b = [x*x for x in a]
	return sqrt(b[0] + b[1] + b[2])

def vector_desc(a):
	return "%.2f, %.2f, %.2f" % (a[0], a[1], a[2])

# Works with vectors or vector2s
def vector_dot(a,b):
	result = 0
	for idx in range(len(a)):
		result += a[idx]*b[idx]
	return result

# I use "rot" to describe a Mario64 angle, a short
def rot_to_deg(rot):
	return rot/65535.0*360.0

# I use "rot16" to use a rounded Mario64 angle, which is what they use to compute
def rot_to_rot16(rot):
	return rot - (rot%16)

def rot_to_rad(rot):
	return rot/65535.0*pi*2

def rad_to_vector2(rad):
	return [cos(rad), sin(rad)]

