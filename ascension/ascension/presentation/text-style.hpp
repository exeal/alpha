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
		// from CSS Color Module Level 3 //////////////////////////////////////////////////////////

		/**
		 * Describes the foreground color of the text content. @c boost#none means 'currentColor'
		 * CSS 3.
		 * @see CSS Color Module Level 3, 3.1. Foreground color: the ÅecolorÅf property
		 *      (http://www.w3.org/TR/css3-color/#foreground)
		 * @see SVG 1.1 (Second Edition), 12.2 The ÅecolorÅf property
		 *      (http://www.w3.org/TR/SVG11/color.html#ColorProperty)
		 * @see XSL 1.1, 7.18.1 "color" (http://www.w3.org/TR/xsl/#color)
		 */
		typedef StyleProperty<
			sp::Complex<
				boost::optional<graphics::Color>
			>, sp::Inherited
		> ColorProperty;

		inline graphics::Color computeColor(const ColorProperty* current,
				const ColorProperty* parent, const ColorProperty& ancestor) {
			if(current != nullptr && !current->inherits() && current->get() != boost::none)
				return *current->get();
			else if(parent != nullptr && !parent->inherits() && parent->get() != boost::none)
				return *parent->get();
			else if(!ancestor.inherits() && ancestor.get() != boost::none)
				return *ancestor.get();
			else
				return graphics::SystemColors::get(graphics::SystemColors::WINDOW_TEXT);
		}

		// from CSS Backgrounds and Borders Module Level 3 ////////////////////////////////////////

		/**
		 * @c null also means 'transparent'.
		 * @see CSS Backgrounds and Borders Module Level 3, 3.10. Backgrounds Shorthand: the
		 *      ÅebackgroundÅf property (http://www.w3.org/TR/css3-background/#the-background)
		 * @see SVG 1.1 (Second Edition), 11.3 Fill Properties
		 *      (http://www.w3.org/TR/SVG11/painting.html#FillProperties)
		 * @see XSL 1.1, 7.31.1 "background" (http://www.w3.org/TR/xsl/#background)
		 */
		struct Background {
			/**
			 * [Copied from CSS3] This property sets the background color of an element. The color
			 * is drawn behind any background images.
			 * @see CSS Backgrounds and Borders Module Level 3, 3.2. Base Color: the
			 *      Åebackground-colorÅf property
			 *      (http://www.w3.org/TR/css3-background/#the-background-color)
			 * @see XSL 1.1, 7.8.2 "background-color" (http://www.w3.org/TR/xsl/#background-color)
			 */
			StyleProperty<
				sp::Complex<
					boost::optional<graphics::Color>
				>, sp::NotInherited
			> color;
			/**
			 * @see CSS Backgrounds and Borders Module Level 3, 3.1. Layering Multiple Background
			 *      Images (http://www.w3.org/TR/css3-background/#layering)
			 */
			struct Layer {
				struct Image {} image;
				struct RepeatStyle {} repeat;
				enum Attachment {} attachment;
				struct Position {} position;
				enum Clip {} clip;
				enum Origin {} origin;
				struct Size {} size;
			};

			Background() : color(boost::make_optional(graphics::Color::TRANSPARENT_BLACK)) {}
		};

		inline std::unique_ptr<graphics::Paint> computeBackground(
				const Background* current, const Background* parent, const Background& ancestor) {
			// TODO: This code is not complete.
			if(current != nullptr && !current->color.inherits()
					&& current->color.get() != boost::none && current->color.get()->isFullyTransparent())
				return std::unique_ptr<graphics::Paint>(new graphics::SolidColor(*current->color.get()));
			else if(parent != nullptr && !parent->color.inherits()
					&& parent->color.get() != boost::none && parent->color.get()->isFullyTransparent())
				return std::unique_ptr<graphics::Paint>(new graphics::SolidColor(*parent->color.get()));
			else if(!ancestor.color.inherits()
					&& ancestor.color.get() != boost::none && ancestor.color.get()->isFullyTransparent())
				return std::unique_ptr<graphics::Paint>(new graphics::SolidColor(*ancestor.color.get()));
			return std::unique_ptr<graphics::Paint>(
				new graphics::SolidColor(graphics::SystemColors::get(graphics::SystemColors::WINDOW)));
		}

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

		// from CSS Fonts Module Level 3 //////////////////////////////////////////////////////////

		/**
		 * [Copied from CSS3] An <absolute-size> keyword refers to an entry in a table of font
		 * sizes computed and kept by the user agent.
		 * @see http://www.w3.org/TR/css3-fonts/#ltabsolute-sizegt
		 */
		ASCENSION_BEGIN_SCOPED_ENUM(AbsoluteFontSize)
			XX_SMALL, X_SMALL, SMALL, MEDIUM, LARGE, X_LARGE, XX_LARGE
		ASCENSION_END_SCOPED_ENUM;

		/**
		 * [Copied from CSS3] A <relative-size> keyword is interpreted relative to the table of
		 * font sizes and the font size of the parent element.
		 * @see http://www.w3.org/TR/css3-fonts/#ltrelative-sizegt
		 */
		ASCENSION_BEGIN_SCOPED_ENUM(RelativeFontSize)
			LARGER, SMALLER
		ASCENSION_END_SCOPED_ENUM;

		// from CSS Line Layout Module Level 3 ////////////////////////////////////////////////////

		/// Enumerated values for @c TextRunStyle#textHeight.
		ASCENSION_BEGIN_SCOPED_ENUM(TextHeightEnums)
			AUTO, FONT_SIZE, TEXT_SIZE, MAX_SIZE
		ASCENSION_END_SCOPED_ENUM;

		/// Enumerated values for @c TextRunStyle#lineHeight.
		ASCENSION_BEGIN_SCOPED_ENUM(LineHeightEnums)
			NORMAL, NONE
		ASCENSION_END_SCOPED_ENUM;

		/**
		 * [Copied from CSS3] This property enumerates which aspects of the elements in a line box
		 * contribute to the height height of that line box.
		 * @see CSS Line Layout Module Level 3, 3.4.2 Line Stacking: the Åeline-box-containÅf
		 *      property (http://dev.w3.org/csswg/css3-linebox/#LineStacking)
		 * @see XSL 1.1, 7.16.6 "line-stacking-strategy"
		 *      (http://www.w3.org/TR/xsl/#line-stacking-strategy)
		 */
		ASCENSION_BEGIN_SCOPED_ENUM(LineBoxContain)
			// TODO: 'NONE' should be 0.
			// TODO: Values other than 'NONE' can be combined by bitwise-OR.
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

		/// Enumerated values for @c TextRunStyle#alignmentAdjust.
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

		/// Enumerated values for @c TextRunStyle#baselineShift.
		ASCENSION_BEGIN_SCOPED_ENUM(BaselineShiftEnums)
			BASELINE,
			SUB,
			SUPER
		ASCENSION_END_SCOPED_ENUM;

		/// Enumerated values for @c InlineBoxAlignment.
		ASCENSION_BEGIN_SCOPED_ENUM(InlineBoxAlignmentEnums)
			INITIAL, LAST
		ASCENSION_END_SCOPED_ENUM;

		/**
		 * [Copied from CSS3] The Åeinline-box-alignÅf property determines which line of a multi-line
		 * inline block aligns with the previous and next inline elements within a line.
		 * @see CSS Line Layout Module Level 3, 4.9 Inline box alignment: the
		 *      Åeinline-box-alignÅf property
		 *      (http://dev.w3.org/csswg/css3-linebox/#inline-box-align-prop)
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
		struct TextWrapping {
			TextWrap textWrap;
			OverflowWrap overflowWrap;
			Measure measure;
			/// Default constructor.
			TextWrapping() : textWrap(TextWrap::NORMAL), overflowWrap(OverflowWrap::NORMAL), measure(0) {}
		};

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
			// TODO: Some values should be able to be combined by bitwise-OR.
			NONE, FIRST, FORCE_END, ALLOW_END, LAST
		ASCENSION_END_SCOPED_ENUM;

		/**
		 * @see CSS Text Level 3, 10.1. Line Decoration: Underline, Overline, and Strike-Through
		 *      (http://www.w3.org/TR/css3-text/#line-decoration)
		 * @see SVG 1.1 (Second Edition), 10.12 Text decoration
		 *      (http://www.w3.org/TR/2011/REC-SVG11-20110816/text.html#TextDecorationProperties)
		 * @see XSL 1.1, 7.17.4 "text-decoration" (http://www.w3.org/TR/xsl/#text-decoration)
		 */
		struct TextDecorations {
			enum Style {NONE, SOLID, DOTTED, DAHSED};
			struct Part {
				StyleProperty<
					sp::Complex<boost::optional<graphics::Color>>,
					sp::NotInherited
				> color;	// if is Color(), same as the foreground
				StyleProperty<
					sp::Enumerated<Style, NONE>,
					sp::NotInherited
				> style;	///< Default value is @c NONE.
			} overline, strikethrough, baseline, underline;
		};

		struct TextEmphasis {};

		struct TextShadow {};

		/**
		 * Visual style settings of a text run.
		 * @see TextLineStyle, TextToplevelStyle, StyledTextRun, StyledTextRunIterator
		 */
		struct TextRunStyle :
				public FastArenaObject<TextRunStyle>,
				public std::enable_shared_from_this<TextRunStyle> {
#if 1
			/// Foreground color of the text content. See @c ColorProperty.
			ColorProperty color;
#else
			/// Text paint style.
			std::shared_ptr<graphics::Paint> foreground;
#endif
			/// The background properties. See @c Background.
			Background background;
			/// Border of the text run. See the description of @c Border.
			Border border;
			/**
			 * @see CSS Fonts Module Level 3, 3.1 Font family: the font-family property
			 *      (http://www.w3.org/TR/css3-fonts/#font-family-prop)
			 * @see SVG 1.1 (Second Edition), 10.10 Font selection properties
			 *      (http://www.w3.org/TR/SVG11/text.html#FontFamilyProperty)
			 * @see XSL 1.1, 7.9.2 "font-family" (http://www.w3.org/TR/xsl/#font-family)
			 */
			StyleProperty<
				sp::Complex<
					graphics::font::FontFamiliesSpecification
				>, sp::Inherited
			> fontFamily;
			/// 'font-weight' property. See @c FontWeight.
			StyleProperty<
				sp::Enumerated<graphics::font::FontWeight, graphics::font::FontWeight::NORMAL>,
				sp::Inherited
			> fontWeight;
			/// 'font-stretch' property. See @c FontStretch.
			StyleProperty<
				sp::Enumerated<graphics::font::FontStretch, graphics::font::FontStretch::NORMAL>,
				sp::Inherited
			> fontStretch;
			/// 'font-style' property. See @c FontStyle.
			StyleProperty<
				sp::Enumerated<graphics::font::FontStyle, graphics::font::FontStyle::NORMAL>,
				sp::Inherited
			> fontStyle;
			/**
			 * [Copied from CSS3] This property indicates the desired height of glyphs from the
			 * font. For scalable fonts, the font-size is a scale factor applied to the EM unit of
			 * the font.
			 * @see CSS Fonts Module Level 3, 3.5 Font size: the font-size property
			 *      (http://www.w3.org/TR/css3-fonts/#font-size-prop)
			 * @see SVG 1.1 (Second Edition), 10.10 Font selection properties
			 *      (http://www.w3.org/TR/SVG11/text.html#FontFamilyProperty)
			 * @see XSL 1.1, 7.9.4 "font-size" (http://www.w3.org/TR/xsl/#font-size)
			 */
			StyleProperty<
				sp::Multiple<
					boost::variant<AbsoluteFontSize, RelativeFontSize, Length>,
					AbsoluteFontSize, AbsoluteFontSize::MEDIUM
				>, sp::Inherited
			> fontSize;
			/// 'font-size-adjust' property. @c boost#none means 'none'.
			StyleProperty<
				sp::Complex<
					boost::optional<double>
				>, sp::Inherited
			> fontSizeAdjust;
//			StyleProperty<
//				sp::Complex<
//					std::map<graphics::font::TrueTypeFontTag, uint32_t>
//				>, sp::Inherited
//			> fontFeatureSettings;
//			StyleProperty<
//				sp::Complex<
//					boost::optional<String>
//				>, sp::Inherited
//			> fontLanguageOverride;
			/**
			 * [Copied from CSS3] The Åetext-heightÅf property determine the block-progression
			 * dimension of the text content area of an inline box (non-replaced elements).
			 * @see CSS Line Layout Module Level 3, 3.3 Block-progression dimensions: the
			 *      Åetext-heightÅf property (http://dev.w3.org/csswg/css3-linebox/#inline1)
			 */
			StyleProperty<
				sp::Multiple<
					boost::variant<TextHeightEnums, double>,
					TextHeightEnums, TextHeightEnums::AUTO
				>, sp::Inherited
			> textHeight;
			/**
			 * [Copied from CSS3] The Åeline-heightÅf property controls the amount of leading space
			 * which is added before and after the block-progression dimension of an inline box
			 * (not including replaced inline boxes, but including the root inline box) to
			 * determine the extended block-progression dimension of the inline box.
			 * @see CSS Line Layout Module Level 3, 3.4.1 Line height adjustment: the Åeline-heightÅf
			 *      property (http://dev.w3.org/csswg/css3-linebox/#InlineBoxHeight)
			 * @see XSL 1.1, 7.16.4 "line-height" (http://www.w3.org/TR/xsl/#line-height)
			 */
			StyleProperty<
				sp::Multiple<
					boost::variant<LineHeightEnums, double, Length>,
					LineHeightEnums, LineHeightEnums::NORMAL
				>, sp::Inherited
			> lineHeight;
			/// The dominant baseline of the line. See @c DominantBaseline.
			StyleProperty<
				sp::Enumerated<DominantBaseline, DominantBaseline::AUTO>,
				sp::NotInherited
			> dominantBaseline;
			/// The alignment baseline. Default value is @c ALIGNMENT_BASELINE_AUTO.
			StyleProperty<
				sp::Enumerated<AlignmentBaseline, AlignmentBaseline::BASELINE>,
				sp::NotInherited
			> alignmentBaseline;
			/**
			 * [Copied from CSS3] The Åealignment-adjustÅf property allows more precise alignment of
			 * elements, such as graphics, that do not have a baseline-table or lack the desired
			 * baseline in their baseline-table. With the Åealignment-adjustÅf property, the position
			 * of the baseline identified by the Åealignment-baselineÅf can be explicitly determined.
			 * It also determines precisely the alignment point for each glyph within a textual
			 * element. The user agent should use heuristics to determine the position of a non
			 * existing baseline for a given element.
			 * @see CSS Line Layout Module Level 3, 4.6 Setting the alignment point: the
			 *      Åealignment-adjustÅf property
			 *      (http://dev.w3.org/csswg/css3-linebox/#alignment-adjust-prop)
			 * @see CSS3 module: line, 4.6. Setting the alignment point: the 'alignment-adjust'
			 *      property (http://www.w3.org/TR/css3-linebox/#alignment-adjust-prop)
			 * @see XSL 1.1, 7.14.1 "alignment-adjust" (http://www.w3.org/TR/xsl/#alignment-adjust)
			 */
			StyleProperty<
				sp::Multiple<
					boost::variant<AlignmentAdjustEnums, Length>,
					AlignmentAdjustEnums, AlignmentAdjustEnums::AUTO
				>, sp::NotInherited
			> alignmentAdjust;
			/**
			 * [Copied from CSS3] The Åebaseline-shiftÅf property allows repositioning of the
			 * dominant-baseline relative to the dominant-baseline. The shifted object might be a
			 * sub- or superscript. Within the shifted element, the whole baseline table is offset;
			 * not just a single baseline. For sub- and superscript, the amount of offset is
			 * determined from the nominal font of the parent.
			 * @see CSS Line Layout Module Level 3, 4.7 Repositioning the dominant baseline: the
			 *      Åebaseline-shiftÅf property
			 *      (http://dev.w3.org/csswg/css3-linebox/#baseline-shift-prop)
			 * @see CSS3 module: line, 4.7. Repositioning the dominant baseline: the
			 *     'baseline-shift' property
			 *      (http://www.w3.org/TR/css3-linebox/#baseline-shift-prop)
			 * @see SVG 1.1 (Second Edition), 10.9.2 Baseline alignment properties
			 *      (http://www.w3.org/TR/SVG/text.html#BaselineShiftProperty)
			 * @see XSL 1.1, 7.14.3 "baseline-shift" (http://www.w3.org/TR/xsl/#baseline-shift)
			 */
			StyleProperty<
				sp::Multiple<
					boost::variant<BaselineShiftEnums, Length>,
					BaselineShiftEnums, BaselineShiftEnums::BASELINE
				>, sp::NotInherited
			> baselineShift;
			StyleProperty<
				sp::Enumerated<TextTransform, TextTransform::NONE>,
				sp::Inherited
			> textTransform;
//			???? hyphens;
			/**
			 * [Copied from CSS3] This property specifies the minimum, maximum, and optimal spacing
			 * between ÅgwordsÅh. Additional spacing is applied to each word-separator character left
			 * in the text after the white space processing rules have been applied, and should be
			 * applied half on each side of the character.
			 * @see CSS Text Level 3, 8.1. Word Spacing: the Åeword-spacingÅf property
			 *      (http://www.w3.org/TR/css3-text/#word-spacing)
			 * @see SVG 1.1 (Second Edition), 10.11 Spacing properties
			 *      (http://www.w3.org/TR/SVG11/text.html#WordSpacingProperty)
			 * @see XSL 1.1, 7.17.8 "word-spacing" (http://www.w3.org/TR/xsl/#word-spacing)
			 */
			StyleProperty<
				sp::Complex<SpacingLimit>,
				sp::Inherited
			> wordSpacing;
			/**
			 * [Copied from CSS3] This property specifies the minimum, maximum, and optimal spacing
			 * between characters. Letter-spacing is applied in addition to any word-spacing.
			 * ÅenormalÅf optimum letter-spacing is typically zero. Letter-spacing must not be
			 * applied at the beginning or at the end of a line. At element boundaries, the total
			 * letter spacing between two characters is given by and rendered within the innermost
			 * element that contains the boundary. For the purpose of letter-spacing, each
			 * consecutive run of atomic inlines (such as image and/or inline blocks) is treated as
			 * a single character.
			 * @see CSS Text Level 3, 8.2. Word Spacing: the Åeletter-spacingÅf property
			 *      (http://www.w3.org/TR/css3-text/#letter-spacing)
			 * @see SVG 1.1 (Second Edition), 10.11 Spacing properties
			 *      (http://www.w3.org/TR/SVG11/text.html#LetterSpacingProperty)
			 * @see XSL 1.1, 7.17.2 "letter-spacing" (http://www.w3.org/TR/xsl/#letter-spacing)
			 */
			StyleProperty<
				sp::Complex<SpacingLimit>,
				sp::Inherited
			> letterSpacing;
			/// Text decoration properties. See @c TextDecorations.
			TextDecorations textDecorations;
			/// Text emphasis properties. See @c TextEmphasis.
			TextEmphasis textEmphasis;
			/// Text shadow properties. See @c TextShadow.
			TextShadow textShadow;
//			RubyProperties rubyProperties;
//			Effects effects;
			/// Set @c false to disable shaping. Default is @c true.
			StyleProperty<
				sp::Enumerated<bool, true>,
				sp::NotInherited
			> shapingEnabled;

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
		struct TextLineStyle {
			/// The default text run style. The default value is @c null.
			std::shared_ptr<const TextRunStyle> defaultRunStyle;
			/// 'line-box-contain' property. See @c LineBoxContain.
			StyleProperty<
				sp::Enumerated<LineBoxContain, LineBoxContain::BLOCK | LineBoxContain::INLINE | LineBoxContain::REPLACED>,
				sp::Inherited
			> lineBoxContain;
			/// Åeinline-box-alignÅf property. See @c InlineBoxAlignment.
			StyleProperty<
				sp::Multiple<
					InlineBoxAlignment,
					InlineBoxAlignmentEnums, InlineBoxAlignmentEnums::LAST
				>, sp::NotInherited
			> inlineBoxAlignment;
//			StyleProperty<
//				sp::Enumerated<TextSpaceCollapse, TextSpaceCollapse::COLLAPSE>,
//				sp::Inherited
//			> textSpaceCollapse;
			/**
			 * [Copied from CSS3] This property determines the measure of the tab character
			 * (U+0009) when rendered. Integers represent the measure in space characters (U+0020).
			 * @see CSS Text Level 3, 3.2. Tab Character Size: the Åetab-sizeÅf property
			 *      (http://www.w3.org/TR/css3-text/#tab-size)
			 */
			StyleProperty<
				sp::Multiple<
					boost::variant<unsigned int, Length>,
					unsigned int, 8
				>, sp::Inherited
			> tabSize;
			/// The line breaking strictness. See @c LineBreak.
			StyleProperty<
				sp::Enumerated<LineBreak, LineBreak::AUTO>,
				sp::Inherited
			> lineBreak;
			/// The word breaking rules. See @c WordBreak.
			StyleProperty<
				sp::Enumerated<WordBreak, WordBreak::NORMAL>,
				sp::Inherited
			> wordBreak;
			/// 'text-wrap' property. See @c TextWrap.
			StyleProperty<
				sp::Enumerated<TextWrap, TextWrap::NORMAL>,
				sp::Inherited
			> textWrap;
			/// 'overflow-wrap' property. See @c OverflowWrap.
			StyleProperty<
				sp::Enumerated<OverflowWrap, OverflowWrap::NORMAL>,
				sp::Inherited
			> overflowWrap;
			/// 'text-align' property. See @c TextAlignment.
			StyleProperty<
				sp::Enumerated<TextAlignment, TextAlignment::START>,
				sp::Inherited
			> textAlignment;
			/// 'text-align-last' property. See @c TextAlignmentLast.
			StyleProperty<
				sp::Enumerated<TextAlignmentLast, TextAlignmentLast::AUTO>,
				sp::Inherited
			> textAlignmentLast;
			/// 'text-justify' property. See @c TextJustification.
			StyleProperty<
				sp::Enumerated<TextJustification, TextJustification::AUTO>,
				sp::Inherited
			> textJustification;
			/// 'text-indent' property. See @c TextIndent.
			StyleProperty<
				sp::Complex<TextIndent>,
				sp::Inherited
			> textIndent;
			/// 'hanging-punctuation' property. See @c HangingPunctuation.
			StyleProperty<
				sp::Enumerated<HangingPunctuation, HangingPunctuation::NONE>,
				sp::Inherited
			> hangingPunctuation;
			/// 'dominant-baseline' property. See @c DominantBaseline.
			StyleProperty<
				sp::Enumerated<DominantBaseline, DominantBaseline::AUTO>,
				sp::NotInherited
			> dominantBaseline;
			/// The number substitution process. The default value is @c NumberSubstitution().
			StyleProperty<
				sp::Complex<NumberSubstitution>,
				sp::Inherited
			> numberSubstitution;
		};

		std::shared_ptr<const TextRunStyle> defaultTextRunStyle(
			const TextLineStyle& textLineStyle) /*noexcept*/;

		/**
		 * 
		 * The writing modes specified by this style may be overridden by
		 * @c graphics#font#TextRenderer#writingMode.
		 * @see TextRunStyle, TextLineStyle, Presentation#globalTextStyle,
		 *      Presentation#setGlobalTextStyle
		 */
		struct TextToplevelStyle : public std::enable_shared_from_this<TextToplevelStyle> {
			/// 'direction' property. See @c ReadingDirection.
			StyleProperty<
				sp::Enumerated<ReadingDirection, LEFT_TO_RIGHT>,
				sp::Inherited
			> direction;
//			StyleProperty<
//				sp::Enumerated<UnicodeBidi, UnicodeBidi::NORMAL>,
//				sp::NotInherited
//			> unicodeBidi;
			/// 'writing-mode' property. See @c BlockFlowDirection.
			StyleProperty<
				sp::Enumerated<BlockFlowDirection, HORIZONTAL_TB>,
				sp::Inherited
			> writingMode;
			/// 'text-orientation' property. See @c TextOrientation.
			StyleProperty<
				sp::Enumerated<TextOrientation, MIXED_RIGHT>,
				sp::Inherited
			> textOrientation;
			/// The default text line style. The default value is @c null.
			std::shared_ptr<const TextLineStyle> defaultLineStyle;
		};

		/**
		 * @tparam Parent @c std#shared_ptr&lt;const TextLineStyle&gt; or
		 *                @c RulerConfiguration#LineNumbers*
		 */
		template<typename Parent>
		inline graphics::Color computeColor(const ColorProperty* current,
				const Parent parent, const TextToplevelStyle& ancestor) {
			const ColorProperty* parentColor = nullptr;
			if(const TextLineStyle* p = parent.get()) {
				if(p->defaultRunStyle)
					parentColor = &p->defaultRunStyle->color;
			}
			const ColorProperty* ancestorColor = nullptr;
			if(ancestor.defaultLineStyle && ancestor.defaultLineStyle->defaultRunStyle)
				ancestorColor = &ancestor.defaultLineStyle->defaultRunStyle->color;
			return computeColor(current, parentColor,
				(ancestorColor != nullptr) ? *ancestorColor : ColorProperty());
		}

		inline std::unique_ptr<graphics::Paint> computeBackground(const Background* current,
				std::shared_ptr<const TextLineStyle> parent, const TextToplevelStyle& ancestor) {
			const Background* parentBackground = nullptr;
			if(const TextLineStyle* p = parent.get()) {
				if(p->defaultRunStyle)
					parentBackground = &p->defaultRunStyle->background;
			}
			const Background* ancestorBackground = nullptr;
			if(ancestor.defaultLineStyle && ancestor.defaultLineStyle->defaultRunStyle)
				ancestorBackground = &ancestor.defaultLineStyle->defaultRunStyle->background;
			Background inheritedBackground;
			inheritedBackground.color.inherit();
			return computeBackground(current, parentBackground,
				(ancestorBackground != nullptr) ? *ancestorBackground : inheritedBackground);
		}

		std::shared_ptr<const TextLineStyle> defaultTextLineStyle(
			const TextToplevelStyle& textToplevelStyle) /*noexcept*/;
	}

	namespace detail {
		ASCENSION_BEGIN_SCOPED_ENUM(PhysicalTextAnchor)
			LEFT = presentation::TextAlignment::LEFT,
			CENTER = presentation::TextAlignment::CENTER,
			RIGHT = presentation::TextAlignment::RIGHT
		ASCENSION_END_SCOPED_ENUM;

		inline PhysicalTextAnchor computePhysicalTextAnchor(
				presentation::TextAnchor anchor, presentation::ReadingDirection readingDirection) {
			switch(anchor) {
				case presentation::TextAnchor::MIDDLE:
					return PhysicalTextAnchor::CENTER;
				case presentation::TextAnchor::START:
					return (readingDirection == presentation::LEFT_TO_RIGHT) ? PhysicalTextAnchor::LEFT : PhysicalTextAnchor::RIGHT;
				case presentation::TextAnchor::END:
					return (readingDirection == presentation::LEFT_TO_RIGHT) ? PhysicalTextAnchor::RIGHT : PhysicalTextAnchor::LEFT;
				default:
					ASCENSION_ASSERT_NOT_REACHED();
			}
		}
	}
} // namespace ascension.presentation

#endif // !ASCENSION_TEXT_STYLE_HPP
