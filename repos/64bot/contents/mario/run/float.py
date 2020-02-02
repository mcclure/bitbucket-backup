# Force mario continually in a particular direction

load("mario/basics")

class Float(Runnable):
	def __init__(self, x, y, z):
		self.set(x,y,z)

	def set(self, x, y, z):
		self.x = x
		self.y = y
		self.z = z

	# Continually move.
	def onBlank(self):
		move_mario(self.x, self.y, self.z)

result(Float(0,0,40))
