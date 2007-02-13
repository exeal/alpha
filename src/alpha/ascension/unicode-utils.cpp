/**
 * @file unicode-utils.cpp
 * @author exeal
 * @date 2005-2007
 */

#include "stdafx.h"
#include "unicode-utils.hpp"
#include <vector>
using namespace ascension;
using namespace ascension::unicode;
using namespace ascension::unicode::internal;
using namespace std;
using manah::AutoBuffer;


#ifndef ASCENSION_NO_UNICODE_NORMALIZATION

// Normalizer ///////////////////////////////////////////////////////////////

/**
 * @class ascension::unicode::Normalizer unicode-utils.hpp
 * @c Nomalizer supports the standard normalization forms described in
 * <a href="http://www.unicode.org/reports/tr15/">UAX #15: Unicode Normalization Forms</a> revision 27.
 *
 * This class has an interface for C++ standard bidirectional iterator and returns normalized text
 * incrementally. The following illustrates this:
 *
 * @code
 * String text(L"C\x0301\x0327");
 * Normalizer n(text, Normalizer::FORM_NFD);
 * // print all normalized characters of the sequence.
 * while(!n.isLast()) {
 *   std::cout << std::dec << (n.tell() - text.data()) << " : "
 *             << std::hex << *n << std::endl;
 *   ++n;
 * }
 * @endcode
 *
 * @c Normalizer is boundary safe. @c #isFirst and @c #isLast check the iterator is at the boundary.
 * If you increment or decrement the iterator over that boundary, @c std#out_of_range exception
 * will be thrown.
 *
 * The @c operator* returns the code point of normalized character, not the character unit value.
 *
 * Notice that an instance of this class does not duplicate the input string. If you dispose the input
 * after an instance is initialized, the result is undefined.
 *
 * This class also has static functions to provide fundamental processes for normalization.
 *
 * This class is not available if configuration symbol @c ASCENSION_NO_UNICODE_NORMALIZATION is defind.
 * And the features about compatibility mapping are not available if configuration symbol
 * @c ASCENSION_NO_UNICODE_COMPATIBILITY_MAPPING is defind. See the description of config.hpp.
 *
 * @note This class is not derivable.
 */

namespace {
	// Based on 3.12 Combining Jamo Behavior and UAX #15 X16 Hangul of Unicode 5.0.
	const Char
		S_BASE = 0xAC00, L_BASE = 0x1100, V_BASE = 0x1161, T_BASE = 0x11A7,
		L_COUNT = 19, V_COUNT = 21, T_COUNT = 28, N_COUNT = V_COUNT * T_COUNT, S_COUNT = L_COUNT * N_COUNT;
	String decomposeHangul(Char c) {
		// from The Unicode Standard 5.0 pp.1356
		const Char sIndex = c - S_BASE;
		if(c < S_BASE || sIndex >= S_COUNT)
			return String(1, c);
		StringBuffer result(ios_base::out);
		result.sputc(L_BASE + sIndex / N_COUNT);				// L
		result.sputc(V_BASE + (sIndex % N_COUNT) / T_COUNT);	// V
		const Char t = T_BASE + sIndex % T_COUNT;
		if(t != T_BASE)
			result.sputc(t);
		return result.str();
	}
	basic_string<CodePoint> composeHangul(UTF16To32Iterator<const Char*, utf16boundary::USE_BOUNDARY_ITERATORS> i) {
		// from The Unicode Standard 5.0 pp.1356~1357
		if(i.isLast())
			return basic_string<CodePoint>();
		basic_stringbuf<CodePoint> result;
		CodePoint last = *i;

		while(!(++i).isLast()) {
			const CodePoint c = *i;

			// 1. check to see if two current characters are L and V
			if(last >= L_BASE && c >= V_BASE) {
				const CodePoint lIndex = last - L_BASE, vIndex = c - V_BASE;
				if(lIndex < L_COUNT && vIndex < V_COUNT) {
					// make syllable of form LV
					last = S_BASE + (lIndex * V_COUNT + vIndex) * T_COUNT;
					result.pubseekoff(-1, ios_base::cur); result.sputc(last);	// reset last
					continue;	// discard c
				}
			}

			// 2. check to see if two current characters are LV and T
			if(last >= S_BASE && c >= T_BASE) {
				const CodePoint sIndex = last - S_BASE, tIndex = c - T_BASE;
				if(sIndex < S_COUNT && tIndex < T_COUNT && (sIndex % T_COUNT) == 0) {
					// make syllable of form LVT
					last += tIndex;
					result.pubseekoff(-1, ios_base::cur); result.sputc(last);	// reset last
					continue;	// discard c
				}
			}

			// if neither case was true, just add the character
			last = c;
			result.sputc(c);
		}
		return result.str();
	}

	// 
	basic_string<CodePoint> decompose(CodePoint c, Normalizer::Type type) {
		// TODO: not implemented.
		return basic_string<CodePoint>(1, c);
	}
	struct LessCCC {
		bool operator()(CodePoint c1, CodePoint c2) {
			return CanonicalCombiningClass::of(c1) < CanonicalCombiningClass::of(c2);
		}
	};
	inline void reorderCombiningMarks(basic_string<CodePoint>& s) {
		basic_string<CodePoint>::iterator current(s.begin()), next;
		const basic_string<CodePoint>::iterator last(s.end());
		while(current != last) {
			next = find_if(current, last, not1(ptr_fun(CanonicalCombiningClass::of)));
			sort(current, next, LessCCC());
			current = next;
		}
	}
	basic_string<CodePoint> compose(const basic_string<CodePoint>& s) {
		// TODO: not implemented.
		return s;
	}
} // namespace @0

/// Default constructor.
Normalizer::Normalizer() {
}

/**
 * Constructor. The normalization will start at the beginning of the text.
 * @param first the start of the text to be normalized
 * @param last the end of the text to be normalized
 * @param form the normalization form
 */
Normalizer::Normalizer(const Char* first, const Char* last, Form form) : form_(form), current_(first, first, last), normalizedBuffer_(0) {
	normalizeCurrentBlock(FORWARD);
}

/**
 * Constructor. The normalization will start at the beginning of the text.
 * @param text the text to be normalized
 * @param form the normalization form
 */
Normalizer::Normalizer(const String& text, Form form) : form_(form), current_(text.data(), text.data(), text.data() + text.length()) {
	normalizeCurrentBlock(FORWARD);
}

/// Copy-constructor.
Normalizer::Normalizer(const Normalizer& rhs) : form_(rhs.form_),
		current_(rhs.current_), normalizedBuffer_(rhs.normalizedBuffer_), indexInBuffer_(rhs.indexInBuffer_) {
}

/// Destructor.
Normalizer::~Normalizer() throw() {
}

/// Assignment operator.
Normalizer& Normalizer::operator=(const Normalizer& rhs) {
	normalizedBuffer_ = rhs.normalizedBuffer_;
	current_ = rhs.current_;
	form_ = rhs.form_;
	indexInBuffer_ = rhs.indexInBuffer_;
	return *this;
}

std::basic_string<CodePoint> Normalizer::normalize(UTF16To32Iterator<const Char*, utf16boundary::USE_BOUNDARY_ITERATORS> i, Form form) {
	// decompose
	basic_stringbuf<CodePoint> buffer(ios_base::out);
	for(; !i.isLast(); ++i) {
		const basic_string<CodePoint> d(decompose(*i, (form == FORM_D || form == FORM_C) ? CANONICAL : COMPATIBILITY));
		buffer.sputn(d.data(), static_cast<streamsize>(d.length()));
	}
	// reorder combining marks
	basic_string<CodePoint> decomposed(buffer.str());
	reorderCombiningMarks(decomposed);
	// compose if needed
	return (form == FORM_C || form == FORM_KC) ? compose(decomposed) : decomposed;
}

