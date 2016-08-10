/**
 * @file regex.cpp
 * @author exeal
 * @date 2006-2012, 2014
 */

#ifndef ASCENSION_NO_REGEX
#include <ascension/corelib/regex.hpp>
#include <ascension/corelib/text/case-folder.hpp>
#include <ascension/corelib/text/string-character-iterator.hpp>
#ifndef ASCENSION_NO_MIGEMO
#	include <ascension/corelib/encoder.hpp>
#	include <boost/dll/shared_library.hpp>
#	include "../third-party/migemo.h"
#endif // !ASCENSION_NO_MIGEMO


namespace ascension {
	namespace regex {
		// Matcher ////////////////////////////////////////////////////////////////////////////////////////
		
		/**
		 * @class ascension::regex::Matcher regex.hpp
		 *
		 * An engine that performs match operations on a character sequence represented by
		 * @a CodePointIterator by interpreting a @c Pattern.
		 *
		 * <h3>Java/ICU-like replacements</h3>
		 *
		 * This class defines two types of methods for replacing matched subsequences: Java/ICU like and
		 * in-place replacement. Java/ICU style methods are the following:
		 *
		 * - @c #appendReplacement
		 * - @c #appendTail
		 * - @c #replaceAll
		 * - @c #replaceFirst
		 *
		 * The @c #appendReplacement and @c #appendTail methods can be used in tandem in order to collect
		 * the result into an output iterator, or the more convenient @c #replaceAll method can be used to
		 * create a string in which every macthing subsequence in the input sequence is replaced.
		 *
		 * <h3>In-place replacements</h3>
		 *
		 * - @c #replaceInplace
		 * - @c #endInplaceReplacement
		 *
		 * <h3>Region boundaries</h3>
		 *
		 * By default, a matcher uses anchoring and opaque region bounds.
		 */
		
		/**
		 * @fn ascension::regex::Matcher::appendReplacement(OutputIterator out, const String& replacement)
		 * @brief
		 * @tparam OutputIterator
		 * @param out
		 * @param replacement
		 * @return
		 */
		
		/**
		 * @fn String ascension::regex::Matcher::replaceAll(const String& replacement)
		 * @brief
		 * @param replacement
		 * @return
		 */
		
		/**
		 * @fn String ascension::regex::Matcher::replceFirst(const String& replacement)
		 * @brief
		 * @param replacement
		 * @return
		 */
		
		
		// PatternSyntaxException /////////////////////////////////////////////////////////////////////////////////////

		PatternSyntaxException::PatternSyntaxException(
				const boost::regex_error& src, const String& pattern) : invalid_argument(""), impl_(src), pattern_(pattern) {
		}

		PatternSyntaxException::Code PatternSyntaxException::getCode() const {
			// convert a native boost.regex_constants.error_type into the corresponding PatternSyntaxException.Code
			using namespace boost::regex_constants;
			switch(impl_.code()) {
				case error_collate:		return INVALID_COLLATION_CHARACTER;
				case error_ctype:		return INVALID_CHARACTER_CLASS_NAME;
				case error_escape:		return TRAILING_BACKSLASH;
				case error_backref:		return INVALID_BACK_REFERENCE;
				case error_brack:		return UNMATCHED_BARCKET;			
				case error_paren:		return UNMATCHED_PAREN;
				case error_brace:		return UNMATCHED_BRACE;
				case error_badbrace:	return INVALID_CONTENT_OF_BRACES;
				case error_range:		return INVALID_RANGE_END;
				case error_space:		return MEMORY_EXHAUSTED;
				case error_badrepeat:	return INVALID_REPEATITION;
				case error_complexity:	return TOO_COMPLEX_REGULAR_EXPRESSION;
				case error_stack:		return STACK_OVERFLOW;
				case error_bad_pattern:	return UNKNOWN_ERROR;
				// g++ says 'error_ok', 'error_no_match', 'error_end', 'error_size',
				// 'error_right_paren', 'error_empty' and 'error_unknown' are not handled here...
			}
			return NOT_ERROR;
		}


		// Pattern ////////////////////////////////////////////////////////////////////////////////////////
		
