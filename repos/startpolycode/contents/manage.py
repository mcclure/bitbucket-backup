#!/usr/bin/python

# Script for maintaining a "Polycode" symlink within your project dir, such that
# when you switch between revs of your project the script will sync the symlink
# to the version of Polycode you were using at that time.

# Run without arguments for help.

# Notes: In order for this to work, you will need to add "current_polycode.txt"
# (which the script creates) to your repository. Also, add the following to your
# .hg/hgrc file to make sure the syncing is enforced:
#	[hooks]
#	precommit.polycodemanage = ./manage.py test

# Version 1.0.3
# Written by A. McClure -- http://runhello.com

import sys
import os
import re
import subprocess
import shutil

flags = []
cmds = []
returncode = 0
need_help = False

# Some important constants
current_polycode = "current_polycode.txt"
polycode_dir = "Polycode"
polycode_installpath = "Release/Darwin"
polycode_working_dir = "../Polycode"
polycode_products_dir = "../Polycode-product"
polycode_url = "git+ssh://git@github.com/ivansafrin/Polycode.git"

# Parse command line
# TODO: Don't parse the command line myself, that's silly.
nodash = re.compile("^-+")
myname = sys.argv[0]
for arg in sys.argv[1:]:
	undashed = nodash.sub("", arg)
	if undashed == arg:
		cmds.append(undashed)
	else:
		flags.append(undashed)
def have_flag(a):
	return a in flags

# Utilities

def backtick(args, cwd = None):
	p = subprocess.Popen(args, stdout=subprocess.PIPE, cwd=cwd)
	return p.stdout.read()
def hg_id_for(dir):
	return backtick(["hg", "id"], dir)
def hg_id():
	try:
		f = open(path_for([polycode_dir, current_polycode]))
		i = f.read()
		f.close()
		return i
	except IOError:
		return hg_id_for(polycode_dir)
def cur_id():
	f = open(current_polycode)
	i = f.read()
	f.close()
	return i
def just_id(i):
	return i.split(" ")[0]
def id_clean(i):
	return not("+" in i)
def path_for(seq):
	if not isinstance(seq, str):
		seq = "/".join(seq) # Construct path
	return seq
def p_link(to):
	return subprocess.call(["ln", "-s", "-f", "-h", path_for(to), polycode_dir])
def mkdir(at):
	return subprocess.call(["mkdir", "-p", path_for(at)])
def repo_clone():
	print "Checking out a new Polycode."
	if subprocess.call(["hg", "clone", polycode_url, polycode_working_dir]):
		print "ERROR: Failed to check out Polycode."
		return 1
	return 0
def repo_update(rev = None):
	cmd = ["hg","up"]
	if rev:
		cmd += [rev]
	if subprocess.call(cmd, cwd=polycode_working_dir):
		print "ERROR: Updating working directory for build failed (somehow?)."
		return 1
	return 0
def release_dir():
	return [polycode_working_dir, polycode_installpath]
def need_load_error():
	print "ERROR: No %s dir?\nTry:\n\t%s load\nOr:\n\t%s detach" % (polycode_dir, myname, myname)
def need_save_error():
	print "ERROR: No %s file?\nTry:\n\t%s save" % (current_polycode, myname)
def local_modifications_error():
	print "ERROR: Local modifications in %s\nTry:\n\tcd %s\n\tEDITOR=vi hg commit\n\tcd ..\n\t%s save" % (polycode_dir, polycode_dir, myname)
def cmake_in(dir, extra_args = []):
	return subprocess.call(["cmake", "-G", "Xcode"] + extra_args + [".."], cwd=dir)
def xcodebuild_in(dir, target):
	return subprocess.call(["xcodebuild", "-target", target, "-configuration", "Release"], cwd=dir)

# Command implementations

def cmd_test():
	ci = None
	hi = None
	# Look up claimed current id
	try:
		ci = just_id(cur_id())
	except IOError:
		need_save_error()
		return 1
	# Look up de facto current id
	try:
		hi = just_id(hg_id())
	except OSError:
		need_load_error()
		return 1
	# Scrub for local modifications
	# FIXME: If a + sign ever somehow gets into a build product current_polycode.txt, this will give an unhelpful message.
	if not id_clean(hi):
		local_modifications_error()
		return 1
	# Check for id mismatch
	if ci != hi:
		print "ERROR: %s out of date\nTry:\n\t%s save" % (current_polycode, myname)
		return 1
	return 0
	
def cmd_save():
	try:
		# Load and sanity-check de facto current id
		hi = hg_id()
		if not id_clean(hi) and not have_flag("f"):
			local_modifications_error()
			print "Or call with -f to save anyway"
			return 1
		# Write current_polycode.txt
		f = open(current_polycode, "w")
		f.write(hi)
		f.close()
	except OSError:
		need_load_error()
		return 1
	return 0
	
