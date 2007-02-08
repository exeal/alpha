/**
 * @file encoder.cpp
 * @author exeal
 * @date 2004-2006
 */

#include "StdAfx.h"
#include "encoder.hpp"
#include <algorithm>

using namespace ascension::encodings;
using namespace std;


DEFINE_DETECTOR(CPEX_AUTODETECT_SYSTEMLANG, SystemLang);
DEFINE_DETECTOR(CPEX_AUTODETECT_USERLANG, UserLang);


namespace {
	set<CodePage>* working;

	BOOL CALLBACK enumCodePagesProc(LPWSTR name) {
		const CodePage cp = wcstoul(name, 0, 10);
		if(toBoolean(::IsValidCodePage(cp)))
			working->insert(cp);
		return TRUE;
	}

	void detectCodePage_SystemLang(const uchar*, size_t, CodePage&, size_t&) {
		assert(false);
	}
	void detectCodePage_UserLang(const uchar*, size_t, CodePage&, size_t&) {
		assert(false);
	}
} // namespace `anonymous'


/**
 * Creates a new encoder.
 * @param cp the code page of the encoder
 * @return the encoder or @c null if @a cp is not valid
 */
auto_ptr<Encoder> EncoderFactory::createEncoder(CodePage cp) {
	EncoderMap::iterator it = registeredEncoders_.find(cp);
	if(it != registeredEncoders_.end())
		return it->second();
	else {
		try {
			return auto_ptr<Encoder>(new WindowsEncoder(cp));
		} catch(invalid_argument&) {
			return auto_ptr<Encoder>(0);
		}
	}
}

/**
 * Detects the code page of the string buffer.
 * @param src the string
 * @param length the length of the string (in bytes)
 * @param cp the code page
 * @return the detected code page
 */
CodePage EncoderFactory::detectCodePage(const uchar* src, size_t length, CodePage cp) {
	assert(src != 0);

	if(!isCodePageForAutoDetection(cp))
		return cp;

	CodePage result;
	size_t score;

	// 透過的に言語を選択する場合
	if(cp == CPEX_AUTODETECT_SYSTEMLANG || cp == CPEX_AUTODETECT_USERLANG) {
		const LANGID langID = (cp == CPEX_AUTODETECT_SYSTEMLANG) ? ::GetSystemDefaultLangID() : ::GetUserDefaultLangID();
		switch(PRIMARYLANGID(langID)) {
		case LANG_ARMENIAN:	cp = CPEX_ARMENIAN_AUTODETECT;	break;
		case LANG_JAPANESE:	cp = CPEX_JAPANESE_AUTODETECT;	break;
//		case LANG_KOREAN:	cp = CPEX_KOREAN_AUTODETECT;	break;
		default:			cp = CPEX_UNICODE_AUTODETECT;	break;
		}
	}

	DetectorMap::iterator it = registeredDetectors_.find(cp);

	assert(it != registeredDetectors_.end());
	it->second(src, length, result, score);
	return (score != 0) ? result : ::GetACP();
}

/**
 * Lists all available code pages.
 * @param[out] codePages receives all code pages
 */
void EncoderFactory::enumCodePages(set<CodePage>& codePages) const {
	codePages.clear();
	for(EncoderMap::const_iterator it = registeredEncoders_.begin(); it != registeredEncoders_.end(); ++it)
		codePages.insert(it->first);
	for(DetectorMap::const_iterator it = registeredDetectors_.begin(); it != registeredDetectors_.end(); ++it)
		codePages.insert(it->first);
	working = &codePages;
	::EnumSystemCodePagesW(enumCodePagesProc, CP_INSTALLED);
}
