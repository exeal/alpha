/**
 * @file point.cpp
 * @author exeal
 * @date 2003-2007
 */

#include "stdafx.h"
#include "viewer.hpp"
#include "../../manah/win32/utility.hpp"

using namespace ascension;
using namespace ascension::text;
using namespace ascension::layout;
using namespace ascension::viewers;
using namespace ascension::presentation;
using namespace ascension::unicode;
using namespace ascension::unicode::ucd;
//using namespace ascension::internal;
using namespace manah::win32;
using namespace std;


namespace {
	/// The clipboard.
	class Clipboard : public manah::Noncopyable {
	public:
		class Text : public manah::Unassignable {
		public:
			Text(HGLOBAL handle, const Char* text) throw() : handle_(handle), text_(text) {}
			Text(const Text& rhs) throw() : handle_(rhs.handle_), text_(rhs.text_) {const_cast<Text&>(rhs).handle_ = 0;}
			~Text() throw() {if(handle_ != 0) ::GlobalUnlock(handle_);}
			const Char* getData() const throw() {return text_;}
			length_t getRawSize() const throw() {return (handle_ != 0) ? ::GlobalSize(handle_) : 0;}
			operator bool() const throw() {return handle_ != 0 && text_ != 0;}
		private:
			HGLOBAL handle_;
			const Char* const text_;
		};
		Clipboard(HWND window) throw() : opened_(toBoolean(::OpenClipboard(window))) {}
		~Clipboard() throw() {if(opened_) ::CloseClipboard();}
		bool isOpen() const throw() {return opened_;}
		Text read() throw();
		void write(const Char* first, const Char* last, bool asRectangle = false) throw();
		void write(const String& s, bool asRectangle = false) throw() {write(s.data(), s.data() + s.length(), asRectangle);}
	private:
		bool opened_;
	};

	/**
	 * Reads the text from the clipboard.
	 * @return the text
	 */
	Clipboard::Text Clipboard::read() throw() {
		assert(isOpen());
		if(HGLOBAL data = ::GetClipboardData(CF_UNICODETEXT))
			return Text(data, static_cast<Char*>(::GlobalLock(data)));
		return Text(0, 0);
	}

	/**
	 * Writes the text into the clipboard.
	 * @param first the start of the text
	 * @param last the end of the text
	 * @param asRectangle true to write the text using rectangle data format
	 * @see ASCENSION_RECTANGLE_TEXT_CLIP_FORMAT
	 */
	void Clipboard::write(const Char* first, const Char* last, bool asRectangle /* = false */) throw() {
		assert(isOpen());
		if(HGLOBAL data = ::GlobalAlloc(GMEM_MOVEABLE, sizeof(Char) * (last - first + 1))) {
			if(Char* buffer = static_cast<Char*>(::GlobalLock(data))) {
				uninitialized_copy(first, last, buffer);
				::GlobalUnlock(data);
				::EmptyClipboard();
				::SetClipboardData(CF_UNICODETEXT, data);
				if(asRectangle) {
					if(const UINT clipFormat = ::RegisterClipboardFormat(ASCENSION_RECTANGLE_TEXT_CLIP_FORMAT)) {
						data = ::GlobalAlloc(GMEM_MOVEABLE, 1);
						buffer = static_cast<Char*>(::GlobalLock(data));
						buffer[0] = 0;
						::GlobalUnlock(data);
						::SetClipboardData(clipFormat, data);
					}
				}
			}
		}
	}
} // namespace @0

namespace {
	Position getForwardCharacterPosition(const Document& document, const Position& position, EditPoint::CharacterUnit cu, length_t offset = 1) {
		if(offset == 0)
			return position;
		else if(cu == EditPoint::UTF16_CODE_UNIT) {
			UTF32To16Iterator<DocumentCharacterIterator> i(DocumentCharacterIterator(document, position));
			while(offset-- > 0)
				++i;
			return i.tell().tell();
		} else if(cu == EditPoint::UTF32_CODE_UNIT) {
			DocumentCharacterIterator i(document, position);
			while(offset-- > 0)
				i.next();
			return i.tell();
		} else if(cu == EditPoint::GRAPHEME_CLUSTER) {
			GraphemeBreakIterator<DocumentCharacterIterator> i(DocumentCharacterIterator(document, position));
			i.next(offset);
			return i.base().tell();
		} else if(cu == EditPoint::GLYPH_CLUSTER) {
			// TODO: not implemented.
		}
		throw invalid_argument("unknown character unit.");
	}
	Position getBackwardCharacterPosition(const Document& document, const Position& position, EditPoint::CharacterUnit cu, length_t offset = 1) {
		assert(cu != EditPoint::DEFAULT_UNIT);
		if(offset == 0)
			return position;
		else if(cu == EditPoint::UTF16_CODE_UNIT) {
			UTF32To16Iterator<DocumentCharacterIterator> i(DocumentCharacterIterator(document, position));
			while(offset-- > 0)
				--i;
			return i.tell().tell();
		} else if(cu == EditPoint::UTF32_CODE_UNIT) {
			DocumentCharacterIterator i(document, position);
			while(offset-- > 0)
				i.previous();
			return i.tell();
		} else if(cu == EditPoint::GRAPHEME_CLUSTER) {
			GraphemeBreakIterator<DocumentCharacterIterator> i(DocumentCharacterIterator(document, position));
			i.next(-static_cast<signed_length_t>(offset));
			return i.base().tell();
		} else if(cu == EditPoint::GLYPH_CLUSTER) {
			// TODO: not implemented.
		}
		throw invalid_argument("unknown character unit.");
	}
} // namespace @0


// EditPoint ////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param document the document
 * @param position the initial position of the point
 * @param listener the listener. can be @c null if not needed
 * @throw std#invalid_argument @a position is outside of the document
 */
EditPoint::EditPoint(Document& document, const Position& position /* = Position() */, IPointListener* listener /* = 0 */)
	: Point(document, position), listener_(listener), characterUnit_(GRAPHEME_CLUSTER) {
}

/// Copy-constructor.
EditPoint::EditPoint(const EditPoint& rhs) throw() : Point(rhs), listener_(rhs.listener_), characterUnit_(rhs.characterUnit_) {
}

/// Destructor.
EditPoint::~EditPoint() throw() {
	if(listener_ != 0)
		listener_->pointDestroyed();
}

/**
 * Moves to the previous (backward) character.
 * @param offset the offset of the movement
 */
void EditPoint::backwardCharacter(length_t offset /* = 1 */) {
	verifyDocument();
	normalize();
	moveTo(getBackwardCharacterPosition(*getDocument(), *this, getCharacterUnit(), offset));
}

/// Moves to the beginning of the document.
void EditPoint::beginningOfDocument() {
	moveTo(Position::ZERO_POSITION);
}

/// Moves to the beginning of the line.
void EditPoint::beginningOfLine() {
	moveTo(Position(min(getLineNumber(), getDocument()->getNumberOfLines() - 1), 0));
}

/**
 * Deletes the current character and inserts the specified text.
 * @param first the start of the text
 * @param last the end of the text
 */
void EditPoint::destructiveInsert(const Char* first, const Char* last) {
	verifyDocument();
	if(getDocument()->isReadOnly())
		return;
	EditPoint p = EditPoint(*this);
	p.adaptToDocument(false);
	Document& document = *getDocument();
	p.forwardCharacter();
	if(p != *this) {
		const bool adapts = adaptsToDocument();
		adaptToDocument(false);
		document.erase(Region(*this, p));
		moveTo(document.insert(*this, first, last));
		adaptToDocument(adapts);
	}
}

/// @see Point#doMoveTo
void EditPoint::doMoveTo(const Position& to) {
	verifyDocument();
	if(to != getPosition()) {
		const Position oldPosition = getPosition();
		Point::doMoveTo(to);
		if(listener_ != 0)
			listener_->pointMoved(*this, oldPosition);
	}
}

/// Moves to the end of the document.
void EditPoint::endOfDocument() {
	const length_t lines = getDocument()->getNumberOfLines();
	moveTo(Position(lines - 1, getDocument()->getLineLength(lines - 1)));
}

/// Moves to the end of the line.
void EditPoint::endOfLine() {
	moveTo(Position(min(getLineNumber(), getDocument()->getNumberOfLines() - 1), getDocument()->getLineLength(getLineNumber())));
}

/**
 * 指定位置までのテキストを削除
 * @param length もう1つの位置までの文字数 (負でもよい)
 * @param cu 文字数の計算方法。@c DEFAULT_UNIT を指定すると現在の設定が使用される
 */
void EditPoint::erase(signed_length_t length /* = 1 */, EditPoint::CharacterUnit cu /* = DEFAULT_UNIT */) {
	verifyDocument();
	if(getDocument()->isReadOnly() || length == 0)
		return;
	erase((length > 0) ?
		getForwardCharacterPosition(*getDocument(), *this, getCharacterUnit(), length) : getBackwardCharacterPosition(*getDocument(), *this, UTF16_CODE_UNIT, -length));
}

/**
 * Erase the region between the point and the other specified point.
 * @param other the other point
 */
void EditPoint::erase(const Position& other) {
	verifyDocument();
	if(getDocument()->isReadOnly() || other == getPosition())
		return;
	getDocument()->erase(Region(*this, other));
}

/**
 * Moves to the next (forward) character.
 * @param offset the offset of the movement
 */
void EditPoint::forwardCharacter(length_t offset /* = 1 */) {
	verifyDocument();
	normalize();
	moveTo(getForwardCharacterPosition(*getDocument(), *this, getCharacterUnit(), offset));
}

/**
 * Returns the code point of the current character.
 * @param useLineFeed true to return LF (U+000A) when the current position is end of the line. otherwise LS (U+2008)
 * @return the code point. if the current position is end of document, result is @c INVALID_CODE_POINT
 */
CodePoint EditPoint::getCodePoint(bool useLineFeed /* = false */) const {
	verifyDocument();
	const String& line = getDocument()->getLine(getLineNumber());
	if(getColumnNumber() == line.length())
		return (getLineNumber() == getDocument()->getNumberOfLines() - 1) ? INVALID_CODE_POINT : (useLineFeed ? LINE_FEED : LINE_SEPARATOR);
	return surrogates::decodeFirst(line.begin() + getColumnNumber(), line.end());
}

