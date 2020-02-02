# -*- coding: utf-8 -*-
# Author: Andi McClure <andi.m.mcclure@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import os
import sys
from m64py.core.defs import *
from m64py.loader import load, unload_library
import ctypes as C
import m64py.core.core as CORE

# Kludge: Runnables maybe shouldn't be so tightly integrated with the debug class; they're
# more part of pythonconsole than the core debug support.

debug_singleton = None # Protect debug from collection since it owns C callbacks
voidcall = C.CFUNCTYPE(None)

class Runnable:
    def __init__(self):
        pass

    def start(self):        
        try:
            self.hasBoot = self.onBoot is not None
        except AttributeError:
            self.hasBoot = False

        try:
            self.hasBreakpoint = self.onBreakpoint is not None
        except AttributeError:
            self.hasBreakpoint = False

        try:
            self.hasBlank = self.onBlank is not None
        except AttributeError:
            self.hasBlank = False

        self.id = debug_singleton.register(self) # Register for events

        try:
            onStart = self.onStart
        except AttributeError:
            pass
        else:
            onStart()

    def stop(self):
        try:
            onStop = self.onStop
        except AttributeError:
            pass
        else:
            onStop()
        finally:
            debug_singleton.unregister(self.id)

# Debug class implementation

null_ptr = C.POINTER(C.c_int)()

class Buttons(C.Structure): # Corresponds to struct in header file
    _fields_ = [("r_dpad", C.c_ubyte),
                ("l_dpad", C.c_ubyte),
                ("d_dpad", C.c_ubyte),
                ("u_dpad", C.c_ubyte),
                ("start_button", C.c_ubyte),
                ("z_trig", C.c_ubyte),
                ("b_button", C.c_ubyte),
                ("a_button", C.c_ubyte),
                ("r_cbutton", C.c_ubyte),
                ("l_cbutton", C.c_ubyte),
                ("d_cbutton", C.c_ubyte),
                ("u_cbutton", C.c_ubyte),
                ("r_trig", C.c_ubyte),
                ("l_trig", C.c_ubyte),
                ("reserved1", C.c_ubyte),
                ("reserved2", C.c_ubyte),
                ("x_axis", C.c_byte),
                ("y_axis", C.c_byte),
               ]

def meta_read(fn, addr, width, count = None):
    if count is None:
        return fn(addr)
    else:
        return [fn(addr + width*x) for x in range(count)]

def meta_write(fn, addr, val, width):
    if not isinstance(val, list):
        fn(addr, val)
    else:
        x = 0
        for v in val:
            fn(addr + width*x, val)
            x += 1

class LoadResult:
    NoError, NotExist, WrongVersion = range(3)

