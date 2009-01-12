/**
 * @file ambient.cpp
 */

#include "ambient.hpp"
#include "ambient-iid.hpp"
#include "application.hpp"
#include "../resource/messages.h"
using namespace alpha::ambient;
using namespace manah;
using namespace manah::com;
using namespace std;
using ascension::NullPointerException;


namespace {
	inline int compareAutomationNames(const basic_string<OLECHAR>& lhs, const basic_string<OLECHAR>& rhs) throw() {
		return ::CompareStringW(LOCALE_NEUTRAL,
			NORM_IGNORECASE | NORM_IGNOREKANATYPE | NORM_IGNORENONSPACE | NORM_IGNOREWIDTH,
			lhs.data(), static_cast<int>(lhs.length()), rhs.data(), static_cast<int>(rhs.length()));
	}
	struct AutomationNameComparison {
		bool operator()(const basic_string<OLECHAR>& lhs, const basic_string<OLECHAR>& rhs) const {return compareAutomationNames(lhs, rhs) == CSTR_LESS_THAN;}
	};

	class Enumeration : public IObjectSafetyImpl<
		INTERFACESAFE_FOR_UNTRUSTED_CALLER, INTERFACESAFE_FOR_UNTRUSTED_CALLER, IUnknownImpl<
			typelist::Cat<MANAH_INTERFACE_SIGNATURE(IDispatch),
			typelist::Cat<MANAH_INTERFACE_SIGNATURE(IObjectSafety)> >
		>
	> {
	public:
		Enumeration(const map<const basic_string<OLECHAR>, long>& table, map<const basic_string<OLECHAR>, long>::const_iterator defaultValue);
		// IDispatch
		STDMETHODIMP GetIDsOfNames(REFIID iid, OLECHAR** names, unsigned int numberOfNames, LCID, DISPID* id);
		STDMETHODIMP GetTypeInfo(unsigned int index, LCID, ITypeInfo** typeInfo);
		STDMETHODIMP GetTypeInfoCount(unsigned int* number);
		STDMETHODIMP Invoke(DISPID id, REFIID iid, LCID, WORD flags, DISPPARAMS* parameters, VARIANT* result, EXCEPINFO* exception, unsigned int* argErr);
	private:
		map<const basic_string<OLECHAR>, DISPID, AutomationNameComparison> nameToIDs_;
		vector<long> idToValues_;
		const bool hasDefaultValue_;
	};
} // namespace @0


// Enumeration //////////////////////////////////////////////////////////////

Enumeration::Enumeration(const map<const basic_string<OLECHAR>, long>& table,
		map<const basic_string<OLECHAR>, long>::const_iterator defaultValue) : hasDefaultValue_(defaultValue != table.end()) {
	idToValues_.reserve(table.size());
	if(hasDefaultValue_) {
		nameToIDs_.insert(*defaultValue);
		idToValues_.push_back(defaultValue->second);
	} else
		idToValues_.push_back(0);	// dummy value
	for(map<const basic_string<OLECHAR>, long>::const_iterator i(table.begin()), e(table.end()); i != e; ++i) {
		if(i != defaultValue) {
			nameToIDs_[i->first] = static_cast<DISPID>(idToValues_.size() + 1);
			idToValues_.push_back(i->second);
		}
	}
}

STDMETHODIMP Enumeration::GetIDsOfNames(REFIID iid, OLECHAR** names, unsigned int numberOfNames, LCID, DISPID* id) {
	if(iid != IID_NULL || names == 0 || id == 0)
		return E_INVALIDARG;
	else if(numberOfNames == 0)
		return S_OK;
	bool failedOnce = false;
	map<const basic_string<OLECHAR>, DISPID, AutomationNameComparison>::const_iterator i(nameToIDs_.find(basic_string<OLECHAR>(names[0])));
	if(i != nameToIDs_.end())
		id[0] = i->second;
	else {
		id[0] = DISPID_UNKNOWN;
		failedOnce = true;
	}
	if(numberOfNames > 1) {
		failedOnce = true;
		fill(id + 1, id + numberOfNames, DISPID_UNKNOWN);
	}
	return !failedOnce ? S_OK : DISP_E_UNKNOWNNAME;
}

