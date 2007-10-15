﻿#!/usr/local/bin/perl

# gen-uax29-test.pl (c) 2007 exeal
#
# This script generates C++ code for tests about concrete break iterators
# implement ascension.unicode.BreakIterator. See ascension/test/break-iterator-test.cpp.
#
# This takes three input files obtained from Unicode.org:
# - GraphemeBreakTest.txt
# - WordBreakTest.txt
# - SentenceBreakTest.txt
# in UNIDATA/auxiliary/.

use strict;
use warnings;
use integer;
use IO::File;

my $out = new IO::File('> break-iterator-test.cpp');
die "can't open output file: break-iterator-test.cpp\n" unless(defined($out));

sub handleFile($$) {
	my ($fileName, $postfix) = @_;
	my $in = new IO::File('< ' . $fileName);
	die("can't open: $fileName\n") if(!defined($in));
	my $lastIndices = '';
	while(<$in>) {
		next unless(m/^÷[^\#]+/);
		$_ = $&;
		my ($i, $charIndex, $text) = (0, 0, '');
		my @indices;
		while(/(÷|×)\s([\dA-F]+)/g) {
			$text .= '\x' . $2;
			push(@indices, $charIndex) if($1 eq '÷');
			++$charIndex;
		}
		push(@indices, $charIndex);
		if(join(', ', @indices) ne $lastIndices) {
			print $out "\t" . 'initializeContainer(indices) = ' . join(', ', @indices) . ";\n";
			$lastIndices = join(', ', @indices);
		}
		print $out "\tcheck$postfix(L\"$text\", indices);\n";
	}
}

die "usage: gen-uax29-test.pl [input-file-directory]\n" if($#ARGV != 0 or $ARGV[0] eq '-h');
my $directory = shift @ARGV;
if($directory ne '') {
	$directory =~ s/\//\\/;
	$directory .= '\\' unless($directory =~ /\\$/);
}

print $out '// automatically generated by `perl gen-uax29-test.pl` at $ ' . scalar(localtime) . " \$\n\n";
print $out <<'HEAD';
// break-iterator-test.cpp
#include <boost/test/unit_test.hpp>
#include <boost/test/included/unit_test.hpp>
#include "../unicode.hpp"

namespace {
	template<class Container>
	class ContainerInitializeContext {
	public:
		explicit ContainerInitializeContext(Container& c) throw() : c_(c) {}
		ContainerInitializeContext& operator,(typename Container::value_type e) {c_.push_back(e); return *this;}
	private:
		Container& c_;
	};
	template<class Container>
	class ContainerInitializer {
	public:
		explicit ContainerInitializer(Container& c) throw() : c_(c) {}
		ContainerInitializeContext<Container> operator=(typename Container::value_type e) {
			c_.clear(); c_.push_back(e); return ContainerInitializeContext<Container>(c_);}
	private:
		Container& c_;
	};
	template<class Container>
	inline ContainerInitializer<Container> initializeContainer(Container& c) {
		return ContainerInitializer<Container>(c);
	}

	template<template<class> class Iterator>
	void check(Iterator<ascension::unicode::StringCharacterIterator>& i, const std::vector<std::size_t>& indices) {
		const ascension::Char* const p = i.base().tell();
		// forward iteration
		for(std::vector<std::size_t>::const_iterator j = indices.begin(); j != indices.end(); ++j)
			BOOST_WARN_EQUAL((i++).base().tell(), p + *j);
		// backward iteration
		BOOST_WARN(!i.base().hasNext());	// BOOST_REQUIRE is preferred
		for(std::vector<std::size_t>::const_reverse_iterator j = indices.rbegin(); j != indices.rend(); ++j)
			BOOST_WARN_EQUAL((i--).base().tell(), p + *j);
		// random check
		BOOST_WARN(!i.base().hasPrevious());	// BOOST_REQUIRE is preferred
		for(std::vector<std::size_t>::const_iterator j = indices.begin(); j != indices.end(); ++j)
			BOOST_WARN(i.isBoundary(ascension::unicode::StringCharacterIterator(p, i.base().getLast(), p + *j)));
	}

	inline void checkGBI(const ascension::String& s, const std::vector<std::size_t>& indices) {
		ascension::unicode::StringCharacterIterator text(s);
		return check<ascension::unicode::GraphemeBreakIterator>(
			ascension::unicode::GraphemeBreakIterator<ascension::unicode::StringCharacterIterator>(text), indices);
	}

	inline void checkWBI(const ascension::String& s, const std::vector<std::size_t>& indices) {
		ascension::unicode::StringCharacterIterator text(s);
		return check<ascension::unicode::WordBreakIterator>(
			ascension::unicode::WordBreakIterator<ascension::unicode::StringCharacterIterator>(
				text, ascension::unicode::AbstractWordBreakIterator::BOUNDARY_OF_SEGMENT, ascension::unicode::IdentifierSyntax()), indices);
	}

	inline void checkSBI(const ascension::String& s, const std::vector<std::size_t>& indices) {
	}
}
HEAD

print $out "\nvoid testGraphemeBreakIterator() {\n\tstd::vector<std::size_t> indices;\n";
handleFile($directory . 'GraphemeBreakTest.txt', 'GBI');
print $out "}\n\nvoid testWordBreakIterator() {\n\tstd::vector<std::size_t> indices;\n";
handleFile($directory . 'WordBreakTest.txt', 'WBI');
print $out "}\n\nvoid testSentenceBreakIterator() {\n\tstd::vector<std::size_t> indices;\n";
handleFile($directory . 'SentenceBreakTest.txt', 'SBI');
print $out "}\n";

print $out <<'FOOTER';

boost::unit_test::test_suite* init_unit_test_suite(int, char*[]) {
	boost::unit_test::test_suite* test = BOOST_TEST_SUITE("Break iterator test");
	test->add(BOOST_TEST_CASE(&testGraphemeBreakIterator));
	test->add(BOOST_TEST_CASE(&testWordBreakIterator));
	test->add(BOOST_TEST_CASE(&testSentenceBreakIterator));
	return test;
}
FOOTER
