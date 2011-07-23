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
#include <ascension/presentation/length.hpp>
#include <map>

namespace ascension {

	namespace presentation {

		template<typename T>
		class Inheritable {
		public:
			typedef T value_type;
			typedef Inheritable<T> Type;
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

		template<bool condition, typename T>
		struct InheritableIf {
			typedef typename detail::Select<condition, Inheritable<T>, T>::Type Type;
		};

		template<typename T>
		struct AbstractFourSides {
			T before, after, start, end;
		};

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
				public FastArenaObject<TextRunStyle>, public std::tr1::enable_shared_from_this<TextRunStyle> {
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
		 *      (http://www.w3.org/TR/2010/WD-css3-text-20101005/#text-justify)
		 */
		enum TextJustification {
			/// Specifies no justification.
			NO_JUSTIFICATION,
//			AUTO_JUSTIFICATION, TRIM, INTER_WORD, INTER_IDEOGRAPH, INTER_CLUSTER, DISTRIBUTE, KASHIDA
		};

		/**
		 * Orientation of the text layout.
		 * @see TextLineStyle#readingDirection, defaultReadingDirection,
		 * @see "CSS Writing Modes Module Level 3, 2.1. Specifying Directionality: the ÅedirectionÅf
		 *      property" (http://www.w3.org/TR/css3-writing-modes/#direction)
		 * @see SVG 1.1 (Second Edition), 10.7.4 Relationship with bidirectionality
		 *      (http://www.w3.org/TR/SVG/text.html#DirectionProperty)
		 * @see XSL 1.1, 7.29.1 "direction" (http://www.w3.org/TR/xsl/#direction)
		 */
		enum ReadingDirection {
			LEFT_TO_RIGHT,	///< Left-to-right directionality.
			RIGHT_TO_LEFT	///< Right-to-left directionality.
		};

		ReadingDirection defaultReadingDirection(const Presentation& presentation);

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
		struct TextLineStyle : public std::tr1::enable_shared_from_this<TextLineStyle> {
			/// The default text run style. The default value is @c null.
			std::tr1::shared_ptr<const TextRunStyle> defaultRunStyle;
			/**
			 * The reading direction of the line. The default value is
			 * @c Inheritable&lt;ReadingDirection&gt;() which means to refers to the writing mode
			 * of the presentation.
			 */
			Inheritable<ReadingDirection> readingDirection;
			/**
			 * The alignment point in inline-progression-dimension. The default value is
			 * @c TEXT_ANCHOR_START.
			 */
			Inheritable<TextAnchor> anchor;
			/**
			 * The alignment point in block-progression-dimension, which is the dominant baseline
			 * of the line. The default value is @c DOMINANT_BASELINE_AUTO.
			 */
			Inheritable<DominantBaseline> dominantBaseline;
			/// The default value is @c CONSIDER_SHIFTS.
			Inheritable<LineHeightShiftAdjustment> lineHeightShiftAdjustment;
			/// The default value is @c MAX_HEIGHT.
			Inheritable<LineStackingStrategy> lineStackingStrategy;
			/// The number substitution setting. The default value is @c NumberSubstitution().
			NumberSubstitution numberSubstitution;

			/// Default constructor initializes the all members with the default values.
			TextLineStyle() /*throw()*/ :
				anchor(TEXT_ANCHOR_START), dominantBaseline(DOMINANT_BASELINE_AUTO),
				lineHeightShiftAdjustment(CONSIDER_SHIFTS), lineStackingStrategy(MAX_HEIGHT) {}
		};

		struct WritingModeBase {
			/**
			 * Defines block flow directions.
			 * @see "CSS Writing Modes Module Level 3, 3.1. Block Flow Direction: the Åewriting-modeÅf
			 *      property" (http://www.w3.org/TR/css3-writing-modes/#writing-mode)
			 * @see "SVG 1.1 (Second Edition), 10.7.2 Setting the inline-progression-direction"
			 *      (http://www.w3.org/TR/SVG/text.html#WritingModeProperty)
			 * @see "XSL 1.1, 7.29.7 "writing-mode"" (http://www.w3.org/TR/xsl/#writing-mode)
			 */
			enum BlockFlowDirection {
				HORIZONTAL_TB,	///< Top-to-bottom block flow. The writing mode is horizontal.
				VERTICAL_RL,	///< Right-to-left block flow. The writing mode is vertical.
				VERTICAL_LR		///< Left-to-right block flow. The writing mode is vertical.
			};
	
			/**
			 * Defines the orientation of characters within a line.
			 * @see "CSS Writing Modes Module Level 3, 5.1. Orienting Text: the Åetext-orientationÅf
			 *      property" (http://www.w3.org/TR/css3-writing-modes/#text-orientation)
			 * @see "SVG 1.1 (Second Edition), 10.7.3 Glyph orientation within a text run"
			 *      (http://www.w3.org/TR/SVG/text.html#GlyphOrientation)
			 */
			enum TextOrientation {
				VERTICAL_RIGHT, UPRIGHT, SIDEWAYS_RIGHT, SIDEWAYS_LEFT, SIDEWAYS, USE_GLYPH_ORIENTATION
			};

			/// Returns @c true if @a dir is horizontal direction.
			static inline bool isHorizontal(BlockFlowDirection dir) {
				switch(dir) {
					case HORIZONTAL_TB:
						return true;
					case VERTICAL_RL:
					case VERTICAL_LR:
						return false;
					default:
						throw UnknownValueException("dir");
				}
			}

			/// Returns @c true if @a dir is vertical direction.
			static inline bool isVertical(BlockFlowDirection dir) {return !isHorizontal(dir);}
		};

		/**
		 * @tparam inheritable All data members are inheritable when set to @c true
		 * @see WritingModeBase
		 * @see "CSS Writing Modes Module Level 3" (http://www.w3.org/TR/css3-writing-modes/)
		 * @see "SVG 1.1 (Second Edition), 10.7 Text layout"
		 *      (http://www.w3.org/TR/SVG/text.html#TextLayout)
		 * @see "XSL 1.1, 7.29 Writing-mode-related Properties"
		 *      (http://www.w3.org/TR/xsl/#writing-mode-related)
		 */
		template<bool inheritable>
		struct WritingMode : public WritingModeBase {
			/// The inline flow direction.
			typename InheritableIf<inheritable, ReadingDirection>::Type inlineFlowDirection;
			/// The block flow direction.
			typename InheritableIf<inheritable, BlockFlowDirection>::Type blockFlowDirection;
			/// The text orientation.
			typename InheritableIf<inheritable, TextOrientation>::Type textOrientation;

			/**
			 * Constructor initializes the data members with the given values.
			 * @param inlineFlowDirection
			 * @param blockFlowDirection
			 * @param textOrientation
			 */
			explicit WritingMode(
				typename InheritableIf<inheritable, ReadingDirection>::Type
					inlineFlowDirection = LEFT_TO_RIGHT/*ASCENSION_DEFAULT_TEXT_READING_DIRECTION*/,
				typename InheritableIf<inheritable, BlockFlowDirection>::Type
					blockFlowDirection = HORIZONTAL_TB,
				typename InheritableIf<inheritable, TextOrientation>::Type
					textOrientation = VERTICAL_RIGHT) :
				inlineFlowDirection(inlineFlowDirection), blockFlowDirection(blockFlowDirection),
				textOrientation(textOrientation) /*throw()*/ {}
			/// Implicit conversion operator.
			template<bool otherInheritable>
			inline operator WritingMode<otherInheritable>() const {
				return WritingMode<otherInheritable>(inlineFlowDirection, blockFlowDirection, textOrientation);
			}
			/// Equality operator.
			template<bool otherInheritable>
			inline bool operator==(const WritingMode<otherInheritable>& other) const {
				return inlineFlowDirection == other.inlineFlowDirection
					&& blockFlowDirection == other.blockFlowDirection && textOrientation == other.textOrientation;
			}
			/// Inequality operator.
			template<bool otherInheritable>
			inline bool operator!=(const WritingMode<otherInheritable>& other) const {return !(*this == other);}
		};

		/**
		 * Performs abstract-to-physical mappings according to the given writing mode.
		 * @tparam From The type for @a from
		 * @tparam To The type for @a to
		 * @param writingMode The writing mode
		 * @param from The abstract value to map
		 * @param[out] to The result physical value
		 * @see mapPhysicalToAbstract
		 */
		template<typename From, typename To>
		inline graphics::PhysicalFourSides<To>& mapAbstractToPhysical(
				const WritingMode<false>& writingMode,
				const AbstractFourSides<From>& from, graphics::PhysicalFourSides<To>& to) {
			const WritingModeBase::TextOrientation textOrientation(resolveTextOrientation(writingMode));
			switch(writingMode.blockFlowDirection) {
				case WritingModeBase::HORIZONTAL_TB:
					to.top = from.before;
					to.bottom = from.after;
					to.left = (writingMode.inlineFlowDirection == LEFT_TO_RIGHT) ? from.start : from.end;
					to.right = (writingMode.inlineFlowDirection == LEFT_TO_RIGHT) ? from.end : from.start;
					break;
				case WritingModeBase::VERTICAL_RL:
				case WritingModeBase::VERTICAL_LR:
					to.left = (writingMode.blockFlowDirection == WritingModeBase::VERTICAL_LR) ? from.before : from.after;
					to.right = (writingMode.blockFlowDirection == WritingModeBase::VERTICAL_RL) ? from.before : from.after;
					{
						int n = (textOrientation == WritingModeBase::SIDEWAYS_LEFT) ? 1 : 0;
						n += (writingMode.inlineFlowDirection == LEFT_TO_RIGHT) ? 1 : 0;
						to.top = (n == 1) ? from.start : from.end;
						to.bottom = (n == 1) ? from.start : from.end;
					}
		 			break;			
				default:
					throw UnknownValueException("writingMode.blockFlowDirection");
			}
			return to;
		}

		/**
		 * Performs abstract-to-physical mappings according to the given writing mode.
		 * @tparam From The type for @a from
		 * @tparam To The type for @a to
		 * @param writingMode The writing mode
		 * @param from The physical value to map
		 * @param[out] to The result abstract value
		 * @see #mapAbstractToPhysical
		 */
		template<typename From, typename To>
		AbstractFourSides<To>& mapPhysicalToAbstract(
				const WritingMode<false>& writingMode,
				const graphics::PhysicalFourSides<From>& from, AbstractFourSides<To>& to) {
			const WritingModeBase::TextOrientation textOrientation(resolveTextOrientation(writingMode));
			switch(writingMode.blockFlowDirection) {
				case WritingModeBase::HORIZONTAL_TB:
					to.before = from.top;
					to.after = from.bottom;
					to.start = (writingMode.inlineFlowDirection == LEFT_TO_RIGHT) ? from.left : from.right;
					to.end = (writingMode.inlineFlowDirection == RIGHT_TO_LRFT) ? from.left : from.right;
					break;
				case WritingModeBase::VERTICAL_RL:
				case WritingModeBase::VERTICAL_LR:
					to.before = (writingMode.blockFlowDirection == WritingModeBase::VERTICAL_LR) ? from.left : from.right;
					to.after = (writingMode.blockFlowDirection == WritingModeBase::VERTICAL_RL) ? from.left : from.right;
					{
						int n = (textOrientation == WritingModeBase::SIDEWAYS_LEFT) ? 1 : 0;
						n += (writingMode.inlineFlowDirection == LEFT_TO_RIGHT) ? 1 : 0;
						to.start = (n == 1) ? from.top : from.bottom;
						to.end = (n == 1) ? from.top : from.bottom;
					}
		 			break;			
				default:
					throw UnknownValueException("writingMode.blockFlowDirection");
			}
			return to;
		}

		/***/
		inline WritingModeBase::TextOrientation resolveTextOrientation(const WritingMode<false>& writingMode) {
			switch(writingMode.textOrientation) {
				case WritingModeBase::SIDEWAYS:
					if(writingMode.blockFlowDirection == WritingModeBase::VERTICAL_RL)
						return WritingModeBase::SIDEWAYS_RIGHT;
					else if(writingMode.blockFlowDirection == WritingModeBase::VERTICAL_LR)
						return WritingModeBase::SIDEWAYS_LEFT;
					else
						return WritingModeBase::SIDEWAYS;
				case WritingModeBase::USE_GLYPH_ORIENTATION:
					return WritingModeBase::VERTICAL_RIGHT;
				default:
					return writingMode.textOrientation;
			}
		}
	}

	namespace graphics {
		namespace font {
			class TextRenderer;
		}
	}

	namespace presentation {
		WritingMode<false> resolveWritingMode(
			const Presentation& presentation, const graphics::font::TextRenderer& textRenderer);

		/**
		 * @see TextRunStyle, TextLineStyle, Presentation#globalTextStyle,
		 *      Presentation#setGlobalTextStyle
		 */
		struct TextToplevelStyle : public std::tr1::enable_shared_from_this<TextToplevelStyle> {
			/// The writing mode.
			WritingMode<true> writingMode;
			/// The default text line style. The default value is @c null.
			std::tr1::shared_ptr<const TextLineStyle> defaultLineStyle;
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
