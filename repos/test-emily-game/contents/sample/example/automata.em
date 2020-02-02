# Sample program from intro.md
# NOT PART OF REGRESSION TESTS

width = 80

foreach ^upto ^perform = {
    counter = 0
    while ^(counter < upto) ^( perform counter; counter = counter + 1; )
}

inherit ^class = [ parent = class ]

line = [                               # Object for one line of printout
    createFrom ^old = {
        foreach width ^at { # Rule 135
            final = width - 1
            here   = old at
            before = old ( at == 0 ? final : at - 1 )
            after  = old ( at == final ? 0 : at + 1 )
            this.append: ( here && before && after ) \
                     || !( here || before || after )
        }
    }
    print ^ = {
        this.each ^cell { print: cell ? "*" : " " }
        println ""                                          # Next line
    }
]

repeatWith ^old = {  # Repeatedly print a line, then generate a new one
    do: old.print
    new = inherit line
    new.createFrom old
    repeatWith new
}

starting = inherit line        # Create a starting line full of garbage
next = 1
foreach width ^at (
    starting.append: at != next
    if (at == next) ^( next = next * 2 )
)
repeatWith starting                                             # Begin