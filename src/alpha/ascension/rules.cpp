/**
 * @file rules.cpp
 * @author exeal
 * @date 2004-2006 (was Lexer.cpp)
 * @date 2006-2007
 */

#include "rules.hpp"
using namespace ascension;
using namespace ascension::kernel;
using namespace ascension::presentation;
using namespace ascension::rules;
using namespace ascension::text;
using namespace std;
using namespace manah;
using rules::internal::HashTable;


namespace {
	template<typename T>
	struct InRange : unary_function<T, bool> {
		InRange(T first, T last) : f(first), l(last) {}
		bool operator()(T v) const throw() {return v >= f && v <= l;}
		T f, l;
	};
}

// internal.HashTable ///////////////////////////////////////////////////////

namespace ascension {
	namespace rules {
		namespace internal {
			class HashTable {
				MANAH_NONCOPYABLE_TAG(HashTable);
			public:
				template<typename StringSequence>
				HashTable(StringSequence first, StringSequence last, bool caseSensitive);
				~HashTable() throw();
				template<typename CharacterSequence>
				static ulong hashCode(CharacterSequence first, CharacterSequence last);
				bool matches(const Char* first, const Char* last) const;
				size_t maximumLength() const throw() {return maxLength_;}
			private:
				struct Entry {
					String data;
					Entry* next;
					explicit Entry(const String& str) throw() : data(str) {}
					~Entry() throw() {delete next;}
				};
				Entry** entries_;
				size_t numberOfEntries_;
				size_t maxLength_;	// the length of the longest keyword
				const bool caseSensitive_;
			};
		}
	}
} // namespace ascension.rules.internal

/**
 * Constructor.
 * @param first the start of the strings
 * @param last the end of the strings
 * @param caseSensitive set true to enable case sensitive match
 */
template<typename StringSequence>
HashTable::HashTable(StringSequence first, StringSequence last, bool caseSensitive)
		: numberOfEntries_(distance(first, last)), maxLength_(0), caseSensitive_(caseSensitive) {
	entries_ = new Entry*[numberOfEntries_];
	fill_n(entries_, numberOfEntries_, static_cast<Entry*>(0));
	while(first != last) {
		const String folded(caseSensitive_ ? *first : CaseFolder::fold(*first));
		const size_t h = hashCode(folded.begin(), folded.end());
		Entry* const newEntry = new Entry(folded);
		if(folded.length() > maxLength_)
			maxLength_ = folded.length();
		newEntry->next = (entries_[h % numberOfEntries_] != 0) ? entries_[h % numberOfEntries_] : 0;
		entries_[h % numberOfEntries_] = newEntry;
		++first;
	}
}

/// Destructor.
HashTable::~HashTable() {
	for(size_t i = 0; i < numberOfEntries_; ++i)
		delete entries_[i];
	delete[] entries_;
}

/**
 * Returns the hash value of the specified string. @a CharacterSequence must represent UTF-16
 * character sequence.
 * @param first the start of the character sequence
 * @param last the end of the character sequence
 * @return the hash value
 */
template<typename CharacterSequence>
inline ulong HashTable::hashCode(CharacterSequence first, CharacterSequence last) {
	ASCENSION_STATIC_ASSERT(sizeof(*first) == 2);
	ulong h = 0;
	while(first < last) {
		h *= 2;
		h += *first;
		++first;
	}
	return h;
}

/**
 * Searches the specified string.
 * @param first the start of the string
 * @param last the end of the string
 * @return true if the specified string is found
 */
bool HashTable::matches(const Char* first, const Char* last) const {
	if(caseSensitive_) {
		if(static_cast<size_t>(last - first) > maxLength_)
			return false;
		const size_t h = hashCode(first, last);
		for(Entry* entry = entries_[h % numberOfEntries_]; entry != 0; entry = entry->next) {
			if(entry->data.length() == static_cast<size_t>(last - first) && wmemcmp(entry->data.data(), first, entry->data.length()) == 0)
				return true;
		}
	} else {
		const String folded(CaseFolder::fold(String(first, last)));
		const size_t h = hashCode(folded.begin(), folded.end());
		for(Entry* entry = entries_[h % numberOfEntries_]; entry != 0; entry = entry->next) {
			if(entry->data.length() == folded.length() && wmemcmp(entry->data.data(), folded.data(), folded.length()) == 0)
				return true;
		}
	}
	return false;
}


// URIDetector //////////////////////////////////////////////////////////////

#if 0
/**
 * 文字列がメールアドレスかを調べる
 * @param first, last 調べる文字列
 * @param asIRI UCS 文字を認めるか (未実装)
 * @return メールアドレスの終端
 */
const Char* URIDetector::eatMailAddress(const Char* first, const Char* last, bool) {
//	p.matches(first, last);
/*
	namespace xp = boost::xpressive;
	static const xp::wcregex pattern = xp::bos
		>> xp::set[xp::_w | xp::_d]
		>> *xp::set[xp::_w | xp::_d | '.' | '-' | '_']
		>> '@'
		>> +xp::set[xp::_w | xp::_d | '-' | '_']
		>> +('.' >> +xp::set[xp::_w | xp::_d | '-' | '_']);
	xp::match_results<const Char*> m;
	if(!xp::regex_search(first, last, m, pattern))
		return first;
	return first + m.length();
*/
	// このメソッドは "/[\w\d][\w\d\.\-_]*@[\w\d\-_]+(?:\.[\w\d\-_]+)+/" のようなパターンマッチを行う
#define IS_ALNUM(ch)					\
	(((ch) >= L'A' && (ch) <= L'Z')		\
	|| ((ch) >= L'a' && (ch) <= L'z')	\
	|| ((ch) >= L'0' && (ch) <= L'9'))
#define IS_ALNUMBAR(ch)	\
	(IS_ALNUM(ch) || ch == L'-' || ch == L'_')

	if(last - first < 5)
		return first;

	// 1 文字目
	if(!IS_ALNUM(*first))
		return first;

	// 2 文字目から '@'
	const Char* const originalFirst = first++;
	for(; first < last - 3; ++first) {
		if(!IS_ALNUMBAR(*first) && *first != L'.')
			break;
	}
	if(*first != L'@' || last - first == 3)
		return originalFirst;

	// '@' の後ろ
	const Char* const atMark = first;
	bool dotAppeared = false;
	for(first = atMark + 1; first < last; ++first) {
		if(IS_ALNUMBAR(*first))
			continue;
		else if(*first == L'.') {
			if(first[-1] == L'.')
				return originalFirst;
			dotAppeared = true;
		} else
			break;
	}
	return (dotAppeared && (first - atMark > 2)) ? first : originalFirst;
}
#endif /* 0 */

namespace {
	// Procedures implement productions of RFC 3986 and RFC 3987.
	// Each procedures take two parameter represent the parsed character sequence. These must be
	// first != null, last != null and first <= last. Return value is the end of parsed sequence.
	// "[nullable]" indicates that the procedure can return empty read sequence.

	// (from RFC2234)
	// ALPHA =  %x41-5A / %x61-7A ; A-Z / a-z
	// DIGIT =  %x30-39           ; 0-9
	const locale& cl = locale::classic();
   
