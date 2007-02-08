/**
 * @file regex.cpp
 * @author exeal
 * @date 2006-2007
 */

#ifndef ASCENSION_NO_REGEX
#include "stdafx.h"
#include "regex.hpp"
#include "internal.hpp"	// internal::SharedLibrary
#include "encoder.hpp"
#ifndef ASCENSION_NO_MIGEMO
#endif /* !ASCENSION_NO_MIGEMO */
#ifdef _DEBUG
#	pragma comment(lib, "libboost_regex-vc71-sgd-1_33.lib")
#else
#	pragma comment(lib, "libboost_regex-vc71-s-1_33.lib")
#endif /* _DEBUG */

using namespace ascension;
using namespace ascension::regex;
using namespace ascension::unicode;
using namespace std;
using regex::internal::RegexTraits;


#ifndef ASCENSION_NO_MIGEMO
#include "migemo.h"

ASCENSION_BEGIN_SHARED_LIB_ENTRIES(MigemoEntries, 7)
	ASCENSION_SHARED_LIB_ENTRY(0, "migemo_open", migemo*(*signature)(char*))
	ASCENSION_SHARED_LIB_ENTRY(1, "migemo_close", void(*signature)(migemo*))
	ASCENSION_SHARED_LIB_ENTRY(2, "migemo_query", unsigned char*(*signature)(migemo*, unsigned char*))
	ASCENSION_SHARED_LIB_ENTRY(3, "migemo_release", void(*signature)(migemo*, unsigned char*))
	ASCENSION_SHARED_LIB_ENTRY(4, "migemo_load", int(*signature)(migemo*, int, char*))
	ASCENSION_SHARED_LIB_ENTRY(5, "migemo_is_enable", int(*signature)(migemo*))
	ASCENSION_SHARED_LIB_ENTRY(6, "migemo_set_operator", int(*signature)(migemo*, int, unsigned char*))
ASCENSION_END_SHARED_LIB_ENTRIES()

