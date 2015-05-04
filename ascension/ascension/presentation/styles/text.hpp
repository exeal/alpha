/**
 * @file text.hpp
 * @author exeal
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2011 was presentation.hpp
 * @date 2011-05-04 separated from presentation.hpp
 * @date 2012-07-16 reunioned with text-line-style.hpp
 * @date 2014-09-23 Separated from text-style.hpp
 */

#ifndef ASCENSION_STYLES_TEXT_HPP
#define ASCENSION_STYLES_TEXT_HPP
#include <ascension/corelib/future/scoped-enum-emulation.hpp>
#include <ascension/graphics/font/text-alignment.hpp>
#include <ascension/presentation/absolute-length.hpp>
#include <ascension/presentation/style-property.hpp>
#include <ascension/presentation/styles/length.hpp>
#include <ascension/presentation/styles/percentage.hpp>
#include <boost/functional/hash/extensions.hpp>
#include <boost/functional/hash/hash.hpp>
#include <boost/operators.hpp>
#if 0
#include <boost/optional.hpp>
#endif
#include <boost/variant.hpp>
#include <tuple>

namespace ascension {
	namespace presentation {
		namespace styles {
			/// @defgroup css_text_3 CSS Text Module Level 3
			/// @see CSS Text Module Level 3 - W3C Last Call Working Draft 10 October 2013
			///      (http://www.w3.org/TR/css3-text/)
			/// @{

			/// Enumerated values for @c TextTransform. The documentation of the members are copied from CSS 3.
			ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(TextTransformEnums)
				/// No effects.
				NONE,
				/// Puts the first letter of each word in titlecase; other characters are unaffected.
				CAPITALIZE,
				/// Puts all letters in uppercase.
				UPPERCASE,
				/// Puts all letters in lowercase.
				LOWERCASE,
				/// Puts all letters in fullwidth form. If the character does not have a corresponding fullwidth form,
				/// it is left as is. This value is typically used to typeset Latin characters and digits like
				/// ideographic characters.
				FULL_WIDTH/*,
				FULL_SIZE_KANA*/
			ASCENSION_SCOPED_ENUM_DECLARE_END(TextTransformEnums)

			/**
			 * [Copied from CSS3] This property transforms text for styling purposes.
			 * @see CSS Text Module Level 3, 2.1. Case Transforms: the ‘text-transform’ property
			 *      (http://www.w3.org/TR/css3-text/#text-transform-property)
			 * @see XSL 1.1, 7.17.6 "text-transform" (http://www.w3.org/TR/xsl/#text-transform)
			 */
			typedef StyleProperty<
				Enumerated<TextTransformEnums, TextTransformEnums::NONE>,
				Inherited<true>
			> TextTransform;

			/// Enumerated values for @c WhiteSpace. The documentation of the members are copied from CSS 3.
			ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(WhiteSpaceEnums)
				/// This value directs user agents to collapse sequences of white space into a single characer (or in
				/// some cases, no character). Lines may wrap at allowed soft wrap opportunities, as determined by the
				/// line-breaking rules in effect, in order to minimize inline-axis overflow.
				NORMAL		= 1 << 0 | 1 << 1 | 1 << 2,
				/// This value prevents user agents from collapsing sequences of white space. Segment breaks such as
				/// line feeds and carriage returns are preserved as forced line breaks. Lines only break at forced
				/// line breaks; content that does not fit within the block container overflows it.
				PRE			= 0 << 0 | 0 << 1 | 0 << 2,
				/// Like ‘normal’, this value collapses white space; but like ‘pre’, it does not allow wrapping.
				NOWRAP		= 1 << 0 | 1 << 1 | 0 << 2,
				/// Like ‘pre’, this value preserves white space; but like ‘normal’, it allows wrapping.
				PRE_WRAP	= 0 << 0 | 0 << 1 | 1 << 2,
				/// Like ‘normal’, this value collapses consecutive spaces and allows wrapping, but preserves segment
				/// breaks in the source as forced line breaks.
				PRE_LINE	= 0 << 0 | 1 << 1 | 1 << 2
			ASCENSION_SCOPED_ENUM_DECLARE_END(WhiteSpaceEnums)

