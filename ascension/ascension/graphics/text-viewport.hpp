/**
 * @file text-viewport.hpp
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2012
 * @date 2010-11-20 separated from ascension/layout.hpp
 * @date 2011-11-12 renamed from rendering.hpp
 * @date 2012-02-18 separated from text-renderer.hpp
 */

#ifndef ASCENSION_TEXT_VIEWPORT_HPP
#define ASCENSION_TEXT_VIEWPORT_HPP

//#include <ascension/config.hpp>	// ASCENSION_DEFAULT_TEXT_READING_DIRECTION
#include <ascension/graphics/geometry.hpp>
#include <ascension/graphics/line-layout-vector.hpp>
#include <ascension/kernel/point.hpp>	// kernel.locations
#include <boost/iterator/iterator_facade.hpp>

namespace ascension {

	namespace viewers {
		namespace base {
			class Widget;
		}
	}

	namespace graphics {
		namespace font {

			class TextRenderer;
			struct VisualLine;

			/**
			 * Interface for objects which are interested in change of scroll positions of a
			 * @c TextViewport.
			 * @see TextViewport#addListener, TextViewport#removeListener
			 */
			class TextViewportListener {
			private:
				/**
				 * The bounds of the text viewport was changed.
				 * @param oldBounds
				 * @see TextViewport#boundsInView, TextViewport#setBoundsInView
				 */
				virtual void viewportBoundsInViewChanged(const NativeRectangle& oldBounds) /*throw()*/ = 0;
				/**
				 * @param oldLine
				 * @param oldInlineProgressionOffset
				 * @see TextViewport#firstVisibleLineInLogicalNumber,
				 *      TextViewport#firstVisibleLineInVisualNumber,
				 *      TextViewport#firstVisibleSublineInLogicalLine,
				 *      TextViewport#inlineProgressionOffset, TextViewport#scroll,
				 *      TextViewport#scrollTo
				 */
				virtual void viewportScrollPositionChanged(
					const VisualLine& oldLine, Index oldInlineProgressionOffset) /*throw()*/ = 0;
				friend class TextViewport;
			};

			/**
			 */
			class TextViewport : public VisualLinesListener {
			public:
				TextRenderer& textRenderer() /*throw()*/;
				const TextRenderer& textRenderer() const /*throw()*/;
				// observers
				void addListener(TextViewportListener& listener);
				void addVisualLinesListener(VisualLinesListener& listener);
				void removeListener(TextViewportListener& listener);
				void removeVisualLinesListener(VisualLinesListener& listener);
				// extents
				float numberOfVisibleCharactersInLine() const /*throw()*/;
				float numberOfVisibleLines() const /*throw()*/;
				// content- or allocation-rectangles
				Scalar allocationMeasure() const /*throw()*/;
				Scalar contentMeasure() const /*throw()*/;
				// view positions
				const NativeRectangle& boundsInView() const /*throw()*/;
				Index firstVisibleLineInLogicalNumber() const /*throw()*/;
				Index firstVisibleLineInVisualNumber() const /*throw()*/;
				Index firstVisibleSublineInLogicalLine() const /*throw()*/;
				Index inlineProgressionOffset() const /*throw()*/;
				void setBoundsInView(const NativeRectangle& bounds);
				// scrolls
				bool isScrollLocked() const /*throw()*/;
				void lockScroll();
				void scroll(const NativeSize& offset, viewers::base::Widget* widget);
				void scroll(SignedIndex dbpd, SignedIndex dipd, viewers::base::Widget* widget);
				void scrollTo(const NativePoint& position, viewers::base::Widget* widget);
				void scrollTo(Index bpd, Index ipd, viewers::base::Widget* widget);
				void scrollTo(const VisualLine& line, Index ipd, viewers::base::Widget* widget);
				void unlockScroll();
			private:
				void adjustBpdScrollPositions() /*throw()*/;
				// VisualLinesListener
				void visualLinesDeleted(const Range<Index>& lines,
					Index sublines, bool longestLineChanged) /*throw()*/;
				void visualLinesInserted(const Range<Index>& lines) /*throw()*/;
				void visualLinesModified(
					const Range<Index>& lines, SignedIndex sublinesDifference,
					bool documentChanged, bool longestLineChanged) /*throw()*/;
			private:
				TextRenderer& textRenderer_;
				NativeRectangle boundsInView_;
				VisualLine firstVisibleLine_;
				struct ScrollOffsets {
					Index ipd, bpd;
				} scrollOffsets_;
				std::size_t lockCount_;
				detail::Listeners<TextViewportListener> listeners_;
				detail::Listeners<VisualLinesListener> visualLinesListeners_;
			};

			class BaselineIterator : public boost::iterator_facade<
				BaselineIterator, Scalar, std::random_access_iterator_tag, Scalar, std::ptrdiff_t
			> {
			public:
				BaselineIterator(const TextViewport& viewport, Index line, bool trackOutOfViewport);
				Index line() const /*throw()*/;
				NativePoint positionInView() const;
				const NativePoint& positionInViewport() const;
				const TextViewport& viewport() const /*throw()*/;
				bool tracksOutOfViewport() const /*throw()*/;
			private:
				void advance(difference_type n);
				void initializeWithFirstVisibleLine();
				void invalidate() /*throw()*/;
				bool isValid() const /*throw()*/;
				void move(Index line);
				// boost.iterator_facade
				void decrement();
				const reference dereference() const;
				bool equal(const BaselineIterator& other);
				void increment();
				friend class boost::iterator_core_access;
			private:
				const TextViewport* viewport_;	// this is not a reference, for operator=
				bool tracksOutOfViewport_;		// this is not const, for operator=
				graphics::font::VisualLine line_;
				Scalar distanceFromViewportBeforeEdge_;
				NativePoint positionInViewport_;
			};

			// free functions
			Scalar lineIndent(const TextLayout& layout, Scalar contentMeasure, Index subline = 0);
			Scalar lineStartEdge(const TextLayout& layout, Scalar contentMeasure, Index subline = 0);
			NativePoint modelToView(const TextViewport& viewport,
				const kernel::Position& position, bool fullSearchBpd,
				graphics::font::TextLayout::Edge edge = graphics::font::TextLayout::LEADING);
			kernel::Position viewToModel(const TextViewport& viewport,
				const NativePoint& pointInView, TextLayout::Edge edge,
				kernel::locations::CharacterUnit snapPolicy = kernel::locations::GRAPHEME_CLUSTER);
			boost::optional<kernel::Position> viewToModelInBounds(const TextViewport& viewport,
				const NativePoint& pointInView, TextLayout::Edge edge,
				kernel::locations::CharacterUnit snapPolicy = kernel::locations::GRAPHEME_CLUSTER);
		}
	}
} // namespace ascension.graphics.font

#endif // !ASCENSION_TEXT_VIEWPORT_HPP
