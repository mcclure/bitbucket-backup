# "Operating system" bridges, on platforms that have one of those

private.setMethodFrom ^key method = \
    internal.setPropertyKey current key method

{
    argCache = null

    setMethodFrom .args ^(
        if !argCache ^( argCache = do: internal.getArgs )
        argCache
    )
}
