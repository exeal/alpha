#!/usr/local/bin/perl

# sjis_table_to_jis.pl
# (c) 2004 exeal
# シフト JIS ベースで書かれた JISX0213-UCS 変換表を JIS ベースのものに変換。
# 出力先は標準出力。
# ファイル先頭の注釈は全て取り除かれ、
# 1列目に (面番号)-(JIS) 、2列目に UCS 、3列目に注釈。

use strict;
use integer;

sub showUsageAndExit($) {
	print(shift() . "\nusage: sjis_table_to_jis.pl <table-file-name>\n");
	exit();
}

showUsageAndExit('bad parameter number.') if($#ARGV != 0);

my $tableFile = $ARGV[0];

open(FH, $tableFile) or die('can not open table file.\n');

my @table;
my $line;
while($line = <FH>) {
	next if(length($line) == 0 or substr($line, 0, 1) eq '#');
	if($line =~ m/^0x([A-F\d]+)/) {
		my $sjis = hex($1);
		next if($sjis < 0x100);
		my ($leadByte, $trailByte) = (($sjis & 0xFF00) >> 8, $sjis & 0x0FF);
		my ($jk, $jt, $plane);
		my $kuIsEven = $trailByte > 0x9E;

		# 区
		$jk = $leadByte * 2 - 0x101 + ($kuIsEven ? 1 : 0) if($leadByte >= 0x81 and $leadByte <= 0x9F);
		$jk = $leadByte * 2 - 0x181 + ($kuIsEven ? 1 : 0) if($leadByte >= 0xE0 and $leadByte <= 0xEF);
		$jk = $leadByte * 2 - 0x19B + ($kuIsEven ? 1 : 0) if(($leadByte >= 0xF5 and $leadByte <= 0xFC) or ($leadByte == 0xF4 and $kuIsEven));
		if(($leadByte >= 0xF0 and $leadByte <= 0xF3) or ($leadByte == 0xF4 and !$kuIsEven)) {
			$jk = !$kuIsEven ? 1 : 8 if($leadByte == 0xF0);
			$jk = !$kuIsEven ? 3 : 4 if($leadByte == 0xF1);
			$jk = !$kuIsEven ? 5 : 12 if($leadByte == 0xF2);
			$jk = !$kuIsEven ? 13 : 14 if($leadByte == 0xF3);
			$jk = 15 if($leadByte == 0xF4);
		}

		# 点
		if($jk % 2 == 0) {
			$jt = $trailByte - 0x9E;
		} else {
			$jt = $trailByte - 0x3F if($trailByte <= 0x3F + 63);
			$jt = $trailByte - 0x40 if($trailByte > 0x3F + 63);
		}

		# 面
		$plane = ($leadByte < 0xF0) ? 1 : 2;

		printf('%d-0x%X', $plane, (($jk << 8) | $jt) + 0x2020);
		print($');
	}
}
close(FH);

__END__