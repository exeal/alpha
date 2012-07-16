/**
 * @file text-style.hpp
 * @author exeal
 * @see text-line-style.hpp
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2011 was presentation.hpp
 * @date 2011-05-04 separated from presentation.hpp
 * @date 2012-07-16 reunioned with text-line-style.hpp
 */

#ifndef ASCENSION_TEXT_STYLE_HPP
#define ASCENSION_TEXT_STYLE_HPP

#include <ascension/corelib/future/scoped-enum-emulation.hpp>
#include <ascension/graphics/color.hpp>	// graphics.Color
#include <ascension/graphics/font.hpp>	// graphics.font.FontProperties, ...
#include <ascension/graphics/paint.hpp>	// graphics.Paint
#include <ascension/presentation/length.hpp>
#include <ascension/presentation/style-property.hpp>
#include <ascension/presentation/writing-mode.hpp>
#include <map>
#include <memory>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>

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

		// from CSS Backgrounds and Borders Module Level 3 ////////////////////////////////////////

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
				 * The foreground color of the border. Default value is @c boost#none which means
				 * 'currentColor' that the value of @c TextRunStyle#color.
				 */
				boost::optional<graphics::Color> color;
				/// Style of the border. Default value is @c NONE.
				Style style;
				/// Thickness of the border. Default value is @c MEDIUM.
				Length width;

				/// Default constructor.
				Part() /*noexcept*/ : style(NONE), width(MEDIUM) {}
				/// Returns the computed width.
				Length computedWidth() const {
					return (style != NONE) ? width : Length(0.0, width.unitType());
				}
				/// Returns @c true if this part is invisible (but may be consumes place).
				bool hasVisibleStyle() const /*noexcept*/ {
					return style != NONE && style != HIDDEN;
				}
			};
			FlowRelativeFourSides<Part> sides;
		};

		// from CSS Line Layout Module Level 3 ////////////////////////////////////////////////////

		/// Enumerated values for @c TextHeight.
		ASCENSION_BEGIN_SCOPED_ENUM(TextHeightEnums)
			AUTO, FONT_SIZE, TEXT_SIZE, MAX_SIZE
		ASCENSION_END_SCOPED_ENUM

		/**
		 * [Copied from CSS3] The Åetext-heightÅf property determine the block-progression dimension
		 * of the text content area of an inline box (non-replaced elements).
		 * @see CSS Line Layout Module Level 3, 3.3 Block-progression dimensions: the Åetext-heightÅf
		 *      property (http://dev.w3.org/csswg/css3-linebox/#inline1)
		 */
		typedef boost::variant<TextHeightEnums, double> TextHeight;

		/// Enumerated values for @c LineHeight.
		ASCENSION_BEGIN_SCOPED_ENUM(LineHeightEnums)
			NORMAL, NONE
		ASCENSION_END_SCOPED_ENUM

		/**
		 * [Copied from CSS3] The Åeline-heightÅf property controls the amount of leading space which
		 * is added before and after the block-progression dimension of an inline box (not
		 * including replaced inline boxes, but including the root inline box) to determine the
		 * extended block-progression dimension of the inline box.
		 * @see CSS Line Layout Module Level 3, 3.4.1 Line height adjustment: the Åeline-heightÅf
		 *      property (http://dev.w3.org/csswg/css3-linebox/#InlineBoxHeight)
		 * @see XSL 1.1, 7.16.4 "line-height" (http://www.w3.org/TR/xsl/#line-height)
		 */
		typedef boost::variant<LineHeightEnums, double, Length> LineHeight;

		/**
		 * [Copied from CSS3] This property enumerates which aspects of the elements in a line box
		 * contribute to the height height of that line box.
		 * @see CSS Line Layout Module Level 3, 3.4.2 Line Stacking: the Åeline-box-containÅf
		 *      property (http://dev.w3.org/csswg/css3-linebox/#LineStacking)
		 * @see XSL 1.1, 7.16.6 "line-stacking-strategy"
		 *      (http://www.w3.org/TR/xsl/#line-stacking-strategy)
		 */
		ASCENSION_BEGIN_SCOPED_ENUM(LineBoxContain)
			BLOCK, INLINE, FONT, GLYPHS, REPLACED, INLINE_BOX, NONE
		ASCENSION_END_SCOPED_ENUM;
		/**
		 * [Copied from CSS3] The Åedominant-baselineÅf property is used to determine or re-determine
		 * a scaled-baseline-table.
		 * @see CSS Line Layout Module Level 3, 4.4 Dominant baseline: the Åedominant-baselineÅf
		 *      property (http://dev.w3.org/csswg/css3-linebox/#dominant-baseline-prop)
		 * @see CSS3 module: line, 4.4. Dominant baseline: the 'dominant-baseline' property
		 *      (http://www.w3.org/TR/css3-linebox/#dominant-baseline-prop)
		 * @see SVG 1.1 (Second Edition), 10.9.2 Baseline alignment properties
		 *      (http://www.w3.org/TR/SVG/text.html#DominantBaselineProperty)
		 * @see XSL 1.1, 7.14.5 "dominant-basline" (http://www.w3.org/TR/xsl/#dominant-baseline)
		 */
		ASCENSION_BEGIN_SCOPED_ENUM(DominantBaseline)
			AUTO,
			USE_SCRIPT,
			NO_CHANGE,
			RESET_SIZE,
			ALPHABETIC,
			HANGING,
			IDEOGRAPHIC,
			MATHEMATICAL,
			CENTRAL,
			MIDDLE,
			TEXT_AFTER_EDGE,
			TEXT_DEFORE_EDGE,
		ASCENSION_END_SCOPED_ENUM;

		/**
		 * [Copied from CSS3] This property specifies how an inline-level element is aligned with
		 * respect to its parent. That is, to which of the parent's baselines the alignment point
		 * of this element is aligned. Unlike the Åedominant-baselineÅf property the
		 * Åealignment-baselineÅf property has no effect on its children dominant-baselines.
		 * @see CSS Line Layout Module Level 3, 4.5 Aligning the alignment point of an element: the
		 *      Åealignment-baselineÅf property
		 *      (http://dev.w3.org/csswg/css3-linebox/#alignment-baseline-prop)
		 * @see CSS3 module: line, 4.5. Aligning the alignment point of an element: the
		 *      'alignment-baseline' property
		 *      (http://www.w3.org/TR/css3-linebox/#alignment-baseline-prop)
		 * @see SVG 1.1 (Second Edition), 10.9.2 Baseline alignment properties
		 *      (http://www.w3.org/TR/SVG/text.html#AlignmentBaselineProperty)
		 * @see XSL 1.1, 7.14.2 "alignment-baseline" (http://www.w3.org/TR/xsl/#alignment-baseline)
		 */
		ASCENSION_BEGIN_SCOPED_ENUM(AlignmentBaseline)
			BASELINE,
			USE_SCRIPT,
			BEFORE_EDGE,
			TEXT_BEFORE_EDGE,
			AFTER_EDGE,
			TEXT_AFTER_EDGE,
			CENTRAL,
			MIDDLE,
			IDEOGRAPHIC,
			ALPHABETIC,
			HANGING,
			MATHEMATICAL
		ASCENSION_END_SCOPED_ENUM;

		/// Enumerated values for @c AlignmentAdjust.
		ASCENSION_BEGIN_SCOPED_ENUM(AlignmentAdjustEnums)
			AUTO,
			BASELINE,
			BEFORE_EDGE,
			TEXT_BEFORE_EDGE,
			MIDDLE,
			CENTRAL,
			AFTER_EDGE,
			TEXT_AFTER_EDGE,
			IDEOGRAPHIC,
			ALPHABETIC,
			HANGING,
			MATHEMATICAL
		ASCENSION_END_SCOPED_ENUM;

		/**
		 * [Copied from CSS3] The Åealignment-adjustÅf property allows more precise alignment of
		 * elements, such as graphics, that do not have a baseline-table or lack the desired
		 * baseline in their baseline-table. With the Åealignment-adjustÅf property, the position of
		 * the baseline identified by the Åealignment-baselineÅf can be explicitly determined. It
		 * also determines precisely the alignment point for each glyph within a textual element.
		 * The user agent should use heuristics to determine the position of a non existing
		 * baseline for a given element.
		 * @see CSS Line Layout Module Level 3, 4.6 Setting the alignment point: the
		 *      Åealignment-adjustÅf property
		 *      (http://dev.w3.org/csswg/css3-linebox/#alignment-adjust-prop)
		 * @see CSS3 module: line, 4.6. Setting the alignment point: the 'alignment-adjust'
		 *      property (http://www.w3.org/TR/css3-linebox/#alignment-adjust-prop)
		 * @see XSL 1.1, 7.14.1 "alignment-adjust" (http://www.w3.org/TR/xsl/#alignment-adjust)
		 */
		typedef boost::variant<AlignmentAdjustEnums, Length> AlignmentAdjust;

		/// Enumerated values for @c BaselineShift.
		ASCENSION_BEGIN_SCOPED_ENUM(BaselineShiftEnums)
			BASELINE,
			SUB,
			SUPER
		ASCENSION_END_SCOPED_ENUM;

		/**
		 * [Copied from CSS3] The Åebaseline-shiftÅf property allows repositioning of the
		 * dominant-baseline relative to the dominant-baseline. The shifted object might be a sub-
		 * or superscript. Within the shifted element, the whole baseline table is offset; not just
		 * a single baseline. For sub- and superscript, the amount of offset is determined from the
		 * nominal font of the parent.
		 * @see CSS Line Layout Module Level 3, 4.7 Repositioning the dominant baseline: the
		 *      Åebaseline-shiftÅf property
		 *      (http://dev.w3.org/csswg/css3-linebox/#baseline-shift-prop)
		 * @see CSS3 module: line, 4.7. Repositioning the dominant baseline: the 'baseline-shift'
		 *      property (http://www.w3.org/TR/css3-linebox/#baseline-shift-prop)
		 * @see SVG 1.1 (Second Edition), 10.9.2 Baseline alignment properties
		 *      (http://www.w3.org/TR/SVG/text.html#BaselineShiftProperty)
		 * @see XSL 1.1, 7.14.3 "baseline-shift" (http://www.w3.org/TR/xsl/#baseline-shift)
		 */
		typedef boost::variant<BaselineShiftEnums, Length> BaselineShift;

		/// Enumerated values for @c InlineBoxAlignment.
		ASCENSION_BEGIN_SCOPED_ENUM(InlineBoxAlignmentEnums)
			INITIAL, LAST
		ASCENSION_END_SCOPED_ENUM;

		/**
		 * [Copied from CSS3] The Åeinline-box-alignÅf property determines which line of a multi-line
		 * inline block aligns with the previous and next inline elements within a line.
		 * @see CSS Line Layout Module Level 3, 4.9 Inline box alignment: the Åeinline-box-alignÅf
		 *      property (http://dev.w3.org/csswg/css3-linebox/#inline-box-align-prop)
		 */
		typedef boost::variant<InlineBoxAlignmentEnums, Index> InlineBoxAlignment;

		// from CSS Text Level 3 //////////////////////////////////////////////////////////////////

		/**
		 * [Copied from CSS3] This property transforms text for styling purposes.
		 * @see CSS Text Level 3, 2.1. Transforming Text: the Åetext-transformÅf property
		 *      (http://www.w3.org/TR/css3-text/#text-transform)
		 * @see XSL 1.1, 7.17.6 "text-transform" (http://www.w3.org/TR/xsl/#text-transform)
		 */
		ASCENSION_BEGIN_SCOPED_ENUM(TextTransform)
			NONE, CAPITALIZE, UPPERCASE, LOWERCASE, FULL_WIDTH, FULL_SIZE_KANA
		ASCENSION_END_SCOPED_ENUM;

