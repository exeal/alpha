/**
 * @file text-style.hpp
 * @author exeal
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2011 was presentation.hpp
 * @date 2011-05-04 separated from presentation.hpp
 * @date 2012
 */

#ifndef ASCENSION_TEXT_STYLE_HPP
#define ASCENSION_TEXT_STYLE_HPP

#include <ascension/graphics/color.hpp>	// graphics.Color
#include <ascension/graphics/font.hpp>	// graphics.font.FontProperties, ...
#include <ascension/graphics/paint.hpp>	// graphics.Paint
#include <ascension/presentation/inheritable.hpp>
#include <ascension/presentation/length.hpp>
#include <ascension/presentation/writing-mode.hpp>
#include <map>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/optional.hpp>

namespace ascension {
	namespace presentation {
#if 0
		struct Space {
			Length minimum, optimum, maximum;
			boost::optional<int> precedence;
			enum Conditionality {DISCARD, RETAIN} conditionality;
		};
#else
		typedef Length Space;
#endif

		/**
		 * @see "CSS Backgrounds and Borders Module Level 3"
		 *      (http://www.w3.org/TR/2011/CR-css3-background-20110215/)
		 */
		struct Border {
			/**
			 * @see 
			 */
			enum Style {
				NONE, HIDDEN, DOTTED, DASHED, SOLID,
				DOT_DASH, DOT_DOT_DASH,
				DOUBLE, GROOVE, RIDGE, INSET, OUTSET
			};
			static const Length THIN, MEDIUM, THICK;
			struct Part {
				/**
				 * The foreground color of the border. Default value is Color() which means that
				 * the value of @c TextRunStyle#foreground
				 */
				graphics::Color color;
				/// Style of the border. Default value is @c NONE.
				Style style;
				/// Thickness of the border. Default value is @c MEDIUM.
				Length width;

				/// Default constructor.
				Part() : color(), style(NONE), width(MEDIUM) {}
				/// Returns the computed width.
				Length computedWidth() const {return (style != NONE) ? width : Length(0.0, width.unitType());}
				/// Returns @c true if this part is invisible (but may be consumes place).
				bool hasVisibleStyle() const /*throw()*/ {return style != NONE && style != HIDDEN;}
			};
			AbstractFourSides<Part> sides;
		};

		/**
		 * Dominant baselines from XSL 1.1, 7.14.5 "dominant-baseline".
		 * @see "CSS3 module: line, 4.4. Dominant baseline: the 'dominant-baseline' property"
		 *      (http://www.w3.org/TR/css3-linebox/#dominant-baseline-prop)
		 * @see "SVG 1.1 (Second Edition), 10.9.2 Baseline alignment properties"
		 *      (http://www.w3.org/TR/SVG/text.html#DominantBaselineProperty)
		 */
		enum DominantBaseline {
			DOMINANT_BASELINE_AUTO,
			DOMINANT_BASELINE_USE_SCRIPT,
			DOMINANT_BASELINE_NO_CHANGE,
			DOMINANT_BASELINE_RESET_SIZE,
			DOMINANT_BASELINE_IDEOGRAPHIC,
			DOMINANT_BASELINE_ALPHABETIC,
			DOMINANT_BASELINE_HANGING,
			DOMINANT_BASELINE_MATHEMATICAL,
			DOMINANT_BASELINE_CENTRAL,
			DOMINANT_BASELINE_MIDDLE,
			DOMINANT_BASELINE_TEXT_AFTER_EDGE,
			DOMINANT_BASELINE_TEXT_DEFORE_EDGE,
		};

