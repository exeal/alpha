/**
 * @file ruler.hpp
 * @author exeal
 * @date 2003-2006 was EditView.h
 * @date 2006-2011 was viewer.hpp
 * @date 2011-06-06 separated from viewer.hpp
 */

#ifndef ASCENSION_RULER_HPP
#define ASCENSION_RULER_HPP

#include <ascension/config.hpp>	// ASCENSION_DEFAULT_TEXT_READING_DIRECTION, ...
//#include <ascension/graphics/rendering.hpp>
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
				/// Start value of the line number. Default value is 1.
				length_t startValue;
				/// Minimum number of digits. Default value is 4.
				uint8_t minimumDigits;
				/// Padding width at the start. Default value is 6-pixel.
				presentation::Length paddingStart;
				/// Padding width at the end. Default value is 1-pixel.
				presentation::Length paddingEnd;
				/**
				 * Color or style of the text. Default value is
				 * @c resentation#Inheritable&lt;graphics#Paint&gt;() which is fallbacked to the
				 * foreground of the text run style of the viewer's presentation global text style.
				 */
				presentation::Inheritable<graphics::Paint> foreground;
				/**
				 * Color or style of the background. Default value is 
				 * @c resentation#Inheritable&lt;graphics#Paint&gt;() which is fallbacked to the
				 * background of the text run style of the viewer's presentation global text style.
				 */
				presentation::Inheritable<graphics::Paint> background;
				/**
				 * Style of the border. If the @c color is default value, fallbacked to the color
				 * of @c #foreground member. Default value is @c presentation#Border#Part().
				 */
				presentation::Border::Part border;
				/// Digit substitution type. @c DST_CONTEXTUAL can't set. Default value is @c DST_USER_DEFAULT.
				presentation::NumberSubstitution numberSubstitution;

				LineNumbers() /*throw()*/;
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
				 * @c presentation#Inheritable&lt;graphics#Scalar&gt; which means to use
				 * platform-dependent setting.
				 */
				presentation::Inheritable<presentation::Length> width;
				/**
				 * Color or style of the content. If @c color is default value, fallbacked to the
				 * platform-dependent color. Default value is @c graphics#Paint().
				 */
				graphics::Paint paint;
				/**
				 * Style of the border. If @c color is default value, fallbacked to the
				 * platform-dependent color. Default value is @c presentation#Border#Part().
				 */
				presentation::Border::Part border;

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
			const viewers::RulerConfiguration& configuration() const /*throw()*/;
			graphics::NativeRectangle indicatorMarginBounds() const /*throw()*/;
			graphics::Scalar indicatorMarginWidth() const /*throw()*/;
			graphics::NativeRectangle lineNumbersBounds() const /*throw()*/;
			graphics::Scalar lineNumbersWidth() const /*throw()*/;
			void paint(graphics::PaintContext& context);
			void setConfiguration(const viewers::RulerConfiguration& configuration);
			void update() /*throw()*/;
			graphics::Scalar width() const /*throw()*/;
		private:
			uint8_t maximumDigitsForLineNumbers() const /*throw()*/;
			void recalculateWidth() /*throw()*/;
#if defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI) && 0
			void updateGDIObjects() /*throw()*/;
#endif
		private:
			viewers::TextViewer& viewer_;
			viewers::RulerConfiguration configuration_;
			graphics::Scalar indicatorMarginContentWidth_, indicatorMarginBorderWidth_,
				lineNumbersContentWidth_, lineNumbersPaddingStartWidth_, lineNumbersPaddingEndWidth_, lineNumbersBorderWidth_;
			uint8_t lineNumberDigitsCache_;
#if defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI) && 0
			win32::Handle<HPEN> indicatorMarginPen_, lineNumbersPen_;
			win32::Handle<HBRUSH> indicatorMarginBrush_, lineNumbersBrush_;
			const bool enablesDoubleBuffering_;
			win32::Handle<HDC> memoryDC_;
			win32::Handle<HBITMAP> memoryBitmap_;
#endif
		};

		/// Returns the ruler's configurations.
		inline const viewers::RulerConfiguration& RulerPainter::configuration() const /*throw()*/ {
			return configuration_;
		}

		/**
		 * Returns the width of the indicator margin in pixels.
		 * @return The width of the indicator margin or zero if not visible
		 * @see #lineNumbersWidth, #width
		 */
		inline graphics::Scalar RulerPainter::indicatorMarginWidth() const /*throw()*/ {
			return indicatorMarginContentWidth_ + indicatorMarginBorderWidth_;
		}

		/**
		 * Returns the width of the line numbers in pixels.
		 * @return The width of the line numbers or zero if not visible
		 * @see #indicatorMarginWidth, #width
		 */
		inline graphics::Scalar RulerPainter::lineNumbersWidth() const /*throw()*/ {
			return lineNumbersContentWidth_ + lineNumbersPaddingStartWidth_ + lineNumbersPaddingEndWidth_, lineNumbersBorderWidth_;
		}

		/**
		 * Returns the width of the ruler in pixels.
		 * @return The width of the ruler or zero if not visible
		 * @see #indicatorMarginWidth, #lineNumbersWidth
		 */
		inline graphics::Scalar RulerPainter::width() const /*throw()*/ {
			return indicatorMarginWidth() + lineNumbersWidth();
		}
	}

}

#endif // !ASCENSION_RULER_HPP