			inline BOOST_CONSTEXPR bool collapsesNewLines(WhiteSpaceEnums value) BOOST_NOEXCEPT {
				return (boost::native_value(value) & (1 << 0)) != 0;
			}

			inline BOOST_CONSTEXPR bool collapsesSpacesAndTabs(WhiteSpaceEnums value) BOOST_NOEXCEPT {
				return (boost::native_value(value) & (1 << 1)) != 0;
			}

			inline BOOST_CONSTEXPR bool wrapsText(WhiteSpaceEnums value) BOOST_NOEXCEPT {
				return (boost::native_value(value) & (1 << 2)) != 0;
			}

			/**
			 * [Copied from CSS3] This property specifies two things: whether and how white space inside the element is
			 * collapsed, and whether lines may wrap at unforced soft wrap opportunities.
			 * @see CSS Text Module Level 3, 3. White Space and Wrapping: the ‘white-space’ property
			 *      (http://www.w3.org/TR/css3-text/#white-space)
			 */			
			typedef StyleProperty<
				Enumerated<WhiteSpaceEnums, WhiteSpaceEnums::NORMAL>,
				Inherited<true>
			> WhiteSpace;

			/**
			 * [Copied from CSS3] This property determines the tab size used to render preserved tab characters
			 * (U+0009). Integers represent the measure as multiples of the space character's advance width (U+0020).
			 * @see CSS Text Module Level 3, 4.2. Tab Character Size: the ‘tab-size’ property
			 *      (http://www.w3.org/TR/css3-text/#tab-size-property)
			 */
			typedef StyleProperty<
				Multiple<
					boost::variant<Integer, Length>,
					Integer, 8
				>,
				Inherited<true>
			> TabSize;

			/// Enumerated values for @c LineBreak. The documentation of the members are copied from CSS 3.
			ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(LineBreakEnums)
				/// The UA determines the set of line-breaking restrictions to use, and it may vary the restrictions
				/// based on the length of the line; e.g., use a less restrictive set of line-break rules for short
				/// lines.
				AUTO,
				/// Breaks text using the least restrictive set of line-breaking rules. Typically used for short lines,
				/// such as in newspapers.
				LOOSE,
				/// Breaks text using the most common set of line-breaking rules.
				NORMAL,
#ifdef STRICT
#	define ASCENSION_DEFINED_STRICT STRICT
#	undef STRICT
#endif
				/// Breaks text using the most stringest set of line-breaking rules.
				STRICT
#ifdef ASCENSION_DEFINED_STRICT
#	define STRICT ASCENSION_DEFINED_STRICT
#	undef ASCENSION_DEFINED_STRICT
#endif
			ASCENSION_SCOPED_ENUM_DECLARE_END(LineBreakEnums)

			/**
			 * [Copied from CSS3] This property specifies the strictness of line-breaking rules applied
			 * within an element: particularly how wrapping interacts with punctuation and symbols.
			 * @see CSS Text Module Level 3, 5.2. Breaking Rules for Punctuation: the ‘line-break’ property
			 *      (http://www.w3.org/TR/css3-text/#line-break-property)
			 */
			typedef StyleProperty<
				Enumerated<LineBreakEnums, LineBreakEnums::AUTO>,
				Inherited<true>
			> LineBreak;

			/// Enumerated values for @c WordBreak. The documentation of the members are copied from CSS 3.
			ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(WordBreakEnums)
				/// Words break according to their usual rules.
				NORMAL,
				/// In addition to ‘normal’ soft wrap opportunities, lines may break between any two letters (except
				/// where forbidden by the ‘line-break’ property). Hyphenation is not applied. This option is used
				/// mostly in a context where the text is predominantly using CJK characters with few non-CJK excerpts
				/// and it is desired that the text be better distributed on each line.
				BREAK_ALL,
				/// Implicit soft wrap opportunities between letters are suppressed, i.e. breaks are prohibited between
				/// pairs of letters (including those explicitly allowed by ‘line-break’) except where opportunities
				/// exist due to dictionary-based breaking. Otherwise this option is equivalent to ‘normal’. In this
				/// style, sequences of CJK characters do not break.
				KEEP_ALL
			ASCENSION_SCOPED_ENUM_DECLARE_END(WordBreakEnums)

