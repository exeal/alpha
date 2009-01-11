/**
 * @file unicode-property.cpp
 * @author exeal
 * @date 2005-2009
 */

#include <ascension/unicode-property.hpp>
using namespace ascension;
using namespace ascension::text;
using namespace ascension::text::ucd;
using namespace ascension::text::ucd::internal;
using namespace std;


// CharacterIterator ////////////////////////////////////////////////////////

/**
 * @class ascension::text::CharacterIterator
 * Abstract class defines an interface for bidirectional iteration on text.
 *
 * <h3>Code point-based interface</h3>
 *
 * The operations perform using code point (not code unit). @c #current returns a code point (not
 * code unit value) of the character the iterator adresses, and @c #next skips a legal low
 * surrogate code unit.
 *
 * Following example prints all code points of the text.
 *
 * @code
 * void printAllCharacters(CharacterIterator& i) {
 *   for(i.first(); i.hasNext(); i.next())
 *     print(i.current());
 * }
 * @endcode
 *
 * Relational operations (@c #equals and @c #less) must be applied to the same types.
 *
 * @code
 * StringCharacterIterator i1 = ...;
 * auto_ptr<CharacterIterator> i2 = i1.clone(); // i2 is a clone of i1
 * StringCharacterIterator i3 = ...;            // i3 is not a clone of i1, but has a same type
 * DocumentCharacterIterator i4 = ...;          // i4 is not a clone of i1, and has a different type
 *
 * i1.equals(*i2); // ok
 * i2->less(i1);   // ok
 * i1.equals(i3);  // ok
 * i1.equals(i4);  // error! (std::invalid_argument exception)
 * @endcode
 *
 * Also, @c #assign has a like restricton. Any partial assignments are not allowed.
 *
 * <h3>Offsets</h3>
 *
 * A @c CharacterIterator has a position in the character sequence (offset). Initial offset value
 * is 0, and will be decremented or incremented when the iterator moves.
 *
 * The offset will be reset to 0 also when @c first or @c last is called.
 *
 * @code
 * CharacterIterator i = ...;
 * i.first();    // offset == 0
 * i.next();     // offset == 1 (or 2 if the first character is not in BMP)
 * i.last();     // offset == 0
 * i.previous(); // offset == -1 (or -2)
 * @endcode
 *
 * <h3>Implementation of @c CharacterIterator class</h3>
 *
 * A concrete iterator class must implement the following private methods:
 *
 * - @c #doAssign for @c #assign.
 * - @c #doClone for @c #clone.
 * - @c #doFirst and @c #doLast for @c #first and @c #last.
 * - @c #doEquals and @c #doLess for @c #equals and @c #less.
 * - @c #doNext and @c #doPrevious for @c #next and @c #previous
 *
 * And also must implement the following public methods: @c #current, @c #hasNext, @c #hasPrevious.
 *
 * @c #doClone must be implemented by protected copy-constructor of @c CharacterIterator.
 * @c #doAssign must be implemented by protected <code>CharacterIterator#operator=</code>.
 *
 * <h3>Type-safety</h3>
 *
 * @c CharacterIterator is an abstract type and actual types of two concrete instances may be
 * different. This makes comparison of iterators difficult.
 *
 * Instances of @c CharacterIterator know the derived class (by using @c ConcreteTypeTag). So the
 * right-hand-sides of @c #doAssign, @c #doEquals, and @c #doLess have same types of the callee.
 * This means that the following implementation with down-cast is safe.
 *
 * @code
 * bool MyIterator::equals(const CharacterIterator& rhs) const {
 *   // rhs actually refers a MyIterator.
 *   const MyIterator& concrete = static_cast<const MyIterator&>(rhs);
 *   // compare this and concrete...
 * }
 * @endcode
 */

/// Indicates the iterator is the last.
const CodePoint CharacterIterator::DONE = 0xFFFFFFFFUL;


// StringCharacterIterator //////////////////////////////////////////////////

const CharacterIterator::ConcreteTypeTag StringCharacterIterator::CONCRETE_TYPE_TAG_ = CharacterIterator::ConcreteTypeTag();