		/**
		 * Alignment baseline from XSL 1.1, 7.14.2 "alignment-baseline".
		 * @see "CSS3 module: line, 4.5. Aligning the alignment point of an element: the
		 *      'alignment-baseline' property"
		 *      (http://www.w3.org/TR/css3-linebox/#alignment-baseline-prop)
		 * @see "SVG 1.1 (Second Edition), 10.9.2 Baseline alignment properties"
		 *      (http://www.w3.org/TR/SVG/text.html#AlignmentBaselineProperty)
		 */
		enum AlignmentBaseline {
			ALIGNMENT_BASELINE_BASELINE,
			ALIGNMENT_BASELINE_USE_SCRIPT,
			ALIGNMENT_BASELINE_BEFORE_EDGE,
			ALIGNMENT_BASELINE_TEXT_BEFORE_EDGE,
			ALIGNMENT_BASELINE_AFTER_EDGE,
			ALIGNMENT_BASELINE_TEXT_AFTER_EDGE,
			ALIGNMENT_BASELINE_CENTRAL,
			ALIGNMENT_BASELINE_MIDDLE,
			ALIGNMENT_BASELINE_IDEOGRAPHIC,
			ALIGNMENT_BASELINE_ALPHABETIC,
			ALIGNMENT_BASELINE_HANGING,
			ALIGNMENT_BASELINE_MATHEMATICAL
		};
#if 0
		enum AlignmentAdjustEnums {
			ALIGNMENT_ADJUST_AUTO,
			ALIGNMENT_ADJUST_BASELINE,
			ALIGNMENT_ADJUST_BEFORE_EDGE,
			ALIGNMENT_ADJUST_TEXT_BEFORE_EDGE,
			ALIGNMENT_ADJUST_MIDDLE,
			ALIGNMENT_ADJUST_CENTRAL,
			ALIGNMENT_ADJUST_AFTER_EDGE,
			ALIGNMENT_ADJUST_TEXT_AFTER_EDGE,
			ALIGNMENT_ADJUST_IDEOGRAPHIC,
			ALIGNMENT_ADJUST_ALPHABETIC,
			ALIGNMENT_ADJUST_HANGING,
			ALIGNMENT_ADJUST_MATHEMATICAL
		};

		/**
		 * @see "SVG 1.1 (Second Edition), 10.9.2 Baseline alignment properties"
		 *      (http://www.w3.org/TR/SVG/text.html#BaselineShiftProperty)
		 */
		enum BaselineShiftEnums {
			BASELINE_SHIFT_BASELINE,
			BASELINE_SHIFT_SUB,
			BASELINE_SHIFT_SUPER
		};
#endif
		struct Decorations {
			enum Style {NONE, SOLID, DOTTED, DAHSED};
			struct Part {
				graphics::Color color;	// if is Color(), same as the foreground
				Inheritable<Style> style;	///< Default value is @c NONE.
				/// Default constructor.
				Part() : style(NONE) {}
			} overline, strikethrough, baseline, underline;
		};

		enum TextTransform {
			CAPITALIZE, UPPERCASE, LOWERCASE, NONE
		};

		/**
		 * Visual style settings of a text run.
		 * @see TextLineStyle, TextToplevelStyle, StyledTextRun, StyledTextRunIterator
		 */
		struct TextRunStyle :
				public FastArenaObject<TextRunStyle>, public std::enable_shared_from_this<TextRunStyle> {
			/// Foreground color.
			graphics::Paint foreground;
			/// Background color.
			graphics::Paint background;
			/// Border of the text run. See the description of @c Border.
			Border border;
			/// Font family name. An empty string means inherit the parent.
			String fontFamily;	// TODO: replace with graphics.font.FontFamilies.
			/// Font properties. See @c graphics#FontProperties.
			graphics::font::FontProperties<Inheritable> fontProperties;
			/// 'font-size-adjust' property. 0.0 means 'none', negative value means 'inherit'.
			double fontSizeAdjust;
			/// The dominant baseline of the line. Default value is @c DOMINANT_BASELINE_AUTO.
			Inheritable<DominantBaseline> dominantBaseline;
			/// The alignment baseline. Default value is @c ALIGNMENT_BASELINE_AUTO.
			AlignmentBaseline alignmentBaseline;
			std::locale locale;
			/// Typography features applied to the text. See the description of @c TypographyProperties.
			std::map<graphics::font::TrueTypeFontTag, uint32_t> typographyProperties;
			Decorations decorations;
			/// Letter spacing in DIP. Default is 0.
			Length letterSpacing;
			/// Word spacing in DIP. Default is 0.
			Length wordSpacing;
//			Inheritable<TextTransform> textTransform;
//			RubyProperties rubyProperties;
//			Effects effects;
			/// Set @c false to disable shaping. Default is @c true.
			bool shapingEnabled;

			/// Default constructor initializes the members by their default values.
			TextRunStyle() : letterSpacing(0), wordSpacing(0), shapingEnabled(true) {}
			TextRunStyle& resolveInheritance(const TextRunStyle& base, bool baseIsRoot);
		};

