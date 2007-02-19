/**
 * @file point.cpp
 * @author exeal
 * @date 2003-2007
 */

#include "stdafx.h"
#include "viewer.hpp"
#include "break-iterator.hpp"
#include "../../manah/win32/utility.hpp"

using namespace ascension;
using namespace ascension::text;
using namespace ascension::viewers;
using namespace ascension::presentation;
using namespace ascension::unicode;
//using namespace ascension::internal;
using namespace manah::windows;
using namespace std;


namespace {
	/// The clipboard
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


// EditPoint ////////////////////////////////////////////////////////////////

/**
 * Moves to the next character.
 * @param offset the offset of the movement
 */
void EditPoint::charNext(length_t offset /* = 1 */) {
	verifyDocument();
	normalize();
	moveTo(getNextCharPos(*this, offset));
}

/**
 * Moves to the previous character.
 * @param offset the offset of the movement
 */
void EditPoint::charPrev(length_t offset /* = 1 */) {
	verifyDocument();
	normalize();
	moveTo(getPrevCharPos(*this, offset));
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
	p.charNext();
	if(p != *this) {
		const bool adapts = adaptsToDocument();
		adaptToDocument(false);
		document.deleteText(Region(*this, p));
		moveTo(document.insertText(*this, first, last));
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

/**
 * 指定位置までのテキストを削除
 * @param length もう1つの位置までの文字数 (負でもよい)
 * @param cu 文字数の計算方法。@c CU_DEFAULT を指定すると現在の設定が使用される
 */
void EditPoint::erase(signed_length_t length /* = 1 */, EditPoint::CharacterUnit cu /* = CU_DEFAULT */) {
	verifyDocument();
	if(getDocument()->isReadOnly() || length == 0)
		return;
	erase((length > 0) ? getNextCharPos(*this, length) : getPrevCharPos(*this, -length, CU_UTF16));
}

/**
 * 指定位置までのテキストを削除
 * @param other もう1つの位置
 */
void EditPoint::erase(const Position& other) {
	verifyDocument();
	if(getDocument()->isReadOnly() || other == getPosition())
		return;
	const bool adapts = adaptsToDocument();
	adaptToDocument(false);
	moveTo(getDocument()->deleteText(Region(*this, other)));
	adaptToDocument(adapts);
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
	return surrogates::decode(line.data() + getColumnNumber(), line.length() - getColumnNumber());
}

/// Returns the length of the current line.
length_t EditPoint::getLineLength() const {
	verifyDocument();
	normalize();
	return getDocument()->getLineLength(getLineNumber());
}

/**
 * 与えられた位置から指定文字数分進んだ位置を返す
 * @param pt 基準位置
 * @param length 文字数
 * @param cu 文字数計算方法。省略すると @a pt の設定値
 */
Position EditPoint::getNextCharPos(const EditPoint& pt, length_t length, EditPoint::CharacterUnit cu /* = CU_DEFAULT */) {
	if(length == 0)
		return pt;
	const Document& document = *pt.getDocument();
	if(cu == CU_DEFAULT)
		cu = pt.characterUnit_;
	if(cu == CU_GRAPHEME_CLUSTER) {
		GraphemeBreakIterator<DocumentCharacterIterator> i(DocumentCharacterIterator(document, pt));
		i.next(length);
		return i.base().tell();
	}

	const length_t lines = document.getNumberOfLines();
	const String* line = &document.getLine(pt.getLineNumber());
	const Char* p = line->data();
	Position pos = pt;
	while(length-- > 0) {
		if(pos.column == line->length()) {	// 行末なので次の行に移動
			if(pos.line == lines - 1)	// 最終行であれば移動しない
				return pos;
			line = &document.getLine(++pos.line);
			p = line->data();
			pos.column = 0;
		} else if(cu == CU_UTF16 || line->length() - pos.column == 1)
			++pos.column;
		else {
			assert(cu == CU_UTF32);
			pos.column += (surrogates::decode(p + pos.column, line->length() - pos.column) > 0xFFFF) ? 2 : 1;
		}
	}
	return pos;
}

/**
 * 与えられた位置から指定文字数分戻った位置を返す
 * @param pt 基準位置
 * @param length 文字数
 * @param cu 文字数計算方法。省略すると @a pt の設定値
 */
Position EditPoint::getPrevCharPos(const EditPoint& pt, length_t length, EditPoint::CharacterUnit cu /* = CU_DEFAULT */) {
	if(length == 0)
		return pt;
	const Document& document = *pt.getDocument();
	if(cu == CU_DEFAULT)
		cu = pt.characterUnit_;
	if(cu == CU_GRAPHEME_CLUSTER) {
		GraphemeBreakIterator<DocumentCharacterIterator> i(DocumentCharacterIterator(document, pt));
		i.next(-static_cast<signed_length_t>(length));
		return i.base().tell();
	}

	const String* line = &document.getLine(pt.getLineNumber());
	const Char* p = line->data();
	Position pos = pt;
	while(length-- > 0) {
		if(pos.column == 0) {	// 行頭なので前の行に移動
			if(pos.line == 0)	// 先頭行であれば移動しない
				return pos;
			line = &document.getLine(--pos.line);
			p = line->data();
			pos.column = line->length();
		} else if(cu == CU_UTF16 || pos.column == 1)
			--pos.column;
		else if(cu == CU_UTF32) {
			assert(cu == CU_GRAPHEME_CLUSTER);
			pos.column -= (surrogates::isHighSurrogate(p[pos.column - 2])
						&& surrogates::isLowSurrogate(p[pos.column - 1])) ? 2 : 1;
		}
	}
	return pos;
}

/**
 * 範囲内のテキストを返す
 * @param length もう1つの位置までの文字数 (負でもよい)
 * @param lbr 改行文字の扱い (@a length の数え方には影響しないので注意)
 */
String EditPoint::getText(signed_length_t length, LineBreakRepresentation lbr /* = LBR_PHYSICAL_DATA */) const {
	verifyDocument();
	normalize();
	if(length == 0)
		return L"";
	return getText((length > 0) ? getNextCharPos(*this, length) : getPrevCharPos(*this, -length), lbr);
}

/**
 * 範囲内のテキストを返す
 * @param other もう1つの位置
 * @param lbr 改行文字の扱い
 */
String EditPoint::getText(const Position& other, LineBreakRepresentation lbr /* = LBR_PHYSICAL_DATA */) const {
	// TODO: this code can be rewritten via Document#writeToStream
	verifyDocument();

	Position position = other;
	const Document& document = *getDocument();
	
	position.line = min(other.line, document.getNumberOfLines() - 1);
	normalize();
	if(other == getPosition())
		return L"";

	const Position& start = min(getPosition(), position);
	const Position& end = max(getPosition(), position);

	if(start.line == end.line)	// 1行の場合
		return document.getLine(end.line).substr(start.column, end.column - start.column);
	else {	// 複数行の場合
		StringBuffer text(ios_base::out);
		length_t line = start.line;
		Char eol[3] = L"";

		switch(lbr) {
		case LBR_LINE_FEED:
			wcscpy(eol, L"\n"); break;
		case LBR_CRLF:
			wcscpy(eol, L"\r\n"); break;
		case LBR_LINE_SEPARATOR:
			wcscpy(eol, L"\x2028"); break;
		case LBR_DOCUMENT_DEFAULT:
			wcscpy(eol, getLineBreakString(document.getLineBreak())); break;
		}
		const streamsize eolSize = static_cast<streamsize>(wcslen(eol));
		while(true) {
			const Document::Line& ln = document.getLineInfo(line);
			const String& s = ln.getLine();
			if(line == start.line)	// 先頭行
				text.sputn(s.data() + start.column, static_cast<streamsize>(s.length() - start.column));
			else if(line == end.line) {	// 最終行
				text.sputn(s.data(), static_cast<streamsize>(end.column));
				break;
			} else
				text.sputn(s.data(), static_cast<streamsize>(s.length()));
			if(lbr == LBR_PHYSICAL_DATA)
				text.sputn(getLineBreakString(ln.getLineBreak()), static_cast<streamsize>(getLineBreakLength(ln.getLineBreak())));
			else
				text.sputn(eol, eolSize);
			++line;
		}
		return text.str();
	}
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
	moveTo(getDocument()->insertText(*this, first, last));
	adaptToDocument(adapts);
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

/// Returns true if the point is the start of the document.
bool EditPoint::isStartOfDocument() const {
	verifyDocument();
	normalize();
	return isExcludedFromRestriction() ? getPosition() == getDocument()->getStartPosition() : getPosition() == Position(0, 0);
}

/// Returns true if the point is the start of the line.
bool EditPoint::isStartOfLine() const {
	verifyDocument();
	normalize();
	if(isExcludedFromRestriction()) {
		const Position start = getDocument()->getStartPosition();
		return (start.line == getLineNumber()) ? start.column == getColumnNumber() : getColumnNumber() == 0;
	} else
		return getColumnNumber() == 0;
}

/**
 * Moves to the next line.
 * @param offset the offset of the movement
 */
void EditPoint::lineDown(length_t offset /* = 1 */) {
	verifyDocument();
	normalize();
	const length_t newLine = min(getLineNumber() + offset, getDocument()->getEndPosition(isExcludedFromRestriction()).line);
	if(newLine != getLineNumber())
		moveTo(Position(newLine, getColumnNumber()));
}

/**
 * Moves to the previous line.
 * @param offset the offset of the movement
 */
void EditPoint::lineUp(length_t offset /* = 1 */) {
	verifyDocument();
	normalize();
	const length_t newLine = (getLineNumber() > offset) ?
		max(getLineNumber() - offset, getDocument()->getStartPosition(isExcludedFromRestriction()).line) : 0;
	if(newLine != getLineNumber())
		moveTo(Position(newLine, getColumnNumber()));
}

/**
 * Moves to the specified offset.
 * @param offset the offset from the start of the document.
 */
void EditPoint::moveToAbsoluteCharOffset(length_t offset) {
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
		const length_t lineLength = document.getLineLength(line) + 1;	// +1 は改行分
		if(readCount + lineLength >= offset) {
			moveTo(Position(line, readCount + lineLength - offset));
			return;
		}
		readCount += lineLength;
	}
	moveTo(Position(end.line, document.getLineLength(end.line)));
}

/// Moves to the end of the document.
void EditPoint::moveToEndOfDocument() {
	const length_t lines = getDocument()->getNumberOfLines();
	moveTo(Position(lines - 1, getDocument()->getLineLength(lines - 1)));
}

/// Moves to the end of the line.
void EditPoint::moveToEndOfLine() {
	moveTo(Position(min(getLineNumber(), getDocument()->getNumberOfLines() - 1), getDocument()->getLineLength(getLineNumber())));
}

/**
 * Moves to the start of the next bookmarked line.
 * @return false if the bookmark is not found
 */
bool EditPoint::moveToNextBookmark() {
	verifyDocument();

	const Bookmarker& bookmarker = getDocument()->getBookmarker();
	length_t line;
	const length_t endLine = getDocument()->getEndPosition().line;

	// 探す
	for(line = getLineNumber() + 1; line <= endLine; ++line) {
		if(bookmarker.isMarked(line)) {
			moveTo(Position(line, 0));
			return true;
		}
	}

	// 見つからなければ折り返す
	for(line = getDocument()->getStartPosition().line; line < getLineNumber(); ++line) {
		if(bookmarker.isMarked(line)) {
			moveTo(Position(line, 0));
			return true;
		}
	}

	return false;
}

/**
 * Moves to the start of the previous bookmarked line.
 * @return false if the bookmark is not found
 */
bool EditPoint::moveToPrevBookmark() {
	verifyDocument();

	const Bookmarker& bookmarker = getDocument()->getBookmarker();
	length_t line = getLineNumber();
	const length_t startLine = getDocument()->getStartPosition().line;

	// 探す
	while(line-- != startLine) {
		if(bookmarker.isMarked(line)) {
			moveTo(Position(line, 0));
			return true;
		}
	}

	// 見つからなければ折り返す
	for(line = getDocument()->getEndPosition().line; line > getLineNumber(); --line) {
		if(bookmarker.isMarked(line)) {
			moveTo(Position(line, 0));
			return true;
		}
	}

	return false;
}

/// Moves to the start of the document.
void EditPoint::moveToStartOfDocument() {
	moveTo(Position(0, 0));
}

/// Moves to the start of the line.
void EditPoint::moveToStartOfLine() {
	moveTo(Position(min(getLineNumber(), getDocument()->getNumberOfLines() - 1), 0));
}

/**
 * Breaks the line.
 * @note This method is hidden by @c VisualPoint#newLine (C++ rule).
 */
void EditPoint::newLine() {
	verifyDocument();
	if(getDocument()->isReadOnly())
		return;
	insert(getLineBreakString(getDocument()->getLineBreak()));
}


// VisualPoint //////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param viewer the viewer
 * @param listener the listener. can be @c null
 */
VisualPoint::VisualPoint(TextViewer& viewer, IPointListener* listener /* = 0 */) throw()
		: EditPoint(viewer.getDocument(), listener), LineLayoutBuffer(viewer, ASCENSION_VISUAL_POINT_CACHE_LINES, false),
		viewer_(&viewer), clipboardNativeEncoding_(::GetACP()), lastX_(-1), crossingLines_(false), visualLine_(-1), visualSubline_(0) {
			static_cast<text::internal::IPointCollection<VisualPoint>&>(viewer).addNewPoint(*this);
}

/// Copy-constructor.
VisualPoint::VisualPoint(const VisualPoint& rhs) : EditPoint(rhs), LineLayoutBuffer(*rhs.viewer_, ASCENSION_VISUAL_POINT_CACHE_LINES, false),
		viewer_(rhs.viewer_), lastX_(rhs.lastX_), crossingLines_(false), visualLine_(rhs.visualLine_), visualSubline_(rhs.visualSubline_) {
	if(viewer_ == 0)
		throw DisposedViewerException();
	static_cast<text::internal::IPointCollection<VisualPoint>*>(viewer_)->addNewPoint(*this);
}

/// Destructor.
VisualPoint::~VisualPoint() throw() {
	if(viewer_ != 0)
		static_cast<text::internal::IPointCollection<VisualPoint>*>(viewer_)->removePoint(*this);
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
 * Moves to left character.
 * @param offset the offset of the movement
 */
void VisualPoint::charLeft(length_t offset /* = 1 */) {
	verifyViewer();
	normalize();
	moveTo((viewer_->getConfiguration().orientation == LEFT_TO_RIGHT) ? getPrevCharPos(*this, offset) : getNextCharPos(*this, offset));
}

/**
 * Moves to the right character.
 * @param offset the offset of the movement
 */
void VisualPoint::charRight(length_t offset /* = 1 */) {
	verifyViewer();
	normalize();
	moveTo((viewer_->getConfiguration().orientation == LEFT_TO_RIGHT) ? getNextCharPos(*this, offset) : getPrevCharPos(*this, offset));
}

/**
 * 指定位置までのテキストをクリップボードにコピー
 * @param length コピーする文字数 (負でもよい)
 */
void VisualPoint::copy(signed_length_t length) {
	verifyViewer();
	const String text = getText(length);	
	Clipboard(*viewer_).write(text.data(), text.data() + text.length());
}

/**
 * 指定位置までのテキストをクリップボードにコピー
 * @param other もう1つの位置
 */
void VisualPoint::copy(const Position& other) {
	verifyViewer();
	const String text = getText(other);
	Clipboard(*viewer_).write(text.data(), text.data() + text.length());
}

/**
 * 指定位置までのテキストを削除してクリップボードにコピー
 * @param length 削除する文字数 (負でもよい)
 */
void VisualPoint::cut(signed_length_t length) {
	verifyViewer();
	if(getDocument()->isReadOnly())
		return;
	const String text = getText(length);
	Clipboard(*viewer_).write(text.data(), text.data() + text.length());
	erase(length);
}

/**
 * 指定位置までのテキストを削除してクリップボードにコピー
 * @param other もう1つの位置
 */
void VisualPoint::cut(const Position& other) {
	verifyViewer();
	if(getDocument()->isReadOnly())
		return;
	const String text = getText(other);
	Clipboard(*viewer_).write(text.data(), text.data() + text.length());
	erase(other);
}

/// @see EditPoint#doMoveTo
void VisualPoint::doMoveTo(const Position& to) {
	verifyViewer();
	if(getLineNumber() == to.line && visualLine_ != INVALID_INDEX) {
		visualLine_ -= visualSubline_;
		visualSubline_ = (viewer_->getTextRenderer().getNumberOfSublinesOfLine(to.line) > 1) ? getLayout(to.line).getSubline(to.column) : 0;
		visualLine_ += visualSubline_;
	} else
		visualLine_ = INVALID_INDEX;
	EditPoint::doMoveTo(to);
	if(!crossingLines_)
		lastX_ = -1;
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
		document.deleteText(region);
		document.insertText(region.getTop(), indent);
		return getPosition();
	}

	const Position oldPosition = getPosition();
	Position otherResult = other;
	length_t line = region.getTop().line;
	const bool adapts = adaptsToDocument();

	adaptToDocument(false);

	// 最初の行を (逆) インデント
	if(level > 0) {
		document.insertText(Position(line, box ? region.getTop().column : 0), indent);
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
			document.deleteText(Position(line, 0), Position(line, deleteLength));
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
				document.insertText(Position(line, insertPosition), indent);
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
				document.deleteText(Position(line, 0), Position(line, deleteLength));
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

/// 
inline const IdentifierSyntax& VisualPoint::getIdentifierSyntax() const {
	return getDocument()->getContentTypeInformation().getIdentifierSyntax(getContentType());
}

/**
 * Returns the layout of the specified line.
 * @param line the line number or -1 to get for the current line
 * @return the layout
 */
const LineLayout& VisualPoint::getLayout(length_t line /* = -1 */) const {
	verifyViewer();
	if(line == -1)
		line = getLineNumber();
	if(isLineCached(line))
		return getLineLayout(line);
	const TextRenderer& renderer = viewer_->getTextRenderer();
	return renderer.isLineCached(line) ? renderer.getLineLayout(line) : getLineLayout(line);
}

/// Returns the visual column of the point.
length_t VisualPoint::getVisualColumnNumber() const {
	if(lastX_ == -1)
		const_cast<VisualPoint*>(this)->updateLastX();
	const TextViewer::Configuration& c = viewer_->getConfiguration();
	if(c.alignment == ALIGN_LEFT || (c.alignment != ALIGN_RIGHT && c.orientation == LEFT_TO_RIGHT))
		return lastX_ / viewer_->getTextRenderer().getAverageCharacterWidth();
	else
		return (getLayout().getWidth() - lastX_) / viewer_->getTextRenderer().getAverageCharacterWidth();
}

/**
 * Inserts the spcified text as a rectangle at the current position.
 * @param first the start of the text
 * @param last the end of the text
 * @see text#EditPoint#insert
 */
void VisualPoint::insertBox(const Char* first, const Char* last) {
	verifyViewer();

	Document& document = *getDocument();

	if(document.isReadOnly() || first == last)
		return;

	const length_t lineCount = document.getNumberOfLines();
	const String breakString = getLineBreakString(document.getLineBreak());
	list<const Char*>	lines;

	// 行頭位置のリストを作る
	lines.push_back(first);
	for(const Char* p = first; p != last; ) {
		p = find_first_of(p, last, LINE_BREAK_CHARACTERS, endof(LINE_BREAK_CHARACTERS));
		p += (*p == CARRIAGE_RETURN && p < last - 1 && *(p + 1) == LINE_FEED) ? 2 : 1;
		lines.push_back(p);
	}

	length_t i = getLineNumber();	// 挿入行
	String line;					// 挿入行となる部分文字列
	const int xFirstLineInsert =	// 挿入先頭行の挿入位置
		getLayout().getLocation(getColumnNumber()).x;	// TODO: recognize wrap.
	for(list<const Char*>::const_iterator it = lines.begin(); it != lines.end(); ++it, ++i) {
		if(*it == lines.back())	// 最終行
			line.assign(*it, last);
		else {
			++it;
			length_t lineLength = *it - first - ((*(*it - 1) == LINE_FEED && *(*it - 2) == CARRIAGE_RETURN) ? 2 : 1);
			lineLength -= *(--it) - first;
			line.assign(*it, lineLength);
		}
		if(i >= lineCount - 1 && *it != lines.back())	// 挿入位置に行が存在しない場合は作成する
			line += breakString;

		// TODO: not implemented.
		if(!line.empty()) {
/*			const Position visualOffsets = viewer_->getPresentation().mapLogicalPositionToVisualOffsetsInLine(*this);
			moveTo(document.insertText(Position(i,
				viewer_->getPresentation().getCharacterForX(i, visualOffsets.line, xFirstLineInsert, true)),
					viewer_->calculateSpacesReachingVirtualPoint(i, xFirstLineInsert) + line));
*/		}
	}
}

/**
 * Returns true if the point is end of the visual line.
 * @see text#EditPoint#isEndOfLine
 */
bool VisualPoint::isEndOfVisualLine() const {
	verifyViewer();
	if(isEndOfLine())
		return true;
	const LineLayout& layout = getLayout();
	const length_t subline = layout.getSubline(getColumnNumber());
	return getColumnNumber() == layout.getSublineOffset(subline) + layout.getSublineLength(subline);
}

/// Returns true if the current position is the first non-white space character in the line.
bool VisualPoint::isFirstCharOfLine() const {
	verifyViewer();
	normalize();
	const Position start = getDocument()->getStartPosition(isExcludedFromRestriction());
	const length_t offset = (start.line == getLineNumber()) ? start.column : 0;
	const String& line = getDocument()->getLine(getLineNumber());
	return line.data() + getColumnNumber() - offset
		== getIdentifierSyntax().eatWhiteSpaces(line.data() + offset, line.data() + line.length(), true);
}

/// Returns true if the current position is the last non-white space character in the line.
bool VisualPoint::isLastCharOfLine() const {
	verifyViewer();
	normalize();
	const Position end = getDocument()->getEndPosition(isExcludedFromRestriction());
	const String& line = getDocument()->getLine(getLineNumber());
	const length_t lineLength = (end.line == getLineNumber()) ? end.column : line.length();
	return line.data() + lineLength - getColumnNumber()
		== getIdentifierSyntax().eatWhiteSpaces(line.data() + getColumnNumber(), line.data() + lineLength, true);
}

/**
 * Returns true if the point is the start of the visual line.
 * @see EditPoint#isStartOfLine
 */
bool VisualPoint::isStartOfVisualLine() const {
	verifyViewer();
	if(isStartOfLine())
		return true;
	const LineLayout& layout = getLayout();
	return getColumnNumber() == layout.getSublineOffset(layout.getSubline(getColumnNumber()));
}

/// @see LineLayoutBuffer#layoutDeleted
void VisualPoint::layoutDeleted(length_t first, length_t last, length_t sublines) throw() {
	if(!adaptsToDocument() && getLineNumber() >= first && getLineNumber() < last)
		visualLine_ = INVALID_INDEX;
}

/// @see LineLayoutBuffer#layoutInserted
void VisualPoint::layoutInserted(length_t first, length_t last) throw() {
	if(!adaptsToDocument() && getLineNumber() >= first && getLineNumber() < last)
		visualLine_ = INVALID_INDEX;
}

/// @see LineLayoutBuffer#layoutModified
void VisualPoint::layoutModified(length_t first, length_t last, length_t newSublines, length_t oldSublines, bool) throw() {
	if(visualLine_ != INVALID_INDEX) {
		// 折り返しの変化に従って、visualLine_ と visualSubine_ を調整する
		if(last <= getLineNumber()) {
			visualLine_ += newSublines;
			visualLine_ -= oldSublines;
		} else if(first == getLineNumber()) {
			visualLine_ -= visualSubline_;
			visualSubline_ = getLayout().getSubline(getColumnNumber());
			visualLine_ += visualSubline_;
		} else if(first < getLineNumber())
			visualLine_ = INVALID_INDEX;
	}
}

/**
 * Moves to the end of the visual line.
 * @see EditPoint#moveToEndOfLine
 */
void VisualPoint::moveToEndOfVisualLine() {
	verifyViewer();
	const LineLayout& layout = getLayout();
	const length_t subline = layout.getSubline(getColumnNumber());
	moveTo(Position(getLineNumber(), (subline < layout.getNumberOfSublines() - 1) ? layout.getSublineOffset(subline + 1) : getLineLength()));
}

/// Moves to the first non-white space character.
void VisualPoint::moveToFirstCharOfLine() {
	verifyViewer();
	const length_t line = min(getLineNumber(), getDocument()->getEndPosition(isExcludedFromRestriction()).line);
	const Char* const p = getDocument()->getLine(line).data();
	moveTo(Position(line, getIdentifierSyntax().eatWhiteSpaces(p, p + getDocument()->getLineLength(line), true) - p));
}

/// Moves to the last non-white space character.
void VisualPoint::moveToLastCharOfLine() {
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

/**
 * Moves to the start of the visual line.
 * @see EditPoint#moveToStartOfLine
 */
void VisualPoint::moveToStartOfVisualLine() {
	verifyViewer();
	const LineLayout& layout = getLayout();
	moveTo(Position(getLineNumber(), layout.getSublineOffset(layout.getSubline(getLineNumber()))));
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

	String breakString = getLineBreakString(getDocument()->getLineBreak());

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
void VisualPoint::pageDown(length_t offset /* = 1 */) {
	verifyViewer();
	// TODO: calculate exact number of visual line.
	visualLineDown(viewer_->getNumberOfVisibleLines() * offset);
}

/**
 * Moves to the previous page.
 * @param offset the offset of the movement
 */
void VisualPoint::pageUp(length_t offset /* = 1 */) {
	verifyViewer();
	// TODO: calculate exact number of visual line.
	visualLineUp(viewer_->getNumberOfVisibleLines() * offset);
}

/**
 * 範囲内のテキストをクリップボードの内容で置換
 * @param length もう1つの位置までの文字数 (負でもよい)
 */
void VisualPoint::paste(signed_length_t length /* = 0 */) {
	verifyViewer();
	if(getDocument()->isReadOnly() || length == 0) {
		paste(getPosition());
		return;
	}
	paste((length > 0) ? getNextCharPos(*this, length) : getPrevCharPos(*this, -length, CU_UTF16));
}

/**
 * 範囲内のテキストをクリップボードの内容で置換
 * @param other もう1つの位置
 */
void VisualPoint::paste(const Position& other) {
	verifyViewer();

	if(getDocument()->isReadOnly())
		return;
	else if(const UINT availableClipFormat = canPaste()) {
		if(other != getPosition())
			erase(other);

		Clipboard clipboard(*viewer_);
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
 * 指定範囲がビューの中央になるようにスクロールする。ただし既に可視なら何もしない
 * @param length 範囲を構成するもう一方の点までの文字数
 * @return 範囲がビューに納まる場合は true を返す
 */
bool VisualPoint::recenter(signed_length_t length /* = 0 */) {
	verifyViewer();
	return recenter((length >= 0) ? getNextCharPos(*this, length) : getPrevCharPos(*this, -length));
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
 * 指定範囲が可視になるようにビューをスクロールする
 * @param length 範囲を構成するもう一方の点までの文字数
 * @return 範囲がビューに納まる場合は true を返す (未実装につき常に true)
 */
bool VisualPoint::show(signed_length_t length /* = 0 */) {
	verifyDocument();
	return show((length >= 0) ? getNextCharPos(*this, length) : getPrevCharPos(*this, -length));
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

	// 垂直方向
	if(visualLine_ == INVALID_INDEX) {
		visualLine_ = viewer_->getTextRenderer().mapLogicalLineToVisualLine(getLineNumber());
		visualSubline_ = getLayout().getSubline(getColumnNumber());
		visualLine_ += visualSubline_;
	}
	si.fMask = SIF_POS;
	viewer_->getScrollInfo(SB_VERT, si);
	if(visualLine_ < si.nPos * viewer_->getScrollRate(false))	// 画面より上
		to.y = static_cast<long>(visualLine_ * viewer_->getScrollRate(false));
	else if(visualLine_ - si.nPos * viewer_->getScrollRate(false) > visibleLines - 1)	// 画面より下
		to.y = static_cast<long>((visualLine_ - visibleLines + 1) * viewer_->getScrollRate(false));
	if(to.y < -1)
		to.y = 0;

	// 水平方向
	if(!viewer_->getConfiguration().lineWrap.wrapsAtWindowEdge()) {
		const length_t visibleColumns = viewer_->getNumberOfVisibleColumns();
		const ulong x = getLayout().getLocation(getColumnNumber(), LineLayout::LEADING).x;
		viewer_->getScrollInfo(SB_HORZ, si);
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
 * 範囲内のテキストをスペースインデント
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
 * 範囲内のテキストをタブインデント
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
bool VisualPoint::transposeChars() {
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

	if(BinaryProperty::is<BinaryProperty::GRAPHEME_EXTEND>(getCodePoint()))	// クラスタの先頭でない
		return false;
	else if(IS_RESTRICTION(getPosition()))	// アクセス不能領域
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

	if(top.line == bottom.line)	// 1行しか無い
		return false;

	if(getLineNumber() == top.line)
		moveTo(getLineNumber() + 1, getColumnNumber());

	const String str1 = (getLineNumber() - 1 == top.line) ?
		getDocument()->getLine(getLineNumber() - 1).substr(top.column) : getDocument()->getLine(getLineNumber() - 1);
	const String str2 = (getLineNumber() == bottom.line) ?
		getDocument()->getLine(getLineNumber()).substr(0, bottom.column) : getDocument()->getLine(getLineNumber());

	// 2行とも空にする
	if(!str2.empty()) {
		moveToStartOfLine();
		erase(static_cast<signed_length_t>(str2.length()), CU_UTF16);
	}
	if(!str1.empty()) {
		moveTo(getLineNumber() - 1, (getLineNumber() == top.line) ? top.column : 0);
		erase(static_cast<signed_length_t>(str1.length()), CU_UTF16);
		moveTo(getLineNumber() + 1, getColumnNumber());
	}

	// 行に書き込む
	if(!str1.empty()) {
		moveToStartOfLine();
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

	// まず前方の単語 (1st-word-*) を探す
	pos[0] = (--i).base().tell();
	i.setComponent(AbstractWordBreakIterator::END_OF_ALPHANUMERICS);
	pos[1] = (++i).base().tell();
	if(pos[1] == pos[0])	// 単語が空
		return false;

	// 次に後方の単語 (2nd-word-*) を探す
	i.base().seek(*this);
	i.setComponent(AbstractWordBreakIterator::START_OF_ALPHANUMERICS);
	pos[2] = (++i).base().tell();
	if(pos[2] == getPosition())
		return false;
	pos[3] = (++i).base().tell();
	if(pos[2] == pos[3])	// 単語が空
		return false;

	// 置換する
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
	if(!isDocumentDisposed())
		lastX_ = getLayout().getLocation(getColumnNumber(), LineLayout::LEADING).x;
}

/**
 * Moves to the next visual line.
 * @param offset the offset of the movement
 */
void VisualPoint::visualLineDown(length_t offset /* = 1 */) {
	verifyViewer();
	normalize();
	const TextRenderer& renderer = viewer_->getTextRenderer();
	length_t line = getLineNumber(), subline = getLayout().getSubline(getColumnNumber());
	if(lastX_ == -1)
		updateLastX();
	renderer.offsetVisualLine(line, subline, static_cast<signed_length_t>(offset));
	crossingLines_ = true;
	moveTo(Position(line, getLayout(line).getOffset(lastX_, renderer.getLinePitch() * static_cast<long>(subline))));
	crossingLines_ = false;
}

/**
 * Moves to the previous visual line.
 * @param offset the offset of the movement
 */
void VisualPoint::visualLineUp(length_t offset /* = 1 */) {
	verifyViewer();
	normalize();
	const TextRenderer& renderer = viewer_->getTextRenderer();
	length_t line = getLineNumber(), subline = getLayout().getSubline(getColumnNumber());
	if(lastX_ == -1)
		updateLastX();
	renderer.offsetVisualLine(line, subline, -static_cast<signed_length_t>(offset));
	crossingLines_ = true;
	moveTo(Position(line, getLayout(line).getOffset(lastX_, renderer.getLinePitch() * static_cast<long>(subline))));
	crossingLines_ = false;
}

/**
 * Moves to the end of the left word.
 * @param offset the number of words
 */
void VisualPoint::wordEndLeft(length_t offset /* = 1 */) {
	verifyViewer();
	(viewer_->getConfiguration().orientation == LEFT_TO_RIGHT) ? wordEndPrev(offset) : wordEndNext(offset);
}

/**
 * Moves to the end of the next word.
 * @param offset the number of words
 */
void VisualPoint::wordEndNext(length_t offset /* = 1 */) {
	verifyViewer();
	normalize();
	WordBreakIterator<DocumentCharacterIterator> i(
		DocumentCharacterIterator(*getDocument(), *this), AbstractWordBreakIterator::END_OF_SEGMENT, getIdentifierSyntax());
	i += offset;
	moveTo(i.base().tell());
}

/**
 * Moves to the end of the previous word.
 * @param offset the number of words
 */
void VisualPoint::wordEndPrev(length_t offset /* = 1 */) {
	verifyViewer();
	normalize();
	WordBreakIterator<DocumentCharacterIterator> i(
		DocumentCharacterIterator(*getDocument(), *this), AbstractWordBreakIterator::END_OF_SEGMENT, getIdentifierSyntax());
	i -= offset;
	moveTo(i.base().tell());
}

/**
 * Moves to the end of the right word.
 * @param offset the number of words
 */
void VisualPoint::wordEndRight(length_t offset /* = 1 */) {
	verifyViewer();
	(viewer_->getConfiguration().orientation == LEFT_TO_RIGHT) ? wordEndNext(offset) : wordEndPrev(offset);
}

/**
 * Moves to the start of the left word.
 * @param offset the number of words
 */
void VisualPoint::wordLeft(length_t offset /* = 1 */) {
	verifyViewer();
	(viewer_->getConfiguration().orientation == LEFT_TO_RIGHT) ? wordPrev(offset) : wordNext(offset);
}

/**
 * Moves to the start of the next word.
 * @param offset the number of words
 */
void VisualPoint::wordNext(length_t offset /* = 1 */) {
	verifyViewer();
	normalize();
	WordBreakIterator<DocumentCharacterIterator> i(
		DocumentCharacterIterator(*getDocument(), *this), AbstractWordBreakIterator::START_OF_SEGMENT, getIdentifierSyntax());
	i += offset;
	moveTo(i.base().tell());
}

/**
 * Moves to the start of the previous word.
 * @param offset the number of words
 */
void VisualPoint::wordPrev(length_t offset /* = 1 */) {
	verifyViewer();
	normalize();
	WordBreakIterator<DocumentCharacterIterator> i(
		DocumentCharacterIterator(*getDocument(), *this), AbstractWordBreakIterator::START_OF_SEGMENT, getIdentifierSyntax());
	i -= offset;
	moveTo(i.base().tell());
}

/**
 * Moves to the start of the right word.
 * @param offset the number of words
 */
void VisualPoint::wordRight(length_t offset /* = 1 */) {
	verifyViewer();
	(viewer_->getConfiguration().orientation == LEFT_TO_RIGHT) ? wordNext(offset) : wordPrev(offset);
}


// Caret ////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param viewer the viewer
 */
Caret::Caret(TextViewer& viewer) throw() : VisualPoint(viewer, 0),
		anchor_(new SelectionAnchor(viewer)), selectionMode_(CHARACTER), pastingFromClipboardRing_(false),
		leaveAnchorNext_(false), leadingAnchor_(false), autoShow_(true), box_(0), matchBracketsTrackingMode_(DONT_TRACK),
		overtypeMode_(false), editingByThis_(false), othersEditedFromLastInputChar_(false),
		matchBrackets_(make_pair(Position::INVALID_POSITION, Position::INVALID_POSITION)) {
	excludeFromRestriction(true);
	anchor_->excludeFromRestriction(true);
}

/// Destructor.
Caret::~Caret() throw() {
	delete anchor_;
	delete box_;
}

#if 0
/**
 * Starts the auto completion.
 * This method will fail in the following situations:
 * <ul>
 *  <li>the document is read only</li>
 *  <li>the selection is not empty (in this case, just the selection will be cleared)</li>
 *  <li>there are no candidates</li>
 * </ul>
 * @return succeeded or not
 */
bool Caret::beginAutoCompletion() {
	verifyViewer();
	if(!isSelectionEmpty()) {
		clearSelection();
		return false;
	} else if(getDocument()->isReadOnly())
		return false;

	// TODO: implement Caret#beginAutoCompletion
#if 0
	// <<実験的な動作>>
	// 周辺の行から識別子を収集してみる
	set<String> candidateWords;
	const length_t RECOG_LINES = 100;	// 直前の何行を考慮するか
	for(length_t i = (getCaret().getLineNumber() > RECOG_LINES) ?
			getCaret().getLineNumber() - RECOG_LINES : 0;i < getCaret().getLineNumber(); ++i) {
		const Tokens& tokens = sharedData_->layoutManager.getLine(i).getTokens();
		const String& line = getDocument().getLine(i);
		for(size_t j = 0; j < tokens.count; ++j) {
			const Token& token = tokens.array[j];
			if(token.getType() == Token::IDENTIFIER) {
				if(j < tokens.count - 1)
					candidateWords.insert(line.substr(token.getIndex(),
						tokens.array[j + 1].getIndex() - token.getIndex()));
				else /* if(j == tokens.count - j) */ {
					candidateWords.insert(line.substr(token.getIndex()));
					break;
				}
			}
		}
	}
	// キーワードからも収集 -> やーめた
/*	const KeywordsMap& keywords = getLexer().getKeywords();
	for(KeywordsMap::const_iterator it = keywords.begin(); it != keywords.end(); ++it) {
		for(KeywordSet::const_iterator word = it->second.begin(); word != it->second.end(); ++word)
			candidateWords.insert(*word);
	}
*/
	// 候補が1つも無ければ終了
	if(candidateWords.empty()) {
		beep();
		return;
	}

	// 開始
	if(!completionWindow_->isWindow())
		completionWindow_->create();
	completionWindow_->start(candidateWords);

	if(completionWindow_->updateListCursel())	// 候補が絞られた -> 候補ウィンドウを出さずに補完
		completionWindow_->complete();
	else {
		// 位置決め (まだ不完全)
		POINT caretPoint;	// キャレット位置
		Rect clientRect;	// クライアント矩形
		RECT listRect;		// 補完ウィンドウの矩形

		getClientRect(clientRect);
		::GetCaretPos(&caretPoint);

		const int IDEAL_WIDTH = 170;						// 根拠の無い値
		const int idealHeight = clientRect.getHeight() / 3;	// 適当な値
		const bool rtl = isTextDirectionRTL();

		// 水平位置と幅
		(rtl ? listRect.right : listRect.left) = caretPoint.x;
		(rtl ? listRect.left : listRect.right) = rtl ? (listRect.right - IDEAL_WIDTH) : (listRect.left + IDEAL_WIDTH);
		if(!rtl && listRect.right > clientRect.right) {
			listRect.left = max(listRect.left - (listRect.right - clientRect.right), clientRect.left);
			listRect.right = clientRect.right;
		} else if(rtl && listRect.left < clientRect.left) {
			listRect.right = min(listRect.right - (listRect.left - clientRect.left), clientRect.right);
			listRect.left = clientRect.left;
		}

		// 垂直位置と高さ
		if(clientRect.bottom - (caretPoint.y + sharedData_->layoutManager.getLineHeight()) >= idealHeight) {	// キャレットの下に表示
			listRect.top = caretPoint.y + sharedData_->layoutManager.getLineHeight();
			listRect.bottom = listRect.top + idealHeight;
		} else if(caretPoint.y - clientRect.top >= idealHeight) {	// キャレットの上に表示
			listRect.top = caretPoint.y - idealHeight;
			listRect.bottom = caretPoint.y;
		} else if(clientRect.bottom - (caretPoint.y + sharedData_->layoutManager.getLineHeight()) >= caretPoint.y - clientRect.top) {
			listRect.top = caretPoint.y + sharedData_->layoutManager.getLineHeight();
			listRect.bottom = clientRect.bottom;
		} else {
			listRect.top = clientRect.top;
			listRect.bottom = caretPoint.y;
		}

		completionWindow_->modifyStyleEx(rtl ? 0 : WS_EX_LAYOUTRTL, rtl ? WS_EX_LAYOUTRTL : 0);
		completionWindow_->moveWindow(listRect, false);
		completionWindow_->setFont(
			sharedData_->options.appearance[USE_EDITOR_FONT_FOR_COMPLETION] ?
				sharedData_->layoutManager.getRegularFont() : 0);
		completionWindow_->showWindow(SW_SHOW);
	}
#endif
	return false;
}
#endif

/**
 * Starts rectangular selection.
 * @see #endBoxSelection, #isSelectionRectangle
 */
void Caret::beginBoxSelection() {
	verifyViewer();
	if(box_ == 0) {
		box_ = new VirtualBox(getTextViewer(), getSelectionRegion());
		listeners_.notify<const Caret&>(ICaretListener::selectionShapeChanged, *this);
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
		listeners_.notify<const Caret&, const pair<Position, Position>&, bool>(ICaretListener::matchBracketsChanged, *this, oldPair, false);
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
	const String s = getSelectionText(LBR_PHYSICAL_DATA);
	Clipboard(getTextViewer()).write(s, isSelectionRectangle());
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

/// @see VisualPoint#doMoveTo
void Caret::doMoveTo(const Position& to) {
	const Region oldRegion(anchor_->isInternalUpdating() ?
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
	if(isSelectionRectangle())
		box_->update(getSelectionRegion());
	if((oldRegion.first != getPosition() || oldRegion.second != getPosition()))
		listeners_.notify<const Caret&, const Region&>(ICaretListener::caretMoved, *this, oldRegion);
	if(autoShow_)
		show();
	checkMatchBrackets();
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
		listeners_.notify<const Caret&>(ICaretListener::selectionShapeChanged, *this);
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
	else if(!isSelectionRectangle())	// 線形
		moveTo(document.deleteText(*anchor_, *this));
	else {	// 矩形
		const length_t topLine = getTopPoint().getLineNumber();
		const length_t bottomLine = getBottomPoint().getLineNumber();
		length_t first, last;
		Position resultPosition;
		const bool adapts = adaptsToDocument();

		adaptToDocument(false);
		for(length_t line = topLine; line <= bottomLine; ++line) {
			box_->getOverlappedSubline(line, 0, first, last);	// TODO: recognize wrap (second parameter).
			resultPosition = document.deleteText(Position(line, first), Position(line, last));
		}
		endBoxSelection();
		adaptToDocument(adapts);
		moveTo(resultPosition);
	}
}

/**
 * アンカーを移動させずにキャレットを移動させる
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
 * アンカーを移動させずにキャレットを移動させる
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
 * アンカーを移動させずにキャレットを移動させる
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
 * アンカーを移動させずにキャレットを移動させる
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
 * アンカーを移動させずにキャレットを移動させる
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
 * @param[out] last the end of the range
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
	last = (line == bottom.line) ? bottom.column : getDocument()->getLineLength(line);
	return true;
}

/**
 * Returns the selected range on the specified visual line,
 * @param line the logical line
 * @param subline the visual subline
 * @param[out] first the start of the range
 * @param[out] last the end of the range
 * @return true if there is selected range on the line
 * @throw text#BadPositionException @a line or @a subline is outside of the document
 * @see #getSelectedRangeOnLine
 */
bool Caret::getSelectedRangeOnVisualLine(length_t line, length_t subline, length_t& first, length_t& last) const {
	verifyViewer();
	if(!isSelectionRectangle()) {
		if(!getSelectedRangeOnLine(line, first, last))
			return false;
		const LineLayout& layout = getLayout(line);
		const length_t sublineOffset = layout.getSublineOffset(subline);
		first = max(first, sublineOffset);
		last = min(last, sublineOffset + layout.getSublineLength(subline));
		return first != last;
	} else
		return box_->getOverlappedSubline(line, subline, first, last);
}


/**
 * Returns the preceding identifier.
 * Fails if the caret has selection or the number of scanned characters exceeded @a maxLength.
 * @param maxLength the maximum length of the identifier to find
 * @return the identifier or an empty string if failed
 * @deprecated 0.8
 */
String Caret::getPrecedingIdentifier(length_t maxLength) const {
	verifyViewer();
	if(!isSelectionEmpty() || isStartOfLine() || maxLength == 0)
		return L"";

	DocumentPartition partition;
	getDocument()->getPartitioner().getPartition(*this, partition);
	const length_t partitionStart = (partition.region.getTop().line == getLineNumber()) ? partition.region.getTop().column : 0;
	if(partitionStart == getColumnNumber())	// どちらのパーティションに属するか微妙だ...
		return L"";

	const IdentifierSyntax& syntax = getIdentifierSyntax();
	const String& line = getDocument()->getLine(getLineNumber());
	assert(getColumnNumber() > 0);
	UTF16To32Iterator<const Char*, utf16boundary::USE_BOUNDARY_ITERATORS> i(
		line.data() + getColumnNumber(), line.data() + partitionStart, line.data() + line.length());
	for(--i; !i.isFirst(); --i) {
		if(!syntax.isIdentifierContinueCharacter(*i))
			break;
		else if(getColumnNumber() - (i.tell() - line.data()) > maxLength)
			return L"";
	}
	return String(i.tell(), getColumnNumber() - (i.tell() - line.data()));
}

/**
 * Returns the selected text.
 * @param lbr 改行の扱い。矩形選択の場合はドキュメントの既定の改行が使われる
 * @return the text
 */
String Caret::getSelectionText(LineBreakRepresentation lbr /* = LBR_PHYSICAL_DATA */) const {
	verifyViewer();

	if(isSelectionEmpty())
		return L"";
	else if(!isSelectionRectangle())	// 矩形選択でない場合
		return getTopPoint().getText(getBottomPoint(), lbr);

	// 矩形選択の場合
	StringBuffer s(ios_base::out);
	const length_t bottomLine = getBottomPoint().getLineNumber();
	length_t first, last;
	for(length_t line = getTopPoint().getLineNumber(); line <= bottomLine; ++line) {
		const Document::Line& ln = getDocument()->getLineInfo(line);
		box_->getOverlappedSubline(line, 0, first, last);	// TODO: recognize wrap (second parameter).
		s.sputn(ln.getLine().data() + first, static_cast<streamsize>(last - first));
		s.sputn(getLineBreakString(ln.getLineBreak()), static_cast<streamsize>(getLineBreakLength(ln.getLineBreak())));
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
 * @see #isOvertypeMode, #setOvertypeMode, StandardCommand#TextInputCommand
 */
bool Caret::inputCharacter(CodePoint cp, bool validateSequence /* = true */, bool blockControls /* = true */) {
	verifyViewer();

	Document& document = *getDocument();
	if(document.isReadOnly())
		return false;
	else if(blockControls && cp <= 0xFF && cp != 0x09 && cp != 0x1E && cp != 0x1F && toBoolean(iscntrl(static_cast<int>(cp))))
		return false;

	// 入力シーケンスのチェック
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
	if(!isSelectionEmpty())	// 選択がある場合 -> 置換するだけ
		replaceSelection(buffer, buffer + ((cp < 0x10000) ? 1 : 2));
	else if(overtypeMode_) {	// 上書きモード
		if(!document.isSequentialEditing())
			document.beginSequentialEdit();
		getTextViewer().freeze(true);
		destructiveInsert(buffer, buffer + ((cp < 0x10000) ? 1 : 2));
		getTextViewer().unfreeze(true);
	} else {
		const IdentifierSyntax& ctypes = getIdentifierSyntax();
		const bool alpha = ctypes.isIdentifierContinueCharacter(cp);

//		// 識別子文字以外なら補完終了
//		if(!alpha && completionWindow_.isRunning())
//			completionWindow_.complete();

		// 後続の入力を1つの連続編集にまとめる準備
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
		if(getTextViewer().hitTest(pt) != TextViewer::TEXT_AREA)	// マージン上であれば無視
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
		if(isStartOfLine())	// 0 文字行
			moveTo(*this);
		else	// 行末
			select((--i).base().tell(), *this);
	} else if(isStartOfLine())	// 行頭
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
		listeners_.notify<const Caret&>(ICaretListener::overtypeModeChanged, *this);
	}
}

/// @see Point#update
void Caret::update(const DocumentChange& change) {
	// ドキュメントが変更されたときに、
	// アンカーとキャレットの移動を同時に (一度に) 通知するための細工
	leaveAnchorNext_ = leadingAnchor_ = true;
	anchor_->beginInternalUpdate(change);
	Point::update(change);
	anchor_->endInternalUpdate();
	leaveAnchorNext_ = leadingAnchor_ = false;
}
