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

# Version 1.0.21
# Written by Andi McClure -- http://runhello.com

import sys
import os
import re
import subprocess
import platform
import shutil
import optparse

returncode = 0

# Some important constants
current_polycode = "current_polycode.txt"
host_platform = platform.system()
target_platform = host_platform
polycode_dir = "Polycode"
polycode_configuration = "Release"
polycode_working_dir = "../Polycode"
polycode_products_dir = "../Polycode-product"
polycode_url = "https://bitbucket.org/runhello/polycode"

# Command line help. Notice %% on lines which use string modulo
help  = "\n"
help += "\t%prog install\n1st time run. Setup precommit hook, then \"load -b -d --bindings-only --no-tools\"\n"
help += "\t%%prog detach\nPoint '%s' symlink to the current working build\n" % (polycode_dir)
help += "\t%%prog save\nRemember the current Polycode revision as %s\n" % (current_polycode)
help += "\t%%prog load\nPoint '%s' symlink to the revision in %s\n" % (polycode_dir, current_polycode)
help += "\t%prog rebuild\nRebuild current working build\n"
help += "\t%prog test\nReturn 0 if safe to check in, 1 otherwise.\n"
help += "\t%prog info\nPrint current state of things.\n"
help += "\n"
help += "Arguments accepted (for use with \"load\" and \"rebuild\"):\n"
help += "\t-b build (rebuild if necessary; use with \"load\")\n"
help += "\t-d dependencies (when building, build dependencies)\n"
help += "\t-p player (when building, build player)\n"
help += "\t--bindings-only (when building, build Lua bindings, but not Player)\n"
help += "\t--no-tools (when building, don't build Player tools)\n"
help += "\t--target=[platform] (build for target OS [platform])\n"
help += "\t--host=[platform] (pretend we are building on target OS [platform])\n"
help += "\t--cmake-args=\"[args]\" (pass additional arguments [args] to cmake)\n"
help += "\n"
help += "So for a complicated example:\n"
help += "%prog load -b -d -q --no-tools --bindings-only --target=\"Windows\" --cmake-args=\"-DCMAKE_TOOLCHAIN_FILE=/Users/mcc/work/p/tmp/toolchain.cmake\"\n\tWould set up the current revision, cleaning then building everything,\nforce the target platform to Windows, and pass in a toolchain file."

# Parse command line
parser = optparse.OptionParser(usage=help)
for a in ["q","p","d","b","f"]: # Single letter args, flags
	parser.add_option("-"+a, action="store_true")
for a in ["bindings-only", "no-tools", "debug"]: # Long args, flags
	parser.add_option("--"+a, action="store_true")
for a in ["target", "host", "cmake-args"]: # Long args with arguments
	parser.add_option("--"+a, action="store")
(options, cmds) = parser.parse_args()
def flag(a):
	try:
		return getattr(options, a)
	except AttributeError:
		return None

# Extract and sanity-check platforms
if flag("target"):
	target_platform = flag("target")
if flag("host"):
	host_platform = flag("host")
bindings = flag("p") or flag("bindings_only")
myname = sys.argv[0]
	
supported_platforms = ["Linux", "Darwin", "Windows"]
if host_platform not in supported_platforms or target_platform not in supported_platforms:
	print "ERROR: Building on host platform %s for target platform %s\nOnly the following platforms are supported currently: %s" % (host_platform, target_platform, ", ".join(supported_platforms))
	sys.exit(1)
if host_platform == "Windows":
	print "WARNING: Building on Windows has never been tested with this script."

# Utilities

def backtick(args, cwd = None):
	p = subprocess.Popen(args, stdout=subprocess.PIPE, cwd=cwd)
	return p.stdout.read()
def have_tool(toolcmd, toolname, toolurl):
	try:
		backtick([toolcmd, "--version"])
		return True
	except OSError:
		print "ERROR: You need to install a program called '%s' for this script to work.\n\nTry visiting: %s" % (toolname, toolurl)
		return False
def have_hg():
	return have_tool("hg","mercurial","http://mercurial.selenic.com/")
