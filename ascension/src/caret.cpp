/**
 * @file caret.cpp
 * @author exeal
 * @date 2003-2008 was point.cpp
 * @date 2008-2009 separated from point.cpp
 */

#undef MANAH_OVERRIDDEN_FILE
static const char MANAH_OVERRIDDEN_FILE[] = __FILE__;

#include "ascension/viewer.hpp"
#include "ascension/session.hpp"
#include "manah/win32/utility.hpp"
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
#pragma comment(lib, "urlmon.lib")
	// IDataObject implementation for OLE image drag-and-drop. Caret.createTextObject returns this
	// object as a result.
	//
	// This does not support any device-specific renderings. All methods can be overridden.
	//
	// References:
	// - "The Shell Drag/Drop Helper Object Part 1: IDropTargetHelper"
	//   (http://msdn.microsoft.com/en-us/library/ms997500.aspx)
	// - "The Shell Drag/Drop Helper Object Part 2: IDropSourceHelper"
	//   (http://msdn.microsoft.com/en-us/library/ms997502.aspx)
	// and their Japanese translations
	// - "Shell Drag/Drop Helper オブジェクト 第 1 部 : IDropTargetHelper"
	//   (http://www.microsoft.com/japan/msdn/windows/windows2000/ddhelp_pt1.aspx)
	// - "Shell Drag/Drop Helper オブジェクト 第 2 部 : IDropSourceHelper"
	//   (http://www.microsoft.com/japan/msdn/windows/windows2000/ddhelp_pt2.aspx)
	// ...but these documents have many bugs. Well, there is no interface named "IDropSourceHelper".
	class GenericDataObject : public manah::com::IUnknownImpl<
		manah::typelist::Cat<MANAH_INTERFACE_SIGNATURE(IDataObject)>, manah::com::NoReferenceCounting > {
	public:
		virtual ~GenericDataObject() throw();
		// IDataObject
		virtual STDMETHODIMP GetData(FORMATETC* format, STGMEDIUM* medium);
		virtual STDMETHODIMP GetDataHere(FORMATETC*, STGMEDIUM*) {return E_NOTIMPL;}
		virtual STDMETHODIMP QueryGetData(FORMATETC* format);
		virtual STDMETHODIMP GetCanonicalFormatEtc(FORMATETC* in, FORMATETC* out);
		virtual STDMETHODIMP SetData(FORMATETC* format, STGMEDIUM* medium, BOOL release);
		virtual STDMETHODIMP EnumFormatEtc(DWORD direction, IEnumFORMATETC** enumerator);
		virtual STDMETHODIMP DAdvise(LPFORMATETC, DWORD, LPADVISESINK, LPDWORD) {return OLE_E_ADVISENOTSUPPORTED;}
		virtual STDMETHODIMP DUnadvise(DWORD) {return OLE_E_ADVISENOTSUPPORTED;}
		virtual STDMETHODIMP EnumDAdvise(LPENUMSTATDATA*) {return OLE_E_ADVISENOTSUPPORTED;}
	private:
		struct Entry {
			FORMATETC format;
			STGMEDIUM medium;
		};
		list<Entry*>::iterator find(const FORMATETC& format, list<Entry*>::iterator initial) const /*throw()*/;
	private:
		list<Entry*> entries_;
	};

	GenericDataObject::~GenericDataObject() /*throw()*/ {
		for(list<Entry*>::iterator i(entries_.begin()), e(entries_.end()); i != e; ++i) {
			::CoTaskMemFree((*i)->format.ptd);
			::ReleaseStgMedium(&(*i)->medium);
		}
	}
	list<GenericDataObject::Entry*>::iterator GenericDataObject::find(
			const FORMATETC& format, list<Entry*>::iterator initial) const /*throw()*/ {
		const list<Entry*>::iterator e(const_cast<GenericDataObject*>(this)->entries_.end());
		if(format.ptd == 0) {	// this does not support DVTARGETDEVICE
			for(list<Entry*>::iterator i(initial); i != e; ++i) {
				const FORMATETC& other = (*i)->format;
				if(other.cfFormat == format.cfFormat && other.dwAspect == format.dwAspect && other.lindex == format.lindex)
					return i;
			}
		}
		return e;
	}
	STDMETHODIMP GenericDataObject::GetData(FORMATETC* format, STGMEDIUM* medium) {
		if(format == 0 || medium == 0)
			return E_INVALIDARG;
		else if(format->lindex != -1)
			return DV_E_LINDEX;
		list<Entry*>::const_iterator entry(find(*format, entries_.begin()));
		if(entry == entries_.end())
			return DV_E_FORMATETC;
		else if(((*entry)->format.tymed & format->tymed) == 0)
			return DV_E_TYMED;
		const HRESULT hr = ::CopyStgMedium(&(*entry)->medium, medium);
		if(SUCCEEDED(hr))
			medium->pUnkForRelease = 0;
		return hr;
	}
	STDMETHODIMP GenericDataObject::QueryGetData(LPFORMATETC format) {
		if(format == 0)
			return E_INVALIDARG;
		else if(format->lindex != -1)
			return DV_E_LINDEX;
		list<Entry*>::const_iterator entry(find(*format, entries_.begin()));
		if(entry == entries_.end())
			return DV_E_FORMATETC;
		return (((*entry)->format.tymed & format->tymed) != 0) ? S_OK : DV_E_TYMED;
	}
	STDMETHODIMP GenericDataObject::GetCanonicalFormatEtc(FORMATETC* in, FORMATETC* out) {
		if(in == 0 || out == 0)
			return E_INVALIDARG;
		else if(in->lindex != -1)
			return DV_E_LINDEX;
		else if(in->ptd != 0)
			return DV_E_FORMATETC;
		*out = *in;
		return DATA_S_SAMEFORMATETC;
	}
	STDMETHODIMP GenericDataObject::SetData(FORMATETC* format, STGMEDIUM* medium, BOOL release) {
		if(format == 0 || medium == 0)
			return E_INVALIDARG;
		STGMEDIUM clone;
		if(!release) {
			if(FAILED(::CopyStgMedium(medium, &clone)))
				return E_FAIL;
		}
		list<Entry*>::iterator entry(entries_.begin());
		while(true) {
			entry = find(*format, entry);
			if(entry == entries_.end() || ((*entry)->format.tymed & format->tymed) != 0)
				break;
		}
		if(entry == entries_.end()) {	// a entry has the given format does not exist
			Entry* const newEntry = static_cast<Entry*>(::CoTaskMemAlloc(sizeof(Entry)));
			if(newEntry == 0)
				return E_OUTOFMEMORY;
			newEntry->format = *format;
			memset(&newEntry->medium, 0, sizeof(STGMEDIUM));
			entries_.push_back(newEntry);
			entry = --entries_.end();
		} else if((*entry)->medium.tymed != TYMED_NULL) {
			::ReleaseStgMedium(&(*entry)->medium);
			memset(&(*entry)->medium, 0, sizeof(STGMEDIUM));
		}

		assert((*entry)->medium.tymed == TYMED_NULL);
		(*entry)->medium = toBoolean(release) ? *medium : clone;
		return S_OK;
	}
	STDMETHODIMP GenericDataObject::EnumFormatEtc(DWORD direction, IEnumFORMATETC** enumerator) {
		if(direction == DATADIR_SET)
			return E_NOTIMPL;
		else if(direction != DATADIR_GET)
			return E_INVALIDARG;
		else if(enumerator == 0)
			return E_INVALIDARG;
		FORMATETC* buffer = static_cast<FORMATETC*>(::CoTaskMemAlloc(sizeof(FORMATETC) * entries_.size()));
		if(buffer == 0)
			return E_OUTOFMEMORY;
		size_t j = 0;
		for(list<Entry*>::const_iterator i(entries_.begin()), e(entries_.end()); i != e; ++i, ++j)
			buffer[j] = (*i)->format;
		const HRESULT hr = ::CreateFormatEnumerator(static_cast<UINT>(entries_.size()), buffer, enumerator);
		::CoTaskMemFree(buffer);
		return hr;
	}

	template<typename Procedure>
	inline HRESULT tryOleClipboard(Procedure procedure) {
		HRESULT hr;
		for(int i = 0; i < 100; ++i) {
			if(CLIPBRD_E_CANT_OPEN != (hr = procedure()))
				break;
			::Sleep(0);
		}
		return hr;
	}
	template<typename Procedure, typename Parameter>
	inline HRESULT tryOleClipboard(Procedure procedure, Parameter parameter) {
		HRESULT hr;
		for(int i = 0; i < 100; ++i) {
			if(CLIPBRD_E_CANT_OPEN != (hr = procedure(parameter)))
				break;
			::Sleep(0);
		}
		return hr;
	}
} // namespace @0

