/**
 * @file win32cp.cpp
 * Defines encoder classes implement Windows code pages.
 * @author exeal
 * @date 2007
 */

#ifdef ASCENSION_WINDOWS

namespace {
	const pair<MIBenum, uint> MIBtoWinCP[] = {
		make_pair(3,	20127),	// US-ASCII
		make_pair(4,	28591),	// ISO-8859-1
		make_pair(5,	28592),	// ISO-8859-2
		make_pair(6,	28593),	// ISO-8859-3
		make_pair(7,	28594),	// ISO-8859-4
		make_pair(8,	28595),	// ISO-8859-5
		make_pair(9,	28596),	// ISO-8859-6
		make_pair(10,	28597),	// ISO-8859-7
		make_pair(11,	28598),	// ISO-8859-8
		make_pair(12,	28599),	// ISO-8859-9
		make_pair(13,	28600),	// ISO-8859-10
		make_pair(17,	932),	// Shift_JIS <-> 2024 Windows-31J
		make_pair(18,	51932),	// EUC-JP
		make_pair(37,	50225),	// ISO-2022-KR
		make_pair(38,	51949),	// EUC-KR
		make_pair(39,	50220),	// ISO-2022-JP
		make_pair(40,	20932),	// ISO-2022-JP-2
		make_pair(65,	708),	// ASMO_449
		// ?T.61?
		// ?ISO-2022-CN?
		// ?ISO-2022-CN-EXT?
		make_pair(106,	65001),	// UTF-8
		make_pair(109,	28603),	// ISO-8859-13
		make_pair(110,	28604),	// ISO-8859-14
		make_pair(111,	28605),	// ISO-8859-15
		make_pair(112,	28606),	// ISO-8859-16
		make_pair(113,	936),	// GBK
		make_pair(114,	54936),	// GB-18030
		make_pair(1012,	65000),	// UTF-7
		make_pair(1013,	1201),	// UTF-16BE
		make_pair(1014,	1200),	// UTF-16LE
		make_pair(1018,	12001),	// UTF-32BE
		make_pair(1019,	12000),	// UTF-32LE
		make_pair(2009,	850),	// IBM850
		make_pair(2013,	862),	// IBM862
		make_pair(2025,	20936),	// GB2312
		make_pair(2026,	950),	// Big5
		make_pair(2028,	37),	// IBM037
		make_pair(2011,	437),	// IBM437
		make_pair(2044,	500),	// IBM500
		make_pair(2045,	851),	// IBM851
		make_pair(2010,	852),	// IBM852
		make_pair(2046,	855),	// IBM855
		make_pair(2047,	857),	// IBM857
		make_pair(2048,	860),	// IBM860
		make_pair(2049,	861),	// IBM861
		make_pair(2050,	863),	// IBM863
		make_pair(2051,	864),	// IBM864
		make_pair(2052,	865),	// IBM865
		make_pair(2053,	868),	// IBM868
		make_pair(2054,	869),	// IBM869
		make_pair(2055,	870),	// IBM870
		make_pair(2056,	871),	// IBM871
		make_pair(2057,	880),	// IBM880
		make_pair(2058,	891),	// IBM891
		make_pair(2059,	903),	// IBM903
		make_pair(2060,	904),	// IBM904
		make_pair(2061,	905),	// IBM905
		make_pair(2062,	918),	// IBM918
		make_pair(2063,	1026),	// IBM1026
		make_pair(2084,	20866),	// KOI8-R
		make_pair(2085,	52936),	// HZ-GB-2312
		make_pair(2086,	866),	// IBM866
		make_pair(2087,	775),	// IBM775
		make_pair(2088,	21866),	// KOI8-U
		make_pair(2089,	858),	// IBM00858
		// ?IBM00924?
		make_pair(2091,	1140),	// IBM01140
		make_pair(2092,	1141),	// IBM01141
		make_pair(2093,	1142),	// IBM01142
		make_pair(2094,	1143),	// IBM01143
		make_pair(2095,	1144),	// IBM01144
		make_pair(2096,	1145),	// IBM01145
		make_pair(2097,	1146),	// IBM01146
		make_pair(2098,	1147),	// IBM01147
		make_pair(2099,	1148),	// IBM01148
		make_pair(2100,	1149),	// IBM01149
		make_pair(2102,	1047),	// IBM01047
		make_pair(2250,	1250),	// windows-1250
		make_pair(2251,	1251),	// windows-1251
		make_pair(2252,	1252),	// windows-1252
		make_pair(2253,	1253),	// windows-1253
		make_pair(2254,	1254),	// windows-1254
		make_pair(2255,	1255),	// windows-1255
		make_pair(2256,	1256),	// windows-1256
		make_pair(2257,	1257),	// windows-1257
		make_pair(2258,	1258),	// windows-1258
		make_pair(2259,	874),	// TIS-620 <-> IBM874
		// Windows auto detections
		make_pair(EncodingDetector::UNIVERSAL_DETECTOR,	50001),
		make_pair(EncodingDetector::JIS_DETECTOR,		50932),
		make_pair(EncodingDetector::KS_DETECTOR,		50949)
	};
}