	// sub-delims = "!" / "$" / "&" / "'" / "(" / ")" / "*" / "+" / "," / ";" / "="
	inline const Char* handleSubDelims(const Char* first, const Char* last) {
		return (first < last && wcschr(L"!$&'()*+,;=", *first) != 0) ? ++first : 0;
	}

	// gen-delims = ":" / "/" / "?" / "#" / "[" / "]" / "@"
	inline const Char* handleGenDelims(const Char* first, const Char* last) {
		return (first < last && wcschr(L":/?#[]@", *first) != 0) ? ++first : 0;
	}

	// reserved = gen-delims / sub-delims
	inline const Char* handleReserved(const Char* first, const Char* last) {
		return (handleGenDelims(first, last) != 0 || handleSubDelims(first, last) != 0) ? ++first : 0;
	}

	// unreserved = ALPHA / DIGIT / "-" / "." / "_" / "~"
	inline const Char* handleUnreserved(const Char* first, const Char* last) {
		return (first < last && (isalnum(*first, cl) || wcschr(L"-._~", *first) != 0)) ? ++first : 0;
	}

	// pct-encoded = "%" HEXDIG HEXDIG
	inline const Char* handlePctEncoded(const Char* first, const Char* last) {
		return (last - first >= 3 && first[0] == L'%' && isxdigit(first[1], cl) && isxdigit(first[2], cl)) ? first += 3 : 0;
	}

	// IPv6address =                            6( h16 ":" ) ls32
    //             /                       "::" 5( h16 ":" ) ls32
    //             / [               h16 ] "::" 4( h16 ":" ) ls32
    //             / [ *1( h16 ":" ) h16 ] "::" 3( h16 ":" ) ls32
    //             / [ *2( h16 ":" ) h16 ] "::" 2( h16 ":" ) ls32
    //             / [ *3( h16 ":" ) h16 ] "::"    h16 ":"   ls32
    //             / [ *4( h16 ":" ) h16 ] "::"              ls32
    //             / [ *5( h16 ":" ) h16 ] "::"              h16
    //             / [ *6( h16 ":" ) h16 ] "::"
	const Char* ASCENSION_FASTCALL handleIPv6address(const Char* first, const Char* last) {return 0;}

	// IPvFuture = "v" 1*HEXDIG "." 1*( unreserved / sub-delims / ":" )
	const Char* ASCENSION_FASTCALL handleIPvFuture(const Char* first, const Char* last) {
		if(last - first >= 4 && *first == L'v' && isxdigit(*++first, cl)) {
			while(isxdigit(*first, cl)) {
				if(++first == last)
					return 0;
			}
			if(*first == L'.' && ++first < last) {
				const Char* p = first;
				while(p < last) {
					if(0 != (p = handleUnreserved(p, last)) || (0 != (p = handleSubDelims(p, last))))
						continue;
					else if(*p == L':')
						++p;
					else
						break;
				}
				return (p > first) ? p : 0;
			}
		}
		return 0;
	}

	// IP-literal = "[" ( IPv6address / IPvFuture  ) "]"
	inline const Char* handleIPLiteral(const Char* first, const Char* last) {
		if(first < last && *first == L'[') {
			const Char* p;
			if(0 != (p = handleIPv6address(++first, last)) || 0 != (p = handleIPvFuture(first, last))) {
				if(p < last && *p == L']')
					return ++p;
			}
		}
		return 0;
	}

	// port = *DIGIT
	inline const Char* handlePort(const Char* first, const Char* last) {	// [nullable]
		while(first < last && isdigit(*first, cl))
			++first;
		return first;
	}
	
	// dec-octet = DIGIT             ; 0-9
    //           / %x31-39 DIGIT     ; 10-99
    //           / "1" 2DIGIT        ; 100-199
    //           / "2" %x30-34 DIGIT ; 200-249
    //           / "25" %x30-35      ; 250-255
	const Char* ASCENSION_FASTCALL handleDecOctet(const Char* first, const Char* last) {
		if(first < last) {
			if(*first == L'0')
				return ++first;
			else if(*first == L'1')
				return (++first < last && isdigit(*first, cl) && ++first < last && isdigit(*first, cl)) ? ++first : first;
			else if(*first == L'2') {
				if(++first < last) {
					if(*first >= L'0' && *first <= L'4') {
						if(isdigit(*++first, cl))
							++first;
					} else if(*first == L'5') {
						if(*first >= L'0' && *first <= L'5')
							++first;
					}
				}
				return first;
			} else if(*first >= L'3' && *first <= L'9')
				return (++first < last && isdigit(*first, cl)) ? ++first : first;
		}
		return 0;
	}

	// IPv4address = dec-octet "." dec-octet "." dec-octet "." dec-octet
	inline const Char* handleIPv4address(const Char* first, const Char* last) {
		return (last - first >= 7
			&& 0 != (first = handleDecOctet(first, last))
			&& first < last && *first == L'.'
			&& 0 != (first = handleDecOctet(++first, last))
			&& first < last && *first == L'.'
			&& 0 != (first = handleDecOctet(++first, last))
			&& first < last && *first == L'.'
			&& 0 != (first = handleDecOctet(++first, last))) ? first : 0;
	}

	// h16 = 1*4HEXDIG
	const Char* ASCENSION_FASTCALL handleH16(const Char* first, const Char* last) {
		if(first < last && isxdigit(*first, cl)) {
			const Char* const e = min(++first + 3, last);
			while(first < e && isxdigit(*first, cl))
				++first;
			return first;
		}
		return 0;
	}

	// ls32 = ( h16 ":" h16 ) / IPv4address
	inline const Char* handleLs32(const Char* first, const Char* last) {
		const Char* p;
		return (0 != (p = handleH16(first, last)) && p + 2 < last && *++p == L':' && 0 != (p = handleH16(p, last))
			|| 0 != (p = handleIPv4address(first, last))) ? p : 0;
	}

	// scheme = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
	const Char* ASCENSION_FASTCALL handleScheme(const Char* first, const Char* last) {
		if(isalpha(*first, cl)) {
			while(++first != last) {
				if(!isalnum(*first, cl) && *first != L'+' && *first != L'-' && *first != L'.')
					return first;
			}
			return last;
		}
		return 0;
	}

	// iprivate = %xE000-F8FF / %xF0000-FFFFD / %x100000-10FFFD
	inline const Char* handlePrivate(const Char* first, const Char* last) {
		if(first < last) {
			if(*first >= 0xE000 && *first <= 0xF8FF)
				return ++first;
			const CodePoint c = surrogates::decodeFirst(first, last);
			if((c >= 0xF0000 && c <= 0xFFFFD) || (c >= 0x100000 && c <= 0x10FFFD))
				return first += 2;
		}
		return 0;
	}

	// ucschar = %xA0-D7FF / %xF900-FDCF / %xFDF0-FFEF
    //         / %x10000-1FFFD / %x20000-2FFFD / %x30000-3FFFD
    //         / %x40000-4FFFD / %x50000-5FFFD / %x60000-6FFFD
    //         / %x70000-7FFFD / %x80000-8FFFD / %x90000-9FFFD
    //         / %xA0000-AFFFD / %xB0000-BFFFD / %xC0000-CFFFD
    //         / %xD0000-DFFFD / %xE1000-EFFFD
	inline const Char* handleUcschar(const Char* first, const Char* last) {
		if(first < last) {
			if((*first >= 0x00A0 && *first <= 0xD7FF) || (*first >= 0xF900 && *first <= 0xFDCF) || (*first >= 0xFDF0 && *first <= 0xFFEF))
				return ++first;
			const CodePoint c = surrogates::decodeFirst(first, last);
			if(c >= 0x10000 && c < 0xF0000 && (c & 0xFFFF) >= 0x0000 && (c & 0xFFFF) <= 0xFFFD) {
				if((c & 0xF0000) != 0xE || (c & 0xFFFF) >= 0x1000)
					return first += 2;
			}
		}
		return 0;
	}

