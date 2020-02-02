This is a set of scripts for controlling a Nintendo 64 emulator.

To run them, you need a specially modified emulator. You can find the emulator [here](https://bitbucket.org/runhello/mupen64plus-ui-python). Unfortunately, at the moment you will need to build it yourself. When you set it up, set the "scripts dir" in the preferences to this directory.

# Scripts

Here are some of the example scripts included; enter the given text into the Python console to run them:

## Mario 64

Note: Scripts only work on the US and JP releases. The game should be loaded before you try to run them.

* `load("mario/basics")`

    Loads a `mario()` and `move_mario(x,y,z)` into scope, which can be used to get and set Mario's position. See source.

* `load("mario/objects")`

    Loads the basics into scope, plus functions for viewing objects. See source.

* `script("mario/script/pshoo")`

    Shoots Mario high in the sky.

* `script("mario/script/heal")`

    Restores Mario's health to 8 pieces.

* `start("mario/run/float")`

    Causes Mario to float slowly upward eternally (he must get off the ground first however). A set() field on the script object lets you control the vector of the added movement.

* `start("mario/run/runjump")`

    Causes Mario to run forward while jumping for as long as the script runs.

* `start("mario/run/watch_position")`

    Continuously prints to log (see "Help" menu) Mario's position.

* `start("mario/run/watch_closest")`

    Continuously prints to log information about the closest object. A "quiet" boolean field on the script object controls whether the information is printed continuously or only when the nearest object changes.

* `start("mario/run/watch_heave_ho")`

    Continuously prints to log Mario's position, the position and orientation of any Heave Hos in the level, and the "launch point" (where you have to stand to get thrown) for those Heave Hos.

# Sources

The scripts use memory offsets taken from the following sources:

* [Origami64 Wiki](http://wiki.origami64.net/sm64:ram_memory_map)
* SM64Central [ROM](http://www.smwcentral.net/?p=nmap&m=sm64rom) and [RAM](http://www.smwcentral.net/?p=nmap&m=sm64ram) maps
* [James S RAM map](https://sites.google.com/site/jamesskingdom/Home/video-game-secrets-by-james-s/super-mario-64-exposed/sm64-n64-memory-map)

The Pannenkeok2012 videos, especially "Angles" and "The Science of cloning", were also invaluable.
