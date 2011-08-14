/**
 * @file regex.cpp
 * @author exeal
 * @date 2006-2011
 */

#ifndef ASCENSION_NO_REGEX
#include <ascension/corelib/regex.hpp>
#include <ascension/corelib/shared-library.hpp>	// detail.SharedLibrary
#ifndef ASCENSION_NO_MIGEMO
#	include <ascension/corelib/encoder.hpp>
#endif // !ASCENSION_NO_MIGEMO

using namespace ascension;
using namespace ascension::regex;
using namespace ascension::text;
using namespace ascension::text::ucd;
using namespace std;
using detail::RegexTraits;

#ifndef ASCENSION_NO_MIGEMO
#include "../third-party/migemo.h"

ASCENSION_DEFINE_SHARED_LIB_ENTRIES(CMigemo, 7);
ASCENSION_SHARED_LIB_ENTRY(CMigemo, 0, "migemo_open", migemo*(*signature)(char*));
ASCENSION_SHARED_LIB_ENTRY(CMigemo, 1, "migemo_close", void(*signature)(migemo*));
ASCENSION_SHARED_LIB_ENTRY(CMigemo, 2, "migemo_query", unsigned char*(*signature)(migemo*, unsigned char*));
ASCENSION_SHARED_LIB_ENTRY(CMigemo, 3, "migemo_release", void(*signature)(migemo*, unsigned char*));
ASCENSION_SHARED_LIB_ENTRY(CMigemo, 4, "migemo_load", int(*signature)(migemo*, int, char*));
ASCENSION_SHARED_LIB_ENTRY(CMigemo, 5, "migemo_is_enable", int(*signature)(migemo*));
ASCENSION_SHARED_LIB_ENTRY(CMigemo, 6, "migemo_set_operator", int(*signature)(migemo*, int, unsigned char*));

