/**
 * @file uri-detector.cpp
 * @author exeal
 * @date 2004-2006 (was Lexer.cpp)
 * @date 2006-2014 was rules.cpp
 * @date 2014-01-13 separated from rules.cpp
 * @date 2014-11-16 Separated from token-rules.cpp
 */

#include <ascension/corelib/text/utf.hpp>
#include <ascension/rules/hash-table.hpp>
#include <ascension/rules/uri-detector.hpp>
#include <boost/foreach.hpp>
#include <boost/numeric/interval.hpp>
#include <boost/range/algorithm/equal.hpp>
#include <boost/range/algorithm/find.hpp>

namespace ascension {
	namespace rules {
		namespace {
			// Procedures implement productions of RFC 3986 and RFC 3987.
			// Each procedures take two parameter represent the parsed character sequence. These must be
			// first != null, last != null and first <= last. Return value is the end of parsed sequence.
			// "[nullable]" indicates that the procedure can return empty read sequence.

			inline bool inRange(CodePoint c, CodePoint lo, CodePoint hi) BOOST_NOEXCEPT {
				return boost::numeric::in(c, boost::numeric::interval<CodePoint>(lo, hi));
			}

			// ALPHA = %x41-5A / %x61-7A ; A-Z / a-z (RFC 2234)
			inline bool isALPHA(CodePoint c) BOOST_NOEXCEPT {
				return inRange(c, 'A', 'Z') || inRange(c, 'a', 'z');
			}

			// DIGIT = %x30-39 ; 0-9 (RFC 2234)
			inline bool isDIGIT(CodePoint c) BOOST_NOEXCEPT {
				return inRange(c, '0', '9');
			}

			// HEXDIG = DIGIT / "A" / "B" / "C" / "D" / "E" / "F" (RFC 2234)
			inline bool isHEXDIG(CodePoint c) BOOST_NOEXCEPT {
				return isDIGIT(c) || inRange(c, 'A', 'F');
			}
		   
			// sub-delims = "!" / "$" / "&" / "'" / "(" / ")" / "*" / "+" / "," / ";" / "="
			inline StringPiece::const_iterator handleSubDelims(const StringPiece& s) {
				static const StringPiece::value_type SUB_DELIMS[] = {'!', '$', '&', '\'', '(', ')', '*', '+', ',', ';', '='};
				return (s.cbegin() < s.cend() && boost::find(SUB_DELIMS, s.front()) != s.cend()) ? s.cbegin() + 1 : nullptr;
			}
		
			// gen-delims = ":" / "/" / "?" / "#" / "[" / "]" / "@"
			inline StringPiece::const_iterator handleGenDelims(const StringPiece& s) {
				static const StringPiece::value_type GEN_DELIMS[] = {':', '/', '?', '#', '[', ']', '@'};
				return (s.cbegin() < s.cend() && boost::find(GEN_DELIMS, s.front()) != s.cend()) ? s.cbegin() + 1: nullptr;
			}
		
			// reserved = gen-delims / sub-delims
			inline StringPiece::const_iterator handleReserved(const StringPiece& s) {
				return (handleGenDelims(s) != nullptr || handleSubDelims(s) != nullptr) ? s.cbegin() + 1 : nullptr;
			}
		
			// unreserved = ALPHA / DIGIT / "-" / "." / "_" / "~"
			inline StringPiece::const_iterator handleUnreserved(const StringPiece& s) {
				static const StringPiece::value_type UNRESERVED_LEFTOVERS[] = {'-', '.', '_', '~'};
				return (s.cbegin() < s.cend() && (isALPHA(s.front()) || isDIGIT(s.front()) || boost::find(UNRESERVED_LEFTOVERS, s.front()) != s.cend())) ? s.cbegin() + 1 : nullptr;
			}
		