	// iunreserved = ALPHA / DIGIT / "-" / "." / "_" / "~" / ucschar
	inline const Char* handleIunreserved(const Char* first, const Char* last) {
		const Char* p;
		return (0 != (p = handleUnreserved(first, last)) || 0 != (p = handleUcschar(first, last))) ? p : 0;
	}

	// ipchar = iunreserved / pct-encoded / sub-delims / ":" / "@"
	inline const Char* handlePchar(const Char* first, const Char* last) {
		if(first < last) {
			const Char* p;
			if(0 != (p = handleIunreserved(first, last))
					|| 0 != (p = handlePctEncoded(first, last))
					|| 0 != (p = handleSubDelims(first, last)))
				return p;
			else if(*first == L':' || *first == L'@')
				return ++first;
		}
		return 0;
	}

	// isegment = *ipchar
	inline const Char* handleSegment(const Char* first, const Char* last) {	// [nullable]
		for(const Char* eop ; first < last; first = eop) {
			if(0 == (eop = handlePchar(first, last)))
				break;
		}
		return first;
	}

	// isegment-nz = 1*ipchar
	inline const Char* handleSegmentNz(const Char* first, const Char* last) {
		const Char* const eos = handleSegment(first, last);
		return (eos > first) ? eos : 0;
	}

	// isegment-nz-nc = 1*( iunreserved / pct-encoded / sub-delims / "@" ) ; non-zero-length segment without any colon ":"
	inline const Char* handleSegmentNzNc(const Char* first, const Char* last) {
		const Char* const f = first;
		for(const Char* eop; first < last; ) {
			if(0 != (eop = handleUnreserved(first, last))
					|| 0 != (eop = handlePctEncoded(first, last))
					|| 0 != (eop = handleSubDelims(first, last)))
				first = eop;
			else if(*first == L'@')
				++first;
			else
				break;
		}
		return (first > f) ? first : 0;
	}

	// ipath-empty = 0<ipchar>
	const Char* ASCENSION_FASTCALL handlePathEmpty(const Char* first, const Char*) {return first;}	// [nullable]

	// ipath-abempty = *( "/" isegment )
	const Char* ASCENSION_FASTCALL handlePathAbempty(const Char* first, const Char* last) {	// [nullable]
		while(first < last && *first != L'/')
			first = handleSegment(first + 1, last);
		return first;
	}
   
	// ipath-rootless = isegment-nz *( "/" isegment )
	inline const Char* handlePathRootless(const Char* first, const Char* last) {
		const Char* const eos = handleSegmentNz(first, last);
		return (eos != 0) ? handlePathAbempty(eos, last) : 0;
	}

	// ipath-noscheme = isegment-nz-nc *( "/" isegment )
	inline const Char* handlePathNoscheme(const Char* first, const Char* last) {
		const Char* const eos = handleSegmentNzNc(first, last);
		return (eos != 0) ? handlePathAbempty(eos, last) : 0;
	}

	// ipath-absolute = "/" [ isegment-nz *( "/" isegment ) ]
	inline const Char* handlePathAbsolute(const Char* first, const Char* last) {
		return (first < last && *first == L'/') ? handlePathRootless(++first, last) : 0;
	}

	// ireg-name = *( iunreserved / pct-encoded / sub-delims )
	inline const Char* handleRegName(const Char* first, const Char* last) {	// [nullable]
		for(const Char* p; first < last; first = p) {
			if(0 == (p = handleIunreserved(first, last))
					&& 0 == (p = handlePctEncoded(first, last))
					&& 0 == (p = handleSubDelims(first, last)))
				break;
		}
		return first;
	}

	// ihost = IP-literal / IPv4address / ireg-name
	inline const Char* handleHost(const Char* first, const Char* last) {	// [nullable]
		const Char* p;
		return (0 != (p = handleIPLiteral(first, last)) || 0 != (p = handleIPv4address(first, last))) ? p : handleRegName(first, last);
	}

	// iuserinfo = *( iunreserved / pct-encoded / sub-delims / ":" )
	const Char* ASCENSION_FASTCALL handleUserinfo(const Char* first, const Char* last) {	// [nullable]
		for(const Char* eop; first < last; ) {
			if(0 != (eop = handleUnreserved(first, last))
					|| 0 != (eop = handlePctEncoded(first, last))
					|| 0 != (eop = handleSubDelims(first, last)))
				first = eop;
			else if(*first == L':')
				++first;
			else
				break;
		}
		return first;
	}

	// iauthority = [ iuserinfo "@" ] ihost [ ":" port ]
	const Char* ASCENSION_FASTCALL handleAuthority(const Char* first, const Char* last) {	// [nullable]
		first = handleUserinfo(first, last);
		assert(first != 0);
		if(first == last)
			return 0;
		else if(*first == L'@')
			++first;
		if(0 != (first = handleHost(first, last))) {
			if(first != last) {
				if(*first == L':')
					++first;
				first = handlePort(first, last);
				assert(first != 0);
			}
		}
		return first;
	}

	// ihier-part = ("//" iauthority ipath-abempty) / ipath-absolute / ipath-rootless / ipath-empty
	const Char* ASCENSION_FASTCALL handleHierPart(const Char* first, const Char* last) {
		const Char* eop;
		(last - first > 2 && wmemcmp(first, L"//", 2) == 0
			&& 0 != (eop = handleAuthority(first + 2, last)) && 0 != (eop = handlePathAbempty(eop, last)))
			|| 0 != (eop = handlePathAbsolute(first, last))
			|| 0 != (eop = handlePathRootless(first, last))
			|| 0 != (eop = handlePathEmpty(first, last));
		return eop;
	}

	// iquery = *( ipchar / iprivate / "/" / "?" )
	const Char* ASCENSION_FASTCALL handleQuery(const Char* first, const Char* last) {	// [nullable]
		for(const Char* eop; first < last; ) {
			if(0 != (eop = handlePchar(first, last)) || 0 != (eop = handlePrivate(first, last)))
				first = eop;
			else if(*first == L'/' || *first == L'?')
				++first;
			else
				break;
		}
		return first;
	}

	// ifragment = *( ipchar / "/" / "?" )
	const Char* ASCENSION_FASTCALL handleFragment(const Char* first, const Char* last) {	// [nullable]
		for(const Char* eop; first < last; ) {
			if(0 != (eop = handlePchar(first, last)))
				first = eop;
			else if(*first == L'/' || *first == L'?')
				++first;
			else
				break;
		}
		return first;
	}

