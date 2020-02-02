# Simple SDL-based game library

# Helper: Load a C function and store it in module private
private .setForeign ^key spec = (
    private (key) = package.emily.ffi.c.function [
        parent=spec                    # Take given args as a base, and overlay with
        name= "GAME" + (key.toString);           # Custom mutated name
        if (! : spec.has.return) ^( return = .int ) # Custom mutated return type
    ]
)

# Helper: Unload a function out of private and into a new name in export
# "Wrap" with another function that preps arguments/return somehow
private.privateWrapTo ^publicKey privateKey wrapper = (
    (publicKey) = wrapper: private privateKey
)
# Helper: Same, but function name is same in private and export
private.privateWrap ^key = privateWrapTo key key

# Note: Some of these helpers seem general and probably should go somewhere else.

# Wrapper: On call, turn an argument block into currying
private.uncurryWrap ^spec fn = {
    default = spec.default // []
    names   = spec.names   // []
    count   = spec.count   // default.count // names.count // 0

    count > 0 ? (
        ^args { # Take an argument object and unpack it into curried arguments
            result = fn
            upto count ^i (
                result = result: args i // (
                    names.has i && args.has (names i) ? args (names i) : \
                    default i //                                         \
                    fail: "Needed "+(count.toString)+" arguments"
                )
            )
            result
        }
    ) : (
        ^( do fn ) # 0-arg function, just throw away argument
    )
}

# Wrapper: On call, participate in sdl.c error handling convention
setForeign .globalError [ return = .string ]
private.errorWrap ^fn args = {
    result = fn args
    if (0 != result) ^(       # Call
        fail: do globalError  # If nonzero return, throw
    )
}
private.negativeError ^result = (
    if (0 > result) ^(       # Call
        fail: do globalError  # If nonzero return, throw
    )
    result
)
private.negativeErrorWrap ^fn args = \
    negativeError: fn args
private.boolWrap ^fn args = ( # No error check, maybe negativeErrorWrap could be a good idea though
    0 != fn args
)

# Wrapper: All of the above
private.bothWrap ^resultWrap uncurrySpec fn = (
    x = resultWrap: (uncurryWrap uncurrySpec) fn
    x
)

private.exportWith ^resultWrap name spec = (
    setForeign name spec
    privateWrap name: bothWrap resultWrap spec
)
private.export = exportWith errorWrap

# Imported functions from C

textureFlags = [
    canDisplay = 1
    canEdit = 2
    canComposit = 4
    normalCreate = this.canDisplay + this.canComposit
    normalLoad   = this.canDisplay
]

events = [
    confused = 0
    draw         = 2
    escPress     = 4
    escRelease   = 5
    leftPress    = 6
    leftRelease  = 7
    rightPress   = 8
    rightRelease = 9
    upPress      = 10
    upRelease    = 11
    downPress    = 12
    downRelease  = 13
]

export .init [
    args = [.int, .int, .int]; names=[.fullscreen, .w, .h]; default=[0,640,360]
]

export .finish []

exportWith (^x:x) .resourcePath [ return= .string ]

export .delay [ args = [.int,]; names=[.delay,]; default=[33,] ]

exportWith negativeErrorWrap .imgCount []
exportWith boolWrap .imgValid [ args = [.int,]; names=[.img,] ]
export .imgFree [ args = [.int,]; names=[.img,] ]

private.stackWrap ^fn args = {
    result = do imgCount
    errorWrap fn args
    result
}

# -1,-1 for "size of screen"
exportWith stackWrap .imgPush [ args = [.int, .int, .int]; names=[.w, .h, .flags]; default=[-1, -1, textureFlags.normalCreate] ]
exportWith stackWrap .imgLoad [ args = [.string, .int]; names=[.name, .flags]; default=["", textureFlags.normalLoad] ]
exportWith negativeErrorWrap .imgRead [
    args =   [.int, .int, .int, .int]
    names=   [.img, .x, .y, .channel]
    default= [-1, 0, 0, 3] # FIXME: Should only default on last arg
]

# Hand roll constructing a structure
setForeign .imgInfo [ args = [.int, .int] ]
imgInfo ^spec = {
    img = spec 0 // spec.img // 0
    info = private.imgInfo img
    [w=negativeError: info 0; h=negativeError: info 1]
}

export .imgClear [
    args = [.int, .double, .double, .double, .double]
    names=[.img, .r, .g, .b, .a]
    default=[0,0,0,0,1]
]

export .imgCopy [
    args = [.int, .int, .int, .int, .int, .int, .int, .double]
    names=[.dst, .img, .x, .y, .srcX, .srcY, .blend, .alpha]
    default=[0, -1, 0, 0, 0, 0, 1, 1]
]

export .present []

exportWith negativeErrorWrap .tick [
    args = [.int,]
    names= [.tpf,]
    default=[33,]
]