namespace {
	/// Wrapper for C/Migemo.
	class Migemo : protected detail::SharedLibrary<CMigemo> {
	public:
		/**
		 * Constructor.
		 * @param runtimeFileName The name of the runtime library
		 * @param dictionaryPathName The location of the dictionaries
		 * @throw std#runtime_error The runtime can't load
		 * @throw std#invalid_argument @a dictionaryPathName is empty
		 */
		Migemo(const string& runtimeFileName, const string& dictionaryPathName) :
				detail::SharedLibrary<CMigemo>(runtimeFileName.c_str()),
				instance_(0), lastNativePattern_(0), lastPattern_(0) {
			if(dictionaryPathName.empty())
				throw invalid_argument("Dictionary path name is empty.");
			CMigemo::Procedure<0>::signature migemoOpen;
			CMigemo::Procedure<4>::signature migemoLoad;
			CMigemo::Procedure<6>::signature migemoSetOperator;
			if((migemoOpen = get<0>()) && (migemoQuery_ = get<2>())
					&& (migemoRelease_ = get<3>()) && (migemoLoad = get<4>())
					&& (migemoSetOperator = get<6>())) {
				if(0 != (instance_ = migemoOpen(0))) {
					// load dictionaries
					size_t directoryLength = dictionaryPathName.length();
					if(dictionaryPathName[directoryLength - 1] != '/'
							&& dictionaryPathName[directoryLength - 1] != '\\')
						++directoryLength;
					AutoBuffer<char> pathName(new char[directoryLength + 32]);
					strcpy(pathName.get(), dictionaryPathName.c_str());
					if(directoryLength != dictionaryPathName.length())
						strcat(pathName.get(), "/");
					strcpy(pathName.get() + directoryLength, "migemo-dict");
					migemoLoad(instance_, MIGEMO_DICTID_MIGEMO, pathName.get());
					strcpy(pathName.get() + directoryLength, "roma2hira.dat");
					migemoLoad(instance_, MIGEMO_DICTID_ROMA2HIRA, pathName.get());
					strcpy(pathName.get() + directoryLength, "hira2kata.dat");
					migemoLoad(instance_, MIGEMO_DICTID_HIRA2KATA, pathName.get());
					strcpy(pathName.get() + directoryLength, "han2zen.dat");
					migemoLoad(instance_, MIGEMO_DICTID_HAN2ZEN, pathName.get());
					// define some operators
					migemoSetOperator(instance_, MIGEMO_OPINDEX_OR,
						const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>("|")));
					migemoSetOperator(instance_, MIGEMO_OPINDEX_NEST_IN,
						const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>("(")));
					migemoSetOperator(instance_, MIGEMO_OPINDEX_NEST_OUT,
						const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(")")));
					migemoSetOperator(instance_, MIGEMO_OPINDEX_SELECT_IN,
						const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>("[")));
					migemoSetOperator(instance_, MIGEMO_OPINDEX_SELECT_OUT,
						const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>("]")));
				}
			}
		}
		/// Destructor.
		~Migemo() throw() {
			releasePatterns();
			if(instance_ != 0) {
				if(CMigemo::Procedure<1>::signature migemoClose = get<1>())
					migemoClose(instance_);
			}
		}
		/// @see #query(const Char*, const Char*, size_t&)
		const unsigned char* query(const unsigned char* text) {
			if(text == 0)
				throw invalid_argument("Invalid text.");
			else if(!isEnable())
				return 0;
			if(lastNativePattern_ != 0)
				migemoRelease_(instance_, lastNativePattern_);
			lastNativePattern_ = migemoQuery_(instance_, const_cast<unsigned char*>(text));
			return reinterpret_cast<unsigned char*>(lastNativePattern_);
		}
		/**
		 * Transforms the specified text into the corresponding regular expression.
		 * @param first The source text (in native Japanese encoding)
		 * @param last The end of the source text
		 * @param[out] outputLength The length of the regular expression includes @c null
		 * @return The regular expression or @c null if failed
		 * @throw NullPointerException @a s is @c null
		 */
		const Char* query(const StringPiece& s, size_t& outputLength) {
			if(!isEnable())
				return 0;
			else if(s.beginning() == 0)
				throw NullPointerException("s");

			// convert the source text from UTF-16 to native Japanese encoding
			auto_ptr<encoding::Encoder> encoder(encoding::Encoder::forMIB(encoding::standard::SHIFT_JIS));
			if(encoder.get() == 0)
				return 0;
			else {
				size_t bufferLength = encoder->properties().maximumNativeBytes() * length(s);
				AutoBuffer<Byte> buffer(new Byte[bufferLength + 1]);
				Byte* toNext;
				const Char* fromNext;
				if(encoding::Encoder::COMPLETED != encoder->fromUnicode(buffer.get(),
						buffer.get() + bufferLength, toNext,
						s.beginning(), s.end(), fromNext), encoding::Encoder::REPLACE_UNMAPPABLE_CHARACTERS)
					return 0;
				*toNext = 0;
				query(buffer.get());
				if(lastNativePattern_ == 0)
					return 0;
			}

			// convert the result pattern from native Japanese encoding to UTF-16
			const size_t nativePatternLength = strlen(reinterpret_cast<char*>(lastNativePattern_));
			outputLength = encoder->properties().maximumUCSLength() * (nativePatternLength + 1);
			delete[] lastPattern_;
			lastPattern_ = new Char[outputLength];
			Char* toNext;
			const Byte* fromNext;
			encoder->setSubstitutionPolicy(encoding::Encoder::REPLACE_UNMAPPABLE_CHARACTERS).toUnicode(
				lastPattern_, lastPattern_ + outputLength, toNext, lastNativePattern_, lastNativePattern_ + nativePatternLength, fromNext);
			outputLength = toNext - lastPattern_;
			return lastPattern_;
		}
		/// Releases the pattern explicitly.
		void releasePatterns() throw() {
			if(lastNativePattern_ != 0) {
				migemoRelease_(instance_, reinterpret_cast<unsigned char*>(lastNativePattern_));
				lastNativePattern_ = 0;
			}
			delete[] lastPattern_;
			lastPattern_ = 0;
		}
		/// Returns true if the library is enable.
		bool isEnable() const {
			if(instance_ == 0)
				return false;
			else if(CMigemo::Procedure<5>::signature migemoIsEnable = get<5>())
				return migemoIsEnable(instance_) != 0;
			return false;
		}
	private:
		migemo* instance_;
		CMigemo::Procedure<2>::signature migemoQuery_;
		CMigemo::Procedure<3>::signature migemoRelease_;
		unsigned char* lastNativePattern_;
		Char* lastPattern_;
	};
	auto_ptr<Migemo> migemoLib;
} // namespace @0


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


