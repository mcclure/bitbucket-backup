# String utilities

join ^joiner a = {
    result = ""
    first = true
    a.each ^s(
        first ? (first = null) : (result = result + joiner)
        result = result + s
    )
    result
}

fromCodepoint = internal.string.codepointToString
