#!/usr/local/bin/perl

# prop-names.pl (c) 2007 exeal
#
# This script generates C++ code snippet for property name implementation.
# See ascension/unicode-utils.cpp.

use strict;
use warnings;
use IO::File;

sub buildID($) {
    $_ = shift;
    s/([[:upper:]])([[:upper:]][[:lower:]])/$1_$2/g;
    s/([[:lower:]])([[:upper:]])/$1_$2/;
    s/\-/_/;
    uc;
}

die 'usage: perl prop-names.pl <property-name> <input-directory>' if $#ARGV != 1;

my $property = $ARGV[0];
my $fileName = $ARGV[1];
$fileName .= '/' unless $fileName =~ /[\/\\]$/;
$fileName .= 'PropertyValueAliases.txt';

my $input = new IO::File($fileName) or die "file not found: $fileName.\n";
my $gc = $property =~ /gc/i;
unless($property =~ /ccc/i) {
	my $pattern = '^' . $property . '\s*;\s([\w\-\/]+)\s+;\s([\w\-]+)(\s+;\s([\w\-]+))?';
	while(<$input>) {
	    next unless /$pattern/i;
	    print "\t";
	    if($1 ne 'n/a') {	# the second field is an abbreviated name or 'n/a'
	    	print(($gc and length($1) == 1) ? "names_[L\"$1&\"] = " : "names_[L\"$1\"] = ");
	    }
	    print "names_[L\"$2\"] = " if $2 ne $1;		# the third field is a long name
	    print "names_[L\"$4\"] = " if $4;			# the optional forth field is another alias
	    printf("%s;\n", buildID $2);
	}
} else {
	my $pattern = '^' . $property . '\s*;\s(\d+)\s*;\s([\w\-\/]+)\s+;\s([\w\-]+)';
	while(<$input>) {
	    next unless /$pattern/i;
	    print "\t";
	    print "names_[L\"$2\"] = ";	# the third field is an abbreviated name
	    print "names_[L\"$3\"] = ";	# the forth field is a long name
	    printf("%d;\n", $1);		# the second field is an numeric value
	}
}

__END__
