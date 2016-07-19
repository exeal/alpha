/**
 * @file text-viewport.cpp
 * @author exeal
 * @date 2003-2006 was LineLayout.cpp
 * @date 2006-2012, 2014
 * @date 2010-11-20 separated from ascension/layout.cpp
 * @date 2011-11-12 renamed from rendering.cpp
 * @date 2012-02-18 separated from text-renderer.hpp
 */

#include <ascension/corelib/numeric-range-algorithm/clamp.hpp>
#include <ascension/corelib/text/break-iterator.hpp>	// text.GraphemeBreakIterator
#include <ascension/graphics/font/baseline-iterator.hpp>
#include <ascension/graphics/font/line-layout-vector.hpp>
#include <ascension/graphics/font/text-layout.hpp>
#include <ascension/graphics/font/text-renderer.hpp>
#include <ascension/graphics/font/text-viewport.hpp>
#include <ascension/graphics/geometry/point-xy.hpp>
#include <ascension/graphics/geometry/algorithms/make.hpp>
#include <ascension/graphics/geometry/algorithms/translate.hpp>
#include <ascension/kernel/document-character-iterator.hpp>
#include <ascension/presentation/presentation.hpp>
#include <ascension/presentation/text-line-style.hpp>
#include <ascension/presentation/text-toplevel-style.hpp>
#include <ascension/presentation/writing-mode-mappings.hpp>
#include <boost/geometry/algorithms/equals.hpp>
#include <tuple>
#define BOOST_THREAD_NO_LIB
#include <boost/thread/lock_guard.hpp>
#undef BOOST_THREAD_NO_LIB

namespace ascension {
	namespace graphics {
		namespace font {
			// free functions /////////////////////////////////////////////////////////////////////////////////////////

			PhysicalTwoAxes<boost::optional<TextViewport::ScrollOffset>>
			convertFlowRelativeScrollPositionsToPhysical(const TextViewport& viewport,
					const presentation::FlowRelativeTwoAxes<boost::optional<TextViewport::ScrollOffset>>& positions) {
				if(const auto textRenderer = viewport.textRenderer().lock()) {
					switch(textRenderer->blockFlowDirection()) {
						case presentation::HORIZONTAL_TB:
							return PhysicalTwoAxes<boost::optional<TextViewport::ScrollOffset>>(positions.ipd(), positions.bpd());
						case presentation::VERTICAL_RL:
							return PhysicalTwoAxes<boost::optional<TextViewport::ScrollOffset>>(
								(positions.bpd() != boost::none) ?
									boost::make_optional(textRenderer->layouts().numberOfVisualLines() - *positions.bpd() - 1) : boost::none,
								positions.ipd());
						case presentation::VERTICAL_LR:
							return PhysicalTwoAxes<boost::optional<TextViewport::ScrollOffset>>(positions.bpd(), positions.ipd());
						default:
							ASCENSION_ASSERT_NOT_REACHED();
					}
				}

				static const boost::optional<TextViewport::ScrollOffset> zero(0);
				return makePhysicalTwoAxes((_x = zero, _y = zero));
			}

			presentation::FlowRelativeTwoAxes<boost::optional<TextViewport::ScrollOffset>>
			convertPhysicalScrollPositionsToAbstract(const TextViewport& viewport,
					const PhysicalTwoAxes<boost::optional<TextViewport::ScrollOffset>>& positions) {
				if(const auto textRenderer = viewport.textRenderer().lock()) {
					presentation::FlowRelativeTwoAxes<boost::optional<TextViewport::ScrollOffset>> result;
					switch(textRenderer->blockFlowDirection()) {
						case presentation::HORIZONTAL_TB:
							result.bpd() = positions.y();
							result.ipd() = positions.x();
							break;
						case presentation::VERTICAL_RL:
							result.bpd() = (positions.x() != boost::none) ?
								boost::make_optional(textRenderer->layouts().numberOfVisualLines() - *positions.x() - 1) : boost::none;
							result.ipd() = positions.y();
							break;
						case presentation::VERTICAL_LR:
							result.bpd() = positions.x();
							result.ipd() = positions.y();
							break;
						default:
							ASCENSION_ASSERT_NOT_REACHED();
					}
					return result;
				}

				static const boost::optional<TextViewport::ScrollOffset> zero(0);
				return presentation::makeFlowRelativeTwoAxes((presentation::_bpd = zero, presentation::_ipd = zero));
			}

			/**
			 * Converts the given inline progression offset in text viewer into scroll offset.
			 * @param viewport The viewport
			 * @param offset The inline progression offset in the viewer associated with @a viewport in user units. If
			 *               this is @c boost#none, @c viewport.scrollPositions().ipd() is used
			 * @return A converted inline progression scroll offset in @a viewport
			 * @see inlineProgressionOffsetInViewportScroll
			 */
			Scalar inlineProgressionOffsetInViewerGeometry(const TextViewport& viewport,
					const boost::optional<TextViewport::ScrollOffset>& offset /* = boost::none */) {
				return static_cast<Scalar>(boost::get_optional_value_or(offset, viewport.scrollPositions().ipd()) /* / viewport.dimensionRates().ipd() */);
			}

			/**
			 * Converts an inline progression scroll offset into viewer's geometry.
			 * @param viewport The viewport
			 * @param offset The inline progression scroll offset in @a viewport to convert. If this is @c boost#none,
			 *               the scroll position of @a viewport is used
			 * @return A converted inline progression offset in viewer geometry in user units
			 * @see inlineProgressionOffsetInViewerGeometry
			 */
			TextViewport::ScrollOffset inlineProgressionOffsetInViewportScroll(const TextViewport& viewport, const boost::optional<Scalar>& offset /* = boost::none */) {
#if 1
				return (offset != boost::none) ?
					static_cast<TextViewport::ScrollOffset>(boost::get(offset) /* * viewport.dimensionRates().ipd() */) : viewport.scrollPositions().ipd();
#else
				const TextViewport::ScrollOffset offset = scrollOffset ? *scrollOffset : viewport.inlineProgressionOffset();
				return viewport.averageCharacterWidth() * offset;
#endif
			}

			/**
			 * Returns distance from the edge of content-area to the edge of the specified visual line in pixels. The
			 * edges are the start side of @a layout (ex. left if left-to-right, bottom if bottom-to-top).
			 * @param layout The layout of the line
			 * @param contentMeasure The measure of 'content-rectangle' of the content-area in pixels
			 * @param subline The visual subline number
			 * @return The indentation in pixels
			 * @throw IndexOutOfBoundsException @a subline is invalid
			 * @see font#lineStartEdge
			 */
			Scalar lineIndent(const TextLayout& layout, Scalar contentMeasure, Index subline /* = 0 */) {
				// TODO: ??? Is this same as TextLayout.lineStartEdge method?
				switch(boost::native_value(layout.anchor(subline))) {
					case TextAnchor::START:
						return 0;
					case TextAnchor::MIDDLE:
						return (contentMeasure - layout.measure(subline)) / 2;
					case TextAnchor::END:
						return contentMeasure - layout.measure(subline);
					default:
						throw UnknownValueException("layout.anchor(subline) returned unknown value.");
				}
			}

			/**
			 * Returns distance from left/top-edge of the content-area to 'start-edge' of the specified line in pixels.
			 * @param layout The layout of the line
			 * @param contentMeasure The measure of 'content-rectangle' of the content-area in pixels
			 * @param subline The visual subline number
			 * @return Distance from left/top-edge of the content-area to start-edge of @a line in pixels
			 * @throw IndexOutOfBoundsException @a subline is invalid
			 * @see font#lineIndent, TextLayout#lineStartEdge, TextViewer#inlineProgressionOffsetInViewport
			 */
			Scalar lineStartEdge(const TextLayout& layout, Scalar contentMeasure, Index subline /* = 0 */) {
				const Scalar indent = lineIndent(layout, contentMeasure, subline);
				return (boost::fusion::at_key<presentation::styles::Direction>(layout.style()) == presentation::LEFT_TO_RIGHT) ? indent : contentMeasure - indent;
			}