/// Default constructor.
StringCharacterIterator::StringCharacterIterator() /*throw()*/ : CharacterIterator(CONCRETE_TYPE_TAG_) {
}

StringCharacterIterator::StringCharacterIterator(const Char* first, const Char* last) :
		CharacterIterator(CONCRETE_TYPE_TAG_), current_(first), first_(first), last_(last) {
	if(first_ > last_)
		throw invalid_argument("the first is greater than last.");
}

StringCharacterIterator::StringCharacterIterator(const Char* first, const Char* last, const Char* start) :
		CharacterIterator(CONCRETE_TYPE_TAG_), current_(start), first_(first), last_(last) {
	if(first_ > last_ || current_ < first_ || current_ > last_)
		throw invalid_argument("invalid input.");
}

StringCharacterIterator::StringCharacterIterator(const String& s) :
		CharacterIterator(CONCRETE_TYPE_TAG_), current_(s.data()), first_(s.data()), last_(s.data() + s.length()) {
	if(first_ > last_)
		throw invalid_argument("the first is greater than last.");
}

StringCharacterIterator::StringCharacterIterator(const String& s, String::const_iterator start) :
		CharacterIterator(CONCRETE_TYPE_TAG_), current_(s.data() + (start - s.begin())), first_(s.data()), last_(s.data() + s.length()) {
	if(first_ > last_ || current_ < first_ || current_ > last_)
		throw invalid_argument("invalid input.");
}

/// Copy-constructor.
StringCharacterIterator::StringCharacterIterator(const StringCharacterIterator& rhs) /*throw()*/
		: CharacterIterator(rhs), current_(rhs.current_), first_(rhs.first_), last_(rhs.last_) {
}

/// @see CharacterIterator#current
CodePoint StringCharacterIterator::current() const /*throw()*/ {
	return (current_ != last_) ? surrogates::decodeFirst(current_, last_) : DONE;
}

/// @see CharacterIterator#doAssign
void StringCharacterIterator::doAssign(const CharacterIterator& rhs) {
	CharacterIterator::operator=(rhs);
	current_ = static_cast<const StringCharacterIterator&>(rhs).current_;
	first_ = static_cast<const StringCharacterIterator&>(rhs).first_;
	last_ = static_cast<const StringCharacterIterator&>(rhs).last_;
}

/// @see CharacterIterator#doClone
auto_ptr<CharacterIterator> StringCharacterIterator::doClone() const {
	return auto_ptr<CharacterIterator>(new StringCharacterIterator(*this));
}

/// @see CharacterIterator#doEquals
bool StringCharacterIterator::doEquals(const CharacterIterator& rhs) const {
	return current_ == static_cast<const StringCharacterIterator&>(rhs).current_;
}

/// @see CharacterIterator#doFirst
void StringCharacterIterator::doFirst() {
	current_ = first_;
}

/// @see CharacterIterator#doLast
void StringCharacterIterator::doLast() {
	current_ = last_;
}

/// @see CharacterIterator#doLess
bool StringCharacterIterator::doLess(const CharacterIterator& rhs) const {
	return current_ < static_cast<const StringCharacterIterator&>(rhs).current_;
}

/// @see CharacterIterator#doNext
void StringCharacterIterator::doNext() {
	if(current_ == last_)
//		throw out_of_range("the iterator is the last.");
		return;
	current_ = surrogates::next(current_, last_);
}

/// @see CharacterIterator#doPrevious
void StringCharacterIterator::doPrevious() {
	if(current_ == first_)
//		throw out_of_range("the iterator is the first.");
		return;
	current_ = surrogates::previous(first_, current_);
}


