import binaryheap, sets

type
    AstarNeighbors*[Node] = proc (start:Node) : seq[Node] {.closure.}
    AstarId*[Node, Id] = proc (node:Node) : Id {.closure.}
    AstarNeighborCost*[Node] = proc (start:Node, dest:Node) : int {.closure.}
    AstarCostEstimate*[Node] = proc (start:Node, dest:Node) : int {.closure.}

proc reverse[T](s:var seq[T]) =
    let stop = s.len-1
    for x in 0..(stop div 2):
        swap(s[x], s[stop-x])

proc astar*[Node,Id](origin:Node, goal:Node, id:AstarId[Node,Id], neighbors:AstarNeighbors[Node],
                  neighborCost:AstarNeighborCost[Node], costEstimate:AstarCostEstimate[Node]) : seq[Node] =
    if origin == goal:
        return @[origin]

    type MapNode = ref object
        node: Node
        came: MapNode
        cost: int
    proc mapNode(n:Node,ca:MapNode,co:int) : MapNode = return MapNode(node:n,came:ca,cost:co)
    type Job = object
        weight: int
        mapNode: MapNode
    proc job(w:auto,m:auto):auto = return Job(weight:w, mapNode:m)

    var queue = newHeap[Job]() do (a, b: Job) -> int:
        return a.weight - b.weight
    var closed = initSet[int]()

    queue.push(job(0, mapNode(origin,nil,0)))
    closed.incl( id(origin) )
    while(queue.size > 0):
        let current = queue.pop
        let currentMapNode = current.mapNode
        let currentNode = currentMapNode.node
        for neighbor in neighbors(currentNode):
            if neighbor == goal:
                var cursor = currentMapNode
                var path:seq[Node] = @[neighbor]
                while cursor != nil:
                    path.add(cursor.node)
                    cursor = cursor.came
                path.reverse
                return path

            let neighborId = id(neighbor)
            if closed.contains(neighborId):
                continue
            closed.incl(neighborId)

            let linkCost = neighborCost(currentNode, neighbor)
            let cost = currentMapNode.cost + linkCost
            let neighborMapNode = mapNode(neighbor, currentMapNode, cost)
            let neighborWeight = cost + costEstimate(origin, neighbor)
            let neighborJob = job(neighborWeight, neighborMapNode)

            queue.push(neighborJob)
    return nil