STDMETHODIMP Enumeration::GetTypeInfo(unsigned int, LCID, ITypeInfo** typeInfo) {
	MANAH_VERIFY_POINTER(typeInfo);
	return (*typeInfo = 0), E_NOTIMPL;
}

STDMETHODIMP Enumeration::GetTypeInfoCount(unsigned int* number) {
	MANAH_VERIFY_POINTER(number);
	return (*number = 0), S_OK;
}

STDMETHODIMP Enumeration::Invoke(DISPID id, REFIID iid, LCID, WORD flags,
		DISPPARAMS* parameters, VARIANT* result, EXCEPINFO* exception, unsigned int* argErr) {
	if(parameters == 0)
		return E_POINTER;
	else if(iid != IID_NULL)
		return E_INVALIDARG;
	else if(parameters->cArgs != 0)
		return DISP_E_BADPARAMCOUNT;
	else if(id < 0 || static_cast<size_t>(id) >= idToValues_.size() + 1 || (flags & DISPATCH_PROPERTYGET) == 0)
		return DISP_E_MEMBERNOTFOUND;
	if(result != 0) {
		::VariantClear(result);
		::VariantInit(result);
		result->vt = VT_I4;
		result->lVal = idToValues_[id - 1];
	}
	return S_OK;
}


// IEnumVARIANTStaticImpl ///////////////////////////////////////////////////

/**
 * Constructor.
 * @param array the array contains the elements to be enumerated
 * @param length the length of @a array
 */
IEnumVARIANTStaticImpl::IEnumVARIANTStaticImpl(AutoBuffer<VARIANT> array, size_t length) : data_(new SharedData) {
	data_->refs = 1;
	data_->array = array.release();
	data_->length = length;
}

/// Copy-constructor.
IEnumVARIANTStaticImpl::IEnumVARIANTStaticImpl(const IEnumVARIANTStaticImpl& rhs) : data_(rhs.data_), current_(rhs.current_) {
	++data_->refs;
}

/// Destructor.
IEnumVARIANTStaticImpl::~IEnumVARIANTStaticImpl() throw() {
	if(--data_->refs == 0) {
		for(size_t i = 0; i < data_->length; ++i)
			::VariantClear(data_->array + i);
		delete[] data_->array;
		delete data_;
	}
}

/// Implements @c IEnumVARIANT#Clone.
STDMETHODIMP IEnumVARIANTStaticImpl::Clone(IEnumVARIANT** enumerator) {
	MANAH_VERIFY_POINTER(enumerator);
	if(*enumerator = new(nothrow) IEnumVARIANTStaticImpl(*this))
		return S_OK;
	return E_OUTOFMEMORY;
}

/// Implements @c IEnumVARIANT#Next.
STDMETHODIMP IEnumVARIANTStaticImpl::Next(ULONG numberOfElements, VARIANT* values, ULONG* numberOfFetchedElements) {
	MANAH_VERIFY_POINTER(values);
	ULONG fetched;
	for(fetched = 0; fetched < numberOfElements && current_ + fetched < data_->array + data_->length; ++fetched) {
		const HRESULT hr = ::VariantCopy(values + fetched, current_ + fetched);
		if(FAILED(hr)) {
			for(ULONG i = 0; i < fetched; ++i)
				::VariantClear(current_ + i);
			return hr;
		}
	}
	if(numberOfFetchedElements != 0)
		*numberOfFetchedElements = fetched;
	return (fetched == numberOfElements) ? S_OK : S_FALSE;
}

/// Implements @c IEnumVARIANT#Reset.
STDMETHODIMP IEnumVARIANTStaticImpl::Reset() {
	current_ = data_->array;
	return S_OK;
}

/// Implements @c IEnumVARIANT#Skip.
STDMETHODIMP IEnumVARIANTStaticImpl::Skip(ULONG numberOfElements) {
	const VARIANT* const previous = current_;
	current_ = min(current_ + numberOfElements, data_->array + data_->length);
	return (current_ - previous == numberOfElements) ? S_OK : S_FALSE;
}


// ScriptSystem.ScriptHost //////////////////////////////////////////////////