			// pct-encoded = "%" HEXDIG HEXDIG
			inline StringPiece::const_iterator handlePctEncoded(const StringPiece& s) {
				return (s.length() >= 3 && s[0] == '%' && isHEXDIG(s[1]) && isHEXDIG(s[2])) ? s.cbegin() + 3 : nullptr;
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
			StringPiece::const_iterator ASCENSION_FASTCALL handleIPv6address(const StringPiece& s) {return nullptr;}
		
			// IPvFuture = "v" 1*HEXDIG "." 1*( unreserved / sub-delims / ":" )
			StringPiece::const_iterator ASCENSION_FASTCALL handleIPvFuture(const StringPiece& s) {
				if(s.length() >= 4 && s.front() == 'v') {
					auto p(std::next(s.cbegin()));
					if(isHEXDIG(*p)) {
						while(isHEXDIG(*p)) {
							if(++p == s.cend())
								return nullptr;
						}
						if(*p == '.' && ++p < s.cend()) {
							auto q(p);
							while(q < s.cend()) {
								if(nullptr != (q = handleUnreserved(s.substr(q - s.cbegin()))) || (nullptr != (q = handleSubDelims(s.substr(q - s.cbegin())))))
									continue;
								else if(*q == ':')
									++q;
								else
									break;
							}
							return (q > p) ? q : nullptr;
						}
					}
				}
				return nullptr;
			}
		
			// IP-literal = "[" ( IPv6address / IPvFuture  ) "]"
			inline StringPiece::const_iterator handleIPLiteral(const StringPiece& s) {
				if(s.cbegin() < s.cend() && s.front() == '[') {
					StringPiece::const_iterator p;
					if(nullptr != (p = handleIPv6address(s.substr(1))) || nullptr != (p = handleIPvFuture(s.substr(1)))) {
						if(p < s.cend() && *p == ']')
							return ++p;
					}
				}
				return nullptr;
			}
		
			// port = *DIGIT
			inline StringPiece::const_iterator handlePort(const StringPiece& s) {	// [nullable]
				auto p(s.cbegin());
				while(p < s.cend() && isDIGIT(*p))
					++p;
				return p;
			}
			
			// dec-octet = DIGIT             ; 0-9
			//           / %x31-39 DIGIT     ; 10-99
			//           / "1" 2DIGIT        ; 100-199
			//           / "2" %x30-34 DIGIT ; 200-249
			//           / "25" %x30-35      ; 250-255
			StringPiece::const_iterator ASCENSION_FASTCALL handleDecOctet(const StringPiece& s) {
				if(s.cbegin() < s.cend()) {
					if(s.front() == '0')
						return std::next(s.cbegin());
					else if(s.front() == '1') {
						auto p(s.cbegin());
						return (++p < s.cend() && isDIGIT(*p) && ++p < s.cend() && isDIGIT(*p)) ? ++p : p;
					} else if(s.front() == '2') {
						auto p(std::next(s.cbegin()));
						if(p < s.cend()) {
							if(inRange(*p, '0', '4')) {
								if(isDIGIT(*++p))
									++p;
							} else if(*p == '5') {
								if(inRange(*p, '0', '5'))
									++p;
							}
						}
						return p;
					} else if(inRange(s.front(), '3', '9')) {
						auto p(s.cbegin());
						return (++p < s.cend() && isDIGIT(*p)) ? ++p : p;
					}
				}
				return nullptr;
			}
		
			// IPv4address = dec-octet "." dec-octet "." dec-octet "." dec-octet
			inline StringPiece::const_iterator handleIPv4address(const StringPiece& s) {
				auto p(s.cbegin());
				return (s.length() >= 7
					&& nullptr != (p = handleDecOctet(s.substr(p - s.cbegin())))
					&& p < s.cend() && *p == '.'
					&& nullptr != (p = handleDecOctet(s.substr(++p - s.cbegin())))
					&& p < s.cend() && *p == '.'
					&& nullptr != (p = handleDecOctet(s.substr(++p - s.cbegin())))
					&& p < s.cend() && *p == '.'
					&& nullptr != (p = handleDecOctet(s.substr(++p - s.cbegin())))) ? p : nullptr;
			}
		
			// h16 = 1*4HEXDIG
			StringPiece::const_iterator ASCENSION_FASTCALL handleH16(const StringPiece& s) {
				if(s.cbegin() < s.cend() && isHEXDIG(s.front())) {
					auto p(std::next(s.cbegin()));
					const auto e(std::min(p + 3, s.cend()));
					while(p < e && isHEXDIG(*p))
						++p;
					return p;
				}
				return nullptr;
			}
		
			// ls32 = ( h16 ":" h16 ) / IPv4address
			inline StringPiece::const_iterator handleLs32(const StringPiece& s) {
				StringPiece::const_iterator p;
				return ((nullptr != (p = handleH16(s)) && p + 2 < s.cend() && *++p == ':' && nullptr != (p = handleH16(s.substr(p - s.cbegin()))))
					|| (nullptr != (p = handleIPv4address(s)))) ? p : nullptr;
			}
		
			// scheme = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
			StringPiece::const_iterator ASCENSION_FASTCALL handleScheme(const StringPiece& s) {
				if(isALPHA(s.front())) {
					for(auto p(std::next(s.cbegin())); p != s.cend(); ++p) {
						if(!isALPHA(*p) && !isDIGIT(*p) && *p != '+' && *p != '-' && *p != '.')
							return p;
					}
					return s.cend();
				}
				return nullptr;
			}
		
			// iprivate = %xE000-F8FF / %xF0000-FFFFD / %x100000-10FFFD
			inline StringPiece::const_iterator handlePrivate(const StringPiece& s) {
				if(s.cbegin() < s.cend()) {
					if(inRange(s.front(), 0xe000u, 0xf8ffu))
						return std::next(s.cbegin());
					const CodePoint c = text::utf::decodeFirst(s);
					if(inRange(c, 0xf0000u, 0xffffdu) || inRange(c, 0x100000u, 0x10fffdu))
						return std::next(s.cbegin(), 2);
				}
				return nullptr;
			}
		
			// ucschar = %xA0-D7FF / %xF900-FDCF / %xFDF0-FFEF
			//         / %x10000-1FFFD / %x20000-2FFFD / %x30000-3FFFD
			//         / %x40000-4FFFD / %x50000-5FFFD / %x60000-6FFFD
			//         / %x70000-7FFFD / %x80000-8FFFD / %x90000-9FFFD
			//         / %xA0000-AFFFD / %xB0000-BFFFD / %xC0000-CFFFD
			//         / %xD0000-DFFFD / %xE1000-EFFFD
			inline StringPiece::const_iterator handleUcschar(const StringPiece& s) {
				if(s.cbegin() < s.cend()) {
					if(inRange(s.front(), 0x00a0u, 0xd7ffu) || inRange(s.front(), 0xf900u, 0xfdcfu) || inRange(s.front(), 0xfdf0u, 0xffefu))
						return std::next(s.cbegin());
					const CodePoint c = text::utf::decodeFirst(s);
					static_assert(std::is_unsigned<CodePoint>::value, "");
					if(c >= 0x10000u && c < 0xf0000u && (c & 0xffffu) <= 0xfffdu) {
						if((c & 0xf0000u) != 0xe || (c & 0xffffu) >= 0x1000u)
							return std::next(s.cbegin(), 2);
					}
				}
				return nullptr;
			}
		
			// iunreserved = ALPHA / DIGIT / "-" / "." / "_" / "~" / ucschar
			inline StringPiece::const_iterator handleIunreserved(const StringPiece& s) {
				StringPiece::const_iterator p;
				return (nullptr != (p = handleUnreserved(s)) || nullptr != (p = handleUcschar(s))) ? p : nullptr;
			}
		
			// ipchar = iunreserved / pct-encoded / sub-delims / ":" / "@"
			inline StringPiece::const_iterator handlePchar(const StringPiece& s) {
				if(s.cbegin() < s.cend()) {
					StringPiece::const_iterator p(s.cbegin()), q;
					if(nullptr != (q = handleIunreserved(s.substr(p - s.cbegin())))
							|| nullptr != (q = handlePctEncoded(s.substr(p - s.cbegin())))
							|| nullptr != (q = handleSubDelims(s.substr(p - s.cbegin()))))
						return q;
					else if(*p == ':' || *p == '@')
						return ++p;
				}
				return nullptr;
			}
		
			// isegment = *ipchar
			inline StringPiece::const_iterator handleSegment(const StringPiece& s) {	// [nullable]
				auto p(s.cbegin());
				for(StringPiece::const_iterator eop ; p < s.cend(); p = eop) {
					if(nullptr == (eop = handlePchar(s.substr(p - s.cbegin()))))
						break;
				}
				return p;
			}
		
			// isegment-nz = 1*ipchar
			inline StringPiece::const_iterator handleSegmentNz(const StringPiece& s) {
				const auto eos(handleSegment(s));
				return (eos > s.cbegin()) ? eos : nullptr;
			}
		
			// isegment-nz-nc = 1*( iunreserved / pct-encoded / sub-delims / "@" ) ; non-zero-length segment without any colon ":"
			inline StringPiece::const_iterator handleSegmentNzNc(const StringPiece& s) {
				auto p(s.cbegin());
				for(StringPiece::const_iterator eop; p < s.cend(); ) {
					if(nullptr != (eop = handleUnreserved(s.substr(p - s.cbegin())))
							|| nullptr != (eop = handlePctEncoded(s.substr(p - s.cbegin())))
							|| nullptr != (eop = handleSubDelims(s.substr(p - s.cbegin()))))
						p = eop;
					else if(*p == '@')
						++p;
					else
						break;
				}
				return (p > s.cbegin()) ? p : nullptr;
			}
		
			// ipath-empty = 0<ipchar>
			StringPiece::const_iterator ASCENSION_FASTCALL handlePathEmpty(const StringPiece& s) {	// [nullable]
				return s.cbegin();
			}
		
			// ipath-abempty = *( "/" isegment )
			StringPiece::const_iterator ASCENSION_FASTCALL handlePathAbempty(const StringPiece& s) {	// [nullable]
				auto p(s.cbegin());
				while(p < s.cend() && *p == '/')
					p = handleSegment(s.substr(p + 1 - s.cbegin()));
				return p;
			}
		   
			// ipath-rootless = isegment-nz *( "/" isegment )
			inline StringPiece::const_iterator handlePathRootless(const StringPiece& s) {
				const auto eos(handleSegmentNz(s));
				return (eos != nullptr) ? handlePathAbempty(s.substr(eos - s.cbegin())) : nullptr;
			}
		
			// ipath-noscheme = isegment-nz-nc *( "/" isegment )
			inline StringPiece::const_iterator handlePathNoscheme(const StringPiece& s) {
				const auto eos(handleSegmentNzNc(s));
				return (eos != nullptr) ? handlePathAbempty(s.substr(eos - s.cbegin())) : nullptr;
			}
		
			// ipath-absolute = "/" [ isegment-nz *( "/" isegment ) ]
			inline StringPiece::const_iterator handlePathAbsolute(const StringPiece& s) {
				return (s.cbegin() < s.cend() && s.front() == '/') ? handlePathRootless(s.substr(1)) : nullptr;
			}
		
			// ireg-name = *( iunreserved / pct-encoded / sub-delims )
			inline StringPiece::const_iterator handleRegName(const StringPiece& s) {	// [nullable]
				auto p(s.cbegin());
				for(StringPiece::const_iterator q; p < s.cend(); p = q) {
					const auto ss(s.substr(p - s.cbegin()));
					if(nullptr == (q = handleIunreserved(ss))
							&& nullptr == (q = handlePctEncoded(ss))
							&& nullptr == (q = handleSubDelims(ss)))
						break;
				}
				return p;
			}
		
			// ihost = IP-literal / IPv4address / ireg-name
			inline StringPiece::const_iterator handleHost(const StringPiece& s) {	// [nullable]
				StringPiece::const_iterator p;
				return (nullptr != (p = handleIPLiteral(s)) || nullptr != (p = handleIPv4address(s))) ? p : handleRegName(s);
			}
		
			// iuserinfo = *( iunreserved / pct-encoded / sub-delims / ":" )
			StringPiece::const_iterator ASCENSION_FASTCALL handleUserinfo(const StringPiece& s) {	// [nullable]
				auto p(s.cbegin());
				for(StringPiece::const_iterator eop; p < s.cend(); ) {
					if(nullptr != (eop = handleUnreserved(s.substr(p - s.cbegin())))
							|| nullptr != (eop = handlePctEncoded(s.substr(p - s.cbegin())))
							|| nullptr != (eop = handleSubDelims(s.substr(p - s.cbegin()))))
						p = eop;
					else if(*p == ':')
						++p;
					else
						break;
				}
				return p;
			}
		
			// iauthority = [ iuserinfo "@" ] ihost [ ":" port ]
			StringPiece::const_iterator ASCENSION_FASTCALL handleAuthority(const StringPiece& s) {	// [nullable]
				auto p(handleUserinfo(s));
				assert(p != nullptr);
				if(p > s.cbegin()) {
					if(p >= s.cend() || *++p != '@')
						p = s.cbegin();
				} else if(p < s.cend() && *++p != '@')
					--p;
				if(nullptr != (p = handleHost(s.substr(p - s.cbegin())))) {
					if(p != s.cend()) {
						if(*p == ':')
							++p;
						p = handlePort(s.substr(p - s.cbegin()));
						assert(p != nullptr);
					}
				}
				return p;
			}
		
			// ihier-part = ("//" iauthority ipath-abempty) / ipath-absolute / ipath-rootless / ipath-empty
			StringPiece::const_iterator ASCENSION_FASTCALL handleHierPart(const StringPiece& s) {
				static const StringPiece::value_type DOUBLE_SLASH[] = {'/', '/'};
				StringPiece::const_iterator eop;
				(s.length() > 2 && boost::equal(s.substr(0, 2), DOUBLE_SLASH)
					&& nullptr != (eop = handleAuthority(s.substr(2))) && nullptr != (eop = handlePathAbempty(s.substr(eop - s.cbegin()))))
					|| nullptr != (eop = handlePathAbsolute(s))
					|| nullptr != (eop = handlePathRootless(s))
					|| nullptr != (eop = handlePathEmpty(s));
				return eop;
			}
		
			// iquery = *( ipchar / iprivate / "/" / "?" )
			StringPiece::const_iterator ASCENSION_FASTCALL handleQuery(const StringPiece& s) {	// [nullable]
				auto p(s.cbegin());
				for(StringPiece::const_iterator eop; p < s.cend(); ) {
					if(nullptr != (eop = handlePchar(s.substr(p - s.cbegin()))) || nullptr != (eop = handlePrivate(s.substr(p - s.cbegin()))))
						p = eop;
					else if(*p == '/' || *p == '?')
						++p;
					else
						break;
				}
				return p;
			}
		
			// ifragment = *( ipchar / "/" / "?" )
			StringPiece::const_iterator ASCENSION_FASTCALL handleFragment(const StringPiece& s) {	// [nullable]
				auto p(s.cbegin());
				for(StringPiece::const_iterator eop; p < s.cend(); ) {
					if(nullptr != (eop = handlePchar(s.substr(p - s.cbegin()))))
						p = eop;
					else if(*p == '/' || *p == '?')
						++p;
					else
						break;
				}
				return p;
			}
		
			// IRI = scheme ":" ihier-part [ "?" iquery ] [ "#" ifragment ]
			StringPiece::const_iterator ASCENSION_FASTCALL handleIRI(const StringPiece& s) {
				if(auto p = handleScheme(s)) {
					if(*p == ':') {
						if(nullptr != (p = handleHierPart(s.substr(++p - s.cbegin())))) {
							if(*p == '?') {
								p = handleQuery(s.substr(++p - s.cbegin()));
								assert(p != nullptr);
							}
							if(*p == '#') {
								p = handleFragment(s.substr(++p - s.cbegin()));
								assert(p != nullptr);
							}
							return p;
						}
					}
				}
				return nullptr;
			}
		} // namespace @0