void Normalizer::normalizeCurrentBlock(Direction direction) {
	UTF16To32Iterator<const Char*, utf16boundary::USE_BOUNDARY_ITERATORS> next(current_);
	++next;
	while(!next.isLast()) {
		if(CanonicalCombiningClass::of(*next) == 0)
			break;
	}
	normalizedBuffer_ = normalize(
		UTF16To32Iterator<const Char*, utf16boundary::USE_BOUNDARY_ITERATORS>(current_.tell(), current_.tell(), next.tell()), form_);
	indexInBuffer_ = (direction == FORWARD) ? 0 : normalizedBuffer_.length() - 1;
}


#endif /* !ASCENSION_NO_UNICODE_NORMALIZATION */

// IdentifierSyntax /////////////////////////////////////////////////////////

/**
 * @class ascension::unicode::IdentifierSyntax unicode-utils.hpp
 * @c IdentifierSyntax classifies characters and detect identifiers based on
 * <a href="http://www.unicode.org/reports/tr31/">UAX #31: Identifier and Pattern Syntax</a>.
 *
 * This class conforms to Unicode 5.0 UAX #31 revision 7 Level 1.
 *
 * <dl>
 *   <dt>R1 Default Identifiers</dt>
 *   <dd>Supported if the character classification is @c UNICODE_DEFAULT.</dd>
 *   <dt>R2 Alternative Identifiers</dt>
 *   <dd>Supported if the character classification is @c UNICODE_ALTERNATIVE.</dd>
 *   <dt>R3 Pattern_White_Space and Pattern_Syntax Characters</dt>
 *   <dd>@c #isWhiteSpace and @c eatWhiteSpaces methods interpret white space characters according
 *     to the character classification:
 *     - Only SPACE (U+0020), when @c ASCII is set.
 *     - Characters that legacyctype#isspace returns true, when @c LEGACY_POSIX is set.
 *     - Characters have binary property Pattern_White_Space, when either @c UNICODE_DEFAULT or
 *       @c UNICODE_ALTERNATIVE is set.
 *     Pattern_Syntax property is not recognized by this class.</dd>
 *   <dt>R4 Normalized Identifiers</dt>
 *   <dd>Supported if the normalization type is not @c Normalizer#DONT_NORMALIZE.</dd>
 *   <dt>R5 Case-Insensitive Identifiers</dt>
 *   <dd>Supported if the case folding type is not @c CASEFOLDING_NONE.</dd>
 * </dl>
 *
 * <h3>Definitions relative to the Unicode identifier syntax</h3>
 *
 * This class provides methods to add/remove characters to/from default code point categories for
 * identifier syntax. For example, the following code defines '_' as the identifier non-start character:
 *
 * @code
 * IdentifierSyntax is;
 * is.overrideIDNonStartCharacters(L"_", L"");
 * @endcode
 *
 * @see text#IContentTypeInformationProvider#getIdentifierSyntax
 */

namespace {
	template<typename InputIterator>
	void implementOverrides(InputIterator addingFirst, InputIterator addingLast,
			InputIterator subtractingFirst, InputIterator subtractingLast,
			basic_string<CodePoint>& added, basic_string<CodePoint>& subtracted) {
		basic_stringbuf<CodePoint> adding, subtracting;
		while(addingFirst != addingLast)
			adding.sputc(*addingFirst);
		while(subtractingFirst != subtractingLast)
			subtracting.sputc(*subtractingFirst);
		basic_string<CodePoint> a(adding.str()), s(subtracting.str());
		sort(a.begin(), a.end());
		sort(s.begin(), s.end());
		vector<CodePoint> v;
		set_intersection(a.begin(), a.end(), s.begin(), s.end(), back_inserter(v));
		if(!v.empty())
			throw invalid_argument("same character was found the both sets");
		added = a;
		subtracted = s;
	}
} // namespace @0

/**
 * Default constructor.
 * The character classification type is initialized to @c ASCENSION_DEFAULT_CHARACTER_DETECTION_TYPE.
 */
IdentifierSyntax::IdentifierSyntax() throw() : type_(ASCENSION_DEFAULT_CHARACTER_CLASSIFICATION), caseFolding_(CASEFOLDING_NONE)
#ifndef ASCENSION_NO_UNICODE_NORMALIZATION
		, normalizationType_(Normalizer::DONT_NORMALIZE)
#endif /* !ASCENSION_NO_UNICODE_NORMALIZATION */
{
}

/**
 * Constructor.
 * @param type the classification type
 * @param caseFolding the case folding type for caseless match
 * @param normalizationType the normalization type for canonical/compatibility equivalents
 */
IdentifierSyntax::IdentifierSyntax(CharacterClassification type, const CaseFoldings& caseFolding /* = CASEFOLDING_NONE */
#ifndef ASCENSION_NO_UNICODE_NORMALIZATION
		, Normalizer::Type normalizationType /* = Normalizer::DONT_NORMALIZE */
#endif /* !ASCENSION_NO_UNICODE_NORMALIZATION */
) throw() : type_(type), caseFolding_(caseFolding)
#ifndef ASCENSION_NO_UNICODE_NORMALIZATION
		, normalizationType_(normalizationType)
#endif /* !ASCENSION_NO_UNICODE_NORMALIZATION */
{
}

/**
 * Returns true if the specified character is ID_Continue.
 * @param cp the code point of the character
 * @return true if @a cp is identifier continue character
 */
bool IdentifierSyntax::isIdentifierContinueCharacter(CodePoint cp) const throw() {
	if(binary_search(addedIDNonStartCharacters_.begin(), addedIDNonStartCharacters_.end(), cp)
			|| binary_search(addedIDStartCharacters_.begin(), addedIDStartCharacters_.end(), cp))
		return true;
	else if(binary_search(subtractedIDStartCharacters_.begin(), subtractedIDStartCharacters_.end(), cp)
			|| binary_search(subtractedIDNonStartCharacters_.begin(), subtractedIDNonStartCharacters_.end(), cp))
		return false;
	switch(type_) {
	case ASCII:
		return (cp >= 'A' && cp <= 'Z') || (cp >= 'a' && cp <= 'z') || (cp >= '0' && cp <= '9');
	case LEGACY_POSIX:
		return legacyctype::isword(cp);
	case UNICODE_DEFAULT:
	case UNICODE_ALTERNATIVE:
		return BinaryProperty::is<BinaryProperty::ID_CONTINUE>(cp);
	}
	return false;	// –³ˆÓ–¡
}

/**
 * Returns true if the specified character is an ID_Start.
 * @param cp the code point of the character
 * @return true if @a cp is an identifier start character
 */
bool IdentifierSyntax::isIdentifierStartCharacter(CodePoint cp) const throw() {
	if(binary_search(addedIDStartCharacters_.begin(), addedIDStartCharacters_.end(), cp))
		return true;
	else if(binary_search(subtractedIDStartCharacters_.begin(), subtractedIDStartCharacters_.end(), cp))
		return false;
	switch(type_) {
	case ASCII:
		return (cp >= 'A' && cp <= 'Z') || (cp >= 'a' && cp <= 'z');
	case LEGACY_POSIX:
		return legacyctype::isalpha(cp);
	case UNICODE_DEFAULT:
	case UNICODE_ALTERNATIVE:
		return BinaryProperty::is<BinaryProperty::ID_START>(cp);
	}
	return false;	// –³ˆÓ–¡
}

/**
 * Returns true if the specified character is a white space.
 * @param cp the code point of the character
 * @param includeTab set true to treat a horizontal tab as a white space
 * @return true if @a cp is a white space
 */
