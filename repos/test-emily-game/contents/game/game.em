p = package.emily
e = p.game # E for Engine

# Support

hasArg = p.array.contains: p.os.args
trace = hasArg "-debug" ? println : p.functional.util.void
abs ^x = x < 0 ? -x : x
forever = while ^(true)

w = 160; h = 90;
tile = 15; tw = ceiling: 160/tile; th = ceiling: 90/tile

e.init [ w=w; h=h; fullscreen=hasArg "-fullscreen"?1:0 ] # 90p

# Platformer level map
map = [
    new ^fill w h = [ inherit this | w=w; h=h; wOffset = 0
        storage = [ upto w ^(
            [ upto h ^( fill, ) ],
        ) ]
    ]

    # Practical
    rotate ^x = ( this.wOffset = (this.wOffset + x) % this.w )

    # Magic
    indexFilter ^x = int x ? (x + this.wOffset) % this.w : x
    set ^x = this.storage.set: this.indexFilter x # Don't call this tho :/
    parent ^x = this.storage: this.indexFilter x

    # Drawing
    drawAt ^target x xpos = upto (this.h) ^y{
        img = this x y
        if img ^(
            e.imgCopy[target, img, xpos, y*tile; blend=0]
        )
    }

    draw ^target = upto (this.w) ^x: \
        this.drawAt target x (x*tile)
]

# Game
trace "Startup"
bgTile = e.imgLoad["bg1.png",]
lvTile = e.imgLoad["wall1.png",]
mapImg = e.imgLoad["level1.png", e.textureFlags.canEdit]
endImg = e.imgLoad["gameover.png",]
levelSize = e.imgInfo[mapImg,]

blit = [
    new ^w h = [ inherit this |
        private.newImg = e.imgPush[w, h]
        img=newImg
        e.imgClear[newImg, 1,1,1,0]
        w=w; h=h; wScroll = 0
    ]

    scroll ^x = (
        this.wScroll = this.wScroll + x
        this.wScroll = this.wScroll % this.w
    )

    draw ^ = {
        img = this.img # FIXME
        thisw = this.w
        wScroll = -(this.wScroll)
        e.imgCopy[0, img; x=wScroll]
        if (thisw + wScroll < w) ^(e.imgCopy[0, img; x=thisw+wScroll])
    }
]

makeScreen ^ = blit.new ((tw+1)*tile) (th*tile)
background = do makeScreen
foreground = do makeScreen

clearSlice = e.imgPush[tile, th*tile]
e.imgClear[clearSlice, 1,1,1,0]

level = map.new null (levelSize.w) (levelSize.h)
upto (level.w) ^x: upto (level.h) ^y( level x y = e.imgRead[mapImg, x, y] == 0 ? null : lvTile )
level.draw: foreground.img

upto (tw+1) ^x: upto th ^y: e.imgCopy[background.img, bgTile, x*tile, y*tile]

loadFrames ^count base = {
    result = []
    upto count ^i(
        # FIXME: Notice something horrible; The . is implicit in the toString
        result i = e.imgLoad[base + ((i+1).toString) + "png",]
    )
    result
}

playerframes = 3
player       = loadFrames playerframes "walk"
playerBurn   = loadFrames playerframes "burn"
playerBurned = loadFrames playerframes "burned"

scrollX = 0
withinX = 0
frame = 0

tileFor ^i = floor: i/tile

# "Game state"
gm = [
    upv = 0
    bottom = 4*tile # Unused
    x = 4*tile
    y = 0
    jump = -10
    ground = null
    halt = null
    burn = 0
    burned = 0
    burnRecovery = 30
]

tick ^ = {
    if (gm.halt) ^( do return )

    # Move player
    burning = gm.burn > 0

    # Fall and correct:
    burning ? (
        dx = 4
        gm.burn = gm.burn - 1
    ) : (
        if (gm.burned > 0) ^( gm.burned = gm.burned - 1 )

        inX = tileFor: gm.x                                 # Current down-rounded x position
        intoY = tileFor: gm.y + gm.upv                      # Current up-rounded y position plus velocity
        grounded ^x = level (withinX + inX + x) (intoY + 1) # With x offset, can I fall through this block?
        gm.ground = intoY >= 0 && gm.upv >= 0 && (          # Test: Am I on the ground?
            grounded 0 || (withinX > 0 && grounded 1)
        )
        gm.ground ? (
            gm.y = intoY*tile
            gm.upv = 0
        ) : (
            gm.upv = gm.upv + 1
        )
        dx = 1
    )

    gm.y = gm.y + floor(gm.upv / 2) # We now know if it's safe to add upv or not. # FIXME: /2 above

    colliding ^x = gm.y >= 0 && level (withinX + tileFor (scrollX + x - 2) + 1) (tileFor: gm.y) # -2 because one blank pixel in sprite
    # Move forward and correct:
    colliding (gm.x) ? (
        gm.x = gm.x - 1
    ) : do ^@(
        upto (dx-1) ^z(
            colliding (gm.x + 1) ? do return : (gm.x = gm.x + 1)
        )
    )

    if (gm.x < -tile + 1) ^( gm.halt = 1 )
}

# Notice: Has side effects
render ^ = {
    # Draw basic level
    do: background.draw
    do: foreground.draw

    gm.halt ? (
        e.imgCopy[0, endImg]
    ) : (
        # Make level move
        foreground.scroll 1
        if (frame%2==0) ^(background.scroll 1)

        # If level image needs updating

        nonlocal scrollX = scrollX + 1
        if (scrollX >= tile) ^{
            xpos = (withinX*tile % background.w)
            e.imgCopy[foreground.img, clearSlice, xpos, 0; blend=0]
            level.drawAt (foreground.img) (withinX+tw+1) xpos
            nonlocal scrollX = 0
            nonlocal withinX = withinX + 1
        }

        # Draw player
        gm.burn > 0 ? (
            sprite = playerBurn
            char = null
        ) : (
            sprite = player
            char = gm.burned > 0
        )
        animateFrame = floor(frame / 4) % playerframes

        e.imgCopy[0, sprite: animateFrame, gm.x, gm.y]

        if (char) ^ : \
            e.imgCopy[0, playerBurned: animateFrame, gm.x, gm.y; alpha=gm.burned/gm.burnRecovery]
    )
}

do ^@{
    quit = return
    events = e.events

    handlers = [
        (events.draw)      ^ = ( do tick; do render; nonlocal frame = frame + 1 )

        (events.upPress)   ^ = if (gm.ground) ^( gm.upv = -12; )
        (events.downPress) ^ = if (gm.ground && gm.burned <= 0) ^( gm.burn = 10; gm.burned = gm.burnRecovery )

        (events.escPress)    = quit
        parent ^ = nullfn
    ]

    do: handlers: events.draw
    forever ^{ do: handlers: e.tick [] }
}

e.finish []

trace "Done"
