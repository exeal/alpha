/**
 * @file caret-windows.cpp
 * @author exeal
 * @date 2003-2008 was point.cpp
 * @date 2008-2010 separated from point.cpp
 * @date 2011-10-03 separated from caret.cpp
 */

#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/viewer.hpp>
#include <ascension/corelib/text/character-property.hpp>
#include <ascension/kernel/document-character-iterator.hpp>
#include <ascension/text-editor/command.hpp>	// command.TextInputCommand
#include <ascension/text-editor/session.hpp>
#include <ascension/win32/com/unknown-impl.hpp>

using namespace ascension;
using namespace ascension::graphics;
using namespace ascension::kernel;
using namespace ascension::viewers;
using namespace ascension::presentation;
using namespace ascension::text;
using namespace ascension::text::ucd;
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
	class GenericDataObject : public win32::com::IUnknownImpl<
		typelist::Cat<ASCENSION_WIN32_COM_INTERFACE_SIGNATURE(IDataObject)>, win32::com::NoReferenceCounting> {
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
		(*entry)->medium = win32::boole(release) ? *medium : clone;
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
 * @param caret The caret gives the selection
 * @param rtf Set @c true if the content is available as Rich Text Format. This feature is not
 *            implemented yet and the parameter is ignored
 * @param[out] content The data object
 * @retval S_OK Succeeded
 * @retval E_OUTOFMEMORY Failed to allocate memory for @a content
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
	if(0 != ::GetLocaleInfoW(caret.clipboardLocale(), LOCALE_IDEFAULTANSICODEPAGE, codePageString, ASCENSION_COUNTOF(codePageString))) {
		wchar_t* eob;
		codePage = wcstoul(codePageString, &eob, 10);
		format.cfFormat = CF_TEXT;
		if(int ansiLength = ::WideCharToMultiByte(codePage, 0, text.c_str(), static_cast<int>(text.length()), 0, 0, 0, 0)) {
			AutoBuffer<char> ansiBuffer(new(nothrow) char[ansiLength]);
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
 * @param data The data object
 * @param[out] rectangle @c true if the data is rectangle format. can be @c null
 * @return A pair of the result HRESULT and the text content. @c SCODE is one of @c S_OK,
 *         @c E_OUTOFMEMORY and @c DV_E_FORMATETC
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
							if(0 != ::GetLocaleInfoW(*static_cast<USHORT*>(::GlobalLock(locale.hGlobal)),
									LOCALE_IDEFAULTANSICODEPAGE, buffer, ASCENSION_COUNTOF(buffer))) {
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
						AutoBuffer<wchar_t> ucsBuffer(new(nothrow) wchar_t[ucsLength]);
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


// ClipboardException /////////////////////////////////////////////////////////////////////////////

ClipboardException::ClipboardException(HRESULT hr) : runtime_error("") {
	void* buffer = 0;
	::FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		0, hr, 0, reinterpret_cast<char*>(&buffer), 0, 0);
	runtime_error(static_cast<char*>(buffer));
	::LocalFree(buffer);
}


// Caret //////////////////////////////////////////////////////////////////////////////////////////

namespace {
	inline win32::Handle<HIMC> inputMethod(const TextViewer& viewer) {
		return win32::Handle<HIMC>(
			::ImmGetContext(viewer.identifier().get()),
			bind1st(ptr_fun(&::ImmReleaseContext), viewer.identifier().get()));
	}
}

void Caret::abortInput() {
	if(context_.inputMethodCompositionActivated)	// stop IME input
		::ImmNotifyIME(inputMethod(textViewer()).get(), NI_COMPOSITIONSTR, CPS_CANCEL, 0);
}

/// Moves the IME form to valid position.
void Caret::adjustInputMethodCompositionWindow() {
	assert(win32::boole(::IsWindow(textViewer().identifier().get())));
	if(!context_.inputMethodCompositionActivated)
		return;
	const TextViewer& viewer = textViewer();
	win32::Handle<HIMC> imc(inputMethod(viewer));
	if(imc.get() != 0) {
		// composition window placement
		COMPOSITIONFORM cf;
		cf.rcArea = viewer.bounds(false);
		const PhysicalFourSides<Scalar> margins(viewer.spaceWidths());
		cf.rcArea.left += margins.left();
		cf.rcArea.top += margins.top();
		cf.rcArea.right -= margins.right();
		cf.rcArea.bottom -= margins.bottom();
		cf.dwStyle = CFS_POINT;
		cf.ptCurrentPos = viewer.localPointForCharacter(beginning(), false, font::TextLayout::LEADING);
		if(cf.ptCurrentPos.y == numeric_limits<Scalar>::max() || cf.ptCurrentPos.y == numeric_limits<Scalar>::min())
			cf.ptCurrentPos.y = (cf.ptCurrentPos.y == numeric_limits<Scalar>::min()) ? cf.rcArea.top : cf.rcArea.bottom;
		else
			cf.ptCurrentPos.y = max(cf.ptCurrentPos.y, cf.rcArea.top);
		::ImmSetCompositionWindow(imc.get(), &cf);
		cf.dwStyle = CFS_RECT;
		::ImmSetCompositionWindow(imc.get(), &cf);

		// composition font
		LOGFONTW font;
		::GetObjectW(viewer.textRenderer().defaultFont()->nativeObject().get(), sizeof(LOGFONTW), &font);
		::ImmSetCompositionFontW(imc.get(), &font);	// this may be ineffective for IME settings
	}
}

/**
 * Returns @c true if a paste operation can be performed.
 * @note Even when this method returned @c true, the following @c #paste call can fail.
 * @param useKillRing Set @c true to get the content from the kill-ring of the session, not from
 *                    the system clipboard
 * @return true if the clipboard data is pastable
 */
bool Caret::canPaste(bool useKillRing) const {
	if(!useKillRing) {
		const UINT rectangleClipFormat = ::RegisterClipboardFormatW(ASCENSION_RECTANGLE_TEXT_CLIP_FORMAT);
		if(rectangleClipFormat != 0 && win32::boole(::IsClipboardFormatAvailable(rectangleClipFormat)))
			return true;
		else if(win32::boole(::IsClipboardFormatAvailable(CF_UNICODETEXT)) || win32::boole(::IsClipboardFormatAvailable(CF_TEXT)))
			return true;
	} else {
		if(const texteditor::Session* const session = document().session())
			return session->killRing().numberOfKills() != 0;
	}
	return false;
}

/**
 * Returns the locale identifier used to convert non-Unicode text.
 * @see #setClipboardLocale
 */
LCID Caret::clipboardLocale() const /*throw()*/ {
	return clipboardLocale_;
}

/// @see detail#InputEventHandler#handleInputEvent
LRESULT Caret::handleInputEvent(UINT message, WPARAM wp, LPARAM lp, bool& consumed) {
	switch(message) {
		case WM_CHAR:
			return onChar(wp, consumed), (consumed ? 0 : 1);
		case WM_IME_COMPOSITION:
			return onImeComposition(wp, lp, consumed), 0;
		case WM_IME_ENDCOMPOSITION:
			context_.inputMethodCompositionActivated = false;
			resetVisualization();
			break;
		case WM_IME_NOTIFY:
			if(wp == IMN_SETOPENSTATUS)
				inputPropertyListeners_.notify(&InputPropertyListener::inputMethodOpenStatusChanged);
			break;
		case WM_IME_REQUEST:
			return onImeRequest(wp, lp, consumed);
		case WM_IME_STARTCOMPOSITION:
			context_.inputMethodCompositionActivated = true;
			adjustInputMethodCompositionWindow();
			utils::closeCompletionProposalsPopup(textViewer());
			break;
		case WM_INPUTLANGCHANGE:
			inputPropertyListeners_.notify(&InputPropertyListener::inputLocaleChanged);
			break;
		case WM_SYSCHAR:
			break;
#ifdef WM_UNICHAR
		case WM_UNICHAR:
#endif // WM_UNICHAR
			if(wp != UNICODE_NOCHAR)
				return onChar(wp, consumed), (consumed ? 0 : 1);
			break;
	}
	return 0;
}

/// Handles Win32 @c WM_CHAR, @c WM_SYSCHAR and @c WM_UNICHAR window messages.
void Caret::onChar(CodePoint c, bool& consumed) {
	consumed = texteditor::commands::CharacterInputCommand(textViewer(), c)() != 0;
}

/// Handles Win32 @c WM_IME_COMPOSITION window message.
void Caret::onImeComposition(WPARAM wp, LPARAM lp, bool& consumed) {
	if(document().isReadOnly())
		return;
	else if(/*event.lParam == 0 ||*/ win32::boole(lp & GCS_RESULTSTR)) {	// completed
		win32::Handle<HIMC> imc(inputMethod(textViewer()));
		if(imc.get() != 0) {
			if(const length_t len = ::ImmGetCompositionStringW(imc.get(), GCS_RESULTSTR, 0, 0) / sizeof(WCHAR)) {
				// this was not canceled
				const AutoBuffer<Char> text(new Char[len + 1]);
				::ImmGetCompositionStringW(imc.get(), GCS_RESULTSTR, text.get(), static_cast<DWORD>(len * sizeof(WCHAR)));
				text[len] = 0;
				if(!context_.inputMethodComposingCharacter)
					texteditor::commands::TextInputCommand(textViewer(), text.get())();
				else {
					Document& doc = document();
					try {
						doc.insertUndoBoundary();
						doc.replace(Region(*this,
							static_cast<DocumentCharacterIterator&>(DocumentCharacterIterator(doc, *this).next()).tell()),
							String(1, static_cast<Char>(wp)));
						doc.insertUndoBoundary();
					} catch(const DocumentCantChangeException&) {
					}
					context_.inputMethodComposingCharacter = false;
					resetVisualization();
				}
			}
//			adjustInputMethodCompositionWindow();
			consumed = true;	// prevent to be send WM_CHARs
		}
	} else if(win32::boole(GCS_COMPSTR & lp)) {
		if(win32::boole(lp & CS_INSERTCHAR)) {
			Document& doc = document();
			const Position temp(*this);
			try {
				if(context_.inputMethodComposingCharacter)
					doc.replace(Region(*this,
						static_cast<DocumentCharacterIterator&>(DocumentCharacterIterator(doc, *this).next()).tell()),
						String(1, static_cast<Char>(wp)));
				else
					insert(doc, *this, String(1, static_cast<Char>(wp)));
				context_.inputMethodComposingCharacter = true;
				if(win32::boole(lp & CS_NOMOVECARET))
					moveTo(temp);
			} catch(...) {
			}
			consumed = true;
			resetVisualization();
		}
	}
}

/// Handles Win32 @c WM_IME_REQUEST window message.
LRESULT Caret::onImeRequest(WPARAM command, LPARAM lp, bool& consumed) {
	const Document& doc = document();

	// this command will be sent two times when reconversion is invoked
	if(command == IMR_RECONVERTSTRING) {
		if(doc.isReadOnly() || isSelectionRectangle()) {
			textViewer().beep();
			return 0L;
		}
		consumed = true;
		if(isSelectionEmpty(*this)) {	// IME selects the composition target automatically if no selection
			if(RECONVERTSTRING* const rcs = reinterpret_cast<RECONVERTSTRING*>(lp)) {
				const String& lineString = doc.line(line());
				rcs->dwStrLen = static_cast<DWORD>(lineString.length());
				rcs->dwStrOffset = sizeof(RECONVERTSTRING);
				rcs->dwTargetStrOffset = rcs->dwCompStrOffset = static_cast<DWORD>(sizeof(Char) * column());
				rcs->dwTargetStrLen = rcs->dwCompStrLen = 0;
				lineString.copy(reinterpret_cast<Char*>(reinterpret_cast<char*>(rcs) + rcs->dwStrOffset), rcs->dwStrLen);
			}
			return sizeof(RECONVERTSTRING) + sizeof(Char) * doc.lineLength(line());
		} else {
			const String selection(selectedString(*this, text::NLF_RAW_VALUE));
			if(RECONVERTSTRING* const rcs = reinterpret_cast<RECONVERTSTRING*>(lp)) {
				rcs->dwStrLen = rcs->dwTargetStrLen = rcs->dwCompStrLen = static_cast<DWORD>(selection.length());
				rcs->dwStrOffset = sizeof(RECONVERTSTRING);
				rcs->dwTargetStrOffset = rcs->dwCompStrOffset = 0;
				selection.copy(reinterpret_cast<Char*>(reinterpret_cast<char*>(rcs) + rcs->dwStrOffset), rcs->dwStrLen);
			}
			return sizeof(RECONVERTSTRING) + sizeof(Char) * selection.length();
		}
	}

	// before reconversion. a RECONVERTSTRING contains the ranges of the composition
	else if(command == IMR_CONFIRMRECONVERTSTRING) {
		if(RECONVERTSTRING* const rcs = reinterpret_cast<RECONVERTSTRING*>(lp)) {
			const Region region(doc.accessibleRegion());
			if(!isSelectionEmpty(*this)) {
				// reconvert the selected region. the selection may be multi-line
				if(rcs->dwCompStrLen < rcs->dwStrLen)	// the composition region was truncated.
					rcs->dwCompStrLen = rcs->dwStrLen;	// IME will alert and reconversion will not be happen if do this
														// (however, NotePad narrows the selection...)
			} else {
				// reconvert the region IME passed if no selection (and create the new selection).
				// in this case, reconversion across multi-line (prcs->dwStrXxx represents the entire line)
				if(doc.isNarrowed() && line() == region.first.line) {	// the document is narrowed
					if(rcs->dwCompStrOffset / sizeof(Char) < region.first.column) {
						rcs->dwCompStrLen += static_cast<DWORD>(sizeof(Char) * region.first.column - rcs->dwCompStrOffset);
						rcs->dwTargetStrLen = rcs->dwCompStrOffset;
						rcs->dwCompStrOffset = rcs->dwTargetStrOffset = static_cast<DWORD>(sizeof(Char) * region.first.column);
					} else if(rcs->dwCompStrOffset / sizeof(Char) > region.second.column) {
						rcs->dwCompStrOffset -= rcs->dwCompStrOffset - sizeof(Char) * region.second.column;
						rcs->dwTargetStrOffset = rcs->dwCompStrOffset;
						rcs->dwCompStrLen = rcs->dwTargetStrLen
							= static_cast<DWORD>(sizeof(Char) * region.second.column - rcs->dwCompStrOffset);
					}
				}
				select(
					Position(line(), rcs->dwCompStrOffset / sizeof(Char)),
					Position(line(), rcs->dwCompStrOffset / sizeof(Char) + rcs->dwCompStrLen));
			}
			consumed = true;
			return true;
		}
	}

	// queried position of the composition window
	else if(command == IMR_QUERYCHARPOSITION)
		return false;	// handled by updateIMECompositionWindowPosition...

	// queried document content for higher conversion accuracy
	else if(command == IMR_DOCUMENTFEED) {
		if(line() == anchor().line()) {
			consumed = true;
			if(RECONVERTSTRING* const rcs = reinterpret_cast<RECONVERTSTRING*>(lp)) {
				rcs->dwStrLen = static_cast<DWORD>(doc.lineLength(line()));
				rcs->dwStrOffset = sizeof(RECONVERTSTRING);
				rcs->dwCompStrLen = rcs->dwTargetStrLen = 0;
				rcs->dwCompStrOffset = rcs->dwTargetStrOffset = sizeof(Char) * static_cast<DWORD>(beginning().column());
				doc.line(line()).copy(reinterpret_cast<Char*>(reinterpret_cast<char*>(rcs) + rcs->dwStrOffset), rcs->dwStrLen);
			}
			return sizeof(RECONVERTSTRING) + sizeof(Char) * doc.lineLength(line());
		}
	}

	return 0L;
}

/**
 * Replaces the selected text by the content of the clipboard.
 * This method inserts undo boundaries at the beginning and the end.
 * @note When using the kill-ring, this method may exit in defective condition.
 * @param useKillRing Set @c true to use the kill ring
 * @throw ClipboardException The clipboard operation failed
 * @throw ClipboardException(DV_E_FORMATETC) The current clipboard format is not supported
 * @throw IllegalStateException @a useKillRing was @c true but the kill-ring was not available
 * @throw std#bad_alloc Internal memory allocation failed
 * @throw ... Any exceptions @c kernel#Document#replace throws
 */
void Caret::paste(bool useKillRing) {
	AutoFreeze af(&textViewer());
	if(!useKillRing) {
		win32::com::ComPtr<IDataObject> content;
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
		replaceSelection(text.second, rectangle);
	} else {
		texteditor::Session* const session = document().session();
		if(session == 0 || session->killRing().numberOfKills() == 0)
			throw IllegalStateException("the kill-ring is not available.");
		texteditor::KillRing& killRing = session->killRing();
		const pair<String, bool>& text = context_.yanking ? killRing.setCurrent(+1) : killRing.get();

		const Position temp(beginning());
		try {
			if(!isSelectionEmpty(*this) && context_.yanking)
				document().undo();
			replaceSelection(text.first, text.second);
		} catch(...) {
			killRing.setCurrent(-1);
			throw;
		}
		if(!text.second)
			endRectangleSelection();
		else
			beginRectangleSelection();
		select(temp, position());
		context_.yanking = true;
	}
	document().insertUndoBoundary();
}

/**
 * Sets the locale used to convert non-Unicode data in the clipboard.
 * @param newLocale The locale identifier
 * @return The identifier of the locale set by the caret
 * @throw std#invalid_argument @a newLocale is not installed on the system
 * @see #clipboardLocale
 */
LCID Caret::setClipboardLocale(LCID newLocale) {
	if(!win32::boole(::IsValidLocale(newLocale, LCID_INSTALLED)))
		throw invalid_argument("newLocale");
	std::swap(clipboardLocale_, newLocale);
	return newLocale;
}


// viewers free functions /////////////////////////////////////////////////////////////////////////

/**
 * Copies the selected content to the clipboard.
 * If the caret does not have a selection, this function does nothing.
 * @param caret The caret gives the selection
 * @param useKillRing Set @c true to send to the kill ring, not only the system clipboard
 * @throw ClipboardException The clipboard operation failed
 * @throw std#bad_alloc Internal memory allocation failed
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
 * @param caret The caret gives the selection
 * @param useKillRing Set @c true to send also the kill ring
 * @return false if the change was interrupted
 * @throw ClipboardException The clipboard operation failed
 * @throw std#bad_alloc Internal memory allocation failed
 * @throw ... Any exceptions @c Document#replace throws
 */
void viewers::cutSelection(Caret& caret, bool useKillRing) {
	if(isSelectionEmpty(caret))
		return;

	win32::com::ComPtr<IDataObject> previousContent;
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
