# Take control of Mario. While this script runs, Mario will run forward and jump mindlessly.

class Runjump(Runnable):
	def __init__(self):
		self.ticks = 0

	# When starting, jam the analog stick forward.
	def onStart(self):
		debug.buttons_override(y_axis=127)

	# Hit A every five frames.
	def onBlank(self):
		debug.buttons_override(a_button=(self.ticks%5==0))
		self.ticks += 1

	# Return control to user.
	def onStop(self):
		debug.buttons_override_disable()

result(Runjump())
