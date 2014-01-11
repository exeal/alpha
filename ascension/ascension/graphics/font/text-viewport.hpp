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
#include <ascension/graphics/font/text-renderer-observers.hpp>
#include <ascension/kernel/point.hpp>	// kernel.locations
#include <ascension/presentation/writing-mode.hpp>
#include <boost/flyweight.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/operators.hpp>	// boost.equality_comparable
#include <boost/optional.hpp>
#include <boost/utility/value_init.hpp>

#undef ASCENSION_PIXELFUL_SCROLL_IN_BPD	// provisional

namespace ascension {
	namespace graphics {
		namespace font {

			class TextViewportListener;
			struct VisualLine;

			/**
			 */
			class TextViewport : public DefaultFontListener, public VisualLinesListener, public ComputedBlockFlowDirectionListener {
				ASCENSION_NONCOPYABLE_TAG(TextViewport);
			public:
				typedef Index ScrollOffset;
				typedef SignedIndex SignedScrollOffset;
			public:
				~TextViewport() BOOST_NOEXCEPT;

				/// @name Text Renderer
				/// @{
				TextRenderer& textRenderer() BOOST_NOEXCEPT;
				const TextRenderer& textRenderer() const BOOST_NOEXCEPT;
				/// @}

				/// @name Observers
				/// @{
				void addListener(TextViewportListener& listener);
				void freezeNotification();
				void removeListener(TextViewportListener& listener);
				void thawNotification();
				/// @}

				/// @name Content- or Allocation-rectangles
				/// @{
				Scalar allocationMeasure() const BOOST_NOEXCEPT;
				Scalar contentMeasure() const BOOST_NOEXCEPT;
				/// @}

				/// @name View Bounds
				/// @{
				const Rectangle& boundsInView() const BOOST_NOEXCEPT;
				float numberOfVisibleLines() const BOOST_NOEXCEPT;
				void setBoundsInView(const Rectangle& bounds);
				/// @}

				/// @name View Positions
				/// @{
				const VisualLine& firstVisibleLine() const BOOST_NOEXCEPT;
				const presentation::AbstractTwoAxes<ScrollOffset>& scrollPositions() const BOOST_NOEXCEPT;
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
				ScrollOffset blockFlowScrollOffsetInFirstVisibleVisualLine() const BOOST_NOEXCEPT;
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
				/// @}

				/// @name Scrolls
				/// @{
				bool isScrollLocked() const BOOST_NOEXCEPT;
				void lockScroll();
				void scroll(const presentation::AbstractTwoAxes<SignedScrollOffset>& offsets);
				void scroll(const PhysicalTwoAxes<SignedScrollOffset>& offsets);
				void scrollBlockFlowPage(SignedScrollOffset pages);
				void scrollTo(const presentation::AbstractTwoAxes<boost::optional<ScrollOffset>>& positions);
				void scrollTo(const PhysicalTwoAxes<boost::optional<ScrollOffset>>& positions);
				void scrollTo(const VisualLine& line, ScrollOffset ipd);
				void unlockScroll();
				/// @}