bool IdentifierSyntax::isWhiteSpace(CodePoint cp, bool includeTab) const throw() {
	if(includeTab && cp == 0x0009)
		return true;
	switch(type_) {
	case ASCII:
		return cp == 0x0020;
	case LEGACY_POSIX:
		return legacyctype::isspace(cp);
	case UNICODE_DEFAULT:
	case UNICODE_ALTERNATIVE:
		return BinaryProperty::is<BinaryProperty::PATTERN_WHITE_SPACE>(cp);
	}
	return false;	// –³ˆÓ–¡
}

/**
 * Overrides default identifier start character set.
 * @param adding the set of characters to add to the default ID_Start characters
 * @param subtracting the set of characters to subtract from the default ID_Start characters
 * @throw std#invalid_argument an isolated surrogate is found, or same character was found at both @a adding and @a subtracting
 */
void IdentifierSyntax::overrideIdentifierStartCharacters(const String& adding, const String& subtracting) {
	if(adding.end() != surrogates::searchIsolatedSurrogate(adding.begin(), adding.end())
			|| subtracting.end() != surrogates::searchIsolatedSurrogate(subtracting.begin(), subtracting.end()))
		throw invalid_argument("an isolated surrogate found.");
	typedef UTF16To32Iterator<String::const_iterator, utf16boundary::DONT_CHECK> I;
	implementOverrides(I(adding.begin()), I(adding.end()),
		I(subtracting.begin()), I(subtracting.end()), addedIDStartCharacters_, subtractedIDStartCharacters_);
}

/**
 * Overrides default identifier start character set.
 * @param adding the set of characters to add to the default ID_Start characters
 * @param subtracting the set of characters to subtract from the default ID_Start characters
 * @throw std#invalid_argument an isolated surrogate is found, or same character was found at both @a adding and @a subtracting
 */
void IdentifierSyntax::overrideIdentifierStartCharacters(const set<CodePoint>& adding, const set<CodePoint>& subtracting) {
	if(adding.end() != find_if(adding.begin(), adding.end(), ptr_fun(surrogates::isSurrogate))
			|| subtracting.end() != find_if(subtracting.begin(), subtracting.end(), ptr_fun(surrogates::isSurrogate)))
		throw invalid_argument("an isolated surrogate found.");
	implementOverrides(adding.begin(), adding.end(),
		subtracting.begin(), subtracting.end(), addedIDStartCharacters_, subtractedIDStartCharacters_);
}

/**
 * Overrides standard identifier only continue character set.
 * @param adding the set of characters to add to standard ID_Continue characters
 * @param subtracting the set of characters to subtract to standard ID_Continue characters
 * @throw std#invalid_argument an isolated surrogate is found, or same character was found at both @a adding and @a subtracting
 */
void IdentifierSyntax::overrideIdentifierNonStartCharacters(const String& adding, const String& subtracting) {
	if(adding.end() != surrogates::searchIsolatedSurrogate(adding.begin(), adding.end())
			|| subtracting.end() != surrogates::searchIsolatedSurrogate(subtracting.begin(), subtracting.end()))
		throw invalid_argument("an isolated surrogate found.");
	typedef UTF16To32Iterator<String::const_iterator, utf16boundary::DONT_CHECK> I;
	implementOverrides(I(adding.begin()), I(adding.end()),
		I(subtracting.begin()), I(subtracting.end()), addedIDNonStartCharacters_, subtractedIDNonStartCharacters_);
}

/**
 * Overrides standard identifier only continue character set.
 * @param adding the set of characters to add to standard ID_Continue characters
 * @param subtracting the set of characters to subtract to standard ID_Continue characters
 * @throw std#invalid_argument an isolated surrogate is found, or same character was found at both @a adding and @a subtracting
 */
void IdentifierSyntax::overrideIdentifierNonStartCharacters(const set<CodePoint>& adding, const set<CodePoint>& subtracting) {
	if(adding.end() != find_if(adding.begin(), adding.end(), ptr_fun(surrogates::isSurrogate))
			|| subtracting.end() != find_if(subtracting.begin(), subtracting.end(), ptr_fun(surrogates::isSurrogate)))
		throw invalid_argument("an isolated surrogate found.");
	implementOverrides(adding.begin(), adding.end(),
		subtracting.begin(), subtracting.end(), addedIDNonStartCharacters_, subtractedIDNonStartCharacters_);
}

namespace {
	/// Returns true if the specified character is Line_Break=NU.
	bool isNU(CodePoint cp, int gc) throw() {
		return (gc == GeneralCategory::NUMBER_DECIMAL_DIGIT && cp < 0xFF00 || cp > 0xFFEF)
			|| cp == 0x066B		// Arabic Decimal Separator
			|| cp == 0x066C;	// Arabic Thousands Separator
	}
	const CodePoint QU[] = {
		0x0022,	// Quotation Mark
		0x0027,	// Apostrophe
		0x275B,	// Heavy Single Turned Comma Quotation Mark Ornament
		0x275C,	// Heavy Single Comma Quotation Mark Ornament
		0x275D,	// Heavy Double Turned Comma Quotation Mark Ornament
		0x275E	// Heavy Double Comma Quotation Mark Ornament
	};
	/// Returns true if the specified character is Line_Break=QU.
	bool isQU(CodePoint cp, int gc) throw() {
		return gc == GeneralCategory::PUNCTUATION_FINAL_QUOTE
			|| gc == GeneralCategory::PUNCTUATION_INITIAL_QUOTE
			|| binary_search(QU, endof(QU), cp);
	}
} // namespace @0


//
// property names are from UNIDATA/PropertyAliases.txt
// property value names are from UNIDATA/PropertyValueAliases.txt
// via `perl prop-names.pl`
//
#define PROP(shortName, longName, property)	names_[L##shortName] = names_[L##longName] = property
#define PROP1(longName, property)			names_[L##longName] = property


// GeneralCategory //////////////////////////////////////////////////////////

/// The long name of the property.
const Char GeneralCategory::LONG_NAME[] = L"General_Category";
/// The short name of the property.
const Char GeneralCategory::SHORT_NAME[] = L"gc";
map<const Char*, int, PropertyNameComparer<Char> > GeneralCategory::names_;

