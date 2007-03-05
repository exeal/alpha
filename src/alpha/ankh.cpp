/**
 * @file ankh.cpp
 * @author exeal
 * @date 2006-2007
 */

#include "stdafx.h"
#include "ankh.hpp"
#include "ankh-idl_i.c"
#include "application.hpp"
#include "select-language-dialog.hpp"
#include "ascension/encoder.hpp"
#include "../manah/com/ole-type-wrapper.hpp"	// ComBSTR
#include <sstream>
using namespace alpha::ankh;
using namespace manah::win32;
using namespace manah::com;
using namespace std;

namespace {
	/// �g�b�v���x���I�u�W�F�N�g�̎��ʎq
	const OLECHAR TOP_LEVEL_OBJECT_NAME[] = OLESTR("Ankh");
	/// �X�N���v�g�z�X�g�I�u�W�F�N�g�̎��ʎq
	const OLECHAR HOST_OBJECT_NAME[] = OLESTR("WScript");
	/// �X�N���v�g�z�X�g�I�u�W�F�N�g�̎��ʎq (�Z�k�`)
	const OLECHAR HOST_OBJECT_SHORT_NAME[] = OLESTR("WSH");
	/// �u�I�[�g���[�V�����I�u�W�F�N�g���쐬�ł��܂���v
	const HRESULT ANKH_E_CANNOTCREATEAUTOMATION = 0x800A01AD;
	/// �u�N���X�̓I�[�g���[�V�������T�|�[�g���Ă��܂���v
	const HRESULT ANKH_E_AUTOMATIONUNCOMPATIBLECLASS = 0x800A01AE;

	/// �I�[�g���[�V�������ʎq�̔�r
	inline int compareAutomationName(const OLECHAR* lhs, size_t cchLhs, const OLECHAR* rhs, size_t cchRhs) {
		return ::CompareStringW(LOCALE_NEUTRAL,
				NORM_IGNORECASE | NORM_IGNOREKANATYPE | NORM_IGNORENONSPACE | NORM_IGNOREWIDTH,
				lhs, static_cast<DWORD>(cchLhs), rhs, static_cast<DWORD>(cchRhs));
	}

	/// �C�x���g�V���N
	class AdhocEventSinkBase : virtual public IDispatchEx {
	public:
		// �R���X�g���N�^
		explicit AdhocEventSinkBase(const IID& eventIID) throw();
		virtual ~AdhocEventSinkBase() throw();
		// ���\�b�h
		HRESULT			connect(IConnectionPoint& eventSource);
		HRESULT			disconnect();
		static HRESULT	findSourceConnectionPoint(IDispatch& source,
							manah::com::ComPtr<IConnectionPoint>& connectionPoint, const CLSID& coclassID = CLSID_NULL);
		// IUnknown
		IMPLEMENT_UNKNOWN_MULTI_THREADED()
		STDMETHODIMP	QueryInterface(REFIID riid, void** ppvObject);
		// IDispatch
		STDMETHODIMP	GetTypeInfoCount(UINT* pcTypeInfo);
		STDMETHODIMP	GetTypeInfo(UINT iTypeInfo, LCID lcid, ITypeInfo** ppTypeInfo);
		STDMETHODIMP	GetIDsOfNames(REFIID riid,
							OLECHAR** rgszNames, unsigned int cNames, LCID lcid, DISPID* rgDispId);
		STDMETHODIMP	Invoke(DISPID dispidMember, REFIID riid, LCID lcid,
							WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult,
							EXCEPINFO* pExcepInfo, unsigned int* puArgErr);
		// IDispatchEx
		STDMETHODIMP	GetDispID(BSTR bstrName, DWORD grfdex, DISPID* pid);
		STDMETHODIMP	InvokeEx(DISPID id, LCID lcid, WORD wFlags,
							DISPPARAMS* pdp, VARIANT* pvarRes, EXCEPINFO* pei, IServiceProvider* pspCaller);
		STDMETHODIMP	DeleteMemberByName(BSTR bstrName, DWORD grfdex);
		STDMETHODIMP	DeleteMemberByDispID(DISPID id);
		STDMETHODIMP	GetMemberProperties(DISPID id, DWORD grfdexFetch, DWORD* pgrfdex);
		STDMETHODIMP	GetMemberName(DISPID id, BSTR* pbstrName);
		STDMETHODIMP	GetNextDispID(DWORD grfdex, DISPID id, DISPID* pid);
		STDMETHODIMP	GetNameSpaceParent(IUnknown** ppunk);
	protected:
		virtual HRESULT	fireEvent(const OLECHAR* name, LCID locale, WORD flags,
			DISPPARAMS* params, VARIANT* result, EXCEPINFO* exception, IServiceProvider* serviceProvider) = 0;
	private:
		ComPtr<IConnectionPoint> eventSource_;
		const IID eventIID_;
		DWORD cookie_;
		map<DISPID, basic_string<OLECHAR> > eventIDTable_;
	};
} // namespace @0

/// WScript ���C�x���g�ڑ�����������̂Ɏg���A�h�z�b�N�ȃC�x���g�V���N�I�u�W�F�N�g
class ScriptHost::LegacyAdhocEventSink : public AdhocEventSinkBase {
public:
	// �R���X�g���N�^
	LegacyAdhocEventSink(IActiveScript& scriptEngine, const IID& eventIID, const std::wstring& prefix);
protected:
	HRESULT	fireEvent(const OLECHAR* name, LCID locale, WORD flags,
		DISPPARAMS* params, VARIANT* result, EXCEPINFO* exception, IServiceProvider* serviceProvider);
private:
	ComPtr<IActiveScript>		scriptEngine_;
	const basic_string<OLECHAR>	prefix_;	// �C�x���g�n���h�����̐ړ���
};

/// @c ScriptHost#ConnectObjectEx ����������̂Ɏg���C�x���g�V���N�I�u�W�F�N�g
class ScriptHost::AdhocEventSink : public AdhocEventSinkBase {
public:
	// �R���X�g���N�^
	AdhocEventSink(IDispatch& eventSink, const IID& eventIID);
protected:
	HRESULT	fireEvent(const OLECHAR* name, LCID locale, WORD flags,
		DISPPARAMS* params, VARIANT* result, EXCEPINFO* exception, IServiceProvider* serviceProvider);
private:
	ComPtr<IDispatch> sink_;
};

namespace {
	/// �񋓂̑��Ȏ���
	class AutomationEnumeration :
			virtual public IDispatchEx,
			public IObjectSafetyImpl<INTERFACESAFE_FOR_UNTRUSTED_CALLER, INTERFACESAFE_FOR_UNTRUSTED_CALLER> {
	public:
		// �R���X�g���N�^
		AutomationEnumeration();
		// ���\�b�h
		void	addProperty(const OLECHAR* name, long value);
		// IUnknown
		IMPLEMENT_UNKNOWN_MULTI_THREADED()
		BEGIN_INTERFACE_TABLE()
			IMPLEMENTS_LEFTMOST_INTERFACE(IDispatchEx)
			IMPLEMENTS_INTERFACE(IDispatch)
			IMPLEMENTS_INTERFACE(IObjectSafety)
		END_INTERFACE_TABLE()
		// IDispatch
		STDMETHODIMP	GetTypeInfoCount(UINT* pcTypeInfo);
		STDMETHODIMP	GetTypeInfo(UINT iTypeInfo, LCID lcid, ITypeInfo** ppTypeInfo);
		STDMETHODIMP	GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId);
		STDMETHODIMP	Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
							DISPPARAMS *pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr);
		// IDispatchEx
		STDMETHODIMP	GetDispID(BSTR bstrName, DWORD grfdex, DISPID* pid);
		STDMETHODIMP	InvokeEx(DISPID id, LCID lcid, WORD wFlags,
							DISPPARAMS* pdp, VARIANT* pvarRes, EXCEPINFO* pei, IServiceProvider* pspCaller);
		STDMETHODIMP	DeleteMemberByName(BSTR bstrName, DWORD grfdex);
		STDMETHODIMP	DeleteMemberByDispID(DISPID id);
		STDMETHODIMP	GetMemberProperties(DISPID id, DWORD grfdexFetch, DWORD* pgrfdex);
		STDMETHODIMP	GetMemberName(DISPID id, BSTR* pbstrName);
		STDMETHODIMP	GetNextDispID(DWORD grfdex, DISPID id, DISPID* pid);
		STDMETHODIMP	GetNameSpaceParent(IUnknown** ppunk);
	private:
		typedef map<basic_string<OLECHAR>, DISPID, AutomationNameComparison> NameToIDTable;
		typedef map<DISPID, long> IDToValueTable;
		enum {INITIAL_ID = 100};
		NameToIDTable nameTable_;
		IDToValueTable idTable_;
		DISPID nextID_;
	};
} // namespace @0


// Arguments ////////////////////////////////////////////////////////////////

/// �R���X�g���N�^
Arguments::Arguments(const vector<wstring>& arguments) : arguments_(arguments) {
}

/// @see IArguments#Count
STDMETHODIMP Arguments::Count(long* count) {
	return (count != 0) ? get_length(count) : S_OK;
}

/// @see IArguments#get__NewEnum
STDMETHODIMP Arguments::get__NewEnum(IUnknown** enumerator) {
	VERIFY_POINTER(enumerator);
	*enumerator = 0;
	return E_NOTIMPL;
}

/// @see IArguments#get_Item
STDMETHODIMP Arguments::get_Item(long index, VARIANT** value) {
	VERIFY_POINTER(value);
	if(index < 0 || index >= static_cast<long>(arguments_.size()))
		return 0x800A0009;
	if(*value = static_cast<VARIANT*>(::CoTaskMemAlloc(sizeof(VARIANT)))) {
		(*value)->vt = VT_BSTR;
		(*value)->bstrVal = ::SysAllocString(arguments_[index].c_str());
		if((*value)->bstrVal == 0) {
			::CoTaskMemFree(*value);
			*value = 0;
		}
	}
	return (*value != 0) ? S_OK : E_OUTOFMEMORY;
}

/// @see IArguments#get_length
STDMETHODIMP Arguments::get_length(long* count) {
	VERIFY_POINTER(count);
	*count = static_cast<long>(arguments_.size());
	return S_OK;
}

/// @see IArguments#get_Named
STDMETHODIMP Arguments::get_Named(INamedArguments** named) {
	VERIFY_POINTER(named);
	*named = 0;
	return E_NOTIMPL;
}

/// @see IArguments#get_Unnamed
STDMETHODIMP Arguments::get_Unnamed(IUnnamedArguments** unnamed) {
	VERIFY_POINTER(unnamed);
	*unnamed = 0;
	return E_NOTIMPL;
}

/// @see IArguments#ShowUsage
STDMETHODIMP Arguments::ShowUsage() {
	return E_NOTIMPL;
}


// ScriptHost ///////////////////////////////////////////////////////////////

const wchar_t* ScriptHost::NAME = L"Ankh Script Host";
const unsigned short ScriptHost::MAJOR_VERSION = 0;
const unsigned short ScriptHost::MINOR_VERSION = 7;
const unsigned short ScriptHost::BUILD_NUMBER = 0;

/**
 *	�R���X�g���N�^
 *	@param scriptSystem		�X�N���v�g�V�X�e��
 *	@param scriptEngine		�z�X�g����X�N���v�g�G���W��
 *	@param engineLanguage	@a scriptEngine �̌���
 *	@param ownerWindow		�A�v���P�[�V�����E�B���h�E
 */
