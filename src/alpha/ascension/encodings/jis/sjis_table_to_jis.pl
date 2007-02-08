#!/usr/local/bin/perl

# sjis_table_to_jis.pl
# (c) 2004 exeal
# �V�t�g JIS �x�[�X�ŏ����ꂽ JISX0213-UCS �ϊ��\�� JIS �x�[�X�̂��̂ɕϊ��B
# �o�͐�͕W���o�́B
# �t�@�C���擪�̒��߂͑S�Ď�菜����A
# 1��ڂ� (�ʔԍ�)-(JIS) �A2��ڂ� UCS �A3��ڂɒ��߁B

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

		# ��
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

		# �_
		if($jk % 2 == 0) {
			$jt = $trailByte - 0x9E;
		} else {
			$jt = $trailByte - 0x3F if($trailByte <= 0x3F + 63);
			$jt = $trailByte - 0x40 if($trailByte > 0x3F + 63);
		}

		# ��
		$plane = ($leadByte < 0xF0) ? 1 : 2;

		printf('%d-0x%X', $plane, (($jk << 8) | $jt) + 0x2020);
		print($');
	}
}
close(FH);

__END__