/**
 * Returns the text content from the given data object.
 * @param data the data object
 * @param[out] rectangle true if the data is rectangle format. can be @c null
 * @return a pair of the result HRESULT and the text content. SCODE is one of S_OK, E_OUTOFMEMORY and DV_E_FORMATETC
 */
pair<HRESULT, String> viewers::getTextFromDataObject(IDataObject& data, bool* rectangle /* = 0 */) {
	pair<HRESULT, String> result;
	FORMATETC fe = {CF_UNICODETEXT, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	STGMEDIUM stm = {TYMED_HGLOBAL, 0};
	if(S_OK == (result.first = data.QueryGetData(&fe))) {	// the data suppports CF_UNICODETEXT ?
		if(SUCCEEDED(result.first = data.GetData(&fe, &stm))) {
			if(const Char* buffer = static_cast<Char*>(::GlobalLock(stm.hGlobal))) {
				try {
					result.second = String(buffer);
				} catch(...) {
					result.first = E_OUTOFMEMORY;
				}
				::GlobalUnlock(stm.hGlobal);
				::ReleaseStgMedium(&stm);
			}
		}
	}

	if(FAILED(result.first)) {
		fe.cfFormat = CF_TEXT;
		if(S_OK == (result.first = data.QueryGetData(&fe))) {	// the data supports CF_TEXT ?
			if(SUCCEEDED(result.first = data.GetData(&fe, &stm))) {
				if(const char* nativeBuffer = static_cast<char*>(::GlobalLock(stm.hGlobal))) {
					// determine the encoding of the content of the clipboard
					UINT codePage = ::GetACP();
					fe.cfFormat = CF_LOCALE;
					if(S_OK == (result.first = data.QueryGetData(&fe))) {
						STGMEDIUM locale = {TYMED_HGLOBAL, 0};
						if(S_OK == (result.first = data.GetData(&fe, &locale))) {
							wchar_t buffer[6];
							if(0 != ::GetLocaleInfoW(*static_cast<ushort*>(::GlobalLock(locale.hGlobal)),
									LOCALE_IDEFAULTANSICODEPAGE, buffer, MANAH_COUNTOF(buffer))) {
								wchar_t* eob;
								codePage = wcstoul(buffer, &eob, 10);
							}
						}
						::ReleaseStgMedium(&locale);
					}
					// convert ANSI text into Unicode by the code page
					const length_t nativeLength = min<length_t>(
						strlen(nativeBuffer), ::GlobalSize(stm.hGlobal) / sizeof(char)) + 1;
					const length_t ucsLength = ::MultiByteToWideChar(
						codePage, MB_PRECOMPOSED, nativeBuffer, static_cast<int>(nativeLength), 0, 0);
					if(ucsLength != 0) {
						manah::AutoBuffer<wchar_t> ucsBuffer(new(nothrow) wchar_t[ucsLength]);
						if(ucsBuffer.get() != 0) {
							if(0 != ::MultiByteToWideChar(codePage, MB_PRECOMPOSED,
									nativeBuffer, static_cast<int>(nativeLength), ucsBuffer.get(), static_cast<int>(ucsLength))) {
								try {
									result.second = String(ucsBuffer.get(), ucsLength - 1);
								} catch(...) {
									result.first = E_OUTOFMEMORY;
								}
							}
						}
					}
					::GlobalUnlock(stm.hGlobal);
					::ReleaseStgMedium(&stm);
				}
			}
		}
	}

	if(FAILED(result.first))
		result.first = DV_E_FORMATETC;
	if(SUCCEEDED(result.first) && rectangle != 0) {
		fe.cfFormat = static_cast<CLIPFORMAT>(::RegisterClipboardFormatW(ASCENSION_RECTANGLE_TEXT_CLIP_FORMAT));
		*rectangle = fe.cfFormat != 0 && data.QueryGetData(&fe) == S_OK;
	}

	return result;
}


// ClipboardException ///////////////////////////////////////////////////////

ClipboardException::ClipboardException(HRESULT hr) : runtime_error("") {
	void* buffer = 0;
	::FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		0, hr, 0, reinterpret_cast<char*>(&buffer), 0, 0);
	runtime_error(static_cast<char*>(buffer));
	::LocalFree(buffer);
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
		EditPoint(viewer.document(), position, listener), viewer_(&viewer),
		lastX_(-1), crossingLines_(false), visualLine_(INVALID_INDEX), visualSubline_(0) {
	static_cast<kernel::internal::IPointCollection<VisualPoint>&>(viewer).addNewPoint(*this);
	viewer_->textRenderer().addVisualLinesListener(*this);
}

/**
 * Copy-constructor.
 * @param rhs the source object
 * @throw DisposedDocumentException the document to which @a rhs belongs had been disposed
 * @throw DisposedViewerException the text viewer to which @a rhs belongs had been disposed
 */
VisualPoint::VisualPoint(const VisualPoint& rhs) : EditPoint(rhs), viewer_(rhs.viewer_),
		lastX_(rhs.lastX_), crossingLines_(false), visualLine_(rhs.visualLine_), visualSubline_(rhs.visualSubline_) {
	if(viewer_ == 0)
		throw DisposedViewerException();
	static_cast<kernel::internal::IPointCollection<VisualPoint>*>(viewer_)->addNewPoint(*this);
	viewer_->textRenderer().addVisualLinesListener(*this);
}

/// Destructor.
VisualPoint::~VisualPoint() /*throw()*/ {
	if(viewer_ != 0) {
		static_cast<kernel::internal::IPointCollection<VisualPoint>*>(viewer_)->removePoint(*this);
		viewer_->textRenderer().removeVisualLinesListener(*this);
	}
}

/**
 * Returns the position returned by N pages.
 * @param pages the number of pages to return
 */
VerticalDestinationProxy VisualPoint::backwardPage(length_t pages /* = 1 */) const {
	// TODO: calculate exact number of visual lines.
	return backwardVisualLine(viewer_->numberOfVisibleLines() * pages);
}

/**
 * Returns the position returned by N visual lines.
 * @param lines the number of the visual lines to return
 */
VerticalDestinationProxy VisualPoint::backwardVisualLine(length_t lines /* = 1 */) const {
	Position p(normalized());
	const TextRenderer& renderer = viewer_->textRenderer();
	length_t subline = renderer.lineLayout(p.line).subline(p.column);
	if(p.line == 0 && subline == 0)
		return VerticalDestinationProxy(p);
	renderer.offsetVisualLine(p.line, subline, -static_cast<signed_length_t>(lines));
	const LineLayout& layout = renderer.lineLayout(p.line);
	p.column = layout.offset(lastX_ - renderer.lineIndent(p.line), renderer.linePitch() * static_cast<long>(subline));
	if(layout.subline(p.column) != subline)
		p = offsetCharacterPosition(*document(), p, Direction::BACKWARD, characterUnit());
	return VerticalDestinationProxy(p);
}

/**
 * Returns the beginning of the visual line.
 * @see EditPoint#beginningOfLine
 */
Position VisualPoint::beginningOfVisualLine() const {
	verifyViewer();
	const Position p(normalized());
	const LineLayout& layout = textViewer().textRenderer().lineLayout(p.line);
	return Position(p.line, layout.sublineOffset(layout.subline(p.column)));
}

/**
 * Returns true if a paste operation can be performed.
 * @return true if the clipboard data is pastable
 */
bool VisualPoint::canPaste() {
	const UINT rectangleClipFormat = ::RegisterClipboardFormatW(ASCENSION_RECTANGLE_TEXT_CLIP_FORMAT);
	if(rectangleClipFormat != 0 && toBoolean(::IsClipboardFormatAvailable(rectangleClipFormat)))
		return true;
	else if(toBoolean(::IsClipboardFormatAvailable(CF_UNICODETEXT)) || toBoolean(::IsClipboardFormatAvailable(CF_TEXT)))
		return true;
	return false;
}

/// Returns the beginning of the line or the first printable character in the line by context.
Position VisualPoint::contextualBeginningOfLine() const {
	return isFirstPrintableCharacterOfLine() ? beginningOfLine() : firstPrintableCharacterOfLine();
}

/// Moves to the beginning of the visual line or the first printable character in the visual line
/// by context.
Position VisualPoint::contextualBeginningOfVisualLine() const {
	return isFirstPrintableCharacterOfLine() ? beginningOfVisualLine() : firstPrintableCharacterOfVisualLine();
}

/// Moves to the end of the line or the last printable character in the line by context.
Position VisualPoint::contextualEndOfLine() const {
	return isLastPrintableCharacterOfLine() ? endOfLine() : lastPrintableCharacterOfLine();
}

/// Moves to the end of the visual line or the last printable character in the visual line by
/// context.
Position VisualPoint::contextualEndOfVisualLine() const {
	return isLastPrintableCharacterOfLine() ? endOfVisualLine() : lastPrintableCharacterOfVisualLine();
}

/**
 * Indents the specified region.
 * @param other もう1つの位置
 * @param character a character to make indents
 * @param rectangle set true for rectangular indents (will be ignored level is negative)
 * @param level the level of the indentation
 * @return 操作の結果 @a pos が移動するとよい位置
 * @deprecated 0.8
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
 * Returns the end of the visual line.
 * @see EditPoint#endOfLine
 */
Position VisualPoint::endOfVisualLine() const {
	Position p(normalized());
	const LineLayout& layout = textViewer().textRenderer().lineLayout(p.line);
	const length_t subline = layout.subline(p.column);
	p.column = (subline < layout.numberOfSublines() - 1) ?
		layout.sublineOffset(subline + 1) : document()->lineLength(p.line);
	if(layout.subline(p.column) != subline)
		p = offsetCharacterPosition(*document(), p, Direction::BACKWARD, characterUnit());
	return p;
}

/// Returns the first printable character in the line.
Position VisualPoint::firstPrintableCharacterOfLine() const {
	Position p(normalized());
	const Char* const s = document()->line(p.line).data();
	p.column = identifierSyntax().eatWhiteSpaces(s, s + document()->lineLength(p.line), true) - s;
	return p;
}

/// Returns the first printable character in the visual line.
Position VisualPoint::firstPrintableCharacterOfVisualLine() const {
	Position p(normalized());
	const String& s = document()->line(p.line);
	const LineLayout& layout = viewer_->textRenderer().lineLayout(p.line);
	const length_t subline = layout.subline(p.column);
	p.column = identifierSyntax().eatWhiteSpaces(
		s.begin() + layout.sublineOffset(subline),
		s.begin() + ((subline < layout.numberOfSublines() - 1) ?
			layout.sublineOffset(subline + 1) : s.length()), true) - s.begin();
	return p;
}

/**
 * Returns the position advanced by N pages.
 * @param pages the number of pages to advance
 */
VerticalDestinationProxy VisualPoint::forwardPage(length_t pages /* = 1 */) const {
	// TODO: calculate exact number of visual lines.
	return forwardVisualLine(viewer_->numberOfVisibleLines() * pages);
}

/**
 * Returns the position advanced by N visual lines.
 * @param lines the number of the visual lines to advance
 */
VerticalDestinationProxy VisualPoint::forwardVisualLine(length_t lines /* = 1 */) const {
	Position p(normalized());
	const TextRenderer& renderer = viewer_->textRenderer();
	const LineLayout* layout = &renderer.lineLayout(p.line);
	length_t subline = layout->subline(p.column);
	if(p.line == document()->numberOfLines() - 1 && subline == layout->numberOfSublines() - 1)
		return VerticalDestinationProxy(p);
	renderer.offsetVisualLine(p.line, subline, static_cast<signed_length_t>(lines));
	layout = &renderer.lineLayout(p.line);
	p.column = layout->offset(lastX_ - renderer.lineIndent(p.line), renderer.linePitch() * static_cast<long>(subline));
	if(layout->subline(p.column) != subline)
		p = offsetCharacterPosition(*document(), p, Direction::BACKWARD, characterUnit());
	return VerticalDestinationProxy(p);
}

/**
 * Inserts the spcified text as a rectangle at the current position. This method has two
 * restrictions as the follows:
 * - If the text viewer is line wrap mode, this method inserts text as linear not rectangle.
 * - If the destination line is bidirectional, the insertion may be performed incorrectly.
 * @param first the start of the text
 * @param last the end of the text
 * @return false if the change was interrupted
 * @throw ReadOnlyDocumentException the document is read only
 * @throw NullPointerException @a first and/or @a last are @c null
 * @throw std#invalid_argument @a first &gt; @a last
 * @see kernel#EditPoint#insert
 */
bool VisualPoint::insertRectangle(const Char* first, const Char* last) {
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

	Document& doc = *document();
	if(doc.isReadOnly())
		throw ReadOnlyDocumentException();
	else if(first == last)
		return true;

	const length_t numberOfLines = doc.numberOfLines();
	length_t line = lineNumber();
	const TextRenderer& renderer = textViewer().textRenderer();
	const int x = renderer.lineLayout(line).location(columnNumber()).x + renderer.lineIndent(line, 0);
	const IDocumentInput* const documentInput = doc.input();
	const String newline(getNewlineString((documentInput != 0) ? documentInput->newline() : ASCENSION_DEFAULT_NEWLINE));
	// TODO: guarentee complete rollback when failed.
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
			if(!doc.insert(Position(line, column), s))
				return false;
		}

		if(eol == last)
			break;
		bol = eol + ((eol[0] == CARRIAGE_RETURN && eol < last - 1 && eol[1] == LINE_FEED) ? 2 : 1);
	}
	return true;
}

