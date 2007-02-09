// common.hpp
// (c) 2002-2007 exeal

#ifndef MANAH_COM_COMMON_HPP
#define MANAH_COM_COMMON_HPP

#include "../object.hpp"
#include <objbase.h>
#include <objsafe.h>
#include <cassert>
#include <stdexcept>


namespace manah {
namespace com {

// マクロとか //////////////////////////////////////////////////////////////

#define RETURN_IF_FAILED(hr)	\
	if(FAILED(hr))				\
		return (hr)

#define VERIFY_POINTER(p)	\
	if((p) == 0)			\
		return E_POINTER

inline const BSTR safeBSTR(const BSTR bstr) throw() {return (bstr != 0) ? bstr : OLESTR("");}

inline bool isEmptyBSTR(const BSTR bstr) throw() {return bstr == 0 || *bstr == 0;}

inline VARIANT_BOOL toVariantBoolean(bool b) throw() {return (b != 0) ? VARIANT_TRUE : VARIANT_FALSE;}


/// ComPtr::operator-> が返す AddRef 、Release 呼び出しを禁止したプロキシ
template<class T> class ComPtrProxy : public T {
private:
	STDMETHOD_(ULONG, AddRef)() = 0;
	STDMETHOD_(ULONG, Release)() = 0;
	T** operator &() const throw();	// &*p を防ぐ
};

/// 暗黙の型変換を認めるポリシー
struct AllowConversion {};
/// 暗黙の型変換を認めないポリシー
struct DisallowConversion {};

/**
 * COM スマートポインタ
 * @param T インターフェイス型
 * @param ConversionPolicy インターフェイス型への暗黙の型変換を行うか
 */
template<class T, class ConversionPolicy = AllowConversion>
class ComPtr {
public:
	/// インターフェイス型
	typedef T Interface;
	/// コンストラクタ
	explicit ComPtr(Interface* p = 0) throw() : pointee_(p) {if(pointee_ != 0) pointee_->AddRef();}
	/// コピーコンストラクタ
	ComPtr(const ComPtr<Interface, ConversionPolicy>& rhs) throw() : pointee_(rhs.pointee_) {if(pointee_ != 0) pointee_->AddRef();}
	/// デストラクタ
	virtual ~ComPtr() throw() {if(pointee_ != 0) pointee_->Release();}
	/// @c ::CoCreateInstance によりオブジェクトを初期化する
	HRESULT createInstance(REFCLSID clsid, IUnknown* unkOuter = 0, DWORD clsContext = CLSCTX_ALL, REFIID riid = __uuidof(Interface)) {
		assert(isNull()); return ::CoCreateInstance(clsid, unkOuter, clsContext, riid, reinterpret_cast<void**>(&pointee_));}
	/// 生のポインタを返す
	ComPtrProxy<Interface>* get() const throw() {return static_cast<ComPtrProxy<T>*>(pointee_);}
	/// 初期化のための出力ポインタを返す
	Interface** initialize() throw() {release(); return &pointee_;}
	/// @p p と同じオブジェクトかどうかを返す
	bool isEqualObject(IUnknown* p) const throw() {
		if(pointee_ == 0 && p == 0)			return true;
		else if(pointee_ == 0 || p == 0)	return false;
		ComPtr<IUnknown> ps[2];
		pointee_->QueryInterface(IID_IUnknown, ps[0].initialize());
		p->QueryInterface(IID_IUnknown, ps[1].initialize());
		return ps[1].get() == ps[2].get();
	}
	/// NULL ポインタか?
	bool isNull() const throw() {return pointee_ == 0;}
	/// ポインタを開放する
	void release() throw() {reset(0);}
	/// ポインタを設定する
	void reset(Interface* p = 0) throw() {if(pointee_ != 0) pointee_->Release(); pointee_ = p; if(p != 0) pointee_->AddRef();}