	// IRI = scheme ":" ihier-part [ "?" iquery ] [ "#" ifragment ]
	const Char* ASCENSION_FASTCALL handleIRI(const Char* first, const Char* last) {
		if(0 != (first = handleScheme(first, last))) {
			if(*first == L':') {
				if(0 != (first = handleHierPart(++first, last))) {
					if(*first == L'?') {
						first = handleQuery(++first, last);
						assert(first != 0);
					}
					if(*first == L'#') {
						first = handleFragment(++first, last);
						assert(first != 0);
					}
					return first;
				}
			}
		}
		return 0;
	}

	const bool URI_CHARS[] = {
		false,	false,	false,	false,	false,	false,	false,	false,	// 0x00
		false,	false,	false,	false,	false,	false,	false,	false,
		false,	false,	false,	false,	false,	false,	false,	false,	// 0x10
		false,	false,	false,	false,	false,	false,	false,	false,
		false,	true,	false,	true,	true,	true,	true,	false,	// 0x20
		false,	false,	false,	true,	true,	true,	true,	true,
		true,	true,	true,	true,	true,	true,	true,	true,	// 0x30
		true,	true,	true,	true,	false,	true,	false,	true,
		true,	true,	true,	true,	true,	true,	true,	true,	// 0x40
		true,	true,	true,	true,	true,	true,	true,	true,
		true,	true,	true,	true,	true,	true,	true,	true,	// 0x50
		true,	true,	true,	false,	true,	false,	false,	true,
		false,	true,	true,	true,	true,	true,	true,	true,	// 0x60
		true,	true,	true,	true,	true,	true,	true,	true,
		true,	true,	true,	true,	true,	true,	true,	true,	// 0x70
		true,	true,	true,	false,	false,	false,	true,	false
	};
} // namespace @0

/// Constructor. The set of the valid schemes is empty.
URIDetector::URIDetector() throw() : validSchemes_(0) {
}

/// Destructor.
URIDetector::~URIDetector() throw() {
	delete validSchemes_;
}

/// Returns the default generic instance.
const URIDetector& URIDetector::defaultGenericInstance() throw() {
	static URIDetector singleton;
	return singleton;
}

/**
 * Returns the end of a URL begins at the given position.
 * @param first the beginning of the character sequence
 * @param last the end of the character sequence
 * @return the end of the detected URI, or @a first (not @c null) if not matched
 * @throw NullPointerException @a first and/or @a last are @c null
 */
const Char* URIDetector::detect(const Char* first, const Char* last) const {
	if(first == 0)
		throw NullPointerException("first");
	else if(last == 0)
		throw NullPointerException("last");
	else if(first >= last)
		return first;

	// check scheme
	const Char* endOfScheme;
	if(validSchemes_ != 0) {
		endOfScheme = wmemchr(first + 1, ':', min<size_t>(last - first - 1, validSchemes_->maximumLength()));
		if(!validSchemes_->matches(first, endOfScheme))
			endOfScheme = 0;
	} else {
		endOfScheme = wmemchr(first + 1, ':', static_cast<size_t>(last - first - 1));
		if(handleScheme(first, endOfScheme) != endOfScheme)
			endOfScheme = 0;
	}
	if(endOfScheme == 0)
		return first;
	else if(endOfScheme == last - 1)	// terminated with <ipath-empty>
		return last;
	return (0 != (last = handleIRI(first, last))) ? last : first;
}

/**
 * Searches a URI in the specified character sequence.
 * @param first the beginning of the character sequence
 * @param last the end of the character sequence
 * @param[out] result the range of the found URI in the target character sequence
 * @return true if a URI was found
 * @throw NullPointerException @a first and/or @a last are @c null
 */
bool URIDetector::search(const Char* first, const Char* last, pair<const Char*, const Char*>& result) const {
	if(first == 0)
		throw NullPointerException("first");
	else if(last == 0)
		throw NullPointerException("last");
	else if(first >= last)
		return false;

	// search scheme
	const Char* nextColon = wmemchr(first, L':', last - first);
	if(nextColon == 0)
		return false;
	while(true) {
		if(handleScheme(first, nextColon) == nextColon) {
			if(validSchemes_ == 0 || validSchemes_->matches(first, nextColon)) {
				if(0 != (result.second = handleIRI(result.first = first, last)))
					return true;
			}
			first = nextColon;
		} else
			++first;
		if(first == nextColon) {
			first = nextColon;
			nextColon = wmemchr(first, L':', last - first);
			if(nextColon == 0)
				break;
		}
	}
	return false;
}

/**
 * Sets the valid schemes.
 * @param scheme the set of the schemes to set
 * @return the detector
 * @throw invalid_argument invalid name as a scheme was found
 */
URIDetector& URIDetector::setValidSchemes(const set<String>& schemes) {
	// validation
	for(set<String>::const_iterator i(schemes.begin()), e(schemes.end()); i != e; ++i) {
		const Char* const p = i->data();
		if(handleScheme(p, p + i->length()) != p + i->length())
			throw invalid_argument("schemes");
	}

	// rebuild hash table
	HashTable* newSchemes = new HashTable(schemes.begin(), schemes.end(), true);
	delete validSchemes_;
	validSchemes_ = newSchemes;

	return *this;
}


// Token ////////////////////////////////////////////////////////////////////

const Token::ID Token::DEFAULT_TOKEN = 0;
const Token::ID Token::UNCALCULATED = static_cast<Token::ID>(-1);


// Rule /////////////////////////////////////////////////////////////////////

/**
 * Protected constructor.
 * @param tokenID the identifier of the token which will be returned by the rule. can be @c Token#NULL_ID
 * @param caseSensitive set false to enable caseless match
 */
Rule::Rule(Token::ID tokenID, bool caseSensitive) throw() : id_(tokenID), caseSensitive_(caseSensitive) {
}


// RegionRule ///////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param id the identifier of the token which will be returned by the rule
 * @param startSequence the pattern's start sequence
 * @param endSequence the pattern's end sequence. if empty, token will end at end of line
 * @param escapeCharacter the character which a character will be ignored
 * @param caseSensitive set false to enable caseless match
 * @throw std#invalid_argument @a startSequence is empty
 */
RegionRule::RegionRule(Token::ID id, const String& startSequence, const String& endSequence,
		Char escapeCharacter /* = NONCHARACTER */, bool caseSensitive /* = true */)
		: Rule(id, caseSensitive), startSequence_(startSequence), endSequence_(endSequence), escapeCharacter_(escapeCharacter) {
	if(startSequence.empty())
		throw invalid_argument("the start sequence is empty.");
}

/// @see Rule#parse
auto_ptr<Token> RegionRule::parse(const ITokenScanner& scanner, const Char* first, const Char* last) const throw() {
	// match the start sequence
	if(first[0] != startSequence_[0]
			|| static_cast<size_t>(last - first) < startSequence_.length() + endSequence_.length()
			|| (startSequence_.length() > 1 && wmemcmp(first + 1, startSequence_.data() + 1, startSequence_.length() - 1) != 0))
		return auto_ptr<Token>(0);
	const Char* end = last;
	if(!endSequence_.empty()) {
		// search the end sequence
		for(const Char* p = first + startSequence_.length(); p <= last - endSequence_.length(); ++p) {
			if(escapeCharacter_ != NONCHARACTER && *p == escapeCharacter_)
				++p;
			else if(*p == endSequence_[0] && wmemcmp(p + 1, endSequence_.data() + 1, endSequence_.length() - 1) == 0) {
				end = p + endSequence_.length();
				break;
			}
		}
	}
	auto_ptr<Token> result(new Token);
	result->id = getTokenID();
	result->region.first.line = result->region.second.line = scanner.getPosition().line;
	result->region.first.column = scanner.getPosition().line;
	result->region.second.column = result->region.first.column + (end - first);
	return result;
}