/// Returns the Win32 code page corresponds to the given MIBenum value.
uint encoding::convertMIBtoWinCP(MIBenum mib) throw() {
	for(size_t i = 0; i < countof(MIBtoWinCP); ++i)
		if(MIBtoWinCP[i].first == mib)
			return MIBtoWinCP[i].second;
	return 0;
}

/// Returns the MIBenum value corresponds to the given Win32 code page.
MIBenum encoding::convertWinCPtoMIB(uint codePage) throw() {
	for(size_t i = 0; i < countof(MIBtoWinCP); ++i)
		if(MIBtoWinCP[i].second == codePage)
			return MIBtoWinCP[i].first;
	return 0;
}


// WindowsEncoder ///////////////////////////////////////////////////////////

namespace {
	class WindowsEncoder : public Encoder {
	public:
		WindowsEncoder(::UINT codePage, MIBenum mib) throw();
		static std::string	getDisplayName(::UINT codePage) throw();
	private:
		Result		doFromUnicode(uchar* to, uchar* toEnd, uchar*& toNext,
						const Char* from, const Char* fromEnd, const Char*& fromNext, State*) const;
		Result		doToUnicode(Char* to, Char* toEnd, Char*& toNext,
						const uchar* from, const uchar* fromEnd, const uchar*& fromNext, State*) const;
		std::string	aliases() const throw();
		std::size_t	maximumNativeBytes() const throw();
		MIBenum		mibEnum() const throw();
		std::string	name() const throw();
	private:
	private:
		const ::UINT codePage_;
		const MIBenum mib_;
	};
} // namespace @0

/**
 * Constructor.
 * @param codePage the code page
 * @param mib the MIBenum value
 */
WindowsEncoder::WindowsEncoder(::UINT codePage, MIBenum mib) throw() : codePage_(codePage), mib_(mib) {
}

/// @see Encoder#doFromUnicode
Encoder::Result WindowsEncoder::doFromUnicode(uchar* to, uchar* toEnd, uchar*& toNext,
		const Char* from, const Char* fromEnd, const Char*& fromNext, State*) const {
	static const ::WCHAR defaultCharacters[] = L"?";
	const ::HRESULT enteredApartment = ::CoInitializeEx(0, COINIT_APARTMENTTHREADED);
	::IMultiLanguage2* mlang;
	if(S_OK == ::CoCreateInstance(::CLSID_CMultiLanguage, 0, CLSCTX_INPROC, ::IID_IMultiLanguage2, reinterpret_cast<void**>(&mlang))) {
		::DWORD mode = 0;
		::UINT sourceSize = static_cast<::UINT>(fromEnd - from), destinationSize = static_cast<::UINT>(toEnd - to);
		if(S_OK == mlang->ConvertStringFromUnicodeEx(&mode, codePage_,
				const_cast<Char*>(from), &sourceSize, reinterpret_cast<char*>(to), &destinationSize,
				(policy() == REPLACE_UNMAPPABLE_CHARACTER) ? MLCONVCHARF_USEDEFCHAR : 0,
				(policy()== REPLACE_UNMAPPABLE_CHARACTER) ? const_cast<::WCHAR*>(defaultCharacters) : 0)) {
			mlang->Release();
			if(SUCCEEDED(enteredApartment))
				::CoUninitialize();
			fromNext = from + sourceSize;
			toNext = to + destinationSize;
			if(fromNext == fromEnd)
				return COMPLETED;
			else if(toNext == toEnd)
				return INSUFFICIENT_BUFFER;
			else
				return (policy() == REPLACE_UNMAPPABLE_CHARACTER
					|| policy() == IGNORE_UNMAPPABLE_CHARACTER) ? MALFORMED_INPUT : UNMAPPABLE_CHARACTER;
		}
		mlang->Release();
	}
	if(SUCCEEDED(enteredApartment))
		::CoUninitialize();
	return (policy() == REPLACE_UNMAPPABLE_CHARACTER) ? MALFORMED_INPUT : UNMAPPABLE_CHARACTER;
}