void GeneralCategory::buildNames() {
	PROP("C&",	"Other",					OTHER);
	PROP("Cc",	"Control",					OTHER_CONTROL);
	PROP("Cf",	"Format",					OTHER_FORMAT);
	PROP("Cn",	"Unassigned",				OTHER_UNASSIGNED);
	PROP("Co",	"Private_Use",				OTHER_PRIVATE_USE);
	PROP("Cs",	"Surrogate",				OTHER_SURROGATE);
	PROP("L&",	"Letter",					LETTER);
	PROP("LC",	"Cased_Letter",				LETTER_CASED);
	PROP("Ll",	"Lowercase_Letter",			LETTER_LOWERCASE);
	PROP("Lm",	"Modifier_Letter",			LETTER_MODIFIER);
	PROP("Lo",	"Other_Letter",				LETTER_OTHER);
	PROP("Lt",	"Titlecase_Letter",			LETTER_TITLECASE);
	PROP("Lu",	"Uppercase_Letter",			LETTER_UPPERCASE);
	PROP("M&",	"Mark",						MARK);
	PROP("Mc",	"Spacing_Mark",				MARK_SPACING_COMBINING);
	PROP("Me",	"Enclosing_Mark",			MARK_ENCLOSING);
	PROP("Mn",	"Nonspacing_Mark",			MARK_NONSPACING);
	PROP("N&",	"Number",					NUMBER);
	PROP("Nd",	"Decimal_Number",			NUMBER_DECIMAL_DIGIT);
	PROP("Nl",	"Letter_Number",			NUMBER_LETTER);
	PROP("No",	"Other_Number",				NUMBER_OTHER);
	PROP("P&",	"Punctuation",				PUNCTUATION);
	PROP("Pc",	"Connector_Punctuation",	PUNCTUATION_CONNECTOR);
	PROP("Pd",	"Dash_Punctuation",			PUNCTUATION_DASH);
	PROP("Pe",	"Close_Punctuation",		PUNCTUATION_CLOSE);
	PROP("Pf",	"Final_Punctuation",		PUNCTUATION_FINAL_QUOTE);
	PROP("Pi",	"Initial_Punctuation",		PUNCTUATION_INITIAL_QUOTE);
	PROP("Po",	"Other_Punctuation",		PUNCTUATION_OTHER);
	PROP("Ps",	"Open_Punctuation",			PUNCTUATION_OPEN);
	PROP("S&",	"Symbol",					SYMBOL);
	PROP("Sc",	"Currency_Symbol",			SYMBOL_CURRENCY);
	PROP("Sk",	"Modifier_Symbol",			SYMBOL_MODIFIER);
	PROP("Sm",	"Math_Symbol",				SYMBOL_MATH);
	PROP("So",	"Other_Symbol",				SYMBOL_OTHER);
	PROP("Z&",	"Separator",				SEPARATOR);
	PROP("Zl",	"Line_Separator",			SEPARATOR_LINE);
	PROP("Zp",	"Paragraph_Separator",		SEPARATOR_PARAGRAPH);
	PROP("Zs",	"Space_Separator",			SEPARATOR_SPACE);
}


// CodeBlock ////////////////////////////////////////////////////////////////

/// The long name of the property.
const Char CodeBlock::LONG_NAME[] = L"Block";
/// The short name of the property.
const Char CodeBlock::SHORT_NAME[] = L"blk";
map<const Char*, int, PropertyNameComparer<Char> > CodeBlock::names_;