// NumberRule ///////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param id the identifier of the token which will be returned by the rule
 */
NumberRule::NumberRule(Token::ID id) throw() : Rule(id, true) {
}

/// @see Rule#parse
auto_ptr<Token> NumberRule::parse(const ITokenScanner& scanner, const Char* first, const Char* last) const throw() {
	assert(first < last);
	/*
		This is based on ECMAScript 3 "7.8.3 Numeric Literals" and performs the following regular
		expression match:
			/(0|[1-9][0-9]*)(\.[0-9]+)?([e|E][\+\-]?[0-9]+)?/ for DecimalLiteral (case 1)
			/\.[0-9]+([e|E][\+\-]?[0-9]+)?/ for DecimalLiteral (case 2)
			/0[x|X][0-9A-Fa-f]+/ for HexIntegerLiteral
		Octal integer literals are not supported. See "B.1.1 Numeric Literals" in the same specification.
	*/
	// ISSUE: this implementation accepts some illegal format like as "0.1.2".
	const Char* e;
	if(last - first > 2 && first[0] == L'0' && (first[1] == L'x' || first[1] == L'X')) {	// HexIntegerLiteral?
		for(e = first + 2; e < last; ++e) {
			if((*e >= L'0' && *e <= L'9') || (*e >= L'A' && *e <= L'F') || (*e >= L'a' && *e <= L'f'))
				continue;
			break;
		}
		if(e == first + 2)
			return auto_ptr<Token>(0);
	} else {	// DecimalLiteral?
		bool foundDecimalIntegerLiteral = false, foundDot = false;
		if(first[0] >= L'0' && first[0] <= L'9') {	// DecimalIntegerLiteral ::= /0|[1-9][0-9]*/
			e = first + 1;
			foundDecimalIntegerLiteral = true;
			if(first[0] != L'0')
				e = find_if(e, last, not1(InRange<Char>(L'0', L'9')));
		} else
			e = first;
		if(e < last && *e == L'.') {	// . DecimalDigits ::= /\.[0-9]+/
			foundDot = true;
			e = find_if(++e, last, not1(InRange<Char>(L'0', L'9')));
			if(e[-1] == L'.')
				return auto_ptr<Token>(0);
		}
		if(!foundDecimalIntegerLiteral && !foundDot)
			return auto_ptr<Token>(0);
		if(e < last && (*e == L'e' || *e == L'E')) {	// ExponentPart ::= /[e|E][\+\-]?[0-9]+/
			if(++e == last)
				return auto_ptr<Token>(0);
			if(*e == L'+' || *e == L'-') {
				if(++e == last)
					return auto_ptr<Token>(0);
			}
			e = find_if(++e, last, not1(InRange<Char>(L'0', L'9')));
		}
	}

	// e points the end of the found token
	assert(e > first);
	// "The source character immediately following a NumericLiteral must not be an IdentifierStart or DecimalDigit."
	if(e < last && ((*e >= L'0' && *e <= L'9') || scanner.getIdentifierSyntax().isIdentifierStartCharacter(surrogates::decodeFirst(e, last))))
		return auto_ptr<Token>(0);

	auto_ptr<Token> temp(new Token);
	temp->id = getTokenID();
	temp->region.first.line = temp->region.second.line = scanner.getPosition().line;
	temp->region.first.column = scanner.getPosition().column;
	temp->region.second.column = temp->region.first.column + e - first;
	return temp;
}


// URIRule //////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param id the identifier of the token which will be returned by the rule
 * @param uriDetector the URI detector
 */
URIRule::URIRule(Token::ID id, const URIDetector& uriDetector,
		bool delegateOwnership) throw() : Rule(id, true), uriDetector_(&uriDetector, delegateOwnership) {
}

/// @see Rule#parse
auto_ptr<Token> URIRule::parse(const ITokenScanner& scanner, const Char* first, const Char* last) const throw() {
	assert(first < last);
	const Char* const e = uriDetector_->detect(first, last);
	if(e == first)
		return auto_ptr<Token>(0);
	auto_ptr<Token> temp(new Token);
	temp->id = getTokenID();
	temp->region.first.line = temp->region.second.line = scanner.getPosition().line;
	temp->region.first.column = scanner.getPosition().column;
	temp->region.second.column = temp->region.first.column + e - first;
	return temp;
}


// WordRule /////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param id the identifier of the token which will be returned by the rule
 * @param first the start of the words
 * @param last the end of the words
 * @param caseSensitive set false to enable caseless match
 * @throw std#invalid_argument @a first and/or @a last are @c null
 */
WordRule::WordRule(Token::ID id, const String* first, const String* last, bool caseSensitive /* = true */) : Rule(id, caseSensitive) {
	if(first == 0 || last == 0 || first >= last)
		throw invalid_argument("the input string list is invalid.");
	words_ = new HashTable(first, last, caseSensitive);
}

/**
 * Constructor.
 * @param id the identifier of the token which will be returned by the rule
 * @param first the start of the string
 * @param last the end of the string
 * @param separator the separator character in the string
 * @param caseSensitive set false to enable caseless match
 * @throw NullPointerException @a first and/or @a last are @c null
 * @throw std#invalid_argument @a separator is a surrogate
 */
WordRule::WordRule(Token::ID id, const Char* first, const Char* last, Char separator, bool caseSensitive) : Rule(id, caseSensitive) {
	if(first == 0 || last == 0)
		throw NullPointerException("first and/or last is null.");
	else if(first >= last)
		throw invalid_argument("the input string list is invalid.");
	else if(surrogates::isSurrogate(separator))
		throw invalid_argument("the separator is a surrogate character.");
	list<String> words;
	first = find_if(first, last, not1(bind1st(equal_to<Char>(), separator)));
	for(const Char* p = first; p < last; first = ++p) {
		p = find(first, last, separator);
		if(p == first)
			continue;
		words.push_back(String(first, p));
	}
	if(words.empty())
		throw invalid_argument("the input string includes no words.");
	words_ = new HashTable(words.begin(), words.end(), caseSensitive);
}

/// Destructor.
WordRule::~WordRule() throw() {
	delete words_;
}

/// @see Rule#parse
auto_ptr<Token> WordRule::parse(const ITokenScanner& scanner, const Char* first, const Char* last) const {
	if(!words_->matches(first, last))
		return auto_ptr<Token>(0);
	auto_ptr<Token> result(new Token);
	result->id = getTokenID();
	result->region.first.line = result->region.second.line = scanner.getPosition().line;
	result->region.first.column = scanner.getPosition().column;
	result->region.second.column = result->region.first.column + (last - first);
	return result;
}


#ifndef ASCENSION_NO_REGEX

// RegexRule ////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param id the identifier of the token which will be returned by the rule
 * @param pattern the pattern string
 * @param caseSensitive set false to enable caseless match
 * @throw regex#PatternSyntaxException the specified pattern is invalid
 */
