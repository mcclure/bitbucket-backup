# Warning: This script will crash if loaded when no ROM is open.
# NOTE: By convention all Mario scripts in this repository use Z-up

load("math")

def mario_version():
	regionByte = debug.mem_read_8(0x9000003E)
	if regionByte == 0x45:
		return "US"
	if regionByte == 0x4A:
		return "JP"
	return "UNKNOWN"

memory_map = None

def reload_mario():
	global memory_map
	version = mario_version()
	if version == "UNKNOWN":
		trace("WARNING: Unrecognized region. Assuming US")

	if version == "JP":
		class memory_map_jp:
			loaded = version

			xyz = [0x80339E44, 0x80339E3C, 0x80339E40] # Floats
			stars = 0x80339EAA # Short
			health = 0x80339EAE  # Byte (fragment count)
			object_count = 0x8033BF00 # Int
			first_object = 0x8033C118 # Pointer !!! UNSURE ABOUT THIS ONE
			mario_object = 0x8035FDE8 # Pointer
			standing_object = 0x8032FED4 # Pointer

		memory_map = memory_map_jp

	else:
		class memory_map_us:
			loaded = version
			xyz = [0x8033B1B4, 0x8033B1AC, 0x8033B1B0] 
			stars = 0x8033B21A
			health = 0x8033B21E
			object_count = 0x8033D270
			first_object = 0x8033D488 # !!! UNSURE ABOUT THIS ONE
			mario_object = 0x80361158
			standing_object = 0x80330E34

		memory_map = memory_map_us

reload_mario()

def mario_at_raw():
	return [debug.mem_read_float_32(v) for v in memory_map.xyz]

def mario_at():
	return vector_desc(mario_at_raw())

def move_mario(x,y,z):
	mem = [debug.mem_read_float_32(v) for v in memory_map.xyz]
	mem[0] += x
	mem[1] += y
	mem[2] += z
	for i in range(3):
		debug.mem_write_float_32(memory_map.xyz[i], mem[i])
