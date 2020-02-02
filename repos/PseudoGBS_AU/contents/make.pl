#!/usr/bin/env perl

# Copy the source directories into place

unless (-d "checkout") {
	system("hg clone . checkout") and die;
}

my $d = "PseudoGBS_AU";

sub writeto {
	my ($contents) = @_;
	open(FH, ">>$d/source/contents.txt");
	print FH $contents;
	close(FH);
}

sub run {
	my ($tag) = @_;
	my $d2 = "$d/source/$tag";

	system("mkdir $d2") and die;
	system("rm -rf temp") and die;
	chdir("checkout") or die;
	system("hg book -i") and die;
	system("hg up $tag") and die;
	system("hg archive ../temp") and die;
	my $id = `hg id -i`;
	chdir("..") or die;
	writeto("$tag: $id");
	system("cp temp/LICENSE.txt $d2/") and die;
	system("cp -R temp/PublicUtility $d2/") and die;
	system("cp -R temp/AUPublic $d2/") and die;
	system("cp -R temp/AudioUnitInstrumentExample $d2/") and die;
}

system("mkdir PseudoGBS_AU") and die;
system("mkdir PseudoGBS_AU/source") and die;
system("cp README.md PseudoGBS_AU/README-IMPORTANT.md") and die;
system("cp channel4_helper_script.py PseudoGBS_AU") and die;
writeto("This directory contains archives of the following commits from https://bitbucket.org/runhello/pseudogbs_au\n\n");

run("Channel3");
run("Channel3Drumkit");
run("Channel4");
run("Channel4Drumkit");