def cmd_build():
	def typical_build(path, extra_args = [], also_install = True):
		build = path_for(path)
		mkdir(build)
		cmake_in(build, extra_args)
		xcodebuild_in(build, "ALL_BUILD")
		if also_install:
			xcodebuild_in(build, "install")

	# Clean
	if have_flag("q"):
		print "Cleaning previous build."
		def do_rmtree(path):
			path = path_for(path)
			if not path or len(path) <= 3: # Just in case...
				return
			print "Deleting %s" % (path)
			shutil.rmtree(path, True)
		do_rmtree([polycode_working_dir, "Dependencies", "Build"])
		do_rmtree([polycode_working_dir, "Build"])
		do_rmtree([polycode_working_dir, "Release"])
		do_rmtree([polycode_working_dir, "Standalone", "Build"])

	# Build dependencies
	if have_flag("d"):
		print "Building dependencies."
		typical_build([polycode_working_dir, "Dependencies", "Build"], [], False)

	# Build main library
	print "Building Polycode."
	typical_build([polycode_working_dir, "Build"], ["-DPOLYCODE_BUILD_PLAYER=1", "-DPOLYCODE_INSTALL_PLAYER=1"] if have_flag("p") else [])

	# Build player
	if have_flag("p"):
		typical_build([polycode_working_dir, "Standalone", "Build"])
	
def cmd_load(): # TODO: Take an argument
	# Get current_polycode id
	try:
		ci = just_id(cur_id())
	except IOError:
		print "ERROR: No %s file?\nTry:\n\t%s rebuild\n\t%s detach\n\t%s save" % (current_polycode, myname, myname, myname)
		return 1
		
	# Check for local modifications
	if not id_clean(ci) and not have_flag("f"):
		print "ERROR: Saved Polycode version %s had local modifications,\nwhich won't be possible to recover.\nCall again with -f to load anyway" % ci
		return 1
		
	# Check to see if we already have the revision we need
	desired_path = path_for([polycode_products_dir, ci])
	if not os.path.isdir(desired_path):
		# We don't; build it
		if not have_flag("b"):
			print "ERROR: Don't have a product built for version %s.\nTry again with -b -d to build one (could take awhile)." % ci
			return 1
		print "Building new product for version %s." % ci
		
		# Check to see if we have a Polycode scratch dir to build from
		if not os.path.isdir(polycode_working_dir):
			if repo_clone():
				return 1
		else:
			if not id_clean(just_id(hg_id_for(polycode_working_dir))):
				print "ERROR: Working dir %s has local modifications; script can't build over that. Try:\n\tcd %s\n\thg revert --all\n(This is maybe destructive)" % (polycode_working_dir, polycode_working_dir)
				return 1
				
		# Grab the needed copy of Polycode
		if repo_update(ci):
			return 1
			
		# Actually build
		if cmd_build():
			print "ERROR: Build failed."
			return 1
		
		# Catalogue result
		mkdir(polycode_products_dir)
		if subprocess.call(["cp", "-R", path_for(release_dir()), desired_path]):
			print "ERROR: Couldn't copy built product to build directory"
			return 1
		new_id = hg_id_for(polycode_working_dir)
		f = open(path_for([desired_path, current_polycode]), "w")
		f.write(new_id)
		f.close()
	# Make actual symlink
	p_link(desired_path)
	
def cmd_rebuild():
	if not os.path.isdir(polycode_working_dir):
		if repo_clone():
			return 1
	return cmd_build()

# Actual action

if len(cmds) == 1:
	arg = cmds[0]
	if arg == 'detach':
		returncode = p_link(release_dir())
	elif arg == 'save':
		returncode = cmd_save()
	elif arg == 'load':
		returncode = cmd_load()
	elif arg == 'test':
		returncode = cmd_test()
	elif arg == 'rebuild':
		returncode = cmd_rebuild()
	elif arg == 'info':
		try:
			subprocess.call(["ls", "-l", polycode_dir])
			print
			hi = just_id(hg_id())
			print "Building against Polycode revision %s%s\n" % (hi, " (with local modifications)" if not id_clean(hi) else "")
		except OSError:
			pass
		returncode = cmd_test()
	else:
		need_help = True
else:
	need_help = True

if need_help:
	print
	print "Usage:"
	print "\t%s detach\nPoint '%s' symlink to the current working build" % (myname, polycode_dir)
	print "\t%s save\nRemember the current Polycode revision as %s" % (myname, current_polycode)
	print "\t%s load\nPoint '%s' symlink to the revision in %s" % (myname, polycode_dir, current_polycode)
	print "\t%s rebuild\nRebuild current working build" % (myname)
	print "\t%s test\nReturn 0 if safe to check in, 1 otherwise." % (myname)
	print "\t%s info\nPrint current state of things." % (myname)
	print
	print "Arguments accepted (for use with \"load\" and \"rebuild\"):"
	print "\t-b build (rebuild if necessary; use with \"load\")"
	print "\t-d dependencies (when building, build dependencies)"
	print "\t-p player (when building, build player)"
	print "\t-q qlean (when building, qlean build directory first)"
	print
	print "So for example:"
	print "\t%s load -b -d -p -q\nWould set up the current revision, cleaning then building everything." % (myname)
	print
	returncode = 1

sys.exit(returncode)
