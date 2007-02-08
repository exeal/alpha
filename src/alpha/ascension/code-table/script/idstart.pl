#!/usr/local/bin/perl

# idstart.pl (c) 2003-2004 exeal
# CEditView::IsIdentifierStartChar �᥽�åɼ����Τ���ˡ�
# DerivedCoreProperties.txt ���饳���ɥݥ���ȤΥ���޶��ڤ�Υꥹ�Ȥ��������롣
# �����������ɥݥ���Ȥ�500�ʾ�Ϣ³���Ƥ���н��Ϥ���ʤ���
# ������1�Ĥ�2��:
#
#   idstart.pl [-d:directory]
#
# �������ɸ����ϡ�

use strict;
use integer;

# show usage and abort
sub usage($) {
	die("\n" . shift() . "\nUsage: idstart.pl [-d:directory]\n\n");
}

my $fileName = 'DerivedCoreProperties.txt';
my $pattern;


if($#ARGV == 0) {
	usage("Parameter is illegal.") unless($ARGV[0] =~ /\-d\:(.+)/);
	my $path = $1;
	$path =~ tr/\"//;
	$fileName = $path . "\\" . $fileName;
} elsif($#ARGV != -1) {
	usage("Bad parameter number.");
}

my $line;
my $matchCount = 0;

if(!open(FH, $fileName)) {
	die("\nCannot open $strFileName.\nIf UnicodeData.txt is not current directory, you can use -d:<directory> switch to specify where the file is.\n");
}

while($line = <FH>) {
	if($line =~ m/^(.+?)\s*\;\s*(.+?)\s*\#/) {
		next if($2 ne 'ID_Start');
		my $cp = $1;
		if($cp =~ m/^([0-9A-Z]+)\.\.([0-9A-Z]+)$/) {
			my $start = hex($1);
			my $end = hex($2);
			my $i;
			next if($end - $start > 500);
			for($i = $start; $i <= $end; ++$i) {
				print(($matchCount++ % 8 == 0) ? "\n" : ' ');
				printf(($i < 0x10000) ? "0x%04lX," : "0x%lX,", $i);
			}
		} else {
			print(($matchCount++ % 8 == 0) ? "\n" : ' ');
			print("0x$cp,");
		}
	}
}

close(FH);

# print "\n$matchCount characters output.\n\n";

__END__