inline String EditPoint::getText(signed_length_t length, NewlineRepresentation nlr /* = NLR_PHYSICAL_DATA */) const {
	return getText((length >= 0) ?
		getForwardCharacterPosition(*getDocument(), *this, getCharacterUnit(), length)
		: getBackwardCharacterPosition(*getDocument(), *this, getCharacterUnit(), length), nlr);
}

inline String EditPoint::getText(const Position& other, NewlineRepresentation nlr /* = NLR_PHYSICAL_DATA */) const {
	OutputStringStream s;
	getDocument()->writeToStream(s, Region(*this, other), nlr);
	return s.str();
}

/**
 * Inserts the spcified text at the current position.
 * @param first the start of the text
 * @param last the end of the text
 * @see viewers#VisualPoint#insertBox
 */
void EditPoint::insert(const Char* first, const Char* last) {
	verifyDocument();
	if(getDocument()->isReadOnly() || first == last)
		return;
	const bool adapts = adaptsToDocument();
	adaptToDocument(false);
	moveTo(getDocument()->insert(*this, first, last));
	adaptToDocument(adapts);
}

/// Returns true if the point is the beginning of the document.
bool EditPoint::isBeginningOfDocument() const {
	verifyDocument();
	normalize();
	return isExcludedFromRestriction() ? getPosition() == getDocument()->getStartPosition() : getPosition() == Position::ZERO_POSITION;
}

/// Returns true if the point is the beginning of the line.
bool EditPoint::isBeginningOfLine() const {
	verifyDocument();
	normalize();
	if(isExcludedFromRestriction()) {
		const Position start = getDocument()->getStartPosition();
		return (start.line == getLineNumber()) ? start.column == getColumnNumber() : getColumnNumber() == 0;
	} else
		return getColumnNumber() == 0;
}

/// Returns true if the point is the end of the document.
bool EditPoint::isEndOfDocument() const {
	verifyDocument();
	normalize();
	return isExcludedFromRestriction() ? getPosition() == getDocument()->getEndPosition()
		: getLineNumber() == getDocument()->getNumberOfLines() - 1 && getColumnNumber() == getDocument()->getLineLength(getLineNumber());
}

/// Returns true if the point is the end of the line.
bool EditPoint::isEndOfLine() const {
	verifyDocument();
	normalize();
	if(isExcludedFromRestriction()) {
		const Position end = getDocument()->getEndPosition();
		return (end.line == getLineNumber()) ?
			getColumnNumber() == end.column : getColumnNumber() == getDocument()->getLineLength(getLineNumber());
	} else
		return getColumnNumber() == getDocument()->getLineLength(getLineNumber());
}

/**
 * Moves to the specified offset.
 * @param offset the offset from the start of the document.
 * @deprecated 0.8
 */
void EditPoint::moveToAbsoluteCharacterOffset(length_t offset) {
	verifyDocument();

	const Document& document = *getDocument();
	length_t readCount = 0;
	const Position start = document.getStartPosition();
	const Position end = document.getEndPosition();

	if(document.getLineLength(start.line) + 1 - start.column >= offset) {
		moveTo(Position(start.line, start.column + offset));
		return;
	}
	readCount += document.getLineLength(start.line) + 1 - start.column;
	for(length_t line = start.line + 1; line <= end.line; ++line) {
		const length_t lineLength = document.getLineLength(line) + 1;	// +1 is for a newline
		if(readCount + lineLength >= offset) {
			moveTo(Position(line, readCount + lineLength - offset));
			return;
		}
		readCount += lineLength;
	}
	moveTo(Position(end.line, document.getLineLength(end.line)));
}

/**
 * Breaks the line.
 * @note This method is hidden by @c VisualPoint#newLine (C++ rule).
 */
void EditPoint::newLine() {
	verifyDocument();
	if(getDocument()->isReadOnly())
		return;
	insert(getNewlineString(getDocument()->getNewline()));
}

/**
 * Moves to the beginning of the next bookmarked line.
 * @return false if the bookmark is not found
 */
bool EditPoint::nextBookmark() {
	verifyDocument();

	const Bookmarker& bookmarker = getDocument()->getBookmarker();
	length_t line;
	const length_t endLine = getDocument()->getEndPosition().line;

	// search...
	for(line = getLineNumber() + 1; line <= endLine; ++line) {
		if(bookmarker.isMarked(line)) {
			moveTo(Position(line, 0));
			return true;
		}
	}

	// wrap around if not found
	for(line = getDocument()->getStartPosition().line; line < getLineNumber(); ++line) {
		if(bookmarker.isMarked(line)) {
			moveTo(Position(line, 0));
			return true;
		}
	}

	return false;
}

/**
 * Moves to the next line.
 * @param offset the offset of the movement
 */
void EditPoint::nextLine(length_t offset /* = 1 */) {
	verifyDocument();
	normalize();
	const length_t newLine = min(getLineNumber() + offset, getDocument()->getEndPosition(isExcludedFromRestriction()).line);
	if(newLine != getLineNumber())
		moveTo(Position(newLine, getColumnNumber()));
}

/**
 * Moves to the beginning of the previous bookmarked line.
 * @return false if the bookmark is not found
 */
bool EditPoint::previousBookmark() {
	verifyDocument();

	const Bookmarker& bookmarker = getDocument()->getBookmarker();
	length_t line = getLineNumber();
	const length_t startLine = getDocument()->getStartPosition().line;

	// search...
	while(line-- != startLine) {
		if(bookmarker.isMarked(line)) {
			moveTo(Position(line, 0));
			return true;
		}
	}

	// wrap around if not found
	for(line = getDocument()->getEndPosition().line; line > getLineNumber(); --line) {
		if(bookmarker.isMarked(line)) {
			moveTo(Position(line, 0));
			return true;
		}
	}

	return false;
}

/**
 * Moves to the previous line.
 * @param offset the offset of the movement
 */
void EditPoint::previousLine(length_t offset /* = 1 */) {
	verifyDocument();
	normalize();
	const length_t newLine = (getLineNumber() > offset) ?
		max(getLineNumber() - offset, getDocument()->getStartPosition(isExcludedFromRestriction()).line) : 0;
	if(newLine != getLineNumber())
		moveTo(Position(newLine, getColumnNumber()));
}


// VisualPoint //////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param viewer the viewer
 * @param position the initial position of the point
 * @param listener the listener. can be @c null
 * @throw std#invalid_argument @a position is outside of the document
 */
VisualPoint::VisualPoint(TextViewer& viewer, const Position& position /* = Position() */, IPointListener* listener /* = 0 */) :
		EditPoint(viewer.getDocument(), position, listener),viewer_(&viewer),
		clipboardNativeEncoding_(::GetACP()), lastX_(-1), crossingLines_(false), visualLine_(INVALID_INDEX), visualSubline_(0) {
	static_cast<text::internal::IPointCollection<VisualPoint>&>(viewer).addNewPoint(*this);
	viewer_->getTextRenderer().addVisualLinesListener(*this);
}

/// Copy-constructor.
VisualPoint::VisualPoint(const VisualPoint& rhs) : EditPoint(rhs), viewer_(rhs.viewer_),
		lastX_(rhs.lastX_), crossingLines_(false), visualLine_(rhs.visualLine_), visualSubline_(rhs.visualSubline_) {
	if(viewer_ == 0)
		throw DisposedViewerException();
	static_cast<text::internal::IPointCollection<VisualPoint>*>(viewer_)->addNewPoint(*this);
	viewer_->getTextRenderer().addVisualLinesListener(*this);
}

/// Destructor.
VisualPoint::~VisualPoint() throw() {
	if(viewer_ != 0) {
		static_cast<text::internal::IPointCollection<VisualPoint>*>(viewer_)->removePoint(*this);
		viewer_->getTextRenderer().removeVisualLinesListener(*this);
	}
}

/**
 * Moves to the beginning of the visual line.
 * @see EditPoint#beginningOfLine
 */
void VisualPoint::beginningOfVisualLine() {
	verifyViewer();
	const LineLayout& layout = getTextViewer().getTextRenderer().getLineLayout(getLineNumber());
	moveTo(Position(getLineNumber(), layout.getSublineOffset(layout.getSubline(getColumnNumber()))));
}

/**
 * Returns if a paste operation can be performed.
 * @return the pastable clipboard format or 0
 */
UINT VisualPoint::canPaste() {
	const UINT boxClipFormat = ::RegisterClipboardFormatW(ASCENSION_RECTANGLE_TEXT_CLIP_FORMAT);
	if(boxClipFormat != 0 && toBoolean(::IsClipboardFormatAvailable(boxClipFormat)))
		return boxClipFormat;
	if(toBoolean(::IsClipboardFormatAvailable(CF_UNICODETEXT)))
		return CF_UNICODETEXT;
	if(toBoolean(::IsClipboardFormatAvailable(CF_TEXT)))
		return CF_TEXT;
	return 0;
}

/**
 * Writes the specified region into the clipboard.
 * @param length the number of the characters to copy. can be negative
 */
void VisualPoint::copy(signed_length_t length) {
	verifyViewer();
	const String text = getText(length);	
	Clipboard(viewer_->getHandle()).write(text.data(), text.data() + text.length());
}

/**
 * Writes the specified region into the clipboard.
 * @param other もう1つの位置
 */
void VisualPoint::copy(const Position& other) {
	verifyViewer();
	const String text = getText(other);
	Clipboard(viewer_->getHandle()).write(text.data(), text.data() + text.length());
}

/**
 * Erases the specified region and writes into the clipboard.
 * @param length the number of the characters to delete
 */
void VisualPoint::cut(signed_length_t length) {
	verifyViewer();
	if(getDocument()->isReadOnly())
		return;
	const String text = getText(length);
	Clipboard(viewer_->getHandle()).write(text.data(), text.data() + text.length());
	erase(length);
}

/**
 * Erases the specified region and writes into the clipboard.
 * @param other もう1つの位置
 */
