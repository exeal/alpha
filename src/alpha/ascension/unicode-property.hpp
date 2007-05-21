/**
 * @file unicode-property.hpp
 * Defines Unicode property entries and provides methods to retrieve a property of character.
 * @author exeal
 * @date 2007
 */

#ifndef ASCENSION_UNICODE_PROPERTY_HPP
#define ASCENSION_UNICODE_PROPERTY_HPP
#if ASCENSION_UNICODE_VERSION > 0x0500
#error These class definitions and implementations are based on old version of Unicode.
#endif
#include "unicode.hpp"

namespace ascension {
	namespace unicode {

		namespace internal {
			// helpers for Unicode properties implementation
			template<typename Code> struct CodeRange {
				Code first, last;
				bool operator<(Code rhs) const {return first < rhs;}
			};
			struct PropertyRange {
				CodePoint first, last;
				ushort property;
				bool operator<(CodePoint rhs) const {return first < rhs;}
			};
			template<typename Element> static const Element* findInRange(const Element* first, const Element* last, CodePoint cp) {
				const Element* p = std::lower_bound(first, last, cp);
				if(p == last) return 0;
				else if(p->first == cp) return p;
				else if(p->first > cp && p != first && p[-1].last >= cp) return p - 1;
				else return 0;
			}
		} // namespace internal

		/**
		 * A function object compares Unicode property (value) names based on "Property and Property Value Matching"
		 * (http://www.unicode.org/Public/UNIDATA/UCD.html#Property_and_Property_Value_Matching).
		 * @param p1 one property name
		 * @param p2 the other property name
		 * @return true if p1 &lt; p2
		 */
		template<typename CharType>
		struct PropertyNameComparer {
			bool		operator()(const CharType* p1, const CharType* p2) const;
			static int	compare(const CharType* p1, const CharType* p2);
		};

		const int NOT_PROPERTY = 0;

		/// General categories. These values are based on Unicode standard 5.0.0 "4.5 General Category".
		class GeneralCategory {
		public:
			enum {
				// sub-categories
				LETTER_UPPERCASE = 1,		///< Lu = Letter, uppercase
				LETTER_LOWERCASE,			///< Ll = Letter, lowercase
				LETTER_TITLECASE,			///< Lt = Letter, titlecase
				LETTER_MODIFIER,			///< Lm = Letter, modifier
				LETTER_OTHER,				///< Lo = Letter, other
				MARK_NONSPACING,			///< Mn = Mark, nonspacing
				MARK_SPACING_COMBINING,		///< Mc = Mark, spacing combining
				MARK_ENCLOSING,				///< Me = Mark, enclosing
				NUMBER_DECIMAL_DIGIT,		///< Nd = Number, decimal digit
				NUMBER_LETTER,				///< Nl = Number, letter
				NUMBER_OTHER,				///< No = Number, other
				PUNCTUATION_CONNECTOR,		///< Pc = Punctuation, connector
				PUNCTUATION_DASH,			///< Pd = Punctuation, dash
				PUNCTUATION_OPEN,			///< Ps = Punctuation, open
				PUNCTUATION_CLOSE,			///< Pe = Punctuation, close
				PUNCTUATION_INITIAL_QUOTE,	///< Pi = Punctuation, initial quote
				PUNCTUATION_FINAL_QUOTE,	///< Pf = Punctuation, final quote
				PUNCTUATION_OTHER,			///< Po = Punctuation, other
				SYMBOL_MATH,				///< Sm = Symbol, math
				SYMBOL_CURRENCY,			///< Sc = Symbol, currency
				SYMBOL_MODIFIER,			///< Sk = Symbol, modifier
				SYMBOL_OTHER,				///< So = Symbol, other
				SEPARATOR_SPACE,			///< Zs = Separator, space
				SEPARATOR_LINE,				///< Zl = Separator, line
				SEPARATOR_PARAGRAPH,		///< Zp = Separator, paragraph
				OTHER_CONTROL,				///< Cc = Other, control
				OTHER_FORMAT,				///< Cf = Other, format
				OTHER_SURROGATE,			///< Cs = Other, surrogate
				OTHER_PRIVATE_USE,			///< Co = Other, private use
				OTHER_UNASSIGNED,			///< Cn = Other, not assigned
				// super-categories
				LETTER,			///< L = Letter
				LETTER_CASED,	///< Lc = Letter, cased
				MARK,			///< M = Mark
				NUMBER,			///< N = Number
				PUNCTUATION,	///< P = Punctuation
				SYMBOL,			///< S = Symbol
				SEPARATOR,		///< Z = Separator
				OTHER,			///< C = Other
				COUNT
			};
			static const Char LONG_NAME[], SHORT_NAME[];
			static int	forName(const Char* name);
			template<int superCategory>
			static bool	is(int subCategory);
			static int	of(CodePoint cp) throw();
		private:
			static const internal::PropertyRange ranges_[];
			static const std::size_t count_;
			static std::map<const Char*, int, PropertyNameComparer<Char> > names_;
			static void buildNames();
		};
		
