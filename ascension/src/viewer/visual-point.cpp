/**
 * @file visual-point.cpp
 * @author exeal
 * @date 2003-2008 was point.cpp
 * @date 2008-2010 separated from point.cpp
 * @date 2011-10-02 separated from caret.cpp
 */

#include <ascension/viewer/viewer.hpp>
#include <ascension/viewer/visual-point.hpp>
#include <ascension/corelib/text/identifier-syntax.hpp>

using namespace ascension;
using namespace ascension::graphics;
using namespace ascension::kernel;
using namespace ascension::viewers;
using namespace ascension::presentation;
using namespace ascension::text;
using namespace std;


namespace {
	// copied from point.cpp
	inline const IdentifierSyntax& identifierSyntax(const Point& p) {
		return p.document().contentTypeInformation().getIdentifierSyntax(p.contentType());
	}
} // namespace @0

/**
 * Centers the current visual line addressed by the given visual point in the text viewer by
 * vertical scrolling the window.
 * @param p The visual point
 * @throw DocumentDisposedException 
 * @throw TextViewerDisposedException 
 */
void utils::recenter(VisualPoint& p) {
	// TODO: not implemented.
}

/**
 * Scrolls the text viewer until the given point is visible in the window.
 * @param p The visual point. this position will be normalized before the process
 * @throw DocumentDisposedException 
 * @throw TextViewerDisposedException 
 */
void utils::show(VisualPoint& p) {
	TextViewer& viewer = p.textViewer();
	const Position np(p.normalized());
	const font::TextRenderer& renderer = viewer.textRenderer();
	const length_t visibleLines = viewer.numberOfVisibleLines();
	NativePoint to(geometry::make<NativePoint>(-1, -1));

	// for vertical direction
	if(p.visualLine() < viewer.verticalScrollBar().position() * viewer.scrollRate(false))	// 画面より上
		geometry::y(to) = static_cast<Scalar>(p.visualLine() * viewer.scrollRate(false));
	else if(p.visualLine() - viewer.verticalScrollBar().position() * viewer.scrollRate(false) > visibleLines - 1)	// 画面より下
		geometry::y(to) = static_cast<Scalar>((p.visualLine() - visibleLines + 1) * viewer.scrollRate(false));
	if(geometry::y(to) < -1)
		geometry::y(to) = 0;

	// for horizontal direction
	if(!viewer.configuration().lineWrap.wrapsAtWindowEdge()) {
		const font::Font::Metrics& fontMetrics = renderer.defaultFont()->metrics();
		const length_t visibleColumns = viewer.numberOfVisibleColumns();
		const Scalar x = geometry::x(renderer.layouts().at(np.line).location(np.column, font::TextLayout::LEADING)) + renderer.lineIndent(np.line, 0);
		const Scalar scrollOffset = viewer.horizontalScrollBar().position() * viewer.scrollRate(true) * fontMetrics.averageCharacterWidth();
		if(x <= scrollOffset)	// 画面より左
			geometry::x(to) = x / fontMetrics.averageCharacterWidth() - visibleColumns / 4;
		else if(x >= (viewer.horizontalScrollBar().position() * viewer.scrollRate(true) + visibleColumns) * fontMetrics.averageCharacterWidth())	// 画面より右
			geometry::x(to) = x / fontMetrics.averageCharacterWidth() - visibleColumns * 3 / 4;
		if(geometry::x(to) < -1)
			geometry::x(to) = 0;
	}
	if(geometry::x(to) >= -1 || geometry::y(to) != -1)
		viewer.scrollTo(geometry::x(to), geometry::y(to), true);
}


// TextViewerDisposedException ////////////////////////////////////////////////////////////////////

TextViewerDisposedException::TextViewerDisposedException() :
		logic_error("The text viewer the object connecting to has been disposed.") {
}


// VisualPoint ////////////////////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param viewer The viewer
 * @param position The initial position of the point
 * @param listener The listener. can be @c null
 * @throw BadPositionException @a position is outside of the document
 */