			namespace {
				Point lineStartEdge(const TextViewport& viewport, const VisualLine& line, const TextLayout* layout) {
					const auto textRenderer(viewport.textRenderer().lock());
					assert(textRenderer.get() != nullptr);
					const presentation::FlowRelativeTwoAxes<Scalar> lineStart(
						presentation::_ipd = textRenderer->lineStartEdge(line) - viewport.scrollPositions().ipd(), presentation::_bpd = static_cast<Scalar>(0));

					const presentation::WritingMode wm((layout != nullptr) ? writingMode(*layout) : textRenderer->writingModes());
					Point result;
					{
						PhysicalTwoAxes<Scalar> temp;
						presentation::mapDimensions(wm, presentation::_from = lineStart, presentation::_to = temp);
						boost::geometry::assign(result, geometry::make<Point>(temp));
					}

					switch(textRenderer->lineRelativeAlignment()) {
						case TextRenderer::LEFT:
							geometry::x(result) += 0;
							break;
						case TextRenderer::RIGHT:
							geometry::x(result) += geometry::dx(viewport.size());
							break;
						case TextRenderer::HORIZONTAL_CENTER:
							geometry::x(result) += geometry::dx(viewport.size()) / 2;
							break;
						case TextRenderer::TOP:
							geometry::y(result) += 0;
							break;
						case TextRenderer::BOTTOM:
							geometry::y(result) += geometry::dy(viewport.size());
							break;
						case TextRenderer::VERTICAL_CENTER:
							geometry::y(result) += geometry::dy(viewport.size()) / 2;
							break;
						default:
							ASCENSION_ASSERT_NOT_REACHED();
					}

					assert(geometry::x(result) == 0 || geometry::y(result) == 0);
					return result;
				}
			}

			/**
			 * Returns the start egde of the specified line in viewport-coordinates.
			 * @param viewport The viewport
			 * @param line The line numbers
			 * @return The start edge of the @a line in viewport-coordinates, in user units. If the writing mode of the
			 *         layout is horizontal, the y-coordinate is zero. Otherwise if vertical, the x-coordinate is zero
			 * @throw IndexOutOfBoundsException @a line is invalid
			 */
			Point lineStartEdge(const TextViewport& viewport, const VisualLine& line) {
				if(const auto textRenderer = viewport.textRenderer().lock())
					return lineStartEdge(viewport, line, textRenderer->layouts().at(line.line));	// this may throw IndexOutOfBoundsException
				return boost::geometry::make_zero<Point>();
			}

			/**
			 * Returns the start egde of the specified line in viewport-coordinates.
			 * @param viewport The viewport
			 * @param line The line numbers
			 * @return The start edge of the @a line in viewport-coordinates, in user units. If the writing mode of the
			 *         layout is horizontal, the y-coordinate is zero. Otherwise if vertical, the x-coordinate is zero
			 * @throw IndexOutOfBoundsException @a line is invalid
			 */
			Point lineStartEdge(TextViewport& viewport, const VisualLine& line, const UseCalculatedLayoutTag&) {
				if(const auto textRenderer = viewport.textRenderer().lock())
					return lineStartEdge(viewport, line, &textRenderer->layouts().at(line.line, LineLayoutVector::USE_CALCULATED_LAYOUT));	// this may throw IndexOutOfBoundsException
				return boost::geometry::make_zero<Point>();
			}

			/**
			 * Converts the point in the viewport into the logical line number and visual subline offset.
			 * @param viewport The viewport
			 * @param p The point in the viewport in user coordinates
			 * @param[out] snapped @c true if there was not a line at @a p. Optional
			 * @return The visual line numbers
			 * @throw IllegalStateException @c viewport.lock() returned @c null
			 * @note This function may change the layout.
			 * @see scrollPositions, TextLayout#hitTestCharacter, TextLayout#locateLine
			 */
			VisualLine locateLine(const TextViewport& viewport, const Point& p, bool* snapped /* = nullptr */) {
				const auto textRenderer(const_cast<TextViewport&>(viewport).textRenderer().lock());
				if(textRenderer.get() == nullptr)
					throw IllegalStateException("viewport.lock() returned null.");

				// calculate block-progression-dimension
				Scalar bpd;
				switch(textRenderer->blockFlowDirection()) {
					case presentation::HORIZONTAL_TB:
						bpd = geometry::y(p) - *boost::const_begin(viewportContentExtent(viewport));
						break;
					case presentation::VERTICAL_RL:
						bpd = geometry::dx(viewport.size()) - geometry::x(p) - *boost::const_begin(viewportContentExtent(viewport));
						break;
					case presentation::VERTICAL_LR:
						bpd = geometry::x(p) - *boost::const_begin(viewportContentExtent(viewport));
						break;
					default:
						ASCENSION_ASSERT_NOT_REACHED();
				}

				// locate visual line
				LineLayoutVector& layouts = textRenderer->layouts();
				const Index nlines = layouts.document().numberOfLines();
				BaselineIterator baseline(viewport, false);
				VisualLine result(boost::get(baseline.line()));
				bool snap = true;
				if(bpd >= 0) {	// before 'before-edge' ?
					boost::optional<std::reference_wrapper<const graphics::font::TextLayout>> lastLine;
					for(; ; ++baseline) {
						const boost::optional<VisualLine> line(baseline.line());
						if(line == boost::none)	// after 'after-edge' ?
							break;
						else if(includes(baseline.extentWithHalfLeadings(), bpd)) {
							result = boost::get(line);
							snap = false;
							break;
						} else if(line->line == nlines - 1) {
							if(lastLine == boost::none)
								lastLine = layouts.at(line->line, LineLayoutVector::USE_CALCULATED_LAYOUT);
							if(line->subline == boost::get(lastLine).get().numberOfLines() - 1) {
								result = boost::get(line);
								break;
							}
						}
					}
				}

				if(snapped != nullptr)
					*snapped = snap;
				return result;
			}

			/**
			 * Converts the specified position in the document to a point in the viewport-coordinates.
			 * @param viewport The viewport
			 * @param position The document position
			 * @param fullSearchBpd If this is @c false, this method stops at before- or after-edge of the viewport. If
			 *                      this happened, the block-progression-dimension of the returned point is
			 *                      @c std#numeric_limits&lt;Scalar&gt;#max() (for the before-edge) or
			 *                      @c std#numeric_limits&lt;Scalar&gt;#lowest() (for the after-edge). If this is
			 *                      @c true, the calculation is performed completely and returns an exact location will
			 *                      (may be very slow)
			 * @return The point in viewport-coordinates in user units. The block-progression-dimension addresses the
			 *         baseline of the line, or @c std#numeric_limits#lowest() or @c std#numeric_limits#max() if
			 *         @a position is outside of @a viewport (in this case, inline-progression-dimension is zero. See
			 *         the documentation of @c font#BaselineIterator class)
			 * @throw BadPositionException @a position is outside of the document
			 * @note This function may change the layout.
			 * @see #viewToModel, #viewToModelInBounds, TextLayout#location
			 */
			Point modelToView(const TextViewport& viewport, const TextHit<kernel::Position>& position/*, bool fullSearchBpd*/) {
				const auto textRenderer(viewport.textRenderer().lock());
				if(textRenderer.get() == nullptr)
					return boost::geometry::make_zero<Point>();

				// compute alignment-point of the line
				const BaselineIterator baseline(viewport, position, false/*fullSearchBpd*/);
				Point p(baseline.positionInViewport());
				if(baseline.line() == boost::none)
					return p;	// 'position' is outside of the viewport and can't calculate more
				const Point lineStart(lineStartEdge(viewport, VisualLine(position.characterIndex().line, 0)));
				geometry::translate(
					geometry::_from = p, geometry::_to = p,
					geometry::_tx = geometry::x(lineStart), geometry::_ty = geometry::y(lineStart));
				const bool horizontal = isHorizontal(textRenderer->blockFlowDirection());

				// compute offset in the line layout
				const TextLayout* const layout = textRenderer->layouts().at(kernel::line(position.characterIndex()));
				assert(layout != nullptr);
				const TextHit<> hitInLine(position.isLeadingEdge() ?
					TextHit<>::leading(kernel::offsetInLine(position.characterIndex())) : TextHit<>::trailing(kernel::offsetInLine(position.characterIndex())));
				presentation::FlowRelativeTwoAxes<Scalar> abstractOffset(layout->hitToPoint(hitInLine));
				abstractOffset.bpd() = 0;	// because subline is already known
				PhysicalTwoAxes<Scalar> physicalOffset;
				presentation::mapDimensions(writingMode(*layout), presentation::_from = abstractOffset, presentation::_to = physicalOffset);

				// compute the result
				geometry::translate(geometry::_from = p, geometry::_to = p, geometry::_tx = physicalOffset.x(), geometry::_ty = physicalOffset.y());

				return p;
			}

			template<>
			float pageSize<presentation::BlockFlowDirection>(const TextViewport& viewport) {
				return viewport.numberOfVisibleLines();
			}

			template<>
			float pageSize<presentation::ReadingDirection>(const TextViewport& viewport) {
				if(const auto textRenderer = viewport.textRenderer().lock()) {
					const Dimension bounds = viewport.size();
					return isHorizontal(textRenderer->blockFlowDirection()) ? geometry::dx(bounds) : geometry::dy(bounds);
				}
				return 0.f;
			}