		/**
		 * @class ascension::regex::Pattern regex.hpp
		 * A (compiled) regular expression pattern.
		 *
		 * This class is implemented in terms of Boost.Regex, so the most features are same as
		 * @c boost#basic_regex and the algorithm functions for match. The syntax options used by this
		 * class are @c (boost#regex_constants#perl | boost#regex_constants_nocollate). Because of this,
		 * almost all of the ECMAScript regular expression syntax features are supported. For the details,
		 * see the document of Boost.Regex (http://www.boost.org/).
		 *
		 * Standard call sequence is following:
		 *
		 * @code
		 * Pattern p(patternString);
		 * std::unique_ptr<MatchResult<const Char*>> m(p.matches(target, endof(target)));
		 * @endcode
		 *
		 * @note Following sections are draft and subject to change.
		 *
		 * <h3>Unicode support</h3>
		 *
		 * This class partially conformant to <a href="http://www.unicode.org/reports/tr18/">UTS #18:
		 * Unicode Regular Expressions</a> revision 11. The following list refer to this:
		 *
		 * <dl>
		 *   <dt>1.1 Hex Notation</dt>
		 *   <dd>Supports @c \\x{HHHH} or @c \\x{HHHHHH} notations to refer to a character has the
		 *     corresponding code point (the number of 'H' is unlimited). @c \\u is not usable for this
		 *     purpose.</dd>
		 *   <dt>1.2 Properties</dt>
		 *   <dd>Supports the following properties:
		 *     - General_Category
		 *     - Block
		 *     - Script
		 *     - Alphabetic
		 *     - Uppercase
		 *     - Lowercase
		 *     - White_Space
		 *     - Noncharacter_Code_Point
		 *     - Default_Ignorable_Code_Point
		 *     - ANY, ASCII, ASSIGNED
		 *     And if @c Pattern#EXTENDED_PROPERTIES syntax options is set, then the following
		 *     properties are also supported:
		 *     - Hangul_Syllable_Type
		 *     - Grapheme_Cluster_Break
		 *     - Word_Break
		 *     - Sentence_Break
		 *     - any other binary properties
		 *   </dd>
		 *   <dt>1.3 Subtraction and Intersection</dt>
		 *   <dd>Follows to Boost.Regex.</dd>
		 *   <dt>1.4 Simple Word Boundary</dt>
		 *   <dd>Follows to Boost.Regex. However, see the following section "Word character".</dd>
		 *   <dt>1.5 Simple Loose Match</dt>
		 *   <dd>Follows to Boost.Regex.</dd>
		 *   <dt>1.6 Line Boundaries</dt>
		 *   <dd>Follows to Boost.Regex. In addition, supports U+0085, U+2028, and U+2029.</dd>
		 *   <dt>1.7 Code Points</dt>
		 *   <dd>Supported. However, @c #matches and @c #search methods take UTF-32 code unit sequence, not UTF-16.</dd>
		 *   <dt>2.1 Canonical Equivalents</dt>
		 *   <dd>Designed @c Pattern#CANONICAL_EQUIVALENTS but not supported currently.</dd>
		 *   <dt>2.2 Default Grapheme Clusters</dt>
		 *   <dd>Not supported. You can use @c text#GraphemeBreakIterator to implement "whole grapheme cluster match".</dd>
		 *   <dt>2.3 Default Word Boundaries</dt>
		 *   <dd>Not supported You can use @c text#WordBreakIterator to implement "whole word match".</dd>
		 *   <dt>2.4 Default Loose Matches .. 2.6 Wildcard Properties</dt>
		 *   <dd>Follows to Boost.Regex.</dd>
		 *   <dt>3.1 Tailored Punctuation .. 3.11 Submatchers</dt>
		 *   <dd>Follows to Boost.Regex.</dd>
		 * </dl>
		 *
		 * @c Pattern can handle property name and value pair notation like "\p{name=value}" or "\p{name:value}".
		 *
		 * POSIX compatible character classes are also supported (as standard recommendation version).
		 */

