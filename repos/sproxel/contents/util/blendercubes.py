import string
import bpy
import mathutils

fp = open("/nethome/joeld/tree1.csv")

lines = list( filter( lambda x: len(x.strip()), fp.readlines() ) )
fp.close()

sz = tuple(map( lambda x: int(x), lines[0].split(',')))
lines = list(map(lambda x: x.strip(), lines[1:] ))
lines.reverse()

print("size is " + str(sz))


for yval in range(sz[1]):
    
    xzlines = lines[ yval*sz[2] : yval*sz[2] + sz[2]  ]
    for zval in range(sz[2]):
        zline = xzlines[zval]

        voxels = zline.split(",")
        for xval in range(sz[0]):
            
            col = voxels[xval]
            if not col == "#00000000":
                bpy.ops.mesh.primitive_cube_add()
                c = bpy.context.active_object
                c.delta_location = mathutils.Vector(( xval*2, zval*2, yval*2 ))