			/**
			 * @fn ascension::graphics::font::pageSize
			 * @tparam coordinate 0 for x-coordinate, and 1 for y-coordinate
			 * @param viewport The viewport gives the page
			 * @return The size of the page in user units if @a coordinate is inline-dimension, or in number of
			 *         visual lines if @a coordinate is block-dimension
			 */

			template<>
			float pageSize<0>(const TextViewport& viewport) {
				if(const auto textRenderer = viewport.textRenderer().lock()) {
					switch(textRenderer->blockFlowDirection()) {
						case presentation::HORIZONTAL_TB:
							return pageSize<presentation::ReadingDirection>(viewport);
						case presentation::VERTICAL_RL:
							return -pageSize<presentation::BlockFlowDirection>(viewport);
						case presentation::VERTICAL_LR:
							return +pageSize<presentation::BlockFlowDirection>(viewport);
						default:
							ASCENSION_ASSERT_NOT_REACHED();
					}
				}
				return 0.f;
			}

			template<>
			float pageSize<1>(const TextViewport& viewport) {
				if(const auto textRenderer = viewport.textRenderer().lock())
					return isHorizontal(textRenderer->blockFlowDirection()) ?
						pageSize<presentation::BlockFlowDirection>(viewport) : pageSize<presentation::ReadingDirection>(viewport);
				return 0.f;
			}

//			template TextViewport::SignedScrollOffset pageSize<presentation::BlockFlowDirection>(const TextViewport& viewport);
//			template TextViewport::SignedScrollOffset pageSize<presentation::ReadingDirection>(const TextViewport& viewport);
//			template TextViewport::SignedScrollOffset pageSize<0>(const TextViewport& viewport);
//			template TextViewport::SignedScrollOffset pageSize<1>(const TextViewport& viewport);

			template<>
			boost::integer_range<TextViewport::ScrollOffset> scrollableRange<presentation::BlockFlowDirection>(const TextViewport& viewport) {
				if(const auto textRenderer = viewport.textRenderer().lock())
					return boost::irange(static_cast<TextViewport::ScrollOffset>(0),
						static_cast<TextViewport::ScrollOffset>(textRenderer->layouts().numberOfVisualLines() - pageSize<presentation::BlockFlowDirection>(viewport)) + 1);
				return boost::irange<TextViewport::ScrollOffset>(0, 0);
			}

			template<>
			boost::integer_range<TextViewport::ScrollOffset> scrollableRange<presentation::ReadingDirection>(const TextViewport& viewport) {
				return boost::irange(static_cast<TextViewport::ScrollOffset>(0),
					static_cast<TextViewport::ScrollOffset>(viewport.contentMeasure() - pageSize<presentation::ReadingDirection>(viewport)) + 1);
			}

			template<>
			boost::integer_range<TextViewport::ScrollOffset> scrollableRange<0>(const TextViewport& viewport) {
				if(const auto textRenderer = viewport.textRenderer().lock())
					return isHorizontal(textRenderer->blockFlowDirection()) ?
						scrollableRange<presentation::ReadingDirection>(viewport) : scrollableRange<presentation::BlockFlowDirection>(viewport);
				return boost::irange<TextViewport::ScrollOffset>(0, 0);
			}

			template<>
			boost::integer_range<TextViewport::ScrollOffset> scrollableRange<1>(const TextViewport& viewport) {
				if(const auto textRenderer = viewport.textRenderer().lock())
					return isHorizontal(textRenderer->blockFlowDirection()) ?
						scrollableRange<presentation::BlockFlowDirection>(viewport) : scrollableRange<presentation::ReadingDirection>(viewport);
				return boost::irange<TextViewport::ScrollOffset>(0, 0);
			}

//			template boost::integer_range<TextViewport::ScrollOffset> scrollableRange<presentation::BlockFlowDirection>(const TextViewport& viewport);
//			template boost::integer_range<TextViewport::ScrollOffset> scrollableRange<presentation::ReadingDirection>(const TextViewport& viewport);
//			template boost::integer_range<TextViewport::ScrollOffset> scrollableRange<0>(const TextViewport& viewport);
//			template boost::integer_range<TextViewportScrollOffset> scrollableRange<1>(const TextViewport& viewport);

			/**
			 * Scrolls the specified viewport by the given physical dimensions.
			 * @param viewport The viewport
			 * @param pages The number of pages to scroll
			 */
			void scrollPage(TextViewport& viewport, const PhysicalTwoAxes<TextViewport::SignedScrollOffset>& pages) {
				if(const auto textRenderer = viewport.textRenderer().lock()) {
					presentation::FlowRelativeTwoAxes<TextViewport::SignedScrollOffset> delta;
					presentation::mapDimensions(textRenderer->writingModes(), presentation::_from = pages, presentation::_to = delta);
					viewport.scrollBlockFlowPage(delta.bpd());
					delta.bpd() = 0;
					delta.ipd() *= static_cast<TextViewport::ScrollOffset>(pageSize<presentation::ReadingDirection>(viewport));
					viewport.scroll(delta);
				}
			}

			/***/
			NumericRange<Scalar> viewportContentExtent(const TextViewport& viewport) BOOST_NOEXCEPT {
				if(const auto textRenderer = viewport.textRenderer().lock()) {
					const presentation::BlockFlowDirection blockFlowDirection = textRenderer->blockFlowDirection();
					const PhysicalFourSides<Scalar>& physicalSpaces = textRenderer->spaceWidths();
					Scalar spaceBefore, spaceAfter;
					switch(blockFlowDirection) {
						case presentation::HORIZONTAL_TB:
							spaceBefore = physicalSpaces.top();
							spaceAfter = physicalSpaces.bottom();
							break;
						case presentation::VERTICAL_RL:
							spaceBefore = physicalSpaces.right();
							spaceAfter = physicalSpaces.left();
							break;
						case presentation::VERTICAL_LR:
							spaceBefore = physicalSpaces.left();
							spaceAfter = physicalSpaces.right();
							break;
						default:
							ASCENSION_ASSERT_NOT_REACHED();
					}
					const Scalar borderBefore = 0, borderAfter = 0, paddingBefore = 0, paddingAfter = 0;	// TODO: Not implemented.
					const Scalar before = spaceBefore + borderBefore + paddingBefore;
					const Scalar after = (presentation::isHorizontal(blockFlowDirection) ?
						geometry::dy(viewport.size()) : geometry::dx(viewport.size())) - spaceAfter - borderAfter - paddingBefore;
					return nrange(before, after);
				}
				return nrange<Scalar>(0, 0);
			}

			namespace {
				inline Scalar mapViewportIpdToLineLayout(const TextViewport& viewport, const TextLayout& line, Scalar ipd) {
					return ipd - viewport.scrollPositions().ipd() - lineStartEdge(line, viewport.contentMeasure());
				}

