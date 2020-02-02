#import sproxelConsole
#sproxelConsole.console_write("test!")

print "in test.py!!!"
import sys
import sproxel
from PySide.QtCore import *
from PySide.QtGui import *

print "trying glue..."
from PySide.SproxelGlue import *

print "in test.py!", 1, 2, 3
print "plugin pathes:", sproxel.plugin_pathes
print "sys.path: ", sys.path
#print "sys.modules:", sys.modules

print "main window: ", repr(sproxel.main_window)

#sproxel.main_window.statusBar().showMessage("Python was here!")


print 'plugins info:', sproxel.plugins_info


l=sproxel.Layer((10, 20, 30), name='My Layer')
l.name="Changed name"
l.offset=(1, 2, 3)
print 'layer:', repr(l), l.name, l.offset, l.size, l.bounds, l.dataType
l.set(5, 5, 5, 0x112233);
print 'color:', l.getColor(5, 5, 5)
#l.reset()
#print 'layer:', repr(l), l.name, l.offset, l.size, l.bounds, l.dataType

print ''
s=sproxel.Sprite(l)
print 'sprite:', repr(s)
s.insertLayerAbove(0)
print 'curLayerIndex:', s.curLayerIndex
print 'curLayer:', s.curLayer, s.curLayer.name, s.curLayer.bounds
print 'layer(0):', s.layer(0), s.layer(0).name
print 'layer("Changed name"):', s.layer('Changed name')

prj=sproxel.Project()
print 'project:', repr(prj)
prj.sprites=(s,)
print 'sprites:', prj.sprites

prj=sproxel.get_project()
#sprites=prj.sprites
#s=sproxel.Sprite(sprites[0]); s.name='python1'; sprites.append(s)
#s=sproxel.Sprite(sprites[0]); s.name='python2'; sprites.append(s)
#prj.sprites=sprites
print 'project:', repr(prj)
print 'sprites:', prj.sprites

# Create a Label and show it
#label = QLabel("Hello World")
#label.show()
