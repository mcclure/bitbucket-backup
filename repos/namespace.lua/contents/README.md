I like Lua. I don't like how Lua handles globals. If you forget to use "local" or you misspell something when assigning, you've just made a new global. You can quarantine stuff with "require" but if the "require" assigns any globals, it stomps your namespace. Annoying. Fortunately Lua has such powerful metaprogramming you can add namespace support to the language in pure Lua. So I did.

# How to use

This repo consists of one file, [namespace.lua](namespace.lua). To use it, copy namespace.lua into your Lua project. Then put this at the top of your entry point file:

    namespace = require "namespace"

After this, at the top of each file you require, declare the namespace name:

    namespace "WHATEVER"

Every file with the same namespace key will have the same globals. Simple.

**namespace.lua only works with Lua 5.1.**

This repo has a simple [example](example). To run it, run `lua example/example.lua` from the root.

# Features

The functions in namespace.lua are documented in the file itself (it's short). However, here's an overview of some things it can do:

## Inheritance

If you say `namespace("namespace1", "namespace2")` at the top of your file, you are specifying you want "namespace1" to inherit from "namespace2". If this is the first time "namespace1" has been used, it will be created by copying all the variables out of "namespace2". The copying only happens once, variables added to "namespace1" afterward will not be visible in "namespace2".

## Single-use namespaces

If you say just `namespace()` at the top of your file, the file will get a single-use namespace not shared by any other file. If you say `namespace(nil, "namespace2")` your single-use namespace will inherit from "namespace2".

## Access namespaces as tables

Saying `local namespace1 = namespace.space("namespace1")` will store the globals table for "namespace1" as a table. You can inherit a namespace when you do this, with `namespace.space("ns1", "ns2")`.

## Prepared setup

Maybe you know which namespace a particular other namespace should inherit from, or you have some symbols you want to preload into the namespace. But: you don't know what order your files will be required in, or aren't sure if files will get required at all.

You can set the inheritance for a namespace ahead of time, by saying `namespace.prepare("namespace1", "namespace2")`. Then, later, you can just say `namespace "namespace1"` and it will already know "namespace1" should inherit from "namespace2".

Alternately, you can say

    namespace.prepare("namespace1", nil, function(ns1) -- No inherit
    	ns1.aglobal = 5
    end)

This doesn't create "namespace1", but it prepares the function so that it will be called (and assign the global variable "aglobal" into namespace1) the first time "namespace1" is used.

## Safe require

Maybe you need to require a lua file you did not write, and you're afraid it might set some globals. If you let namespace.lua do the requiring for you, with `namespace.require("something")`, then something.lua will be required but something.lua (and any new files something.lua requires) will be executed in a new, quarantined single-use namespace.

You can also specify a namespace for your required file to run in, and an inherit namespace as well, with `namespace.require("something", "namespace1", "namespace2")`

# Advanced features

That should be all you need! But if you want to get fancy...

## Changing the origin

If you create a namespace, but don't specify a namespace to inherit from, the new namespace will inherit from the Lua "default environment"-- that is, [getfenv(0)](http://lua-users.org/wiki/EnvironmentsTutorial), the globals table which is set by default at the start of each file (before you call `namespace()`).

This can be a problem because if you've been using `require()` instead of `namespace.require()`, those files could be polluting the default environment with global symbols you don't want. If however you call:

    namespace.origin()

It will set an "origin" table which namespace() will inherit-by-default from instead of the Lua default environment. If you call `namespace.origin()` without any arguments, it will set the origin by making a **copy** of the current default environment; you can also pass in a string name to set a particular namespace as the origin.

The way I recommend using namespace.lua is setting up all your namespaces in your entry point file before you do anything else. But if you don't do it that way, it's a good idea to call `namespace.origin()` right after saying `namespace = require "namespace"` so when you start creating namespaces later, they don't pick up symbols you don't expect.

## "Unpolluting" the default environment

Let's say, after setting the origin, you call vanilla require() a bunch and it pollutes the default environment. If you call:

    namespace.unpollute()

It will replace the default environment with a copy of the origin.

I don't think there's actually a good use for this feature. If you have been using `namespace()` and `namespace.require()`, you should never need it.

## Raw tables

In situations where you pass in a namespace name-- for example, the "inherit" argument on any function, or the argument to `namespace.origin()`-- you can pass in a table, and that raw table will be used. I do not recommend doing this unless you're already familiar with `setfenv()` and you really known what you're doing. If you do this clumsily you can do things like create a namespace where `print` and `require` don't exist. 

# Changelog

**Version 1.1:** Allow a prepare() namespace to inherit from another prepare() namespace; fixes to error messages  
**Version 1.0:** Initial release

# License

namespace.lua was written by Andi McClure <<andi.m.mcclure@gmail.com>>. It is available to you under the zlib license, which I interpret to mean that if you redistribute namespace.lua as source code you must include a credit to me, but if you package it into some kind of executable binary with source inaccessible you are not bound by this requirement.

	Copyright (c) 2019 Andi McClure

	This software is provided 'as-is', without any express or implied
	warranty. In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
	   appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
	   misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.

If this license does not work for you for some reason, you may at your option use namespace.lua under the MIT license, or like... email me, or something, I don't know.