namespace {
	/// Wrapper for C/Migemo.
	class Migemo : protected ascension::internal::SharedLibrary<MigemoEntries> {
	public:
		/**
		 * Constructor.
		 * @param runtimeFileName the name of the runtime library
		 * @param dictionaryPathName the location of the dictionaries
		 * @throw std#runtime_error the runtime can't load
		 * @throw std#invalid_argument @p dictionaryPathName is empty
		 */
		Migemo(const string& runtimeFileName, const string& dictionaryPathName) :
				ascension::internal::SharedLibrary<MigemoEntries>(runtimeFileName.c_str()),
				instance_(0), lastNativePattern_(0), lastPattern_(0) {
			if(dictionaryPathName.empty())
				throw invalid_argument("Dictionary path name is empty.");
			MigemoEntries::Procedure<0>::signature migemoOpen;
			MigemoEntries::Procedure<4>::signature migemoLoad;
			MigemoEntries::Procedure<6>::signature migemoSetOperator;
			if((migemoOpen = get<0>()) && (migemoQuery_ = get<2>())
					&& (migemoRelease_ = get<3>()) && (migemoLoad = get<4>()) && (migemoSetOperator = get<6>())) {
				if(instance_ = migemoOpen(0)) {
					// load dictionaries
					size_t directoryLength = dictionaryPathName.length();
					if(dictionaryPathName[directoryLength - 1] != '/' && dictionaryPathName[directoryLength - 1] != '\\')
						++directoryLength;
					manah::AutoBuffer<char> pathName(new char[directoryLength + 32]);
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
					migemoSetOperator(instance_, MIGEMO_OPINDEX_OR, reinterpret_cast<uchar*>("|"));
					migemoSetOperator(instance_, MIGEMO_OPINDEX_NEST_IN, reinterpret_cast<uchar*>("("));
					migemoSetOperator(instance_, MIGEMO_OPINDEX_NEST_OUT, reinterpret_cast<uchar*>(")"));
					migemoSetOperator(instance_, MIGEMO_OPINDEX_SELECT_IN, reinterpret_cast<uchar*>("["));
					migemoSetOperator(instance_, MIGEMO_OPINDEX_SELECT_OUT, reinterpret_cast<uchar*>("]"));
				}
			}
		}
		/// Destructor.
		~Migemo() throw() {
			releasePatterns();
			if(instance_ != 0) {
				if(MigemoEntries::Procedure<1>::signature migemoClose = get<1>())
					migemoClose(instance_);
			}
		}
		/// @see #query(const Char*, const Char*, size_t&)
		const uchar* query(const uchar* text) {
			if(text == 0)
				throw invalid_argument("Invalid text.");
			else if(!isEnable())
				return 0;
			if(lastNativePattern_ != 0)
				migemoRelease_(instance_, lastNativePattern_);
			lastNativePattern_ = migemoQuery_(instance_, const_cast<uchar*>(text));
			return reinterpret_cast<uchar*>(lastNativePattern_);
		}
		/**
		 * Transforms the specified text into the corresponding regular expression.
		 * @param first the source text (in native Japanese encoding)
		 * @param last the end of the source text
		 * @param[out] outputLength the length of the regular expression includes @c null
		 * @return the regular expression or @c null if failed
		 * @throw std#invalid_argument the source text is invalid
		 */
		const Char* query(const Char* first, const Char* last, size_t& outputLength) {
			if(!isEnable())
				return 0;
			else if(first == 0 || last == 0 || first > last)
				throw invalid_argument("Invalid text.");

			// convert the source text from UTF-16 to native Japanese encoding
			auto_ptr<encodings::Encoder> encoder = encodings::EncoderFactory::getInstance().createEncoder(932);
			if(encoder.get() == 0)
				return 0;
			size_t bufferLength = encoder->getMaxNativeCharLength() * (last - first);
			manah::AutoBuffer<uchar> buffer(new uchar[bufferLength + 1]);
			if(0 == (bufferLength = encoder->fromUnicode(buffer.get(), bufferLength, first, last - first)))
				return 0;
			buffer.get()[bufferLength] = 0;
			query(buffer.get());
			if(lastNativePattern_ == 0)
				return 0;

			// convert the result pattern from native Japanese encoding to UTF-16
			const size_t nativePatternLength = strlen(reinterpret_cast<char*>(lastNativePattern_));
			outputLength = encoder->getMaxUCSCharLength() * (nativePatternLength + 1);
			delete[] lastPattern_;
			lastPattern_ = new Char[outputLength];
			outputLength = encoder->toUnicode(lastPattern_, outputLength, lastNativePattern_, nativePatternLength);
			return lastPattern_;
		}
		/// @see #query(const Char*, const Char*, size_t&)
		const Char* query(const String& s, size_t& outputLength) {
			return query(s.data(), s.data() + s.length(), outputLength);
		}
		/// Releases the pattern explicitly.
		void releasePatterns() throw() {
			if(lastNativePattern_ != 0) {
				migemoRelease_(instance_, reinterpret_cast<uchar*>(lastNativePattern_));
				lastNativePattern_ = 0;
			}
			delete[] lastPattern_;
			lastPattern_ = 0;
		}
		/// Returns true if the library is enable.
		bool isEnable() const throw() {
			if(instance_ == 0)
				return false;
			else if(MigemoEntries::Procedure<5>::signature migemoIsEnable = get<5>())
				return toBoolean(migemoIsEnable(instance_));
			return false;
		}
	private:
		migemo* instance_;
		MigemoEntries::Procedure<2>::signature migemoQuery_;
		MigemoEntries::Procedure<3>::signature migemoRelease_;
		uchar* lastNativePattern_;
		wchar_t* lastPattern_;
	};
	auto_ptr<Migemo> migemoLib;
} // namespace @0


// Pattern //////////////////////////////////////////////////////////////////

