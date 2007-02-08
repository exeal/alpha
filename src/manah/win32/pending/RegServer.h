// RegServer.h
/////////////////////////////////////////////////////////////////////////////

#include <olectl.h>
#include <string>
#include <assert.h>

namespace _Armaiti
{

class CRegServer
{
public:
	static HINSTANCE	m_hInstance;

public:
	static inline HRESULT	DllRegisterServer(
			const char* lpszClassName,	// class name
			REFCLSID rclsid,			// CLSID(GUID)
			const char* lpszProgID) {	// ProgID(XX.XX.XX)
		assert(lpszClassName != 0);
		assert(lpszProgID != 0);

		HRESULT		hr = S_OK;
		char		szFileName[MAX_PATH];
		char		szCLSIDText[40];	// "{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXX}"
		LPOLESTR	lpszCLSIDText;
		std::string	arrEntries[5][3];
		HKEY		hKey;
		long		err;

		lpszCLSIDText = new OLECHAR[40];
		::GetModuleFileNameA(m_hInstance, szFileName, MAX_PATH);
		if(!::StringFromGUID2(rclsid, lpszCLSIDText, 40))
			return E_INVALIDARG;
		wcstombs(szCLSIDText, lpszCLSIDText, 40);
		*(szCLSIDText + 39) = 0;
		delete lpszCLSIDText;
		arrEntries[0][0] = "CLSID\\";	arrEntries[0][0] += szCLSIDText;
		arrEntries[0][1] = "";
		arrEntries[0][2] = lpszClassName;
		arrEntries[1][0] = "CLSID\\";	arrEntries[1][0] += szCLSIDText;	arrEntries[1][0] += "\\InprocServer32";
		arrEntries[1][1] = "";
		arrEntries[1][2] = "/";
		arrEntries[2][0] = "CLSID\\";	arrEntries[2][0] += szCLSIDText;	arrEntries[2][0] += "\\ProgID";
		arrEntries[2][1] = "";
		arrEntries[2][2] = lpszProgID;
		arrEntries[3][0] = lpszProgID;
		arrEntries[3][1] = "";
		arrEntries[3][2] = lpszClassName;
		arrEntries[4][0] = lpszProgID;	arrEntries[4][0] += "\\CLSID";
		arrEntries[4][1] = "";
		arrEntries[4][2] = szCLSIDText;

		for(int i = 0; SUCCEEDED(hr) && i < 5; i++){
			const char* pszKeyName		= arrEntries[i][0].c_str();
			const char* pszValueName	= arrEntries[i][1].c_str();
			const char* pszValue		= arrEntries[i][2].c_str();
			if(*pszValue == '/')
				pszValue = szFileName;
			else if(*pszValue == 0)
				pszValue = NULL;

			err = ::RegCreateKeyA(HKEY_CLASSES_ROOT, pszKeyName, &hKey);
			if(err == ERROR_SUCCESS){
				err = ::RegSetValueExA(hKey, pszValueName, 0,
					REG_SZ, reinterpret_cast<const BYTE*>(pszValue), strlen(pszValue) + 1);
				::RegCloseKey(hKey);
			}
			if(err != ERROR_SUCCESS){
				CRegServer::DllUnregisterServer(lpszClassName, rclsid, lpszProgID);
				hr = SELFREG_E_CLASS;
			}
		}
		return hr;
	}

	static inline HRESULT	DllUnregisterServer(
			const char* lpszClassName,	// class name
			REFCLSID rclsid,			// CLSID(GUID)
			const char* lpszProgID) {	// ProgID(XX.XX.XX)
		HRESULT	hr = S_OK;
		std::string arrKeyNames[5];
		char		szCLSIDText[40];
		LPOLESTR	lpszCLSIDText;

		lpszCLSIDText = new OLECHAR[40];
		::StringFromGUID2(rclsid, lpszCLSIDText, 40);
		wcstombs(szCLSIDText, lpszCLSIDText, 40);
		*(szCLSIDText + 39) = 0;
		delete lpszCLSIDText;
		arrKeyNames[0] = "CLSID\\";	arrKeyNames[0] += szCLSIDText;
		arrKeyNames[1] = "CLSID\\";	arrKeyNames[1] += szCLSIDText;	arrKeyNames[1] += "\\InprocServer32";
		arrKeyNames[2] = "CLSID\\";	arrKeyNames[2] += szCLSIDText;	arrKeyNames[2] += "\\ProgID";
		arrKeyNames[3] = lpszProgID;
		arrKeyNames[4] = lpszProgID;	arrKeyNames[4] += "\\CLSID";

		for(int i = 4; i >= 0; i--){
			if(ERROR_SUCCESS != ::RegDeleteKeyA(HKEY_CLASSES_ROOT, arrKeyNames[i].c_str()))
				hr = S_FALSE;
		}

		return hr;
	}

	static inline HRESULT RegisterTypeLib(
			REFIID libid,
			const OLECHAR* pszFileName = 0,
			const OLECHAR* pszHelpDir = 0) {
		OLECHAR		wszDirName[MAX_PATH];
		wchar_t*	pwsz = 0;
		ITypeLib*	pTypeLib = 0;
		TLIBATTR*	pTLAttr = 0;
		HRESULT		hr;

		::GetModuleFileNameW(m_hInstance, wszDirName, MAX_PATH);
		if(pszFileName != 0) {
			pwsz = wcsrchr(wszDirName, L'\\');
			assert(pwsz != 0);
			wcscpy(pwsz + 1, pszFileName);
		}
		::OutputDebugStringW(wszDirName);
		hr = ::LoadTypeLib(wszDirName, &pTypeLib);
		if(FAILED(hr))
			return hr;
		pTypeLib->GetLibAttr(&pTLAttr);
		assert(pTLAttr != 0);
		assert(::IsEqualGUID(pTLAttr->guid, libid));
		hr = ::RegisterTypeLib(pTypeLib, wszDirName, const_cast<OLECHAR*>(pszHelpDir));

		pTypeLib->ReleaseTLibAttr(pTLAttr);
		pTypeLib->Release();

		return hr;
	}

	static inline HRESULT UnregisterTypeLib(
			REFGUID libid,
			unsigned short wVerMajor = 0,
			unsigned short wVerMinor = 0,
			LCID lcid = LOCALE_USER_DEFAULT) {
		ITypeLib*	pTypeLib = 0;
		TLIBATTR*	pTLAttr = 0;
		HRESULT		hr;

		hr = ::LoadRegTypeLib(libid, wVerMajor, wVerMinor, lcid, &pTypeLib);
		if(FAILED(hr))
			return hr;
		pTypeLib->GetLibAttr(&pTLAttr);
		assert(pTLAttr != 0);
		assert(::IsEqualGUID(pTLAttr->guid, libid));
		hr = ::UnRegisterTypeLib(
			libid, pTLAttr->wMajorVerNum, pTLAttr->wMinorVerNum, pTLAttr->lcid, pTLAttr->syskind);

		pTypeLib->ReleaseTLibAttr(pTLAttr);
		pTypeLib->Release();

		return hr;
	}
};

HINSTANCE CRegServer::m_hInstance = 0;

};