		/**
		 * Represents a styled text run, with the beginning position (offset) in the line and the
		 * style.
		 * @note This class does not provides the length of the text run.
		 * @note This class is not intended to be derived.
		 * @see StyledTextRunIterator, StyledTextRunEnumerator
		 */
		class StyledTextRun {
		public:
//			/// Default constructor.
//			StyledTextRun() /*throw()*/ : position_(INVALID_INDEX) {}
			/**
			 * Constructor.
			 * @param position The beginning position of the text style
			 * @param style The style of the text run
			 */
			StyledTextRun(Index position,
				std::shared_ptr<const TextRunStyle> style) /*throw()*/ : position_(position), style_(style) {}
			/// Returns the position in the line of the text range which the style applies.
			Index position() const /*throw()*/ {return position_;}
			/// Returns the style of the text run.
			std::shared_ptr<const TextRunStyle> style() const /*throw()*/ {return style_;}
		private:
			Index position_;
			std::shared_ptr<const TextRunStyle> style_;
		};

		/**
		 *
		 * @see StyledTextRunEnumerator
		 */
		class StyledTextRunIterator {
		public:
			/// Destructor.
			virtual ~StyledTextRunIterator() /*throw()*/ {}
			/// Returns the current styled text run or throws @c NoSuchElementException.
			virtual StyledTextRun current() const = 0;
			/// Returns @c false if the iterator addresses the end of the range.
			virtual bool hasNext() const = 0;
			/// Moves the iterator to the next styled run or throws @c NoSuchElementException.
			virtual void next() = 0;
		};

		/**
		 *
		 * @see StyledTextRunIterator
		 */
		class StyledTextRunEnumerator : public boost::iterator_facade<
			StyledTextRunEnumerator, std::pair<Range<Index>, std::shared_ptr<const TextRunStyle>>,
			std::input_iterator_tag, std::pair<Range<Index>, std::shared_ptr<const TextRunStyle>>,
			std::ptrdiff_t
		> {
		public:
			StyledTextRunEnumerator();
			StyledTextRunEnumerator(std::unique_ptr<StyledTextRunIterator> sourceIterator, Index end);
		private:
			friend class boost::iterator_core_access;
			const reference dereference() const;
			bool equal(const StyledTextRunEnumerator& other) const /*throw()*/;
			void increment();
		private:
			std::unique_ptr<StyledTextRunIterator> iterator_;
			boost::optional<StyledTextRun> current_, next_;
			const Index end_;
		};

		/**
		 * @see CSS Text Level 3 - 4.1. Line Breaking Strictness: the 'line-break' property
		 *      (http://www.w3.org/TR/css3-text/#line-break)
		 */
		enum LineBreak {
			LINE_BREAK_AUTO,
			LINE_BREAK_LOOSE,
			LINE_BREAK_NORMAL,
			LINE_BREAK_STRICT
		};

		/**
		 * @see CSS Text Level 3 - 4.2. Word Breaking Rules: the 'word-break' property
		 *      (http://www.w3.org/TR/css3-text/#word-break)
		 */
		enum WordBreak {
			WORD_BREAK_NORMAL,
			WORD_BREAK_KEEP_ALL,
			WORD_BREAK_BREAK_ALL
		};

		/**
		 * @see CSS Text Level 3 - 6.1. Text Wrap Settings: 'text-wrap' property
		 *      (http://www.w3.org/TR/css3-text/#text-wrap)
		 */
		enum TextWrap {
			TEXT_WRAP_NORMAL,
			TEXT_WRAP_NONE,
			TEXT_WRAP_AVOID
		};

		/**
		 * @see CSS Text Level 3 - 6.2. Emergency Wrapping: the 'overflow-wrap' property
		 *      (http://www.w3.org/TR/css3-text/#overflow-wrap)
		 */
		enum OverflowWrap {
			OVERFLOW_WRAP_NORMAL = 0,
			OVERFLOW_WRAP_BREAK_WORD = 1,
			OVERFLOW_WRAP_HYPHENATE = 2
		};

		template<typename Measure, bool inheritable>
		struct TextWrappingBase {
			typename InheritableIf<inheritable, TextWrap>::Type textWrap;
			typename InheritableIf<inheritable, OverflowWrap>::Type overflowWrap;
			Measure measure;
			/// Default constructor.
			TextWrappingBase() : textWrap(TEXT_WRAP_NONE), overflowWrap(OVERFLOW_WRAP_NORMAL), measure(0) {}
		};