				// implements viewToModel and viewToModelInBounds in font namespace.
				boost::optional<TextHit<kernel::Position>> internalViewToModel(const TextViewport& viewport,
						const Point& point, bool abortNoCharacter, kernel::locations::CharacterUnit snapPolicy) {
					const auto textRenderer(viewport.textRenderer().lock());
					if(textRenderer.get() == nullptr)
						return boost::none;

					// locate the logical line
					bool outside;
					const Index line(locateLine(viewport, point, &outside).line);
					if(abortNoCharacter && outside)
						return boost::none;
					const TextLayout* const layout = textRenderer->layouts().at(line);
					assert(layout != nullptr);
					const BaselineIterator baseline(viewport, VisualLine(line, 0), false);
					// locate the position in the line
					const presentation::WritingMode wm(writingMode(*layout));
					const bool horizontal = presentation::isHorizontal(wm.blockFlowDirection);
					const PhysicalTwoAxes<Scalar> lineLocalPoint(horizontal ?
						geometry::make<Point>((
							geometry::_x = mapViewportIpdToLineLayout(viewport, *layout, geometry::x(point)),
							geometry::_y = geometry::y(point) - geometry::y(baseline.positionInViewport())))
						: geometry::make<Point>((
							geometry::_x = geometry::x(point) - geometry::x(baseline.positionInViewport()),
							geometry::_y = mapViewportIpdToLineLayout(viewport, *layout, geometry::y(point)))));
					presentation::FlowRelativeTwoAxes<Scalar> abstractLineLocalPoint;
					presentation::mapDimensions(wm, presentation::_from = lineLocalPoint, presentation::_to = abstractLineLocalPoint);
					TextHit<> hitInLine(layout->hitTestCharacter(abstractLineLocalPoint, &outside));
					if(abortNoCharacter && outside)
						return boost::none;

					// snap intervening position to the boundary
					if(hitInLine.characterIndex() != 0 && snapPolicy != kernel::locations::UTF16_CODE_UNIT) {
						using namespace text;
						const kernel::Document& document = textRenderer->layouts().document();
						const String& s = document.lineString(line);
						const bool interveningSurrogates =
							surrogates::isLowSurrogate(s[hitInLine.characterIndex()]) && surrogates::isHighSurrogate(s[hitInLine.characterIndex() - 1]);
						const Scalar ipd = horizontal ? lineLocalPoint.x() : lineLocalPoint.y();
						if(snapPolicy == kernel::locations::UTF32_CODE_UNIT) {
							if(interveningSurrogates) {
								const Index index = hitInLine.characterIndex() - 1;
								const TextHit<> leading(TextHit<>::leading(index)), trailing(TextHit<>::trailing(index));
								const Scalar leadingIpd = layout->hitToPoint(leading).ipd();
								const Scalar trailingIpd = layout->hitToPoint(trailing).ipd();
								hitInLine = (abs(ipd - leadingIpd) <= abs(ipd - trailingIpd)) ? leading : trailing;
							}
						} else if(snapPolicy == kernel::locations::GRAPHEME_CLUSTER) {
							text::GraphemeBreakIterator<kernel::DocumentCharacterIterator> i(
								kernel::DocumentCharacterIterator(document,
									kernel::Region::makeSingleLine(line, boost::irange<Index>(0, s.length())),
									kernel::Position(line, hitInLine.characterIndex())));
							if(interveningSurrogates || !i.isBoundary(i.base())) {
								--i;
								const TextHit<> leading(TextHit<>::leading(kernel::offsetInLine(i.base().tell())));
								const TextHit<> trailing(TextHit<>::trailing(kernel::offsetInLine((++i).base().tell())));
								const Scalar leadingIpd = layout->hitToPoint(leading).ipd();
								const Scalar trailingIpd = layout->hitToPoint(trailing).ipd();
								hitInLine = (abs(ipd - leadingIpd) <= abs(ipd - trailingIpd)) ? leading : trailing;
							}
						} else
							throw UnknownValueException("snapPolicy");
					}
					return hitInLine.isLeadingEdge() ?
						TextHit<kernel::Position>::leading(kernel::Position(line, hitInLine.characterIndex()))
						: TextHit<kernel::Position>::trailing(kernel::Position(line, hitInLine.characterIndex()));
				}
			}

			/**
			 * Returns the document position nearest from the specified point.
			 * @param viewport The viewport
			 * @param point The point in viewport-coordinates. This can be outside of the viewport
			 * @param snapPolicy Which character boundary the returned position snapped to
			 * @return The document position
			 * @throw UnknownValueException @a snapPolicy is invalid
			 * @note This function may change the layout.
			 * @see #modelToView, #viewToModelInBounds, TextLayout#hitTestCharacter
			 */
			TextHit<kernel::Position> viewToModel(const TextViewport& viewport,
					const Point& point, kernel::locations::CharacterUnit snapPolicy /* = kernel::locations::GRAPHEME_CLUSTER */) {
				return boost::get(internalViewToModel(viewport, point, false, snapPolicy));
			}

			/**
			 * Returns the document position nearest from the specified point. This method returns
			 * @c boost#none immediately when @a pointInView hovered outside of the text layout (e.g. far left
			 * or right of the line, beyond the last line, ...)
			 * @param viewport The viewport
			 * @param point The point in viewport-coordinates. This can be outside of the viewport
			 * @param snapPolicy Which character boundary the returned position snapped to
			 * @return The document position, or @c boost#none if @a pointInView is outside of the layout
			 * @throw UnknownValueException @a snapPolicy is invalid
			 * @note This function may change the layout.
			 * @see #modelToView, #viewToModel, TextLayout#hitTestCharacter
			 */
			boost::optional<TextHit<kernel::Position>> viewToModelInBounds(const TextViewport& viewport,
					const Point& point, kernel::locations::CharacterUnit snapPolicy /* = k::locations::GRAPHEME_CLUSTER */) {
				return internalViewToModel(viewport, point, true, snapPolicy);
			}


			// TextViewport ///////////////////////////////////////////////////////////////////////////////////////////

			/**
			 * @typedef ascension::graphics::font::TextViewport::ResizedSignal
			 * The signal which gets emitted when the size of the text viewport was changed.
			 * @param oldSize The old size of the text viewport
			 * @see #resize, #resizedSignal, #size
			 */

			/**
			 * @typedef ascension::graphics::font::TextViewport::ScrolledSignal
			 * The signal which gets emitted when the scroll positions of the text viewport were changed.
			 * @param positionsBeforeScroll The scroll positions in abstract coordinates returned by
			 *                              @c #scrollPositions() before the scroll
			 * @param firstVisibleLineBeforeScroll The first visible line returned by @c #firstVisibleLine() before
			 *                                     the scroll
			 * @note In this case, the position was changed by only scrolling
			 * @see #firstVisibleLine, #scroll, #scrolledSignal, #scrollPositions, #scrollTo
			 */

			/**
			 * @typedef ascension::graphics::font::TextViewport::ScrollPropertiesChangedSignal
			 * The signal which gets emitted when the scroll properties (position, page size and range) were changed.
			 * @param changedDimensions Describes which dimension was changed
			 * @note In this case, the position was changed because only the layout was changed
			 * @see #scrollPropertiesChangedSignal
			 */

			/**
			 * Creates a new @c TextViewport instance.
			 * @param textRenderer The text renderer
			 * @throw NullPointerException @a textRenderer is @c null
			 */
			TextViewport::TextViewport(std::weak_ptr<TextRenderer> textRenderer
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
				, const FontRenderContext& frc
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
				) : textRenderer_(textRenderer),
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
					fontRenderContext_(frc),
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
					size_(boost::geometry::make_zero<Dimension>()),
					scrollPositions_(0, 0), firstVisibleLine_(0, 0), repairingLayouts_(false) {
				if(const auto renderer = textRenderer_.lock()) {
					documentAccessibleRegionChangedConnection_ =
						const_cast<kernel::Document&>(renderer->layouts().document()).accessibleRegionChangedSignal().connect(
							std::bind(&TextViewport::documentAccessibleRegionChanged, this, std::placeholders::_1));
					defaultFontChangedConnection_ = renderer->defaultFontChangedSignal().connect(
						std::bind(&TextViewport::defaultFontChanged, this, std::placeholders::_1));
					renderer->layouts().addVisualLinesListener(*this);
					writingModesChangedConnection_ =
						renderer->writingModesChangedSignal().connect(std::bind(&TextViewport::writingModesChanged, this, std::placeholders::_1));
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
					updateDefaultLineExtent();
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
				} else
					throw NullPointerException("textRenderer");
			}

			/// Destructor.
			TextViewport::~TextViewport() BOOST_NOEXCEPT {
				if(const auto renderer = textRenderer().lock())
					renderer->layouts().removeVisualLinesListener(*this);
			}

			/**
			 * @internal
			 * @see #calculateBpdScrollPosition
			 */
			inline void TextViewport::adjustBpdScrollPositions() BOOST_NOEXCEPT {
				if(const auto renderer = textRenderer().lock()) {
					auto newScrollPositions(scrollPositions());
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
					auto newBlockFlowScrollOffsetInFirstVisibleVisualLine(blockFlowScrollOffsetInFirstVisibleVisualLine_);
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD

					decltype(firstVisibleLine_) newFirstVisibleLine;
					const Index nlines = renderer->layouts().document().numberOfLines();
					if(firstVisibleLine().line > nlines - 1) {
						newFirstVisibleLine.line = nlines - 1;
						newFirstVisibleLine.subline = renderer->layouts().numberOfSublinesOfLine(newFirstVisibleLine.line) - 1;
					} else {
						newFirstVisibleLine.line = firstVisibleLine().line;
						newFirstVisibleLine.subline = std::min(
							firstVisibleLine().subline, renderer->layouts().numberOfSublinesOfLine(newFirstVisibleLine.line) - 1);
					}
					if(newFirstVisibleLine != firstVisibleLine()) {
						newScrollPositions.bpd() = calculateBpdScrollPosition(newFirstVisibleLine);
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
						newBlockFlowScrollOffsetInFirstVisibleVisualLine = 0;
						// TODO: Not implemented.
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
					}

					// commit
					updateScrollPositions(newScrollPositions, newFirstVisibleLine,
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
						newBlockFlowScrollOffsetInFirstVisibleVisualLine,
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
						false);
				}
			}