void VisualPoint::cut(const Position& other) {
	verifyViewer();
	if(getDocument()->isReadOnly())
		return;
	const String text = getText(other);
	Clipboard(viewer_->getHandle()).write(text.data(), text.data() + text.length());
	erase(other);
}

/**
 * 範囲内のテキストをインデント
 * @param other もう1つの位置
 * @param character インデントに使う文字
 * @param box 矩形インデントか (インデントレベルが負のときは無視される)
 * @param level インデントレベル
 * @return 操作の結果 @a pos が移動するとよい位置
 */
Position VisualPoint::doIndent(const Position& other, Char character, bool box, long level) {
	verifyViewer();

	Document& document = *getDocument();

	if(document.isReadOnly() || level == 0)
		return other;

	const String indent = String(abs(level), character);
	const Region region(*this, other);

	if(region.getTop().line == region.getBottom().line) {	// 選択が1行以内 -> 単純な文字挿入
		document.erase(region);
		document.insert(region.getTop(), indent);
		return getPosition();
	}

	const Position oldPosition = getPosition();
	Position otherResult = other;
	length_t line = region.getTop().line;
	const bool adapts = adaptsToDocument();

	adaptToDocument(false);

	// 最初の行を (逆) インデント
	if(level > 0) {
		document.insert(Position(line, box ? region.getTop().column : 0), indent);
		if(line == otherResult.line && otherResult.column != 0)
			otherResult.column += level;
		if(line == getLineNumber() && getColumnNumber() != 0)
			moveTo(getLineNumber(), getColumnNumber() + level);
	} else {
		const String& s = document.getLine(line);
		length_t indentLength;
		for(indentLength = 0; indentLength < s.length(); ++indentLength) {
			// 空白類文字が BMP にしか無いという前提
			if(s[indentLength] == L'\t' && GeneralCategory::of(s[indentLength]) != GeneralCategory::SEPARATOR_SPACE)
				break;
		}
		if(indentLength > 0) {
			const length_t deleteLength = min<length_t>(-level, indentLength);
			document.erase(Position(line, 0), Position(line, deleteLength));
			if(line == otherResult.line && otherResult.column != 0)
				otherResult.column -= deleteLength;
			if(line == getLineNumber() && getColumnNumber() != 0)
				moveTo(getLineNumber(), getColumnNumber() - deleteLength);
		}
	}

	// 選択のある全ての行を (逆) インデント
	if(level > 0) {
		for(++line; line <= region.getBottom().line; ++line) {
			if(document.getLineLength(line) != 0 && (line != region.getBottom().line || region.getBottom().column > 0)) {
				length_t insertPosition = 0;
				if(box) {
					length_t dummy;
					viewer_->getCaret().getBoxForRectangleSelection().getOverlappedSubline(line, 0, insertPosition, dummy);	// TODO: recognize wrap (second parameter).
				}
				document.insert(Position(line, insertPosition), indent);
				if(line == otherResult.line && otherResult.column != 0)
					otherResult.column += level;
				if(line == getLineNumber() && getColumnNumber() != 0)
					moveTo(getLineNumber(), getColumnNumber() + level);
			}
		}
	} else {
		for(++line; line <= region.getBottom().line; ++line) {
			const String& s = document.getLine(line);
			length_t indentLength;
			for(indentLength = 0; indentLength < s.length(); ++indentLength) {
				// 空白類文字が BMP にしか無いという前提
				if(s[indentLength] == L'\t' && GeneralCategory::of(s[indentLength]) != GeneralCategory::SEPARATOR_SPACE)
					break;
			}
			if(indentLength > 0) {
				const length_t deleteLength = min<length_t>(-level, indentLength);
				document.erase(Position(line, 0), Position(line, deleteLength));
				if(line == otherResult.line && otherResult.column != 0)
					otherResult.column -= deleteLength;
				if(line == getLineNumber() && getColumnNumber() != 0)
					moveTo(getLineNumber(), getColumnNumber() - deleteLength);
			}
		}
	}

	adaptToDocument(adapts);
	if(getListener() != 0)
		getListener()->pointMoved(*this, oldPosition);
	return otherResult;
}

/// @see EditPoint#doMoveTo
void VisualPoint::doMoveTo(const Position& to) {
	verifyViewer();
	if(getLineNumber() == to.line && visualLine_ != INVALID_INDEX) {
		visualLine_ -= visualSubline_;
		const LineLayout* layout = viewer_->getTextRenderer().getLineLayoutIfCached(to.line);
		visualSubline_ = (layout != 0) ? layout->getSubline(to.column) : 0;
		visualLine_ += visualSubline_;
	} else
		visualLine_ = INVALID_INDEX;
	EditPoint::doMoveTo(to);
	if(!crossingLines_)
		lastX_ = -1;
}

/**
 * Moves to the end of the visual line.
 * @see EditPoint#endOfLine
 */
void VisualPoint::endOfVisualLine() {
	verifyViewer();
	const LineLayout& layout = getTextViewer().getTextRenderer().getLineLayout(getLineNumber());
	const length_t subline = layout.getSubline(getColumnNumber());
	Position newPosition(getLineNumber(), (subline < layout.getNumberOfSublines() - 1) ?
		layout.getSublineOffset(subline + 1) : getDocument()->getLineLength(getLineNumber()));
	if(layout.getSubline(newPosition.column) != subline)
		newPosition = getBackwardCharacterPosition(*getDocument(), newPosition, getCharacterUnit());
	moveTo(newPosition);
}

/// Moves to the first printable character in the line.
void VisualPoint::firstPrintableCharacterOfLine() {
	verifyViewer();
	const length_t line = min(getLineNumber(), getDocument()->getEndPosition(isExcludedFromRestriction()).line);
	const Char* const p = getDocument()->getLine(line).data();
	moveTo(Position(line, getIdentifierSyntax().eatWhiteSpaces(p, p + getDocument()->getLineLength(line), true) - p));
}

/// Moves to the first printable character in the visual line.
void VisualPoint::firstPrintableCharacterOfVisualLine() {
	verifyViewer();
	const length_t line = min(getLineNumber(), getDocument()->getEndPosition(isExcludedFromRestriction()).line);
	const String& s = getDocument()->getLine(line);
	const LineLayout& layout = viewer_->getTextRenderer().getLineLayout(line);
	const length_t subline = layout.getSubline(getColumnNumber());
	moveTo(Position(line,
		getIdentifierSyntax().eatWhiteSpaces(s.begin() + layout.getSublineOffset(subline),
			s.begin() + ((subline < layout.getNumberOfSublines() - 1) ? layout.getSublineOffset(subline + 1) : s.length()), true) - s.begin()));
}

/// 
inline const IdentifierSyntax& VisualPoint::getIdentifierSyntax() const {
	return getDocument()->getContentTypeInformation().getIdentifierSyntax(getContentType());
}

/// Returns the visual column of the point.
length_t VisualPoint::getVisualColumnNumber() const {
	if(lastX_ == -1)
		const_cast<VisualPoint*>(this)->updateLastX();
	const TextViewer::Configuration& c = viewer_->getConfiguration();
	const TextRenderer& renderer = viewer_->getTextRenderer();
	if(c.alignment == ALIGN_LEFT || (c.alignment != ALIGN_RIGHT && c.orientation == LEFT_TO_RIGHT))
		return lastX_ / renderer.getAverageCharacterWidth();
	else
		return (renderer.getWidth() - lastX_) / renderer.getAverageCharacterWidth();
}

/**
 * Inserts the spcified text as a rectangle at the current position. This method has two
 * restrictions as the follows:
 * - If the text viewer is line wrap mode, this method inserts text as linear not rectangle.
 * - If the destination line is bidirectional, the insertion may be performed incorrectly.
 * @param first the start of the text
 * @param last the end of the text
 * @see text#EditPoint#insert
 */
void VisualPoint::insertBox(const Char* first, const Char* last) {
	verifyViewer();

	// HACK: 
	if(getTextViewer().getConfiguration().lineWrap.wraps())
		return insert(first, last);

	Document& document = *getDocument();
	if(document.isReadOnly() || first == last)
		return;

	const length_t numberOfLines = document.getNumberOfLines();
	length_t line = getLineNumber();
	const TextRenderer& renderer = getTextViewer().getTextRenderer();
	const int x = renderer.getLineLayout(line).getLocation(getColumnNumber()).x + renderer.getLineIndent(line, 0);
	const String breakString = getNewlineString(document.getNewline());
	for(const Char* bol = first; ; ++line) {
		// find the next EOL
		const Char* const eol = find_first_of(bol, last, LINE_BREAK_CHARACTERS, endof(LINE_BREAK_CHARACTERS));

		// insert text if the source line is not empty
		if(eol > bol) {
			const LineLayout& layout = renderer.getLineLayout(line);
			const length_t column = layout.getOffset(x - renderer.getLineIndent(line), 0);
			String s = layout.fillToX(x);
			s.append(bol, eol);
			if(line >= numberOfLines - 1)
				s.append(breakString);
			document.insert(Position(line, column), s);
		}

		if(eol == last)
			break;
		bol = eol + ((eol[0] == CARRIAGE_RETURN && eol < last - 1 && eol[1] == LINE_FEED) ? 2 : 1);
	}
}

/**
 * Returns true if the point is the beginning of the visual line.
 * @see EditPoint#isBeginningOfLine
 */
bool VisualPoint::isBeginningOfVisualLine() const {
	verifyViewer();
	if(isBeginningOfLine())
		return true;
	const LineLayout& layout = getTextViewer().getTextRenderer().getLineLayout(getLineNumber());
	return getColumnNumber() == layout.getSublineOffset(layout.getSubline(getColumnNumber()));
}

/**
 * Returns true if the point is end of the visual line.
 * @see text#EditPoint#isEndOfLine
 */
bool VisualPoint::isEndOfVisualLine() const {
	verifyViewer();
	if(isEndOfLine())
		return true;
	const LineLayout& layout = getTextViewer().getTextRenderer().getLineLayout(getLineNumber());
	const length_t subline = layout.getSubline(getColumnNumber());
	return getColumnNumber() == layout.getSublineOffset(subline) + layout.getSublineLength(subline);
}

