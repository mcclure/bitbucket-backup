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

import sys
import os.path
import logging
import traceback
import re
from copy import copy
from m64py.core.debug import Runnable, Buttons

class ScriptContext:
    def __init__(self, name):
        self.result = None
        self.name = name

class ReturningResult(BaseException):
    pass

pyext = re.compile(r'\.py$')
def short_name(path):
    path = os.path.basename(path)
    return pyext.sub('', path)

trace_level_max = 9001

def build_scope(manager):
    scope = {}

    def run_script(name, isLoad):
        path = name
        if manager.scriptdir and not os.path.isabs(path):
            path = os.path.join(manager.scriptdir, path)

        if not os.path.exists(path) and not pyext.search(path):
            altpath = path + '.py'
            if os.path.exists(altpath):
                path = altpath

        with open(path) as f:
            content = f.read()

        old_context = manager.current_script_context
        new_context = ScriptContext(name)
        manager.current_script_context = new_context

        try:
            result = manager.evaluate(content, scope if isLoad else build_scope(manager), "Script " + short_name(name))
        except ReturningResult:
            pass # This is a supported way to terminate a script
        finally:
            manager.current_script_context = old_context

        if new_context.result:
            return new_context.result
        return None

    def start(target):
        name = None

        if type(target) == str:
            name = short_name(target)
            obj = run_script(target, False)
        else:
            obj = target
        obj.start()

        if name:
            if name not in manager.running:
                manager.running[name] = []
            manager.running[name].append(obj)

        return obj

    def stop(target):
        if type(target) == str:
            name = short_name(target)
            if name in manager.running:
                for obj in manager.running[name]:
                    obj.stop()
                del(manager.running[name])
            else:
                raise RuntimeException("No script named \"%s\" is running." % name)
        else:
            target.stop()

    def panic():
        for target in manager.worker.debug.running():
            stop(target)
        manager.worker.debug.buttons_override_disable()
        manager.trace_level = 1

    def result(value):
        if manager.current_script_context:
            manager.current_script_context.result = value
        else:
            raise RuntimeError("Called result() but no script is currently running")
        raise ReturningResult()

    def trace(msg, level=1):
        if level >= trace_level_max:
            raise ValueError("Trace level cannot go over %s" % (trace_level_max-1))
        if level >= manager.trace_level:
            if manager.current_script_context:
                prefix = "Python script '%s': " % (manager.current_script_context.name)
            else:
                prefix = "Python console: "
            sys.stderr.write("%s%s\n" % (prefix, msg))

    def set_trace_level(level):
        manager.trace_level = level

    scope['script'] = lambda x: run_script(x, False)
    scope['load'] = lambda x: run_script(x, True)
    scope['result'] = result
    scope['last'] = lambda: manager.last_result
    scope['last_exception'] = lambda: manager.last_exception
    scope['trace'] = trace
    scope['trace_level'] = lambda: manager.trace_level
    scope['set_trace_level'] = set_trace_level
    scope['trace_level_max'] = trace_level_max
    scope['Runnable'] = Runnable
    scope['Buttons'] = Buttons
    scope['ReturningResult'] = ReturningResult
    scope['start'] = start
    scope['stop'] = stop
    scope['panic'] = panic
    scope['worker'] = manager.worker
    scope['settings'] = manager.settings
    scope['core'] = manager.worker.core
    scope['debug'] = manager.worker.debug

    return scope

class ScriptManager:
    def __init__(self, worker, settings):
        self.worker = worker
        self.settings = settings
        self.scope = build_scope(self)
        self.current_script_context = None
        self.last_result = None # Set externally
        self.last_exception = None # Set externally
        self.trace_level = 1
        self.running = {}
        self.original_sys_path = copy(sys.path)
        self.scriptdir = None

    # Make import work. ** NOTICE HOW DANGEROUS THIS IS **
    def update_scriptdir(self):
        scriptdir = self.settings.qset.value("Paths/Scripts")
        if scriptdir != self.scriptdir:
            self.scriptdir = scriptdir
            sys.path = self.original_sys_path + [scriptdir]

    # Returns a ScriptResult, Value tuple
    def evaluate(self, source, scope=None, origin="Python Console"):
        self.update_scriptdir()

        source += "\n"
        isEval = False
        code = None
        if not scope:
            scope = self.scope
        try:
            code = compile(source, origin, mode="eval")
            isEval = True
        except SyntaxError as e:
            pass
        if not code:
            code = compile(source, origin, mode="exec") # SyntaxErrors bubble up
        
        if isEval:
            return eval(code, scope, scope)
        else:
            exec(code, scope, scope)

        return None

# UI SUPPORT
# What the non-GUI expects of this code: "settings" should be set before first call to evaluate()

from PyQt5.QtCore import pyqtSignal
from PyQt5.QtWidgets import QDialog
from PyQt5.QtGui import QTextCursor

from m64py.ui.pythonconsole_ui import Ui_PythonConsole

class PythonConsoleView(QDialog, Ui_PythonConsole):
    def __init__(self, parent=None):
        QDialog.__init__(self, parent)
        self.setupUi(self)
        
    def evaluate(self, msg):
        entry = self.textEntry.toPlainText().rstrip()
        if entry:
            self.textDisplay.append("> " + entry + "\n")

            try:
                value = scriptmanager.evaluate(entry)
            except Exception as e:
                # TODO: Slice off bottom three frames
                self.textDisplay.append(traceback.format_exc() + "\n")
                scriptmanager.last_exception = e
            else: # If no exception
                self.textEntry.setPlainText("")
                if value is not None:
                    self.textDisplay.append(str(value) + "\n")
                scriptmanager.last_result = value
                scriptmanager.last_exception = None

            scroll = self.textDisplay.verticalScrollBar()
            scroll.setValue(scroll.maximum())

pythonconsoleview = None
scriptmanager = None

def pythonconsoleforeground(parent):
    global pythonconsoleview
    global scriptmanager
    if not pythonconsoleview:
        pythonconsoleview = PythonConsoleView(parent)
        scriptmanager = ScriptManager(parent.worker, parent.settings)
    pythonconsoleview.show()
    pythonconsoleview.activateWindow()
    pythonconsoleview.raise_()