VisualPoint::VisualPoint(TextViewer& viewer, const Position& position /* = Position() */, PointListener* listener /* = 0 */) :
		Point(viewer.document(), position, listener), viewer_(&viewer),
		lastX_(-1), crossingLines_(false), visualLine_(INVALID_INDEX), visualSubline_(0) {
	static_cast<detail::PointCollection<VisualPoint>&>(viewer).addNewPoint(*this);
	viewer_->textRenderer().layouts().addVisualLinesListener(*this);
}

/**
 * Copy-constructor.
 * @param other The source object
 * @throw DocumentDisposedException The document to which @a other belongs had been disposed
 * @throw TextViewerDisposedException The text viewer to which @a other belongs had been disposed
 */
VisualPoint::VisualPoint(const VisualPoint& other) : Point(other), viewer_(other.viewer_),
		lastX_(other.lastX_), crossingLines_(false), visualLine_(other.visualLine_), visualSubline_(other.visualSubline_) {
	if(viewer_ == 0)
		throw TextViewerDisposedException();
	static_cast<detail::PointCollection<VisualPoint>*>(viewer_)->addNewPoint(*this);
	viewer_->textRenderer().layouts().addVisualLinesListener(*this);
}

/// Destructor.
VisualPoint::~VisualPoint() /*throw()*/ {
	if(viewer_ != 0) {
		static_cast<detail::PointCollection<VisualPoint>*>(viewer_)->removePoint(*this);
		viewer_->textRenderer().layouts().removeVisualLinesListener(*this);
	}
}

/// @see Point#aboutToMove
void VisualPoint::aboutToMove(Position& to) {
	if(isTextViewerDisposed())
		throw TextViewerDisposedException();
	Point::aboutToMove(to);
}

/// @see Point#moved
void VisualPoint::moved(const Position& from) {
	if(isTextViewerDisposed())
		return;
	if(position() != Position() && from.line == line() && visualLine_ != INVALID_INDEX) {
		const font::TextLayout* const layout = viewer_->textRenderer().layouts().atIfCached(line());
		visualLine_ -= visualSubline_;
		visualSubline_ = (layout != 0) ? layout->lineAt(column()) : 0;
		visualLine_ += visualSubline_;
	} else
		visualLine_ = INVALID_INDEX;
	Point::moved(from);
	if(!crossingLines_)
		lastX_ = -1;
}
#if 0
/**
 * Inserts the spcified text as a rectangle at the current position. This method has two
 * restrictions as the follows:
 * - If the text viewer is line wrap mode, this method inserts text as linear not rectangle.
 * - If the destination line is bidirectional, the insertion may be performed incorrectly.
 * @param first The start of the text
 * @param last The end of the text
 * @throw NullPointerException @a first and/or @a last are @c null
 * @throw std#invalid_argument @a first &gt; @a last
 * @throw ... Any exceptions @c Document#insert throws
 * @see kernel#EditPoint#insert
 */
void VisualPoint::insertRectangle(const Char* first, const Char* last) {
	verifyViewer();

	// HACK: 
	if(textViewer().configuration().lineWrap.wraps())
		return insert(first, last);

	if(first == 0)
		throw NullPointerException("first");
	else if(last == 0)
		throw NullPointerException("last");
	else if(first > last)
		throw invalid_argument("first > last");
	else if(first == last)
		return;

	Document& doc = *document();
	DocumentLocker lock(doc);

	const length_t numberOfLines = doc.numberOfLines();
	length_t line = lineNumber();
	const TextRenderer& renderer = textViewer().textRenderer();
	const int x = renderer.lineLayout(line).location(columnNumber()).x + renderer.lineIndent(line, 0);
	const DocumentInput* const documentInput = doc.input();
	const String newline(getNewlineString((documentInput != 0) ? documentInput->newline() : ASCENSION_DEFAULT_NEWLINE));
	for(const Char* bol = first; ; ++line) {
		// find the next EOL
		const Char* const eol = find_first_of(bol, last, NEWLINE_CHARACTERS, ASCENSION_ENDOF(NEWLINE_CHARACTERS));

		// insert text if the source line is not empty
		if(eol > bol) {
			const LineLayout& layout = renderer.lineLayout(line);
			const length_t column = layout.offset(x - renderer.lineIndent(line), 0);
			String s(layout.fillToX(x));
			s.append(bol, eol);
			if(line >= numberOfLines - 1)
				s.append(newline);
			doc.insert(Position(line, column), s);	// this never throw
		}

		if(eol == last)
			break;
		bol = eol + ((eol[0] == CARRIAGE_RETURN && eol < last - 1 && eol[1] == LINE_FEED) ? 2 : 1);
	}
}
#endif
/// @internal @c Point#moveTo for @c VerticalDestinationProxy.
void VisualPoint::moveTo(const VerticalDestinationProxy& to) {
	if(lastX_ == -1)
		updateLastX();
	crossingLines_ = true;
	try {
		moveTo(to.position());
	} catch(...) {
		crossingLines_ = false;
		throw;
	}
	crossingLines_ = false;
}

