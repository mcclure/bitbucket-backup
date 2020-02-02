# FFI interface for C

# internalPackage exposes an object with an awkward interface.
# Expose a more convenient interface and convert.
function ^spec = {
    f = internal.ffi.newForeign()
    if (spec.has.name)   ^(f.name: spec.name)
    if (spec.has.args)   ^(spec.args.each ^a (f.args a))
    if (spec.has.return) ^(f.return: spec.return)
    f.make()
}