void CodeBlock::buildNames() {
	names_[L"Aegean_Numbers"] = AEGEAN_NUMBERS;
	names_[L"Alphabetic_Presentation_Forms"] = ALPHABETIC_PRESENTATION_FORMS;
	names_[L"Ancient_Greek_Musical_Notation"] = ANCIENT_GREEK_MUSICAL_NOTATION;
	names_[L"Ancient_Greek_Numbers"] = ANCIENT_GREEK_NUMBERS;
	names_[L"Arabic"] = ARABIC;
	names_[L"Arabic_Presentation_Forms-A"] = ARABIC_PRESENTATION_FORMS_A;
	names_[L"Arabic_Presentation_Forms-B"] = ARABIC_PRESENTATION_FORMS_B;
	names_[L"Arabic_Supplement"] = ARABIC_SUPPLEMENT;
	names_[L"Armenian"] = ARMENIAN;
	names_[L"Arrows"] = ARROWS;
	names_[L"Balinese"] = BALINESE;
	names_[L"Basic_Latin"] = BASIC_LATIN;
	names_[L"Bengali"] = BENGALI;
	names_[L"Block_Elements"] = BLOCK_ELEMENTS;
	names_[L"Bopomofo"] = BOPOMOFO;
	names_[L"Bopomofo_Extended"] = BOPOMOFO_EXTENDED;
	names_[L"Box_Drawing"] = BOX_DRAWING;
	names_[L"Braille_Patterns"] = BRAILLE_PATTERNS;
	names_[L"Buginese"] = BUGINESE;
	names_[L"Buhid"] = BUHID;
	names_[L"Byzantine_Musical_Symbols"] = BYZANTINE_MUSICAL_SYMBOLS;
	names_[L"Cherokee"] = CHEROKEE;
	names_[L"CJK_Compatibility"] = CJK_COMPATIBILITY;
	names_[L"CJK_Compatibility_Forms"] = CJK_COMPATIBILITY_FORMS;
	names_[L"CJK_Compatibility_Ideographs"] = CJK_COMPATIBILITY_IDEOGRAPHS;
	names_[L"CJK_Compatibility_Ideographs_Supplement"] = CJK_COMPATIBILITY_IDEOGRAPHS_SUPPLEMENT;
	names_[L"CJK_Radicals_Supplement"] = CJK_RADICALS_SUPPLEMENT;
	names_[L"CJK_Strokes"] = CJK_STROKES;
	names_[L"CJK_Symbols_and_Punctuation"] = CJK_SYMBOLS_AND_PUNCTUATION;
	names_[L"CJK_Unified_Ideographs"] = CJK_UNIFIED_IDEOGRAPHS;
	names_[L"CJK_Unified_Ideographs_Extension_A"] = CJK_UNIFIED_IDEOGRAPHS_EXTENSION_A;
	names_[L"CJK_Unified_Ideographs_Extension_B"] = CJK_UNIFIED_IDEOGRAPHS_EXTENSION_B;
	names_[L"Combining_Diacritical_Marks"] = COMBINING_DIACRITICAL_MARKS;
	names_[L"Combining_Diacritical_Marks_for_Symbols"] = COMBINING_DIACRITICAL_MARKS_FOR_SYMBOLS;
	names_[L"Combining_Diacritical_Marks_Supplement"] = COMBINING_DIACRITICAL_MARKS_SUPPLEMENT;
	names_[L"Combining_Half_Marks"] = COMBINING_HALF_MARKS;
	names_[L"Control_Pictures"] = CONTROL_PICTURES;
	names_[L"Coptic"] = COPTIC;
	names_[L"Counting_Rod_Numerals"] = COUNTING_ROD_NUMERALS;
	names_[L"Cuneiform"] = CUNEIFORM;
	names_[L"Cuneiform_Numbers_and_Punctuation"] = CUNEIFORM_NUMBERS_AND_PUNCTUATION;
	names_[L"Currency_Symbols"] = CURRENCY_SYMBOLS;
	names_[L"Cypriot_Syllabary"] = CYPRIOT_SYLLABARY;
	names_[L"Cyrillic"] = CYRILLIC;
	names_[L"Cyrillic_Supplement"] = names_[L"Cyrillic_Supplementary"] = CYRILLIC_SUPPLEMENT;
	names_[L"Deseret"] = DESERET;
	names_[L"Devanagari"] = DEVANAGARI;
	names_[L"Dingbats"] = DINGBATS;
	names_[L"Enclosed_Alphanumerics"] = ENCLOSED_ALPHANUMERICS;
	names_[L"Enclosed_CJK_Letters_and_Months"] = ENCLOSED_CJK_LETTERS_AND_MONTHS;
	names_[L"Ethiopic"] = ETHIOPIC;
	names_[L"Ethiopic_Extended"] = ETHIOPIC_EXTENDED;
	names_[L"Ethiopic_Supplement"] = ETHIOPIC_SUPPLEMENT;
	names_[L"General_Punctuation"] = GENERAL_PUNCTUATION;
	names_[L"Geometric_Shapes"] = GEOMETRIC_SHAPES;
	names_[L"Georgian"] = GEORGIAN;
	names_[L"Georgian_Supplement"] = GEORGIAN_SUPPLEMENT;
	names_[L"Glagolitic"] = GLAGOLITIC;
	names_[L"Gothic"] = GOTHIC;
	names_[L"Greek_and_Coptic"] = GREEK_AND_COPTIC;
	names_[L"Greek_Extended"] = GREEK_EXTENDED;
	names_[L"Gujarati"] = GUJARATI;
	names_[L"Gurmukhi"] = GURMUKHI;
	names_[L"Halfwidth_and_Fullwidth_Forms"] = HALFWIDTH_AND_FULLWIDTH_FORMS;
	names_[L"Hangul_Compatibility_Jamo"] = HANGUL_COMPATIBILITY_JAMO;
	names_[L"Hangul_Jamo"] = HANGUL_JAMO;
	names_[L"Hangul_Syllables"] = HANGUL_SYLLABLES;
	names_[L"Hanunoo"] = HANUNOO;
	names_[L"Hebrew"] = HEBREW;
	names_[L"High_Private_Use_Surrogates"] = HIGH_PRIVATE_USE_SURROGATES;
	names_[L"High_Surrogates"] = HIGH_SURROGATES;
	names_[L"Hiragana"] = HIRAGANA;
	names_[L"Ideographic_Description_Characters"] = IDEOGRAPHIC_DESCRIPTION_CHARACTERS;
	names_[L"IPA_Extensions"] = IPA_EXTENSIONS;
	names_[L"Kanbun"] = KANBUN;
	names_[L"Kangxi_Radicals"] = KANGXI_RADICALS;
	names_[L"Kannada"] = KANNADA;
	names_[L"Katakana"] = KATAKANA;
	names_[L"Katakana_Phonetic_Extensions"] = KATAKANA_PHONETIC_EXTENSIONS;
	names_[L"Kharoshthi"] = KHAROSHTHI;
	names_[L"Khmer"] = KHMER;
	names_[L"Khmer_Symbols"] = KHMER_SYMBOLS;
	names_[L"Lao"] = LAO;
	names_[L"Latin-1_Supplement"] = LATIN_1_SUPPLEMENT;
	names_[L"Latin_Extended-A"] = LATIN_EXTENDED_A;
	names_[L"Latin_Extended-B"] = LATIN_EXTENDED_B;
	names_[L"Latin_Extended-C"] = LATIN_EXTENDED_C;
	names_[L"Latin_Extended-D"] = LATIN_EXTENDED_D;
	names_[L"Latin_Extended_Additional"] = LATIN_EXTENDED_ADDITIONAL;
	names_[L"Letterlike_Symbols"] = LETTERLIKE_SYMBOLS;
	names_[L"Limbu"] = LIMBU;
	names_[L"Linear_B_Ideograms"] = LINEAR_B_IDEOGRAMS;
	names_[L"Linear_B_Syllabary"] = LINEAR_B_SYLLABARY;
	names_[L"Low_Surrogates"] = LOW_SURROGATES;
	names_[L"Malayalam"] = MALAYALAM;
	names_[L"Mathematical_Alphanumeric_Symbols"] = MATHEMATICAL_ALPHANUMERIC_SYMBOLS;
	names_[L"Mathematical_Operators"] = MATHEMATICAL_OPERATORS;
	names_[L"Miscellaneous_Mathematical_Symbols-A"] = MISCELLANEOUS_MATHEMATICAL_SYMBOLS_A;
	names_[L"Miscellaneous_Mathematical_Symbols-B"] = MISCELLANEOUS_MATHEMATICAL_SYMBOLS_B;
	names_[L"Miscellaneous_Symbols"] = MISCELLANEOUS_SYMBOLS;
	names_[L"Miscellaneous_Symbols_and_Arrows"] = MISCELLANEOUS_SYMBOLS_AND_ARROWS;
	names_[L"Miscellaneous_Technical"] = MISCELLANEOUS_TECHNICAL;
	names_[L"Modifier_Tone_Letters"] = MODIFIER_TONE_LETTERS;
	names_[L"Mongolian"] = MONGOLIAN;
	names_[L"Musical_Symbols"] = MUSICAL_SYMBOLS;
	names_[L"Myanmar"] = MYANMAR;
	names_[L"New_Tai_Lue"] = NEW_TAI_LUE;
	names_[L"NKo"] = NKO;
	names_[L"No_Block"] = NO_BLOCK;
	names_[L"Number_Forms"] = NUMBER_FORMS;
	names_[L"Ogham"] = OGHAM;
	names_[L"Old_Italic"] = OLD_ITALIC;
	names_[L"Old_Persian"] = OLD_PERSIAN;
	names_[L"Optical_Character_Recognition"] = OPTICAL_CHARACTER_RECOGNITION;
	names_[L"Oriya"] = ORIYA;
	names_[L"Osmanya"] = OSMANYA;
	names_[L"Phags-pa"] = PHAGS_PA;
	names_[L"Phoenician"] = PHOENICIAN;
	names_[L"Phonetic_Extensions"] = PHONETIC_EXTENSIONS;
	names_[L"Phonetic_Extensions_Supplement"] = PHONETIC_EXTENSIONS_SUPPLEMENT;
	names_[L"Private_Use_Area"] = PRIVATE_USE_AREA;
	names_[L"Runic"] = RUNIC;
	names_[L"Shavian"] = SHAVIAN;
	names_[L"Sinhala"] = SINHALA;
	names_[L"Small_Form_Variants"] = SMALL_FORM_VARIANTS;
	names_[L"Spacing_Modifier_Letters"] = SPACING_MODIFIER_LETTERS;
	names_[L"Specials"] = SPECIALS;
	names_[L"Superscripts_and_Subscripts"] = SUPERSCRIPTS_AND_SUBSCRIPTS;
	names_[L"Supplemental_Arrows-A"] = SUPPLEMENTAL_ARROWS_A;
	names_[L"Supplemental_Arrows-B"] = SUPPLEMENTAL_ARROWS_B;
	names_[L"Supplemental_Mathematical_Operators"] = SUPPLEMENTAL_MATHEMATICAL_OPERATORS;
	names_[L"Supplemental_Punctuation"] = SUPPLEMENTAL_PUNCTUATION;
	names_[L"Supplementary_Private_Use_Area-A"] = SUPPLEMENTARY_PRIVATE_USE_AREA_A;
	names_[L"Supplementary_Private_Use_Area-B"] = SUPPLEMENTARY_PRIVATE_USE_AREA_B;
	names_[L"Syloti_Nagri"] = SYLOTI_NAGRI;
	names_[L"Syriac"] = SYRIAC;
	names_[L"Tagalog"] = TAGALOG;
	names_[L"Tagbanwa"] = TAGBANWA;
	names_[L"Tags"] = TAGS;
	names_[L"Tai_Le"] = TAI_LE;
	names_[L"Tai_Xuan_Jing_Symbols"] = TAI_XUAN_JING_SYMBOLS;
	names_[L"Tamil"] = TAMIL;
	names_[L"Telugu"] = TELUGU;
	names_[L"Thaana"] = THAANA;
	names_[L"Thai"] = THAI;
	names_[L"Tibetan"] = TIBETAN;
	names_[L"Tifinagh"] = TIFINAGH;
	names_[L"Ugaritic"] = UGARITIC;
	names_[L"Unified_Canadian_Aboriginal_Syllabics"] = UNIFIED_CANADIAN_ABORIGINAL_SYLLABICS;
	names_[L"Variation_Selectors"] = VARIATION_SELECTORS;
	names_[L"Variation_Selectors_Supplement"] = VARIATION_SELECTORS_SUPPLEMENT;
	names_[L"Vertical_Forms"] = VERTICAL_FORMS;
	names_[L"Yi_Radicals"] = YI_RADICALS;
	names_[L"Yi_Syllables"] = YI_SYLLABLES;
	names_[L"Yijing_Hexagram_Symbols"] = YIJING_HEXAGRAM_SYMBOLS;
}


#ifndef ASCENSION_NO_UNICODE_NORMALIZATION

// CanonicalCombiningClass //////////////////////////////////////////////////

/// The long name of the property.
const Char CanonicalCombiningClass::LONG_NAME[] = L"Canonical_Combining_Class";
/// The shoer name of the property.
const Char CanonicalCombiningClass::SHORT_NAME[] = L"ccc";
map<const Char*, int, PropertyNameComparer<Char> > CanonicalCombiningClass::names_;