/// Updates @c lastX_ with the current position.
inline void VisualPoint::updateLastX() {
	assert(!crossingLines_);
	if(isTextViewerDisposed())
		throw TextViewerDisposedException();
	if(!isDocumentDisposed()) {
		const font::TextLayout& layout = textViewer().textRenderer().layouts().at(line());
		lastX_ = layout.location(column(), font::TextLayout::LEADING).x;
		lastX_ += textViewer().textRenderer().lineIndent(line(), 0);
	}
}

/// Returns the visual column of the point.
length_t VisualPoint::visualColumn() const {
	if(lastX_ == -1)
		const_cast<VisualPoint*>(this)->updateLastX();
	const TextViewer::Configuration& c = viewer_->configuration();
	const font::TextRenderer& renderer = viewer_->textRenderer();
//	if(resolveTextAlignment(c.alignment, c.readingDirection) != ALIGN_RIGHT)
		return lastX_ / renderer.defaultFont()->metrics().averageCharacterWidth();
//	else
//		return (renderer.width() - lastX_) / renderer.averageCharacterWidth();
}

/// Returns the visual line number.
length_t VisualPoint::visualLine() const {
	if(visualLine_ == INVALID_INDEX) {
		VisualPoint& self = const_cast<VisualPoint&>(*this);
		const Position p(normalized());
		self.visualLine_ = textViewer().textRenderer().layouts().mapLogicalLineToVisualLine(p.line);
		self.visualSubline_ = textViewer().textRenderer().layouts().at(p.line).lineAt(p.column);
		self.visualLine_ += visualSubline_;
	}
	return visualLine_;
}

/// @see VisualLinesListener#visualLinesDeleted
void VisualPoint::visualLinesDeleted(const Range<length_t>& lines, length_t, bool) /*throw()*/ {
	if(!adaptsToDocument() && includes(lines, line()))
		visualLine_ = INVALID_INDEX;
}

/// @see VisualLinesListener#visualLinesInserted
void VisualPoint::visualLinesInserted(const Range<length_t>& lines) /*throw()*/ {
	if(!adaptsToDocument() && includes(lines, line()))
		visualLine_ = INVALID_INDEX;
}

/// @see VisualLinesListener#visualLinesModified
void VisualPoint::visualLinesModified(const Range<length_t>& lines, signed_length_t sublineDifference, bool, bool) /*throw()*/ {
	if(visualLine_ != INVALID_INDEX) {
		// adjust visualLine_ and visualSubine_ according to the visual lines modification
		if(lines.end() <= line())
			visualLine_ += sublineDifference;
		else if(lines.beginning() == line()) {
			visualLine_ -= visualSubline_;
			visualSubline_ = textViewer().textRenderer().layouts().at(line()).lineAt(min(column(), document().lineLength(line())));
			visualLine_ += visualSubline_;
		} else if(lines.beginning() < line())
			visualLine_ = INVALID_INDEX;
	}
}


// viewers.locations free functions ///////////////////////////////////////////////////////////////

/**
 * Returns the position returned by N pages.
 * @param p The base position
 * @param pages The number of pages to return
 * @return The destination
 */
VerticalDestinationProxy locations::backwardPage(const VisualPoint& p, length_t pages /* = 1 */) {
	// TODO: calculate exact number of visual lines.
	return backwardVisualLine(p, p.textViewer().numberOfVisibleLines() * pages);
}

/**
 * Returns the position returned by N visual lines.
 * @param p The base position
 * @param lines The number of the visual lines to return
 * @return The destination
 */