		/// Returns true if the specified character is a letter.
		template<> inline bool GeneralCategory::is<GeneralCategory::LETTER>(int gc) {return gc >= LETTER_UPPERCASE && gc <= LETTER_OTHER;}
		/// Returns true if the specified sub-category is a cased letter.
		template<> inline bool GeneralCategory::is<GeneralCategory::LETTER_CASED>(int gc) {return gc >= LETTER_UPPERCASE && gc <= LETTER_TITLECASE;}
		/// Returns true if the specified sub-category is a mark.
		template<> inline bool GeneralCategory::is<GeneralCategory::MARK>(int gc) {return gc >= MARK_NONSPACING && gc <= MARK_ENCLOSING;}
		/// Returns true if the specified sub-category is a number.
		template<> inline bool GeneralCategory::is<GeneralCategory::NUMBER>(int gc) {return gc >= NUMBER_DECIMAL_DIGIT && gc <= NUMBER_OTHER;}
		/// Returns true if the specified sub-category is a punctuation.
		template<> inline bool GeneralCategory::is<GeneralCategory::PUNCTUATION>(int gc) {return gc >= PUNCTUATION_CONNECTOR && gc <= PUNCTUATION_OTHER;}
		/// Returns true if the specified sub-category is a symbol.
		template<> inline bool GeneralCategory::is<GeneralCategory::SYMBOL>(int gc) {return gc >= SYMBOL_MATH && gc <= SYMBOL_OTHER;}
		/// Returns true if the specified sub-category is a separator.
		template<> inline bool GeneralCategory::is<GeneralCategory::SEPARATOR>(int gc) {return gc >= SEPARATOR_SPACE && gc <= SEPARATOR_PARAGRAPH;}
		/// Returns true if the specified sub-category is an other.
		template<> inline bool GeneralCategory::is<GeneralCategory::OTHER>(int gc) {return gc >= OTHER_CONTROL && gc <= OTHER_UNASSIGNED;}
		