class ScriptSystem::ScriptHost : public IUnknownImpl<
	typelist::Cat<MANAH_INTERFACE_SIGNATURE(IActiveScriptSite),
	typelist::Cat<MANAH_INTERFACE_SIGNATURE(IActiveScriptSiteWindow),
	typelist::Cat<MANAH_INTERFACE_SIGNATURE(IActiveScriptSiteInterruptPoll),
	typelist::Cat<MANAH_INTERFACE_SIGNATURE(IServiceProvider)> > > >
> {
public:
	// constructors
	explicit ScriptHost(IActiveScript& scriptEngine);
	~ScriptHost() throw();
	// attributes
	HRESULT addToplevel(const OLECHAR* name, IDispatch& object);
	HRESULT removeToplevel(const OLECHAR* name);
	ComPtr<IActiveScript> scriptEngine() const /*throw()*/;
	ComPtr<IDispatch> toplevel(const OLECHAR* name) const;
	// 
	HRESULT executeFile(const WCHAR* fileName, VARIANT& result, bool onlyRequire);
	// IActiveScriptSite
	STDMETHODIMP GetLCID(LCID *lcid);
	STDMETHODIMP GetItemInfo(LPCOLESTR name, DWORD returnMask, IUnknown** item, ITypeInfo** typeInfo);
	STDMETHODIMP GetDocVersionString(BSTR* version);
	STDMETHODIMP OnScriptTerminate(const VARIANT* result, const EXCEPINFO* exception);
	STDMETHODIMP OnStateChange(SCRIPTSTATE scriptState);
	STDMETHODIMP OnScriptError(IActiveScriptError* error);
	STDMETHODIMP OnEnterScript();
	STDMETHODIMP OnLeaveScript();
	// IActiveScriptSiteWindow
	STDMETHODIMP GetWindow(HWND* window);
	STDMETHODIMP EnableModeless(BOOL enable);
	// IActiveScriptSiteInterruptPoll
	STDMETHODIMP QueryContinue();
	// IServiceProvider
	STDMETHODIMP QueryService(REFGUID guidService, REFIID iid, void** object);
private:
	static const OLECHAR PUBLIC_NAME[];
	static const OLECHAR PUBLIC_SHORT_NAME[];
	ComPtr<IActiveScript> scriptEngine_;
	typedef map<basic_string<OLECHAR>, IDispatch*, AutomationNameComparison> MemberTable;
	MemberTable toplevels_;
	vector<const basic_string<WCHAR> > loadedScripts_;
};

const OLECHAR ScriptSystem::ScriptHost::PUBLIC_NAME[] = OLESTR("WScript");
const OLECHAR ScriptSystem::ScriptHost::PUBLIC_SHORT_NAME[] = OLESTR("WSH");

/// Constructor.
ScriptSystem::ScriptHost::ScriptHost(IActiveScript& scriptEngine) : scriptEngine_(&scriptEngine) {
}

/// Destructor.
ScriptSystem::ScriptHost::~ScriptHost() throw() {
	// release the all top level objects
	for(MemberTable::iterator i(toplevels_.begin()), e(toplevels_.end()); i != e; ++i)
		i->second->Release();
}

/**
 * Registers the object as top level.
 * @param name the name of the object
 * @param object the object to register
 * @retval S_OK succeeded
 * @retval E_POINTER @a name is @c null
 * @retval E_INVALIDARG there is already an object has same name
 * @retval ... internal @c IActiveScript#AddNamedItem may return an error HRESULT
 */
HRESULT ScriptSystem::ScriptHost::addToplevel(const OLECHAR* name, IDispatch& object) {
	if(name == 0)
		return E_POINTER;
	else if(toplevels_.find(name) != toplevels_.end())
		throw E_INVALIDARG;
	const HRESULT hr = scriptEngine_->AddNamedItem(name, SCRIPTITEM_ISVISIBLE);
	if(SUCCEEDED(hr)) {
		toplevels_.insert(make_pair(basic_string<OLECHAR>(name), &object));
		object.AddRef();
	}
	return hr;
}

/// @see IActiveScriptSiteWindow#EnableModeless
STDMETHODIMP ScriptSystem::ScriptHost::EnableModeless(BOOL enable) {
	return manah::toBoolean(enable) ? E_FAIL : S_OK;
}

/**
 */
