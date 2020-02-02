import yaml, basic2d
import maze

type IntersectionRecord = object
  id, s, e, sDist, eDist: int
var intersectionList: seq[IntersectionRecord]

type IntersectionBuildSite = ref object
    record: IntersectionRecord
    result: Intersection

var intersectionMap = newTable[int, IntersectionBuildSite](16)

var s = newFileStream("level.yaml")
load(s, intersectionList)
s.close()

for r in intersectionList:
    intersectionMap[r.id] = IntersectionBuildSite(record:r, result:nil)

var level: seq[Intersection] = @[]

proc makeIntersection(at:Point2d) : Intersection =
    result = intersection(at)
    level.add result

var root = intersectionMap[1]
root.result = makeIntersection(point2d(laneGap, laneGap))

var currentPass = @[root]
while currentPass.len > 0:
    var nextPass: seq[IntersectionBuildSite] = @[]
    for site in currentPass:
        proc reach(neighborId:int, neighborDist:int, dir:Dir) =
            if neighborId > 0:
                var neighbor = intersectionMap[neighborId]
                let dist = if neighborDist > 0: neighborDist*laneGap else:laneGap
                if neighbor.result == nil:
                    var at = site.result.at + clock[dir.int]*dist.float
                    neighbor.result = makeIntersection(at)
                    nextPass.add(neighbor)
                site.result.connect(neighbor.result, dir, dist)
        reach(site.record.s, site.record.sDist, dirSouth)
        reach(site.record.e, site.record.eDist, dirEast)

    currentPass = nextPass

runLevel(level)