ScriptHost::ScriptHost(ScriptSystem& scriptSystem, IActiveScript& scriptEngine, HWND ownerWindow /* = 0 */)
		: scriptSystem_(scriptSystem), scriptEngine_(scriptEngine), ownerWindow_(ownerWindow), lastScriptCookie_(0), timeout_(-1) {
	if(ownerWindow != 0 && !toBoolean(::IsWindow(ownerWindow)))
		throw invalid_argument("invalid window handle.");
	scriptEngine_.AddRef();

	ComQIPtr<IObjectSafety> safety;
	if(SUCCEEDED(scriptEngine.QueryInterface(IID_IObjectSafety, &safety))) {
		DWORD supportedOptions, enabledOptions;
		if(SUCCEEDED(safety->GetInterfaceSafetyOptions(IID_IActiveScript, &supportedOptions, &enabledOptions)))
			safety->SetInterfaceSafetyOptions(IID_IActiveScript,
				supportedOptions, INTERFACESAFE_FOR_UNTRUSTED_DATA | INTERFACE_USES_SECURITY_MANAGER);
	}
	scriptEngine_.SetScriptSite(this);

	ComQIPtr<IActiveScriptParse> parser;
	scriptEngine_.QueryInterface(IID_IActiveScriptParse, &parser);
	parser->InitNew();
	scriptEngine_.AddNamedItem(HOST_OBJECT_NAME, SCRIPTITEM_ISVISIBLE);
	scriptEngine_.AddNamedItem(HOST_OBJECT_SHORT_NAME, SCRIPTITEM_ISVISIBLE);
}

/// �f�X�g���N�^
ScriptHost::~ScriptHost() {
	for(LegacyEventConnections::iterator i = legacyEventConnections_.begin(); i != legacyEventConnections_.end(); ++i)
		delete i->second;
	for(EventConnections::iterator i = eventConnections_.begin(); i != eventConnections_.end(); ++i)
		delete i->second;
	scriptEngine_.Close();
	scriptEngine_.Release();
}

/// �X�N���v�g�G���W���� @c IActiveScript#Close ���Ăяo��
HRESULT ScriptHost::closeEngine() throw() {
	return scriptEngine_.Close();
}

/// @see IScriptHost#ConnectObject
STDMETHODIMP ScriptHost::ConnectObject(IDispatch* eventSource, BSTR prefix) {
	if(eventSource == 0 || isEmptyBSTR(prefix))
		return E_INVALIDARG;
	const HRESULT hr = connectObject(*eventSource, prefix);
	if(FAILED(hr)) {
		ComException e(hr, IID_IScriptHost, OLESTR("Ankh.ScriptHost.ConnectObject"));
		e.throwLogicalThreadError();
		return e.getSCode();
	}
	return S_OK;
}

/**
 * WScript ���̃C�x���g�ڑ����s��
 * @param source �C�x���g�\�[�X
 * @param prefix �C�x���g��������v���V�W�����̐ړ���
 * @param coclassID �C�x���g�C���^�[�t�F�C�X�̓���Ɏg�� coclass �� GUID
 * @return ����
 */
HRESULT ScriptHost::connectObject(IDispatch& source, const BSTR prefix, const CLSID& coclassID /* = CLSID_NULL */) {
	if(isEmptyBSTR(prefix))
		return E_INVALIDARG;

	HRESULT hr;
	ComPtr<IConnectionPoint> connectionPoint;
	if(FAILED(hr = AdhocEventSinkBase::findSourceConnectionPoint(source, connectionPoint, coclassID)))
		return hr;
	IID eventIID;
	if(FAILED(hr = connectionPoint->GetConnectionInterface(&eventIID)))
		return hr;

	// �ڑ�����
	if(LegacyAdhocEventSink* sink = new(nothrow) LegacyAdhocEventSink(scriptEngine_, eventIID, prefix)) {
		sink->AddRef();
		if(SUCCEEDED(hr = sink->connect(*connectionPoint))) {
			legacyEventConnections_.insert(make_pair(&source, sink));
			source.AddRef();
		} else
			sink->Release();
		return hr;
	} else
		return E_OUTOFMEMORY;
}

/**
 * �C�x���g�ڑ����s��
 * @param source �C�x���g�\�[�X
 * @param sink �C�x���g�V���N
 * @param coclassID �C�x���g�C���^�[�t�F�C�X�̓���Ɏg�� coclass �� GUID
 * @return ����
 */
HRESULT ScriptHost::connectObject(IDispatch& source, IDispatch& sink, const CLSID& coclassID /* = CLSID_NULL */) {
	HRESULT hr;
	ComPtr<IConnectionPoint> connectionPoint;
	if(FAILED(hr = AdhocEventSinkBase::findSourceConnectionPoint(source, connectionPoint, coclassID)))
		return hr;
	IID eventIID;
	if(FAILED(hr = connectionPoint->GetConnectionInterface(&eventIID)))
		return hr;

	// �ڑ�����
	if(AdhocEventSink* newConnection = new(nothrow) AdhocEventSink(sink, eventIID)) {
		if(FAILED(hr = newConnection->connect(*connectionPoint))) {
			delete newConnection;
			return hr;
		}
		eventConnections_.insert(make_pair(&source, newConnection));
		return S_OK;
	} else
		return E_OUTOFMEMORY;
}

/// @see IScriptHost#ConnectObjectEx
STDMETHODIMP ScriptHost::ConnectObjectEx(IDispatch* eventSource, IDispatch* eventSink) {
	if(eventSource == 0 || eventSink == 0)
		return E_INVALIDARG;
	const HRESULT hr = connectObject(*eventSource, *eventSink);
	if(FAILED(hr)) {
		ComException e(hr, IID_IScriptHost, OLESTR("Ankh.ScriptHost.ConnectObjectEx"));
		e.throwLogicalThreadError();
		return e.getSCode();
	}
	return S_OK;
}

/// @see IScriptHost#CreateObject
STDMETHODIMP ScriptHost::CreateObject(BSTR progID, BSTR prefix, IDispatch** newObject) {
	if(newObject == 0)
		return S_OK;
	*newObject = 0;
	if(isEmptyBSTR(progID))
		return E_INVALIDARG;

	HRESULT hr;
	CLSID clsid;
	try {
		if(FAILED(hr = ::CLSIDFromProgID(progID, &clsid)))
			throw ComException(hr, IID_IScriptHost, OLESTR("Ankh.ScriptHost.CreateObject"));
		else if(URLPOLICY_ALLOW != verifyObjectCreation(clsid))
			throw ComException(ANKH_E_CANNOTCREATEAUTOMATION, IID_IScriptHost, OLESTR("Ankh.ScriptHost.CreateObject"));
		ComQIPtr<IUnknown> temp;
		if(FAILED(hr = ::CoCreateInstance(clsid, 0, CLSCTX_ALL, IID_IUnknown, &temp)))
			throw ComException(hr, IID_IScriptHost, OLESTR("Ankh.ScriptHost.CreateObject"));
		else if(FAILED(temp->QueryInterface(IID_IDispatch, reinterpret_cast<void**>(newObject))))
			throw ComException(ANKH_E_AUTOMATIONUNCOMPATIBLECLASS, IID_IScriptHost, OLESTR("Ankh.ScriptHost.CreateObject"));
		else if(URLPOLICY_ALLOW != verifyObjectRunning(**newObject, clsid))
			throw ComException(ANKH_E_CANNOTCREATEAUTOMATION, IID_IScriptHost, OLESTR("Ankh.ScriptHost.CreateObject"));
		else if(!isEmptyBSTR(prefix) && FAILED(hr = connectObject(**newObject, prefix, clsid)))
			throw ComException(hr, IID_IScriptHost, OLESTR("Ankh.ScriptHost.CreateObject"));
	} catch(ComException& e) {
		if(*newObject != 0) {
			(*newObject)->Release();
			*newObject = 0;
		}
		e.throwLogicalThreadError();
		return e.getSCode();
	}
	return S_OK;
}

/// @see IScriptHost#DisconnectObject
STDMETHODIMP ScriptHost::DisconnectObject(IDispatch* eventSource) {
	if(eventSource == 0)
		return E_INVALIDARG;
	try {
		const LegacyEventConnections::iterator i = legacyEventConnections_.find(eventSource);
		if(i == legacyEventConnections_.end())
			throw ComException(E_INVALIDARG, IID_IScriptHost, OLESTR("Ankh.ScriptHost.DisconnectObject"));
		HRESULT hr;
		if(FAILED(hr = i->second->disconnect()))
			throw ComException(hr, IID_IScriptHost, OLESTR("Ankh.ScriptHost.DisconnectObject"));
		i->first->Release();
		i->second->Release();
		legacyEventConnections_.erase(i);
	} catch(ComException& e) {
		e.throwLogicalThreadError();
		return e.getSCode();
	}
	return S_OK;
}

/// @see IScriptHost#DisconnectObjectEx
STDMETHODIMP ScriptHost::DisconnectObjectEx(IDispatch* eventSource, IDispatch* eventSink) {
	if(eventSource == 0 || eventSink == 0)
		return E_INVALIDARG;
	const pair<EventConnections::iterator, EventConnections::iterator> p = eventConnections_.equal_range(eventSource);
	for(EventConnections::iterator i = p.first; i != p.second; ++i) {
		ComQIPtr<IDispatch> sink;
		if(FAILED(i->second->QueryInterface(IID_IDispatch, &sink)))
			continue;
		if(sink.get() == eventSink) {
			HRESULT hr;
			if(FAILED(hr = i->second->disconnect())) {
				ComException e(hr, IID_IScriptHost, OLESTR("Ankh.ScriptHost.DisconnectObjectEx"));
				e.throwLogicalThreadError();
				return e.getSCode();
			}
			delete i->second;
			eventConnections_.erase(i);
			return S_OK;
		}
	}
	return E_INVALIDARG;
}

/// @see IScriptHost#Echo
STDMETHODIMP ScriptHost::Echo(SAFEARRAY* arguments) {
	if(arguments == 0 || ::SafeArrayGetDim(arguments) != 1)
		return E_INVALIDARG;
	else if(!scriptSystem_.isInteractive() || !toBoolean(::IsWindow(ownerWindow_)))
		return S_OK;

	wostringstream ss;
	long argumentCount;
	VARIANTARG* args;
	VARIANTARG arg;
	::SafeArrayGetUBound(arguments, 1, &argumentCount);
	++argumentCount;
	::SafeArrayAccessData(arguments, reinterpret_cast<void**>(&args));
	for(long i = 0; i < argumentCount; ++i) {
		::VariantInit(&arg);
		if(FAILED(::VariantChangeType(&arg, &args[i], 0, VT_BSTR)))
			return DISP_E_TYPEMISMATCH;
		ss << safeBSTR(arg.bstrVal);
		::VariantClear(&arg);
		if(i != argumentCount - 1)
			ss << L" ";
	}
	::SafeArrayUnaccessData(arguments);
	::MessageBoxW(ownerWindow_, ss.str().c_str(), ScriptHost::NAME, MB_OK);
	return S_OK;
}

/// @see IActiveScriptSiteWindow#EnableModeless
STDMETHODIMP ScriptHost::EnableModeless(BOOL fEnable) {
	return toBoolean(fEnable) ? E_FAIL : S_OK;
}

/// @see IScriptHost#get_Application
STDMETHODIMP ScriptHost::get_Application(IDispatch** application) {
	VERIFY_POINTER(application);
	(*application = this)->AddRef();
	return S_OK;
}

/// @see IScriptHost#get_Arguments
STDMETHODIMP ScriptHost::get_Arguments(IArguments** arguments) {
	VERIFY_POINTER(arguments);
	*arguments = 0;
	return E_NOTIMPL;
}

/// @see IScriptHost#get_BuildVersion
STDMETHODIMP ScriptHost::get_BuildVersion(int* version) {
	VERIFY_POINTER(version);
	*version = BUILD_NUMBER;
	return S_OK;
}

/// @see IScriptHost#get_FullName
STDMETHODIMP ScriptHost::get_FullName(BSTR* name) {
	VERIFY_POINTER(name);
	WCHAR pathName[MAX_PATH];
	::GetModuleFileNameW(0, pathName, MAX_PATH);
	*name = ::SysAllocString(pathName);
	return (*name != 0) ? S_OK : E_OUTOFMEMORY;
}

/// @see IScriptHost#get_Interactive
STDMETHODIMP ScriptHost::get_Interactive(VARIANT_BOOL* interactive) {
	VERIFY_POINTER(interactive);
	*interactive = toVariantBoolean(scriptSystem_.isInteractive());
	return S_OK;
}

/// @see IScriptHost#get_Name
STDMETHODIMP ScriptHost::get_Name(BSTR* name) {
	VERIFY_POINTER(name);
	*name = ::SysAllocString(ScriptHost::NAME);
	return (*name != 0) ? S_OK : E_OUTOFMEMORY;
}

