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

/// テキストデータのドラッグアンドドロップに使用する IDataObject 実装。ANSI 、Unicode の両方に対応
class TextDataObject : public virtual IDataObject {
public:
	// コンストラクタ
	TextDataObject(IDropSource& dropSource);
	virtual ~TextDataObject();
	// メソッド
	DWORD	doDragDrop(DWORD effect);
	void	setTextData(const char* text);
	void	setTextData(const wchar_t* text);
	void	setAvailableFormatSet(const std::set<CLIPFORMAT>& formats);
	// IUnknown
	IMPLEMENT_UNKNOWN_SINGLE_THREADED()
	BEGIN_INTERFACE_TABLE()
		IMPLEMENTS_LEFTMOST_INTERFACE(IDataObject)
	END_INTERFACE_TABLE()
	// IDataObject
	STDMETHODIMP	GetData(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium);
	STDMETHODIMP	GetDataHere(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium);
	STDMETHODIMP	QueryGetData(LPFORMATETC pformatetc);
	STDMETHODIMP	GetCanonicalFormatEtc(LPFORMATETC pformatectIn, LPFORMATETC pformatetcOut);
	STDMETHODIMP	SetData(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium, BOOL fRelease);
	STDMETHODIMP	EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC* ppenumFormatEtc);
	STDMETHODIMP	DAdvise(LPFORMATETC pformatetc, DWORD advf, LPADVISESINK pAdvSink, LPDWORD pdwConnection);
	STDMETHODIMP	DUnadvise(DWORD dwConnection);
	STDMETHODIMP	EnumDAdvise(LPENUMSTATDATA* ppenumAdvise);
private:
	ComPtr<IDropSource>		dropSource_;
	std::set<CLIPFORMAT>	clipFormats_;
	HGLOBAL					ansiData_;
	HGLOBAL					unicodeData_;

	/// IDataObject::EnumFormatEtc の戻り値
	class AvailableFormatsEnumerator : virtual public IEnumFORMATETC {
	public:
		// コンストラクタ
		AvailableFormatsEnumerator(const std::set<CLIPFORMAT>& formats);
		// IUnknown
		IMPLEMENT_UNKNOWN_SINGLE_THREADED()
		BEGIN_INTERFACE_TABLE()
			IMPLEMENTS_LEFTMOST_INTERFACE(IEnumFORMATETC)
		END_INTERFACE_TABLE()
		// IEnumFORMATETC
        STDMETHODIMP	Next(ULONG celt, FORMATETC* rgelt, ULONG* pceltFetched);
        STDMETHODIMP	Skip(ULONG celt);
        STDMETHODIMP	Reset();
        STDMETHODIMP	Clone(IEnumFORMATETC** ppenum);
	private:
		const std::set<CLIPFORMAT>* clipFormats_;
		std::set<CLIPFORMAT>::const_iterator it_;
	};
};


// TextDataObject class implementation
/////////////////////////////////////////////////////////////////////////////

/**
 *	コンストラクタ
 *	@param dropSource ドラッグ元
 */
inline TextDataObject::TextDataObject(IDropSource& dropSource) : dropSource_(&dropSource), ansiData_(0), unicodeData_(0) {}

/// デストラクタ
inline TextDataObject::~TextDataObject() {
	if(ansiData_ != 0)		::GlobalFree(ansiData_);
	if(unicodeData_ != 0)	::GlobalFree(unicodeData_);
}

/// @see	IDataObject::DAdvise
inline STDMETHODIMP TextDataObject::DAdvise(LPFORMATETC pformatetc, DWORD advf, LPADVISESINK pAdvSink, LPDWORD pdwConnection) {
	return OLE_E_ADVISENOTSUPPORTED;
}

/**
 *	ドラッグアンドドロップを開始する
 *	@param effect	DoDragDrop 参照
 *	@return			結果
 */
inline DWORD TextDataObject::doDragDrop(DWORD effect) {
	DWORD			effectOwn;
	const HRESULT	hr = ::DoDragDrop(this, dropSource_, effect, &effectOwn);
	return SUCCEEDED(hr) ? effectOwn : DROPEFFECT_NONE;
}

