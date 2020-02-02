#!/usr/bin/python

# Generates a JSON file containing SDL2 guids, button mappings, and button labelings.

import sys
import os.path
import simplejson as json
import collections
from ordereddict import OrderedDict

script = sys.argv[0]
verbose = False

def warn(s):
	"""Print with self advertisement."""
	print "%s: %s" % (sys.argv[0], s)

def fail(s):
	"""Print with self advertisement, then exit."""
	msg(s)
	sys.exit(1)

def debug(s):
	"""Print with self advertisement if in debugging mode."""
	if verbose:
		warn(s)
	
def dget(d, key, default):
	"""Read from a dictionary. If key not found, write and return default."""
	try:
		return d[key]
	except KeyError:
		d[key] = default
		return default

def isDict(d):
	return type(d) == dict or type(d) == OrderedDict

def dpull(d1, d2):
	"""Copy all keys from d2 into d1. Merge dictionaries on collision."""
	for k in d2:
		if k in d1 and isDict(d1[k]) and isDict(d2[k]):
			d1[k] = dmerge(d1[k], d2[k])
		else:
			d1[k] = d2[k]

def dmerge(d1, d2):
	"""Return a new dictionary which is the "union" of these two. Either may be nil."""
	a = OrderedDict()
	dpull(a, d1)
	dpull(a, d2)
	return a

unsurprising_platforms = ["Linux", "Windows", "Mac OS X"]

# Place being executed from. TODO: Optionally take src and target files as arguments
location = os.path.dirname(script)

# We will generate the game controller database from two files.
# One, the standard SDL2 gamecontrollerdb.txt.
# Two, a "patches" file containing additional information and known corrections to the SDL database.
plusbase = "gamecontrollerplus.json"
textfile = os.path.join(location, "gamecontrollerdb.txt")
plusfile = os.path.join(location, plusbase)
outfile  = os.path.join(location, "..", "desktop", "Internal", "gamecontroller.json")

entries = {}

# Plusdict is initialized with the "patches" file structure; the gamecontrollerdb content is filled in from there.
try:
	plusdict = json.load(open(plusfile), object_pairs_hook=OrderedDict)
except IOError:
	fail("%s could not be opened?" % (plusbase))

# The "meta" dictionary is the only thing in the patches file which doesn't get passed through.
# It contains:
# "bettername": A mapping of names in the SDL2 database to plusfile-format names. If a bettername is present, the old one is discarded.
# "simplername": A mapping of plusfile-format names to human-readable names. If a simplername is present, both are included in the final.
# "inherit": If A inherits B, automatically populate B's labels into A
# "comments": Guaranteed to be ignored by this script
if "meta" in plusdict:
	meta = plusdict["meta"]
	del plusdict["meta"]
else:
	meta = {}
dpull(meta, {"inherit":{}, "bettername":{}, "simplername":{}})

# controller structures stored in:
# controller_array: indexed by position in file; entries: indexed by unique-name
controller_array = dget(plusdict, "db", [])
for controller in controller_array:
	try:
		entries[ controller["name"] ] = controller
	except KeyError:
		fail("Malformed %s? Found nameless controller." % (plusbase))

def get_controller(name):
	"""Fetch the controller by the given unique-name. If none found, initialize one."""
	try:
		return entries[name]
	except KeyError:
		entry = OrderedDict({"name":name})
		if name in meta["simplername"]: # See "meta" above
			entry["simplername"] = meta["simplername"][name]
		if name in meta["inherit"]: # See "meta" above
			entry["inherit"] = meta["inherit"][name]
		entries[name] = entry
		controller_array.append(entry)
		return entry

# Now ready to read the SDL2 database
for line in open(textfile):
	line = line.split("#")[0].strip() # Clear comments and trailing whitespace
	if not line: # Skip blank lines
		continue
		
	# Each line is a comma separated a:b dict, first two fields are special
	items = collections.deque(line.split(","))
	try:
		guid = items.popleft()
		name = items.popleft()
		if name in meta["bettername"]: # See meta above
			debug( "Replacing %s with %s" % (name, meta["bettername"][name]) )
			name = meta["bettername"][name]
	except IndexError:
		warn("Skipping malformed line: \"%s\"" % (line))
		continue
	platform = None
	mapping = {}
	for term in items:
		if not term:
			continue
		kv = term.split(":")
		try:
			if kv[0] == "platform" or kv[0] == "Platform": # Horribly the official file is not consistent
				platform = kv[1]
			else:
				mapping[kv[0]] = kv[1]
		except IndexError:
			warn("Skipping malformed kv \"%s\" for controller %s" % (term, name))
	if not platform:
		warn("Controller %s doesn't mention platform, skipping" % (name))
		continue
	if platform not in unsurprising_platforms:
		warn("Controller %s has unrecognized platform %s" % (name, platform))
	
	# Notice: JSON always overrides txt.
	controller = get_controller(name)
	platform = dget( dget(controller, "platforms", {}), platform, {} )
	
	# TODO: Better to allow a list of guids? Reconsider if this ever comes up.
	dget(platform, "guid", guid) # Write guid if not present
	
	platform["mapping"] = dmerge(mapping, dget(platform, "mapping", {}))

# Done
json.dump(plusdict, open(outfile, 'w'), indent=4)