		template<typename Measure>
		struct TextWrapping : public TextWrappingBase<Measure, false> {};
		template<typename Measure>
		struct Inheritable<TextWrapping<Measure>> : public TextWrappingBase<Measure, true> {};

		/**
		 * @c TextAnchor describes an alignment of text relative to the given point.
		 * @see resolveTextAlignment, TextLineStyle#alignment, TextLineStyle#lastSublineAlignment,
		 *      defaultTextAnchor
		 * @see XSL 1.1, 7.16.9 "text-align"
		 *      (http://www.w3.org/TR/2006/REC-xsl11-20061205/#text-align)
		 * @see "CSS Text Level 3, 7.1. Text Alignment: the 'text-align' property"
		 *      (http://www.w3.org/TR/2010/WD-css3-text-20101005/#text-align)
		 * @see "SVG 1.1, 10.9.1 Text alignment properties"
		 *      (http://www.w3.org/TR/SVG/text.html#TextAlignmentProperties)
		 */
		enum TextAnchor {
			/// The text is aligned to the start edge of the paragraph.
			TEXT_ANCHOR_START,
			/// The text is aligned to the middle (center) of the paragraph.
			TEXT_ANCHOR_MIDDLE,
			/// The text is aligned to the end edge of the paragraph.
			TEXT_ANCHOR_END
			/// Inherits the parent's setting.
			/// @note Some methods which take @c TextAnchor don't accept this value.
//			MATCH_PARENT_DIRECTION
		};

		class Presentation;
		TextAnchor defaultTextAnchor(const Presentation& presentation);

		/**
		 * @c TextJustification describes the justification method.
		 * @note This definition is under construction.
		 * @see "CSS Text Level 3, 7.3. Justification Method: the 'text-justify' property"
		 *      (http://www.w3.org/TR/2011/WD-css3-text-20110901/#text-justify)
		 */
		enum TextJustification {
			/// Specifies no justification.
			AUTO_JUSTIFICATION, NONE_JUSTIFICATION, INTER_WORD, INTER_IDEOGRAPH, INTER_CLUSTER, DISTRIBUTE, KASHIDA
		};

		/**
		 * @see XSL 1.1, 7.16.5 "line-height-shift-adjustment"
		 *      (http://www.w3.org/TR/xsl/#line-height-shift-adjustment)
		 */
		enum LineHeightShiftAdjustment {
			CONSIDER_SHIFTS, DISREGARD_SHIFTS
		};

		/**
		 * [Copied from XSL 1.1 documentation] Selects the strategy for positioning adjacent lines,
		 * relative to each other.
		 * @see XSL 1.1, 7.16.6 "line-stacking-strategy"
		 *      (http://www.w3.org/TR/xsl/#line-stacking-strategy)
		 */
		enum LineStackingStrategy {
			/// [Copied from XSL 1.1 documentation] Uses the per-inline-height-rectangle.
			LINE_HEIGHT,
			/// [Copied from XSL 1.1 documentation] Uses the nominal-requested-line-rectangle.
			FONT_HEIGHT,
			/// [Copied from XSL 1.1 documentation] Uses the maximal-line-rectangle.
			MAX_HEIGHT
		};

		struct NumberSubstitution {
			/// Specifies how to apply number substitution on digits and related punctuation.
			enum Method {
				/// Uses the user setting.
				USER_SETTING,
				/**
				 * The substitution method should be determined based on the system setting for
				 * the locale given in the text.
				 */
				FROM_LOCALE,
				/**
				 * The number shapes depend on the context (the nearest preceding strong character,
				 * or the reading direction if there is none).
				 */
				CONTEXTUAL,
				/**
				 * No substitution is performed. Characters U+0030..0039 are always rendered as
				 * nominal numeral shapes (European numbers, not Arabic-Indic digits).
				 */
				NONE,
				/// Numbers are rendered using the national number shapes.
				NATIONAL,
				/// Numbers are rendered using the traditional shapes for the specified locale.
				TRADITIONAL
			} method;	///< The substitution method. Default value is @c USER_SETTING.
			/// The name of the locale to be used.
			std::string localeName;
			/// Whether to ignore user override. Default value is @c false.
			bool ignoreUserOverride;