/**
 * Returns true if the point is the beginning of the visual line.
 * @see EditPoint#isBeginningOfLine
 */
bool VisualPoint::isBeginningOfVisualLine() const {
	verifyViewer();
	if(isBeginningOfLine())	// this considers narrowing
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
	if(isEndOfLine())	// this considers narrowing
		return true;
	const LineLayout& layout = textViewer().textRenderer().lineLayout(lineNumber());
	const length_t subline = layout.subline(columnNumber());
	return columnNumber() == layout.sublineOffset(subline) + layout.sublineLength(subline);
}

/// Returns true if the current position is the first printable character in the line.
bool VisualPoint::isFirstPrintableCharacterOfLine() const {
	verifyViewer();
	normalize();
	const Position start(document()->accessibleRegion().first);
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
	const Position end(document()->accessibleRegion().second);
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

/// Returns the last printable character in the line.
Position VisualPoint::lastPrintableCharacterOfLine() const {
	Position p(normalized());
	const String& s(document()->line(p.line));
	const IdentifierSyntax& syntax = identifierSyntax();
	for(length_t spaceLength = 0; spaceLength < s.length(); ++spaceLength) {
		if(syntax.isWhiteSpace(s[s.length() - spaceLength - 1], true))
			return p.column = s.length() - spaceLength, p;
	}
	return p.column = s.length(), p;
}

/// Moves to the last printable character in the visual line.
Position VisualPoint::lastPrintableCharacterOfVisualLine() const {
	// TODO: not implemented.
	return normalized();
}

/**
 * Returns the position advanced to the left by N characters.
 * @param characters the number of characters to adavance
 */
Position VisualPoint::leftCharacter(length_t characters /* = 1 */) const {
	verifyViewer();
	return (viewer_->configuration().orientation == LEFT_TO_RIGHT) ?
		backwardCharacter(characters) : forwardCharacter(characters);
}

/**
 * Returns the beginning of the left N words.
 * @param words the number of words to adavance
 */
Position VisualPoint::leftWord(length_t words /* = 1 */) const {
	verifyViewer();
	return (viewer_->configuration().orientation == LEFT_TO_RIGHT) ? backwardWord(words) : forwardWord(words);
}

/**
 * Returns the end of the left N words.
 * @param words the number of words to advance
 */
Position VisualPoint::leftWordEnd(length_t words /* = 1 */) const {
	verifyViewer();
	return (viewer_->configuration().orientation == LEFT_TO_RIGHT) ? backwardWordEnd(words) : forwardWordEnd(words);
}

/// @internal @c Point#moveTo for @c PositionProxy.
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

/**
 * Breaks the line.
 * @note This methos hides @c EditPoint#newLine (C++ rule).
 * @param inheritIndent true to inherit the indent of the previous line
 * @param newlines the number of newlines to insert
 * @return false if the change was interrupted
 * @throw ReadOnlyDocumentException the document is read only
 */
bool VisualPoint::newLine(bool inheritIndent, size_t newlines /* = 1 */) {
	verifyViewer();
	if(document()->isReadOnly())
		throw ReadOnlyDocumentException();
	else if(newlines == 0)
		return true;

	const IDocumentInput* di = document()->input();
	String s(getNewlineString((di != 0) ? di->newline() : ASCENSION_DEFAULT_NEWLINE));

	if(inheritIndent) {	// simple auto-indent
		const String& currentLine = document()->line(lineNumber());
		const length_t len = identifierSyntax().eatWhiteSpaces(
			currentLine.data(), currentLine.data() + columnNumber(), true) - currentLine.data();
		s += currentLine.substr(0, len);
	}

	if(newlines > 1) {
		basic_stringbuf<Char> b;
		for(size_t i = 0; i < newlines; ++i)
			b.sputn(s.data(), static_cast<streamsize>(s.length()));
		s = b.str();
	}
	return insert(s);
}

/**
 * Inserts the content of the clipboard at the current position.
 * @return false if the change was interrupted
 * @throw ClipboardException the clipboard operation failed
 * @throw ClipboardException(DV_E_FORMATETC) the current clipboard format is not supported
 * @throw ReadOnlyDocumentException the document is read only
 * @throw bad_alloc internal memory allocation failed
 */
bool VisualPoint::paste() {
	verifyViewer();
	if(document()->isReadOnly())
		throw ReadOnlyDocumentException();
	else if(!canPaste())
		throw ClipboardException(DV_E_FORMATETC);

	IDataObject* content;
	HRESULT hr = tryOleClipboard(::OleGetClipboard, &content);
	if(hr == E_OUTOFMEMORY)
		throw bad_alloc("::OleGetClipboard returned E_OUTOFMEMORY.");
	else if(FAILED(hr))
		throw ClipboardException(hr);
	bool rectangle;
	const pair<HRESULT, String> text(getTextFromDataObject(*content, &rectangle));
	content->Release();
	if(text.first == E_OUTOFMEMORY)
		throw bad_alloc("getTextFromDataObject returned E_OUTOFMEMORY.");
	else if(FAILED(text.first))
		throw ClipboardException(text.first);
	return rectangle ? insertRectangle(text.second) : insert(text.second);
}

/// Centers the current visual line in the viewer by vertical scrolling the window.
void VisualPoint::recenter() {
	verifyViewer();
	// TODO: not implemented.
}

/**
 * Returns the right N characters.
 * @param words the number of characters to advance
 */
Position VisualPoint::rightCharacter(length_t characters /* = 1 */) const {
	verifyViewer();
	return (viewer_->configuration().orientation == LEFT_TO_RIGHT) ?
		forwardCharacter(characters) : backwardCharacter(characters);
}

/**
 * Returns the beginning of the right N words.
 * @param words the number of words to advance
 */
Position VisualPoint::rightWord(length_t words /* = 1 */) const {
	verifyViewer();
	return (viewer_->configuration().orientation == LEFT_TO_RIGHT) ? forwardWord(words) : backwardWord(words);
}

/**
 * Returns the end of the right N words.
 * @param words the number of words to advance
 */
Position VisualPoint::rightWordEnd(length_t words /* = 1 */) const {
	verifyViewer();
	return (viewer_->configuration().orientation == LEFT_TO_RIGHT) ? forwardWordEnd(words) : backwardWordEnd(words);
}

/// Scrolls the text viewer until the point is visible in the window.
void VisualPoint::show() {
	verifyViewer();
	const TextRenderer& renderer = viewer_->textRenderer();
	const length_t visibleLines = viewer_->numberOfVisibleLines();
	MANAH_AUTO_STRUCT_SIZE(SCROLLINFO, si);
	POINT to = {-1, -1};

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
}

/**
 * Indents the specified region by using horizontal tabs.
 * @param other もう1つの位置
 * @param rectangle set true for rectangular indents (will be ignored level is negative)
 * @param level the level of the indentation
 * @return インデント後に @a pos が移動すべき位置
 * @return false if the change was interrupted
 * @throw ReadOnlyDocumentException the document is read only
 * @deprecated 0.8
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
 * @return false if the change was interrupted
 * @throw ReadOnlyDocumentException the document is read only
 * @deprecated 0.8
 */
Position VisualPoint::tabIndent(const Position& other, bool rectangle, long level /* = 1 */) {
	verifyViewer();
	return doIndent(other, L'\t', rectangle, level);
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
void VisualPoint::visualLinesDeleted(length_t first, length_t last, length_t, bool) /*throw()*/ {
	if(!adaptsToDocument() && lineNumber() >= first && lineNumber() < last)
		visualLine_ = INVALID_INDEX;
}

/// @see IVisualLinesListener#visualLinesInserted
void VisualPoint::visualLinesInserted(length_t first, length_t last) /*throw()*/ {
	if(!adaptsToDocument() && lineNumber() >= first && lineNumber() < last)
		visualLine_ = INVALID_INDEX;
}

/// @see IVisualLinesListener#visualLinesModified
void VisualPoint::visualLinesModified(length_t first, length_t last, signed_length_t sublineDifference, bool, bool) /*throw()*/ {
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
 * @class ascension::viewers::Caret
 *
 * @c Caret is an extension of @c VisualPoint. A caret has a selection on the text viewer.
 * And supports line selection, word selection, rectangle (box) selection, tracking match
 * brackets, and clipboard enhancement.
 *
 * A caret has one another point called "anchor" (or "mark"). The selection is a region
 * between the caret and the anchor. Anchor is @c VisualPoint but client can't operate
 * this directly.
 *
 * Usually, the anchor will move adapting to the caret automatically. If you want to move
 * the anchor isolately, create the selection by using @c #select method or call
 * @c #extendSelection method.
 *
 * When the caret moves, the text viewer will scroll automatically to show the caret. See
 * the description of @c #enableAutoShow and @c #isAutoShowEnabled.
 *
 * このクラスの編集用のメソッドは @c EditPoint 、@c VisualPoint の編集用メソッドと異なり、
 * 積極的に連続編集とビューの凍結を使用する
 *
 * 行選択および単語選択は、選択の作成および拡張時にアンカーとキャレットを行境界や単語境界に束縛する機能で、
 * @c #extendSelection メソッドで実際にこれらの点が移動する位置を制限する。
 * また、この場合 @c #extendSelection を呼び出すとアンカーが自動的に移動する。
 * @c #beginLineSelection 、@c #beginWordSelection でこれらのモードに入ることができ、
 * @c #restoreSelectionMode で通常状態に戻ることができる。
 * また、これらのモードで @c #moveTo か @c #select を使っても通常状態に戻る
 *
 * 対括弧の検索はプログラムを編集しているときに役立つ機能で、キャレット位置に括弧があれば対応する括弧を検索する。
 * 括弧のペアを強調表示するのは、現時点ではビューの責任である
 *
 * To enter rectangle selection mode, call @c #beginRectangleSelection method. To exit,
 * call @c #endRectangleSelection method. You can get the information of the current
 * rectangle selection by using @c #boxForRectangleSelection method.
 *
 * This class does not accept @c IPointListener. Use @c ICaretListener interface instead.
 *
 * @note This class is not intended to subclass.
 */

/**
 * Constructor.
 * @param viewer the viewer
 * @param position the initial position of the point
 * @throw BadPositionException @a position is outside of the document
 */
Caret::Caret(TextViewer& viewer, const Position& position /* = Position() */) /*throw()*/ : VisualPoint(viewer, position, 0),
		anchor_(new SelectionAnchor(viewer)), clipboardLocale_(::GetUserDefaultLCID()),
		yanking_(false), leaveAnchorNext_(false), leadingAnchor_(false), autoShow_(true), box_(0),
		matchBracketsTrackingMode_(DONT_TRACK), overtypeMode_(false), callingDocumentInsertForTyping_(false),
		lastTypedPosition_(Position::INVALID_POSITION), regionBeforeMoved_(Position::INVALID_POSITION, Position::INVALID_POSITION),
		matchBrackets_(make_pair(Position::INVALID_POSITION, Position::INVALID_POSITION)) {
	document()->addListener(*this);
}

/// Destructor.
Caret::~Caret() /*throw()*/ {
	if(Document* const d = document())
		d->removeListener(*this);
	delete anchor_;
	delete box_;
}

/**
 * Registers the listener.
 * @param listener the listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
void Caret::addListener(ICaretListener& listener) {
	listeners_.add(listener);
}

/**
 * Registers the character input listener.
 * @param listener the listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
void Caret::addCharacterInputListener(ICharacterInputListener& listener) {
	characterInputListeners_.add(listener);
}

/**
 * Registers the state listener.
 * @param listener the listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
void Caret::addStateListener(ICaretStateListener& listener) {
	stateListeners_.add(listener);
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

/// Clears the selection. The anchor will move to the caret.
void Caret::clearSelection() {
	endRectangleSelection();
	leaveAnchorNext_ = false;
	moveTo(*this);
}

/**
 * Copies the selected content to the clipboard.
 * @param useKillRing true to send to also the kill ring
 * @throw ClipboardException the clipboard operation failed
 * @throw bad_alloc internal memory allocation failed
 */
void Caret::copySelection(bool useKillRing) {
	verifyViewer();
	if(isSelectionEmpty())
		return;

	IDataObject* data;
	HRESULT hr = createTextObject(true, data);
	if(hr == E_OUTOFMEMORY)
		throw bad_alloc("Caret.createTextObject returned E_OUTOFMEMORY.");
	hr = tryOleClipboard(::OleSetClipboard, data);
	if(FAILED(hr)) {
		data->Release();
		throw ClipboardException(hr);
	}
	hr = tryOleClipboard(::OleFlushClipboard);
	data->Release();
	if(useKillRing) {
		if(texteditor::Session* const session = document()->session())
			session->killRing().addNew(selectionText(NLF_RAW_VALUE), isSelectionRectangle());
	}
}

/**
 * Creates an IDataObject represents the selected content.
 * @param rtf set true if the content is available as Rich Text Format. this feature is not
 * implemented yet and the parameter is ignored
 * @param[out] content the data object
 * @retval S_OK succeeded
 * @retval E_OUTOFMEMORY failed to allocate memory for @a content
 */
HRESULT Caret::createTextObject(bool rtf, IDataObject*& content) const {
	GenericDataObject* o = new(nothrow) GenericDataObject();
	if(o == 0)
		return E_OUTOFMEMORY;
	o->AddRef();

	// get text on the given region
	const String text(selectionText(NLF_CR_LF));

	// register datas...
	FORMATETC format;
	format.ptd = 0;
	format.dwAspect = DVASPECT_CONTENT;
	format.lindex = -1;
	format.tymed = TYMED_HGLOBAL;
	STGMEDIUM medium;
	medium.tymed = TYMED_HGLOBAL;
	medium.pUnkForRelease = 0;

	// Unicode text format
	format.cfFormat = CF_UNICODETEXT;
	medium.hGlobal = ::GlobalAlloc(GHND | GMEM_SHARE, sizeof(Char) * (text.length() + 1));
	if(medium.hGlobal == 0) {
		o->Release();
		return E_OUTOFMEMORY;
	}
	wcscpy(static_cast<wchar_t*>(::GlobalLock(medium.hGlobal)), text.c_str());
	::GlobalUnlock(medium.hGlobal);
	HRESULT hr = o->SetData(&format, &medium, false);

	// rectangle text format
	if(isSelectionRectangle()) {
		if(0 != (format.cfFormat = static_cast<CLIPFORMAT>(::RegisterClipboardFormatW(ASCENSION_RECTANGLE_TEXT_CLIP_FORMAT))))
			hr = o->SetData(&format, &medium, false);
	}

	::GlobalFree(medium.hGlobal);

	// ANSI text format and locale
	hr = S_OK;
	UINT codePage = CP_ACP;
	wchar_t codePageString[6];
	if(0 != ::GetLocaleInfoW(clipboardLocale_, LOCALE_IDEFAULTANSICODEPAGE, codePageString, MANAH_COUNTOF(codePageString))) {
		wchar_t* eob;
		codePage = wcstoul(codePageString, &eob, 10);
		format.cfFormat = CF_TEXT;
		if(int ansiLength = ::WideCharToMultiByte(codePage, 0, text.c_str(), static_cast<int>(text.length()), 0, 0, 0, 0)) {
			manah::AutoBuffer<char> ansiBuffer(new(nothrow) char[ansiLength]);
			if(ansiBuffer.get() != 0) {
				ansiLength = ::WideCharToMultiByte(codePage, 0,
					text.data(), static_cast<int>(text.length()), ansiBuffer.get(), ansiLength, 0, 0);
				if(ansiLength != 0) {
					if(0 != (medium.hGlobal = ::GlobalAlloc(GHND | GMEM_SHARE, sizeof(char) * (ansiLength + 1)))) {
						if(char* const temp = static_cast<char*>(::GlobalLock(medium.hGlobal))) {
							memcpy(temp, ansiBuffer.get(), sizeof(char) * ansiLength);
							temp[ansiLength] = 0;
							::GlobalUnlock(medium.hGlobal);
							hr = o->SetData(&format, &medium, false);
						} else
							hr = E_FAIL;
						::GlobalFree(medium.hGlobal);
						if(SUCCEEDED(hr)) {
							format.cfFormat = CF_LOCALE;
							if(0 != (medium.hGlobal = ::GlobalAlloc(GHND | GMEM_SHARE, sizeof(LCID)))) {
								if(LCID* const lcid = static_cast<LCID*>(::GlobalLock(medium.hGlobal))) {
									*lcid = clipboardLocale_;
									hr = o->SetData(&format, &medium, false);
								}
								::GlobalUnlock(medium.hGlobal);
								::GlobalFree(medium.hGlobal);
							}
						}
					}
				}
			}
		}
	}

	if(rtf) {
		const CLIPFORMAT rtfFormat = static_cast<CLIPFORMAT>(::RegisterClipboardFormatW(L"Rich Text Format"));	// CF_RTF
		const CLIPFORMAT rtfWithoutObjectsFormat = static_cast<CLIPFORMAT>(::RegisterClipboardFormatW(L"Rich Text Format Without Objects"));	// CF_RTFNOOBJS
		// TODO: implement the follow...
	}

	content = o;
	return S_OK;
}

/**
 * Copies and deletes the selected text. If the selection is empty, this method does nothing.
 * @param useKillRing true to send also the kill ring
 * @return false if the change was interrupted
 * @throw ClipboardException the clipboard operation failed
 * @throw ReadOnlyDocumentException the document is read only
 * @throw bad_alloc internal memory allocation failed
 */
bool Caret::cutSelection(bool useKillRing) {
	verifyViewer();
	if(isSelectionEmpty())
		return true;
	else if(document()->isReadOnly())
		throw ReadOnlyDocumentException();

	manah::com::ComPtr<IDataObject> previousData;
	HRESULT hr = tryOleClipboard(::OleGetClipboard, previousData.initialize());
	if(hr == E_OUTOFMEMORY)
		throw bad_alloc("::OleGetClipboard returned E_OUTOFMEMORY.");
	else if(FAILED(hr))
		throw ClipboardException(hr);
	copySelection(useKillRing);	// this may throw
	if(!eraseSelection()) {
		hr = tryOleClipboard(::OleSetClipboard, previousData.get());
		return false;
	}
	return true;
}

/// @see kernel#IDocumentListener#documentAboutToBeChanged
void Caret::documentAboutToBeChanged(const Document&, const DocumentChange&) {
	if(lastTypedPosition_ != Position::INVALID_POSITION && !callingDocumentInsertForTyping_) {
		document()->insertUndoBoundary();
		lastTypedPosition_ = Position::INVALID_POSITION;
	}
}

/// @see kernel#IDocumentListener#documentChanged
void Caret::documentChanged(const Document&, const DocumentChange&) {
	yanking_ = false;
	if(regionBeforeMoved_.first != Position::INVALID_POSITION)
		updateVisualAttributes();
}

/// @see VisualPoint#doMoveTo
void Caret::doMoveTo(const Position& to) {
	regionBeforeMoved_ = Region(anchor_->isInternalUpdating() ?
		anchor_->positionBeforeInternalUpdate() : anchor_->position(), position());
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
 * Deletes the selected text. This method ends the rectangle selection mode.
 * @return false if the change was interrupted
 * @throw ReadOnlyDocumentException the document is read only
 */
bool Caret::eraseSelection() {
	verifyViewer();
	Document& doc = *document();
	if(doc.isReadOnly())
		throw ReadOnlyDocumentException();
	else if(isSelectionEmpty())
		return true;
	else if(!isSelectionRectangle()) {	// the selection is linear
		const Position to(min<Position>(*anchor_, *this));
		if(!doc.erase(*anchor_, *this))
			return false;
		moveTo(to);
		return true;
	} else {	// the selection is rectangle
		const Position resultPosition(beginning());
		const bool adapts = adaptsToDocument();
		adaptToDocument(false);
		const length_t firstLine = beginning().lineNumber(), lastLine = end().lineNumber();
		pair<length_t, length_t> rangeInLine;
		bool interrupted = false;

		// rectangle deletion can't delete newline characters

		textViewer().freeze();
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
				if(!interrupted) {
					const bool temp = !doc.erase(Position(points[i]->lineNumber(), points[i]->columnNumber()),
						Position(points[i]->lineNumber(), points[i]->columnNumber() + sizes[i]));
					if(i == 0 && !temp)
						interrupted = true;
				}
				delete points[i];
			}
		} else {
			for(length_t line = resultPosition.line; line <= lastLine; ++line) {
				box_->overlappedSubline(line, 0, rangeInLine.first, rangeInLine.second);
				const bool temp = doc.erase(Position(line, rangeInLine.first), Position(line, rangeInLine.second));
				if(line == resultPosition.line && !temp) {
					interrupted = true;
					break;
				}
			}
		}
		textViewer().unfreeze();
		adaptToDocument(adapts);
		if(!interrupted) {
			endRectangleSelection();
			moveTo(resultPosition);
		}
		return !interrupted;
	}
}

/**
 * Moves to the specified position without the anchor adapting.
 * @param to the destination position
 */
void Caret::extendSelection(const Position& to) {
	verifyViewer();
	leaveAnchorNext_ = true;
	moveTo(to);
	leaveAnchorNext_ = false;
}

/**
 * Moves to the specified position without the anchor adapting.
 * @param to the destination position
 */
void Caret::extendSelection(const VerticalDestinationProxy& to) {
	verifyViewer();
	leaveAnchorNext_ = true;
	moveTo(to);
	leaveAnchorNext_ = false;
}

/**
 * Inputs the specified character at current position.
 * <p>If the selection is not empty, replaces the selected region. Otherwise if in overtype mode,
 * replaces a character at current position (but this does not erase newline character).</p>
 * <p>This method may insert undo boundaries for compound typing.</p>
 * @param cp the code point of the character to be input
 * @param validateSequence true to perform input sequence check using the active ISC
 * @param blockControls true to refuse any ASCII control characters except HT (U+0009), RS (U+001E)
 * and US (U+001F)
 * @return false if the input was refused because of @a validateSequence or @a blockControls, or 
 * the change was interrupted
 * @throw ReadOnlyDocumentException the document is read only
 * @see #isOvertypeMode, #setOvertypeMode, texteditor#commands#TextInputCommand
 */
bool Caret::inputCharacter(CodePoint cp, bool validateSequence /* = true */, bool blockControls /* = true */) {
	verifyViewer();

	Document& doc = *document();
	if(doc.isReadOnly())
		throw ReadOnlyDocumentException();
	else if(blockControls && cp <= 0xFF && cp != 0x09 && cp != 0x1E && cp != 0x1F && toBoolean(iscntrl(static_cast<int>(cp))))
		return false;

	// check the input sequence
	if(validateSequence) {
		if(const texteditor::Session* const session = doc.session()) {
			if(const texteditor::InputSequenceCheckers* const checker = session->inputSequenceCheckers()) {
				const Char* const line = doc.line(beginning().lineNumber()).data();
				if(!checker->check(line, line + beginning().columnNumber(), cp)) {
					eraseSelection();
					return false;	// invalid sequence
				}
			}
		}
	}

	bool interrupted;
	Char buffer[2];
	surrogates::encode(cp, buffer);
	if(!isSelectionEmpty()) {	// just replace if the selection is not empty
		doc.insertUndoBoundary();
		if(!(interrupted = !replaceSelection(buffer, buffer + ((cp < 0x10000) ? 1 : 2))))
			doc.insertUndoBoundary();
	} else if(overtypeMode_) {
		textViewer().freeze(true);
		doc.insertUndoBoundary();
		if(!(interrupted = !destructiveInsert(buffer, buffer + ((cp < 0x10000) ? 1 : 2))))
			doc.insertUndoBoundary();
		textViewer().unfreeze(true);
	} else {
		const bool alpha = identifierSyntax().isIdentifierContinueCharacter(cp);
		if(lastTypedPosition_ != Position::INVALID_POSITION && (!alpha || lastTypedPosition_ != position())) {
			// end sequential typing
			doc.insertUndoBoundary();
			lastTypedPosition_ = Position::INVALID_POSITION;
		}
		if(alpha && lastTypedPosition_ == Position::INVALID_POSITION)
			// (re)start sequential typing
			doc.insertUndoBoundary();

		callingDocumentInsertForTyping_ = true;
		interrupted = !insert(buffer, buffer + ((cp < 0x10000) ? 1 : 2));
		callingDocumentInsertForTyping_ = false;
		if(!interrupted && alpha)
			lastTypedPosition_ = position();
	}

	if(interrupted)
		return false;
	characterInputListeners_.notify<const Caret&, CodePoint>(&ICharacterInputListener::characterInputted, *this, cp);
	return true;
}

/**
 * Returns true if the specified point is over the selection.
 * @param pt the client coordinates of the point
 * @return true if the point is over the selection
 */
bool Caret::isPointOverSelection(const POINT& pt) const {
	verifyViewer();
	if(isSelectionEmpty())
		return false;
	else if(isSelectionRectangle())
		return box_->isPointOver(pt);
	else {
		if(textViewer().hitTest(pt) != TextViewer::TEXT_AREA)	// ignore if on the margin
			return false;
		RECT rect;
		textViewer().getClientRect(rect);
		if(pt.x > rect.right || pt.y > rect.bottom)
			return false;
		const Position pos(textViewer().characterForClientXY(pt, LineLayout::TRAILING));
		return pos >= beginning() && pos <= end();
	}
}

/**
 * Replaces the selected text by the content of the clipboard.
 * This method inserts undo boundaries at the beginning and the end.
 * @param useKillRing true to use the kill ring
 * @return false if the change was interrupted
 * @throw ClipboardException the clipboard operation failed
 * @throw ClipboardException(DV_E_FORMATETC) the current clipboard format is not supported
 * @throw ReadOnlyDocumentException the document is read only
 * @throw bad_alloc internal memory allocation failed
 */
bool Caret::pasteToSelection(bool useKillRing) {
	verifyViewer();
	if(document()->isReadOnly())
		throw ReadOnlyDocumentException();
	texteditor::Session* const session = document()->session();
	if(useKillRing && (session == 0 || session->killRing().numberOfKills() == 0))
		return true;

	textViewer().freeze(true);
	document()->insertUndoBoundary();
	bool failed = true;
	if(!useKillRing) {
		if(!(failed = !eraseSelection())) {	// if there is no selection, this does nothing
			try {
				failed = !paste();	// TODO: if threw, deleted content is lost.
			} catch(ClipboardException&) {
				textViewer().unfreeze(true);
				throw;
			}
		}
	} else {
		texteditor::KillRing& killRing = session->killRing();
		const pair<String, bool>& text = yanking_ ? killRing.setCurrent(+1) : killRing.get();

		if(!isSelectionEmpty()) {
			if(yanking_)
				document()->undo();
			failed = !eraseSelection();
		}
		if(!failed) {
			const Position p(position());
			if(!text.second) {
				if(!(failed = !insert(text.first)))
					endRectangleSelection();
			} else {
				if(!(failed = !insertRectangle(text.first)))
					beginRectangleSelection();
			}
			select(p, position());
			yanking_ = true;
		}
	}
	document()->insertUndoBoundary();
	textViewer().unfreeze(true);
	return !failed;
}

/// @see IPointListener#pointMoved
void Caret::pointMoved(const EditPoint& self, const Position& oldPosition) {
	assert(&self == &*anchor_);
	yanking_ = false;
	if(leadingAnchor_)	// doMoveTo で anchor_->moveTo 呼び出し中
		return;
	if((oldPosition == position()) != isSelectionEmpty())
		checkMatchBrackets();
	listeners_.notify<const Caret&, const Region&>(&ICaretListener::caretMoved, *this, Region(oldPosition, position()));
}

/**
 * Removes the listener
 * @param listener the listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
void Caret::removeListener(ICaretListener& listener) {
	listeners_.remove(listener);
}

/**
 * Removes the character input listener
 * @param listener the listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
void Caret::removeCharacterInputListener(ICharacterInputListener& listener) {
	characterInputListeners_.remove(listener);
}

/**
 * Removes the state listener
 * @param listener the listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
void Caret::removeStateListener(ICaretStateListener& listener) {
	stateListeners_.remove(listener);
}

/**
 * Replaces the selected region with the specified text.
 * If the selection is empty, inserts the text at current position.
 * @param first the start of the text
 * @param last the end of the text
 * @param rectangleInsertion true to insert text as rectangle
 * @return false if the change was interrupted
 * @throw ReadOnlyDocumentException the document is read only
 * @throw NullPointerException @a first and/or @last is @c null
 * @throw std#invalid_argument @a first &gt; @a last
 */
bool Caret::replaceSelection(const Char* first, const Char* last, bool rectangleInsertion /* = false */) {
	verifyViewer();
	if(document()->isReadOnly())
		throw ReadOnlyDocumentException();
	else if(first == 0)
		throw NullPointerException("first");
	else if(last == 0)
		throw NullPointerException("last");
	else if(first > last)
		throw invalid_argument("first > last");
	const Region oldRegion(selectionRegion());
	textViewer().freeze(true);
	const bool interrupted = !eraseSelection();
	if(!interrupted) {
		// TODO: insert and insertRectangle may throw and breaks strong guarentee for exception^safety.
		if(rectangleInsertion)
			insertRectangle(first, last);
		else
			insert(first, last);
	}
	textViewer().unfreeze(true);
	return !interrupted;
}

/**
 * Selects the specified region. The active selection mode will be cleared.
 * @param anchor the position where the anchor moves to
 * @param caret the position where the caret moves to
 */
void Caret::select(const Position& anchor, const Position& caret) {
	verifyViewer();
	yanking_ = false;
	if(anchor != anchor_->position() || caret != position()) {
		const Region oldRegion(selectionRegion());
		leadingAnchor_ = true;
		anchor_->moveTo(anchor);
		leadingAnchor_ = false;
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

/// Selects the word at the caret position. This creates a linear selection.
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
 * Sets the locale used to convert non-Unicode data in the clipboard.
 * @param newLocale the locale identifier
 * @return the identifier of the locale set by the caret
 * @throw std#invalid_argument @a newLocale is not installed on the system
 */
LCID Caret::setClipboardLocale(LCID newLocale) {
	if(!toBoolean(::IsValidLocale(newLocale, LCID_INSTALLED)))
		throw invalid_argument("newLocale");
	swap(clipboardLocale_, newLocale);
	return newLocale;
}

/**
 * Sets character input mode.
 * @param overtype true to set to overtype mode, false to set to insert mode
 * @return this caret
 * @see #inputCharacter, #isOvertypeMode
 */
Caret& Caret::setOvertypeMode(bool overtype) /*throw()*/ {
	if(overtype != overtypeMode_) {
		overtypeMode_ = overtype;
		stateListeners_.notify<const Caret&>(&ICaretStateListener::overtypeModeChanged, *this);
	}
	return *this;
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
