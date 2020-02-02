#!/usr/bin/python

# Script for copying scenes out of one Stencyl file and into another

# Version 1.0.0b
# Written by Andi McClure

import sys
import optparse
import xml.dom.minidom
import os
import shutil
import re
#import subprocess
#import platform
#import tempfile

returncode = 0

# Constants

# Command line help?
help  = "%prog -i GAMEPATH list"
help  = "%prog -i GAMEPATH -o GAMEPATH2 copy SCENENAME"

# Parse command line
parser = optparse.OptionParser(usage=help)
parser.add_option("-i", "--input-game", action="store", help="Path to game to pull from")
parser.add_option("-o", "--output-game", action="store", help="Path to game to pull from")
parser.add_option("-n", "--new-name", action="store", help="New name for scene after copy (optional)")
parser.add_option("-d", "--raw-id", action="store_true", help="Specify raw ID# for scene to copy instead of name")
parser.add_option("--no-snippet", action="store_true", help="Don't copy scripts")
parser.add_option("--no-actor", action="store_true", help="Don't copy actors")
parser.add_option("-m", "--minimal", action="store_true", help="Like no-snippet plus no-actor")
(options, cmds) = parser.parse_args()
def flag(a):
	try:
		return getattr(options, a)
	except AttributeError:
		return None

skip_actor = flag("no_actor") or flag("minimal")
skip_snippet = flag("no_snippet") or flag("minimal")

# State

in_game = None
out_game = None

class Game:
	def __init__(self, path):
		# Basic sanity
		self.valid = False			# DEF bool load successful
		self.basepath = path		# DEF string path directory whole
		if not os.path.isdir(path):
			print("Folder path '%s' doesn't exist." % (path))
			return
			
		# Load scenes
		self.scenepath = os.path.join(path, "scenes")		# DEF string path scene directory
		self.scenexmlpath = os.path.join(self.scenepath, "scenes.xml")	# DEF string path scene file
		if not os.path.exists(self.scenexmlpath):
			print("Folder path '%s' doesn't contain a scenes.xml." % (self.scenexmlpath))
			return
		self.scenedom = xml.dom.minidom.parse(self.scenexmlpath) # DEF DOM scenes.xml
		self.items = []										 # DEF array scene descriptors by order
		self.itemsbyid = {}									 # DEF dictionary scene discriptors by id
		self.itemsbyname = {}								 # DEF dictionary scene discriptors by name
		self.highestsceneid = -1							 # DEF string numeric-highest seen scene id
		self.highestsnippetid = -1							 # DEF string numeric-highest seen snippet id
		for node in self.scenedom.documentElement.getElementsByTagName("scene"):
			id = node.getAttribute("id")
			if not id:
				print("Warning: Malformed scene?")
			else:
				make = {"node":node, "id":id}
				self.items.append(make)
				self.itemsbyid[id] = make
				name = node.getAttribute("name")
				if name:
					self.itemsbyname[name] = make
				if int(id) > int(self.highestsceneid):
					self.highestsceneid = id
		
		# Load snippets
		self.snippetpath = os.path.join(self.basepath, "snippets") # DEF string path snippet directory
		self.snippetxmlpath = os.path.join(self.snippetpath, "list.xml") # DEF string path snippet file
		if skip_snippet or not os.path.exists(self.snippetxmlpath):
			if not skip_snippet:
				print("Warning: Folder path '%s' doesn't contain a scenes.xml." % (self.snippetpath))
			self.snippetdom = None
		else:
			self.snippetdom = xml.dom.minidom.parse(self.snippetxmlpath)
			for node in self.snippetdom.documentElement.getElementsByTagName("snippet"):
				id = node.getAttribute("id")
				if not id:
					print("Warning: Malformed scene?")
				else:
					if int(id) > int(self.highestsnippetid):
						self.highestsnippetid = id
		
		self.valid = True
	
	def write(self):
		with open(self.scenexmlpath ,"w") as f:
			self.scenedom.writexml(f)

		if self.snippetdom:
			with open(self.snippetxmlpath ,"w") as f:
				self.snippetdom.writexml(f)
	
def load_in():
	if not flag("input_game"):
		return False
	global in_game
	in_game = Game(flag("input_game"))
	return in_game.valid
	
def load_out():
	if not flag("output_game"):
		return False
	global out_game
	out_game = Game(flag("output_game"))
	return out_game.valid

def do_list():
	global returncode
	if not load_in():
		returncode = 2
		return
	for i in in_game.items:
		name = i["node"].getAttribute("name")
		print("id:%s name:'%s'" % (i["id"], name))