		/// Code blocks. These values are based on Blocks.txt obtained from UCD.
		class CodeBlock {
		public:
			enum {
				NO_BLOCK = GeneralCategory::COUNT,
				BASIC_LATIN, LATIN_1_SUPPLEMENT, LATIN_EXTENDED_A, LATIN_EXTENDED_B, IPA_EXTENSIONS,
				SPACING_MODIFIER_LETTERS, COMBINING_DIACRITICAL_MARKS, GREEK_AND_COPTIC, CYRILLIC,
				CYRILLIC_SUPPLEMENT, ARMENIAN, HEBREW, ARABIC, SYRIAC, ARABIC_SUPPLEMENT, THAANA,
				NKO, DEVANAGARI, BENGALI, GURMUKHI, GUJARATI, ORIYA, TAMIL, TELUGU, KANNADA, MALAYALAM,
				SINHALA, THAI, LAO, TIBETAN, MYANMAR, GEORGIAN, HANGUL_JAMO, ETHIOPIC, ETHIOPIC_SUPPLEMENT,
				CHEROKEE, UNIFIED_CANADIAN_ABORIGINAL_SYLLABICS, OGHAM, RUNIC, TAGALOG, HANUNOO, BUHID,
				TAGBANWA, KHMER, MONGOLIAN, LIMBU, TAI_LE, NEW_TAI_LUE, KHMER_SYMBOLS, BUGINESE,
				BALINESE, PHONETIC_EXTENSIONS, PHONETIC_EXTENSIONS_SUPPLEMENT,
				COMBINING_DIACRITICAL_MARKS_SUPPLEMENT, LATIN_EXTENDED_ADDITIONAL, GREEK_EXTENDED,
				GENERAL_PUNCTUATION, SUPERSCRIPTS_AND_SUBSCRIPTS, CURRENCY_SYMBOLS,
				COMBINING_DIACRITICAL_MARKS_FOR_SYMBOLS, LETTERLIKE_SYMBOLS, NUMBER_FORMS, ARROWS,
				MATHEMATICAL_OPERATORS, MISCELLANEOUS_TECHNICAL, CONTROL_PICTURES,
				OPTICAL_CHARACTER_RECOGNITION, ENCLOSED_ALPHANUMERICS, BOX_DRAWING, BLOCK_ELEMENTS,
				GEOMETRIC_SHAPES, MISCELLANEOUS_SYMBOLS, DINGBATS, MISCELLANEOUS_MATHEMATICAL_SYMBOLS_A,
				SUPPLEMENTAL_ARROWS_A, BRAILLE_PATTERNS, SUPPLEMENTAL_ARROWS_B,
				MISCELLANEOUS_MATHEMATICAL_SYMBOLS_B, SUPPLEMENTAL_MATHEMATICAL_OPERATORS,
				MISCELLANEOUS_SYMBOLS_AND_ARROWS, GLAGOLITIC, LATIN_EXTENDED_C, COPTIC,
				GEORGIAN_SUPPLEMENT, TIFINAGH, ETHIOPIC_EXTENDED, SUPPLEMENTAL_PUNCTUATION,
				CJK_RADICALS_SUPPLEMENT, KANGXI_RADICALS, IDEOGRAPHIC_DESCRIPTION_CHARACTERS,
				CJK_SYMBOLS_AND_PUNCTUATION, HIRAGANA, KATAKANA, BOPOMOFO, HANGUL_COMPATIBILITY_JAMO,
				KANBUN, BOPOMOFO_EXTENDED, CJK_STROKES, KATAKANA_PHONETIC_EXTENSIONS,
				ENCLOSED_CJK_LETTERS_AND_MONTHS, CJK_COMPATIBILITY, CJK_UNIFIED_IDEOGRAPHS_EXTENSION_A,
				YIJING_HEXAGRAM_SYMBOLS, CJK_UNIFIED_IDEOGRAPHS, YI_SYLLABLES, YI_RADICALS,
				MODIFIER_TONE_LETTERS, LATIN_EXTENDED_D, SYLOTI_NAGRI, PHAGS_PA, HANGUL_SYLLABLES,
				HIGH_SURROGATES, HIGH_PRIVATE_USE_SURROGATES, LOW_SURROGATES, PRIVATE_USE_AREA,
				CJK_COMPATIBILITY_IDEOGRAPHS, ALPHABETIC_PRESENTATION_FORMS, ARABIC_PRESENTATION_FORMS_A,
				VARIATION_SELECTORS, VERTICAL_FORMS, COMBINING_HALF_MARKS, CJK_COMPATIBILITY_FORMS,
				SMALL_FORM_VARIANTS, ARABIC_PRESENTATION_FORMS_B, HALFWIDTH_AND_FULLWIDTH_FORMS,
				SPECIALS, LINEAR_B_SYLLABARY, LINEAR_B_IDEOGRAMS, AEGEAN_NUMBERS, ANCIENT_GREEK_NUMBERS,
				OLD_ITALIC, GOTHIC, UGARITIC, OLD_PERSIAN, DESERET, SHAVIAN, OSMANYA, CYPRIOT_SYLLABARY,
				PHOENICIAN, KHAROSHTHI, CUNEIFORM, CUNEIFORM_NUMBERS_AND_PUNCTUATION, BYZANTINE_MUSICAL_SYMBOLS,
				MUSICAL_SYMBOLS, ANCIENT_GREEK_MUSICAL_NOTATION, TAI_XUAN_JING_SYMBOLS,
				COUNTING_ROD_NUMERALS, MATHEMATICAL_ALPHANUMERIC_SYMBOLS, CJK_UNIFIED_IDEOGRAPHS_EXTENSION_B,
				CJK_COMPATIBILITY_IDEOGRAPHS_SUPPLEMENT, TAGS, VARIATION_SELECTORS_SUPPLEMENT,
				SUPPLEMENTARY_PRIVATE_USE_AREA_A, SUPPLEMENTARY_PRIVATE_USE_AREA_B, COUNT
			};
			static const Char LONG_NAME[], SHORT_NAME[];
			static int	forName(const Char* name);
			static int	of(CodePoint cp) throw();
		private:
			static const internal::PropertyRange ranges_[];
			static const std::size_t count_;
			static std::map<const Char*, int, PropertyNameComparer<Char> > names_;
			static void buildNames();
		};

#ifndef ASCENSION_NO_UNICODE_NORMALIZATION
		/**
		 * Canonical combining classes. These are based on Unicode standard 5.0.0 "4.3 Combining Classes".
		 * @see Normalizer
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
				ATTAHCED_ABOVE_RIGHT	= 216,	///< Above right attached (216).
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
			static const Char LONG_NAME[], SHORT_NAME[];
			static int	forName(const Char* name);
			static int	of(CodePoint cp) throw();
		private:
			static const Char SRC_UCS2[];
			static const CodePoint SRC_UCS4[];
			static const uchar DEST_UCS2[], DEST_UCS4[];
			static const std::size_t UCS2_COUNT, UCS4_COUNT;
			static std::map<const Char*, int, PropertyNameComparer<Char> > names_;
			static void buildNames();
		};
#endif /* !ASCENSION_NO_UNICODE_NORMALIZATION */

