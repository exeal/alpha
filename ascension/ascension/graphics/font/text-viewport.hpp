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
#include <ascension/corelib/scope-guard.hpp>
#include <ascension/graphics/physical-directions-dimensions.hpp>
#include <ascension/graphics/font/line-layout-vector.hpp>
#include <ascension/graphics/font/text-hit.hpp>
#include <ascension/graphics/font/text-renderer-observers.hpp>
#include <ascension/graphics/font/text-viewport-listener.hpp>
#include <ascension/kernel/point.hpp>	// kernel.locations
#include <ascension/presentation/writing-mode.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/operators.hpp>	// boost.equality_comparable
#include <boost/optional.hpp>
#include <boost/utility/value_init.hpp>

#undef ASCENSION_PIXELFUL_SCROLL_IN_BPD	// provisional

namespace ascension {
	namespace presentation {
		struct ComputedTextToplevelStyle;
		class DeclaredTextToplevelStyle;
	}

	namespace graphics {
		namespace font {
			class TextViewport;

			namespace detail {
				std::shared_ptr<TextViewport> createTextViewport(TextRenderer& textRenderer);
			}

			/**
			 */
			class TextViewport : public VisualLinesListener, private boost::noncopyable {
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
				const presentation::FlowRelativeTwoAxes<TextViewportScrollOffset>& scrollPositions() const BOOST_NOEXCEPT;
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
				ScrollOffset blockFlowScrollOffsetInFirstVisibleVisualLine() const BOOST_NOEXCEPT;
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
				/// @}

				/// @name Scrolls
				/// @{
				bool isScrollLocked() const BOOST_NOEXCEPT;
				void lockScroll();
				void scroll(const presentation::FlowRelativeTwoAxes<TextViewportSignedScrollOffset>& offsets);
				void scroll(const PhysicalTwoAxes<TextViewportSignedScrollOffset>& offsets);
				void scrollBlockFlowPage(TextViewportSignedScrollOffset pages);
				void scrollTo(const presentation::FlowRelativeTwoAxes<TextViewportScrollOffset>& positions);
				void scrollTo(const presentation::FlowRelativeTwoAxes<boost::optional<TextViewportScrollOffset>>& positions);
				void scrollTo(const PhysicalTwoAxes<TextViewportScrollOffset>& positions);
				void scrollTo(const PhysicalTwoAxes<boost::optional<TextViewportScrollOffset>>& positions);
				void scrollTo(const VisualLine& line, TextViewportScrollOffset ipd);
				void unlockScroll();
				/// @}

