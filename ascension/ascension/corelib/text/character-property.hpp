/**
 * @file character-property.hpp
 * Defines Unicode property entries and provides methods to retrieve a property of character.
 * @author exeal
 * @date 2007-2011 was unicode-property.hpp
 * @date 2011-05-07 renamed from unicode-property.hpp
 */

#ifndef ASCENSION_CHARACTER_PROPERTY_HPP
#define ASCENSION_CHARACTER_PROPERTY_HPP
#if ASCENSION_UNICODE_VERSION > 0x0510
#error These class definitions and implementations are based on old version of Unicode.
#endif
#include <ascension/corelib/text/identifier-syntax.hpp>
#include <algorithm>	// std.binary_search, std.lower_bound, std.upper_bound
#include <locale>		// std.locale, std.tolower
#include <string>		// std.char_traits

namespace ascension {
	namespace text {
		/**
		 * Implements <a href="http://www.unicode.org/Public/UNIDATA/UCD.html">UCD (Unicode
		 * Character Database)</a>.
		 */
		namespace ucd {
			/**
			 * A function object compares Unicode property (value) names based on "Property and
			 * Property Value Matching"
			 * (http://www.unicode.org/Public/UNIDATA/UCD.html#Property_and_Property_Value_Matching).
			 */
			struct PropertyNameComparer {
				/**
				 * Compares the given two strings.
				 * @tparam CharType1 The type of @a p1
				 * @tparam CharType2 The type of @a p2
				 * @param p1 One property name
				 * @param p2 The other property name
				 * @return true if p1 &lt; p2
				 */
				template<typename CharType1, typename CharType2>
				bool operator()(const CharType1* p1, const CharType2* p2) const {
					return compare(p1, p2) < 0;
				}
				/**
				 * Compares the given two strings.
				 * @tparam CharType1 The type of @a p1
				 * @tparam CharType2 The type of @a p2
				 * @param p1 One property name
				 * @param p2 The other property name
				 * @return &lt; 0 if @a p1 &lt; @a p2
				 * @return 0 if @a p1 == @a p2
				 * @return &gt; 0 if @a p1 &gt; @a p2
				 */
				template<typename CharType1, typename CharType2>
				static int compare(const CharType1* p1, const CharType2* p2) {
					while(*p1 != 0 && *p2 != 0) {
						if(*p1 == '_' || *p1 == '-' || *p1 == ' ') {
							++p1;
							continue;
						} else if(*p2 == '_' || *p2 == '-' || *p2 == ' ') {
							++p2;
							continue;
						}
						const int c1 =
							std::char_traits<CharType1>::to_int_type(
								std::tolower(*p1, std::locale::classic()));
						const int c2 =
							std::char_traits<CharType2>::to_int_type(
								std::tolower(*p2, std::locale::classic()));
						if(c1 != c2)
							return c1 - c2;
						else
							++p1, ++p2;
					}
					return *p1 - *p2;
				}
			};

			/// An invalid property value.
			const int NOT_PROPERTY = 0;

#include "src/generated/uprops-data-types"

