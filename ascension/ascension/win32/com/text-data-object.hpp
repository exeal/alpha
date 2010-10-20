// text-data-object.hpp
// (c) 2002-2007 exeal

#ifndef MANAH_TEXT_DATA_OBJECT_HPP
#define MANAH_TEXT_DATA_OBJECT_HPP
#include "unknown-impl.hpp"
#include "../win32/ui/window.hpp"
#include <set>

namespace manah {
namespace com {
namespace ole {

/// @c IDataObject implementation for OLE text drag and drop. This supports both ANSI and Unicode format strings.
class TextDataObject : public virtual ::IDataObject {
public:
	// constructor
	TextDataObject(::IDropSource& dropSource);
	virtual ~TextDataObject();
	// methods
	DWORD	doDragDrop(DWORD effect);
	void	setTextData(const char* text);
	void	setTextData(const wchar_t* text);
	template<typename InputIterator>
	void	setAvailableFormatSet(InputIterator first, InputIterator last);
	// IUnknown
	IMPLEMENT_UNKNOWN_SINGLE_THREADED()
	BEGIN_INTERFACE_TABLE()
		IMPLEMENTS_LEFTMOST_INTERFACE(IDataObject)
	END_INTERFACE_TABLE()
	// IDataObject
	STDMETHODIMP	GetData(::LPFORMATETC pformatetcIn, ::LPSTGMEDIUM pmedium);
	STDMETHODIMP	GetDataHere(::LPFORMATETC pformatetc, ::LPSTGMEDIUM pmedium);
	STDMETHODIMP	QueryGetData(::LPFORMATETC pformatetc);
	STDMETHODIMP	GetCanonicalFormatEtc(::LPFORMATETC pformatectIn, ::LPFORMATETC pformatetcOut);
	STDMETHODIMP	SetData(::LPFORMATETC pformatetc, ::LPSTGMEDIUM pmedium, BOOL fRelease);
	STDMETHODIMP	EnumFormatEtc(DWORD dwDirection, ::LPENUMFORMATETC* ppenumFormatEtc);
	STDMETHODIMP	DAdvise(::LPFORMATETC pformatetc, DWORD advf, ::LPADVISESINK pAdvSink, LPDWORD pdwConnection);
	STDMETHODIMP	DUnadvise(DWORD dwConnection);
	STDMETHODIMP	EnumDAdvise(::LPENUMSTATDATA* ppenumAdvise);
private:
	ComPtr<::IDropSource> dropSource_;
	std::set<::CLIPFORMAT> clipFormats_;
	HGLOBAL ansiData_, unicodeData_;

