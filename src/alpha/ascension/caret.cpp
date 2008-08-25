/**
 * @file caret.cpp
 * @author exeal
 * @date 2003-2008 was point.cpp
 * @date 2008 separated from point.cpp
 */

#include "viewer.hpp"
#include "session.hpp"
#include "../../manah/win32/utility.hpp"
using namespace ascension;
using namespace ascension::kernel;
using namespace ascension::layout;
using namespace ascension::viewers;
using namespace ascension::presentation;
using namespace ascension::text;
using namespace ascension::text::ucd;
using namespace manah::win32;
using namespace std;
using manah::toBoolean;


namespace {
	/// The clipboard.
	class Clipboard {
		MANAH_NONCOPYABLE_TAG(Clipboard);
	public:
		class Text {
			MANAH_UNASSIGNABLE_TAG(Text);
		public:
			Text(::HGLOBAL handle, const Char* text) throw() : handle_(handle), text_(text) {}
			Text(const Text& rhs) throw() : handle_(rhs.handle_), text_(rhs.text_) {const_cast<Text&>(rhs).handle_ = 0;}
			~Text() throw() {if(handle_ != 0) ::GlobalUnlock(handle_);}
			const Char* data() const throw() {return text_;}
			length_t rawSize() const throw() {return (handle_ != 0) ? ::GlobalSize(handle_) : 0;}
			operator bool() const throw() {return handle_ != 0 && text_ != 0;}
		private:
			::HGLOBAL handle_;
			const Char* const text_;
		};
		Clipboard(::HWND window) throw();
		~Clipboard() throw() {if(opened_) ::CloseClipboard();}
		bool isOpen() const throw() {return opened_;}
		Text read() throw();
		void write(const Char* first, const Char* last, bool asRectangle = false) throw();
		void write(const String& s, bool asRectangle = false) throw() {write(s.data(), s.data() + s.length(), asRectangle);}
	private:
		bool opened_;
	};

	/**
	 * Constructor opens the clipboard.
	 * @param window the owner of the clipboard
	 */
	Clipboard::Clipboard(::HWND window) throw() : opened_(false) {
		for(int i = 0; i < 100; ++i) {
			if(opened_ = toBoolean(::OpenClipboard(window)))
				break;
			::Sleep(0);
		}
	}

