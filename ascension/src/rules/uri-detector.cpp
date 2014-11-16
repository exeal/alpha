/**
 * @file uri-detector.cpp
 * @author exeal
 * @date 2004-2006 (was Lexer.cpp)
 * @date 2006-2014 was rules.cpp
 * @date 2014-01-13 separated from rules.cpp
 * @date 2014-11-16 Separated from token-rules.cpp
 */

#include <ascension/corelib/text/case-folder.hpp>
#include <ascension/corelib/ustring.hpp>	// umemchr, umemcmp, ustrchr
#include <ascension/rules/uri-detector.hpp>
#include <boost/foreach.hpp>
#include <boost/numeric/interval.hpp>
#include <boost/range/iterator.hpp>
#include <locale>

namespace ascension {
	namespace rules {
		namespace detail {
			class HashTable : private boost::noncopyable {
			public:

				/**
				 * Constructor.
				 * @tparam StringSequence A type of @a first and @a last
				 * @param first The start of the strings
				 * @param last The end of the strings
				 * @param caseSensitive Set @c true to enable case sensitive match
				 */
				template<typename StringSequence>
				HashTable(StringSequence first, StringSequence last, bool caseSensitive)
						: entries_(std::distance(first, last)), maxLength_(0), caseSensitive_(caseSensitive) {
					while(first != last) {
						const String folded(caseSensitive_ ? *first : text::CaseFolder::fold(*first));
						const std::size_t h = hashCode(folded);
						std::unique_ptr<Entry> newEntry(new Entry(folded));
						if(folded.length() > maxLength_)
							maxLength_ = folded.length();
						if(entries_[h % entries_.size()] != nullptr)
							newEntry->next = std::move(entries_[h % entries_.size()]);
						entries_[h % entries_.size()] = std::move(newEntry);
						++first;
					}
				}

				/**
				 * Returns the hash value of the specified string.
				 * @tparam SinglePassReadableRange UTF-16 character sequence
				 * @param characterSequence The character sequence
				 * @return The hash value
				 */
				template<typename SinglePassReadableRange>
				static std::uint32_t hashCode(const SinglePassReadableRange& characterSequence) {
					static_assert(
						text::CodeUnitSizeOf<boost::range_iterator<const SinglePassReadableRange>::type>::value == 2,
						"characterSequence should be 16-bit character sequence.");
					std::uint32_t h = 0;
					BOOST_FOREACH(auto c, characterSequence) {
						h *= 2;
						h += c;
					}
					return h;
				}

				/**
				 * Searches the specified string.
				 * @param textString The text string
				 * @return @c true if the specified string is found
				 */
				bool matches(const StringPiece& textString) const {
					if(caseSensitive_) {
						if(textString.length() > maxLength_)
							return false;
						const std::size_t h = hashCode(textString);
						for(const std::unique_ptr<Entry>* entry = &entries_[h % entries_.size()]; *entry != nullptr; entry = &(*entry)->next) {
							if((*entry)->data.length() == textString.length() && umemcmp((*entry)->data.data(), textString.cbegin(), (*entry)->data.length()) == 0)
								return true;
						}
					} else {
						const String folded(text::CaseFolder::fold(textString));
						const std::size_t h = hashCode(folded);
						for(const std::unique_ptr<Entry>* entry = &entries_[h % entries_.size()]; *entry != nullptr; entry = &(*entry)->next) {
							if((*entry)->data.length() == folded.length() && umemcmp((*entry)->data.data(), folded.data(), folded.length()) == 0)
								return true;
						}
					}
					return false;
				}

				std::size_t maximumLength() const BOOST_NOEXCEPT {return maxLength_;}
			private:
				struct Entry {
					String data;
					std::unique_ptr<Entry> next;
					explicit Entry(const String& str) BOOST_NOEXCEPT : data(str) {}
				};
				std::vector<std::unique_ptr<Entry>> entries_;
				std::size_t maxLength_;	// the length of the longest keyword
				const bool caseSensitive_;
			};
		}

		// URIDetector ////////////////////////////////////////////////////////////////////////////////////////////////

		namespace {
			// Procedures implement productions of RFC 3986 and RFC 3987.
			// Each procedures take two parameter represent the parsed character sequence. These must be
			// first != null, last != null and first <= last. Return value is the end of parsed sequence.
			// "[nullable]" indicates that the procedure can return empty read sequence.
		