void CanonicalCombiningClass::buildNames() {
	names_[L"ATBL"] = names_[L"Attached_Below_Left"] = 200;
	names_[L"ATB"] = names_[L"Attached_Below"] = 202;
	names_[L"ATAR"] = names_[L"Attached_Above_Right"] = 216;
	names_[L"BL"] = names_[L"Below_Left"] = 218;
	names_[L"B"] = names_[L"Below"] = 220;
	names_[L"BR"] = names_[L"Below_Right"] = 222;
	names_[L"L"] = names_[L"Left"] = 224;
	names_[L"R"] = names_[L"Right"] = 226;
	names_[L"AL"] = names_[L"Above_Left"] = 228;
	names_[L"A"] = names_[L"Above"] = 230;
	names_[L"AR"] = names_[L"Above_Right"] = 232;
	names_[L"DB"] = names_[L"Double_Below"] = 233;
	names_[L"DA"] = names_[L"Double_Above"] = 234;
	names_[L"IS"] = names_[L"Iota_Subscript"] = 240;
}

#endif /* !ASCENSION_NO_UNICODE_NORMALIZATION */


// Script ///////////////////////////////////////////////////////////////////

/// The long name of the property.
const Char Script::LONG_NAME[] = L"Script";
/// The short name of the property.
const Char Script::SHORT_NAME[] = L"sc";
map<const Char*, int, PropertyNameComparer<Char> > Script::names_;

void Script::buildNames() {
	names_[L"Arab"] = names_[L"Arabic"] = ARABIC;
	names_[L"Armn"] = names_[L"Armenian"] = ARMENIAN;
	names_[L"Bali"] = names_[L"Balinese"] = BALINESE;
	names_[L"Beng"] = names_[L"Bengali"] = BENGALI;
	names_[L"Bopo"] = names_[L"Bopomofo"] = BOPOMOFO;
	names_[L"Brai"] = names_[L"Braille"] = BRAILLE;
	names_[L"Bugi"] = names_[L"Buginese"] = BUGINESE;
	names_[L"Buhd"] = names_[L"Buhid"] = BUHID;
	names_[L"Cans"] = names_[L"Canadian_Aboriginal"] = CANADIAN_ABORIGINAL;
	names_[L"Cher"] = names_[L"Cherokee"] = CHEROKEE;
	names_[L"Copt"] = names_[L"Coptic"] = names_[L"Qaac"] = COPTIC;
	names_[L"Cprt"] = names_[L"Cypriot"] = CYPRIOT;
	names_[L"Cyrl"] = names_[L"Cyrillic"] = CYRILLIC;
	names_[L"Deva"] = names_[L"Devanagari"] = DEVANAGARI;
	names_[L"Dsrt"] = names_[L"Deseret"] = DESERET;
	names_[L"Ethi"] = names_[L"Ethiopic"] = ETHIOPIC;
	names_[L"Geor"] = names_[L"Georgian"] = GEORGIAN;
	names_[L"Glag"] = names_[L"Glagolitic"] = GLAGOLITIC;
	names_[L"Goth"] = names_[L"Gothic"] = GOTHIC;
	names_[L"Grek"] = names_[L"Greek"] = GREEK;
	names_[L"Gujr"] = names_[L"Gujarati"] = GUJARATI;
	names_[L"Guru"] = names_[L"Gurmukhi"] = GURMUKHI;
	names_[L"Hang"] = names_[L"Hangul"] = HANGUL;
	names_[L"Hani"] = names_[L"Han"] = HAN;
	names_[L"Hano"] = names_[L"Hanunoo"] = HANUNOO;
	names_[L"Hebr"] = names_[L"Hebrew"] = HEBREW;
	names_[L"Hira"] = names_[L"Hiragana"] = HIRAGANA;
	names_[L"Hrkt"] = names_[L"Katakana_Or_Hiragana"] = KATAKANA_OR_HIRAGANA;
	names_[L"Ital"] = names_[L"Old_Italic"] = OLD_ITALIC;
	names_[L"Kana"] = names_[L"Katakana"] = KATAKANA;
	names_[L"Khar"] = names_[L"Kharoshthi"] = KHAROSHTHI;
	names_[L"Khmr"] = names_[L"Khmer"] = KHMER;
	names_[L"Knda"] = names_[L"Kannada"] = KANNADA;
	names_[L"Laoo"] = names_[L"Lao"] = LAO;
	names_[L"Latn"] = names_[L"Latin"] = LATIN;
	names_[L"Limb"] = names_[L"Limbu"] = LIMBU;
	names_[L"Linb"] = names_[L"Linear_B"] = LINEAR_B;
	names_[L"Mlym"] = names_[L"Malayalam"] = MALAYALAM;
	names_[L"Mong"] = names_[L"Mongolian"] = MONGOLIAN;
	names_[L"Mymr"] = names_[L"Myanmar"] = MYANMAR;
	names_[L"Nkoo"] = names_[L"Nko"] = NKO;
	names_[L"Ogam"] = names_[L"Ogham"] = OGHAM;
	names_[L"Orya"] = names_[L"Oriya"] = ORIYA;
	names_[L"Osma"] = names_[L"Osmanya"] = OSMANYA;
	names_[L"Phag"] = names_[L"Phags_Pa"] = PHAGS_PA;
	names_[L"Phnx"] = names_[L"Phoenician"] = PHOENICIAN;
	names_[L"Qaai"] = names_[L"Inherited"] = INHERITED;
	names_[L"Runr"] = names_[L"Runic"] = RUNIC;
	names_[L"Shaw"] = names_[L"Shavian"] = SHAVIAN;
	names_[L"Sinh"] = names_[L"Sinhala"] = SINHALA;
	names_[L"Sylo"] = names_[L"Syloti_Nagri"] = SYLOTI_NAGRI;
	names_[L"Syrc"] = names_[L"Syriac"] = SYRIAC;
	names_[L"Tagb"] = names_[L"Tagbanwa"] = TAGBANWA;
	names_[L"Tale"] = names_[L"Tai_Le"] = TAI_LE;
	names_[L"Talu"] = names_[L"New_Tai_Lue"] = NEW_TAI_LUE;
	names_[L"Taml"] = names_[L"Tamil"] = TAMIL;
	names_[L"Telu"] = names_[L"Telugu"] = TELUGU;
	names_[L"Tfng"] = names_[L"Tifinagh"] = TIFINAGH;
	names_[L"Tglg"] = names_[L"Tagalog"] = TAGALOG;
	names_[L"Thaa"] = names_[L"Thaana"] = THAANA;
	names_[L"Thai"] = THAI;
	names_[L"Tibt"] = names_[L"Tibetan"] = TIBETAN;
	names_[L"Ugar"] = names_[L"Ugaritic"] = UGARITIC;
	names_[L"Xpeo"] = names_[L"Old_Persian"] = OLD_PERSIAN;
	names_[L"Xsux"] = names_[L"Cuneiform"] = CUNEIFORM;
	names_[L"Yiii"] = names_[L"Yi"] = YI;
	names_[L"Zyyy"] = names_[L"Common"] = COMMON;
	names_[L"Zzzz"] = names_[L"Unknown"] = UNKNOWN;
}


// HangulSyllableType ///////////////////////////////////////////////////////

/// The long name of the property.
const Char HangulSyllableType::LONG_NAME[] = L"Hangul_Syllable_Type";
/// The short name of the property.
const Char HangulSyllableType::SHORT_NAME[] = L"hst";
map<const Char*, int, PropertyNameComparer<Char> > HangulSyllableType::names_;

