// document-test.cpp

#include <ascension/kernel/document.hpp>
#include <ascension/kernel/document-character-iterator.hpp>
#include <ascension/kernel/document-stream.hpp>
#include <ascension/corelib/text/utf-iterator.hpp>	// text.utf.decode
#include <boost/test/included/test_exec_monitor.hpp>
namespace a = ascension;
namespace k = ascension::kernel;
namespace x = ascension::text;

void testMiscellaneousFunctions() {
	// U+2028 is <E2 80 A8> in UTF-8
	// U+2029 is <E2 80 A9> in UTF-8
	const a::String s(x::utf::decode("abc\ndef\r\n\rghi\xe2\x80\xa8\xe2\x80\xa9"));
	// newlines:                         ^    ^   ^    ^           ^

	BOOST_CHECK_EQUAL(x::calculateNumberOfLines(s), 6);
	BOOST_CHECK_EQUAL(x::eatNewline(s.begin(), s.end()), x::NLF_RAW_VALUE);
	BOOST_CHECK_EQUAL(x::eatNewline(s.begin() + 3, s.end()), x::NLF_LINE_FEED);
	BOOST_CHECK_EQUAL(x::eatNewline(s.begin() + 7, s.end()), x::NLF_CR_LF);
	BOOST_CHECK_EQUAL(x::eatNewline(s.begin() + 8, s.end()), x::NLF_LINE_FEED);
	BOOST_CHECK_EQUAL(x::eatNewline(s.begin() + 9, s.end()), x::NLF_CARRIAGE_RETURN);
	BOOST_CHECK_EQUAL(x::eatNewline(s.begin() + 13, s.end()), x::NLF_LINE_SEPARATOR);
	BOOST_CHECK_EQUAL(x::eatNewline(s.begin() + 14, s.end()), x::NLF_PARAGRAPH_SEPARATOR);
//	BOOST_CHECK_EQUAL(x::eatNewline(s.begin() + 15, s.end()), x::NLF_RAW_VALUE);
}

void testSimpleChange() {
	k::Document d;

	// initial state
	BOOST_CHECK_EQUAL(d.length(), 0);
	BOOST_CHECK_EQUAL(d.accessibleRegion(), k::Region(k::Position(0, 0)));
	BOOST_CHECK_EQUAL(d.region(), k::Region(k::Position(0, 0)));
	BOOST_CHECK(d.region().isEmpty());
	BOOST_CHECK_EQUAL(d.input(), static_cast<k::DocumentInput*>(0));
	BOOST_CHECK(!d.isChanging());
	BOOST_CHECK(!d.isModified());
	BOOST_CHECK(!d.isNarrowed());
	BOOST_CHECK(!d.isReadOnly());
	BOOST_CHECK(!d.isCompoundChanging());
	BOOST_CHECK(d.line(0).empty());
	BOOST_CHECK_EQUAL(d.lineLength(0), 0);
	BOOST_CHECK_EQUAL(d.lineOffset(0), 0);
	BOOST_CHECK_EQUAL(d.numberOfLines(), 1);
	BOOST_CHECK_EQUAL(d.session(), static_cast<a::texteditor::Session*>(0));

	// simple change
	k::insert(d, k::Position(0, 0), x::utf::decode("abcde"));
	BOOST_CHECK(d.isModified());
	BOOST_CHECK_EQUAL(d.line(0), x::utf::decode("abcde"));
	BOOST_CHECK_EQUAL(d.length(), 5);
	BOOST_CHECK_EQUAL(d.region(), k::Region(k::Position(0, 0), k::Position(0, 5)));
	BOOST_CHECK_EQUAL(d.revisionNumber(), 1);
	BOOST_CHECK_EQUAL(d.numberOfUndoableChanges(), 1);
	k::erase(d, k::Position(0, 0), k::Position(0, 3));
	BOOST_CHECK_EQUAL(d.line(0), x::utf::decode("de"));
	BOOST_CHECK_EQUAL(d.revisionNumber(), 2);
	BOOST_CHECK_EQUAL(d.numberOfUndoableChanges(), 1);
	d.undo();
	BOOST_CHECK_EQUAL(d.line(0), a::String());
	BOOST_CHECK_EQUAL(d.revisionNumber(), 0);
	BOOST_CHECK(!d.isModified());
	BOOST_CHECK_EQUAL(d.numberOfUndoableChanges(), 0);
	BOOST_CHECK_EQUAL(d.numberOfRedoableChanges(), 1);
	d.redo();
	BOOST_CHECK_EQUAL(d.line(0), x::utf::decode("de"));
	BOOST_CHECK_EQUAL(d.numberOfUndoableChanges(), 1);
	BOOST_CHECK_EQUAL(d.numberOfRedoableChanges(), 0);
	d.undo();
	BOOST_CHECK_EQUAL(d.revisionNumber(), 0);
}