			/**
			 * [Copied from CSS3] This property specifies soft wrap opportunities between letters, i.e. where it is
			 * “normal” and permissible to break lines of text.
			 * @see CSS Text Module Level 3, 5.3. Breaking Rules for Letters: the ‘word-break’ property
			 *      (http://www.w3.org/TR/css3-text/#word-break-property)
			 */
			typedef StyleProperty<
				Enumerated<WordBreakEnums, WordBreakEnums::NORMAL>,
				Inherited<true>
			> WordBreak;

			/// Enumerated values for @c Hyphens. The documentation of the members are copied from CSS 3.
			ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(HyphensEnums)
				/// Words are not hyphenated, even if characters inside the word explicitly define hyphenation
				/// opportunities.
				NONE,
				/// Words are only hyphenated where there are characters inside the word that explicitly suggest
				/// hyphenation opportunities.
				MANUAL,
				/// Words may be broken at appropriate hyphenation points either as determined by hyphenation
				/// characters inside the word or as determined automatically by a language-appropriate hyphenation
				/// resource. Conditional hyphenation characters inside a word, if present, take priority over
				/// automatic resources when determining hyphenation opportunities within the word.
				AUTO
			ASCENSION_SCOPED_ENUM_DECLARE_END(HyphensEnums)

			/**
			 * [Copied from CSS3] This property controls whether hyphenation is allowed to create more soft wrap
			 * opportunities within a line of text.
			 * @see CSS Text Module Level 3, 6.1. Hyphenation Control: the ‘hyphens’ property
			 *      (http://www.w3.org/TR/css3-text/#hyphens)
			 * @see XSL 1.1, 7.10 Common Hyphenation Properties
			 *      (http://www.w3.org/TR/xsl/#common-hyphenation-properties)
			 */
			typedef StyleProperty<
				Enumerated<HyphensEnums, HyphensEnums::MANUAL>,
				Inherited<true>
			> Hyphens;
			
			/// Enumerated values for @c OverflowWrap. The documentation of the members are copied from CSS 3.
			ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(OverflowWrapEnums)
				/// Lines may break only at allowed break points. However, the restrictions introduced by ‘word-break:
				/// keep-all’ may be relaxed to match ‘word-break: normal’ if there are no otherwise-acceptable break
				/// points in the line.
				NORMAL,
				/// An unbreakable "word" may be broken at an arbitrary point if there are no otherwise-acceptable
				/// break points in the line. Shaping characters are still shaped as if the word were not broken, and
				/// grapheme clusters must together stay as one unit. No hyphenation character is inserted at the break
				/// point.
				BREAK_WORD/*,
				HYPHENATE*/
			ASCENSION_SCOPED_ENUM_DECLARE_END(OverflowWrapEnums)

			/**
			 * [Copied from CSS3] This property specifies whether the UA may arbitrarily break within a word to prevent
			 * overflow when an otherwise-unbreakable string is too long to fit within the line box. It only has an
			 * effect when ‘white-space’ allows wrapping.
			 * @see CSS Text Module Level 3 - 6.2. Overflow Wrapping: the ‘word-wrap’/‘overflow-wrap’ property
			 *      (http://www.w3.org/TR/css3-text/#overflow-wrap-property)
			 */
			typedef StyleProperty<
				Enumerated<OverflowWrapEnums, OverflowWrapEnums::NORMAL>,
				Inherited<true>
			> OverflowWrap;

			/// @see graphics#font#TextAlignment
			typedef StyleProperty<
				Enumerated<graphics::font::TextAlignment, graphics::font::TextAlignment::START>,
				Inherited<true>
			> TextAlignment;

			using graphics::font::TextAnchor;
			
			/// Enumerated values for @c TextAlignmentLast. The documentation of the members are copied from CSS 3.
			ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(TextAlignmentLastEnums)
				START = graphics::font::TextAlignment::START,
				CENTER = graphics::font::TextAlignment::CENTER,
				END = graphics::font::TextAlignment::END,
				LEFT = graphics::font::TextAlignment::LEFT,
				RIGHT = graphics::font::TextAlignment::RIGHT,
				JUSTIFY = graphics::font::TextAlignment::JUSTIFY,
				AUTO = graphics::font::TextAlignment::START_END + 1
			ASCENSION_SCOPED_ENUM_DECLARE_END(TextAlignmentLastEnums)