void HangulSyllableType::buildNames() {
	PROP("L",	"Leading_Jamo",		LEADING_JAMO);
	PROP("LV",	"LV_Syllable",		LV_SYLLABLE);
	PROP("LVT",	"LVT_Syllable",		LVT_SYLLABLE);
	PROP("NA",	"Not_Applicable",	NOT_APPLICABLE);
	PROP("T",	"Trailing_Jamo",	TRAILING_JAMO);
	PROP("V",	"Vowel_Jamo",		VOWEL_JAMO);
}


// BinaryProperty ///////////////////////////////////////////////////////////

#include "code-table/uprops-table"

//map<const Char*, int, PropertyNameComparer<Char> > BinaryProperty::names_;

/**
 * Returns true if the specified character has the binary property.
 * @param cp the code point of the character
 * @param property the binary property
 * @return true if the character of @a cp has @a property
 */
bool BinaryProperty::is(CodePoint cp, int property) {
	switch(property) {
	case ALPHABETIC:							return is<ALPHABETIC>(cp);
	case ASCII_HEX_DIGIT:						return is<ASCII_HEX_DIGIT>(cp);
	case BIDI_CONTROL:							return is<BIDI_CONTROL>(cp);
//	case BIDI_MIRRORED:							return is<BIDI_MIRRORED>(cp);
//	case COMPOSITION_EXCLUSION:					return is<COMPOSITION_EXCLUSION>(cp);
	case DASH:									return is<DASH>(cp);
	case DEFAULT_IGNORABLE_CODE_POINT:			return is<DEFAULT_IGNORABLE_CODE_POINT>(cp);
	case DEPRECATED:							return is<DEPRECATED>(cp);
	case DIACRITIC:								return is<DIACRITIC>(cp);
//	case EXPANDS_ON_NFC:						return is<EXPANDS_ON_NFC>(cp);
//	case EXPANDS_ON_NFD:						return is<EXPANDS_ON_NFD>(cp);
//	case EXPANDS_ON_NFKC:						return is<EXPANDS_ON_NFKC>(cp);
//	case EXPANDS_ON_NFKD:						return is<EXPANDS_ON_NFKD>(cp);
	case EXTENDER:								return is<EXTENDER>(cp);
//	case FULL_COMPOSITION_EXCLUSION:			return is<FULL_COMPOSITION_EXCLUSION>(cp);
	case GRAPHEME_BASE:							return is<GRAPHEME_BASE>(cp);
	case GRAPHEME_EXTEND:						return is<GRAPHEME_EXTEND>(cp);
	case HEX_DIGIT:								return is<HEX_DIGIT>(cp);
	case HYPHEN:								return is<HYPHEN>(cp);
	case ID_CONTINUE:							return is<ID_CONTINUE>(cp);
	case ID_START:								return is<ID_START>(cp);
	case IDEOGRAPHIC:							return is<IDEOGRAPHIC>(cp);
	case IDS_BINARY_OPERATOR:					return is<IDS_BINARY_OPERATOR>(cp);
	case IDS_TRINARY_OPERATOR:					return is<IDS_TRINARY_OPERATOR>(cp);
	case JOIN_CONTROL:							return is<JOIN_CONTROL>(cp);
	case LOGICAL_ORDER_EXCEPTION:				return is<LOGICAL_ORDER_EXCEPTION>(cp);
	case LOWERCASE:								return is<LOWERCASE>(cp);
	case MATH:									return is<MATH>(cp);
	case NONCHARACTER_CODE_POINT:				return is<NONCHARACTER_CODE_POINT>(cp);
	case OTHER_ALPHABETIC:						return is<OTHER_ALPHABETIC>(cp);
	case OTHER_DEFAULT_IGNORABLE_CODE_POINT:	return is<OTHER_DEFAULT_IGNORABLE_CODE_POINT>(cp);
	case OTHER_GRAPHEME_EXTEND:					return is<OTHER_GRAPHEME_EXTEND>(cp);
	case OTHER_ID_CONTINUE:						return is<OTHER_ID_CONTINUE>(cp);
	case OTHER_ID_START:						return is<OTHER_ID_START>(cp);
	case OTHER_LOWERCASE:						return is<OTHER_LOWERCASE>(cp);
	case OTHER_MATH:							return is<OTHER_MATH>(cp);
	case OTHER_UPPERCASE:						return is<OTHER_UPPERCASE>(cp);
	case PATTERN_SYNTAX:						return is<PATTERN_SYNTAX>(cp);
	case PATTERN_WHITE_SPACE:					return is<PATTERN_WHITE_SPACE>(cp);
	case QUOTATION_MARK:						return is<QUOTATION_MARK>(cp);
	case RADICAL:								return is<RADICAL>(cp);
	case SOFT_DOTTED:							return is<SOFT_DOTTED>(cp);
	case STERM:									return is<STERM>(cp);
	case TERMINAL_PUNCTUATION:					return is<TERMINAL_PUNCTUATION>(cp);
	case UNIFIED_IDEOGRAPH:						return is<UNIFIED_IDEOGRAPH>(cp);
	case UPPERCASE:								return is<UPPERCASE>(cp);
	case VARIATION_SELECTOR:					return is<VARIATION_SELECTOR>(cp);
	case WHITE_SPACE:							return is<WHITE_SPACE>(cp);
//	case XID_CONTINUE:							return is<XID_CONTINUE>(cp);
//	case XID_START:								return is<XID_START>(cp);
	default:									return false;
	}
}


// GraphemeClusterBreak /////////////////////////////////////////////////////

/// The long name of the property.
const Char GraphemeClusterBreak::LONG_NAME[] = L"Grapheme_Cluster_Break";
/// The short name of the property.
const Char GraphemeClusterBreak::SHORT_NAME[] = L"GCB";
map<const Char*, int, PropertyNameComparer<Char> > GraphemeClusterBreak::names_;

/// Returns Grapheme_Cluster_Break value of the specified character.
int GraphemeClusterBreak::of(CodePoint cp) throw() {
	if(cp == CARRIAGE_RETURN)
		return CR;
	else if(cp == LINE_FEED)
		return LF;
	const int gc = GeneralCategory::of(cp);
	if((gc == GeneralCategory::SEPARATOR_LINE
			|| gc == GeneralCategory::SEPARATOR_PARAGRAPH
			|| gc == GeneralCategory::OTHER_CONTROL
			|| gc == GeneralCategory::OTHER_FORMAT)
			&& cp != ZERO_WIDTH_NON_JOINER
			&& cp != ZERO_WIDTH_JOINER)
		return CONTROL;
	else if(BinaryProperty::is<BinaryProperty::GRAPHEME_EXTEND>(cp))
		return EXTEND;
	switch(HangulSyllableType::of(cp)) {
	case HangulSyllableType::LEADING_JAMO:	return L;
	case HangulSyllableType::VOWEL_JAMO:	return V;
	case HangulSyllableType::TRAILING_JAMO:	return T;
	case HangulSyllableType::LV_SYLLABLE:	return LV;
	case HangulSyllableType::LVT_SYLLABLE:	return LVT;
	default:								return OTHER;
	}
}

void GraphemeClusterBreak::buildNames() {
	PROP("CN",	"Control",	CONTROL);
	PROP1("CR",				CR);
	PROP("EX",	"Extend",	EXTEND);
	PROP1("L",				L);
	PROP1("LF",				LF);
	PROP1("LV",				LV);
	PROP1("LVT",			LVT);
	PROP1("T",				T);
	PROP1("V",				V);
	PROP("XX",	"OTHER",	OTHER);
}


// WordBreak ////////////////////////////////////////////////////////////////

