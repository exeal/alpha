#!/usr/local/bin/perl

# make_table.pl
# (c) 2004-2005 exeal
# JIS X02xx <-> UCS の変換表を作成。
# A2U のときは 0x2100 から 0x7EFF までを出力する。

use strict;
use integer;

sub showUsageAndExit($) {
	print(shift() . "\nusage: make_table.pl <a2u|u2a> [1|2]\n");
	exit;
}

showUsageAndExit('bad parameter number.') if($#ARGV != 0 and $#ARGV != 1);
showUsageAndExit('bad parameter ' . $ARGV[0]) if($ARGV[0] ne 'a2u' and $ARGV[0] ne 'u2a');
showUsageAndExit('bad parameter ' . $ARGV[1]) if($#ARGV == 1 and $ARGV[1] ne '1' and $ARGV[1] ne '2');

my $jisToUcs = $ARGV[0] eq 'a2u';
my $targetPlane = ($ARGV[1] ne '') ? ord($ARGV[1]) - ord('0') : 0;
my $REPLACEMENT_CHARACTER = $jisToUcs ? 'RP__CH' : '0';


my (@table, @tableSip);
my $line;
while($line = <STDIN>) {
	next if(length($line) == 0 or substr($line, 0, 1) eq '#');
	if($line =~ m/^(\d\-)?0x([A-F\d]+)\t[0U][x\+]([A-F\d]+)(\+[A-F\d]+)?\t\#\s(.+)$/) {
		if($targetPlane != 0) {
			my $plane = ($1 ne '') ? ord($1) - ord('0') : 0;
			next if($plane != $targetPlane);
		}
		my $jis = hex($2);
		my $ucs = hex($3);
		$jis += 0x10000 if(!$jisToUcs and $line =~ m/^2\-/);	# JIS X0213 plane 2
		if($4 ne '') {	# JIS X0213 character corresponds to 2 UCS characters
			next unless($jisToUcs);
			$ucs = ($ucs << 16) | hex(substr($4, 1, 4));
		}
		$ucs = hex($1) if($5 =~ m/Fullwidth:\sU\+([\w\d]+)$/);
		if(!$jisToUcs and $ucs >= 0x20000) {
			$tableSip[$ucs - 0x20000] = $jis;
		} else {
			$table[$jisToUcs ? $jis : $ucs] = $jisToUcs ? $ucs : $jis;
		}
	}
}

my $i;
my $start = $jisToUcs ? 0x2100 : 0x0000;
my $end = $jisToUcs ? 0x7F00 : $#table + 1;
for($i = $start; $i < $end; ++$i) {
	print(defined($table[$i]) ? sprintf('0x%04X', $table[$i]) : $REPLACEMENT_CHARACTER);
	print(($i % 0x10 == 0x0F) ? ",\n" : ',')
}
for($i = 0; $i < $#tableSip + 1; ++$i) {
	print(defined($tableSip[$i]) ? sprintf('0x%04X', $tableSip[$i]) : $REPLACEMENT_CHARACTER);
	print(($i % 0x10 == 0x0F) ? ",\n" : ',')
}

__END__