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
#include <ascension/corelib/detail/scope-guard.hpp>
#include <ascension/corelib/signals.hpp>
#include <ascension/graphics/physical-directions-dimensions.hpp>
#include <ascension/graphics/font/text-hit.hpp>
#include <ascension/graphics/font/text-viewport-base.hpp>
#include <ascension/graphics/font/visual-line.hpp>
#include <ascension/graphics/font/visual-lines-listener.hpp>
#include <ascension/graphics/geometry/dimension.hpp>
#include <ascension/graphics/geometry/point.hpp>
#include <ascension/kernel/locations.hpp>
#include <ascension/presentation/flow-relative-two-axes.hpp>
#include <boost/operators.hpp>	// boost.equality_comparable
#include <boost/optional.hpp>
#include <boost/utility/value_init.hpp>

#undef ASCENSION_PIXELFUL_SCROLL_IN_BPD	// provisional

namespace ascension {
	namespace presentation {
		struct ComputedTextToplevelStyle;
		class DeclaredTextToplevelStyle;
		class Presentation;
	}

	namespace graphics {
		namespace font {
			class TextRenderer;
			class TextViewport;
			class UseCalculatedLayoutTag;

			namespace detail {
				std::shared_ptr<TextViewport> createTextViewport(TextRenderer& textRenderer);
			}

			/**
			 */
			class TextViewport : public TextViewportBase, public VisualLinesListener, private boost::noncopyable {
			public:
				~TextViewport() BOOST_NOEXCEPT;

				/// @name Text Renderer
				/// @{
				TextRenderer& textRenderer() BOOST_NOEXCEPT;
				const TextRenderer& textRenderer() const BOOST_NOEXCEPT;
				/// @}

				/// @name Notifications
				/// @{
				void freezeNotification();
				void thawNotification();
				/// @}

				/// @name Content- or Allocation-rectangles
				/// @{
				Scalar allocationMeasure() const BOOST_NOEXCEPT;
				Scalar contentMeasure() const BOOST_NOEXCEPT;
				/// @}

				/// @name Size
				/// @{
				typedef boost::signals2::signal<void(const Dimension&)> ResizedSignal;
				float numberOfVisibleLines() const BOOST_NOEXCEPT;
				void resize(const Dimension& newSize);
				SignalConnector<ResizedSignal> resizedSignal() BOOST_NOEXCEPT;
				const Dimension& size() const BOOST_NOEXCEPT;
				/// @}

				/// @name View Positions
				/// @{
				typedef boost::signals2::signal<void(const presentation::FlowRelativeTwoAxes<ScrollOffset>&, const VisualLine&)> ScrolledSignal;
				const VisualLine& firstVisibleLine() const BOOST_NOEXCEPT;
				SignalConnector<ScrolledSignal> scrolledSignal() BOOST_NOEXCEPT;
				const presentation::FlowRelativeTwoAxes<ScrollOffset>& scrollPositions() const BOOST_NOEXCEPT;
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
				ScrollOffset blockFlowScrollOffsetInFirstVisibleVisualLine() const BOOST_NOEXCEPT;
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
				/// @}

