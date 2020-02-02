null = ()

do ^x = x null

nullfn ^ = null

rawTern = internal.tern

# DUPLICATES valueUtil
tern ^predicate a b = do: rawTern predicate a b

not ^x = rawTern x null true

if ^predicate body = tern predicate ^(do body) nullfn

loop ^f = if (do f) ^(loop f)

while ^predicate body = if (do predicate) ^(do body; while predicate body)

upto ^x perform = {
    counter = 0
    result = null
    while ^(counter < x) ^( result = perform counter; counter = counter + 1; )
    result # For consistency with while
}

and ^a b = tern (do a) b nullfn

or ^a b = { aValue = do a; tern aValue ^(aValue) b }

xor ^ a b = {
    aValue = do a
    bValue = do b
    rawTern aValue (
        rawTern bValue null aValue
    ) bValue
}

check ^obj key else = obj.has key ? obj key : do else

sp = " "

ln = "\n"

true = internal.true

fail = internal.fail

print = internal.out.print

println ^s = ( print s ln; do: internal.out.flush ; println )

# FIXME: Any whitespace not just ln
printsp ^s = {
    printsp2 ^s = s == ln ? (
        print s; printsp
    ) : (
        print sp s; printsp2
    )
    print s; printsp2
}

inherit ^x = [ parent=x ]

thisTransplant = internal.thisTransplant
thisInit       = internal.thisInit
thisFreeze     = internal.thisFreeze
thisUpdate     = internal.thisUpdate

floor = internal.double.floor
ceiling ^x = {
    i = floor x
    i < x ? i + 1 : i
}

atom   = internal.type.isAtom
string = internal.type.isString
number = internal.type.isNumber
int ^x = number x && x == floor x # Sort of a virtual type, I suppose...?

internal.setPropertyKey current .scope ^x(x)