// PatternSyntaxException /////////////////////////////////////////////////////////////////////////

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
 * std::auto_ptr<MatchResult<const Char*> > m(p.matches(target, endof(target)));
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
		impl_.assign(UTF16To32Iterator<const Char*>(regex.beginning(), regex.end()),
			UTF16To32Iterator<const Char*>(regex.beginning(), regex.end(), regex.end()),
			boost::regex_constants::perl | boost::regex_constants::collate
			| (((flags & CASE_INSENSITIVE) != 0) ? boost::regex_constants::icase : 0)
			| (((flags & LITERAL) != 0) ? boost::regex_constants::literal : 0));
	} catch(const boost::regex_error& e) {
		throw PatternSyntaxException(e, String(regex.beginning(), regex.end()));
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
	if(regex.beginning() == 0)
		throw NullPointerException("regex");
	try {
		impl_.assign(UTF16To32Iterator<const Char*>(regex.beginning(), regex.end()),
			UTF16To32Iterator<const Char*>(regex.beginning(), regex.end(), regex.end()), nativeSyntax);
	} catch(const boost::regex_error& e) {
		throw PatternSyntaxException(e, String(regex.beginning(), regex.end()));
	}
}

/// Destructor.
Pattern::~Pattern() /*throw()*/ {
}


// detail.RegexTraits /////////////////////////////////////////////////////////////////////////////

bool RegexTraits::unixLineMode = false;
bool RegexTraits::usesExtendedProperties = false;
map<const char*, int, PropertyNameComparer> RegexTraits::names_;

void RegexTraits::buildNames() {
	// POSIX
	names_["alpha"] = BinaryProperty::ALPHABETIC;
	names_["lower"] = BinaryProperty::LOWERCASE;
	names_["upper"] = BinaryProperty::UPPERCASE;
	names_["punct"] = GeneralCategory::PUNCTUATION;
	names_["digit"] = names_["d"] = GeneralCategory::DECIMAL_NUMBER;
	names_["xdigit"] = POSIX_XDIGIT;
	names_["alnum"] = POSIX_ALNUM;
	names_["space"] = names_["s"] = BinaryProperty::WHITE_SPACE;
	names_["blank"] = POSIX_BLANK;
	names_["cntrl"] = GeneralCategory::CONTROL;
	names_["graph"] = POSIX_GRAPH;
	names_["print"] = POSIX_PRINT;
	names_["word"] = names_["w"] = POSIX_WORD;
	// special GC
	names_["ANY"] = GC_ANY;
	names_["ASSIGNED"] = GC_ASSIGNED;
	names_["ASCII"] = GC_ASCII;
}

bool RegexTraits::isctype(char_type c, const char_class_type& f) const {
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
		static const char EQ_OPS[] = {'=', ':'};
		InputIterator value(find_first_of(first, last, EQ_OPS, ASCENSION_ENDOF(EQ_OPS)));
		if(value == last)
			return last;
		return (find_first_of(++value, last, EQ_OPS, ASCENSION_ENDOF(EQ_OPS)) == last) ? value : last;
	}
}