/// Returns true if the current position is the first printable character in the line.
bool VisualPoint::isFirstPrintableCharacterOfLine() const {
	verifyViewer();
	normalize();
	const Position start = getDocument()->getStartPosition(isExcludedFromRestriction());
	const length_t offset = (start.line == getLineNumber()) ? start.column : 0;
	const String& line = getDocument()->getLine(getLineNumber());
	return line.data() + getColumnNumber() - offset
		== getIdentifierSyntax().eatWhiteSpaces(line.data() + offset, line.data() + line.length(), true);
}

/// Returns true if the current position is the first printable character in the visual line.
bool VisualPoint::isFirstPrintableCharacterOfVisualLine() const {
	// TODO: not implemented.
	return false;
}

/// Returns true if the current position is the last printable character in the line.
bool VisualPoint::isLastPrintableCharacterOfLine() const {
	verifyViewer();
	normalize();
	const Position end = getDocument()->getEndPosition(isExcludedFromRestriction());
	const String& line = getDocument()->getLine(getLineNumber());
	const length_t lineLength = (end.line == getLineNumber()) ? end.column : line.length();
	return line.data() + lineLength - getColumnNumber()
		== getIdentifierSyntax().eatWhiteSpaces(line.data() + getColumnNumber(), line.data() + lineLength, true);
}

/// Returns true if the current position is the last printable character in the visual line.
bool VisualPoint::isLastPrintableCharacterOfVisualLine() const {
	// TODO: not implemented.
	return false;
}

/// Moves to the last printable character in the line.
void VisualPoint::lastPrintableCharacterOfLine() {
	verifyViewer();
	const length_t line = min(getLineNumber(), getDocument()->getEndPosition(isExcludedFromRestriction()).line);
	const length_t lineLength = getDocument()->getLineLength(line);
	const Char* const p = getDocument()->getLine(line).data();
	const IdentifierSyntax& syntax = getIdentifierSyntax();

	for(length_t spaceLength = 0; spaceLength < lineLength; ++spaceLength) {
		if(syntax.isWhiteSpace(p[lineLength - spaceLength - 1], true)) {
			moveTo(Position(line, lineLength - spaceLength));
			return;
		}
	}
	moveTo(Position(line, lineLength));
}

/// Moves to the last printable character in the visual line.
void VisualPoint::lastPrintableCharacterOfVisualLine() {
	// TODO: not implemented.
}

/**
 * Moves to left character.
 * @param offset the offset of the movement
 */
void VisualPoint::leftCharacter(length_t offset /* = 1 */) {
	verifyViewer();
	(viewer_->getConfiguration().orientation == LEFT_TO_RIGHT) ? backwardCharacter(offset) : forwardCharacter(offset);
}

/**
 * Moves to the beginning of the left word.
 * @param offset the number of words
 */
void VisualPoint::leftWord(length_t offset /* = 1 */) {
	verifyViewer();
	(viewer_->getConfiguration().orientation == LEFT_TO_RIGHT) ? previousWord(offset) : nextWord(offset);
}

/**
 * Moves to the end of the left word.
 * @param offset the number of words
 */
void VisualPoint::leftWordEnd(length_t offset /* = 1 */) {
	verifyViewer();
	(viewer_->getConfiguration().orientation == LEFT_TO_RIGHT) ? previousWordEnd(offset) : nextWordEnd(offset);
}

/**
 * Breaks the line.
 * @note This methos hides @c EditPoint#newLine (C++ rule).
 * @param inheritIndent true to inherit the indent of the previous line
 */
void VisualPoint::newLine(bool inheritIndent) {
	verifyViewer();
	if(getDocument()->isReadOnly())
		return;

	String breakString = getNewlineString(getDocument()->getNewline());

	if(inheritIndent) {	// 自動インデント
		const String& currentLine = getDocument()->getLine(getLineNumber());
		const length_t len = getIdentifierSyntax().eatWhiteSpaces(
			currentLine.data(), currentLine.data() + getColumnNumber(), true) - currentLine.data();
		breakString += currentLine.substr(0, len);
	}
	insert(breakString);
}

/**
 * Moves to the next page.
 * @param offset the offset of the movement
 */
void VisualPoint::nextPage(length_t offset /* = 1 */) {
	verifyViewer();
	// TODO: calculate exact number of visual lines.
	nextVisualLine(viewer_->getNumberOfVisibleLines() * offset);
}

/**
 * Moves to the next visual line.
 * @param offset the offset of the movement
 */
void VisualPoint::nextVisualLine(length_t offset /* = 1 */) {
	verifyViewer();
	normalize();
	const TextRenderer& renderer = viewer_->getTextRenderer();
	const LineLayout* layout = &renderer.getLineLayout(getLineNumber());
	length_t line = getLineNumber(), subline = layout->getSubline(getColumnNumber());
	if(line == getDocument()->getNumberOfLines() - 1 && subline == layout->getNumberOfSublines() - 1)
		return;
	if(lastX_ == -1)
		updateLastX();
	renderer.offsetVisualLine(line, subline, static_cast<signed_length_t>(offset));
	layout = &renderer.getLineLayout(line);
	Position newPosition(line, layout->getOffset(lastX_ - renderer.getLineIndent(line), renderer.getLinePitch() * static_cast<long>(subline)));
	if(layout->getSubline(newPosition.column) != subline)
		newPosition = getBackwardCharacterPosition(*getDocument(), newPosition, getCharacterUnit());
	crossingLines_ = true;
	moveTo(newPosition);
	crossingLines_ = false;
}

/**
 * Moves to the beginning of the next word.
 * @param offset the number of words
 */
void VisualPoint::nextWord(length_t offset /* = 1 */) {
	verifyViewer();
	normalize();
	WordBreakIterator<DocumentCharacterIterator> i(
		DocumentCharacterIterator(*getDocument(), *this), AbstractWordBreakIterator::START_OF_SEGMENT, getIdentifierSyntax());
	i += offset;
	moveTo(i.base().tell());
}

/**
 * Moves to the end of the next word.
 * @param offset the number of words
 */
void VisualPoint::nextWordEnd(length_t offset /* = 1 */) {
	verifyViewer();
	normalize();
	WordBreakIterator<DocumentCharacterIterator> i(
		DocumentCharacterIterator(*getDocument(), *this), AbstractWordBreakIterator::END_OF_SEGMENT, getIdentifierSyntax());
	i += offset;
	moveTo(i.base().tell());
}

/**
 * Replaces the specified region by the content of the clipboard.
 * @param length the number of characters to be replaced
 */
void VisualPoint::paste(signed_length_t length /* = 0 */) {
	verifyViewer();
	if(getDocument()->isReadOnly() || length == 0) {
		paste(getPosition());
		return;
	}
	paste((length > 0) ?
		getForwardCharacterPosition(*getDocument(), *this, getCharacterUnit(), length) : getBackwardCharacterPosition(*getDocument(), *this, UTF16_CODE_UNIT, -length));
}

/**
 * Replaces the specified region by the content of the clipboard.
 * @param other もう1つの位置
 */
void VisualPoint::paste(const Position& other) {
	verifyViewer();

	if(getDocument()->isReadOnly())
		return;
	else if(const UINT availableClipFormat = canPaste()) {
		if(other != getPosition())
			erase(other);

		Clipboard clipboard(viewer_->getHandle());
		if(Clipboard::Text text = clipboard.read()) {
			const Char* const data = text.getData();
			if(availableClipFormat == ::RegisterClipboardFormatW(ASCENSION_RECTANGLE_TEXT_CLIP_FORMAT))
				insertBox(data, data + wcslen(data));
			else
				insert(data, data + wcslen(data));
		}
	}
}

/**
 * Moves to the previous page.
 * @param offset the offset of the movement
 */
void VisualPoint::previousPage(length_t offset /* = 1 */) {
	verifyViewer();
	// TODO: calculate exact number of visual lines.
	previousVisualLine(viewer_->getNumberOfVisibleLines() * offset);
}

/**
 * Moves to the previous visual line.
 * @param offset the offset of the movement
 */
void VisualPoint::previousVisualLine(length_t offset /* = 1 */) {
	verifyViewer();
	normalize();
	const TextRenderer& renderer = viewer_->getTextRenderer();
	length_t line = getLineNumber(), subline = renderer.getLineLayout(line).getSubline(getColumnNumber());
	if(line == 0 && subline == 0)
		return;
	if(lastX_ == -1)
		updateLastX();
	renderer.offsetVisualLine(line, subline, -static_cast<signed_length_t>(offset));
	const LineLayout& layout = renderer.getLineLayout(line);
	Position newPosition(line, layout.getOffset(lastX_ - renderer.getLineIndent(line), renderer.getLinePitch() * static_cast<long>(subline)));
	if(layout.getSubline(newPosition.column) != subline)
		newPosition = getBackwardCharacterPosition(*getDocument(), newPosition, getCharacterUnit());
	crossingLines_ = true;
	moveTo(newPosition);
	crossingLines_ = false;
}

/**
 * Moves to the beginning of the previous word.
 * @param offset the number of words
 */
void VisualPoint::previousWord(length_t offset /* = 1 */) {
	verifyViewer();
	normalize();
	WordBreakIterator<DocumentCharacterIterator> i(
		DocumentCharacterIterator(*getDocument(), *this), AbstractWordBreakIterator::START_OF_SEGMENT, getIdentifierSyntax());
	i -= offset;
	moveTo(i.base().tell());
}

/**
 * Moves to the end of the previous word.
 * @param offset the number of words
 */
void VisualPoint::previousWordEnd(length_t offset /* = 1 */) {
	verifyViewer();
	normalize();
	WordBreakIterator<DocumentCharacterIterator> i(
		DocumentCharacterIterator(*getDocument(), *this), AbstractWordBreakIterator::END_OF_SEGMENT, getIdentifierSyntax());
	i -= offset;
	moveTo(i.base().tell());
}