/// The long name of the property.
const Char WordBreak::LONG_NAME[] = L"Word_Break";
/// The short name of the property.
const Char WordBreak::SHORT_NAME[] = L"WB";
map<const Char*, int, PropertyNameComparer<Char> > WordBreak::names_;

/**
 * Returns Word_Break value of the specified character.
 * @param cp the character
 * @param syntax the identifier syntax definition for deciding what character is ID_Start
 * @param lc the locale
 */
int WordBreak::of(CodePoint cp,
		const IdentifierSyntax& syntax /* = IdentifierSyntax(IdentifierSyntax::UNICODE_DEFAULT) */,
		const locale& lc /* = locale::classic() */) throw() {
	static const CodePoint KATAKANAS[] = {
		0x3031,	// Vertical Kana Repeat Mark
		0x3032,	// Vertical Kana Repeat With Voiced Sound Mark
		0x3033,	// Vertical Kana Repeat Mark Upper Half
		0x3034,	// Vertical Kana Repeat With Voiced Sound Mark Upper Half
		0x3035,	// Vertical Kana Repeat Mark Lower Half
		0x309B,	// Katakana-Hiragana Voiced Sound Mark
		0x309C,	// Katakana-Hiragana Semi-Voiced Sound Mark
		0x30A0,	// Katakana-Hiragana Double Hyphen
		0x30FC,	// Katakana-Hiragana Prolonged Sound Mark
		0xFF70,	// Halfwidth Katakana-Hiragana Prolonged Sound Mark
		0xFF9E,	// Halfwidth Katakana Voiced Sound Mark
		0xFF9F	// Halfwidth Katakana Semi-Voiced Sound Mark
	};
	static const CodePoint MID_LETTERS[] = {
		0x0027,	// Apostrophe
		0x00B7,	// Middle Dot
		0x05F4,	// Hebrew Punctuation Gershayim
		0x2019,	// Right Single Quotation Mark
		0x2027	// Hyphenation Point
	};
	static const CodePoint MID_NUMS[] = {
		0x002C,	// Comma
		0x002E,	// Full Stop
		0x003B,	// Semicolon
		0x037E,	// Greek Question Mark
		0x0589,	// Armenian Full Stop
		0x060D,	// Arabic Date Separator
		0x2044,	// Fraction Slash
		0xFE10,	// Presentation Form For Vertical Comma
		0xFE13,	// Presentation Form For Vertical Colon
		0xFE14	// Presentation Form For Vertical Semicolon
	};
	static bool localeInitialized = false;
	static auto_ptr<locale> japanese, swedish;
	if(!localeInitialized) {
		localeInitialized = true;
		try {
			japanese.reset(new locale("japanese"));
			swedish.reset(new locale("swedish"));
		} catch(...) {
			japanese.reset(0);
		}
	}
	if(cp == CARRIAGE_RETURN)
		return GraphemeClusterBreak::CR;
	else if(cp == LINE_FEED)
		return GraphemeClusterBreak::LF;
	const int gc = GeneralCategory::of(cp);
	if(gc == GeneralCategory::OTHER_FORMAT && cp != ZERO_WIDTH_NON_JOINER && cp != ZERO_WIDTH_JOINER)
		return FORMAT;
	else if(Script::of(cp) == Script::KATAKANA
			|| binary_search(KATAKANAS, endof(KATAKANAS), cp))
		return KATAKANA;
	else if(BinaryProperty::is<BinaryProperty::GRAPHEME_EXTEND>(cp))
		return GraphemeClusterBreak::EXTEND;
	else if((syntax.isIdentifierStartCharacter(cp)
			|| cp == 0x00A0		// No-Break Space
			|| cp == 0x05F3))	// Hebrew Punctuation Geresh
		return A_LETTER;
	else if(binary_search(MID_LETTERS, endof(MID_LETTERS), cp)
			|| (cp == 0x003A && swedish.get() != 0 && lc == *swedish.get()))	// Colon (for Swedish)
		return MID_LETTER;
	else if(binary_search(MID_NUMS, endof(MID_NUMS), cp))
		return MID_NUM;
	else if(isNU(cp, gc))
		return NUMERIC;
	else if(gc == GeneralCategory::PUNCTUATION_CONNECTOR)
		return EXTEND_NUM_LET;
	else
		return OTHER;
}

void WordBreak::buildNames() {
	PROP("EX",	"ExtendNumLet",	EXTEND_NUM_LET);
	PROP("FO",	"Format",		FORMAT);
	PROP("KA",	"Katakana",		KATAKANA);
	PROP("LE",	"ALetter",		A_LETTER);
	PROP("ML",	"MidLetter",	MID_LETTER);
	PROP("MN",	"MidNum",		MID_NUM);
	PROP("NU",	"Numeric",		NUMERIC);
	PROP("XX",	"Other",		OTHER);
}


// SentenceBreak ////////////////////////////////////////////////////////////

/// The long name of the property.
const Char SentenceBreak::LONG_NAME[] = L"Sentence_Break";
/// The short name of the property.
const Char SentenceBreak::SHORT_NAME[] = L"SB";
map<const Char*, int, PropertyNameComparer<Char> > SentenceBreak::names_;

/// Returns Sentence_Break value of the specified character.
int SentenceBreak::of(CodePoint cp) throw() {
	static const CodePoint SEPS[] = {LINE_FEED, CARRIAGE_RETURN, NEXT_LINE, LINE_SEPARATOR, PARAGRAPH_SEPARATOR};
	if(BinaryProperty::is<BinaryProperty::GRAPHEME_EXTEND>(cp))
		return GraphemeClusterBreak::EXTEND;
	else if(binary_search(SEPS, endof(SEPS), cp))
		return SEP;
	const int gc = GeneralCategory::of(cp);
	if(gc == GeneralCategory::OTHER_FORMAT && cp != ZERO_WIDTH_NON_JOINER && cp != ZERO_WIDTH_JOINER)
		return FORMAT;
	else if(BinaryProperty::is<BinaryProperty::WHITE_SPACE>(cp) && cp != 0x00A0)
		return SP;
	else if(BinaryProperty::is<BinaryProperty::LOWERCASE>(cp))
		return LOWER;
	else if(gc == GeneralCategory::LETTER_TITLECASE || BinaryProperty::is<BinaryProperty::UPPERCASE>(cp))
		return UPPER;
	else if(BinaryProperty::is<BinaryProperty::ALPHABETIC>(cp) || cp == 0x00A0 || cp == 0x05F3)
		return O_LETTER;
	else if(isNU(cp, gc))
		return NUMERIC;
	else if(cp == 0x002E)
		return A_TERM;
	else if(BinaryProperty::is<BinaryProperty::STERM>(cp))
		return S_TERM;
	else if(gc == GeneralCategory::PUNCTUATION_OPEN || gc == GeneralCategory::PUNCTUATION_CLOSE || isQU(cp, gc))
		return CLOSE;
	return OTHER;
}

void SentenceBreak::buildNames() {
	PROP("AT",	"ATerm",	A_TERM);
	PROP("CL",	"Close",	CLOSE);
	PROP("FO",	"Format",	FORMAT);
	PROP("LE",	"OLetter",	O_LETTER);
	PROP("LO",	"Lower",	LOWER);
	PROP("NU",	"Numeric",	NUMERIC);
	PROP("SE",	"Sep",		SEP);
	PROP1("SP",				SP);
	PROP("ST",	"STerm",	S_TERM);
	PROP("UP",	"Upper",	UPPER);
	PROP("XX",	"Other",	OTHER);
}

#undef PROP


// StringFolder /////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param text the original string
 * @param options the folding options
 */
StringFolder::StringFolder(const String& text, const FoldingOptions& options) : original_(text) {
}
