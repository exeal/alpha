/**
 * @file text-viewport-listener.hpp
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2012
 * @date 2010-11-20 Separated from ascension/layout.hpp
 * @date 2011-11-12 Renamed from rendering.hpp
 * @date 2012-02-18 Separated from text-renderer.hpp
 * @date 2014-06-01 Seperated from text-viewport.hpp
 */

#ifndef ASCENSION_TEXT_VIEWPORT_LISTENER_HPP
#define ASCENSION_TEXT_VIEWPORT_LISTENER_HPP

//#include <ascension/config.hpp>	// ASCENSION_DEFAULT_TEXT_READING_DIRECTION
#include <ascension/directions.hpp>	// presentation.AbstractTwoAxes
#include <ascension/graphics/geometry.hpp>	// graphics.Rectangle

namespace ascension {
	namespace graphics {
		namespace font {
			struct VisualLine;
			typedef Index TextViewportScrollOffset;
			typedef SignedIndex TextViewportSignedScrollOffset;

			/**
			 * Interface for objects which are interested in change of scroll positions of a
			 * @c TextViewport.
			 * @see TextViewport#addListener, TextViewport#removeListener
			 */
			class TextViewportListener {
			private:
				/**
				 * The bounds of the text viewport was changed.
				 * @param oldBounds The old bounds in viewer-local coordinates
				 * @see TextViewport#boundsInView, TextViewport#setBoundsInView
				 */
				virtual void viewportBoundsInViewChanged(const Rectangle& oldBounds) BOOST_NOEXCEPT = 0;
				/**
				 * The scroll positions of the text viewport were changed.
				 * @param positionsBeforeScroll The scroll positions in abstract coordinates
				 *                              returned by @c TextViewport#scrollPositions()
				 *                              before the scroll
				 * @param firstVisibleLineBeforeScroll The first visible line returned by
				 *                                     @c TextViewport#firstVisibleLine() before
				 *                                     the scroll
				 * @note In this case, the position was changed by only scrolling
				 * @see TextViewport#firstVisibleLine, TextViewport#scrollPositions,
				 *      TextViewport#scroll, TextViewport#scrollTo
				 */
				virtual void viewportScrollPositionChanged(
					const presentation::AbstractTwoAxes<TextViewportScrollOffset>& positionsBeforeScroll,
					const VisualLine& firstVisibleLineBeforeScroll) BOOST_NOEXCEPT = 0;
				/**
				 * The scroll properties (position, page size and range) were changed.
				 * @param changedDimensions Describes which dimension was changed
				 * @note In this case, the position was changed because only the layout was changed
				 */
				virtual void viewportScrollPropertiesChanged(
					const presentation::AbstractTwoAxes<bool>& changedDimensions) BOOST_NOEXCEPT = 0;
				friend class TextViewport;
			};
		}
	}
} // namespace ascension.graphics.font

#endif // !ASCENSION_TEXT_VIEWPORT_HPP
