# Utils for array-like objects

range ^base limit = [
    base=base; count=limit-base

    each ^f = {
        iter = 0
        while ^(iter < this.count) ^(f: this.base + iter; iter = iter + 1)
    }

    parent ^field = (
        int field && field <this. count ? (
            return: field + this.base
        ) : (
            field == .has ? return ^i(
                i >= this.base && i < this.base + this.count
            ) : ()
        )
        fail "not in range"
    )
]

rangeTo = range 0

copy ^a = [ a.each ^x( x, ) ] # FIXME: This idiom...

contains ^a key = ( a.each ^x: if (key == x) ^(return true); return null )