		/// Constructor. The set of the valid schemes is empty.
		URIDetector::URIDetector() BOOST_NOEXCEPT : validSchemes_(nullptr) {
		}

		/// Destructor.
		URIDetector::~URIDetector() BOOST_NOEXCEPT {
			delete validSchemes_;
		}

		/// Returns the default generic instance.
		const URIDetector& URIDetector::defaultGenericInstance() BOOST_NOEXCEPT {
			static URIDetector singleton;
			return singleton;
		}

		/**
		 * Returns the default instance accepts URI schemes defined by IANA
		 * (http://www.iana.org/assignments/uri-schemes.html).
		 */
		const URIDetector& URIDetector::defaultIANAURIInstance() BOOST_NOEXCEPT {
			static URIDetector singleton;
			if(singleton.validSchemes_ == nullptr) {
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
				singleton.setValidSchemes(String(SCHEMES, SCHEMES + std::extent<decltype(SCHEMES)>::value), '|');
			}
			return singleton;
		}

		/**
		 * Returns the end of a URL begins at the given position.
		 * @param text The character sequence
		 * @return The end of the detected URI, or @a text.begin() (not @c null) if not matched
		 * @throw NullPointerException @a text is @c null
		 */
		StringPiece::const_iterator URIDetector::detect(const StringPiece& text) const {
			if(text.cbegin() == nullptr)
				throw NullPointerException("text");
			else if(text.empty())
				return text.cbegin();
		
			// check scheme
			StringPiece::const_iterator endOfScheme;
			if(validSchemes_ != nullptr) {
				auto s(text.substr(1));
				if(s.length() > validSchemes_->maximumLength())
					s.remove_suffix(s.length() - validSchemes_->maximumLength());
				endOfScheme = boost::find(s, ':');
				if(!validSchemes_->matches(makeStringPiece(text.cbegin(), endOfScheme)))
					endOfScheme = nullptr;
			} else {
				endOfScheme = boost::find(text.substr(1), ':');
				if(handleScheme(makeStringPiece(text.cbegin(), endOfScheme)) != endOfScheme)
					endOfScheme = nullptr;
			}
			if(endOfScheme == text.cend())
				return text.cbegin();
			else if(endOfScheme == text.cend() - 1)	// terminated with <ipath-empty>
				return text.cend();
			if(const auto e = handleIRI(text))
				return e;
			return text.cbegin();
		}

