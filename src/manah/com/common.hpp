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

// �}�N���Ƃ� //////////////////////////////////////////////////////////////

#define RETURN_IF_FAILED(hr)	\
	if(FAILED(hr))				\
		return (hr)

#define VERIFY_POINTER(p)	\
	if((p) == 0)			\
		return E_POINTER

inline const BSTR safeBSTR(const BSTR bstr) throw() {return (bstr != 0) ? bstr : OLESTR("");}

inline bool isEmptyBSTR(const BSTR bstr) throw() {return bstr == 0 || *bstr == 0;}

inline VARIANT_BOOL toVariantBoolean(bool b) throw() {return (b != 0) ? VARIANT_TRUE : VARIANT_FALSE;}


/// ComPtr::operator-> ���Ԃ� AddRef �ARelease �Ăяo�����֎~�����v���L�V
template<class T> class ComPtrProxy : public T {
private:
	STDMETHOD_(ULONG, AddRef)() = 0;
	STDMETHOD_(ULONG, Release)() = 0;
	T** operator &() const throw();	// &*p ��h��
};

/// �Öق̌^�ϊ���F�߂�|���V�[
struct AllowConversion {};
/// �Öق̌^�ϊ���F�߂Ȃ��|���V�[
struct DisallowConversion {};

/**
 * COM �X�}�[�g�|�C���^
 * @param T �C���^�[�t�F�C�X�^
 * @param ConversionPolicy �C���^�[�t�F�C�X�^�ւ̈Öق̌^�ϊ����s����
 */
template<class T, class ConversionPolicy = AllowConversion>
class ComPtr {
public:
	/// �C���^�[�t�F�C�X�^
	typedef T Interface;
	/// �R���X�g���N�^
	explicit ComPtr(Interface* p = 0) throw() : pointee_(p) {if(pointee_ != 0) pointee_->AddRef();}
	/// �R�s�[�R���X�g���N�^
	ComPtr(const ComPtr<Interface, ConversionPolicy>& rhs) throw() : pointee_(rhs.pointee_) {if(pointee_ != 0) pointee_->AddRef();}
	/// �f�X�g���N�^
	virtual ~ComPtr() throw() {if(pointee_ != 0) pointee_->Release();}
	/// @c ::CoCreateInstance �ɂ��I�u�W�F�N�g������������
	HRESULT createInstance(REFCLSID clsid, IUnknown* unkOuter = 0, DWORD clsContext = CLSCTX_ALL, REFIID riid = __uuidof(Interface)) {
		assert(isNull()); return ::CoCreateInstance(clsid, unkOuter, clsContext, riid, reinterpret_cast<void**>(&pointee_));}
	/// ���̃|�C���^��Ԃ�
	ComPtrProxy<Interface>* get() const throw() {return static_cast<ComPtrProxy<T>*>(pointee_);}
	/// �������̂��߂̏o�̓|�C���^��Ԃ�
	Interface** initialize() throw() {release(); return &pointee_;}
	/// @p p �Ɠ����I�u�W�F�N�g���ǂ�����Ԃ�
	bool isEqualObject(IUnknown* p) const throw() {
		if(pointee_ == 0 && p == 0)			return true;
		else if(pointee_ == 0 || p == 0)	return false;
		ComPtr<IUnknown> ps[2];
		pointee_->QueryInterface(IID_IUnknown, ps[0].initialize());
		p->QueryInterface(IID_IUnknown, ps[1].initialize());
		return ps[1].get() == ps[2].get();
	}
	/// NULL �|�C���^��?
	bool isNull() const throw() {return pointee_ == 0;}
	/// �|�C���^���J������
	void release() throw() {reset(0);}
	/// �|�C���^��ݒ肷��
	void reset(Interface* p = 0) throw() {if(pointee_ != 0) pointee_->Release(); pointee_ = p; if(p != 0) pointee_->AddRef();}

	/// �A�h���X���Z�q (�������̂��߂̏o�͈����ɂ̂ݎg��)
	T** operator&() throw() {return initialize();}
	/// �����o�A�N�Z�X���Z�q
	ComPtrProxy<T>* operator->() const throw() {assert(!isNull()); return get();}
	/// �t�Q�Ɖ��Z�q
	ComPtrProxy<T>& operator*() const throw() {assert(!isNull()); return *get();}
	/// ������Z�q
	ComPtr<Interface, ConversionPolicy>& operator=(Interface* rhs) throw() {reset(rhs); return *this;}
	/// �������Z�q
	bool operator==(const Interface* rhs) const throw() {return pointee_ == rhs;}
	/// �s�������Z�q
	bool operator!=(const Interface* rhs) const throw() {return !(pointee_ == rhs);}
	/// �Öق̕ϊ����Z�q
	operator Interface*() const throw();
	/// �_���l�ւ̌^�ϊ����Z�q
	operator bool() const throw() {return !isNull();}

private:
	Interface* pointee_;
};

template<class T, typename AllowConversion> inline ComPtr<T, AllowConversion>::operator T*() const throw() {return pointee_;}