			/**
			 * General categories.
			 * These values are based on Unicode standard 5.0.0 "4.5 General Category".
			 */
			class GeneralCategory {
			public:
				enum {
					FIRST_VALUE = NOT_PROPERTY + 1,
					// sub-categories
					UPPERCASE_LETTER = FIRST_VALUE,	///< Letter, uppercase (Lu = Uppercase_Letter)
					LOWERCASE_LETTER,				///< Letter, lowercase (Ll = Lowercase_Letter)
					TITLECASE_LETTER,				///< Letter, titlecase (Lt = Titlecase_Letter)
					MODIFIER_LETTER,				///< Letter, modifier (Lm = Modifier_Letter)
					OTHER_LETTER,					///< Letter, other (Lo = Other_Letter)
					NONSPACING_MARK,				///< Mark, nonspacing (Mn = Nonspacing_Mark)
					SPACING_MARK,					///< Mark, spacing combining (Mc = Spacing_Mark)
					ENCLOSING_MARK,					///< Mark, enclosing (Me = Enclosing_Mark)
					DECIMAL_NUMBER,					///< Number, decimal digit (Nd = Decimal_Number)
					LETTER_NUMBER,					///< Number, letter (Nl = Letter_Number)
					OTHER_NUMBER,					///< Number, other (No = Other_Number)
					CONNECTOR_PUNCTUATION,			///< Punctuation, connector (Pc = Connector_Punctuation)
					DASH_PUNCTUATION,				///< Punctuation, dash (Pd = Dash_Punctuation)
					OPEN_PUNCTUATION,				///< Punctuation, open (Ps = Open_Punctuation)
					CLOSE_PUNCTUATION,				///< Punctuation, close (Pe = Close_Punctuation)
					INITIAL_PUNCTUATION,			///< Punctuation, initial quote (Pi = Initial_Punctuation)
					FINAL_PUNCTUATION,				///< Punctuation, final quote (Pf = Final_Punctuation)
					OTHER_PUNCTUATION,				///< Punctuation, other (Po = Other_Punctuation)
					MATH_SYMBOL,					///< Symbol, math (Sm = Math_Symbol)
					CURRENCY_SYMBOL,				///< Symbol, currency (Sc = Currency_Symbol)
					MODIFIER_SYMBOL,				///< Symbol, modifier (Sk = Modifier_Symbol)
					OTHER_SYMBOL,					///< Symbol, other (So = Other_Symbol)
					SPACE_SEPARATOR,				///< Separator, space (Zs = Space_Separator)
					LINE_SEPARATOR,					///< Separator, line (Zl = Line_Separator)
					PARAGRAPH_SEPARATOR,			///< Separator, paragraph (Zp = Paragraph_Separator)
					CONTROL,						///< Other, control (Cc = Control)
					FORMAT,							///< Other, format (Cf = Format)
					SURROGATE,						///< Other, surrogate (Cs = Surrogate)
					PRIVATE_USE,					///< Other, private use (Co = Private_Use)
					UNASSIGNED,						///< Other, not assigned (Cn = Unassigned)
					// super-categories
					LETTER,			///< L (L&amp;) = Letter
					CASED_LETTER,	///< Letter, cased (LC = Cased_Letter)
					MARK,			///< M (M&amp;) = Mark
					NUMBER,			///< N (N&amp;) = Number
					PUNCTUATION,	///< P (P&amp;) = Punctuation
					SYMBOL,			///< S (S&amp;) = Symbol
					SEPARATOR,		///< Z (Z&amp;) = Separator
					OTHER,			///< C (C&amp;) = Other
					LAST_VALUE
				};
				static const int DEFAULT_VALUE;
				static const char LONG_NAME[], SHORT_NAME[];
				template<typename CharType> static int forName(const CharType* name);
				template<int superCategory> static bool is(int subCategory);
				static int of(CodePoint c) BOOST_NOEXCEPT;
			private:
				static const detail::CharacterPropertyPartition VALUES_[];
				static const std::size_t NUMBER_;
				static const detail::CharacterPropertyValueName NAMES_[];
			};
		
			/// Code blocks. These values are based on Blocks.txt obtained from UCD.
			/// The order of the definition of the enumeration is based on their code values.
			class Block {
			public:
				enum {
					FIRST_VALUE = GeneralCategory::LAST_VALUE,
					NO_BLOCK = FIRST_VALUE,
#include "src/generated/uprops-blocks-definition"
					LAST_VALUE
				};
				static const int DEFAULT_VALUE;
				static const char LONG_NAME[], SHORT_NAME[];				
				template<typename CharType> static int forName(const CharType* name);
				static int of(CodePoint c) BOOST_NOEXCEPT;
			private:
				static const detail::CharacterPropertyPartition VALUES_[];
				static const std::size_t NUMBER_;
				static const detail::CharacterPropertyValueName NAMES_[];
			};

