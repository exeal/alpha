#!/usr/local/bin/perl

# dif208-213.pl
# (c) 2004 exeal
# JIS X0208 ‚Æ X0213 ‚Ì•ÏŠ·•\‚ğ“Ç‚İ‚ñ‚Å X0213 ‚É‚µ‚©Œ»‚ê‚È‚¢•¶š‚ğo—Í‚·‚éB
# ‘ÎÛ‚Í X0213 ‚Ì‘æ1–Ê‚Ì•¶š‚Ì‚İ

use strict;
use integer;

sub showUsageAndExit($) {
	print(shift() . "\nusage: dif208-213.pl <x208-table> <x213-table> <j|u>\n");
	exit();
}

showUsageAndExit('bad parameter number.') if($#ARGV != 2);
showUsageAndExit('bad parameter ' . $ARGV[2]) if($ARGV[2] ne 'j' and $ARGV[2] ne 'u');

my ($x208tableFile, $x213tableFile) = ($ARGV[0], $ARGV[1]);

open(FH, $x208tableFile) or die("Can not open table file: $x208tableFile \n");

my @table208;
my $line;

while($line = <FH>) {
	$table208[hex $1] = 1 if($line =~ m/^0x([A-Fa-f\d]+)\sU\+[A-Fa-f\d]+/);
}
close FH;

open(FH, $x213tableFile) or die("Can not open table file: $x213tableFile \n");

my @table213Only;

while($line = <FH>) {
	if($line =~ m/^1\-0x([A-Fa-f\d]+)\sU\+[A-Fa-f\d]+/) {
		$table213Only[hex $1] = 1 if(!defined($table208[hex $1]));
	}
}
close FH;

my $lastDefined = -1;

for($line = 0; $line < 0x10000; ++$line) {
	if(defined $table213Only[$line]) {
		$lastDefined = $line if($lastDefined == -1);
	} elsif($lastDefined != -1) {
		if($lastDefined != $line - 1) {
			printf "%04X .. %04X\n", $lastDefined, $line - 1;
		} else {
			printf "%04X\n", $lastDefined;
		}
		$lastDefined = -1;
	}
}

__END__