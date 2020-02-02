exportLet .parent: directory.primitive # Prototype inherits other prototype

# Given a binary function, embed it on the current object as a method
private.setMethodFrom ^key method = \
    internal.setPropertyKey current key method

private.viewObject ^iterator str = [
    each ^fn = {
        strIter = iterator str
        loop: ^(strIter fn)
    }

    internal.setPropertyKey current .count ^this {
        result = 0
        this.each ^( result = result + 1 )
        result
    }

    parent ^field = (
        int field ? {
            index = 0
            this.each ^v(
                index == field ? return v : (index = index + 1)
            )
        } : (
            field == .has ? return ^i( i >= 0 && i < this.count ) : ()
        )
        null field        # FIXME: Should be a way to fail / "throw"
    )
]

setMethodFrom .char:      viewObject: internal.string.iterUtf8
setMethodFrom .codepoint: viewObject: internal.string.iterUtf8Codepoint

setMethodFrom .plus: internal.string.concat
