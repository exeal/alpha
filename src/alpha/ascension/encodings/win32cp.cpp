/**
 * @file win32cp.cpp
 * Defines encoder classes implement Windows code pages.
 * @author exeal
 * @date 2007
 */

#ifdef _WIN32

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

namespace {
	/// 
	class WindowsEncoder : public Encoder {
	protected:
		WindowsEncoder(::UINT codePage, MIBenum mib) : codePage_(codePage), mib_(mib) {}
		::UINT getCodePage() const throw() {return codePage_;}
	private:
		std::string	getAliases() const throw();
		std::size_t	getMaximumNativeBytes() const throw();
		MIBenum		getMIBenum() const throw();
	private:
		const ::UINT codePage_;
		const MIBenum mib_;
	};
	/// Encoder uses Windows NLS.
	class WindowsNLSEncoder : public WindowsEncoder {
		MANAH_UNASSIGNABLE_TAG(WindowsNLSEncoder);
	public:
		WindowsNLSEncoder(::UINT codePage, MIBenum mib);
		WindowsNLSEncoder(const WindowsNLSEncoder& rhs);
	private:
		Result		doFromUnicode(uchar* to, uchar* toEnd, uchar*& toNext,
						const Char* from, const Char* fromEnd, const Char*& fromNext, Policy policy) const;
		Result		doToUnicode(Char* to, Char* toEnd, Char*& toNext,
						const uchar* from, const uchar* fromEnd, const uchar*& fromNext, Policy policy) const;
		std::string	getName() const throw();
	};
	/// Encoder uses Windows MLang.
	class MLangEncoder : public WindowsEncoder {
		MANAH_UNASSIGNABLE_TAG(MLangEncoder);
	public:
		MLangEncoder(::UINT codePage, MIBenum mib);
		MLangEncoder(const MLangEncoder& rhs);
	private:
		Result		doFromUnicode(uchar* to, uchar* toEnd, uchar*& toNext,
						const Char* from, const Char* fromEnd, const Char*& fromNext, Policy policy) const;
		Result		doToUnicode(Char* to, Char* toEnd, Char*& toNext,
						const uchar* from, const uchar* fromEnd, const uchar*& fromNext, Policy policy) const;
		std::string	getName() const throw();
	};
	static manah::com::ComPtr<::IMultiLanguage2> mlang;
	string getMLangEncodingName(::UINT codePage) throw() {
		if(mlang.get() != 0) {
			::MIMECPINFO mcpi;
			if(S_OK == mlang->GetCodePageInfo(codePage, LOCALE_USER_DEFAULT, &mcpi)) {
				string name(wcslen(mcpi.wszWebCharset), 'X');
				for(size_t i = 0; i < name.length(); ++i)
					name[i] = mask8Bit(mcpi.wszWebCharset[i]);
				return name;
			}
		}
		static const char format[] = "x-windows-%lu";
		char name[countof(format) + 32];
		sprintf(name, format, codePage);
		return name;
	}
} // namespace @0


// WindowsEncoder ///////////////////////////////////////////////////////////

/// @see Encoder#getAliases
string WindowsEncoder::getAliases() const throw() {
	// TODO: not implemented.
	return "";
}

/// @see Encoder#getMaximumNativeBytes
size_t WindowsEncoder::getMaximumNativeBytes() const throw() {
	::CPINFOEXW cpi;
	return toBoolean(::GetCPInfoExW(codePage_, 0, &cpi)) ? static_cast<uchar>(cpi.MaxCharSize) : 4;
}

/// @see Encoder#getMIB
MIBenum WindowsEncoder::getMIBenum() const throw() {
	return mib_;
}


// WindowsNLSEncoder ////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param cp the code page
 * @param mib the MIBenum value
 * @throw std#invalid_argument @a cp is not supported by the system
 */
WindowsNLSEncoder::WindowsNLSEncoder(::UINT cp, MIBenum mib) : WindowsEncoder(cp, mib) {
	if(!toBoolean(::IsValidCodePage(cp)))
		throw invalid_argument("the specified code page is not supported.");
}

/// @see Encoder#doFromUnicode
Encoder::Result WindowsNLSEncoder::doFromUnicode(uchar* to, uchar* toEnd, uchar*& toNext,
		const Char* from, const Char* fromEnd, const Char*& fromNext, Policy policy) const {
	if(from == fromEnd) {
		// no conversion
		fromNext = from;
		toNext = to;
		return COMPLETED;
	} else if(const int writtenBytes = ::WideCharToMultiByte(getCodePage(),
			(policy == REPLACE_UNMAPPABLE_CHARACTER ? WC_DEFAULTCHAR : 0),
			from, static_cast<int>(fromEnd - from), reinterpret_cast<char*>(to), static_cast<int>(toEnd - to), 0, 0)) {
		// succeeded (fromNext is not modified)
		toNext = to + writtenBytes;
		return COMPLETED;
	} else {
		::DWORD e = ::GetLastError();
		return (e == ERROR_INSUFFICIENT_BUFFER) ? INSUFFICIENT_BUFFER : UNMAPPABLE_CHARACTER;
	}
}