		/**
		 * Scripts. These are based on
		 * <a href="http://www.unicode.org/reports/tr24/">UAX #24: Script Names</a> revision 9 and
		 * Scripts.txt obtained from UCD.
		 */
		class Script {
		public:
			enum {
				UNKNOWN = CodeBlock::COUNT, COMMON,
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
				// derived
				KATAKANA_OR_HIRAGANA,
				COUNT
			};
			static const Char LONG_NAME[], SHORT_NAME[];
			static int	forName(const Char* name);
			static int	of(CodePoint cp) throw();
		private:
			static const internal::PropertyRange ranges_[];
			static const std::size_t count_;
			static std::map<const Char*, int, PropertyNameComparer<Char> > names_;
			static void buildNames();
		};

		/// Hangul syllable types. These values are based on HangulSyllableType.txt obtained from UCD.
		class HangulSyllableType {
		public:
			enum {
				NOT_APPLICABLE = Script::COUNT,	///< NA = Not_Applicable
				LEADING_JAMO,					///< L = Leading_Jamo
				VOWEL_JAMO,						///< V = Vowel_Jamo
				TRAILING_JAMO,					///< T = Trailing_Jamo
				LV_SYLLABLE,					///< LV = LV_Syllable
				LVT_SYLLABLE,					///< LVT = LVT_Syllable
				COUNT
			};
			static const Char LONG_NAME[], SHORT_NAME[];
			static int	forName(const Char* name);
			static int	of(CodePoint cp) throw();
		private:
			static std::map<const Char*, int, PropertyNameComparer<Char> > names_;
			static void buildNames();
		};
		