/// @see IScriptHost#get_Path
STDMETHODIMP ScriptHost::get_Path(BSTR* name) {
	VERIFY_POINTER(name);
	WCHAR pathName[MAX_PATH];
	::GetModuleFileNameW(0, pathName, MAX_PATH);
	*name = ::SysAllocStringLen(pathName, static_cast<UINT>(::PathFindFileNameW(pathName) - pathName) - 1);
	return (*name != 0) ? S_OK : E_OUTOFMEMORY;
}

/// @see IScriptHost#get_ScriptFullName
STDMETHODIMP ScriptHost::get_ScriptFullName(BSTR* name) {
	VERIFY_POINTER(name);
	*name = 0;
	return E_NOTIMPL;
}

/// @see IScriptHost#get_ScriptName
STDMETHODIMP ScriptHost::get_ScriptName(BSTR* name) {
	VERIFY_POINTER(name);
	*name = 0;
	return E_NOTIMPL;
}

/// @see IScriptHost#get_StdErr
STDMETHODIMP ScriptHost::get_StdErr(IDispatch** stdErr) {
	VERIFY_POINTER(stdErr);
	return E_NOTIMPL;
}

/// @see IScriptHost#get_StdIn
STDMETHODIMP ScriptHost::get_StdIn(IDispatch** stdIn) {
	VERIFY_POINTER(stdIn);
	return E_NOTIMPL;
}

/// @see IScriptHost#get_StdOut
STDMETHODIMP ScriptHost::get_StdOut(IDispatch** stdOut) {
	VERIFY_POINTER(stdOut);
	return E_NOTIMPL;
}

/// @see IScriptHost#get_Timeout
STDMETHODIMP ScriptHost::get_Timeout(long* timeout) {
	VERIFY_POINTER(timeout);
	*timeout = timeout_;
	return S_OK;
}

/// @see IScriptHost#get_Version
STDMETHODIMP ScriptHost::get_Version(BSTR* version) {
	VERIFY_POINTER(version);
	wchar_t buffer[10];
	swprintf(buffer, L"%u.%u", MAJOR_VERSION, MINOR_VERSION);
	*version = ::SysAllocString(buffer);
	return (*version != 0) ? S_OK : E_OUTOFMEMORY;
}

/// @see IActiveScriptSite#GetDocVersionString
STDMETHODIMP ScriptHost::GetDocVersionString(BSTR* pbstrVersion) {
	VERIFY_POINTER(pbstrVersion);
	*pbstrVersion = 0;
	return E_NOTIMPL;
}

/// @see IActiveScriptSite#GetItemInfo
STDMETHODIMP ScriptHost::GetItemInfo(LPCOLESTR pstrName, DWORD dwReturnMask, IUnknown** ppiunkItem, ITypeInfo** ppti) {
	if(pstrName == 0)
		return E_INVALIDARG;

	ComPtr<IDispatch> object;
	if(compareAutomationName(pstrName, wcslen(pstrName), HOST_OBJECT_NAME, countof(HOST_OBJECT_NAME) - 1) == CSTR_EQUAL
			|| compareAutomationName(pstrName, wcslen(pstrName), HOST_OBJECT_SHORT_NAME, countof(HOST_OBJECT_SHORT_NAME) - 1) == CSTR_EQUAL)
		object = this;
	else if(!scriptSystem_.getTopLevelObject(pstrName, object)) {
		if(toBoolean(dwReturnMask & SCRIPTINFO_IUNKNOWN)) {
			VERIFY_POINTER(ppiunkItem);
			*ppiunkItem = 0;
		}
		if(toBoolean(dwReturnMask & SCRIPTINFO_ITYPEINFO)) {
			VERIFY_POINTER(ppti);
			*ppti = 0;
		}
		return TYPE_E_ELEMENTNOTFOUND;
	}
	if(toBoolean(dwReturnMask & SCRIPTINFO_IUNKNOWN)) {
		VERIFY_POINTER(ppiunkItem);
		(*ppiunkItem = object)->AddRef();
	}
	if(toBoolean(dwReturnMask & SCRIPTINFO_ITYPEINFO)) {
		VERIFY_POINTER(ppti);
		object->GetTypeInfo(0, 0, ppti);
	}
	return S_OK;
}

/// @see IActiveScriptSite#GetLCID
STDMETHODIMP ScriptHost::GetLCID(LCID* plcid) {
	VERIFY_POINTER(plcid);
	*plcid = LOCALE_INVARIANT;
	return S_OK;
}

/// @see IScriptHost#GetObject
STDMETHODIMP ScriptHost::GetObject(BSTR pathName, BSTR progID, BSTR prefix, IDispatch** newObject) {
	if(newObject == 0)
		return S_OK;
	*newObject = 0;

	struct InternalException : public ComException {
		InternalException(HRESULT hr) : ComException(hr, IID_IScriptHost, OLESTR("Ankh.ScriptHost.GetObject")) {}};
	struct CannotCreateAutomationException : public InternalException {
		CannotCreateAutomationException() : InternalException(ANKH_E_CANNOTCREATEAUTOMATION) {}};
	struct AutomationUncompatibleClassException : public InternalException {
		AutomationUncompatibleClassException() : InternalException(ANKH_E_AUTOMATIONUNCOMPATIBLECLASS) {}};

	HRESULT hr;
	try {
		if(!isEmptyBSTR(progID)) {
			// ProgID ���^����ꂽ�ꍇ�� pathName �̓t�@�C���� -> �t�@�C�����j�J���g��
			CLSID clsid;
			if(FAILED(hr = ::CLSIDFromProgID(progID, &clsid)))
				throw InternalException(hr);
			else if(URLPOLICY_ALLOW != verifyObjectCreation(clsid))
				throw CannotCreateAutomationException();

			if(!isEmptyBSTR(pathName)) {
				ComPtr<IMoniker> fileMoniker;
				if(FAILED(hr = ::CreateFileMoniker(pathName, &fileMoniker)))
					throw InternalException(hr);
				ComPtr<IBindCtx> bc;
				if(FAILED(hr = ::CreateBindCtx(0, &bc)) || (FAILED(hr = fileMoniker->IsRunning(bc, 0, 0))))
					throw InternalException(hr);
				if(hr == S_OK) {	// �I�u�W�F�N�g�͊��ɋN�����Ă���
					ComQIPtr<IUnknown> temp;
					if(FAILED(hr = fileMoniker->BindToObject(bc, 0, IID_IUnknown, &temp)))
						throw InternalException(hr);
					else if(FAILED(temp->QueryInterface(IID_IDispatch, reinterpret_cast<void**>(newObject))))
						throw AutomationUncompatibleClassException();
				} else {			// �܂��N�����Ă��Ȃ��ꍇ�͐V�����쐬
					ComQIPtr<IUnknown> temp;
					if(SUCCEEDED(hr = ::CoCreateInstance(clsid, 0, CLSCTX_ALL, IID_IUnknown, &temp))) {
						if(FAILED(temp->QueryInterface(IID_IDispatch, reinterpret_cast<void**>(newObject))))
							throw AutomationUncompatibleClassException();
						ComQIPtr<IPersistFile> file;
						if(FAILED((*newObject)->QueryInterface(IID_IPersistFile, &file)))
							return E_INVALIDARG;
						hr = file->Load(pathName, STGM_READ);
					}
				}
			} else {	// CLSID �����^����ꂽ�ꍇ�� CreateObject �Ɠ���
				ComQIPtr<IUnknown> temp;
				if(SUCCEEDED(hr = ::CoCreateInstance(clsid, 0, CLSCTX_ALL, IID_IUnknown, &temp))) {
					if(FAILED(temp->QueryInterface(IID_IDispatch, reinterpret_cast<void**>(newObject))))
						throw AutomationUncompatibleClassException();
				}
			}

			if(FAILED(hr))
				throw InternalException(hr);
			else if(URLPOLICY_ALLOW != verifyObjectRunning(**newObject, clsid))
				throw CannotCreateAutomationException();
			else if(!isEmptyBSTR(prefix) && FAILED(hr = connectObject(**newObject, prefix, clsid)))
				throw InternalException(hr);
		} else {
			// ProgID ���^�����Ă��Ȃ��ꍇ�� pathName ��ʏ�̕\�����Ƃ��Ĉ���
			// (CLSID �����񂩂�ACLSID �x�[�X�̃Z�L�����e�B�l�S�V�G�[�V�������ł���...)
			if(isEmptyBSTR(pathName))
				return E_INVALIDARG;
			ComQIPtr<IUnknown> temp;
			if(FAILED(hr = ::CoGetObject(pathName, 0, IID_IUnknown, &temp)))
				throw InternalException(hr);
			else if(FAILED(temp->QueryInterface(IID_IDispatch, reinterpret_cast<void**>(newObject))))
				throw AutomationUncompatibleClassException();
			else if(!isEmptyBSTR(prefix) && FAILED(hr = connectObject(**newObject, prefix)))
				throw InternalException(hr);
		}
	} catch(ComException& e) {
		if(*newObject != 0) {
			(*newObject)->Release();
			*newObject = 0;
		}
		e.throwLogicalThreadError();
		return e.getSCode();
	}
	return S_OK;
}

/// �X�N���v�g�G���W����Ԃ�
ComPtr<IActiveScript> ScriptHost::getScriptEngine() const throw() {
	return ComPtr<IActiveScript>(&scriptEngine_);
}

/// @see IActiveScriptSiteWindow#GetWindow
STDMETHODIMP ScriptHost::GetWindow(HWND* phwnd) {
	VERIFY_POINTER(phwnd);
	*phwnd = ownerWindow_;
	return S_OK;
}

/**
 * �X�N���v�g�̍ŏ�ʃI�u�W�F�N�g�ɑ΂��ČĂяo����������
 * @param name ���O
 * @param type �Ăяo���^�C�v�B@c IDispatch#Invoke �Ɠ���
 * @param locale ���P�[�����ʎq
 * @param[in,out] params �����B�s�v�ȏꍇ�� @c null
 * @param[out] result �߂�l�B�s�v�ȏꍇ�� @c null
 * @param[out] exception ��O�B�s�v�ȏꍇ�� @c null
 * @return ���ۂȂ�
 */
HRESULT ScriptHost::invokeTopLevelEntity(const OLECHAR* name, WORD type,
		LCID locale /* = LOCALE_USER_DEFAULT */, DISPPARAMS* params /* = 0 */, VARIANT* result /* = 0 */, EXCEPINFO* exception /* = 0 */) {
	if(name == 0)
		return E_INVALIDARG;
	ComPtr<IDispatch> topLevel;
	HRESULT hr;
	if(FAILED(hr = scriptEngine_.GetScriptDispatch(0, &topLevel)))
		return hr;
	DISPID id;
	if(FAILED(hr = topLevel->GetIDsOfNames(IID_NULL, const_cast<OLECHAR**>(&name), 1, locale, &id)))
		return hr;
	DISPPARAMS emptyParams = {0, 0, 0, 0};
	unsigned int invalidArgument;
	return topLevel->Invoke(id, IID_NULL, locale, type, (params != 0) ? params : &emptyParams, result, exception, &invalidArgument);
}

/**
 * �X�N���v�g���t�@�C������ǂݍ���ŕ]������
 * @param fileName �t�@�C�����B���ɓǂݍ��܂�Ă���ꍇ�͉������Ȃ�
 * @return ����
 */
