#!/usr/local/bin/perl

use strict;
use integer;

die if($#ARGV != 0);

my $fileName = $ARGV[0];
my $line;
my @table;

open(FH, $fileName) or die;
while($line = <FH>) {
	if($line =~ m/^0x([A-Fa-f\d]{2,4})\s+[0U][x\+]([A-Fa-f\d]{4})/) {
		my $native = hex($1);
		my $ucs = hex($2);
		$table[$ucs] = $native;
	}
}
close FH;

my $i;
for($i = 0; $i <= $#table; ++$i) {
	printf("0x%04X\t0x%02X\n", $i, $table[$i]) if(defined $table[$i]);
}

__END__