		/**
		 * Binary properties These values are based on UCD.html and PropList.txt obtained from UCD.
		 * @note Some values are not implemented.
		 */
		class BinaryProperty {
		public:
			enum {
				ALPHABETIC = HangulSyllableType::COUNT, ASCII_HEX_DIGIT, BIDI_CONTROL, BIDI_MIRRORED,
				COMPOSITION_EXCLUSION, DASH, DEFAULT_IGNORABLE_CODE_POINT, DEPRECATED, DIACRITIC,
				EXPANDS_ON_NFC, EXPANDS_ON_NFD, EXPANDS_ON_NFKC, EXPANDS_ON_NFKD, EXTENDER,
				FULL_COMPOSITION_EXCLUSION, GRAPHEME_BASE, GRAPHEME_EXTEND, HEX_DIGIT, HYPHEN,
				ID_CONTINUE, ID_START, IDEOGRAPHIC, IDS_BINARY_OPERATOR, IDS_TRINARY_OPERATOR,
				JOIN_CONTROL, LOGICAL_ORDER_EXCEPTION, LOWERCASE, MATH, NONCHARACTER_CODE_POINT,
				OTHER_ALPHABETIC, OTHER_DEFAULT_IGNORABLE_CODE_POINT, OTHER_GRAPHEME_EXTEND,
				OTHER_ID_CONTINUE, OTHER_ID_START, OTHER_LOWERCASE, OTHER_MATH, OTHER_UPPERCASE,
				PATTERN_SYNTAX, PATTERN_WHITE_SPACE, QUOTATION_MARK, RADICAL, SOFT_DOTTED, STERM,
				TERMINAL_PUNCTUATION, UNIFIED_IDEOGRAPH, UPPERCASE, VARIATION_SELECTOR, WHITE_SPACE,
				XID_CONTINUE, XID_START, COUNT
			};
			static int	forName(const Char* first, const Char* last);
			static bool	is(CodePoint cp, int property);
			template<int property>
			static bool	is(CodePoint cp) throw();
		private:
			static std::map<const Char*, int, PropertyNameComparer<Char> > names_;
			static void buildNames();
#include "code-table/uprops-binary-property-table-definition"
		};

#include "code-table/uprops-implementation"
		// derived core properties (explicit specialization)
		template<> inline bool BinaryProperty::is<BinaryProperty::ALPHABETIC>(CodePoint cp) {
			const int gc = GeneralCategory::of(cp);
			return gc == GeneralCategory::LETTER_UPPERCASE
				|| gc == GeneralCategory::LETTER_LOWERCASE
				|| gc == GeneralCategory::LETTER_TITLECASE
				|| gc == GeneralCategory::LETTER_OTHER
				|| gc == GeneralCategory::NUMBER_LETTER
				|| is<OTHER_ALPHABETIC>(cp);}
		template<> inline bool BinaryProperty::is<BinaryProperty::DEFAULT_IGNORABLE_CODE_POINT>(CodePoint cp) {
			const int gc = GeneralCategory::of(cp);
			return (gc == GeneralCategory::OTHER_FORMAT
				|| gc == GeneralCategory::OTHER_CONTROL
				|| gc == GeneralCategory::OTHER_SURROGATE
				|| is<OTHER_DEFAULT_IGNORABLE_CODE_POINT>(cp)
				|| is<NONCHARACTER_CODE_POINT>(cp))
				&& !is<WHITE_SPACE>(cp)
				&& (cp < 0xFFF9 || cp > 0xFFFB);}
		template<> inline bool BinaryProperty::is<BinaryProperty::LOWERCASE>(CodePoint cp) {
			return GeneralCategory::of(cp) == GeneralCategory::LETTER_LOWERCASE || is<OTHER_LOWERCASE>(cp);}
		template<> inline bool BinaryProperty::is<BinaryProperty::GRAPHEME_EXTEND>(CodePoint cp) {
			const int gc = GeneralCategory::of(cp);
			return gc == GeneralCategory::MARK_ENCLOSING
				|| gc == GeneralCategory::MARK_NONSPACING
				|| is<OTHER_GRAPHEME_EXTEND>(cp);}
		template<> inline bool BinaryProperty::is<BinaryProperty::GRAPHEME_BASE>(CodePoint cp) {
			const int gc = GeneralCategory::of(cp);
			return !GeneralCategory::is<GeneralCategory::OTHER>(gc)
				&& gc != GeneralCategory::SEPARATOR_LINE
				&& gc != GeneralCategory::SEPARATOR_PARAGRAPH
				&& !is<GRAPHEME_EXTEND>(cp);}
		template<> inline bool BinaryProperty::is<BinaryProperty::ID_CONTINUE>(CodePoint cp) {
			const int gc = GeneralCategory::of(cp);
			return GeneralCategory::is<GeneralCategory::LETTER>(gc)
				|| gc == GeneralCategory::MARK_NONSPACING
				|| gc == GeneralCategory::MARK_SPACING_COMBINING
				|| gc == GeneralCategory::NUMBER_DECIMAL_DIGIT
				|| gc == GeneralCategory::NUMBER_LETTER
				|| gc == GeneralCategory::PUNCTUATION_CONNECTOR
				|| is<OTHER_ID_START>(cp)
				|| is<OTHER_ID_CONTINUE>(cp);}
		template<> inline bool BinaryProperty::is<BinaryProperty::ID_START>(CodePoint cp) {
			const int gc = GeneralCategory::of(cp);
			return GeneralCategory::is<GeneralCategory::LETTER>(gc)
				|| gc == GeneralCategory::NUMBER_LETTER
				|| is<OTHER_ID_START>(cp);}
		template<> inline bool BinaryProperty::is<BinaryProperty::MATH>(CodePoint cp) {
			return GeneralCategory::of(cp) == GeneralCategory::SYMBOL_MATH || is<OTHER_MATH>(cp);}
		template<> inline bool BinaryProperty::is<BinaryProperty::UPPERCASE>(CodePoint cp) {
			return GeneralCategory::of(cp) == GeneralCategory::LETTER_UPPERCASE || is<OTHER_UPPERCASE>(cp);}

#ifndef ASCENSION_NO_UAX14
		/// Line_Break property. These values are based on UAX #14.
		class LineBreak {
		public:
			enum {
				// non-tailorable line breaking classes
				MANDATORY_BREAK = BinaryProperty::COUNT, CARRIAGE_RETURN, LINE_FEED, COMBINING_MARK,
				NEXT_LINE, SURROGATE, WORD_JOINER, ZW_SPACE, GLUE, SPACE,
				// break opportunities
				BREAK_BOTH, BREAK_AFTER, BREAK_BEFORE, HYPHEN, CONTINGENT_BREAK,
				// characters prohibiting certain breaks
				CLOSE_PUNCTUATION, EXCLAMATION, INSEPARABLE, NONSTARTER, OPEN_PUNCTUATION, QUOTATION,
				// numeric context
				INFIX_NUMERIC, NUMERIC, POSTFIX_NUMERIC, PREFIX_NUMERIC, BREAK_SYMBOLS,
				// other characters
				AMBIGUOUS, ALPHABETIC, H2, H3, IDEOGRAPHIC, JL, JV, JT, COMPLEX_CONTEXT, UNKNOWN,
				COUNT
			};
			static const Char LONG_NAME[], SHORT_NAME[];
			static int	forName(const Char* name);
			static int	of(CodePoint cp) throw();
		private:
			static std::map<const Char*, int, PropertyNameComparer<Char> > names_;
			static void buildNames();
		};
#endif /* !ASCENSION_NO_UAX14 */

