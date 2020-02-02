package trans;
use strict;
use warnings;
use base 'Exporter';

our @EXPORT = qw($verbose @files @langs @dicts $in);

our $verbose;
our (@files, @langs, @dicts);
our $in = "trans/jumpman_trans_all.txt";

for my $arg (@ARGV) {
    if ($arg eq "--verbose" or $arg eq "-v") {
        $verbose = 1;
    } else {
        push(@files, $arg);
    }
}

{
    my @lines;
    
    open(FH, "<:encoding(UTF-8)",  $in);
    for(<FH>) {
        chomp;
        push(@lines, $_);
    }
    close(FH);
        
    die "Translation file doesn't start with en?" unless ("en" eq shift @lines);

    while (my $line = shift @lines) {
        push(@langs, $line);
        push(@dicts, {});
    }    
    
    while (@lines) {
        my $en = shift @lines;
        die "Malformed file?" unless ($en);
        my $at = 0;
        
        while (my $line = shift @lines) {
            my $dict = $dicts[$at];
            $$dict{$en} = $line;
            $at++;
        }
    }
}

1;