			/**
			 * [Copied from CSS3] This property describes how the last line of a block or a line right before a forced
			 * line break is aligned when ‘text-align’ is ‘justify’. If a line is also the first line of the block or
			 * the first line after a forced line break, then, unless ‘text-align’ assigns an explicit first line
			 * alignment (via ‘start end’), ‘text-align-last’ takes precedence over ‘text-align’.
			 *
			 * If ‘auto’ is specified, content on the affected line is aligned per ‘text-align’ unless ‘text-align’ is
			 * set to ‘justify’. In this case, content is justified if ‘text-justify’ is ‘distribute’ and start-aligned
			 * otherwise. All other values have the same meanings as in ‘text-align’.
			 * @see CSS Text Module Level 3, 7.2. Last Line Alignment: the ‘text-align-last’ property
			 *      (http://www.w3.org/TR/css3-text/#text-align-last-property)
			 * @see XSL 1.1, 7.16.10 "text-align-last" (http://www.w3.org/TR/xsl/#text-align-last)
			 * @note The name of this type is @c TextAlignmentLast, not @c TextAlignLast.
			 */
			typedef StyleProperty<
				Enumerated<TextAlignmentLastEnums, TextAlignmentLastEnums::AUTO>,
				Inherited<true>
			> TextAlignmentLast;

			class Presentation;
			TextAnchor defaultTextAnchor(const Presentation& presentation);
			
			/// @see graphics#font#TextJustification
			typedef StyleProperty<
				Enumerated<graphics::font::TextJustification, graphics::font::TextJustification::AUTO>,
				Inherited<true>
			> TextJustification;

			/**
			 * [Copied from XSL11] A space-specifier is a compound datatype whose components are minimum, optimum,
			 * maximum, conditionality, and precedence.
			 * @tparam T The type of @c #optimum, @c #minimum and @c #maximum data members
			 * @see XSL 1.1, 4.3 Spaces and Conditionality (http://www.w3.org/TR/xsl/#spacecond)
			 * @note CSS Text Module Level 3 dropped this compound datatype in Last Call.
			 */
			template<typename T>
			struct SpacingLimit : private boost::equality_comparable<SpacingLimit<T>> {
				typedef T value_type;
				value_type optimum;	///< 'optimum' member.
				value_type minimum;	///< 'minimum' member.
				value_type maximum;	///< 'maximum' member.
#if 0
				// followings are defined in only XSL 1.1
				boost::optional<int> precedence;	// boost.none means 'force'
				enum Conditionality {DISCARD, RETAIN} conditionality;
#endif
				/// Default constructor initializes nothing.
				SpacingLimit() {}
				template<typename U>
				explicit SpacingLimit(const U& allValues)
						: optimum(allValues), minimum(allValues), maximum(allValues) {}
				template<typename OptimumAndMinimum, typename Maximum>
				SpacingLimit(const OptimumAndMinimum& optimumAndMinimum, const Maximum& maximum)
						: optimum(optimumAndMinimum), minimum(optimumAndMinimum), maximum(maximum) {}
				template<typename Optimum, typename Minimum, typename Maximum>
				SpacingLimit(const Optimum& optimum, const Minimum& minimum, const Maximum& maximum)
						: optimum(optimum), minimum(minimum), maximum(maximum) {}
				template<typename U>
				SpacingLimit& operator=(const U& allValues) {
					optimum = minimum = maximum = allValues;
					return *this;
				}
				template<typename OptimumAndMinimum, typename Maximum>
				SpacingLimit& operator=(const std::tuple<OptimumAndMinimum, Maximum>& other) {
					this->optimum = this->minimum = std::get<0>(other);
					this->maximum = std::get<1>(other);
					return *this;
				}
				template<typename Optimum, typename Minimum, typename Maximum>
				SpacingLimit& operator=(const std::tuple<Optimum, Minimum, Maximum>& other) {
					this->optimum = std::get<0>(other);
					this->minimum = std::get<1>(other);
					this->maximum = std::get<2>(other);
					return *this;
				}
				/// Equality operator.
				bool operator==(const SpacingLimit& other) const {
					return optimum == other.optimum && minimum == other.minimum && maximum == other.maximum;
				}
			};

