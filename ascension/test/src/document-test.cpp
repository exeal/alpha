#define BOOST_TEST_MODULE document_test
#include <boost/test/included/unit_test.hpp>

#include <ascension/kernel/document.hpp>
#include "from-latin1.hpp"

namespace {
	ascension::String contents(const ascension::kernel::Document& d) {
		std::basic_ostringstream<ascension::Char> s;
		ascension::kernel::writeDocumentToStream(s, d, d.region());
		return s.str();
	}
}

BOOST_AUTO_TEST_SUITE(construction)
	BOOST_AUTO_TEST_CASE(default_construction_test) {
		const ascension::kernel::Document d;

		// attributes
		BOOST_TEST((d.input().lock().get() == nullptr));
		BOOST_TEST(!d.isModified());
		BOOST_TEST(!d.isReadOnly());
		BOOST_TEST((d.session() == nullptr));

		// contents
		BOOST_TEST(boost::equal(d.accessibleRegion(), ascension::kernel::Region::zero()));
		BOOST_TEST(d.length() == 0u);
		BOOST_TEST(d.lineContent(0u).revisionNumber() == 0u);
		BOOST_TEST(d.lineContent(0u).text() == ascension::String());
		BOOST_TEST(d.lineLength(0u) == 0u);
		BOOST_TEST(d.lineOffset(0u) == 0u);
		BOOST_TEST(d.lineString(0u) == ascension::String());
		BOOST_TEST(d.numberOfLines() == 1u);
		BOOST_TEST(boost::equal(d.region(), ascension::kernel::Region::zero()));
		BOOST_TEST(d.revisionNumber() == 0u);

		// manipulation
		BOOST_TEST(!d.isChanging());

		// undo/redo and compound changes
		BOOST_TEST(!d.isCompoundChanging());
		BOOST_TEST(d.isRecordingChanges());
		BOOST_TEST(d.numberOfUndoableChanges() == 0u);
		BOOST_TEST(d.numberOfRedoableChanges() == 0u);

		// narrowing
		BOOST_TEST(!d.isNarrowed());
	}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(modifications)
	BOOST_AUTO_TEST_CASE(insertion_test) {
		ascension::kernel::Document d;

		auto e(ascension::kernel::insert(d, ascension::kernel::Position::zero(), fromLatin1("first")));
		BOOST_TEST(e == ascension::kernel::Position(0u, 5u));
		BOOST_TEST(d.isModified());
		BOOST_TEST(d.accessibleRegion().equal(ascension::kernel::Region::makeSingleLine(0u, boost::irange(0u, 5u))));
		BOOST_TEST(d.length() == 5u);
		BOOST_TEST(d.lineContent(0u).text() == fromLatin1("first"));
		BOOST_TEST(d.lineLength(0u) == 5u);
		BOOST_TEST(d.lineOffset(0u) == 0u);
		BOOST_TEST(d.lineString(0u) == fromLatin1("first"));
		BOOST_TEST(d.numberOfLines() == 1u);
		BOOST_TEST(d.region().equal(ascension::kernel::Region::makeSingleLine(0u, boost::irange(0u, 5u))));
		BOOST_TEST(d.revisionNumber() == 1u);
		BOOST_TEST(d.numberOfUndoableChanges() == 1u);
		BOOST_TEST(d.numberOfRedoableChanges() == 0u);

		e = ascension::kernel::insert(d, e, fromLatin1(" line"));
		BOOST_TEST(e == ascension::kernel::Position(0u, 10u));
		BOOST_TEST(d.accessibleRegion().equal(ascension::kernel::Region::makeSingleLine(0u, boost::irange(0u, 10u))));
		BOOST_TEST(d.length() == 10u);
		BOOST_TEST(d.lineString(0u) == fromLatin1("first line"));
		BOOST_TEST(d.numberOfLines() == 1u);
		BOOST_TEST(d.region().equal(ascension::kernel::Region::makeSingleLine(0u, boost::irange(0u, 10u))));
		BOOST_TEST(d.revisionNumber() == 2u);
		BOOST_TEST(d.numberOfUndoableChanges() == 1u);
		BOOST_TEST(d.numberOfRedoableChanges() == 0u);

		e = ascension::kernel::insert(d, ascension::kernel::Position::zero(), fromLatin1("This is "));
		BOOST_TEST(e == ascension::kernel::Position(0u, 8u));
		BOOST_TEST(d.length() == 18u);
		BOOST_TEST(d.lineString(0u) == fromLatin1("This is first line"));
		BOOST_TEST(d.numberOfLines() == 1u);
		BOOST_TEST(d.region().equal(ascension::kernel::Region::makeSingleLine(0u, boost::irange(0u, 18u))));
		BOOST_TEST(d.numberOfUndoableChanges() == 2u);

		ascension::kernel::insert(d, e, fromLatin1("the "));
		BOOST_TEST(d.length() == 22u);
		BOOST_TEST(d.lineString(0u) == fromLatin1("This is the first line"));
		BOOST_TEST(d.numberOfLines() == 1u);
		BOOST_TEST(d.region().equal(ascension::kernel::Region::makeSingleLine(0u, boost::irange(0u, 22u))));

		e = ascension::kernel::insert(d, ascension::kernel::Position(0, 18u), fromLatin1("line.\nHere is the second "));
		BOOST_TEST(e == ascension::kernel::Position(1u, 19u));
		BOOST_TEST(d.length() == 24u + 23u);
		BOOST_REQUIRE(d.numberOfLines() == 2u);
		BOOST_TEST(d.lineString(0u) == fromLatin1("This is the first line."));
		BOOST_TEST(d.lineString(1u) == fromLatin1("Here is the second line"));
		BOOST_TEST(*boost::const_begin(d.region()) == ascension::kernel::Position::zero());
		BOOST_TEST(*boost::const_end(d.region()) == ascension::kernel::Position(1u, 23u));

		e = ascension::kernel::insert(d, ascension::kernel::Position(1u, 23u), fromLatin1("\r\n"));
		BOOST_TEST(e == ascension::kernel::Position::bol(2u));
		BOOST_REQUIRE(d.numberOfLines() == 3u);
		BOOST_TEST(d.lineLength(2u) == 0u);

		ascension::kernel::Document d2;
		ascension::kernel::insert(d2, ascension::kernel::Position::zero(), fromLatin1("aaaaa\nbbbbb"));
		BOOST_REQUIRE(d2.length() == 6u + 5u);
		BOOST_REQUIRE(d2.numberOfLines() == 2u);
		BOOST_TEST(d2.lineString(0u) == fromLatin1("aaaaa"));
		BOOST_TEST(d2.lineString(1u) == fromLatin1("bbbbb"));
		ascension::kernel::insert(d2, ascension::kernel::Position(0u, 2u), fromLatin1("XXX\nYYY\nZZZ"));
		BOOST_REQUIRE(d2.length() == 6u + 4u + 7u + 5u);
		BOOST_REQUIRE(d2.numberOfLines() == 4u);
		BOOST_TEST(d2.lineString(0u) == fromLatin1("aaXXX"));
		BOOST_TEST(d2.lineString(1u) == fromLatin1("YYY"));
		BOOST_TEST(d2.lineString(2u) == fromLatin1("ZZZaaa"));
		BOOST_TEST(d2.lineString(3u) == fromLatin1("bbbbb"));
	}

	BOOST_AUTO_TEST_CASE(single_line_removal_test) {
		ascension::kernel::Document d;
		ascension::kernel::insert(d, ascension::kernel::Position::zero(), fromLatin1("abcde"));
		BOOST_REQUIRE(contents(d) == fromLatin1("abcde"));

		ascension::kernel::erase(d, ascension::kernel::Region::makeSingleLine(0u, boost::irange(4u, 5u)));
		BOOST_TEST(contents(d) == fromLatin1("abcd"));

		ascension::kernel::erase(d, ascension::kernel::Region::makeSingleLine(0u, boost::irange(0u, 1u)));
		BOOST_TEST(contents(d) == fromLatin1("bcd"));

		ascension::kernel::erase(d, d.region());
		BOOST_TEST(d.length() == 0u);
		BOOST_TEST(d.isModified());
	}

	BOOST_AUTO_TEST_CASE(newline_removal_test) {
		namespace k = ascension::kernel;
		k::Document d;
		k::insert(d, k::Position::zero(), fromLatin1("abcde\nfghij\r\nklmno"));
		BOOST_REQUIRE(contents(d) == fromLatin1("abcde\nfghij\r\nklmno"));

		// removing a newline is joining the surrounding lines
		k::erase(d, k::Region(k::Position(0u, 5u), k::Position(1u, 0u)));
		BOOST_TEST(contents(d) == fromLatin1("abcdefghij\r\nklmno"));
		BOOST_TEST(d.numberOfLines() == 2u);

		// a CR+LF is a single newline
		k::erase(d, k::Region(k::Position(0u, 10u), k::Position(1u, 0u)));
		BOOST_TEST(contents(d) == fromLatin1("abcdefghijklmno"));
		BOOST_TEST(d.numberOfLines() == 1u);

		// a eos is not a eol
		BOOST_CHECK_THROW(
			k::erase(d, k::Region(k::Position(0u, 15u), k::Position(1u, 0u))),
			k::BadRegionException);
	}
BOOST_AUTO_TEST_SUITE_END()