		/// Grapheme_Cluster_Break property. These values are based on UAX #29.
		class GraphemeClusterBreak {
		public:
			enum {
#ifndef ASCENSION_NO_UAX14
				CR = LineBreak::COUNT,
#else
				CR = BinaryProperty::COUNT,
#endif /* !ASCENSION_NO_UAX14 */
				LF, CONTROL, EXTEND, L, V, T, LV, LVT, OTHER, COUNT
			};
			static const Char LONG_NAME[], SHORT_NAME[];
			static int	forName(const Char* name);
			static int	of(CodePoint cp) throw();
		private:
			static std::map<const Char*, int, PropertyNameComparer<Char> > names_;
			static void buildNames();
		};

		/// Word_Break property. These values are based on UAX #29.
		class WordBreak {
		public:
			enum {
				FORMAT = GraphemeClusterBreak::COUNT, KATAKANA, A_LETTER, MID_LETTER, MID_NUM, NUMERIC, EXTEND_NUM_LET, OTHER, COUNT
			};
			static const Char LONG_NAME[], SHORT_NAME[];
			static int	forName(const Char* name);
			static int	of(CodePoint cp,
							const IdentifierSyntax& syntax = IdentifierSyntax(IdentifierSyntax::UNICODE_DEFAULT),
							const std::locale& lc = std::locale::classic()) throw();
		private:
			static std::map<const Char*, int, PropertyNameComparer<Char> > names_;
			static void buildNames();
		};

		/// Sentence_Break property. These values are based on UAX #29.
		class SentenceBreak {
		public:
			enum {
				SEP = WordBreak::COUNT, FORMAT, SP, LOWER, UPPER, O_LETTER, NUMERIC, A_TERM, S_TERM, CLOSE, OTHER, COUNT
			};
			static const Char LONG_NAME[], SHORT_NAME[];
			static int	forName(const Char* name);
			static int	of(CodePoint cp) throw();
		private:
			static std::map<const Char*, int, PropertyNameComparer<Char> > names_;
			static void buildNames();
		};