			/**
			 * [Copied from CSS3] This property specifies additional spacing between “words”. Missing values are
			 * assumed to be ‘normal’.
			 * @c std#tuple&lt;&gt; means 'normal' keyword.
			 * @see CSS Text Level 3, 8.1. Word Spacing: the ‘word-spacing’ property
			 *      (http://www.w3.org/TR/css3-text/#word-spacing)
			 * @see SVG 1.1 (Second Edition), 10.11 Spacing properties
			 *      (http://www.w3.org/TR/SVG11/text.html#WordSpacingProperty)
			 * @see XSL 1.1, 7.17.8 "word-spacing" (http://www.w3.org/TR/xsl/#word-spacing)
			 */
#if 1
			typedef StyleProperty<
				MultipleWithInitialIndex<
					boost::variant<std::tuple<>, Length, Percentage>,
					boost::mpl::int_<0>
				>,
				Inherited<true>,
				boost::variant<Length, Percentage>
			> WordSpacing;

			template<>
			inline SpecifiedValue<WordSpacing>::type uncompute<WordSpacing>(const ComputedValue<WordSpacing>::type& computedValue) {
				if(const Length* const length = boost::get<Length>(&computedValue))
					return *length;
				else if(const Percentage* const percentage = boost::get<Percentage>(&computedValue))
					return *percentage;
				else
					throw UnknownValueException("computedValue");
			}
#else
			typedef SpacingLimit<
				StyleProperty<
					Complex<boost::optional<Length>>,
					Inherited<true>
				>
			> WordSpacing;
#endif

			/**
			 * [Copied from CSS3] This property specifies additional spacing between adjacent characters (commonly
			 * called <strong>tracking</strong>).
			 * @see CSS Text Level 3, 8.2. Word Spacing: the ‘letter-spacing’ property
			 *      (http://www.w3.org/TR/css3-text/#letter-spacing)
			 * @see SVG 1.1 (Second Edition), 10.11 Spacing properties
			 *      (http://www.w3.org/TR/SVG11/text.html#LetterSpacingProperty)
			 * @see XSL 1.1, 7.17.2 "letter-spacing" (http://www.w3.org/TR/xsl/#letter-spacing)
			 */
#if 1
			typedef StyleProperty<
				Complex<boost::optional<Length>>,	// boost.none means 'normal' keyword
				Inherited<true>,
				Length
			> LetterSpacing;

			template<>
			inline SpecifiedValue<LetterSpacing>::type uncompute<LetterSpacing>(const ComputedValue<LetterSpacing>::type& computedValue) {
				return boost::make_optional(computedValue);
			}
#else
			typedef SpacingLimit<
				StyleProperty<
					Complex<boost::optional<Length>>,
					Inherited<true>
				>
			> LetterSpacing;
#endif

			/**
			 * Basic type of @c TextIndent.
			 * @tparam LengthType The type of @c #length data member. Usually @c Length or @c Scalar
			 * @tparam BooleanType The type of @c #eachLine and @c #hanging data members. Usually @c bool
			 */
			template<typename LengthType, typename BooleanType>
			struct BasicTextIndent : private boost::equality_comparable<BasicTextIndent<LengthType, BooleanType>> {
				/**
				 * [Copied from CSS3] Gives the amount of the indent as an absolute length. If this is in percentage,
				 * as a percentage of the containing block's logical width.
				 */
				LengthType length;
				/// [Copied from CSS3] Inverts which lines are affected.
				BooleanType hanging;
				/**
				 * [Copied from CSS3] Indentation affects the first line of the block container as well as each line
				 * after a forced line break, but does not affect lines after a soft wrap break.
				 */
				BooleanType eachLine;
				/**
				 * Default constructor initializes @c #length by calling the default constructor of @c LengthType, and
				 * @c #hanging and @c #eachLine with false.
				 */
				BasicTextIndent() : length(), hanging(false), eachLine(false) {}
				/**
				 * Creates a @c BasicTextIndent instance with the given values.
				 * @param length The initial value of the @c #length member
				 * @param hanging The initial value of the @c #hanging member
				 * @param eachLine The initial value of the @c #eachLine member
				 */
				BasicTextIndent(const LengthType& length, BooleanType hanging, BooleanType eachLine)
					: length(length), hanging(hanging), eachLine(eachLine) {}
				/// Equality operator.
				bool operator==(const BasicTextIndent<LengthType, BooleanType>& other) const {
					return length == other.length && hanging == other.hanging && eachLine == other.eachLine;
				}
			};

