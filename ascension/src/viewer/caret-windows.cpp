/**
 * @file caret-windows.cpp
 * @author exeal
 * @date 2003-2008 was point.cpp
 * @date 2008-2010 separated from point.cpp
 * @date 2011-10-03 separated from caret.cpp
 * @date 2011-2013
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
using namespace ascension::viewers;
using namespace ascension::presentation;
using namespace ascension::text;
using namespace ascension::text::ucd;
using namespace std;
namespace k = ascension::kernel;


namespace {
#pragma comment(lib, "urlmon.lib")

	// managed (but ugly) STGMEDIUM
	struct GlobalStgMedium : public STGMEDIUM {
		GlobalStgMedium() /*noexcept*/ {
			tymed = TYMED_NULL;
			pUnkForRelease = nullptr;
		}
		explicit GlobalStgMedium(size_t nbytes, UINT flags = GHND | GMEM_SHARE) {
			tymed = TYMED_HGLOBAL;
			if(0 == (hGlobal = ::GlobalAlloc(flags, nbytes)))
				throw makePlatformError();
		}
		~GlobalStgMedium() /*noexcept*/ {
			if(lockedBuffer_ != nullptr)
				::GlobalUnlock(hGlobal);
			::ReleaseStgMedium(this);
		}
		void* lock() {
			assert(tymed == TYMED_HGLOBAL && hGlobal != nullptr);
			if(lockedBuffer_ != nullptr)
				lockedBuffer_ = static_cast<char*>(::GlobalLock(hGlobal));
			if(lockedBuffer_ == nullptr)
				throw makePlatformError();
			return lockedBuffer();
		}
		void* lockedBuffer() /*noexcept*/ {return lockedBuffer_;}
		const void* lockedBuffer() const /*noexcept*/ {return lockedBuffer_;}
	private:
		char* lockedBuffer_;
	};

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
		ASCENSION_WIN32_COM_INTERFACE(IDataObject), win32::com::NoReferenceCounting> {
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
		if(format.ptd == nullptr) {	// this does not support DVTARGETDEVICE
			for(list<Entry*>::iterator i(initial); i != e; ++i) {
				const FORMATETC& other = (*i)->format;
				if(other.cfFormat == format.cfFormat && other.dwAspect == format.dwAspect && other.lindex == format.lindex)
					return i;
			}
		}
		return e;
	}
	STDMETHODIMP GenericDataObject::GetData(FORMATETC* format, STGMEDIUM* medium) {
		if(format == nullptr || medium == nullptr)
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
			medium->pUnkForRelease = nullptr;
		return hr;
	}
	STDMETHODIMP GenericDataObject::QueryGetData(LPFORMATETC format) {
		if(format == nullptr)
			return E_INVALIDARG;
		else if(format->lindex != -1)
			return DV_E_LINDEX;
		list<Entry*>::const_iterator entry(find(*format, entries_.begin()));
		if(entry == entries_.end())
			return DV_E_FORMATETC;
		return (((*entry)->format.tymed & format->tymed) != 0) ? S_OK : DV_E_TYMED;
	}
	STDMETHODIMP GenericDataObject::GetCanonicalFormatEtc(FORMATETC* in, FORMATETC* out) {
		if(in == nullptr || out == nullptr)
			return E_INVALIDARG;
		else if(in->lindex != -1)
			return DV_E_LINDEX;
		else if(in->ptd != nullptr)
			return DV_E_FORMATETC;
		*out = *in;
		return DATA_S_SAMEFORMATETC;
	}
	STDMETHODIMP GenericDataObject::SetData(FORMATETC* format, STGMEDIUM* medium, BOOL release) {
		if(format == nullptr || medium == nullptr)
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
			if(newEntry == nullptr)
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
		else if(enumerator == nullptr)
			return E_INVALIDARG;
		FORMATETC* buffer = static_cast<FORMATETC*>(::CoTaskMemAlloc(sizeof(FORMATETC) * entries_.size()));
		if(buffer == nullptr)
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
	inline const IdentifierSyntax& identifierSyntax(const k::Point& p) {
		return p.document().contentTypeInformation().getIdentifierSyntax(contentType(p));
	}
} // namespace @0

