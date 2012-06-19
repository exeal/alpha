/**
 * @file ruler.hpp
 * @author exeal
 * @date 2003-2006 was EditView.h
 * @date 2006-2011 was viewer.hpp
 * @date 2011-06-06 separated from viewer.hpp
 * @date 2011-2012
 */

#ifndef ASCENSION_RULER_HPP
#define ASCENSION_RULER_HPP

#include <ascension/config.hpp>	// ASCENSION_DEFAULT_TEXT_READING_DIRECTION, ...
#include <ascension/presentation/text-style.hpp>


namespace ascension {
	namespace viewers {

		class TextViewer;

		/**
		 * A ruler's configuration.
		 * @see TextViewer#rulerConfiguration, TextViewer#setConfiguration
		 */
		struct RulerConfiguration {
			/// Configuration about a line numbers area.
			struct LineNumbers {
				/**
				 * Whether the area is visible or not. Default value is @c false and the line
				 * numbers is invisible.
				 */
				bool visible;
				/**
				 * Reading direction of the digits. Default value is
				 * @c presentation#Inheritable&lt;presentation#ReadingDirection&gt;().
				 */
				presentation::Inheritable<presentation::ReadingDirection> readingDirection;
				/// Anchor of the digits. Default value is @c presentation#TEXT_ANCHOR_END.
				presentation::TextAnchor anchor;
				/// Start value of the line number. Default value is @c 1.
				Index startValue;
				/// Minimum number of digits. Default value is @c 4.
				uint8_t minimumDigits;
				/// Padding width at the start. Default value is 6-pixel.
				presentation::Length paddingStart;
				/// Padding width at the end. Default value is 1-pixel.
				presentation::Length paddingEnd;
#if 1
				/**
				 * Color of the text. Default value is @c boost#none which is fallbacked to the
				 * foreground of the text run style of the viewer's presentation global text style.
				 */
				boost::optional<graphics::Color> color;
#else
				/**
				 * Color or style of the text. Default value is @c null which is fallbacked to the
				 * foreground of the text run style of the viewer's presentation global text style.
				 */
				std::shared_ptr<graphics::Paint> foreground;
#endif
				/**
				 * Color or style of the background. Default value is @c null which is fallbacked
				 * to the background of the text run style of the viewer's presentation global text
				 * style.
				 */
				std::shared_ptr<graphics::Paint> background;
				/**
				 * Style of the border-end. If the @c color is default value, fallbacked to the
				 * color of @c #foreground member. Default value is @c presentation#Border#Part().
				 */
				presentation::Border::Part borderEnd;
				/// Digit substitution type. @c DST_CONTEXTUAL can't set. Default value is @c DST_USER_DEFAULT.
				presentation::NumberSubstitution numberSubstitution;

				LineNumbers() /*noexcept*/;
			} lineNumbers;	/// Configuration about the line numbers area.

			/// Configuration about an indicator margin.
			struct IndicatorMargin {
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
				presentation::Inheritable<presentation::Length> width;
				/**
				 * Color or style of the content. If @c color is default value, fallbacked to the
				 * platform-dependent color. Default value is @c null.
				 */
				std::shared_ptr<graphics::Paint> paint;
				/**
				 * Style of the border-end. If @c color is default value, fallbacked to the
				 * platform-dependent color. Default value is @c presentation#Border#Part().
				 */
				presentation::Border::Part borderEnd;

				IndicatorMargin() /*throw()*/;
			} indicatorMargin;	/// Configuration about the indicator margin.
			/**
			 * Alignment (anchor) of the ruler. Must be either
			 * @c presentation#TEXT_ANCHOR_START or @c presentation#TEXT_ANCHOR_END. Default
			 * value is @c presentation#TEXT_ANCHOR_START.
			 */
			presentation::TextAnchor alignment;

			RulerConfiguration() /*throw()*/;
		};
	}

	namespace detail {
		/// @internal @c RulerPainter paints the ruler of the @c TextViewer.
		class RulerPainter {
			ASCENSION_NONCOPYABLE_TAG(RulerPainter);
		public:
			enum SnapAlignment {LEFT, TOP, RIGHT, BOTTOM};
		public:
			explicit RulerPainter(viewers::TextViewer& viewer) /*throw()*/;
			SnapAlignment alignment() const /*throw()*/;
			graphics::Scalar allocationWidth() const /*throw()*/;
			const viewers::RulerConfiguration& configuration() const /*throw()*/;
			graphics::NativeRectangle indicatorMarginAllocationRectangle() const /*throw()*/;
			graphics::Scalar indicatorMarginAllocationWidth() const /*throw()*/;
			graphics::NativeRectangle lineNumbersAllocationRectangle() const /*throw()*/;
			graphics::Scalar lineNumbersAllocationWidth() const /*throw()*/;
			void paint(graphics::PaintContext& context);
			void scroll(const graphics::font::VisualLine& from);
			void setConfiguration(const viewers::RulerConfiguration& configuration);
			void update() /*throw()*/;
		private:
			uint8_t maximumDigitsForLineNumbers() const /*throw()*/;
			void recalculateWidth() /*throw()*/;
#if defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI) && 0
			void updateGDIObjects() /*throw()*/;
#endif
		private:
			viewers::TextViewer& viewer_;
			viewers::RulerConfiguration configuration_;
			graphics::Scalar indicatorMarginContentWidth_, indicatorMarginBorderEndWidth_;
			graphics::Scalar lineNumbersContentWidth_,
				lineNumbersPaddingStartWidth_, lineNumbersPaddingEndWidth_, lineNumbersBorderEndWidth_;
			uint8_t lineNumberDigitsCache_;
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
		inline graphics::Scalar RulerPainter::allocationWidth() const /*throw()*/ {
			return indicatorMarginAllocationWidth() + lineNumbersAllocationWidth();
		}

		/// Returns the ruler's configurations.
		inline const viewers::RulerConfiguration& RulerPainter::configuration() const /*throw()*/ {
			return configuration_;
		}

		/**
		 * Returns the width of 'allocation-rectangle' of the indicator margin in pixels.
		 * @return The width of the indicator margin or zero if not visible
		 * @see #lineNumbersWidth, #width
		 */
		inline graphics::Scalar RulerPainter::indicatorMarginAllocationWidth() const /*throw()*/ {
			return indicatorMarginContentWidth_ + indicatorMarginBorderEndWidth_;
		}

		/**
		 * Returns the width of 'allocation-rectangle' of the line numbers in pixels.
		 * @return The width of the line numbers or zero if not visible
		 * @see #indicatorMarginWidth, #width
		 */
		inline graphics::Scalar RulerPainter::lineNumbersAllocationWidth() const /*throw()*/ {
			return lineNumbersContentWidth_ + lineNumbersPaddingStartWidth_
				+ lineNumbersPaddingEndWidth_, lineNumbersBorderEndWidth_;
		}
	}

}

#endif // !ASCENSION_RULER_HPP
