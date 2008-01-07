#!/usr/local/bin/perl

use strict;
use warnings;
use integer;
use XML::Parser;

die 'usage: perl dump-charmap.pl <input-file-name>' if($#ARGV != 0);

my @table;
my $substitution;

sub sh {
	my($p, $element, %attributes) = @_;
	if($element eq 'a') {
		$table[hex $attributes{'b'}] = hex $attributes{'u'};
	} elsif($element eq 'characterMapping') {
		print '# Reading ' . $attributes{'id'} . "...\n";
	} elsif($element eq 'assignments') {
		$substitution = '0x' . $attributes{'sub'};
	}
}

my $xml = new XML::Parser(Handlers => {Start => \&sh});
$xml->parsefile($ARGV[0]);

my $has_surrogates = 0;
my $iso_646_compatible = 1;
my($iso_646_c0_compatible, $iso_646_c1_compatible) = (1, 1);
my $has_euro_sign = 0;
for(my $i = 0; $i <= 0xFF; ++$i) {
	$table[$i] = 0xFFFD unless(defined $table[$i]);
	printf "0x%04X,", $table[$i];
	if($i != $table[$i]) {
		$iso_646_compatible = 0 if($i < 0x80);
		$iso_646_c0_compatible = 0 if($i < 0x20);
		$iso_646_c1_compatible = 0 if($i >= 0x80 and $i < 0xA0);
	}
	$has_surrogates = 1 if($table[$i] > 0xFFFF);
	$has_euro_sign = 1 if($table[$i] == 0x20AC);
	print((($i + 1) % 8 == 0) ? "\n" : ' ');
}

print "# Substitution byte is $substitution\.\n";
print '# Has surrogates [\u10000-\u10FFFF]? : ' . ($has_surrogates ? 'yes' : 'no') . "\n";
print '# ISO 646 compatible [0x00-0x7F]? : ' . ($iso_646_compatible ? 'yes' : 'no') . "\n";
print '# ISO 646 C0 compatible [0x00-0x1F]? : ' . ($iso_646_c0_compatible ? 'yes' : 'no') . "\n";
print '# ISO 646 C1 compatible [0x80-0x9F]? : ' . ($iso_646_c1_compatible ? 'yes' : 'no') . "\n";

my @ibm_pc_7bit = (
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F,
	0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017,
	0x0018, 0x0019, 0x001C, 0x001B, 0x007F, 0x001D, 0x001E, 0x001F,
	0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
	0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,
	0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
	0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
	0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
	0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
	0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
	0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,
	0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
	0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,
	0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
	0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x001A
);
my $ibm_pc_compatible = not $iso_646_compatible;
for(my $i = 0; $i < 0x80 and $ibm_pc_compatible; ++$i) {
	$ibm_pc_compatible = 0 if($table[$i] != $ibm_pc_7bit[$i]);
}
print '# IBM PC compatible [0x00-0x7F]? : ' . ($ibm_pc_compatible ? 'yes' : 'no') . "\n";
print '# Has Euro sign [\u20AC]? : ' . ($has_euro_sign ? 'yes' : 'no') . "\n";
