This is a modified version of the M64py frontend. It contains a Python console which can be used to control the emulator. To see the information for M64py itself, see [README_ORIGINAL.md](README_ORIGINAL.md).

# SETUP

To compile, run `./setup.py build_qt`.

To run, you will need copies of the mupen64plus DLLs. You will want a special version of the "core" DLL, one that has debug support. If you want to simulate input, you will also want my special fork of the "input" DLL.

What I do is:

1. Download the source to mupen64plus-core and build with `make all DEBUGGER=1 CPU=x86` (I installed MinGW using MSYS2, so I also had to add `-I/mingw32/include/binutils` to the CFLAGS and `-L/mingw32/lib/binutils/ -liberty` to the LDFLAGS in Makefile. The version I built was `d679ba0292be679` from [git](https://github.com/mupen64plus/mupen64plus-core).)
2. Download the source to my mupen64plus-input-sdl fork, which is [on Bitbucket](https://bitbucket.org/runhello/mupen64plus-input-sdl), and build it with `make all DEBUGGER=1 CPU=x86`. The version I built was `4bd1ae3b580c`.
3. Download and install M64py (the normal one) from its [website](http://m64py.sourceforge.net/). The DLLs you did not build yourself will come from here. I installed version 0.2.3.
4. If you are on Windows, copy the M64py folder (`C:\Program Files (x86)\M64Py`) somewhere other than "Program Files". This is because there in step 5 you will replace a DLL, and modern Windows security does not allow this in the "Program Files" directory.
5. Replace the input plugin in your M64py folder with the one you built in step 2.
6. Create a directory to store your scripts in.
7. Launch `m64py` out of this directory. (I have to use `./m64py --sdl2` because I have SDL2 installed.)
8. After opening, go to the Settings->Paths dialog (on first open it should bring you here automatically). Browse and set "library file" to the DLL file you built in step (1), set "Plugins Directory" and "Data Directory" to the M64py directory you set up in step (4), and set "scripts directory" to the directory you created in step (6).
9. On my system, I also had to go to the Settings->Plugins tab at this point and select my plugins manually. I picked mupen64-plus-video-rice.dll, mupen64plus-audio-sdl.dll, mupen64plus-input-sdl.dll, and mupen64plus-rsp-hle.dll.
10. Open your [mupen64plus.cfg file](http://mupen64plus.org/wiki/index.php?title=FileLocations) and change EnableDebugger to "true".	

(Note: If you use the DLLs from the website, those are 32-bit, so when following the steps above you will need to build a 32-bit mupen64plus-core and run with a 32-bit Python. Otherwise you will get confusing errors.)

# USAGE

Open the Python console using the "Python Console" option under View.

After entering a line of Python, evaluate it with alt+Enter.

# API

The following symbols will be available in scope when executing a line in the Python console, or when executing a script.

## `script( name )`

A script named *name* will be loaded from disk and executed. If *name* is an absolute path it will be executed from that path, otherwise *name* will be loaded from the scripts directory set in Settings->Paths. You can leave the file extension off of *name* and it will figure out what you meant (`script("startup")` will successfully load `startup.py`). Scripts may return values (see below).

## `load( name )`

Same as `script( name )`, but the script will be executed in the current scope. This is good if a script defines a bunch of classes and libraries.

(Note: The scripts directory is added to `sys.path`, so you can also load things out of it using `import`.)

## `result( value )`

*(Only makes sense in a script.)* Scripts may return values by calling this function. Calling `result` will terminate the script (see `ReturningResult` below).

## `last()`

*(Only makes sense in the Python console.)* Returns the value returned by the last thing executed in the Python console. 

## `last_exception()`

*(Only makes sense in the Python console.)* If the last thing executed in the Python console failed, this returns the exception that was caught.

## `trace( msg, level=1 )`

Print a message to the m64 "Log viewer" (it's in the Help menu). `level` is optional; if present, it will set the "trace level" of the message. Messages will not be printed if their trace level is lower than the current system trace level.

## `trace_level()`

Returns the current system trace level. Default is 1.

## `set_trace_level( level )`

Sets the current system trace level. Set this to `trace_level_max` to never see any traces.

## `start( target )`

Starts a runnable script. `target` can be either a `Runnable` object (see below), or can be a string, in which case it will be treated as a script name (the script must return a `Runnable` object using `result( value )`). The runnable object will be returned. The script will run until it is stopped using `stop( target )`.

## `stop( target )`

Stops a runnable script. `target` can be either a `Runnable` object, or can be a name string, in which case all scripts started using that same name string will be stopped.

## `panic()`

Reset everything. Stop all running scripts, disable all input overrides, reset the trace level to default.

## `debug`

The M64py debug interface. This is documented in its own section below.

## `worker`

The M64py worker object.

## `settings`

The M64py settings object.

## `core`

The M64py core object.

## `Runnable`

This is a class; inherit from it if you are creating a value that can be used with `start( target )` and `stop( target )`.

When implementing a `Runnable` subclass, you may implement any of the following methods:

* `onStart()`-- will be called when `start()` is called, after object registration
* `onBoot()`-- will be called when emulation starts (ie the ROM resets)
* `onBreakpoint()`-- will be called when the emulator halts for a breakpoint
* `onBlank()`-- will be called once per frame (at vertical blank time)
* `onStop()`-- will be called when `stop()` is called, just before object deregistration

## `Buttons`

This is a class; it contains the complete state of one N64 controller. It contains the following fields, whose definition is obvious. It works with the `buttons_override` functions (see below)

* r_dpad, l_dpad, d_dpad, u_dpad, start_button, z_trig, b_button, a_button, r_cbutton, l_cbutton, d_cbutton, u_cbutton, r_trig, l_trig, reserved1, reserved2, x_axis, y_axis

## `ReturningResult`

As mentioned above, the `result()` function terminates the script. It does this by throwing the special exception type `ReturningResult`.

You usually should never need to think about this. However it does mean you should not call `result()` inside of a `try: except:`, or you will accidentally prevent the script from terminating. Instead, use `try: except Exception:`, so that `ReturningResult` will not be caught (it's a `BaseException`).

# The `debug` object

If you are writing a script, you will probably be spending most of your time interacting with this object. Here are the methods it exposes:

## `debug.check_supported()`

Throws an exception if the mupen64plus core library you are using does not support debugging (memory access and Runnable).

## `debug.check_supported_input()`

Throws an exception if the mupen64plus input library you are using does not support input overriding.

## mem_read

    debug.mem_read_8( addr, [count] )
    debug.mem_read_16( addr, [count] )
    debug.mem_read_32( addr, [count] )
    debug.mem_read_64( addr, [count] )

Read 8, 16, 32, or 64-bit values from the given N64 memory address. *count* is optional. If *count* is omitted, returns a single value. If *count* is included, reads *count* values and returns an array.

## mem_read_str

	debug.mem_read_str( addr, [count] )

Same as `debug.mem_read_8`, but returns the value as a string rather than an integer.

## mem_read_float

	debug.mem_read_float_32( addr, [count] )
	debug.mem_read_float_64( addr, [count] )

Same as `debug.mem_read_32` and `debug.mem_read_64`, but return the values as floats or doubles rather than integers.

## mem_write

    debug.mem_write_8( addr, val )
    debug.mem_write_16( addr, val )
    debug.mem_write_32( addr, val )
    debug.mem_write_64( addr, val )

Write an 8, 16, 32, or 64-bit value from the given N64 RAM address. If *val* is a list, all values from the list will be written at sequential offsets.

## mem_write_str

	debug.mem_write_str( addr, val )

Same as `debug.mem_write_8`, but *val* must be a Python string.

## mem_write_float

	debug.mem_read_float_32( addr, val )
	debug.mem_read_float_64( addr, val )

Same as `debug.mem_write_32` and `debug.mem_write_64`, but the values are input as floats or doubles rather than integers.

## Float conversion

	debug.int_to_float_32( val )
	debug.int_to_float_64( val )
	debug.float_to_int_32( val )
	debug.float_to_int_64( val )

Reinterpret integers as floats or vice versa (IE, interpret 0x42c80000 as a raw 4-byte value containing a float, and return 100.0).

## Button overrides

At any one time, the button override can either be enabled or disabled (if it is disabled, controls will be passed through from whatever gamepad you have attached). 

There are a couple of different ways to set button overrides. The easy way is this function. It has many arguments, but all are optional:

	debug.buttons_override(r_dpad=None, l_dpad=None, d_dpad=None, u_dpad=None, start_button=None, z_trig=None, b_button=None, a_button=None, r_cbutton=None, l_cbutton=None, d_cbutton=None, u_cbutton=None, r_trig=None, l_trig=None, reserved1=None, reserved2=None, x_axis=None, y_axis=None, reset=False, controller=0)

When you call this function, button overrides will be turned on. Any argument passed in which is the name of a button will be set to the value you passed in (you can pass in either a number, or `True` or `False`). The button will be set for the controller you passed in, or if you did not pass in a controller it will set for the first controller. If you set the `reset` argument, then any argument you do not specifically pass in will be reset to 0 (buttons released, analog stick to neutral).

So for some examples:

* `debug.buttons_override()` enables button overriding, and does nothing else.
* `debug.buttons_override(a_button=True, b_button=False)` enables button overriding, and also sets the state of the A button to "1" (pressed down) and the state of the B button to "0" (released) on the first controller. The state of other buttons, such as the Z trigger, will not be affected.
* `debug.buttons_override(reset=True)` will enable button overriding and also reset the state of the first controller to everything-neutral.

To disable the button override and restore human control, call

	debug.buttons_override_disable()

Disabling the button overrides will also reset the state of all buttons to neutral.

Instead of using the `buttons_override` function, you can also directly get and set the `Buttons` structure that debug is storing the button state as. To get the entire button state for a controller, you can say:

	debug.buttons_override_struct_last(controller=0)

Or to set an entire button state, use:

    debug.buttons_override_struct(current, controller=0):

For an example all together:

	buttons = debug.buttons_override_struct_last()
	buttons.r_dpad = 1
	debug.buttons_override_struct(buttons)

The `controller` argument is, again, optional for both of these functions.

# LICENSE

As described in README_ORIGINAL, M64py is GPL. However, at your option, instead of the GPL, you may use any work in this source tree by Andi McClure under the terms of the following license:

> Copyright (C) 2016 Andi McClure
> 
> Permission is hereby granted, free of charge, to any person obtaining
> a copy of this software and associated documentation files (the
> "Software"), to deal in the Software without restriction, including
> without limitation the rights to use, copy, modify, merge, publish,
> distribute, sublicense, and/or sell copies of the Software, and to
> permit persons to whom the Software is furnished to do so, subject to
> the following conditions:
>
> The above copyright notice and this permission notice shall be
> included in all copies or substantial portions of the Software.
>
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
> EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
> MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
> IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
> CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
> TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
> SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

(Obviously this assumes you have first cleanly separated the code from the GPL code with which it is currently intertwined.)