#!/usr/local/bin/perl

# props.pl (c) 2005-2007 exeal
#
# This script generates C++ source files
# - uprops-implementation
# - uprops-table
# for CharProperty class implementation from the public files obtained from Unicode.org
# - UnicodeData.txt
# - Blocks.txt
# - Scripts.txt
# - PropList.txt
# - CaseFolding.txt
# as input.

use strict;
use integer;

my %categoryMap = (
	Lu => 1, Ll => 2, Lt => 3, Lm => 4, Lo => 5,
	Mn => 6, Mc => 7, Me => 8,
	Nd => 9, Nl => 10, No => 11,
	Pc => 12, Pd => 13, Ps => 14, Pe => 15, Pi => 16, Pf => 17, Po => 18,
	Sm => 19, Sc => 20, Sk => 21, So => 22,
	Zs => 23, Zl => 24, Zp => 25,
	Cc => 26, Cf => 27, Cs => 28, Co => 29, Cn => 30
);

# process input files' directory
die "usage: props.pl [input-file-directory]\n" if($#ARGV > 0);
my $directory = shift @ARGV;
if($directory ne '') {
	$directory =~ s/\//\\/;
	$directory .= '\\' unless($directory =~ /\\$/);
}


# generate general categories code
sub processGeneralCategories() {
	open(INPUT, $directory . 'UnicodeData.txt')
		or die "Input file '${directory}UnicodeData.txt' not found.\n";
	print 'generating general categories table...' . "\n";
	print OUTPUT_TABLE 'const PropertyRange GeneralCategory::ranges_[] = {' . "\n";

	my ($first, $last) = (0, -1);
	my $continuedGc = 'Cc';

	while(<INPUT>) {
		next unless(/^([\dA-Fa-f]+)\;(.+?)\;(\w\w)/);
		my ($cp, $gc) = (hex $1, $3);
		my $isRange = ($2 =~ /^\<.+?First\>$/) ? 1 : 0;

		if($cp != $last + 1 or $gc ne $continuedGc) {	# not continued
			printf OUTPUT_TABLE '{0x%X,0x%X,%d},', $first, $last, $categoryMap{$continuedGc};
			$first = $cp;
			$continuedGc = $gc;
			if(!$isRange) {$last = $first;}
			else {
				my $nextLine = readline *INPUT;
				$nextLine =~ /^([\dA-Fa-f]+)/;
				$last = hex $1;
			}
		} else {	# continued
			if(!$isRange) {++$last;}
			else {
				my $nextLine = readline *INPUT;
				$nextLine =~ /^([\dA-Fa-f]+)/;
				$last = hex $1;
			}
		}
	}
	printf OUTPUT_TABLE "{0x%X,0x%X,%d}};\n", $first, $last, $categoryMap{$continuedGc};
	print OUTPUT_TABLE 'const size_t GeneralCategory::count_ = countof(GeneralCategory::ranges_);' . "\n";
	close INPUT;
}

# generate blocks code
sub processCodeBlocks() {
	open(INPUT, $directory . 'Blocks.txt')
		or die "Input file '${directory}Blocks.txt' not found.\n";
	print 'generating blocks table...' . "\n";
	print OUTPUT_TABLE 'const PropertyRange CodeBlock::ranges_[] = {' . "\n";

	my $blockNumber = 40;	# CodeBlock::BASIC_LATIN
	while(<INPUT>) {
		next unless(/^([\dA-Fa-f]+)\.\.([\dA-Fa-f]+)/);
		printf OUTPUT_TABLE '{0x%X,0x%X,%d},', hex($1), hex($2), $blockNumber++;
	}
	close INPUT;
	print OUTPUT_TABLE "};\n";
	print OUTPUT_TABLE 'const size_t CodeBlock::count_ = countof(CodeBlock::ranges_);' . "\n";
}

# generate scripts code
sub processScripts() {
	open(INPUT, $directory . 'Scripts.txt')
		or die "Input file '${directory}Scripts.txt' not found.\n";
	print 'generating scripts table...' . "\n";
	print OUTPUT_TABLE 'const PropertyRange Script::ranges_[] = {' . "\n";

	my @ranges;
	my $scriptNumber = 195;	# Script::COMMON
	my ($first, $last) = (0, -1);
	while(<INPUT>) {
		if(/^([\dA-Fa-f]+)\s+\;/) {	# singleton
			my $cp = hex $1;
			if($cp == $last + 1) {++$last;}
			else {
				my %newEntry = ('first' => $first, 'last' => $last, 'script' => $scriptNumber);
				push @ranges, \%newEntry;
				$first = $last = $cp;
			}
		} elsif(/^([\dA-Fa-f]+)\.\.([\dA-Fa-f]+)\s+\;/) {	# range
			my ($begin, $end) = (hex $1, hex $2);
			if($begin == $last + 1) {$last = $end;}
			else {
				my %newEntry = ('first' => $first, 'last' => $last, 'script' => $scriptNumber);
				push @ranges, \%newEntry;
				($first, $last) = ($begin, $end);
			}
		} elsif(/^\# Total/) {	# end of section
			my %newEntry = ('first' => $first, 'last' => $last, 'script' => $scriptNumber);
			push @ranges, \%newEntry;
			++$scriptNumber;
		}
	}
	@ranges = sort {$a->{first} <=> $b->{first}} @ranges;
	foreach(@ranges) {
		printf OUTPUT_TABLE '{0x%X,0x%X,%d},', $_->{first}, $_->{last}, $_->{script};
	}
	print OUTPUT_TABLE "};\n";
	print OUTPUT_TABLE 'const size_t Script::count_ = countof(Script::ranges_);' . "\n";
	close INPUT;
}

# generate binary properties code
sub processBinaryProperties() {
	open(INPUT, $directory . 'PropList.txt')
		or die "Input file '${directory}PropList.txt' not found.\n";
	print 'generating binary properties table...' . "\n";

	my @ranges;
	my $readPoints = 0;
	my $continuedPoints = 0;
	my $ucs4 = 0;
	my $propertyName;
	while(<INPUT>) {
		$propertyName = $1 if($readPoints == 0 and /^[\da-fA-F\.\s]+\;\s+(\w+)/);
		if(/^([\dA-Fa-f]+)\s+/) {	# singleton
			my $cp = hex $1;
			push @ranges, ($cp, $cp);
			++$readPoints;
			$ucs4 = 1 if($cp > 0xFFFF);
		} elsif(/^([\dA-Fa-f]+)\.\.([\dA-Fa-f]+)\s+/) {	# range
			my ($first, $last) = (hex $1, hex $2);
			push @ranges, ($first, $last);
			$readPoints += $last - $first + 1;
			$continuedPoints += $last - $first + 1;
			$ucs4 = 1 if($last > 0xFFFF);
		} elsif(/^\# Total/) {	# end of section
			if($continuedPoints * 3 / 2 < $readPoints) {	# simple array
				my $i;
				print BP_TABLE_DEFINITION $ucs4 ? 'static const CodePoint ' : 'static const Char ';
				print BP_TABLE_DEFINITION 'tableOfBp__' . $propertyName . "_[];\n";
				print OUTPUT_TABLE $ucs4 ? 'const CodePoint ' : 'const Char ';
				print OUTPUT_TABLE 'BinaryProperty::tableOfBp__' . $propertyName . '_[] = {';
				for($i = 0; $i <= $#ranges; $i += 2) {
					printf OUTPUT_TABLE '0x%04X,', $ranges[$i];
				}
				print OUTPUT_IMPL 'template<> inline bool BinaryProperty::is<BinaryProperty::' . uc($propertyName) . '>(CodePoint cp) {';
				print OUTPUT_IMPL 'return std::binary_search(tableOfBp__' . $propertyName
					. '_,tableOfBp__' . $propertyName
					. '_+' . ($#ranges + 1) / 2 . ','
					. ($ucs4 ? 'cp);' : 'static_cast<Char>(cp));')
					. "}\n";
			} else {	# range based array
				my $i;
				print BP_TABLE_DEFINITION $ucs4 ? 'static const internal::CodeRange<CodePoint> ' : 'static const internal::CodeRange<Char> ';
				print BP_TABLE_DEFINITION 'tableOfBp__' . $propertyName . "_[];\n";
				print OUTPUT_TABLE $ucs4 ? 'const CodeRange<CodePoint> ' : 'const CodeRange<Char> ';
				print OUTPUT_TABLE 'BinaryProperty::tableOfBp__' . $propertyName . '_[] = {';
				for($i = 0; $i <= $#ranges; $i += 2) {
					printf OUTPUT_TABLE '{0x%04X,0x%04X},', $ranges[$i], $ranges[$i + 1];
				}
				print OUTPUT_IMPL 'template<> inline bool BinaryProperty::is<BinaryProperty::' . uc($propertyName) . '>(CodePoint cp) {';
				print OUTPUT_IMPL 'return findInRange(tableOfBp__' . $propertyName
					. '_,tableOfBp__' . $propertyName
					. '_+' . ($#ranges + 1) . ',cp)!=0;'
					. "}\n";
			}
			print OUTPUT_TABLE "};\n";
			@ranges = ();
			$readPoints = $continuedPoints = 0;
			$ucs4 = 0;
		}
	}
	close INPUT;
}

# generate case-folding code
sub processCaseFolding() {
	open(INPUT, $directory . 'CaseFolding.txt')
		or die "Input file '${directory}CaseFolding.txt' not found.\n";
	print 'generating case-folding table...' . "\n";

	my (@cased, @folded);
	while(<INPUT>) {
		next unless(/([\dA-Fa-f]+)\;\s+[CS]\;\s+([\dA-Fa-f]+)/);
		push @cased, hex($1);
		push @folded, hex($2);
	}

	my ($i, $ucs2Count);
	print OUTPUT_TABLE 'const Char CaseFolder::CASED_UCS2[] = {' . "\n";
	for($i = 0; $i <= $#cased; ++$i) {
		if($cased[$i] >= 0x10000) {$ucs2Count = $i; last;}
		printf OUTPUT_TABLE '0x%X,', $cased[$i];
	}
	print OUTPUT_TABLE "};\n" . 'const CodePoint CaseFolder::CASED_UCS4[] = {' . "\n";
	for(; $i <= $#cased; ++$i) {printf OUTPUT_TABLE '0x%X,', $cased[$i];}
	print OUTPUT_TABLE "};\n" . 'const Char CaseFolder::FOLDED_UCS2[] = {' . "\n";
	for($i = 0; $i <= $#folded; ++$i) {
		last if($folded[$i] >= 0x10000);
		printf OUTPUT_TABLE '0x%X,', $folded[$i];
	}
	print OUTPUT_TABLE "};\n" . 'const CodePoint CaseFolder::FOLDED_UCS4[] = {' . "\n";
	for(; $i <= $#folded; ++$i) {printf OUTPUT_TABLE '0x%X,', $folded[$i];}
	print OUTPUT_TABLE "};\n";

	print OUTPUT_IMPL <<"END_OF_CASE_FOLDING";
inline CodePoint CaseFolder::foldSimple(CodePoint cp) {
	if(cp < 0x10000) {
		const Char* const p = std::lower_bound(CASED_UCS2, CASED_UCS2 + $ucs2Count, static_cast<Char>(cp));
		return (*p == cp) ? FOLDED_UCS2[p - CASED_UCS2] : cp;
	} else {
		const CodePoint* const p = std::lower_bound(CASED_UCS4, CASED_UCS4 + ($#cased + 1 - $ucs2Count), cp);
		return (*p == cp) ? FOLDED_UCS4[p - CASED_UCS4] : cp;
	}
}
END_OF_CASE_FOLDING

	close INPUT;
}

# generate NFD code
sub processNFD() {
	open(INPUT, $directory . 'UnicodeData.txt')
		or die "Input file '${directory}UnicodeData.txt' not found.\n";
	print 'generating NFD table...' . "\n";

	my (@src, @nfd);
	while(<INPUT>) {
		next unless(m/^([\dA-Fa-f]+)\;[^;]+\;[^;]+\;[^;]+\;[^;]+\;([\w\s]+)\;/);
		push @src, hex($1);
		my @pair = split(' ', $2);
		if($#pair == 0) {	# one-to-one mapping
			push @nfd, hex($pair[0]);
		} else {	# one-to-two mapping
			$pair[0] = hex($pair[0]);
			$pair[1] = hex($pair[1]);
			# this hack works at this time (Unicode 4.1)...
			# a SMP character is to two SMP characters
			# a SIP character is to one character
			($pair[0] -= 0x10000), ($pair[1] -= 0x10000) if($src[$#src] >= 0x10000 and $src[$#src] < 0x20000);
			push @nfd, $pair[0] << 16 | $pair[1];
		}
	}
	close INPUT;

	my $i;
	print BP_TABLE_DEFINITION 'static const CodePoint NFD_SRC[];';
	print BP_TABLE_DEFINITION 'static const CodePoint NFD_RES[];';
	print OUTPUT_TABLE 'const CodePoint CharProperty::NFD_SRC[] = {';
	for($i = 0; $i <= $#src; ++$i) {
		printf OUTPUT_TABLE '0x%X,', $src[$i];
	}
	print OUTPUT_TABLE "};\n";
	print OUTPUT_TABLE 'const CodePoint CharProperty::NFD_RES[] = {';
	for($i = 0; $i <= $#nfd; ++$i) {
		printf OUTPUT_TABLE '0x%X,', $nfd[$i];
	}
	print OUTPUT_TABLE "};\n";
	print OUTPUT_IMPL <<"END_OF_NFD";
inline std::size_t CharProperty::toNFD(CodePoint cp, CodePoint& first, CodePoint& second) {
	const CodePoint* const p = std::lower_bound(NFD_SRC, NFD_SRC + $#src - 1, cp);
	if(*p != cp)
		return (first = cp), 1;
	const CodePoint res = NFD_RES[p - NFD_SRC];
	if(res <= 0x10FFFF)
		return (first = res), 1;
	else if(cp >= 0x10000 && cp < 0x20000)
		return (first = (res >> 16) + 0x10000), (second = (res & 0xFFFF) + 0x10000), 2;
	else
		return (first = (res >> 16)), (second = (res & 0xFFFF)), 2;
}
inline length_t CharProperty::toNFD(CodePoint cp, Char* dest) {
	CodePoint first, second;
	if(toNFD(cp, first, second) == 1)
		return surrogates::encode(first, dest) ? 2 : 1;
	else {
		const length_t c = surrogates::encode(first, dest) ? 2 : 1;
		return c + surrogates::encode(second, dest + c) ? 2 : 1;
	}
}
END_OF_NFD
}


# open output files
my $header = '// automatically generated by props.pl at $ ' . scalar(localtime) . " \$\n";
die "Cannot open output file '../uprops-table'.\n" unless(open OUTPUT_TABLE, '>..\\uprops-table');
print OUTPUT_TABLE $header;
die "Cannot open output file '../uprops-binary-property-table-definition'.\n" unless(open BP_TABLE_DEFINITION, '>..\\uprops-binary-property-table-definition');
print BP_TABLE_DEFINITION $header;
die "Cannot open output file '../uprops-implementation'.\n" unless(open OUTPUT_IMPL, '>..\\uprops-implementation');
print OUTPUT_IMPL $header;

# version diagnostics
print OUTPUT_TABLE '#if ASCENSION_UNICODE_VERSION != 0x0500' . "\n";
print OUTPUT_TABLE '#error These codes are based on old version of Unicode.' . "\n";
print OUTPUT_TABLE '#endif' . "\n";

# process all!
processGeneralCategories();
processCodeBlocks();
processScripts();
processBinaryProperties();
processCaseFolding();
#processNFD();
print "done.\n";

close OUTPUT_TABLE;
close BP_TABLE_DEFINITION;
close OUTPUT_IMPL;

__END__