HRESULT ScriptSystem::ScriptHost::executeFile(const WCHAR* fileName, VARIANT& result, bool onlyRequire) {
	if(fileName == 0)
		throw NullPointerException("fileName");
	if(onlyRequire) {	// check if the file is already loaded
		for(vector<const basic_string<WCHAR> >::const_iterator i(loadedScripts_.begin()), e(loadedScripts_.end()); i != e; ++i) {
			if(ascension::kernel::fileio::comparePathNames(fileName, i->c_str()))
				return S_OK;
		}
	}

	HANDLE file = ::CreateFileW(fileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
						0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 0);
	if(file == INVALID_HANDLE_VALUE)
		return HRESULT_FROM_WIN32(::GetLastError());

	HRESULT hr = S_OK;
	OLECHAR* source = 0;
	const DWORD fileSize = ::GetFileSize(file, 0);
	if(fileSize != 0) {
		if(HANDLE mappedFile = ::CreateFileMapping(file, 0, PAGE_READONLY, 0, 0, 0)) {
			if(const uchar* const buffer = static_cast<const uchar*>(::MapViewOfFile(mappedFile, FILE_MAP_READ, 0, 0, 0))) {
				if(0 != (source = static_cast<OLECHAR*>(::HeapAlloc(::GetProcessHeap(), HEAP_NO_SERIALIZE, sizeof(OLECHAR) * (fileSize + 1))))) {
					// convert from UTF-8 into UTF-16
					using namespace ascension::encoding;
					auto_ptr<Encoder> encoder(Encoder::forMIB(fundamental::UTF_8));
					OLECHAR* toNext;
					const uchar* fromNext;
					if(Encoder::COMPLETED == encoder->setSubstitutionPolicy(
							Encoder::REPLACE_UNMAPPABLE_CHARACTER).toUnicode(
								source, source + fileSize, toNext, buffer, buffer + fileSize, fromNext))
						*toNext = 0;
					else
						hr = HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);
				} else
					hr = HRESULT_FROM_WIN32(::GetLastError());
				::UnmapViewOfFile(buffer);
			} else
				hr = HRESULT_FROM_WIN32(::GetLastError());
			::CloseHandle(mappedFile);
		} else
			hr = HRESULT_FROM_WIN32(::GetLastError());
	}
	::CloseHandle(file);

	if(SUCCEEDED(hr)) {
		// parse and evaluate
		MANAH_AUTO_STRUCT(EXCEPINFO, exception);
		ComQIPtr<IActiveScriptParse, &IID_IActiveScriptParse> parser(scriptEngine_);
		if(parser.get() != 0) {
			loadedScripts_.push_back(ascension::kernel::fileio::canonicalizePathName(fileName));	// record the executed script file
			hr = parser->ParseScriptText((fileSize != 0) ? source : OLESTR(""),
				0, 0, 0, static_cast<DWORD>(loadedScripts_.size() - 1), 0, SCRIPTTEXT_ISVISIBLE, 0, &exception);
			if(FAILED(hr) && hr != DISP_E_EXCEPTION)
				loadedScripts_.pop_back();	// failed to parse (but not runtime error)
			else
				hr = scriptEngine_->SetScriptState(SCRIPTSTATE_CONNECTED);
		}
	}

	if(source != 0)
		::HeapFree(::GetProcessHeap(), HEAP_NO_SERIALIZE, source);
	return hr;
}

/// @see IActiveScriptSite#GetDocVersionString
STDMETHODIMP ScriptSystem::ScriptHost::GetDocVersionString(BSTR* version) {
	MANAH_VERIFY_POINTER(version);
	return (*version = 0), E_NOTIMPL;
}

/// @see IActiveScriptSite#GetItemInfo
STDMETHODIMP ScriptSystem::ScriptHost::GetItemInfo(LPCOLESTR name, DWORD returnMask, IUnknown** item, ITypeInfo** typeInfo) {
	if(name == 0)
		return E_INVALIDARG;
	if(toBoolean(returnMask & SCRIPTINFO_IUNKNOWN)) {
		MANAH_VERIFY_POINTER(item);
		*item = 0;
	}
	if(toBoolean(returnMask & SCRIPTINFO_ITYPEINFO)) {
		MANAH_VERIFY_POINTER(typeInfo);
		*typeInfo = 0;
	}

	ComPtr<IDispatch> object;
//	if(wcslen(name) == MANAH_COUNTOF(SYSTEM_PUBLIC_NAME) - 1 && wcscmp(name, SYSTEM_PUBLIC_NAME) == 0)
//		object.reset(&ScriptSystem::instance());
//	else if(compareAutomationNames(name, PUBLIC_NAME) == CSTR_EQUAL || compareAutomationNames(name, PUBLIC_SHORT_NAME) == CSTR_EQUAL)
//		object.reset(this);
//	else
		object.reset(toplevel(name).get());
	if(object.get() == 0)
		return TYPE_E_ELEMENTNOTFOUND;
	if(toBoolean(returnMask & SCRIPTINFO_IUNKNOWN))
		(*item = object.get())->AddRef();
	if(toBoolean(returnMask & SCRIPTINFO_ITYPEINFO))
		object->GetTypeInfo(0, 0, typeInfo);
	return S_OK;
}

