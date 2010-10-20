// document-test.cpp

#include <ascension/document.hpp>
#include <ascension/stream.hpp>
#include <boost/test/included/test_exec_monitor.hpp>
namespace a = ascension;
namespace k = ascension::kernel;

void testMiscellaneousFunctions() {
	const a::String s(L"abc\ndef\r\n\rghi\x2028\x2029");
	// newlines:           ^    ^   ^    ^     ^

	BOOST_CHECK_EQUAL(k::getNumberOfLines(s), 6);
	BOOST_CHECK_EQUAL(k::eatNewline(s.begin(), s.end()), k::NLF_RAW_VALUE);
	BOOST_CHECK_EQUAL(k::eatNewline(s.begin() + 3, s.end()), k::NLF_LINE_FEED);
	BOOST_CHECK_EQUAL(k::eatNewline(s.begin() + 7, s.end()), k::NLF_CR_LF);
	BOOST_CHECK_EQUAL(k::eatNewline(s.begin() + 8, s.end()), k::NLF_LINE_FEED);
	BOOST_CHECK_EQUAL(k::eatNewline(s.begin() + 9, s.end()), k::NLF_CARRIAGE_RETURN);
	BOOST_CHECK_EQUAL(k::eatNewline(s.begin() + 13, s.end()), k::NLF_LINE_SEPARATOR);
	BOOST_CHECK_EQUAL(k::eatNewline(s.begin() + 14, s.end()), k::NLF_PARAGRAPH_SEPARATOR);
//	BOOST_CHECK_EQUAL(k::eatNewline(s.begin() + 15, s.end()), k::NLF_RAW_VALUE);
}

void testSimpleChange() {
	k::Document d;

	// initial state
	BOOST_CHECK_EQUAL(d.length(), 0);
	BOOST_CHECK_EQUAL(d.accessibleRegion(), k::Region());
	BOOST_CHECK_EQUAL(d.region(), k::Region());
	BOOST_CHECK_EQUAL(d.begin().tell(), d.end().tell());
	BOOST_CHECK_EQUAL(d.input(), static_cast<k::IDocumentInput*>(0));
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
	k::insert(d, k::Position(), a::String(L"abcde"));
	BOOST_CHECK(d.isModified());
	BOOST_CHECK_EQUAL(d.line(0), a::String(L"abcde"));
	BOOST_CHECK_EQUAL(d.length(), 5);
	BOOST_CHECK_EQUAL(d.region(), k::Region(k::Position(0, 0), k::Position(0, 5)));
	BOOST_CHECK_EQUAL(d.revisionNumber(), 1);
	BOOST_CHECK_EQUAL(d.numberOfUndoableChanges(), 1);
	k::erase(d, k::Position(0, 0), k::Position(0, 3));
	BOOST_CHECK_EQUAL(d.line(0), a::String(L"de"));
	BOOST_CHECK_EQUAL(d.revisionNumber(), 2);
	BOOST_CHECK_EQUAL(d.numberOfUndoableChanges(), 1);
	d.undo();
	BOOST_CHECK_EQUAL(d.line(0), a::String(L""));
	BOOST_CHECK_EQUAL(d.revisionNumber(), 0);
	BOOST_CHECK(!d.isModified());
	BOOST_CHECK_EQUAL(d.numberOfUndoableChanges(), 0);
	BOOST_CHECK_EQUAL(d.numberOfRedoableChanges(), 1);
	d.redo();
	BOOST_CHECK_EQUAL(d.line(0), a::String(L"de"));
	BOOST_CHECK_EQUAL(d.numberOfUndoableChanges(), 1);
	BOOST_CHECK_EQUAL(d.numberOfRedoableChanges(), 0);
	d.undo();
	BOOST_CHECK_EQUAL(d.revisionNumber(), 0);
}

void testUndoBoundary() {
	k::Document d;

	k::insert(d, k::Position(), a::String(L"a"));
	k::insert(d, k::Position(0, 1), a::String(L"b"));
	BOOST_CHECK_EQUAL(d.numberOfUndoableChanges(), 1);
	BOOST_CHECK_EQUAL(d.revisionNumber(), 2);
	d.undo();
	BOOST_CHECK_EQUAL(d.numberOfUndoableChanges(), 0);
	BOOST_CHECK_EQUAL(d.revisionNumber(), 0);
	k::insert(d, k::Position(), a::String(L"a"));
	d.insertUndoBoundary();
	k::insert(d, k::Position(0, 1), a::String(L"b"));
	BOOST_CHECK_EQUAL(d.numberOfUndoableChanges(), 2);
	d.undo();
	BOOST_CHECK_EQUAL(d.numberOfUndoableChanges(), 1);
	d.undo();
	BOOST_CHECK_EQUAL(d.numberOfUndoableChanges(), 0);
}

void testCompoundChange() {
	k::Document d;

	d.beginCompoundChange();
	k::insert(d, d.region().end(), a::String(L"This "));
	k::insert(d, d.region().end(), a::String(L"is a "));
	k::insert(d, d.region().end(), a::String(L"compound."));
	d.endCompoundChange();
	BOOST_CHECK_EQUAL(d.line(0), a::String(L"This is a compound."));
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
	k::insert(d, d.region().end(), L"This is the first line.\nThis is the second line.\r\nAnd this is the last line.");

	k::DocumentCharacterIterator i(d.begin());
	BOOST_CHECK_EQUAL(i.document(), &d);
	BOOST_CHECK_EQUAL(i.tell(), d.region().beginning());
	BOOST_CHECK(i.hasNext());
	BOOST_CHECK(!i.hasPrevious());
	BOOST_CHECK_EQUAL(*i, 'T');
	i = std::find(i, d.end(), a::LINE_SEPARATOR);
	BOOST_CHECK_EQUAL(*i, a::LINE_SEPARATOR);
	std::advance(i, +25);
	BOOST_CHECK_EQUAL(*i, a::LINE_SEPARATOR);
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
	BOOST_CHECK_EQUAL(d.line(0), a::String(L"012"));
}

void testBookmarks() {
	k::Document d;
	k::insert(d, d.region().end(),
		L"m\n"
		L"\n"
		L"m\n"
		L"m\n"
		L"\n"
		L"\n"
		L"m\n"
		L"");
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
	k::insert(d, d.region().beginning(), L"\n");
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