bool ScriptHost::loadScript(const WCHAR* fileName) {
	assert(fileName != 0);

	HANDLE file = ::CreateFileW(fileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
						0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN, 0);
	if(file == INVALID_HANDLE_VALUE)
		return false;

	OLECHAR* source = 0;
	const DWORD fileSize = ::GetFileSize(file, 0);
	if(fileSize != 0) {
		// �t�@�C���̃}�b�s���O
		HANDLE mappedFile = ::CreateFileMapping(file, 0, PAGE_READONLY, 0, 0, 0);
		if(mappedFile == 0) {
			::CloseHandle(file);
			return false;
		}
		const uchar* const buffer = static_cast<const uchar*>(::MapViewOfFile(mappedFile, FILE_MAP_READ, 0, 0, 0));
		source = static_cast<OLECHAR*>(::HeapAlloc(::GetProcessHeap(), HEAP_NO_SERIALIZE, sizeof(wchar_t) * (fileSize + 1)));
		if(buffer == 0 || source == 0) {
			::CloseHandle(mappedFile);
			::CloseHandle(file);
			return false;
		}

		// UTF-8 ���� UTF-16 �ɕϊ�
		auto_ptr<ascension::encodings::Encoder> encoder(ascension::encodings::EncoderFactory::getInstance().createEncoder(CP_UTF8));
		const size_t len = encoder->toUnicode(source, fileSize, buffer, fileSize);
		source[len] = 0;
		::UnmapViewOfFile(buffer);
		::CloseHandle(mappedFile);
		::CloseHandle(file);
	}

	// �]��
	AutoZero<::EXCEPINFO> exception;
	ComQIPtr<IActiveScriptParse> parser;
	if(FAILED(scriptEngine_.QueryInterface(IID_IActiveScriptParse, &parser))) {
		::HeapFree(::GetProcessHeap(), HEAP_NO_SERIALIZE, source);
		return false;
	}
	parser->ParseScriptText((fileSize != 0) ? source : OLESTR(""),
		0, 0, 0, static_cast<DWORD>(++lastScriptCookie_), 0, SCRIPTTEXT_ISVISIBLE, 0, &exception);
	if(fileSize != 0)
		::HeapFree(::GetProcessHeap(), HEAP_NO_SERIALIZE, source);
	scriptEngine_.SetScriptState(SCRIPTSTATE_CONNECTED);

	// �ǂݍ��񂾃X�N���v�g�t�@�C�����L�^����
	loadedScripts_.insert(make_pair(lastScriptCookie_, fileName));
	return true;
}

/// @see IActiveScriptSite#OnEnterScript
STDMETHODIMP ScriptHost::OnEnterScript() {
	return S_OK;
}

/// @see IActiveScriptSite#OnLeaveScript
STDMETHODIMP ScriptHost::OnLeaveScript() {
	return S_OK;
}

/// @see IActiveScriptSite#OnScriptError
STDMETHODIMP ScriptHost::OnScriptError(IActiveScriptError* pscripterror) {
	if(pscripterror == 0)
		return E_INVALIDARG;
	else if(!scriptSystem_.isInteractive())
		return S_OK;	// ����

	AutoZero<::EXCEPINFO> exception;	// Python �ł̓[���N���A�K�{
	pscripterror->GetExceptionInfo(&exception);
	if(exception.scode == S_OK)	// �G���[�ł͂Ȃ�
		return S_OK;

	// �G���[���b�Z�[�W���o��
	Alpha& app = Alpha::getInstance();
	DWORD srcContext;
	unsigned long line;
	long column;
	pscripterror->GetSourcePosition(&srcContext, &line, &column);
	map<DWORD_PTR, basic_string<WCHAR> >::const_iterator i = loadedScripts_.find(srcContext);
	app.messageBox(MSG_SCRIPT__SCRIPT_ERROR_DIALOG, MB_ICONHAND, MARGS
		% ((i != loadedScripts_.end()) ? i->second.c_str() : app.loadString(MSG_OTHER__UNKNOWN))
		% (line + 1) % (column + 1)
		% ((exception.bstrDescription != 0) ? exception.bstrDescription : app.loadString(MSG_OTHER__UNKNOWN))
		% exception.scode % ((exception.bstrSource != 0) ? exception.bstrSource : app.loadString(MSG_OTHER__UNKNOWN)));
	::SysFreeString(exception.bstrSource);
	::SysFreeString(exception.bstrDescription);
	::SysFreeString(exception.bstrHelpFile);

	return S_OK;
}

/// @see IActiveScriptSite#OnScriptTerminate
STDMETHODIMP ScriptHost::OnScriptTerminate(const VARIANT*, const EXCEPINFO*) {
	return S_OK;
}

/// @see IActiveScriptSite#OnStateChange
STDMETHODIMP ScriptHost::OnStateChange(SCRIPTSTATE) {
	return S_OK;
}

/// @see IScriptHost#put_Interactive
STDMETHODIMP ScriptHost::put_Interactive(VARIANT_BOOL interactive) {
	scriptSystem_.setInteractive(toBoolean(interactive));
	return S_OK;
}

/// @see IScriptHost#put_Timeout
STDMETHODIMP ScriptHost::put_Timeout(long timeout) {
	if(timeout < 0)
		return E_INVALIDARG;
	timeout_ = timeout;
	return S_OK;
}

/// @see IActiveScriptSite#QueryContinue
STDMETHODIMP ScriptHost::QueryContinue() {
	return S_OK;
}

/// @see IServiceProvider3QueryService
STDMETHODIMP ScriptHost::QueryService(REFGUID guidService, REFIID riid, void** ppvObject) {
	VERIFY_POINTER(ppvObject);
	if(guidService == SID_SInternetHostSecurityManager)
		return scriptSystem_.QueryInterface(riid, ppvObject);
	*ppvObject = 0;
	return E_NOINTERFACE /* SVC_E_UNKNOWNSERVICE */;
}

/// @see IScriptHost#Quit
STDMETHODIMP ScriptHost::Quit(int) {
	return E_NOTIMPL;
}

/// @see IScriptHost#Sleep
STDMETHODIMP ScriptHost::Sleep(long time) {
	if(time < 0)
		return E_INVALIDARG;
	::Sleep(time);
	return S_OK;
}

/**
 * ActiveX �I�u�W�F�N�g�̍쐬���\���z�X�g�ɖ₢���킹��
 * @param clsid �쐬���悤�Ƃ��Ă���I�u�W�F�N�g�� CLSID
 * @return �|���V�[
 */
DWORD ScriptHost::verifyObjectCreation(const CLSID& clsid) {
	ComQIPtr<IObjectSafety> safety;
	if(FAILED(scriptEngine_.QueryInterface(IID_IObjectSafety, &safety)))
		return URLPOLICY_DISALLOW;

	DWORD supportedOptions, enabledOptions;
	if(FAILED(safety->GetInterfaceSafetyOptions(IID_IActiveScript, &supportedOptions, &enabledOptions)))
		return URLPOLICY_DISALLOW;
	if(toBoolean(enabledOptions & INTERFACE_USES_SECURITY_MANAGER)) {
		DWORD policy;
		if(FAILED(scriptSystem_.ProcessUrlAction(URLACTION_ACTIVEX_RUN,
				reinterpret_cast<BYTE*>(&policy), sizeof(DWORD), reinterpret_cast<BYTE*>(const_cast<CLSID*>(&clsid)), sizeof(CLSID), 0, 0)))
			return URLPOLICY_DISALLOW;
		return GetUrlPolicyPermissions(policy);
	} else
		return toBoolean(enabledOptions & INTERFACESAFE_FOR_UNTRUSTED_CALLER) ? URLPOLICY_DISALLOW : URLPOLICY_ALLOW;
}

/**
 * ActiveX �I�u�W�F�N�g�̍쐬���\���z�X�g�ɖ₢���킹��
 * @param object �I�u�W�F�N�g
 * @param clsid �I�u�W�F�N�g�� CLSID
 * @return �|���V�[
 */
DWORD ScriptHost::verifyObjectRunning(IDispatch& object, const CLSID& clsid) {
	ComQIPtr<IObjectSafety> safety;
	if(FAILED(scriptEngine_.QueryInterface(IID_IObjectSafety, &safety)))
		return URLPOLICY_DISALLOW;

	DWORD supportedOptions, enabledOptions;
	if(FAILED(safety->GetInterfaceSafetyOptions(IID_IDispatch, &supportedOptions, &enabledOptions)))
		return URLPOLICY_DISALLOW;
/*	if(toBoolean(enabledOptions & INTERFACE_USES_SECURITY_MANAGER)) {
		DWORD policy, cb;
		DWORD* customPolicy;
		CONFIRMSAFETY cs = {clsid, &object, 0};
		cs.pUnk->AddRef();
		if(FAILED(scriptSystem_.QueryCustomPolicy(GUID_CUSTOM_CONFIRMOBJECTSAFETY,
				reinterpret_cast<BYTE**>(&customPolicy), &cb, reinterpret_cast<BYTE*>(&cs), sizeof(CONFIRMSAFETY), 0)))
			return URLPOLICY_DISALLOW;
		policy = URLPOLICY_DISALLOW;
		if(customPolicy != 0) {
			if(cb >= sizeof(DWORD))
				policy = *customPolicy;
			::CoTaskMemFree(customPolicy);
		}
		return policy;
	} else
		return toBoolean(enabledOptions & INTERFACESAFE_FOR_UNTRUSTED_CALLER) ? URLPOLICY_DISALLOW : URLPOLICY_ALLOW;*/
	return URLPOLICY_ALLOW;
}


// FileBoundScriptHost //////////////////////////////////////////////////////

FileBoundScriptHost::FileBoundScriptHost(
		const WCHAR* fileName, ScriptSystem& scriptSystem, IActiveScript& scriptEngine, HWND ownerWindow /* = 0 */) :
		ScriptHost(scriptSystem, scriptEngine, ownerWindow) {
	WCHAR buffer[MAX_PATH];
	::PathCanonicalizeW(buffer, fileName);
	fileName_.assign(buffer);
}

/// @see IScriptHost#get_ScriptFullName
STDMETHODIMP FileBoundScriptHost::get_ScriptFullName(BSTR* name) {
	VERIFY_POINTER(name);
	*name = ::SysAllocString(fileName_.c_str());
	return (*name != 0) ? S_OK : E_OUTOFMEMORY;
}

/// @see IScriptHost#get_ScriptName
STDMETHODIMP FileBoundScriptHost::get_ScriptName(BSTR* name) {
	VERIFY_POINTER(name);
	*name = ::SysAllocString(::PathFindFileNameW(fileName_.c_str()));
	return (*name != 0) ? S_OK : E_OUTOFMEMORY;
}

/// @see IScriptHost#Quit
STDMETHODIMP FileBoundScriptHost::Quit(int) {
	::EXCEPINFO exception;
	memset(&exception, 0, sizeof(::EXCEPINFO));
	return getScriptEngine()->InterruptScriptThread(SCRIPTTHREADID_ALL, &exception, 0);
}


// ScriptSystem /////////////////////////////////////////////////////////////

/// �R���X�g���N�^
ScriptSystem::ScriptSystem() : globalNamespace_(0/*new Namespace(L"", 0)*/),
		interactive_(true), securityLevel_(0), crossEngineTopLevelAccessesEnabled_(false) {
}

/// �f�X�g���N�^
ScriptSystem::~ScriptSystem() {
	shutdown();
	for(EngineAssociationMap::const_iterator i = engineAssociations_.begin(); i != engineAssociations_.end(); ++i)
		delete *i;
}

/**
 * �t�@�C�����p�^�[���ƃX�N���v�g�G���W���̑Ή���ǉ�����
 * @param filePattern �t�@�C�����p�^�[��
 * @param engineID �X�N���v�g�G���W���� CLSID
 */
void ScriptSystem::addEngineScriptNameAssociation(const WCHAR* filePattern, const CLSID& engineID) {
	engineAssociations_.insert(new EngineAssociation(filePattern, engineID));
}

/**
 * �ŏ�ʃI�u�W�F�N�g�̒ǉ�
 * @param name �I�u�W�F�N�g�̖��O
 * @param object �I�u�W�F�N�g
 * @throw std::invalid_argument ���ɓ����̃I�u�W�F�N�g�����݂����ꍇ�X���[
 */
void ScriptSystem::addTopLevelObject(const OLECHAR* name, IDispatch& object) {
	assert(name != 0);
	if(topLevelObjects_.find(name) != topLevelObjects_.end())
		throw invalid_argument("There is already same name object.");
	topLevelObjects_.insert(make_pair(name, &object));
	object.AddRef();
	for(ScriptHosts::const_iterator i = scriptHosts_.begin(); i != scriptHosts_.end(); ++i)
		i->second->getScriptEngine()->AddNamedItem(name, SCRIPTITEM_ISVISIBLE);
}

/**
 * �t�@�C��������X�N���v�g�G���W������肷��
 * @param fileName �t�@�C����
 * @param[out] clsid �X�N���v�g�G���W���� CLSID
 * @return ����ɐ��������ꍇ�� true
 */