/// @see IActiveScriptSite#GetLCID
STDMETHODIMP ScriptSystem::ScriptHost::GetLCID(LCID* lcid) {
	MANAH_VERIFY_POINTER(lcid);
	return (*lcid = LOCALE_INVARIANT), S_OK;
}

/// @see IActiveScriptSiteWindow#GetWindow
STDMETHODIMP ScriptSystem::ScriptHost::GetWindow(HWND* window) {
	MANAH_VERIFY_POINTER(window);
	*window = Alpha::instance().getMainWindow().getHandle();
	return S_OK;
}

/// @see IActiveScriptSite#OnEnterScript
STDMETHODIMP ScriptSystem::ScriptHost::OnEnterScript() {
	return S_OK;
}

/// @see IActiveScriptSite#OnLeaveScript
STDMETHODIMP ScriptSystem::ScriptHost::OnLeaveScript() {
	return S_OK;
}

/// @see IActiveScriptSite#OnScriptError
STDMETHODIMP ScriptSystem::ScriptHost::OnScriptError(IActiveScriptError* error) {
	if(error == 0)
		return E_INVALIDARG;
//	else if(!scriptSystem_.isInteractive())
//		return S_OK;	// ignore

	MANAH_AUTO_STRUCT(EXCEPINFO, exception);
	error->GetExceptionInfo(&exception);
	if(exception.scode == S_OK)	// not an error
		return S_OK;

	// show error message
	Alpha& app = Alpha::instance();
	DWORD sourceContext;
	unsigned long line;
	long column;
	error->GetSourcePosition(&sourceContext, &line, &column);
	const basic_string<WCHAR> sourceName((sourceContext < loadedScripts_.size()) ? loadedScripts_[sourceContext] : app.loadMessage(MSG_OTHER__UNKNOWN));
	app.messageBox(MSG_SCRIPT__ERROR_DIALOG, MB_ICONHAND, MARGS
		% sourceName
		% (line + 1) % (column + 1)
		% ((exception.bstrDescription != 0) ? exception.bstrDescription : app.loadMessage(MSG_OTHER__UNKNOWN))
		% exception.scode % ((exception.bstrSource != 0) ? exception.bstrSource : app.loadMessage(MSG_OTHER__UNKNOWN)));
	::SysFreeString(exception.bstrSource);
	::SysFreeString(exception.bstrDescription);
	::SysFreeString(exception.bstrHelpFile);

	return S_OK;
}

/// @see IActiveScriptSite#OnScriptTerminate
STDMETHODIMP ScriptSystem::ScriptHost::OnScriptTerminate(const VARIANT*, const EXCEPINFO*) {
	return S_OK;
}

/// @see IActiveScriptSite#OnStateChange
STDMETHODIMP ScriptSystem::ScriptHost::OnStateChange(SCRIPTSTATE) {
	return S_OK;
}

/// @see IActiveScriptSite#QueryContinue
STDMETHODIMP ScriptSystem::ScriptHost::QueryContinue() {
	return S_OK;
}

/// @see IServiceProvider#QueryService
STDMETHODIMP ScriptSystem::ScriptHost::QueryService(REFGUID guidService, REFIID iid, void** object) {
	MANAH_VERIFY_POINTER(object);
//	if(guidService == SID_SInternetHostSecurityManager)
//		return scriptSystem_.QueryInterface(iid, object);
	*object = 0;
	return E_NOINTERFACE /* SVC_E_UNKNOWNSERVICE */;
}

