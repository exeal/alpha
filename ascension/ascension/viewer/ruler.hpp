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
#include <ascension/graphics/rendering.hpp>
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
				 * Foreground color of the text. Default value is invalid color which is
				 * fallbacked to the foreground color of the system normal text.
				 */
				graphics::Color foreground;
				/**
				 * Background color of the text. Default value is invalid color which is
				 * fallbacked to the background color of the system normal text.
				 */
				graphics::Color background;
				/**
				 * Color of the border. Default value is invalid color which is fallbacked to
				 * the foreground color of the system normal text.
				 */
				graphics::Color borderColor;
				/// Border style.
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
				 * Background color. Default value is invalid color which is fallbacked to the
				 * platform-dependent color. On Win32, it is @c COLOR_3DFACE.
				 */
				graphics::Paint paint;
				/**
				 * Color of the border. Default value is invalid color which is fallbacked to
				 * the platform-dependent color. On Win32, it is @c COLOR_3DSHADOW.
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
			RulerPainter(viewers::TextViewer& viewer, bool enableDoubleBuffering) /*throw()*/;
			const viewers::RulerConfiguration& configuration() const /*throw()*/;
			void paint(graphics::PaintContext& context);
			void setConfiguration(const viewers::RulerConfiguration& configuration);
			void update() /*throw()*/;
			int width() const /*throw()*/;
		private:
			uint8_t maximumDigitsForLineNumbers() const /*throw()*/;
			void recalculateWidth() /*throw()*/;
			void updateGDIObjects() /*throw()*/;
			viewers::TextViewer& viewer_;
			viewers::RulerConfiguration configuration_;
			graphics::Scalar width_;
			uint8_t lineNumberDigitsCache_;
			win32::Handle<HPEN> indicatorMarginPen_, lineNumbersPen_;
			win32::Handle<HBRUSH> indicatorMarginBrush_, lineNumbersBrush_;
			const bool enablesDoubleBuffering_;
			win32::Handle<HDC> memoryDC_;
			win32::Handle<HBITMAP> memoryBitmap_;
		};

		/// Returns the ruler's configurations.
		inline const viewers::RulerConfiguration& RulerPainter::configuration() const /*throw()*/ {return configuration_;}

		/// Returns the width of the ruler.
		inline graphics::Scalar RulerPainter::width() const /*throw()*/ {return width_;}
	}

}

#endif // !ASCENSION_RULER_HPP