bool ScriptSystem::associateEngine(const WCHAR* fileName, CLSID& clsid) const {
	assert(fileName != 0);
	// �������Ǘ����Ă���֘A�t�����X�g������
	for(std::set<EngineAssociation*>::const_iterator i = engineAssociations_.begin(); i != engineAssociations_.end(); ++i) {
		if(toBoolean(::PathMatchSpecW(fileName, (*i)->filePattern.c_str())))
			return (clsid = (*i)->clsid), true;
	}
	// �ʖڂȏꍇ�̓��W�X�g������T�� (�����ȕ��@)
	if(associateEngineFromRegistry(fileName, clsid))
		return true;
	// ����ł��ʖڂȏꍇ�̓G���h���[�U�ɐu���Ă݂� (�Θb���[�h�̏ꍇ)
	if(interactive_) {
		alpha::ui::SelectLanguageDialog dialog(fileName);
		if(IDOK == dialog.doModal(Alpha::getInstance().getMainWindow().get()))
			return SUCCEEDED(::CLSIDFromProgID(dialog.getSelectedLanguage().c_str(), &clsid));
	}
	return false;
}

/**
 * �t�@�C���̊g���q����X�N���v�g�G���W������肷�郆�[�e�B���e�B
 * @param fileName �t�@�C����
 * @param[out] clsid �X�N���v�g�G���W���� CLSID
 * @return ����ɐ��������ꍇ�� true
 */
bool ScriptSystem::associateEngineFromRegistry(const WCHAR* fileName, CLSID& clsid) {
	assert(fileName != 0);
	clsid = CLSID_NULL;

	// �Ⴆ�� JScript ����:
	// HKCR\.JS\@="JSFile" -> HKCR\JSFile\ScriptEngine\@="JScript" -> "JScript" �ƂȂ�

	const WCHAR* const extension = ::PathFindExtensionW(fileName);
	if(extension == 0)
		return false;

	HKEY key;
	if(ERROR_SUCCESS != ::RegOpenKeyExW(HKEY_CLASSES_ROOT, extension, 0, KEY_READ, &key))
		return false;

	wchar_t fileType[100], progID[100], scriptEngine[100];
	DWORD dataSize = sizeof(fileType);
	long err = ::RegQueryValueExW(key, 0, 0, 0, reinterpret_cast<BYTE*>(fileType), &dataSize);
	::RegCloseKey(key);
	if(err != ERROR_SUCCESS)
		return false;

	wcscpy(scriptEngine, fileType);
	wcscat(scriptEngine, L"\\ScriptEngine");
	err = ::RegOpenKeyExW(HKEY_CLASSES_ROOT, scriptEngine, 0, KEY_READ, &key);
	dataSize = sizeof(progID);
	if(err == ERROR_SUCCESS) {
		err = ::RegQueryValueExW(key, 0, 0, 0, reinterpret_cast<BYTE*>(progID), &dataSize);
		::RegCloseKey(key);
	} /*else {
		::RegCloseKey(key);
		// �L�[���� "ScriptFile" ��t���čĒ��� (PerlScript �Ƃ�)
		wcscpy(scriptEngine, fileType);
		wcscat(scriptEngine, L"ScriptFile\\ScriptEngine");
		if(ERROR_SUCCESS != ::RegOpenKeyExW(HKEY_CLASSES_ROOT, scriptEngine, 0, KEY_READ, &key))
			return false;
		err = ::RegQueryValueExW(key, 0, 0, 0, reinterpret_cast<BYTE*>(progID), &dataSize);
		::RegCloseKey(key);
	}
*/	if(err != ERROR_SUCCESS)
		return false;
	return SUCCEEDED(::CLSIDFromProgID(progID, &clsid));
}

/**
 * �����֐����Ăяo��
 * @param function �֐�
 * @param locale ���P�[�����ʎq
 * @param[in,out] params �������B�s�v�ȏꍇ�� @c null
 * @param[out] result �߂�l�B�s�v�ȏꍇ�� @c null
 * @param[out] exception ��O�B�s�v�ȏꍇ�� @c null
 * @return @c IDispatch#Invoke �̖߂�l
 */
HRESULT ScriptSystem::callAnonymousFunction(IDispatch& function, LCID locale /* = LOCALE_USER_DEFAULT */,
		DISPPARAMS* params /* = 0 */, VARIANT* result /* = 0 */, EXCEPINFO* exception /* = 0 */) {
	DISPPARAMS emptyParams = {0, 0, 0, 0};
	return function.Invoke(DISPID_VALUE, IID_NULL, locale,
		DISPATCH_METHOD, (params != 0) ? params : &emptyParams, result, exception, 0);
}

/// 
void ScriptSystem::enableCrossEngineTopLevelAccesses(bool enable /* = true */) {
	crossEngineTopLevelAccessesEnabled_ = enable;
}

/// @see IScriptSystem#ExecuteScript
STDMETHODIMP ScriptSystem::ExecuteScript(BSTR fileName) {
	if(fileName == 0)
		return E_INVALIDARG;

	CLSID engineID;
	if(!associateEngine(fileName, engineID))
		return E_FAIL;	// TODO: ���[�U�֒ʒm

	WCHAR filePath[MAX_PATH];
	resolveScriptFileName(fileName, filePath);

	ComPtr<ScriptHost> scriptHost;
	const HRESULT hr = launchNewEngine(engineID, scriptHost, false);
	return SUCCEEDED(hr) ? (scriptHost->loadScript(filePath) ? S_OK : E_FAIL) : hr;
}

/// @see IScriptSystem#get_SecurityLevel
STDMETHODIMP ScriptSystem::get_SecurityLevel(short* level) {
	VERIFY_POINTER(level);
	*level = 0;
	return S_OK;
}

/// �O���[�o���Ȗ��O��Ԃ�Ԃ�
Namespace& ScriptSystem::getGlobalNamespace() const {
	return *globalNamespace_;
}

/// @see IInternetHostSecurityManager#GetSecurityId
STDMETHODIMP ScriptSystem::GetSecurityId(BYTE* pbSecurityId, DWORD* pcbSecurityId, DWORD_PTR dwReserved) {
	pbSecurityId = 0;
	pcbSecurityId = 0;
	return (dwReserved == 0) ? E_NOTIMPL : E_INVALIDARG;
}

/**
 * �ŏ�ʃI�u�W�F�N�g�̎擾
 * @param name �I�u�W�F�N�g�̖��O
 * @param[out] object �I�u�W�F�N�g
 * @return ���������ꍇ true
 */
bool ScriptSystem::getTopLevelObject(const OLECHAR* name, ComPtr<IDispatch>& object) const throw() {
	assert(name != 0);
	if(compareAutomationName(name, wcslen(name), TOP_LEVEL_OBJECT_NAME, countof(TOP_LEVEL_OBJECT_NAME) - 1) == CSTR_EQUAL)
		return (object = const_cast<ScriptSystem*>(this)), true;
	MemberTable::iterator i = const_cast<ScriptSystem*>(this)->topLevelObjects_.find(name);
	return (i != topLevelObjects_.end()) ? (object = i->second), true : false;
}

/**
 * �ŏ�ʃI�u�W�F�N�g�ɑ΂��ČĂяo����������B@c ScriptSystem ���g�ւ̌Ăяo���͕s��
 * @param name ���O
 * @param preferedLanguage �D�悵�ČĂяo���������錾��B@c CLSID_NULL ���Ɠ��ɖ���
 * @param type �Ăяo���^�C�v�B@c IDispatch::Invoke �Ɠ���
 * @param locale ���P�[�����ʎq
 * @param[in,out] params  �����B�s�v�ȏꍇ�� @c null
 * @param[out] result �߂�l�B�s�v�ȏꍇ�� @c null
 * @param[out] exception ��O�B�s�v�ȏꍇ�� @c null
 * @return ���ۂȂ�
 */
HRESULT ScriptSystem::invokeTopLevelEntity(const OLECHAR* name, const CLSID& preferedLanguage, WORD type,
		LCID locale /* = LOCALE_USER_DEFAULT */, DISPPARAMS* params /* = 0 */, VARIANT* result /* = 0 */, EXCEPINFO* exception /* = 0 */) {
	if(name == 0)
		return E_INVALIDARG;
	else if(type == DISPATCH_PROPERTYGET) {
		ComPtr<IDispatch> object;
		if(getTopLevelObject(name, object)) {
			VERIFY_POINTER(result);
			if(params != 0 && params->cArgs != 0)
				return DISP_E_BADPARAMCOUNT;
			result->vt = VT_DISPATCH;
			(result->pdispVal = object)->AddRef();
			return S_OK;
		}
	}
	if(preferedLanguage != CLSID_NULL) {
		ScriptHosts::iterator i = scriptHosts_.find(preferedLanguage);
		if(i != scriptHosts_.end()) {
			const HRESULT hr = i->second->invokeTopLevelEntity(name, type, locale, params, result, exception);
			if(hr != DISP_E_UNKNOWNNAME && hr != DISP_E_MEMBERNOTFOUND)
				return hr;
		}
	}
	for(ScriptHosts::iterator i = scriptHosts_.begin(); i != scriptHosts_.end(); ++i) {
		if(preferedLanguage == i->first)
			continue;
		const HRESULT hr = i->second->invokeTopLevelEntity(name, type, locale, params, result, exception);
		if(hr != DISP_E_UNKNOWNNAME && hr != DISP_E_MEMBERNOTFOUND)
			return hr;
	}
	return DISP_E_MEMBERNOTFOUND;
}

/// @see IScriptSystem#IsScriptFileLoaded
STDMETHODIMP ScriptSystem::IsScriptFileLoaded(BSTR fileName, VARIANT_BOOL* loaded) {
	if(fileName == 0 || fileName[0] == 0)
		return E_INVALIDARG;
	else if(loaded == 0)
		return S_OK;
	return E_NOTIMPL;
}

/**
 * �V�����X�N���v�g�G���W�����N������
 * @param engineID �X�N���v�g�G���W���� CLSID
 * @param[out] newHost �N�������G���W���̃z�X�g
 * @param addToList �X�N���v�g�z�X�g���Ǘ����X�g�Ɋ܂߂邩
 * @return ����
 */
HRESULT ScriptSystem::launchNewEngine(const CLSID& engineID, ComPtr<ScriptHost>& newHost, bool addToList) {
	HRESULT hr;
	ComPtr<IActiveScript> newEngine;
	if(FAILED(hr = newEngine.createInstance(engineID, 0, CLSCTX_INPROC)))
		return hr;
	if(ScriptHost* host = new(nothrow) ScriptHost(*this, *newEngine, Alpha::getInstance().getMainWindow().get())) {
		host->AddRef();
		newHost.reset(host);
		// �Ǘ����X�g�Ɋ܂߂�
		if(addToList) {
			scriptHosts_.insert(make_pair(engineID, newHost));
			host->AddRef();
		}
		host->Release();
		// �g�b�v���x���̓���
		newEngine->AddNamedItem(TOP_LEVEL_OBJECT_NAME, /*SCRIPTITEM_GLOBALMEMBERS |*/ SCRIPTITEM_ISVISIBLE);
		for(MemberTable::iterator i = topLevelObjects_.begin(); i != topLevelObjects_.end(); ++i)
			newEngine->AddNamedItem(i->first.c_str(), SCRIPTITEM_ISVISIBLE);
		return S_OK;
	} else
		return E_OUTOFMEMORY;
}