	/// アドレス演算子 (初期化のための出力引数にのみ使う)
	T** operator&() throw() {return initialize();}
	/// メンバアクセス演算子
	ComPtrProxy<T>* operator->() const throw() {assert(!isNull()); return get();}
	/// 逆参照演算子
	ComPtrProxy<T>& operator*() const throw() {assert(!isNull()); return *get();}
	/// 代入演算子
	ComPtr<Interface, ConversionPolicy>& operator=(Interface* rhs) throw() {reset(rhs); return *this;}
	/// 等価演算子
	bool operator==(const Interface* rhs) const throw() {return pointee_ == rhs;}
	/// 不等価演算子
	bool operator!=(const Interface* rhs) const throw() {return !(pointee_ == rhs);}
	/// 暗黙の変換演算子
	operator Interface*() const throw();
	/// 論理値への型変換演算子
	operator bool() const throw() {return !isNull();}

private:
	Interface* pointee_;
};

template<class T, typename AllowConversion> inline ComPtr<T, AllowConversion>::operator T*() const throw() {return pointee_;}


/**
 * @c IUnknown#QueryInterface での初期化専用の COM スマートポインタ
 * @param T インターフェイス型
 * @param iid インターフェイスの IID
 * @param ConversionPolicy インターフェイス型への暗黙の型変換を行うか
 */
template<class T, const IID* iid = &__uuidof(T), class ConversionPolicy = AllowConversion>
class ComQIPtr : public ComPtr<T, ConversionPolicy> {
public:
	/// コンストラクタ
	explicit ComQIPtr(T* p = 0) : ComPtr<T, ConversionPolicy>(p) {}
	/// コピーコンストラクタ
	ComQIPtr(const ComQIPtr<T, iid, ConversionPolicy>& rhs) : ComPtr<T, ConversionPolicy>(rhs.get()) {}
	/// 初期化のための出力ポインタを返す
	void** initialize() throw() {release(); return reinterpret_cast<void**>(ComPtr<T, ConversionPolicy>::operator &());}
	/// アドレス演算子 (初期化のための QI 出力引数にのみ使う)
	void** operator &() throw() {return initialize();}
};


/// @c IErrorInfo を C++ 例外として扱うためのラッパクラス (出所: Essential COM (Don Box))
class ComException {
public:
	/**
	 * コンストラクタ
	 * @param scode SCODE
	 * @param riid IID
	 * @param source この例外を投げたクラス
	 * @param description 例外の説明。@c null の場合 @p scode より取得
	 * @param helpFile ヘルプファイルのパス
	 * @param helpContext ヘルプトピックの番号
	 */
	ComException(HRESULT scode, REFIID riid,
			const OLECHAR* source, const OLECHAR* description = 0, const OLECHAR* helpFile = 0, DWORD helpContext = 0) {
		ICreateErrorInfo* pcei = 0;

		assert(FAILED(scode));

		HRESULT hr = ::CreateErrorInfo(&pcei);
		assert(SUCCEEDED(hr));

		hr = pcei->SetGUID(riid);
		assert(SUCCEEDED(hr));
		if(source != 0) {
			hr = pcei->SetSource(const_cast<OLECHAR*>(source));
			assert(SUCCEEDED(hr));
		}
		if(description != 0) {
			hr = pcei->SetDescription(const_cast<OLECHAR*>(description));
			assert(SUCCEEDED(hr));
		} else {
			BSTR bstrDescription = 0;
			ComException::getDescriptionOfSCode(scode, bstrDescription);
			hr = pcei->SetDescription(bstrDescription);
			::SysFreeString(bstrDescription);
			assert(SUCCEEDED(hr));
		}
		if(helpFile != 0) {
			hr = pcei->SetHelpFile(const_cast<OLECHAR*>(helpFile));
			assert(SUCCEEDED(hr));
		}
		hr = pcei->SetHelpContext(helpContext);
		assert(SUCCEEDED(hr));

		hr_ = scode;
		hr = pcei->QueryInterface(IID_IErrorInfo, reinterpret_cast<void**>(&errorInfo_));
		assert(SUCCEEDED(hr));
		pcei->Release();
	}
	/// デストラクタ
	virtual ~ComException() {if(errorInfo_ != 0) errorInfo_->Release();}

	// メソッド
public:
	/// エラーの @c IErrorInfo を返す
	void getErrorInfo(IErrorInfo*& errorInfo) const {errorInfo = errorInfo_; errorInfo->AddRef();}
	/// エラーの @c HRESULT を返す
	HRESULT getSCode() const throw() {return hr_;}
	/// 例外オブジェクトを論理スレッド例外として投げる
	void throwLogicalThreadError() {::SetErrorInfo(0, errorInfo_);}
	/**
	 * @c HRESULT に対応するエラーメッセージを返す
	 * @param[in] hr HRESULT
	 * @param[out] description エラーメッセージ
	 * @param[in] languageId 言語 ID
	 */
	static void getDescriptionOfSCode(HRESULT hr, BSTR& description, DWORD languageId = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)) {
		void* buffer = 0;
		FormatMessageW(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			0, hr, languageId, reinterpret_cast<wchar_t*>(&buffer), 0, 0);
		description = ::SysAllocString(reinterpret_cast<OLECHAR*>(buffer));
		::LocalFree(buffer);
	}

