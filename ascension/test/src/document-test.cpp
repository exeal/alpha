#define BOOST_TEST_MODULE document_test
#include <boost/test/included/unit_test.hpp>

#include <ascension/kernel/document.hpp>
#include <boost/range/irange.hpp>
#include "from-latin1.hpp"

namespace k = ascension::kernel;

namespace {
	ascension::String contents(const k::Document& d) {
		std::basic_ostringstream<ascension::Char> s;
		k::writeDocumentToStream(s, d, d.region());
		return s.str();
	}
}

BOOST_AUTO_TEST_SUITE(construction)
	BOOST_AUTO_TEST_CASE(default_construction_test) {
		const k::Document d;

		// attributes
		BOOST_TEST((d.input().lock().get() == nullptr));
		BOOST_TEST(!d.isModified());
		BOOST_TEST(!d.isReadOnly());
		BOOST_TEST((d.session() == nullptr));

		// contents
		BOOST_TEST(boost::equal(d.accessibleRegion(), k::Region::zero()));
		BOOST_TEST(d.length() == 0u);
		BOOST_TEST(d.lineContent(0u).revisionNumber() == 0u);
#if 1
		BOOST_TEST(d.lineContent(0u).text().empty());
#else
		BOOST_TEST(d.lineContent(0u).text() == ascension::String());	// C2338 at VS14
#endif
		BOOST_TEST(d.lineLength(0u) == 0u);
		BOOST_TEST(d.lineOffset(0u) == 0u);
#if 1
		BOOST_TEST(d.lineString(0u).empty());
#else
		BOOST_TEST(d.lineString(0u) == ascension::String());	// C2338 at VS14
#endif
		BOOST_TEST(d.numberOfLines() == 1u);
		BOOST_TEST(boost::equal(d.region(), k::Region::zero()));
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
		k::Document d;

		auto e(k::insert(d, k::Position::zero(), fromLatin1("first")));
		BOOST_TEST(e == k::Position(0u, 5u));
		BOOST_TEST(d.accessibleRegion().equal(k::Region::makeSingleLine(0u, boost::irange(0u, 5u))));
		BOOST_TEST(d.length() == 5u);
		BOOST_CHECK_EQUAL(d.lineContent(0u).text(), fromLatin1("first"));
		BOOST_TEST(d.lineLength(0u) == 5u);
		BOOST_TEST(d.lineOffset(0u) == 0u);
		BOOST_TEST(d.lineString(0u) == fromLatin1("first"));
		BOOST_TEST(d.numberOfLines() == 1u);
		BOOST_TEST(d.region().equal(k::Region::makeSingleLine(0u, boost::irange(0u, 5u))));
		BOOST_TEST(d.revisionNumber() == 1u);
		BOOST_TEST(d.numberOfUndoableChanges() == 1u);
		BOOST_TEST(d.numberOfRedoableChanges() == 0u);

		e = k::insert(d, e, fromLatin1(" line"));
		BOOST_TEST(e == k::Position(0u, 10u));
		BOOST_TEST(d.accessibleRegion().equal(k::Region::makeSingleLine(0u, boost::irange(0u, 10u))));
		BOOST_TEST(d.length() == 10u);
		BOOST_TEST(d.lineString(0u) == fromLatin1("first line"));
		BOOST_TEST(d.numberOfLines() == 1u);
		BOOST_TEST(d.region().equal(k::Region::makeSingleLine(0u, boost::irange(0u, 10u))));
		BOOST_TEST(d.revisionNumber() == 2u);
		BOOST_TEST(d.numberOfUndoableChanges() == 1u);
		BOOST_TEST(d.numberOfRedoableChanges() == 0u);

		e = k::insert(d, k::Position::zero(), fromLatin1("This is "));
		BOOST_TEST(e == k::Position(0u, 8u));
		BOOST_TEST(d.length() == 18u);
		BOOST_TEST(d.lineString(0u) == fromLatin1("This is first line"));
		BOOST_TEST(d.numberOfLines() == 1u);
		BOOST_TEST(d.region().equal(k::Region::makeSingleLine(0u, boost::irange(0u, 18u))));
		BOOST_TEST(d.numberOfUndoableChanges() == 2u);

		k::insert(d, e, fromLatin1("the "));
		BOOST_TEST(d.length() == 22u);
		BOOST_TEST(d.lineString(0u) == fromLatin1("This is the first line"));
		BOOST_TEST(d.numberOfLines() == 1u);
		BOOST_TEST(d.region().equal(k::Region::makeSingleLine(0u, boost::irange(0u, 22u))));

		e = k::insert(d, k::Position(0, 18u), fromLatin1("line.\nHere is the second "));
		BOOST_TEST(e == k::Position(1u, 19u));
		BOOST_TEST(d.length() == 24u + 23u);
		BOOST_REQUIRE(d.numberOfLines() == 2u);
		BOOST_TEST(d.lineString(0u) == fromLatin1("This is the first line."));
		BOOST_TEST(d.lineString(1u) == fromLatin1("Here is the second line"));
		BOOST_TEST(*boost::const_begin(d.region()) == k::Position::zero());
		BOOST_TEST(*boost::const_end(d.region()) == k::Position(1u, 23u));

		e = k::insert(d, k::Position(1u, 23u), fromLatin1("\r\n"));
		BOOST_TEST(e == k::Position::bol(2u));
		BOOST_REQUIRE(d.numberOfLines() == 3u);
		BOOST_TEST(d.lineLength(2u) == 0u);

		k::Document d2;
		k::insert(d2, k::Position::zero(), fromLatin1("aaaaa\nbbbbb"));
		BOOST_REQUIRE(d2.length() == 6u + 5u);
		BOOST_REQUIRE(d2.numberOfLines() == 2u);
		BOOST_TEST(d2.lineString(0u) == fromLatin1("aaaaa"));
		BOOST_TEST(d2.lineString(1u) == fromLatin1("bbbbb"));
		k::insert(d2, k::Position(0u, 2u), fromLatin1("XXX\nYYY\nZZZ"));
		BOOST_REQUIRE(d2.length() == 6u + 4u + 7u + 5u);
		BOOST_REQUIRE(d2.numberOfLines() == 4u);
		BOOST_TEST(d2.lineString(0u) == fromLatin1("aaXXX"));
		BOOST_TEST(d2.lineString(1u) == fromLatin1("YYY"));
		BOOST_TEST(d2.lineString(2u) == fromLatin1("ZZZaaa"));
		BOOST_TEST(d2.lineString(3u) == fromLatin1("bbbbb"));
	}

	BOOST_AUTO_TEST_CASE(single_line_removal_test) {
		k::Document d;
		k::insert(d, k::Position::zero(), fromLatin1("abcde"));
		BOOST_REQUIRE(contents(d) == fromLatin1("abcde"));

		k::erase(d, k::Region::makeSingleLine(0u, boost::irange(4u, 5u)));
		BOOST_TEST(contents(d) == fromLatin1("abcd"));

		k::erase(d, k::Region::makeSingleLine(0u, boost::irange(0u, 1u)));
		BOOST_TEST(contents(d) == fromLatin1("bcd"));

		k::erase(d, d.region());
		BOOST_TEST(d.length() == 0u);
		BOOST_TEST(d.isModified());
	}

	BOOST_AUTO_TEST_CASE(newline_removal_test) {
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

	BOOST_AUTO_TEST_CASE(multi_line_removal_test) {
		k::Document d;
		k::insert(d, k::Position::zero(), fromLatin1("abcde\nfghij\r\nklmno\rpqrst"));
		BOOST_REQUIRE(contents(d) == fromLatin1("abcde\nfghij\r\nklmno\rpqrst"));

		k::erase(d, k::Region(k::Position(0u, 1u), k::Position(1u, 4u)));
		BOOST_TEST(contents(d) == fromLatin1("aj\r\nklmno\rpqrst"));
		BOOST_TEST(d.numberOfLines() == 3u);

		k::erase(d, k::Region(k::Position(0u, 1u), k::Position(2u, 2u)));
		BOOST_TEST(contents(d) == fromLatin1("arst"));
		BOOST_TEST(d.numberOfLines() == 1u);
	}

	BOOST_AUTO_TEST_CASE(modified_mark_test) {
		k::Document d;
		BOOST_TEST(!d.isModified());

		// only the following here...
		d.setModified();
		BOOST_TEST(d.isModified());
		d.markUnmodified();
		BOOST_TEST(!d.isModified());
	}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(undo_redo)
	BOOST_AUTO_TEST_CASE(simple_test) {
		k::Document d;
		BOOST_REQUIRE(d.isRecordingChanges());

		// empty operations
		k::insert(d, k::Position::zero(), ascension::StringPiece());
		BOOST_TEST(d.numberOfUndoableChanges() == 0u);
		k::erase(d, k::Region::zero());
		BOOST_TEST(d.numberOfUndoableChanges() == 0u);

		// simple undo
		k::insert(d, k::Position::zero(), fromLatin1("abcde"));
		BOOST_REQUIRE(d.numberOfUndoableChanges() == 1u);
		BOOST_REQUIRE(d.isModified());
		BOOST_CHECK_THROW(d.undo(2), std::invalid_argument);
		d.undo(1);
		BOOST_TEST(d.length() == 0u);
		BOOST_TEST(d.numberOfUndoableChanges() == 0u);
		BOOST_TEST(d.revisionNumber() == 0u);
		BOOST_TEST(!d.isModified());

		// simple redo
		BOOST_REQUIRE(d.numberOfRedoableChanges() == 1u);
		BOOST_CHECK_THROW(d.redo(2), std::invalid_argument);
		d.redo(1);
		BOOST_TEST(contents(d) == fromLatin1("abcde"));
		BOOST_TEST(d.numberOfUndoableChanges() == 1u);
		BOOST_TEST(d.numberOfRedoableChanges() == 0u);
		BOOST_TEST(d.revisionNumber() == 1u);
		BOOST_TEST(d.isModified());
	}

	BOOST_AUTO_TEST_CASE(insert_insert_auto_merge_test) {
		k::Document d;
		BOOST_REQUIRE(d.isRecordingChanges());

		auto e(k::insert(d, k::Position::zero(), fromLatin1("abc")));
		BOOST_REQUIRE(d.numberOfUndoableChanges() == 1u);
		BOOST_REQUIRE(d.revisionNumber() == 1u);
		k::insert(d, e, fromLatin1("def"));
		BOOST_TEST(d.numberOfUndoableChanges() == 1u);
		BOOST_TEST(d.revisionNumber() == 2u);
		k::insert(d, k::Position::zero(), fromLatin1("012"));
		BOOST_TEST(d.numberOfUndoableChanges() == 2u);
		BOOST_REQUIRE(contents(d) == fromLatin1("012abcdef"));
		BOOST_REQUIRE(d.revisionNumber() == 3u);
		d.undo(1);
		BOOST_TEST(contents(d) == fromLatin1("abcdef"));
		BOOST_TEST(d.revisionNumber() == 2u);
		d.undo(1);
		BOOST_TEST(contents(d) == fromLatin1(""));
		BOOST_TEST(d.revisionNumber() == 0u);

		e = k::insert(d, k::Position::zero(), fromLatin1("abc"));
		BOOST_REQUIRE(d.numberOfUndoableChanges() == 1u);
		k::insert(d, e, fromLatin1("def\nghi"));
		BOOST_TEST(d.numberOfUndoableChanges() == 2u);
	}

	BOOST_AUTO_TEST_CASE(erase_erase_auto_merge_test) {
		k::Document d;

		k::insert(d, k::Position::zero(), fromLatin1("abcde"));
		k::erase(d, k::Region::makeSingleLine(0u, boost::irange(4u, 5u)));
		BOOST_REQUIRE(d.numberOfUndoableChanges() == 2u);
		k::erase(d, k::Region::makeSingleLine(0u, boost::irange(3u, 4u)));
		BOOST_TEST(d.numberOfUndoableChanges() == 2u);
		k::erase(d, k::Region::makeSingleLine(0u, boost::irange(0u, 1u)));
		BOOST_TEST(d.numberOfUndoableChanges() == 3u);
		k::erase(d, k::Region::makeSingleLine(0u, boost::irange(0u, 1u)));
		BOOST_TEST(d.numberOfUndoableChanges() == 3u);
		BOOST_REQUIRE(contents(d) == fromLatin1("c"));
		d.undo(1);
		BOOST_TEST(contents(d) == fromLatin1("abc"));
		d.undo(1);
		BOOST_TEST(contents(d) == fromLatin1("abcde"));

		k::insert(d, *boost::const_end(d.region()), fromLatin1("\nfgh"));
		k::erase(d, k::Region::makeSingleLine(1u, boost::irange(1u, 3u)));
		BOOST_REQUIRE(d.numberOfUndoableChanges() == 3u);
		k::erase(d, k::Region(k::Position::zero(), k::Position(1u, 1u)));
		BOOST_TEST(d.numberOfUndoableChanges() == 4u);
	}

	BOOST_AUTO_TEST_CASE(replace_insert_auto_compound_test) {
		k::Document d;

		k::insert(d, k::Position::zero(), fromLatin1("abcdef"));
		const auto e(d.replace(k::Region::makeSingleLine(0u, boost::irange(0u, 3u)), fromLatin1("A")));
		BOOST_REQUIRE(d.numberOfUndoableChanges() == 2u);
		k::insert(d, e, fromLatin1("BC"));
		BOOST_TEST(d.numberOfUndoableChanges() == 2u);
		BOOST_REQUIRE(contents(d) == fromLatin1("ABCdef"));
		d.undo(1);
		BOOST_TEST(contents(d) == fromLatin1("abcdef"));

		d.replace(k::Region::makeSingleLine(0u, boost::irange(0u, 3u)), fromLatin1("XYZ"));
		BOOST_REQUIRE(d.numberOfUndoableChanges() == 2u);
		k::insert(d, k::Position::zero(), fromLatin1("UVW"));
		BOOST_TEST(d.numberOfUndoableChanges() == 3u);
		BOOST_REQUIRE(contents(d) == fromLatin1("UVWXYZdef"));
		d.undo(1);
		BOOST_TEST(contents(d) == fromLatin1("XYZdef"));
		d.undo(1);
		BOOST_TEST(contents(d) == fromLatin1("abcdef"));

		k::insert(d, *boost::const_end(d.region()), fromLatin1("\nghi"));
		k::erase(d, k::Region::makeSingleLine(1u, boost::irange(1u, 3u)));
		BOOST_REQUIRE(d.numberOfUndoableChanges() == 3u);
		k::erase(d, k::Region(k::Position::zero(), k::Position(1u, 1u)));
		BOOST_TEST(d.numberOfUndoableChanges() == 4u);
	}

	BOOST_AUTO_TEST_CASE(undo_boundary_test) {
		// insert => boundary => insert
		k::Document d;
		auto e(k::insert(d, k::Position::zero(), fromLatin1("abc")));
		d.insertUndoBoundary();
		k::insert(d, e, fromLatin1("def"));
		BOOST_TEST(d.numberOfUndoableChanges() == 2u);
		d.undo(1);
		BOOST_TEST(contents(d) == fromLatin1("abc"));

		// erase => boundary => erase
		BOOST_REQUIRE(d.numberOfUndoableChanges() == 1u);
		k::erase(d, k::Region::makeSingleLine(0u, boost::irange(0u, 1u)));
		d.insertUndoBoundary();
		k::erase(d, k::Region::makeSingleLine(0u, boost::irange(0u, 1u)));
		BOOST_TEST(d.numberOfUndoableChanges() == 3u);

		// replace => boundary => insert
		k::Document d2;
		k::insert(d2, k::Position::zero(), fromLatin1("abcdef"));
		e = d2.replace(k::Region::makeSingleLine(0u, boost::irange(0u, 3u)), fromLatin1("A"));
		BOOST_REQUIRE(d2.numberOfUndoableChanges() == 2u);
		d2.insertUndoBoundary();
		k::insert(d2, e, fromLatin1("BC"));
		BOOST_TEST(d2.numberOfUndoableChanges() == 3u);
		d2.undo(1);
		BOOST_TEST(contents(d2) == fromLatin1("Adef"));
	}

	BOOST_AUTO_TEST_CASE(explicit_compound_test) {
		k::Document d;
		BOOST_REQUIRE(d.isRecordingChanges());

		d.beginCompoundChange();
		k::insert(d, k::Position::zero(), fromLatin1("abc"));
		auto e(k::insert(d, k::Position::zero(), fromLatin1("def")));
		BOOST_TEST(d.numberOfUndoableChanges() == 1u);
		d.endCompoundChange();
		BOOST_TEST(d.numberOfUndoableChanges() == 1u);
		BOOST_TEST(d.revisionNumber() == 2u);

		e = k::insert(d, e, fromLatin1("ghi"));
		BOOST_TEST(d.numberOfUndoableChanges() == 2u);

		// Document.insertUndoBoundary can't break the compound change
		d.beginCompoundChange();
		e = k::insert(d, e, fromLatin1("jkl"));
		d.insertUndoBoundary();
		e = k::insert(d, e, fromLatin1("mno"));
		d.endCompoundChange();
		BOOST_TEST(d.numberOfUndoableChanges() == 3u);

		// empty compound change breaks automatic composition
		d.beginCompoundChange();
		d.endCompoundChange();
		BOOST_TEST(d.numberOfUndoableChanges() == 3u);
		k::insert(d, e, fromLatin1("pqr"));
		BOOST_TEST(d.numberOfUndoableChanges() == 4u);

		BOOST_REQUIRE(d.revisionNumber() == 6u);
		d.undo(1);
		BOOST_TEST(d.revisionNumber() == 5u);
		d.undo(1);
		BOOST_TEST(d.revisionNumber() == 3u);
		d.undo(1);
		BOOST_TEST(d.revisionNumber() == 2u);
		d.undo(1);
		BOOST_TEST(d.revisionNumber() == 0u);
	}

	BOOST_AUTO_TEST_CASE(composition_interruption_test) {
		k::Document d;
		BOOST_REQUIRE(d.isRecordingChanges());

		d.beginCompoundChange();
		k::insert(d, k::Position::zero(), fromLatin1("abc"));
		k::insert(d, k::Position::zero(), fromLatin1("def"));
		BOOST_REQUIRE(d.numberOfUndoableChanges() == 1u);
		d.undo(1);
		BOOST_TEST(d.numberOfUndoableChanges() == 0u);
		BOOST_TEST(!d.isCompoundChanging());

		BOOST_TEST(d.numberOfRedoableChanges() == 1u);
		d.redo(1);
		BOOST_TEST(contents(d) == fromLatin1("defabc"));
	}

	BOOST_AUTO_TEST_CASE(recursive_composition_test) {
		k::Document d;
		BOOST_REQUIRE(d.isRecordingChanges());

		d.beginCompoundChange();
		k::insert(d, k::Position::zero(), fromLatin1("abc"));
		d.beginCompoundChange();
		k::insert(d, k::Position::zero(), fromLatin1("def"));
		k::insert(d, k::Position::zero(), fromLatin1("ghi"));
		d.endCompoundChange();
		k::insert(d, k::Position::zero(), fromLatin1("jkl"));
		d.endCompoundChange();
		BOOST_TEST(d.numberOfUndoableChanges() == 1u);
		BOOST_TEST(!d.isCompoundChanging());

		d.beginCompoundChange();
		d.beginCompoundChange();
		d.beginCompoundChange();
		k::insert(d, k::Position::zero(), fromLatin1("mno"));
		d.undo(1);
		BOOST_TEST(!d.isCompoundChanging());
	}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(narrowing)
	BOOST_AUTO_TEST_CASE(basic_test) {
		k::Document d;
		k::insert(d, k::Position::zero(), fromLatin1("abcde\nfghij"));
		BOOST_REQUIRE(!d.isNarrowed());
		// a b[c d e
		// f g h]i j

		// attribute checks
		k::Region barrier(k::Position(0u, 2u), k::Position(1u, 3u));
		d.narrowToRegion(barrier);
		BOOST_TEST(d.isNarrowed());
		BOOST_CHECK_EQUAL(d.accessibleRegion(), barrier);
		BOOST_TEST(*boost::const_begin(d.region()) == k::Position::zero());
		BOOST_TEST(*boost::const_end(d.region()) == k::Position(1u, 5u));

		// accessibility checks
		BOOST_CHECK_THROW(k::erase(d, k::Region::makeSingleLine(0u, boost::irange(1, 3))), k::DocumentAccessViolationException);
		BOOST_CHECK_THROW(k::erase(d, k::Region::makeSingleLine(1u, boost::irange(2, 4))), k::DocumentAccessViolationException);
		BOOST_CHECK_THROW(k::insert(d, k::Position(0u, 1u), fromLatin1("xyzzy")), k::DocumentAccessViolationException);
		BOOST_CHECK_THROW(k::insert(d, k::Position(1u, 4u), fromLatin1("xyzzy")), k::DocumentAccessViolationException);
		BOOST_CHECK_NO_THROW(k::insert(d, k::Position(0u, 2u), fromLatin1("[")));
		BOOST_CHECK_NO_THROW(k::insert(d, k::Position(1u, 3u), fromLatin1("]")));

		// extension
		BOOST_TEST(*boost::const_end(d.accessibleRegion()) == k::Position(1u, 4u));

		// widen
		d.widen();
		BOOST_TEST(!d.isNarrowed());
		BOOST_CHECK_EQUAL(d.accessibleRegion(), d.region());

		// renarrow
		d.narrowToRegion(barrier);
		BOOST_REQUIRE_EQUAL(d.accessibleRegion(), barrier);
	}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_CASE(reset_test) {
	k::Document d;
	k::insert(d, k::Position::zero(), fromLatin1("abcde\nfghij"));
	d.narrowToRegion(k::Region::makeSingleLine(0u, boost::irange(1u, 2u)));
	d.setReadOnly(true);
	BOOST_REQUIRE(d.isModified());
	BOOST_REQUIRE(d.isNarrowed());
	BOOST_REQUIRE(d.isReadOnly());
	BOOST_REQUIRE(d.revisionNumber() > 0u);
}