/**
 * Removes the toplevel object with the given name.
 * @param name the name of the object to remove
 * @retval S_OK succeeded to remove
 * @retval S_FALSE there was not an object with the given name
 * @retval E_POINTER @a name is @c null
 */
HRESULT ScriptSystem::ScriptHost::removeToplevel(const OLECHAR* name) {
	if(name == 0)
		return E_POINTER;
	MemberTable::iterator i(toplevels_.find(basic_string<OLECHAR>(name)));
	if(i == toplevels_.end())
		return S_FALSE;
	i->second->Release();
	toplevels_.erase(i);
	return S_OK;
}

/// Returns the script engine.
inline ComPtr<IActiveScript> ScriptSystem::ScriptHost::scriptEngine() const /*throw()*/ {
	return scriptEngine_;
}

/// Returns the top level object has the given name.
ComPtr<IDispatch> ScriptSystem::ScriptHost::toplevel(const OLECHAR* name) const {
	if(name == 0)
		throw NullPointerException("name");
	MemberTable::const_iterator i(toplevels_.find(name));
	return ComPtr<IDispatch>((i != toplevels_.end()) ? i->second : 0);
}


// ScriptSystem /////////////////////////////////////////////////////////////

/// Default constructor.
ScriptSystem::ScriptSystem() {
	static const OLECHAR languageName[] = OLESTR("JScript");
	CLSID engineID = CLSID_NULL;
	if(FAILED(::CLSIDFromProgID(languageName, &engineID)))
		throw runtime_error("");

	ComPtr<IActiveScript> scriptEngine(engineID, IID_IActiveScript, CLSCTX_INPROC);
	if(scriptEngine.get() == 0)
		throw runtime_error("");

//	ComQIPtr<IObjectSafety> safety;
//	if(SUCCEEDED(scriptEngine->QueryInterface(IID_IObjectSafety, &safety))) {
//		DWORD supportedOptions, enabledOptions;
//		if(SUCCEEDED(safety->GetInterfaceSafetyOptions(IID_IActiveScript, &supportedOptions, &enabledOptions)))
//			safety->SetInterfaceSafetyOptions(IID_IActiveScript,
//				supportedOptions, INTERFACESAFE_FOR_UNTRUSTED_DATA | INTERFACE_USES_SECURITY_MANAGER);
//	}
	(scriptHost_ = new ScriptHost(*scriptEngine.get()))->AddRef();
	scriptEngine->SetScriptSite(scriptHost_);
	static_cast<IActiveScriptSite*>(scriptHost_);
	static_cast<IActiveScriptSiteWindow*>(scriptHost_);
	static_cast<IActiveScriptSiteInterruptPoll*>(scriptHost_);
	static_cast<IServiceProvider*>(scriptHost_);

	ComQIPtr<IActiveScriptParse, &IID_IActiveScriptParse> parser(scriptEngine);
	if(parser.get() == 0)
		throw runtime_error("");
	parser->InitNew();
	scriptHost_->addToplevel(OLESTR("ambient"), static_cast<IScriptSystem&>(*this));
//	scriptEngine->AddNamedItem(OLESTR("ambient"), /*SCRIPTITEM_GLOBALMEMBER |*/ SCRIPTITEM_ISVISIBLE);
}

/// Destructor.
ScriptSystem::~ScriptSystem() throw() {
	scriptHost_->removeToplevel(OLESTR("ambient"));
	scriptHost_->scriptEngine()->Close();
	scriptHost_->Release();
}

/// @see IScriptSystem#ExecuteFile
STDMETHODIMP ScriptSystem::ExecuteFile(BSTR fileName, VARIANT** result) {
	return executeFile(fileName, result, false, OLESTR("ambient.ScriptSystem.ExecuteFile"));
}

HRESULT ScriptSystem::executeFile(BSTR fileName, VARIANT** result, bool onlyRequire, const OLECHAR* sourceName) {
	HRESULT hr;
	VARIANT r;
	::VariantInit(&r);
	try {
		hr = scriptHost_->executeFile(fileName, r, onlyRequire);
	} catch(NullPointerException&) {
		return E_INVALIDARG;
	}
	if(result != 0)
		::VariantCopy(*result, &r);
	::VariantClear(&r);
	if(FAILED(hr)) {
		ComException e(hr, IID_IScriptSystem, sourceName);
		e.raise();
		return e.scode();
	}
	return hr;
}

