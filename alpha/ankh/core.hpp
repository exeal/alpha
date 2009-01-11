/**
 * @file core.hpp
 * @author exeal
 * @date 2006-2007
 */

#ifndef ANKH_CORE_HPP
#define ANKH_CORE_HPP
#include "../../manah/com/unknown-impl.hpp"
#include "../../manah/com/dispatch-impl.hpp"
#include "ankh.h"
#include <activscp.h>
#include <dispex.h>
#include <shlwapi.h>
#include <set>
#include <map>
#include <vector>

#define AnkhObjectBase(itf, safety)												\
	manah::com::ole::IDispatchImpl<itf,											\
		manah::com::ole::PathTypeLibTypeInfoHolder<AnkhTypeLibPath, &IID_##itf>	\
	>,																			\
	public manah::com::IObjectSafetyImpl<INTERFACESAFE_FOR_UNTRUSTED_CALLER, safety>

#define AnkhSafeObjectBase(itf)		AnkhObjectBase(itf, INTERFACESAFE_FOR_UNTRUSTED_CALLER)
#define AnkhUnsafeObjectBase(itf)	AnkhObjectBase(itf, 0)

#define MAKE_WIN32_ERROR_HRESULT(code)	MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, code)

namespace alpha {
	namespace ankh {

		class ScriptSystem;
		class Namespace;

		inline int compareAutomationNames(const std::wstring& lhs, const std::wstring& rhs) throw() {
			return ::CompareStringW(LOCALE_NEUTRAL,
				NORM_IGNORECASE | NORM_IGNOREKANATYPE | NORM_IGNORENONSPACE | NORM_IGNOREWIDTH,
				lhs.data(), static_cast<int>(lhs.length()), rhs.data(), static_cast<int>(rhs.length()));
		}
		struct AutomationNameComparison : public std::binary_function<std::wstring, std::wstring, bool> {
			bool operator ()(const std::wstring& lhs, const std::wstring& rhs) const throw() {return compareAutomationNames(lhs, rhs) == CSTR_LESS_THAN;}
		};
		struct CLSIDComparison : public std::binary_function<CLSID, CLSID, bool> {
			bool operator ()(const CLSID& lhs, const CLSID& rhs) const throw() {
				return std::memcmp(&lhs, &rhs, sizeof(CLSID)) < 0;
			}
		};
		struct AnkhTypeLibPath {static const OLECHAR* getPath() throw() {return OLESTR("Ankh.tlb");}};

		/// Implements @c INamedArguments interface.
		class NamedArguments : public AnkhSafeObjectBase(INamedArguments) {
		public:
			// IUnknown
			IMPLEMENT_UNKNOWN_MULTI_THREADED()
			BEGIN_INTERFACE_TABLE()
				IMPLEMENTS_LEFTMOST_INTERFACE(INamedArguments)
				IMPLEMENTS_INTERFACE(IDispatch)
			END_INTERFACE_TABLE()
			// INamedArguments
			STDMETHODIMP	get__NewEnum(IUnknown** enumerator);
			STDMETHODIMP	get_Item(BSTR switchString, VARIANT** value);
			STDMETHODIMP	get_length(long* count);
			STDMETHODIMP	Count(long* count);
			STDMETHODIMP	Exists(BSTR switchString, VARIANT_BOOL* exists);
		};

		/// Implements @c IUnnamedArguments interface.
		class UnnamedArguments : public AnkhSafeObjectBase(IUnnamedArguments) {
		public:
			// IUnknown
			IMPLEMENT_UNKNOWN_MULTI_THREADED()
			BEGIN_INTERFACE_TABLE()
				IMPLEMENTS_LEFTMOST_INTERFACE(IUnnamedArguments)
				IMPLEMENTS_INTERFACE(IDispatch)
			END_INTERFACE_TABLE()
			// IUnnamedArguments
			STDMETHODIMP	get__NewEnum(IUnknown** enumerator);
			STDMETHODIMP	get_Item(long index, VARIANT** value);
			STDMETHODIMP	get_length(long* count);
			STDMETHODIMP	Count(long* count);
		};

		/// Implements @c IArguments interface.
		class Arguments : public AnkhSafeObjectBase(IArguments) {
		public:
			Arguments(const std::vector<std::wstring>& arguments);
			// IUnknown
			IMPLEMENT_UNKNOWN_MULTI_THREADED()
			BEGIN_INTERFACE_TABLE()
				IMPLEMENTS_LEFTMOST_INTERFACE(IArguments)
				IMPLEMENTS_INTERFACE(IDispatch)
			END_INTERFACE_TABLE()
			// IArguments
			STDMETHODIMP	get__NewEnum(IUnknown** enumerator);
			STDMETHODIMP	get_Item(long index, VARIANT** value);
			STDMETHODIMP	get_length(long* count);
			STDMETHODIMP	get_Named(INamedArguments** named);
			STDMETHODIMP	get_Unnamed(IUnnamedArguments** unnamed);
			STDMETHODIMP	Count(long* count);
			STDMETHODIMP	ShowUsage();
		private:
			std::vector<std::wstring> arguments_;
		};