/// @see Encoder#doToUnicode
Encoder::Result WindowsEncoder::doToUnicode(Char* to, Char* toEnd, Char*& toNext,
		const uchar* from, const uchar* fromEnd, const uchar*& fromNext, State*) const {
	const ::HRESULT enteredApartment = ::CoInitializeEx(0, COINIT_APARTMENTTHREADED);
	::IMultiLanguage2* mlang;
	if(S_OK == ::CoCreateInstance(::CLSID_CMultiLanguage, 0, CLSCTX_INPROC, ::IID_IMultiLanguage2, reinterpret_cast<void**>(&mlang))) {
		::DWORD mode = 0;
		::UINT sourceSize = static_cast<::UINT>(fromEnd - from), destinationSize = static_cast<::UINT>(toEnd - to);
		if(S_OK == mlang->ConvertStringToUnicodeEx(&mode, codePage_,
				reinterpret_cast<char*>(const_cast<uchar*>(from)), &sourceSize, to, &destinationSize, 0, 0)) {
			mlang->Release();
			if(SUCCEEDED(enteredApartment))
				::CoUninitialize();
			fromNext = from + sourceSize;
			toNext = to + destinationSize;
			if(fromNext == fromEnd)
				return COMPLETED;
			return (toNext == toEnd) ? INSUFFICIENT_BUFFER : UNMAPPABLE_CHARACTER;
		}
		mlang->Release();
	}
	if(SUCCEEDED(enteredApartment))
		::CoUninitialize();
	return (policy() == REPLACE_UNMAPPABLE_CHARACTER) ? MALFORMED_INPUT : UNMAPPABLE_CHARACTER;
}

/// @see Encoder#aliases
string WindowsEncoder::aliases() const throw() {
	// TODO: not implemented.
	return "";
}

/// Returns the human-readable encoding name of the given Win32 code page.
string WindowsEncoder::getDisplayName(::UINT codePage) throw() {
	const ::HRESULT enteredApartment = ::CoInitializeEx(0, COINIT_APARTMENTTHREADED);
	::IMultiLanguage2* mlang;
	if(S_OK == ::CoCreateInstance(::CLSID_CMultiLanguage, 0, CLSCTX_INPROC, ::IID_IMultiLanguage2, reinterpret_cast<void**>(&mlang))) {
		::MIMECPINFO mcpi;
		if(S_OK == mlang->GetCodePageInfo(codePage, LOCALE_USER_DEFAULT, &mcpi)) {
			string name(wcslen(mcpi.wszWebCharset), 'X');
			for(size_t i = 0; i < name.length(); ++i)
				name[i] = mask8Bit(mcpi.wszWebCharset[i]);
			mlang->Release();
			if(SUCCEEDED(enteredApartment))
				::CoUninitialize();
			return name;
		}
		mlang->Release();
	}
	if(SUCCEEDED(enteredApartment))
		::CoUninitialize();
	static const char format[] = "x-windows-%lu";
	char name[countof(format) + 32];
	sprintf(name, format, codePage);
	return name;
}

/// @see Encoder#maximumNativeBytes
size_t WindowsEncoder::maximumNativeBytes() const throw() {
	::CPINFOEXW cpi;
	return toBoolean(::GetCPInfoExW(codePage_, 0, &cpi)) ? static_cast<uchar>(cpi.MaxCharSize) : 4;
}

/// @see Encoder#mibEnum
MIBenum WindowsEncoder::mibEnum() const throw() {
	return mib_;
}