RegexRule::RegexRule(Token::ID id, const String& pattern, bool caseSensitive /* = true */)
		: Rule(id, caseSensitive), pattern_(regex::Pattern::compile(pattern)) {
}

/// @see Rule#parse
auto_ptr<Token> RegexRule::parse(const ITokenScanner& scanner, const Char* first, const Char* last) const {
	const UTF16To32Iterator<const Char*> b(first, last), e(first, last, last);
	auto_ptr<regex::Matcher<UTF16To32Iterator<const Char*> > > matcher(pattern_->matcher(b, e));
	if(!matcher->lookingAt())
		return auto_ptr<Token>(0);
	auto_ptr<Token> token(new Token);
	token->id = getTokenID();
	token->region.first.line = token->region.second.line = scanner.getPosition().line;
	token->region.first.column = scanner.getPosition().column;
	token->region.second.column = token->region.first.column + (matcher->end().tell() - matcher->start().tell());
	return token;
}

#endif /* !ASCENSION_NO_REGEX */


// NullTokenScanner /////////////////////////////////////////////////////////

/// @see ITokenScanner#getIdentifierSyntax
const IdentifierSyntax& NullTokenScanner::getIdentifierSyntax() const throw() {
	return IdentifierSyntax::defaultInstance();
}

/// @see ITokenScanner#getPosition
Position NullTokenScanner::getPosition() const throw() {
	return Position::INVALID_POSITION;
}

/// @see ITokenScanner#isDone
bool NullTokenScanner::isDone() const throw() {
	return true;
}

/// @see ITokenScanner#nextToken
auto_ptr<Token> NullTokenScanner::nextToken() {
	return auto_ptr<Token>(0);
}

/// @see ITokenScanner#parse
void NullTokenScanner::parse(const Document&, const Region&) {
}


// LexicalTokenScanner //////////////////////////////////////////////////////

/**
 * Constructor.
 * @param contentType the content the scanner parses
 */
LexicalTokenScanner::LexicalTokenScanner(ContentType contentType) throw() : contentType_(contentType), current_() {
}

/// Destructor.
LexicalTokenScanner::~LexicalTokenScanner() throw() {
	for(list<const Rule*>::iterator i = rules_.begin(); i != rules_.end(); ++i)
		delete *i;
	for(list<const WordRule*>::iterator i = wordRules_.begin(); i != wordRules_.end(); ++i)
		delete *i;
}

/// @see ITokenScanner#getIdentifierSyntax
const IdentifierSyntax& LexicalTokenScanner::getIdentifierSyntax() const throw() {
	return current_.document()->contentTypeInformation().getIdentifierSyntax(contentType_);
}

/**
 * Adds the new rule to the scanner.
 * @param rule the rule to be added
 * @throw NullPointerException @a rule is @c null
 * @throw std#invalid_argument @a rule is already registered
 * @throw BadScannerStateException the scanner is running
 */
void LexicalTokenScanner::addRule(auto_ptr<const Rule> rule) {
	if(rule.get() == 0)
		throw NullPointerException("rule");
	else if(!isDone())
		throw BadScannerStateException();
	else if(find(rules_.begin(), rules_.end(), rule.get()) != rules_.end())
		throw invalid_argument("the rule is already registered.");
	rules_.push_back(rule.release());
}

/**
 * Adds the new word rule to the scanner.
 * @param rule the rule to be added
 * @throw NullPointerException @a rule is @c null
 * @throw std#invalid_argument @a rule is already registered
 * @throw BadScannerStateException the scanner is running
 */
void LexicalTokenScanner::addWordRule(auto_ptr<const WordRule> rule) {
	if(rule.get() == 0)
		throw NullPointerException("rule");
	else if(!isDone())
		throw BadScannerStateException();
	else if(find(wordRules_.begin(), wordRules_.end(), rule.get()) != wordRules_.end())
		throw invalid_argument("the rule is already registered.");
	wordRules_.push_back(rule.release());
}

/// @see ITokenScanner#getPosition
Position LexicalTokenScanner::getPosition() const throw() {
	return current_.tell();
}

/// @see ITokenScanner#isDone
bool LexicalTokenScanner::isDone() const throw() {
	return !current_.hasNext();
}

/// @see ITokenScanner#nextToken
auto_ptr<Token> LexicalTokenScanner::nextToken() {
	const IdentifierSyntax& idSyntax = getIdentifierSyntax();
	auto_ptr<Token> result;
	const String* line = &current_.line();
	while(current_.hasNext()) {
		if(current_.current() == LINE_SEPARATOR) {
			current_.next();
			line = &current_.line();
			if(!current_.hasNext())
				break;
		}
		const Char* const p = line->data() + current_.tell().column;
		const Char* const last = line->data() + line->length();
		for(list<const Rule*>::const_iterator i = rules_.begin(); i != rules_.end(); ++i) {
			result = (*i)->parse(*this, p, last);
			if(result.get() != 0) {
				current_.seek(result->region.end());
				return result;
			}
		}
		const Char* const wordEnd = idSyntax.eatIdentifier(p, last);
		if(wordEnd > p) {
			if(!wordRules_.empty()) {
				for(list<const WordRule*>::const_iterator i = wordRules_.begin(); i != wordRules_.end(); ++i) {
					result = (*i)->parse(*this, p, wordEnd);
					if(result.get() != 0) {
						current_.seek(result->region.end());
						return result;
					}
				}
			}
			current_.seek(Position(current_.tell().line, wordEnd - line->data()));
		} else
			current_.next();
	}
	return result;
}

/// @see ITokenScanner#parse
void LexicalTokenScanner::parse(const Document& document, const Region& region) {
	current_ = DocumentCharacterIterator(document, region);
}


// TransitionRule ///////////////////////////////////////////////////////////

/**
 * Protected constructor.
 * @param contentType the content type of the transition source
 * @param destination the content type of the transition destination
 */
TransitionRule::TransitionRule(ContentType contentType, ContentType destination) throw() : contentType_(contentType), destination_(destination) {
}

/// Destructor.
TransitionRule::~TransitionRule() throw() {
}

/**
 * @fn TransitionRule::matches
 * Returns true if the rule matches the specified text.
 * @param line the target line text
 * @param column the column number at which match starts
 * @return the length of the matched pattern. if and only if the match failed, returns 0.
 * if matched zero width text, returns 1
 */


// LiteralTransitionRule ////////////////////////////////////////////////////

/**
 * Constructor.
 * @param contentType the content type of the transition source
 * @param destination the content type of the transition destination
 * @param pattern the pattern string to introduce the transition.
 * if empty string is specified, the transition will be occured at the end of line
 * @param escapeCharacter the character which a character will be ignored. if @c NONCHARACTER is
 * specified, the escape character will be not set. this is always case-sensitive
 * @param caseSensitive set false to enable caseless match
 */
LiteralTransitionRule::LiteralTransitionRule(ContentType contentType, ContentType destination,
		const String& pattern, Char escapeCharacter /* = NONCHARACTER */, bool caseSensitive /* = true */) :
		TransitionRule(contentType, destination), pattern_(pattern), escapeCharacter_(escapeCharacter), caseSensitive_(caseSensitive) {
}