		/// @internal Private constructor.
		Pattern::Pattern(const StringPiece& regex, int flags /* = 0 */) : flags_(flags) {
			if((flags & ~(CANON_EQ | CASE_INSENSITIVE | COMMENTS | DOTALL | LITERAL | MULTILINE | UNICODE_CASE | UNIX_LINES)) != 0)
				throw UnknownValueException("flags includes illegal bit values.");
			try {
				impl_.assign(text::utf::CharacterDecodeIterator<const Char*>(regex.cbegin(), regex.cend()),
					text::utf::CharacterDecodeIterator<const Char*>(regex.cbegin(), regex.cend(), regex.cend()),
					boost::regex_constants::perl | boost::regex_constants::collate
					| (((flags & CASE_INSENSITIVE) != 0) ? boost::regex_constants::icase : 0)
					| (((flags & LITERAL) != 0) ? boost::regex_constants::literal : 0));
			} catch(const boost::regex_error& e) {
				throw PatternSyntaxException(e, String(regex.cbegin(), regex.cend()));
			}
		}

		/**
		 * Protected constructor builds regular expression pattern with the Boost.Regex native syntax flags.
		 * @param regex The pattern string
		 * @param options The syntax options
		 * @param nativeSyntax The syntax flags of @c boost#syntax_option_type
		 * @throw NullPointerException @a regex is @c null
		 * @throw PatternSyntaxException The expression's syntax is invalid
		 */
		Pattern::Pattern(const StringPiece& regex, boost::regex_constants::syntax_option_type nativeSyntax) : flags_(0) {
			if(regex.cbegin() == nullptr)
				throw NullPointerException("regex");
			try {
				impl_.assign(text::utf::CharacterDecodeIterator<const Char*>(regex.cbegin(), regex.cend()),
					text::utf::CharacterDecodeIterator<const Char*>(regex.cbegin(), regex.cend(), regex.cend()), nativeSyntax);
			} catch(const boost::regex_error& e) {
				throw PatternSyntaxException(e, String(regex.cbegin(), regex.cend()));
			}
		}
		
		/// Destructor.
		Pattern::~Pattern() BOOST_NOEXCEPT {
		}