			/**
			 * Returns the measure of the 'allocation-rectangle'.
			 * @return The measure of the 'allocation-rectangle' in user units
			 * @see #contentMeasure
			 */
			Scalar TextViewport::allocationMeasure() const BOOST_NOEXCEPT {
				if(const auto renderer = textRenderer().lock()) {
					const bool horizontal = presentation::isHorizontal(renderer->blockFlowDirection());
					const Scalar spaces = horizontal ?
						renderer->spaceWidths().left() + renderer->spaceWidths().right()
						: renderer->spaceWidths().top() + renderer->spaceWidths().bottom();
					const Scalar borders = 0;
					const Scalar paddings = 0;
					return std::max(renderer->layouts().maximumMeasure() + spaces + borders + paddings,
						static_cast<Scalar>(horizontal ? geometry::dx(size()) : geometry::dy(size())));
				}
				return static_cast<Scalar>(0);
			}

			/**
			 * @internal Calculates the value for @c scrollPositions_.bpd() data member.
			 * @param line The first visible line numbers
			 * @return The scroll position value for @a line
			 * @see #adjustBpdScrollPositions
			 */
			inline TextViewport::ScrollOffset TextViewport::calculateBpdScrollPosition(const boost::optional<VisualLine>& line) const BOOST_NOEXCEPT {
				if(const auto renderer = textRenderer().lock()) {
					const VisualLine ln(boost::get_optional_value_or(line, firstVisibleLine()));
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
					// TODO: Not implemented.
					boost::value_initialized<ScrollOffset> newBpdOffset;
					for(Index line = 0; ; ++line) {
						const TextLayout* const layout = renderer->layouts().at(line);
						if(line == ln.line) {
							// TODO: Consider *rate* of scroll.
							newBpdOffset += static_cast<ScrollOffset>((layout != nullptr) ?
								boost::size(layout->extent(boost::irange<Index>(0, ln.subline))) : (defaultLineExtent_ * ln.subline));
							break;
						} else
							newBpdOffset += static_cast<ScrollOffset>((layout != nullptr) ? boost::size(layout->extent()) : defaultLineExtent_);
					}
#else
					return renderer->layouts().mapLogicalLineToVisualLine(ln.line) + ln.subline;
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
				}
				return 0;
			}

			/**
			 * Returns the measure of the 'content-rectangle'.
			 * @return The measure of the 'content-rectangle' in user units
			 * @see #allocationMeasure
			 */
			Scalar TextViewport::contentMeasure() const BOOST_NOEXCEPT {
				if(const auto renderer = textRenderer().lock())
					return std::max(
						renderer->layouts().maximumMeasure(),
						static_cast<Scalar>(presentation::isHorizontal(renderer->blockFlowDirection()) ?
							geometry::dx(size()) : geometry::dy(size())));
				return static_cast<Scalar>(0);
			}

			/// @see TextRenderer#DefaultFontChangedSignal
			void TextViewport::defaultFontChanged(const TextRenderer&) BOOST_NOEXCEPT {
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
				updateDefaultLineExtent();
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
				emitScrollPropertiesChanged(presentation::FlowRelativeTwoAxes<bool>(presentation::_ipd = true, presentation::_bpd = false));
			}

			/// @see kernel#AccessibleRegionChangedSignal
			void TextViewport::documentAccessibleRegionChanged(const kernel::Document& document) {
				// TODO: Not implemented.
			}

			/// @internal Invokes @c #ScrolledSignal.
			inline void TextViewport::emitScrolled(
					const presentation::FlowRelativeTwoAxes<ScrollOffset>& positionsBeforeScroll,
					const VisualLine& firstVisibleLineBeforeScroll) BOOST_NOEXCEPT {
				static const decltype(frozenNotification_.count) minimumCount;
				if(frozenNotification_.count == minimumCount)
					scrolledSignal_(positionsBeforeScroll, firstVisibleLineBeforeScroll);
				else if(frozenNotification_.positionBeforeChanged == boost::none) {
					frozenNotification_.positionBeforeChanged = FrozenNotification::Position();
					boost::get(frozenNotification_.positionBeforeChanged).offsets = positionsBeforeScroll;
					boost::get(frozenNotification_.positionBeforeChanged).line = firstVisibleLineBeforeScroll;
				}
			}

			/// @internal Invokes @c #ScrollPropertiesChangedSignal.
			inline void TextViewport::emitScrollPropertiesChanged(const presentation::FlowRelativeTwoAxes<bool>& dimensions) BOOST_NOEXCEPT {
				static const decltype(frozenNotification_.count) minimumCount;
				if(frozenNotification_.count == minimumCount)
					scrollPropertiesChangedSignal_(dimensions);
				else {
					if(dimensions.ipd())
						frozenNotification_.dimensionsPropertiesChanged.ipd() = dimensions.ipd();
					if(dimensions.bpd())
						frozenNotification_.dimensionsPropertiesChanged.bpd() = dimensions.bpd();
				}
			}

			/**
			 * Increases the freeze count for the notification.
			 * If the freeze count is non-zero, all notifications of @c TextViewportListener are stopped. The
			 * notifications are queued until the freeze count is decreased to zero.
			 * @throw std#overflow_error The freeze count is about to overflow
			 * @see #thawNotification
			 */
			void TextViewport::freezeNotification() {
				const auto c = boost::get(frozenNotification_.count);
				if(c == std::numeric_limits<std::decay<decltype(c)>::type>::max())
					throw std::overflow_error("");
				++frozenNotification_.count;
			}

			/**
			 * Increments the scroll lock count.
			 * @throw std#overflow_error
			 * @see #isScrollLocked, #unlockScroll
			 */
			void TextViewport::lockScroll() {
				const auto c = boost::get(lockCount_);
				if(c == std::numeric_limits<std::decay<decltype(c)>::type>::max())
					throw std::overflow_error("");
				++lockCount_;
			}

#if 0
			/**
			 * Returns the number of the drawable columns in the window.
			 * @return The number of columns
			 */
			float TextViewport::numberOfVisibleCharactersInLine() const BOOST_NOEXCEPT {
				const bool horizontal = isHorizontal(textRenderer().computedBlockFlowDirection());
				Scalar ipd(horizontal ? geometry::dx(boundsInView()) : geometry::dy(boundsInView()));
				if(ipd == 0)
					return 0;
			//	ipd -= horizontal ? (spaceWidths().left() + spaceWidths().right()) : (spaceWidths().top() + spaceWidths().bottom());
				return ipd / averageCharacterWidth();
			}
#endif

			/**
			 * Returns the number of the drawable visual lines in the viewport.
			 * @return The number of visual lines
			 */
			float TextViewport::numberOfVisibleLines() const BOOST_NOEXCEPT {
				const auto renderer(const_cast<TextViewport*>(this)->textRenderer().lock());
				if(renderer.get() == nullptr)
					return 0.f;

				const bool horizontal = presentation::isHorizontal(renderer->blockFlowDirection());
				Scalar bpd(horizontal ? geometry::dy(size()) : geometry::dx(size()));
				if(bpd <= 0)
					return 0;
//				bpd -= horizontal ? (spaceWidths().top() + spaceWidths().bottom()) : (spaceWidths().left() + spaceWidths().right());

				Index line = firstVisibleLine().line, nlines = 0;
				LineLayoutVector& layouts = renderer->layouts();
				const TextLayout* layout = &layouts.at(line, LineLayoutVector::USE_CALCULATED_LAYOUT);
				for(TextLayout::LineMetricsIterator lm(*layout, firstVisibleLine().subline); ; ) {
					const Scalar lineExtent = lm.height();
					if(lineExtent >= bpd)
						return nlines + bpd / lineExtent;
					bpd -= lineExtent;
					++nlines;
					if(lm.line() == layout->numberOfLines() - 1) {
						if(line == renderer->layouts().document().numberOfLines() - 1)
							return static_cast<float>(nlines);
						layout = &layouts[++line];
						lm = TextLayout::LineMetricsIterator(*layout, 0);
					} else
						++lm;
				}
			}

			void TextViewport::repairUncalculatedLayouts() {
				if(!repairingLayouts_) {
					if(const auto renderer = textRenderer().lock()) {
						ascension::detail::ValueSaver<bool> temp(repairingLayouts_);
						repairingLayouts_ = true;

						const Scalar extent = presentation::isHorizontal(renderer->blockFlowDirection()) ? geometry::dy(size()) : geometry::dx(size());
						LineLayoutVector& layouts = renderer->layouts();
						VisualLine line(firstVisibleLine());
						const TextLayout* layout = &layouts.at(line.line, LineLayoutVector::USE_CALCULATED_LAYOUT);
						Scalar bpd = 0;

						// process the partially visible line
						if(line.subline > 0)
							bpd -= boost::size(layout->extent(boost::irange(static_cast<Index>(0), line.subline)));

						// repair the following lines
						for(const Index nlines = layouts.document().numberOfLines(); ++line.line < nlines && bpd < extent; ) {
							layout = &layouts.at(line.line, LineLayoutVector::USE_CALCULATED_LAYOUT);	// repair the line
							const auto lineExtent(layout->extent());
							bpd += boost::size(lineExtent);
						}
					}
				}
			}