class Debug:
    """Mupen64Plus Debug library"""

    def __init__(self, core, settings):
        self.core = core
        self.settings = settings
        self.inited = False
        self.supported = False
        self.input_loading_error = None
        self.runnables = {}
        self.runnables_index = [] # Always-sorted keys list. Could also just use OrderedDict if python weren't weird
        self.runnable_id_generator = 0
        self.ctype_callbacks = None
        self.input = None
        self.buttons_override_reset_state()
        self.input_plugin_path_last = None
        global debug_singleton
        if debug_singleton is not None:
            raise Exception("Multiple Debug objects created?") # Probably actually survivable but, shrug
        debug_singleton = self

    # Class internals

    def init(self):
        self.core.m64p.DebugMemRead8.restype = C.c_ubyte
        self.core.m64p.DebugMemRead16.restype = C.c_ushort
        self.core.m64p.DebugMemRead32.restype = C.c_uint
        self.core.m64p.DebugMemRead64.restype = C.c_ulonglong

        # TODO: Check at startup if EnableDebugger is on.

        # Callbacks
        # Kludge: Am I required to protect the ctype FUNC objects from collection? Anyway, I do.
        self.ctype_callbacks = (voidcall(self.init_callback), voidcall(self.breakpoint_callback), voidcall(self.blank_callback))
        self.supported = M64ERR_UNSUPPORTED != self.set_callbacks(*self.ctype_callbacks)
        self.inited = True

        self.init_input()

    def input_plugin_path(self):
        qset = self.settings.qset
        return os.path.join(qset.value('Paths/Plugins'), qset.value('Plugins/Input'))

    def init_input(self): # Separate from init() so it can re-occur on "boot"
        self.input_plugin_path_last = self.input_plugin_path()
        try:
            self.input = load(self.input_plugin_path_last)
        except: # This could fail in different kinds of ways depending on host OS
            self.input_loading_error = LoadResult.NotExist
            self.input = None
        else:
            try:
                self.input.ButtonsOverrideExploded
            except AttributeError:
                self.input_loading_error = LoadResult.WrongVersion
            else:
                self.input_loading_error = LoadResult.NoError

    def set_callbacks(self, init_callback = None, interrupt_callback = None, blank_callback = None):
        """If you are using pythonconsole, do not call this directly."""
        return self.core.m64p.DebugSetCallbacks(init_callback if init_callback else null_ptr,
            interrupt_callback if interrupt_callback else null_ptr,
            blank_callback if blank_callback else null_ptr)

    def init_callback(self):
        if self.input_plugin_path_last != self.input_plugin_path():
            okay_load = True
            if self.input:
                try:
                    unload_library(self.input)
                except Exception as e:
                    okay_load = False
                    sys.stderr.write("Warning: Python scripting extensions attempted to unload/reload the input plugin after a change, and failed: " + e)
            if okay_load:
                self.init_input()

        for key in self.runnables_index:
            obj = self.runnables[key]
            if obj.hasBoot:
                obj.onBoot()

        # Normally, mupen64plus starts halted. That is ont what we want.
        self.core.m64p.DebugSetRunState(C.c_uint(2)) # Enum 2 is RUNNING # TODO: Make a constant
        self.core.m64p.DebugStep()

    def breakpoint_callback(self):
        for key in self.runnables_index:
            obj = self.runnables[key]
            if obj.hasBreakpoint:
                obj.onBreakpoint()

    def blank_callback(self):
        for key in self.runnables_index:
            obj = self.runnables[key]
            if obj.hasBlank:
                obj.onBlank()

    def check_supported(self):
        if not self.inited:
            raise RuntimeError("Debug object used without being inited!") # This should be impossible
        if not self.supported:
            raise RuntimeError("Cannot call debug function-- mupen64plus core was built without debugging")

    def check_supported_input(self):
        if not self.inited:
            raise RuntimeError("Debug object used without being inited!") # This should be impossible
        if self.input_loading_error == LoadResult.NotExist:
            raise RuntimeError("Cannot call input function-- mupen64plus does not have an input plugin selected")
        if self.input_loading_error == LoadResult.WrongVersion:
            raise RuntimeError("Cannot call input function-- the input plugin you have selected in mupen64plus does not have the scripting extensions")

    # Input

    def buttons_override_reset_state(self): # For internal use
        self.current_buttons = [Buttons() for _ in range(4)]

    def buttons_override_disable(self):
        self.check_supported_input()
        self.input.ButtonsOverrideOff()
        self.buttons_override_reset_state()

    def buttons_override_struct(self, current, controller=0):
        self.check_supported_input()
        C.pointer(self.current_buttons[controller])[0] = current
        self.input.ButtonsOverrideExploded(controller, current)

    def buttons_override_struct_last(self, controller=0):
        current = Buttons()
        C.pointer(current)[0] = self.current_buttons[controller]
        return current

    def buttons_override(self, r_dpad=None, l_dpad=None, d_dpad=None, u_dpad=None, start_button=None, z_trig=None, b_button=None, a_button=None, r_cbutton=None, l_cbutton=None, d_cbutton=None, u_cbutton=None, r_trig=None, l_trig=None, reserved1=None, reserved2=None, x_axis=None, y_axis=None, reset=False, controller=0):
        self.check_supported_input()

        if reset:
            self.current_buttons = Buttons()

        current = self.current_buttons[controller]

        if r_dpad is not None:
            current.r_dpad = r_dpad

        if l_dpad is not None:
            current.l_dpad = l_dpad

        if d_dpad is not None:
            current.d_dpad = d_dpad

        if u_dpad is not None:
            current.u_dpad = u_dpad

        if start_button is not None:
            current.start_button = start_button

        if z_trig is not None:
            current.z_trig = z_trig

        if b_button is not None:
            current.b_button = b_button

        if a_button is not None:
            current.a_button = a_button

        if r_cbutton is not None:
            current.r_cbutton = r_cbutton

        if l_cbutton is not None:
            current.l_cbutton = l_cbutton

        if d_cbutton is not None:
            current.d_cbutton = d_cbutton

        if u_cbutton is not None:
            current.u_cbutton = u_cbutton

        if r_trig is not None:
            current.r_trig = r_trig

        if l_trig is not None:
            current.l_trig = l_trig

        if reserved1 is not None:
            current.reserved1 = reserved1

        if reserved2 is not None:
            current.reserved2 = reserved2

        if x_axis is not None:
            current.x_axis = x_axis

        if y_axis is not None:
            current.y_axis = y_axis

        self.input.ButtonsOverrideExploded(controller, current)

    # Runnables

    def reset_runnables_index(self):
        self.runnables_index = sorted(self.runnables.iterkeys())

    def register(self, obj): # NOTE: IF THIS IS GETTING CALLED FROM MULTIPLE THREADS, IT'S UNSAFE
        self.runnable_id_generator += 1
        self.runnables[self.runnable_id_generator] = obj
        self.reset_runnables_index()
        return self.runnable_id_generator

    def unregister(self, id):
        del(self.runnables[id])
        self.reset_runnables_index()

    def running(self):
        return self.runnables.values()

    # Utility

    def int_to_float_32(self, val):
        return C.cast((C.c_uint*1)(val), C.POINTER(C.c_float)).contents.value

    def int_to_float_64(self, val):
        return C.cast((C.c_ulonglong*1)(val), C.POINTER(C.c_double)).contents.value

    def float_to_int_32(self, val):
        return C.cast((C.c_float*1)(val), C.POINTER(C.c_uint)).contents.value

    def float_to_int_64(self, val):
        return C.cast((C.c_double*1)(val), C.POINTER(C.c_ulonglong)).contents.value

    # Wrappers

    def _mem_read_8(self, addr):
        return self.core.m64p.DebugMemRead8(C.c_uint(addr))
    def mem_read_8(self, addr, count = None):
        self.check_supported()
        return meta_read(self._mem_read_8, addr, 1, count)

    def _mem_read_16(self,addr):
        return self.core.m64p.DebugMemRead16(C.c_uint(addr))
    def mem_read_16(self, addr, count = None):
        self.check_supported()
        return meta_read(self._mem_read_16, addr, 2, count)

    def _mem_read_32(self,addr):
        return self.core.m64p.DebugMemRead32(C.c_uint(addr))
    def mem_read_32(self, addr, count = None):
        self.check_supported()
        return meta_read(self._mem_read_32, addr, 4, count)

    def mem_read_64(self,addr):
        return self.core.m64p.DebugMemRead64(C.c_uint(addr))
    def mem_read_64(self, addr, count = None):
        self.check_supported()
        return meta_read(self._mem_read_64, addr, 8, count)

    def mem_read_str(self, addr, count = None):
        result = self.mem_read_8(addr, count)
        if count is None:
            return chr(result)
        else:
            return "".join([chr(x) for x in result])

    def mem_read_float_32(self, addr, count = None):
        result = self.mem_read_32(addr, count)
        if count is None:
            return self.int_to_float_32(result)
        else:
            return "".join([self.int_to_float_32(x) for x in result])

    def mem_read_float_64(self, addr, count = None):
        result = self.mem_read_64(addr, count)
        if count is None:
            return self.int_to_float_64(result)
        else:
            return "".join([self.int_to_float_64(x) for x in result])

    def _mem_write_8(self, addr, val):
        self.core.m64p.DebugMemWrite8(C.c_uint(addr), C.c_ubyte(val))
    def mem_write_8(self, addr, val):
        self.check_supported()
        meta_write(self._mem_write_8, addr, val, 1)

    def _mem_write_16(self, addr, val):
        self.core.m64p.DebugMemWrite16(C.c_uint(addr), C.c_ushort(val))
    def mem_write_16(self, addr, val):
        self.check_supported()
        meta_write(self._mem_write_16, addr, val, 2)

    def _mem_write_32(self, addr, val):
        self.core.m64p.DebugMemWrite32(C.c_uint(addr), C.c_uint(val))
    def mem_write_32(self, addr, val):
        self.check_supported()
        meta_write(self._mem_write_32, addr, val, 4)

    def _mem_write_64(self, addr, val):
        self.core.m64p.DebugMemWrite64(C.c_uint(addr), C.c_ulonglong(val))
    def mem_write_64(self, addr, val):
        self.check_supported()
        meta_write(self._mem_write_64, addr, val, 8)

    def mem_write_str(self, addr, val):
        self.mem_write_8(addr, [ord(x) for x in val])

    def mem_write_float_32(self, addr, val):
        if not isinstance(val, list):
            val = self.float_to_int_32(val)
        else:
            val = [self.float_to_int_32(x) for x in val]
        self.mem_write_32(addr, val)

    def mem_write_float_64(self, addr, val):
        if not isinstance(val, list):
            val = self.float_to_int_64(val)
        else:
            val = [self.float_to_int_64(x) for x in val]
        self.mem_write_32(addr, val)
