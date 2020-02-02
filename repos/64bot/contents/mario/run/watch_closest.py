# Continuously print Mario's position to the console.

load("mario/objects")

class WatchClosest(Runnable):
	def __init__(self, quiet=False):
		self.quiet = quiet
		self.prev = -1

	def onBlank(self):
		closest = object_closest()
		if self.quiet:
			if closest == self.prev:
				return
			self.prev = closest

		model = debug.mem_read_32(closest+object_map.model)
		
		line = ["Closest #%d of %d" % (object_index(closest), object_count())]
		if not self.quiet:
			line.append("dist %.2f" % (object_dist_from_mario(closest)))
		line.append("model %x%s" % (model,
			(" (%s)" % (object_models_reverse[model])) if model in object_models_reverse else ""))
		trace(", ".join(line))

result(WatchClosest())