//		enum TextSpaceCollapse;

		/**
		 * [Copied from CSS3] This property determines the measure of the tab character (U+0009)
		 * when rendered. Integers represent the measure in space characters (U+0020).
		 * @see CSS Text Level 3, 3.2. Tab Character Size: the Åetab-sizeÅf property
		 *      (http://www.w3.org/TR/css3-text/#tab-size)
		 */
		typedef boost::variant<unsigned int, Length> TabSize;

		/**
		 * [Copied from CSS3] This property specifies the strictness of line-breaking rules applied
		 * within an element: particularly how line-breaking interacts with punctuation.
		 * @see CSS Text Level 3, 4.1. Line Breaking Strictness: the Åeline-breakÅf property
		 *      (http://www.w3.org/TR/css3-text/#line-break)
		 */
		ASCENSION_BEGIN_SCOPED_ENUM(LineBreak)
			AUTO,
			LOOSE,
			NORMAL,
			STRICT
		ASCENSION_END_SCOPED_ENUM;

		/**
		 * [Copied from CSS3] This property specifies line break opportunities within words.
		 * @see CSS Text Level 3, 4.2. Word Breaking Rules: the 'word-break' property
		 *      (http://www.w3.org/TR/css3-text/#word-break)
		 */
		ASCENSION_BEGIN_SCOPED_ENUM(WordBreak)
			NORMAL,
			KEEP_ALL,
			BREAK_ALL
		ASCENSION_END_SCOPED_ENUM;