			private:
				TextViewport(TextRenderer& textRenderer
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
					, const FontRenderContext& frc
#endif // ASCENSION_PIXELFUL_SCROLL_IN_BPD
				);
				void adjustBpdScrollPositions() BOOST_NOEXCEPT;
				TextViewportScrollOffset calculateBpdScrollPosition(const boost::optional<VisualLine>& line) const BOOST_NOEXCEPT;
				void documentAccessibleRegionChanged(const kernel::Document& document);
				void fireScrollPositionChanged(
					const presentation::FlowRelativeTwoAxes<TextViewportScrollOffset>& positionsBeforeScroll,
					const VisualLine& firstVisibleLineBeforeScroll) BOOST_NOEXCEPT;
				void fireScrollPropertiesChanged(const presentation::FlowRelativeTwoAxes<bool>& dimensions) BOOST_NOEXCEPT;
				void repairUncalculatedLayouts();
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
				void updateDefaultLineExtent();
#endif // ASCENSION_PIXELFUL_SCROLL_IN_BPD
				// TextRenderer.DefaultFontChangedSignal
				void defaultFontChanged(const TextRenderer& textRenderer);
				// VisualLinesListener
				void visualLinesDeleted(const boost::integer_range<Index>& lines,
					Index sublines, bool longestLineChanged) BOOST_NOEXCEPT;
				void visualLinesInserted(const boost::integer_range<Index>& lines) BOOST_NOEXCEPT;
				void visualLinesModified(
					const boost::integer_range<Index>& lines, SignedIndex sublinesDifference,
					bool documentChanged, bool longestLineChanged) BOOST_NOEXCEPT;
				// Presentation.ComputedTextToplevelStyleChangedSignal
				void computedTextToplevelStyleChanged(
					const presentation::Presentation& presentation,
					const presentation::DeclaredTextToplevelStyle& previouslyDeclared,
					const presentation::ComputedTextToplevelStyle& previouslyComputed);
			private:
				TextRenderer& textRenderer_;
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
				boost::flyweight<FontRenderContext> fontRenderContext_;
#endif // ASCENSION_PIXELFUL_SCROLL_IN_BPD
				Rectangle boundsInView_;
				presentation::FlowRelativeTwoAxes<TextViewportScrollOffset> scrollPositions_;
				VisualLine firstVisibleLine_;
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
				boost::value_initialized<ScrollOffset> blockFlowScrollOffsetInFirstVisibleVisualLine_;
				Scalar defaultLineExtent_;
#endif // ASCENSION_PIXELFUL_SCROLL_IN_BPD
				boost::value_initialized<std::size_t> lockCount_;
				struct FrozenNotification {
					boost::value_initialized<std::size_t> count;
					struct Position : private boost::equality_comparable<Position> {
						presentation::FlowRelativeTwoAxes<TextViewportScrollOffset> offsets;
						VisualLine line;
						bool operator==(const Position& other) const BOOST_NOEXCEPT {
							return offsets == other.offsets && line == other.line;
						}
					};
					boost::optional<Position> positionBeforeChanged;
					presentation::FlowRelativeTwoAxes<bool> dimensionsPropertiesChanged;
					boost::optional<Rectangle> boundsBeforeChanged;
				} frozenNotification_;
				bool repairingLayouts_;
				ascension::detail::Listeners<TextViewportListener> listeners_;
				boost::signals2::scoped_connection computedTextToplevelStyleChangedConnection_,
					documentAccessibleRegionChangedConnection_, defaultFontChangedConnection_;
				friend std::shared_ptr<TextViewport> detail::createTextViewport(TextRenderer& textRenderer);
			};

			typedef ascension::detail::MutexWithClass<
				TextViewport, &TextViewport::lockScroll, &TextViewport::unlockScroll
			> ScrollLocker;
			typedef ascension::detail::MutexWithClass<
				TextViewport, &TextViewport::freezeNotification, &TextViewport::thawNotification
			> TextViewportNotificationLocker;

			class BaselineIterator : public boost::iterators::iterator_facade<
				BaselineIterator, Scalar, boost::iterators::random_access_traversal_tag, Scalar, std::ptrdiff_t
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
				friend class boost::iterators::iterator_core_access;
			private:
				const TextViewport* viewport_;	// this is not a reference, for operator=
				bool tracksOutOfViewport_;	// this is not const, for operator=. this is always false
				VisualLine line_;
				Scalar distanceFromViewportBeforeEdge_;
				Point positionInViewport_;
			};

			/// @defgroup scrollable_ranges_in_viewport Scrollable Ranges in Viewport
			/// @{
			template<typename AbstractCoordinate> TextViewportScrollOffset pageSize(const TextViewport& viewport);
			template<std::size_t physicalCoordinate> TextViewportSignedScrollOffset pageSize(const TextViewport& viewport);
			template<typename AbstractCoordinate> boost::integer_range<TextViewportScrollOffset> scrollableRange(const TextViewport& viewport);
			template<std::size_t physicalCoordinate> boost::integer_range<TextViewportScrollOffset> scrollableRange(const TextViewport& viewport);
			/// @}

