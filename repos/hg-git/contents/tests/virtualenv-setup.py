#!/usr/bin/env python
#
# virtualenv-setup.py
#
# Created 2011 by A. McClure, you may consider this file public domain
#
# This script creates a series of virtualenvs, with the specified versions of mercurial and dulwich
# installed in each. After this you can run your newly created mercurials like envs/1.8/bin/hg
# The intent is to allow you to rapidly test hg-git changes in several different versions of hg quickly.
#
# Before running this, you should have a system with hg, hg-git, and virtualenv already configured.
# Notice the script does NOT set up hg-git in the virtualenvs; you should set up your own ~/.hgrc to
# point to the version of hg-git you want to test.
#
# TODO: Add a cmd to invoke ./run-tests.py for all envs and prepare some kind of report on the results.

import sys
import os
import subprocess

args = sys.argv[1:]
owd = os.getcwd()
do_clean = 'clean' in args
do_build = 'build' in args

versions = ['1.9.2', '1.9', '1.8.4', '1.8']

dulwich_version = "dulwich-0.8.0"

# I'm not doing any path correctness right now, this will probably break hard on Windows
scratch_path = "ves_scratch/"
mercurial_path = scratch_path + "hg"
dulwich_path = scratch_path + "dulwich"
baserev_path = "envs/"

known_env = []

# At what path would an env with this hg tag be stored?
def rev_path(rev):
    return baserev_path + str(rev)

# Get a scratch copy of the mercurial repository.
def ensure_mercurial():
    if not os.path.isdir(mercurial_path):
        print("\t-- Downloading mercurial")
        subprocess.call(["mkdir", "-p", scratch_path])
        subprocess.call(["hg", "clone", "http://selenic.com/hg", mercurial_path])
        
# Get a copy of the dulwich repository and move it to the single desired version.
# Notice it would be easy to test for a range of dulwiches as is currently done with mercurial.
def ensure_dulwich():
    if not os.path.isdir(dulwich_path):
        print("\t--Downloading dulwich")
        subprocess.call(["mkdir", "-p", scratch_path])
        subprocess.call(["hg", "clone", "git://git.samba.org/jelmer/dulwich.git", dulwich_path])
        subprocess.call(["hg", "-R", dulwich_path, "up", dulwich_version])
    
# Set up an environment for the given version of hg.
def ensure_env(version):
    path = rev_path(version)
    if not os.path.isdir(path):
        print("\t-- Setting up version %s" % version)
        ensure_mercurial()
        ensure_dulwich()
        subprocess.call(["mkdir", "-p", baserev_path])
        
        print("\t-- Making virtualenv")
        subprocess.call(["virtualenv", path])
        
        env_python = "../../" + path + "/bin/python"
        print("\t-- Installing dulwich")
        os.chdir(dulwich_path)
        subprocess.call([env_python, "setup.py", "install"])
        os.chdir(owd)
        
        print("\t-- Loading appropriate mercurial")
        subprocess.call(["hg", "-R", mercurial_path, "up", version])
        print("\t-- Installing mercurial")
        os.chdir(mercurial_path)
        subprocess.call([env_python, "setup.py", "install"])
        os.chdir(owd)
    known_env.append(path)

# Act
if do_clean:
    # The correct thing to do is import shutil; shutil.rmtree()
    # But that's scary! For now I'm going to make the user copy and paste this themselves.
    print("\tEnter these:")
    print("rm -rf " + scratch_path)
    print("rm -rf " + baserev_path)
elif do_build:
    for version in versions:
        ensure_env(version)
    
    print("\t-- Your new mercurials:\n")
    known_env.sort()
    for env in known_env:
        print(owd + "/" + env + "/bin/hg")
else:
    print("Usage:")
    print("\t./virtualenv-setup.py clean")
    print("\t./virtualenv-setup.py build")