def have_cmake():
	return have_tool("cmake","cmake","http://www.cmake.org/")
def ply_okay():
    if not bindings: # Fine to not have ply if you aren't doing bindings
        return True
    try:
        import ply
    except ImportError:
        print "ERROR: You need to install a python module called 'ply' for this script to work.\n\nTry running: `sudo easy_install ply`"
        return False
    return True
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
	return i.split()[0]
def id_clean(i):
	return not("+" in i)
def path_for(seq):
	if not isinstance(seq, str):
		seq = "/".join(seq) # Construct path
	return seq
def p_link(to):
	if host_platform == "Darwin":
		extra_args = ["-h"]
	else:
		extra_args = ["-n"]
	return subprocess.call(["ln", "-s", "-f"] + extra_args + [path_for(to), polycode_dir])
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
def polycode_installpath(): # Always install to Release
	return path_for(["Release", target_platform])
def release_dir():
	return [polycode_working_dir, polycode_installpath()]
def build_dir():
	return "Build_%s" % (target_platform)
def need_load_error():
	print "ERROR: No %s dir?\nTry:\n\t%s load\nOr:\n\t%s detach" % (polycode_dir, myname, myname)
def need_save_error():
	print "ERROR: No %s file?\nTry:\n\t%s save" % (current_polycode, myname)
def local_modifications_error():
	print "ERROR: Local modifications in %s\nTry:\n\tcd %s\n\tEDITOR=vi hg commit\n\tcd ..\n\t%s save" % (polycode_dir, polycode_dir, myname)
def cmake_args():
	if flag("cmake_args"):
		return [flag("cmake_args")]
	return []
if target_platform == "Darwin":
	def cmake_in(dir, extra_args = []):
		return subprocess.call(["cmake", "-G", "Xcode"] + extra_args + cmake_args() + [".."], cwd=dir)
	def build_in(dir, target, makecmd = None):
		return subprocess.call(["xcodebuild", "-target", target, "-configuration", polycode_configuration], cwd=dir)
else: # Linux, Windows
	def cmake_in(dir, extra_args = []):
		return subprocess.call(["cmake", "-G", "Unix Makefiles", "-DCMAKE_BUILD_TYPE="+polycode_configuration] + extra_args + cmake_args() + [".."], cwd=dir)
	def build_in(dir, target, makecmd = None):
		return subprocess.call(["make"] + ([makecmd] if makecmd else []) + ["VERBOSE=1"], cwd=dir)

# Command implementations

def cmd_test():
	if not have_hg():
		return 1
		
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
	if not have_hg():
		return 1
		
	try:
		# Load and sanity-check de facto current id
		hi = hg_id()
		if not id_clean(hi) and not flag("f"):
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
	if not (have_hg() and have_cmake() and ply_okay()):
		return 1
		
	def typical_build(path, extra_args = [], also_install = True):
		build = path_for(path)
		mkdir(build)
		if cmake_in(build, extra_args):
			print "ERROR: cmake failed";
			sys.exit(1) # TODO: try to snake back out through the stack cleanly...
		if build_in(build, "ALL_BUILD"):
			print "ERROR: build failed";
			sys.exit(1)
		if also_install:
			build_in(build, "install", "install")

	# Clean
	if flag("q"):
		print "Cleaning previous build."
		def do_rmtree(path):
			path = path_for(path)
			if not path or len(path) <= 3: # Just in case...
				return
			print "Deleting %s" % (path)
			shutil.rmtree(path, True)
		if flag("d"):
			do_rmtree([polycode_working_dir, "Release", target_platform]) # Only clear product dir if cleaning EVERYTHING
			do_rmtree([polycode_working_dir, "Dependencies", build_dir()])
		do_rmtree([polycode_working_dir, build_dir()])

	# Build dependencies
	if flag("d"):
		print "Building dependencies."
		typical_build([polycode_working_dir, "Dependencies", build_dir()], [], False)

	# Build main library
	print "Building Polycode."
	typical_build([polycode_working_dir, build_dir()],
		(["-DPOLYCODE_BUILD_PLAYER=1", "-DPOLYCODE_INSTALL_PLAYER=1"] if flag("p") else []) +
		(["-DPOLYCODE_BUILD_BINDINGS=1"] if bindings else []) +
		(["-DPOLYCODE_BUILD_TOOLS=0"] if flag("no_tools") else []) +
		(["-DPOLYCODE_DEBUG_SYMBOLS=1"] if flag("debug") else []) )
		
	# Copy Extras
	print "Copying extras."
	extra = ["Modules/Bindings/2DPhysics/Source/Physics2DLUA.cpp",
	         "Modules/Bindings/3DPhysics/Source/Physics3DLUA.cpp",
			 "Bindings/Scripts/create_lua_library/CppHeaderParser.py",
			 "Bindings/Scripts/create_lua_library/create_lua_library.py",]
	extra_dir = path_for(release_dir() + ["Framework", "Extra"])
	mkdir(extra_dir)
	for e in extra:
		e = path_for([polycode_working_dir, e])
		subprocess.call(["cp", e, extra_dir])
	
	# That's all
	print "Done."
	