/**
 * @c IUnknown#QueryInterface �ł̏�������p�� COM �X�}�[�g�|�C���^
 * @param T �C���^�[�t�F�C�X�^
 * @param iid �C���^�[�t�F�C�X�� IID
 * @param ConversionPolicy �C���^�[�t�F�C�X�^�ւ̈Öق̌^�ϊ����s����
 */
template<class T, const IID* iid = &__uuidof(T), class ConversionPolicy = AllowConversion>
class ComQIPtr : public ComPtr<T, ConversionPolicy> {
public:
	/// �R���X�g���N�^
	explicit ComQIPtr(T* p = 0) : ComPtr<T, ConversionPolicy>(p) {}
	/// �R�s�[�R���X�g���N�^
	ComQIPtr(const ComQIPtr<T, iid, ConversionPolicy>& rhs) : ComPtr<T, ConversionPolicy>(rhs.get()) {}
	/// �������̂��߂̏o�̓|�C���^��Ԃ�
	void** initialize() throw() {release(); return reinterpret_cast<void**>(ComPtr<T, ConversionPolicy>::operator &());}
	/// �A�h���X���Z�q (�������̂��߂� QI �o�͈����ɂ̂ݎg��)
	void** operator &() throw() {return initialize();}
};


/// @c IErrorInfo �� C++ ��O�Ƃ��Ĉ������߂̃��b�p�N���X (�o��: Essential COM (Don Box))
class ComException {
public:
	/**
	 * �R���X�g���N�^
	 * @param scode SCODE
	 * @param riid IID
	 * @param source ���̗�O�𓊂����N���X
	 * @param description ��O�̐����B@c null �̏ꍇ @p scode ���擾
	 * @param helpFile �w���v�t�@�C���̃p�X
	 * @param helpContext �w���v�g�s�b�N�̔ԍ�
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
	/// �f�X�g���N�^
	virtual ~ComException() {if(errorInfo_ != 0) errorInfo_->Release();}

	// ���\�b�h
public:
	/// �G���[�� @c IErrorInfo ��Ԃ�
	void getErrorInfo(IErrorInfo*& errorInfo) const {errorInfo = errorInfo_; errorInfo->AddRef();}
	/// �G���[�� @c HRESULT ��Ԃ�
	HRESULT getSCode() const throw() {return hr_;}
	/// ��O�I�u�W�F�N�g��_���X���b�h��O�Ƃ��ē�����
	void throwLogicalThreadError() {::SetErrorInfo(0, errorInfo_);}
	/**
	 * @c HRESULT �ɑΉ�����G���[���b�Z�[�W��Ԃ�
	 * @param[in] hr HRESULT
	 * @param[out] description �G���[���b�Z�[�W
	 * @param[in] languageId ���� ID
	 */
	static void getDescriptionOfSCode(HRESULT hr, BSTR& description, DWORD languageId = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)) {
		void* buffer = 0;
		FormatMessageW(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			0, hr, languageId, reinterpret_cast<wchar_t*>(&buffer), 0, 0);
		description = ::SysAllocString(reinterpret_cast<OLECHAR*>(buffer));
		::LocalFree(buffer);
	}

	// �f�[�^�����o
private:
	HRESULT hr_;
	IErrorInfo* errorInfo_;
};


/**
 * �N���e�B�J���Z�N�V�����œ����������s���BATL �ƂقƂ�Ǔ���
 * @param automatic �C���X�^���X�̐����A�j�����Ɏ����I�ɃN���e�B�J���Z�N�V�������������A�j�󂷂邩
 */
template<bool automatic = true> class ComCriticalSection {
public:
	/// �R���X�g���N�^
	ComCriticalSection() {if(automatic) if(FAILED(doInitialize())) throw std::runtime_error("Failed to initialize critical section!");}
	/// �f�X�g���N�^
	~ComCriticalSection() {if(automatic) doTerminate();}
	/// ���b�N
	void lock() {::EnterCriticalSection(&cs_);}
	/// ���b�N����
	void unlock() {::LeaveCriticalSection(&cs_);}
	/// �C���X�^���X��������
	HRESULT initialize();
	/// �C���X�^���X�̌㏈��
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
 * @brief @c ISupportErrorInfo �̕W���I�Ȏ���
 *
 * ���̎����͒P��̃C���^�[�t�F�C�X�����T�|�[�g���Ȃ�
 */
template<const IID* iid> class ISupportErrorInfoImpl : virtual public ISupportErrorInfo {
public:
	virtual ~ISupportErrorInfoImpl() {}
	STDMETHODIMP InterfaceSupportsErrorInfo(REFIID riid) {return (riid == *iid) ? S_OK : S_FALSE;}
};


/**
 * @brief @c IObjectSafety �̒P���Ȏ���
 *
 * ���̎����͒P��̃C���^�[�t�F�C�X�����T�|�[�g���Ȃ�
 * @param supportedSafety �T�|�[�g����I�v�V����
 * @param initialSafety �I�v�V�����̏����l
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
