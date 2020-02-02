This is a test with the eventual aim of making a programming language interpreter. Right now all that's here is a parser for a sort of LISP-with-significant-whitespace. If you're looking for the actual Emily language, try [emilylang.org](http://emilylang.org).

To run a program, try:

    ./emily.py ./test/exec/while.em

Or use `-e`:

    ./emily.py -e 'println 47'

For other supported args run without arguments:

    ./emily.py

To run the tests:

    ./develop/regression.py -a

To run the self-hosting interpreter, try:

    ./emily.py em/emily.em ./test/exec/while.em
    ./emily.py em/emily.em -e 'println 47'

To run the tests with the self-hosting interpreter:

    ./develop/regression.py --meta -a

You are not currently granted any rights to the contents of this repository.

— Andi McClure <<andi.m.mcclure@gmail.com>>
