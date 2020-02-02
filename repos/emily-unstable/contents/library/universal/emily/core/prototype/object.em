eq ^ = null

# Given a binary function, embed it on the current object as a method
private.setMethodFrom ^key method = \
    internal.setPropertyKey current key method

setMethodFrom .append ^this v {
    if !(this.has.count) ^(this.count = 0)
    this (this.count) = v
    this.count = this.count + 1
}

setMethodFrom .each ^this f {
    idx = 0
    while ^(this.has idx) ^(
        f (this idx)
        idx = idx + 1
    )
}

setMethodFrom .map ^this f {
    result = []
    this.each: ^x(result.append: f x)
    result
}
