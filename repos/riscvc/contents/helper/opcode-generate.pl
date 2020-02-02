#!/usr/bin/perl
use strict;
use sort 'stable'; # Needed for funny sort later

# Argument 0 is path to opcode-map.tex from https://github.com/riscv/riscv-isa-manual
# Argument 1 is a C file where //! is treated as special, and content is snipped between the following:
# //! PROLOGUE
# //! END
# //! METHOD
# //! CONTENT
# //! END
# The PROLOGUE will be shoved at the top of the file. This will be followed by a function with signature METHOD.
# CONTENT is interpreted in the following way:
#     INSTRNAME:
#         code goes here
#     default:
#         code goes here
# Indentation is significant for CONTENT.

my $tab = q[    ];

my ($path, $input, $output, $opcode) = @ARGV;
$path or die "Please give path to opcode-map.tex and instr-table.tex";
$input or die "No input file given";
$output or die "No output file given";
$opcode or die "No opcode file given";

my $tp;
$tp = "$path/opcode-map.tex"; open(FH, $tp) or die "Couldn't open $tp";

open(OUT, ">$opcode") or die "Couldn't open $opcode for write";
print(OUT "#pragma once\n\n");

my %opcodes = ();

my $tabley;
for(<FH>) {
	my @line = split(" & "); # This file seems to use this as a delimiter

	# Table has: 2 top rows label, 1 label column left and right, content is 7x4
	if (@line >= 9) {
		if ($tabley >= 2) {
			for my $tablex (1..7) {
				my $opcode = $line[$tablex];
				$opcode =~ s/^\s*(\S*)\s*$/$1/s;
				next if ($opcode =~ /\{/); # Ignore anything italicised for now
				$opcode =~ s/-/_/g;
				my $opcodepad = " "x(9-length($opcode)); # Pad to 9 chars

				my $value = 0b11; # Standard instruction set
				$value |= ( ($tablex - 1) << 2 ); # "inst[4:2]"
				$value |= ( ($tabley - 2) << 5 ); # "inst[6:5]"
				my $valuestr = sprintf("  0x%02x     // %d", $value, $value);

				print(OUT "#define $opcode$opcodepad $valuestr\n");
				$opcodes{$value} = $opcode;
			}
		}
		$tabley++;
	}
}

close(FH);
close(OUT);

$tp = "$path/instr-table.tex"; open(FH, $tp) or die "Couldn't open $tp";
open(SOURCE, $input) or die "Couldn't open $input";

sub hashWith { local $_; map { $_ => 1 } @_ }
sub hashReverse { local $_; map { $_[$_] => $_ } (0..@_) }
sub strim { local $_ = $_[0]; s/^\s*//s; s/\s*$//s; return $_ }

my @sourceSections;
my %sourceSectionNames = hashReverse(qw(PROLOGUE METHOD CONTENT));
my $sourceSection;
for(<SOURCE>) {
	if (s/^\s*\/\/\s*\!(.*)$/$1/s) {
		$_ = strim($_);
		if ($_ eq "END") { $sourceSection = undef; }
		elsif (exists($sourceSectionNames{$_})) { $sourceSection = $sourceSectionNames{$_} }
	} elsif (defined($sourceSection)) {
		$sourceSections[$sourceSection] .= $_;
	}
}

open(OUT, ">$output") or die "Couldn't open $output for write";

for (0,1) { $sourceSections[$_] = strim($sourceSections[$_]) }
print(OUT sprintf("%s\n\n%s\n", $sourceSections[0], $sourceSections[1]));

my ($contentPrefix, $labelStrip, $codeStrip); my %instructionCode;
{ $sourceSections[1] =~ /([^\n\r]*)$/s; my $x = $1; $x =~ /^(\s*)/; $contentPrefix = $1; }

{
	my $name;
	for my $line (split(/[\r\n|\n]/, $sourceSections[2])) {
		if ($line =~ /^(\s*)(\S.*)$/s) {
			my ($prefix, $content) = ($1,$2);
			$content =~ s/\s+$//s;
			if ($content =~ /^(\S+)\s*\:$/s and (not defined($labelStrip) or $prefix eq $labelStrip)) {
				$name = $1;
				$codeStrip = undef;
			} else {
				die qq[Stray line in CONTENT before first label: "$content"] if (not defined($name));

				if (not defined($codeStrip)) {
					die qq[Confusing indentation on this line in CONTENT:] if ($prefix !~ /^$labelStrip\s/s);
					$codeStrip = $prefix;
					$prefix = "";
				} else {
					$prefix =~ s/^$codeStrip//s;
				}
				$instructionCode{$name} .= "$prefix$content\n";
			}
		} elsif (defined($name)) {
			$instructionCode{$name} .= "\n";
		}
	}
}
for my $key (keys %instructionCode) { $instructionCode{$key} =~ s/\s+$//s; }

# R-type: 421111 (funct7, rs2, rs1, funct3, rd, opcode)
# I-type: 61111 (imm[11:0], rs1, funct3, rd, opcode)
# S-type: 421111 (imm[11:5], rs2, rs1, funct3, imm[4:0], opcode)
# B-type: 421111 (imm[12$\vert$10:5], rs2, rs1, funct3, imm[4:1$\vert$11], opcode)
# U-type: 811 (imm[31:12], rd, opcode)
# J-type: 811 (imm[20$\vert$10:1$\vert$11$\vert$19:12], rd, opcode)

my $intable;  # As we scan the file: Are we looking at the RV32I table?
my %row = (); # Data for this row of the table (the gap between a lone & and a \cline)
sub resetRow { %row = ("lastcol" => [], "signature" => ""); }

my @seenOpcodes = (); # Order in which opcodes were first seen in the table
my %dataOpcodes = (); # Arrays of instruction data sorted by opcode

# Translate the strings of column widths in TeX to instruction type codes that look
# like the ones in the manual. Needs further translation based on checking for funct7
# FENCE is a funny variant of "I" with further packing in the immediate
# SYSTEM is formally I-type but all fields are constant except the immediate, which the manual describes as a "funct12".
my %signatureIs = ("811" => "UJ", "61111" => "I", "421111" => "RSB", "2311111" => "FENCE");
my %hasFunct3 = hashWith(qw(R I S B FENCE));
my %hasFunct7 = hashWith(qw(R));
my %hasFunct12 = hashWith(qw(SYSTEM));
my %leftTag = (S => "imm[11:5]", B => 'imm[12$\\vert$10:5]', U => "imm[31:12]", "J" => 'imm[20$\\vert$10:1$\\vert$11$\\vert$19:12]');

sub isBinary { return $_[0] !~ /[^01]/ }
sub binary { return oct("0b".$_[0]) } # Binary string -> number
sub leftTagFilter { # Converts RSB -> R, S or B, UJ -> U or J (returns undef for R)
	my ($signature, $lastcol, $whitelistRef) = @_;
	for my $key (@$whitelistRef) {
		if ($$lastcol[0] eq $leftTag{$key}) {
			return $key;
		}
	}
}

# Build @seenOpcodes/%dataOpcodes
for(<FH>) { # Scan line by line
	if ($intable) { # Between table title and \end
		if (/\\end/) { $intable = 0; next; } # Done with entire table
		elsif (/\\cline/) { # Done with one row
			if ($row{instr}) { # Did you get an instruction name?
				my $lastcol = $row{lastcol}; # Columns in the row [ie the last tex argument of each stanza]
				my $opcode = $opcodes{binary($$lastcol[-1])}; # Opcode name
				my $dataOpcodeArray = $dataOpcodes{$opcode}; # Instructions of this opcode 
				if (!$dataOpcodeArray) { # This is the first instruction of this opcode
					push(@seenOpcodes, $opcode);
					$dataOpcodeArray = [];
					$dataOpcodes{$opcode} = $dataOpcodeArray;
				}
				my $dataOpcode = {instr => $row{instr}}; # Description of this instruction
				push(@$dataOpcodeArray, $dataOpcode);

				my $signatureCode = $row{signature}; # Signature code
				my $signature = $signatureIs{$signatureCode}; # Signature letter
				my $roughSignature = $signature; # Delete this line
				if (!$signature) { die "For $row{instr} unrecognized signature code $signatureCode" }
				elsif ($opcode eq "SYSTEM") { # SYSTEM is formatted like an I
					$signature = "SYSTEM";
				} elsif ($signature eq "UJ") {
					$signature = leftTagFilter($signature, $lastcol, [qw[U J]])
						or die "For $row{instr} unrecognized immediate code $$lastcol[0]";
				} elsif ($signature eq "RSB") {
					$signature = leftTagFilter($signature, $lastcol, [qw[S B]]);
					if (!$signature) {
						if (isBinary($signature)) { $signature = "R" }
						else { die "For $row{instr} unrecognized immediate code $$lastcol[0]" }
					}
				}

				if ($hasFunct3{$signature}) {
					$$dataOpcode{hasFunct3} = 1;
					$$dataOpcode{FUNCT3} |= (binary($$lastcol[-3]));
				}
				if ($hasFunct7{$signature}) {
					$$dataOpcode{hasFunct7} = 1;
					$$dataOpcode{FUNCT7} |= (binary($$lastcol[0])); # Assume all funct7s also have funct3
				}
				if ($hasFunct12{$signature}) {
					$$dataOpcode{hasFunct12} = 1;
					$$dataOpcode{FUNCT12} |= (binary($$lastcol[0])); # Assume no funct12s have another funct
				}
				if ($$lastcol[1] eq "shamt") {
					$$dataOpcode{shamt} = 1;
				}

				$$dataOpcode{instr} = $row{instr};
				$$dataOpcode{signature} = $signature;

				#print "// $row{instr}: $opcode (";
				#print join(", ", @$lastcol);
				#print(") signature: $signature [$roughSignature] funct " . sprintf("0x%02x",$$dataOpcode{funct}) . " shamt:$$dataOpcode{shamt}\n");
			}
			resetRow();
		} elsif (s/.*?\\multicolumn\{([^\}]*)\}//) { # Looking for \multicolumn{a}{b}{c}{d}, want first and last {} group.
			$row{signature} .= $1; # Use the first {} to build the "signature" (see below)
			my $last = "";
			while (s/^\s*{([^\}]*)\}//) { $last = $1; } # Append the final {} to the $lastcol array
			my $lastcol = $row{lastcol};
			push(@$lastcol, $last);
			if (/\s*\&\s*(\S+)/) { $row{instr} = $1; } # The space after the & is always empty but the last line puts the instruction name there
		}
	} else { # Search for start of table
		$intable = /RV32I Base Instruction Set/;
		resetRow() if ($intable);
	}
}

# sort same funct3 values together within each opcode
for my $key (@seenOpcodes) {
	my $arr = $dataOpcodes{$key};
	@$arr = sort { $$a{hasFunct3} ? $$a{FUNCT3} <=> $$b{FUNCT3} : 0 } @$arr;
}

my $o = ""; # Build this string
sub o { # Append a line to the output string with the given indentation
	my ($i, $v) = @_;
	if ($v) { $o .= $contentPrefix . ($tab x $i) . $v . "\n"; }
	else { $o .= "\n"; }
}
sub instructionCode {
	my ($i, $name, $noFirstLine) = @_;
	if (exists($instructionCode{$name})) {
		my $content = $instructionCode{$name};
		if ($content) {
			o() unless ($noFirstLine);
			for my $line (split(/[\r\n|\n]/, $content)) {
				o($i, $line);
			}
		}
	} else {
		o() unless ($noFirstLine);
		o($i, "// WARNING: No implementation");
	}
}
sub closeSwitch { # The end of every switch() statement is the same
	my ($i, $alreadyBlanked) = @_;
	o() unless ($alreadyBlanked); # Caller may suppress blank line
	o($i+1, "default: {");
	instructionCode($i+2, "default", 1);
	o($i+1, "} break;");
	o($i,   "}");
}
sub closeSwitch7 { # The end of the switch() for FUNCT7 is shaped a litle funny
	my ($i) = @_;
	closeSwitch($i+1, 1);
	o($i,"} break;");
}

my $anyOpcodes;

if ($contentPrefix) { o(); }
else { $contentPrefix = $tab; }

o(0, "switch(VREAD(instr, OPCODE)) {"); # A tree of nested switches: First on opcode, then funct3/funct12, then funct7.
for my $opcode (@seenOpcodes) {
	if ($anyOpcodes) { o(); } else { $anyOpcodes = 1; }
	o(1, "case $opcode: {");
	my $instrs = $dataOpcodes{$opcode};
	
	@$instrs > 0 or die "How did you get here??";
	my $firstInstr = $$instrs[0];
	my $topFunct; # Does this instruction use funct3/funct12? If so which one?
	my $lastTopFunct; # If funct3/funct12 is used, what was its last value?
	if ($$firstInstr{hasFunct3}) { $topFunct = "FUNCT3" }
	elsif ($$firstInstr{hasFunct12}) { $topFunct = "FUNCT12" }

	my $i = 2;

	if ($topFunct) {
		o($i, "switch (VREAD(instr, $topFunct)) {");
		$i++;
	}

	for my $instr (@$instrs) { # For each of this opcode's instructions:
		o() if $instr != $firstInstr;
		if ($topFunct) { # Handle funct3/funct12
			my $functValue = $$instr{$topFunct};
			# The funct3/funct12 will be different for each instruction UNLESS funct7 is in use.
			if ($lastTopFunct ne $functValue) { # If the funct3/funct12 changed
				if ($i > 3) { # Close off any funct7 switch we were building
					closeSwitch7(3);
					o();
					$i = 3;
				}
				o($i, sprintf("case 0x%02x: {", $functValue)); # New funct3 case
				$i++;
				if ($$instr{hasFunct7}) {
					o($i, "switch (VREAD(instr, FUNCT7)) {"); # Open new funct7 switch
					$i++;
				}
				$lastTopFunct = $functValue;
			}
			if ($$instr{hasFunct7}) {
				o($i, sprintf("case 0x%02x: {", $$instr{FUNCT7})); # New funct7 case
				$i++;
			}
		}

		# CODE HERE
		o($i, "// $$instr{instr}");

		instructionCode($i, $$instr{instr});

		o(--$i, "} break;") if ($topFunct); # Could be closing a funct3, funct7 or funct12
	}

	if ($i > 3) { # The loop ended but we were still building a funct7. Close it off
		o();
		closeSwitch7(3);
	}

	if ($topFunct) { # Close off a funct3/funct12 switch
		closeSwitch(2);
	}

	o(1, "} break;");
}
closeSwitch(0); # Close opcode switch

$o .= "}\n"; # Last thing close off METHOD. No prefix at all

print(OUT "$o\n");