/// @see IScriptSystem#GetServiceProvider
STDMETHODIMP ScriptSystem::GetServiceProvider(IServiceObjectProvider** serviceProvider) {
	MANAH_VERIFY_POINTER(serviceProvider);
	(*serviceProvider = &ServiceProvider::instance())->AddRef();
	return S_OK;
}

/// @see IScriptSystem#LoadConstants
STDMETHODIMP ScriptSystem::LoadConstants(VARIANT* libraryNameOrObject, VARIANT* parent) {
	MANAH_VERIFY_POINTER(libraryNameOrObject);
	MANAH_VERIFY_POINTER(parent);
	HRESULT hr;

	ComPtr<ITypeLib> typeLibrary;
	if(libraryNameOrObject->vt == VT_BSTR) {
		if(FAILED(hr = ::LoadTypeLib(libraryNameOrObject->bstrVal, typeLibrary.initialize())))
			return hr;
	} else if(libraryNameOrObject->vt == VT_DISPATCH) {
		if(libraryNameOrObject->pdispVal == 0)
			return E_INVALIDARG;
		UINT numberOfTypes;
		if(FAILED(hr = libraryNameOrObject->pdispVal->GetTypeInfoCount(&numberOfTypes)))
			return hr;
		else if(numberOfTypes == 0)
			return S_OK;
		ComPtr<ITypeInfo> typeInfo;
		if(FAILED(hr = libraryNameOrObject->pdispVal->GetTypeInfo(0, LOCALE_USER_DEFAULT, typeInfo.initialize())))
			return hr;
		UINT index;
		if(FAILED(hr = typeInfo->GetContainingTypeLib(typeLibrary.initialize(), &index)))
			return hr;
	} else
		return DISP_E_TYPEMISMATCH;

	ComQIPtr<IDispatchEx, &IID_IDispatchEx> parentex;
	if(parent->vt == VT_DISPATCH)
		parentex = parent->pdispVal;
	else if(parent->vt == VT_UNKNOWN)
		parentex = parent->punkVal;
	else if(parent->vt != VT_NULL)
		return DISP_E_TYPEMISMATCH;

	return loadConstantsFromTypeLibrary(*typeLibrary, GUID_NULL, parentex.get());
}

/**
 * Loads the enumerations (constants) from the given type library.
 * @param typeLibrary the type library
 * @param guid the GUID of the enumeration to load. if this is @c GUID_NULL, all enumerations in the library will be loaded
 * @param parent the parent object of the enumerations. if this is @c null, the enumerations will be 
 * @return 
 */
