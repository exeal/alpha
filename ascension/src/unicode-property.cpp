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
		return (gc == GeneralCategory::DECIMAL_NUMBER && cp < 0xFF00 || cp > 0xFFEF)
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
		return gc == GeneralCategory::FINAL_PUNCTUATION
			|| gc == GeneralCategory::INITIAL_PUNCTUATION
			|| binary_search(QU, MANAH_ENDOF(QU), cp);
	}
} // namespace @0

#include "generated/uprops-value-names"


// GeneralCategory //////////////////////////////////////////////////////////

/// The default value of the property.
const int GeneralCategory::DEFAULT_VALUE = GeneralCategory::UNASSIGNED;
/// The long name of the property.
const Char GeneralCategory::LONG_NAME[] = L"General_Category";
/// The short name of the property.
const Char GeneralCategory::SHORT_NAME[] = L"gc";


// Block ////////////////////////////////////////////////////////////////////

/// The default value of the property.
const int Block::DEFAULT_VALUE = Block::NO_BLOCK;
/// The long name of the property.
const Char Block::LONG_NAME[] = L"Block";
/// The short name of the property.
const Char Block::SHORT_NAME[] = L"blk";


// CanonicalCombiningClass //////////////////////////////////////////////////

/// The default of the property.
const int CanonicalCombiningClass::DEFAULT_VALUE = CanonicalCombiningClass::NOT_REORDERED;
/// The long name of the property.
const Char CanonicalCombiningClass::LONG_NAME[] = L"Canonical_Combining_Class";
/// The shoer name of the property.
const Char CanonicalCombiningClass::SHORT_NAME[] = L"ccc";


// Script ///////////////////////////////////////////////////////////////////

/// The default value of the property.
const int Script::DEFAULT_VALUE = Script::UNKNOWN;
/// The long name of the property.
const Char Script::LONG_NAME[] = L"Script";
/// The short name of the property.
const Char Script::SHORT_NAME[] = L"sc";


// HangulSyllableType ///////////////////////////////////////////////////////

/// The default value of the property.
const int HangulSyllableType::DEFAULT_VALUE = HangulSyllableType::NOT_APPLICABLE;
/// The long name of the property.
const Char HangulSyllableType::LONG_NAME[] = L"Hangul_Syllable_Type";
/// The short name of the property.
const Char HangulSyllableType::SHORT_NAME[] = L"hst";


// BinaryProperty ///////////////////////////////////////////////////////////

#include "generated/uprops-code-table"


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

/// The default value of the property.
const int EastAsianWidth::DEFAULT_VALUE = EastAsianWidth::NEUTRAL;
/// The long name of the property.
const Char EastAsianWidth::LONG_NAME[] = L"East_Asian_Width";
/// The short name of the property.
const Char EastAsianWidth::SHORT_NAME[] = L"ea";


// LineBreak ////////////////////////////////////////////////////////////////

/// The default value of the property.
const int LineBreak::DEFAULT_VALUE = LineBreak::UNKNOWN;
/// The long name of the property.
const Char LineBreak::LONG_NAME[] = L"Line_Break";
/// The short name of the property.
const Char LineBreak::SHORT_NAME[] = L"lb";


// GraphemeClusterBreak /////////////////////////////////////////////////////

/// The default value of the property.
const int GraphemeClusterBreak::DEFAULT_VALUE = GraphemeClusterBreak::OTHER;
/// The long name of the property.
const Char GraphemeClusterBreak::LONG_NAME[] = L"Grapheme_Cluster_Break";
/// The short name of the property.
const Char GraphemeClusterBreak::SHORT_NAME[] = L"GCB";

/// Returns Grapheme_Cluster_Break value of the specified character.
int GraphemeClusterBreak::of(CodePoint cp) /*throw()*/ {
	if(cp == CARRIAGE_RETURN)
		return CR;
	else if(cp == LINE_FEED)
		return LF;
	const int gc = GeneralCategory::of(cp);
	if((gc == GeneralCategory::LINE_SEPARATOR
			|| gc == GeneralCategory::PARAGRAPH_SEPARATOR
			|| gc == GeneralCategory::CONTROL
			|| gc == GeneralCategory::FORMAT)
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

/// The default value of the property.
const int WordBreak::DEFAULT_VALUE = GraphemeClusterBreak::OTHER;
/// The long name of the property.
const Char WordBreak::LONG_NAME[] = L"Word_Break";
/// The short name of the property.
const Char WordBreak::SHORT_NAME[] = L"WB";

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
	if(gc == GeneralCategory::FORMAT && cp != ZERO_WIDTH_NON_JOINER && cp != ZERO_WIDTH_JOINER)
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
	else if(gc == GeneralCategory::CONNECTOR_PUNCTUATION)
		return EXTEND_NUM_LET;
	else
		return OTHER;
}


// SentenceBreak ////////////////////////////////////////////////////////////

/// The default value of the property.
const int SentenceBreak::DEFAULT_VALUE = GraphemeClusterBreak::OTHER;
/// The long name of the property.
const Char SentenceBreak::LONG_NAME[] = L"Sentence_Break";
/// The short name of the property.
const Char SentenceBreak::SHORT_NAME[] = L"SB";

/// Returns Sentence_Break value of the specified character.
int SentenceBreak::of(CodePoint cp) /*throw()*/ {
	static const CodePoint SEPS[] = {LINE_FEED, CARRIAGE_RETURN, NEXT_LINE, LINE_SEPARATOR, PARAGRAPH_SEPARATOR};
	if(BinaryProperty::is<BinaryProperty::GRAPHEME_EXTEND>(cp))
		return GraphemeClusterBreak::EXTEND;
	else if(binary_search(SEPS, MANAH_ENDOF(SEPS), cp))
		return SEP;
	const int gc = GeneralCategory::of(cp);
	if(gc == GeneralCategory::FORMAT && cp != ZERO_WIDTH_NON_JOINER && cp != ZERO_WIDTH_JOINER)
		return FORMAT;
	else if(BinaryProperty::is<BinaryProperty::WHITE_SPACE>(cp) && cp != 0x00A0)
		return SP;
	else if(BinaryProperty::is<BinaryProperty::LOWERCASE>(cp))
		return LOWER;
	else if(gc == GeneralCategory::TITLECASE_LETTER || BinaryProperty::is<BinaryProperty::UPPERCASE>(cp))
		return UPPER;
	else if(BinaryProperty::is<BinaryProperty::ALPHABETIC>(cp) || cp == 0x00A0 || cp == 0x05F3)
		return O_LETTER;
	else if(isNU(cp, gc))
		return NUMERIC;
	else if(cp == 0x002E)
		return A_TERM;
	else if(BinaryProperty::is<BinaryProperty::STERM>(cp))
		return S_TERM;
	else if(gc == GeneralCategory::OPEN_PUNCTUATION || gc == GeneralCategory::CLOSE_PUNCTUATION || isQU(cp, gc))
		return CLOSE;
	return OTHER;
}