VerticalDestinationProxy locations::backwardVisualLine(const VisualPoint& p, length_t lines /* = 1 */) {
	Position np(p.normalized());
	const font::TextRenderer& renderer = p.textViewer().textRenderer();
	length_t subline = renderer.layouts().at(np.line).lineAt(np.column);
	if(np.line == 0 && subline == 0)
		return VisualPoint::makeVerticalDestinationProxy(np);
	font::VisualLine visualLine(np.line, subline);
	renderer.layouts().offsetVisualLine(visualLine, -static_cast<signed_length_t>(lines));
	const font::TextLayout& layout = renderer.layouts().at(visualLine.line);
	if(p.lastX_ == -1)
		const_cast<VisualPoint&>(p).updateLastX();
	np.column = layout.offset(
		geometry::make<NativePoint>(
			p.lastX_ - renderer.lineIndent(np.line),
			renderer.defaultFont()->metrics().linePitch() * static_cast<long>(subline)
		)).second;
	if(layout.lineAt(np.column) != visualLine.subline)
		np = nextCharacter(p.document(), np, Direction::BACKWARD, GRAPHEME_CLUSTER);
	return VisualPoint::makeVerticalDestinationProxy(np);
}

/**
 * Returns the beginning of the visual line.
 * @param p The base position
 * @return The destination
 * @see EditPoint#beginningOfLine
 */
Position locations::beginningOfVisualLine(const VisualPoint& p) {
	const Position np(p.normalized());
	const font::TextLayout& layout = p.textViewer().textRenderer().layouts().at(np.line);
	return Position(np.line, layout.lineOffset(layout.lineAt(np.column)));
}

/**
 * Returns the beginning of the line or the first printable character in the line by context.
 * @param p The base position
 * @return The destination
 */
Position locations::contextualBeginningOfLine(const VisualPoint& p) {
	return isFirstPrintableCharacterOfLine(p) ? beginningOfLine(p) : firstPrintableCharacterOfLine(p);
}

/**
 * Moves to the beginning of the visual line or the first printable character in the visual line by
 * context.
 * @param p The base position
 * @return The destination
 */
Position locations::contextualBeginningOfVisualLine(const VisualPoint& p) {
	return isFirstPrintableCharacterOfLine(p) ?
		beginningOfVisualLine(p) : firstPrintableCharacterOfVisualLine(p);
}

/**
 * Moves to the end of the line or the last printable character in the line by context.
 * @param p The base position
 * @return The destination
 */
Position locations::contextualEndOfLine(const VisualPoint& p) {
	return isLastPrintableCharacterOfLine(p) ? endOfLine(p) : lastPrintableCharacterOfLine(p);
}

/**
 * Moves to the end of the visual line or the last printable character in the visual line by
 * context.
 * @param p The base position
 * @return The destination
 */
Position locations::contextualEndOfVisualLine(const VisualPoint& p) {
	return isLastPrintableCharacterOfLine(p) ?
		endOfVisualLine(p) : lastPrintableCharacterOfVisualLine(p);
}

/**
 * Returns the end of the visual line.
 * @param p The base position
 * @return The destination
 * @see EditPoint#endOfLine
 */
Position locations::endOfVisualLine(const VisualPoint& p) {
	Position np(p.normalized());
	const font::TextLayout& layout = p.textViewer().textRenderer().layouts().at(np.line);
	const length_t subline = layout.lineAt(np.column);
	np.column = (subline < layout.numberOfLines() - 1) ?
		layout.lineOffset(subline + 1) : p.document().lineLength(np.line);
	if(layout.lineAt(np.column) != subline)
		np = nextCharacter(p.document(), np, Direction::BACKWARD, GRAPHEME_CLUSTER);
	return np;
}

/**
 * Returns the first printable character in the line.
 * @param p The base position
 * @return The destination
 */
Position locations::firstPrintableCharacterOfLine(const VisualPoint& p) {
	Position np(p.normalized());
	const Char* const s = p.document().line(np.line).data();
	np.column = identifierSyntax(p).eatWhiteSpaces(s, s + p.document().lineLength(np.line), true) - s;
	return np;
}

/**
 * Returns the first printable character in the visual line.
 * @param p The base position
 * @return The destination
 */