namespace {
	/// Returns true if the specified character is Line_Break=NU.
	bool isNU(CodePoint cp, int gc) /*throw()*/ {
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
	bool isQU(CodePoint cp, int gc) /*throw()*/ {
		return gc == GeneralCategory::PUNCTUATION_FINAL_QUOTE
			|| gc == GeneralCategory::PUNCTUATION_INITIAL_QUOTE
			|| binary_search(QU, MANAH_ENDOF(QU), cp);
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
const PropertyBase<GeneralCategory>::Names GeneralCategory::names_[GeneralCategory::LAST_VALUE - GeneralCategory::FIRST_VALUE] = {
	{L"Ll",	L"Lowercase_Letter"},
	{L"Lu",	L"Uppercase_Letter"},
	{L"Lt",	L"Titlecase_Letter"},
	{L"Lm",	L"Modifier_Letter"},
	{L"Lo",	L"Other_Letter"},
	{L"Mn",	L"Nonspacing_Mark"},
	{L"Mc",	L"Spacing_Mark"},
	{L"Me",	L"Enclosing_Mark"},
	{L"Nd",	L"Decimal_Number"},
	{L"Nl",	L"Letter_Number"},
	{L"No",	L"Other_Number"},
	{L"Pc",	L"Connector_Punctuation"},
	{L"Pd",	L"Dash_Punctuation"},
	{L"Ps",	L"Open_Punctuation"},
	{L"Pe",	L"Close_Punctuation"},
	{L"Pi",	L"Initial_Punctuation"},
	{L"Pf",	L"Final_Punctuation"},
	{L"Po",	L"Other_Punctuation"},
	{L"Sm",	L"Math_Symbol"},
	{L"Sc",	L"Currency_Symbol"},
	{L"Sk",	L"Modifier_Symbol"},
	{L"So",	L"Other_Symbol"},
	{L"Zs",	L"Space_Separator"},
	{L"Zl",	L"Line_Separator"},
	{L"Zp",	L"Paragraph_Separator"},
	{L"Cc",	L"Control"},
	{L"Cf",	L"Format"},
	{L"Cs",	L"Surrogate"},
	{L"Co",	L"Private_Use"},
	{L"Cn",	L"Unassigned"},
	{L"L&",	L"Letter"},
	{L"Lc",	L"Cased_Letter"},
	{L"M&",	L"Mark"},
	{L"N&",	L"Number"},
	{L"P&",	L"Punctuation"},
	{L"S&",	L"Symbol"},
	{L"Z&",	L"Separator"},
	{L"C&",	L"Other"}
};


// CodeBlock ////////////////////////////////////////////////////////////////

/// The long name of the property.
const Char CodeBlock::LONG_NAME[] = L"Block";
/// The short name of the property.
const Char CodeBlock::SHORT_NAME[] = L"blk";
const CodeBlock::Names CodeBlock::names_[CodeBlock::LAST_VALUE - CodeBlock::FIRST_VALUE] = {
	{0, L"No_Block"},
	{0, L"Basic_Latin"},
	{0, L"Latin-1_Supplement"},
	{0, L"Latin_Extended-A"},
	{0, L"Latin_Extended-B"},
	{0, L"IPA_Extensions"},
	{0, L"Spacing_Modifier_Letters"},
	{0, L"Combining_Diacritical_Marks"},
	{0, L"Greek_and_Coptic"},
	{0, L"Cyrillic"},
	{0, L"Cyrillic_Supplement"},
	{0, L"Armenian"},
	{0, L"Hebrew"},
	{0, L"Arabic"},
	{0, L"Syriac"},
	{0, L"Arabic_Supplement"},
	{0, L"Thaana"},
	{0, L"NKo"},
	{0, L"Devanagari"},
	{0, L"Bengali"},
	{0, L"Gurmukhi"},
	{0, L"Gujarati"},
	{0, L"Oriya"},
	{0, L"Tamil"},
	{0, L"Telugu"},
	{0, L"Kannada"},
	{0, L"Malayalam"},
	{0, L"Sinhala"},
	{0, L"Thai"},
	{0, L"Lao"},
	{0, L"Tibetan"},
	{0, L"Myanmar"},
	{0, L"Georgian"},
	{0, L"Hangul_Jamo"},
	{0, L"Ethiopic"},
	{0, L"Ethiopic_Supplement"},
	{0, L"Cherokee"},
	{0, L"Unified_Canadian_Aboriginal_Syllabics"},
	{0, L"Ogham"},
	{0, L"Runic"},
	{0, L"Tagalog"},
	{0, L"Hanunoo"},
	{0, L"Buhid"},
	{0, L"Tagbanwa"},
	{0, L"Khmer"},
	{0, L"Mongolian"},
	{0, L"Libmu"},
	{0, L"Tai_Le"},
	{0, L"New_Tai_Lue"},
	{0, L"Khmer_Symbols"},
	{0, L"Buginese"},
	{0, L"Balinese"},
	{0, L"Phonetic_Extensions"},
	{0, L"Phonetic_Extensions_Supplement"},
	{0, L"Combining_Diacritical_Marks_Supplement"},
	{0, L"Latin_Extended_Additional"},
	{0, L"Greek_Extended"},
	{0, L"General_Punctuation"},
	{0, L"Superscripts_and_Subscripts"},
	{0, L"Currency_Symbols"},
	{0, L"Combining_Diacritical_Marks_for_Symbols"},
	{0, L"Letterlike_Symbols"},
	{0, L"Number_Forms"},
	{0, L"Arrows"},
	{0, L"Mathematical_Operators"},
	{0, L"Miscellaneous_Technical"},
	{0, L"Control_Pictures"},
	{0, L"Optical_Character_Recognition"},
	{0, L"Enclosed_Alphanumerics"},
	{0, L"Box_Drawing"},
	{0, L"Block_Elements"},
	{0, L"Geometric_Shapes"},
	{0, L"Miscellaneous_Symbols"},
	{0, L"Dingbats"},
	{0, L"Miscellaneous_Mathematical_Symbols-A"},
	{0, L"Supplemental_Arrows-A"},
	{0, L"Braille_Patterns"},
	{0, L"Supplemental_Arrows-B"},
	{0, L"Miscellaneous_Mathematical_Symbols-B"},
	{0, L"Supplemental_Mathematical_Operators"},
	{0, L"Miscellaneous_Symbols_and_Arrows"},
	{0, L"Glagolitic"},
	{0, L"Latin_Extended-C"},
	{0, L"Coptic"},
	{0, L"Georgian_Supplement"},
	{0, L"Tifinagh"},
	{0, L"Ethiopic_Extended"},
	{0, L"Supplemental_Punctuation"},
	{0, L"CJK_Radicals_Supplement"},
	{0, L"Kangxi_Radicals"},
	{0, L"Ideographic_Description_Characters"},
	{0, L"CJK_Symbols_and_Punctuation"},
	{0, L"Hiragana"},
	{0, L"Katakana"},
	{0, L"Bopomofo"},
	{0, L"Hangul_Compatibility_Jamo"},
	{0, L"Kanbun"},
	{0, L"Bopomofo_Extended"},
	{0, L"CJK_Strokes"},
	{0, L"Katakana_Phonetic_Extensions"},
	{0, L"Enclosed_CJK_Letters_and_Months"},
	{0, L"CJK_Compatibility"},
	{0, L"CJK_Unified_Ideographs_Extension_A"},
	{0, L"Yijing_Hexagram_Symbols"},
	{0, L"CJK_Unified_Ideographs"},
	{0, L"Yi_Syllables"},
	{0, L"Yi_Radicals"},
	{0, L"Modifier_Tone_Letters"},
	{0, L"Latin_Extended-D"},
	{0, L"Syloti_Nagri"},
	{0, L"Phags-pa"},
	{0, L"Hangul_Syllables"},
	{0, L"High_Surrogates"},
	{0, L"High_Private_Use_Surrogates"},
	{0, L"Low_Surrogates"},
	{0, L"Private_Use_Area"},
	{0, L"CJK_Compatibility_Ideographs"},
	{0, L"Alphabetic_Presentation_Forms"},
	{0, L"Arabic_Presentation_Forms-A"},
	{0, L"Variation_Selectors"},
	{0, L"Vertical_Forms"},
	{0, L"Combining_Half_Marks"},
	{0, L"CJK_Compatibility_Forms"},
	{0, L"Small_Form_Variants"},
	{0, L"Arabic_Presentation_Forms-B"},
	{0, L"Halfwidth_and_Fullwidth_Forms"},
	{0, L"Specials"},
	{0, L"Linear_B_Syllabary"},
	{0, L"Linear_B_Ideograms"},
	{0, L"Aegean_Numbers"},
	{0, L"Ancient_Greek_Numbers"},
	{0, L"Old_Italic"},
	{0, L"Gothic"},
	{0, L"Ugaritic"},
	{0, L"Old_Persian"},
	{0, L"Deseret"},
	{0, L"Shavian"},
	{0, L"Osmanya"},
	{0, L"Cypriot_Syllabary"},
	{0, L"Phoenician"},
	{0, L"Kharoshthi"},
	{0, L"Cuneiform"},
	{0, L"Cuneiform_Numbers_and_Punctuation"},
	{0, L"Byzantine_Musical_Symbols"},
	{0, L"Musical_Symbols"},
	{0, L"Ancient_Greek_Musical_Notation"},
	{0, L"Tai_Xuan_Jing_Symbols"},
	{0, L"Counting_Rod_Numerals"},
	{0, L"Mathematical_Alphanumeric_Symbols"},
	{0, L"CJK_Unified_Ideographs_Extension_B"},
	{0, L"CJK_Compatibility_Ideographs_Supplement"},
	{0, L"Tags"},
	{0, L"Variation_Selectors_Supplement"},
	{0, L"Supplementary_Private_Use_Area-A"},
	{0, L"Supplementary_Private_Use_Area-B"},
};


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

#endif // !ASCENSION_NO_UNICODE_NORMALIZATION


// Script ///////////////////////////////////////////////////////////////////

/// The long name of the property.
const Char Script::LONG_NAME[] = L"Script";
/// The short name of the property.
const Char Script::SHORT_NAME[] = L"sc";
const Script::Names Script::names_[Script::LAST_VALUE - Script::FIRST_VALUE] = {
	{L"Zzzz",	L"Unknown"},
	{L"Zyyy",	L"Common"},
	{L"Latn",	L"Latin"},
	{L"Grek",	L"Greek"},
	{L"Cyrl",	L"Cyrillic"},
	{L"Armn",	L"Armenian"},
	{L"Hebr",	L"Hebrew"},
	{L"Arab",	L"Arabic"},
	{L"Syrc",	L"Syriac"},
	{L"Thaa",	L"Thaana"},
	{L"Deva",	L"Devanagari"},
	{L"Beng",	L"Bengali"},
	{L"Guru",	L"Gurmukhi"},
	{L"Gujr",	L"Gujarati"},
	{L"Orya",	L"Oriya"},
	{L"Taml",	L"Tamil"},
	{L"Telu",	L"Telugu"},
	{L"Knda",	L"Kannada"},
	{L"Mlym",	L"Malayalam"},
	{L"Sinh",	L"Sinhala"},
	{0,			L"Thai"},
	{L"Laoo",	L"Laoo"},
	{L"Tibt",	L"Tibetan"},
	{L"Mymr",	L"Myanmar"},
	{L"Geor",	L"Georgian"},
	{L"Hang",	L"Hangul"},
	{L"Ethi",	L"Ethiopic"},
	{L"Cher",	L"Cherokee"},
	{L"Cans",	L"Canadian_Aboriginal"},
	{L"Ogam",	L"Ogham"},
	{L"Runr",	L"Runic"},
	{L"Khmr",	L"Khmer"},
	{L"Mong",	L"Mongolian"},
	{L"Hira",	L"Hiragana"},
	{L"Kana",	L"Katakana"},
	{L"Bopo",	L"Bopomofo"},
	{L"Hani",	L"Han"},
	{L"Yiii",	L"Yi"},
	{L"Ital",	L"Old_Italic"},
	{L"Goth",	L"Gothic"},
	{L"Dsrt",	L"Deseret"},
	{L"Qaai",	L"Inherited"},
	{L"Tglg",	L"Tagalog"},
	{L"Hano",	L"Hanunoo"},
	{L"Buhd",	L"Buhid"},
	{L"Tagb",	L"Tagbanwa"},
	{L"Limb",	L"Limbu"},
	{L"Tale",	L"Tai_Le"},
	{L"Linb",	L"Linear_B"},
	{L"Ugar",	L"Ugaritic"},
	{L"Shaw",	L"Shavian"},
	{L"Osma",	L"Osmanya"},
	{L"Cprt",	L"Cypriot"},
	{L"Brai",	L"Braille"},
	{L"Bugi",	L"Buginese"},
	{L"Copt",	L"Coptic"},
	{L"Talu",	L"New_Tai_Lue"},
	{L"Glag",	L"Glagolitic"},
	{L"Tfng",	L"Tifinagh"},
	{L"Sylo",	L"Syloti_Nagri"},
	{L"Xpeo",	L"Old_Persian"},
	{L"Khar",	L"Kharoshthi"},
	{L"Bali",	L"Balinese"},
	{L"Xsux",	L"Cuneiform"},
	{L"Phnx",	L"Phoenician"},
	{L"Phag",	L"Phags_Pa"},
	{L"Nkoo",	L"NKO"},
	{L"Hrkt",	L"Katakana_Or_Hiragana"},
};


// HangulSyllableType ///////////////////////////////////////////////////////

/// The long name of the property.
const Char HangulSyllableType::LONG_NAME[] = L"Hangul_Syllable_Type";
/// The short name of the property.
const Char HangulSyllableType::SHORT_NAME[] = L"hst";
const HangulSyllableType::Names HangulSyllableType::names_[HangulSyllableType::LAST_VALUE - HangulSyllableType::FIRST_VALUE] = {
	{L"NA",		L"Not_Applicable"},
	{L"L",		L"Leading_Jamo"},
	{L"V",		L"Vowel_Jamo"},
	{L"T",		L"Trailing_Jamo"},
	{L"LV",		L"LV_Syllable"},
	{L"LVT",	L"LVT_Syllable"}
};


// BinaryProperty ///////////////////////////////////////////////////////////

#include "generated/uprops-table"

const BinaryProperty::Names BinaryProperty::names_[BinaryProperty::LAST_VALUE - BinaryProperty::FIRST_VALUE] = {
	{L"Alpha",		L"Alphabetic"},
	{L"AHex",		L"ASCII_Hex_Digit"},
	{L"Bidi_C",		L"Bidi_Control"},
	{L"Bidi_M",		L"Bidi_Mirrored"},
	{L"CE",			L"Composition_Exclusion"},
	{0,				L"Dash"},
	{L"DI",			L"Default_Ignorable_Code_Point"},
	{L"Dep",		L"Deprecated"},
	{L"Dia",		L"Diacritic"},
	{L"XO_NFC",		L"Expands_On_NFC"},
	{L"XO_NFD",		L"Expands_On_NFD"},
	{L"XO_NFKC",	L"Expands_On_NFKC"},
	{L"XO_NFKD",	L"Expands_On_NFKD"},
	{L"Ext",		L"Extender"},
	{L"Comp_Ex",	L"Full_Composition_Exclusion"},
	{L"Gr_Base",	L"Grapheme_Base"},
	{L"Gr_Ext",		L"Grapheme_Extend"},
	{L"Hex",		L"Hex_Digit"},
	{0,				L"Hyphen"},
	{L"IDC",		L"ID_Continue"},
	{L"IDS",		L"ID_Start"},
	{L"Ideo",		L"Ideographic"},
	{L"IDSB",		L"IDS_Binary_Operator"},
	{L"IDST",		L"IDS_Trinary_Operator"},
	{L"Join_C",		L"Join_Control"},
	{L"LOE",		L"Logical_Order_Exception"},
	{L"Lower",		L"Lowercase"},
	{0,				L"Math"},
	{L"NChar",		L"Noncharacter_Code_Point"},
	{L"OAlpha",		L"Other_Alphabetic"},
	{L"ODI",		L"Other_Default_Ignorable_Code_Point"},
	{L"OGr_Ext",	L"Other_Grapheme_Extend"},
	{L"OIDC",		L"Other_ID_Continue"},
	{L"OIDS",		L"Other_ID_Start"},
	{L"OLower",		L"Other_Lowercase"},
	{L"OMath",		L"Other_Math"},
	{L"OUpper",		L"Other_Uppercase"},
	{L"Pat_Syn",	L"Pattern_Syntax"},
	{L"Pat_WS",		L"Pattern_White_Space"},
	{L"QMark",		L"Quotation_Mark"},
	{0,				L"Radical"},
	{L"SD",			L"Soft_Dotted"},
	{0,				L"STerm"},
	{L"Term",		L"Terminal_Punctuation"},
	{L"UIdeo",		L"Unified_Ideograph"},
	{L"Upper",		L"Uppercase"},
	{L"VS",			L"Variation_Selector"},
	{L"WSpace",		L"White_Space"},
	{L"XIDC",		L"XID_Continue"},
	{L"XIDS",		L"XID_Start"}
};

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


// EastAsianWidth ///////////////////////////////////////////////////////////

/// The long name of the property.
const Char EastAsianWidth::LONG_NAME[] = L"East_Asian_Width";
/// The short name of the property.
const Char EastAsianWidth::SHORT_NAME[] = L"ea";
const EastAsianWidth::Names EastAsianWidth::names_[EastAsianWidth::LAST_VALUE - EastAsianWidth::FIRST_VALUE] = {
	{L"F",	L"Fullwidth"},
	{L"H",	L"Halfwidth"},
	{L"W",	L"Wide"},
	{L"Na",	L"Narrow"},
	{L"A",	L"Ambiguous"},
	{L"N",	L"Neutral"}
};


// LineBreak ////////////////////////////////////////////////////////////////

/// The long name of the property.
const Char LineBreak::LONG_NAME[] = L"Line_Break";
/// The short name of the property.
const Char LineBreak::SHORT_NAME[] = L"lb";
const LineBreak::Names LineBreak::names_[LineBreak::LAST_VALUE - LineBreak::FIRST_VALUE] = {
	{L"BK",	L"Mandatory_Break"},
	{L"CR",	L"Carriage_Return"},
	{L"LF",	L"Line_Feed"},
	{L"CM",	L"Combining_Mark"},
	{L"NL",	L"Next_Line"},
	{L"SG",	L"Surrogate"},
	{L"WJ",	L"Word_Joiner"},
	{L"ZW",	L"ZWSpace"},
	{L"GL",	L"Glue"},
	{L"SP",	L"Space"},
	{L"B2",	L"Break_Both"},
	{L"BA",	L"Break_After"},
	{L"BB",	L"Break_Before"},
	{L"HY",	L"Hyphen"},
	{L"CB",	L"Contingent_Break"},
	{L"CL",	L"Close_Punctuation"},
	{L"EX",	L"Exclamation"},
	{L"IN",	L"Inseparable"},
	{L"NS",	L"Nonstarter"},
	{L"OP",	L"Open_Punctuation"},
	{L"QU",	L"Quotation"},
	{L"IS",	L"Infix_Numeric"},
	{L"NU",	L"Numeric"},
	{L"PO",	L"Postfix_Numeric"},
	{L"PR",	L"Prefix_Numeric"},
	{L"SY",	L"Break_Symbols"},
	{L"AI",	L"Ambiguous"},
	{L"AL",	L"Alphabetic"},
	{0,	L"H2"},
	{0,	L"H3"},
	{L"ID",	L"Ideographic"},
	{0,	L"JL"},
	{0,	L"JV"},
	{0,	L"JT"},
	{L"SA",	L"Complex_Context"},
	{L"XX",	L"Unknown"}
};


// GraphemeClusterBreak /////////////////////////////////////////////////////

/// The long name of the property.
const Char GraphemeClusterBreak::LONG_NAME[] = L"Grapheme_Cluster_Break";
/// The short name of the property.
const Char GraphemeClusterBreak::SHORT_NAME[] = L"GCB";
const GraphemeClusterBreak::Names GraphemeClusterBreak::names_[GraphemeClusterBreak::LAST_VALUE - GraphemeClusterBreak::FIRST_VALUE] = {
	{0,		L"CR"},
	{0,		L"LF"},
	{L"CN",	L"Control"},
	{L"EX",	L"Extend"},
	{0,		L"L"},
	{0,		L"V"},
	{0,		L"T"},
	{0,		L"LV"},
	{0,		L"LVT"},
	{L"XX",	L"Other"}
};

/// Returns Grapheme_Cluster_Break value of the specified character.
int GraphemeClusterBreak::of(CodePoint cp) /*throw()*/ {
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


// WordBreak ////////////////////////////////////////////////////////////////

/// The long name of the property.
const Char WordBreak::LONG_NAME[] = L"Word_Break";
/// The short name of the property.
const Char WordBreak::SHORT_NAME[] = L"WB";
const WordBreak::Names WordBreak::names_[WordBreak::LAST_VALUE - WordBreak::FIRST_VALUE] = {
	{L"FO",	L"Format"},
	{L"KA",	L"Katakana"},
	{L"LE",	L"ALetter"},
	{L"ML",	L"MidLetter"},
	{L"MN",	L"MidNum"},
	{L"NU",	L"Numeric"},
	{L"EX",	L"ExtendNumLet"},
	{L"XX",	L"Other"}
};

/**
 * Returns Word_Break value of the specified character.
 * @param cp the character
 * @param syntax the identifier syntax definition for deciding what character is ID_Start
 * @param lc the locale
 */
int WordBreak::of(CodePoint cp,
		const IdentifierSyntax& syntax /* = IdentifierSyntax(IdentifierSyntax::UNICODE_DEFAULT) */,
		const locale& lc /* = locale::classic() */) /*throw()*/ {
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
			|| binary_search(KATAKANAS, MANAH_ENDOF(KATAKANAS), cp))
		return KATAKANA;
	else if(BinaryProperty::is<BinaryProperty::GRAPHEME_EXTEND>(cp))
		return GraphemeClusterBreak::EXTEND;
	else if((syntax.isIdentifierStartCharacter(cp)
			|| cp == 0x00A0		// No-Break Space
			|| cp == 0x05F3))	// Hebrew Punctuation Geresh
		return A_LETTER;
	else if(binary_search(MID_LETTERS, MANAH_ENDOF(MID_LETTERS), cp)
			|| (cp == 0x003A && swedish.get() != 0 && lc == *swedish.get()))	// Colon (for Swedish)
		return MID_LETTER;
	else if(binary_search(MID_NUMS, MANAH_ENDOF(MID_NUMS), cp))
		return MID_NUM;
	else if(isNU(cp, gc))
		return NUMERIC;
	else if(gc == GeneralCategory::PUNCTUATION_CONNECTOR)
		return EXTEND_NUM_LET;
	else
		return OTHER;
}


// SentenceBreak ////////////////////////////////////////////////////////////

/// The long name of the property.
const Char SentenceBreak::LONG_NAME[] = L"Sentence_Break";
/// The short name of the property.
const Char SentenceBreak::SHORT_NAME[] = L"SB";
const SentenceBreak::Names SentenceBreak::names_[SentenceBreak::LAST_VALUE - SentenceBreak::FIRST_VALUE] = {
	{L"SE",	L"Sep"},
	{L"FO",	L"Format"},
	{0,		L"SP"},
	{L"LO",	L"Lower"},
	{L"UP",	L"Upper"},
	{L"LE",	L"OLetter"},
	{L"NU",	L"Numeric"},
	{L"AT",	L"ATerm"},
	{L"ST",	L"STerm"},
	{L"CL",	L"Close"},
	{L"XX",	L"Other"}
};

/// Returns Sentence_Break value of the specified character.
int SentenceBreak::of(CodePoint cp) /*throw()*/ {
	static const CodePoint SEPS[] = {LINE_FEED, CARRIAGE_RETURN, NEXT_LINE, LINE_SEPARATOR, PARAGRAPH_SEPARATOR};
	if(BinaryProperty::is<BinaryProperty::GRAPHEME_EXTEND>(cp))
		return GraphemeClusterBreak::EXTEND;
	else if(binary_search(SEPS, MANAH_ENDOF(SEPS), cp))
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

#undef PROP