		/**
		 * Searches a URI in the specified character sequence.
		 * @param text The character sequence
		 * @return The found URI in the target character sequence, or @c null if not found
		 * @throw NullPointerException @a text is @c null
		 */
		StringPiece URIDetector::search(const StringPiece& text) const {
			if(text.cbegin() == nullptr)
				throw NullPointerException("text");
			else if(text.empty())
				return StringPiece();
		
			// search scheme
			auto nextColon = boost::find(text, ':');
			if(nextColon == text.cend())
				return StringPiece();
			for(StringPiece::const_iterator p(text.cbegin()); ; ) {
				if(handleScheme(makeStringPiece(p, nextColon)) == nextColon) {
					if(validSchemes_ == nullptr || validSchemes_->matches(makeStringPiece(p, nextColon))) {
						if(const auto e = handleIRI(text.substr(p - text.cbegin())))
							return makeStringPiece(p, e);
					}
					p = nextColon;
				} else
					++p;
				if(p == nextColon) {
					p = nextColon;
					nextColon = boost::find(boost::make_iterator_range(p, text.cend()), ':');
					if(nextColon == nullptr)
						break;
				}
			}
			return StringPiece();
		}

		/**
		 * Sets the valid schemes.
		 * @param schemes The set of the schemes to set
		 * @param caseSensitive Set @c true to use case-sensitive comparison for scheme name matching.
		 *                      However, RFC 3986 Section 3.1 says that schemes are case-insensitive
		 * @return The detector
		 * @throw std#invalid_argument Invalid name as a scheme was found
		 */
		URIDetector& URIDetector::setValidSchemes(const std::set<String>& schemes, bool caseSensitive /* = false */) {
			// validation
			BOOST_FOREACH(const String& s, schemes) {
				const auto p = s.data();
				if(handleScheme(StringPiece(p, s.length())) != p + s.length())
					throw std::invalid_argument("schemes");
			}
		
			// rebuild hash table
			detail::HashTable* newSchemes = new detail::HashTable(std::begin(schemes), std::end(schemes), !caseSensitive);
			delete validSchemes_;
			validSchemes_ = newSchemes;
		
			return *this;
		}

		/**
		 * Sets the valid schemes.
		 * @param schemes The string contains the schemes separated by @a separator
		 * @param caseSensitive Set @c true to use case-sensitive comparison for scheme name matching
		 * @param separator The character delimits scheme names in @a schemes. this can be a surrogate
		 * @return The detector
		 * @throw NullPointerException @a schemes is @c null
		 * @throw std#invalid_argument Invalid name as a scheme was found
		 */
		URIDetector& URIDetector::setValidSchemes(const StringPiece& schemes, Char separator, bool caseSensitive /* = false */) {
			if(schemes.cbegin() == nullptr || schemes.cend() == nullptr)
				throw NullPointerException("schemes");
			std::set<String> container;
			for(Index previous = 0, next; previous < schemes.length(); previous = next + 1) {
				next = schemes.substr(previous).find(separator);
				next = (next != StringPiece::npos) ? next + previous : schemes.length();
				if(next > previous)
					container.insert(schemes.substr(previous, next - previous).to_string());
			}
			return setValidSchemes(container, caseSensitive);
		}
	}
}