			private:
				TextViewport(TextRenderer& textRenderer
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
					, const FontRenderContext& frc
#endif // ASCENSION_PIXELFUL_SCROLL_IN_BPD
				);
				void adjustBpdScrollPositions() BOOST_NOEXCEPT;
				ScrollOffset calculateBpdScrollPosition(const boost::optional<VisualLine>& line) const BOOST_NOEXCEPT;
				void documentAccessibleRegionChanged(const kernel::Document& document);
				void fireScrollPositionChanged(
					const presentation::AbstractTwoAxes<TextViewport::ScrollOffset>& positionsBeforeScroll,
					const VisualLine& firstVisibleLineBeforeScroll) BOOST_NOEXCEPT;
				void fireScrollPropertiesChanged(const presentation::AbstractTwoAxes<bool>& dimensions) BOOST_NOEXCEPT;
				void repairUncalculatedLayouts();
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
				void updateDefaultLineExtent();
#endif // ASCENSION_PIXELFUL_SCROLL_IN_BPD
				// DefaultFontListener
				void defaultFontChanged();
				// VisualLinesListener
				void visualLinesDeleted(const boost::integer_range<Index>& lines,
					Index sublines, bool longestLineChanged) BOOST_NOEXCEPT;
				void visualLinesInserted(const boost::integer_range<Index>& lines) BOOST_NOEXCEPT;
				void visualLinesModified(
					const boost::integer_range<Index>& lines, SignedIndex sublinesDifference,
					bool documentChanged, bool longestLineChanged) BOOST_NOEXCEPT;
				// ComputedBlockFlowDirectionListener
				void computedBlockFlowDirectionChanged(presentation::BlockFlowDirection used);
			private:
				TextRenderer& textRenderer_;
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
				boost::flyweight<FontRenderContext> fontRenderContext_;
#endif // ASCENSION_PIXELFUL_SCROLL_IN_BPD
				boost::signals2::scoped_connection documentAccessibleRegionChangedConnection_;
				Rectangle boundsInView_;
				presentation::AbstractTwoAxes<ScrollOffset> scrollPositions_;
				VisualLine firstVisibleLine_;
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
				boost::value_initialized<ScrollOffset> blockFlowScrollOffsetInFirstVisibleVisualLine_;
				Scalar defaultLineExtent_;
#endif // ASCENSION_PIXELFUL_SCROLL_IN_BPD
				boost::value_initialized<std::size_t> lockCount_;
				struct FrozenNotification {
					boost::value_initialized<std::size_t> count;
					struct Position : private boost::equality_comparable<Position> {
						presentation::AbstractTwoAxes<ScrollOffset> offsets;
						VisualLine line;
						bool operator==(const Position& other) const BOOST_NOEXCEPT {
							return offsets == other.offsets && line == other.line;
						}
					};
					boost::optional<Position> positionBeforeChanged;
					presentation::AbstractTwoAxes<bool> dimensionsPropertiesChanged;
					boost::optional<Rectangle> boundsBeforeChanged;
				} frozenNotification_;
				bool repairingLayouts_;
				detail::Listeners<TextViewportListener> listeners_;
			};

			typedef detail::LockGuard<
				TextViewport, &TextViewport::lockScroll, &TextViewport::unlockScroll
			> ScrollLocker;
			typedef detail::LockGuard<
				TextViewport, &TextViewport::freezeNotification, &TextViewport::thawNotification
			> TextViewportNotificationLocker;

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
					const presentation::AbstractTwoAxes<TextViewport::ScrollOffset>& positionsBeforeScroll,
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

			class BaselineIterator : public boost::iterator_facade<
				BaselineIterator, Scalar, std::random_access_iterator_tag, Scalar, std::ptrdiff_t
			> {
			public:
				explicit BaselineIterator(const TextViewport& viewport/*, bool trackOutOfViewport*/);
				BaselineIterator(const TextViewport& viewport, const VisualLine& line/*, bool trackOutOfViewport*/);
				BaselineIterator(const TextViewport& viewport, const TextHit<kernel::Position>& position/*, bool trackOutOfViewport*/);
				boost::optional<VisualLine> line() const BOOST_NOEXCEPT;
				const VisualLine& snappedLine() const BOOST_NOEXCEPT;
//				Point positionInView() const;
				const Point& positionInViewport() const;
				const TextViewport& viewport() const BOOST_NOEXCEPT;
				bool tracksOutOfViewport() const BOOST_NOEXCEPT;
			private:
				void internalAdvance(const VisualLine* to, const boost::optional<difference_type>& delta);
				void initializeWithFirstVisibleLine();
				void invalidate() BOOST_NOEXCEPT;
				bool isValid() const BOOST_NOEXCEPT;
#if 0
				void move(Index line);
#endif
				void verifyNotDone() const {
//					if(*this == BaselineIterator())
//						throw NoSuchElementException();
				}
				// boost.iterator_facade
				void advance(difference_type n);
//				difference_type distance_to(const BaseIterator& other) const;
				void decrement();
				const reference dereference() const;
				bool equal(const BaselineIterator& other) const;
				void increment();
				friend class boost::iterator_core_access;
			private:
				const TextViewport* viewport_;	// this is not a reference, for operator=
				bool tracksOutOfViewport_;	// this is not const, for operator=. this is always false
				VisualLine line_;
				Scalar distanceFromViewportBeforeEdge_;
				Point positionInViewport_;
			};

