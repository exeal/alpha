/**
 * @file rules.cpp
 * @author exeal
 * @date 2004-2006 (was Lexer.cpp)
 * @date 2006-2011
 */

#include <ascension/rules.hpp>
#include <ascension/corelib/ustring.hpp>
#include <ascension/corelib/utility.hpp>	// detail.searchBound
using namespace ascension;
using namespace ascension::kernel;
using namespace ascension::presentation;
using namespace ascension::rules;
using namespace ascension::text;
using namespace std;
using detail::HashTable;


namespace {
	// bad idea :(
	template<typename T> inline bool inRange(T v, T b, T e) {return v >= b && v <= e;}
	template<typename T> struct InRange : unary_function<T, bool> {
		InRange(T first, T last) : f(first), l(last) {}
		bool operator()(T v) const {return inRange(v, f, l);}
		T f, l;
	};
}

// detail.HashTable ///////////////////////////////////////////////////////////////////////////////

namespace ascension {
	namespace detail {
		class HashTable {
			ASCENSION_NONCOPYABLE_TAG(HashTable);
		public:
			template<typename StringSequence>
			HashTable(StringSequence first, StringSequence last, bool caseSensitive);
			~HashTable() /*throw()*/;
			template<typename CharacterSequence>
			static uint32_t hashCode(CharacterSequence first, CharacterSequence last);
			bool matches(const Char* first, const Char* last) const;
			size_t maximumLength() const /*throw()*/ {return maxLength_;}
		private:
			struct Entry {
				String data;
				Entry* next;
				explicit Entry(const String& str) /*throw()*/ : data(str) {}
				~Entry() /*throw()*/ {delete next;}
			};
			Entry** entries_;
			size_t numberOfEntries_;
			size_t maxLength_;	// the length of the longest keyword
			const bool caseSensitive_;
		};
	}
} // namespace ascension.detail

/**
 * Constructor.
 * @tparam StringSequence A type of @a first and @a last
 * @param first The start of the strings
 * @param last The end of the strings
 * @param caseSensitive Set @c true to enable case sensitive match
 */