/**
 * @class ascension#regex#Pattern
 * A (compiled) regular expression pattern.
 *
 * This class is implemented in terms of Boost.Regex, so the most features are same as
 * @c boost#basic_regex and the algorithm functions for match. The syntax options used
 * by this class are @c (boost#regex_constants#perl | boost#regex_constants_nocollate).
 * Because of this, almost all of the ECMAScript regular expression syntax features are
 * supported. For the details, see the document of Boost.Regex (http://www.boost.org/).
 *
 * <h3>Unicode support</h3>
 *
 * This class partially conformant to UTS #18: Unicode Regular Expressions revision 11.
 * The following list refer to this:
 *
 * <dl>
 *   <dt>1.1 Hex Notation</dt>
 *   <dd>Supports @c \x{HHHH} or @c \x{HHHHHH} notations to refer to the corresponding
 *     code point (the number of 'H' is unlimited). @c \u is not usable for this purpose.</dd>
 *   <dt>1.2 Properties</dt>
 *   <dd>Supports the following properties when @c Pattern#UNICODE_PROPERTY option is given:
 *     <ul>
 *       <li>General_Category</li>
 *       <li>Block</li>
 *       <li>Script</li>
 *       <li>Alphabetic</li>
 *       <li>Uppercase</li>
 *       <li>Lowercase</li>
 *       <li>White_Space</li>
 *       <li>Noncharacter_Code_Point</li>
 *       <li>Default_Ignorable_Code_Point</li>
 *       <li>ANY, ASCII, ASSIGNED</li>
 *     </ul>
 *     And if @c Pattern#EXTENDED_UNICODE_PROPERTY options is set, then the following
 *     properties are also supported:
 *     <ul>
 *       <li>Hangul_Syllable_Type</li>
 *       <li>Grapheme_Cluster_Break</li>
 *       <li>Word_Break</li>
 *       <li>Sentence_Break</li>
 *       <li>any other binary properties</li>
 *     </ul>
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
 *   <dd>Supported.</dd>
 *   <dt>2.1 Canonical Equivalents</dt>
 *   <dd>Designed @c Pattern#CANONICAL_EQUIVALENT but not supported currently.</dd>
 *   <dt>2.2 Default Grapheme Clusters</dt>
 *   <dd>Not supported, but supports whole grapheme cluster match feature. See "Boundaries".</dd>
 *   <dt>2.3 Default Word Boundaries</dt>
 *   <dd>Not supported, but supports whole word match feature. See "Boundaries".</dd>
 *   <dt>2.4 Default Loose Matches .. 2.6 Wildcard Properties</dt>
 *   <dd>Not supported.</dd>
 *   <dt>3.1 Tailored Punctuation .. 3.3 Tailored Word Boundaries</dt>
 *   <dd>...</dd>
 */

/**
 * Constructor builds regular expression pattern using UTF-16 string.
 * @param first the start of the pattern string
 * @param last the end of the pattern string
 * @param options the syntax options
 * @throw boost#regex_error the specified pattern is invalid
 */
Pattern::Pattern(const Char* first, const Char* last, const manah::Flags<SyntaxOption>& options /* = NORMAL */) : options_(options) {
	RegexTraits::enablesExtendedProperties = options_.has(EXTENDED_PROPERTIES);
	impl_.assign(UTF16To32Iterator<const Char*, utf16boundary::USE_BOUNDARY_ITERATORS>(first, first, last),
		UTF16To32Iterator<const Char*, utf16boundary::USE_BOUNDARY_ITERATORS>(last, first, last),
		boost::regex_constants::perl | (options_.has(CASE_INSENSITIVE) ? boost::regex_constants::icase : 0));
}

/**
 * Protected constructor builds regular expression pattern with the additional syntax flags.
 * @param first the start of the pattern string
 * @param last the end of the pattern string
 * @param options the syntax options
 * @param nativeSyntax the syntax flags of @c boost#syntax_option_type
 * @throw boost#regex_error the specified pattern is invalid
 */
Pattern::Pattern(const Char* first, const Char* last,
		const manah::Flags<SyntaxOption>& options, boost::regex_constants::syntax_option_type nativeSyntax) : options_(options) {
	RegexTraits::enablesExtendedProperties = options_.has(EXTENDED_PROPERTIES);
	impl_.assign(UTF16To32Iterator<const Char*, unicode::utf16boundary::USE_BOUNDARY_ITERATORS>(first, first, last),
			UTF16To32Iterator<const Char*, unicode::utf16boundary::USE_BOUNDARY_ITERATORS>(last, first, last),
		nativeSyntax | (options_.has(CASE_INSENSITIVE) ? boost::regex_constants::icase : 0));
}


// internal::RegexTraits ////////////////////////////////////////////////////

bool RegexTraits::enablesExtendedProperties = false;
map<const Char*, int, PropertyNameComparer<Char> > RegexTraits::names_;