		/**
		 * Compiles the given regular expression and attempts to match the given input against it.
		 * An invocation of this convenience method of the form
		 *
		 * @code
		 * Pattern.matches(regex, input);
		 * @endcode
		 * behaves in exactly the same way as the expression
		 * @code
		 * Pattern.compile(regex).matcher(input).matches()
		 * @endcode
		 *
		 * If a pattern is to be used multiple times, compiling it once and reusing it will be more efficient than
		 * invoking this method each time.
		 * @param regex the expression to be compiled
		 * @param input the character sequence to be matched
		 * @throw PatternSyntaxException the expression's syntax is invalid
		 */
		bool Pattern::matches(const StringPiece& regex, const StringPiece& input) {
			return matches(regex, text::StringCharacterIterator(input), text::StringCharacterIterator(input, input.cend()));
		}

#ifndef ASCENSION_NO_MIGEMO
		namespace {
			/// Wrapper for C/Migemo.
			class Migemo {
			public:
				/**
				 * Constructor.
				 * @param runtimeFileName The name of the runtime library
				 * @param dictionaryPathName The location of the dictionaries
				 * @throw std#runtime_error The runtime can't load
				 * @throw std#invalid_argument @a dictionaryPathName is empty
				 */
				Migemo(const boost::filesystem::path& runtimeFileName, const boost::filesystem::path& dictionaryPathName) : library_(runtimeFileName),
						migemoQuery_(library_.get<unsigned char*(migemo*, const unsigned char*)>("migemo_query")),
						migemoRelease_(library_.get<void(migemo*, unsigned char*)>("migemo_release")) {
					if(dictionaryPathName.empty())
						throw std::invalid_argument("Dictionary path name is empty.");
					auto migemoOpen(library_.get<migemo*(const char*)>("migemo_open"));
					auto migemoClose(library_.get<void(migemo*)>("migemo_close"));
					auto migemoLoad(library_.get<int(migemo*, int, const char*)>("migemo_load"));
					auto migemoSetOperator(library_.get<int(migemo*, int, const unsigned char*)>("migemo_set_operator"));

					instance_.reset(migemoOpen(0), migemoClose);
					if(instance_.get() == nullptr)
						throw std::runtime_error("migemo_open returned null.");

					// load dictionaries
					migemoLoad(instance_.get(), MIGEMO_DICTID_MIGEMO, (dictionaryPathName / "migemo-dict").string().c_str());
					migemoLoad(instance_.get(), MIGEMO_DICTID_ROMA2HIRA, (dictionaryPathName / "roma2hira.dat").string().c_str());
					migemoLoad(instance_.get(), MIGEMO_DICTID_HIRA2KATA, (dictionaryPathName / "hira2kata.dat").string().c_str());
					migemoLoad(instance_.get(), MIGEMO_DICTID_HAN2ZEN, (dictionaryPathName / "han2zen.dat").string().c_str());

					// define some operators
					static const unsigned char OPERATORS[] = {'|', '\0', '(', '\0', ')', '\0', '[', '\0', ']', '\0'};
					migemoSetOperator(instance_.get(), MIGEMO_OPINDEX_OR, OPERATORS + 0);
					migemoSetOperator(instance_.get(), MIGEMO_OPINDEX_NEST_IN, OPERATORS + 2);
					migemoSetOperator(instance_.get(), MIGEMO_OPINDEX_NEST_OUT, OPERATORS + 4);
					migemoSetOperator(instance_.get(), MIGEMO_OPINDEX_SELECT_IN, OPERATORS + 6);
					migemoSetOperator(instance_.get(), MIGEMO_OPINDEX_SELECT_OUT, OPERATORS + 8);
				}
				/// @see #query(const StringPiece&, std#size_t&)
				std::shared_ptr<const unsigned char> query(const unsigned char* text) {
					if(text == nullptr)
						throw NullPointerException("text");
					else if(!isEnable())
						throw IllegalStateException("not enable");
					std::shared_ptr<unsigned char> nativePattern(migemoQuery_(instance_.get(), text), std::bind(migemoRelease_, instance_.get(), std::placeholders::_1));
					if(nativePattern.get() == nullptr)
						throw std::runtime_error("migemo_query returned null.");
					return lastNativePattern_ = nativePattern;
				}
				/**
				 * Transforms the specified text into the corresponding regular expression.
				 * @param s The source text (in native Japanese encoding)
				 * @param[out] outputLength The length of the regular expression includes @c null
				 * @return The regular expression
				 * @throw IllegalStateException @c #isEnable returned @c false
				 * @throw NullPointerException @a s is @c null
				 * @throw encoding#UnsupportedEncoding The internal encoding failed
				 * @throw std#runtime_error 
				 */
				const String& query(const StringPiece& s, std::size_t& outputLength) {
					if(!isEnable())
						throw IllegalStateException("not enable");
					else if(s.cbegin() == nullptr)
						throw NullPointerException("s");
		
					// convert the source text from UTF-16 to native Japanese encoding
					std::unique_ptr<encoding::Encoder> encoder(encoding::Encoder::forMIB(encoding::standard::SHIFT_JIS));
					if(encoder.get() == nullptr)
						throw encoding::UnsupportedEncodingException("Shift_JIS is not supported in this platform.");
					else {
						std::size_t bufferLength = encoder->properties().maximumNativeBytes() * s.length();
						std::unique_ptr<Byte[]> buffer(new Byte[bufferLength + 1]);
						Byte* toNext;
						const Char* fromNext;
						if(encoding::Encoder::COMPLETED != encoder->fromUnicode(buffer.get(),
								buffer.get() + bufferLength, toNext,
								s.cbegin(), s.cend(), fromNext), encoding::Encoder::REPLACE_UNMAPPABLE_CHARACTERS)
							throw encoding::UnsupportedEncodingException("internal encoding failed.");
						*toNext = 0;
						query(buffer.get());	// may throw std.runtime_error
					}
		
					// convert the result pattern from native Japanese encoding to UTF-16
					const std::size_t nativePatternLength = strlen(reinterpret_cast<const char*>(lastNativePattern_.get()));
					std::unique_ptr<Char[]> pattern(new Char[outputLength = encoder->properties().maximumUCSLength() * (nativePatternLength + 1)]);
					Char* toNext;
					const Byte* fromNext;
					encoder->setSubstitutionPolicy(encoding::Encoder::REPLACE_UNMAPPABLE_CHARACTERS).toUnicode(
						pattern.get(), pattern.get() + outputLength, toNext, lastNativePattern_.get(), lastNativePattern_.get() + nativePatternLength, fromNext);
					outputLength = toNext - pattern.get();
					lastPattern_ = pattern.get();
					return lastPattern_;
				}
				/// Returns true if the library is enable.
				bool isEnable() const {
					if(instance_ == nullptr)
						return false;
					else if(auto migemoIsEnable = library_.get<int(migemo*)>("migemo_is_enable"))
						return migemoIsEnable(instance_.get()) != 0;
					return false;
				}
			private:
				boost::dll::shared_library library_;
				std::shared_ptr<migemo> instance_;
				std::function<unsigned char*(migemo*, const unsigned char*)> migemoQuery_;
				std::function<void(migemo*, unsigned char*)> migemoRelease_;
				std::shared_ptr<const unsigned char> lastNativePattern_;
				String lastPattern_;
			};
			std::unique_ptr<Migemo> migemoLib;
		} // namespace @0


