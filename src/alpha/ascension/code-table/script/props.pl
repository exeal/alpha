#!/usr/local/bin/perl

# props.pl (c) 2005-2007 exeal
#
# This script generates C++ source files
# - uprops-implementation
# - uprops-table
# - uprops-binary-property-table-definition
# for ascension.unicode.ucd.* Unicode property classes. This uses the folowing public files
# obtained from Unicode.org:
# - UnicodeData.txt
# - Blocks.txt
# - Scripts.txt
# - PropList.txt
# - CaseFolding.txt
# - EastAsianWidth.txt
# - LineBreak.txt

use strict;
use warnings;
use integer;
use IO::File;

my %categoryMap = (
	Lu => 1, Ll => 2, Lt => 3, Lm => 4, Lo => 5,
	Mn => 6, Mc => 7, Me => 8,
	Nd => 9, Nl => 10, No => 11,
	Pc => 12, Pd => 13, Ps => 14, Pe => 15, Pi => 16, Pf => 17, Po => 18,
	Sm => 19, Sc => 20, Sk => 21, So => 22,
	Zs => 23, Zl => 24, Zp => 25,
	Cc => 26, Cf => 27, Cs => 28, Co => 29, Cn => 30
);

my %eastAsianWidthMap = (
	F => 318, H => 319, W => 320, Na => 321, A => 322, N => 323
);

my %lineBreakMap = (
	# normative
	BK => 324, CR => 325, LF => 326, CM => 327, SG => 329, GL => 332, CB => 338, SP => 333, ZW => 331,
	NL => 328, WJ => 330, JL => 355, JV => 356, JT => 357, H2 => 352, H3 => 353,
	# informative
	XX => 359, OP => 343, CL => 339, QU => 344, NS => 342, EX => 340, SY => 349,
	IS => 345, PR => 348, PO => 347, NU => 346, AL => 351, ID => 354, IN => 341, HY => 337,
	BB => 336, BA => 335, SA => 358, AI => 350, B2 => 334
);