/**
 * 指定範囲がビューの中央になるようにスクロールする。ただし既に可視なら何もしない
 * @param length 範囲を構成するもう一方の点までの文字数
 * @return 範囲がビューに納まる場合は true を返す
 */
bool VisualPoint::recenter(signed_length_t length /* = 0 */) {
	verifyViewer();
	return recenter((length >= 0) ?
		getForwardCharacterPosition(*getDocument(), *this, getCharacterUnit(), length)
		: getBackwardCharacterPosition(*getDocument(), *this, getCharacterUnit(), -length));
}

/**
 * 指定範囲がビューの中央になるようにスクロールする。ただし既に可視なら何もしない
 * @param other 範囲を構成するもう一方の点
 * @return 範囲がビューに納まる場合は true を返す (未実装につき常に true)
 */
bool VisualPoint::recenter(const Position& other) {
	verifyViewer();
	// TODO: not implemented.
	return true;
}

/**
 * Moves to the right character.
 * @param offset the offset of the movement
 */
void VisualPoint::rightCharacter(length_t offset /* = 1 */) {
	verifyViewer();
	(viewer_->getConfiguration().orientation == LEFT_TO_RIGHT) ? forwardCharacter(offset) : backwardCharacter(offset);
}

/**
 * Moves to the beginning of the right word.
 * @param offset the number of words
 */
void VisualPoint::rightWord(length_t offset /* = 1 */) {
	verifyViewer();
	(viewer_->getConfiguration().orientation == LEFT_TO_RIGHT) ? nextWord(offset) : previousWord(offset);
}

/**
 * Moves to the end of the right word.
 * @param offset the number of words
 */
void VisualPoint::rightWordEnd(length_t offset /* = 1 */) {
	verifyViewer();
	(viewer_->getConfiguration().orientation == LEFT_TO_RIGHT) ? nextWordEnd(offset) : previousWordEnd(offset);
}

/**
 * 指定範囲が可視になるようにビューをスクロールする
 * @param length 範囲を構成するもう一方の点までの文字数
 * @return 範囲がビューに納まる場合は true を返す (未実装につき常に true)
 */
bool VisualPoint::show(signed_length_t length /* = 0 */) {
	verifyDocument();
	return show((length >= 0) ?
		getForwardCharacterPosition(*getDocument(), *this, getCharacterUnit(), length)
		: getBackwardCharacterPosition(*getDocument(), *this, getCharacterUnit(), -length));
}

/**
 * 点が可視になるようにビューをスクロールする
 * @param other 範囲を構成するもう一方の点
 * @return 範囲がビューに納まる場合は true を返す (未実装につき常に true)
 */
bool VisualPoint::show(const Position& other) {
	verifyViewer();

	const TextRenderer& renderer = viewer_->getTextRenderer();
	const length_t visibleLines = viewer_->getNumberOfVisibleLines();
	AutoZeroCB<::SCROLLINFO> si;
	::POINT to = {-1, -1};

	// for vertical direction
	if(visualLine_ == INVALID_INDEX) {
		visualLine_ = viewer_->getTextRenderer().mapLogicalLineToVisualLine(getLineNumber());
		visualSubline_ = renderer.getLineLayout(getLineNumber()).getSubline(getColumnNumber());
		visualLine_ += visualSubline_;
	}
	si.fMask = SIF_POS;
	viewer_->getScrollInformation(SB_VERT, si);
	if(visualLine_ < si.nPos * viewer_->getScrollRate(false))	// 画面より上
		to.y = static_cast<long>(visualLine_ * viewer_->getScrollRate(false));
	else if(visualLine_ - si.nPos * viewer_->getScrollRate(false) > visibleLines - 1)	// 画面より下
		to.y = static_cast<long>((visualLine_ - visibleLines + 1) * viewer_->getScrollRate(false));
	if(to.y < -1)
		to.y = 0;

	// for horizontal direction
	if(!viewer_->getConfiguration().lineWrap.wrapsAtWindowEdge()) {
		const length_t visibleColumns = viewer_->getNumberOfVisibleColumns();
		const ulong x = renderer.getLineLayout(getLineNumber()).getLocation(getColumnNumber(), LineLayout::LEADING).x + renderer.getLineIndent(getLineNumber(), 0);
		viewer_->getScrollInformation(SB_HORZ, si);
		const ulong scrollOffset = si.nPos * viewer_->getScrollRate(true) * renderer.getAverageCharacterWidth();
		if(x <= scrollOffset)	// 画面より左
			to.x = x / renderer.getAverageCharacterWidth() - visibleColumns / 4;
		else if(x >= (si.nPos * viewer_->getScrollRate(true) + visibleColumns) * renderer.getAverageCharacterWidth())	// 画面より右
			to.x = x / renderer.getAverageCharacterWidth() - visibleColumns * 3 / 4;
		if(to.x < -1)
			to.x = 0;
	}
	if(to.x >= -1 || to.y != -1)
		viewer_->scrollTo(to.x, to.y, true);

	return true;
}

/**
 * Indents the specified region by using horizontal tabs.
 * @param other もう1つの位置
 * @param box 矩形インデントか (インデントレベルが負であれば無視される)
 * @param level インデントレベル
 * @return インデント後に @a pos が移動すべき位置
 */
Position VisualPoint::spaceIndent(const Position& other, bool box, long level /* = 1 */) {
	verifyViewer();
	return doIndent(other, L' ', box, level);
}

/**
 * Indents the specified region by using horizontal tabs.
 * @param other もう1つの位置
 * @param box 矩形インデントか (インデントレベルが負であれば無視される)
 * @param level インデントレベル
 * @return インデント後に @a pos が移動すべき位置
 */
Position VisualPoint::tabIndent(const Position& other, bool box, long level /* = 1 */) {
	verifyViewer();
	return doIndent(other, L'\t', box, level);
}

/**
 * Transposes the two grapheme clusters on either side of the point.
 * If the point is not start of a cluster, this method fails.
 * If the transposing target is not in the current line, this method fails.
 * @return false if failed
 */
bool VisualPoint::transposeCharacters() {
	verifyViewer();

#define IS_RESTRICTION(position) (position < top || position > bottom)

	if(getDocument()->isReadOnly())
		return false;

	// As transposing characters in string "ab":
	//
	//  a b -- transposing clusters 'a' and 'b'. result is "ba"
	// ^ ^ ^
	// | | next-cluster (named pos[2])
	// | middle-cluster (named pos[1]; usually current-position)
	// previous-cluster (named pos[0])

	Position pos[3];
	const Position top = getDocument()->getStartPosition();
	const Position bottom = getDocument()->getEndPosition();

	if(BinaryProperty::is<BinaryProperty::GRAPHEME_EXTEND>(getCodePoint()))	// not the start of a grapheme
		return false;
	else if(IS_RESTRICTION(getPosition()))	// inaccessible
		return false;

	if(getColumnNumber() == 0 || getPosition() == top) {
		GraphemeBreakIterator<DocumentCharacterIterator> i(DocumentCharacterIterator(*getDocument(), pos[0] = getPosition()));
		pos[1] = (++i).base().tell();
		if(pos[1].line != pos[0].line || pos[1] == pos[0] || IS_RESTRICTION(pos[1]))
			return false;
		pos[2] = (++i).base().tell();
		if(pos[2].line != pos[1].line || pos[2] == pos[1] || IS_RESTRICTION(pos[2]))
			return false;
	} else if(getColumnNumber() == getDocument()->getLineLength(getLineNumber()) || getPosition() == bottom) {
		GraphemeBreakIterator<DocumentCharacterIterator> i(DocumentCharacterIterator(*getDocument(), pos[2] = getPosition()));
		pos[1] = (--i).base().tell();
		if(pos[1].line != pos[2].line || pos[1] == pos[2] || IS_RESTRICTION(pos[1]))
			return false;
		pos[0] = (--i).base().tell();
		if(pos[0].line != pos[1].line || pos[0] == pos[1] || IS_RESTRICTION(pos[0]))
			return false;
	} else {
		GraphemeBreakIterator<DocumentCharacterIterator> i(DocumentCharacterIterator(*getDocument(), pos[1] = getPosition()));
		pos[2] = (++i).base().tell();
		if(pos[2].line != pos[1].line || pos[2] == pos[1] || IS_RESTRICTION(pos[2]))
			return false;
		i.base().seek(pos[1]);
		pos[0] = (--i).base().tell();
		if(pos[0].line != pos[1].line || pos[0] == pos[1] || IS_RESTRICTION(pos[0]))
			return false;
	}

	moveTo(getLineNumber(), pos[1].column);
	String s = getText(pos[2]);
	moveTo(getLineNumber(), pos[0].column);
	s += getText(pos[1]);
	erase(pos[2]);
	insert(s);

	return true;

#undef IS_RESTRICTION
}

/**
 * Transposes the current line and the previous line.
 * If the current line is the first line, transposes with the next line.
 * The line breaks will not be exchanged.
 * @return false if failed
 */
bool VisualPoint::transposeLines() {
	verifyViewer();

	if(getDocument()->isReadOnly())
		return false;

	const Position top = getDocument()->getStartPosition();
	const Position bottom = getDocument()->getEndPosition();

	if(top.line == bottom.line)	// there is just one line
		return false;

	if(getLineNumber() == top.line)
		moveTo(getLineNumber() + 1, getColumnNumber());

	const String str1 = (getLineNumber() - 1 == top.line) ?
		getDocument()->getLine(getLineNumber() - 1).substr(top.column) : getDocument()->getLine(getLineNumber() - 1);
	const String str2 = (getLineNumber() == bottom.line) ?
		getDocument()->getLine(getLineNumber()).substr(0, bottom.column) : getDocument()->getLine(getLineNumber());

	// make the two lines empty
	if(!str2.empty()) {
		beginningOfLine();
		erase(static_cast<signed_length_t>(str2.length()), UTF16_CODE_UNIT);
	}
	if(!str1.empty()) {
		moveTo(getLineNumber() - 1, (getLineNumber() == top.line) ? top.column : 0);
		erase(static_cast<signed_length_t>(str1.length()), UTF16_CODE_UNIT);
		moveTo(getLineNumber() + 1, getColumnNumber());
	}

	// insert into the two lines
	if(!str1.empty()) {
		beginningOfLine();
		insert(str1);
	}
	moveTo(getLineNumber() - 1, getColumnNumber());
	if(!str2.empty()) {
		moveTo(getLineNumber(), (getLineNumber() == top.line) ? top.column : 0);
		insert(str2);
	}
	moveTo(Position(getLineNumber() + 2, 0));

	return true;
}