		// MigemoPattern //////////////////////////////////////////////////////////////////////////////////////////////

		std::string MigemoPattern::runtimePathName_;
		std::string MigemoPattern::dictionaryPathName_;

		/**
		 * Private constructor.
		 * @param pattern The pattern string
		 * @param caseSensitive Set @c true to enable case-sensitive match
		 */
		MigemoPattern::MigemoPattern(const StringPiece& pattern, bool caseSensitive) :
				Pattern(pattern, static_cast<boost::regex_constants::syntax_option_type>(
					!caseSensitive ? boost::regex_constants::icase : 0)
					| boost::regex_constants::no_char_classes | boost::regex_constants::nosubs
					| boost::regex_constants::perl) {
		}

		/**
		 * Constructor creates new regular expression pattern for Migemo match.
		 * @param pattern The pattern string
		 * @param caseSensitive Set @c true to enable case-sensitive match
		 * @return The pattern or @c null if Migemo is not installed
		 */
		std::unique_ptr<const MigemoPattern> MigemoPattern::compile(const StringPiece& pattern, bool caseSensitive) {
			install();
			if(isMigemoInstalled()) {
				std::size_t len;
				const String& p = migemoLib->query(pattern, len);
				using namespace boost::regex_constants;
				return std::unique_ptr<const MigemoPattern>(new MigemoPattern(p, caseSensitive));
			} else
				return std::unique_ptr<const MigemoPattern>();
		}

		/**
		 * Initializes the library with the given path names.
		 * @param runtimePathName The C/Migemo library file name
		 * @param dictionaryPathName The directory name of the dictionaries to load (by @c migemo_load)
		 * @note The given path names are converted into default encoding by using the C++ global locale
		 */
		void MigemoPattern::initialize(const boost::filesystem::path& runtimePathName, const boost::filesystem::path& dictionaryPathName) {
			runtimePathName_ = runtimePathName.string();
			dictionaryPathName_ = dictionaryPathName.string();
		}

		inline void MigemoPattern::install() {
			if(migemoLib.get() == nullptr) {
				try {
					migemoLib.reset(new Migemo(runtimePathName_, dictionaryPathName_));
				} catch(const std::runtime_error&) {
				}
			}
		}

		/// Returns @c true if Migemo is installed.
		bool MigemoPattern::isMigemoInstalled() BOOST_NOEXCEPT {
			return migemoLib.get() != nullptr && migemoLib->isEnable();
		}
#endif // !ASCENSION_NO_MIGEMO

		namespace detail {
			// detail.RegexTraits /////////////////////////////////////////////////////////////////////////////////////////

			bool RegexTraits::unixLineMode = false;
			bool RegexTraits::usesExtendedProperties = false;
			std::map<const char*, int, text::ucd::PropertyNameComparer> RegexTraits::names_;