win32::com::SmartPointer<widgetapi::NativeMimeData> utils::createMimeDataForSelectedString(const Caret& caret, bool rtf) {
	win32::com::SmartPointer<IDataObject> content(new GenericDataObject());

	// get text on the given region
	const String text(selectedString(caret, text::Newline::CARRIAGE_RETURN_FOLLOWED_BY_LINE_FEED));

	// register datas...
	FORMATETC format;
	format.ptd = nullptr;
	format.dwAspect = DVASPECT_CONTENT;
	format.lindex = -1;
	format.tymed = TYMED_HGLOBAL;

	// Unicode text format
	{
		format.cfFormat = CF_UNICODETEXT;
		GlobalStgMedium unicodeData(sizeof(Char) * (text.length() + 1));
		wcscpy(static_cast<wchar_t*>(unicodeData.lock()), text.c_str());
		HRESULT hr = content->SetData(&format, &unicodeData, false);

		// rectangle text format
		if(caret.isSelectionRectangle()) {
			if(0 != (format.cfFormat = static_cast<CLIPFORMAT>(::RegisterClipboardFormatW(ASCENSION_RECTANGLE_TEXT_MIME_FORMAT))))
				hr = content->SetData(&format, &unicodeData, false);
		}
	}

	// ANSI text format and locale
	UINT codePage = CP_ACP;
	wchar_t codePageString[6];
	if(0 != ::GetLocaleInfoW(caret.clipboardLocale(), LOCALE_IDEFAULTANSICODEPAGE, codePageString, ASCENSION_COUNTOF(codePageString))) {
		wchar_t* eob;
		codePage = wcstoul(codePageString, &eob, 10);
	}
	format.cfFormat = CF_TEXT;
	int nativeLength = ::WideCharToMultiByte(codePage, 0, text.c_str(), static_cast<int>(text.length()), nullptr, 0, nullptr, nullptr);
	if(nativeLength != 0 || text.empty()) {
		unique_ptr<char[]> nativeBuffer(new char[nativeLength]);
		nativeLength = ::WideCharToMultiByte(codePage, 0,
			text.data(), static_cast<int>(text.length()), nativeBuffer.get(), nativeLength, nullptr, nullptr);
		if(nativeLength != 0 || text.empty()) {
			GlobalStgMedium nativeData(sizeof(char) * (nativeLength + 1));
			nativeData.lock();
			memcpy(nativeData.lockedBuffer(), nativeBuffer.get(), sizeof(char) * nativeLength);
			static_cast<char*>(nativeData.lockedBuffer())[nativeLength] = 0;
			HRESULT hr = content->SetData(&format, &nativeData, false);
			if(SUCCEEDED(hr)) {
				format.cfFormat = CF_LOCALE;
				GlobalStgMedium localeData(sizeof(LCID));
				*static_cast<LCID*>(localeData.lock()) = caret.clipboardLocale();
				hr = content->SetData(&format, &localeData, false);
			}
		}
	}

	if(rtf) {
		const CLIPFORMAT rtfFormat = static_cast<CLIPFORMAT>(::RegisterClipboardFormatW(L"Rich Text Format"));	// CF_RTF
		const CLIPFORMAT rtfWithoutObjectsFormat = static_cast<CLIPFORMAT>(::RegisterClipboardFormatW(L"Rich Text Format Without Objects"));	// CF_RTFNOOBJS
		// TODO: implement the follow...
	}

	return win32::com::SmartPointer<IDataObject>(content);
}