			// (from RFC2234)
			// ALPHA =  %x41-5A / %x61-7A ; A-Z / a-z
			// DIGIT =  %x30-39           ; 0-9
			const std::locale& cl = std::locale::classic();
		   
			// sub-delims = "!" / "$" / "&" / "'" / "(" / ")" / "*" / "+" / "," / ";" / "="
			inline const Char* handleSubDelims(const Char* first, const Char* last) {
				static const Char SUB_DELIMS[] = {'!', '$', '&', '\'', '(', ')', '*', '+', ',', ';', '=', 0};
				return (first < last && ustrchr(SUB_DELIMS, *first) != nullptr) ? ++first : nullptr;
			}
		
			// gen-delims = ":" / "/" / "?" / "#" / "[" / "]" / "@"
			inline const Char* handleGenDelims(const Char* first, const Char* last) {
				static const Char GEN_DELIMS[] = {':', '/', '?', '#', '[', ']', '@', 0};
				return (first < last && ustrchr(GEN_DELIMS, *first) != nullptr) ? ++first : nullptr;
			}
		
			// reserved = gen-delims / sub-delims
			inline const Char* handleReserved(const Char* first, const Char* last) {
				return (handleGenDelims(first, last) != nullptr || handleSubDelims(first, last) != nullptr) ? ++first : nullptr;
			}
		
			// unreserved = ALPHA / DIGIT / "-" / "." / "_" / "~"
			inline const Char* handleUnreserved(const Char* first, const Char* last) {
				static const Char UNRESERVED_LEFTOVERS[] = {'-', '.', '_', '~', 0};
				return (first < last && (std::isalnum(*first, cl) || ustrchr(UNRESERVED_LEFTOVERS, *first) != nullptr)) ? ++first : nullptr;
			}
		
			// pct-encoded = "%" HEXDIG HEXDIG
			inline const Char* handlePctEncoded(const Char* first, const Char* last) {
				return (last - first >= 3 && first[0] == '%' && std::isxdigit(first[1], cl) && std::isxdigit(first[2], cl)) ? first += 3 : nullptr;
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
			const Char* ASCENSION_FASTCALL handleIPv6address(const Char* first, const Char* last) {return nullptr;}
		
			// IPvFuture = "v" 1*HEXDIG "." 1*( unreserved / sub-delims / ":" )
			const Char* ASCENSION_FASTCALL handleIPvFuture(const Char* first, const Char* last) {
				if(last - first >= 4 && *first == 'v' && std::isxdigit(*++first, cl)) {
					while(std::isxdigit(*first, cl)) {
						if(++first == last)
							return nullptr;
					}
					if(*first == '.' && ++first < last) {
						const Char* p = first;
						while(p < last) {
							if(nullptr != (p = handleUnreserved(p, last)) || (nullptr != (p = handleSubDelims(p, last))))
								continue;
							else if(*p == ':')
								++p;
							else
								break;
						}
						return (p > first) ? p : nullptr;
					}
				}
				return nullptr;
			}
		
			// IP-literal = "[" ( IPv6address / IPvFuture  ) "]"
			inline const Char* handleIPLiteral(const Char* first, const Char* last) {
				if(first < last && *first == '[') {
					const Char* p;
					if(nullptr != (p = handleIPv6address(++first, last)) || nullptr != (p = handleIPvFuture(first, last))) {
						if(p < last && *p == ']')
							return ++p;
					}
				}
				return nullptr;
			}
		
			// port = *DIGIT
			inline const Char* handlePort(const Char* first, const Char* last) {	// [nullable]
				while(first < last && std::isdigit(*first, cl))
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
						return (++first < last && std::isdigit(*first, cl) && ++first < last && std::isdigit(*first, cl)) ? ++first : first;
					else if(*first == '2') {
						if(++first < last) {
							if(boost::numeric::in(*first, boost::numeric::interval<Char>('0', '4'))) {
								if(std::isdigit(*++first, cl))
									++first;
							} else if(*first == '5') {
								if(boost::numeric::in(*first, boost::numeric::interval<Char>('0', '5')))
									++first;
							}
						}
						return first;
					} else if(boost::numeric::in(*first, boost::numeric::interval<Char>('3', '9')))
						return (++first < last && std::isdigit(*first, cl)) ? ++first : first;
				}
				return nullptr;
			}
		