void RegexTraits::buildNames() {
	// POSIX
	names_[L"alpha"] = BinaryProperty::ALPHABETIC;
	names_[L"lower"] = BinaryProperty::LOWERCASE;
	names_[L"upper"] = BinaryProperty::UPPERCASE;
	names_[L"punct"] = GeneralCategory::PUNCTUATION;
	names_[L"digit"] = names_[L"d"] = GeneralCategory::NUMBER_DECIMAL_DIGIT;
	names_[L"xdigit"] = POSIX_XDIGIT;
	names_[L"alnum"] = POSIX_ALNUM;
	names_[L"space"] = names_[L"s"] = BinaryProperty::LOWERCASE;
	names_[L"blank"] = POSIX_BLANK;
	names_[L"cntrl"] = GeneralCategory::OTHER_CONTROL;
	names_[L"graph"] = POSIX_GRAPH;
	names_[L"print"] = POSIX_PRINT;
	names_[L"word"] = names_[L"w"] = POSIX_WORD;
	// special GC
	names_[L"ANY"] = GC_ANY;
	names_[L"ASSIGNED"] = GC_ASSIGNED;
	names_[L"ASCII"] = GC_ASCII;
}

inline size_t RegexTraits::findPropertyValue(const String& expression) {
	static const Char EQ_OPS[] = L"=:";
	const std::size_t value = expression.find_first_of(EQ_OPS);
	if(value == String::npos)
		return 0;
	return (String::npos == expression.find_first_of(EQ_OPS, value + 1)) ? value + 1 : String::npos;
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

	// 一般上位分類
	const int gc = GeneralCategory::of(c);
	if((f[GeneralCategory::LETTER] && GeneralCategory::is<GeneralCategory::LETTER>(gc))
			|| (f[GeneralCategory::LETTER_CASED] && GeneralCategory::is<GeneralCategory::LETTER_CASED>(gc))
			|| (f[GeneralCategory::MARK] && GeneralCategory::is<GeneralCategory::MARK>(gc))
			|| (f[GeneralCategory::NUMBER] && GeneralCategory::is<GeneralCategory::NUMBER>(gc))
			|| (f[GeneralCategory::SYMBOL] && GeneralCategory::is<GeneralCategory::SYMBOL>(gc))
			|| (f[GeneralCategory::PUNCTUATION] && GeneralCategory::is<GeneralCategory::PUNCTUATION>(gc))
			|| (f[GeneralCategory::SEPARATOR] && GeneralCategory::is<GeneralCategory::SEPARATOR>(gc))
			|| (f[GeneralCategory::OTHER] && GeneralCategory::is<GeneralCategory::OTHER>(gc))
			|| (f[GC_ANY])
			|| (f[GC_ASSIGNED] && gc != GeneralCategory::OTHER_UNASSIGNED)
			|| (f[GC_ASCII] && c < 0x0080))
		return true;

	// 一般分類、ブロック、スクリプト
	if(f[gc] || f[CodeBlock::of(c)])
		return true;
	const int script = Script::of(c);
	if(f[script] || (f[Script::KATAKANA_OR_HIRAGANA] && (script == Script::HIRAGANA || script == Script::KATAKANA)))
		return true;

	if(!enablesExtendedProperties) {
		return (f[BinaryProperty::ALPHABETIC] && BinaryProperty::is<BinaryProperty::ALPHABETIC>(c))
				|| (f[BinaryProperty::UPPERCASE] && BinaryProperty::is<BinaryProperty::UPPERCASE>(c))
				|| (f[BinaryProperty::LOWERCASE] && BinaryProperty::is<BinaryProperty::LOWERCASE>(c))
				|| (f[BinaryProperty::WHITE_SPACE] && BinaryProperty::is<BinaryProperty::WHITE_SPACE>(c))
				|| (f[BinaryProperty::NONCHARACTER_CODE_POINT] && BinaryProperty::is<BinaryProperty::NONCHARACTER_CODE_POINT>(c))
				|| (f[BinaryProperty::DEFAULT_IGNORABLE_CODE_POINT] && BinaryProperty::is<BinaryProperty::DEFAULT_IGNORABLE_CODE_POINT>(c));
	} else {
		// 2 値プロパティ
		for(int i = BinaryProperty::ALPHABETIC; i < BinaryProperty::COUNT; ++i) {
			if(f[i] && BinaryProperty::is(c, i))
				return true;
		}
		// その他のプロパティ
		if(f[HangulSyllableType::of(c)]
				|| f[GraphemeClusterBreak::of(c)]
				|| f[WordBreak::of(c)]
				|| f[SentenceBreak::of(c)])
			return true;
		return false;
	}
}