/// @see Encoder#name
string WindowsEncoder::name() const throw() {
	return getDisplayName(codePage_);
}


// MLangDetector ////////////////////////////////////////////////////////////

namespace {
	class MLangDetector : public EncodingDetector {
	public:
		explicit MLangDetector(MIBenum mib, ::UINT codePage, ::MLDETECTCP flag = ::MLDETECTCP_NONE) :
			EncodingDetector(mib, WindowsEncoder::getDisplayName(codePage)), codePage_(codePage), flag_(flag) {}
	private:
		MIBenum doDetect(const uchar* first, const uchar* last, ptrdiff_t* convertibleBytes) const throw() {
			const ::HRESULT enteredApartment = ::CoInitializeEx(0, COINIT_APARTMENTTHREADED);
			::IMultiLanguage2* mlang;
			if(S_OK == ::CoCreateInstance(::CLSID_CMultiLanguage, 0, CLSCTX_INPROC, ::IID_IMultiLanguage2, reinterpret_cast<void**>(&mlang))) {
				::UINT numberOfCodePages;
				if(S_OK == mlang->GetNumberOfCodePageInfo(&numberOfCodePages)) {
					manah::AutoBuffer<::DetectEncodingInfo> results(new ::DetectEncodingInfo[numberOfCodePages]);
					int bytes = static_cast<int>(last - first);
					int c = numberOfCodePages;
					if(S_OK == mlang->DetectInputCodepage(flag_, codePage_,
							const_cast<char*>(reinterpret_cast<const char*>(first)), &bytes, results.get(), &c)) {
						const ::DetectEncodingInfo* bestScore = 0;
						for(int i = 0; i < c; ++i) {
							if(bestScore == 0 || results[i].nConfidence > bestScore->nConfidence)
								bestScore = &results[i];
						}
						if(bestScore != 0) {
							if(convertibleBytes != 0)
								*convertibleBytes = (last - first) * bestScore->nDocPercent;
							mlang->Release();
							if(SUCCEEDED(enteredApartment))
								::CoUninitialize();
							return convertWinCPtoMIB(bestScore->nCodePage);
						}
					}
				}
				mlang->Release();
			}
			if(SUCCEEDED(enteredApartment))
				::CoUninitialize();
			if(convertibleBytes != 0)
				*convertibleBytes = 0;
			return fundamental::UTF_8;
		}
	private:
		::UINT codePage_;
		::MLDETECTCP flag_;
	};
} // namespace @0


namespace {
	struct WindowsEncoderInstaller {
		WindowsEncoderInstaller() {
			const ::HRESULT enteredApartment = ::CoInitializeEx(0, COINIT_APARTMENTTHREADED);
			::IMultiLanguage* mlang;
			if(S_OK == ::CoCreateInstance(::CLSID_CMultiLanguage, 0, CLSCTX_INPROC, ::IID_IMultiLanguage, reinterpret_cast<void**>(&mlang))) {
				manah::com::ComPtr<::IEnumCodePage> e;
				if(S_OK == mlang->EnumCodePages(MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID, &e)) {
					::MIMECPINFO cpi;
					::ULONG fetched;
					for(e->Reset(); S_OK == e->Next(1, &cpi, &fetched); ) {
						if(const MIBenum mib = convertWinCPtoMIB(cpi.uiCodePage)) {
							if(!Encoder::supports(mib))
								Encoder::registerEncoder(auto_ptr<Encoder>(new WindowsEncoder(cpi.uiCodePage, mib)));
						}
					}
				}
				EncodingDetector::registerDetector(auto_ptr<EncodingDetector>(new MLangDetector(EncodingDetector::UNIVERSAL_DETECTOR, 50001)));
				EncodingDetector::registerDetector(auto_ptr<EncodingDetector>(new MLangDetector(EncodingDetector::JIS_DETECTOR, 50932)));
				EncodingDetector::registerDetector(auto_ptr<EncodingDetector>(new MLangDetector(EncodingDetector::KS_DETECTOR, 50949)));
			}
			mlang->Release();
			if(SUCCEEDED(enteredApartment))
				::CoUninitialize();
		}
	} windowsEncoderInstaller;
}


#endif /* ASCENSION_WINDOWS */