template<typename StringSequence>
HashTable::HashTable(StringSequence first, StringSequence last, bool caseSensitive)
		: numberOfEntries_(std::distance(first, last)), maxLength_(0), caseSensitive_(caseSensitive) {
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
HashTable::~HashTable() /*throw()*/ {
	for(size_t i = 0; i < numberOfEntries_; ++i)
		delete entries_[i];
	delete[] entries_;
}

/**
 * Returns the hash value of the specified string.
 * @tparam CharacterSequence A type of @a first and @a last. Must represent UTF-16 character
 *                           sequence.
 * @param first The start of the character sequence
 * @param last The end of the character sequence
 * @return The hash value
 */
template<typename CharacterSequence>
inline uint32_t HashTable::hashCode(CharacterSequence first, CharacterSequence last) {
	ASCENSION_STATIC_ASSERT(sizeof(*first) == 2);
	uint32_t h = 0;
	while(first < last) {
		h *= 2;
		h += *first;
		++first;
	}
	return h;
}

/**
 * Searches the specified string.
 * @param first The start of the string
 * @param last The end of the string
 * @return @c true if the specified string is found
 */
bool HashTable::matches(const Char* first, const Char* last) const {
	if(caseSensitive_) {
		if(static_cast<size_t>(last - first) > maxLength_)
			return false;
		const size_t h = hashCode(first, last);
		for(Entry* entry = entries_[h % numberOfEntries_]; entry != 0; entry = entry->next) {
			if(entry->data.length() == static_cast<size_t>(last - first) && umemcmp(entry->data.data(), first, entry->data.length()) == 0)
				return true;
		}
	} else {
		const String folded(CaseFolder::fold(String(first, last)));
		const size_t h = hashCode(folded.begin(), folded.end());
		for(Entry* entry = entries_[h % numberOfEntries_]; entry != 0; entry = entry->next) {
			if(entry->data.length() == folded.length() && umemcmp(entry->data.data(), folded.data(), folded.length()) == 0)
				return true;
		}
	}
	return false;
}


// URIDetector ////////////////////////////////////////////////////////////////////////////////////

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
		static const Char SUB_DELIMS[] = {'!', '$', '&', '\'', '(', ')', '*', '+', ',', ';', '=', 0};
		return (first < last && ustrchr(SUB_DELIMS, *first) != 0) ? ++first : 0;
	}

	// gen-delims = ":" / "/" / "?" / "#" / "[" / "]" / "@"
	inline const Char* handleGenDelims(const Char* first, const Char* last) {
		static const Char GEN_DELIMS[] = {':', '/', '?', '#', '[', ']', '@', 0};
		return (first < last && ustrchr(GEN_DELIMS, *first) != 0) ? ++first : 0;
	}

	// reserved = gen-delims / sub-delims
	inline const Char* handleReserved(const Char* first, const Char* last) {
		return (handleGenDelims(first, last) != 0 || handleSubDelims(first, last) != 0) ? ++first : 0;
	}

	// unreserved = ALPHA / DIGIT / "-" / "." / "_" / "~"
	inline const Char* handleUnreserved(const Char* first, const Char* last) {
		static const Char UNRESERVED_LEFTOVERS[] = {'-', '.', '_', '~', 0};
		return (first < last && (isalnum(*first, cl) || ustrchr(UNRESERVED_LEFTOVERS, *first) != 0)) ? ++first : 0;
	}

	// pct-encoded = "%" HEXDIG HEXDIG
	inline const Char* handlePctEncoded(const Char* first, const Char* last) {
		return (last - first >= 3 && first[0] == '%' && isxdigit(first[1], cl) && isxdigit(first[2], cl)) ? first += 3 : 0;
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
		if(last - first >= 4 && *first == 'v' && isxdigit(*++first, cl)) {
			while(isxdigit(*first, cl)) {
				if(++first == last)
					return 0;
			}
			if(*first == '.' && ++first < last) {
				const Char* p = first;
				while(p < last) {
					if(0 != (p = handleUnreserved(p, last)) || (0 != (p = handleSubDelims(p, last))))
						continue;
					else if(*p == ':')
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
		if(first < last && *first == '[') {
			const Char* p;
			if(0 != (p = handleIPv6address(++first, last)) || 0 != (p = handleIPvFuture(first, last))) {
				if(p < last && *p == ']')
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
			if(*first == '0')
				return ++first;
			else if(*first == '1')
				return (++first < last && isdigit(*first, cl) && ++first < last && isdigit(*first, cl)) ? ++first : first;
			else if(*first == '2') {
				if(++first < last) {
					if(inRange<Char>(*first, '0', '4')) {
						if(isdigit(*++first, cl))
							++first;
					} else if(*first == '5') {
						if(inRange<Char>(*first, '0', '5'))
							++first;
					}
				}
				return first;
			} else if(inRange<Char>(*first, '3', '9'))
				return (++first < last && isdigit(*first, cl)) ? ++first : first;
		}
		return 0;
	}

	// IPv4address = dec-octet "." dec-octet "." dec-octet "." dec-octet
	inline const Char* handleIPv4address(const Char* first, const Char* last) {
		return (last - first >= 7
			&& 0 != (first = handleDecOctet(first, last))
			&& first < last && *first == '.'
			&& 0 != (first = handleDecOctet(++first, last))
			&& first < last && *first == '.'
			&& 0 != (first = handleDecOctet(++first, last))
			&& first < last && *first == '.'
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
		return ((0 != (p = handleH16(first, last)) && p + 2 < last && *++p == ':' && 0 != (p = handleH16(p, last)))
			|| (0 != (p = handleIPv4address(first, last)))) ? p : 0;
	}

	// scheme = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
	const Char* ASCENSION_FASTCALL handleScheme(const Char* first, const Char* last) {
		if(isalpha(*first, cl)) {
			while(++first != last) {
				if(!isalnum(*first, cl) && *first != '+' && *first != '-' && *first != '.')
					return first;
			}
			return last;
		}
		return 0;
	}

	// iprivate = %xE000-F8FF / %xF0000-FFFFD / %x100000-10FFFD
	inline const Char* handlePrivate(const Char* first, const Char* last) {
		if(first < last) {
			if(inRange<Char>(*first, 0xe000, 0xf8ff))
				return ++first;
			const CodePoint c = utf16::decodeFirst(first, last);
			if(inRange<CodePoint>(c, 0xf0000u, 0xffffdu) || inRange<CodePoint>(c, 0x100000u, 0x10fffdu))
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
			if(inRange<Char>(*first, 0x00a0, 0xd7ff) || inRange<Char>(*first, 0xf900, 0xfdcf) || inRange<Char>(*first, 0xfdf0, 0xffef))
				return ++first;
			const CodePoint c = utf16::decodeFirst(first, last);
			if(c >= 0x10000 && c < 0xf0000 && (c & 0xffff) >= 0x0000 && (c & 0xffff) <= 0xfffd) {
				if((c & 0xf0000) != 0xe || (c & 0xffff) >= 0x1000)
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
			else if(*first == ':' || *first == '@')
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
			else if(*first == '@')
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
		while(first < last && *first == '/')
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
		return (first < last && *first == '/') ? handlePathRootless(++first, last) : 0;
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
			else if(*first == ':')
				++first;
			else
				break;
		}
		return first;
	}

	// iauthority = [ iuserinfo "@" ] ihost [ ":" port ]
	const Char* ASCENSION_FASTCALL handleAuthority(const Char* first, const Char* last) {	// [nullable]
		const Char* const beginning = first;
		first = handleUserinfo(first, last);
		assert(first != 0);
		if(first > beginning) {
			if(first >= last || *++first != '@')
				first = beginning;
		} else if(first < last && *++first != '@')
			--first;
		if(0 != (first = handleHost(first, last))) {
			if(first != last) {
				if(*first == ':')
					++first;
				first = handlePort(first, last);
				assert(first != 0);
			}
		}
		return first;
	}

	// ihier-part = ("//" iauthority ipath-abempty) / ipath-absolute / ipath-rootless / ipath-empty
	const Char* ASCENSION_FASTCALL handleHierPart(const Char* first, const Char* last) {
		static const Char DOUBLE_SLASH[] = {'/', '/', 0};
		const Char* eop;
		(last - first > 2 && umemcmp(first, DOUBLE_SLASH, 2) == 0
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
			else if(*first == '/' || *first == '?')
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
			else if(*first == '/' || *first == '?')
				++first;
			else
				break;
		}
		return first;
	}

	// IRI = scheme ":" ihier-part [ "?" iquery ] [ "#" ifragment ]
	const Char* ASCENSION_FASTCALL handleIRI(const Char* first, const Char* last) {
		if(0 != (first = handleScheme(first, last))) {
			if(*first == ':') {
				if(0 != (first = handleHierPart(++first, last))) {
					if(*first == '?') {
						first = handleQuery(++first, last);
						assert(first != 0);
					}
					if(*first == '#') {
						first = handleFragment(++first, last);
						assert(first != 0);
					}
					return first;
				}
			}
		}
		return 0;
	}
} // namespace @0

/// Constructor. The set of the valid schemes is empty.
URIDetector::URIDetector() /*throw()*/ : validSchemes_(0) {
}

/// Destructor.
URIDetector::~URIDetector() /*throw()*/ {
	delete validSchemes_;
}

/// Returns the default generic instance.
const URIDetector& URIDetector::defaultGenericInstance() /*throw()*/ {
	static URIDetector singleton;
	return singleton;
}

/**
 * Returns the default instance accepts URI schemes defined by IANA
 * (http://www.iana.org/assignments/uri-schemes.html).
 */
const URIDetector& URIDetector::defaultIANAURIInstance() /*throw()*/ {
	static URIDetector singleton;
	if(singleton.validSchemes_ == 0) {
		const char SCHEMES[] =
			// permanent URI schemes
			"aaa|aaas|acap|cap|cid|crid|data|dav|dict|dns|fax|file|ftp|go|gopher|h323|http|https"
			"|icap|im|imap|info|ipp|iris|iris.beep|iris.xpc|iris.xpcs|iris.lwz|ldap"
			"|mailto|mid|modem|msrp|msrps|mtqp|mupdate|news|nfs|nntp|opaquelocktoken|pop|pres|rtsp"
			"|service|shttp|sip|sips|snmp|soap.beep|soap.beeps|tag|tel|telnet|tftp|thismessage|tip|tv"
			"|urn|vemmi|xmlrpc.beep|xmlrpc.beeps|xmpp|z39.50r"
			// provisional URI schemes
			"|afs|dtn|iax2|mailserver|pack|tn3270"
			// historical URI schemes
			"prospero|wais";
		singleton.setValidSchemes(String(SCHEMES, ASCENSION_ENDOF(SCHEMES)), '|');
	}
	return singleton;
}

/**
 * Returns the end of a URL begins at the given position.
 * @param first The beginning of the character sequence
 * @param last The end of the character sequence
 * @return The end of the detected URI, or @a first (not @c null) if not matched
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
		endOfScheme = umemchr(first + 1, ':', min<size_t>(last - first - 1, validSchemes_->maximumLength()));
		if(!validSchemes_->matches(first, endOfScheme))
			endOfScheme = 0;
	} else {
		endOfScheme = umemchr(first + 1, ':', static_cast<size_t>(last - first - 1));
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
 * @param first The beginning of the character sequence
 * @param last The end of the character sequence
 * @param[out] result The range of the found URI in the target character sequence
 * @return @c true if a URI was found
 * @throw NullPointerException @a first and/or @a last are @c null
 */
bool URIDetector::search(const Char* first, const Char* last, Range<const Char*>& result) const {
	if(first == 0)
		throw NullPointerException("first");
	else if(last == 0)
		throw NullPointerException("last");
	else if(first >= last)
		return false;

	// search scheme
	const Char* nextColon = umemchr(first, ':', last - first);
	if(nextColon == 0)
		return false;
	while(true) {
		if(handleScheme(first, nextColon) == nextColon) {
			if(validSchemes_ == 0 || validSchemes_->matches(first, nextColon)) {
				if(const Char* const e = handleIRI(first, last)) {
					result = Range<const Char*>(first, e);
					return true;
				}
			}
			first = nextColon;
		} else
			++first;
		if(first == nextColon) {
			first = nextColon;
			nextColon = umemchr(first, ':', last - first);
			if(nextColon == 0)
				break;
		}
	}
	return false;
}

/**
 * Sets the valid schemes.
 * @param scheme The set of the schemes to set
 * @param caseSensitive Set @c true to use case-sensitive comparison for scheme name matching.
 *                      However, RFC 3986 Section 3.1 says that schemes are case-insensitive
 * @return The detector
 * @throw std#invalid_argument Invalid name as a scheme was found
 */
URIDetector& URIDetector::setValidSchemes(const set<String>& schemes, bool caseSensitive /* = false */) {
	// validation
	for(set<String>::const_iterator i(schemes.begin()), e(schemes.end()); i != e; ++i) {
		const Char* const p = i->data();
		if(handleScheme(p, p + i->length()) != p + i->length())
			throw invalid_argument("schemes");
	}

	// rebuild hash table
	HashTable* newSchemes = new HashTable(schemes.begin(), schemes.end(), !caseSensitive);
	delete validSchemes_;
	validSchemes_ = newSchemes;

	return *this;
}

/**
 * Sets the valid schemes.
 * @param scheme The string contains the schemes separated by @a separator
 * @param caseSensitive Set @c true to use case-sensitive comparison for scheme name matching
 * @param separator The character delimits scheme names in @a schemes. this can be a surrogate
 * @return The detector
 * @throw std#invalid_argument Invalid name as a scheme was found
 */
URIDetector& URIDetector::setValidSchemes(const String& schemes, Char separator, bool caseSensitive /* = false */) {
	set<String> container;
	for(length_t previous = 0, next; previous < schemes.length(); previous = next + 1) {
		next = schemes.find(separator, previous);
		if(next == String::npos)
			next = schemes.length();
		if(next > previous)
			container.insert(schemes.substr(previous, next - previous));
	}
	return setValidSchemes(container, caseSensitive);
}


// Token //////////////////////////////////////////////////////////////////////////////////////////

const Token::Identifier Token::UNCALCULATED = static_cast<Token::Identifier>(-1);


// Rule ///////////////////////////////////////////////////////////////////////////////////////////

/**
 * Protected constructor.
 * @param tokenID The identifier of the token which will be returned by the rule. Can't be
 *                @c Token#UNCALCULATED which is for internal use
 * @throw std#invalid_argument @a tokenID is invalid
 */
Rule::Rule(Token::Identifier tokenID) : id_(tokenID) {
	if(tokenID == Token::UNCALCULATED)
		throw invalid_argument("tokenID");
}


// RegionRule /////////////////////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param id The identifier of the token which will be returned by the rule
 * @param startSequence The pattern's start sequence
 * @param endSequence The pattern's end sequence. if empty, token will end at end of line
 * @param escapeCharacter The character which a character will be ignored
 * @param caseSensitive Set @c false to enable caseless match
 * @throw std#invalid_argument @a startSequence is empty
 */
RegionRule::RegionRule(Token::Identifier id, const String& startSequence, const String& endSequence,
		Char escapeCharacter /* = NONCHARACTER */, bool caseSensitive /* = true */) : Rule(id),
		startSequence_(startSequence), endSequence_(endSequence), escapeCharacter_(escapeCharacter), caseSensitive_(caseSensitive) {
	if(startSequence.empty())
		throw invalid_argument("the start sequence is empty.");
}

/// @see Rule#parse
auto_ptr<Token> RegionRule::parse(const TokenScanner& scanner, const Char* first, const Char* last) const /*throw()*/ {
	// match the start sequence
	if(first[0] != startSequence_[0]
			|| static_cast<size_t>(last - first) < startSequence_.length() + endSequence_.length()
			|| (startSequence_.length() > 1 && umemcmp(first + 1, startSequence_.data() + 1, startSequence_.length() - 1) != 0))
		return auto_ptr<Token>(0);
	const Char* end = last;
	if(!endSequence_.empty()) {
		// search the end sequence
		for(const Char* p = first + startSequence_.length(); p <= last - endSequence_.length(); ++p) {
			if(escapeCharacter_ != NONCHARACTER && *p == escapeCharacter_)
				++p;
			else if(*p == endSequence_[0] && umemcmp(p + 1, endSequence_.data() + 1, endSequence_.length() - 1) == 0) {
				end = p + endSequence_.length();
				break;
			}
		}
	}
	auto_ptr<Token> result(new Token);
	result->id = tokenID();
	result->region.first.line = result->region.second.line = scanner.getPosition().line;
	result->region.first.column = scanner.getPosition().line;
	result->region.second.column = result->region.first.column + (end - first);
	return result;
}


// NumberRule /////////////////////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param id The identifier of the token which will be returned by the rule
 */
NumberRule::NumberRule(Token::Identifier id) /*throw()*/ : Rule(id) {
}

/// @see Rule#parse
auto_ptr<Token> NumberRule::parse(const TokenScanner& scanner, const Char* first, const Char* last) const /*throw()*/ {
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
	if(scanner.getPosition().column > 0	// see below
			&& (inRange<Char>(first[-1], '0', '9') || inRange<Char>(first[-1], 'A', 'F') || inRange<Char>(first[-1], 'a', 'f')))
		return auto_ptr<Token>(0);
	const Char* e;
	if(last - first > 2 && first[0] == '0' && (first[1] == 'x' || first[1] == 'X')) {	// HexIntegerLiteral?
		for(e = first + 2; e < last; ++e) {
			if(inRange<Char>(*e, '0', '9') || inRange<Char>(*e, 'A', 'F') || inRange<Char>(*e, 'a', 'f'))
				continue;
			break;
		}
		if(e == first + 2)
			return auto_ptr<Token>(0);
	} else {	// DecimalLiteral?
		bool foundDecimalIntegerLiteral = false, foundDot = false;
		if(inRange<Char>(first[0], '0', '9')) {	// DecimalIntegerLiteral ::= /0|[1-9][0-9]*/
			e = first + 1;
			foundDecimalIntegerLiteral = true;
			if(first[0] != '0')
				e = find_if(e, last, not1(InRange<Char>('0', '9')));
		} else
			e = first;
		if(e < last && *e == '.') {	// . DecimalDigits ::= /\.[0-9]+/
			foundDot = true;
			e = find_if(++e, last, not1(InRange<Char>('0', '9')));
			if(e[-1] == '.')
				return auto_ptr<Token>(0);
		}
		if(!foundDecimalIntegerLiteral && !foundDot)
			return auto_ptr<Token>(0);
		if(e < last && (*e == 'e' || *e == 'E')) {	// ExponentPart ::= /[e|E][\+\-]?[0-9]+/
			if(++e == last)
				return auto_ptr<Token>(0);
			if(*e == '+' || *e == '-') {
				if(++e == last)
					return auto_ptr<Token>(0);
			}
			e = find_if(++e, last, not1(InRange<Char>('0', '9')));
		}
	}

	// e points the end of the found token
	assert(e > first);
	// "The source character immediately following a NumericLiteral must not be an IdentifierStart or DecimalDigit."
	if(e < last && (inRange<Char>(*e, '0', '9') || scanner.getIdentifierSyntax().isIdentifierStartCharacter(utf16::decodeFirst(e, last))))
		return auto_ptr<Token>(0);

	auto_ptr<Token> temp(new Token);
	temp->id = tokenID();
	temp->region.first.line = temp->region.second.line = scanner.getPosition().line;
	temp->region.first.column = scanner.getPosition().column;
	temp->region.second.column = temp->region.first.column + e - first;
	return temp;
}


// URIRule ////////////////////////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param id The identifier of the token which will be returned by the rule
 * @param uriDetector The URI detector
 */
URIRule::URIRule(Token::Identifier id, const URIDetector& uriDetector,
		bool delegateOwnership) /*throw()*/ : Rule(id), uriDetector_(&uriDetector, delegateOwnership) {
}

/// @see Rule#parse
auto_ptr<Token> URIRule::parse(const TokenScanner& scanner, const Char* first, const Char* last) const /*throw()*/ {
	assert(first < last);
	const Char* const e = uriDetector_->detect(first, last);
	if(e == first)
		return auto_ptr<Token>(0);
	auto_ptr<Token> temp(new Token);
	temp->id = tokenID();
	temp->region.first.line = temp->region.second.line = scanner.getPosition().line;
	temp->region.first.column = scanner.getPosition().column;
	temp->region.second.column = temp->region.first.column + e - first;
	return temp;
}


// WordRule ///////////////////////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param id The identifier of the token which will be returned by the rule
 * @param first The start of the words
 * @param last The end of the words
 * @param caseSensitive Set @c false to enable caseless match
 * @throw NullPointerException @a first and/or @a last are @c null
 * @throw std#invalid_argument @a first &gt;= @a last
 */
WordRule::WordRule(Token::Identifier id, const String* first, const String* last, bool caseSensitive /* = true */) : Rule(id) {
	if(first == 0)
		throw NullPointerException("first");
	else if(last == 0)
		throw NullPointerException("last");
	else if(first >= last)
		throw invalid_argument("first >= last");
	words_ = new HashTable(first, last, caseSensitive);
}

/**
 * Constructor.
 * @param id The identifier of the token which will be returned by the rule
 * @param first The start of the string
 * @param last The end of the string
 * @param separator The separator character in the string
 * @param caseSensitive Set @c false to enable caseless match
 * @throw NullPointerException @a first and/or @a last are @c null
 * @throw std#invalid_argument @a first &gt; last, or @a separator is a surrogate
 */
WordRule::WordRule(Token::Identifier id, const Char* first, const Char* last, Char separator, bool caseSensitive) : Rule(id) {
	if(first == 0)
		throw NullPointerException("first");
	if(last == 0)
		throw NullPointerException("last");
	else if(first >= last)
		throw invalid_argument("first >= last");
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
WordRule::~WordRule() /*throw()*/ {
	delete words_;
}

/// @see Rule#parse
auto_ptr<Token> WordRule::parse(const TokenScanner& scanner, const Char* first, const Char* last) const {
	if(!words_->matches(first, last))
		return auto_ptr<Token>(0);
	auto_ptr<Token> result(new Token);
	result->id = tokenID();
	result->region.first.line = result->region.second.line = scanner.getPosition().line;
	result->region.first.column = scanner.getPosition().column;
	result->region.second.column = result->region.first.column + (last - first);
	return result;
}


#ifndef ASCENSION_NO_REGEX

// RegexRule //////////////////////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param id The identifier of the token which will be returned by the rule
 * @param pattern The compiled regular expression
 * @throw regex#PatternSyntaxException The specified pattern is invalid
 */
RegexRule::RegexRule(Token::Identifier id, auto_ptr<const regex::Pattern> pattern) : Rule(id), pattern_(pattern) {
}

/// @see Rule#parse
auto_ptr<Token> RegexRule::parse(const TokenScanner& scanner, const Char* first, const Char* last) const {
	const UTF16To32Iterator<const Char*> b(first, last), e(first, last, last);
	auto_ptr<regex::Matcher<UTF16To32Iterator<const Char*> > > matcher(pattern_->matcher(b, e));
	if(!matcher->lookingAt())
		return auto_ptr<Token>(0);
	auto_ptr<Token> token(new Token);
	token->id = tokenID();
	token->region.first.line = token->region.second.line = scanner.getPosition().line;
	token->region.first.column = scanner.getPosition().column;
	token->region.second.column = token->region.first.column + (matcher->end().tell() - matcher->start().tell());
	return token;
}

#endif // !ASCENSION_NO_REGEX


// NullTokenScanner ///////////////////////////////////////////////////////////////////////////////

/// @see TokenScanner#getIdentifierSyntax
const IdentifierSyntax& NullTokenScanner::getIdentifierSyntax() const /*throw()*/ {
	return IdentifierSyntax::defaultInstance();
}

/// @see TokenScanner#getPosition
Position NullTokenScanner::getPosition() const /*throw()*/ {
	return Position();
}

/// @see TokenScanner#hasNext
bool NullTokenScanner::hasNext() const /*throw()*/ {
	return false;
}

/// @see TokenScanner#nextToken
auto_ptr<Token> NullTokenScanner::nextToken() {
	return auto_ptr<Token>(0);
}

/// @see TokenScanner#parse
void NullTokenScanner::parse(const Document&, const Region&) {
}


// LexicalTokenScanner ////////////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param contentType The content the scanner parses
 */
LexicalTokenScanner::LexicalTokenScanner(ContentType contentType) /*throw()*/ : contentType_(contentType), current_() {
}

/// Destructor.
LexicalTokenScanner::~LexicalTokenScanner() /*throw()*/ {
	for(list<const Rule*>::iterator i = rules_.begin(); i != rules_.end(); ++i)
		delete *i;
	for(list<const WordRule*>::iterator i = wordRules_.begin(); i != wordRules_.end(); ++i)
		delete *i;
}

/// @see ITokenScanner#getIdentifierSyntax
const IdentifierSyntax& LexicalTokenScanner::getIdentifierSyntax() const /*throw()*/ {
	return current_.document()->contentTypeInformation().getIdentifierSyntax(contentType_);
}

/**
 * Adds the new rule to the scanner.
 * @param rule The rule to be added
 * @throw NullPointerException @a rule is @c null
 * @throw std#invalid_argument @a rule is already registered
 * @throw BadScannerStateException The scanner is running
 */
void LexicalTokenScanner::addRule(auto_ptr<const Rule> rule) {
	if(rule.get() == 0)
		throw NullPointerException("rule");
	else if(hasNext())
		throw BadScannerStateException();
	else if(find(rules_.begin(), rules_.end(), rule.get()) != rules_.end())
		throw invalid_argument("the rule is already registered.");
	rules_.push_back(rule.release());
}

/**
 * Adds the new word rule to the scanner.
 * @param rule The rule to be added
 * @throw NullPointerException @a rule is @c null
 * @throw std#invalid_argument @a rule is already registered
 * @throw BadScannerStateException The scanner is running
 */
void LexicalTokenScanner::addWordRule(auto_ptr<const WordRule> rule) {
	if(rule.get() == 0)
		throw NullPointerException("rule");
	else if(hasNext())
		throw BadScannerStateException();
	else if(find(wordRules_.begin(), wordRules_.end(), rule.get()) != wordRules_.end())
		throw invalid_argument("the rule is already registered.");
	wordRules_.push_back(rule.release());
}

/// @see ITokenScanner#getPosition
Position LexicalTokenScanner::getPosition() const /*throw()*/ {
	return current_.tell();
}

/// @see ITokenScanner#hasNext
bool LexicalTokenScanner::hasNext() const /*throw()*/ {
	return current_.hasNext();
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


// TransitionRule /////////////////////////////////////////////////////////////////////////////////

/**
 * Protected constructor.
 * @param contentType The content type of the transition source
 * @param destination The content type of the transition destination
 */
TransitionRule::TransitionRule(ContentType contentType,
		ContentType destination) : contentType_(contentType), destination_(destination) /*throw()*/ {
}

/// Destructor.
TransitionRule::~TransitionRule() /*throw()*/ {
}

/**
 * @fn TransitionRule::clone
 * Creates and returns a copy of the object.
 * @return A copy of the object
 */

/**
 * @fn TransitionRule::matches
 * Returns @c true if the rule matches the specified text. Note that an implementation can't use
 * the partitioning of the document to generate the new partition.
 * @param line The target line text
 * @param column The column number at which match starts
 * @return The length of the matched pattern. If and only if the match failed, returns 0.
 *         If matched zero width text, returns 1
 * @todo This documentation is confusable.
 */


// LiteralTransitionRule //////////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param contentType The content type of the transition source
 * @param destination The content type of the transition destination
 * @param pattern The pattern string to introduce the transition. If empty string is specified, the
 *                transition will be occurred at the end of line
 * @param escapeCharacter The character which a character will be ignored. If @c NONCHARACTER is
 *                        specified, the escape character will be not set. This is always
 *                        case-sensitive
 * @param caseSensitive Set @c false to enable caseless match
 */
LiteralTransitionRule::LiteralTransitionRule(ContentType contentType, ContentType destination,
		const String& pattern, Char escapeCharacter /* = NONCHARACTER */, bool caseSensitive /* = true */) :
		TransitionRule(contentType, destination), pattern_(pattern), escapeCharacter_(escapeCharacter), caseSensitive_(caseSensitive) {
}

/// @see TransitionRule#clone
auto_ptr<TransitionRule> LiteralTransitionRule::clone() const {
	return auto_ptr<TransitionRule>(new LiteralTransitionRule(*this));
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
		return (umemcmp(pattern_.data(), line.data() + column, pattern_.length()) == 0) ? pattern_.length() : 0;
	return (CaseFolder::compare(StringCharacterIterator(pattern_),
		StringCharacterIterator(line, line.begin() + column)) == 0) ? pattern_.length() : 0;
}


#ifndef ASCENSION_NO_REGEX

// RegexTransitionRule ////////////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param contentType The content type of the transition source
 * @param destination The content type of the transition destination
 * @param pattern The compiled regular expression to introduce the transition
 * @throw regex#PatternSyntaxException @a pattern is invalid
 */
RegexTransitionRule::RegexTransitionRule(ContentType contentType, ContentType destination,
		auto_ptr<const regex::Pattern> pattern) : TransitionRule(contentType, destination), pattern_(pattern) {
}

/// Copy-constructor.
RegexTransitionRule::RegexTransitionRule(const RegexTransitionRule& other) :
		TransitionRule(other), pattern_(new regex::Pattern(*other.pattern_.get())) {
}

/// @see TransitionRule#clone
auto_ptr<TransitionRule> RegexTransitionRule::clone() const {
	return auto_ptr<TransitionRule>(new RegexTransitionRule(*this));
}

/// @see TransitionRule#matches
length_t RegexTransitionRule::matches(const String& line, length_t column) const {
	try {
		typedef UTF16To32Iterator<String::const_iterator> I;
		auto_ptr<regex::Matcher<I> > matcher(pattern_->matcher(I(line.begin(), line.end()), I(line.begin(), line.end(), line.end())));
		matcher->region(I(line.begin(), line.end(), line.begin() + column), matcher->regionEnd());
		matcher->useAnchoringBounds(false).useTransparentBounds(true);
		return matcher->lookingAt() ? max(matcher->end().tell() - matcher->start().tell(), 1) : 0;
	} catch(runtime_error&) {
		return 0;
	}
}

#endif // !ASCENSION_NO_REGEX


// LexicalPartitioner /////////////////////////////////////////////////////////////////////////////

/// Constructor.
LexicalPartitioner::LexicalPartitioner() /*throw()*/ {
}

/// Destructor.
LexicalPartitioner::~LexicalPartitioner() /*throw()*/ {
	deleteRules(rules_);
	for(size_t i = 0, c = partitions_.size(); i < c; ++i)
		delete partitions_[i];
}

/// @internal Deletes all the transition rules.
void LexicalPartitioner::deleteRules(list<const TransitionRule*>& rules) /*throw()*/ {
	for(list<const TransitionRule*>::const_iterator i(rules.begin()); i != rules.end(); ++i)
		delete *i;
}

/**
 * Computes and constructs the partitions on the specified region.
 * @param start The start of the region to compute
 * @param minimalLast The partitioner must scan to this position at least
 * @param[out] changedRegion The region whose content type was changed
 */
void LexicalPartitioner::computePartitioning(const Position& start, const Position& minimalLast, Region& changedRegion) {
	// TODO: see LexicalPartitioner.documentChanged.
}

/// @see kernel#DocumentPartitioner#documentAboutToBeChanged
void LexicalPartitioner::documentAboutToBeChanged() /*throw()*/ {
}

/// @see kernel#DocumentPartitioner#documentChanged
void LexicalPartitioner::documentChanged(const DocumentChange& change) /*throw()*/ {
	// this code reconstructs partitions in the region changed by the document modification using
	// the registered partitioning rules

//	assert(!change.erasedRegion().isEmpty() || !change.insertedRegion().isEmpty());
//	if(change.region().isEmpty())
//		return;
	// TODO: there is more efficient implementation using LexicalPartitioner.computePartitioning.
	const Document& doc = *document();

	// move the partitions adapting to the document change. this does not affect partitions out of
	// the deleted region
	if(!change.erasedRegion().isEmpty()) {
		for(size_t i = 1, c = partitions_.size(); i < c; ++i) {
			Partition& p = *partitions_[i];
			if(p.start < change.erasedRegion().beginning())
				continue;
			else if(p.start > change.erasedRegion().end()) {
				p.start = positions::updatePosition(p.start, change, Direction::FORWARD);
				p.tokenStart = positions::updatePosition(p.tokenStart, change, Direction::FORWARD);
			} else if(((i + 1 < c) ? partitions_[i + 1]->start : doc.region().end()) <= change.erasedRegion().end()) {
				// this partition is encompassed with the deleted region
				delete partitions_[i];
				partitions_.erase(i);
				if(i < c - 1 && partitions_[i]->contentType == partitions_[i - 1]->contentType) {
					delete partitions_[i];
					partitions_.erase(i);
					--c;
				}
				--i;
				if(--c == 1)
					break;
			} else
				// this partition will be erased later
				p.start = p.tokenStart = change.erasedRegion().beginning();
		}
	}
	if(!change.insertedRegion().isEmpty()) {
		for(size_t i = 1, c = partitions_.size(); i < c; ++i) {
			Partition& p = *partitions_[i];
			p.start = positions::updatePosition(p.start, change, Direction::FORWARD);
			p.tokenStart = positions::updatePosition(p.tokenStart, change, Direction::FORWARD);
		}
	}
	verify();

	// compute partitioning for the affected region using the registered rules
	vector<Partition*> newPartitions;	// newly computed partitions for the affected region
	DocumentCharacterIterator i(doc,	// the beginning of the region to parse ~ the end of the document
		Region(Position(min(change.erasedRegion().beginning().line, change.insertedRegion().beginning().line), 0), doc.region().end()));
	ContentType contentType, destination;
	contentType = (i.tell().line == 0) ? DEFAULT_CONTENT_TYPE
		: (*partitionAt(Position(i.tell().line - 1, doc.lineLength(i.tell().line - 1))))->contentType;
	for(const String* line = &doc.line(i.tell().line); ; ) {	// scan and tokenize into partitions...
		const bool atEOL = i.tell().column == line->length();
		length_t tokenLength = tryTransition(*line, i.tell().column, contentType, destination);
		if(tokenLength != 0) {	// a token was found
			if(atEOL)
				tokenLength = 0;	// a line terminator is zero-length...
			const Position tokenEnd(i.tell().line, i.tell().column + tokenLength);
			// insert the new partition behind the current
			assert(destination != contentType);
			newPartitions.push_back(new Partition(
				destination, (destination > contentType) ? i.tell() : tokenEnd, i.tell(), tokenLength));
			contentType = destination;
			// go to the end of the found token
			if(!atEOL)
				i.seek(tokenEnd);
		}
		// this loop can end at only EOL
		if(atEOL) {
			// the end of the document
			if(!i.hasNext())
				break;
			// if reached the end of the affected region and content types are same, we are done
			else if(i.tell() >= max(change.erasedRegion().second, change.insertedRegion().second) && transitionStateAt(i.tell()) == contentType)
				break;
		}
		// go to the next character if no transition occurred
		if(tokenLength == 0) {
			i.next();
			if(i.tell().column == 0)	// entered the next line
				line = &doc.line(i.tell().line);
		}
	}

	// replace partitions encompassed with the affected region
	erasePartitions(i.region().beginning(), i.tell());
	partitions_.insert(partitionAt(i.region().beginning()) + 1, newPartitions.begin(), newPartitions.end());

#ifdef _DEBUG
	static bool trace = false;
	if(trace)
		dump();
#endif
	verify();
	notifyDocument(Region(Position(min(
		change.erasedRegion().beginning().line, change.insertedRegion().beginning().line), 0), i.tell()));
}

/// @see kernel#DocumentPartitioner#doGetPartition
void LexicalPartitioner::doGetPartition(const Position& at, DocumentPartition& partition) const /*throw()*/ {
	detail::GapVector<Partition*>::const_iterator i(partitionAt(at));
	const Partition& p = **i;
	partition.contentType = p.contentType;
	partition.region.first = p.start;
	partition.region.second = (i < partitions_.end() - 1) ? (*++i)->start : document()->region().second;
}

/// @see kernel#DocumentPartitioner#doInstall
void LexicalPartitioner::doInstall() /*throw()*/ {
	for(size_t i = 0, c = partitions_.size(); i < c; ++i)
		delete partitions_[i];
	partitions_.clear();
	partitions_.insert(0, new Partition(DEFAULT_CONTENT_TYPE, Position(0, 0), Position(0, 0), 0));
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
			<< static_cast<uint32_t>(p.start.line) << ", " << static_cast<uint32_t>(p.start.column) << ")\n";
	}
#endif // _DEBUG
}

// erases partitions encompassed with the region between the given two positions.
void LexicalPartitioner::erasePartitions(const Position& first, const Position& last) {
	// locate the first partition to delete
	size_t deletedFirst = partitionAt(first) - partitions_.begin();
	if(first >= partitions_[deletedFirst]->getTokenEnd())
		++deletedFirst;	// do not delete this partition
//	else if(deletedFirst < partitions_.getSize() - 1 && partitions_[deletedFirst + 1]->tokenStart < change.getRegion().getBottom())
//		++deletedFirst;	// delete from the next partition
	// locate the last partition to delete
	size_t deletedLast = partitionAt(last) - partitions_.begin() + 1;	// exclusive
	if(deletedLast < partitions_.size() && partitions_[deletedLast]->tokenStart < last)
		++deletedLast;
//	else if(titions_[predeletedLast - 1]->start == change.getRegion().getBottom())
//		--deletedLast;
	if(deletedLast > deletedFirst) {
		if(deletedFirst > 0 && deletedLast < partitions_.size()
				&& partitions_[deletedFirst - 1]->contentType == partitions_[deletedLast]->contentType)
			++deletedLast;	// combine
		for(size_t i = deletedFirst; i < deletedLast; ++i)
			delete partitions_[i];
		partitions_.erase(deletedFirst, deletedLast - deletedFirst);
	}

	// push a default partition if no partition includes the start of the document
	const Document& d = *document();
	if(partitions_.empty() || partitions_[0]->start != d.region().first) {
		if(partitions_.empty() || partitions_[0]->contentType != DEFAULT_CONTENT_TYPE) {
			const Position bob(d.region().first);
			partitions_.insert(0, new Partition(DEFAULT_CONTENT_TYPE, bob, bob, 0));
		} else {
			partitions_[0]->start = partitions_[0]->tokenStart = d.region().first;
			partitions_[0]->tokenLength = 0;
		}
	}

	// delete the partition whose start position is the end of the document
	if(partitions_.size() > 1 && partitions_.back()->start == d.region().second) {
		delete partitions_[partitions_.size() - 1];
		partitions_.erase(partitions_.size() - 1);
	}
}

namespace {
	template<typename Partition>
	struct PartitionPositionCompare {
		bool operator()(const Position& at, const Partition* p) const {
			return at < p->start;
		}
	};
}

// returns the index of the partition encompasses the given position.
inline detail::GapVector<LexicalPartitioner::Partition*>::const_iterator
		LexicalPartitioner::partitionAt(const Position& at) const /*throw()*/ {
	detail::GapVector<Partition*>::const_iterator p(
		detail::searchBound(partitions_.begin(), partitions_.end(), at, PartitionPositionCompare<Partition>()));
	if(p == partitions_.end()) {
		assert(partitions_.front()->start != document()->region().first);	// twilight context
		return partitions_.begin();
	}
	if(at.line < document()->numberOfLines()
			&& (*p)->tokenStart == at && p != partitions_.begin() && at.column == document()->lineLength(at.line))
		--p;
//	if(result > 0 && partitions_[result]->start == partitions_[result - 1]->start)
//		--p;
	while(p + 1 < partitions_.end() && (*(p + 1))->start == (*p)->start)
		++p;
	return p;
}

/**
 * @fn void ascension::rules::LexicalPartitioner::setRules(InputIterator first, InputIterator last)
 * @brief Sets the new transition rules.
 * @tparam InputIterator Input iterator provides transition rules. The deference operator should
 *                       return a value implicitly convertible to a pointer to @c TransitionRule.
 *                       This method calls @c TransitionRule#clone to copy the values
 * @param first, last The transition rules
 * @throw IllegalStateException This partitioner had already been connected to a document
 */

// returns the transition state (corresponding content type) at the given position.
inline ContentType LexicalPartitioner::transitionStateAt(const Position& at) const /*throw()*/ {
	if(at.line == 0 && at.column == 0)
		return DEFAULT_CONTENT_TYPE;
	detail::GapVector<Partition*>::const_iterator i(partitionAt(at));
	if((*i)->start == at)
		--i;
	return (*i)->contentType;
}

/**
 *
 * @param line The scanning line text
 * @param column The column number at which match starts
 * @param contentType The current content type
 * @param[out] destination The type of the transition destination content
 * @return The length of the pattern matched or 0 if the all rules did not matched
 */
inline length_t LexicalPartitioner::tryTransition(
		const String& line, length_t column, ContentType contentType, ContentType& destination) const /*throw()*/ {
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
				throw runtime_error("LexicalPartitioner.verify failed.");
			previousWasEmpty = true;
		} else {
			assert(partitions_[i]->start < partitions_[i + 1]->start);
			previousWasEmpty = false;
		}
	}
//	assert(partitions_.back()->start < getDocument()->getEndPosition(false) || partitions_.getSize() == 1);
#endif // _DEBUG
}


// LexicalPartitionPresentationReconstructor //////////////////////////////////////////////////////

/**
 * Constructor.
 * @param presentation The presentation which gives the document and default text run style
 * @param tokenScanner The token scanner to use for tokenization
 * @param styles Token identifier to its text style map
 * @param defaultStyle The style for the regions out of tokens. can be @c null
 * @throw NullPointerException @a tokenScanner is @c null
 */
LexicalPartitionPresentationReconstructor::LexicalPartitionPresentationReconstructor(
		const Presentation& presentation, auto_ptr<TokenScanner> tokenScanner,
		const map<Token::Identifier, tr1::shared_ptr<const presentation::TextRunStyle> >& styles,
		tr1::shared_ptr<const presentation::TextRunStyle> defaultStyle /* = tr1::shared_ptr<const presentation::TextRunStyle>() */)
		: presentation_(presentation), tokenScanner_(tokenScanner), styles_(styles) {
	if(tokenScanner_.get() == 0)
		throw NullPointerException("tokenScanner");
}
