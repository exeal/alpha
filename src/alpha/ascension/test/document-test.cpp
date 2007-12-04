// document-test.cpp

#include "../document.hpp"
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
	BOOST_CHECK_EQUAL(k::eatNewline(s.begin() + 15, s.end()), k::NLF_RAW_VALUE);
}

void testDocument() {
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
	BOOST_CHECK(!d.isSequentialEditing());
	BOOST_CHECK(d.line(0).empty());
	BOOST_CHECK_EQUAL(d.lineLength(0), 0);
	BOOST_CHECK_EQUAL(d.lineOffset(0), 0);
	BOOST_CHECK_EQUAL(d.numberOfLines(), 1);
	BOOST_CHECK_EQUAL(d.session(), static_cast<a::texteditor::Session*>(0));

	// simple modification
	d.insert(k::Position(), a::String(L"abcde"));
	BOOST_CHECK(d.isModified());
	BOOST_CHECK_EQUAL(d.line(0), a::String(L"abcde"));
	BOOST_CHECK_EQUAL(d.length(), 5);
	BOOST_CHECK_EQUAL(d.region(), k::Region(k::Position(0, 0), k::Position(0, 5)));
	BOOST_CHECK_EQUAL(d.revisionNumber(), 1);
	BOOST_CHECK_EQUAL(d.numberOfUndoableEdits(), 1);
	d.erase(k::Position(0, 0), k::Position(0, 3));
	BOOST_CHECK_EQUAL(d.line(0), a::String(L"de"));
	BOOST_CHECK_EQUAL(d.revisionNumber(), 2);
	BOOST_CHECK_EQUAL(d.numberOfUndoableEdits(), 2);
	d.undo();
	BOOST_CHECK_EQUAL(d.line(0), a::String(L"abcde"));
	BOOST_CHECK_EQUAL(d.revisionNumber(), 1);
	d.undo();
	BOOST_CHECK_EQUAL(d.length(), 0);
	BOOST_CHECK_EQUAL(d.revisionNumber(), 0);
	BOOST_CHECK(!d.isModified());
	BOOST_CHECK_EQUAL(d.numberOfUndoableEdits(), 0);
	BOOST_CHECK_EQUAL(d.numberOfRedoableEdits(), 2);
	d.redo();
	BOOST_CHECK_EQUAL(d.line(0), a::String(L"abcde"));
	BOOST_CHECK_EQUAL(d.numberOfRedoableEdits(), 1);
	d.undo();

	// compound modification
	d.beginSequentialEdit();
	d.insert(d.region().end(), a::String(L"This "));
	d.insert(d.region().end(), a::String(L"is a "));
	d.insert(d.region().end(), a::String(L"compound."));
	d.endSequentialEdit();
	BOOST_CHECK_EQUAL(d.line(0), a::String(L"This is a compound."));
	BOOST_CHECK_EQUAL(d.revisionNumber(), 3);
	BOOST_CHECK_EQUAL(d.numberOfUndoableEdits(), 1);
	d.undo();
	BOOST_CHECK_EQUAL(d.length(), 0);
	BOOST_CHECK_EQUAL(d.revisionNumber(), 0);
	BOOST_CHECK_EQUAL(d.numberOfRedoableEdits(), 1);
	d.redo();
	BOOST_CHECK_EQUAL(d.length(), 19);
}

void testIterators() {
	k::Document d;
	d.insert(d.region().end(), L"This is the first line.\nThis is the second line.\r\nAnd this is the last line.");

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

int test_main(int, char*[]) {
	testMiscellaneousFunctions();
	testDocument();
	testIterators();
	testStreams();
	return 0;
}