/**
 * Transposes the two words on either side of the point.
 * @return false if failed
 */
bool VisualPoint::transposeWords() {
	verifyViewer();

	if(getDocument()->isReadOnly())
		return false;

	// As transposing words in string "(\w+)[^\w*](\w+)":
	//
	//  abc += xyz -- transposing words "abc" and "xyz". result is "xyz+=abc"
	// ^   ^  ^   ^
	// |   |  |   2nd-word-end (named pos[3])
	// |   |  2nd-word-start (named pos[2])
	// |   1st-word-end (named pos[1])
	// 1st-word-start (named pos[0])

	const Position top = getDocument()->getStartPosition();
	const Position bottom = getDocument()->getEndPosition();
	WordBreakIterator<DocumentCharacterIterator> i(
		DocumentCharacterIterator(*getDocument(), *this), AbstractWordBreakIterator::START_OF_ALPHANUMERICS, getIdentifierSyntax());
	Position pos[4];

	// find the backward word (1st-word-*)...
	pos[0] = (--i).base().tell();
	i.setComponent(AbstractWordBreakIterator::END_OF_ALPHANUMERICS);
	pos[1] = (++i).base().tell();
	if(pos[1] == pos[0])	// the word is empty
		return false;

	// ...and then backward one (2nd-word-*)
	i.base().seek(*this);
	i.setComponent(AbstractWordBreakIterator::START_OF_ALPHANUMERICS);
	pos[2] = (++i).base().tell();
	if(pos[2] == getPosition())
		return false;
	pos[3] = (++i).base().tell();
	if(pos[2] == pos[3])	// the word is empty
		return false;

	// replace
	moveTo(pos[2]);
	String str = getText(pos[3]);
	moveTo(pos[1]);
	str += getText(pos[2]);
	moveTo(pos[0]);
	str += getText(pos[1]);
	erase(pos[3]);
	insert(str);

	return true;
}

/// Updates @c lastX_ with the current position.
inline void VisualPoint::updateLastX() {
	assert(!crossingLines_);
	verifyViewer();
	if(!isDocumentDisposed()) {
		const LineLayout& layout = getTextViewer().getTextRenderer().getLineLayout(getLineNumber());
		lastX_ = layout.getLocation(getColumnNumber(), LineLayout::LEADING).x;
		lastX_ += getTextViewer().getTextRenderer().getLineIndent(getLineNumber(), 0);
	}
}

/// @see IVisualLinesListener#visualLinesDeleted
void VisualPoint::visualLinesDeleted(length_t first, length_t last, length_t, bool) throw() {
	if(!adaptsToDocument() && getLineNumber() >= first && getLineNumber() < last)
		visualLine_ = INVALID_INDEX;
}

/// @see IVisualLinesListener#visualLinesInserted
void VisualPoint::visualLinesInserted(length_t first, length_t last) throw() {
	if(!adaptsToDocument() && getLineNumber() >= first && getLineNumber() < last)
		visualLine_ = INVALID_INDEX;
}

/// @see IVisualLinesListener#visualLinesModified
void VisualPoint::visualLinesModified(length_t first, length_t last, signed_length_t sublineDifference, bool, bool) throw() {
	if(visualLine_ != INVALID_INDEX) {
		// adjust visualLine_ and visualSubine_ according to the visual lines modification
		if(last <= getLineNumber())
			visualLine_ += sublineDifference;
		else if(first == getLineNumber()) {
			visualLine_ -= visualSubline_;
			visualSubline_ = getTextViewer().getTextRenderer().getLineLayout(
				getLineNumber()).getSubline(min(getColumnNumber(), getDocument()->getLineLength(getLineNumber())));
			visualLine_ += visualSubline_;
		} else if(first < getLineNumber())
			visualLine_ = INVALID_INDEX;
	}
}

// Caret ////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param viewer the viewer
 * @param position the initial position of the point
 * @throw std#invalid_argument @a position is outside of the document
 */
Caret::Caret(TextViewer& viewer, const Position& position /* = Position() */) throw() : VisualPoint(viewer, position, 0),
		anchor_(new SelectionAnchor(viewer)), selectionMode_(CHARACTER), pastingFromClipboardRing_(false),
		leaveAnchorNext_(false), leadingAnchor_(false), autoShow_(true), box_(0), matchBracketsTrackingMode_(DONT_TRACK),
		overtypeMode_(false), editingByThis_(false), othersEditedFromLastInputChar_(false),
		regionBeforeMoved_(Position::INVALID_POSITION, Position::INVALID_POSITION),
		matchBrackets_(make_pair(Position::INVALID_POSITION, Position::INVALID_POSITION)) {
	getDocument()->addListener(*this);
	excludeFromRestriction(true);
	anchor_->excludeFromRestriction(true);
}

/// Destructor.
Caret::~Caret() throw() {
	if(Document* document = getDocument())
		document->removeListener(*this);
	delete anchor_;
	delete box_;
}

/**
 * Starts rectangular selection.
 * @see #endBoxSelection, #isSelectionRectangle
 */
void Caret::beginBoxSelection() {
	verifyViewer();
	if(box_ == 0) {
		box_ = new VirtualBox(getTextViewer(), getSelectionRegion());
		stateListeners_.notify<const Caret&>(ICaretStateListener::selectionShapeChanged, *this);
	}
}

/**
 * Starts line selection mode.
 * The rectangular selection will be revoked automatically.
 * @see #beginWordSelection, #restoreSelectionMode
 */
void Caret::beginLineSelection() {
	verifyViewer();
	endBoxSelection();
	pastingFromClipboardRing_ = false;
	if(selectionMode_ == LINE)
		return;
	selectionMode_ = LINE;
	extendSelection(Position(modeInitialAnchorLine_ = anchor_->getLineNumber(), 0));
}

/**
 * Starts word selection mode.
 * The rectangular selection will be revoked automatically.
 * @see #beginLineSelection, #restoreSelectionMode
 */
void Caret::beginWordSelection() {
	verifyViewer();
	endBoxSelection();
	pastingFromClipboardRing_ = false;
	if(selectionMode_ == WORD)
		return;
	selectWord();
	selectionMode_ = WORD;
	modeInitialAnchorLine_ = getLineNumber();
	wordSelectionChars_[0] = anchor_->getColumnNumber();
	wordSelectionChars_[1] = getColumnNumber();
}

/// 対括弧の追跡を更新する
void Caret::checkMatchBrackets() {
	bool matched;
	pair<Position, Position> oldPair(matchBrackets_);
	// TODO: implement matching brackets checking
/*	if(!isSelectionEmpty() || matchBracketsTrackingMode_ == DONT_TRACK)
		matched = false;
	else if(matched = getViewer().searchMatchBracket(getPosition(), matchBrackets_.first, true))
		matchBrackets_.second = getPosition();
	else if(matchBracketsTrackingMode_ == TRACK_FOR_SURROUND_CHARACTERS && !isStartOfLine()) {	// 1文字前も調べる
		const String& line = getDocument()->getLine(getLineNumber());
		GraphemeBreakIterator i(line.data(), line.data() + line.length(), line.data() + getColumnNumber());
		if(matched = getViewer().searchMatchBracket(Position(getLineNumber(), (--i).tell() - line.data()), matchBrackets_.first, true))
			matchBrackets_.second = Position(getLineNumber(), i.tell() - line.data());
	}
	if(!matched)
		matchBrackets_.first = matchBrackets_.second = Position::INVALID_POSITION;
*/	// TODO: check if the pair is out of view.
	if(matchBrackets_ != oldPair)
		stateListeners_.notify<const Caret&, const pair<Position,
			Position>&, bool>(ICaretStateListener::matchBracketsChanged, *this, oldPair, false);
}

/// Clears the selection.
void Caret::clearSelection() {
	endBoxSelection();
	restoreSelectionMode();
	leaveAnchorNext_ = false;
	moveTo(*this);
}

/**
 * Copies the selected text to the clipboard.
 * @param alsoSendToClipboardRing true to send also the clipboard ring
 */
void Caret::copySelection(bool alsoSendToClipboardRing) {
	verifyViewer();
	if(isSelectionEmpty())
		return;
	const String s = getSelectionText(NLR_PHYSICAL_DATA);
	Clipboard(getTextViewer().getHandle()).write(s, isSelectionRectangle());
	if(alsoSendToClipboardRing) {	// クリップボードリングにも転送
		if(texteditor::Session* session = getDocument()->getSession())
			session->getClipboardRing().add(s, isSelectionRectangle());
	}
}

/**
 * Copies and deletes the selected text.
 * @param alsoSendToClipboardRing true to send also the clipboard ring
 */
void Caret::cutSelection(bool alsoSendToClipboardRing) {
	verifyViewer();
	if(isSelectionEmpty() || getDocument()->isReadOnly())
		return;
	copySelection(alsoSendToClipboardRing);
	getTextViewer().freeze(true);
	getDocument()->beginSequentialEdit();
	eraseSelection();
	getDocument()->endSequentialEdit();
	getTextViewer().unfreeze(true);
}

/// @see text#IDocumentListener#documentAboutToBeChanged
void Caret::documentAboutToBeChanged(const Document&) {
	// do nothing
}

/// @see text#IDocumentListener#documentChanged
void Caret::documentChanged(const Document&, const DocumentChange&) {
	if(regionBeforeMoved_.first != Position::INVALID_POSITION)
		updateVisualAttributes();
}