Position locations::firstPrintableCharacterOfVisualLine(const VisualPoint& p) {
	Position np(p.normalized());
	const String& s = p.document().line(np.line);
	const font::TextLayout& layout = p.textViewer().textRenderer().layouts().at(np.line);
	const length_t subline = layout.lineAt(np.column);
	np.column = identifierSyntax(p).eatWhiteSpaces(
		s.begin() + layout.lineOffset(subline),
		s.begin() + ((subline < layout.numberOfLines() - 1) ?
			layout.lineOffset(subline + 1) : s.length()), true) - s.begin();
	return np;
}

/**
 * Returns the position advanced by N pages.
 * @param p The base position
 * @param pages The number of pages to advance
 * @return The destination
 */
VerticalDestinationProxy locations::forwardPage(const VisualPoint& p, length_t pages /* = 1 */) {
	// TODO: calculate exact number of visual lines.
	return forwardVisualLine(p, p.textViewer().numberOfVisibleLines() * pages);
}

/**
 * Returns the position advanced by N visual lines.
 * @param p The base position
 * @param lines The number of the visual lines to advance
 * @return The destination
 */
VerticalDestinationProxy locations::forwardVisualLine(const VisualPoint& p, length_t lines /* = 1 */) {
	Position np(p.normalized());
	const font::TextRenderer& renderer = p.textViewer().textRenderer();
	const font::TextLayout* layout = &renderer.layouts().at(np.line);
	length_t subline = layout->lineAt(np.column);
	if(np.line == p.document().numberOfLines() - 1 && subline == layout->numberOfLines() - 1)
		return VisualPoint::makeVerticalDestinationProxy(np);
	font::VisualLine visualLine(np.line, subline);
	renderer.layouts().offsetVisualLine(visualLine, static_cast<signed_length_t>(lines));
	layout = &renderer.layouts().at(visualLine.line);
	if(p.lastX_ == -1)
		const_cast<VisualPoint&>(p).updateLastX();
	np.column = layout->offset(
		geometry::make<NativePoint>(
			p.lastX_ - renderer.lineIndent(visualLine.line),
			renderer.defaultFont()->metrics().linePitch() * static_cast<long>(visualLine.subline)
		)).second;
	if(layout->lineAt(np.column) != visualLine.subline)
		np = nextCharacter(p.document(), np, Direction::BACKWARD, GRAPHEME_CLUSTER);
	return VisualPoint::makeVerticalDestinationProxy(np);
}

/**
 * Returns @c true if the point is the beginning of the visual line.
 * @param p The base position
 * @see EditPoint#isBeginningOfLine
 */
bool locations::isBeginningOfVisualLine(const VisualPoint& p) {
	if(isBeginningOfLine(p))	// this considers narrowing
		return true;
	const Position np(p.normalized());
	const font::TextLayout& layout = p.textViewer().textRenderer().layouts().at(np.line);
	return np.column == layout.lineOffset(layout.lineAt(np.column));
}

/**
 * Returns @c true if the point is end of the visual line.
 * @param p The base position
 * @see kernel#EditPoint#isEndOfLine
 */
bool locations::isEndOfVisualLine(const VisualPoint& p) {
	if(isEndOfLine(p))	// this considers narrowing
		return true;
	const Position np(p.normalized());
	const font::TextLayout& layout = p.textViewer().textRenderer().layouts().at(np.line);
	const length_t subline = layout.lineAt(np.column);
	return np.column == layout.lineOffset(subline) + layout.lineLength(subline);
}

/// Returns @c true if the given position is the first printable character in the line.
bool locations::isFirstPrintableCharacterOfLine(const VisualPoint& p) {
	const Position np(p.normalized()), bob(p.document().accessibleRegion().first);
	const length_t offset = (bob.line == np.line) ? bob.column : 0;
	const String& line = p.document().line(np.line);
	return line.data() + np.column - offset
		== identifierSyntax(p).eatWhiteSpaces(line.data() + offset, line.data() + line.length(), true);
}

/// Returns @c true if the given position is the first printable character in the visual line.
bool locations::isFirstPrintableCharacterOfVisualLine(const VisualPoint& p) {
	// TODO: not implemented.
	return false;
}