	/**
	 * Reads the text from the clipboard.
	 * @return the text
	 */
	Clipboard::Text Clipboard::read() throw() {
		assert(isOpen());
		if(::HGLOBAL data = ::GetClipboardData(CF_UNICODETEXT))
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
					if(const ::UINT clipFormat = ::RegisterClipboardFormatW(ASCENSION_RECTANGLE_TEXT_CLIP_FORMAT)) {
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
	class TextObject : virtual public ::IDataObject {
	public:
		// constructor
		TextObject(const String& plainContent, bool rectangle, const string& rtfContent);
		// IUnknown
		MANAH_IMPLEMENT_UNKNOWN_SINGLE_THREADED()
		MANAH_BEGIN_INTERFACE_TABLE()
			MANAH_IMPLEMENTS_LEFTMOST_INTERFACE(IDataObject)
		MANAH_END_INTERFACE_TABLE()
		// IDataObject
		STDMETHODIMP GetData(::LPFORMATETC pformatetcIn, ::LPSTGMEDIUM pmedium);
		STDMETHODIMP GetDataHere(::LPFORMATETC, ::LPSTGMEDIUM) {return E_NOTIMPL;}
		STDMETHODIMP QueryGetData(::LPFORMATETC pformatetc);
		STDMETHODIMP GetCanonicalFormatEtc(::LPFORMATETC, ::LPFORMATETC) {return DATA_S_SAMEFORMATETC;}
		STDMETHODIMP SetData(::LPFORMATETC, ::LPSTGMEDIUM, ::BOOL) {return E_NOTIMPL;}
		STDMETHODIMP EnumFormatEtc(::DWORD dwDirection, ::LPENUMFORMATETC* ppenumFormatEtc);
		STDMETHODIMP DAdvise(::LPFORMATETC, ::DWORD, ::LPADVISESINK, ::LPDWORD) {return OLE_E_ADVISENOTSUPPORTED;}
		STDMETHODIMP DUnadvise(::DWORD) {return OLE_E_ADVISENOTSUPPORTED;}
		STDMETHODIMP EnumDAdvise(::LPENUMSTATDATA*) {return OLE_E_ADVISENOTSUPPORTED;}
	private:
		class FormatEnumerator : virtual public IEnumFORMATETC {
		public:
			explicit FormatEnumerator(const list<::CLIPFORMAT>& formats) : formats_(formats) {Reset();}
			// IUnknown
			MANAH_IMPLEMENT_UNKNOWN_SINGLE_THREADED()
			MANAH_BEGIN_INTERFACE_TABLE()
				MANAH_IMPLEMENTS_LEFTMOST_INTERFACE(IEnumFORMATETC)
			MANAH_END_INTERFACE_TABLE()
			// IEnumFORMATETC
			STDMETHODIMP Next(::ULONG celt, ::FORMATETC* rgelt, ::ULONG* pceltFetched);
			STDMETHODIMP Skip(::ULONG celt);
			STDMETHODIMP Reset() {current_ = formats_.begin(); return S_OK;}
			STDMETHODIMP Clone(::IEnumFORMATETC** ppenum);
		private:
			const list<::CLIPFORMAT> formats_;
			list<::CLIPFORMAT>::const_iterator current_;
		};
	private:
		const String unicodeContent_;
		auto_ptr<string> ansiContent_;
		auto_ptr<const string> rtfContent_;
		bool rectangle_;
		static ::CLIPFORMAT rectangleFormatInteger_, rtfInteger_, rtfWithoutObjectsInteger_;
	};

	::CLIPFORMAT TextObject::rectangleFormatInteger_, TextObject::rtfInteger_, TextObject::rtfWithoutObjectsInteger_;

	TextObject::TextObject(const String& plainContent, bool rectangle, const string& rtfContent)
			: unicodeContent_(plainContent), rtfContent_(new string(rtfContent)), rectangle_(rectangle) {
		if(rectangleFormatInteger_ == 0) {
			rectangleFormatInteger_ = ::RegisterClipboardFormatW(ASCENSION_RECTANGLE_TEXT_CLIP_FORMAT);
			rtfInteger_ = ::RegisterClipboardFormatW(L"Rich Text Format");									// CF_RTF
			rtfWithoutObjectsInteger_ = ::RegisterClipboardFormatW(L"Rich Text Format Without Objects");	// CF_RTFNOOBJS
		}
	}

	STDMETHODIMP TextObject::EnumFormatEtc(::DWORD dwDirection, ::LPENUMFORMATETC* ppenumFormatEtc) {
		if(dwDirection == DATADIR_SET)
			return E_NOTIMPL;
		else if(dwDirection != DATADIR_GET)
			return E_INVALIDARG;
		else if(ppenumFormatEtc == 0)
			return E_INVALIDARG;
		list<::CLIPFORMAT> formats;
		formats.push_back(CF_UNICODETEXT);
		formats.push_back(CF_TEXT);
		formats.push_back(CF_LOCALE);
		if(rectangle_)
			formats.push_back(rectangleFormatInteger_);
		if(rtfContent_.get() != 0) {
			formats.push_back(rtfInteger_);
			formats.push_back(rtfWithoutObjectsInteger_);
		}
		if(*ppenumFormatEtc = new(nothrow) FormatEnumerator(formats))
			return (*ppenumFormatEtc)->AddRef(), S_OK;
		return E_OUTOFMEMORY;
	}

	STDMETHODIMP TextObject::GetData(::LPFORMATETC pformatetcIn, ::LPSTGMEDIUM pmedium) {
		if(pformatetcIn == 0 || pmedium == 0)
			return E_INVALIDARG;
		if(pformatetcIn->dwAspect != DVASPECT_CONTENT
				|| pformatetcIn->lindex != -1
				|| !toBoolean(pformatetcIn->tymed & TYMED_HGLOBAL))
			return DV_E_FORMATETC;

		if(pformatetcIn->cfFormat == CF_UNICODETEXT || pformatetcIn->cfFormat == rectangleFormatInteger_) {
			if(0 == (pmedium->hGlobal = ::GlobalAlloc(GHND | GMEM_SHARE, sizeof(Char) * (unicodeContent_.length() + 1))))
				return E_OUTOFMEMORY;
			wcscpy(static_cast<wchar_t*>(::GlobalLock(pmedium->hGlobal)), unicodeContent_.c_str());
			::GlobalUnlock(pmedium->hGlobal);
		} else if(pformatetcIn->cfFormat == CF_TEXT) {
			if(ansiContent_.get() == 0) {
				// convert Unicode content into ANSI one
				int ansiLength = ::WideCharToMultiByte(CP_ACP, WC_SEPCHARS | WC_DEFAULTCHAR,
					unicodeContent_.data(), static_cast<int>(unicodeContent_.length()), 0, 0, 0, 0);
				if(ansiLength == 0)
					return DV_E_FORMATETC;
				manah::AutoBuffer<char> ansiBuffer(new(nothrow) char[ansiLength]);
				if(ansiBuffer.get() == 0)
					return E_OUTOFMEMORY;
				ansiLength = ::WideCharToMultiByte(CP_ACP, WC_SEPCHARS | WC_DEFAULTCHAR,
					unicodeContent_.data(), static_cast<int>(unicodeContent_.length()), ansiBuffer.get(), ansiLength, 0, 0);
				try {
					ansiContent_.reset(new string(ansiBuffer.get(), ansiLength));
				} catch(bad_alloc&) {
					return E_OUTOFMEMORY;
				} catch(...) {
				}
				if(ansiContent_.get() == 0)
					return DV_E_FORMATETC;
			}
			if(0 == (pmedium->hGlobal = ::GlobalAlloc(GHND | GMEM_SHARE, sizeof(char) * (ansiContent_->length() + 1))))
				return E_OUTOFMEMORY;
			strcpy(static_cast<char*>(::GlobalLock(pmedium->hGlobal)), ansiContent_->c_str());
			::GlobalUnlock(pmedium->hGlobal);
		} else if(pformatetcIn->cfFormat == CF_LOCALE) {
			if(0 == (pmedium->hGlobal = ::GlobalAlloc(GHND | GMEM_SHARE, sizeof(::LCID))))
				return E_OUTOFMEMORY;
			*static_cast<::LCID*>(::GlobalLock(pmedium->hGlobal)) = ::GetUserDefaultLCID();
			::GlobalUnlock(pmedium->hGlobal);
		} else if(pformatetcIn->cfFormat == rtfInteger_) {
			// TODO: implement.
		} else if(pformatetcIn->cfFormat == rtfWithoutObjectsInteger_) {
			// TODO: implement.
		}

		pmedium->tymed = TYMED_HGLOBAL;
		pmedium->pUnkForRelease = 0;

		return S_OK;
	}


	STDMETHODIMP TextObject::QueryGetData(::LPFORMATETC pformatetc) {
		if(pformatetc == 0)
			return E_INVALIDARG;
		else if(pformatetc->lindex != -1)
			return DV_E_LINDEX;
		else if(!toBoolean(pformatetc->tymed & TYMED_HGLOBAL))
			return DV_E_TYMED;
		else if(pformatetc->dwAspect != DVASPECT_CONTENT)
			return DV_E_DVASPECT;
		switch(pformatetc->cfFormat) {
		case CF_TEXT:
		case CF_LOCALE:
		case CF_UNICODETEXT:
			break;
		default:
			if(pformatetc->cfFormat == rectangleFormatInteger_ && !rectangle_
					|| pformatetc->cfFormat == rtfInteger_ && rtfContent_.get() == 0
					|| pformatetc->cfFormat == rtfWithoutObjectsInteger_ && rtfContent_.get() == 0)
				return DV_E_FORMATETC;
		}
		return S_OK;
	}

	STDMETHODIMP TextObject::FormatEnumerator::Clone(IEnumFORMATETC** ppenum) {
		MANAH_VERIFY_POINTER(ppenum);
		if(*ppenum = new(nothrow) FormatEnumerator(formats_))
			return (*ppenum)->AddRef(), S_OK;
		return E_OUTOFMEMORY;
	}

	STDMETHODIMP TextObject::FormatEnumerator::Next(::ULONG celt, ::FORMATETC* rgelt, ::ULONG* pceltFetched) {
		if(celt > 1 && pceltFetched == 0)
			return E_INVALIDARG;

		::ULONG fetched = 0;
		for(list<::CLIPFORMAT>::const_iterator e(formats_.end()); fetched < celt && current_ != e; ++fetched, ++current_) {
			rgelt[fetched].cfFormat = *current_;
			rgelt[fetched].ptd = 0;
			rgelt[fetched].dwAspect = DVASPECT_CONTENT;
			rgelt[fetched].lindex = -1;
			rgelt[fetched].tymed = TYMED_HGLOBAL;
			
		}
		if(pceltFetched != 0)
			*pceltFetched = fetched;

		return (fetched == celt) ? S_OK : S_FALSE;
	}

	STDMETHODIMP TextObject::FormatEnumerator::Skip(::ULONG celt) {
		for(list<::CLIPFORMAT>::const_iterator e(formats_.end()); celt != 0 && current_ != e; ++current_)
			--celt;
		return celt == 0 ? S_OK : S_FALSE;
	}
} // namespace @0

/**
 * Creates an IDataObject represents the specified region.
 * @param viewer the text viewer
 * @param region the region
 * @param rectangle set true if the content is rectangle
 * @param rtf set true if the content is available as Rich Text Format
 * @param[out] content
 */
void viewers::makeRegionTextObject(const TextViewer& viewer, const Region& region, bool rectangle, bool rtf, ::IDataObject*& content) {
	basic_ostringstream<Char> s;
	writeDocumentToStream(s, viewer.document(), region, NLF_CR_LF);
	const String text(s.str());

	// 
	::HGLOBAL richContent = 0;
	if(rtf) {
	}

//	if(content = new(nothrow) TextObject(text, rectangle, richContent))
//		content->AddRef();
//	else
//		throw bad_alloc();
}


// VisualPoint //////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param viewer the viewer
 * @param position the initial position of the point
 * @param listener the listener. can be @c null
 * @throw BadPositionException @a position is outside of the document
 */
VisualPoint::VisualPoint(TextViewer& viewer, const Position& position /* = Position() */, IPointListener* listener /* = 0 */) :
		EditPoint(viewer.document(), position, listener),viewer_(&viewer),
		clipboardNativeEncoding_(encoding::Encoder::getDefault().properties().name()),
		lastX_(-1), crossingLines_(false), visualLine_(INVALID_INDEX), visualSubline_(0) {
	static_cast<kernel::internal::IPointCollection<VisualPoint>&>(viewer).addNewPoint(*this);
	viewer_->textRenderer().addVisualLinesListener(*this);
}

/// Copy-constructor.
VisualPoint::VisualPoint(const VisualPoint& rhs) : EditPoint(rhs), viewer_(rhs.viewer_),
		lastX_(rhs.lastX_), crossingLines_(false), visualLine_(rhs.visualLine_), visualSubline_(rhs.visualSubline_) {
	if(viewer_ == 0)
		throw DisposedViewerException();
	static_cast<kernel::internal::IPointCollection<VisualPoint>*>(viewer_)->addNewPoint(*this);
	viewer_->textRenderer().addVisualLinesListener(*this);
}

/// Destructor.
VisualPoint::~VisualPoint() throw() {
	if(viewer_ != 0) {
		static_cast<kernel::internal::IPointCollection<VisualPoint>*>(viewer_)->removePoint(*this);
		viewer_->textRenderer().removeVisualLinesListener(*this);
	}
}

/**
 * Moves to the beginning of the visual line.
 * @see EditPoint#beginningOfLine
 */
void VisualPoint::beginningOfVisualLine() {
	verifyViewer();
	const LineLayout& layout = textViewer().textRenderer().lineLayout(lineNumber());
	moveTo(Position(lineNumber(), layout.sublineOffset(layout.subline(columnNumber()))));
}

/**
 * Returns if a paste operation can be performed.
 * @return the pastable clipboard format or 0
 */
UINT VisualPoint::canPaste() {
	const UINT rectangleClipFormat = ::RegisterClipboardFormatW(ASCENSION_RECTANGLE_TEXT_CLIP_FORMAT);
	if(rectangleClipFormat != 0 && toBoolean(::IsClipboardFormatAvailable(rectangleClipFormat)))
		return rectangleClipFormat;
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
	const String text(getText(length));
	Clipboard(viewer_->getHandle()).write(text.data(), text.data() + text.length());
}

/**
 * Writes the specified region into the clipboard.
 * @param other もう1つの位置
 */
void VisualPoint::copy(const Position& other) {
	verifyViewer();
	const String text(getText(other));
	Clipboard(viewer_->getHandle()).write(text.data(), text.data() + text.length());
}

/**
 * Erases the specified region and writes into the clipboard.
 * @param length the number of the characters to delete
 */
void VisualPoint::cut(signed_length_t length) {
	verifyViewer();
	if(document()->isReadOnly())
		return;
	const String text(getText(length));
	Clipboard(viewer_->getHandle()).write(text.data(), text.data() + text.length());
	erase(length);
}

/**
 * Erases the specified region and writes into the clipboard.
 * @param other もう1つの位置
 */
void VisualPoint::cut(const Position& other) {
	verifyViewer();
	if(document()->isReadOnly())
		return;
	const String text(getText(other));
	Clipboard(viewer_->getHandle()).write(text.data(), text.data() + text.length());
	erase(other);
}

/**
 * Indents the specified region.
 * @param other もう1つの位置
 * @param character a character to make indents
 * @param rectangle set true for rectangular indents (will be ignored level is negative)
 * @param level the level of the indentation
 * @return 操作の結果 @a pos が移動するとよい位置
 */
Position VisualPoint::doIndent(const Position& other, Char character, bool rectangle, long level) {
	verifyViewer();

	Document& doc = *document();

	if(doc.isReadOnly() || level == 0)
		return other;

	const String indent = String(abs(level), character);
	const Region region(*this, other);

	if(region.beginning().line == region.end().line) {	// 選択が 1 行以内 -> 単純な文字挿入
		doc.erase(region);
		doc.insert(region.beginning(), indent);
		return position();
	}

	const Position oldPosition(position());
	Position otherResult(other);
	length_t line = region.beginning().line;
	const bool adapts = adaptsToDocument();

	adaptToDocument(false);

	// 最初の行を (逆) インデント
	if(level > 0) {
		doc.insert(Position(line, rectangle ? region.beginning().column : 0), indent);
		if(line == otherResult.line && otherResult.column != 0)
			otherResult.column += level;
		if(line == lineNumber() && columnNumber() != 0)
			moveTo(lineNumber(), columnNumber() + level);
	} else {
		const String& s = doc.line(line);
		length_t indentLength;
		for(indentLength = 0; indentLength < s.length(); ++indentLength) {
			// 空白類文字が BMP にしか無いという前提
			if(s[indentLength] == L'\t' && GeneralCategory::of(s[indentLength]) != GeneralCategory::SEPARATOR_SPACE)
				break;
		}
		if(indentLength > 0) {
			const length_t deleteLength = min<length_t>(-level, indentLength);
			doc.erase(Position(line, 0), Position(line, deleteLength));
			if(line == otherResult.line && otherResult.column != 0)
				otherResult.column -= deleteLength;
			if(line == lineNumber() && columnNumber() != 0)
				moveTo(lineNumber(), columnNumber() - deleteLength);
		}
	}

	// 選択のある全ての行を (逆) インデント
	if(level > 0) {
		for(++line; line <= region.end().line; ++line) {
			if(doc.lineLength(line) != 0 && (line != region.end().line || region.end().column > 0)) {
				length_t insertPosition = 0;
				if(rectangle) {
					length_t dummy;
					viewer_->caret().boxForRectangleSelection().overlappedSubline(line, 0, insertPosition, dummy);	// TODO: recognize wrap (second parameter).
				}
				doc.insert(Position(line, insertPosition), indent);
				if(line == otherResult.line && otherResult.column != 0)
					otherResult.column += level;
				if(line == lineNumber() && columnNumber() != 0)
					moveTo(lineNumber(), columnNumber() + level);
			}
		}
	} else {
		for(++line; line <= region.end().line; ++line) {
			const String& s = doc.line(line);
			length_t indentLength;
			for(indentLength = 0; indentLength < s.length(); ++indentLength) {
				// 空白類文字が BMP にしか無いという前提
				if(s[indentLength] == L'\t' && GeneralCategory::of(s[indentLength]) != GeneralCategory::SEPARATOR_SPACE)
					break;
			}
			if(indentLength > 0) {
				const length_t deleteLength = min<length_t>(-level, indentLength);
				doc.erase(Position(line, 0), Position(line, deleteLength));
				if(line == otherResult.line && otherResult.column != 0)
					otherResult.column -= deleteLength;
				if(line == lineNumber() && columnNumber() != 0)
					moveTo(lineNumber(), columnNumber() - deleteLength);
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
	if(lineNumber() == to.line && visualLine_ != INVALID_INDEX) {
		visualLine_ -= visualSubline_;
		const LineLayout* layout = viewer_->textRenderer().lineLayoutIfCached(to.line);
		visualSubline_ = (layout != 0) ? layout->subline(to.column) : 0;
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
	const LineLayout& layout = textViewer().textRenderer().lineLayout(lineNumber());
	const length_t subline = layout.subline(columnNumber());
	Position newPosition(lineNumber(), (subline < layout.numberOfSublines() - 1) ?
		layout.sublineOffset(subline + 1) : document()->lineLength(lineNumber()));
	if(layout.subline(newPosition.column) != subline)
		newPosition = getBackwardCharacterPosition(*document(), newPosition, characterUnit());
	moveTo(newPosition);
}

/// Moves to the first printable character in the line.
void VisualPoint::firstPrintableCharacterOfLine() {
	verifyViewer();
	const length_t line = min(lineNumber(),
		(isExcludedFromRestriction() ? document()->accessibleRegion() : document()->region()).second.line);
	const Char* const p = document()->line(line).data();
	moveTo(Position(line, identifierSyntax().eatWhiteSpaces(p, p + document()->lineLength(line), true) - p));
}

/// Moves to the first printable character in the visual line.
void VisualPoint::firstPrintableCharacterOfVisualLine() {
	verifyViewer();
	const length_t line = min(lineNumber(),
		(isExcludedFromRestriction() ? document()->accessibleRegion() : document()->region()).second.line);
	const String& s = document()->line(line);
	const LineLayout& layout = viewer_->textRenderer().lineLayout(line);
	const length_t subline = layout.subline(columnNumber());
	moveTo(Position(line,
		identifierSyntax().eatWhiteSpaces(s.begin() + layout.sublineOffset(subline),
			s.begin() + ((subline < layout.numberOfSublines() - 1) ? layout.sublineOffset(subline + 1) : s.length()), true) - s.begin()));
}

/// 
inline const IdentifierSyntax& VisualPoint::identifierSyntax() const {
	return document()->contentTypeInformation().getIdentifierSyntax(getContentType());
}

/**
 * Inserts the spcified text as a rectangle at the current position. This method has two
 * restrictions as the follows:
 * - If the text viewer is line wrap mode, this method inserts text as linear not rectangle.
 * - If the destination line is bidirectional, the insertion may be performed incorrectly.
 * @param first the start of the text
 * @param last the end of the text
 * @see kernel#EditPoint#insert
 */
void VisualPoint::insertRectangle(const Char* first, const Char* last) {
	verifyViewer();

	// HACK: 
	if(textViewer().configuration().lineWrap.wraps())
		return insert(first, last);

	Document& doc = *document();
	if(doc.isReadOnly() || first == last)
		return;

	const length_t numberOfLines = doc.numberOfLines();
	length_t line = lineNumber();
	const TextRenderer& renderer = textViewer().textRenderer();
	const int x = renderer.lineLayout(line).location(columnNumber()).x + renderer.lineIndent(line, 0);
	const IDocumentInput* const documentInput = doc.input();
	const String newline(getNewlineString((documentInput != 0) ? documentInput->newline() : ASCENSION_DEFAULT_NEWLINE));
	for(const Char* bol = first; ; ++line) {
		// find the next EOL
		const Char* const eol = find_first_of(bol, last, NEWLINE_CHARACTERS, MANAH_ENDOF(NEWLINE_CHARACTERS));

		// insert text if the source line is not empty
		if(eol > bol) {
			const LineLayout& layout = renderer.lineLayout(line);
			const length_t column = layout.offset(x - renderer.lineIndent(line), 0);
			String s(layout.fillToX(x));
			s.append(bol, eol);
			if(line >= numberOfLines - 1)
				s.append(newline);
			doc.insert(Position(line, column), s);
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
	const LineLayout& layout = textViewer().textRenderer().lineLayout(lineNumber());
	return columnNumber() == layout.sublineOffset(layout.subline(columnNumber()));
}

/**
 * Returns true if the point is end of the visual line.
 * @see kernel#EditPoint#isEndOfLine
 */
bool VisualPoint::isEndOfVisualLine() const {
	verifyViewer();
	if(isEndOfLine())
		return true;
	const LineLayout& layout = textViewer().textRenderer().lineLayout(lineNumber());
	const length_t subline = layout.subline(columnNumber());
	return columnNumber() == layout.sublineOffset(subline) + layout.sublineLength(subline);
}

/// Returns true if the current position is the first printable character in the line.
bool VisualPoint::isFirstPrintableCharacterOfLine() const {
	verifyViewer();
	normalize();
	const Position start((isExcludedFromRestriction() ? document()->accessibleRegion() : document()->region()).first);
	const length_t offset = (start.line == lineNumber()) ? start.column : 0;
	const String& line = document()->line(lineNumber());
	return line.data() + columnNumber() - offset
		== identifierSyntax().eatWhiteSpaces(line.data() + offset, line.data() + line.length(), true);
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
	const Position end((isExcludedFromRestriction() ? document()->accessibleRegion() : document()->region()).second);
	const String& line = document()->line(lineNumber());
	const length_t lineLength = (end.line == lineNumber()) ? end.column : line.length();
	return line.data() + lineLength - columnNumber()
		== identifierSyntax().eatWhiteSpaces(line.data() + columnNumber(), line.data() + lineLength, true);
}

/// Returns true if the current position is the last printable character in the visual line.
bool VisualPoint::isLastPrintableCharacterOfVisualLine() const {
	// TODO: not implemented.
	return false;
}

/// Moves to the last printable character in the line.
void VisualPoint::lastPrintableCharacterOfLine() {
	verifyViewer();
	const length_t line = min(lineNumber(),
		(isExcludedFromRestriction() ? document()->accessibleRegion() : document()->region()).second.line);
	const length_t lineLength = document()->lineLength(line);
	const Char* const p = document()->line(line).data();
	const IdentifierSyntax& syntax = identifierSyntax();

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
	(viewer_->configuration().orientation == LEFT_TO_RIGHT) ? backwardCharacter(offset) : forwardCharacter(offset);
}

/**
 * Moves to the beginning of the left word.
 * @param offset the number of words
 */
void VisualPoint::leftWord(length_t offset /* = 1 */) {
	verifyViewer();
	(viewer_->configuration().orientation == LEFT_TO_RIGHT) ? previousWord(offset) : nextWord(offset);
}

/**
 * Moves to the end of the left word.
 * @param offset the number of words
 */
void VisualPoint::leftWordEnd(length_t offset /* = 1 */) {
	verifyViewer();
	(viewer_->configuration().orientation == LEFT_TO_RIGHT) ? previousWordEnd(offset) : nextWordEnd(offset);
}

/**
 * Breaks the line.
 * @note This methos hides @c EditPoint#newLine (C++ rule).
 * @param inheritIndent true to inherit the indent of the previous line
 */
void VisualPoint::newLine(bool inheritIndent) {
	verifyViewer();
	if(document()->isReadOnly())
		return;

	const IDocumentInput* di = document()->input();
	String s(getNewlineString((di != 0) ? di->newline() : ASCENSION_DEFAULT_NEWLINE));

	if(inheritIndent) {	// 自動インデント
		const String& currentLine = document()->line(lineNumber());
		const length_t len = identifierSyntax().eatWhiteSpaces(
			currentLine.data(), currentLine.data() + columnNumber(), true) - currentLine.data();
		s += currentLine.substr(0, len);
	}
	insert(s);
}

/**
 * Moves to the next page.
 * @param offset the offset of the movement
 */
void VisualPoint::nextPage(length_t offset /* = 1 */) {
	verifyViewer();
	// TODO: calculate exact number of visual lines.
	nextVisualLine(viewer_->numberOfVisibleLines() * offset);
}

/**
 * Moves to the next visual line.
 * @param offset the offset of the movement
 */
void VisualPoint::nextVisualLine(length_t offset /* = 1 */) {
	verifyViewer();
	normalize();
	const TextRenderer& renderer = viewer_->textRenderer();
	const LineLayout* layout = &renderer.lineLayout(lineNumber());
	length_t line = lineNumber(), subline = layout->subline(columnNumber());
	if(line == document()->numberOfLines() - 1 && subline == layout->numberOfSublines() - 1)
		return;
	if(lastX_ == -1)
		updateLastX();
	renderer.offsetVisualLine(line, subline, static_cast<signed_length_t>(offset));
	layout = &renderer.lineLayout(line);
	Position newPosition(line, layout->offset(lastX_ - renderer.lineIndent(line), renderer.linePitch() * static_cast<long>(subline)));
	if(layout->subline(newPosition.column) != subline)
		newPosition = getBackwardCharacterPosition(*document(), newPosition, characterUnit());
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
		DocumentCharacterIterator(*document(), *this), AbstractWordBreakIterator::START_OF_SEGMENT, identifierSyntax());
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
		DocumentCharacterIterator(*document(), *this), AbstractWordBreakIterator::END_OF_SEGMENT, identifierSyntax());
	i += offset;
	moveTo(i.base().tell());
}

/**
 * Replaces the specified region by the content of the clipboard.
 * @param length the number of characters to be replaced
 */
void VisualPoint::paste(signed_length_t length /* = 0 */) {
	verifyViewer();
	if(document()->isReadOnly() || length == 0) {
		paste(position());
		return;
	}
	paste((length > 0) ?
		getForwardCharacterPosition(*document(), *this, characterUnit(), length) : getBackwardCharacterPosition(*document(), *this, UTF16_CODE_UNIT, -length));
}

/**
 * Replaces the specified region by the content of the clipboard.
 * @param other もう1つの位置
 */
void VisualPoint::paste(const Position& other) {
	verifyViewer();

	if(document()->isReadOnly())
		return;
	else if(const UINT availableClipFormat = canPaste()) {
		if(other != position())
			erase(other);

		Clipboard clipboard(viewer_->getHandle());
		if(Clipboard::Text text = clipboard.read()) {
			const Char* const data = text.data();
			if(availableClipFormat == ::RegisterClipboardFormatW(ASCENSION_RECTANGLE_TEXT_CLIP_FORMAT))
				insertRectangle(data, data + wcslen(data));
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
	previousVisualLine(viewer_->numberOfVisibleLines() * offset);
}

/**
 * Moves to the previous visual line.
 * @param offset the offset of the movement
 */
void VisualPoint::previousVisualLine(length_t offset /* = 1 */) {
	verifyViewer();
	normalize();
	const TextRenderer& renderer = viewer_->textRenderer();
	length_t line = lineNumber(), subline = renderer.lineLayout(line).subline(columnNumber());
	if(line == 0 && subline == 0)
		return;
	if(lastX_ == -1)
		updateLastX();
	renderer.offsetVisualLine(line, subline, -static_cast<signed_length_t>(offset));
	const LineLayout& layout = renderer.lineLayout(line);
	Position newPosition(line, layout.offset(lastX_ - renderer.lineIndent(line), renderer.linePitch() * static_cast<long>(subline)));
	if(layout.subline(newPosition.column) != subline)
		newPosition = getBackwardCharacterPosition(*document(), newPosition, characterUnit());
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
		DocumentCharacterIterator(*document(), *this), AbstractWordBreakIterator::START_OF_SEGMENT, identifierSyntax());
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
		DocumentCharacterIterator(*document(), *this), AbstractWordBreakIterator::END_OF_SEGMENT, identifierSyntax());
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
		getForwardCharacterPosition(*document(), *this, characterUnit(), length)
		: getBackwardCharacterPosition(*document(), *this, characterUnit(), -length));
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
	(viewer_->configuration().orientation == LEFT_TO_RIGHT) ? forwardCharacter(offset) : backwardCharacter(offset);
}

/**
 * Moves to the beginning of the right word.
 * @param offset the number of words
 */
void VisualPoint::rightWord(length_t offset /* = 1 */) {
	verifyViewer();
	(viewer_->configuration().orientation == LEFT_TO_RIGHT) ? nextWord(offset) : previousWord(offset);
}

/**
 * Moves to the end of the right word.
 * @param offset the number of words
 */
void VisualPoint::rightWordEnd(length_t offset /* = 1 */) {
	verifyViewer();
	(viewer_->configuration().orientation == LEFT_TO_RIGHT) ? nextWordEnd(offset) : previousWordEnd(offset);
}

/**
 * 指定範囲が可視になるようにビューをスクロールする
 * @param length 範囲を構成するもう一方の点までの文字数
 * @return 範囲がビューに納まる場合は true を返す (未実装につき常に true)
 */
bool VisualPoint::show(signed_length_t length /* = 0 */) {
	verifyDocument();
	return show((length >= 0) ?
		getForwardCharacterPosition(*document(), *this, characterUnit(), length)
		: getBackwardCharacterPosition(*document(), *this, characterUnit(), -length));
}

/**
 * 点が可視になるようにビューをスクロールする
 * @param other 範囲を構成するもう一方の点
 * @return 範囲がビューに納まる場合は true を返す (未実装につき常に true)
 */
bool VisualPoint::show(const Position& other) {
	verifyViewer();

	const TextRenderer& renderer = viewer_->textRenderer();
	const length_t visibleLines = viewer_->numberOfVisibleLines();
	MANAH_AUTO_STRUCT_SIZE(::SCROLLINFO, si);
	::POINT to = {-1, -1};

	// for vertical direction
	if(visualLine_ == INVALID_INDEX) {
		visualLine_ = viewer_->textRenderer().mapLogicalLineToVisualLine(lineNumber());
		visualSubline_ = renderer.lineLayout(lineNumber()).subline(columnNumber());
		visualLine_ += visualSubline_;
	}
	si.fMask = SIF_POS;
	viewer_->getScrollInformation(SB_VERT, si);
	if(visualLine_ < si.nPos * viewer_->scrollRate(false))	// 画面より上
		to.y = static_cast<long>(visualLine_ * viewer_->scrollRate(false));
	else if(visualLine_ - si.nPos * viewer_->scrollRate(false) > visibleLines - 1)	// 画面より下
		to.y = static_cast<long>((visualLine_ - visibleLines + 1) * viewer_->scrollRate(false));
	if(to.y < -1)
		to.y = 0;

	// for horizontal direction
	if(!viewer_->configuration().lineWrap.wrapsAtWindowEdge()) {
		const length_t visibleColumns = viewer_->numberOfVisibleColumns();
		const ulong x = renderer.lineLayout(lineNumber()).location(columnNumber(), LineLayout::LEADING).x + renderer.lineIndent(lineNumber(), 0);
		viewer_->getScrollInformation(SB_HORZ, si);
		const ulong scrollOffset = si.nPos * viewer_->scrollRate(true) * renderer.averageCharacterWidth();
		if(x <= scrollOffset)	// 画面より左
			to.x = x / renderer.averageCharacterWidth() - visibleColumns / 4;
		else if(x >= (si.nPos * viewer_->scrollRate(true) + visibleColumns) * renderer.averageCharacterWidth())	// 画面より右
			to.x = x / renderer.averageCharacterWidth() - visibleColumns * 3 / 4;
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
 * @param rectangle set true for rectangular indents (will be ignored level is negative)
 * @param level the level of the indentation
 * @return インデント後に @a pos が移動すべき位置
 */
Position VisualPoint::spaceIndent(const Position& other, bool rectangle, long level /* = 1 */) {
	verifyViewer();
	return doIndent(other, L' ', rectangle, level);
}

/**
 * Indents the specified region by using horizontal tabs.
 * @param other もう1つの位置
 * @param rectangle set true for rectangular indents (will be ignored level is negative)
 * @param level the level of the indentation
 * @return インデント後に @a pos が移動すべき位置
 */
Position VisualPoint::tabIndent(const Position& other, bool rectangle, long level /* = 1 */) {
	verifyViewer();
	return doIndent(other, L'\t', rectangle, level);
}

/**
 * Transposes the two grapheme clusters on either side of the point.
 * If the point is not start of a cluster, this method fails.
 * If the transposing target is not in the current line, this method fails.
 * @return false if failed
 */
bool VisualPoint::transposeCharacters() {
	verifyViewer();
	if(document()->isReadOnly())
		return false;

	// As transposing characters in string "ab":
	//
	//  a b -- transposing clusters 'a' and 'b'. result is "ba"
	// ^ ^ ^
	// | | next-cluster (named pos[2])
	// | middle-cluster (named pos[1]; usually current-position)
	// previous-cluster (named pos[0])

	Position pos[3];
	const Region region(document()->accessibleRegion());

	if(BinaryProperty::is<BinaryProperty::GRAPHEME_EXTEND>(getCodePoint()))	// not the start of a grapheme
		return false;
	else if(!region.includes(position()))	// inaccessible
		return false;

	if(columnNumber() == 0 || position() == region.first) {
		GraphemeBreakIterator<DocumentCharacterIterator> i(DocumentCharacterIterator(*document(), pos[0] = position()));
		pos[1] = (++i).base().tell();
		if(pos[1].line != pos[0].line || pos[1] == pos[0] || !region.includes(pos[1]))
			return false;
		pos[2] = (++i).base().tell();
		if(pos[2].line != pos[1].line || pos[2] == pos[1] || !region.includes(pos[2]))
			return false;
	} else if(columnNumber() == document()->lineLength(lineNumber()) || position() == region.second) {
		GraphemeBreakIterator<DocumentCharacterIterator> i(DocumentCharacterIterator(*document(), pos[2] = position()));
		pos[1] = (--i).base().tell();
		if(pos[1].line != pos[2].line || pos[1] == pos[2] || !region.includes(pos[1]))
			return false;
		pos[0] = (--i).base().tell();
		if(pos[0].line != pos[1].line || pos[0] == pos[1] || !region.includes(pos[0]))
			return false;
	} else {
		GraphemeBreakIterator<DocumentCharacterIterator> i(DocumentCharacterIterator(*document(), pos[1] = position()));
		pos[2] = (++i).base().tell();
		if(pos[2].line != pos[1].line || pos[2] == pos[1] || !region.includes(pos[2]))
			return false;
		i.base().seek(pos[1]);
		pos[0] = (--i).base().tell();
		if(pos[0].line != pos[1].line || pos[0] == pos[1] || !region.includes(pos[0]))
			return false;
	}

	moveTo(lineNumber(), pos[1].column);
	String s = getText(pos[2]);
	moveTo(lineNumber(), pos[0].column);
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

	if(document()->isReadOnly())
		return false;

	const Region region(document()->accessibleRegion());
	if(region.first.line == region.second.line)	// there is just one line
		return false;

	if(lineNumber() == region.first.line)
		moveTo(lineNumber() + 1, columnNumber());

	const String str1 = (lineNumber() - 1 == region.first.line) ?
		document()->line(lineNumber() - 1).substr(region.first.column) : document()->line(lineNumber() - 1);
	const String str2 = (lineNumber() == region.second.line) ?
		document()->line(lineNumber()).substr(0, region.second.column) : document()->line(lineNumber());

	// make the two lines empty
	if(!str2.empty()) {
		beginningOfLine();
		erase(static_cast<signed_length_t>(str2.length()), UTF16_CODE_UNIT);
	}
	if(!str1.empty()) {
		moveTo(lineNumber() - 1, (lineNumber() == region.first.line) ? region.first.column : 0);
		erase(static_cast<signed_length_t>(str1.length()), UTF16_CODE_UNIT);
		moveTo(lineNumber() + 1, columnNumber());
	}

	// insert into the two lines
	if(!str1.empty()) {
		beginningOfLine();
		insert(str1);
	}
	moveTo(lineNumber() - 1, columnNumber());
	if(!str2.empty()) {
		moveTo(lineNumber(), (lineNumber() == region.first.line) ? region.first.column : 0);
		insert(str2);
	}
	moveTo(Position(lineNumber() + 2, 0));

	return true;
}

/**
 * Transposes the two words on either side of the point.
 * @return false if failed
 */
bool VisualPoint::transposeWords() {
	verifyViewer();

	if(document()->isReadOnly())
		return false;

	// As transposing words in string "(\w+)[^\w*](\w+)":
	//
	//  abc += xyz -- transposing words "abc" and "xyz". result is "xyz+=abc"
	// ^   ^  ^   ^
	// |   |  |   2nd-word-end (named pos[3])
	// |   |  2nd-word-start (named pos[2])
	// |   1st-word-end (named pos[1])
	// 1st-word-start (named pos[0])

	const Region region(document()->accessibleRegion());
	WordBreakIterator<DocumentCharacterIterator> i(
		DocumentCharacterIterator(*document(), *this), AbstractWordBreakIterator::START_OF_ALPHANUMERICS, identifierSyntax());
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
	if(pos[2] == position())
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
		const LineLayout& layout = textViewer().textRenderer().lineLayout(lineNumber());
		lastX_ = layout.location(columnNumber(), LineLayout::LEADING).x;
		lastX_ += textViewer().textRenderer().lineIndent(lineNumber(), 0);
	}
}

/// Returns the visual column of the point.
length_t VisualPoint::visualColumnNumber() const {
	if(lastX_ == -1)
		const_cast<VisualPoint*>(this)->updateLastX();
	const TextViewer::Configuration& c = viewer_->configuration();
	const TextRenderer& renderer = viewer_->textRenderer();
	if(c.alignment == ALIGN_LEFT || (c.alignment != ALIGN_RIGHT && c.orientation == LEFT_TO_RIGHT))
		return lastX_ / renderer.averageCharacterWidth();
	else
		return (renderer.getWidth() - lastX_) / renderer.averageCharacterWidth();
}

/// @see IVisualLinesListener#visualLinesDeleted
void VisualPoint::visualLinesDeleted(length_t first, length_t last, length_t, bool) throw() {
	if(!adaptsToDocument() && lineNumber() >= first && lineNumber() < last)
		visualLine_ = INVALID_INDEX;
}

/// @see IVisualLinesListener#visualLinesInserted
void VisualPoint::visualLinesInserted(length_t first, length_t last) throw() {
	if(!adaptsToDocument() && lineNumber() >= first && lineNumber() < last)
		visualLine_ = INVALID_INDEX;
}

/// @see IVisualLinesListener#visualLinesModified
void VisualPoint::visualLinesModified(length_t first, length_t last, signed_length_t sublineDifference, bool, bool) throw() {
	if(visualLine_ != INVALID_INDEX) {
		// adjust visualLine_ and visualSubine_ according to the visual lines modification
		if(last <= lineNumber())
			visualLine_ += sublineDifference;
		else if(first == lineNumber()) {
			visualLine_ -= visualSubline_;
			visualSubline_ = textViewer().textRenderer().lineLayout(
				lineNumber()).subline(min(columnNumber(), document()->lineLength(lineNumber())));
			visualLine_ += visualSubline_;
		} else if(first < lineNumber())
			visualLine_ = INVALID_INDEX;
	}
}

// Caret ////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param viewer the viewer
 * @param position the initial position of the point
 * @throw BadPositionException @a position is outside of the document
 */
Caret::Caret(TextViewer& viewer, const Position& position /* = Position() */) throw() : VisualPoint(viewer, position, 0),
		anchor_(new SelectionAnchor(viewer)), selectionMode_(CHARACTER), pastingFromClipboardRing_(false),
		leaveAnchorNext_(false), leadingAnchor_(false), autoShow_(true), box_(0), matchBracketsTrackingMode_(DONT_TRACK),
		overtypeMode_(false), editingByThis_(false), othersEditedFromLastInputChar_(false),
		regionBeforeMoved_(Position::INVALID_POSITION, Position::INVALID_POSITION),
		matchBrackets_(make_pair(Position::INVALID_POSITION, Position::INVALID_POSITION)) {
	document()->addListener(*this);
	excludeFromRestriction(true);
	anchor_->excludeFromRestriction(true);
}

/// Destructor.
Caret::~Caret() throw() {
	if(Document* const d = document())
		d->removeListener(*this);
	delete anchor_;
	delete box_;
}

/**
 * Starts line selection mode.
 * The rectangular selection will be revoked automatically.
 * @see #beginWordSelection, #restoreSelectionMode
 */
void Caret::beginLineSelection() {
	verifyViewer();
	endRectangleSelection();
	pastingFromClipboardRing_ = false;
	if(selectionMode_ == LINE)
		return;
	selectionMode_ = LINE;
	extendSelection(Position(modeInitialAnchorLine_ = anchor_->lineNumber(), 0));
}

/**
 * Starts rectangular selection.
 * @see #endRectangleSelection, #isSelectionRectangle
 */
void Caret::beginRectangleSelection() {
	verifyViewer();
	if(box_ == 0) {
		box_ = new VirtualBox(textViewer(), selectionRegion());
		stateListeners_.notify<const Caret&>(&ICaretStateListener::selectionShapeChanged, *this);
	}
}

/**
 * Starts word selection mode.
 * The rectangular selection will be revoked automatically.
 * @see #beginLineSelection, #restoreSelectionMode
 */
void Caret::beginWordSelection() {
	verifyViewer();
	endRectangleSelection();
	pastingFromClipboardRing_ = false;
	if(selectionMode_ == WORD)
		return;
	selectWord();
	selectionMode_ = WORD;
	modeInitialAnchorLine_ = lineNumber();
	wordSelectionChars_[0] = anchor_->columnNumber();
	wordSelectionChars_[1] = columnNumber();
}

/// 対括弧の追跡を更新する
void Caret::checkMatchBrackets() {
//	bool matched;
	pair<Position, Position> oldPair(matchBrackets_);
	// TODO: implement matching brackets checking
/*	if(!isSelectionEmpty() || matchBracketsTrackingMode_ == DONT_TRACK)
		matched = false;
	else if(matched = getViewer().searchMatchBracket(getPosition(), matchBrackets_.first, true))
		matchBrackets_.second = getPosition();
	else if(matchBracketsTrackingMode_ == TRACK_FOR_SURROUND_CHARACTERS && !isStartOfLine()) {	// 1文字前も調べる
		const String& line = document()->getLine(lineNumber());
		GraphemeBreakIterator i(line.data(), line.data() + line.length(), line.data() + columnNumber());
		if(matched = getViewer().searchMatchBracket(Position(lineNumber(), (--i).tell() - line.data()), matchBrackets_.first, true))
			matchBrackets_.second = Position(lineNumber(), i.tell() - line.data());
	}
	if(!matched)
		matchBrackets_.first = matchBrackets_.second = Position::INVALID_POSITION;
*/	// TODO: check if the pair is out of view.
	if(matchBrackets_ != oldPair)
		stateListeners_.notify<const Caret&, const pair<Position,
			Position>&, bool>(&ICaretStateListener::matchBracketsChanged, *this, oldPair, false);
}

/// Clears the selection.
void Caret::clearSelection() {
	endRectangleSelection();
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
	const String s(selectionText(NLF_RAW_VALUE));
	Clipboard(textViewer().getHandle()).write(s, isSelectionRectangle());
	if(alsoSendToClipboardRing) {	// クリップボードリングにも転送
		if(texteditor::Session* const session = document()->session())
			session->clipboardRing().add(s, isSelectionRectangle());
	}
}

/**
 * Copies and deletes the selected text.
 * @param alsoSendToClipboardRing true to send also the clipboard ring
 */
void Caret::cutSelection(bool alsoSendToClipboardRing) {
	verifyViewer();
	if(isSelectionEmpty() || document()->isReadOnly())
		return;
	copySelection(alsoSendToClipboardRing);
	textViewer().freeze(true);
	document()->beginSequentialEdit();
	eraseSelection();
	document()->endSequentialEdit();
	textViewer().unfreeze(true);
}

/// @see kernel#IDocumentListener#documentAboutToBeChanged
bool Caret::documentAboutToBeChanged(const Document&, const DocumentChange&) {
	// do nothing
	return true;
}

/// @see kernel#IDocumentListener#documentChanged
void Caret::documentChanged(const Document&, const DocumentChange&) {
	if(regionBeforeMoved_.first != Position::INVALID_POSITION)
		updateVisualAttributes();
}

/// @see VisualPoint#doMoveTo
void Caret::doMoveTo(const Position& to) {
	regionBeforeMoved_ = Region(anchor_->isInternalUpdating() ?
		anchor_->positionBeforeInternalUpdate() : anchor_->position(), position());
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
	if(!document()->isChanging())
		updateVisualAttributes();
}

/**
 * Ends the rectangular selection.
 * @see #beginRectangleSelection, #isSelectionRectangle
 */
void Caret::endRectangleSelection() {
	verifyViewer();
	if(box_ != 0) {
		delete box_;
		box_ = 0;
		stateListeners_.notify<const Caret&>(&ICaretStateListener::selectionShapeChanged, *this);
	}
}

/**
 * Deletes the selected text.
 * This method does not freeze the viewer and not begin the sequential edit.
 */
void Caret::eraseSelection() {
	verifyViewer();
	Document& doc = *document();
	if(doc.isReadOnly() || isSelectionEmpty())
		return;
	else if(!isSelectionRectangle())	// the selection is linear
		moveTo(doc.erase(*anchor_, *this));
	else {	// the selection is rectangle
		const Position resultPosition(beginning());
		const bool adapts = adaptsToDocument();
		adaptToDocument(false);
		const length_t firstLine = beginning().lineNumber(), lastLine = end().lineNumber();
		pair<length_t, length_t> rangeInLine;;

		if(textViewer().configuration().lineWrap.wraps()) {	// ...and the lines are wrapped
			// hmmm..., this is heavy work
			vector<Point*> points;
			vector<length_t> sizes;
			points.reserve((lastLine - firstLine) * 2);
			sizes.reserve((lastLine - firstLine) * 2);
			const TextRenderer& renderer = textViewer().textRenderer();
			for(length_t line = resultPosition.line; line <= lastLine; ++line) {
				const LineLayout& layout = renderer.lineLayout(line);
				for(length_t subline = 0; subline < layout.numberOfSublines(); ++subline) {
					box_->overlappedSubline(line, subline, rangeInLine.first, rangeInLine.second);
					points.push_back(new Point(doc, Position(line, rangeInLine.first)));
					sizes.push_back(rangeInLine.second - rangeInLine.first);
				}
			}
			const size_t sublines = points.size();
			for(size_t i = 0; i < sublines; ++i) {
				doc.erase(Position(points[i]->lineNumber(), points[i]->columnNumber()),
					Position(points[i]->lineNumber(), points[i]->columnNumber() + sizes[i]));
				delete points[i];
			}
		} else {
			for(length_t line = resultPosition.line; line <= lastLine; ++line) {
				box_->overlappedSubline(line, 0, rangeInLine.first, rangeInLine.second);
				doc.erase(Position(line, rangeInLine.first), Position(line, rangeInLine.second));
			}
		}

		endRectangleSelection();
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
		const length_t lc = document()->numberOfLines();
		Region s;
		s.first.line = (to.line >= modeInitialAnchorLine_) ? modeInitialAnchorLine_ : modeInitialAnchorLine_ + 1;
		s.first.column = (s.first.line > lc - 1) ? document()->lineLength(--s.first.line) : 0;
		s.second.line = (to.line >= modeInitialAnchorLine_) ? to.line + 1 : to.line;
		s.second.column = (s.second.line > lc - 1) ? document()->lineLength(--s.second.line) : 0;
		select(s);
		selectionMode_ = LINE;
	} else if(selectionMode_ == WORD) {
		if(to.line < modeInitialAnchorLine_ || (to.line == modeInitialAnchorLine_ && to.column < wordSelectionChars_[0])) {
			WordBreakIterator<DocumentCharacterIterator> i(
				DocumentCharacterIterator(*document(), to), AbstractWordBreakIterator::BOUNDARY_OF_SEGMENT, identifierSyntax());
			--i;
			select(Position(modeInitialAnchorLine_, wordSelectionChars_[1]),
				(i.base().tell().line == to.line) ? i.base().tell() : Position(to.line, 0));
		} else if(to.line > modeInitialAnchorLine_ || (to.line == modeInitialAnchorLine_ && to.column > wordSelectionChars_[1])) {
			WordBreakIterator<DocumentCharacterIterator> i(
				DocumentCharacterIterator(*document(), to), AbstractWordBreakIterator::BOUNDARY_OF_SEGMENT, identifierSyntax());
			++i;
			select(Position(modeInitialAnchorLine_, wordSelectionChars_[0]),
				(i.base().tell().line == to.line) ? i.base().tell() : Position(to.line, document()->lineLength(to.line)));
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

	Document& doc = *document();
	if(doc.isReadOnly())
		return false;
	else if(blockControls && cp <= 0xFF && cp != 0x09 && cp != 0x1E && cp != 0x1F && toBoolean(iscntrl(static_cast<int>(cp))))
		return false;

	// check the input sequence
	if(validateSequence) {
		if(const texteditor::Session* const session = doc.session()) {
			if(const texteditor::InputSequenceCheckers* const checker = session->inputSequenceCheckers()) {
				const Char* const line = doc.line(beginning().lineNumber()).data();
				if(!checker->check(line, line + beginning().columnNumber(), cp)) {
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
		if(!doc.isSequentialEditing())
			doc.beginSequentialEdit();
		textViewer().freeze(true);
		destructiveInsert(buffer, buffer + ((cp < 0x10000) ? 1 : 2));
		textViewer().unfreeze(true);
	} else {
		const IdentifierSyntax& ctypes = identifierSyntax();
		const bool alpha = ctypes.isIdentifierContinueCharacter(cp);

//		// exit the completion mode if the character is not ID_Start or ID_Continue
//		if(!alpha && completionWindow_.isRunning())
//			completionWindow_.complete();

		// prepare a packing the following multiple inputs
		if(othersEditedFromLastInputChar_ || !alpha)
			doc.endSequentialEdit();
		if(alpha && !doc.isSequentialEditing()) {
			doc.beginSequentialEdit();
			othersEditedFromLastInputChar_ = false;
		}

		editingByThis_ = true;
		insert(buffer, buffer + ((cp < 0x10000) ? 1 : 2));
		editingByThis_ = false;
	}
	characterInputListeners_.notify<const Caret&, CodePoint>(&ICharacterInputListener::characterInputted, *this, cp);

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
		if(textViewer().hitTest(pt) != TextViewer::TEXT_AREA)	// ignore if on the margin
			return false;
		::RECT rect;
		textViewer().getClientRect(rect);
		if(pt.x > rect.right || pt.y > rect.bottom)
			return false;
		const Position pos(textViewer().characterForClientXY(pt, LineLayout::TRAILING));
		return pos >= beginning() && pos <= end();
	}
}

/**
 * Replaces the selected text by the content of the clipboard.
 * @param fromClipboardRing true to use the clipboard ring
 */
void Caret::pasteToSelection(bool fromClipboardRing) {
	verifyViewer();
	if(document()->isReadOnly())
		return;
	texteditor::Session* const session = document()->session();
	if(fromClipboardRing && (session == 0 || session->clipboardRing().numberOfItems() == 0))
		return;

	document()->beginSequentialEdit();
	textViewer().freeze(true);
	if(!fromClipboardRing) {
		if(!isSelectionEmpty())
			eraseSelection();
		paste();
	} else {
		size_t activeItem = session->clipboardRing().activeItem();
		if(pastingFromClipboardRing_ && ++activeItem == session->clipboardRing().numberOfItems())
			activeItem = 0;

		String str;
		bool rectangle;
		session->clipboardRing().text(activeItem, str, rectangle);
		session->clipboardRing().setActiveItem(activeItem);
		if(!isSelectionEmpty()) {
			if(pastingFromClipboardRing_)
				document()->undo();
			eraseSelection();
		}
		const Position p(position());
		if(!rectangle) {
			insert(str);
			endRectangleSelection();
		} else {
			insertRectangle(str);
			beginRectangleSelection();
		}
		select(p, position());
		pastingFromClipboardRing_ = true;
	}
	document()->endSequentialEdit();
	textViewer().unfreeze(true);
}

/// @see IPointListener#pointMoved
void Caret::pointMoved(const EditPoint& self, const Position& oldPosition) {
	assert(&self == &*anchor_);
	pastingFromClipboardRing_ = false;
	if(leadingAnchor_)	// doMoveTo で anchor_->moveTo 呼び出し中
		return;
	if((oldPosition == position()) != isSelectionEmpty())
		checkMatchBrackets();
	listeners_.notify<const Caret&, const Region&>(&ICaretListener::caretMoved, *this, Region(oldPosition, position()));
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
	if(document()->isReadOnly())
		return;
	const Region oldRegion(selectionRegion());
	document()->beginSequentialEdit();
	textViewer().freeze(true);
	if(!isSelectionEmpty())
		eraseSelection();
	else if(isSelectionRectangle())
		endRectangleSelection();
	if(rectangleInsertion)
		insertRectangle(first, last);
	else
		insert(first, last);
	textViewer().unfreeze(true);
	document()->endSequentialEdit();
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
	if(anchor != anchor_->position() || caret != position()) {
		const Region oldRegion(selectionRegion());
		if(selectionMode_ == CHARACTER) {
			leadingAnchor_ = true;
			anchor_->moveTo(anchor);
			leadingAnchor_ = false;
		}
		VisualPoint::doMoveTo(caret);
		if(isSelectionRectangle())
			box_->update(selectionRegion());
		if(autoShow_)
			show();
		listeners_.notify<const Caret&, const Region&>(&ICaretListener::caretMoved, *this, oldRegion);
	}
	checkMatchBrackets();
}

/**
 * Returns the selected range on the specified logical line.
 * This method returns a logical range, and does not support rectangular selection.
 * @param line the logical line
 * @param[out] first the start of the range
 * @param[out] last the end of the range. this can include the end of the line
 * @return true if there is selected range on the line
 * @throw kernel#BadPositionException @a line is outside of the document
 * @see #getSelectedRangeOnVisualLine
 */
bool Caret::selectedRangeOnLine(length_t line, length_t& first, length_t& last) const {
	verifyViewer();
	const Position top(beginning());
	if(top.line > line)
		return false;
	const Position bottom(end());
	if(bottom.line < line)
		return false;
	first = (line == top.line) ? top.column : 0;
	last = (line == bottom.line) ? bottom.column : document()->lineLength(line) + 1;
	return true;
}

/**
 * Returns the selected range on the specified visual line,
 * @param line the logical line
 * @param subline the visual subline
 * @param[out] first the start of the range
 * @param[out] last the end of the range. this can include the logical end of the line
 * @return true if there is selected range on the line
 * @throw kernel#BadPositionException @a line or @a subline is outside of the document
 * @see #getSelectedRangeOnLine
 */
bool Caret::selectedRangeOnVisualLine(length_t line, length_t subline, length_t& first, length_t& last) const {
	verifyViewer();
	if(!isSelectionRectangle()) {
		if(!selectedRangeOnLine(line, first, last))
			return false;
		const LineLayout& layout = textViewer().textRenderer().lineLayout(line);
		const length_t sublineOffset = layout.sublineOffset(subline);
		first = max(first, sublineOffset);
		last = min(last, sublineOffset + layout.sublineLength(subline) + ((subline < layout.numberOfSublines() - 1) ? 0 : 1));
		return first != last;
	} else
		return box_->overlappedSubline(line, subline, first, last);
}

/**
 * Returns the selected text.
 * @param newline the newline representation for multiline selection. if the selection is
 * rectangular, this value is ignored and the document's newline is used instead
 * @return the text
 */
String Caret::selectionText(Newline newline /* = NLF_RAW_VALUE */) const {
	verifyViewer();

	if(isSelectionEmpty())
		return L"";
	else if(!isSelectionRectangle())
		return getText(*anchor_, newline);

	// rectangular selection
	basic_stringbuf<Char> s(ios_base::out);
	const length_t bottomLine = end().lineNumber();
	length_t first, last;
	for(length_t line = beginning().lineNumber(); line <= bottomLine; ++line) {
		const Document::Line& ln = document()->getLineInformation(line);
		box_->overlappedSubline(line, 0, first, last);	// TODO: recognize wrap (second parameter).
		s.sputn(ln.text().data() + first, static_cast<streamsize>(last - first));
		s.sputn(getNewlineString(ln.newline()), static_cast<streamsize>(getNewlineStringLength(ln.newline())));
	}
	return s.str();
}

/// Selects the word at the caret position.
void Caret::selectWord() {
	verifyViewer();

	WordBreakIterator<DocumentCharacterIterator> i(
		DocumentCharacterIterator(*document(), *this), AbstractWordBreakIterator::BOUNDARY_OF_SEGMENT, identifierSyntax());
	endRectangleSelection();
	if(isEndOfLine()) {
		if(isBeginningOfLine())	// an empty line
			moveTo(*this);
		else	// eol
			select((--i).base().tell(), *this);
	} else if(isBeginningOfLine())	// bol
		select(*this, (++i).base().tell());
	else {
		const Position p = (++i).base().tell();
		i.base().seek(Position(lineNumber(), columnNumber() + 1));
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
		stateListeners_.notify<const Caret&>(&ICaretStateListener::overtypeModeChanged, *this);
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
		box_->update(selectionRegion());
	if((regionBeforeMoved_.first != position() || regionBeforeMoved_.second != position()))
		listeners_.notify<const Caret&, const Region&>(&ICaretListener::caretMoved, *this, regionBeforeMoved_);
	if(autoShow_)
		show();
	checkMatchBrackets();
	regionBeforeMoved_.first = regionBeforeMoved_.second = Position::INVALID_POSITION;
}
