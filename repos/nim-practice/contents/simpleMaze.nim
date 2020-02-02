import maze, basic2d

var level = @[ Intersection(point2D(laneWidth*2, laneWidth*2)) ]

proc grow(frm:Intersection, dir:Dir) : Intersection =
    result = intersection(frm.at + clock[dir.int]*laneGap.float)
    frm.connect(result, dir)
    level.add result

var 
    a = grow(grow(level[0], dirSouth), dirEast)
    b = grow(a, dirSouth)

a = grow(a, dirEast)
b = grow(b, dirEast)
a.connect(b, dirSouth)
discard grow(b, dirSouth)

runLevel(level)