/**
 * @file ruler.hpp
 * @author exeal
 * @date 2003-2006 was EditView.h
 * @date 2006-2011 was viewer.hpp
 * @date 2011-06-06 separated from viewer.hpp
 * @date 2011-2014
 */

#ifndef ASCENSION_RULER_HPP
#define ASCENSION_RULER_HPP

#include <ascension/config.hpp>	// ASCENSION_DEFAULT_TEXT_READING_DIRECTION, ...
#include <ascension/graphics/font/computed-text-styles.hpp>
#include <boost/utility/value_init.hpp>


namespace ascension {
	namespace viewers {
		class TextViewer;

		/**
		 * A ruler's style.
		 * @see TextViewer#declaredRulerConfiguration, TextViewer#setConfiguration
		 */
		struct RulerStyles : std::enable_shared_from_this<RulerStyles> {
			/// Style of the line numbers area.
			struct LineNumbers : std::enable_shared_from_this<LineNumbers> {
				/**
				 * Whether the area is visible or not. Default value is @c false and the line
				 * numbers is invisible.
				 */
				bool visible;
				/// Reading direction of the digits. See @c presentation#ReadingDirection.
				presentation::StyleProperty<
					presentation::sp::Enumerated<
						presentation::ReadingDirection, presentation::LEFT_TO_RIGHT
					>, presentation::sp::Inherited
				> readingDirection;
				/// Alignment of the digits. See @c presentation#TextAlignment.
				presentation::StyleProperty<
					presentation::sp::Enumerated<
						presentation::TextAlignment, presentation::TextAlignment::END
					>,
					presentation::sp::NotInherited
				> textAlignment;
				/// Justification of the digits. See @c presentation#TextJustification.
				presentation::StyleProperty<
					presentation::sp::Enumerated<
						presentation::TextJustification, presentation::TextJustification::AUTO
					>,
					presentation::sp::Inherited
				> textJustification;
				/// Start value of the line number. Default value is @c 1.
				presentation::StyleProperty<
					presentation::sp::Enumerated<Index, 1>,
					presentation::sp::NotInherited
				> startValue;
				/// Minimum number of digits. Default value is @c 4.
				presentation::StyleProperty<
					presentation::sp::Enumerated<std::uint8_t, 4>,
					presentation::sp::NotInherited
				> minimumDigits;
				/// Padding width at the start. Default value is 6-pixel.
				presentation::StyleProperty<
					presentation::sp::Lengthed<6, presentation::Length::PIXELS>,
					presentation::sp::NotInherited
				> paddingStart;
				/// Padding width at the end. Default value is 1-pixel.
				presentation::StyleProperty<
					presentation::sp::Lengthed<1, presentation::Length::PIXELS>,
					presentation::sp::NotInherited
				> paddingEnd;
#if 1
				/**
				 * Color of the text. Default value is @c boost#none which is fallbacked to the
				 * foreground of the text run style of the viewer's presentation global text style.
				 */
				presentation::ColorProperty<presentation::sp::Inherited> color;
#else
				/**
				 * Color or style of the text. Default value is @c null which is fallbacked to the
				 * foreground of the text run style of the viewer's presentation global text style.
				 */
				std::shared_ptr<graphics::Paint> foreground;
#endif
				/// Color or style of the background.
				presentation::Background background;
				/// Style of the border-end. Default value is @c presentation#Border#Side().
				/// @note Line margins area does not have other three border sides.
				presentation::Border::Side borderEnd;
				/// Digit substitution type. @c DST_CONTEXTUAL can't set. Default value is @c DST_USER_DEFAULT.
				presentation::StyleProperty<
					presentation::sp::Complex<presentation::NumberSubstitution>,
					presentation::sp::NotInherited
				> numberSubstitution;
			};

			/// Style of the indicator margin.
			struct IndicatorMargin : std::enable_shared_from_this<IndicatorMargin> {
				/**
				 * Whether the indicator margin is visible or not. Default value is @c false
				 * and the indicator margin is invisible.
				 */
				bool visible;
				/**
				 * Width of the indicator margin. Default value is
				 * @c presentation#Inheritable&lt;graphics#Length&gt; which means to use
				 * platform-dependent setting.
				 */
				presentation::StyleProperty<
					presentation::sp::Complex<
						boost::optional<presentation::Length>
					>, presentation::sp::NotInherited
				> width;
				/// Color or style of the content.
				presentation::Background background;
				/// Style of the border-end. Default value is @c presentation#Border#Side().
				/// @note An indicator margin does have other three border sides.
				presentation::Border::Side borderEnd;

				IndicatorMargin() BOOST_NOEXCEPT;
			};