			/// @defgroup scroll_positions_in_viewport Scroll Positions in Viewport
			/// @{
			PhysicalTwoAxes<boost::optional<TextViewportScrollOffset>>
				convertFlowRelativeScrollPositionsToPhysical(const TextViewport& viewport,
					const presentation::FlowRelativeTwoAxes<boost::optional<TextViewportScrollOffset>>& positions);
			presentation::FlowRelativeTwoAxes<boost::optional<TextViewportScrollOffset>>
				convertPhysicalScrollPositionsToAbstract(const TextViewport& viewport,
					const PhysicalTwoAxes<boost::optional<TextViewportScrollOffset>>& positions);
			Scalar inlineProgressionOffsetInViewerGeometry(
				const TextViewport& viewport, const boost::optional<TextViewportScrollOffset>& scrollOffset = boost::none);
			TextViewportScrollOffset inlineProgressionOffsetInViewportScroll(
				const TextViewport& viewport, const boost::optional<Scalar>& ipd = boost::none);
			/// @}

			/// @defgroup model_and_view_coordinates_conversions Model and View Coordinates Conversions
			/// @{
			Point lineStartEdge(const TextViewport& viewport, const VisualLine& line);
			Point lineStartEdge(TextViewport& viewport, const VisualLine& line, const LineLayoutVector::UseCalculatedLayoutTag&);
			VisualLine locateLine(const TextViewport& viewport,
				const Point& p, bool* snapped = nullptr) BOOST_NOEXCEPT;
			Point modelToView(const TextViewport& viewport, const TextHit<kernel::Position>& position/*, bool fullSearchBpd*/);
			TextHit<kernel::Position> viewToModel(
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

			void scrollPage(TextViewport& viewport, const PhysicalTwoAxes<TextViewportSignedScrollOffset>& pages);


			// inline implementation //////////////////////////////////////////////////////////////////////////////////

			/**
			 * Returns the size of the viewport in viewer-local coordinate in pixels.
			 * @see #setBoundsInView
			 */
			inline const Rectangle& TextViewport::boundsInView() const BOOST_NOEXCEPT {
				return boundsInView_;
			}

			/**
			 * Returns the first visible visual line in the viewport.
			 * @see #scrollPositions
			 */
			inline const VisualLine& TextViewport::firstVisibleLine() const BOOST_NOEXCEPT {
				return firstVisibleLine_;
			}

			/**
			 * Returns @c true if the viewport scroll is locked.
			 * @see #lockScroll, unlockScroll
			 */
			inline bool TextViewport::isScrollLocked() const BOOST_NOEXCEPT {
				static const decltype(lockCount_) minimumCount;
				return boost::get(lockCount_) != minimumCount;
			}

			/**
			 * Returns the current scroll positions.
			 * @see #firstVisibleLine
			 */
			inline const presentation::FlowRelativeTwoAxes<TextViewportScrollOffset>& TextViewport::scrollPositions() const BOOST_NOEXCEPT {
				return scrollPositions_;
			}

			/**
			 * Scrolls the viewport to the specified position in abstract dimensions.
			 * This method does nothing if scroll is locked.
			 * @param positions The destination of scroll in abstract dimensions in user units
			 */
			inline void TextViewport::scrollTo(const presentation::FlowRelativeTwoAxes<TextViewportScrollOffset>& positions) {
				 return scrollTo(presentation::FlowRelativeTwoAxes<boost::optional<TextViewportScrollOffset>>(presentation::_ipd = positions.ipd(), presentation::_bpd = positions.bpd()));
			}

			/**
			 * Scrolls the viewport to the specified position.
			 * @param positions
			 */
			inline void TextViewport::scrollTo(const PhysicalTwoAxes<TextViewportScrollOffset>& positions) {
				 return scrollTo(PhysicalTwoAxes<boost::optional<TextViewportScrollOffset>>(_x = positions.x(), _y = positions.y()));
			}

			/// Returns @c TextRenderer object.
			inline TextRenderer& TextViewport::textRenderer() BOOST_NOEXCEPT {
				return textRenderer_;
			}

			/// Returns @c TextRenderer object.
			inline const TextRenderer& TextViewport::textRenderer() const BOOST_NOEXCEPT {
				return textRenderer_;
			}

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
