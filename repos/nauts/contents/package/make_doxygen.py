#!/usr/bin/python

# Run this from project root
# Run doxygen on Polycode in such a way as to generate xml, but not html.
# Parse the xml and create text files usable by PolyConsole help()

import os
import sys
import optparse
import subprocess
import xml.dom.minidom

base = "media/help"

# b for rebuild, l for list-only
parser = optparse.OptionParser()
for a in ["b","l","v"]: # Single letter args, flags
	parser.add_option("-"+a, action="store_true")
(options, cmds) = parser.parse_args()

def path_for(seq):
	if not isinstance(seq, str):
		seq = "/".join(seq) # Construct path
	return seq

doxygen_path = path_for(['package','documentation'])
xml_path = path_for([doxygen_path, 'xml'])
if options.b or not os.path.isdir(xml_path):
	subprocess.call(["doxygen", "Polycode.doxygen"], cwd=doxygen_path)
	subprocess.call(["doxygen", "Physics2D.doxygen"], cwd=doxygen_path)

xmls = []
for x in os.listdir(xml_path):
	if x.startswith("class") or x.startswith("struct"):
		xmls += [x]

if options.l:
	print xmls
	sys.exit(0)
	
def findChild(dom, name, recurse = True): # Could I have just used dom.getElementsByTagName ?
	c = dom.firstChild
	while c:
		if c.nodeType == xml.dom.minidom.Node.ELEMENT_NODE:
			if c.localName == name: # Don't recurse on a "found" node
				yield c
			elif recurse:
				for d in findChild(c, name):
					yield d
		c = c.nextSibling
		
def mashText(dom):
	result = ""
	c = dom.firstChild
	while c:
		if c.nodeType == xml.dom.minidom.Node.ELEMENT_NODE:
			result += mashText(c)
		elif c.nodeType == xml.dom.minidom.Node.TEXT_NODE:
			result += c.data
		c = c.nextSibling
	return result
	
def stringstrip(s, prefix):
	if s.startswith(prefix):
		s = s[len(prefix):]
	return s
	
def descriptionUnder(dom, name):
	try:
		return mashText( findChild(dom, name, False).next() ).strip()
	except StopIteration:
		return None
	
def description(dom):
	result = descriptionUnder(dom, "detaileddescription")
	if not result:
		result = descriptionUnder(dom, "briefdescription")
	return result
	
def dump(filename, str):
	tfile = open(filename, "w")
	tfile.write(str)
	tfile.close()

class Arg:
	def __init__(self):
		self.name = None
		self.type = None
		self.default = None
		self.description = None

class Meth:
	def __init__(self):
		self.name = None
		self.args = []
		self.description = None
		self.poison = False
	
class Obj:
	def __init__(self):
		self.name = None
		self.methods = {}
		self.description = None
		self.poison = False
	
objects = {}
	
for x in xmls:
	dom = xml.dom.minidom.parse(path_for([xml_path, x]))
	compoundname = findChild(dom, "compoundname").next()
	o = Obj()
	o.name = stringstrip(compoundname.firstChild.data, "Polycode::")
	if options.v:
		print o.name
	o.description = description(compoundname.parentNode)
	# "No description" is the filter used on polycode.org
	o.poison = not o.description or o.name.startswith("TiXml")
	if options.v and o.description:
		print "\t%s" % o.description
	for member in findChild(dom, "memberdef"):
		if member.nodeType == xml.dom.minidom.Node.ELEMENT_NODE and member.localName == "memberdef" and member.getAttribute("kind") == "function":
			m = Meth()
			name = findChild(member, "name", False).next()
			m.name = name.firstChild.data
			m.description = descriptionUnder(member, "briefdescription")
			m.poison = m.name.startswith("~") or m.name.startswith("operator")
			for arg in findChild(member, "declname"): # Safe?
				a = Arg()
				a.name = mashText(arg).strip()
				try:
					a.type = mashText(findChild(arg.parentNode, "type", False).next())
				except StopIteration:
					pass
				try:
					a.default = mashText(findChild(arg.parentNode, "defval", False).next())
				except StopIteration:
					pass
				m.args += [a]
			for ddesc in findChild(member, "detaileddescription", False):
				for item in findChild(ddesc, "parameteritem"):
					try:
						argname = mashText(findChild(item, "parametername").next()).strip()
						argdescription = mashText(findChild(item, "parameterdescription").next()).strip()
						for a in m.args:
							if a.name == argname:
								a.description = argdescription
					except StopIteration:
						pass
			if m.name not in o.methods:
				o.methods[m.name] = m
				if options.v:
					print "\t%s%s" % (m.name, " (POISON)" if m.poison else "")
					if m.args:
						print "\t\t%s" % m.args
					if options.v and m.description:
						print "\t\t%s" % m.description
	objects[o.name] = o
	
savedobj = []
for _o in sorted(objects.keys()):
	o = objects[_o]
	if not o.poison:
		savedobj += ["\"%s\"" % o.name]
		obase = path_for([base, o.name])
		subprocess.call(["mkdir", "-p", obase])
		ofile = path_for([obase, "index.txt"])
		print ofile
		ostr = ""
		ostr += "class %s\n" % o.name
		if o.description:
			ostr += "%s\n" % o.description
		if o.methods:
			ostr += "\nMethods: "
			first = True
			for _m in sorted(o.methods.keys()):
				if not o.methods[_m].poison:
					if first:
						first = False
					else:
						ostr += ", "
					ostr += "\"%s:%s\"" % (o.name, _m)
		dump(ofile, ostr)
		for _m in sorted(o.methods.keys()):
			m = o.methods[_m]
			if not m.poison:
				mfile = path_for([obase, "%s.txt" % m.name])
				print mfile
				mstr = ""
				mstr += "%s:%s(" % (o.name, _m)
				mstr += ", ".join([a.name for a in m.args])
				mstr += ")\n";
				if m.description:
					mstr += "%s\n" % m.description
				if m.args:
					mstr += "\nArguments:"
					for arg in m.args:
						mstr += "\n\t"
						if arg.type:
							mstr += "%s " % arg.type
						mstr += "%s" % arg.name
						if arg.default:
							mstr += " (optional; default %s)" % arg.default
						if arg.description:
							mstr += "\n%s" % arg.description
				dump(mfile, mstr)

dump(path_for([base, "classes.txt"]), "Known classes: %s" % ", ".join(savedobj))
