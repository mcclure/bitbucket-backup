## Draw a maze outline

import sdl2 except rect, Rect
import basic2d, lists, tables
from math import round
from sequtils import map
import mainloop, rect, astar

const
    laneGap* = 80
    playerRad* = 9 
    laneWidth* = 10
    playerSpeed* = 2

# Maze mechanics

type Dir* = enum
    dirNorth
    dirEast
    dirSouth
    dirWest

proc abs(f:float) : float = return if f > 0: f else: -f
proc ix*(p:Point2d) : int = return round(p.x).int
proc iy*(p:Point2d) : int = return round(p.y).int
proc dirRot*(dir:Dir, by:int) : Dir = return ( (dir.int+by) mod 4 ).Dir
var clock* = [vector2d(0,-1), vector2d(1,0), vector2d(0,1), vector2d(-1,0)]

type Intersection* = ref object
    at*:Point2D
    exit*: array[Dir, Intersection]
    dist*: array[Dir, int]
    id: int
    toward: TableRef[int, Dir]

var intersectionIdGenerator: int

proc intersection*(at:Point2D) : Intersection =
    intersectionIdGenerator += 1 
    return Intersection(at:at, id:intersectionIdGenerator)

var intersections: seq[Intersection]

proc connect*(frm:Intersection, to:Intersection, dir:Dir, dist:int = laneGap) =
    frm.exit[dir] = to
    frm.dist[dir] = dist
    let toDir = dir.dirRot(2)
    to.exit[toDir] = frm
    to.dist[toDir] = dist

# MUST be actual neighbors
proc whichWay(start:Intersection, dest:Intersection) : Dir =
    for i in 0..3:
        let dir = i.Dir
        let neighbor = start.exit[dir]
        if neighbor == dest: return dir
    return dirNorth # FIXME: Maybe I should use an option here?

proc ensureToward(i:Intersection) : TableRef[int, Dir] =
    if i.toward == nil:
        i.toward.new; i.toward[] = initTable[int, Dir](8)
    return i.toward

# Game state

var
    pressedStack = initSinglyLinkedList[Dir]()

# Input utils

proc empty[T](li: SinglyLinkedList[T]) : bool = li.head == nil
proc top[T](li: SinglyLinkedList[T]) : T = li.head.value
proc removeIfPresent[T](li: var SinglyLinkedList[T], value: T) = # Overkill, could have just used doubly linked list
    proc removed(li: var SinglyLinkedList[T], n: SinglyLinkedNode[T]) =
        if n == li.tail:
            li.tail = nil
    var at = li.head
    if at != nil:
        if at.value == value:
            li.head = at.next
            removed(li, at)
        else:
            var next = at.next
            while (next != nil):
                if (next.value == value):
                    at.next = next.next
                    removed(li, next)
                    return

proc pressedDir() : Dir = pressedStack.top
proc anyPressed() : bool = not pressedStack.empty

# SDL utils

proc drawLine(renderer: RendererPtr; x1, y1, x2, y2: int) =
    drawLine(renderer, x1.cint, y1.cint, x2.cint, y2.cint)

proc castKeyEvent(evt:Event):KeyboardEventPtr =
    return cast[KeyboardEventPtr](unsafeAddr(evt))

proc fillRect(renderer: RendererPtr, rect: Rect) =
    var rect = sdl2.rect(rect.w.round.cint, rect.n.round.cint, rect.rad.x.round.cint*2, rect.rad.y.round.cint*2)
    renderer.fillRect(rect)

proc fillCorridor(renderer: RendererPtr; a, b: Point2d) =
    let r = rectBounds(a, b).inset_fixed(-laneWidth)
    renderer.fillRect(r)

# Entity logic

type
    Mover = ref object of RootObj
        base: Intersection
        face: Dir
        moveReverse: bool
        movePixels:int
        moving: bool
    Player = ref object of Mover
    Looper = ref object of Mover
        step: 0..3
    Chaser = ref object of Mover

var player:Player = nil

proc isHalfway(m: Mover) : bool =
    return m.movePixels > m.base.dist[m.face] div 2

method next(m0: Mover) : Mover {.base.} =
    var m : Mover; m.shallowCopy(m0)
    if m.moving:
        m.movePixels += playerSpeed * (if m.moveReverse: -1 else: 1)
        if m.movePixels >= m.base.dist[m.face]:
            m.movePixels = 0
            m.base = m.base.exit[m.face]
    return m

method draw(m: Mover) {.base.} =
    var playerPos = m.base.at
    if m.movePixels != 0:
        playerPos = playerPos + clock[m.face.int]*m.movePixels.float
    render.fillRect( square(playerPos, playerRad) ) # FIXME: Diameter can't be odd with this system :/