			// IPv4address = dec-octet "." dec-octet "." dec-octet "." dec-octet
			inline const Char* handleIPv4address(const Char* first, const Char* last) {
				return (last - first >= 7
					&& nullptr != (first = handleDecOctet(first, last))
					&& first < last && *first == '.'
					&& nullptr != (first = handleDecOctet(++first, last))
					&& first < last && *first == '.'
					&& nullptr != (first = handleDecOctet(++first, last))
					&& first < last && *first == '.'
					&& nullptr != (first = handleDecOctet(++first, last))) ? first : nullptr;
			}
		
			// h16 = 1*4HEXDIG
			const Char* ASCENSION_FASTCALL handleH16(const Char* first, const Char* last) {
				if(first < last && std::isxdigit(*first, cl)) {
					const Char* const e = std::min(++first + 3, last);
					while(first < e && std::isxdigit(*first, cl))
						++first;
					return first;
				}
				return nullptr;
			}
		
			// ls32 = ( h16 ":" h16 ) / IPv4address
			inline const Char* handleLs32(const Char* first, const Char* last) {
				const Char* p;
				return ((nullptr != (p = handleH16(first, last)) && p + 2 < last && *++p == ':' && nullptr != (p = handleH16(p, last)))
					|| (nullptr != (p = handleIPv4address(first, last)))) ? p : nullptr;
			}
		
			// scheme = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
			const Char* ASCENSION_FASTCALL handleScheme(const Char* first, const Char* last) {
				if(std::isalpha(*first, cl)) {
					while(++first != last) {
						if(!std::isalnum(*first, cl) && *first != '+' && *first != '-' && *first != '.')
							return first;
					}
					return last;
				}
				return nullptr;
			}
		
			// iprivate = %xE000-F8FF / %xF0000-FFFFD / %x100000-10FFFD
			inline const Char* handlePrivate(const Char* first, const Char* last) {
				if(first < last) {
					if(boost::numeric::in(*first, boost::numeric::interval<Char>(0xe000u, 0xf8ffu)))
						return ++first;
					const CodePoint c = text::utf::decodeFirst(first, last);
					if(boost::numeric::in(c, boost::numeric::interval<CodePoint>(0xf0000u, 0xffffdu))
							|| boost::numeric::in(c, boost::numeric::interval<CodePoint>(0x100000u, 0x10fffdu)))
						return first += 2;
				}
				return nullptr;
			}
		
			// ucschar = %xA0-D7FF / %xF900-FDCF / %xFDF0-FFEF
			//         / %x10000-1FFFD / %x20000-2FFFD / %x30000-3FFFD
			//         / %x40000-4FFFD / %x50000-5FFFD / %x60000-6FFFD
			//         / %x70000-7FFFD / %x80000-8FFFD / %x90000-9FFFD
			//         / %xA0000-AFFFD / %xB0000-BFFFD / %xC0000-CFFFD
			//         / %xD0000-DFFFD / %xE1000-EFFFD
			inline const Char* handleUcschar(const Char* first, const Char* last) {
				if(first < last) {
					if(boost::numeric::in(*first, boost::numeric::interval<Char>(0x00a0u, 0xd7ffu))
							|| boost::numeric::in(*first, boost::numeric::interval<Char>(0xf900u, 0xfdcfu))
							|| boost::numeric::in(*first, boost::numeric::interval<Char>(0xfdf0u, 0xffefu)))
						return ++first;
					const CodePoint c = text::utf::decodeFirst(first, last);
					if(c >= 0x10000u && c < 0xf0000u && (c & 0xffffu) >= 0x0000u && (c & 0xffffu) <= 0xfffdu) {
						if((c & 0xf0000u) != 0xe || (c & 0xffffu) >= 0x1000u)
							return first += 2;
					}
				}
				return nullptr;
			}
		
			// iunreserved = ALPHA / DIGIT / "-" / "." / "_" / "~" / ucschar
			inline const Char* handleIunreserved(const Char* first, const Char* last) {
				const Char* p;
				return (nullptr != (p = handleUnreserved(first, last)) || nullptr != (p = handleUcschar(first, last))) ? p : nullptr;
			}
		