/// @see Encoder#doToUnicode
Encoder::Result WindowsNLSEncoder::doToUnicode(Char* to, Char* toEnd, Char*& toNext,
		const uchar* from, const uchar* fromEnd, const uchar*& fromNext, Policy policy) const {
	if(from == fromEnd) {
		// no conversion
		fromNext = from;
		toNext = to;
		return COMPLETED;
	} else if(const int writtenChars = ::MultiByteToWideChar(getCodePage(),
			policy == REPLACE_UNMAPPABLE_CHARACTER ? 0 : MB_ERR_INVALID_CHARS,
			reinterpret_cast<const char*>(from), static_cast<int>(fromEnd - from), to, static_cast<int>(toEnd - to))) {
		// succeeded (fromNext is not modified)
		toNext = to + writtenChars;
		return COMPLETED;
	} else
		return (::GetLastError() == ERROR_INSUFFICIENT_BUFFER) ? INSUFFICIENT_BUFFER : UNMAPPABLE_CHARACTER;
}

/// @see Encoder#getName
string WindowsNLSEncoder::getName() const throw() {
	static const char format[] = "x-windows-%lu";
	char name[countof(format) + 32];
	sprintf(name, format, getCodePage());
	return name;
}


// MLangEncoder /////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param cp the code page
 * @param mib the MIBenum value
 * @throw std#invalid_argument @a cp is not supported by the system
 */
MLangEncoder::MLangEncoder(::UINT cp, MIBenum mib) : WindowsEncoder(cp, mib) {
	if(mlang.get() == 0)
		mlang.createInstance(CLSID_CMultiLanguage, 0, CLSCTX_INPROC);
	::MIMECPINFO mci;
	if(S_OK != mlang->GetCodePageInfo(cp, LOCALE_USER_DEFAULT, &mci))
		throw invalid_argument("the specified code page is not supported.");
}

/// @see Encoder#doFromUnicode
Encoder::Result MLangEncoder::doFromUnicode(uchar* to, uchar* toEnd, uchar*& toNext,
		const Char* from, const Char* fromEnd, const Char*& fromNext, Policy policy) const {
	static const ::WCHAR defaultCharacters[] = L"?";
	::DWORD mode = 0;
	::UINT sourceSize = static_cast<::UINT>(fromEnd - from), destinationSize = static_cast<::UINT>(toEnd - to);
	if(S_OK == mlang->ConvertStringFromUnicodeEx(&mode, getCodePage(),
			const_cast<Char*>(from), &sourceSize, reinterpret_cast<char*>(to), &destinationSize,
			(policy == REPLACE_UNMAPPABLE_CHARACTER) ? MLCONVCHARF_USEDEFCHAR : 0,
			(policy == REPLACE_UNMAPPABLE_CHARACTER) ? const_cast<::WCHAR*>(defaultCharacters) : 0)) {
		fromNext = from + sourceSize;
		toNext = to + destinationSize;
		if(fromNext == fromEnd)
			return COMPLETED;
		else if(toNext == toEnd)
			return INSUFFICIENT_BUFFER;
		else
			return (policy == REPLACE_UNMAPPABLE_CHARACTER
				|| policy == IGNORE_UNMAPPABLE_CHARACTER) ? MALFORMED_INPUT : UNMAPPABLE_CHARACTER;
	} else
		return (policy == REPLACE_UNMAPPABLE_CHARACTER) ? MALFORMED_INPUT : UNMAPPABLE_CHARACTER;
}

/// @see Encoder#doToUnicode
Encoder::Result MLangEncoder::doToUnicode(Char* to, Char* toEnd, Char*& toNext,
		const uchar* from, const uchar* fromEnd, const uchar*& fromNext, Policy policy) const {
	::DWORD mode = 0;
	::UINT sourceSize = static_cast<::UINT>(fromEnd - from), destinationSize = static_cast<::UINT>(toEnd - to);
	if(S_OK == mlang->ConvertStringToUnicodeEx(&mode, getCodePage(),
			reinterpret_cast<char*>(const_cast<uchar*>(from)), &sourceSize, to, &destinationSize, 0, 0)) {
		fromNext = from + sourceSize;
		toNext = to + destinationSize;
		if(fromNext == fromEnd)
			return COMPLETED;
		return (toNext == toEnd) ? INSUFFICIENT_BUFFER : UNMAPPABLE_CHARACTER;
	} else
		return UNMAPPABLE_CHARACTER;
}