			/**
			 * Canonical combining classes. These are based on Unicode standard 5.0.0 "4.3
			 * Combining Classes".
#ifndef ASCENSION_NO_UNICODE_NORMALIZATION
			 * @see Normalizer
#endif // !ASCENSION_NO_UNICODE_NORMALIZATION
			 */
			class CanonicalCombiningClass {
			public:
				enum {
					NOT_REORDERED			= 0,	///< Spacing, split, enclosing, reordrant, and Tibetan subjoined (0).
					OVERLAY					= 1,	///< Overlays and interior (1).
					NUKTA					= 7,	///< Nuktas (7).
					KANA_VOICING			= 8,	///< Hiragana/Katakana voicing marks (8).
					VIRAMA					= 9,	///< Viramas (9).
					ATTACHED_BELOW_LEFT		= 200,	///< Below left attached (200).
					ATTACHED_BELOW			= 202,	///< Below attached (202).
					ATTACHED_BELOW_RIGHT	= 204,	///< Below right attached (204). This class does not currently have members.
					ATTACHED_LEFT			= 208,	///< Left attached (208). This class does not currently have members.
					ATTACHED_RIGHT			= 210,	///< Right attached (210). This class does not currently have members.
					ATTACHED_ABOVE_LEFT		= 212,	///< Above left attached (212). This class does not currently have members.
					ATTACHED_ABOVE			= 214,	///< Above attached (214). This class does not currently have members.
					ATTACHED_ABOVE_RIGHT	= 216,	///< Above right attached (216).
					BELOW_LEFT				= 218,	///< Below left (218).
					BELOW					= 220,	///< Below (220)
					BELOW_RIGHT				= 222,	///< Below right (222).
					LEFT					= 224,	///< Left (224).
					RIGHT					= 226,	///< Right (226).
					ABOVE_LEFT				= 228,	///< Above left (228).
					ABOVE					= 230,	///< Above (230).
					ABOVE_RIGHT				= 232,	///< Above right (232).
					DOUBLE_BELOW			= 233,	///< Double below (233).
					DOUBLE_ABOVE			= 234,	///< Double above (234).
					IOTA_SUBSCRIPT			= 240	///< Below (iota subscript) (240).
				};
				static const int DEFAULT_VALUE;
				static const char LONG_NAME[], SHORT_NAME[];
				template<typename CharType> static int forName(const CharType* name);
				static int of(CodePoint cp) BOOST_NOEXCEPT;
			private:
				static const CodePoint CHARACTERS_[];
				static const std::uint8_t VALUES_[];
				static const std::size_t NUMBER_;
				static const detail::CharacterPropertyValueName NAMES_[];
			};

			/**
			 * Scripts. These are based on
			 * <a href="http://www.unicode.org/reports/tr24/">UAX #24: Script Names</a> revision 9
			 * and Scripts.txt obtained from UCD.
			 */
			class Script {
			public:
				enum {
					FIRST_VALUE = Block::LAST_VALUE, UNKNOWN = FIRST_VALUE, COMMON,
					// Unicode 4.0
					LATIN, GREEK, CYRILLIC, ARMENIAN, HEBREW, ARABIC, SYRIAC, THAANA,
					DEVANAGARI, BENGALI, GURMUKHI, GUJARATI, ORIYA, TAMIL, TELUGU, KANNADA,
					MALAYALAM, SINHALA, THAI, LAO, TIBETAN, MYANMAR, GEORGIAN, HANGUL,
					ETHIOPIC, CHEROKEE, CANADIAN_ABORIGINAL, OGHAM, RUNIC, KHMER, MONGOLIAN,
					HIRAGANA, KATAKANA, BOPOMOFO, HAN, YI, OLD_ITALIC, GOTHIC, DESERET,
					INHERITED, TAGALOG, HANUNOO, BUHID, TAGBANWA, LIMBU, TAI_LE,
					LINEAR_B, UGARITIC, SHAVIAN, OSMANYA, CYPRIOT, BRAILLE,
					// Unicode 4.1
					BUGINESE, COPTIC, NEW_TAI_LUE, GLAGOLITIC, TIFINAGH, SYLOTI_NAGRI,
					OLD_PERSIAN, KHAROSHTHI,
					// Unicode 5.0
					BALINESE, CUNEIFORM, PHOENICIAN, PHAGS_PA, NKO,
					// Unicode 5.1
					SUNDANESE, LEPCHA, OL_CHIKI, VAI, SAURASHTRA, KAYAH_LI, REJANG, LYCIAN,
					CARIAN, LYDIAN, CHAM,
					// derived
					KATAKANA_OR_HIRAGANA,
					LAST_VALUE
				};
				static const int DEFAULT_VALUE;
				static const char LONG_NAME[], SHORT_NAME[];
				template<typename CharType> static int forName(const CharType* name);
				static int of(CodePoint c) BOOST_NOEXCEPT;
			private:
				static const detail::CharacterPropertyPartition VALUES_[];
				static const std::size_t NUMBER_;
				static const detail::CharacterPropertyValueName NAMES_[];
			};

			/**
			 * Hangul syllable types. These values are based on HangulSyllableType.txt obtained
			 * from UCD.
			 */
			class HangulSyllableType {
			public:
				enum {
					FIRST_VALUE = Script::LAST_VALUE,
					NOT_APPLICABLE = FIRST_VALUE,	///< NA = Not_Applicable
					LEADING_JAMO,					///< L = Leading_Jamo
					VOWEL_JAMO,						///< V = Vowel_Jamo
					TRAILING_JAMO,					///< T = Trailing_Jamo
					LV_SYLLABLE,					///< LV = LV_Syllable
					LVT_SYLLABLE,					///< LVT = LVT_Syllable
					LAST_VALUE
				};
				static const int DEFAULT_VALUE;
				static const char LONG_NAME[], SHORT_NAME[];
				template<typename CharType> static int forName(const CharType* name);
				static int of(CodePoint cp) BOOST_NOEXCEPT;
			private:
				static const detail::CharacterPropertyValueName NAMES_[];
			};
			
