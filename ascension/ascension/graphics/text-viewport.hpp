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
#include <ascension/presentation/writing-mode.hpp>
#include <boost/iterator/iterator_facade.hpp>

namespace ascension {
	namespace graphics {
		namespace font {

			class TextRenderer;
			class TextViewportListener;
			struct VisualLine;

			/**
			 */
			class TextViewport : public VisualLinesListener {
				ASCENSION_NONCOPYABLE_TAG(TextViewport);
			public:
				typedef Index ScrollOffset;
				typedef SignedIndex SignedScrollOffset;
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
				ScrollOffset firstVisibleLineInLogicalNumber() const /*throw()*/;
				ScrollOffset firstVisibleLineInVisualNumber() const /*throw()*/;
				ScrollOffset firstVisibleSublineInLogicalLine() const /*throw()*/;
				ScrollOffset inlineProgressionOffset() const /*throw()*/;
				void setBoundsInView(const NativeRectangle& bounds);
				// scrolls
				bool isScrollLocked() const /*throw()*/;
				void lockScroll();
				void scroll(const presentation::AbstractTwoAxes<SignedScrollOffset>& offsets);
				void scroll(const PhysicalTwoAxes<SignedScrollOffset>& offsets);
				void scrollTo(const presentation::AbstractTwoAxes<boost::optional<ScrollOffset>>& positions);
				void scrollTo(const PhysicalTwoAxes<boost::optional<ScrollOffset>>& positions);
				void scrollTo(const VisualLine& line, ScrollOffset ipd);
				void unlockScroll();
			private:
				explicit TextViewport(TextRenderer& textRenderer);
				void adjustBpdScrollPositions() /*throw()*/;
				void documentAccessibleRegionChanged(const kernel::Document& document);
				// VisualLinesListener
				void visualLinesDeleted(const Range<Index>& lines,
					Index sublines, bool longestLineChanged) /*throw()*/;
				void visualLinesInserted(const Range<Index>& lines) /*throw()*/;
				void visualLinesModified(
					const Range<Index>& lines, SignedIndex sublinesDifference,
					bool documentChanged, bool longestLineChanged) /*throw()*/;
			private:
				TextRenderer& textRenderer_;
				boost::signals2::scoped_connection documentAccessibleRegionChangedConnection_;
				NativeRectangle boundsInView_;
				VisualLine firstVisibleLine_;
				presentation::AbstractTwoAxes<ScrollOffset> scrollOffsets_;
				std::size_t lockCount_;
				detail::Listeners<TextViewportListener> listeners_;
				detail::Listeners<VisualLinesListener> visualLinesListeners_;
			};

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
				virtual void viewportBoundsInViewChanged(const NativeRectangle& oldBounds) /*throw()*/ = 0;
				/**
				 * @param offsets
				 * @param oldLine
				 * @param oldInlineProgressionOffset
				 * @see TextViewport#firstVisibleLineInLogicalNumber,
				 *      TextViewport#firstVisibleLineInVisualNumber,
				 *      TextViewport#firstVisibleSublineInLogicalLine,
				 *      TextViewport#inlineProgressionOffset, TextViewport#scroll,
				 *      TextViewport#scrollTo
				 */
				virtual void viewportScrollPositionChanged(
					const presentation::AbstractTwoAxes<TextViewport::SignedScrollOffset>& offsets,
					const VisualLine& oldLine, TextViewport::ScrollOffset oldInlineProgressionOffset) /*throw()*/ = 0;
				friend class TextViewport;
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
			PhysicalTwoAxes<boost::optional<TextViewport::ScrollOffset>>
			convertFlowRelativeScrollPositionsToPhysical(const TextViewport& viewport,
				const presentation::AbstractTwoAxes<boost::optional<TextViewport::ScrollOffset>>& positions);
			presentation::AbstractTwoAxes<boost::optional<TextViewport::ScrollOffset>>
			convertPhysicalScrollPositionsToAbstract(const TextViewport& viewport,
				const PhysicalTwoAxes<boost::optional<TextViewport::ScrollOffset>>& positions);
			Scalar inlineProgressionScrollOffsetInPixels(
				const TextViewport& viewport, TextViewport::ScrollOffset scrollOffset);
			Scalar lineIndent(const TextLayout& layout, Scalar contentMeasure, Index subline = 0);
			Scalar lineStartEdge(const TextLayout& layout, Scalar contentMeasure, Index subline = 0);
			VisualLine locateLine(const TextViewport& viewport,
				const NativePoint& p, bool* snapped = nullptr) /*throw()*/;
			NativePoint modelToView(const TextViewport& viewport,
				const kernel::Position& position, bool fullSearchBpd,
				graphics::font::TextLayout::Edge edge = graphics::font::TextLayout::LEADING);
			template<std::size_t coordinate> TextViewport::SignedScrollOffset pageSize(const TextViewport& viewport);
			Range<TextViewport::ScrollOffset> scrollableRangeInBlockDimension(const TextViewport& viewport);
			Range<TextViewport::ScrollOffset> scrollableRangeInInlineDimension(const TextViewport& viewport);
			template<std::size_t coordinate>
			Range<TextViewport::ScrollOffset> scrollableRangeInPhysicalDirection(const TextViewport& viewport);
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