			// ipchar = iunreserved / pct-encoded / sub-delims / ":" / "@"
			inline const Char* handlePchar(const Char* first, const Char* last) {
				if(first < last) {
					const Char* p;
					if(nullptr != (p = handleIunreserved(first, last))
							|| nullptr != (p = handlePctEncoded(first, last))
							|| nullptr != (p = handleSubDelims(first, last)))
						return p;
					else if(*first == ':' || *first == '@')
						return ++first;
				}
				return nullptr;
			}
		
			// isegment = *ipchar
			inline const Char* handleSegment(const Char* first, const Char* last) {	// [nullable]
				for(const Char* eop ; first < last; first = eop) {
					if(nullptr == (eop = handlePchar(first, last)))
						break;
				}
				return first;
			}
		
			// isegment-nz = 1*ipchar
			inline const Char* handleSegmentNz(const Char* first, const Char* last) {
				const Char* const eos = handleSegment(first, last);
				return (eos > first) ? eos : nullptr;
			}
		
			// isegment-nz-nc = 1*( iunreserved / pct-encoded / sub-delims / "@" ) ; non-zero-length segment without any colon ":"
			inline const Char* handleSegmentNzNc(const Char* first, const Char* last) {
				const Char* const f = first;
				for(const Char* eop; first < last; ) {
					if(nullptr != (eop = handleUnreserved(first, last))
							|| nullptr != (eop = handlePctEncoded(first, last))
							|| nullptr != (eop = handleSubDelims(first, last)))
						first = eop;
					else if(*first == '@')
						++first;
					else
						break;
				}
				return (first > f) ? first : nullptr;
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
				return (eos != nullptr) ? handlePathAbempty(eos, last) : nullptr;
			}
		
			// ipath-noscheme = isegment-nz-nc *( "/" isegment )
			inline const Char* handlePathNoscheme(const Char* first, const Char* last) {
				const Char* const eos = handleSegmentNzNc(first, last);
				return (eos != nullptr) ? handlePathAbempty(eos, last) : nullptr;
			}
		
			// ipath-absolute = "/" [ isegment-nz *( "/" isegment ) ]
			inline const Char* handlePathAbsolute(const Char* first, const Char* last) {
				return (first < last && *first == '/') ? handlePathRootless(++first, last) : nullptr;
			}
		
			// ireg-name = *( iunreserved / pct-encoded / sub-delims )
			inline const Char* handleRegName(const Char* first, const Char* last) {	// [nullable]
				for(const Char* p; first < last; first = p) {
					if(nullptr == (p = handleIunreserved(first, last))
							&& nullptr == (p = handlePctEncoded(first, last))
							&& nullptr == (p = handleSubDelims(first, last)))
						break;
				}
				return first;
			}
		
			// ihost = IP-literal / IPv4address / ireg-name
			inline const Char* handleHost(const Char* first, const Char* last) {	// [nullable]
				const Char* p;
				return (nullptr != (p = handleIPLiteral(first, last)) || nullptr != (p = handleIPv4address(first, last))) ? p : handleRegName(first, last);
			}
		