void testUndoBoundary() {
	k::Document d;

	k::insert(d, k::Position(0, 0), x::utf::decode("a"));
	k::insert(d, k::Position(0, 1), x::utf::decode("b"));
	BOOST_CHECK_EQUAL(d.numberOfUndoableChanges(), 1);
	BOOST_CHECK_EQUAL(d.revisionNumber(), 2);
	d.undo();
	BOOST_CHECK_EQUAL(d.numberOfUndoableChanges(), 0);
	BOOST_CHECK_EQUAL(d.revisionNumber(), 0);
	k::insert(d, k::Position(0, 0), x::utf::decode("a"));
	d.insertUndoBoundary();
	k::insert(d, k::Position(0, 1), x::utf::decode("b"));
	BOOST_CHECK_EQUAL(d.numberOfUndoableChanges(), 2);
	d.undo();
	BOOST_CHECK_EQUAL(d.numberOfUndoableChanges(), 1);
	d.undo();
	BOOST_CHECK_EQUAL(d.numberOfUndoableChanges(), 0);
}

void testCompoundChange() {
	k::Document d;

	d.beginCompoundChange();
	k::insert(d, d.region().end(), x::utf::decode("This "));
	k::insert(d, d.region().end(), x::utf::decode("is a "));
	k::insert(d, d.region().end(), x::utf::decode("compound."));
	d.endCompoundChange();
	BOOST_CHECK_EQUAL(d.line(0), x::utf::decode("This is a compound."));
	BOOST_CHECK_EQUAL(d.revisionNumber(), 3);
	BOOST_CHECK_EQUAL(d.numberOfUndoableChanges(), 1);
	d.undo();
	BOOST_CHECK_EQUAL(d.length(), 0);
	BOOST_CHECK_EQUAL(d.revisionNumber(), 0);
	BOOST_CHECK_EQUAL(d.numberOfRedoableChanges(), 1);
	d.redo();
	BOOST_CHECK_EQUAL(d.length(), 19);
}

void testIterators() {
	k::Document d;
	k::insert(d, d.region().end(), x::utf::decode("This is the first line.\nThis is the second line.\r\nAnd this is the last line."));

	k::DocumentCharacterIterator i(d, d.region().beginning());
	BOOST_CHECK_EQUAL(i.document(), &d);
	BOOST_CHECK_EQUAL(i.tell(), d.region().beginning());
	BOOST_CHECK(i.hasNext());
	BOOST_CHECK(!i.hasPrevious());
	BOOST_CHECK_EQUAL(*i, 'T');
	i = std::find(i, k::DocumentCharacterIterator(d, d.region().end()), x::LINE_SEPARATOR);
	BOOST_CHECK_EQUAL(*i, x::LINE_SEPARATOR);
	std::advance(i, +25);
	BOOST_CHECK_EQUAL(*i, x::LINE_SEPARATOR);
	++i;
	BOOST_CHECK_EQUAL(*i, 'A');

	i.setRegion(k::Region(k::Position(1, 0), k::Position(1, 25)));
	BOOST_CHECK_EQUAL(i.tell(), i.region().end());
	BOOST_CHECK(!i.hasNext());
	++i;
	BOOST_CHECK_EQUAL(i.tell(), i.region().end());
	i.seek(d.region().beginning());
	BOOST_CHECK_EQUAL(i.tell(), i.region().beginning());
}

void testStreams() {
	k::Document d;

	k::DocumentOutputStream os(d);
	os << 0 << 1 << 2 << std::endl;
	BOOST_CHECK_EQUAL(d.line(0), x::utf::decode("012"));
}