RegexTraits::char_class_type RegexTraits::lookup_classname(const char_type* p1, const char_type* p2) const {
	assert(p2 >= p1);
	if(names_.empty())
		buildNames();
	char_class_type klass;
	const char_type* const value = findPropertyValue(p1, p2);
	if(value == p2)
		return klass;

	if(value != 0) {	// "name=value" or "name:value"
		int(*valueNameDetector)(const char_type*) = 0;
		const basic_string<char_type> name(p1, value - 1);
		if(PropertyNameComparer::compare(name.c_str(), GeneralCategory::LONG_NAME) == 0
				|| PropertyNameComparer::compare(name.c_str(), GeneralCategory::SHORT_NAME) == 0)
			valueNameDetector = &GeneralCategory::forName<char_type>;
		else if(PropertyNameComparer::compare(name.c_str(), Block::LONG_NAME) == 0
				|| PropertyNameComparer::compare(name.c_str(), Block::SHORT_NAME) == 0)
			valueNameDetector = &Block::forName<char_type>;
		else if(PropertyNameComparer::compare(name.c_str(), Script::LONG_NAME) == 0
				|| PropertyNameComparer::compare(name.c_str(), Script::SHORT_NAME) == 0)
			valueNameDetector = &Script::forName<char_type>;
		if(valueNameDetector != 0) {
			const int p = valueNameDetector(basic_string<char_type>(
				UTF32To16Iterator<>(value), UTF32To16Iterator<>(p2)).c_str());
			if(p != NOT_PROPERTY)
				klass.set(p);
		}
	} else {	// only "name" or "value"
		string expression;
		expression.reserve(p2 - p1);
		for(size_t i = 0; i < static_cast<size_t>(p2 - p1); ++i) {
			if(p1[i] > 0x007fu)
				return klass;
			expression[i] = p1[i];
		}
		const map<const char*, int, PropertyNameComparer>::const_iterator i(names_.find(expression.c_str()));
		if(i != names_.end())
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


// MigemoPattern //////////////////////////////////////////////////////////////////////////////////

AutoBuffer<char> MigemoPattern::runtimePathName_;
AutoBuffer<char> MigemoPattern::dictionaryPathName_;

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
auto_ptr<const MigemoPattern> MigemoPattern::compile(const StringPiece& pattern, bool caseSensitive) {
	install();
	if(isMigemoInstalled()) {
		size_t len;
		const Char* const p = migemoLib->query(pattern, len);
		using namespace boost::regex_constants;
		return auto_ptr<const MigemoPattern>(new MigemoPattern(StringPiece(p, len), caseSensitive));
	} else
		return auto_ptr<const MigemoPattern>();
}

/**
 * Initializes the library.
 * @param runtimePathName
 * @param dictionaryPathName
 * @throw NullPointerException @a runtimePathName and/or @a dictionaryPathName is @c null
 */
void MigemoPattern::initialize(const char* runtimePathName, const char* dictionaryPathName) {
	if(runtimePathName == 0)
		throw invalid_argument("runtimePathName");
	else if(dictionaryPathName == 0)
		throw invalid_argument("dictionaryPathName");
	runtimePathName_.reset(new char[strlen(runtimePathName) + 1]);
	strcpy(runtimePathName_.get(), runtimePathName);
	dictionaryPathName_.reset(new char[strlen(dictionaryPathName) + 1]);
	strcpy(dictionaryPathName_.get(), dictionaryPathName);
}

inline void MigemoPattern::install() {
	if(migemoLib.get() == 0 && runtimePathName_.get() != 0 && dictionaryPathName_.get() != 0) {
		try {
			migemoLib.reset(new Migemo(runtimePathName_.get(), dictionaryPathName_.get()));
		} catch(runtime_error&) {
		}
	}
}

/// Returns @c true if Migemo is installed.
bool MigemoPattern::isMigemoInstalled() /*throw()*/ {
	return migemoLib.get() != 0 && migemoLib->isEnable();
}

#endif // !ASCENSION_NO_MIGEMO
#endif // !ASCENSION_NO_REGEX