/// @see VisualPoint#doMoveTo
void Caret::doMoveTo(const Position& to) {
	regionBeforeMoved_ = Region(anchor_->isInternalUpdating() ?
		anchor_->getPositionBeforeInternalUpdate() : anchor_->getPosition(), getPosition());
	restoreSelectionMode();
	if(!editingByThis_)
		othersEditedFromLastInputChar_ = true;
	if(leaveAnchorNext_)
		leaveAnchorNext_ = false;
	else {
		leadingAnchor_ = true;
		anchor_->moveTo(to);
		leadingAnchor_ = false;
	}
	VisualPoint::doMoveTo(to);
	if(!getDocument()->isChanging())
		updateVisualAttributes();
}

/**
 * Ends the rectangular selection.
 * @see #beginBoxSelection, #isSelectionRectangle
 */
void Caret::endBoxSelection() {
	verifyViewer();
	if(box_ != 0) {
		delete box_;
		box_ = 0;
		stateListeners_.notify<const Caret&>(ICaretStateListener::selectionShapeChanged, *this);
	}
}

/**
 * Deletes the selected text.
 * This method does not freeze the viewer and not begin the sequential edit.
 */
void Caret::eraseSelection() {
	verifyViewer();
	Document& document = *getDocument();
	if(document.isReadOnly() || isSelectionEmpty())
		return;
	else if(!isSelectionRectangle())	// the selection is linear
		moveTo(document.erase(*anchor_, *this));
	else {	// the selection is rectangle
		const Position resultPosition = getTopPoint();
		const bool adapts = adaptsToDocument();
		adaptToDocument(false);
		const length_t firstLine = getTopPoint().getLineNumber(), lastLine = getBottomPoint().getLineNumber();
		pair<length_t, length_t> rangeInLine;;

		if(getTextViewer().getConfiguration().lineWrap.wraps()) {	// ...and the lines are wrapped
			// hmmm..., this is heavy work
			vector<Point*> points;
			vector<length_t> sizes;
			points.reserve((lastLine - firstLine) * 2);
			sizes.reserve((lastLine - firstLine) * 2);
			const TextRenderer& renderer = getTextViewer().getTextRenderer();
			for(length_t line = resultPosition.line; line <= lastLine; ++line) {
				const LineLayout& layout = renderer.getLineLayout(line);
				for(length_t subline = 0; subline < layout.getNumberOfSublines(); ++subline) {
					box_->getOverlappedSubline(line, subline, rangeInLine.first, rangeInLine.second);
					points.push_back(new Point(document, Position(line, rangeInLine.first)));
					sizes.push_back(rangeInLine.second - rangeInLine.first);
				}
			}
			const size_t sublines = points.size();
			for(size_t i = 0; i < sublines; ++i) {
				document.erase(Position(points[i]->getLineNumber(), points[i]->getColumnNumber()),
					Position(points[i]->getLineNumber(), points[i]->getColumnNumber() + sizes[i]));
				delete points[i];
			}
		} else {
			for(length_t line = resultPosition.line; line <= lastLine; ++line) {
				box_->getOverlappedSubline(line, 0, rangeInLine.first, rangeInLine.second);
				document.erase(Position(line, rangeInLine.first), Position(line, rangeInLine.second));
			}
		}

		endBoxSelection();
		adaptToDocument(adapts);
		moveTo(resultPosition);
	}
}

/**
 * Moves to the specified position without the anchor adapting.
 * @param to the destination position
 */
void Caret::extendSelection(const Position& to) {
	verifyViewer();
	if(selectionMode_ == CHARACTER) {
		leaveAnchorNext_ = true;
		moveTo(to);
		leaveAnchorNext_ = false;
	} else if(selectionMode_ == LINE) {
		const length_t lc = getDocument()->getNumberOfLines();
		Region s;
		s.first.line = (to.line >= modeInitialAnchorLine_) ? modeInitialAnchorLine_ : modeInitialAnchorLine_ + 1;
		s.first.column = (s.first.line > lc - 1) ? getDocument()->getLineLength(--s.first.line) : 0;
		s.second.line = (to.line >= modeInitialAnchorLine_) ? to.line + 1 : to.line;
		s.second.column = (s.second.line > lc - 1) ? getDocument()->getLineLength(--s.second.line) : 0;
		select(s);
		selectionMode_ = LINE;
	} else if(selectionMode_ == WORD) {
		if(to.line < modeInitialAnchorLine_ || (to.line == modeInitialAnchorLine_ && to.column < wordSelectionChars_[0])) {
			WordBreakIterator<DocumentCharacterIterator> i(
				DocumentCharacterIterator(*getDocument(), to), AbstractWordBreakIterator::BOUNDARY_OF_SEGMENT, getIdentifierSyntax());
			--i;
			select(Position(modeInitialAnchorLine_, wordSelectionChars_[1]),
				(i.base().tell().line == to.line) ? i.base().tell() : Position(to.line, 0));
		} else if(to.line > modeInitialAnchorLine_ || (to.line == modeInitialAnchorLine_ && to.column > wordSelectionChars_[1])) {
			WordBreakIterator<DocumentCharacterIterator> i(
				DocumentCharacterIterator(*getDocument(), to), AbstractWordBreakIterator::BOUNDARY_OF_SEGMENT, getIdentifierSyntax());
			++i;
			select(Position(modeInitialAnchorLine_, wordSelectionChars_[0]),
				(i.base().tell().line == to.line) ? i.base().tell() : Position(to.line, getDocument()->getLineLength(to.line)));
		} else
			select(Position(modeInitialAnchorLine_, wordSelectionChars_[0]), Position(modeInitialAnchorLine_, wordSelectionChars_[1]));
		selectionMode_ = WORD;
	}
}

/**
 * Moves the caret without the anchor movement.
 * @param algorithm the movement algorithm
 */
void Caret::extendSelection(mem_fun_t<void, EditPoint>& algorithm) {
	verifyViewer();
	if(selectionMode_ == CHARACTER) {
		leaveAnchorNext_ = true;
		algorithm(this);
		leaveAnchorNext_ = false;
	} else {
		EditPoint temp(*this);
		algorithm(&temp);
		extendSelection(temp);
	}
}

/**
 * Moves the caret without the anchor movement.
 * @param algorithm the movement algorithm
 */
void Caret::extendSelection(mem_fun_t<void, VisualPoint>& algorithm) {
	verifyViewer();
	if(selectionMode_ == CHARACTER) {
		leaveAnchorNext_ = true;
		algorithm(this);
		leaveAnchorNext_ = false;
	} else {
		VisualPoint temp(*this);
		algorithm(&temp);
		extendSelection(temp);
	}
}

/**
 * Moves the caret without the anchor movement.
 * @param algorithm the movement algorithm
 * @param offset the number to apply the algorithm
 */
void Caret::extendSelection(mem_fun1_t<void, EditPoint, length_t>& algorithm, length_t offset) {
	verifyViewer();
	if(selectionMode_ == CHARACTER) {
		leaveAnchorNext_ = true;
		algorithm(this, offset);
		leaveAnchorNext_ = false;
	} else {
		EditPoint temp(*this);
		algorithm(&temp, offset);
		extendSelection(temp);
	}
}

/**
 * Moves the caret without the anchor movement.
 * @param algorithm the movement algorithm
 * @param offset the number to apply the algorithm
 */
void Caret::extendSelection(mem_fun1_t<void, VisualPoint, length_t>& algorithm, length_t offset) {
	verifyViewer();
	if(selectionMode_ == CHARACTER) {
		leaveAnchorNext_ = true;
		algorithm(this, offset);
		leaveAnchorNext_ = false;
	} else {
		VisualPoint temp(*this);
		algorithm(&temp, offset);
		extendSelection(temp);
	}
}

/**
 * Returns the selected range on the specified logical line.
 * This method returns a logical range, and does not support rectangular selection.
 * @param line the logical line
 * @param[out] first the start of the range
 * @param[out] last the end of the range. this can include the end of the line
 * @return true if there is selected range on the line
 * @throw text#BadPositionException @a line is outside of the document
 * @see #getSelectedRangeOnVisualLine
 */
bool Caret::getSelectedRangeOnLine(length_t line, length_t& first, length_t& last) const {
	verifyViewer();
	const Position top = getTopPoint();
	if(top.line > line)
		return false;
	const Position bottom = getBottomPoint();
	if(bottom.line < line)
		return false;
	first = (line == top.line) ? top.column : 0;
	last = (line == bottom.line) ? bottom.column : getDocument()->getLineLength(line) + 1;
	return true;
}

/**
 * Returns the selected range on the specified visual line,
 * @param line the logical line
 * @param subline the visual subline
 * @param[out] first the start of the range
 * @param[out] last the end of the range. this can include the logical end of the line
 * @return true if there is selected range on the line
 * @throw text#BadPositionException @a line or @a subline is outside of the document
 * @see #getSelectedRangeOnLine
 */
bool Caret::getSelectedRangeOnVisualLine(length_t line, length_t subline, length_t& first, length_t& last) const {
	verifyViewer();
	if(!isSelectionRectangle()) {
		if(!getSelectedRangeOnLine(line, first, last))
			return false;
		const LineLayout& layout = getTextViewer().getTextRenderer().getLineLayout(line);
		const length_t sublineOffset = layout.getSublineOffset(subline);
		first = max(first, sublineOffset);
		last = min(last, sublineOffset + layout.getSublineLength(subline) + ((subline < layout.getNumberOfSublines() - 1) ? 0 : 1));
		return first != last;
	} else
		return box_->getOverlappedSubline(line, subline, first, last);
}

/**
 * Returns the selected text.
 * @param nlr the newline representation for multiline selection. if the selection is rectangular,
 * this value is ignored and the document's newline is used instead
 * @return the text
 */
String Caret::getSelectionText(NewlineRepresentation nlr /* = NLR_PHYSICAL_DATA */) const {
	verifyViewer();

	if(isSelectionEmpty())
		return L"";
	else if(!isSelectionRectangle())
		return getText(*anchor_, nlr);

	// rectangular selection
	StringBuffer s(ios_base::out);
	const length_t bottomLine = getBottomPoint().getLineNumber();
	length_t first, last;
	for(length_t line = getTopPoint().getLineNumber(); line <= bottomLine; ++line) {
		const Document::Line& ln = getDocument()->getLineInfo(line);
		box_->getOverlappedSubline(line, 0, first, last);	// TODO: recognize wrap (second parameter).
		s.sputn(ln.getLine().data() + first, static_cast<streamsize>(last - first));
		s.sputn(getNewlineString(ln.getNewline()), static_cast<streamsize>(getNewlineStringLength(ln.getNewline())));
	}
	return s.str();
}