			/**
			 * Resets the size of the viewport.
			 * @param newSize The new bounds to set in in pixels
			 * @see #size, #resizedSignal
			 */
			void TextViewport::resize(const graphics::Dimension& newSize) {
				const graphics::Dimension oldSize(size());
				if(!geometry::equals(newSize, oldSize)) {
					boost::geometry::assign(size_, newSize);
					resizedSignal_(oldSize);
				}
			}

			/// Returns the @c #ResizedSignal signal connector.
			SignalConnector<TextViewport::ResizedSignal> TextViewport::resizedSignal() BOOST_NOEXCEPT {
				return makeSignalConnector(resizedSignal_);
			}

			namespace {
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
				void locateVisualLine(const TextViewport& viewport, TextViewport::ScrollOffset bpdFrom, const VisualLine& lineFrom,
						TextViewport::SignedScrollOffset bpd, bool dontModifyLayout, Scalar defaultLineExtent, VisualLine& line, TextViewport::ScrollOffset& offsetInVisualLine) {
					if(bpd == 0)
						return;
					const auto textRenderer(viewport.textRenderer().lock());
					const LineLayoutVector& layouts = renderer->layouts();
					const TextLayout* layout;
					line = lineFrom;

					// 'bpdFrom' points before-edge of 'lineFrom'
					if(bpd > 0) {
						TextViewport::ScrollOffset bpdToEat = bpd;
						if(line.subline > 0) {	// find in subline.. in 'line'
							if((layout = !dontModifyLayout ? &layouts.at(line.line) : layouts.atIfCached(line.line)) != nullptr) {
								for(TextLayout::LineMetricsIterator lm(layout->lineMetrics(line.subline)); ; ++line.subline, ++lm) {
									const Scalar lineExtent = lm.height();
									if(lineExtent > bpdToEat) {	// found in this layout
										offsetInVisualLine = bpdToEat;
										return;
									}
									bpdToEat -= lineExtent;
									if(line.subline == layout->numberOfLines() - 1)
										break;
								}
							} else {	// rare case
								if(defaultLineExtent > bpdToEat) {	// found in this line
									offsetInVisualLine = bpdToEat;
									return;
								}
								bpdToEat -= defaultLineExtent;
							}
						}

						for(; ; ++line.line) {
							if(line.line == textRenderer->presentation().document().numberOfLines()) {	// reached the last line
								--line.line;
								if(layout == nullptr)
									layout = !dontModifyLayout ? &layouts.at(line.line) : layouts.atIfCached(line.line);
								line.subline = (layout != nullptr) ? layout->numberOfLines() - 1 : 0;
								const Scalar lastLineExtent = (layout != nullptr) ? boost::size(layout->extent(boost::irange(line.subline, line.subline + 1))) : defaultLineExtent;
								const Scalar pageSize = isHorizontal(viewport.textRenderer().computedBlockFlowDirection()) ? geometry::dy(viewport.boundsInView()) : geometry::dx(viewport.boundsInView());
								offsetInVisualLine = (lastLineExtent > pageSize) ? lastLineExtent - pageSize : 0;
								return;
							}
							if((layout = !dontModifyLayout ? &layouts.at(line.line) : layouts.atIfCached(line.line)) != nullptr) {
								const auto locatedLine(layout->locateLine(bpdToEat, boost::none));
								if(locatedLine.second == boost::none) {	// found in this layout
									offsetInVisualLine = bpdToEat - boost::size(layout->extent(boost::irange<Index>(0, line.subline = locatedLine.first)));
									return;
								}
								assert(boost::get(locatedLine.second) == Direction::FORWARD);
								bpdToEat -= boost::size(layout->extent());
							} else {
								if(defaultLineExtent > bpdToEat) {	// found in this line
									line.subline = 0;
									offsetInVisualLine = bpdToEat;
									return;
								}
								bpdToEat -= defaultLineExtent;
							}
						}
					} else {
						// 'bpdFrom' and 'lineFrom' may points one past the last line
						TextViewport::ScrollOffset bpdToEat = -bpd;
						if(line.subline > 0) {	// find 0..subline in 'line'
							if((layout = !dontModifyLayout ? &layouts.at(line.line) : layouts.atIfCached(line.line)) != nullptr) {
								for(TextLayout::LineMetricsIterator lm(layout->lineMetrics(--line.subline)); ; --line.subline, --lm) {
									const Scalar lineExtent = lm.height();
									if(lineExtent > bpdToEat) {	// found in this layout
										offsetInVisualLine = lineExtent - bpdToEat;
										return;
									}
									bpdToEat -= lineExtent;
									if(line.subline == 0)
										break;
								}
							} else {	// rare case
								if(defaultLineExtent > bpdToEat) {	// found in this line
									offsetInVisualLine = defaultLineExtent - bpdToEat;
									return;
								}
								bpdToEat -= defaultLineExtent;
							}
						}

						while(true) {
							if(line.line == 0) {
								line.subline = 0;
								offsetInVisualLine = 0;
								return;
							}
							--line.line;
							layout = !dontModifyLayout ? &layouts.at(line.line) : layouts.atIfCached(line.line);
							const Scalar logicalLineExtent = (layout != nullptr) ? boost::size(layout->extent()) : defaultLineExtent;
							if(logicalLineExtent > bpdToEat) {	// found in this logical line
								if(layout != nullptr) {
									const auto locatedLine(layout->locateLine(logicalLineExtent - bpdToEat, boost::none));
									assert(locatedLine.second == boost::none);
									offsetInVisualLine = logicalLineExtent - bpdToEat - boost::size(layout->extent(boost::irange<Index>(0, line.subline = locatedLine.first)));
								} else {
									line.subline = 0;
									offsetInVisualLine = logicalLineExtent - bpdToEat;
								}
								return;
							}
							bpdToEat -= logicalLineExtent;
						}
					}
				}
#else
				std::tuple<VisualLine, TextViewport::ScrollOffset> locateVisualLine(const TextViewport& viewport,
						const boost::optional<TextViewport::ScrollOffset>& to, const boost::optional<VisualLine>& toLine,
						TextViewport::ScrollOffset from, const VisualLine& lineFrom) {
					assert((to != boost::none && toLine == boost::none) || (to == boost::none && toLine != boost::none));

					TextViewport::ScrollOffset bpd = from;
					VisualLine line(lineFrom);
					const auto textRenderer(viewport.textRenderer().lock());
					const TextLayout* layout = textRenderer->layouts().at(line.line);
					while((to != boost::none && boost::get(to) > bpd) || (toLine != boost::none && boost::get(toLine) > line)) {
						if(layout != nullptr) {
							if(line.subline < layout->numberOfLines() - 1) {
								++line.subline;
								++bpd;
								continue;
							}
						}
						if(line.line == textRenderer->layouts().document().numberOfLines() - 1)
							break;
						layout = textRenderer->layouts().at(++line.line);
						line.subline = 0;
						++bpd;
					}
					while((to != boost::none && boost::get(to) < bpd) || (toLine != boost::none && boost::get(toLine) < line)) {
						if(layout != nullptr) {
							if(line.subline > 0) {
								--line.subline;
								--bpd;
								continue;
							}
						}
						if(line.line == 0)
							break;
						layout = textRenderer->layouts().at(--line.line);
						line.subline = (layout != nullptr) ? layout->numberOfLines() - 1 : 0;
						--bpd;
					}

					return std::make_tuple(line, bpd);
				}
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
			}