/// @see IScriptSystem#LoadConstants
STDMETHODIMP ScriptSystem::LoadConstants(VARIANT* libraryNameOrObject, BSTR itemName) {
	if(libraryNameOrObject == 0)
		return E_INVALIDARG;
	else if(libraryNameOrObject->vt == VT_BSTR) {
		HRESULT hr;
		ComPtr<ITypeLib> typeLib;
		if(FAILED(hr = ::LoadTypeLib(libraryNameOrObject->bstrVal, &typeLib)))
			return hr;
		return loadConstants(*typeLib) ? S_OK : E_FAIL;
	} else if(libraryNameOrObject->vt == VT_DISPATCH) {
		if(libraryNameOrObject->pdispVal == 0)
			return E_INVALIDARG;
		HRESULT hr;
		UINT typeCount;
		if(FAILED(hr = libraryNameOrObject->pdispVal->GetTypeInfoCount(&typeCount)))
			return hr;
		else if(typeCount == 0)
			return S_OK;
		ComPtr<ITypeInfo> typeInfo;
		if(FAILED(hr = libraryNameOrObject->pdispVal->GetTypeInfo(0, LOCALE_USER_DEFAULT, &typeInfo)))
			return hr;
		ComPtr<ITypeLib> typeLib;
		UINT index;
		if(FAILED(hr = typeInfo->GetContainingTypeLib(&typeLib, &index)))
			return hr;
		return loadConstants(*typeLib) ? S_OK : E_FAIL;
	} else
		return DISP_E_TYPEMISMATCH;
}

/**
 * �^���C�u��������� (�萔) ��ǂݍ���
 * @param typeLibrary �^���C�u����
 * @param guid �ǂݍ��ޗ񋓂� @c GUID�B�ȗ�����ƃ��C�u�����̑S�Ă̗񋓂�ǂݍ���
 * @return ����
 */
bool ScriptSystem::loadConstants(ITypeLib& typeLibrary, const GUID& guid /* = GUID_NULL */) {
	const UINT typeInfoCount = typeLibrary.GetTypeInfoCount();
	bool loadedOnce = false;

	for(UINT i = 0; i < typeInfoCount; ++i) {
		ComPtr<ITypeInfo> typeInfo;
		TYPEATTR* typeAttr;
		if(FAILED(typeLibrary.GetTypeInfo(i, &typeInfo)))
			continue;
		else if(FAILED(typeInfo->GetTypeAttr(&typeAttr)))
			continue;
		else if((guid != GUID_NULL && typeAttr->guid != guid) || typeAttr->typekind != TKIND_ENUM) {
			typeInfo->ReleaseTypeAttr(typeAttr);
			continue;
		}

		ComPtr<AutomationEnumeration> enumerator(new AutomationEnumeration);
		bool addedOnce = false;
		for(WORD j = 0; j < typeAttr->cVars; ++j) {
			VARDESC* varDesc;
			if(FAILED(typeInfo->GetVarDesc(j, &varDesc)))
				continue;
			else if(varDesc->varkind != VAR_CONST
					|| toBoolean(varDesc->wVarFlags & VARFLAG_FHIDDEN)
					|| toBoolean(varDesc->wVarFlags & VARFLAG_FNONBROWSABLE)
					|| toBoolean(varDesc->wVarFlags & VARFLAG_FRESTRICTED)) {
				typeInfo->ReleaseVarDesc(varDesc);
				continue;
			}

			UINT fetchedCount;
			BSTR constantName;
			if(SUCCEEDED(typeInfo->GetNames(varDesc->memid, &constantName, 1, &fetchedCount))) {
				bool conflicted = false;
				try {
					enumerator->addProperty(constantName, varDesc->lpvarValue->lVal);
				} catch(invalid_argument& ) {	// ����
					conflicted = true;
				}
				::SysFreeString(constantName);
				if(!conflicted)
					addedOnce = true;
			}
			typeInfo->ReleaseVarDesc(varDesc);
		}
		typeInfo->ReleaseTypeAttr(typeAttr);

		if(addedOnce) {
			BSTR enumName;
			if(SUCCEEDED(typeLibrary.GetDocumentation(i, &enumName, 0, 0, 0))) {
				bool conflicted = false;
				try {
					addTopLevelObject(enumName, *enumerator);
				} catch(invalid_argument&) {
					conflicted = true;
				}
				::SysFreeString(enumName);
				if(!conflicted)
					loadedOnce = true;
			}
		}
	}
	return guid == GUID_NULL || loadedOnce;
}

/// @see IScriptSystem#LoadScript
STDMETHODIMP ScriptSystem::LoadScript(BSTR fileName) {
	if(fileName == 0)
		return E_INVALIDARG;

	CLSID engineID;
	if(!associateEngine(fileName, engineID))
		return E_FAIL;	// TODO: ���[�U�֒ʒm
	WCHAR filePath[MAX_PATH];
	if(!resolveScriptFileName(fileName, filePath))
		return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_FILE_NOT_FOUND);
	map<CLSID, ScriptHost*, CLSIDComparison>::iterator i = scriptHosts_.find(engineID);
	if(i == scriptHosts_.end()) {
		ComPtr<ScriptHost> host;
		const HRESULT hr = launchNewEngine(engineID, host, true);
		return SUCCEEDED(hr) ? (host->loadScript(filePath) ? S_OK : E_FAIL) : hr;
	}
	return i->second->loadScript(filePath) ? S_OK : E_FAIL;
}

/// @see IInternetHostSecurityManager#ProcessUrlAction
STDMETHODIMP ScriptSystem::ProcessUrlAction(DWORD dwAction,
		BYTE* pPolicy, DWORD cbPolicy, BYTE* pContext, DWORD cbContext, DWORD dwFlags, DWORD dwReserved) {
	VERIFY_POINTER(pPolicy);
	memset(pPolicy, 0, cbPolicy);
	if(dwReserved != 0)
		return (*pPolicy = URLPOLICY_DISALLOW), E_INVALIDARG;

	// Java ���ۂ��BASR �� URLACTION_JAVA_CURR_MAX ���g���悤�� (Mar. 2004)
	if(dwAction >= URLACTION_JAVA_MIN && dwAction <= URLACTION_JAVA_MAX && cbPolicy >= sizeof(DWORD))
		return (*reinterpret_cast<DWORD*>(pPolicy) = URLPOLICY_JAVA_MEDIUM), S_FALSE;
//	else if(dwAction != URLACTION_ACTIVEX_RUN)	// Active X �I�u�W�F�N�g�̍쐬�ȊO�͖���
		return (*pPolicy = URLPOLICY_ALLOW), S_OK;
#if 0
	// �ȉ��A�Ώۂ� Active X �I�u�W�F�N�g�̂�
	ScriptSiteSecurityLevel securityLevel = securityLevelForUnsafeObject_;
	ComPtr<ICatInformation> pci;

	// �K�p����Z�L�����e�B���x�������肷��̂Ɏ���\���ɂ��I�u�W�F�N�g�����S���ǂ����𒲂ׂ�B
	// �R���|�[�l���g�J�e�S�� "Controls that are safely scripable" ���g��
	if(cbContext == sizeof(CLSID) && SUCCEEDED(pci.createInstance(CLSID_StdComponentCategoriesMgr))) {
		CATID id = CATID_SafeForScripting;
		if(S_OK == pci->IsClassOfCategories(*reinterpret_cast<CLSID*>(pContext), 1, &id, -1, 0))
			securityLevel = securityLevelForSafeObject_;
	}

	// �ȉ��A�Ώۂ͈��S�łȂ��\���̂��� Active X �I�u�W�F�N�g�̂�
	memset(pPolicy, 0, cbPolicy);

	if(securityLevel <= SSSL_ALLOW) {
		*pPolicy = URLPOLICY_ALLOW;
		return S_OK;
	} else if(securityLevel == SSSL_QUERYUSER
			&& toBoolean(::IsWindow(ownerWindow_))
			&& !toBoolean(dwFlags & PUAF_NOUI)) {
		// �x���_�C�A���O���o��
		AlphaApp&	app = AlphaApp::getInstance();
		UINT		dialogType = MB_YESNO | MB_ICONEXCLAMATION | MB_DEFBUTTON2;
		OLECHAR*	progID = 0;
		OLECHAR*	clsid = 0;

		if(toBoolean(dwFlags & PUAF_FORCEUI_FOREGROUND))
			dialogType |= MB_SETFOREGROUND;

		if(cbContext == sizeof(CLSID)) {
			::ProgIDFromCLSID(*reinterpret_cast<CLSID*>(pContext), &progID);
			::StringFromCLSID(*reinterpret_cast<CLSID*>(pContext), &clsid);
		}
		*pPolicy = (app.messageBox(MSG_ACTIVEX_CAUTION, dialogType, MARGS
			% scriptFileName_
			% ((progID != 0) ? progID : app.loadString(MSG_NOT_OBTAINED))
			% ((clsid != 0) ? clsid : app.loadString(MSG_NOT_OBTAINED))) == IDYES) ? URLPOLICY_ALLOW : URLPOLICY_DISALLOW;

		if(progID != 0)	::CoTaskMemFree(progID);
		if(clsid != 0)	::CoTaskMemFree(clsid);

		return (*pPolicy == URLPOLICY_ALLOW) ? S_OK : S_FALSE;
	} else /*if(nSecurityLevel >= SSSL_DISALLOW)*/ {
		*pPolicy = URLPOLICY_DISALLOW;
		return S_FALSE;
	}
#endif /* 0 */
}

/// @see IScriptSystem#put_SecurityLevel
STDMETHODIMP ScriptSystem::put_SecurityLevel(short) {
	return E_NOTIMPL;
}

/// @see IInternetHostSecurityManager#QueryCustomPolicy
STDMETHODIMP ScriptSystem::QueryCustomPolicy(REFGUID guidKey,
		BYTE** ppPolicy, DWORD* pcbPolicy, BYTE* pContext, DWORD cbContext, DWORD dwReserved) {
	VERIFY_POINTER(ppPolicy);
	VERIFY_POINTER(pcbPolicy);

	*ppPolicy = 0;
	*pcbPolicy = 0;
	if(dwReserved != 0)
		return E_INVALIDARG;
//	if(guidKey == GUID_CUSTOM_CONFIRMSAFETY && cbContext >= sizeof(CONFIRMSAFETY*)) {
//		CONFIRMSAFETY* pcs = reinterpret_cast<CONFIRMSAFETY*>(pContext);
//	}
	*ppPolicy = static_cast<BYTE*>(::CoTaskMemAlloc(sizeof(DWORD)));
	if(*ppPolicy == 0)
		return E_OUTOFMEMORY;
	*pcbPolicy = sizeof(DWORD);
	**ppPolicy = URLPOLICY_ALLOW;
	return S_OK;
}

/// @c #addTopLevelObject �Œǉ������ŏ�ʃI�u�W�F�N�g��S�Ď�菜��
void ScriptSystem::releaseTopLevelObjects() {
	for(MemberTable::iterator it = topLevelObjects_.begin(); it != topLevelObjects_.end(); ++it)
		it->second->Release();
	topLevelObjects_.clear();
}

/**
 * �^����ꂽ�X�N���v�g�t�@�C���������S�p�X�ɕϊ�����
 * @param fileName �X�N���v�g�t�@�C����
 * @param[out] result ���S�p�X�B�������͌Ăяo�������m�ۂ��Ă���
 * @return �t�@�C�������݂��Ȃ��ꍇ�� false
 */
bool ScriptSystem::resolveScriptFileName(const WCHAR* fileName, WCHAR* result) const {
	assert(fileName != 0 && result != 0);
	replace_copy_if(fileName, fileName + wcslen(fileName) + 1, result, bind2nd(equal_to<WCHAR>(), L'/'), L'\\');
	if(toBoolean(::PathIsRelativeW(result))) {
		WCHAR dir[MAX_PATH];
		::GetModuleFileNameW(0, dir, MAX_PATH);
		wcscpy(::PathFindFileNameW(dir), L"script\\");
		::PathCombineW(result, dir, result);
	}
	return toBoolean(::PathFileExistsW(result));
}

/// �V���b�g�_�E�� (�I�����ɂ͂��̃��\�b�h�𖾎��I�ɌĂяo���Ȃ���΂Ȃ�Ȃ�)
void ScriptSystem::shutdown() {
	if(!scriptHosts_.empty()) {
		for(ScriptHosts::iterator i = scriptHosts_.begin(); i != scriptHosts_.end(); ++i) {
			i->second->closeEngine();
			i->second->Release();
		}
		scriptHosts_.clear();
	}
	releaseTopLevelObjects();
}


// Namespace ////////////////////////////////////////////////////////////////

Namespace::Namespace(const wchar_t* name, Namespace* parent) : name_(name), parent_(parent) {
}

Namespace::~Namespace() {
	clear();
}