/// @see	IDataObject::DUnadvise
inline STDMETHODIMP TextDataObject::DUnadvise(DWORD dwConnection) {
	return OLE_E_ADVISENOTSUPPORTED;
}

/// @see	IDataObject::EnumDAdvise
inline STDMETHODIMP TextDataObject::EnumDAdvise(LPENUMSTATDATA* ppenumAdvise) {
	return OLE_E_ADVISENOTSUPPORTED;
}

/// @see	IDataObject::EnumFormatEtc
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

/// @see	IDataObject::GetCanonicalFormatEtc
inline STDMETHODIMP TextDataObject::GetCanonicalFormatEtc(LPFORMATETC pformatectIn, LPFORMATETC pformatetcOut) {
	return DATA_S_SAMEFORMATETC;
}

/// @see	IDataObject::GetData
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

	// 要求された形式のデータがまだ無い場合
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

/// @see	IDataObject::GetDataHere
inline STDMETHODIMP TextDataObject::GetDataHere(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium) {
	return E_NOTIMPL;
}

/// @see	IDataObject::QueryGetData
inline STDMETHODIMP TextDataObject::QueryGetData(LPFORMATETC pformatetc) {
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
 *	現在セットされている文字列を取得するのに利用可能なクリップボード形式の設定
 *	@param formats	クリップボード形式の集合
 */
inline void TextDataObject::setAvailableFormatSet(const std::set<CLIPFORMAT>& formats) throw() {clipFormats_ = formats;}

/// @see	IDataObject::SetData
inline STDMETHODIMP TextDataObject::SetData(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium, BOOL fRelease) {return E_NOTIMPL;}

/**
 *	ANSI 文字列を設定
 *	@param lpszText	セット設定するテキスト
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
 *	Unicode 文字列を設定
 *	@param text	設定するテキスト
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


// AvailableFormatsEnumerator class implementation
/////////////////////////////////////////////////////////////////////////////

/// コンストラクタ
inline TextDataObject::AvailableFormatsEnumerator::AvailableFormatsEnumerator(
		const std::set<CLIPFORMAT>& formats) : clipFormats_(&formats) {Reset();}

/// @see	IEnumFORMATETC::Clone
inline STDMETHODIMP TextDataObject::AvailableFormatsEnumerator::Clone(IEnumFORMATETC** ppenum) {return E_NOTIMPL;}

/// @see	IEnumFORMATETC::Next
inline STDMETHODIMP TextDataObject::AvailableFormatsEnumerator::Next(ULONG celt, FORMATETC* rgelt, ULONG* pceltFetched) {
	if(celt > 1 && pceltFetched == 0)
		return E_INVALIDARG;

	ULONG fetched = 0;
	while(fetched < celt && it_ != clipFormats_->end()) {
		rgelt[fetched].cfFormat = *it_;
		rgelt[fetched].ptd = 0;
		rgelt[fetched].dwAspect = DVASPECT_CONTENT;
		rgelt[fetched].lindex = -1;
		rgelt[fetched].tymed = TYMED_HGLOBAL;
		++fetched;
		++it_;
	}
	if(pceltFetched != 0)
		*pceltFetched = fetched;

	return (fetched == celt) ? S_OK : S_FALSE;
}

/// @see	IEnumFORMATETC::Reset
inline STDMETHODIMP TextDataObject::AvailableFormatsEnumerator::Reset() {it_ = clipFormats_->begin(); return S_OK;}

/// @see	IEnumFORMATETC::Skip
inline STDMETHODIMP TextDataObject::AvailableFormatsEnumerator::Skip(ULONG celt) {
	while(celt != 0 && it_ != clipFormats_->end())
		--celt, ++it_;
	return (celt == 0) ? S_OK : S_FALSE;
}

}}} // namespace manah::com::ole

#endif /* !MANAH_TEXT_DATA_OBJECT_HPP */