pair<String, bool> utils::getTextFromMimeData(const widgetapi::NativeMimeData& data) {
	pair<String, bool> result;
	FORMATETC fe = {CF_UNICODETEXT, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	STGMEDIUM stm = {TYMED_HGLOBAL, nullptr};
	IDataObject& dataObject = const_cast<IDataObject&>(data);
	HRESULT hr = dataObject.QueryGetData(&fe);
	if(hr == S_OK) {	// the data suppports CF_UNICODETEXT ?
		if(SUCCEEDED(hr = dataObject.GetData(&fe, &stm))) {
			if(const Char* const buffer = static_cast<Char*>(::GlobalLock(stm.hGlobal))) {
				try {
					result.first = String(buffer);
				} catch(...) {
					::GlobalUnlock(stm.hGlobal);
					::ReleaseStgMedium(&stm);
					throw;
				}
				::GlobalUnlock(stm.hGlobal);
				::ReleaseStgMedium(&stm);
			} else
				hr = E_FAIL;
		}
	}

	if(FAILED(hr)) {
		fe.cfFormat = CF_TEXT;
		if(S_OK == (hr = dataObject.QueryGetData(&fe))	// the data supports CF_TEXT ?
				&& SUCCEEDED(hr = dataObject.GetData(&fe, &stm))) {
			if(const char* const nativeBuffer = static_cast<char*>(::GlobalLock(stm.hGlobal))) {
				// determine the encoding of the content of the clipboard
				UINT codePage = ::GetACP();
				fe.cfFormat = CF_LOCALE;
				if(S_OK == (hr = dataObject.QueryGetData(&fe))) {
					STGMEDIUM locale = {TYMED_HGLOBAL, nullptr};
					if(S_OK == (hr = dataObject.GetData(&fe, &locale))) {
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
				const Index nativeLength = min<Index>(
					strlen(nativeBuffer), ::GlobalSize(stm.hGlobal) / sizeof(char)) + 1;
				if(nativeLength != 0) {
					const Index ucsLength = ::MultiByteToWideChar(
						codePage, MB_PRECOMPOSED, nativeBuffer, static_cast<int>(nativeLength), nullptr, 0);
					try {
						if(ucsLength == 0)
							throw makePlatformError();
						unique_ptr<WCHAR[]> ucsBuffer(new WCHAR[ucsLength]);
						if(0 != ::MultiByteToWideChar(codePage, MB_PRECOMPOSED,
								nativeBuffer, static_cast<int>(nativeLength), ucsBuffer.get(), static_cast<int>(ucsLength)))
							result.first = String(ucsBuffer.get(), ucsLength - 1);
						else
							throw makePlatformError();
					} catch(...) {
						::GlobalUnlock(stm.hGlobal);
						::ReleaseStgMedium(&stm);
						throw;
					}
				}
				::GlobalUnlock(stm.hGlobal);
				::ReleaseStgMedium(&stm);
			}
		}
	}

	if(FAILED(hr))
		throw ClipboardException(DV_E_FORMATETC);
	fe.cfFormat = static_cast<CLIPFORMAT>(::RegisterClipboardFormatW(ASCENSION_RECTANGLE_TEXT_MIME_FORMAT));
	result.second = fe.cfFormat != 0 && dataObject.QueryGetData(&fe) == S_OK;

	return result;
}


// ClipboardException /////////////////////////////////////////////////////////////////////////////

ClipboardException::ClipboardException(HRESULT hr) : system_error(makePlatformError(hr)) {
}


// Caret //////////////////////////////////////////////////////////////////////////////////////////

void Caret::abortInput() {
	if(context_.inputMethodCompositionActivated)	// stop IME input
		::ImmNotifyIME(inputMethod(textViewer()).get(), NI_COMPOSITIONSTR, CPS_CANCEL, 0);
}

/// Moves the IME form to valid position.
void Caret::adjustInputMethodCompositionWindow() {
	assert(win32::boole(::IsWindow(textViewer().handle().get())));
	if(!context_.inputMethodCompositionActivated)
		return;
	if(win32::Handle<HIMC>::Type imc = inputMethod(textViewer())) {
		// composition window placement
		const shared_ptr<const font::TextViewport> viewport(textViewer().textRenderer().viewport());
		COMPOSITIONFORM cf;
		cf.rcArea = textViewer().textAreaContentRectangle();
		cf.dwStyle = CFS_POINT;
		cf.ptCurrentPos = modelToView(*viewport, font::TextHit<k::Position>::leading(beginning()), false);
		if(cf.ptCurrentPos.y == numeric_limits<Scalar>::max() || cf.ptCurrentPos.y == numeric_limits<Scalar>::min())
			cf.ptCurrentPos.y = (cf.ptCurrentPos.y == numeric_limits<Scalar>::min()) ? cf.rcArea.top : cf.rcArea.bottom;
		else
			cf.ptCurrentPos.y = max(cf.ptCurrentPos.y, cf.rcArea.top);
		::ImmSetCompositionWindow(imc.get(), &cf);
		cf.dwStyle = CFS_RECT;
		::ImmSetCompositionWindow(imc.get(), &cf);

		// composition font
		LOGFONTW font;
		::GetObjectW(textViewer().textRenderer().defaultFont()->asNativeObject().get(), sizeof(LOGFONTW), &font);
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
		const UINT rectangleClipFormat = ::RegisterClipboardFormatW(ASCENSION_RECTANGLE_TEXT_MIME_FORMAT);
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
		win32::Handle<HIMC>::Type imc(inputMethod(textViewer()));
		if(imc.get() != nullptr) {
			if(const Index len = ::ImmGetCompositionStringW(imc.get(), GCS_RESULTSTR, nullptr, 0) / sizeof(WCHAR)) {
				// this was not canceled
				const unique_ptr<Char[]> text(new Char[len + 1]);
				::ImmGetCompositionStringW(imc.get(), GCS_RESULTSTR, text.get(), static_cast<DWORD>(len * sizeof(WCHAR)));
				text[len] = 0;
				if(!context_.inputMethodComposingCharacter)
					texteditor::commands::TextInputCommand(textViewer(), text.get())();
				else {
					k::Document& doc = document();
					try {
						doc.insertUndoBoundary();
						doc.replace(k::Region(*this,
							static_cast<k::DocumentCharacterIterator&>(k::DocumentCharacterIterator(doc, *this).next()).tell()),
							String(1, static_cast<Char>(wp)));
						doc.insertUndoBoundary();
					} catch(const k::DocumentCantChangeException&) {
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
			k::Document& doc = document();
			const k::Position temp(*this);
			try {
				if(context_.inputMethodComposingCharacter)
					doc.replace(k::Region(*this,
						static_cast<k::DocumentCharacterIterator&>(k::DocumentCharacterIterator(doc, *this).next()).tell()),
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
	const k::Document& doc = document();

	// this command will be sent two times when reconversion is invoked
	if(command == IMR_RECONVERTSTRING) {
		if(doc.isReadOnly() || isSelectionRectangle()) {
			textViewer().beep();
			return 0L;
		}
		consumed = true;
		if(isSelectionEmpty(*this)) {	// IME selects the composition target automatically if no selection
			if(RECONVERTSTRING* const rcs = reinterpret_cast<RECONVERTSTRING*>(lp)) {
				const String& lineString = doc.line(line(*this));
				rcs->dwStrLen = static_cast<DWORD>(lineString.length());
				rcs->dwStrOffset = sizeof(RECONVERTSTRING);
				rcs->dwTargetStrOffset = rcs->dwCompStrOffset = static_cast<DWORD>(sizeof(Char) * offsetInLine(*this));
				rcs->dwTargetStrLen = rcs->dwCompStrLen = 0;
				lineString.copy(reinterpret_cast<Char*>(reinterpret_cast<char*>(rcs) + rcs->dwStrOffset), rcs->dwStrLen);
			}
			return sizeof(RECONVERTSTRING) + sizeof(Char) * doc.lineLength(line(*this));
		} else {
			const String selection(selectedString(*this, text::Newline::USE_INTRINSIC_VALUE));
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
			const k::Region region(doc.accessibleRegion());
			if(!isSelectionEmpty(*this)) {
				// reconvert the selected region. the selection may be multi-line
				if(rcs->dwCompStrLen < rcs->dwStrLen)	// the composition region was truncated.
					rcs->dwCompStrLen = rcs->dwStrLen;	// IME will alert and reconversion will not be happen if do this
														// (however, NotePad narrows the selection...)
			} else {
				// reconvert the region IME passed if no selection (and create the new selection).
				// in this case, reconversion across multi-line (prcs->dwStrXxx represents the entire line)
				if(doc.isNarrowed() && line(*this) == region.first.line) {	// the document is narrowed
					if(rcs->dwCompStrOffset / sizeof(Char) < region.first.offsetInLine) {
						rcs->dwCompStrLen += static_cast<DWORD>(sizeof(Char) * region.first.offsetInLine - rcs->dwCompStrOffset);
						rcs->dwTargetStrLen = rcs->dwCompStrOffset;
						rcs->dwCompStrOffset = rcs->dwTargetStrOffset = static_cast<DWORD>(sizeof(Char) * region.first.offsetInLine);
					} else if(rcs->dwCompStrOffset / sizeof(Char) > region.second.offsetInLine) {
						rcs->dwCompStrOffset -= rcs->dwCompStrOffset - sizeof(Char) * region.second.offsetInLine;
						rcs->dwTargetStrOffset = rcs->dwCompStrOffset;
						rcs->dwCompStrLen = rcs->dwTargetStrLen
							= static_cast<DWORD>(sizeof(Char) * region.second.offsetInLine - rcs->dwCompStrOffset);
					}
				}
				select(
					k::Position(line(*this), rcs->dwCompStrOffset / sizeof(Char)),
					k::Position(line(*this), rcs->dwCompStrOffset / sizeof(Char) + rcs->dwCompStrLen));
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
		if(line(*this) == line(anchor())) {
			consumed = true;
			if(RECONVERTSTRING* const rcs = reinterpret_cast<RECONVERTSTRING*>(lp)) {
				rcs->dwStrLen = static_cast<DWORD>(doc.lineLength(line(*this)));
				rcs->dwStrOffset = sizeof(RECONVERTSTRING);
				rcs->dwCompStrLen = rcs->dwTargetStrLen = 0;
				rcs->dwCompStrOffset = rcs->dwTargetStrOffset = sizeof(Char) * static_cast<DWORD>(offsetInLine(beginning()));
				doc.line(line(*this)).copy(reinterpret_cast<Char*>(reinterpret_cast<char*>(rcs) + rcs->dwStrOffset), rcs->dwStrLen);
			}
			return sizeof(RECONVERTSTRING) + sizeof(Char) * doc.lineLength(line(*this));
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
		win32::com::SmartPointer<IDataObject> content;
		HRESULT hr = tryOleClipboard(::OleGetClipboard, content.initialize());
		if(hr == E_OUTOFMEMORY)
			throw bad_alloc("::OleGetClipboard returned E_OUTOFMEMORY.");
		else if(FAILED(hr))
			throw ClipboardException(hr);
		const pair<String, bool> text(utils::getTextFromMimeData(*content));	// this may throw several exceptions
		document().insertUndoBoundary();
		replaceSelection(text.first, text.second);
	} else {
		texteditor::Session* const session = document().session();
		if(session == nullptr || session->killRing().numberOfKills() == 0)
			throw IllegalStateException("the kill-ring is not available.");
		texteditor::KillRing& killRing = session->killRing();
		const pair<String, bool>& text = context_.yanking ? killRing.setCurrent(+1) : killRing.get();

		const k::Position temp(beginning());
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

	win32::com::SmartPointer<widgetapi::NativeMimeData> content(
		utils::createMimeDataForSelectedString(caret, true));	// this may throw std.bad_alloc
	HRESULT hr = tryOleClipboard(::OleSetClipboard, content.get());
	if(FAILED(hr))
		throw ClipboardException(hr);
	hr = tryOleClipboard(::OleFlushClipboard);
	if(useKillRing) {
		if(texteditor::Session* const session = caret.document().session())
			session->killRing().addNew(selectedString(caret, text::Newline::USE_INTRINSIC_VALUE), caret.isSelectionRectangle());
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

	win32::com::SmartPointer<IDataObject> previousContent;
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