/**
 * Inputs the specified character at current position.
 * If the selection is not empty, replaces the selected region.
 * Otherwise if in overtype mode, replaces a character at current position.
 * @param cp the code point of the character to be input
 * @param validateSequence true to perform input sequence check using the active ISC
 * @param blockControls true to refuse any ASCII control characters except HT (U+0009), RS (U+001E) and US (U+001F)
 * @return false if the input was refused
 * @see #isOvertypeMode, #setOvertypeMode, texteditor#commands#TextInputCommand
 */
bool Caret::inputCharacter(CodePoint cp, bool validateSequence /* = true */, bool blockControls /* = true */) {
	verifyViewer();

	Document& document = *getDocument();
	if(document.isReadOnly())
		return false;
	else if(blockControls && cp <= 0xFF && cp != 0x09 && cp != 0x1E && cp != 0x1F && toBoolean(iscntrl(static_cast<int>(cp))))
		return false;

	// check the input sequence
	if(validateSequence) {
		if(const texteditor::Session* session = document.getSession()) {
			if(const texteditor::InputSequenceCheckers* checker = session->getInputSequenceCheckers()) {
				const Char* const line = document.getLine(getTopPoint().getLineNumber()).data();
				if(!checker->check(line, line + getTopPoint().getColumnNumber(), cp)) {
					eraseSelection();
					return false;
				}
			}
		}
	}

	Char buffer[2];
	surrogates::encode(cp, buffer);
	if(!isSelectionEmpty())	// just replace if the selection is not empty
		replaceSelection(buffer, buffer + ((cp < 0x10000) ? 1 : 2));
	else if(overtypeMode_) {
		if(!document.isSequentialEditing())
			document.beginSequentialEdit();
		getTextViewer().freeze(true);
		destructiveInsert(buffer, buffer + ((cp < 0x10000) ? 1 : 2));
		getTextViewer().unfreeze(true);
	} else {
		const IdentifierSyntax& ctypes = getIdentifierSyntax();
		const bool alpha = ctypes.isIdentifierContinueCharacter(cp);

//		// exit the completion mode if the character is not ID_Start or ID_Continue
//		if(!alpha && completionWindow_.isRunning())
//			completionWindow_.complete();

		// prepare a packing the following multiple inputs
		if(othersEditedFromLastInputChar_ || !alpha)
			document.endSequentialEdit();
		if(alpha && !document.isSequentialEditing()) {
			document.beginSequentialEdit();
			othersEditedFromLastInputChar_ = false;
		}

		editingByThis_ = true;
		insert(buffer, buffer + ((cp < 0x10000) ? 1 : 2));
		editingByThis_ = false;
	}
	characterInputListeners_.notify<const Caret&, CodePoint>(ICharacterInputListener::characterInputted, *this, cp);

	return true;
}

/**
 * Returns true if the specified point is over the selection.
 * @param pt the client coordinates of the point
 * @return true if the point is over the selection
 */
bool Caret::isPointOverSelection(const ::POINT& pt) const {
	verifyViewer();
	if(isSelectionEmpty())
		return false;
	else if(isSelectionRectangle())
		return box_->isPointOver(pt);
	else {
		if(getTextViewer().hitTest(pt) != TextViewer::TEXT_AREA)	// ignore if on the margin
			return false;
		::RECT rect;
		getTextViewer().getClientRect(rect);
		if(pt.x > rect.right || pt.y > rect.bottom)
			return false;
		const Position pos = getTextViewer().getCharacterForClientXY(pt, true);
		return pos >= getTopPoint() && pos <= getBottomPoint();
	}
}

/**
 * Replaces the selected text by the content of the clipboard.
 * @param fromClipboardRing true to use the clipboard ring
 */
void Caret::pasteToSelection(bool fromClipboardRing) {
	verifyViewer();
	if(getDocument()->isReadOnly())
		return;
	texteditor::Session* session = getDocument()->getSession();
	if(fromClipboardRing && (session == 0 || session->getClipboardRing().getCount() == 0))
		return;

	const Position anchorOrg = anchor_->getPosition();
	getDocument()->beginSequentialEdit();
	getTextViewer().freeze(true);
	if(!fromClipboardRing) {
		if(!isSelectionEmpty()) {
			eraseSelection();
			moveTo(anchorOrg);
		}
		paste();
	} else {
		String str;
		bool box;
		size_t activeItem = session->getClipboardRing().getActiveItem();

		if(pastingFromClipboardRing_ && ++activeItem == session->getClipboardRing().getCount())
			activeItem = 0;
		session->getClipboardRing().getText(activeItem, str, box);
		session->getClipboardRing().setActiveItem(activeItem);
		if(!isSelectionEmpty()) {
			if(pastingFromClipboardRing_)
				getDocument()->undo();
			eraseSelection();
			moveTo(anchorOrg);
		}
		if(!box) {
			insert(str);
			endBoxSelection();
		} else {
			insertBox(str);
			beginBoxSelection();
		}
		select(anchorOrg, getPosition());
		pastingFromClipboardRing_ = true;
	}
	getDocument()->endSequentialEdit();
	getTextViewer().unfreeze(true);
}

/// @see IPointListener#pointDestroyed
void Caret::pointDestroyed() {
}

/// @see IPointListener#pointMoved
void Caret::pointMoved(const EditPoint& self, const Position& oldPosition) {
	assert(&self == &*anchor_);
	pastingFromClipboardRing_ = false;
	if(leadingAnchor_)	// doMoveTo で anchor_->moveTo 呼び出し中
		return;
	if((oldPosition == getPosition()) != isSelectionEmpty())
		checkMatchBrackets();
	listeners_.notify<const Caret&, const Region&>(ICaretListener::caretMoved, *this, Region(oldPosition, getPosition()));
}

/**
 * Replaces the selected region with the specified text.
 * If the selection is empty, inserts the text at current position.
 * @param first the start of the text
 * @param last the end of the text
 * @param rectangleInsertion true to insert text as rectangle
 */
void Caret::replaceSelection(const Char* first, const Char* last, bool rectangleInsertion /* = false */) {
	verifyViewer();
	if(getDocument()->isReadOnly())
		return;
	const Region oldRegion = getSelectionRegion();
	getDocument()->beginSequentialEdit();
	getTextViewer().freeze(true);
	if(!isSelectionEmpty())
		eraseSelection();
	else if(isSelectionRectangle())
		endBoxSelection();
	if(rectangleInsertion)
		insertBox(first, last);
	else
		insert(first, last);
	getTextViewer().unfreeze(true);
	getDocument()->endSequentialEdit();
}

/**
 * Revokes the current selection mode.
 * @see #beginLineSelection, #beginWordSelection
 */
void Caret::restoreSelectionMode() {
	verifyViewer();
	pastingFromClipboardRing_ = false;
	selectionMode_ = CHARACTER;
}

/**
 * Selects the specified region. The active selection mode will be cleared.
 * @param anchor the position where the anchor moves to
 * @param caret the position where the caret moves to
 */
void Caret::select(const Position& anchor, const Position& caret) {
	verifyViewer();
	if(selectionMode_ != CHARACTER)
		restoreSelectionMode();
	pastingFromClipboardRing_ = false;
	if(anchor != anchor_->getPosition() || caret != getPosition()) {
		const Region oldRegion(getSelectionRegion());
		if(selectionMode_ == CHARACTER) {
			leadingAnchor_ = true;
			anchor_->moveTo(anchor);
			leadingAnchor_ = false;
		}
		VisualPoint::doMoveTo(caret);
		if(isSelectionRectangle())
			box_->update(getSelectionRegion());
		if(autoShow_)
			show();
		listeners_.notify<const Caret&, const Region&>(ICaretListener::caretMoved, *this, oldRegion);
	}
	checkMatchBrackets();
}

/// Selects the word at the caret position.
void Caret::selectWord() {
	verifyViewer();

	WordBreakIterator<DocumentCharacterIterator> i(
		DocumentCharacterIterator(*getDocument(), *this), AbstractWordBreakIterator::BOUNDARY_OF_SEGMENT, getIdentifierSyntax());
	endBoxSelection();
	if(isEndOfLine()) {
		if(isBeginningOfLine())	// an empty line
			moveTo(*this);
		else	// eol
			select((--i).base().tell(), *this);
	} else if(isBeginningOfLine())	// bol
		select(*this, (++i).base().tell());
	else {
		const Position p = (++i).base().tell();
		i.base().seek(Position(getLineNumber(), getColumnNumber() + 1));
		select((--i).base().tell(), p);
	}
}

/**
 * Sets character input mode.
 * @param overtype true to set to overtype mode, false to set to insert mode
 * @see #inputCharacter, #isOvertypeMode
 */
void Caret::setOvertypeMode(bool overtype) throw() {
	if(overtype != overtypeMode_) {
		overtypeMode_ = overtype;
		stateListeners_.notify<const Caret&>(ICaretStateListener::overtypeModeChanged, *this);
	}
}

/// @see Point#update
void Caret::update(const DocumentChange& change) {
	// notify the movement of the anchor and the caret concurrently when the document was changed
	leaveAnchorNext_ = leadingAnchor_ = true;
	anchor_->beginInternalUpdate(change);
	Point::update(change);
	anchor_->endInternalUpdate();
	leaveAnchorNext_ = leadingAnchor_ = false;
}

inline void Caret::updateVisualAttributes() {
	if(isSelectionRectangle())
		box_->update(getSelectionRegion());
	if((regionBeforeMoved_.first != getPosition() || regionBeforeMoved_.second != getPosition()))
		listeners_.notify<const Caret&, const Region&>(ICaretListener::caretMoved, *this, regionBeforeMoved_);
	if(autoShow_)
		show();
	checkMatchBrackets();
	regionBeforeMoved_.first = regionBeforeMoved_.second = Position::INVALID_POSITION;
}