				/// @name Scrolls
				/// @{
				typedef boost::signals2::signal<void(const presentation::FlowRelativeTwoAxes<bool>&)> ScrollPropertiesChangedSignal;
				bool isScrollLocked() const BOOST_NOEXCEPT;
				void lockScroll();
				void scroll(const presentation::FlowRelativeTwoAxes<SignedScrollOffset>& offsets);
				void scroll(const PhysicalTwoAxes<SignedScrollOffset>& offsets);
				void scrollBlockFlowPage(SignedScrollOffset pages);
				SignalConnector<ScrollPropertiesChangedSignal> scrollPropertiesChangedSignal() BOOST_NOEXCEPT;
				void scrollTo(const presentation::FlowRelativeTwoAxes<ScrollOffset>& positions);
				void scrollTo(const presentation::FlowRelativeTwoAxes<boost::optional<ScrollOffset>>& positions);
				void scrollTo(const PhysicalTwoAxes<ScrollOffset>& positions);
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
				void emitScrolled(
					const presentation::FlowRelativeTwoAxes<ScrollOffset>& positionsBeforeScroll,
					const VisualLine& firstVisibleLineBeforeScroll) BOOST_NOEXCEPT;
				void emitScrollPropertiesChanged(const presentation::FlowRelativeTwoAxes<bool>& dimensions) BOOST_NOEXCEPT;
				void repairUncalculatedLayouts();
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
				void updateDefaultLineExtent();
#endif // ASCENSION_PIXELFUL_SCROLL_IN_BPD
				void updateScrollPositions(
					const presentation::FlowRelativeTwoAxes<ScrollOffset>& newScrollPositions,
					const VisualLine& newFirstVisibleLine,
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
					ScrollOffset newBlockFlowScrollOffsetInFirstVisibleVisualLine,
#endif // ASCENSION_PIXELFUL_SCROLL_IN_BPD
					bool notifySignal) BOOST_NOEXCEPT;
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
				Dimension size_;
				presentation::FlowRelativeTwoAxes<ScrollOffset> scrollPositions_;
				VisualLine firstVisibleLine_;
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
				boost::value_initialized<ScrollOffset> blockFlowScrollOffsetInFirstVisibleVisualLine_;
				Scalar defaultLineExtent_;
#endif // ASCENSION_PIXELFUL_SCROLL_IN_BPD
				boost::value_initialized<std::size_t> lockCount_;
				struct FrozenNotification {
					boost::value_initialized<std::size_t> count;
					struct Position : private boost::equality_comparable<Position> {
						presentation::FlowRelativeTwoAxes<ScrollOffset> offsets;
						VisualLine line;
						bool operator==(const Position& other) const BOOST_NOEXCEPT {
							return offsets == other.offsets && line == other.line;
						}
					};
					boost::optional<Position> positionBeforeChanged;
					presentation::FlowRelativeTwoAxes<bool> dimensionsPropertiesChanged;
					boost::optional<Dimension> sizeBeforeChanged;
				} frozenNotification_;
				bool repairingLayouts_;
				ResizedSignal resizedSignal_;
				ScrolledSignal scrolledSignal_;
				ScrollPropertiesChangedSignal scrollPropertiesChangedSignal_;
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

			NumericRange<Scalar> viewportContentExtent(const TextViewport& viewport) BOOST_NOEXCEPT;

			/// @defgroup scrollable_ranges_in_viewport Scrollable Ranges in Viewport
			/// @{
			template<typename AbstractCoordinate> float pageSize(const TextViewport& viewport);
			template<std::size_t physicalCoordinate> float pageSize(const TextViewport& viewport);
			static_assert(std::is_integral<TextViewport::ScrollOffset>::value, "");
			template<typename AbstractCoordinate> boost::integer_range<TextViewport::ScrollOffset> scrollableRange(const TextViewport& viewport);
			template<std::size_t physicalCoordinate> boost::integer_range<TextViewport::ScrollOffset> scrollableRange(const TextViewport& viewport);
			/// @}

			/// @defgroup scroll_positions_in_viewport Scroll Positions in Viewport
			/// @{
			PhysicalTwoAxes<boost::optional<TextViewport::ScrollOffset>>
				convertFlowRelativeScrollPositionsToPhysical(const TextViewport& viewport,
					const presentation::FlowRelativeTwoAxes<boost::optional<TextViewport::ScrollOffset>>& positions);
			presentation::FlowRelativeTwoAxes<boost::optional<TextViewport::ScrollOffset>>
				convertPhysicalScrollPositionsToAbstract(const TextViewport& viewport,
					const PhysicalTwoAxes<boost::optional<TextViewport::ScrollOffset>>& positions);
			Scalar inlineProgressionOffsetInViewerGeometry(
				const TextViewport& viewport, const boost::optional<TextViewport::ScrollOffset>& scrollOffset = boost::none);
			TextViewport::ScrollOffset inlineProgressionOffsetInViewportScroll(
				const TextViewport& viewport, const boost::optional<Scalar>& ipd = boost::none);
			/// @}

			/// @defgroup model_and_viewport_coordinates_conversions Model and Viewport Coordinates Conversions
			/// @see model_and_viewer_coordinates_conversions
			/// @{
			Point lineStartEdge(const TextViewport& viewport, const VisualLine& line);
			Point lineStartEdge(TextViewport& viewport, const VisualLine& line, const UseCalculatedLayoutTag&);
			VisualLine locateLine(const TextViewport& viewport,
				const Point& p, bool* snapped = nullptr) BOOST_NOEXCEPT;
			Point modelToView(const TextViewport& viewport, const TextHit<kernel::Position>& position/*, bool fullSearchBpd*/);
			TextHit<kernel::Position> viewToModel(
				const TextViewport& viewport, const Point& point,
				kernel::locations::CharacterUnit snapPolicy = kernel::locations::GRAPHEME_CLUSTER);
			boost::optional<TextHit<kernel::Position>> viewToModelInBounds(
				const TextViewport& viewport, const Point& point,
				kernel::locations::CharacterUnit snapPolicy = kernel::locations::GRAPHEME_CLUSTER);
			/// @}