			/**
			 * Color of the text. Default value is @c boost#none which is fallbacked to the
			 * foreground of the text run style of the viewer's presentation global text style.
			 */
			presentation::ColorProperty<presentation::sp::Inherited> color;
			/**
			 * Color or style of the background. This can inherit the background of the text
			 * run style of the viewer's presentation global text style.
			 */
			presentation::Background background;
			/**
			 * Alignment (anchor) of the ruler. Must be either @c presentation#TextAlignment#START,
			 * @c presentation#TextAlignment#END, @c presentation#TextAlignment#LEFT or
			 * @c presentation#TextAlignment#RIGHT. In vertical layout,
			 * @c presentation#TextAlignment#LEFT and @c presentation#TextAlignment#RIGHT are
			 * treated as top and bottom respectively.
			 */
			presentation::StyleProperty<
				presentation::sp::Enumerated<
					presentation::TextAlignment, presentation::TextAlignment::START
				>, presentation::sp::NotInherited
			> alignment;
			/// Style of the line numbers area.
			std::shared_ptr<const LineNumbers> lineNumbers;
			/// Style of the indicator margin.
			std::shared_ptr<const IndicatorMargin> indicatorMargin;
		};

		std::shared_ptr<const RulerStyles::LineNumbers> lineNumbers(const RulerStyles& rulerStyles);
		std::shared_ptr<const RulerStyles::IndicatorMargin> indicatorMargin(const RulerStyles& rulerStyles);

		namespace detail {
			/// @internal @c RulerPainter paints the ruler of the @c TextViewer.
			class RulerPainter : private boost::noncopyable {
			public:
				explicit RulerPainter(TextViewer& viewer, std::shared_ptr<const RulerStyles> initialStyles = nullptr);
				graphics::PhysicalDirection alignment() const BOOST_NOEXCEPT;
				graphics::Scalar allocationWidth() const BOOST_NOEXCEPT;
				const RulerStyles& declaredStyles() const BOOST_NOEXCEPT;
				graphics::Rectangle indicatorMarginAllocationRectangle() const BOOST_NOEXCEPT;
				graphics::Scalar indicatorMarginAllocationWidth() const BOOST_NOEXCEPT;
				graphics::Rectangle lineNumbersAllocationRectangle() const BOOST_NOEXCEPT;
				graphics::Scalar lineNumbersAllocationWidth() const BOOST_NOEXCEPT;
				void paint(graphics::PaintContext& context);
				void scroll(const graphics::font::VisualLine& from);
				void setStyles(std::shared_ptr<const RulerStyles> styles);
				void update() BOOST_NOEXCEPT;
			private:
				std::uint8_t computeMaximumDigitsForLineNumbers() const BOOST_NOEXCEPT;
				void computeAllocationWidth() BOOST_NOEXCEPT;
#if defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI) && 0
				void updateGDIObjects() BOOST_NOEXCEPT;
#endif
			private:
				TextViewer& viewer_;
				std::shared_ptr<const RulerStyles> declaredStyles_;
				graphics::font::ComputedBorderSide
					computedIndicatorMarginBorderEnd_, computedLineNumbersBorderEnd_;
				boost::value_initialized<graphics::Scalar>	// in user units
					computedIndicatorMarginContentWidth_, computedLineNumbersContentWidth_,
					computedLineNumbersPaddingStart_, computedLineNumbersPaddingEnd_;
				boost::value_initialized<std::uint8_t> computedLineNumberDigits_;
#if defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI) && 0
				win32::Handle<HPEN> indicatorMarginPen_, lineNumbersPen_;
				win32::Handle<HBRUSH> indicatorMarginBrush_, lineNumbersBrush_;
				const bool enablesDoubleBuffering_;
				win32::Handle<HDC> memoryDC_;
				win32::Handle<HBITMAP> memoryBitmap_;
#endif
			};

			/**
			 * Returns the width of 'allocation-rectangle' of the ruler in pixels.
			 * @return The width of the ruler or zero if not visible
			 * @see #indicatorMarginWidth, #lineNumbersWidth
			 */
			inline graphics::Scalar RulerPainter::allocationWidth() const BOOST_NOEXCEPT {
				return indicatorMarginAllocationWidth() + lineNumbersAllocationWidth();
			}

			/// Returns the ruler's declared styles.
			inline const RulerStyles& RulerPainter::declaredStyles() const BOOST_NOEXCEPT {
				return *declaredStyles_;
			}

			/**
			 * Returns the width of 'allocation-rectangle' of the indicator margin in user units.
			 * @return The width of the indicator margin or zero if not visible
			 * @see #allocationWidth, #lineNumbersAllocationWidth, #indicatorMarginAllocationRectangle
			 */
			inline graphics::Scalar RulerPainter::indicatorMarginAllocationWidth() const BOOST_NOEXCEPT {
				return computedIndicatorMarginContentWidth_ + computedIndicatorMarginBorderEnd_.computedWidth();
			}

			/**
			 * Returns the width of 'allocation-rectangle' of the line numbers in user units.
			 * @return The width of the line numbers or zero if not visible
			 * @see #allocationWidth, #indicatorMarginWidth, #lineNumbersAllocationRectangle
			 */
			inline graphics::Scalar RulerPainter::lineNumbersAllocationWidth() const BOOST_NOEXCEPT {
				return boost::get(computedLineNumbersContentWidth_) + boost::get(computedLineNumbersPaddingStart_)
					+ boost::get(computedLineNumbersPaddingEnd_) + computedLineNumbersBorderEnd_.computedWidth();
			}
		}
	}
}

#endif // !ASCENSION_RULER_HPP