# process input files' directory
die "usage: props.pl [input-file-directory]\n" if($#ARGV > 0);
my $directory = shift @ARGV;
if($directory ne '') {
	$directory =~ s/\//\\/;
	$directory .= '\\' unless($directory =~ /\\$/);
}

my $header = '// automatically generated by props.pl at $ ' . scalar(localtime) . " \$\n";


# converts a list of code points to a C++ wide string literal
sub cp2str(@) {
	my $result = '';
	for(@_) {
		$_ = hex $_;
		if($_ < 0x010000) {
			$result .= sprintf('\x%X', $_);
		} else {
			$result .= sprintf('\x%X\x%X', (($_ >> 10) & 0xFFFF) + 0xD7C0, ($_ & 0x03FF) | 0xDC00);
		}
	}
	return 'L"' . $result . '"';
}

# generate General_Category code
sub processGeneralCategories() {
	open(INPUT, $directory . 'UnicodeData.txt')
		or die "Input file '${directory}UnicodeData.txt' not found.\n";
	print 'generating General_Category table...' . "\n";
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

# generate Block code
sub processCodeBlocks() {
	open(INPUT, $directory . 'Blocks.txt')
		or die "Input file '${directory}Blocks.txt' not found.\n";
	print 'generating Block table...' . "\n";
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

# generate two arrays table for Canonical_Combining_Class
sub processCanonicalCombiningClasses() {
	my $input = new IO::File($directory . 'UnicodeData.txt') or die "Can't open '${directory}UnicodeData.txt'.\n";
	print "generating CanonicalCombining_Class table...\n";

	my (@cp, @ccc);
	while(<$input>) {
		next unless /^([0-9A-F]+);[^;]+;[^;]+;(\d+)/;
		my $klass = int $2;
		if($klass != 0) {
			push @cp, hex($1);
			push @ccc, $klass;
		}
	}
	print OUTPUT_TABLE "#ifndef ASCENSION_NO_UNICODE_NORMALIZATION\n";
	print OUTPUT_TABLE 'const Char CanonicalCombiningClass::SRC_UCS2[] = {';
	my ($i, $j, $numberOfUCS2);
	for($i = 0; $cp[$i] < 0x10000; ++$i) {printf(OUTPUT_TABLE "0x%X,", $cp[$i]);}
	print OUTPUT_TABLE "};\nconst uchar CanonicalCombiningClass::DEST_UCS2[] = {";
	$numberOfUCS2 = $i;
	for($j = 0; $j < $numberOfUCS2; ++$j) {printf(OUTPUT_TABLE "%d,", $ccc[$j]);}
	print OUTPUT_TABLE "};\nconst CodePoint CanonicalCombiningClass::SRC_UCS4[] = {";
	for(; $i <= $#cp; ++$i) {printf(OUTPUT_TABLE "0x%X,", $cp[$i]);}
	print OUTPUT_TABLE "};\nconst uchar CanonicalCombiningClass::DEST_UCS4[] = {";
	for(; $j <= $#ccc; ++$j) {printf(OUTPUT_TABLE "%d,", $ccc[$j]);}
	printf(OUTPUT_TABLE "};\nconst size_t CanonicalCombiningClass::UCS2_COUNT = %d;\n", $numberOfUCS2);
	printf(OUTPUT_TABLE "const size_t CanonicalCombiningClass::UCS4_COUNT = %d;\n", ($#ccc + 1) - $numberOfUCS2);
	print OUTPUT_TABLE "#endif /* !ASCENSION_NO_UNICODE_NORMALIZATION */\n";
	$input->close;
}

# generate Script code
sub processScripts() {
	open(INPUT, $directory . 'Scripts.txt')
		or die "Input file '${directory}Scripts.txt' not found.\n";
	print 'generating Script table...' . "\n";
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
	my $input = new IO::File($directory . 'CaseFolding.txt')
		or die "Input file '${directory}CaseFolding.txt' not found.\n";
	print 'generating case-folding table...' . "\n";

	my (@commonCased, @commonFolded, @simpleCased, @simpleFolded, @fullCased, @fullFolded);
	while(<$input>) {
		next unless(/^([\dA-F]{4});\s+([CSF]);\s+([^;]+)/);
		if($2 eq 'C') {
			push @commonCased, $1;
			push @commonFolded, $3;
		} elsif($2 eq 'S') {
			push @simpleCased, $1;
			push @simpleFolded, $3;
		} else {
			push @fullCased, $1;
			push @fullFolded, 'L"\x' . join('\x', split(' ', $3)) . '"';
		}
	}
	$input->close;

	print OUTPUT_TABLE
		"const Char CaseFolder::COMMON_CASED[] = {\n0x", join(',0x', @commonCased),
		"};\nconst Char CaseFolder::COMMON_FOLDED[] = {\n0x", join(',0x', @commonFolded),
		"};\nconst Char CaseFolder::SIMPLE_CASED[] = {\n0x", join(',0x', @simpleCased),
		"};\nconst Char CaseFolder::SIMPLE_FOLDED[] = {\n0x", join(',0x', @simpleFolded),
		"};\nconst Char CaseFolder::FULL_CASED[] = {\n0x", join(',0x', @fullCased),
		"};\nconst Char* CaseFolder::FULL_FOLDED[] = {", join(',', @fullFolded),
		"};\nconst size_t CaseFolder::NUMBER_OF_COMMON_CASED = ", $#commonCased + 1,
		";\nconst size_t CaseFolder::NUMBER_OF_SIMPLE_CASED = ", $#simpleCased + 1,
		";\nconst size_t CaseFolder::NUMBER_OF_FULL_CASED = ", $#fullCased + 1, ";\n";
}

# generate decomposition mapping tables
sub processDecompositionMappings() {
	my $input = new IO::File($directory . 'UnicodeData.txt')
		or die "Input file '${directory}UnicodeData.txt' not found.\n";
	my $output = new IO::File('>../uprops-decomposition-mapping-table')
		or die "Output file ../uprops-decomposition-mapping-table can't open.\n";
	print 'generating decomposition mapping table...' . "\n";
	print $output $header;

	my (@canonical, @compatibility);
	my ($canonicals, $compatibilities) = (0, 0);
	while(<$input>) {
		next unless(m/^([\dA-F]{4,6});[^;]+;[^;]+;[^;]+;[^;]+;(\<\w+\> )?([^;]+);/);
		unless (defined $2) {
			$canonical[hex $1] = cp2str(split(' ', $3));
			++$canonicals;
		} else {
			$compatibility[hex $1] = cp2str(split(' ', $3));
			++$compatibilities;
		}
	}

	my $i;
	print $output 'const CodePoint CANONICAL_MAPPING_SRC[] = {';
	for($i = 0; $i <= $#canonical; ++$i) {printf($output '0x%X,', $i) if defined $canonical[$i];}
	print $output "};\nconst Char* CANONICAL_MAPPING_DEST[] = {";
	for(@canonical) {print $output "$_," if defined;}
	print $output "};\nconst size_t NUMBER_OF_CANONICAL_MAPPINGS = $canonicals",
		";\n#ifndef ASCENSION_NO_UNICODE_COMPATIBILITY_MAPPING\nconst CodePoint COMPATIBILITY_MAPPING_SRC[] = {";
	for($i = 0; $i <= $#compatibility; ++$i) {printf($output '0x%X,', $i) if defined $compatibility[$i];}
	print $output "};\nconst Char* COMPATIBILITY_MAPPING_DEST[] = {";
	for(@compatibility) {print $output "$_," if defined;}
	print $output "};\nconst size_t NUMBER_OF_COMPATIBILITY_MAPPINGS = $compatibilities;\n#endif\n";

	$input->close;
	$output->close;
}

# generate East_Asian_Width code
sub processEastAsianWidths() {
	my $input = new IO::File($directory . 'EastAsianWidth.txt')
		or die "Input file '${directory}EastAsianWidth.txt' not found.\n";
	my ($first, $last, $ea) = (0, -1, 'ZZ');
	my $isRange = 0;

	print 'generating East_Asian_Width table...' . "\n";
	print OUTPUT_TABLE 'const PropertyRange EastAsianWidth::ranges_[] = {' . "\n";
	while(<$input>) {
		if(m/^([\dA-F]{4,6});([A-Z]{1,2})/ or (($isRange = 1) and m/^([\dA-F]{4,6})\.\.([\dA-F]{4,6});([A-Z]{2})/)) {
			my ($cp, $prop) = (hex($1), $isRange ? $3 : $2);
			if($prop ne $ea or $cp != $last + 1) {
				if($last != -1) {
					printf OUTPUT_TABLE '{0x%X,0x%X,%d},', $first, $last, $eastAsianWidthMap{$ea};
				}
				($first, $ea) = ($cp, $prop);
			}
			$last = $isRange ? hex($2) : $cp;
		}
		$isRange = 0;
	}
	printf OUTPUT_TABLE '{0x%X,0x%X,%d}};', $first, $last, $eastAsianWidthMap{$ea};
	print OUTPUT_TABLE "\nconst size_t EastAsianWidth::count_ = countof(EastAsianWidth::ranges_);\n";
	$input->close;
}

# generate Line_Break code
sub processLineBreaks() {
	my $input = new IO::File($directory . 'LineBreak.txt')
		or die "Input file '${directory}LineBreak.txt' not found.\n";
	my ($first, $last, $lb) = (0, -1, 'ZZ');
	my $isRange = 0;

	print 'generating Line_Break table...' . "\n";
	print OUTPUT_TABLE 'const PropertyRange LineBreak::ranges_[] = {' . "\n";
	while(<$input>) {
		if(m/^([\dA-F]{4,6});([A-Z]{2})/ or (($isRange = 1) and m/^([\dA-F]{4,6})\.\.([\dA-F]{4,6});([A-Z]{2})/)) {
			my ($cp, $prop) = (hex($1), $isRange ? $3 : $2);
			if($prop ne $lb or $cp != $last + 1) {
				if($last != -1) {
					printf OUTPUT_TABLE '{0x%X,0x%X,%d},', $first, $last, $lineBreakMap{$lb};
				}
				($first, $lb) = ($cp, $prop);
			}
			$last = $isRange ? hex($2) : $cp;
		}
		$isRange = 0;
	}
	printf OUTPUT_TABLE '{0x%X,0x%X,%d}};', $first, $last, $lineBreakMap{$lb};
	print OUTPUT_TABLE "\nconst size_t LineBreak::count_ = countof(LineBreak::ranges_);\n";
	$input->close;
}


# open output files
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
processCanonicalCombiningClasses();
processScripts();
processBinaryProperties();
processCaseFolding();
processDecompositionMappings();
processEastAsianWidths();
processLineBreaks();
print "done.\n";

close OUTPUT_TABLE;
close BP_TABLE_DEFINITION;
close OUTPUT_IMPL;

__END__