			/// Specialization of @c boost#hash_value function template for @c TextIndent.
			template<typename LengthType, typename BooleanType>
			inline std::size_t hash_value(const BasicTextIndent<LengthType, BooleanType>& object) {
				std::size_t seed = 0;
				boost::hash_combine(seed, object.length);
				boost::hash_combine(seed, object.hanging);
				boost::hash_combine(seed, object.eachLine);
				return seed;
			}

			/**
			 * [Copied from CSS3] This property specifies the indentation applied to lines of inline content in a
			 * block. The indent is treated as a margin applied to the start edge of the line box. Unless otherwise
			 * specified via the ‘each-line’ and/or ‘hanging’ keywords, only lines that are the first formatted line of
			 * an element are affected. For example, the first line of an anonymous block box is only affected if it is
			 * the first child of its parent element.
			 * @tparam LengthType The type of @c #length data member. Usually @c Length or @c Scalar
			 * @tparam BooleanType The type of @c #hanging and @c #eachLine data members
			 * @see CSS Text Level 3, 9.1. First Line Indentation: the ‘text-indent’ property
			 *      (http://www.w3.org/TR/css3-text/#text-indent-property)
			 * @see XSL 1.1, 7.16.11 "text-indent" (http://www.w3.org/TR/xsl/#text-indent)
			 */
			typedef StyleProperty<
				Complex<
					BasicTextIndent<boost::variant<Length, Percentage>, bool>
				>,
				Inherited<true>
			> TextIndent;
			
			/// Enumerated values for @c HangingPunctuation. The documentation of the members are copied from CSS 3.
			ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(HangingPunctuationEnums)
				// TODO: Some values should be able to be combined by bitwise-OR.
				/// No character hangs.
				NONE,
				/// An opening bracket or quote at the start of the first formatted line of an element hangs. This
				/// applies to all characters in the Unicode categories Ps, Pf, Pi.
				FIRST,
				/// A closing bracket or quote at the end of the <em>last formatted</em> line of an element hangs. This
				/// applies to all characters in the Unicode categories Pe, Pf, Pi.
				LAST,
				/// A stop or comma at the end of a line hangs.
				FORCE_END,
				/// A stop or comma at the end of a line hangs if it does not otherwise fit prior to justification.
				ALLOW_END
			ASCENSION_SCOPED_ENUM_DECLARE_END(HangingPunctuationEnums)

			/**
			 * [Copied from CSS3] This property determines whether a punctuation mark, if one is present, hangs and may
			 * be placed outside the line box (or in the indent) at the start or at the end of a line of text.
			 * @see CSS Text Level 3, 9.2. Hanging Punctuation: the ‘hanging-punctuation’ property
			 *      (http://www.w3.org/TR/css3-text/#hanging-punctuation-property)
			 */
			typedef StyleProperty<
				Enumerated<HangingPunctuationEnums, HangingPunctuationEnums::NONE>,
				Inherited<true>
			> HangingPunctuation;
			/// @}
		}
	}
}

namespace std {
	/// Specialization of @c std#hash class template for @c TextIndent.
	template<typename LengthType, typename BooleanType>
	class hash<ascension::presentation::styles::BasicTextIndent<LengthType, BooleanType>> :
		public std::function<std::hash<void*>::result_type(const ascension::presentation::styles::BasicTextIndent<LengthType, BooleanType>&)> {
	public:
		std::size_t operator()(const ascension::presentation::styles::BasicTextIndent<LengthType, BooleanType>& key) const BOOST_NOEXCEPT {
			return boost::hash<argument_type>()(key);
		}
	};
}

#endif // !ASCENSION_STYLES_TEXT_HPP