		/// A script host implements @c IScriptHost interface.
		class ScriptHost :
				public AnkhUnsafeObjectBase(IScriptHost),
				public manah::com::ISupportErrorInfoImpl<&IID_IScriptHost>,
				virtual public IActiveScriptSite,
				virtual public IActiveScriptSiteWindow,
				virtual public IActiveScriptSiteInterruptPoll,
				virtual public IServiceProvider/*,
				virtual public IActiveScriptSiteDebug */ {
		public:
			// constructors
			ScriptHost(ScriptSystem& scriptSystem, IActiveScript& scriptEngine, HWND ownerWindow = 0);
			~ScriptHost();
			// attributes
			manah::com::ComPtr<IActiveScript>	getScriptEngine() const;
			// operations
			HRESULT	closeEngine() throw();
			HRESULT	connectObject(IDispatch& source, const BSTR prefix, const CLSID& coclassID = CLSID_NULL);
			HRESULT	connectObject(IDispatch& source, IDispatch& sink, const CLSID& coclassID = CLSID_NULL);
			HRESULT invokeTopLevelEntity(const OLECHAR* name, WORD type,
						LCID locale = LOCALE_USER_DEFAULT, DISPPARAMS* params = 0, VARIANT* result = 0, EXCEPINFO* exception = 0);
			bool	loadScript(const WCHAR* fileName);
			// IUnknown
			IMPLEMENT_UNKNOWN_MULTI_THREADED()
			BEGIN_INTERFACE_TABLE()
				IMPLEMENTS_LEFTMOST_INTERFACE(IScriptHost)
				IMPLEMENTS_INTERFACE(IDispatch)
				IMPLEMENTS_INTERFACE(ISupportErrorInfo)
				IMPLEMENTS_INTERFACE(IActiveScriptSite)
				IMPLEMENTS_INTERFACE(IActiveScriptSiteWindow)
				IMPLEMENTS_INTERFACE(IActiveScriptSiteInterruptPoll)
				IMPLEMENTS_INTERFACE(IServiceProvider)
//				IMPLEMENTS_INTERFACE(IActiveScriptSiteDebug)
			END_INTERFACE_TABLE()
			// IScriptHost
			STDMETHODIMP			get_Application(IDispatch** application);
			STDMETHODIMP			get_Arguments(IArguments** arguments);
			STDMETHODIMP			get_BuildVersion(int* version);
			STDMETHODIMP			get_FullName(BSTR* name);
			STDMETHODIMP			get_Interactive(VARIANT_BOOL* interactive);
			STDMETHODIMP			put_Interactive(VARIANT_BOOL interactive);
			STDMETHODIMP			get_Name(BSTR* name);
			STDMETHODIMP			get_Path(BSTR* path);
			virtual STDMETHODIMP	get_ScriptFullName(BSTR* name);
			virtual STDMETHODIMP	get_ScriptName(BSTR* name);
			STDMETHODIMP			get_StdErr(IDispatch** stdErr);
			STDMETHODIMP			get_StdIn(IDispatch** stdIn);
			STDMETHODIMP			get_StdOut(IDispatch** stdOut);
			STDMETHODIMP			get_Timeout(long* timeout);
			STDMETHODIMP			put_Timeout(long timeout);
			STDMETHODIMP			get_Version(BSTR* version);
			STDMETHODIMP			ConnectObject(IDispatch* eventSource, BSTR prefix);
			STDMETHODIMP			ConnectObjectEx(IDispatch* eventSource, IDispatch* eventSink);
			STDMETHODIMP			CreateObject(BSTR progID, BSTR prefix, IDispatch** newObject);
			STDMETHODIMP			DisconnectObject(IDispatch* eventSource);
			STDMETHODIMP			DisconnectObjectEx(IDispatch* eventSource, IDispatch* eventSink);
			STDMETHODIMP			Echo(SAFEARRAY* arguments);
			STDMETHODIMP			GetObject(BSTR pathName, BSTR progID, BSTR prefix, IDispatch** newObject);
			virtual STDMETHODIMP	Quit(int exitCode);
			STDMETHODIMP			Sleep(long time);
			// IActiveScriptSite
			STDMETHODIMP	GetLCID(LCID* plcid);
			STDMETHODIMP	GetItemInfo(LPCOLESTR pstrName,
								DWORD dwReturnMask, IUnknown** ppiunkItem, ITypeInfo** ppti);
			STDMETHODIMP	GetDocVersionString(BSTR* pbstrVersion);
			STDMETHODIMP	OnScriptTerminate(const VARIANT* pvarResult, const EXCEPINFO* pexcepinfo);
			STDMETHODIMP	OnStateChange(SCRIPTSTATE ssScriptState);
			STDMETHODIMP	OnScriptError(IActiveScriptError* pscripterror);
			STDMETHODIMP	OnEnterScript();
			STDMETHODIMP	OnLeaveScript();
			// IActiveScriptSiteWindow
			STDMETHODIMP	GetWindow(HWND* phwnd);
			STDMETHODIMP	EnableModeless(BOOL fEnable);
			// IActiveScriptSiteInterruptPoll
			STDMETHODIMP	QueryContinue();
			// IServiceProvider
			STDMETHODIMP	QueryService(REFGUID guidService, REFIID riid, void** ppvObject);
		private:
			DWORD	verifyObjectCreation(const CLSID& clsid);
			DWORD	verifyObjectRunning(IDispatch& object, const CLSID& clsid);
		private:
			class LegacyAdhocEventSink;
			class AdhocEventSink;
			typedef std::map<IDispatch*, LegacyAdhocEventSink*> LegacyEventConnections;
			typedef std::multimap<IDispatch*, AdhocEventSink*> EventConnections;
			ScriptSystem& scriptSystem_;
			IActiveScript& scriptEngine_;
			HWND ownerWindow_;
			DWORD_PTR lastScriptCookie_;	// tag values for loaded scripts
			std::map<DWORD_PTR, std::basic_string<WCHAR> > loadedScripts_;
			long timeout_;
			LegacyEventConnections legacyEventConnections_;
			EventConnections eventConnections_;
			static const wchar_t*		NAME;
			static const unsigned short	MAJOR_VERSION;
			static const unsigned short	MINOR_VERSION;
			static const unsigned short	BUILD_NUMBER;
		};