			/**
			 * Scrolls the viewport by the specified offsets in abstract dimensions.
			 * This method does nothing if scroll is locked.
			 * @param offsets The offsets to scroll in abstract dimensions in user units
			 */
			void TextViewport::scroll(const presentation::FlowRelativeTwoAxes<SignedScrollOffset>& offsets) {
				const auto renderer(textRenderer().lock());
				if(renderer.get() == nullptr || isScrollLocked())
					return;

				auto newPositions(scrollPositions());

				// inline dimension
				if(offsets.ipd() < 0)
					newPositions.ipd() = (static_cast<ScrollOffset>(-offsets.ipd()) < scrollPositions().ipd()) ? (scrollPositions().ipd() + offsets.ipd()) : 0;
				else if(offsets.ipd() > 0) {
					const bool vertical = presentation::isVertical(renderer->blockFlowDirection());
					const Scalar maximumIpd = !vertical ? geometry::dx(size()) : geometry::dy(size());
					newPositions.ipd() = std::min(scrollPositions().ipd() + offsets.ipd(), static_cast<ScrollOffset>(contentMeasure() - maximumIpd));
				}

				// block dimension
				decltype(firstVisibleLine_) newFirstVisibleLine;
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
				decltype(blockFlowScrollOffsetInFirstVisibleVisualLine_) newBlockFlowScrollOffsetInFirstVisibleVisualLine;
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
				if(offsets.bpd() != 0) {
					boost::lock_guard<TextViewportNotificationLocker> notificationLockGuard(TextViewportNotificationLocker(this));	// the following code can change the layouts
//					const auto scrollableRangeBeforeScroll(scrollableRange<BlockFlowDirection>(*this));
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
					newPositions.bpd() += locateVisualLine(*this,
						scrollPositions().bpd(), firstVisibleLine(), offsets.bpd() - blockFlowScrollOffsetInFirstVisibleVisualLine(),
						false, defaultLineExtent_, newFirstVisibleLine, newBlockFlowScrollOffsetInFirstVisibleVisualLine);
#else
					newFirstVisibleLine = firstVisibleLine();
					newPositions.bpd() += renderer->layouts().offsetVisualLine(newFirstVisibleLine, offsets.bpd(), LineLayoutVector::USE_CALCULATED_LAYOUT);
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
					if(frozenNotification_.dimensionsPropertiesChanged.bpd()/*scrollableRange<BlockFlowDirection>(*this).back() != scrollableRangeBeforeScroll.back()*/)
						newPositions.bpd() = calculateBpdScrollPosition(newFirstVisibleLine);	// some layout might be changed in this code
				} else {
					newFirstVisibleLine = firstVisibleLine();
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
					boost::get(newBlockFlowScrollOffsetInFirstVisibleVisualLine) = blockFlowScrollOffsetInFirstVisibleVisualLine();
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
				}

				// commit
				updateScrollPositions(newPositions, newFirstVisibleLine,
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
					newBlockFlowScrollOffsetInFirstVisibleVisualLine,
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
					true);
			}

			/**
			 * Scrolls the viewport by the specified offsets in physical dimensions.
			 * This method does nothing if scroll is locked.
			 * @param offsets The offsets to scroll in physical dimensions in user units
			 */
			void TextViewport::scroll(const PhysicalTwoAxes<SignedScrollOffset>& offsets) {
				if(const auto renderer = textRenderer().lock()) {
					presentation::FlowRelativeTwoAxes<SignedScrollOffset> abstractOffsets;
					presentation::mapDimensions(renderer->writingModes(), presentation::_from = offsets, presentation::_to = abstractOffsets);
					return scroll(abstractOffsets);
				}
			}

			/**
			 * Scrolls the viewport by the specified number of pages in block flow direction.
			 * This method does nothing if scroll is locked.
			 * @param pages The number of pages to scroll in block flow direction
			 */
			void TextViewport::scrollBlockFlowPage(SignedScrollOffset pages) {
				const auto renderer(textRenderer().lock());
				if(renderer.get() == nullptr || isScrollLocked())
					return;

				const boost::integer_range<ScrollOffset> rangeBeforeScroll(scrollableRange<presentation::BlockFlowDirection>(*this));
				if(pages > 0) {
					const TextViewportNotificationLocker notificationLockGuard(this);
					for(; pages > 0 && scrollPositions().bpd() < rangeBeforeScroll.back(); --pages) {
						const presentation::FlowRelativeTwoAxes<SignedScrollOffset> delta(
							presentation::_bpd = static_cast<SignedScrollOffset>(pageSize<presentation::BlockFlowDirection>(*this)), presentation::_ipd = 0);
						scroll(delta);
					}
				} else if(pages < 0) {
					auto newPositions(scrollPositions());
					decltype(firstVisibleLine_) newFirstVisibleLine;
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
					decltype(blockFlowScrollOffsetInFirstVisibleVisualLine_) newBlockFlowScrollOffsetInFirstVisibleVisualLine;
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
					static_assert(std::is_integral<ScrollOffset>::value, "");
					{
						const TextViewportNotificationLocker notificationLockGuard(this);	// the following code can change the layouts
						LineLayoutVector& layouts = renderer->layouts();
						const Scalar bpd = presentation::isHorizontal(renderer->blockFlowDirection()) ? geometry::dy(size()) : geometry::dx(size());
						Index line = firstVisibleLine().line;
						const TextLayout* layout = &layouts.at(line, LineLayoutVector::USE_CALCULATED_LAYOUT);
						TextLayout::LineMetricsIterator lineMetrics(*layout, firstVisibleLine().subline);
						Scalar bpdInPage = 0;
						while(true) {
							if(lineMetrics.line() > 0) {
								if(bpdInPage += (--lineMetrics).height() > bpd)
									++lineMetrics;
							} else if(line > 0) {
								layout = &layouts.at(--line, LineLayoutVector::USE_CALCULATED_LAYOUT);
								if(bpdInPage += (lineMetrics = TextLayout::LineMetricsIterator(*layout, layout->numberOfLines() - 1)).height() > bpd) {
									layout = &layouts.at(++line, LineLayoutVector::USE_CALCULATED_LAYOUT);
									lineMetrics = TextLayout::LineMetricsIterator(*layout, 0);
								}
							} else
								break;
							--newPositions.bpd();

							if(bpdInPage > bpd) {
								bpdInPage = 0;
								if(++pages == 0)
									break;
							}
						}

						newFirstVisibleLine = VisualLine(line, lineMetrics.line());
						if(frozenNotification_.dimensionsPropertiesChanged.bpd()/*scrollableRange<BlockFlowDirection>(*this).back() != rangeBeforeScroll.back()*/)
							newPositions.bpd() = calculateBpdScrollPosition(newFirstVisibleLine);	// some layout might be changed in this code
					}

					// commit
					updateScrollPositions(newPositions, newFirstVisibleLine,
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
						newBlockFlowScrollOffsetInFirstVisibleVisualLine,
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
						true);
				}
			}

			/// Returns the @c #ScrolledSignal signal connector.
			SignalConnector<TextViewport::ScrolledSignal> TextViewport::scrolledSignal() BOOST_NOEXCEPT {
				return makeSignalConnector(scrolledSignal_);
			}

			/// Returns the @c #ScrollPropertiesChangedSignal signal connector.
			SignalConnector<TextViewport::ScrollPropertiesChangedSignal> TextViewport::scrollPropertiesChangedSignal() BOOST_NOEXCEPT {
				return makeSignalConnector(scrollPropertiesChangedSignal_);
			}