	/// Returned value of @c IDataObject#EnumFormatEtc.
	class AvailableFormatsEnumerator : virtual public IEnumFORMATETC {
	public:
		AvailableFormatsEnumerator(const std::set<CLIPFORMAT>& formats);
		// IUnknown
		IMPLEMENT_UNKNOWN_SINGLE_THREADED()
		BEGIN_INTERFACE_TABLE()
			IMPLEMENTS_LEFTMOST_INTERFACE(IEnumFORMATETC)
		END_INTERFACE_TABLE()
		// IEnumFORMATETC
		STDMETHODIMP	Next(ULONG celt, ::FORMATETC* rgelt, ULONG* pceltFetched);
        STDMETHODIMP	Skip(ULONG celt);
        STDMETHODIMP	Reset();
		STDMETHODIMP	Clone(::IEnumFORMATETC** ppenum);
	private:
		const std::set<::CLIPFORMAT>* clipFormats_;
		std::set<::CLIPFORMAT>::const_iterator current_;
	};
};


// TextDataObject ///////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param dropSource the source of drop.
 */
inline TextDataObject::TextDataObject(IDropSource& dropSource) : dropSource_(&dropSource), ansiData_(0), unicodeData_(0) {
}

/// Destructor.
inline TextDataObject::~TextDataObject() {
	if(ansiData_ != 0)
		::GlobalFree(ansiData_);
	if(unicodeData_ != 0)
		::GlobalFree(unicodeData_);
}

/// @see IDataObject#DAdvise
inline STDMETHODIMP TextDataObject::DAdvise(::LPFORMATETC, DWORD, ::LPADVISESINK, LPDWORD) {
	return OLE_E_ADVISENOTSUPPORTED;
}

/**
 * Begins OLE drag-and-drop.
 * @param effect same as Win32 @c DoDragDrop function
 * @return the result
 */
inline DWORD TextDataObject::doDragDrop(DWORD effect) {
	DWORD			effectOwn;
	const HRESULT	hr = ::DoDragDrop(this, dropSource_, effect, &effectOwn);
	return SUCCEEDED(hr) ? effectOwn : DROPEFFECT_NONE;
}

/// @see IDataObject#DUnadvise
inline STDMETHODIMP TextDataObject::DUnadvise(DWORD) {
	return OLE_E_ADVISENOTSUPPORTED;
}

/// @see IDataObject#EnumDAdvise
inline STDMETHODIMP TextDataObject::EnumDAdvise(::LPENUMSTATDATA*) {
	return OLE_E_ADVISENOTSUPPORTED;
}

/// @see IDataObject#EnumFormatEtc
inline STDMETHODIMP TextDataObject::EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC* ppenumFormatEtc) {
	if(dwDirection == DATADIR_SET)
		return E_NOTIMPL;
	else if(ppenumFormatEtc == 0)
		return E_INVALIDARG;

	*ppenumFormatEtc = new AvailableFormatsEnumerator(clipFormats_);
	if(*ppenumFormatEtc == 0)
		return E_OUTOFMEMORY;
	(*ppenumFormatEtc)->AddRef();
	return S_OK;
}

/// @see IDataObject#GetCanonicalFormatEtc
inline STDMETHODIMP TextDataObject::GetCanonicalFormatEtc(::LPFORMATETC, ::LPFORMATETC) {
	return DATA_S_SAMEFORMATETC;
}

/// @see IDataObject#GetData
inline STDMETHODIMP TextDataObject::GetData(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium) {
	if(pformatetcIn == 0 || pmedium == 0)
		return E_INVALIDARG;
	if(ansiData_ == 0 && unicodeData_ == 0)
		return OLE_E_NOTRUNNING;
	if((pformatetcIn->cfFormat != CF_TEXT && pformatetcIn->cfFormat != CF_UNICODETEXT)
			|| pformatetcIn->dwAspect != DVASPECT_CONTENT
			|| pformatetcIn->lindex != -1
			|| !toBoolean(pformatetcIn->tymed & TYMED_HGLOBAL))
		return DV_E_FORMATETC;

	// Create data if there is not a data has the requested format
	if(pformatetcIn->cfFormat == CF_TEXT && ansiData_ == 0) {
		const wchar_t*	pwsz = static_cast<const wchar_t*>(::GlobalLock(unicodeData_));
		const int		cch = ::WideCharToMultiByte(CP_ACP, 0, pwsz, -1, 0, 0, 0, 0);
		ansiData_ = ::GlobalAlloc(GHND | GMEM_SHARE, sizeof(char) * cch);
		::WideCharToMultiByte(CP_ACP, 0, pwsz, -1, static_cast<char*>(::GlobalLock(ansiData_)), cch, 0, 0);
		::GlobalUnlock(ansiData_);
	} else if(pformatetcIn->cfFormat == CF_UNICODETEXT && unicodeData_ == 0) {
		const char*	psz = static_cast<const char*>(::GlobalLock(ansiData_));
		const int	cch = ::MultiByteToWideChar(CP_ACP, 0, psz, -1, 0, 0);
		unicodeData_ = ::GlobalAlloc(GHND | GMEM_SHARE, sizeof(wchar_t) * cch);
		::MultiByteToWideChar(CP_ACP, 0, psz, -1, static_cast<wchar_t*>(::GlobalLock(unicodeData_)), cch);
		::GlobalUnlock(unicodeData_);
	}

	pmedium->tymed = TYMED_HGLOBAL;
	pmedium->hGlobal = (pformatetcIn->cfFormat == CF_TEXT) ? ansiData_ : unicodeData_;
	pmedium->pUnkForRelease = 0;

	return S_OK;
}

/// @see IDataObject#GetDataHere
inline STDMETHODIMP TextDataObject::GetDataHere(::LPFORMATETC, ::LPSTGMEDIUM) {
	return E_NOTIMPL;
}

/// @see IDataObject#QueryGetData
inline STDMETHODIMP TextDataObject::QueryGetData(::LPFORMATETC pformatetc) {
	if(pformatetc == 0)
		return E_INVALIDARG;
	if(clipFormats_.find(pformatetc->cfFormat) == clipFormats_.end())
		return DV_E_FORMATETC;
	if(ansiData_ == 0 && unicodeData_ == 0)
		return OLE_E_NOTRUNNING;
	if(pformatetc->lindex != -1)
		return DV_E_LINDEX;
	if(!toBoolean(pformatetc->tymed & TYMED_HGLOBAL))
		return DV_E_TYMED;
	if(pformatetc->dwAspect != DVASPECT_CONTENT)
		return DV_E_DVASPECT;

	return S_OK;
}

/**
 * Sets the clipboard formats available for obtaining the text. @p InputIterator should points
 * @c CLIPFORMAT.
 * @param first the first of the formats
 * @param last the last of the formats
 */
template<typename InputIterator>
inline void TextDataObject::setAvailableFormatSet(InputIterator first, InputIterator last) throw() {
	clipFormats_.clear();
	clipFormats_.insert(first, last);
}

/// @see IDataObject#SetData
inline STDMETHODIMP TextDataObject::SetData(::LPFORMATETC, ::LPSTGMEDIUM, BOOL) {
	return E_NOTIMPL;
}

/**
 * Sets the text as ANSI format.
 * @param text the text to set
 */
inline void TextDataObject::setTextData(const char* text) {
	assert(text != 0);

	if(ansiData_ != 0) {
		::GlobalFree(ansiData_);
		ansiData_ = 0;
	}
	if(unicodeData_ != 0) {
		::GlobalFree(unicodeData_);
		unicodeData_ = 0;
	}

	ansiData_ = ::GlobalAlloc(GHND | GMEM_SHARE, sizeof(char) * (std::strlen(text) + 1));
	std::strcpy(static_cast<char*>(::GlobalLock(ansiData_)), text);
	::GlobalUnlock(ansiData_);
	clipFormats_.insert(CF_TEXT);
	clipFormats_.insert(CF_UNICODETEXT);
}

/**
 * Sets the text as Unicode format.
 * @param text the text to set
 */
inline void TextDataObject::setTextData(const wchar_t* text) {
	assert(text != 0);

	if(ansiData_ != 0) {
		::GlobalFree(ansiData_);
		ansiData_ = 0;
	}
	if(unicodeData_ != 0) {
		::GlobalFree(unicodeData_);
		unicodeData_ = 0;
	}

	unicodeData_ = ::GlobalAlloc(GHND | GMEM_SHARE, sizeof(wchar_t) * (std::wcslen(text) + 1));
	std::wcscpy(static_cast<wchar_t*>(::GlobalLock(unicodeData_)), text);
	::GlobalUnlock(unicodeData_);
	clipFormats_.insert(CF_TEXT);
	clipFormats_.insert(CF_UNICODETEXT);
}


// AvailableFormatsEnumerator ///////////////////////////////////////////////

/// Constructor.
inline TextDataObject::AvailableFormatsEnumerator::AvailableFormatsEnumerator(const std::set<::CLIPFORMAT>& formats) : clipFormats_(&formats) {
	Reset();
}

/// @see IEnumFORMATETC#Clone
inline STDMETHODIMP TextDataObject::AvailableFormatsEnumerator::Clone(::IEnumFORMATETC** ppenum) {
	VERIFY_POINTER(ppenum);
	if(*ppenum = new(std::nothrow) AvailableFormatsEnumerator(*clipFormats_))
		return (*ppenum)->AddRef(), S_OK;
	return E_OUTOFMEMORY;
}

/// @see IEnumFORMATETC#Next
inline STDMETHODIMP TextDataObject::AvailableFormatsEnumerator::Next(ULONG celt, ::FORMATETC* rgelt, ULONG* pceltFetched) {
	if(celt > 1 && pceltFetched == 0)
		return E_INVALIDARG;

	ULONG fetched = 0;
	while(fetched < celt && current_ != clipFormats_->end()) {
		rgelt[fetched].cfFormat = *current_;
		rgelt[fetched].ptd = 0;
		rgelt[fetched].dwAspect = DVASPECT_CONTENT;
		rgelt[fetched].lindex = -1;
		rgelt[fetched].tymed = TYMED_HGLOBAL;
		++fetched;
		++current_;
	}
	if(pceltFetched != 0)
		*pceltFetched = fetched;

	return (fetched == celt) ? S_OK : S_FALSE;
}

/// @see IEnumFORMATETC#Reset
inline STDMETHODIMP TextDataObject::AvailableFormatsEnumerator::Reset() {
	current_ = clipFormats_->begin();
	return S_OK;
}

/// @see IEnumFORMATETC#Skip
inline STDMETHODIMP TextDataObject::AvailableFormatsEnumerator::Skip(ULONG celt) {
	while(celt != 0 && current_ != clipFormats_->end())
		--celt, ++current_;
	return (celt == 0) ? S_OK : S_FALSE;
}

}}} // namespace manah::com::ole

#endif /* !MANAH_TEXT_DATA_OBJECT_HPP */
