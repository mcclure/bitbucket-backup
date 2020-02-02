#!/usr/bin/perl

use strict;
use lib "trans";
use trans;
use Encode;

sub translate {
    my ($dict, $in) = @_;
    return $$dict{$in} if (exists($$dict{$in}));
    print "\t\"$in\" not found\n";
    return $in;
}

for my $file (@files) {
    print "$file\n" if ($verbose);
    for my $l (0..(@langs-1)) {
        my $out = $file;
        die "Doesn't start with en?" unless ($out =~ s/^en/$langs[$l]/);

        print "-> $out\n" if ($verbose);
        
        my $dict = $dicts[$l];
        
        `ibtool --generate-stringsfile temp1.strings $file`;

        open(IN, "<:raw", "temp1.strings"); # Perl has failed me
        open(OUT,">:raw", "temp2.strings");
        my $text = do { local $/; <IN> };
        my $content = decode('UTF-16LE', $text); 

        $content =~ s/^(\"[^\"]+\" = \")(.+)(\";\s*)$/$1 . translate($dict, $2) . $3/mge; # Does nothing, just as a test
        
        print OUT encode("UTF-16LE", $content);
        close(IN);
        close(OUT);
        
        `ibtool --strings-file temp2.strings --write $out $file`;
    }
}