def cmd_load(): # TODO: Take an argument
	if not have_hg():
		return 1
		
	# Get current_polycode id
	try:
		ci = just_id(cur_id())
	except IOError:
		print "ERROR: No %s file?\nTry:\n\t%s rebuild\n\t%s detach\n\t%s save" % (current_polycode, myname, myname, myname)
		return 1
		
	# Check for local modifications
	if not id_clean(ci) and not flag("f"):
		print "ERROR: Saved Polycode version %s had local modifications,\nwhich won't be possible to recover.\nCall again with -f to load anyway" % ci
		return 1
		
	# Check to see if we already have the revision we need
	desired_path = path_for([polycode_products_dir, target_platform, ci])
	if not os.path.isdir(desired_path):
		# We don't; build it
		if not flag("b"):
			print "ERROR: Don't have a product built for version %s.\nTry again with -b -d to build one (could take awhile)." % ci
			return 1
		if not (have_cmake() and ply_okay()):
			return 1
		print "Building new product for version %s." % ci
		
		# Check to see if we have a Polycode scratch dir to build from
		if not os.path.isdir(polycode_working_dir):
			if repo_clone():
				return 1
		else:
			if not id_clean(just_id(hg_id_for(polycode_working_dir))) and not flag("f"): # Redundant?
				print "ERROR: Working dir %s has local modifications; script can't build over that. Try:\n\tcd %s\n\thg revert --all\n(This is maybe destructive)" % (polycode_working_dir, polycode_working_dir)
				return 1
				
		# Grab the needed copy of Polycode
		if repo_update(ci):
			print "Couldn't find revision %s locally. Will pull server and try again." % (ci)
			if subprocess.call(["hg", "pull", polycode_url], cwd=polycode_working_dir):
				print "ERROR: Failed to pull new versions from Polycode."
				return 1
			if repo_update(ci):
				return 1
			
		# Actually build
		if cmd_build():
			print "ERROR: Build failed."
			return 1
		
		# Catalogue result
		mkdir([polycode_products_dir, target_platform])
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
	
def cmd_install():
	if subprocess.call(["grep", "-q", "precommit\.polycodemanage", ".hg/hgrc"]): # Will return 1 if not found
		print "Installing precommit hook. Mercurial will now warn you on commit if your current_polycode.txt does not match the version of Polycode you are compiling against. To turn this off, edit .hg/hgrc."
		try:
			f = open(".hg/hgrc", "a")
			f.writelines(["\n","[hooks]\n","precommit.polycodemanage = ./manage.py test\n"])
			f.close()
		except IOError:
			print "ERROR: Could not set up precommit hook."
			return 1
	else:
		print "Precommit hook already installed."
		
	print "Setting up '%s' symlink." % (polycode_dir)
	options.b = True
	options.d = True
	options.bindings_only = True
	options.no_tools = True
	return cmd_load()

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
	elif arg == 'install':
		returncode = cmd_install()
	else:
		parser.error("Don't recognize command %s" % arg)
else:
	parser.error("Too many commands? %s" % cmds)

sys.exit(returncode)