			/// Default constructor.
			NumberSubstitution() /*throw()*/ : method(USER_SETTING), ignoreUserOverride(false) {}
		};

		/**
		 * Specifies the style of a text line. This object also gives the default text run style.
		 * @see TextRunStyle, TextToplevelStyle, TextLineStyleDirector
		 */
		template<bool inheritable>
		struct TextLineStyleBase {
			/// The default text run style. The default value is @c null.
			std::shared_ptr<const TextRunStyle> defaultRunStyle;
			/// The line breaking strictness. The default value is @c LINE_BREAK_AUTO.
			typename InheritableIf<inheritable, LineBreak>::Type lineBreak;
			/// The word breaking rules. The default value is @c WORD_BREAK_NORMAL.
			typename InheritableIf<inheritable, WordBreak>::Type wordBreak;
			/**
			 * The alignment point in inline-progression-dimension. The default value is
			 * @c TEXT_ANCHOR_START.
			 */
			typename InheritableIf<inheritable, TextAnchor>::Type anchor;
			/**
			 * The alignment point in block-progression-dimension, which is the dominant baseline
			 * of the line. The default value is @c DOMINANT_BASELINE_AUTO.
			 */
			typename InheritableIf<inheritable, DominantBaseline>::Type dominantBaseline;
			/// The default value is @c CONSIDER_SHIFTS.
			typename InheritableIf<inheritable, LineHeightShiftAdjustment>::Type lineHeightShiftAdjustment;
			/// The line stacking strategy. The default value is @c MAX_HEIGHT.
			typename InheritableIf<inheritable, LineStackingStrategy>::Type lineStackingStrategy;
			/// The text justification method. The default value is @c AUTO_JUSTIFICATION.
			typename InheritableIf<inheritable, TextJustification>::Type textJustification;
			/// The number substitution process. The default value is @c NumberSubstitution().
			NumberSubstitution numberSubstitution;

			/// Default constructor initializes the all members with the default values.
			TextLineStyleBase() /*throw()*/ :
				lineBreak(LINE_BREAK_AUTO), wordBreak(WORD_BREAK_NORMAL),
				anchor(TEXT_ANCHOR_START), dominantBaseline(DOMINANT_BASELINE_AUTO),
				lineHeightShiftAdjustment(CONSIDER_SHIFTS), lineStackingStrategy(MAX_HEIGHT),
				textJustification(AUTO_JUSTIFICATION) {}
		};

		struct TextLineStyle : public TextLineStyleBase<false>,
			public std::enable_shared_from_this<TextLineStyle> {};
		template<> struct Inheritable<TextLineStyle> : public TextLineStyleBase<true>,
			public std::enable_shared_from_this<Inheritable<TextLineStyle>> {};

		/**
		 * @see TextRunStyle, TextLineStyle, Presentation#globalTextStyle,
		 *      Presentation#setGlobalTextStyle
		 */
		struct TextToplevelStyle : public std::enable_shared_from_this<TextToplevelStyle> {
			/**
			 * The writing mode specified by the presentation. May be overridden by
			 * @c graphics#font#TextRenderer class.
			 * @see graphics#font#TextRenderer#writingMode
			 */
			Inheritable<WritingMode> writingMode;
			/// The default text line style. The default value is @c null.
			std::shared_ptr<const TextLineStyle> defaultLineStyle;
		};
	}

	namespace detail {
		enum PhysicalTextAnchor {LEFT, MIDDLE, RIGHT};

		inline PhysicalTextAnchor computePhysicalTextAnchor(presentation::TextAnchor anchor, presentation::ReadingDirection readingDirection) {
			switch(anchor) {
				case presentation::TEXT_ANCHOR_MIDDLE:
					return MIDDLE;
				case presentation::TEXT_ANCHOR_START:
					return (readingDirection == presentation::LEFT_TO_RIGHT) ? LEFT : RIGHT;
				case presentation::TEXT_ANCHOR_END:
					return (readingDirection == presentation::LEFT_TO_RIGHT) ? RIGHT : LEFT;
				default:
					ASCENSION_ASSERT_NOT_REACHED();
			}
		}
	}
} // namespace ascension.presentation

#endif // !ASCENSION_TEXT_STYLE_HPP