/// @see TransitionRule#matches
length_t LiteralTransitionRule::matches(const String& line, length_t column) const {
	if(escapeCharacter_ != NONCHARACTER && column > 0 && line[column - 1] == escapeCharacter_)
		return 0;
	else if(pattern_.empty() && column == line.length())	// matches EOL
		return 1;
	else if(line.length() - column < pattern_.length())
		return 0;
	else if(caseSensitive_)
		return (wmemcmp(pattern_.data(), line.data() + column, pattern_.length()) == 0) ? pattern_.length() : 0;
	return (CaseFolder::compare(StringCharacterIterator(pattern_),
		StringCharacterIterator(line, line.begin() + column)) == 0) ? pattern_.length() : 0;
}


#ifndef ASCENSION_NO_REGEX

// RegexTransitionRule //////////////////////////////////////////////////////

/**
 * Constructor.
 * @param contentType the content type of the transition source
 * @param destination the content type of the transition destination
 * @param pattern the pattern string to introduce the transition. can't be empty
 * @param caseSensitive set false to enable caseless match
 * @throw regex#PatternSyntaxException @a pattern is invalid
 */
RegexTransitionRule::RegexTransitionRule(ContentType contentType, ContentType destination, const String& pattern,
		bool caseSensitive /* = true */) : TransitionRule(contentType, destination),
		pattern_(regex::Pattern::compile(pattern, caseSensitive ? 0 : regex::Pattern::CASE_INSENSITIVE)) {
}

/// @see TransitionRule#matches
length_t RegexTransitionRule::matches(const String& line, length_t column) const {
	try {
		typedef UTF16To32Iterator<String::const_iterator> I;
		auto_ptr<regex::Matcher<I> > matcher(pattern_->matcher(I(line.begin(), line.end()), I(line.begin(), line.end(), line.begin())));
		matcher->region(I(line.begin(), line.end(), line.begin() + column), matcher->regionEnd());
		matcher->useAnchoringBounds(false).useTransparentBounds(true);
		return matcher->lookingAt() ? max(matcher->end().tell() - matcher->start().tell(), 1) : 0;
	} catch(runtime_error&) {
		return 0;
	}
}

#endif /* !ASCENSION_NO_REGEX */


// LexicalPartitioner ///////////////////////////////////////////////////////

/// Constructor.
LexicalPartitioner::LexicalPartitioner() throw() {
}

/// Destructor.
LexicalPartitioner::~LexicalPartitioner() throw() {
	clearRules();
}

/// Deletes all the transition rules.
void LexicalPartitioner::clearRules() throw() {
	for(list<const TransitionRule*>::const_iterator i(rules_.begin()); i != rules_.end(); ++i)
		delete *i;
	rules_.clear();
}

/**
 * Computes and constructs the partitions on the specified region.
 * @param start the start of the region to compute
 * @param minimalLast the partitioner must scan to this position at least
 * @param[out] changedRegion the region whose content type was changed
 */
void LexicalPartitioner::computePartitioning(const Position& start, const Position& minimalLast, Region& changedRegion) {
	// TODO: see LexicalPartitioner.documentChanged.
}

/// @see kernel#DocumentPartitioner#documentAboutToBeChanged
void LexicalPartitioner::documentAboutToBeChanged() throw() {
}

/// @see kernel#DocumentPartitioner#documentChanged
void LexicalPartitioner::documentChanged(const DocumentChange& change) throw() {
	if(change.region().isEmpty())
		return;
	// TODO: there is more efficient implementation using LexicalPartitioner.computePartitioning.
	const Document& doc = *document();
	const Position eof(doc.region().second);

	// delete the partitions encompassed by the deleted region
	if(change.isDeletion())
		erasePartitions(change.region().beginning(), change.region().end());

	// move the partitions adapting to the document change
	for(size_t i = 0, c = partitions_.size(); i < c; ++i) {
		partitions_[i]->start = updatePosition(partitions_[i]->start, change, FORWARD);
		partitions_[i]->tokenStart = updatePosition(partitions_[i]->tokenStart, change, FORWARD);
	}

	// delete the partitions start at the deleted region
	DocumentCharacterIterator p(doc, Position(change.region().beginning().line, 0));
	Position eol(change.isDeletion() ? change.region().beginning() : change.region().end());
	eol.column = doc.lineLength(eol.line);
	erasePartitions(p.tell(), eol);

	// reconstruct partitions in the affected region
	const Position bof(doc.region().first);
	const String* line = &doc.line(p.tell().line);
	ContentType eolContentType = getTransitionStateAt(eol);
	size_t partition = findClosestPartition(p.tell());
	ContentType contentType = partitions_[partition]->contentType, destination;
	while(true) {	// scan and tokenize into partitions...
		const bool isEOL = p.tell().column == line->length();
		length_t tokenLength = tryTransition(*line, p.tell().column, contentType, destination);
		if(tokenLength != 0) {	// a token was found
			if(isEOL)
				tokenLength = 0;	// a line terminator is zero-length...
			const Position tokenEnd(p.tell().line, p.tell().column + tokenLength);
			// insert the new partition behind the current
			assert(destination != contentType);
			if(partition > 0 || p.tell() > bof)
				partitions_.insert(++partition,
					new Partition(destination, (destination > contentType) ? p.tell() : tokenEnd, p.tell(), tokenLength));
			else {
				Partition& pa = *partitions_[0];
				pa.contentType = destination;
				pa.start = (destination > contentType) ? p.tell() : tokenEnd;
				pa.tokenLength = tokenLength;
				pa.tokenStart = p.tell();
			}
			contentType = destination;
			// go to the end of the found token
			if(!isEOL)
				p.seek(tokenEnd);
		}
		// if reached the end of the affected region and content types are same, we are done
		if(p.tell() == eof || (isEOL && p.tell() == eol && contentType == eolContentType))
			break;
		// go to the next character if no transition occured
		if(tokenLength == 0) {
			p.next();
			if(p.tell().column == 0) {	// entered the next line
				line = &doc.line(p.tell().line);
				if(p.tell().line > eol.line) {
					eol = Position(p.tell().line, doc.lineLength(p.tell().line));
					eolContentType = getTransitionStateAt(eol);
				}
			}
		}
	}
	verify();
	notifyDocument(Region(Position(change.region().beginning().line, 0), p.tell()));
}

/// @see kernel#DocumentPartitioner#doGetPartition
void LexicalPartitioner::doGetPartition(const Position& at, DocumentPartition& partition) const throw() {
	const size_t i = findClosestPartition(at);
	const Partition& p = *partitions_[i];
	partition.contentType = p.contentType;
	partition.region.first = p.start;
	partition.region.second = (i < partitions_.size() - 1) ? partitions_[i + 1]->start : document()->region().second;
}

/// @see kernel#DocumentPartitioner#doInstall
void LexicalPartitioner::doInstall() throw() {
	partitions_.clear();
	partitions_.insert(0, new Partition(DEFAULT_CONTENT_TYPE, Position::ZERO_POSITION, Position::ZERO_POSITION, 0));
	Region dummy;
	const Region entire(document()->region());
	computePartitioning(entire.first, entire.second, dummy);
}