		/// A script host handles only one input file.
		class FileBoundScriptHost : public ScriptHost {
		public:
			FileBoundScriptHost(const WCHAR* fileName, ScriptSystem& scriptSystem, IActiveScript& scriptEngine, HWND ownerWindow = 0);
			STDMETHODIMP	get_ScriptFullName(BSTR* name);
			STDMETHODIMP	get_ScriptName(BSTR* name);
			STDMETHODIMP	Quit(int exitCode);
		private:
			std::basic_string<WCHAR> fileName_;
		};

		/// A script system central implements @c IScriptSystem.
		class ScriptSystem :
				public AnkhUnsafeObjectBase(IScriptSystem),
				virtual public IInternetHostSecurityManager {
		public:
			// constructors
			ScriptSystem();
			~ScriptSystem();
			// attributes
			void	addEngineScriptNameAssociation(const WCHAR* filePattern, const CLSID& engineID);
			void	enableCrossEngineTopLevelAccesses(bool enable = true);
			// operations
			void			addTopLevelObject(const OLECHAR* name, IDispatch& object);
			bool			associateEngine(const WCHAR* fileName, CLSID& clsid) const;
			static HRESULT	callAnonymousFunction(IDispatch& function,
								LCID locale = LOCALE_USER_DEFAULT, DISPPARAMS* params = 0, VARIANT* result = 0, EXCEPINFO* exception = 0);
			bool			getTopLevelObject(const OLECHAR* name, manah::com::ComPtr<IDispatch>& object) const throw();
			HRESULT 		invokeTopLevelEntity(const OLECHAR* name, const CLSID& preferedLanguage, WORD type,
								LCID locale = LOCALE_USER_DEFAULT, DISPPARAMS* params = 0, VARIANT* result = 0, EXCEPINFO* exception = 0);
			bool			isInteractive() const throw();
			bool			loadConstants(ITypeLib& typeLibrary, const GUID& guid = GUID_NULL);
			void			releaseTopLevelObjects();
			bool			runScriptFile(const WCHAR* fileName);
			void			setInteractive(bool interactive);
			void			shutdown();
			// IUnknown
			IMPLEMENT_UNKNOWN_MULTI_THREADED()
			BEGIN_INTERFACE_TABLE()
				IMPLEMENTS_LEFTMOST_INTERFACE(IScriptSystem)
				IMPLEMENTS_INTERFACE(IDispatch)
				IMPLEMENTS_INTERFACE(IInternetHostSecurityManager)
			END_INTERFACE_TABLE()
			// IInternetHostSecurityManager
			STDMETHODIMP	GetSecurityId(BYTE* pbSecurityId, DWORD* pcbSecurityId, DWORD_PTR dwReserved);
			STDMETHODIMP	ProcessUrlAction(DWORD dwAction, BYTE* pPolicy, DWORD cbPolicy,
								BYTE* pContext, DWORD cbContext, DWORD dwFlags, DWORD dwReserved);
			STDMETHODIMP	QueryCustomPolicy(REFGUID guidKey, BYTE** ppPolicy,
								DWORD* pcbPolicy, BYTE* pContext, DWORD cbContext, DWORD dwReserved);
			// IScriptSystem
			STDMETHODIMP	get_Gns(INamespace** namespaceObject);
			STDMETHODIMP	get_SecurityLevel(short* level);
			STDMETHODIMP	put_SecurityLevel(short level);
			STDMETHODIMP	ExecuteScript(BSTR fileName);
			STDMETHODIMP	IsScriptFileLoaded(BSTR fileName, VARIANT_BOOL* loaded);
			STDMETHODIMP	LoadConstants(VARIANT* libraryNameOrObject, BSTR itemName);
			STDMETHODIMP	LoadScript(BSTR fileName);
		private:
			static bool	associateEngineFromRegistry(const WCHAR* fileName, CLSID& clsid);
			HRESULT		launchNewEngine(const CLSID& engineID, manah::com::ComPtr<ScriptHost>& newHost, bool addToList);
			bool		resolveScriptFileName(const WCHAR* fileName, WCHAR* result) const;
			struct EngineAssociation {
				EngineAssociation(const std::basic_string<WCHAR>& pattern, const CLSID& engineID) : filePattern(pattern), clsid(engineID) {}
				const std::basic_string<WCHAR> filePattern;
				const CLSID clsid;
			};
			typedef std::set<EngineAssociation*> EngineAssociationMap;
			EngineAssociationMap engineAssociations_;
			typedef std::map<CLSID, ScriptHost*, CLSIDComparison> ScriptHosts;
			ScriptHosts scriptHosts_;
			typedef std::map<std::basic_string<OLECHAR>, IDispatch*, AutomationNameComparison> MemberTable;
			MemberTable topLevelObjects_;
			bool interactive_;
			unsigned short securityLevel_;
			bool crossEngineTopLevelAccessesEnabled_;
			manah::com::ComPtr<INamespace> globalNamespace_;
		};