			/**
			 * Binary properties These values are based on UCD.html and PropList.txt obtained from
			 * UCD.
			 * @note Some values are not implemented.
			 */
			class BinaryProperty {
			public:
				enum {
					FIRST_VALUE = HangulSyllableType::LAST_VALUE,
					ALPHABETIC = FIRST_VALUE, ASCII_HEX_DIGIT, BIDI_CONTROL, BIDI_MIRRORED,
					COMPOSITION_EXCLUSION, DASH, DEFAULT_IGNORABLE_CODE_POINT, DEPRECATED, DIACRITIC,
					EXPANDS_ON_NFC, EXPANDS_ON_NFD, EXPANDS_ON_NFKC, EXPANDS_ON_NFKD, EXTENDER,
					FULL_COMPOSITION_EXCLUSION, GRAPHEME_BASE, GRAPHEME_EXTEND, HEX_DIGIT, HYPHEN,
					ID_CONTINUE, ID_START, IDEOGRAPHIC, IDS_BINARY_OPERATOR, IDS_TRINARY_OPERATOR,
					JOIN_CONTROL, LOGICAL_ORDER_EXCEPTION, LOWERCASE, MATH, NONCHARACTER_CODE_POINT,
					OTHER_ALPHABETIC, OTHER_DEFAULT_IGNORABLE_CODE_POINT, OTHER_GRAPHEME_EXTEND,
					OTHER_ID_CONTINUE, OTHER_ID_START, OTHER_LOWERCASE, OTHER_MATH, OTHER_UPPERCASE,
					PATTERN_SYNTAX, PATTERN_WHITE_SPACE, QUOTATION_MARK, RADICAL, SOFT_DOTTED, STERM,
					TERMINAL_PUNCTUATION, UNIFIED_IDEOGRAPH, UPPERCASE, VARIATION_SELECTOR, WHITE_SPACE,
					XID_CONTINUE, XID_START, LAST_VALUE
				};
				template<typename CharType> static int forName(const CharType* name);
				static bool is(CodePoint cp, int property);
				template<int property>
				static bool is(CodePoint cp) BOOST_NOEXCEPT;
			private:
				static const detail::CharacterPropertyValueName NAMES_[];
#include "src/generated/uprops-binary-property-values-definition"
			};

			/// East_Asian_Width property. These values are based on UAX #11.
			class EastAsianWidth {
			public:
				enum {
					FIRST_VALUE = BinaryProperty::LAST_VALUE,
					FULLWIDTH = FIRST_VALUE,	///< Fullwidth (F).
					HALFWIDTH,					///< Halfwidth (H).
					WIDE,						///< Wide (W).
					NARROW,						///< Narrow (Na).
					AMBIGUOUS,					///< Ambiguous (A).
					NEUTRAL,					///< Neutral (Not East Asian) (N).
					LAST_VALUE
				};
				static const int DEFAULT_VALUE;
				static const char LONG_NAME[], SHORT_NAME[];
				template<typename CharType> static int forName(const CharType* name);
				static int of(CodePoint c) BOOST_NOEXCEPT;
			private:
				static const detail::CharacterPropertyPartition VALUES_[];
				static const std::size_t NUMBER_;
				static const detail::CharacterPropertyValueName NAMES_[];
			};