			// iuserinfo = *( iunreserved / pct-encoded / sub-delims / ":" )
			const Char* ASCENSION_FASTCALL handleUserinfo(const Char* first, const Char* last) {	// [nullable]
				for(const Char* eop; first < last; ) {
					if(nullptr != (eop = handleUnreserved(first, last))
							|| nullptr != (eop = handlePctEncoded(first, last))
							|| nullptr != (eop = handleSubDelims(first, last)))
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
				assert(first != nullptr);
				if(first > beginning) {
					if(first >= last || *++first != '@')
						first = beginning;
				} else if(first < last && *++first != '@')
					--first;
				if(nullptr != (first = handleHost(first, last))) {
					if(first != last) {
						if(*first == ':')
							++first;
						first = handlePort(first, last);
						assert(first != nullptr);
					}
				}
				return first;
			}
		
			// ihier-part = ("//" iauthority ipath-abempty) / ipath-absolute / ipath-rootless / ipath-empty
			const Char* ASCENSION_FASTCALL handleHierPart(const Char* first, const Char* last) {
				static const Char DOUBLE_SLASH[] = {'/', '/', 0};
				const Char* eop;
				(last - first > 2 && umemcmp(first, DOUBLE_SLASH, 2) == 0
					&& nullptr != (eop = handleAuthority(first + 2, last)) && nullptr != (eop = handlePathAbempty(eop, last)))
					|| nullptr != (eop = handlePathAbsolute(first, last))
					|| nullptr != (eop = handlePathRootless(first, last))
					|| nullptr != (eop = handlePathEmpty(first, last));
				return eop;
			}
		
			// iquery = *( ipchar / iprivate / "/" / "?" )
			const Char* ASCENSION_FASTCALL handleQuery(const Char* first, const Char* last) {	// [nullable]
				for(const Char* eop; first < last; ) {
					if(nullptr != (eop = handlePchar(first, last)) || nullptr != (eop = handlePrivate(first, last)))
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
					if(nullptr != (eop = handlePchar(first, last)))
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
				if(nullptr != (first = handleScheme(first, last))) {
					if(*first == ':') {
						if(nullptr != (first = handleHierPart(++first, last))) {
							if(*first == '?') {
								first = handleQuery(++first, last);
								assert(first != nullptr);
							}
							if(*first == '#') {
								first = handleFragment(++first, last);
								assert(first != nullptr);
							}
							return first;
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
			const Char* endOfScheme;
			if(validSchemes_ != nullptr) {
				endOfScheme = umemchr(text.cbegin() + 1, ':', std::min(text.length() - 1, validSchemes_->maximumLength()));
				if(!validSchemes_->matches(makeStringPiece(text.cbegin(), endOfScheme)))
					endOfScheme = nullptr;
			} else {
				endOfScheme = umemchr(text.cbegin() + 1, ':', text.length() - 1);
				if(handleScheme(text.cbegin(), endOfScheme) != endOfScheme)
					endOfScheme = nullptr;
			}
			if(endOfScheme == nullptr)
				return text.cbegin();
			else if(endOfScheme == end(text) - 1)	// terminated with <ipath-empty>
				return end(text);
			if(const Char* const e = handleIRI(text.cbegin(), text.cend()))
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
			const Char* nextColon = umemchr(text.cbegin(), ':', text.length());
			if(nextColon == nullptr)
				return false;
			for(StringPiece::const_iterator p(text.cbegin()); ; ) {
				if(handleScheme(p, nextColon) == nextColon) {
					if(validSchemes_ == nullptr || validSchemes_->matches(makeStringPiece(p, nextColon))) {
						if(const Char* const e = handleIRI(p, end(text)))
							return StringPiece(p, e - p);
					}
					p = nextColon;
				} else
					++p;
				if(p == nextColon) {
					p = nextColon;
					nextColon = umemchr(p, ':', end(text) - p);
					if(nextColon == nullptr)
						break;
				}
			}
			return StringPiece();
		}

		/**
		 * Sets the valid schemes.
		 * @param scheme The set of the schemes to set
		 * @param caseSensitive Set @c true to use case-sensitive comparison for scheme name matching.
		 *                      However, RFC 3986 Section 3.1 says that schemes are case-insensitive
		 * @return The detector
		 * @throw std#invalid_argument Invalid name as a scheme was found
		 */
		URIDetector& URIDetector::setValidSchemes(const std::set<String>& schemes, bool caseSensitive /* = false */) {
			// validation
			BOOST_FOREACH(const String& s, schemes) {
				const Char* const p = s.data();
				if(handleScheme(p, p + s.length()) != p + s.length())
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
		 * @param scheme The string contains the schemes separated by @a separator
		 * @param caseSensitive Set @c true to use case-sensitive comparison for scheme name matching
		 * @param separator The character delimits scheme names in @a schemes. this can be a surrogate
		 * @return The detector
		 * @throw std#invalid_argument Invalid name as a scheme was found
		 */
		URIDetector& URIDetector::setValidSchemes(const String& schemes, Char separator, bool caseSensitive /* = false */) {
			std::set<String> container;
			for(Index previous = 0, next; previous < schemes.length(); previous = next + 1) {
				next = schemes.find(separator, previous);
				if(next == String::npos)
					next = schemes.length();
				if(next > previous)
					container.insert(schemes.substr(previous, next - previous));
			}
			return setValidSchemes(container, caseSensitive);
		}
	}
}