			void RegexTraits::buildNames() {
				// POSIX
				names_["alpha"] = text::ucd::BinaryProperty::ALPHABETIC;
				names_["lower"] = text::ucd::BinaryProperty::LOWERCASE;
				names_["upper"] = text::ucd::BinaryProperty::UPPERCASE;
				names_["punct"] = text::ucd::GeneralCategory::PUNCTUATION;
				names_["digit"] = names_["d"] = text::ucd::GeneralCategory::DECIMAL_NUMBER;
				names_["xdigit"] = POSIX_XDIGIT;
				names_["alnum"] = POSIX_ALNUM;
				names_["space"] = names_["s"] = text::ucd::BinaryProperty::WHITE_SPACE;
				names_["blank"] = POSIX_BLANK;
				names_["cntrl"] = text::ucd::GeneralCategory::CONTROL;
				names_["graph"] = POSIX_GRAPH;
				names_["print"] = POSIX_PRINT;
				names_["word"] = names_["w"] = POSIX_WORD;
				// special GC
				names_["ANY"] = GC_ANY;
				names_["ASSIGNED"] = GC_ASSIGNED;
				names_["ASCII"] = GC_ASCII;
			}

			bool RegexTraits::isctype(char_type c, const char_class_type& f) const {
				using namespace text::ucd;
				// POSIX
				if((f[POSIX_ALNUM] && legacyctype::isalnum(c))
						|| (f[POSIX_BLANK] && legacyctype::isblank(c))
						|| (f[POSIX_GRAPH] && legacyctype::isgraph(c))
						|| (f[POSIX_PRINT] && legacyctype::isprint(c))
						|| (f[POSIX_PUNCT] && legacyctype::ispunct(c))
						|| (f[POSIX_WORD] && legacyctype::isword(c))
						|| (f[POSIX_XDIGIT] && legacyctype::isxdigit(c)))
					return true;

				// higher general category
				const int gc = GeneralCategory::of(c);
				if((f[GeneralCategory::LETTER] && GeneralCategory::is<GeneralCategory::LETTER>(gc))
						|| (f[GeneralCategory::CASED_LETTER] && GeneralCategory::is<GeneralCategory::CASED_LETTER>(gc))
						|| (f[GeneralCategory::MARK] && GeneralCategory::is<GeneralCategory::MARK>(gc))
						|| (f[GeneralCategory::NUMBER] && GeneralCategory::is<GeneralCategory::NUMBER>(gc))
						|| (f[GeneralCategory::SYMBOL] && GeneralCategory::is<GeneralCategory::SYMBOL>(gc))
						|| (f[GeneralCategory::PUNCTUATION] && GeneralCategory::is<GeneralCategory::PUNCTUATION>(gc))
						|| (f[GeneralCategory::SEPARATOR] && GeneralCategory::is<GeneralCategory::SEPARATOR>(gc))
						|| (f[GeneralCategory::OTHER] && GeneralCategory::is<GeneralCategory::OTHER>(gc))
						|| (f[GC_ANY])
						|| (f[GC_ASSIGNED] && gc != GeneralCategory::UNASSIGNED)
						|| (f[GC_ASCII] && c < 0x0080))
					return true;

				// lower general category, block, and script
				if(f[gc] || f[Block::of(c)])
					return true;
				const int script = Script::of(c);
				if(f[script] || (f[Script::KATAKANA_OR_HIRAGANA] && (script == Script::HIRAGANA || script == Script::KATAKANA)))
					return true;

				if(!usesExtendedProperties) {
					return (f[BinaryProperty::ALPHABETIC] && BinaryProperty::is<BinaryProperty::ALPHABETIC>(c))
							|| (f[BinaryProperty::UPPERCASE] && BinaryProperty::is<BinaryProperty::UPPERCASE>(c))
							|| (f[BinaryProperty::LOWERCASE] && BinaryProperty::is<BinaryProperty::LOWERCASE>(c))
							|| (f[BinaryProperty::WHITE_SPACE] && BinaryProperty::is<BinaryProperty::WHITE_SPACE>(c))
							|| (f[BinaryProperty::NONCHARACTER_CODE_POINT] && BinaryProperty::is<BinaryProperty::NONCHARACTER_CODE_POINT>(c))
							|| (f[BinaryProperty::DEFAULT_IGNORABLE_CODE_POINT] && BinaryProperty::is<BinaryProperty::DEFAULT_IGNORABLE_CODE_POINT>(c));
				} else {
					// binary properties
					for(int i = BinaryProperty::FIRST_VALUE; i < BinaryProperty::LAST_VALUE; ++i) {
						if(f[i] && BinaryProperty::is(c, i))
							return true;
					}
					// others
					if(f[HangulSyllableType::of(c)]
							|| f[GraphemeClusterBreak::of(c)]
							|| f[WordBreak::of(c)]
							|| f[SentenceBreak::of(c)])
						return true;
					return false;
				}
			}

