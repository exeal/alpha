#!/usr/local/bin/perl

# sbcs2ucs.pl (c) 2007 exeal

use strict;
use warnings;
use integer;

print "reading...\n";

my @table;
while(<STDIN>) {
  $table[hex($1)] = hex($2) if(m/^(0x[0-9A-Fa-f]{2})\s+(0x[0-9A-Fa-f]{4})\s+.*$/);
}

my $iso_646_compatible = 1;
for(my $i = 0; $i <= 0xFF; ++$i) {
  if(defined $table[$i]) {
    printf "0x%04X,", $table[$i];
    $iso_646_compatible = 0 if($i < 0x80 and $i != $table[$i]);
  } else {
    print '0xFFFD,';
    $iso_646_compatible = 0 if($i < 0x80);
  }
  print((($i + 1) % 8 == 0) ? "\n" : ' ');
}

print 'This charset is ';
print 'not ' unless($iso_646_compatible);
print "compatible with ISO 646.\n";