HRESULT ScriptSystem::loadConstantsFromTypeLibrary(ITypeLib& typeLibrary, const GUID& guid, IDispatchEx* parent) {
	const UINT numberOfTypes = typeLibrary.GetTypeInfoCount();
	HRESULT hr = S_OK;
	vector<const basic_string<OLECHAR> > registeredNames;	// for rollback when failure

	for(UINT i = 0; i < numberOfTypes; ++i) {
		ComPtr<ITypeInfo> typeInfo;
		TYPEATTR* attribute;
		if(FAILED(hr = typeLibrary.GetTypeInfo(i, typeInfo.initialize())) || FAILED(hr = typeInfo->GetTypeAttr(&attribute)))
			break;	// hr should be E_OUTOFMEMORY
		else if((guid != GUID_NULL && attribute->guid != guid) || attribute->typekind != TKIND_ENUM) {
			// the data is not what we want
			typeInfo->ReleaseTypeAttr(attribute);
			continue;
		}

		map<const basic_string<OLECHAR>, long> constants;
		basic_string<OLECHAR> defaultName;
		for(WORD j = 0; j < attribute->cVars; ++j) {
			VARDESC* variable;
			if(FAILED(hr = typeInfo->GetVarDesc(j, &variable)))
				break;	// hr should be E_OUTOFMEMORY
			else if(variable->varkind != VAR_CONST
					|| toBoolean(variable->wVarFlags & VARFLAG_FHIDDEN)
					|| toBoolean(variable->wVarFlags & VARFLAG_FNONBROWSABLE)
					|| toBoolean(variable->wVarFlags & VARFLAG_FRESTRICTED)) {
				// skip inaccessible members
				typeInfo->ReleaseVarDesc(variable);
				continue;
			}

			UINT numberOfNames;
			BSTR constantName;
			if(SUCCEEDED(hr = typeInfo->GetNames(variable->memid, &constantName, 1, &numberOfNames))) {
				constants.insert(make_pair(constantName, variable->lpvarValue->lVal));
				if((variable->wVarFlags & VARFLAG_FDEFAULTBIND) != 0)
					defaultName.assign(constantName);
				::SysFreeString(constantName);
			}
			typeInfo->ReleaseVarDesc(variable);
			if(FAILED(hr))
				break;	// hr should be E_OUTOFMEMORY
		}
		typeInfo->ReleaseTypeAttr(attribute);
		if(FAILED(hr))
			break;

		if(!constants.empty()) {
/*			if(parent != 0) {
				// add constants to the given object as properties
				for(map<const basic_string<OLECHAR>, long>::const_iterator i(constants.begin()), e(constants.end()); i != e; ++i) {
					DISPID id;
					BSTR name = ::SysAllocString(i->first.c_str());
					if(SUCCEEDED(parent->GetDispID(name, fdexNameEnsure | fdexNameCaseInsensitive, &id))) {
						VARIANTARG arg1;
						::VariantInit(&arg1);
						arg1.vt = VT_I4;
						arg1.lVal = i->second;
						MANAH_AUTO_STRUCT(DISPPARAMS, params);
						params.cArgs = 1;
						params.rgvarg = &arg1;
						parent->InvokeEx(id, LOCALE_NEUTRAL, DISPATCH_PROPERTYPUT, &params, 0, 0, 0);
						::VariantClear(&arg1);
					}
					::SysFreeString(name);
				}
			} else {
*/				// register an enumeration and constants as global
				BSTR name;
				if(SUCCEEDED(hr = typeLibrary.GetDocumentation(i, &name, 0, 0, 0))) {
					ComPtr<Enumeration> enumeration(new Enumeration(constants, constants.find(defaultName)));
					scriptHost_->addToplevel(name, *enumeration.get());
					registeredNames.push_back(name);
					::SysFreeString(name);
				} else
					break;	// hr should be STG_E_INSUFFICIENTMEMORY
//			}
		}
	}

	if(FAILED(hr)) {	// failure -> rollback registrations
		for(size_t i = 0, c = registeredNames.size(); i < c; ++i)
			scriptHost_->removeToplevel(registeredNames[i].c_str());
	}

	return hr;
}

/// @see IScriptSystem#LoadScript
STDMETHODIMP ScriptSystem::LoadScript(BSTR fileName, VARIANT* result) {
	return executeFile(fileName, &result, true, OLESTR("ambient.ScriptSystem.LoadScript"));
}


// ServiceProvider //////////////////////////////////////////////////////////

/// Default constructor.
ServiceProvider::ServiceProvider() {
}

/// Destructor.
ServiceProvider::~ServiceProvider() throw() {
	for(map<basic_string<OLECHAR>, IDispatch*>::iterator i(serviceObjects_.begin()), e(serviceObjects_.end()); i != e; ++i)
		i->second->Release();
}

/// Returns the singleton instance.
ServiceProvider& ServiceProvider::instance() {
	static ServiceProvider singleton;
	return singleton;
}

/// @see IServiceObjectProvider#QueryService
STDMETHODIMP ServiceProvider::QueryService(BSTR serviceName, IDispatch** serviceObject) {
	MANAH_VERIFY_POINTER(serviceObject);
	if(serviceName == 0)
		return E_INVALIDARG;
	map<basic_string<OLECHAR>, IDispatch*>::iterator i(serviceObjects_.find(basic_string<OLECHAR>(serviceName)));
	return (i != serviceObjects_.end()) ? ((*serviceObject = i->second)->AddRef(), S_OK) : S_FALSE;
}

/***/
void ServiceProvider::registerService(const basic_string<OLECHAR>& name, IDispatch& object) {
	if(name.empty())
		throw invalid_argument("name");
	if(serviceObjects_.find(name) != serviceObjects_.end())
		throw invalid_argument("name");
	serviceObjects_.insert(make_pair(name, &object));
	object.AddRef();
}
