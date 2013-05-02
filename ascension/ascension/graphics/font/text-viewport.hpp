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
#include <ascension/graphics/font/line-layout-vector.hpp>
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
				/// @name Text Renderer
				/// @{
				TextRenderer& textRenderer() BOOST_NOEXCEPT;
				const TextRenderer& textRenderer() const BOOST_NOEXCEPT;
				/// @}

				/// @name Observers
				/// @{
				void addListener(TextViewportListener& listener);
				void addVisualLinesListener(VisualLinesListener& listener);
				void removeListener(TextViewportListener& listener);
				void removeVisualLinesListener(VisualLinesListener& listener);
				/// @}

				/// @name Extents
				/// @{
				float numberOfVisibleCharactersInLine() const /*throw()*/;
				float numberOfVisibleLines() const BOOST_NOEXCEPT;
				/// @}

				/// @name Content- or Allocation-rectangles
				/// @{
				Scalar allocationMeasure() const BOOST_NOEXCEPT;
				Scalar contentMeasure() const BOOST_NOEXCEPT;
				/// @}

				/// @name View Positions
				/// @{
				const Rectangle& boundsInView() const BOOST_NOEXCEPT;
				ScrollOffset firstVisibleLineInLogicalNumber() const BOOST_NOEXCEPT;
				ScrollOffset firstVisibleLineInVisualNumber() const BOOST_NOEXCEPT;
				ScrollOffset firstVisibleSublineInLogicalLine() const BOOST_NOEXCEPT;
				ScrollOffset inlineProgressionOffset() const BOOST_NOEXCEPT;
				void setBoundsInView(const Rectangle& bounds);
				/// @}

				/// @name Scrolls
				/// @{
				bool isScrollLocked() const BOOST_NOEXCEPT;
				void lockScroll();
				void scroll(const presentation::AbstractTwoAxes<SignedScrollOffset>& offsets);
				void scroll(const PhysicalTwoAxes<SignedScrollOffset>& offsets);
				void scrollTo(const presentation::AbstractTwoAxes<boost::optional<ScrollOffset>>& positions);
				void scrollTo(const PhysicalTwoAxes<boost::optional<ScrollOffset>>& positions);
				void scrollTo(const VisualLine& line, ScrollOffset ipd);
				void unlockScroll();
				/// @}
			private:
				explicit TextViewport(TextRenderer& textRenderer);
				void adjustBpdScrollPositions() /*throw()*/;
				void documentAccessibleRegionChanged(const kernel::Document& document);
				// VisualLinesListener
				void visualLinesDeleted(const boost::integer_range<Index>& lines,
					Index sublines, bool longestLineChanged) BOOST_NOEXCEPT;
				void visualLinesInserted(const boost::integer_range<Index>& lines) BOOST_NOEXCEPT;
				void visualLinesModified(
					const boost::integer_range<Index>& lines, SignedIndex sublinesDifference,
					bool documentChanged, bool longestLineChanged) BOOST_NOEXCEPT;
			private:
				TextRenderer& textRenderer_;
				boost::signals2::scoped_connection documentAccessibleRegionChangedConnection_;
				Rectangle boundsInView_;
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
				virtual void viewportBoundsInViewChanged(const Rectangle& oldBounds) BOOST_NOEXCEPT = 0;
				/**
				 * The scroll positions of the text viewport were changed.
				 * @param offsets The scrolled offsets in abstract coordinates
				 * @param oldLine Visual line numbers returned by
				 *                @c TextViewport#firstVisibleLineInLogicalNumber() and
				 *                @c TextViewport#firstVisibleSublineInLogicalLine()
				 * @param oldInlineProgressionOffset A value returned by
				 *                                   @c TextViewport#inlineProgressionOffset()
				 *                                   before the scroll
				 * @see TextViewport#firstVisibleLineInLogicalNumber,
				 *      TextViewport#firstVisibleLineInVisualNumber,
				 *      TextViewport#firstVisibleSublineInLogicalLine,
				 *      TextViewport#inlineProgressionOffset, TextViewport#scroll,
				 *      TextViewport#scrollTo
				 */
				virtual void viewportScrollPositionChanged(
					const presentation::AbstractTwoAxes<TextViewport::SignedScrollOffset>& offsets,
					const VisualLine& oldLine, TextViewport::ScrollOffset oldInlineProgressionOffset) BOOST_NOEXCEPT = 0;
				friend class TextViewport;
			};

			class BaselineIterator : public boost::iterator_facade<
				BaselineIterator, Scalar, std::random_access_iterator_tag, Scalar, std::ptrdiff_t
			> {
			public:
				BaselineIterator(const TextViewport& viewport, Index line, bool trackOutOfViewport);
				Index line() const BOOST_NOEXCEPT;
//				Point positionInView() const;
				const Point& positionInViewport() const;
				const TextViewport& viewport() const BOOST_NOEXCEPT;
				bool tracksOutOfViewport() const BOOST_NOEXCEPT;
			private:
				void advance(difference_type n);
				void initializeWithFirstVisibleLine();
				void invalidate() BOOST_NOEXCEPT;
				bool isValid() const BOOST_NOEXCEPT;
#if 0
				void move(Index line);
#endif
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
				Point positionInViewport_;
			};

			/// @defgroup scrollable_ranges_in_viewport Scrollable Ranges in Viewport
			/// @{
			template<std::size_t coordinate> TextViewport::SignedScrollOffset pageSize(const TextViewport& viewport);
			boost::integer_range<TextViewport::ScrollOffset> scrollableRangeInBlockDimension(const TextViewport& viewport);
			boost::integer_range<TextViewport::ScrollOffset> scrollableRangeInInlineDimension(const TextViewport& viewport);
			template<std::size_t coordinate>
			boost::integer_range<TextViewport::ScrollOffset> scrollableRangeInPhysicalDirection(const TextViewport& viewport);
			/// @}

			/// @defgroup scroll_positions_in_viewport Scroll Positions in Viewport
			/// @{
			PhysicalTwoAxes<boost::optional<TextViewport::ScrollOffset>>
				convertFlowRelativeScrollPositionsToPhysical(const TextViewport& viewport,
					const presentation::AbstractTwoAxes<boost::optional<TextViewport::ScrollOffset>>& positions);
			presentation::AbstractTwoAxes<boost::optional<TextViewport::ScrollOffset>>
				convertPhysicalScrollPositionsToAbstract(const TextViewport& viewport,
					const PhysicalTwoAxes<boost::optional<TextViewport::ScrollOffset>>& positions);
			Scalar inlineProgressionScrollOffsetInUserUnits(
				const TextViewport& viewport, const boost::optional<TextViewport::ScrollOffset>& scrollOffset = boost::none);
			/// @}

			/// @defgroup model_and_view_coordinates_conversions Model and View Coordinates Conversions
			/// @{
			Point lineStartEdge(const TextViewport& viewport, const VisualLine& line);
			VisualLine locateLine(const TextViewport& viewport,
				const Point& p, bool* snapped = nullptr) BOOST_NOEXCEPT;
			Point modelToView(const TextViewport& viewport, const TextHit<kernel::Position>& position, bool fullSearchBpd);
			TextHit<kernel::Position>&& viewToModel(
				const TextViewport& viewport, const Point& pointInView,
				kernel::locations::CharacterUnit snapPolicy = kernel::locations::GRAPHEME_CLUSTER);
			boost::optional<TextHit<kernel::Position>> viewToModelInBounds(
				const TextViewport& viewport, const Point& pointInView,
				kernel::locations::CharacterUnit snapPolicy = kernel::locations::GRAPHEME_CLUSTER);
			/// @}

			/// @defgroup
			/// @{
			Scalar lineIndent(const TextLayout& layout, Scalar contentMeasure, Index subline = 0);
			Scalar lineStartEdge(const TextLayout& layout, Scalar contentMeasure, Index subline = 0);
			/// @}


			// inline implementation //////////////////////////////////////////////////////////////

			/// Returns the line the iterator addresses.
			inline Index BaselineIterator::line() const BOOST_NOEXCEPT {return line_.line;}

			/**
			 * Returns the baseline position of the line the iterator addresses.
			 * @return The point in view-local coordinates. If the writing mode is horizontal,
			 *         x-coordinate of the point is zero, otherwise y-coordinate is zero
			 */
			inline const Point& BaselineIterator::positionInViewport() const BOOST_NOEXCEPT {
				return positionInViewport_;
			}

			/// Returns the viewport.
			inline const TextViewport& BaselineIterator::viewport() const BOOST_NOEXCEPT {
				return *viewport_;
			}

			/// Returns @c true if
			inline bool BaselineIterator::tracksOutOfViewport() const BOOST_NOEXCEPT {
				return tracksOutOfViewport_;
			}


			// TextHit specializations for kernel.Position ////////////////////////////////////////

			template<> inline TextHit<kernel::Position>&& TextHit<kernel::Position>::beforeOffset(const kernel::Position& offset) BOOST_NOEXCEPT {
				return TextHit<kernel::Position>(kernel::Position(offset.line, offset.offsetInLine - 1), false);
			}

			template<> inline kernel::Position TextHit<kernel::Position>::insertionIndex() const BOOST_NOEXCEPT {
				kernel::Position result(characterIndex());
				if(!isLeadingEdge())
					++result.offsetInLine;
				return result;
			}

			template<> inline TextHit<kernel::Position>&& TextHit<kernel::Position>::offsetHit(SignedIndex delta) const {
				if(delta > 0 && static_cast<Index>(delta) > std::numeric_limits<Index>::max() - characterIndex().offsetInLine)
					throw std::overflow_error("delta");
				else if(delta < 0 && static_cast<Index>(-delta) > characterIndex().offsetInLine)
					throw std::underflow_error("delta");
				return TextHit(kernel::Position(characterIndex().line, characterIndex().offsetInLine + delta), isLeadingEdge());
			}

			template<> inline TextHit<kernel::Position>&& TextHit<kernel::Position>::otherHit() const BOOST_NOEXCEPT {
				kernel::Position p(characterIndex());
				isLeadingEdge() ? --p.offsetInLine : ++p.offsetInLine;
				return isLeadingEdge() ? trailing(p) : leading(p);
			}
		}
	}
} // namespace ascension.graphics.font

#endif // !ASCENSION_TEXT_VIEWPORT_HPP