			/**
			 * Scrolls the viewport to the specified position in abstract dimensions.
			 * This method does nothing if scroll is locked.
			 * @param positions The destination of scroll in abstract dimensions in user units
			 */
			void TextViewport::scrollTo(const presentation::FlowRelativeTwoAxes<boost::optional<ScrollOffset>>& positions) {
				const auto renderer(textRenderer().lock());
				if(renderer.get() == nullptr || isScrollLocked())
					return;

				decltype(scrollPositions_) newPositions(
					presentation::_ipd = boost::get_optional_value_or(positions.ipd(), scrollPositions().ipd()),
					presentation::_bpd = boost::get_optional_value_or(positions.bpd(), scrollPositions().bpd()));

				// inline dimension
				if(positions.ipd() != boost::none) {
					auto range(scrollableRange<presentation::ReadingDirection>(*this));
					assert(!boost::empty(range));
					range.advance_end(-1);
					newPositions.ipd() = clamp(newPositions.ipd(), range);
				}

				// block dimension
				decltype(firstVisibleLine_) newFirstVisibleLine;
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
				decltype(blockFlowScrollOffsetInFirstVisibleVisualLine_) newBlockFlowScrollOffsetInFirstVisibleVisualLine;
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
				if(positions.bpd() != boost::none) {
//					TextViewportNotificationLocker notificationLockGuard(this);	// this code can't change the layouts, unlike #scroll
					auto range(scrollableRange<presentation::BlockFlowDirection>(*this));
					assert(!boost::empty(range));
					newPositions.bpd() = clamp(newPositions.bpd(), range);

					// locate the nearest visual line
					const Index numberOfLogicalLines = renderer->layouts().document().numberOfLines();
					ScrollOffset bpd;
					VisualLine line;
					assert(includes(range, scrollPositions().bpd()));
					if(newPositions.bpd() < scrollPositions().bpd()) {
						if(newPositions.bpd() - 0 < scrollPositions().bpd() - newPositions.bpd()) {
							bpd = 0;
							line = VisualLine(0, 0);
						} else {
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
							bpd = scrollPositions().bpd() - blockFlowScrollOffsetInFirstVisibleVisualLine();
#else
							bpd = scrollPositions().bpd();
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
							line = firstVisibleLine();
						}
					} else {
						if(newPositions.bpd() - scrollPositions().bpd() < *boost::const_end(range) - newPositions.bpd()) {
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
							bpd = scrollPositions().bpd() - blockFlowScrollOffsetInFirstVisibleVisualLine();
#else
							bpd = scrollPositions().bpd();
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
							line = firstVisibleLine();
						} else {
							if(const TextLayout* const lastLine = renderer->layouts().at(line.line = numberOfLogicalLines - 1)) {
								line.subline = lastLine->numberOfLines() - 1;
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
								bpd = *boost::const_end(range) - boost::size(lastLine->extent(boost::irange(line.subline, line.subline + 1)));
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
							} else {
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
								bpd = *boost::const_end(range) - defaultLineExtent_;
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
								line.subline = 0;
							}
#ifndef ASCENSION_PIXELFUL_SCROLL_IN_BPD
							bpd = renderer->layouts().numberOfVisualLines() - 1;
#endif	// !ASCENSION_PIXELFUL_SCROLL_IN_BPD
						}
					}

#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
					locateVisualLine(*this,
						bpd, line, boost::get(positions.bpd()) - bpd, true, defaultLineExtent_,
						newFirstVisibleLine, newBlockFlowScrollOffsetInFirstVisibleVisualLine);
#else
					std::tie(newFirstVisibleLine, newPositions.bpd()) = locateVisualLine(*this, newPositions.bpd(), boost::none, bpd, line);
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
				} else {
					newFirstVisibleLine = firstVisibleLine_;
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
					newBlockFlowScrollOffsetInFirstVisibleVisualLine = blockFlowScrollOffsetInFirstVisibleVisualLine_;
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
				}

				// commit
				updateScrollPositions(newPositions, newFirstVisibleLine,
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
					newBlockFlowScrollOffsetInFirstVisibleVisualLine,
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
					true);
			}

			/**
			 * Scrolls the viewport to the specified position.
			 * @param positions
			 */
			void TextViewport::scrollTo(const PhysicalTwoAxes<boost::optional<ScrollOffset>>& positions) {
				return scrollTo(convertPhysicalScrollPositionsToAbstract(*this, positions));
			}

			void TextViewport::scrollTo(const VisualLine& line, ScrollOffset ipd) {
				// TODO: not implemented.
			}

			/**
			 * @throw std#underflow_error
			 * @see #freezeNotification
			 */
			void TextViewport::thawNotification() {
				const decltype(frozenNotification_.count) minimum;
				if(frozenNotification_.count == minimum)
					throw std::underflow_error("");
				if(--frozenNotification_.count == minimum) {
					if(frozenNotification_.dimensionsPropertiesChanged.ipd() || frozenNotification_.dimensionsPropertiesChanged.bpd()) {
						scrollPropertiesChangedSignal_(frozenNotification_.dimensionsPropertiesChanged);
						frozenNotification_.dimensionsPropertiesChanged = presentation::FlowRelativeTwoAxes<bool>(false, false);
					}
					if(frozenNotification_.positionBeforeChanged != boost::none) {
						scrolledSignal_(boost::get(frozenNotification_.positionBeforeChanged).offsets, boost::get(frozenNotification_.positionBeforeChanged).line);
						frozenNotification_.positionBeforeChanged = boost::none;
					}
					if(frozenNotification_.sizeBeforeChanged != boost::none) {
						resizedSignal_(boost::get(frozenNotification_.sizeBeforeChanged));
						frozenNotification_.sizeBeforeChanged = boost::none;
					}
				}
			}

			/**
			 * Decrements the scroll lock count.
			 * @throw std#underflow_error
			 * @see #isScrollLocked, #lockScroll
			 */
			void TextViewport::unlockScroll() {
				const auto c = boost::get(lockCount_);
				if(c == boost::value_initialized<std::decay<decltype(c)>::type>())
					throw std::underflow_error("");
				--lockCount_;
			}

#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
			inline void TextViewport::updateDefaultLineExtent() {
				defaultLineExtent_ = textRenderer().defaultFont()->lineMetrics(String(), fontRenderContext_)->height();
			}
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD

			inline void TextViewport::updateScrollPositions(
					const presentation::FlowRelativeTwoAxes<ScrollOffset>& newScrollPositions,
					const VisualLine& newFirstVisibleLine,
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
					ScrollOffset newBlockFlowScrollOffsetInFirstVisibleVisualLine,
#endif // ASCENSION_PIXELFUL_SCROLL_IN_BPD
					bool notifySignal) BOOST_NOEXCEPT {
				if(notifySignal && newScrollPositions == scrollPositions_)
					notifySignal = false;
				std::decay<decltype(scrollPositions_)>::type scrollPositionsBeforeScroll(scrollPositions_);
				std::decay<decltype(firstVisibleLine_)>::type firstVisibleLineBeforeScroll(firstVisibleLine_);
				scrollPositions_ = newScrollPositions;
				firstVisibleLine_ = newFirstVisibleLine;
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
				blockFlowScrollOffsetInFirstVisibleVisualLine_ = newBlockFlowScrollOffsetInFirstVisibleVisualLine);
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD

				if(notifySignal)
					emitScrolled(scrollPositionsBeforeScroll, firstVisibleLineBeforeScroll);
			}

			/// @see VisualLinesListener#visualLinesDeleted
			void TextViewport::visualLinesDeleted(const boost::integer_range<Index>& lines, Index sublines, bool longestLineChanged) BOOST_NOEXCEPT {
				// see also TextViewer.visualLinesDeleted

				if(*boost::const_end(lines) < firstVisibleLine_.line) {	// deleted logical lines before visible area
					firstVisibleLine_.line -= boost::size(lines);
					scrollPositions_.bpd() -= sublines;
				} else if(includes(lines, firstVisibleLine_.line)) {	// deleted logical lines contain the first visible line
					firstVisibleLine_.subline = 0;
					adjustBpdScrollPositions();
				}
				emitScrollPropertiesChanged(presentation::FlowRelativeTwoAxes<bool>(presentation::_ipd = longestLineChanged, presentation::_bpd = true));
			}

			/// @see VisualLinesListener#visualLinesInserted
			void TextViewport::visualLinesInserted(const boost::integer_range<Index>& lines) BOOST_NOEXCEPT {
				// see also TextViewer.visualLinesInserted

				if(*boost::const_end(lines) < firstVisibleLine_.line) {	// inserted before visible area
					firstVisibleLine_.line += boost::size(lines);
					scrollPositions_.bpd() += boost::size(lines);
				} else if(*boost::const_begin(lines) == firstVisibleLine_.line && firstVisibleLine_.subline > 0) {	// inserted around the first visible line
					firstVisibleLine_.line += boost::size(lines);
					adjustBpdScrollPositions();
				}
				emitScrollPropertiesChanged(presentation::FlowRelativeTwoAxes<bool>(presentation::_ipd = true/*longestLineChanged*/, presentation::_bpd = true));
				repairUncalculatedLayouts();
			}

			/// @see VisualLinesListener#visualLinesModified
			void TextViewport::visualLinesModified(const boost::integer_range<Index>& lines,
					SignedIndex sublinesDifference, bool documentChanged, bool longestLineChanged) BOOST_NOEXCEPT {
				// see also TextViewer.visualLinesModified

				if(sublinesDifference != 0)	 {
					if(*boost::const_end(lines) < firstVisibleLine_.line)	// changed before visible area
						scrollPositions_.bpd() += sublinesDifference;
					else if(includes(lines, firstVisibleLine_.line) && firstVisibleLine_.subline > 0) {	// changed lines contain the first visible line
//						firstVisibleLine_.subline = 0;
						adjustBpdScrollPositions();
					}
				}
				emitScrollPropertiesChanged(presentation::FlowRelativeTwoAxes<bool>(presentation::_ipd = longestLineChanged, presentation::_bpd = sublinesDifference != 0));
				repairUncalculatedLayouts();
			}

			/// @see TextRenderer#WritingModesChangedSignal
			void TextViewport::writingModesChanged(const TextRenderer&) {
				emitScrollPropertiesChanged(presentation::FlowRelativeTwoAxes<bool>(presentation::_ipd = true, presentation::_bpd = true));
			}
		}
	}
}
