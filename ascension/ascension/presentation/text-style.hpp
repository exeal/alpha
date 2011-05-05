/**
 * @file text-style.hpp
 * @author exeal
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2011 was presentation.hpp
 * @date 2011-05-04 separated from presentation.hpp
 */

#ifndef ASCENSION_TEXT_STYLE_HPP
#define ASCENSION_TEXT_STYLE_HPP

#include <ascension/corelib/standard-iterator-adapter.hpp>
#include <ascension/graphics/color.hpp>	// graphics.Color
#include <ascension/graphics/font.hpp>	// graphics.font.FontProperties, ...
#include <ascension/graphics/paint.hpp>	// graphics.Paint

namespace ascension {

	namespace presentation {

		template<typename T>
		class Inheritable {
		public:
			typedef T value_type;
		public:
			Inheritable() /*throw()*/ : inherits_(true) {}
			Inheritable(value_type v) /*throw()*/ : value_(v), inherits_(false) {}
			operator value_type() const {return get();}
			value_type get() const {if(inherits()) throw std::logic_error(""); return value_;}
			value_type inherit() /*throw()*/ {inherits_ = true;}
			bool inherits() const /*throw()*/ {return inherits_;}
			Inheritable<value_type>& set(value_type v) /*throw()*/ {value_ = v; inherits_ = false; return *this;}
		private:
			value_type value_;
			bool inherits_;
		};

		/**
		 *
		 * @see "CSS3 Values and Units 3.4 Numbers and units identifiers"
		 *      (http://www.w3.org/TR/2006/WD-css3-values-20060919/#numbers0)
		 */
		struct Length {
			double value;	///< Value of the length.
			enum Unit {
				// relative length units
				EM_HEIGHT,		///< The font size of the relevant font.
				X_HEIGHT,		///< The x-height of the relevant font.
				PIXELS,			///< Pixels, relative to the viewing device.
				// relative length units introduced by CSS 3
				GRIDS,				///< The grid.
				REMS,				///< The font size of the primary font.
				VIEWPORT_WIDTH,		///< The viewport's width.
				VIEWPORT_HEIGHT,	///< The viewport's height.
				VIEWPORT_MINIMUM,	///< The viewport's height or width, whichever is smaller of the two.
				/**
				 * The width of the "0" (ZERO, U+0030) glyph found in the font for the font size
				 * used to render. If the "0" glyph is not found in the font, the average character
				 * width may be used.
				 */
				CHARACTERS,
				// absolute length units
				INCHES,			///< Inches -- 1 inch is equal to 2.54 centimeters.
				CENTIMETERS,	///< Centimeters.
				MILLIMETERS,	///< Millimeters.
				POINTS,			///< Points -- the point used by CSS 2.1 are equal to 1/72nd of an inch.
				PICAS,			///< Picas -- 1 pica is equal to 12 points.
				// used in DirectWrite
				DIPS,			///< Device independent pixels. 1 DIP is equal to 1/96th of an inch.
				// percentages (exactly not a length)
				PERCENTAGE,		///< Percentage.
			};
			Unit unit;	///< Unit of the length.