			class TextLayout;

			/// @defgroup additional_model_and_view_coordinates_conversions Additional Model and View Coordinates Conversions
			/// @{
			Scalar lineIndent(const TextLayout& layout, Scalar contentMeasure, Index subline = 0);
			Scalar lineStartEdge(const TextLayout& layout, Scalar contentMeasure, Index subline = 0);
			/// @}

			void scrollPage(TextViewport& viewport, const PhysicalTwoAxes<TextViewport::SignedScrollOffset>& pages);


			// inline implementation //////////////////////////////////////////////////////////////////////////////////

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
			inline const presentation::FlowRelativeTwoAxes<TextViewport::ScrollOffset>& TextViewport::scrollPositions() const BOOST_NOEXCEPT {
				return scrollPositions_;
			}

			/**
			 * Scrolls the viewport to the specified position in abstract dimensions.
			 * This method does nothing if scroll is locked.
			 * @param positions The destination of scroll in abstract dimensions in user units
			 */
			inline void TextViewport::scrollTo(const presentation::FlowRelativeTwoAxes<ScrollOffset>& positions) {
				 return scrollTo(presentation::FlowRelativeTwoAxes<boost::optional<ScrollOffset>>(presentation::_ipd = positions.ipd(), presentation::_bpd = positions.bpd()));
			}

			/**
			 * Scrolls the viewport to the specified position.
			 * @param positions
			 */
			inline void TextViewport::scrollTo(const PhysicalTwoAxes<ScrollOffset>& positions) {
				 return scrollTo(PhysicalTwoAxes<boost::optional<ScrollOffset>>(_x = positions.x(), _y = positions.y()));
			}

			/**
			 * Returns the size of the viewport in pixels.
			 * @see #resize, TextViewportListener#viewportResized
			 */
			inline const Dimension& TextViewport::size() const BOOST_NOEXCEPT {
				return size_;
			}

			/// Returns @c TextRenderer object.
			inline TextRenderer& TextViewport::textRenderer() BOOST_NOEXCEPT {
				return textRenderer_;
			}

			/// Returns @c TextRenderer object.
			inline const TextRenderer& TextViewport::textRenderer() const BOOST_NOEXCEPT {
				return textRenderer_;
			}


			// TextHit specializations for kernel.Position ////////////////////////////////////////

			template<> inline TextHit<kernel::Position> TextHit<kernel::Position>::beforeOffset(const kernel::Position& offset) BOOST_NOEXCEPT {
				return TextHit<kernel::Position>(kernel::Position(kernel::line(offset), kernel::offsetInLine(offset) - 1), false);
			}

			template<> inline kernel::Position TextHit<kernel::Position>::insertionIndex() const BOOST_NOEXCEPT {
				kernel::Position result(characterIndex());
				if(!isLeadingEdge())
					++result.offsetInLine;
				return result;
			}

			template<> inline TextHit<kernel::Position> TextHit<kernel::Position>::offsetHit(SignedIndex delta) const {
				if(delta > 0 && static_cast<Index>(delta) > std::numeric_limits<Index>::max() - kernel::offsetInLine(characterIndex()))
					throw std::overflow_error("delta");
				else if(delta < 0 && static_cast<Index>(-delta) > kernel::offsetInLine(characterIndex()))
					throw std::underflow_error("delta");
				return TextHit(kernel::Position(kernel::line(characterIndex()), kernel::offsetInLine(characterIndex()) + delta), isLeadingEdge());
			}

			template<> inline TextHit<kernel::Position> TextHit<kernel::Position>::otherHit() const BOOST_NOEXCEPT {
				kernel::Position p(characterIndex());
				isLeadingEdge() ? --p.offsetInLine : ++p.offsetInLine;
				return isLeadingEdge() ? trailing(p) : leading(p);
			}
		}
	}
} // namespace ascension.graphics.font

#endif // !ASCENSION_TEXT_VIEWPORT_HPP