/// @see Encoder#getName
string MLangEncoder::getName() const throw() {
	return getMLangEncodingName(getCodePage());
}


// MLangDetector ////////////////////////////////////////////////////////////

namespace {
	class MLangDetector : public EncodingDetector {
	public:
		explicit MLangDetector(MIBenum mib, ::UINT codePage, ::MLDETECTCP flag = ::MLDETECTCP_NONE) :
			EncodingDetector(mib, getMLangEncodingName(codePage)), codePage_(codePage), flag_(flag) {}
	private:
		MIBenum doDetect(const uchar* first, const uchar* last, ptrdiff_t* convertibleBytes) const throw() {
			if(mlang.get() != 0) {
				::HRESULT hr;
				::UINT numberOfCodePages;
				if(SUCCEEDED(hr = mlang->GetNumberOfCodePageInfo(&numberOfCodePages))) {
					manah::AutoBuffer<::DetectEncodingInfo> results(new ::DetectEncodingInfo[numberOfCodePages]);
					int bytes = static_cast<int>(last - first);
					int c = numberOfCodePages;
					if(SUCCEEDED(hr = mlang->DetectInputCodepage(flag_, codePage_,
							const_cast<char*>(reinterpret_cast<const char*>(first)), &bytes, results.get(), &c))) {
						const ::DetectEncodingInfo* bestScore = 0;
						for(int i = 0; i < c; ++i) {
							if(bestScore == 0 || results[i].nConfidence > bestScore->nConfidence)
								bestScore = &results[i];
						}
						if(bestScore != 0) {
							if(convertibleBytes != 0)
								*convertibleBytes = (last - first) * bestScore->nDocPercent;
							return convertWinCPtoMIB(bestScore->nCodePage);
						}
					}
				}
			}
			if(convertibleBytes != 0)
				*convertibleBytes = 0;
			return fundamental::MIB_UNICODE_UTF8;
		}
	private:
		::UINT codePage_;
		::MLDETECTCP flag_;
	};
} // namespace @0


namespace {
	struct WindowsEncoderInstaller {
		WindowsEncoderInstaller() {
			::EnumSystemCodePagesW(procedure, CP_INSTALLED);

			::HRESULT hr, enteredApartment = ::CoInitialize(0);
			{
				manah::com::ComPtr<::IMultiLanguage> mlang1;
				if(SUCCEEDED(hr = mlang1.createInstance(::CLSID_CMultiLanguage, 0, CLSCTX_INPROC, ::IID_IMultiLanguage))) {
					manah::com::ComPtr<IEnumCodePage> e;
					if(SUCCEEDED(hr = mlang1->EnumCodePages(MIMECONTF_IMPORT | MIMECONTF_EXPORT | MIMECONTF_VALID, &e))) {
						::MIMECPINFO cpi;
						::ULONG fetched;
						for(e->Reset(); S_OK == (hr = e->Next(1, &cpi, &fetched)); ) {
							if(const MIBenum mib = convertWinCPtoMIB(cpi.uiCodePage)) {
								if(!Encoder::supports(mib))
									Encoder::registerEncoder(auto_ptr<Encoder>(new MLangEncoder(cpi.uiCodePage, mib)));
							}
						}
					}
				}
				EncodingDetector::registerDetector(auto_ptr<EncodingDetector>(new MLangDetector(EncodingDetector::UNIVERSAL_DETECTOR, 50001)));
				EncodingDetector::registerDetector(auto_ptr<EncodingDetector>(new MLangDetector(EncodingDetector::JIS_DETECTOR, 50932)));
				EncodingDetector::registerDetector(auto_ptr<EncodingDetector>(new MLangDetector(EncodingDetector::KS_DETECTOR, 50949)));
			}
			if(enteredApartment == S_OK || enteredApartment == S_FALSE)
				::CoUninitialize();
		}
		static ::BOOL CALLBACK procedure(::LPWSTR name) {
			const ::UINT cp = wcstoul(name, 0, 10);
			if(toBoolean(::IsValidCodePage(cp))) {
				::CPINFOEXW cpi;
				if(toBoolean(::GetCPInfoExW(cp, 0, &cpi))) {
					if(const MIBenum mib = convertWinCPtoMIB(cp)) {
						if(!Encoder::supports(mib))
							Encoder::registerEncoder(auto_ptr<Encoder>(new WindowsNLSEncoder(cp, mib)));
					}
				}
			}
			return TRUE;
		}
	} windowsEncoderInstaller;
}


#endif /* _WIN32 */