			namespace {
				template<typename InputIterator>
				inline InputIterator findPropertyValue(InputIterator first, InputIterator last) {
					static const std::array<char, 2> EQ_OPS = {'=', ':'};
					InputIterator value(std::find_first_of(first, last, std::begin(EQ_OPS), std::end(EQ_OPS)));
					if(value == last)
						return first;
					else if(value == first)
						return last;
					return (std::find_first_of(++value, last, std::begin(EQ_OPS), std::end(EQ_OPS)) == last) ? value : last;
				}
			}

			RegexTraits::char_class_type RegexTraits::lookup_classname(const char_type* p1, const char_type* p2) const {
				using namespace text::ucd;
				assert(p2 >= p1);
				if(names_.empty())
					buildNames();
				char_class_type klass;
				const char_type* const value = findPropertyValue(p1, p2);
				if(value == p2)
					return klass;

				if(value != p1) {	// "name=value" or "name:value"
					int(*valueNameDetector)(const char_type*) = nullptr;
					const std::basic_string<char_type> name(p1, value - 1);
					if(PropertyNameComparer::compare(name.c_str(), GeneralCategory::LONG_NAME) == 0
							|| PropertyNameComparer::compare(name.c_str(), GeneralCategory::SHORT_NAME) == 0)
						valueNameDetector = &GeneralCategory::forName<char_type>;
					else if(PropertyNameComparer::compare(name.c_str(), Block::LONG_NAME) == 0
							|| PropertyNameComparer::compare(name.c_str(), Block::SHORT_NAME) == 0)
						valueNameDetector = &Block::forName<char_type>;
					else if(PropertyNameComparer::compare(name.c_str(), Script::LONG_NAME) == 0
							|| PropertyNameComparer::compare(name.c_str(), Script::SHORT_NAME) == 0)
						valueNameDetector = &Script::forName<char_type>;
					if(valueNameDetector != nullptr) {
						const int p = valueNameDetector(std::basic_string<char_type>(value, p2).c_str());
						if(p != NOT_PROPERTY)
							klass.set(p);
					}
				} else {	// only "name" or "value"
					std::string expression;
					expression.reserve(p2 - p1);
					for(std::size_t i = 0; i < static_cast<std::size_t>(p2 - p1); ++i) {
						if(p1[i] > 0x007fu)
							return klass;
						expression += static_cast<char>(p1[i] & 0xff);
					}
					const std::map<const char*, int, PropertyNameComparer>::const_iterator i(names_.find(expression.c_str()));
					if(i != std::end(names_))
						klass.set(i->second);
					else {
#define ASCENSION_CHECK_PREFIX(lower, upper)					\
	((expression.length() > 2									\
	&& (expression[0] == lower[0] || expression[0] == upper[0])	\
	&& (expression[1] == lower[1] || expression[1] == upper[1])) ? 2 : 0)

						int p = GeneralCategory::forName(expression.c_str() + ASCENSION_CHECK_PREFIX("is", "IS"));
						if(p == NOT_PROPERTY) {
							p = Block::forName(expression.c_str() + ASCENSION_CHECK_PREFIX("in", "IN"));
							if(p == NOT_PROPERTY) {
								p = Script::forName(expression.c_str() + ASCENSION_CHECK_PREFIX("is", "IS"));
								if(p == NOT_PROPERTY)
									p = BinaryProperty::forName(expression.c_str());
							}
						}
						if(p != NOT_PROPERTY)
							klass.set(p);
#undef ASCENSION_CHECK_PREFIX
					}
				}
				return klass;
			}

			RegexTraits::char_type RegexTraits::translate_nocase(char_type c) const {
				return text::CaseFolder::fold(translate(c));
			}
		}
	}
}
#endif // !ASCENSION_NO_REGEX