			/// @defgroup scrollable_ranges_in_viewport Scrollable Ranges in Viewport
			/// @{
			template<typename AbstractCoordinate> TextViewport::ScrollOffset pageSize(const TextViewport& viewport);
			template<std::size_t physicalCoordinate> TextViewport::SignedScrollOffset pageSize(const TextViewport& viewport);
			template<typename AbstractCoordinate> boost::integer_range<TextViewport::ScrollOffset> scrollableRange(const TextViewport& viewport);
			template<std::size_t physicalCoordinate> boost::integer_range<TextViewport::ScrollOffset> scrollableRange(const TextViewport& viewport);
			/// @}

			/// @defgroup scroll_positions_in_viewport Scroll Positions in Viewport
			/// @{
			PhysicalTwoAxes<boost::optional<TextViewport::ScrollOffset>>
				convertFlowRelativeScrollPositionsToPhysical(const TextViewport& viewport,
					const presentation::AbstractTwoAxes<boost::optional<TextViewport::ScrollOffset>>& positions);
			presentation::AbstractTwoAxes<boost::optional<TextViewport::ScrollOffset>>
				convertPhysicalScrollPositionsToAbstract(const TextViewport& viewport,
					const PhysicalTwoAxes<boost::optional<TextViewport::ScrollOffset>>& positions);
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			Scalar inlineProgressionScrollOffsetInUserUnits(
				const TextViewport& viewport, const boost::optional<TextViewport::ScrollOffset>& scrollOffset = boost::none);
#endif	// ASCENSION_ABANDONED_AT_VERSION_08
			/// @}

			/// @defgroup model_and_view_coordinates_conversions Model and View Coordinates Conversions
			/// @{
			Point lineStartEdge(const TextViewport& viewport, const VisualLine& line);
			Point lineStartEdge(TextViewport& viewport, const VisualLine& line, const LineLayoutVector::UseCalculatedLayoutTag&);
			VisualLine locateLine(const TextViewport& viewport,
				const Point& p, bool* snapped = nullptr) BOOST_NOEXCEPT;
			Point modelToView(const TextViewport& viewport, const TextHit<kernel::Position>& position/*, bool fullSearchBpd*/);
			TextHit<kernel::Position>&& viewToModel(
				const TextViewport& viewport, const Point& pointInView,
				kernel::locations::CharacterUnit snapPolicy = kernel::locations::GRAPHEME_CLUSTER);
			boost::optional<TextHit<kernel::Position>> viewToModelInBounds(
				const TextViewport& viewport, const Point& pointInView,
				kernel::locations::CharacterUnit snapPolicy = kernel::locations::GRAPHEME_CLUSTER);
			/// @}

			/// @defgroup additional_model_and_view_coordinates_conversions Additional Model and View Coordinates Conversions
			/// @{
			Scalar lineIndent(const TextLayout& layout, Scalar contentMeasure, Index subline = 0);
			Scalar lineStartEdge(const TextLayout& layout, Scalar contentMeasure, Index subline = 0);
			/// @}


			// inline implementation //////////////////////////////////////////////////////////////

			/**
			 * Returns the line the iterator addresses, or @c boost#none if out of the viewport.
			 * @return The visual line this iterator addresses
			 * @retval boost#none The iterator is out of the viewport
			 * @see #snappedLine
			 */
			inline boost::optional<VisualLine> BaselineIterator::line() const BOOST_NOEXCEPT {
				verifyNotDone();
				if(**this == std::numeric_limits<Scalar>::min() || **this == std::numeric_limits<Scalar>::max())
					return boost::none;
				return line_;
			}

			/**
			 * Returns the baseline position of the line the iterator addresses.
			 * @return The point in view-local coordinates. If the writing mode is horizontal,
			 *         x-coordinate of the point is zero, otherwise y-coordinate is zero
			 */
			inline const Point& BaselineIterator::positionInViewport() const BOOST_NOEXCEPT {
				verifyNotDone();
				return positionInViewport_;
			}

			/// Returns the viewport.
			inline const TextViewport& BaselineIterator::viewport() const BOOST_NOEXCEPT {
//				if(viewport_ == nullptr)
//					throw NullPointerException("this");
				return *viewport_;
			}

			/**
			 * Returns the line the iterator addresses. Unlike @c #line method, this returns a line
			 * snapped within the viewport.
			 * @see #line
			 */
			inline const VisualLine& BaselineIterator::snappedLine() const BOOST_NOEXCEPT {
				verifyNotDone();
				return line_;
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