bool Namespace::addObject(const wchar_t* name, IDispatch& object) {
	assert(name != 0);
	if(isLocked() || children_.find(name) != children_.end() || isDefined(name))
		return false;
	objects_.insert(make_pair(name, &object));
	object.AddRef();
	return true;
}

void Namespace::clear() {
	if(isLocked())
		return;
	for(std::map<std::wstring, IDispatch*, AutomationNameComparison>::iterator i = objects_.begin(); i != objects_.end(); ++i)
		i->second->Release();
	for(std::map<std::wstring, Namespace*, AutomationNameComparison>::iterator i = children_.begin(); i != children_.end(); ++i)
		delete i->second;
}

Namespace* Namespace::createNamespace(const wchar_t* name) {
	assert(name != 0);
	if(isLocked() || children_.find(name) != children_.end() || isDefined(name))
		return 0;
	if(Namespace* newNamespace = new Namespace(name, this)) {
//		newNamespace->AddRef();
		children_.insert(make_pair(name, newNamespace));
		return newNamespace;
	}
	return 0;
}

Namespace* Namespace::getChild(const wchar_t* name) const {
	assert(name != 0);
	const map<std::basic_string<wchar_t>, Namespace*, AutomationNameComparison>::iterator i = const_cast<Namespace*>(this)->children_.find(name);
	return (i != children_.end()) ? i->second : 0;
}

size_t Namespace::getChildCount() const {
	return children_.size();
}

const wchar_t* Namespace::getName() const {
	return name_.c_str();
}

IDispatch* Namespace::getObject(const wchar_t* name) const {
	assert(name != 0);
	const map<std::basic_string<wchar_t>, IDispatch*, AutomationNameComparison>::iterator i = const_cast<Namespace*>(this)->objects_.find(name);
	if(i != objects_.end())
		return (i->second->AddRef()), i->second;
	else
		return 0;
}

size_t Namespace::getObjectCount() const {
	return objects_.size();
}

Namespace* Namespace::getParent() const {
	return parent_;
}

bool Namespace::isDefined(const wchar_t* name) const {
	return objects_.find(name) != objects_.end();
}

bool Namespace::isEmpty() const {
	return getChildCount() == 0 && getObjectCount() == 0;
}

bool Namespace::isLocked() const {
	return lockingCookie_ > 0;
}

long Namespace::lock() {
	return isLocked() ? 0 : ++lockingCookie_;
}

void Namespace::unlock(long cookie) {
	lockingCookie_ = 0;
}


// AdhocEventSinkBase ///////////////////////////////////////////////////////

/**
 * �R���X�g���N�^
 * @param eventIID �C�x���g�C���^�[�t�F�C�X�� IID
 */
AdhocEventSinkBase::AdhocEventSinkBase(const IID& eventIID) throw() : eventIID_(eventIID) {
}

/// �f�X�g���N�^
AdhocEventSinkBase::~AdhocEventSinkBase() throw() {
	disconnect();
}

/**
 * �C�x���g�V���N�Ƃ��Đڑ�����
 * @param eventSource �ڑ��|�C���g
 * @return @c IConnectionPoint#Advise �Ɠ���
 */
HRESULT AdhocEventSinkBase::connect(IConnectionPoint& eventSource) {
	if(!eventSource_.isNull())
		return E_UNEXPECTED;
	HRESULT hr;
	if(FAILED(hr = eventSource.Advise(this, &cookie_))) {
		eventSource_ = &eventSource;
		return hr;
	}

	// ���\�b�h�̃G�~�����[�g (DISPID => name �̎������쐬����)�B
	// ���������̓G���[���N�����Ă� S_OK ��Ԃ�
	ComPtr<IConnectionPointContainer> container;
	if(FAILED(hr = eventSource.GetConnectionPointContainer(&container)))
		return S_OK;
	ComQIPtr<IProvideClassInfo>	klass;
	ComPtr<ITypeInfo>			typeInfo;
	ComQIPtr<IDispatch>			activeXObject;
	if((SUCCEEDED(container->QueryInterface(IID_IProvideClassInfo, &klass)) && SUCCEEDED(klass->GetClassInfo(&typeInfo)))
			|| (SUCCEEDED(container->QueryInterface(IID_IDispatch, &activeXObject))
			&& SUCCEEDED(activeXObject->GetTypeInfo(0, LOCALE_USER_DEFAULT, &typeInfo)))) {
		ComPtr<ITypeLib> typeLib;
		UINT i;
		if(SUCCEEDED(typeInfo->GetContainingTypeLib(&typeLib, &i))) {
			typeInfo.release();
			if(SUCCEEDED(typeLib->GetTypeInfoOfGuid(eventIID_, &typeInfo))) {
				TYPEATTR* typeAttribute;
				if(SUCCEEDED(typeInfo->GetTypeAttr(&typeAttribute))) {
					for(WORD i = 0; i < typeAttribute->cFuncs; ++i) {
						FUNCDESC* methodDescription;
						if(SUCCEEDED(typeInfo->GetFuncDesc(i, &methodDescription))) {
							if(methodDescription->funckind == FUNC_DISPATCH) {
								BSTR name;
								if(SUCCEEDED(typeInfo->GetDocumentation(methodDescription->memid, &name, 0, 0, 0))) {
									eventIDTable_.insert(make_pair(methodDescription->memid, name));
									::SysFreeString(name);
								}
							}
							typeInfo->ReleaseFuncDesc(methodDescription);
						}
					}
					typeInfo->ReleaseTypeAttr(typeAttribute);
				}
			}
		}
	}
	return S_OK;
}

/// @see IDispatchEx:#DeleteMemberByDispID
STDMETHODIMP AdhocEventSinkBase::DeleteMemberByDispID(DISPID id) {
	return S_FALSE;
}

/// @see IDispatchEx#DeleteMemberByName
STDMETHODIMP AdhocEventSinkBase::DeleteMemberByName(BSTR bstrName, DWORD grfdex) {
	return S_FALSE;
}

/**
 * �ڑ���ؒf����
 * @return @c IConnecttionPoint#Unadvise �Ɠ���
 */
HRESULT AdhocEventSinkBase::disconnect() throw() {
	HRESULT hr = S_OK;
	if(!eventSource_.isNull() && SUCCEEDED(hr = eventSource_->Unadvise(cookie_))) {
		eventSource_.release();
		eventIDTable_.clear();
	}
	return hr;
}

/**
 * �C�x���g�\�[�X�I�u�W�F�N�g����ڑ��|�C���g����������
 * @param source �C�x���g�\�[�X
 * @param[out] eventIID �ڑ��|�C���g
 * @param[in] coclassID �C�x���g�C���^�[�t�F�C�X�̓���Ɏg�� coclass �� GUID
 * @return ���ۂȂ�
 */
HRESULT AdhocEventSinkBase::findSourceConnectionPoint(IDispatch& source,
		ComPtr<IConnectionPoint>& connectionPoint, const CLSID& coclassID /* = CLSID_NULL */) {
	ComQIPtr<IConnectionPointContainer> container;
	if(FAILED(source.QueryInterface(IID_IConnectionPointContainer, &container)))
		return E_NOINTERFACE;

	// IProvideClassInfo2 -> IProvideMultipleClassInfo -> IProvideClassInfo(ITypeInfo) -> coclassID �̏��Ƀg���C
	HRESULT hr;
	IID eventIID;
	ComQIPtr<IProvideClassInfo2> classInfo2;
	if(SUCCEEDED(hr = container->QueryInterface(IID_IProvideClassInfo2, &classInfo2)))
		hr = classInfo2->GetGUID(GUIDKIND_DEFAULT_SOURCE_DISP_IID, &eventIID);
	else {
		ComQIPtr<IProvideMultipleClassInfo> multiClassInfo;
		if(S_OK == container->QueryInterface(IID_IProvideMultipleClassInfo, &multiClassInfo))
			hr = multiClassInfo->GetInfoOfIndex(0, MULTICLASSINFO_GETIIDSOURCE, 0, 0, 0, 0, &eventIID);
	}
	if(FAILED(hr)) {
		ComQIPtr<IProvideClassInfo> classInfo;
		if(S_OK == container->QueryInterface(IID_IProvideClassInfo, &classInfo)) {
			ComPtr<ITypeInfo> coclassType;
			if(SUCCEEDED(classInfo->GetClassInfo(&coclassType))) {
				TYPEATTR* typeAttr;
				if(SUCCEEDED(coclassType->GetTypeAttr(&typeAttr))) {
					for(unsigned int i = 0; i < typeAttr->cImplTypes; ++i) {
						int flags;
						if(FAILED(coclassType->GetImplTypeFlags(i, &flags)) || (flags != (IMPLTYPEFLAG_FDEFAULT | IMPLTYPEFLAG_FSOURCE)))
							continue;
						HREFTYPE refType;
						ComPtr<ITypeInfo> interfaceType;
						if(SUCCEEDED(coclassType->GetRefTypeOfImplType(i, &refType))
								&& SUCCEEDED(coclassType->GetRefTypeInfo(refType, &interfaceType))) {
							TYPEATTR* attr;
							if(SUCCEEDED(hr = interfaceType->GetTypeAttr(&attr))) {
								if(attr->typekind == TKIND_DISPATCH) {
									eventIID = attr->guid;
									interfaceType->ReleaseTypeAttr(attr);
									break;
								}
								interfaceType->ReleaseTypeAttr(attr);
							}
						}
					}
					coclassType->ReleaseTypeAttr(typeAttr);
				}
			}
		}
	}

	// �܂�������Ȃ��ꍇ�͎�芸�����擪�̃C�x���g���g��
	if(FAILED(hr)) {
		ComPtr<IEnumConnectionPoints> points;
		if(SUCCEEDED(container->EnumConnectionPoints(&points))) {
			if(SUCCEEDED(points->Next(1, &connectionPoint, 0)))
				return connectionPoint->GetConnectionInterface(&eventIID);
		}
		return E_FAIL/*CONNECT_E_CANNOTCONNECT*/;
	}

	// �ڑ��|�C���g��T��
	return container->FindConnectionPoint(eventIID, &connectionPoint);
}

/// @see IDispatchEx#GetDispID
STDMETHODIMP AdhocEventSinkBase::GetDispID(BSTR bstrName, DWORD, DISPID* pid) {
	return GetIDsOfNames(IID_NULL, &bstrName, 1, LOCALE_USER_DEFAULT, pid);
}

/// @see IDispatch#GetIDsOfNames
STDMETHODIMP AdhocEventSinkBase::GetIDsOfNames(REFIID riid, OLECHAR**, unsigned int, LCID, DISPID* rgDispId) {
	if(riid != IID_NULL)
		return E_INVALIDARG;
	VERIFY_POINTER(rgDispId);
	return (*rgDispId = DISPID_UNKNOWN), DISP_E_UNKNOWNNAME;
}

/// @see IDispatchEx#GetMemberName
STDMETHODIMP AdhocEventSinkBase::GetMemberName(DISPID id, BSTR* pbstrName) {
	VERIFY_POINTER(pbstrName);
	map<DISPID, basic_string<OLECHAR> >::const_iterator i = eventIDTable_.find(id);
	if(i == eventIDTable_.end())
		return DISP_E_UNKNOWNNAME;
	return toBoolean(*pbstrName = ::SysAllocString(i->second.c_str())) ? S_OK : E_OUTOFMEMORY;
}

/// @see IDispatchEx#GetMemberProperties
STDMETHODIMP AdhocEventSinkBase::GetMemberProperties(DISPID, DWORD, DWORD* pgrfdex) {
	VERIFY_POINTER(pgrfdex);
	*pgrfdex = 0;
	return E_NOTIMPL;
}

/// @see IDispatchEx#GetNameSpaceParent
STDMETHODIMP AdhocEventSinkBase::GetNameSpaceParent(IUnknown** ppunk) {
	VERIFY_POINTER(ppunk);
	*ppunk = 0;
	return E_NOTIMPL;
}

/// @see IDispatchEx#GetNextDispID
STDMETHODIMP AdhocEventSinkBase::GetNextDispID(DWORD, DISPID, DISPID* pid) {
	VERIFY_POINTER(pid);
	*pid = 0;
	return E_NOTIMPL;
}

