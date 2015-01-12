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
#include <ascension/graphics/font/actual-text-styles.hpp>
#include <ascension/graphics/font/number-substitution.hpp>
#include <ascension/presentation/styles/background.hpp>
#include <ascension/presentation/styles/text.hpp>
#include <ascension/presentation/styles/writing-modes.hpp>
#include <boost/utility/value_init.hpp>


namespace ascension {
	namespace viewers {
		class TextViewer;

		/**
		 * A ruler's style.
		 * @see TextViewer#declaredRulerConfiguration, TextViewer#setConfiguration
		 */
		struct RulerStyles : std::enable_shared_from_this<RulerStyles> {
			/// Whether the area is visible or not.
			typedef presentation::StyleProperty<
				presentation::styles::Enumerated<bool, false>,
				presentation::styles::Inherited<false>
			> Visibility;

			/// Alignment of the line number digits. See @c graphics#font#TextAlignment.
			typedef presentation::StyleProperty<
				presentation::styles::Enumerated<BOOST_SCOPED_ENUM_NATIVE(graphics::font::TextAlignment), graphics::font::TextAlignment::END>,
				presentation::styles::Inherited<false>
			> LineNumbersAlignment;

			/// Start value of the line number.
			typedef presentation::StyleProperty<
				presentation::styles::Enumerated<Index, 1>,
				presentation::styles::Inherited<false>
			> LineNumbersStartValue;

			/// Minimum number of the line number digits.
			typedef presentation::StyleProperty<
				presentation::styles::Enumerated<std::uint8_t, 4>,
				presentation::styles::Inherited<false>
			> LineNumbersMinimumDigits;

			/// Padding width at the start.
			typedef presentation::StyleProperty<
				presentation::styles::Lengthed<6, presentation::styles::Length::PIXELS>,
				presentation::styles::Inherited<false>
			> LineNumbersPaddingStart;

			/// Padding width at the end.
			typedef presentation::StyleProperty<
				presentation::styles::Lengthed<1, presentation::styles::Length::PIXELS>,
				presentation::styles::Inherited<false>
			> LineNumbersPaddingEnd;
			
			/// Width of the indicator margin. @c boost#none means to use platform-dependent setting.
			typedef presentation::StyleProperty<
				presentation::styles::Complex<
					boost::optional<presentation::styles::Length>
				>, presentation::styles::Inherited<false>
			> IndicatorMarginWidth;

			/// Style of the line numbers area.
			typedef boost::fusion::vector<
				Visibility,
				presentation::styles::Direction,
				LineNumbersAlignment,
				presentation::styles::TextJustification,
				LineNumbersStartValue,
				LineNumbersMinimumDigits,
				LineNumbersPaddingStart,
				LineNumbersPaddingEnd,
				presentation::styles::Color,
				presentation::styles::BackgroundColor,
				presentation::styles::BorderColor,	// 'border-end-color' property.
				presentation::styles::BorderStyle,	// 'border-end-style' property.
				presentation::styles::BorderWidth,	// 'border-end-width' property.
				presentation::styles::NumberSubstitution
			> LineNumbers;

			/// Style of the indicator margin.
			typedef boost::fusion::vector<
				Visibility,
				IndicatorMarginWidth,
				presentation::styles::BackgroundColor,
				presentation::styles::BorderColor,	// 'border-end-color' property.
				presentation::styles::BorderStyle,	// 'border-end-style' property.
				presentation::styles::BorderWidth	// 'border-end-width' property.
			> IndicatorMargin;

			/**
			 * Alignment (anchor) of the ruler. Must be either @c presentation#TextAlignment#START,
			 * @c presentation#TextAlignment#END, @c presentation#TextAlignment#LEFT or
			 * @c presentation#TextAlignment#RIGHT. In vertical layout, @c presentation#TextAlignment#LEFT and
			 * @c presentation#TextAlignment#RIGHT are treated as top and bottom respectively.
			 */
			typedef presentation::StyleProperty<
				presentation::styles::Enumerated<BOOST_SCOPED_ENUM_NATIVE(graphics::font::TextAlignment), graphics::font::TextAlignment::START>,
				presentation::styles::Inherited<true>
			> Alignment;

			/**
			 * Color of the text. Default value is @c boost#none which is fallbacked to the foreground of the text run
			 * style of the viewer's presentation global text style.
			 */
			presentation::styles::Color color;
			/**
			 * Color or style of the background. This can inherit the background of the text run style of the viewer's
			 * presentation global text style.
			 */
			presentation::styles::BackgroundColor background;
			/// 'alignment' property.
			presentation::styles::DeclaredValue<Alignment>::type alignment;
			/// Style of the line numbers area. This can be @c null.
			std::shared_ptr<const LineNumbers> lineNumbers;
			/// Style of the indicator margin. This can be @c null.
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
				void recomputeActualStyles() BOOST_NOEXCEPT;
			private:
				TextViewer& viewer_;
				std::shared_ptr<const RulerStyles> declaredStyles_;
				graphics::font::ActualBorderSide
					actualIndicatorMarginBorderEnd_, actualLineNumbersBorderEnd_;
				boost::value_initialized<graphics::Scalar>	// in user units
					actualIndicatorMarginContentWidth_, actualLineNumbersContentWidth_,
					actualLineNumbersPaddingStart_, actualLineNumbersPaddingEnd_;
				boost::value_initialized<std::uint8_t> actualLineNumberDigits_;
				presentation::styles::ComputedValue<presentation::styles::NumberSubstitution>::type actualNumberSubstitution_;
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
				return actualIndicatorMarginContentWidth_ + actualIndicatorMarginBorderEnd_.actualWidth();
			}

			/**
			 * Returns the width of 'allocation-rectangle' of the line numbers in user units.
			 * @return The width of the line numbers or zero if not visible
			 * @see #allocationWidth, #indicatorMarginWidth, #lineNumbersAllocationRectangle
			 */
			inline graphics::Scalar RulerPainter::lineNumbersAllocationWidth() const BOOST_NOEXCEPT {
				return boost::get(actualLineNumbersContentWidth_) + boost::get(actualLineNumbersPaddingStart_)
					+ boost::get(actualLineNumbersPaddingEnd_) + actualLineNumbersBorderEnd_.actualWidth();
			}
		}
	}
}

#endif // !ASCENSION_RULER_HPP
