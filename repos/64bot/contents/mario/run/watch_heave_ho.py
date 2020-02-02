load("mario/objects")

from math import atan2

last_launch = None

def angle_difference_deg(a,b):
	a -= b
	if a < 180:
		a += 360
	if a > 180:
		a -= 360
	return a

class HeaveHoTracker(Runnable):
  def onBlank(self):
    debug.mem_write_8(memory_map.health, 8)
    out = "------------------\nMario: %s\n" % (mario_at())
    for obj in iter_objects():
      if object_model(obj) in [object_models.heave_ho, object_models.heave_ho_wetdry]:
      	obj_at = object_at_raw(obj)
      	offset_to_mario = vector_sub(mario_at_raw(), obj_at)
      	angle_to_mario = atan2(offset_to_mario[1], offset_to_mario[0])*360.0/(pi*2)
      	angle_facing = rot_to_deg(rot_to_rot16(object_rot_raw(obj)[2]))
      	angle_to_mario_closeness = angle_difference_deg(angle_to_mario, angle_facing)
      	toward_mario = object_rot_vector_z(obj)
      	toward_mario_mag = vector_dot(offset_to_mario[0:2], toward_mario[0:2])
      	launch_point = vector_add(object_at_raw(obj), vector_mul(toward_mario, 152.0))
      	global last_launch
      	last_launch = launch_point
      	out += "%s (%s-deg)\nToward %.3f-deg %.3f # Launch %s" % (object_desc(obj, False, True), object_rot_deg(obj), angle_to_mario_closeness, toward_mario_mag, vector_desc(launch_point))
      	standing_obj = debug.mem_read_32(memory_map.standing_object)
      	out += "\nSTANDING "
      	if standing_obj:
      		out += object_desc(standing_obj, False, True)
      	else:
	      	out += "----"
    trace(out)

result(HeaveHoTracker())
