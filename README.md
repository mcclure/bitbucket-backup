This is a "flat" backup of the [BitBucket account](https://bitbucket.org/runhello/) of Andi McClure AKA [Run Hello](https://runhello.com) as of 2020-02-02. In other words this is about ten years of my life in open source software and video games. Bitbucket is deleting all its Mercurial repos, so I'm making a safety backup here in case I can't find another Mercurial host in time.

For each public repo the most recent commit of both the wiki (the original public landing page) and the main repo (in a "contents" page) are included. For some repos more than one contents folder is present because more than one tag was relevant.

# Building

## Restoring ufo "bin"

Several of the repos are based on something called UFO, and it contains a large "bin/" directory containing precompiled binaries. Since this is the same for every "ufo" repo, I deleted the unnecessary extra bin/s. To restore them run this in Bash:

    cp repos/ufo/contents/bin repos/ufo/contents-gamecity/bin
    cp repos/ufo/contents/bin repos/ufo/contents-indiecade/bin
    cp repos/ufo/contents/bin repos/bodyhack-test/contents/bin
    cp repos/ufo/contents/bin repos/nanogunk/contents/bin
    cp repos/ufo/contents/bin repos/twotris/contents/bin

## Polycode

Many of the projects are based on a library called Polycode. As a sort of primitive workalike of subrepos, these projects have a script named "manage.py" that records the version (as an hg revision in "current_polycode.txt") of Polycode that the project was meant to be built with, and automatically checks it out from BitBucket to build it. Problem: The entire point of this repository is BitBucket is going away, and the official Polycode repo by Ivan Safrin will not help you because it uses git (not hg).

To make the Polycode projects buildable from this repo alone, I've stored diffs (in git format) between the version of Polcode in the repo (`1c18a1fa508`) and the versions in the various current_version.txts in the folder [repos/polycode/diffs](repos/polycode/diffs). The manage.py script won't help you completely, but you can apply those diffs manually and at least have a buildable repo with the interface the project expects. Good luck.

(Versions `02dd5d59ea73`, `558e26de5cd0` and `a7c7c61a22a8` seem to be permanently lost.)

# Contents

*For completeness, some links to Github repos are sprinkled in below.*

## Websites

* [emu-coop](https://mcclure.github.io/emu-coop/): A multiplayer hack for 1-player Super Nintendo games
* [qgcon-2018](https://mcclure.github.io/qgcon-2018/): Resources and a video from a talk I gave on making small programming languages.
* [mermaid-lovr](https://mcclure.github.io/mermaid-lovr/): A set of resources for the LÖVR game engine.

## LÖVR projects

[LÖVR](http://lovr.org) is an open source VR engine originally created by Bjorn Swenson. I've been using it since 2018. See [my LÖVR resources page here](https://mcclure.github.io/mermaid-lovr/).

* [Lovr](https://github.com/mcclure/lovr): My fork of LÖVR on GitHub (no important differences from the main repo).
* [lovr-oculus-mobile](https://github.com/mcclure/lovr-oculus-mobile): The main site for oculus mobile support using LÖVR (maintained by me).
* [lodr](https://github.com/mcclure/lodr): A hot code reloading app for LÖVR. 
* [lovr-ent](http://github.com/bjornbytes/lovr-ent): A "starting point project", sample code containing all the Lua helper code I include in all my LÖVR projects. Some of these are usable in non-LÖVR Lua engines.

## Emily language

[Emily](https://emilylang.org) was an experimental programming language I designed and worked on between about 2014 and 2017.

### Interpreter

* [emily](repos/emily): In-progress interpreter for a programming language I designed.
* [test-emily-game](repos/test-emily-game): Experimental branch of emily (0.3b development is effectively occurring here)
* [emily-python](repos/emily-python) aka Emily2: Experimental next-generation (post 0.3) prototype of Emily. Not yet documented.


### Other materials

* [llvm-practice](repos/llvm-practice): Code for my "No Compiler" blog posts, written to learn about compilers.
* [make.py](repos/makepy): Reimplementation of the `make` tool in Python, intended for use as Emily build tool
* [ppx_const](repos/ppx_const): A ppx syntax extension for the OCaml programming language. Adds a compile-time "if" statement.
* [emily-objc](repos/emily-objc): Abandoned first-prototype version of Emily. Suggest just ignoring.

## Jumpcore projects

Jumpcore is my homegrown game engine. I started developing it in 2008 and use it for all my C++ game projects.

### Engine

* [jumpcore](repos/jumpcore): a get-started kit for portable C++ games

### Games

* [jumpman_public](repos/jumpman_public) aka Jumpman
* [drumcircle](repos/drumcircle)
* [body](repos/body) aka You Don't Fit
* [footsteps](repos/footsteps) aka My Own Footsteps
* [pong](repos/pong) aka pongpongpongpongpongpongpongpong
* [splinecraft](repos/splinecraft)
* [ocean](repos/ocean) aka You Will Die Alone At Sea
* [blit](repos/blit/wiki/Home) aka Scrunch

### Unfinished

* [impression](repos/impression)
* [dot](repos/dot)
* [rtarot](repos/rtarot) aka Reverse Tarot
* [jpeg](repos/jpeg): Realtime JPG damager
* [bodyhack](repos/bodyhack)
* [nomadtest](repos/nomadtest)
* [geodesic](repos/geodesic/wiki/Home) aka Icosa

## Polyconsole projects

Polycode is a game engine originally developed by Ivan Safrin. I used it between about 2011 and 2014.

### Engine

* [polycode](repos/polycode): My fork of Ivan Safrin's [Polycode](http://polycode.org/) game engine 
* [polyconsole](repos/polyconsole): My Polycode "template project", a replacement for the Polycode Player which adds additional capabilities

### Games

* [universe](repos/universe) aka Markov Space
* [rad](repos/rad) aka Xaxxaxoxax
* [nauts](repos/nauts) aka Luanauts
* [breathe](repos/breathe)
* [molyjam](repos/molyjam) aka The Shadowland Prophesy
* [evo](repos/evo) aka The World Hates You
* [fps](repos/fps) aka 7DRL
* [brainfarm](repos/brainfarm)
* [voxcut_public](repos/voxcut_public) which contains "pxswap" and "cs/1"
* [whoop](repos/whoop) aka Devil's Chord
* [dontbreathe](repos/dontbreathe) aka Sun Sets
* [launcher](repos/launcher) aka Sweet Nothings
* [fall](repos/fall)
* [iso](repos/iso) aka Responsibilities
* [lesbian](repos/lesbian) aka Lesbian Spider Mars Queens
* [portaudio_test](repos/portaudio_test)
* [geogen](repos/geogen) (Twitter avatar generator)
* [dating](repos/dating) aka He Never Showed Up
* [rainbow](repos/rainbow) aka HOWLER?
* [hcyet](repos/hcyet) aka How Can You Even Tell
* [fps2](repos/fps2) aka Four Shades of Gray
* [cymbal](repos/cymbal) aka Super Fungus Attack

### Unfinished

* [template](repos/template) Pre-Polyconsole Polycode project. Yes, the game is named "template".
* [luatest](repos/luatest) aka Unplayable Asteroids. Pre-Polyconsole.
* [hello](repos/hello) BASIC interpreter and command line
* [flipper](repos/flipper)
* [adp](repos/adp) aka A Dark Place
* [diary](repos/luatracker) (very little is here)
* [luatracker](repos/luatracker) (very little is here)
* [nervous](repos/nervous) aka The Nervous System
* [colors](repos/colors) aka Trilemma
* [sia](repos/sia) aka Death By Chocolate aka Stars
* [sharecart](repos/sharecart) aka Player Piano
* [turing](repos/turing) aka Tau
* [triangle](repos/triangle) aka L?L?L?L
* [rps](repos/rps) aka Very Fast Rock Paper Scissors
* [compass](repos/compass)

## Ufo projects

Ufo is a bundle of LuaJIT with some support libraries. I do not yet have a "trunk" repository for it.

### Games

* [ufo](repos/ufo) aka BECOME A GREAT ARTIST IN JUST 10 SECONDS

### Unfinished

* [nanogunk](repos/nanogunk)
* [twotris](repos/twotris)
* [bodyhack-test](repos/bodyhack-test)

## Twine projects

[Twine](http://twinery.org/) is an open source tool for creating interactive fiction in HTML. I used it for a few projects around 2014.

### Engine

* [twine](repos/twine): My fork of the popular Twine IF tool (contains custom templates, etc)
* [twinetools](repos/twinetools): A small macro collection
* [spool](repos/spool): Tools for Twine games with online multiplayer
* [spool-heroku](repos/spool-heroku): Heroku port of Spool

### Games

* [athena](repos/athena) aka Naked Shades
* [localtwine](repos/localtwine) contains my other Twine games

## Misc games

### Dryad.technology

* [dryad-artifact](repos/dryad-artifact): Pure-JS [dryad.technology](http://dryad.technology) Javascript experiments
* [dryad-nim](repos/dryad-nim): Nim-based [dryad.technology](http://dryad.technology) Javascript experiments

### Unfinished

* [daynight](repos/daynight) aka Day and Night
* [witch](repos/witch) aka Organ Solo
* [nim-practice](repos/daynight) aka Time Problems

## Misc projects

* [exacto](repos/exacto): Manual stencyl project editor
* [whiteboard](repos/whiteboard): A Google AppEngine collaborative pixel art app
* [badpng](repos/badpng): Artistically buggy png encoder
* [votescript](repos/votescript): CGI scripts that can be used to run an online ranked-voting poll
* [pseudogbs_au](repos/pseudogbs_au): An approximation of the GBS ("Game Boy Sound System") as a set of Mac OS X AudioUnits.
* [unitynativepluginexample](repos/unitynativepluginexample): A Unity sample project which incorporates C++ code and uses CMake to build it.
* [dts2nim](https://github.com/mcclure/dts2nim): A tool that converts TypeScript type definition files into a format that can be used by the Nim programming language. Can be used to call TypeScript code from Nim. May not work with most recent TypeScript.
* [emu-coop](https://github.com/mcclure/emu-coop/): A multiplayer hack for 1-player Super Nintendo games (project page, see [main site](https://mcclure.github.io/emu-coop/) here)
* [snes9x-coop](https://github.com/mcclure/snes9x-coop/): A build of snes9x-rr primed to run emu-coop (see [main site](https://mcclure.github.io/emu-coop/) here)
* [namespace.lua](repos/namespace.lua): A Lua library which adds a namespace feature to Lua. For Lua 5.1/LuaJIT.

## Ports and forks

### Games

* [port_epacse](repos/port_epacse) aka epacse
* [port_sword](repos/port_sword) aka Sword In Hand
* [port_rive](repos/port_rive) aka Rive Gauche

### Open source project forks

* [sproxel](repos/sproxel): A voxel editor
* [hg-git](repos/hg-git): Mercurial/git bridge (outdated)

# License

Each "repo" in the repos directory is under a different license. The license information is usually kept on the wiki page and for those repos that have license information on the wiki any files or notices in the main repo are not intended to be a license for the repo contents (for example, there's a file named JUMPCORE_LICENSE.txt in the "geodesic" repo but that's not a license for "geodesic"). Just saying this so there's no confusion.