def cram(path, base, extension):
	return os.path.join( path, ("%s.%s"%(base,extension)) )

def underscorecram(path, base, id, extension):
	return cram(path, ("%s_%s" % (base,id)), extension)

def set_numbercap(outxml, attr, oldid, newid):
	value = outxml.getAttribute(attr)
	newvalue = re.sub("_%s$" % (oldid), "_%s" % (newid), value)
	outxml.setAttribute(attr, newvalue)

def do_one_scene_copy(oldid):
	newid = str(int(out_game.highestsceneid)+1)
	out_game.highestsceneid = newid
	newname = flag("new_name")
	node = in_game.itemsbyid[oldid]["node"]
	
	# Copy files
	shutil.copyfile( cram(in_game.scenepath, oldid, "scn"), cram(out_game.scenepath, newid, "scn") );
	shutil.copyfile( cram(in_game.scenepath, oldid, "png"), cram(out_game.scenepath, newid, "png") );
	
	# Copy "snippets", whatever those are
	# Must happen before copy level xml
	eventsnippetid = None
	if in_game.snippetdom:
		for snippet in in_game.snippetdom.documentElement.getElementsByTagName("snippet"):
			sceneid = snippet.getAttribute("sceneid")
			if sceneid == oldid:
				orig_snippet_id = snippet.getAttribute("id")
				out_game.highestsnippetid = str(int(out_game.highestsnippetid)+1)
				
				if eventsnippetid:
					print("WARNING: Multiple event snippets for a single scene. This is unexpected, and probably something's about to break.")
				eventsnippetid = out_game.highestsnippetid
				
				snippet.setAttribute("sceneid", newid)
				snippet.setAttribute("id", out_game.highestsnippetid)
				set_numbercap(snippet, "class", oldid, newid)
				set_numbercap(snippet, "name", oldid, newid)
				shutil.copyfile( underscorecram(in_game.snippetpath, "SceneEvents", oldid, "design"), underscorecram(out_game.snippetpath, "SceneEvents", newid, "design") )
				shutil.copyfile( cram(in_game.snippetpath, orig_snippet_id, "png"), cram(out_game.snippetpath, out_game.highestsnippetid, "png") )
				out_game.snippetdom.documentElement.appendChild(snippet)
	
	# Manual-copy level xml file
	scenexml = xml.dom.minidom.parse( cram(in_game.scenepath, oldid, "xml") )
	scenexml.documentElement.setAttribute("id", newid)
	if newname: # Set this early, we'll care later
		scenexml.documentElement.setAttribute("name", newname)
	if eventsnippetid:
		scenexml.documentElement.setAttribute("eventsnippetid", eventsnippetid)
	else:
		if scenexml.documentElement.hasAttribute("eventsnippetid"):
			scenexml.documentElement.removeAttribute("eventsnippetid")
	if skip_actor:
		for actorblock in scenexml.documentElement.getElementsByTagName("actors"):
			victims = []
			for actor in actorblock.getElementsByTagName("actor"):
				victims.append( actor )
			for victim in victims:
				actorblock.removeChild(victim)
	with open( cram(out_game.scenepath, newid, "xml") ,"w") as f:
		scenexml.writexml(f)
	
	# Edit master xml file
	node.setAttribute("id", newid)
	if newname:
		node.setAttribute("name", newname)
	out_game.scenedom.documentElement.appendChild(node)
		
	out_game.write()

def do_copy():
	global returncode
	ids = []
	if not (load_in() and load_out()):
		returncode = 2
		return
	for i in range(1,len(cmds)):		
		if flag("raw_id"):
			oldid = cmds[i]
			if not in_game.itemsbyid.has_key(oldid):
				print("Could not find scene id %s in game." % (oldid))
				returncode = 2
				return
			ids.append(oldid)
		else:
			name = cmds[i]
			if not in_game.itemsbyname.has_key(name):
				print("Could not find scene named '%s' in game." % (name))
				returncode = 2
				return
			oldid = in_game.itemsbyname[name]["id"]
			ids.append(oldid)
	for id in ids:
		do_one_scene_copy(id)

if len(cmds)>0 and cmds[0] == "list":
	do_list()
elif len(cmds)>0 and cmds[0] == "copy":
	if len(cmds)>1:
		do_copy()
	else:
		print("SCENEID missing.\n")
		returncode = -1
else:
	if len(cmds)>0:
		print("Command '%s' not recognized.\n" % (cmds[0]))
	else:
		print("Command missing.\n")
	returncode = -1
	
if returncode == -1:
	parser.print_help()

sys.exit(returncode)