/// Dumps the partitions information.
void LexicalPartitioner::dump() const {
#ifdef _DEBUG
	win32::DumpContext dout;
	dout << "LexicalPartitioner dump start:\n";
	for(size_t i = 0; i < partitions_.size(); ++i) {
		const Partition& p = *partitions_[i];
		dout << "\t" << p.contentType << " = ("
			<< static_cast<ulong>(p.start.line) << ", " << static_cast<ulong>(p.start.column) << ")\n";
	}
#endif /* _DEBUG */
}

/***/
void LexicalPartitioner::erasePartitions(const Position& first, const Position& last) {
	// locate the first partition to delete
	size_t deletedFirst = findClosestPartition(first);
	if(first >= partitions_[deletedFirst]->getTokenEnd())
		++deletedFirst;	// do not delete this partition
//	else if(deletedFirst < partitions_.getSize() - 1 && partitions_[deletedFirst + 1]->tokenStart < change.getRegion().getBottom())
//		++deletedFirst;	// delete from the next partition
	// locate the last partition to delete
	size_t deletedLast = findClosestPartition(last) + 1;	// exclusive
	if(deletedLast < partitions_.size() && partitions_[deletedLast]->tokenStart < last)
		++deletedLast;
//	else if(titions_[predeletedLast - 1]->start == change.getRegion().getBottom())
//		--deletedLast;
	if(deletedLast > deletedFirst) {
		if(deletedFirst > 0 && deletedLast < partitions_.size()
				&& partitions_[deletedFirst - 1]->contentType == partitions_[deletedLast]->contentType)
			++deletedLast;	// combine
		partitions_.erase(deletedFirst, deletedLast - deletedFirst);
	}

	// push a default partition if no partition includes the start of the document
	const Document& d = *document();
	if(partitions_.empty() || partitions_[0]->start != d.region().first) {
		if(partitions_.empty() || partitions_[0]->contentType != DEFAULT_CONTENT_TYPE) {
			partitions_.insert(0, new Partition(DEFAULT_CONTENT_TYPE, Position::ZERO_POSITION, Position::ZERO_POSITION, 0));
		} else {
			partitions_[0]->start = partitions_[0]->tokenStart = d.region().first;
			partitions_[0]->tokenLength = 0;
		}
	}

	// delete the partition whose start position is the end of the document
	if(partitions_.size() > 1 && partitions_.back()->start == d.region().second)
		partitions_.erase(partitions_.size() - 1);
}

/**
 * Returns a partition closest to the given position.
 * @param at the position
 * @return the index of the partition
 */
inline size_t LexicalPartitioner::findClosestPartition(const Position& at) const throw() {
	size_t result = ascension::internal::searchBound(
		static_cast<size_t>(0), partitions_.size(), at, bind1st(mem_fun(LexicalPartitioner::getPartitionStart), this));
	if(result == partitions_.size()) {
		assert(partitions_.front()->start != document()->region().first);	// twilight context
		return 0;
	}
	if(at.line < document()->numberOfLines() && partitions_[result]->tokenStart == at && result > 0 && at.column == document()->lineLength(at.line))
		--result;
	if(result > 0 && partitions_[result]->start == partitions_[result - 1]->start)
		--result;
	return result;
}

inline ContentType LexicalPartitioner::getTransitionStateAt(const Position& at) const throw() {
	if(at == Position::ZERO_POSITION)
		return DEFAULT_CONTENT_TYPE;
	size_t i = findClosestPartition(at);
	if(partitions_[i]->start == at)
		--i;
	return partitions_[i]->contentType;
}

/**
 *
 * @param line the scanning line text
 * @param column the column number at which match starts
 * @param contentType the current content type
 * @param[out] destination the type of the transition destination content
 * @return the length of the pattern matched or 0 if the all rules did not matched
 */
inline length_t LexicalPartitioner::tryTransition(
		const String& line, length_t column, ContentType contentType, ContentType& destination) const throw() {
	for(TransitionRules::const_iterator rule(rules_.begin()), e(rules_.end()); rule != e; ++rule) {
		if((*rule)->contentType() == contentType) {
			if(const length_t c = (*rule)->matches(line, column)) {
				destination = (*rule)->destination();
				return c;
			}
		}
	}
	destination = UNDETERMINED_CONTENT_TYPE;
	return 0;
}

/// Diagnoses the partitions.
inline void LexicalPartitioner::verify() const {
#ifdef _DEBUG
	assert(!partitions_.empty());
	assert(partitions_.front()->start == document()->region().first);
	bool previousWasEmpty = false;
	for(size_t i = 0, e = partitions_.size(); i < e - 1; ++i) {
		assert(partitions_[i]->contentType != partitions_[i + 1]->contentType);
		if(partitions_[i]->start == partitions_[i + 1]->start) {
			if(previousWasEmpty)
				throw runtime_error("");
			previousWasEmpty = true;
		} else {
			assert(partitions_[i]->start < partitions_[i + 1]->start);
			previousWasEmpty = false;
		}
	}
//	assert(partitions_.back()->start < getDocument()->getEndPosition(false) || partitions_.getSize() == 1);
#endif /* _DEBUG */
}


// LexicalPartitionPresentationReconstructor ////////////////////////////////

/**
 * Constructor.
 * @param document the document
 * @param tokenScanner the token scanner to use for tokenization
 * @param styles token identifier to its text style map. this must include a element has identifier
 * of @c Token#DEFAULT_TOKEN.
 * @throw std#invalid_argument @a styles does not include @c Token#DEFAULT_TOKEN
 */
LexicalPartitionPresentationReconstructor::LexicalPartitionPresentationReconstructor(
		const Document& document, auto_ptr<ITokenScanner> tokenScanner, const map<Token::ID, const TextStyle>& styles)
		: document_(document), tokenScanner_(tokenScanner), styles_(styles) {
	if(styles.find(Token::DEFAULT_TOKEN) == styles.end())
		throw invalid_argument("the given style map does not include Token.DEFAULT_TOKEN.");
}

/// @see presentation#IPartitionPresentationReconstructor#getPresentation
auto_ptr<LineStyle> LexicalPartitionPresentationReconstructor::getPresentation(const Region& region) const throw() {
	list<StyledText> result;
	Position lastTokenEnd = region.beginning();	// the end of the last token
	tokenScanner_->parse(document_, region);
	while(!tokenScanner_->isDone()) {
		auto_ptr<Token> token(tokenScanner_->nextToken());
		if(token.get() == 0)
			break;
		map<Token::ID, const TextStyle>::const_iterator style(styles_.find(token->id));
		if(style != styles_.end()) {
			if(lastTokenEnd != token->region.beginning()) {
				// fill a default style segment between the two tokens
				result.push_back(StyledText());
				result.back().column = lastTokenEnd.column;
				result.back().style = styles_.find(Token::DEFAULT_TOKEN)->second;
			}
			result.push_back(StyledText());
			result.back().column = token->region.first.column;
			result.back().style = style->second;
		}
		lastTokenEnd = token->region.end();
	}
	if(lastTokenEnd != region.end()) {
		// fill a default style segment at the end of the region
		result.push_back(StyledText());
		result.back().column = lastTokenEnd.column;
		result.back().style = styles_.find(Token::DEFAULT_TOKEN)->second;
	}
	auto_ptr<LineStyle> temp(new LineStyle);
	temp->array = new StyledText[temp->count = result.size()];
	copy(result.begin(), result.end(), temp->array);
	return temp;
}