			/**
			 * Line_Break property. These values are based on UAX #14.
			 * @see AbstractLineBreakIterator, LineBreakIterator
			 */
			class LineBreak {
			public:
				// these identifier are based on PropertyValueAliases.txt. there are some variations
				enum {
					FIRST_VALUE = EastAsianWidth::LAST_VALUE,
					// non-tailorable line breaking classes
					MANDATORY_BREAK = FIRST_VALUE,	///< Mandatory Break (BK).
					CARRIAGE_RETURN,				///< Carriage Return (CR).
					LINE_FEED,						///< Line Feed (LF).
					COMBINING_MARK,					///< Attached Characters and Combining Marks (CM).
					NEXT_LINE,						///< Next Line (NL).
					SURROGATE,						///< Surrogates (SG).
					WORD_JOINER,					///< Word Joiner (WJ).
					ZWSPACE,						///< Zero Width Space (ZW).
					GLUE,							///< Non-breaking ("Glue") (GL).
					SPACE,							///< Space (SP).
					// break opportunities
					BREAK_BOTH,						///< Break Opportunity Before and After (B2).
					BREAK_AFTER,					///< Break Opportunity After (BA).
					BREAK_BEFORE,					///< Break Opportunity Before (BB).
					HYPHEN,							///< Hyphen (HY).
					CONTINGENT_BREAK,				///< Contigent Break Opportunity (CB).
					// characters prohibiting certain breaks
					CLOSE_PUNCTUATION,				///< Closing Punctuation (CL).
					EXCLAMATION,					///< Exclamation/Interrogation (EX).
					INSEPARABLE,					///< Inseparable (IN).
					NONSTARTER,						///< Nonstarter (NS).
					OPEN_PUNCTUATION,				///< Opening Punctuation (OP).
					QUOTATION,						///< Ambiguous Quotation (QU).
					// numeric context
					INFIX_NUMERIC,					///< Infix Separator (Numeric) (IS).
					NUMERIC,						///< Numeric (NU).
					POSTFIX_NUMERIC,				///< Postfix (Numeric) (PO).
					PREFIX_NUMERIC,					///< Prefix (Numeric) (PR).
					BREAK_SYMBOLS,					///< Symbols Allowing Break After (SY).
					// other characters
					AMBIGUOUS,						///< Ambiguous (Alphabetic or Ideographic) (AI).
					ALPHABETIC,						///< Ordinary Alphabetic and Symbol Characters (AL).
					H2,								///< Hangul LV Syllable (H2).
					H3,								///< Hangul LVT Syllable (H3).
					IDEOGRAPHIC,					///< Ideographic (ID).
					JL,								///< Hangul L Jamo (JL).
					JV,								///< Hangul V Jamo (JV).
					JT,								///< Hangul T Jamo (JT).
					COMPLEX_CONTEXT,				///< Complex Context Dependent (South East Asian) (SA).
					UNKNOWN,						///< Unknown (XX).
					LAST_VALUE
				};
				static const int DEFAULT_VALUE;
				static const char LONG_NAME[], SHORT_NAME[];
				template<typename CharType> static int forName(const CharType* name);
				static int of(CodePoint c) BOOST_NOEXCEPT;
			private:
				static const detail::CharacterPropertyPartition VALUES_[];
				static const std::size_t NUMBER_;
				static const detail::CharacterPropertyValueName NAMES_[];
			};

#include "src/generated/uprops-inlines"

			/// Grapheme_Cluster_Break property. These values are based on UAX #29.
			class GraphemeClusterBreak {
			public:
				enum {
					FIRST_VALUE = LineBreak::LAST_VALUE,
					CR = FIRST_VALUE, LF, CONTROL, EXTEND, L, V, T, LV, LVT, OTHER,
					LAST_VALUE
				};
				static const int DEFAULT_VALUE;
				static const char LONG_NAME[], SHORT_NAME[];
				static int of(CodePoint cp) BOOST_NOEXCEPT;
			private:
				static const detail::CharacterPropertyValueName NAMES_[];
			};

			/// Word_Break property. These values are based on UAX #29.
			class WordBreak {
			public:
				enum {
					FIRST_VALUE = GraphemeClusterBreak::LAST_VALUE,
					FORMAT = FIRST_VALUE, KATAKANA, A_LETTER, MID_LETTER, MID_NUM, NUMERIC, EXTEND_NUM_LET, OTHER,
					LAST_VALUE
				};
				static const int DEFAULT_VALUE;
				static const char LONG_NAME[], SHORT_NAME[];
				static int of(CodePoint cp,
					const IdentifierSyntax& syntax = IdentifierSyntax(IdentifierSyntax::UNICODE_DEFAULT),
					const std::locale& lc = std::locale::classic()) BOOST_NOEXCEPT;
			private:
				static const detail::CharacterPropertyValueName NAMES_[];
			};

			/// Sentence_Break property. These values are based on UAX #29.
			class SentenceBreak {
			public:
				enum {
					FIRST_VALUE = WordBreak::LAST_VALUE,
					SEP = FIRST_VALUE, FORMAT, SP, LOWER, UPPER, O_LETTER, NUMERIC, A_TERM, S_TERM, CLOSE, OTHER,
					LAST_VALUE
				};
				static const int DEFAULT_VALUE;
				static const char LONG_NAME[], SHORT_NAME[];
				static int of(CodePoint cp) BOOST_NOEXCEPT;
			private:
				static const detail::CharacterPropertyValueName NAMES_[];
			};

