# A rectangle class

import basic2d

proc xflip(v: Vector2d) : Vector2d = return vector2d(-v.x,  v.y)
proc yflip(v: Vector2d) : Vector2d = return vector2d( v.x, -v.y)

type Rect* = object
    center*: Point2d
    rad*: Vector2d

proc rect*(center: Point2d, rad: Vector2d) : Rect = return Rect(center:center, rad:rad)
proc square*(center: Point2d, rad: float)  : Rect = return rect(center, vector2d(rad,rad))

proc size*(r: Rect) : Vector2d = return r.rad*2

# Northwest, northeast, southwest, southeast corners
proc nw*(r: Rect) : Point2d = return r.center - r.rad
proc ne*(r: Rect) : Point2d = return r.center + r.rad.yflip
proc sw*(r: Rect) : Point2d = return r.center + r.rad.xflip
proc se*(r: Rect) : Point2d = return r.center + r.rad

# North, south, east, west edges
proc n*(r: Rect) : float = r.center.y - r.rad.y
proc s*(r: Rect) : float = r.center.y + r.rad.y
proc w*(r: Rect) : float = r.center.x - r.rad.x
proc e*(r: Rect) : float = r.center.x + r.rad.x

proc vert*(r: Rect, at: range[0..3]) : Point2d =
    case at
    of 0: return r.nw
    of 1: return r.ne
    of 2: return r.se
    of 3: return r.sw

proc translate*(r: Rect, by: Vector2d) : Rect = return rect(r.center + by, r.rad)
# proc scale*(r:Rect, factor: float)   : Rect = return rect(r.center * factor, r.rad * factor) # TODO
proc inset*(r:Rect, factor: float)     : Rect = return rect(r.center, r.rad * factor)
proc inset_fixed*(r: Rect, by: float)  : Rect =
    proc inset1(v: float, by: float) : float = max(0, v - by * 2)
    let rad = vector2d( inset1(r.rad.x, by), inset1(r.rad.y, by) )
    return rect(r.center, rad)

proc alignN*(r:Rect, n: float) : Rect = return r.translate( vector2d( 0, n - r.n ) )
proc alignS*(r:Rect, s: float) : Rect = return r.translate( vector2d( 0, s - r.s ) )
proc alignW*(r:Rect, w: float) : Rect = return r.translate( vector2d( w - r.w, 0 ) )
proc alignE*(r:Rect, e: float) : Rect = return r.translate( vector2d( e - r.e, 0 ) )

const zeroRect = rect(point2d(0,0), vector2d(0,0))

proc rectBounds*(p1: Point2d, p2: Point2d) : Rect =
    var n,s,w,e:float

    if p1.x <= p2.x:
        w = p1.x; e = p2.x
    else:
        w = p2.x; e = p1.x

    if p1.y <= p2.y:
        n = p1.y; s = p2.y
    else:
        n = p2.y; s = p2.y

    return rect( point2d( (e+w)/2, (s+n)/2 ), vector2d( (e-w)/2, (s-n)/2 ) )