void testBookmarks() {
	k::Document d;
	k::insert(d, d.region().end(), x::utf::decode(
		"m\n"
		"\n"
		"m\n"
		"m\n"
		"\n"
		"\n"
		"m\n"
		""));
	// this document has bookmarks at lines: 0, 2, 3, 6
	k::Bookmarker& b = d.bookmarker();
	b.mark(0);
	b.mark(2);
	b.toggle(3);
	b.toggle(6);

	BOOST_CHECK(b.isMarked(0));
	BOOST_CHECK(!b.isMarked(1));
	BOOST_CHECK(b.isMarked(2));
	BOOST_CHECK(b.isMarked(3));
	BOOST_CHECK(!b.isMarked(4));
	BOOST_CHECK(!b.isMarked(5));
	BOOST_CHECK(b.isMarked(6));
	BOOST_CHECK(!b.isMarked(7));
	BOOST_CHECK_EQUAL(b.numberOfMarks(), 4);

	// iterator
	k::Bookmarker::Iterator i(b.begin());
	BOOST_CHECK_EQUAL(*i, 0);
	BOOST_CHECK_EQUAL(*++i, 2);
	BOOST_CHECK_EQUAL(*++i, 3);
	BOOST_CHECK_EQUAL(*++i, 6);
	BOOST_CHECK(++i == b.end());
	BOOST_CHECK_EQUAL(*--i, 6);
	BOOST_CHECK_EQUAL(*--i, 3);
	BOOST_CHECK_EQUAL(*--i, 2);
	BOOST_CHECK_EQUAL(*--i, 0);
	BOOST_CHECK(i == b.begin());

	// Bookmarker.next
	BOOST_CHECK_EQUAL(b.next(0, a::Direction::FORWARD), 2);
	BOOST_CHECK_EQUAL(b.next(1, a::Direction::FORWARD), 2);
	BOOST_CHECK_EQUAL(b.next(7, a::Direction::FORWARD, true), 0);
	BOOST_CHECK_EQUAL(b.next(7, a::Direction::FORWARD, false), a::INVALID_INDEX);
	BOOST_CHECK_EQUAL(b.next(0, a::Direction::FORWARD, true, 8), 0);	// 4n
	BOOST_CHECK_EQUAL(b.next(0, a::Direction::FORWARD, true, 1002), 3);	// 4n + 2

	BOOST_CHECK_EQUAL(b.next(3, a::Direction::BACKWARD), 2);
	BOOST_CHECK_EQUAL(b.next(5, a::Direction::BACKWARD), 3);
	BOOST_CHECK_EQUAL(b.next(0, a::Direction::BACKWARD, true), 6);
	BOOST_CHECK_EQUAL(b.next(0, a::Direction::BACKWARD, false), a::INVALID_INDEX);
	BOOST_CHECK_EQUAL(b.next(1, a::Direction::BACKWARD, true, 2), 6);
	BOOST_CHECK_EQUAL(b.next(1, a::Direction::BACKWARD, true, 5), 0);
	BOOST_CHECK_EQUAL(b.next(2, a::Direction::BACKWARD, true, 1003), 3);	// 4n + 3
	
	BOOST_CHECK_EQUAL(b.next(1, a::Direction::FORWARD, true, 0), a::INVALID_INDEX);
	BOOST_CHECK_EQUAL(b.next(1, a::Direction::BACKWARD, true, 0), a::INVALID_INDEX);

	// update
	k::insert(d, d.region().beginning(), x::utf::decode("\n"));
	BOOST_CHECK(!b.isMarked(0));
	BOOST_CHECK(b.isMarked(1));
	BOOST_CHECK(!b.isMarked(2));
	BOOST_CHECK(b.isMarked(3));
	BOOST_CHECK(b.isMarked(4));
	BOOST_CHECK(!b.isMarked(5));
	BOOST_CHECK(!b.isMarked(6));
	BOOST_CHECK(b.isMarked(7));
	BOOST_CHECK(!b.isMarked(8));

	k::erase(d, k::Position(1, 0), k::Position(5, 0));
	BOOST_CHECK(!b.isMarked(0));
	BOOST_CHECK(b.isMarked(1));
	BOOST_CHECK(!b.isMarked(2));
	BOOST_CHECK(b.isMarked(3));
	BOOST_CHECK(!b.isMarked(4));

	d.resetContent();
	BOOST_CHECK(!b.isMarked(0));	// Document.resetContent removes all the bookmarks
	BOOST_CHECK_EQUAL(b.numberOfMarks(), 0);
}

int test_main(int, char*[]) {
	testMiscellaneousFunctions();
	testSimpleChange();
	testUndoBoundary();
	testCompoundChange();
	testIterators();
	testStreams();
	testBookmarks();
	return 0;
}