RegexTraits::char_class_type RegexTraits::lookup_classname(const char_type* p1, const char_type* p2) const {
	if(names_.empty())
		buildNames();
	char_class_type klass;
	const String expression = String(UTF32To16Iterator<>(p1), UTF32To16Iterator<>(p2));
	const size_t value = findPropertyValue(expression);
	if(value == String::npos)
		return klass;

	if(value != 0) {	// プロパティ名が与えられた
		int(*valueNameDetector)(const Char*) = 0;
		const String name = expression.substr(0, value - 1);
		if(PropertyNameComparer<Char>::compare(name.c_str(), GeneralCategory::LONG_NAME) == 0
				|| PropertyNameComparer<Char>::compare(name.c_str(), GeneralCategory::SHORT_NAME) == 0)
			valueNameDetector = GeneralCategory::forName;
		else if(PropertyNameComparer<Char>::compare(name.c_str(), CodeBlock::LONG_NAME) == 0
				|| PropertyNameComparer<Char>::compare(name.c_str(), CodeBlock::SHORT_NAME) == 0)
			valueNameDetector = CodeBlock::forName;
		else if(PropertyNameComparer<Char>::compare(name.c_str(), Script::LONG_NAME) == 0
				|| PropertyNameComparer<Char>::compare(name.c_str(), Script::SHORT_NAME) == 0)
			valueNameDetector = Script::forName;
		if(valueNameDetector != 0) {
			const int p = valueNameDetector(expression.substr(value).c_str());
			if(p != NOT_PROPERTY)
				klass.set(p);
		}
	} else {
		const map<const Char*, int, PropertyNameComparer<Char> >::const_iterator i = names_.find(expression.c_str());
		if(i != names_.end())
			klass.set(i->second);
		else {
			int p = GeneralCategory::forName(expression.c_str());
			if(p != NOT_PROPERTY)
				klass.set(p);
			else {
				p = CodeBlock::forName(expression.c_str());
				if(p != NOT_PROPERTY)
					klass.set(p);
				else {
					p = Script::forName(expression.c_str());
					if(p != NOT_PROPERTY)
						klass.set(p);
				}
			}
		}
	}
	return klass;
}


// MigemoPattern ////////////////////////////////////////////////////////////

manah::AutoBuffer<char> MigemoPattern::runtimePathName_;
manah::AutoBuffer<char> MigemoPattern::dictionaryPathName_;

/**
 * Private constructor.
 * @param first the start of the pattern string
 * @param last the end of the pattern string
 * @param ignoreCase set true to enable case-insensitive match
 */
MigemoPattern::MigemoPattern(const Char* first, const Char* last, bool ignoreCase) :
		Pattern(first, last, ignoreCase ? CASEFOLDING_UNICODE_SIMPLE : CASEFOLDING_NONE,
			boost::regex_constants::no_char_classes | boost::regex_constants::nosubs | boost::regex_constants::perl) {
}

/**
 * Constructor creates new regular expression pattern for Migemo match.
 * @param first the start of the pattern string
 * @param last the end of the pattern string
 * @param ignoreCase set true to enable case-insensitive match
 * @return the pattern or @c null if Migemo is not installed
 */
auto_ptr<MigemoPattern> MigemoPattern::create(const Char* first, const Char* last, bool ignoreCase) {
	install();
	if(isMigemoInstalled()) {
		size_t len;
		const Char* const p = migemoLib->query(first, last, len);
		using namespace boost::regex_constants;
		return auto_ptr<MigemoPattern>(new MigemoPattern(p, p + len, ignoreCase));
	} else
		return auto_ptr<MigemoPattern>();
}

/**
 * Initializes the library.
 * @param runtimePathName
 * @param dictionaryPathName
 * @throw std#invalid_argument
 */
void MigemoPattern::initialize(const char* runtimePathName, const char* dictionaryPathName) {
	if(runtimePathName == 0 || dictionaryPathName == 0)
		throw invalid_argument("path is invalid.");
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

/// Returns true if Migemo is installed.
bool MigemoPattern::isMigemoInstalled() throw() {
	return migemoLib.get() != 0 && migemoLib->isEnable();
}

#endif /* !ASCENSION_NO_MIGEMO */

//#endif /* !ASCENSION_NO_REGEX */