		/// A namespace implements @c INamespace.
		class Namespace : public AnkhSafeObjectBase(INamespace) {
		public:
			// constructors
			Namespace(const wchar_t* name, Namespace* parent);
			~Namespace();
			// IUnknown
			IMPLEMENT_UNKNOWN_MULTI_THREADED()
			BEGIN_INTERFACE_TABLE()
				IMPLEMENTS_LEFTMOST_INTERFACE(INamespace)
				IMPLEMENTS_INTERFACE(IDispatch)
			END_INTERFACE_TABLE()
			// INamespace
			STDMETHODIMP	get__NewEnum(IUnknown** enumerator);
			STDMETHODIMP	get_Child(BSTR name, INamespace** namespaceObject);
			STDMETHODIMP	get_Defines(BSTR name, VARIANT_BOOL *defined);
			STDMETHODIMP	get_Empty(VARIANT_BOOL* empty);
			STDMETHODIMP	get_Locked(VARIANT_BOOL* locked);
			STDMETHODIMP	get_Member(BSTR name, VARIANT** memberObject);
			STDMETHODIMP	get_Name(BSTR* name);
			STDMETHODIMP	get_NumberOfChildren(long* number);
			STDMETHODIMP	get_NumberOfMembers(long* number);
			STDMETHODIMP	get_Parent(INamespace** parent);
			STDMETHODIMP	AddChild(BSTR name, INamespace** newNamespace);
			STDMETHODIMP	AddMember(BSTR name, VARIANT* entity);
			STDMETHODIMP	Clear();
			STDMETHODIMP	Lock(long* cookie);
			STDMETHODIMP	RemoveChild(BSTR name);
			STDMETHODIMP	RemoveMember(BSTR name);
			STDMETHODIMP	Unlock(long cookie);
			STDMETHODIMP	Unwatch(IDispatch* watcher);
			STDMETHODIMP	Watch(BSTR memberName, IDispatch *watcher);
		private:
			const std::wstring name_;
			Namespace* parent_;	// weak reference
			std::map<std::wstring, Namespace*, AutomationNameComparison> children_;
			std::map<std::wstring, ::VARIANT, AutomationNameComparison> members_;
			long lockingCookie_;
			std::set<std::pair<IDispatch*, ::DISPID> > watchers_;
		};


		/// Returns true if the interactive move is active.
		inline bool ScriptSystem::isInteractive() const throw() {return interactive_;}

		/// Activates/deactivates the interactive mode.
		inline void ScriptSystem::setInteractive(bool interactive) throw() {interactive_ = interactive;}

	}
} // namespace alpha.ankh

#endif /* !ANKH_CORE_HPP */
