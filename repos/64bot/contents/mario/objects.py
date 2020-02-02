load("mario/basics")

class object_map:
	prev = 0x4 # Pointer
	next = 0x8 # Pointer
	model = 0x14 # Pointer
	xyz = [0xA8, 0xA0, 0xA4] # Floats
	rot = [0x1E, 0x1A, 0x1C] # Shorts

object_models = None

def reload_objects():
	global object_models
	version = mario_version()
	if version == "UNKNOWN":
		trace("WARNING: Unrecognized region. Assuming US")

	if version == "JP":
		# Note: This approach is totally incorrect; each object designation only works inside a single course! 
		class object_models_jp:
			cork_box = 0x8018a1cc
			sign = 0x800f5dcc
			goomba = 0x8018a3ac
			tree = 0x8018c420
			bobomb = 0x8018a5fc
			wood_platform = 0x8018a11c
			chomp = 0x800f5dcc
			billboard = 0x0
			red_coin = 0x800f6da4
			post = 0x8018c3f0
			bowling_ball = 0x8018aaac
			spin_platform = 0x8014f880
			conveyor = 0x8014f6e4
			heave_ho = 0x8014f32c
			exclamation_block = 0x8014de7c
			skeeter = 0x800f65e4
			heave_ho_wetdry = 0x8018934c

		object_models = object_models_jp
	else:
		class object_models_us:
			pass # I know nothing

		object_models = object_models_us

reload_objects()

object_models_reverse = {}
for key in dir(object_models):
	if key[0] != '_':
		object_models_reverse[object_models.__dict__[key]] = key

pre_objects_reload_mario = reload_mario
def reload_mario():
	pre_objects_reload_mario()
	reload_objects()

def object_prev(ptr):
	return debug.mem_read_32(ptr + object_map.prev)

def object_next(ptr):
	return debug.mem_read_32(ptr + object_map.next)

def object_at_raw(ptr):
	return [debug.mem_read_float_32(ptr + v) for v in object_map.xyz]

def object_at(ptr):
	return vector_desc(object_at_raw(ptr))

def object_rot_raw(ptr):
	return [debug.mem_read_16(ptr + v) for v in object_map.rot]

def object_rot_deg(ptr):
	return vector_desc([rot_to_deg(rot_to_rot16(v)) for v in object_rot_raw(ptr)])

def object_rot_vector_z(ptr):
	return rad_to_vector2(rot_to_rad(rot_to_rot16(object_rot_raw(ptr)[2]))) + [0]

def object_first():
	return memory_map.first_object

def object_count():
	return debug.mem_read_32(memory_map.object_count)

def object_specific(idx):
	obj = object_first()
	for _ in range(idx):
		obj = object_next(obj)
	return obj

def iter_objects(uncapped = False):
	limit = 240 if uncapped else object_count()
	obj = object_first()
	for _ in range(limit):
		yield obj
		obj = object_next(obj) # Should be fine at end because list is circular?

def object_dist_from_mario(ptr):
	at = mario_at_raw()
	return vector_len(vector_sub(at, object_at_raw(ptr)))

def object_closest():
	closest_obj = None
	closest_dist = 0
	for obj in iter_objects():
		dist = object_dist_from_mario(obj)
		if not closest_obj or dist < closest_dist:
			closest_obj = obj
			closest_dist = dist
	return closest_obj

def object_index(ptr):
	count = 0
	for obj in iter_objects(True):
		if ptr == obj:
			return count
		count += 1

def object_model(ptr):
	return debug.mem_read_32(ptr+object_map.model)

def object_desc(ptr, quiet=False, pos=False):
	model = object_model(ptr)
	line = ["#%d of %d" % (object_index(ptr), object_count())]
	if not quiet:
		line.append("dist %.2f" % (object_dist_from_mario(ptr)))
	line.append("model %x%s" % (model,
		(" (%s)" % (object_models_reverse[model])) if model in object_models_reverse else ""))
	if pos:
		line.append(object_at(ptr))
	return ", ".join(line)