			/// Default constructor.
			Length() /*throw()*/ {}
			/// Constructor.
			explicit Length(double value, Unit unit = PIXELS) : value(value), unit(unit) {}
		};

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
				Length computedWidth() const {return (style != NONE) ? width : Length(0.0, width.unit);}
				/// Returns @c true if this part is invisible (but may be consumes place).
				bool hasVisibleStyle() const /*throw()*/ {return style != NONE && style != HIDDEN;}
			} before, after, start, end;
		};

		/// Dominant baselines from XSL 1.1, 7.14.5 "dominant-baseline".
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

		/// Alignment baseline from XSL 1.1, 7.14.2 "alignment-baseline".
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
		 * @see StyledTextRun, DefaultTextRunStyleProvider, StyledTextRunIterator, TextLineStyle
		 */
		struct TextRunStyle : public FastArenaObject<TextRunStyle> {
			/// Foreground color.
			graphics::Paint foreground;
			/// Background color.
			graphics::Paint background;
			/// Border of the text run. See the description of @c Border.
			Border border;
			/// Font family name. An empty string means inherit the parent.
			String fontFamily;	// TODO: replace with graphics.font.FontFamilies.
			/// Font properties. See @c graphics#FontProperties.
			graphics::font::FontProperties fontProperties;
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
		 * Represents a styled text run, with the beginning position (column) in the line and the
		 * style.
		 * @note This class does not provides the length of the text run.
		 * @note This class is not intended to be derived.
		 * @see StyledTextRunIterator, StyledTextRunEnumerator
		 */
		class StyledTextRun {
		public:
			/// Default constructor.
			StyledTextRun() /*throw()*/ : position_(INVALID_INDEX) {}
			/**
			 * Constructor.
			 * @param position The beginning position of the text style
			 * @param style The style of the text run
			 */
			StyledTextRun(length_t position,
				std::tr1::shared_ptr<const TextRunStyle> style) /*throw()*/ : position_(position), style_(style) {}
			/// Returns the position in the line of the text range which the style applies.
			length_t position() const /*throw()*/ {return position_;}
			/// Returns the style of the text run.
			std::tr1::shared_ptr<const TextRunStyle> style() const /*throw()*/ {return style_;}
		private:
			length_t position_;
			std::tr1::shared_ptr<const TextRunStyle> style_;
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
		class StyledTextRunEnumerator : public detail::IteratorAdapter<
			StyledTextRunEnumerator,
			std::iterator<
				std::input_iterator_tag,
				std::pair<Range<length_t>, std::tr1::shared_ptr<const TextRunStyle> >,
				std::ptrdiff_t,
				std::pair<Range<length_t>, std::tr1::shared_ptr<const TextRunStyle> >*,
				std::pair<Range<length_t>, std::tr1::shared_ptr<const TextRunStyle> >
			>
		> {
		public:
			StyledTextRunEnumerator();
			StyledTextRunEnumerator(std::auto_ptr<StyledTextRunIterator> sourceIterator, length_t end);
		private:
			const reference current() const;
			bool equals(const StyledTextRunEnumerator& other) const /*throw()*/;
			void next();
		private:
			std::auto_ptr<StyledTextRunIterator> iterator_;
			std::pair<bool, StyledTextRun> current_, next_;
			const length_t end_;
		};

		/**
		 * @c TextAnchor describes an alignment of text relative to the given point.
		 * @see resolveTextAlignment, TextLineStyle#alignment, TextLineStyle#lastSublineAlignment
		 * @see XSL 1.1, 7.16.9 "text-align"
		 *      (http://www.w3.org/TR/2006/REC-xsl11-20061205/#text-align)
		 * @see CSS Text Level 3, 7.1. Text Alignment: the 'text-align' property
		 *      (http://www.w3.org/TR/2010/WD-css3-text-20101005/#text-align)
		 * @see SVG 1.1, 10.9.1 Text alignment properties
		 *      (http://www.w3.org/TR/2010/WD-SVG11-20100622/text.html#TextAlignmentProperties)
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

		/**
		 * @c TextJustification describes the justification method.
		 * @note This definition is under construction.
		 * @see CSS Text Level 3, 7.3. Justification Method: the 'text-justify' property
		 *      (http://www.w3.org/TR/2010/WD-css3-text-20101005/#text-justify)
		 */
		enum TextJustification {
			/// Specifies no justification.
			NO_JUSTIFICATION,
//			AUTO_JUSTIFICATION, TRIM, INTER_WORD, INTER_IDEOGRAPH, INTER_CLUSTER, DISTRIBUTE, KASHIDA
		};

		/**
		 * Orientation of the text layout.
		 * @see TextLineStyle#readingDirection
		 */
		enum ReadingDirection {
			LEFT_TO_RIGHT,				///< The text is left-to-right.
			RIGHT_TO_LEFT				///< The text is right-to-left.
		};
#ifdef ASCENSION_WRITING_MODES
		enum ProgressionDirection {
			LEFT_TO_RIGHT, RIGHT_TO_LEFT, TOP_TO_BOTTOM, BOTTOM_TO_TOP
		}

		/// Returns @c true if @a direction is horizontal.
		inline bool isHorizontalDirection(ProgressionDirection direction) {
			return direction == LEFT_TO_RIGHT || direction == RIGHT_TO_LEFT;
		}

		/// Returns @c true if @a direction is vertical.
		inline bool isVerticalDirection(ProgressionDirection direction) {
			return direction == TOP_TO_BOTTOM || direction == BOTTOM_TO_TOP;
		}

		// documentation is presentation.cpp
		class WritingMode {
		public:
			static const WritingMode
				// SVG 1.1
				LR_TB, RL_TB, TB_RL, LR, RL, TB, INHERIT,
				// XSL 1.1 additional
				TB_LR, BT_LR, BT_RL,
				LR_BT, RL_BT,
				LR_ALTERNATING_RL_BT, LR_ALTERNATING_RL_TB,
				/*LR_INVERTING_RL_BT, LR_INVERTING_RL_TB, TB_LR_IN_LR_PAIRS*/;
		public:
			WritingMode() /*throw()*/;
			WritingMode(
				ProgressionDirection blockProgressionDirection,
				ProgressionDirection inlineProgressionDirection,
				bool inlineAlternating = false);
			/// Returns the block-progression-direction.
			ProgressionDirection blockProgressionDirection() const /**/ {return block_;}
			/// Returns the inline-progression-direction.
			ProgressionDirection inlineProgressionDirection() const /**/ {return inline_;}
		private:
			const ProgressionDirection block_, inline_;
			const bool inlineAlternating_;
		};
#endif // ASCENSION_WRITING_MODES
		/// From XSL 1.1, 7.16.5 "line-height-shift-adjustment".
		enum LineHeightShiftAdjustment {
			CONSIDER_SHIFTS, DISREGARD_SHIFTS
		};

		/// From XSL 1.1, 7.16.6 "line-stacking-strategy"
		enum LineStackingStrategy {
			LINE_HEIGHT, FONT_HEIGHT, MAX_HEIGHT
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

		struct TextLineStyle {
			/// The reading direction of the line. Default value is @c INHERIT_READING_DIRECTION.
			Inheritable<ReadingDirection> readingDirection;
			/**
			 * The alignment point in inline-progression-dimension. Default value is
			 * @c TEXT_ANCHOR_START.
			 */
			Inheritable<TextAnchor> anchor;
			/**
			 * The alignment point in block-progression-dimension, which is the dominant baseline
			 * of the line. Default value is @c DOMINANT_BASELINE_AUTO.
			 */
			Inheritable<DominantBaseline> dominantBaseline;
			/// Default value is @c CONSIDER_SHIFTS.
			Inheritable<LineHeightShiftAdjustment> lineHeightShiftAdjustment;
			/// Default value is @c MAX_HEIGHT.
			Inheritable<LineStackingStrategy> lineStackingStrategy;
			/// The number substitution setting. Default value is built by the default constructor.
			NumberSubstitution numberSubstitution;

			/// Default constructor.
			TextLineStyle() /*throw()*/ :
				readingDirection(),
				anchor(TEXT_ANCHOR_START), dominantBaseline(DOMINANT_BASELINE_AUTO),
				lineHeightShiftAdjustment(CONSIDER_SHIFTS), lineStackingStrategy(MAX_HEIGHT) {}
		};

	}
} // namespace ascension.presentation

#endif // !ASCENSION_TEXT_STYLE_HPP