//		enum Hyphens;

		/**
		 * [Copied from CSS3] This property specifies the mode for text wrapping.
		 * @see CSS Text Level 3, 6.1. Text Wrap Settings: 'text-wrap' property
		 *      (http://www.w3.org/TR/css3-text/#text-wrap)
		 */
		ASCENSION_BEGIN_SCOPED_ENUM(TextWrap)
			NORMAL,
			NONE,
			AVOID
		ASCENSION_END_SCOPED_ENUM;

		/**
		 * [Copied from CSS3] This property specifies whether the UA may break within a word to
		 * prevent overflow when an otherwise-unbreakable string is too long to fit within the line
		 * box. It only has an effect when Åetext-wrapÅf is either ÅenormalÅf or ÅeavoidÅf. 
		 * @see CSS Text Level 3 - 6.2. Emergency Wrapping: the 'overflow-wrap' property
		 *      (http://www.w3.org/TR/css3-text/#overflow-wrap)
		 */
		ASCENSION_BEGIN_SCOPED_ENUM(OverflowWrap)
			NORMAL,
			BREAK_WORD/*,
			HYPHENATE*/
		ASCENSION_END_SCOPED_ENUM;

		template<typename Measure>
		struct TextWrappingBase {
			TextWrap textWrap;
			OverflowWrap overflowWrap;
			Measure measure;
			/// Default constructor.
			TextWrappingBase() : textWrap(TextWrap::NORMAL), overflowWrap(OverflowWrap::NORMAL), measure(0) {}
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
		 * @see CSS Text Level 3, 7.1. Text Alignment: the 'text-align' property
		 *      (http://www.w3.org/TR/2010/WD-css3-text-20101005/#text-align)
		 */
		ASCENSION_BEGIN_SCOPED_ENUM(TextAlignment)
			START,
			END,
			LEFT,
			RIGHT,
			CENTER,
			JUSTIFY,
			MATCH_PARENT,
			START_END
		ASCENSION_END_SCOPED_ENUM;

		/**
		 * [Copied from SVG11] The Åetext-anchorÅf property is used to align (start-, middle- or
		 * end-alignment) a string of text relative to a given point.
		 * @see SVG 1.1, 10.9.1 Text alignment properties
		 *      (http://www.w3.org/TR/SVG/text.html#TextAlignmentProperties)
		 */
		ASCENSION_BEGIN_SCOPED_ENUM(TextAnchor)
			START = TextAlignment::START,
			MIDDLE = TextAlignment::CENTER,
			END = TextAlignment::END
		ASCENSION_END_SCOPED_ENUM;

		/**
		 * [Copied from CSS3] This property describes how the last line of a block or a line right
		 * before a forced line break is aligned. If a line is also the first line of the block or
		 * the first line after a forced line break, then, unless Åetext-alignÅf assigns an explicit
		 * first line alignment (via Åestart endÅf), Åetext-align-lastÅf takes precedence over
		 * Åetext-alignÅf. If ÅeautoÅf is specified, content on the affected line is aligned per
		 * Åetext-alignÅf unless Åetext-alignÅf is set to ÅejustifyÅf. In this case, content is justified
		 * if Åetext-justifyÅf is ÅedistributeÅf and start-aligned otherwise. All other values have the
		 * same meanings as in Åetext-alignÅf.
		 * @see CSS Text Level 3, 7.2. Last Line Alignment: the Åetext-align-lastÅf property
		 *      (http://www.w3.org/TR/css3-text/#text-align-last)
		 * @see XSL 1.1, 7.16.10 "text-align-last" (http://www.w3.org/TR/xsl/#text-align-last)
		 */
		ASCENSION_BEGIN_SCOPED_ENUM(TextAlignmentLast)
			START = TextAlignment::START,
			CENTER = TextAlignment::CENTER,
			END = TextAlignment::END,
			LEFT = TextAlignment::LEFT,
			RIGHT = TextAlignment::RIGHT,
			JUSTIFY = TextAlignment::JUSTIFY,
			AUTO = TextAlignment::START_END + 1
		ASCENSION_END_SCOPED_ENUM;

		class Presentation;
		TextAnchor defaultTextAnchor(const Presentation& presentation);

		/**
		 * [Copied from CSS3] This property selects the justification method used when a line's
		 * alignment is set to ÅejustifyÅf (see Åetext-alignÅf), primarily by controlling which
		 * scripts' characters are adjusted together or separately. The property applies to block
		 * containers, but the UA may (but is not required to) also support it on inline elements.
		 * @see CSS Text Level 3, 7.3. Justification Method: the Åetext-justifyÅf property
		 *      (http://www.w3.org/TR/css3-text/#text-justify)
		 */
		ASCENSION_BEGIN_SCOPED_ENUM(TextJustification)
			/// Specifies no justification.
			AUTO, NONE, INTER_WORD, INTER_IDEOGRAPH, INTER_CLUSTER, DISTRIBUTE, KASHIDA
		ASCENSION_END_SCOPED_ENUM;

		/**
		 * @see CSS Text Level 3, 8. Spacing (http://www.w3.org/TR/css3-text/#spacing)
		 * @see XSL 1.1, 4.3 Spaces and Conditionality (http://www.w3.org/TR/xsl/#spacecond)
		 */
		typedef Length SpacingLimit;

		/**
		 * [Copied from CSS3] This property specifies the indentation applied to lines of inline
		 * content in a block.
		 * @see CSS Text Level 3, 9.1. First Line Indentation: the Åetext-indentÅf property
		 *      (http://www.w3.org/TR/css3-text/#text-indent)
		 * @see XSL 1.1, 7.16.11 "text-indent" (http://www.w3.org/TR/xsl/#text-indent)
		 */
		struct TextIndent {
			Length length;
			bool hanging, eachLine;
		};

		/**
		 * [Copied from CSS3] This property determines whether a punctuation mark, if one is
		 * present, may be placed outside the line box (or in the indent) at the start or at the
		 * end of a line of text.
		 * @see CSS Text Level 3, 9.2. Hanging Punctuation: the Åehanging-punctuationÅf property
		 *      (http://www.w3.org/TR/css3-text/#hanging-punctuation)
		 */
		ASCENSION_BEGIN_SCOPED_ENUM(HangingPunctuation)
			NONE, FIRST, FORCE_END, ALLOW_END, LAST
		ASCENSION_END_SCOPED_ENUM;

		/**
		 * @see CSS Text Level 3, 10.1. Line Decoration: Underline, Overline, and Strike-Through
		 *      (http://www.w3.org/TR/css3-text/#line-decoration)
		 * @see SVG 1.1 (Second Edition), 10.12 Text decoration
		 *      (http://www.w3.org/TR/2011/REC-SVG11-20110816/text.html#TextDecorationProperties)
		 * @see XSL 1.1, 7.17.4 "text-decoration" (http://www.w3.org/TR/xsl/#text-decoration)
		 */
		struct Decorations {
			enum Style {NONE, SOLID, DOTTED, DAHSED};
			struct Part {
				boost::optional<graphics::Color> color;	// if is Color(), same as the foreground
				Inheritable<Style> style;	///< Default value is @c NONE.
				/// Default constructor.
				Part() : style(NONE) {}
			} overline, strikethrough, baseline, underline;
		};

//		struct TextEmphasis;

		/**
		 * Visual style settings of a text run.
		 * @see TextLineStyle, TextToplevelStyle, StyledTextRun, StyledTextRunIterator
		 */
		struct TextRunStyle :
				public FastArenaObject<TextRunStyle>,
				public std::enable_shared_from_this<TextRunStyle> {
#if 1
			/// Foreground color. Default value is @c boost#none means that inherits the value.
			StyleProperty<InitializedByDefaultConstructor<boost::optional<graphics::Color>>, Inherited> color;
#else
			/// Text paint style.
			std::shared_ptr<graphics::Paint> foreground;
#endif
			/// Background color. This is not inheritable and @c null means transparent.
			std::shared_ptr<graphics::Paint> background;
			/// Border of the text run. See the description of @c Border.
			Border border;
			/// Font family name. An empty string means inherit the parent.
			String fontFamily;	// TODO: replace with graphics.font.FontFamiliesSpecification.
			/// Font size.
			double fontSizeInPixels;
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
			/// Default constructor.
			StyledTextRun() /*throw()*/ {}
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
		ASCENSION_BEGIN_SCOPED_ENUM(PhysicalTextAnchor) {
			LEFT = presentation::TextAlignment::LEFT,
			CENTER = presentation::TextAlignment::CENTER,
			RIGHT = presentation::TextAlignment::RIGHT
		ASCENSION_END_SCOPED_ENUM;

		inline PhysicalTextAnchor computePhysicalTextAnchor(
				presentation::TextAnchor anchor, presentation::ReadingDirection readingDirection) {
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