	// データメンバ
private:
	HRESULT hr_;
	IErrorInfo* errorInfo_;
};


/**
 * クリティカルセクションで同期処理を行う。ATL とほとんど同じ
 * @param automatic インスタンスの生成、破棄時に自動的にクリティカルセクションを初期化、破壊するか
 */
template<bool automatic = true> class ComCriticalSection {
public:
	/// コンストラクタ
	ComCriticalSection() {if(automatic) if(FAILED(doInitialize())) throw std::runtime_error("Failed to initialize critical section!");}
	/// デストラクタ
	~ComCriticalSection() {if(automatic) doTerminate();}
	/// ロック
	void lock() {::EnterCriticalSection(&cs_);}
	/// ロック解除
	void unlock() {::LeaveCriticalSection(&cs_);}
	/// インスタンスを初期化
	HRESULT initialize();
	/// インスタンスの後処理
	void terminate();

private:
	ComCriticalSection(const ComCriticalSection&);
	ComCriticalSection& operator =(const ComCriticalSection&);
	HRESULT doInitialize() {
		__try {
			::InitializeCriticalSection(&cs_);
		} __except(EXCEPTION_EXECUTE_HANDLER) {
			return (STATUS_NO_MEMORY == ::GetExceptionCode()) ? E_OUTOFMEMORY : E_FAIL;
		}
		return S_OK;
	}
	void doTerminate() {::DeleteCriticalSection(&cs_);}
private:
	CRITICAL_SECTION cs_;
};

template<> inline HRESULT ComCriticalSection<false>::initialize() {return doInitialize();}

template<> inline void ComCriticalSection<false>::terminate() {doTerminate();}


/**
 * @brief @c ISupportErrorInfo の標準的な実装
 *
 * この実装は単一のインターフェイスしかサポートしない
 */
template<const IID* iid> class ISupportErrorInfoImpl : virtual public ISupportErrorInfo {
public:
	virtual ~ISupportErrorInfoImpl() {}
	STDMETHODIMP InterfaceSupportsErrorInfo(REFIID riid) {return (riid == *iid) ? S_OK : S_FALSE;}
};


/**
 * @brief @c IObjectSafety の単純な実装
 *
 * この実装は単一のインターフェイスしかサポートしない
 * @param supportedSafety サポートするオプション
 * @param initialSafety オプションの初期値
 */
template<DWORD supportedSafety, DWORD initialSafety> class IObjectSafetyImpl : virtual public IObjectSafety {
public:
	IObjectSafetyImpl() : enabledSafety_(supportedSafety & initialSafety) {}
	virtual ~IObjectSafetyImpl() {}
public:
	STDMETHODIMP GetInterfaceSafetyOptions(REFIID riid, DWORD* pdwSupportedOptions, DWORD* pdwEnabledOptions) {
		VERIFY_POINTER(pdwSupportedOptions);
		VERIFY_POINTER(pdwEnabledOptions);
		ComQIPtr<IUnknown> p;
		if(SUCCEEDED(QueryInterface(riid, &p))) {
			*pdwSupportedOptions = supportedSafety;
			*pdwEnabledOptions = enabledSafety_;
			return S_OK;
		} else {
			*pdwSupportedOptions = *pdwEnabledOptions = 0;
			return E_NOINTERFACE;
		}
	}
	STDMETHODIMP SetInterfaceSafetyOptions(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions) {
		ComQIPtr<IUnknown> p;
		if(FAILED(QueryInterface(riid, &p)))
			return E_NOINTERFACE;
		else if(toBoolean(dwOptionSetMask & ~supportedSafety))
			return E_FAIL;
		enabledSafety_ = (enabledSafety_ & ~dwOptionSetMask) | (dwOptionSetMask & dwEnabledOptions);
		return S_OK;
	}
protected:
	DWORD	getSafetyOptions() const throw() {return enabledSafety_;}
	void	setSafetyOptions(DWORD options) throw() {enabledSafety_ = (options & supportedSafety);}
private:
	DWORD enabledSafety_;
};

}} // namespace manah::com

#endif /* !MANAH_COM_COMMON_HPP */