/// @see IDispatch#GetTypeInfo
STDMETHODIMP AdhocEventSinkBase::GetTypeInfo(UINT iTypeInfo, LCID lcid, ITypeInfo** ppTypeInfo) {
	VERIFY_POINTER(ppTypeInfo);
	*ppTypeInfo = 0;
	return TYPE_E_ELEMENTNOTFOUND;
}

/// @see IDispatch#GetTypeInfoCount
STDMETHODIMP AdhocEventSinkBase::GetTypeInfoCount(UINT* pcTypeInfo) {
	VERIFY_POINTER(pcTypeInfo);
	*pcTypeInfo = 0;
	return S_OK;
}

/// @see IDispatch#Invoke
STDMETHODIMP AdhocEventSinkBase::Invoke(DISPID dispidMember, REFIID riid,
		LCID lcid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, unsigned int*) {
	return (riid == IID_NULL) ? InvokeEx(dispidMember, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, 0) : E_INVALIDARG;
}

/// @see IDispatchEx#InvokeEx
STDMETHODIMP AdhocEventSinkBase::InvokeEx(DISPID id, LCID lcid, WORD wFlags,
		DISPPARAMS* pdp, VARIANT* pvarRes, EXCEPINFO* pei, IServiceProvider* pspCaller) {
	map<DISPID, basic_string<OLECHAR> >::const_iterator it = eventIDTable_.find(id);
	return (it != eventIDTable_.end()) ?
		fireEvent(it->second.c_str(), lcid, wFlags, pdp, pvarRes, pei, pspCaller) : DISP_E_MEMBERNOTFOUND;
}

/// @see IUnknown#QueryInterface
STDMETHODIMP AdhocEventSinkBase::QueryInterface(REFIID riid, void** ppv) {
	VERIFY_POINTER(ppv);
	if(riid == eventIID_ || riid == IID_IDispatchEx || riid == IID_IDispatch || riid == IID_IUnknown)
		*ppv = static_cast<IDispatchEx*>(this);
	else
		return (*ppv = 0), E_NOINTERFACE;
	reinterpret_cast<IUnknown*>(*ppv)->AddRef();
	return S_OK;
}


// ScriptHost::AdhocEventSink ///////////////////////////////////////////////

/**
 * �R���X�g���N�^
 * @param scriptEngine ����G���W��
 * @param eventIID �C�x���g�C���^�[�t�F�C�X�� IID
 * @param prefix �C�x���g�n���h���̐ړ���
 */
ScriptHost::LegacyAdhocEventSink::LegacyAdhocEventSink(IActiveScript& scriptEngine, const IID& eventIID, const wstring& prefix)
		: AdhocEventSinkBase(eventIID), scriptEngine_(&scriptEngine), prefix_(prefix) {
}

/// @see AdhocEventSinkBase#fireEvent
HRESULT ScriptHost::LegacyAdhocEventSink::fireEvent(const OLECHAR* name,
		LCID locale, WORD flags, DISPPARAMS* params, VARIANT* result, EXCEPINFO* exception, IServiceProvider* serviceProvider) {
	assert(name != 0);
	OLECHAR* procedureName = new(nothrow) OLECHAR[wcslen(name) + prefix_.length() + 1];
	if(procedureName == 0)
		return E_OUTOFMEMORY;
	wcscat(wcscpy(procedureName, prefix_.c_str()), name);

	HRESULT hr;
	ComPtr<IDispatch> topLevel;
	if(SUCCEEDED(hr = scriptEngine_->GetScriptDispatch(0, &topLevel))) {
		DISPID id;
		ComQIPtr<IDispatchEx> ex;
		if(SUCCEEDED(hr = topLevel->QueryInterface(IID_IDispatchEx, &ex))
				&& SUCCEEDED(hr = ex->GetDispID(ole::ComBSTR<>(name), fdexNameCaseInsensitive, &id)))
			hr = ex->InvokeEx(id, locale, flags, params, result, exception, serviceProvider);
		else if(SUCCEEDED(hr = topLevel->GetIDsOfNames(IID_NULL, const_cast<OLECHAR**>(&procedureName), 1, locale, &id)))
			hr = topLevel->Invoke(id, IID_NULL, locale, flags, params, result, exception, 0);
	}
	delete[] procedureName;
	return hr;
}


// ScriptHost::AdhocEventSink ///////////////////////////////////////////////

/**
 *	�R���X�g���N�^
 *	@param eventSink	�C�x���g�V���N�ƂȂ� OLE �I�u�W�F�N�g
 *	@param eventIID		�C�x���g�C���^�[�t�F�C�X�� IID
 */
ScriptHost::AdhocEventSink::AdhocEventSink(IDispatch& eventSink, const IID& eventIID) : AdhocEventSinkBase(eventIID), sink_(&eventSink) {
}

/// @see AdhocEventSinkBase::fireEvent
HRESULT ScriptHost::AdhocEventSink::fireEvent(const OLECHAR* name,
		LCID locale, WORD flags, DISPPARAMS* params, VARIANT* result, EXCEPINFO* exception, IServiceProvider* serviceProvider) {
	HRESULT hr;
	DISPID id;
	ComQIPtr<IDispatchEx> ex;
	if(SUCCEEDED(hr = sink_->QueryInterface(IID_IDispatchEx, &ex))) {
		if(FAILED(hr = ex->GetDispID(ole::ComBSTR<>(name), fdexNameCaseInsensitive, &id)))
			return hr;
		hr = ex->InvokeEx(id, locale, flags, params, result, exception, serviceProvider);
		return hr;
	}
	if(hr == S_OK || SUCCEEDED(hr = sink_->GetIDsOfNames(IID_NULL, const_cast<OLECHAR**>(&name), 1, locale, &id)))
		hr = sink_->Invoke(id, IID_NULL, locale, flags, params, result, exception, 0);
	return hr;
}


// AutomationEnumeration ////////////////////////////////////////////////////

/// �R���X�g���N�^
AutomationEnumeration::AutomationEnumeration() : nextID_(INITIAL_ID) {
}

/**
 * �v���p�e�B��ǉ�����
 * @param mame �v���p�e�B��
 * @param value �l
 * @throw std::invalid_argument ���ɓ����̃v���p�e�B�����݂���Ƃ��X���[
 */
void AutomationEnumeration::addProperty(const OLECHAR* name, long value) {
	assert(name != 0);
	const NameToIDTable::const_iterator it = nameTable_.find(name);
	if(it != nameTable_.end())
		throw std::invalid_argument("There is a property has same name.");
	nameTable_.insert(make_pair(name, nextID_));
	idTable_.insert(make_pair(nextID_++, value));
}

/// @see IDispatchEx#DeleteMemberByDispID
STDMETHODIMP AutomationEnumeration::DeleteMemberByDispID(DISPID id) {
	return (idTable_.find(id) != idTable_.end()) ? S_FALSE : DISP_E_MEMBERNOTFOUND;
}

/// @see IDispatchEx#DeleteMemberByName
STDMETHODIMP AutomationEnumeration::DeleteMemberByName(BSTR bstrName, DWORD) {
	return (nameTable_.find(bstrName) != nameTable_.end()) ? S_FALSE : DISP_E_UNKNOWNNAME;
}

/// @see IDispatchEx#GetDispID
STDMETHODIMP AutomationEnumeration::GetDispID(BSTR bstrName, DWORD grfdex, DISPID* pid) {
	VERIFY_POINTER(pid);
	*pid = DISPID_UNKNOWN;
	if(bstrName == 0)
		return E_INVALIDARG;
	NameToIDTable::const_iterator i = nameTable_.find(bstrName);
	return (i != nameTable_.end()) ? (*pid = i->second), S_OK : DISP_E_UNKNOWNNAME;
}

/// @see IDispatch#GetIDsOfNames
STDMETHODIMP AutomationEnumeration::GetIDsOfNames(
		REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId) {
	VERIFY_POINTER(rgDispId);
	if(riid != IID_NULL || rgszNames == 0)
		return E_INVALIDARG;
	GetDispID(rgszNames[0], fdexNameCaseInsensitive, rgDispId);
	for(UINT i = 1; i < cNames; ++i)
		rgDispId[i] = DISPID_UNKNOWN;
	return (rgDispId[0] != DISPID_UNKNOWN) ? S_OK : DISP_E_UNKNOWNNAME;
}

/// @see IDispatchEx#GetMemberName
STDMETHODIMP AutomationEnumeration::GetMemberName(DISPID id, BSTR* pbstrName) {
	VERIFY_POINTER(pbstrName);
	*pbstrName = 0;
	for(NameToIDTable::const_iterator i = nameTable_.begin(); i != nameTable_.end(); ++i) {
		if(i->second == id)
			return (*pbstrName = ::SysAllocString(i->first.c_str())) ? S_OK : E_OUTOFMEMORY;
	}
	return DISP_E_UNKNOWNNAME;
}

/// @see IDispatchEx#GetMemberProperties
STDMETHODIMP AutomationEnumeration::GetMemberProperties(DISPID id, DWORD grfdexFetch, DWORD* pgrfdex) {
	VERIFY_POINTER(pgrfdex);
	*pgrfdex = 0;
	if(idTable_.find(id) == idTable_.end())
		return DISP_E_UNKNOWNNAME;
	*pgrfdex = fdexPropCanGet | fdexPropCannotPut | fdexPropCannotPutRef
		| fdexPropNoSideEffects | fdexPropCannotCall | fdexPropCannotConstruct | fdexPropCannotSourceEvents;
	*pgrfdex &= grfdexFetch;
	return S_OK;
}

/// @see IDispatchEx#GetNameSpaceParent
STDMETHODIMP AutomationEnumeration::GetNameSpaceParent(IUnknown** ppunk) {
	VERIFY_POINTER(ppunk);
	*ppunk = 0;
	return E_NOTIMPL;
}

/// @see IDispatchEx#GetNextDispID
STDMETHODIMP AutomationEnumeration::GetNextDispID(DWORD grfdex, DISPID id, DISPID* pid) {
	VERIFY_POINTER(pid);
	if(toBoolean(grfdex & fdexEnumDefault))
		return (*pid = DISPID_UNKNOWN), S_FALSE;
	if(id == DISPID_STARTENUM)
		id = INITIAL_ID;
	if(id >= INITIAL_ID && id < nextID_ - 1)
		return (*pid = id + 1), S_OK;
	else
		return (*pid = DISPID_UNKNOWN), S_FALSE;
}

/// @see IDispatch#GetTypeInfo
STDMETHODIMP AutomationEnumeration::GetTypeInfo(UINT iTypeInfo, LCID lcid, ITypeInfo** ppTypeInfo) {
	VERIFY_POINTER(ppTypeInfo);
	*ppTypeInfo = 0;
	return E_NOTIMPL;
}

/// @see IDispatch#GetTypeInfoCount
STDMETHODIMP AutomationEnumeration::GetTypeInfoCount(UINT* pcTypeInfo) {
	VERIFY_POINTER(pcTypeInfo);
	*pcTypeInfo = 0;
	return S_OK;
}

/// @see IDispatch#Invoke
STDMETHODIMP AutomationEnumeration::Invoke(DISPID dispidMember, REFIID riid, LCID lcid,
		WORD wFlags, DISPPARAMS *pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {
	return (riid == IID_NULL) ? InvokeEx(dispidMember, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, 0) : E_INVALIDARG;
}

/// @see IDispatchEx#InvokeEx
STDMETHODIMP AutomationEnumeration::InvokeEx(DISPID id, LCID, WORD wFlags, DISPPARAMS* pdp, VARIANT* pvarRes, EXCEPINFO*, IServiceProvider*) {
	if(pvarRes == 0)
		return S_OK;
	else if(!toBoolean(wFlags & DISPATCH_PROPERTYGET))	// VBS �Ƃ��� DISPATCH_METHOD | DISPATCH_PROPERTYGET ��n���Ă���
		return DISP_E_MEMBERNOTFOUND;
	else if(pdp->cArgs != 0)
		return DISP_E_BADPARAMCOUNT;

	const IDToValueTable::const_iterator i = idTable_.find(id);
	if(i == idTable_.end())
		return DISP_E_MEMBERNOTFOUND;
	pvarRes->vt = VT_I4;
	pvarRes->lVal = i->second;
	return S_OK;
}
