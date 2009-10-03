/**
 * @file caret.cpp
 * @author exeal
 * @date 2003-2008 was point.cpp
 * @date 2008-2009 separated from point.cpp
 */

#undef MANAH_OVERRIDDEN_FILE
static const char MANAH_OVERRIDDEN_FILE[] = __FILE__;

#include <ascension/viewer.hpp>
#include <ascension/session.hpp>
#include <manah/win32/utility.hpp>
using namespace ascension;
using namespace ascension::kernel;
using namespace ascension::layout;
using namespace ascension::viewers;
using namespace ascension::presentation;
using namespace ascension::text;
using namespace ascension::text::ucd;
using namespace manah;
using namespace std;


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

namespace {
	// copied from point.cpp
	inline const IdentifierSyntax& identifierSyntax(const Point& p) {
		return p.document().contentTypeInformation().getIdentifierSyntax(p.contentType());
	}
} // namespace @0

/**
 * Creates an @c IDataObject represents the selected content.
 * @param caret the caret gives the selection
 * @param rtf set true if the content is available as Rich Text Format. this feature is not
 * implemented yet and the parameter is ignored
 * @param[out] content the data object
 * @retval S_OK succeeded
 * @retval E_OUTOFMEMORY failed to allocate memory for @a content
 */
HRESULT utils::createTextObjectForSelectedString(const Caret& caret, bool rtf, IDataObject*& content) {
	GenericDataObject* o = new(nothrow) GenericDataObject();
	if(o == 0)
		return E_OUTOFMEMORY;
	o->AddRef();

	// get text on the given region
	const String text(selectedString(caret, NLF_CR_LF));

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
	if(caret.isSelectionRectangle()) {
		if(0 != (format.cfFormat = static_cast<CLIPFORMAT>(::RegisterClipboardFormatW(ASCENSION_RECTANGLE_TEXT_CLIP_FORMAT))))
			hr = o->SetData(&format, &medium, false);
	}

	::GlobalFree(medium.hGlobal);

	// ANSI text format and locale
	hr = S_OK;
	UINT codePage = CP_ACP;
	wchar_t codePageString[6];
	if(0 != ::GetLocaleInfoW(caret.clipboardLocale(), LOCALE_IDEFAULTANSICODEPAGE, codePageString, MANAH_COUNTOF(codePageString))) {
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
									*lcid = caret.clipboardLocale();
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
 * Returns the text content from the given data object.
 * @param data the data object
 * @param[out] rectangle true if the data is rectangle format. can be @c null
 * @return a pair of the result HRESULT and the text content. SCODE is one of S_OK, E_OUTOFMEMORY and DV_E_FORMATETC
 */
pair<HRESULT, String> utils::getTextFromDataObject(IDataObject& data, bool* rectangle /* = 0 */) {
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

/**
 * Centers the current visual line addressed by the given visual point in the text viewer by
 * vertical scrolling the window.
 * @param p the visual point
 * @throw DocumentDisposedException 
 * @throw TextViewerDisposedException 
 */
void utils::recenter(VisualPoint& p) {
	// TODO: not implemented.
}

/**
 * Scrolls the text viewer until the given point is visible in the window.
 * @param p the visual point. this position will be normalized before the process
 * @throw DocumentDisposedException 
 * @throw TextViewerDisposedException 
 */
void utils::show(VisualPoint& p) {
	TextViewer& viewer = p.textViewer();
	const Position np(p.normalized());
	const TextRenderer& renderer = viewer.textRenderer();
	const length_t visibleLines = viewer.numberOfVisibleLines();
	win32::AutoZeroSize<SCROLLINFO> si;
	POINT to = {-1, -1};

	// for vertical direction
	si.fMask = SIF_POS;
	viewer.getScrollInformation(SB_VERT, si);
	if(p.visualLine() < si.nPos * viewer.scrollRate(false))	// 画面より上
		to.y = static_cast<long>(p.visualLine() * viewer.scrollRate(false));
	else if(p.visualLine() - si.nPos * viewer.scrollRate(false) > visibleLines - 1)	// 画面より下
		to.y = static_cast<long>((p.visualLine() - visibleLines + 1) * viewer.scrollRate(false));
	if(to.y < -1)
		to.y = 0;

	// for horizontal direction
	if(!viewer.configuration().lineWrap.wrapsAtWindowEdge()) {
		const length_t visibleColumns = viewer.numberOfVisibleColumns();
		const ulong x = renderer.lineLayout(np.line).location(np.column, LineLayout::LEADING).x + renderer.lineIndent(np.line, 0);
		viewer.getScrollInformation(SB_HORZ, si);
		const ulong scrollOffset = si.nPos * viewer.scrollRate(true) * renderer.averageCharacterWidth();
		if(x <= scrollOffset)	// 画面より左
			to.x = x / renderer.averageCharacterWidth() - visibleColumns / 4;
		else if(x >= (si.nPos * viewer.scrollRate(true) + visibleColumns) * renderer.averageCharacterWidth())	// 画面より右
			to.x = x / renderer.averageCharacterWidth() - visibleColumns * 3 / 4;
		if(to.x < -1)
			to.x = 0;
	}
	if(to.x >= -1 || to.y != -1)
		viewer.scrollTo(to.x, to.y, true);
}


// TextViewerDisposedException //////////////////////////////////////////////

TextViewerDisposedException::TextViewerDisposedException() :
		logic_error("The text viewer the object connecting to has been disposed.") {
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
		Point(viewer.document(), position, listener), viewer_(&viewer),
		lastX_(-1), crossingLines_(false), visualLine_(INVALID_INDEX), visualSubline_(0) {
	static_cast<kernel::internal::IPointCollection<VisualPoint>&>(viewer).addNewPoint(*this);
	viewer_->textRenderer().addVisualLinesListener(*this);
}

/**
 * Copy-constructor.
 * @param other the source object
 * @throw DocumentDisposedException the document to which @a other belongs had been disposed
 * @throw TextViewerDisposedException the text viewer to which @a other belongs had been disposed
 */
VisualPoint::VisualPoint(const VisualPoint& other) : Point(other), viewer_(other.viewer_),
		lastX_(other.lastX_), crossingLines_(false), visualLine_(other.visualLine_), visualSubline_(other.visualSubline_) {
	if(viewer_ == 0)
		throw TextViewerDisposedException();
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
	if(from.line == line() && visualLine_ != INVALID_INDEX) {
		const LineLayout* layout = viewer_->textRenderer().lineLayoutIfCached(line());
		visualLine_ -= visualSubline_;
		visualSubline_ = (layout != 0) ? layout->subline(column()) : 0;
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
 * @param first the start of the text
 * @param last the end of the text
 * @throw NullPointerException @a first and/or @a last are @c null
 * @throw std#invalid_argument @a first &gt; @a last
 * @throw ... any exceptions @c Document#insert throws
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
		const LineLayout& layout = textViewer().textRenderer().lineLayout(line());
		lastX_ = layout.location(column(), LineLayout::LEADING).x;
		lastX_ += textViewer().textRenderer().lineIndent(line(), 0);
	}
}

/// Returns the visual column of the point.
length_t VisualPoint::visualColumn() const {
	if(lastX_ == -1)
		const_cast<VisualPoint*>(this)->updateLastX();
	const TextViewer::Configuration& c = viewer_->configuration();
	const TextRenderer& renderer = viewer_->textRenderer();
	if(c.alignment == ALIGN_LEFT || (c.alignment != ALIGN_RIGHT && c.orientation == LEFT_TO_RIGHT))
		return lastX_ / renderer.averageCharacterWidth();
	else
		return (renderer.getWidth() - lastX_) / renderer.averageCharacterWidth();
}

/// Returns the visual line number.
length_t VisualPoint::visualLine() const {
	if(visualLine_ == INVALID_INDEX) {
		VisualPoint& self = const_cast<VisualPoint&>(*this);
		const Position p(normalized());
		self.visualLine_ = textViewer().textRenderer().mapLogicalLineToVisualLine(p.line);
		self.visualSubline_ = textViewer().textRenderer().lineLayout(p.line).subline(p.column);
		self.visualLine_ += visualSubline_;
	}
	return visualLine_;
}

/// @see IVisualLinesListener#visualLinesDeleted
void VisualPoint::visualLinesDeleted(length_t first, length_t last, length_t, bool) /*throw()*/ {
	if(!adaptsToDocument() && line() >= first && line() < last)
		visualLine_ = INVALID_INDEX;
}

/// @see IVisualLinesListener#visualLinesInserted
void VisualPoint::visualLinesInserted(length_t first, length_t last) /*throw()*/ {
	if(!adaptsToDocument() && line() >= first && line() < last)
		visualLine_ = INVALID_INDEX;
}

/// @see IVisualLinesListener#visualLinesModified
void VisualPoint::visualLinesModified(length_t first, length_t last, signed_length_t sublineDifference, bool, bool) /*throw()*/ {
	if(visualLine_ != INVALID_INDEX) {
		// adjust visualLine_ and visualSubine_ according to the visual lines modification
		if(last <= line())
			visualLine_ += sublineDifference;
		else if(first == line()) {
			visualLine_ -= visualSubline_;
			visualSubline_ = textViewer().textRenderer().lineLayout(line()).subline(min(column(), document().lineLength(line())));
			visualLine_ += visualSubline_;
		} else if(first < line())
			visualLine_ = INVALID_INDEX;
	}
}

// Caret ////////////////////////////////////////////////////////////////////

// TODO: rewrite this documentation.

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
 * @c Caret hides @c Point#excludeFromRestriction and can't enter the inaccessible region of the
 * document. @c #isExcludedFromRestriction always returns @c true.
 *
 * @c Caret throws @c ReadOnlyDocumentException when tried to change the read-only document.
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
		matchBracketsTrackingMode_(DONT_TRACK), overtypeMode_(false), typing_(false),
		lastTypedPosition_(Position::INVALID_POSITION), regionBeforeMoved_(Position::INVALID_POSITION, Position::INVALID_POSITION),
		matchBrackets_(make_pair(Position::INVALID_POSITION, Position::INVALID_POSITION)) {
	document().addListener(*this);
}

/// Destructor.
Caret::~Caret() /*throw()*/ {
	if(!isDocumentDisposed())
		document().removeListener(*this);
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
	if(box_ == 0) {
		box_ = new VirtualBox(textViewer(), selectedRegion());
		stateListeners_.notify<const Caret&>(&ICaretStateListener::selectionShapeChanged, *this);
	}
}

/**
 * Returns true if a paste operation can be performed.
 * @note Even when this method returned @c true, the following @c #paste call can fail.
 * @param useKillRing set @c true to get the content from the kill-ring of the session, not from
 *                    the system clipboard
 * @return true if the clipboard data is pastable
 */
bool Caret::canPaste(bool useKillRing) const {
	if(!useKillRing) {
		const UINT rectangleClipFormat = ::RegisterClipboardFormatW(ASCENSION_RECTANGLE_TEXT_CLIP_FORMAT);
		if(rectangleClipFormat != 0 && toBoolean(::IsClipboardFormatAvailable(rectangleClipFormat)))
			return true;
		else if(toBoolean(::IsClipboardFormatAvailable(CF_UNICODETEXT)) || toBoolean(::IsClipboardFormatAvailable(CF_TEXT)))
			return true;
	} else {
		if(const texteditor::Session* const session = document().session())
			return session->killRing().numberOfKills() != 0;
	}
	return false;
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

/// @see kernel#IDocumentListener#documentAboutToBeChanged
void Caret::documentAboutToBeChanged(const Document&) {
	// does nothing
}

/// @see kernel#IDocumentListener#documentChanged
void Caret::documentChanged(const Document&, const DocumentChange&) {
	yanking_ = false;
	if(regionBeforeMoved_.first != Position::INVALID_POSITION)
		updateVisualAttributes();
}

/// @see VisualPoint#aboutToMove
void Caret::aboutToMove(Position& to) {
	VisualPoint::aboutToMove(to);
}

/// @see VisualPoint#moved
void Caret::moved(const Position& from) {
	regionBeforeMoved_ = Region(anchor_->isInternalUpdating() ?
		anchor_->positionBeforeInternalUpdate() : anchor_->position(), from);
	if(leaveAnchorNext_)
		leaveAnchorNext_ = false;
	else {
		leadingAnchor_ = true;
		anchor_->moveTo(position());
		leadingAnchor_ = false;
	}
	VisualPoint::moved(from);
	if(!document().isChanging())
		updateVisualAttributes();
}

/**
 * Ends the rectangular selection.
 * @see #beginRectangleSelection, #isSelectionRectangle
 */
void Caret::endRectangleSelection() {
	if(isTextViewerDisposed())
		throw TextViewerDisposedException();
	if(box_ != 0) {
		delete box_;
		box_ = 0;
		stateListeners_.notify<const Caret&>(&ICaretStateListener::selectionShapeChanged, *this);
	}
}
#if 0
/**
 * Deletes the selected text. This method ends the rectangle selection mode.
 * @throw DocumentAccessViolationException the selected region intersects the inaccesible region
 * @throw ... any exceptions @c Document#erase throws
 */
void Caret::eraseSelection() {
	verifyViewer();
	Document& doc = *document();
	if(isSelectionEmpty())
		return;
	else if(!isSelectionRectangle()) {	// the selection is linear
		const Position to(min<Position>(*anchor_, *this));
		doc.erase(*anchor_, *this);	// this can throw all exceptions this method throws
		moveTo(to);
	} else {	// the selection is rectangle
		DocumentLocker lock(*document());
		const Position resultPosition(beginning());
		const bool adapts = adaptsToDocument();
		adaptToDocument(false);
		const length_t firstLine = beginning().lineNumber(), lastLine = end().lineNumber();
		pair<length_t, length_t> rangeInLine;
		bool interrupted = false;

		// rectangle deletion can't delete newline characters
		{
			AutoFreeze af(&textViewer());
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
						try {
							doc.erase(Position(points[i]->lineNumber(), points[i]->columnNumber()),
								Position(points[i]->lineNumber(), points[i]->columnNumber() + sizes[i]));
						} catch(...) {
							if(i == 0) {
								while(i < sublines)
									delete points[i++];
								adaptToDocument(adapts);
								throw;
							}
						}
					}
					delete points[i];
				}
			} else {
				for(length_t line = resultPosition.line; line <= lastLine; ++line) {
					box_->overlappedSubline(line, 0, rangeInLine.first, rangeInLine.second);
					try {
						doc.erase(Position(line, rangeInLine.first), Position(line, rangeInLine.second));
					} catch(...) {
						if(line == resultPosition.line) {
							adaptToDocument(adapts);
							throw;
						}
					}
				}
			}
		}
		adaptToDocument(adapts);
		endRectangleSelection();
		moveTo(resultPosition);
	}
}
#endif
/**
 * Moves to the specified position without the anchor adapting.
 * @param to the destination position
 */
void Caret::extendSelection(const Position& to) {
	leaveAnchorNext_ = true;
	try {
		moveTo(to);
	} catch(...) {
		leaveAnchorNext_ = false;
		throw;
	}
	leaveAnchorNext_ = false;
}

/**
 * Moves to the specified position without the anchor adapting.
 * @param to the destination position
 */
void Caret::extendSelection(const VerticalDestinationProxy& to) {
	leaveAnchorNext_ = true;
	try {
		moveTo(to);
	} catch(...) {
		leaveAnchorNext_ = false;
		throw;
	}
	leaveAnchorNext_ = false;
}

namespace {
	/**
	 * @internal Deletes the forward one character and inserts the specified text.
	 * This function emulates keyboard overtyping input.
	 * @param caret the caret
	 * @param first the beginning of the text
	 * @param last the end of the text
	 * @param keepNewline set @c false to overwrite a newline characer
	 * @throw NullPointerException @a first is @c null
	 * @throw DocumentDisposedException
	 * @throw TextViewerDisposedException
	 * @throw ... any exceptions @c Document#replace throws
	 */
	void destructiveInsert(Caret& caret, const Char* first, const Char* last, bool keepNewline = true) {
		if(first == 0)
			throw NullPointerException("first");
		const bool adapts = caret.adaptsToDocument();
		caret.adaptToDocument(false);
		Position e((keepNewline && locations::isEndOfLine(caret)) ?
			caret.position() : locations::forwardCharacter(caret, locations::GRAPHEME_CLUSTER));
		if(e != caret.position()) {
			try {
				caret.document().replace(Region(caret.position(), e), first, last, &e);
			} catch(...) {
				caret.adaptToDocument(adapts);
				throw;
			}
			caret.moveTo(e);
		}
		caret.adaptToDocument(adapts);
	}
} // namespace @0

/**
 * Inputs the specified character at current position.
 * <p>If the selection is not empty, replaces the selected region. Otherwise if in overtype mode,
 * replaces a character at current position (but this does not erase newline character).</p>
 * <p>This method may insert undo boundaries for compound typing.</p>
 * @param character the code point of the character to input
 * @param validateSequence set @c true to perform input sequence check using the active ISC. see
 *                         @c texteditor#InputSequenceCheckers
 * @param blockControls set @c true to refuse any ASCII control characters except HT (U+0009),
                        RS (U+001E) and US (U+001F)
 * @retval true succeeded
 * @retval false the input was rejected by the input sequence validation (when @a validateSequence
 *               was @c true)
 * @return false @c character was control character and blocked (when @a blockControls was @c true)
 * @throw ... any exceptions @c Document#insertUndoBoundary and @c Caret#replaceSelection throw
 * @see #isOvertypeMode, #setOvertypeMode, texteditor#commands#TextInputCommand
 */
bool Caret::inputCharacter(CodePoint character, bool validateSequence /* = true */, bool blockControls /* = true */) {
	// check blockable control character
	static const CodePoint SAFE_CONTROLS[] = {0x0009u, 0x001eu, 0x001fu};
	if(blockControls && character <= 0x00ffu
			&& toBoolean(iscntrl(static_cast<int>(character)))
			&& !binary_search(SAFE_CONTROLS, MANAH_ENDOF(SAFE_CONTROLS), character))
		return false;

	// check the input sequence
	Document& doc = document();
	if(validateSequence) {
		if(const texteditor::Session* const session = doc.session()) {
			if(const texteditor::InputSequenceCheckers* const checker = session->inputSequenceCheckers()) {
				const Char* const line = doc.line(beginning().line()).data();
				if(!checker->check(line, line + beginning().column(), character)) {
					eraseSelection(*this);
					return false;	// invalid sequence
				}
			}
		}
	}

	Char buffer[2];
	surrogates::encode(character, buffer);
	if(!isSelectionEmpty(*this)) {	// just replace if the selection is not empty
		doc.insertUndoBoundary();
		replaceSelection(buffer, buffer + ((character < 0x10000u) ? 1 : 2));
		doc.insertUndoBoundary();
	} else if(overtypeMode_) {
		prechangeDocument();
		doc.insertUndoBoundary();
		destructiveInsert(*this, buffer, buffer + ((character < 0x10000u) ? 1 : 2));
		doc.insertUndoBoundary();
	} else {
		const bool alpha = identifierSyntax(*this).isIdentifierContinueCharacter(character);
		if(lastTypedPosition_ != Position::INVALID_POSITION && (!alpha || lastTypedPosition_ != position())) {
			// end sequential typing
			doc.insertUndoBoundary();
			lastTypedPosition_ = Position::INVALID_POSITION;
		}
		if(alpha && lastTypedPosition_ == Position::INVALID_POSITION)
			// (re)start sequential typing
			doc.insertUndoBoundary();

		ascension::internal::ValueSaver<bool> lock(typing_);
		typing_ = true;
		replaceSelection(buffer, buffer + ((character < 0x10000u) ? 1 : 2));	// this may throw
		if(alpha)
			lastTypedPosition_ = position();
	}

	characterInputListeners_.notify<const Caret&, CodePoint>(
		&ICharacterInputListener::characterInputted, *this, character);
	return true;
}

/**
 * Replaces the selected text by the content of the clipboard.
 * This method inserts undo boundaries at the beginning and the end.
 * @note When using the kill-ring, this method may exit in defective condition.
 * @param useKillRing set @c true to use the kill ring
 * @throw ClipboardException the clipboard operation failed
 * @throw ClipboardException(DV_E_FORMATETC) the current clipboard format is not supported
 * @throw IllegalStateException @a useKillRing was @c true but the kill-ring was not available
 * @throw bad_alloc internal memory allocation failed
 * @throw ... any exceptions @c kernel#Document#replace throws
 */
void Caret::paste(bool useKillRing) {
	AutoFreeze af(&textViewer(), true);
	if(!useKillRing) {
		com::ComPtr<IDataObject> content;
		HRESULT hr = tryOleClipboard(::OleGetClipboard, content.initialize());
		if(hr == E_OUTOFMEMORY)
			throw bad_alloc("::OleGetClipboard returned E_OUTOFMEMORY.");
		else if(FAILED(hr))
			throw ClipboardException(hr);
		bool rectangle;
		const pair<HRESULT, String> text(utils::getTextFromDataObject(*content, &rectangle));
		if(text.first == E_OUTOFMEMORY)
			throw bad_alloc("ascension.viewers.utils.getTextFromDataObject returned E_OUTOFMEMORY.");
		else if(FAILED(text.first))
			throw ClipboardException(text.first);
		document().insertUndoBoundary();
		viewers::replaceSelection(*this, text.second, rectangle);
	} else {
		texteditor::Session* const session = document().session();
		if(session == 0 || session->killRing().numberOfKills() == 0)
			throw IllegalStateException("the kill-ring is not available.");
		texteditor::KillRing& killRing = session->killRing();
		const pair<String, bool>& text = yanking_ ? killRing.setCurrent(+1) : killRing.get();

		const Position temp(beginning());
		try {
			if(!isSelectionEmpty(*this) && yanking_)
				document().undo();
			viewers::replaceSelection(*this, text.first, text.second);
		} catch(...) {
			killRing.setCurrent(-1);
			throw;
		}
		if(!text.second)
			endRectangleSelection();
		else
			beginRectangleSelection();
		select(temp, position());
		yanking_ = true;
	}
	document().insertUndoBoundary();
}

/// @see IPointListener#pointMoved
void Caret::pointMoved(const Point& self, const Position& oldPosition) {
	assert(&self == &*anchor_);
	yanking_ = false;
	if(leadingAnchor_)	// doMoveTo で anchor_->moveTo 呼び出し中
		return;
	if((oldPosition == position()) != isSelectionEmpty(*this))
		checkMatchBrackets();
	listeners_.notify<const Caret&, const Region&>(&ICaretListener::caretMoved, *this, Region(oldPosition, position()));
}

/// @internal Should be called before change the document.
inline void Caret::prechangeDocument() {
	if(lastTypedPosition_ != Position::INVALID_POSITION && !typing_) {
		document().insertUndoBoundary();
		lastTypedPosition_ = Position::INVALID_POSITION;
	}
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
 * @throw NullPointerException @a first and/or @last is @c null
 * @throw std#invalid_argument @a first &gt; @a last
 * @throw ... any exceptions @c Document#insert and @c Document#erase throw
 */
void Caret::replaceSelection(const Char* first, const Char* last, bool rectangleInsertion /* = false */) {
	Position e;
	prechangeDocument();
	if(!isSelectionRectangle() && !rectangleInsertion)
		document().replace(selectedRegion(), first, last, &e);
	else {
		// TODO: not implemented.
	}
	moveTo(e);
}

/**
 * Selects the specified region. The active selection mode will be cleared.
 * @param anchor the position where the anchor moves to
 * @param caret the position where the caret moves to
 */
void Caret::select(const Position& anchor, const Position& caret) {
	if(isTextViewerDisposed())
		throw TextViewerDisposedException();
	yanking_ = false;
	if(anchor != anchor_->position() || caret != position()) {
		const Region oldRegion(selectedRegion());
		leadingAnchor_ = true;
		anchor_->moveTo(anchor);
		leadingAnchor_ = false;
		VisualPoint::moveTo(caret);	// TODO: this may throw...
		if(isSelectionRectangle())
			box_->update(selectedRegion());
		if(autoShow_)
			utils::show(*this);
		listeners_.notify<const Caret&, const Region&>(&ICaretListener::caretMoved, *this, oldRegion);
	}
	checkMatchBrackets();
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
		box_->update(selectedRegion());
	if((regionBeforeMoved_.first != position() || regionBeforeMoved_.second != position()))
		listeners_.notify<const Caret&, const Region&>(&ICaretListener::caretMoved, *this, regionBeforeMoved_);
	if(autoShow_)
		utils::show(*this);
	checkMatchBrackets();
	regionBeforeMoved_.first = regionBeforeMoved_.second = Position::INVALID_POSITION;
}


// viewers free functions ///////////////////////////////////////////////////

/**
 * Returns true if the specified point is over the selection.
 * @param p the client coordinates of the point
 * @return true if the point is over the selection
 * @throw kernel#DocumentDisposedException the document @a caret connecting to has been disposed
 * @throw TextViewerDisposedException the text viewer @a caret connecting to has been disposed
 */
bool viewers::isPointOverSelection(const Caret& caret, const POINT& p) {
	if(isSelectionEmpty(caret))
		return false;
	else if(caret.isSelectionRectangle())
		return caret.boxForRectangleSelection().isPointOver(p);
	else {
		if(caret.textViewer().hitTest(p) != TextViewer::TEXT_AREA)	// ignore if on the margin
			return false;
		RECT rect;
		caret.textViewer().getClientRect(rect);
		if(p.x > rect.right || p.y > rect.bottom)
			return false;
		const Position pos(caret.textViewer().characterForClientXY(p, LineLayout::TRAILING));
		return pos >= caret.beginning() && pos <= caret.end();
	}
}

/**
 * Returns the selected range on the specified logical line.
 * This function returns a logical range, and does not support rectangular selection.
 * @param caret the caret gives a selection
 * @param line the logical line
 * @param[out] first the start of the range
 * @param[out] last the end of the range. if the selection continued to the next line, this is
 *                  the column of the end of line + 1
 * @return true if there is selected range on the line
 * @throw kernel#DocumentDisposedException the document @a caret connecting to has been disposed
 * @throw kernel#BadPositionException @a line is outside of the document
 * @see #selectedRangeOnVisualLine
 */
bool viewers::selectedRangeOnLine(const Caret& caret, length_t line, length_t& first, length_t& last) {
	const Position bos(caret.beginning());
	if(bos.line > line)
		return false;
	const Position eos(caret.end());
	if(eos.line < line)
		return false;
	first = (line == bos.line) ? bos.column : 0;
	last = (line == eos.line) ? eos.column : caret.document().lineLength(line) + 1;
	return true;
}

/**
 * Returns the selected range on the specified visual line.
 * @param caret the caret gives a selection
 * @param line the logical line
 * @param subline the visual subline
 * @param[out] first the start of the range
 * @param[out] last the end of the range. if the selection continued to the next logical line, this
 *                  is the column of the end of line + 1
 * @return true if there is selected range on the line
 * @throw kernel#DocumentDisposedException the document @a caret connecting to has been disposed
 * @throw TextViewerDisposedException the text viewer @a caret connecting to has been disposed
 * @throw kernel#BadPositionException @a line or @a subline is outside of the document
 * @see #selectedRangeOnLine
 */
bool viewers::selectedRangeOnVisualLine(const Caret& caret, length_t line, length_t subline, length_t& first, length_t& last) {
	if(!caret.isSelectionRectangle()) {
		if(!selectedRangeOnLine(caret, line, first, last))
			return false;
		const LineLayout& layout = caret.textViewer().textRenderer().lineLayout(line);
		const length_t sublineOffset = layout.sublineOffset(subline);
		first = max(first, sublineOffset);
		last = min(last, sublineOffset + layout.sublineLength(subline) + ((subline < layout.numberOfSublines() - 1) ? 0 : 1));
		return first != last;
	} else
		return caret.boxForRectangleSelection().overlappedSubline(line, subline, first, last);
}

/**
 * Writes the selected string into the specified output stream.
 * @param caret the caret gives a selection
 * @param[out] out the output stream
 * @param newline the newline representation for multiline selection. if the selection is
 *                rectangular, this value is ignored and the document's newline is used instead
 * @return @a out
 */
basic_ostream<Char>& viewers::selectedString(const Caret& caret, basic_ostream<Char>& out, Newline newline /* = NLF_RAW_VALUE */) {
	if(!isSelectionEmpty(caret)) {
		if(!caret.isSelectionRectangle())
			writeDocumentToStream(out, caret.document(), caret.selectedRegion(), newline);
		else {
			const Document& document = caret.document();
			const length_t lastLine = caret.end().line();
			length_t first, last;
			for(length_t line = caret.beginning().line(); line <= lastLine; ++line) {
				const Document::Line& ln = document.getLineInformation(line);
				caret.boxForRectangleSelection().overlappedSubline(line, 0, first, last);	// TODO: recognize wrap (second parameter).
				out.write(ln.text().data() + first, static_cast<streamsize>(last - first));
				out.write(newlineString(ln.newline()), static_cast<streamsize>(newlineStringLength(ln.newline())));
			}
		}
	}
	return out;
}

/**
 * Selects the word at the caret position. This creates a linear selection.
 */
void viewers::selectWord(Caret& caret) {
	WordBreakIterator<DocumentCharacterIterator> i(
		DocumentCharacterIterator(caret.document(), caret.position()),
		AbstractWordBreakIterator::BOUNDARY_OF_SEGMENT, identifierSyntax(caret));
	caret.endRectangleSelection();
	if(locations::isEndOfLine(caret)) {
		if(locations::isBeginningOfLine(caret))	// an empty line
			caret.moveTo(caret);
		else	// eol
			caret.select((--i).base().tell(), caret);
	} else if(locations::isBeginningOfLine(caret))	// bol
		caret.select(caret, (++i).base().tell());
	else {
		const Position p((++i).base().tell());
		i.base().seek(Position(caret.line(), caret.column() + 1));
		caret.select((--i).base().tell(), p);
	}
}


// viewers.locations free functions /////////////////////////////////////////

/**
 * Returns the position returned by N pages.
 * @param p the base position
 * @param pages the number of pages to return
 * @return the destination
 */
VerticalDestinationProxy locations::backwardPage(const VisualPoint& p, length_t pages /* = 1 */) {
	// TODO: calculate exact number of visual lines.
	return backwardVisualLine(p, p.textViewer().numberOfVisibleLines() * pages);
}

/**
 * Returns the position returned by N visual lines.
 * @param p the base position
 * @param lines the number of the visual lines to return
 * @return the destination
 */
VerticalDestinationProxy locations::backwardVisualLine(const VisualPoint& p, length_t lines /* = 1 */) {
	Position np(p.normalized());
	const TextRenderer& renderer = p.textViewer().textRenderer();
	length_t subline = renderer.lineLayout(np.line).subline(np.column);
	if(np.line == 0 && subline == 0)
		return VisualPoint::makeVerticalDestinationProxy(np);
	renderer.offsetVisualLine(np.line, subline, -static_cast<signed_length_t>(lines));
	const LineLayout& layout = renderer.lineLayout(np.line);
	np.column = layout.offset(p.lastX_ - renderer.lineIndent(np.line), renderer.linePitch() * static_cast<long>(subline));
	if(layout.subline(np.column) != subline)
		np = nextCharacter(p.document(), np, Direction::BACKWARD, GRAPHEME_CLUSTER);
	return VisualPoint::makeVerticalDestinationProxy(np);
}

/**
 * Returns the beginning of the visual line.
 * @param p the base position
 * @return the destination
 * @see EditPoint#beginningOfLine
 */
Position locations::beginningOfVisualLine(const VisualPoint& p) {
	const Position np(p.normalized());
	const LineLayout& layout = p.textViewer().textRenderer().lineLayout(np.line);
	return Position(np.line, layout.sublineOffset(layout.subline(np.column)));
}

/**
 * Returns the beginning of the line or the first printable character in the line by context.
 * @param p the base position
 * @return the destination
 */
Position locations::contextualBeginningOfLine(const VisualPoint& p) {
	return isFirstPrintableCharacterOfLine(p) ? beginningOfLine(p) : firstPrintableCharacterOfLine(p);
}

/**
 * Moves to the beginning of the visual line or the first printable character in the visual line by
 * context.
 * @param p the base position
 * @return the destination
 */
Position locations::contextualBeginningOfVisualLine(const VisualPoint& p) {
	return isFirstPrintableCharacterOfLine(p) ?
		beginningOfVisualLine(p) : firstPrintableCharacterOfVisualLine(p);
}

/**
 * Moves to the end of the line or the last printable character in the line by context.
 * @param p the base position
 * @return the destination
 */
Position locations::contextualEndOfLine(const VisualPoint& p) {
	return isLastPrintableCharacterOfLine(p) ? endOfLine(p) : lastPrintableCharacterOfLine(p);
}

/**
 * Moves to the end of the visual line or the last printable character in the visual line by
 * context.
 * @param p the base position
 * @return the destination
 */
Position locations::contextualEndOfVisualLine(const VisualPoint& p) {
	return isLastPrintableCharacterOfLine(p) ?
		endOfVisualLine(p) : lastPrintableCharacterOfVisualLine(p);
}

/**
 * Returns the end of the visual line.
 * @param p the base position
 * @return the destination
 * @see EditPoint#endOfLine
 */
Position locations::endOfVisualLine(const VisualPoint& p) {
	Position np(p.normalized());
	const LineLayout& layout = p.textViewer().textRenderer().lineLayout(np.line);
	const length_t subline = layout.subline(np.column);
	np.column = (subline < layout.numberOfSublines() - 1) ?
		layout.sublineOffset(subline + 1) : p.document().lineLength(np.line);
	if(layout.subline(np.column) != subline)
		np = nextCharacter(p.document(), np, Direction::BACKWARD, GRAPHEME_CLUSTER);
	return np;
}

/**
 * Returns the first printable character in the line.
 * @param p the base position
 * @return the destination
 */
Position locations::firstPrintableCharacterOfLine(const VisualPoint& p) {
	Position np(p.normalized());
	const Char* const s = p.document().line(np.line).data();
	np.column = identifierSyntax(p).eatWhiteSpaces(s, s + p.document().lineLength(np.line), true) - s;
	return np;
}

/**
 * Returns the first printable character in the visual line.
 * @param p the base position
 * @return the destination
 */
Position locations::firstPrintableCharacterOfVisualLine(const VisualPoint& p) {
	Position np(p.normalized());
	const String& s = p.document().line(np.line);
	const LineLayout& layout = p.textViewer().textRenderer().lineLayout(np.line);
	const length_t subline = layout.subline(np.column);
	np.column = identifierSyntax(p).eatWhiteSpaces(
		s.begin() + layout.sublineOffset(subline),
		s.begin() + ((subline < layout.numberOfSublines() - 1) ?
			layout.sublineOffset(subline + 1) : s.length()), true) - s.begin();
	return np;
}

/**
 * Returns the position advanced by N pages.
 * @param p the base position
 * @param pages the number of pages to advance
 * @return the destination
 */
VerticalDestinationProxy locations::forwardPage(const VisualPoint& p, length_t pages /* = 1 */) {
	// TODO: calculate exact number of visual lines.
	return forwardVisualLine(p, p.textViewer().numberOfVisibleLines() * pages);
}

/**
 * Returns the position advanced by N visual lines.
 * @param p the base position
 * @param lines the number of the visual lines to advance
 * @return the destination
 */
VerticalDestinationProxy locations::forwardVisualLine(const VisualPoint& p, length_t lines /* = 1 */) {
	Position np(p.normalized());
	const TextRenderer& renderer = p.textViewer().textRenderer();
	const LineLayout* layout = &renderer.lineLayout(np.line);
	length_t subline = layout->subline(np.column);
	if(np.line == p.document().numberOfLines() - 1 && subline == layout->numberOfSublines() - 1)
		return VisualPoint::makeVerticalDestinationProxy(np);
	renderer.offsetVisualLine(np.line, subline, static_cast<signed_length_t>(lines));
	layout = &renderer.lineLayout(np.line);
	np.column = layout->offset(p.lastX_ - renderer.lineIndent(np.line), renderer.linePitch() * static_cast<long>(subline));
	if(layout->subline(np.column) != subline)
		np = nextCharacter(p.document(), np, Direction::BACKWARD, GRAPHEME_CLUSTER);
	return VisualPoint::makeVerticalDestinationProxy(np);
}

/**
 * Returns true if the point is the beginning of the visual line.
 * @param p the base position
 * @see EditPoint#isBeginningOfLine
 */
bool locations::isBeginningOfVisualLine(const VisualPoint& p) {
	if(isBeginningOfLine(p))	// this considers narrowing
		return true;
	const Position np(p.normalized());
	const LineLayout& layout = p.textViewer().textRenderer().lineLayout(np.line);
	return np.column == layout.sublineOffset(layout.subline(np.column));
}

/**
 * Returns true if the point is end of the visual line.
 * @param p the base position
 * @see kernel#EditPoint#isEndOfLine
 */
bool locations::isEndOfVisualLine(const VisualPoint& p) {
	if(isEndOfLine(p))	// this considers narrowing
		return true;
	const Position np(p.normalized());
	const LineLayout& layout = p.textViewer().textRenderer().lineLayout(np.line);
	const length_t subline = layout.subline(np.column);
	return np.column == layout.sublineOffset(subline) + layout.sublineLength(subline);
}

/// Returns true if the given position is the first printable character in the line.
bool locations::isFirstPrintableCharacterOfLine(const VisualPoint& p) {
	const Position np(p.normalized()), bob(p.document().accessibleRegion().first);
	const length_t offset = (bob.line == np.line) ? bob.column : 0;
	const String& line = p.document().line(np.line);
	return line.data() + np.column - offset
		== identifierSyntax(p).eatWhiteSpaces(line.data() + offset, line.data() + line.length(), true);
}

/// Returns true if the given position is the first printable character in the visual line.
bool locations::isFirstPrintableCharacterOfVisualLine(const VisualPoint& p) {
	// TODO: not implemented.
	return false;
}

/// Returns true if the given position is the last printable character in the line.
bool locations::isLastPrintableCharacterOfLine(const VisualPoint& p) {
	const Position np(p.normalized()), eob(p.document().accessibleRegion().second);
	const String& line = p.document().line(np.line);
	const length_t lineLength = (eob.line == np.line) ? eob.column : line.length();
	return line.data() + lineLength - np.column
		== identifierSyntax(p).eatWhiteSpaces(line.data() + np.column, line.data() + lineLength, true);
}

/// Returns true if the given position is the last printable character in the visual line.
bool locations::isLastPrintableCharacterOfVisualLine(const VisualPoint& p) {
	// TODO: not implemented.
	return false;
}

/**
 * Returns the last printable character in the line.
 * @param p the base position
 * @return the destination
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
 * @param p the base position
 * @return the destination
 */
Position locations::lastPrintableCharacterOfVisualLine(const VisualPoint& p) {
	// TODO: not implemented.
	return p.normalized();
}

/**
 * Returns the position advanced to the left by N characters.
 * @param p the base position
 * @param unit defines what a character is
 * @param characters the number of characters to adavance
 * @return the destination
 */
Position locations::leftCharacter(const VisualPoint& p, CharacterUnit unit, length_t characters /* = 1 */) {
	return (p.textViewer().configuration().orientation == LEFT_TO_RIGHT) ?
		backwardCharacter(p, unit, characters) : forwardCharacter(p, unit, characters);
}

/**
 * Returns the beginning of the word where advanced to the left by N words.
 * @param p the base position
 * @param words the number of words to adavance
 * @return the destination
 */
Position locations::leftWord(const VisualPoint& p, length_t words /* = 1 */) {
	return (p.textViewer().configuration().orientation == LEFT_TO_RIGHT) ? backwardWord(p, words) : forwardWord(p, words);
}

/**
 * Returns the end of the word where advanced to the left by N words.
 * @param p the base position
 * @param words the number of words to adavance
 * @return the destination
 */
Position locations::leftWordEnd(const VisualPoint& p, length_t words /* = 1 */) {
	return (p.textViewer().configuration().orientation == LEFT_TO_RIGHT) ? backwardWordEnd(p, words) : forwardWordEnd(p, words);
}

/**
 * Returns the position advanced to the right by N characters.
 * @param p the base position
 * @param unit defines what a character is
 * @param characters the number of characters to adavance
 * @return the destination
 */
Position locations::rightCharacter(const VisualPoint& p, CharacterUnit unit, length_t characters /* = 1 */) {
	return (p.textViewer().configuration().orientation == LEFT_TO_RIGHT) ?
		forwardCharacter(p, unit, characters) : backwardCharacter(p, unit, characters);
}

/**
 * Returns the beginning of the word where advanced to the right by N words.
 * @param p the base position
 * @param words the number of words to adavance
 * @return the destination
 */
Position locations::rightWord(const VisualPoint& p, length_t words /* = 1 */) {
	return (p.textViewer().configuration().orientation == LEFT_TO_RIGHT) ? forwardWord(p, words) : backwardWord(p, words);
}

/**
 * Returns the end of the word where advanced to the right by N words.
 * @param p the base position
 * @param words the number of words to adavance
 * @return the destination
 */
Position locations::rightWordEnd(const VisualPoint& p, length_t words /* = 1 */) {
	return (p.textViewer().configuration().orientation == LEFT_TO_RIGHT) ? forwardWordEnd(p, words) : backwardWordEnd(p, words);
}


// viewers free functions ///////////////////////////////////////////////////

/**
 * Breaks the line at the caret position and moves the caret to the end of the inserted string.
 * @param caret the caret
 * @param inheritIndent true to inherit the indent of the line @c{at.line}
 * @param newlines the number of newlines to insert
 * @throw DocumentDisposedException the document @a caret connecting to has been disposed
 * @throw ... any exceptions @c Document#insert throws
 */
void viewers::breakLine(Caret& caret, bool inheritIndent, size_t newlines /* = 1 */) {
	if(newlines == 0)
		return;

	const IDocumentInput* const di = caret.document().input();
	String s(newlineString((di != 0) ? di->newline() : ASCENSION_DEFAULT_NEWLINE));

	if(inheritIndent) {	// simple auto-indent
		const String& currentLine = caret.document().line(caret.line());
		const length_t len = identifierSyntax(caret).eatWhiteSpaces(
			currentLine.data(), currentLine.data() + caret.column(), true) - currentLine.data();
		s += currentLine.substr(0, len);
	}

	if(newlines > 1) {
		basic_stringbuf<Char> sb;
		for(size_t i = 0; i < newlines; ++i)
			sb.sputn(s.data(), static_cast<streamsize>(s.length()));
		s.assign(sb.str());
	}
	return viewers::replaceSelection(caret, s);
}

/**
 * Copies the selected content to the clipboard.
 * If the caret does not have a selection, this function does nothing.
 * @param caret the caret gives the selection
 * @param useKillRing set @c true to send to the kill ring, not only the system clipboard
 * @throw ClipboardException the clipboard operation failed
 * @throw bad_alloc internal memory allocation failed
 */
void viewers::copySelection(Caret& caret, bool useKillRing) {
	if(isSelectionEmpty(caret))
		return;

	IDataObject* content;
	HRESULT hr = utils::createTextObjectForSelectedString(caret, true, content);
	if(hr == E_OUTOFMEMORY)
		throw bad_alloc("Caret.createTextObject returned E_OUTOFMEMORY.");
	hr = tryOleClipboard(::OleSetClipboard, content);
	if(FAILED(hr)) {
		content->Release();
		throw ClipboardException(hr);
	}
	hr = tryOleClipboard(::OleFlushClipboard);
	content->Release();
	if(useKillRing) {
		if(texteditor::Session* const session = caret.document().session())
			session->killRing().addNew(selectedString(caret, NLF_RAW_VALUE), caret.isSelectionRectangle());
	}
}

/**
 * Copies and deletes the selected text. If the selection is empty, this function does nothing.
 * @param caret the caret gives the selection
 * @param useKillRing true to send also the kill ring
 * @return false if the change was interrupted
 * @throw ClipboardException the clipboard operation failed
 * @throw bad_alloc internal memory allocation failed
 * @throw ... any exceptions @c Document#replace throws
 */
void viewers::cutSelection(Caret& caret, bool useKillRing) {
	if(isSelectionEmpty(caret))
		return;

	com::ComPtr<IDataObject> previousContent;
	HRESULT hr = tryOleClipboard(::OleGetClipboard, previousContent.initialize());
	if(hr == E_OUTOFMEMORY)
		throw bad_alloc("::OleGetClipboard returned E_OUTOFMEMORY.");
	else if(FAILED(hr))
		throw ClipboardException(hr);
	copySelection(caret, useKillRing);	// this may throw
	try {
		viewers::eraseSelection(caret);
	} catch(...) {
		hr = tryOleClipboard(::OleSetClipboard, previousContent.get());
		throw;
	}
}

/**
 * Deletes the selected region.
 * @param caret the caret provides a selection
 * @throw ... any exceptions @c Document#insert and @c Document#erase throw
 */
void viewers::eraseSelection(Caret& caret) {
	return caret.replaceSelection(0, 0);
}

namespace {
	/**
	 * @internal Indents the region selected by the caret.
	 * @param caret the caret gives the region to indent
	 * @param character a character to make indents
	 * @param rectangle set true for rectangular indents (will be ignored level is negative)
	 * @param level the level of the indentation
	 * @deprecated 0.8
	 */
	void indent(Caret& caret, Char character, bool rectangle, long level) {
		// TODO: this code is not exception-safe.
		if(level == 0)
			return;
		const String indent = String(abs(level), character);
		const Region region(caret.selectedRegion());

		if(region.beginning().line == region.end().line) {
			// number of selected lines is one -> just insert tab character(s)
			replaceSelection(caret, indent);
			return;
		}

		const Position oldPosition(caret.position());
		Position otherResult(caret.anchor());
		length_t line = region.beginning().line;

		// indent/unindent the first line
		Document& document = caret.document();
		if(level > 0) {
			insert(document, Position(line, rectangle ? region.beginning().column : 0), indent);
			if(line == otherResult.line && otherResult.column != 0)
				otherResult.column += level;
			if(line == caret.line() && caret.column() != 0)
				caret.moveTo(caret.line(), caret.column() + level);
		} else {
			const String& s = document.line(line);
			length_t indentLength;
			for(indentLength = 0; indentLength < s.length(); ++indentLength) {
				// this assumes that all white space characters belong to BMP
				if(s[indentLength] == '\t' && GeneralCategory::of(s[indentLength]) != GeneralCategory::SPACE_SEPARATOR)
					break;
			}
			if(indentLength > 0) {
				const length_t deleteLength = min<length_t>(-level, indentLength);
				erase(document, Position(line, 0), Position(line, deleteLength));
				if(line == otherResult.line && otherResult.column != 0)
					otherResult.column -= deleteLength;
				if(line == caret.line() && caret.column() != 0)
					caret.moveTo(caret.line(), caret.column() - deleteLength);
			}
		}

		// indent/unindent the following selected lines
		if(level > 0) {
			for(++line; line <= region.end().line; ++line) {
				if(document.lineLength(line) != 0 && (line != region.end().line || region.end().column > 0)) {
					length_t insertPosition = 0;
					if(rectangle) {
						length_t dummy;
						caret.boxForRectangleSelection().overlappedSubline(line, 0, insertPosition, dummy);	// TODO: recognize wrap (second parameter).
					}
					insert(document, Position(line, insertPosition), indent);
					if(line == otherResult.line && otherResult.column != 0)
						otherResult.column += level;
					if(line == caret.line() && caret.column() != 0)
						caret.moveTo(caret.line(), caret.column() + level);
				}
			}
		} else {
			for(++line; line <= region.end().line; ++line) {
				const String& s = document.line(line);
				length_t indentLength;
				for(indentLength = 0; indentLength < s.length(); ++indentLength) {
				// this assumes that all white space characters belong to BMP
					if(s[indentLength] == '\t' && GeneralCategory::of(s[indentLength]) != GeneralCategory::SPACE_SEPARATOR)
						break;
				}
				if(indentLength > 0) {
					const length_t deleteLength = min<length_t>(-level, indentLength);
					erase(document, Position(line, 0), Position(line, deleteLength));
					if(line == otherResult.line && otherResult.column != 0)
						otherResult.column -= deleteLength;
					if(line == caret.line() && caret.column() != 0)
						caret.moveTo(caret.line(), caret.column() - deleteLength);
				}
			}
		}
	}
} // namespace @0

/**
 * Indents the region selected by the caret by using spaces.
 * @param caret the caret gives the region to indent
 * @param rectangle set true for rectangular indents (will be ignored level is negative)
 * @param level the level of the indentation
 * @throw ...
 * @deprecated 0.8
 */
void viewers::indentBySpaces(Caret& caret, bool rectangle, long level /* = 1 */) {
	return indent(caret, ' ', rectangle, level);
}

/**
 * Indents the region selected by the caret by using horizontal tabs.
 * @param caret the caret gives the region to indent
 * @param rectangle set true for rectangular indents (will be ignored level is negative)
 * @param level the level of the indentation
 * @throw ...
 * @deprecated 0.8
 */
void viewers::indentByTabs(Caret& caret, bool rectangle, long level /* = 1 */) {
	return indent(caret, '\t', rectangle, level);
}

/**
 * Replaces the selected region with the specified text.
 * If the selection is empty, inserts the text at current position.
 * @param caret the caret provides a selection
 * @param text the text
 * @param rectangleInsertion true to insert text as rectangle
 * @throw ... any exceptions @c Document#insert and @c Document#erase throw
 */
void viewers::replaceSelection(Caret& caret, const String& text, bool rectangleInsertion /* = false */) {
	return caret.replaceSelection(text.data(), text.data() + text.length(), rectangleInsertion);
}

/**
 * Transposes the character (grapheme cluster) addressed by the caret and the previous character,
 * and moves the caret to the end of them.
 * If the characters to transpose are not inside of the accessible region, this method fails and
 * returns @c false
 * @param caret the caret
 * @return false if there is not a character to transpose in the line, or the point is not the
 *               beginning of a grapheme
 * @throw DocumentDisposedException the document the caret connecting to has been disposed
 * @throw ... any exceptions @c Document#replace throws other than @c DocumentAccessViolationException
 */
bool viewers::transposeCharacters(Caret& caret) {
	// TODO: handle the case when the caret intervened a grapheme cluster.

	// As transposing characters in string "ab":
	//
	//  a b -- transposing clusters 'a' and 'b'. result is "ba"
	// ^ ^ ^
	// | | next-cluster (named pos[2])
	// | middle-cluster (named pos[1]; usually current-position)
	// previous-cluster (named pos[0])

	Position pos[3];
	const Region region(caret.document().accessibleRegion());

	if(text::ucd::BinaryProperty::is<text::ucd::BinaryProperty::GRAPHEME_EXTEND>(locations::characterAt(caret)))	// not the start of a grapheme
		return false;
	else if(!region.includes(caret.position()))	// inaccessible
		return false;

	if(caret.column() == 0 || caret.position() == region.first) {
		GraphemeBreakIterator<DocumentCharacterIterator> i(
			DocumentCharacterIterator(caret.document(), pos[0] = caret.position()));
		pos[1] = (++i).base().tell();
		if(pos[1].line != pos[0].line || pos[1] == pos[0] || !region.includes(pos[1]))
			return false;
		pos[2] = (++i).base().tell();
		if(pos[2].line != pos[1].line || pos[2] == pos[1] || !region.includes(pos[2]))
			return false;
	} else if(caret.column() == caret.document().lineLength(caret.line()) || caret.position() == region.second) {
		GraphemeBreakIterator<DocumentCharacterIterator> i(
			DocumentCharacterIterator(caret.document(), pos[2] = caret.position()));
		pos[1] = (--i).base().tell();
		if(pos[1].line != pos[2].line || pos[1] == pos[2] || !region.includes(pos[1]))
			return false;
		pos[0] = (--i).base().tell();
		if(pos[0].line != pos[1].line || pos[0] == pos[1] || !region.includes(pos[0]))
			return false;
	} else {
		GraphemeBreakIterator<DocumentCharacterIterator> i(
			DocumentCharacterIterator(caret.document(), pos[1] = caret.position()));
		pos[2] = (++i).base().tell();
		if(pos[2].line != pos[1].line || pos[2] == pos[1] || !region.includes(pos[2]))
			return false;
		i.base().seek(pos[1]);
		pos[0] = (--i).base().tell();
		if(pos[0].line != pos[1].line || pos[0] == pos[1] || !region.includes(pos[0]))
			return false;
	}

	basic_ostringstream<Char> ss;
	writeDocumentToStream(ss, caret.document(), Region(pos[1], pos[2]), NLF_LINE_SEPARATOR);
	writeDocumentToStream(ss, caret.document(), Region(pos[0], pos[1]), NLF_LINE_SEPARATOR);
	try {
		replace(caret.document(), Region(pos[0], pos[2]), ss.str());
	} catch(DocumentAccessViolationException&) {
		return false;
	}
	assert(caret.position() == pos[2]);
	return true;
}

/**
 * Transposes the line addressed by the caret and the next line, and moves the caret to the same
 * column in the next line.
 * If the caret is the last line in the document, transposes with the previous line.
 * The intervening newline character is not moved.
 * If the lines to transpose are not inside of the accessible region, this method fails and returns
 * @c false
 * @param caret the caret
 * @return false if there is not a line to transpose
 * @throw DocumentDisposedException the document the caret connecting to has been disposed
 * @throw ... any exceptions @c Document#replace throws other than @c DocumentAccessViolationException
 */
bool viewers::transposeLines(Caret& caret) {
	if(caret.document().numberOfLines() == 1)	// there is just one line
		return false;

	Document& document = caret.document();
	const Position old(caret.position());
	const length_t firstLine = (old.line != document.numberOfLines() - 1) ? old.line : old.line - 1;
	String s(document.line(firstLine + 1));
	s += newlineString(document.getLineInformation(firstLine).newline());
	s += document.line(firstLine);

	try {
		replace(document, Region(
			Position(firstLine, 0), Position(firstLine + 1, document.lineLength(firstLine + 1))), s);
		caret.moveTo((old.line != document.numberOfLines() - 1) ? firstLine + 1 : firstLine, old.column);
	} catch(const DocumentAccessViolationException&) {
		return false;
	}
	return true;
}

/**
 * Transposes the word addressed by the caret and the next word, and moves the caret to the end of
 * them.
 * If the words to transpose are not inside of the accessible region, this method fails and returns
 * @c false
 * @param caret the caret
 * @return false if there is not a word to transpose
 * @throw DocumentDisposedException the document the caret connecting to has been disposed
 * @throw ... any exceptions @c Document#replace throws other than @c DocumentAccessViolationException
 */
bool viewers::transposeWords(Caret& caret) {
	// As transposing words in string "(\w+)[^\w*](\w+)":
	//
	//  abc += xyz -- transposing words "abc" and "xyz". result is "xyz+=abc"
	// ^   ^  ^   ^
	// |   |  |   2nd-word-end (named pos[3])
	// |   |  2nd-word-start (named pos[2])
	// |   1st-word-end (named pos[1])
	// 1st-word-start (named pos[0])

	WordBreakIterator<DocumentCharacterIterator> i(
		DocumentCharacterIterator(caret.document(), caret),
		AbstractWordBreakIterator::START_OF_ALPHANUMERICS, identifierSyntax(caret));
	Position pos[4];

	// find the backward word (1st-word-*)...
	pos[0] = (--i).base().tell();
	i.setComponent(AbstractWordBreakIterator::END_OF_ALPHANUMERICS);
	pos[1] = (++i).base().tell();
	if(pos[1] == pos[0])	// the word is empty
		return false;

	// ...and then backward one (2nd-word-*)
	i.base().seek(caret);
	i.setComponent(AbstractWordBreakIterator::START_OF_ALPHANUMERICS);
	pos[2] = (++i).base().tell();
	if(pos[2] == caret.position())
		return false;
	pos[3] = (++i).base().tell();
	if(pos[2] == pos[3])	// the word is empty
		return false;

	// replace
	basic_ostringstream<Char> ss;
	writeDocumentToStream(ss, caret.document(), Region(pos[2], pos[3]), NLF_RAW_VALUE);
	writeDocumentToStream(ss, caret.document(), Region(pos[1], pos[2]), NLF_RAW_VALUE);
	writeDocumentToStream(ss, caret.document(), Region(pos[0], pos[1]), NLF_RAW_VALUE);
	Position e;
	try {
		replace(caret.document(), Region(pos[0], pos[3]), ss.str(), &e);
	} catch(const DocumentAccessViolationException&) {
		return false;
	}
	return caret.moveTo(e), true;
}
