#!/opt/local/bin/perl

# This is an odd little script, possibly not relevant to anyone's workflow except mine.
# I have a problem: ocamlbuild as of 4.02.1 does not build dependent .cmo's if the
# entrypoint .ml has a syntax error. However, Merlin depends on ocamlbuild to build
# dependent .cmo's or it can't do syntax checking. This script, when run from project root,
# gets me out of the wedge state by forcibly building everything whether needed or no. --Andi

for(`ls src/*.ml`) {
    #print("\t*** $_");
    chomp; s/\.ml$//; system("ocamlbuild -no-links -use-ocamlfind $_.cmo");
}