			/**
			 * Legacy character classification like @c std#ctype (from
			 * <a href="http://www.unicode.org/reports/tr18/">UTS #18: Unicode Regular Expression,
			 * Annex C: Compatibility Property</a>.
			 */
			namespace legacyctype {
				bool isalpha(CodePoint c);
				bool isalnum(CodePoint c);
				bool isblank(CodePoint c);
				bool iscntrl(CodePoint c);
				bool isdigit(CodePoint c);
				bool isgraph(CodePoint c);
				bool islower(CodePoint c);
				bool isprint(CodePoint c);
				bool ispunct(CodePoint c);
				bool isspace(CodePoint c);
				bool isupper(CodePoint c);
				bool isword(CodePoint c);
				bool isxdigit(CodePoint c);
			} // namespace legacyctype


// inline implementations ///////////////////////////////////////////////////
			
/// Returns true if the specified character is a letter.
template<> inline bool GeneralCategory::is<GeneralCategory::LETTER>(int gc) {return gc >= UPPERCASE_LETTER && gc <= OTHER_LETTER;}

/// Returns true if the specified sub-category is a cased letter.
template<> inline bool GeneralCategory::is<GeneralCategory::CASED_LETTER>(int gc) {return gc >= UPPERCASE_LETTER && gc <= TITLECASE_LETTER;}

/// Returns true if the specified sub-category is a mark.
template<> inline bool GeneralCategory::is<GeneralCategory::MARK>(int gc) {return gc >= NONSPACING_MARK && gc <= ENCLOSING_MARK;}

/// Returns true if the specified sub-category is a number.
template<> inline bool GeneralCategory::is<GeneralCategory::NUMBER>(int gc) {return gc >= DECIMAL_NUMBER && gc <= OTHER_NUMBER;}

/// Returns true if the specified sub-category is a punctuation.
template<> inline bool GeneralCategory::is<GeneralCategory::PUNCTUATION>(int gc) {return gc >= CONNECTOR_PUNCTUATION && gc <= OTHER_PUNCTUATION;}

/// Returns true if the specified sub-category is a symbol.
template<> inline bool GeneralCategory::is<GeneralCategory::SYMBOL>(int gc) {return gc >= MATH_SYMBOL && gc <= OTHER_SYMBOL;}

/// Returns true if the specified sub-category is a separator.
template<> inline bool GeneralCategory::is<GeneralCategory::SEPARATOR>(int gc) {return gc >= SPACE_SEPARATOR&& gc <= PARAGRAPH_SEPARATOR;}

/// Returns true if the specified sub-category is an other.
template<> inline bool GeneralCategory::is<GeneralCategory::OTHER>(int gc) {return gc >= CONTROL && gc <= UNASSIGNED;}

/// Specialization to implement Alphabetic property.
template<> inline bool BinaryProperty::is<BinaryProperty::ALPHABETIC>(CodePoint cp) {
	// Alphabetic :=
	//   Lu + Ll + Lt + Lm + Lo + Nl + Other_Alphabetic
	const int gc = GeneralCategory::of(cp);
	return gc == GeneralCategory::UPPERCASE_LETTER
		|| gc == GeneralCategory::LOWERCASE_LETTER
		|| gc == GeneralCategory::TITLECASE_LETTER
		|| gc == GeneralCategory::OTHER_LETTER
		|| gc == GeneralCategory::LETTER_NUMBER
		|| is<OTHER_ALPHABETIC>(cp);}

/// Specialization to implement Default_Ignorable_Code_Point property.
template<> inline bool BinaryProperty::is<BinaryProperty::DEFAULT_IGNORABLE_CODE_POINT>(CodePoint cp) {
	// Default_Ignorable_Code_Point :=
	//   Other_Default_Ignorable_Code_Point
	//   + Cf (Format characters)
	//   + Variation_Selector
	//   - White_Space
	//   - FFF9..FFFB (Annotation Characters)
	//   - 0600..0603, 06DD, 070F (exceptional Cf characters that should be visible)
	static const CodePoint EXCLUDED[] = {0x0600u, 0x0601u, 0x0602u, 0x0603u, 0x06ddu, 0x070fu, 0xfff9u, 0xfffau, 0xfffbu};
	return (GeneralCategory::of(cp) == GeneralCategory::FORMAT
		|| is<VARIATION_SELECTOR>(cp)
		|| is<OTHER_DEFAULT_IGNORABLE_CODE_POINT>(cp))
		&& !is<WHITE_SPACE>(cp)
		&& !std::binary_search(EXCLUDED, ASCENSION_ENDOF(EXCLUDED), cp);}

/// Specialization to implement Grapheme_Extend property.
template<> inline bool BinaryProperty::is<BinaryProperty::GRAPHEME_EXTEND>(CodePoint cp) {
	// Grapheme_Extend :=
	//   Me + Mn + Other_Grapheme_Extend
	const int gc = GeneralCategory::of(cp);
	return gc == GeneralCategory::ENCLOSING_MARK
		|| gc == GeneralCategory::NONSPACING_MARK
		|| is<OTHER_GRAPHEME_EXTEND>(cp);}

/// Specialization to implement Grapheme_Base property.
template<> inline bool BinaryProperty::is<BinaryProperty::GRAPHEME_BASE>(CodePoint cp) {
	// Grapheme_Base :=
	//   [0..10FFFF] - Cc - Cf - Cs - Co - Cn - Zl - Zp - Grapheme_Extend
	const int gc = GeneralCategory::of(cp);
	return !GeneralCategory::is<GeneralCategory::OTHER>(gc)
		&& gc != GeneralCategory::LINE_SEPARATOR
		&& gc != GeneralCategory::PARAGRAPH_SEPARATOR
		&& !is<GRAPHEME_EXTEND>(cp);}

/// Specialization to implement ID_Start property.
template<> inline bool BinaryProperty::is<BinaryProperty::ID_START>(CodePoint cp) {
	// ID_Start :=
	//     Lu + Ll + Lt + Lm + Lo + Nl
	//   + Other_ID_Start
	//   - Pattern_Syntax
	//   - Pattern_White_Space
	const int gc = GeneralCategory::of(cp);
	return (GeneralCategory::is<GeneralCategory::LETTER>(gc)
		|| gc == GeneralCategory::LETTER_NUMBER || is<OTHER_ID_START>(cp))
		&& !is<PATTERN_SYNTAX>(cp) && !is<PATTERN_WHITE_SPACE>(cp);}

/// Specialization to implement ID_Continue property.
template<> inline bool BinaryProperty::is<BinaryProperty::ID_CONTINUE>(CodePoint cp) {
	// ID_Continue :=
	//     ID_Start
	//   + Mn + Mc + Nd + Pc
	//   + Other_ID_Continue
	//   - Pattern_Syntax
	//   - Pattern_White_Space
	if(is<BinaryProperty::ID_START>(cp))
		return true;
	const int gc = GeneralCategory::of(cp);
	return (gc == GeneralCategory::NONSPACING_MARK
		|| gc == GeneralCategory::SPACING_MARK
		|| gc == GeneralCategory::DECIMAL_NUMBER
		|| gc == GeneralCategory::CONNECTOR_PUNCTUATION
		|| is<OTHER_ID_CONTINUE>(cp))
		&& !is<PATTERN_SYNTAX>(cp) && !is<PATTERN_WHITE_SPACE>(cp);}

/// Specialization to implement Lowercase property.
template<> inline bool BinaryProperty::is<BinaryProperty::LOWERCASE>(CodePoint cp) {
	// Lowercase :=
	//   Ll + Other_Lowercase
	return GeneralCategory::of(cp) == GeneralCategory::LOWERCASE_LETTER || is<OTHER_LOWERCASE>(cp);}

/// Specialization to implement Math property.
template<> inline bool BinaryProperty::is<BinaryProperty::MATH>(CodePoint cp) {
	// Math :=
	//   Sm + Other_Math
	return GeneralCategory::of(cp) == GeneralCategory::MATH_SYMBOL || is<OTHER_MATH>(cp);}

/// Specialization to implement Uppercase property.
template<> inline bool BinaryProperty::is<BinaryProperty::UPPERCASE>(CodePoint cp) {
	// Uppercase :=
	//   Lu + Other_Uppercase
	return GeneralCategory::of(cp) == GeneralCategory::UPPERCASE_LETTER || is<OTHER_UPPERCASE>(cp);}

/// Returns the Hangul_Syllable_Type property value of @a cp.
inline int HangulSyllableType::of(CodePoint c) BOOST_NOEXCEPT {
	if((c >= 0x1100u && c <= 0x1159u) || c == 0x115fu)
		return LEADING_JAMO;
	else if(c >= 0x1160u && c <= 0x11a2u)
		return VOWEL_JAMO;
	else if(c >= 0x11a8u && c <= 0x11f9u)
		return TRAILING_JAMO;
	else if(c >= 0xac00u && c <= 0xd7a3u)
		return ((c - 0xac00u) % 28 == 0) ? LV_SYLLABLE : LVT_SYLLABLE;
	else
		return NOT_APPLICABLE;
}

/// Returns true if the character is an alphabet (alpha := \\p{Alphabetic}).
inline bool legacyctype::isalpha(CodePoint c) {return BinaryProperty::is<BinaryProperty::ALPHABETIC>(c);}

/// Returns true if the character is an alphabet or numeric (alnum := [:alpha:] | [:digit:]).
inline bool legacyctype::isalnum(CodePoint c) {return isalpha(c) || isdigit(c);}

/// Returns true if the character is a blank (blank := \\p{Whitespace} - [\\N{LF} \\N{VT} \\N{FF} \\N{CR} \\N{NEL} \\p{gc=Line_Separator} \\p{gc=Paragraph_Separator}]).
inline bool legacyctype::isblank(CodePoint c) {
	if(c == LINE_FEED || c == L'\v' || c == L'\f' || c == CARRIAGE_RETURN || c == NEXT_LINE)
		return false;
	if(BinaryProperty::is<BinaryProperty::WHITE_SPACE>(c)) {
		const int gc = GeneralCategory::of(c);
		return gc != GeneralCategory::LINE_SEPARATOR && gc != GeneralCategory::PARAGRAPH_SEPARATOR;
	}
	return false;
}

/// Returns true if the character is a control code (cntrl := \\p{gc=Control}).
inline bool legacyctype::iscntrl(CodePoint c) {return GeneralCategory::of(c) == GeneralCategory::CONTROL;}

/// Returns true if the character is a digit (digit := \\p{gc=Decimal_Number}).
inline bool legacyctype::isdigit(CodePoint c) {return GeneralCategory::of(c) == GeneralCategory::DECIMAL_NUMBER;}

/// Returns true if the character is graphical (graph := [^[:space:]\\p{gc=Control}\\p{Format}\\p{Surrogate}\\p{Unassigned}]).
inline bool legacyctype::isgraph(CodePoint c) {
	if(isspace(c))	return false;
	const int gc = GeneralCategory::of(c);
	return gc != GeneralCategory::CONTROL
		&& gc != GeneralCategory::FORMAT
		&& gc != GeneralCategory::SURROGATE
		&& gc != GeneralCategory::UNASSIGNED;
}

/// Returns true if the character is lower (lower := \\p{Lowercase}).
inline bool legacyctype::islower(CodePoint c) {return BinaryProperty::is<BinaryProperty::LOWERCASE>(c);}

/// Returns true if the character is printable (print := ([:graph] | [:blank:]) - [:cntrl:]).
inline bool legacyctype::isprint(CodePoint c) {return (isgraph(c) || isblank(c)) && !iscntrl(c);}

/// Returns true if the character is a punctuation (punct := \\p{gc=Punctuation}).
inline bool legacyctype::ispunct(CodePoint c) {return GeneralCategory::is<GeneralCategory::PUNCTUATION>(GeneralCategory::of(c));}

/// Returns true if the character is a white space (space := \\p{Whitespace}).
inline bool legacyctype::isspace(CodePoint c) {return BinaryProperty::is<BinaryProperty::WHITE_SPACE>(c);}

/// Returns true if the character is capital (upper := \\p{Uppercase}).
inline bool legacyctype::isupper(CodePoint c) {return BinaryProperty::is<BinaryProperty::UPPERCASE>(c);}

/// Returns true if the character can consist a word (word := [:alpha:]\\p{gc=Mark}[:digit:]\\p{gc=Connector_Punctuation}).
inline bool legacyctype::isword(CodePoint c) {
	if(isalpha(c) || isdigit(c)) return true;
	const int gc = GeneralCategory::of(c);
	return GeneralCategory::is<GeneralCategory::MARK>(gc) || gc == GeneralCategory::CONNECTOR_PUNCTUATION;}

/// Returns true if the character is a hexadecimal (xdigit := \\p{gc=Decimal_Number} | \\p{Hex_Digit}).
inline bool legacyctype::isxdigit(CodePoint c) {
	return GeneralCategory::of(c) == GeneralCategory::DECIMAL_NUMBER || BinaryProperty::is<BinaryProperty::HEX_DIGIT>(c);}

}}} // namespace ascension.text.ucd

#endif // !ASCENSION_CHARACTER_PROPERTY_HPP