		/**
		 * Legacy character classification like @c std#ctype (from
		 * <a href="http://www.unicode.org/reports/tr18/">UTS #18: Unicode Regular Expression, Annex
		 * C: Compatibility Property</a>.
		 */
		namespace legacyctype {
			/// Returns true if the character is an alphabet (alpha := \\p{Alphabetic}).
			inline bool isalpha(CodePoint cp) {return BinaryProperty::is<BinaryProperty::ALPHABETIC>(cp);}
			/// Returns true if the character is an alphabet or numeric (alnum := [:alpha:] | [:digit:]).
			inline bool isalnum(CodePoint cp) {return isalpha(cp) || isdigit(cp);}
			/// Returns true if the character is a blank (blank := \\p{Whitespace} - [\\N{LF} \\N{VT} \\N{FF} \\N{CR} \\N{NEL} \\p{gc=Line_Separator} \\p{gc=Paragraph_Separator}]).
			inline bool isblank(CodePoint cp) {
				if(cp == LINE_FEED || cp == L'\v' || cp == L'\f' || cp == CARRIAGE_RETURN || cp == NEXT_LINE)	return false;
				if(BinaryProperty::is<BinaryProperty::WHITE_SPACE>(cp)) {
					const int gc = GeneralCategory::of(cp);
					return gc != GeneralCategory::SEPARATOR_LINE && gc != GeneralCategory::SEPARATOR_PARAGRAPH;
				}
				return false;
			}
			/// Returns true if the character is a control code (cntrl := \\p{gc=Control}).
			inline bool iscntrl(CodePoint cp) {return GeneralCategory::of(cp) == GeneralCategory::OTHER_CONTROL;}
			/// Returns true if the character is a digit (digit := \\p{gc=Decimal_Number}).
			inline bool isdigit(CodePoint cp) {return GeneralCategory::of(cp) == GeneralCategory::NUMBER_DECIMAL_DIGIT;}
			/// Returns true if the character is graphical (graph := [^[:space:]\\p{gc=Control}\\p{Format}\\p{Surrogate}\\p{Unassigned}]).
			inline bool isgraph(CodePoint cp) {
				if(isspace(cp))	return false;
				const int gc = GeneralCategory::of(cp);
				return gc != GeneralCategory::OTHER_CONTROL
					&& gc != GeneralCategory::OTHER_FORMAT
					&& gc != GeneralCategory::OTHER_SURROGATE
					&& gc != GeneralCategory::OTHER_UNASSIGNED;
			}
			/// Returns true if the character is lower (lower := \\p{Lowercase}).
			inline bool islower(CodePoint cp) {return BinaryProperty::is<BinaryProperty::LOWERCASE>(cp);}
			/// Returns true if the character is printable (print := ([:graph] | [:blank:]) - [:cntrl:]).
			inline bool isprint(CodePoint cp) {return (isgraph(cp) || isblank(cp)) && !iscntrl(cp);}
			/// Returns true if the character is a punctuation (punct := \\p{gc=Punctuation}).
			inline bool ispunct(CodePoint cp) {return GeneralCategory::is<GeneralCategory::PUNCTUATION>(GeneralCategory::of(cp));}
			/// Returns true if the character is a white space (space := \\p{Whitespace}).
			inline bool isspace(CodePoint cp) {return BinaryProperty::is<BinaryProperty::WHITE_SPACE>(cp);}
			/// Returns true if the character is capital (upper := \\p{Uppercase}).
			inline bool isupper(CodePoint cp) {return BinaryProperty::is<BinaryProperty::UPPERCASE>(cp);}
			/// Returns true if the character can consist a word (word := [:alpha:]\\p{gc=Mark}[:digit:]\\p{gc=Connector_Punctuation}).
			inline bool isword(CodePoint cp) {
				if(isalpha(cp) || isdigit(cp)) return true;
				const int gc = GeneralCategory::of(cp);
				return GeneralCategory::is<GeneralCategory::MARK>(gc) || gc == GeneralCategory::PUNCTUATION_CONNECTOR;}
			/// Returns true if the character is a hexadecimal (xdigit := \\p{gc=Decimal_Number} | \\p{Hex_Digit}).
			inline bool isxdigit(CodePoint cp) {
				return GeneralCategory::of(cp) == GeneralCategory::NUMBER_DECIMAL_DIGIT || BinaryProperty::is<BinaryProperty::HEX_DIGIT>(cp);}
		} // namespace legacyctype


// inline implementations ///////////////////////////////////////////////////

/**
 * Compares the given two strings.
 * @param p1 one property name
 * @param p2 the other property name
 * @return true if p1 &lt; p2
 */
template<typename CharType>
inline bool PropertyNameComparer<CharType>::operator()(const CharType* p1, const CharType* p2) const {return compare(p1, p2) < 0;}

/**
 * Compares the given two strings.
 * @param p1 one property name
 * @param p2 the other property name
 * @return &lt; 0 if @a p1 &lt; @a p2
 * @return 0 if @a p1 == @a p2
 * @return &gt; 0 if @a p1 &gt; @a p2
 */
template<typename CharType>
inline int PropertyNameComparer<CharType>::compare(const CharType* p1, const CharType* p2) {
	while(*p1 != 0 && *p2 != 0) {
		if(*p1 == '_' || *p1 == '-' || *p1 == ' ') {
			++p1; continue;
		} else if(*p2 == '_' || *p2 == '-' || *p2 == ' ') {
			++p2; continue;
		}
		const int c1 = std::tolower(*p1, std::locale::classic()), c2 = std::tolower(*p2, std::locale::classic());
		if(c1 != c2)	return c1 - c2;
		else			++p1, ++p2;
	}
	return *p1 - *p2;
}

#define IMPLEMENT_FORNAME																				\
	if(name == 0) throw std::invalid_argument("the name is null.");										\
	else if(names_.empty()) buildNames();																\
	const std::map<const Char*, int, PropertyNameComparer<Char> >::const_iterator i(names_.find(name));	\
	return (i != names_.end()) ? i->second : NOT_PROPERTY;

/// Returns the General_Category with the given name.
inline int GeneralCategory::forName(const Char* name) {IMPLEMENT_FORNAME}

/// Returns General_Category value of the specified character.
inline int GeneralCategory::of(CodePoint cp) throw() {
	if(const internal::PropertyRange* p = internal::findInRange(ranges_, ranges_ + count_, cp))
		return p->property;
	return OTHER_UNASSIGNED;
}

/// Returns the Block with the given name.
inline int CodeBlock::forName(const Char* name) {IMPLEMENT_FORNAME}

/// Returns Block value of the specified character.
inline int CodeBlock::of(CodePoint cp) throw() {
	if(const internal::PropertyRange* p = internal::findInRange(ranges_, ranges_ + count_, cp))
		return p->property;
	return NO_BLOCK;
}

#ifndef ASCENSION_NO_UNICODE_NORMALIZATION
/// Returns the Canonical_Combining_Class with the given name.
inline int CanonicalCombiningClass::forName(const Char* name) {IMPLEMENT_FORNAME}

/// Returns the Canonical_Combining_Class of the specified character.
inline int CanonicalCombiningClass::of(CodePoint cp) throw() {
	if(cp < 0x10000) {
		const Char* const p = std::lower_bound(SRC_UCS2, SRC_UCS2 + UCS2_COUNT, static_cast<Char>(cp & 0xFFFFU));
		return (*p == cp) ? DEST_UCS2[p - SRC_UCS2] : NOT_REORDERED;
	} else {
		const CodePoint* const p = std::lower_bound(SRC_UCS4, SRC_UCS4 + UCS4_COUNT, cp);
		return (*p != cp) ? DEST_UCS4[p - SRC_UCS4] : NOT_REORDERED;
	}
}
#endif /* !ASCENSION_NO_UNICODE_NORMALIZATION */

/// Returns the Script with the given name.
inline int Script::forName(const Char* name) {IMPLEMENT_FORNAME}

/// Returns Script value of the specified character.
inline int Script::of(CodePoint cp) throw() {
	if(const internal::PropertyRange* p = internal::findInRange(ranges_, ranges_ + count_, cp))
		return p->property;
	return UNKNOWN;
}

/// Returns the Hangul_Syllable_Type with the given name.
inline int HangulSyllableType::forName(const Char* name) {IMPLEMENT_FORNAME}

/// Returns the Hangul syllable type property value of @a cp.
inline int HangulSyllableType::of(CodePoint cp) throw() {
	if(cp >= 0x1100 && cp <= 0x1159 || cp == 0x115F)
		return LEADING_JAMO;
	else if(cp >= 0x1160 && cp <= 0x11A2)
		return VOWEL_JAMO;
	else if(cp >= 0x11A8 && cp <= 0x11F9)
		return TRAILING_JAMO;
	else if(cp >= 0xAC00 && cp <= 0xD7A3)
		return ((cp - 0xAC00) % 28 == 0) ? LV_SYLLABLE : LVT_SYLLABLE;
	else
		return NOT_APPLICABLE;
}

#undef IMPLEMENT_FORNAME

}} // namespace unicode.ascension

#endif /* !ASCENSION_UNICODE_PROPERTY_HPP */