/// Returns @c true if the given position is the last printable character in the line.
bool locations::isLastPrintableCharacterOfLine(const VisualPoint& p) {
	const Position np(p.normalized()), eob(p.document().accessibleRegion().second);
	const String& line = p.document().line(np.line);
	const length_t lineLength = (eob.line == np.line) ? eob.column : line.length();
	return line.data() + lineLength - np.column
		== identifierSyntax(p).eatWhiteSpaces(line.data() + np.column, line.data() + lineLength, true);
}

/// Returns @c true if the given position is the last printable character in the visual line.
bool locations::isLastPrintableCharacterOfVisualLine(const VisualPoint& p) {
	// TODO: not implemented.
	return false;
}

/**
 * Returns the last printable character in the line.
 * @param p The base position
 * @return The destination
 */
Position locations::lastPrintableCharacterOfLine(const VisualPoint& p) {
	Position np(p.normalized());
	const String& s(p.document().line(np.line));
	const IdentifierSyntax& syntax = identifierSyntax(p);
	for(length_t spaceLength = 0; spaceLength < s.length(); ++spaceLength) {
		if(syntax.isWhiteSpace(s[s.length() - spaceLength - 1], true))
			return np.column = s.length() - spaceLength, np;
	}
	return np.column = s.length(), np;
}

/**
 * Moves to the last printable character in the visual line.
 * @param p The base position
 * @return The destination
 */
Position locations::lastPrintableCharacterOfVisualLine(const VisualPoint& p) {
	// TODO: not implemented.
	return p.normalized();
}

namespace {
	inline ReadingDirection defaultUIReadingDirection(const VisualPoint& p) {
		return p.textViewer().textRenderer().defaultUIWritingMode().inlineFlowDirection;
	}
}

/**
 * Returns the position advanced to the left by N characters.
 * @param p The base position
 * @param unit Defines what a character is
 * @param characters The number of characters to adavance
 * @return The destination
 */
Position locations::leftCharacter(const VisualPoint& p, CharacterUnit unit, length_t characters /* = 1 */) {
	
	return (defaultUIReadingDirection(p) == LEFT_TO_RIGHT) ?
		backwardCharacter(p, unit, characters) : forwardCharacter(p, unit, characters);
}

/**
 * Returns the beginning of the word where advanced to the left by N words.
 * @param p The base position
 * @param words The number of words to adavance
 * @return The destination
 */
Position locations::leftWord(const VisualPoint& p, length_t words /* = 1 */) {
	return (defaultUIReadingDirection(p) == LEFT_TO_RIGHT) ? backwardWord(p, words) : forwardWord(p, words);
}

/**
 * Returns the end of the word where advanced to the left by N words.
 * @param p The base position
 * @param words The number of words to adavance
 * @return The destination
 */
Position locations::leftWordEnd(const VisualPoint& p, length_t words /* = 1 */) {
	return (defaultUIReadingDirection(p) == LEFT_TO_RIGHT) ? backwardWordEnd(p, words) : forwardWordEnd(p, words);
}

/**
 * Returns the position advanced to the right by N characters.
 * @param p The base position
 * @param unit Defines what a character is
 * @param characters The number of characters to adavance
 * @return The destination
 */
Position locations::rightCharacter(const VisualPoint& p, CharacterUnit unit, length_t characters /* = 1 */) {
	return (defaultUIReadingDirection(p) == LEFT_TO_RIGHT) ?
		forwardCharacter(p, unit, characters) : backwardCharacter(p, unit, characters);
}

/**
 * Returns the beginning of the word where advanced to the right by N words.
 * @param p The base position
 * @param words The number of words to adavance
 * @return The destination
 */
Position locations::rightWord(const VisualPoint& p, length_t words /* = 1 */) {
	return (defaultUIReadingDirection(p) == LEFT_TO_RIGHT) ? forwardWord(p, words) : backwardWord(p, words);
}

/**
 * Returns the end of the word where advanced to the right by N words.
 * @param p The base position
 * @param words The number of words to adavance
 * @return The destination
 */
Position locations::rightWordEnd(const VisualPoint& p, length_t words /* = 1 */) {
	return (defaultUIReadingDirection(p) == LEFT_TO_RIGHT) ? forwardWordEnd(p, words) : backwardWordEnd(p, words);
}