method next(p0: Player) : Mover =
    var p : Player; p.shallowCopy(p0)
    p.moving = false
    if anyPressed():
        proc mayContinue(target:Intersection, face:Dir) : bool =
            var target = if p.moveReverse: p.base else: p.base.exit[p.face]
            while target != nil:
                if target.exit[pressedDir()] != nil:
                    return true
                target = target.exit[p.face]
            return false
        if p.movePixels == 0:
            if p.base.exit[pressedDir()] != nil:
                p.face = pressedDir()
                p.moveReverse = false
                p.moving = true
            else:
                p.moving = mayContinue(p.base, p.face)
        else:
            var effectivePressedDir = pressedDir()
            if p.moveReverse:
                effectivePressedDir = effectivePressedDir.dirRot(2)
            if p.face == effectivePressedDir:
                p.moving = true
            elif p.face == effectivePressedDir.dirRot(2):
                p.moveReverse = not p.moveReverse # FIXME: Don't allow moving past 0
                p.moving = true
            else: # TODO: Check "out of gravity well"?
                p.moving = mayContinue(if p.moveReverse: p.base else: p.base.exit[p.face], p.face)
    return procCall p.Mover.next

method draw(p:Player) =
    render.setDrawColor( 0, 0, 0, 255 )
    procCall p.Mover.draw

method next(p0: Looper) : Mover =
    var p : Looper; p.shallowCopy(p0)
    if p.movePixels == 0:
        p.moving = false
        for i in 0..3:
            let face = p.face.dirRot(i*p.step)
            if p.base.exit[face] != nil:
                p.face = face
                p.moveReverse = false
                p.moving = true
                break
    return procCall p.Mover.next

method draw(p:Looper) =
    render.setDrawColor( 0, 0, 255, 255 )
    procCall p.Mover.draw

method next(p0: Chaser) : Mover =
    var p : Chaser; p.shallowCopy(p0)
    if p.movePixels == 0:
        let target = if player.isHalfway: player.base.exit[player.face] else: player.base
        if p.base.toward == nil or not (target.id in p.base.toward):
            proc id (node:Intersection) : int = return node.id
            proc neighbors(start:Intersection) : seq[Intersection] =
                result = @[]
                for i in 0..3:
                    let neighbor = start.exit[i.Dir]
                    if neighbor != nil: result.add(neighbor)
            proc neighborCost(start:Intersection, dest:Intersection) : int =
                let way = start.whichWay(dest)
                return start.dist[way]
            proc costEstimate(start:Intersection, dest:Intersection) : int =
                return ( abs(start.at.x - dest.at.x) + abs(start.at.y - dest.at.y) ).round.int # Manhattan
            let path = astar(p.base, target, id, neighbors, neighborCost, costEstimate)

            var prev:Intersection = nil
            for step in path:
                if prev != nil:
                    let toward = prev.ensureToward
                    if not (target.id in toward):
                        toward[target.id] = prev.whichWay(step)
                prev = step
        if p.base.toward != nil and target.id in p.base.toward:
            p.face = p.base.toward[target.id]
            p.moving = true
        else:
            p.moving = false
    return procCall p.Mover.next

method draw(p:Chaser) =
    render.setDrawColor( 255, 0, 0, 255 )
    procCall p.Mover.draw

var movers : seq[Mover] = @[]

# Game logic

onInit = proc() =
    discard

onEvent = proc(evt:Event) = # FIXME: A stack sorta thing would be nice.
    var dir:Dir
    proc getDir(evt:Event, dir: var Dir) : bool =
        var evt = castKeyEvent(evt)
        case evt.keysym.scancode
        of SDL_SCANCODE_LEFT: dir = dirWest
        of SDL_SCANCODE_RIGHT: dir = dirEast
        of SDL_SCANCODE_UP: dir = dirNorth
        of SDL_SCANCODE_DOWN: dir = dirSouth
        else: return false
        return true
    case evt.kind
    of KeyDown:
        if getDir(evt, dir):
            if not pressedStack.contains(dir): # FIXME why is there key repeat
                pressedStack.prepend(dir)
    of KeyUp:
        if getDir(evt, dir):
            pressedStack.removeIfPresent(dir)
    else: discard

onUpdate = proc(dt:int) =
    movers = map(movers, proc(m:auto):auto = return m.next)    

onDraw = proc() =
    render.setDrawColor( 0, 0, 0, 255 )
    render.clear

    # Map
    render.setDrawColor( 255, 255, 255, 255 )
    for i in intersections:
        let
            e = i.exit[dirEast]
            s = i.exit[dirSouth]
        if e != nil:
            render.fillCorridor(i.at, e.at)
        if s != nil:
            render.fillCorridor(i.at, s.at)

    # Player
    for m in movers:
        m.draw()
        
proc runLevel*(level: auto) =
    intersections = level
    player = Player(base:intersections[0])
    movers.add(player)
    movers.add(Looper(base:intersections[0], step:1))
    movers.add(Looper(base:intersections[0], step:3))
    movers.add(Chaser(base:intersections[ len(intersections)-1 ]))
    run()
