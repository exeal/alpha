/**
 * @file text-viewport.cpp
 * @author exeal
 * @date 2003-2006 was LineLayout.cpp
 * @date 2006-2012, 2014
 * @date 2010-11-20 separated from ascension/layout.cpp
 * @date 2011-11-12 renamed from rendering.cpp
 * @date 2012-02-18 separated from text-renderer.hpp
 */

#include <ascension/corelib/text/break-iterator.hpp>	// text.GraphemeBreakIterator
#include <ascension/graphics/font/text-renderer.hpp>
#include <ascension/graphics/font/text-viewport.hpp>
#include <ascension/kernel/document-character-iterator.hpp>
#include <ascension/presentation/text-style.hpp>
#include <ascension/presentation/writing-mode-mappings.hpp>
#include <tuple>
#define BOOST_THREAD_NO_LIB
#include <boost/thread/lock_guard.hpp>
#undef BOOST_THREAD_NO_LIB

namespace ascension {
	namespace graphics {
		namespace font {
			// free functions /////////////////////////////////////////////////////////////////////////////////////////

			PhysicalTwoAxes<boost::optional<TextViewportScrollOffset>>
			convertFlowRelativeScrollPositionsToPhysical(const TextViewport& viewport,
					const presentation::AbstractTwoAxes<boost::optional<TextViewportScrollOffset>>& positions) {
				switch(viewport.textRenderer().computedBlockFlowDirection()) {
					case presentation::HORIZONTAL_TB:
						return PhysicalTwoAxes<boost::optional<TextViewportScrollOffset>>(positions.ipd(), positions.bpd());
					case presentation::VERTICAL_RL:
						return PhysicalTwoAxes<boost::optional<TextViewportScrollOffset>>(
							(positions.bpd() != boost::none) ?
								boost::make_optional(viewport.textRenderer().layouts().numberOfVisualLines() - *positions.bpd() - 1) : boost::none,
							positions.ipd());
					case presentation::VERTICAL_LR:
						return PhysicalTwoAxes<boost::optional<TextViewportScrollOffset>>(positions.bpd(), positions.ipd());
					default:
						ASCENSION_ASSERT_NOT_REACHED();
				}
			}

			presentation::AbstractTwoAxes<boost::optional<TextViewportScrollOffset>>
			convertPhysicalScrollPositionsToAbstract(const TextViewport& viewport,
					const PhysicalTwoAxes<boost::optional<TextViewportScrollOffset>>& positions) {
				presentation::AbstractTwoAxes<boost::optional<TextViewportScrollOffset>> result;
				switch(viewport.textRenderer().computedBlockFlowDirection()) {
					case presentation::HORIZONTAL_TB:
						result.bpd() = positions.y();
						result.ipd() = positions.x();
						break;
					case presentation::VERTICAL_RL:
						result.bpd() = (positions.x() != boost::none) ?
							boost::make_optional(viewport.textRenderer().layouts().numberOfVisualLines() - *positions.x() - 1) : boost::none;
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

			/**
			 * Converts the given inline progression offset in text viewer into scroll offset.
			 * @param viewport The viewport
			 * @param offset The inline progression offset in the viewer associated with @a viewport in user units. If
			 *               this is @c boost#none, @c viewport.scrollPositions().ipd() is used
			 * @return A converted inline progression scroll offset in @a viewport
			 * @see inlineProgressionOffsetInViewportScroll
			 */
			Scalar inlineProgressionOffsetInViewerGeometry(const TextViewport& viewport, const boost::optional<TextViewportScrollOffset>& offset /* = boost::none */) {
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
			TextViewportScrollOffset inlineProgressionOffsetInViewportScroll(const TextViewport& viewport, const boost::optional<Scalar>& offset /* = boost::none */) {
#if 1
				return (offset != boost::none) ?
					static_cast<TextViewportScrollOffset>(boost::get(offset) /* * viewport.dimensionRates().ipd() */) : viewport.scrollPositions().ipd();
#else
				const TextViewportScrollOffset offset = scrollOffset ? *scrollOffset : viewport.inlineProgressionOffset();
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
				return (layout.writingMode().inlineFlowDirection == presentation::LEFT_TO_RIGHT) ? indent : contentMeasure - indent;
			}

			namespace {
				Point lineStartEdge(const TextViewport& viewport, const VisualLine& line, const TextLayout* layout) {
					const TextRenderer& renderer = viewport.textRenderer();
					const presentation::AbstractTwoAxes<Scalar> lineStart(
						presentation::_ipd = renderer.lineStartEdge(line) - viewport.scrollPositions().ipd(), presentation::_bpd = static_cast<Scalar>(0));

					presentation::WritingMode writingMode;
					if(layout != nullptr)
						writingMode = layout->writingMode();
					else {
						writingMode.blockFlowDirection = renderer.computedBlockFlowDirection();
						writingMode.inlineFlowDirection = renderer.direction().getOrInitial();
						writingMode.textOrientation = renderer.textOrientation().getOrInitial();
					}
					Point result(geometry::make<Point>(presentation::mapAbstractToPhysical(writingMode, lineStart)));

					switch(renderer.lineRelativeAlignment()) {
						case TextRenderer::LEFT:
							geometry::x(result) += 0;
							break;
						case TextRenderer::RIGHT:
							geometry::x(result) += geometry::dx(viewport.boundsInView());
							break;
						case TextRenderer::HORIZONTAL_CENTER:
							geometry::x(result) += geometry::dx(viewport.boundsInView()) / 2;
							break;
						case TextRenderer::TOP:
							geometry::y(result) += 0;
							break;
						case TextRenderer::BOTTOM:
							geometry::y(result) += geometry::dy(viewport.boundsInView());
							break;
						case TextRenderer::VERTICAL_CENTER:
							geometry::y(result) += geometry::dy(viewport.boundsInView()) / 2;
							break;
						default:
							ASCENSION_ASSERT_NOT_REACHED();
					}

					assert(geometry::x(result) == 0 || geometry::y(result) == 0);
					return (result);
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
				return lineStartEdge(viewport, line, viewport.textRenderer().layouts().at(line.line));	// this may throw IndexOutOfBoundsException
			}

			/**
			 * Returns the start egde of the specified line in viewport-coordinates.
			 * @param viewport The viewport
			 * @param line The line numbers
			 * @return The start edge of the @a line in viewport-coordinates, in user units. If the writing mode of the
			 *         layout is horizontal, the y-coordinate is zero. Otherwise if vertical, the x-coordinate is zero
			 * @throw IndexOutOfBoundsException @a line is invalid
			 */
			Point lineStartEdge(TextViewport& viewport, const VisualLine& line, const LineLayoutVector::UseCalculatedLayoutTag&) {
				return lineStartEdge(viewport, line, &viewport.textRenderer().layouts().at(line.line, LineLayoutVector::USE_CALCULATED_LAYOUT));	// this may throw IndexOutOfBoundsException
			}

			namespace {
				inline bool isNegativeVertical(const TextLayout& layout) {
					const presentation::WritingMode& writingMode = layout.writingMode();
					if(writingMode.blockFlowDirection == presentation::VERTICAL_RL)
						return presentation::resolveTextOrientation(writingMode) == presentation::SIDEWAYS_LEFT;
					else if(writingMode.blockFlowDirection == presentation::VERTICAL_LR)
						return presentation::resolveTextOrientation(writingMode) != presentation::SIDEWAYS_LEFT;
					return false;
				}

				boost::integer_range<Scalar> viewportContentExtent(const TextViewport& viewport) BOOST_NOEXCEPT {
					const presentation::BlockFlowDirection blockFlowDirection = viewport.textRenderer().computedBlockFlowDirection();
					const PhysicalFourSides<Scalar>& physicalSpaces = viewport.textRenderer().spaceWidths();
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
						geometry::dy(viewport.boundsInView()) : geometry::dx(viewport.boundsInView())) - spaceAfter - borderAfter - paddingBefore;
					return boost::irange(before, after);
				}
			}

			/**
			 * Converts the point in the viewport into the logical line number and visual subline offset.
			 * @param p The point in the viewport in user coordinates
			 * @param[out] snapped @c true if there was not a line at @a p. Optional
			 * @return The visual line numbers
			 * @note This function may change the layout.
			 * @see #location, TextLayout#hitTestCharacter, TextLayout#locateLine
			 */
			VisualLine locateLine(const TextViewport& viewport, const Point& p, bool* snapped /* = nullptr */) BOOST_NOEXCEPT {
				// calculate block-progression-dimension
				Scalar bpd;
				switch(viewport.textRenderer().computedBlockFlowDirection()) {
					case presentation::HORIZONTAL_TB:
						bpd = geometry::y(p) - *viewportContentExtent(viewport).begin();
						break;
					case presentation::VERTICAL_RL:
						bpd = geometry::dx(viewport.boundsInView()) - geometry::x(p) - *viewportContentExtent(viewport).begin();
						break;
					case presentation::VERTICAL_LR:
						bpd = geometry::x(p) - *viewportContentExtent(viewport).begin();
						break;
					default:
						ASCENSION_ASSERT_NOT_REACHED();
				}

				// locate visual line
				BaselineIterator baseline(viewport);
				if(bpd < 0) {	// before before-edge
					if(snapped != nullptr)
						*snapped = true;
					return boost::get(baseline.line());
				}
				VisualLine previousLine(boost::get(baseline.line()));
				++previousLine.line;
				const TextLayout* layout;
				bool negativeVertical;
				for(; ; ++baseline) {
					if(baseline.line() == boost::none)	// after after-edge
						break;

					const VisualLine line(boost::get(baseline.line()));
					if(line.line != previousLine.line) {
						layout = viewport.textRenderer().layouts().at(line.line);
						negativeVertical = isNegativeVertical(*layout);
					}
					const TextLayout::LineMetricsIterator lineMetrics(layout->lineMetrics(line.subline));
					const boost::integer_range<Scalar> lineExtent(
						*baseline - (!negativeVertical ? lineMetrics.ascent() : lineMetrics.descent()),
						*baseline + (!negativeVertical ? lineMetrics.descent() : lineMetrics.ascent()) + lineMetrics.leading());
					if(includes(lineExtent, bpd)) {
						if(snapped != nullptr)
							*snapped = false;
						return line;
					}
					previousLine = line;

					if(line.line == viewport.textRenderer().presentation().document().numberOfLines() - 1 && line.subline == layout->numberOfLines() - 1)
						break;
				}

				if(snapped != nullptr)
					*snapped = true;
				return previousLine;
			}

			/**
			 * Converts the specified position in the document to a point in the viewport-coordinates.
			 * @param viewport The viewport
			 * @param position The document position
			 * @param fullSearchBpd If this is @c false, this method stops at before- or after-edge of the viewport. If
			 *                      this happened, the block-progression-dimension of the returned point is
			 *                      @c std#numeric_limits&lt;Scalar&gt;::max() (for the before-edge) or
			 *                      @c std#numeric_limits&lt;Scalar&gt;::min() (for the after-edge). If this is
			 *                      @c true, the calculation is performed completely and returns an exact location will
			 *                      (may be very slow)
			 * @return The point in viewport-coordinates in user units. The block-progression-dimension addresses the
			 *         baseline of the line, or @c std#numeric_limits#min() or @c std#numeric_limits#max() if
			 *         @a position is outside of @a viewport (in this case, inline-progression-dimension is zero. See
			 *         the documentation of @c font#BaselineIterator class)
			 * @throw BadPositionException @a position is outside of the document
			 * @note This function may change the layout.
			 * @see #viewToModel, #viewToModelInBounds, TextLayout#location
			 */
			Point modelToView(const TextViewport& viewport, const TextHit<kernel::Position>& position/*, bool fullSearchBpd*/) {
				// compute alignment-point of the line
				const BaselineIterator baseline(viewport, position/*, fullSearchBpd*/);
				Point p(baseline.positionInViewport());
				if(baseline.line() == boost::none)
					return p;	// 'position' is outside of the viewport and can't calculate more
				const Point lineStart(lineStartEdge(viewport, VisualLine(position.characterIndex().line, 0)));
				geometry::translate(p, Dimension(geometry::_dx = geometry::x(lineStart), geometry::_dy = geometry::y(lineStart)));
				const bool horizontal = isHorizontal(viewport.textRenderer().computedBlockFlowDirection());

				// compute offset in the line layout
				const TextLayout* const layout = viewport.textRenderer().layouts().at(position.characterIndex().line);
				assert(layout != nullptr);
				const TextHit<> hitInLine(position.isLeadingEdge() ?
					TextHit<>::leading(position.characterIndex().offsetInLine) : TextHit<>::trailing(position.characterIndex().offsetInLine));
				presentation::AbstractTwoAxes<Scalar> abstractOffset(layout->hitToPoint(hitInLine));
				abstractOffset.bpd() = 0;	// because subline is already known
				const PhysicalTwoAxes<Scalar> physicalOffset(presentation::mapAbstractToPhysical(layout->writingMode(), abstractOffset));

				// compute the result
				geometry::translate(p, Dimension(geometry::_dx = physicalOffset.x(), geometry::_dy = physicalOffset.y()));

				return p;
			}

			template<>
			TextViewportScrollOffset pageSize<presentation::BlockFlowDirection>(const TextViewport& viewport) {
				return static_cast<TextViewportScrollOffset>(viewport.numberOfVisibleLines());
			}

			template<>
			TextViewportScrollOffset pageSize<presentation::ReadingDirection>(const TextViewport& viewport) {
				const Rectangle& bounds = viewport.boundsInView();
				return static_cast<TextViewportScrollOffset>(isHorizontal(viewport.textRenderer().computedBlockFlowDirection()) ? geometry::dx(bounds) : geometry::dy(bounds));
			}

			/**
			 * @fn ascension::graphics::font::pageSize
			 * @tparam coordinate 0 for x-coordinate, and 1 for y-coordinate
			 * @param viewport The viewport gives the page
			 * @return The size of the page in user units if @a coordinate is inline-dimension, or in number of
			 *         visual lines if @a coordinate is block-dimension
			 */

			template<>
			TextViewportSignedScrollOffset pageSize<0>(const TextViewport& viewport) {
				switch(viewport.textRenderer().computedBlockFlowDirection()) {
					case presentation::HORIZONTAL_TB:
						return pageSize<presentation::ReadingDirection>(viewport);
					case presentation::VERTICAL_RL:
						return -static_cast<TextViewportSignedScrollOffset>(pageSize<presentation::BlockFlowDirection>(viewport));
					case presentation::VERTICAL_LR:
						return +static_cast<TextViewportSignedScrollOffset>(pageSize<presentation::BlockFlowDirection>(viewport));
					default:
						ASCENSION_ASSERT_NOT_REACHED();
				}
			}

			template<>
			TextViewportSignedScrollOffset pageSize<1>(const TextViewport& viewport) {
				return isHorizontal(viewport.textRenderer().computedBlockFlowDirection()) ?
					pageSize<presentation::BlockFlowDirection>(viewport) : pageSize<presentation::ReadingDirection>(viewport);
			}

//			template TextViewport::SignedScrollOffset pageSize<presentation::BlockFlowDirection>(const TextViewport& viewport);
//			template TextViewport::SignedScrollOffset pageSize<presentation::ReadingDirection>(const TextViewport& viewport);
//			template TextViewport::SignedScrollOffset pageSize<0>(const TextViewport& viewport);
//			template TextViewport::SignedScrollOffset pageSize<1>(const TextViewport& viewport);

			template<>
			boost::integer_range<TextViewportScrollOffset> scrollableRange<presentation::BlockFlowDirection>(const TextViewport& viewport) {
				return boost::irange(static_cast<TextViewportScrollOffset>(0),
					viewport.textRenderer().layouts().numberOfVisualLines()/* - pageSize<presentation::BlockFlowDirection>(viewport) + 1*/);
			}

			template<>
			boost::integer_range<TextViewportScrollOffset> scrollableRange<presentation::ReadingDirection>(const TextViewport& viewport) {
				return boost::irange(static_cast<TextViewportScrollOffset>(0),
					static_cast<TextViewportScrollOffset>(viewport.contentMeasure()) - pageSize<presentation::ReadingDirection>(viewport) + 1);
			}

			template<>
			boost::integer_range<TextViewportScrollOffset> scrollableRange<0>(const TextViewport& viewport) {
				return isHorizontal(viewport.textRenderer().computedBlockFlowDirection()) ?
					scrollableRange<presentation::ReadingDirection>(viewport) : scrollableRange<presentation::BlockFlowDirection>(viewport);
			}

			template<>
			boost::integer_range<TextViewportScrollOffset> scrollableRange<1>(const TextViewport& viewport) {
				return isHorizontal(viewport.textRenderer().computedBlockFlowDirection()) ?
					scrollableRange<presentation::BlockFlowDirection>(viewport) : scrollableRange<presentation::ReadingDirection>(viewport);
			}

//			template boost::integer_range<TextViewportScrollOffset> scrollableRange<presentation::BlockFlowDirection>(const TextViewport& viewport);
//			template boost::integer_range<TextViewportScrollOffset> scrollableRange<presentation::ReadingDirection>(const TextViewport& viewport);
//			template boost::integer_range<TextViewportScrollOffset> scrollableRange<0>(const TextViewport& viewport);
//			template boost::integer_range<TextViewportScrollOffset> scrollableRange<1>(const TextViewport& viewport);

			/**
			 * Scrolls the specified viewport by the given physical dimensions.
			 * @param viewport The viewport
			 * @param pages The number of pages to scroll
			 */
			void scrollPage(TextViewport& viewport, const PhysicalTwoAxes<TextViewportSignedScrollOffset>& pages) {
				presentation::AbstractTwoAxes<TextViewportSignedScrollOffset> delta =
					mapPhysicalToAbstract(viewport.textRenderer().presentation().computeWritingMode(&viewport.textRenderer()), pages);
				viewport.scrollBlockFlowPage(delta.bpd());
				delta.bpd() = 0;
				delta.ipd() *= pageSize<presentation::ReadingDirection>(viewport);
				viewport.scroll(delta);
			}

			namespace {
				inline Scalar mapViewportIpdToLineLayout(const TextViewport& viewport, const TextLayout& line, Scalar ipd) {
					return ipd - viewport.scrollPositions().ipd() - lineStartEdge(line, viewport.contentMeasure());
				}

				// implements viewToModel and viewToModelInBounds in font namespace.
				boost::optional<TextHit<kernel::Position>> internalViewToModel(const TextViewport& viewport,
						const Point& pointInView, bool abortNoCharacter, kernel::locations::CharacterUnit snapPolicy) {
					Point p(pointInView);
					geometry::translate(p, Dimension(geometry::_dx = geometry::left(viewport.boundsInView()), geometry::_dy = geometry::top(viewport.boundsInView())));

					// locate the logical line
					bool outside;
					const VisualLine line(locateLine(viewport, p, &outside));
					if(abortNoCharacter && outside)
						return boost::none;
					const TextLayout* const layout = viewport.textRenderer().layouts().at(line.line);
					assert(layout != nullptr);
					const BaselineIterator baseline(viewport, line);
					// locate the position in the line
					const bool horizontal = isHorizontal(layout->writingMode().blockFlowDirection);
					const PhysicalTwoAxes<Scalar> lineLocalPoint(horizontal ?
						Point(
							geometry::_x = mapViewportIpdToLineLayout(viewport, *layout, geometry::x(p)),
							geometry::_y = geometry::y(p) + geometry::y(baseline.positionInViewport()))
						: Point(
							geometry::_x = geometry::x(p) + geometry::x(baseline.positionInViewport()),
							geometry::_y = mapViewportIpdToLineLayout(viewport, *layout, geometry::y(p))));
					TextHit<> hitInLine(layout->hitTestCharacter(mapPhysicalToAbstract(layout->writingMode(), lineLocalPoint), &outside));
					if(abortNoCharacter && outside)
						return boost::none;

					// snap intervening position to the boundary
					if(hitInLine.characterIndex() != 0 && snapPolicy != kernel::locations::UTF16_CODE_UNIT) {
						using namespace text;
						const kernel::Document& document = viewport.textRenderer().presentation().document();
						const String& s = document.line(line.line);
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
								kernel::DocumentCharacterIterator(document, kernel::Region(line.line, boost::irange<Index>(0, s.length())), kernel::Position(line.line, hitInLine.characterIndex())));
							if(interveningSurrogates || !i.isBoundary(i.base())) {
								--i;
								const TextHit<> leading(TextHit<>::leading(i.base().tell().offsetInLine));
								const TextHit<> trailing(TextHit<>::trailing((++i).base().tell().offsetInLine));
								const Scalar leadingIpd = layout->hitToPoint(leading).ipd();
								const Scalar trailingIpd = layout->hitToPoint(trailing).ipd();
								hitInLine = (abs(ipd - leadingIpd) <= abs(ipd - trailingIpd)) ? leading : trailing;
							}
						} else
							throw UnknownValueException("snapPolicy");
					}
					return hitInLine.isLeadingEdge() ?
						TextHit<kernel::Position>::leading(kernel::Position(line.line, hitInLine.characterIndex()))
						: TextHit<kernel::Position>::trailing(kernel::Position(line.line, hitInLine.characterIndex()));
				}
			}

			/**
			 * Returns the document position nearest from the specified point.
			 * @param pointInView The point in view-coordinates (not viewport-coordinates). This can be outside of the
			 *                    view
			 * @param snapPolicy Which character boundary the returned position snapped to
			 * @return The document position
			 * @throw UnknownValueException @a snapPolicy is invalid
			 * @note This function may change the layout.
			 * @see #modelToView, #viewToModelInBounds, TextLayout#hitTestCharacter
			 */
			TextHit<kernel::Position>&& viewToModel(const TextViewport& viewport,
					const Point& pointInView, kernel::locations::CharacterUnit snapPolicy /* = kernel::locations::GRAPHEME_CLUSTER */) {
				return std::move(*internalViewToModel(viewport, pointInView, false, snapPolicy));
			}

			/**
			 * Returns the document position nearest from the specified point. This method returns
			 * @c boost#none immediately when @a pointInView hovered outside of the text layout (e.g. far left
			 * or right of the line, beyond the last line, ...)
			 * @param pointInView The point in view-coordinates (not viewport-coordinates). This can be outside
			 *                    of the view
			 * @param snapPolicy Which character boundary the returned position snapped to
			 * @return The document position, or @c boost#none if @a pointInView is outside of the layout
			 * @throw UnknownValueException @a snapPolicy is invalid
			 * @note This function may change the layout.
			 * @see #modelToView, #viewToModel, TextLayout#hitTestCharacter
			 */
			boost::optional<TextHit<kernel::Position>> viewToModelInBounds(const TextViewport& viewport,
					const Point& pointInView, kernel::locations::CharacterUnit snapPolicy /* = k::locations::GRAPHEME_CLUSTER */) {
				return internalViewToModel(viewport, pointInView, true, snapPolicy);
			}


			// BaselineIterator ///////////////////////////////////////////////////////////////////////////////////////

			/**
			 * Constructor. Iterator will address the first visible visual line in the given viewport.
			 * @param viewport The text viewport
			 */
			BaselineIterator::BaselineIterator(const TextViewport& viewport/*, bool trackOutOfViewport*/)
					: viewport_(&viewport), tracksOutOfViewport_(false/*trackOutOfViewport*/) {
				initializeWithFirstVisibleLine();
			}

			/**
			 * Constructor.
			 * @param viewport The text viewport
			 * @param line The visual line this iterator addresses
			 */
			BaselineIterator::BaselineIterator(const TextViewport& viewport, const VisualLine& line/*, bool trackOutOfViewport*/)
					: viewport_(&viewport), tracksOutOfViewport_(false/*trackOutOfViewport*/) {
				initializeWithFirstVisibleLine();
				internalAdvance(&line, boost::none);
			}

			/**
			 * Constructor.
			 * @param viewport The text viewport
			 * @param position The position gives a visual line
			 */
			BaselineIterator::BaselineIterator(const TextViewport& viewport, const TextHit<kernel::Position>& position/*, bool trackOutOfViewport*/)
					: viewport_(&viewport), tracksOutOfViewport_(false/*trackOutOfViewport*/) {
				initializeWithFirstVisibleLine();
				VisualLine line(position.characterIndex().line, 0);
				if(line.line < this->viewport().firstVisibleLine().line)
					internalAdvance(&line, boost::none);	// should go beyond before-edge
				else if(line.line == this->viewport().firstVisibleLine().line) {
					line.subline = this->viewport().textRenderer().layouts().at(line.line)->lineAt(position.insertionIndex().offsetInLine);
					internalAdvance(&line, boost::none);
				} else {
					internalAdvance(&line, boost::none);
					if(this->line() != boost::none)
						std::advance(*this, this->viewport().textRenderer().layouts().at(line.line)->lineAt(position.insertionIndex().offsetInLine));
				}
			}

			/// @see boost#iterator_facade#advance
			void BaselineIterator::advance(BaselineIterator::difference_type n) {
				return internalAdvance(nullptr, n);
			}

			/// @see boost#iterator_facade#decrement
			void BaselineIterator::decrement() {
				return advance(-1);
			}

			/// @see boost#iterator_facade#dereference
			const BaselineIterator::reference BaselineIterator::dereference() const {
				if(viewport_ == nullptr)
					throw NoSuchElementException();
				return distanceFromViewportBeforeEdge_;
			}

			/// @see boost#iterator_facade#equal
			bool BaselineIterator::equal(const BaselineIterator& other) const {
//				if(viewport_ == nullptr)
//					return other.viewport_ == nullptr
//						|| (other.line() != boost::none && boost::get(other.line()).line == other.viewport_->textRenderer().presentation().document().numberOfLines());
//				if(other.viewport_ == nullptr)
//					return viewport_ == nullptr
//						|| (line() != boost::none && boost::get(line()).line == viewport_->textRenderer().presentation().document().numberOfLines());
				if(viewport_ != other.viewport_)
					throw std::invalid_argument("");
				return line_ == other.line_;
			}

			/// @see boost#iterator_facade#increment
			void BaselineIterator::increment() {
				return advance(+1);
			}

			namespace {
				template<typename Rectangle>
				inline Point&& calculatePositionInViewport(presentation::BlockFlowDirection blockFlowDirection, const Rectangle& bounds, Scalar distanceFromViewportBeforeEdge) {
					switch(blockFlowDirection) {
						case presentation::HORIZONTAL_TB:
							return Point(geometry::_x = static_cast<Scalar>(0), geometry::_y = geometry::top(bounds) + distanceFromViewportBeforeEdge);
							break;
						case presentation::VERTICAL_RL:
							return Point(geometry::_x = geometry::right(bounds) - distanceFromViewportBeforeEdge, geometry::_y = static_cast<Scalar>(0));
							break;
						case presentation::VERTICAL_LR:
							return Point(geometry::_x = geometry::left(bounds) + distanceFromViewportBeforeEdge, geometry::_y = static_cast<Scalar>(0));
							break;
						default:
							ASCENSION_ASSERT_NOT_REACHED();
					}
				}
			}

			/// @internal Moves this iterator to the first visible line in the viewport.
			void BaselineIterator::initializeWithFirstVisibleLine() {
				const VisualLine firstVisibleLine(viewport().firstVisibleLine());
				const TextLayout* const layout = viewport().textRenderer().layouts().at(firstVisibleLine.line);
				assert(layout != nullptr);
				const Scalar baseline = layout->lineMetrics(firstVisibleLine.subline).ascent();
				Point axis;
				const Rectangle bounds(boost::geometry::make_zero<Point>(), geometry::size(viewport().boundsInView()));
				switch(viewport().textRenderer().computedBlockFlowDirection()) {
					case presentation::HORIZONTAL_TB:
						axis = Point(geometry::_x = 0.0f, geometry::_y = geometry::top(bounds) + baseline);
						break;
					case presentation::VERTICAL_RL:
						axis = Point(geometry::_x = geometry::right(bounds) - baseline, geometry::_y = 0.0f);
						break;
					case presentation::VERTICAL_LR:
						axis = Point(geometry::_x = geometry::left(bounds) + baseline, geometry::_y = 0.0f);
						break;
					default:
						ASCENSION_ASSERT_NOT_REACHED();
				}

				// commit
				line_ = firstVisibleLine;
				distanceFromViewportBeforeEdge_ = baseline;
				positionInViewport_ = axis;
			}

			/// @internal Implements constructor and @c #advance method.
			void BaselineIterator::internalAdvance(const VisualLine* to, const boost::optional<difference_type>& delta) {
				bool forward;
				if(to != nullptr) {
					assert(delta == boost::none);
					if(*to == line())
						return;
					forward = *to > line();
				} else {
					assert(delta != boost::none);
					if(delta == 0)
						return;
					forward = delta > 0;
				}
				if(!tracksOutOfViewport()) {
					if(forward && **this == std::numeric_limits<Scalar>::max()) {
//						line_ = VisualLine(destination, 0);
						return;	// already outside of after-edge of the viewport
					} else if(!forward && **this == std::numeric_limits<Scalar>::min()) {
//						line_ = VisualLine(destination, 0);
						return;	// already outside of before-edge of the viewport
					}
				}

				// calculate extent of the viewport (if needed)
				const TextRenderer& renderer = viewport().textRenderer();
				const presentation::BlockFlowDirection blockFlowDirection(renderer.computedBlockFlowDirection());
				const boost::integer_range<Scalar> viewportExtent(viewportContentExtent(viewport()));

				auto newLine(boost::get(line()));
				auto newBaseline = **this;
				difference_type n = 0;
				const TextLayout* layout = /*tracksOutOfViewport() ? &renderer.layouts()[newLine.line] :*/ renderer.layouts().at(newLine.line);
				TextLayout::LineMetricsIterator lineMetrics(layout->lineMetrics(newLine.subline));
				bool negativeVertical = isNegativeVertical(*layout);
				if(forward) {
					while((to != nullptr && newLine < *to) || (delta != boost::none && n < delta)) {
						if(newLine.subline == layout->numberOfLines() - 1 && newLine.line == renderer.presentation().document().numberOfLines() - 1)
							throw std::overflow_error("");	// TODO: Is this suitable?
						newBaseline += !negativeVertical ? lineMetrics.descent() : lineMetrics.ascent();
						newBaseline += lineMetrics.leading();
						if(!tracksOutOfViewport() && newBaseline >= *viewportExtent.end()) {
							newBaseline = std::numeric_limits<decltype(newBaseline)>::max();	// over after-edge of the viewport
							break;
						}

						// move to forward visual line
						if(newLine.subline == layout->numberOfLines() - 1) {
							layout = renderer.layouts().at(++newLine.line);
							lineMetrics = layout->lineMetrics(newLine.subline = 0);
							negativeVertical = isNegativeVertical(*layout);
						} else {
							++newLine.subline;
							++lineMetrics;
						}
						++n;

						newBaseline += !negativeVertical ? lineMetrics.ascent() : lineMetrics.descent();
					}
				} else {	// backward
					while((to != nullptr && newLine > *to) || (delta != boost::none && n > delta)) {
						if(newLine.subline == 0 && newLine.line == 0)
							throw std::underflow_error("");	// TODO: Is this suitable?
						newBaseline -= !negativeVertical ? lineMetrics.ascent() : lineMetrics.descent();
						newBaseline -= lineMetrics.leading();
						if(!tracksOutOfViewport() && newBaseline < *viewportExtent.begin()) {
							newBaseline = std::numeric_limits<decltype(newBaseline)>::min();	// over before-edge of the viewport
							break;
						}

						// move to backward visual line
						if(newLine.subline == 0) {
							layout = renderer.layouts().at(--newLine.line);
							lineMetrics = layout->lineMetrics(newLine.subline = layout->numberOfLines() - 1);
							negativeVertical = isNegativeVertical(*layout);
						} else {
							--newLine.subline;
							--lineMetrics;
						}
						--n;

						newBaseline -= !negativeVertical ? lineMetrics.descent() : lineMetrics.ascent();
					}
				}

				// commit
				positionInViewport_ = calculatePositionInViewport(blockFlowDirection, viewport().boundsInView(), newBaseline);
				std::swap(line_, newLine);
				std::swap(distanceFromViewportBeforeEdge_, newBaseline);
			}

			/// @internal Invalidates the iterator.
			inline void BaselineIterator::invalidate() BOOST_NOEXCEPT {
				geometry::x(positionInViewport_) = geometry::y(positionInViewport_) = 1;
			}

			/// @internal Returns @c true if the iterator is valid.
			inline bool BaselineIterator::isValid() const BOOST_NOEXCEPT {
				return geometry::x(positionInViewport_) != 0 && geometry::y(positionInViewport_) != 0;
			}
#if 0
			void TextViewer::BaselineIterator::move(Index line) {
				if(line >= viewer_.document().numberOfLines())
					throw k::BadPositionException(k::Position(line, 0));
				Scalar newBaseline;
				if(!isValid()) {
					Index firstVisibleLine, firstVisibleSubline;
					viewer_.firstVisibleLine(&firstVisibleLine, nullptr, &firstVisibleSubline);
					const PhysicalFourSides<Scalar> spaces(viewer_.spaceWidths());
					Scalar spaceBefore;
					switch(utils::writingMode(viewer_).blockFlowDirection) {
						case WritingModeBase::HORIZONTAL_TB:
							spaceBefore = spaces.top;
							break;
						case WritingModeBase::VERTICAL_RL:
							spaceBefore = spaces.right;
							break;
						case WritingModeBase::VERTICAL_LR:
							spaceBefore = spaces.left;
							break;
						default:
							ASCENSION_ASSERT_NOT_REACHED();
					}
					if(line == firstVisibleLine) {
						if(firstVisibleSubline == 0)
							newBaseline = textRenderer().layouts()[line].lineMetrics(0).ascent();
						else if(!tracksOutOfViewport())
							newBaseline = numeric_limits<Scalar>::min();
						else {
							const TextLayout& layout = textRenderer().layouts()[line];
							newBaseline = 0;
							for(Index subline = firstVisibleSubline - 1; ; --subline) {
								newBaseline -= layout.lineMetrics(subline).descent();
								if(subline == 0)
									break;
								newBaseline -= layout.lineMetrics(subline).ascent();
							}
						}
					} else if(line > firstVisibleLine) {
						const NativeRectangle clientBounds(viewer_.bounds(false));
						const Scalar viewportExtent = WritingModeBase::isHorizontal(utils::writingMode(viewer_).blockFlowDirection) ?
							(geometry::dy(clientBounds) - spaces.top - spaces.bottom) : (geometry::dx(clientBounds) - spaces.left - spaces.right);
						newBaseline = 0;
						const TextLayout* layout = &viewer_.textRenderer().layouts()[firstVisibleLine];
						for(Index ln = firstVisibleLine, subline = firstVisibleLine; ; ) {
							newBaseline += layout->lineMetrics(subline).ascent();
							if(ln == line && subline == 0)
								break;
							newBaseline += layout->lineMetrics(subline).descent();
							if(!tracksOutOfViewport() && newBaseline >= viewportExtent) {
								newBaseline = numeric_limits<Scalar>::max();
								break;
							}
							if(++subline == layout->numberOfLines()) {
								layout = &viewer_.textRenderer().layouts()[++ln];
								subline = 0;
							}
						}
					} else if(!tracksOutOfViewport())
						newBaseline = numeric_limits<Scalar>::min();
					else {
						const TextLayout* layout = &viewer_.textRenderer().layouts()[firstVisibleLine];
						for(Index ln = firstVisibleLine, subline = firstVisibleSubline; ; --subline) {
							newBaseline -= layout->lineMetrics(subline).descent();
							if(subline == 0 && ln == line)
								break;
							newBaseline -= layout->lineMetrics(subline).ascent();
							if(subline == 0) {
								layout = &viewer_.textRenderer().layouts()[--ln];
								subline = layout->numberOfLines();
							}
						}
					}
				}
			}
#endif


			// TextViewport ///////////////////////////////////////////////////////////////////////////////////////////

			/**
			 * Private constructor.
			 * @param textRenderer
			 */
			TextViewport::TextViewport(TextRenderer& textRenderer
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
				, const FontRenderContext& frc
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
				) : textRenderer_(textRenderer),
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
					fontRenderContext_(frc),
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
					scrollPositions_(0, 0), firstVisibleLine_(0, 0), repairingLayouts_(false) {
				documentAccessibleRegionChangedConnection_ =
					this->textRenderer().presentation().document().accessibleRegionChangedSignal().connect(
						std::bind(&TextViewport::documentAccessibleRegionChanged, this, std::placeholders::_1));
				this->textRenderer().addDefaultFontListener(*this);
				this->textRenderer().layouts().addVisualLinesListener(*this);
				this->textRenderer().addComputedBlockFlowDirectionListener(*this);
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
				updateDefaultLineExtent();
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
			}

			/// Destructor.
			TextViewport::~TextViewport() BOOST_NOEXCEPT {
				textRenderer().removeDefaultFontListener(*this);
				textRenderer().layouts().removeVisualLinesListener(*this);
				textRenderer().removeComputedBlockFlowDirectionListener(*this);
			}

			/**
			 * Registers the viewport listener.
			 * @param listener The listener to be registered
			 * @throw std#invalid_argument @a listener is already registered
			 */
			void TextViewport::addListener(TextViewportListener& listener) {
				listeners_.add(listener);
			}

			/**
			 * @internal
			 * @see #calculateBpdScrollPosition
			 */
			inline void TextViewport::adjustBpdScrollPositions() BOOST_NOEXCEPT {
				auto newScrollPositions(scrollPositions());
				decltype(firstVisibleLine_) newFirstVisibleLine(
					std::min(firstVisibleLine().line, textRenderer().presentation().document().numberOfLines() - 1),
					std::min(firstVisibleLine().subline, textRenderer().layouts().numberOfSublinesOfLine(firstVisibleLine().line) - 1));
				if(newFirstVisibleLine != firstVisibleLine()) {
					newScrollPositions.bpd() = calculateBpdScrollPosition(newFirstVisibleLine);
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
					boost::get(blockFlowScrollOffsetInFirstVisibleLogicalLine_) = 0;
					// TODO: Not implemented.
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
				}

				swap(newFirstVisibleLine, firstVisibleLine_);
				swap(newScrollPositions, scrollPositions_);
			}

			/**
			 * Returns the measure of the 'allocation-rectangle'.
			 * @return The measure of the 'allocation-rectangle' in user units
			 * @see #contentMeasure
			 */
			Scalar TextViewport::allocationMeasure() const BOOST_NOEXCEPT {
				const TextRenderer& renderer = textRenderer();
				const bool horizontal = isHorizontal(renderer.computedBlockFlowDirection());
				const Scalar spaces = horizontal ?
					renderer.spaceWidths().left() + renderer.spaceWidths().right()
					: renderer.spaceWidths().top() + renderer.spaceWidths().bottom();
				const Scalar borders = 0;
				const Scalar paddings = 0;
				return std::max(renderer.layouts().maximumMeasure() + spaces + borders + paddings,
					static_cast<Scalar>(horizontal ? geometry::dx(boundsInView()) : geometry::dy(boundsInView())));
			}

			/**
			 * @internal Calculates the value for @c scrollPositions_.bpd() data member.
			 * @param line The first visible line numbers
			 * @return The scroll position value for @a line
			 * @see #adjustBpdScrollPositions
			 */
			inline TextViewportScrollOffset TextViewport::calculateBpdScrollPosition(const boost::optional<VisualLine>& line) const BOOST_NOEXCEPT {
				const VisualLine ln(boost::get_optional_value_or(line, firstVisibleLine()));
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
				// TODO: Not implemented.
				boost::value_initialized<ScrollOffset> newBpdOffset;
				for(Index line = 0; ; ++line) {
					const TextLayout* const layout = textRenderer().layouts().at(line);
					if(line == ln.line) {
						// TODO: Consider *rate* of scroll.
						newBpdOffset += static_cast<ScrollOffset>((layout != nullptr) ?
							layout->extent(boost::irange<Index>(0, ln.subline)).size() : (defaultLineExtent_ * ln.subline));
						break;
					} else
						newBpdOffset += static_cast<ScrollOffset>((layout != nullptr) ? layout->extent().size() : defaultLineExtent_);
				}
#else
				return textRenderer().layouts().mapLogicalLineToVisualLine(ln.line) + ln.subline;
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
			}

			/// @see ComputedBlockFlowDirectionListener#computedBlockFlowDirectionChanged
			void TextViewport::computedBlockFlowDirectionChanged(presentation::BlockFlowDirection used) {
				fireScrollPropertiesChanged(presentation::AbstractTwoAxes<bool>(presentation::_ipd = true, presentation::_bpd = true));
			}

			/**
			 * Returns the measure of the 'content-rectangle'.
			 * @return The measure of the 'content-rectangle' in user units
			 * @see #allocationMeasure
			 */
			Scalar TextViewport::contentMeasure() const BOOST_NOEXCEPT {
				return std::max(
					textRenderer().layouts().maximumMeasure(),
					static_cast<Scalar>(isHorizontal(textRenderer().computedBlockFlowDirection()) ?
						geometry::dx(boundsInView()) : geometry::dy(boundsInView())));
			}

			/// @see DefaultFontListener#defaultFontChanged
			void TextViewport::defaultFontChanged() BOOST_NOEXCEPT {
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
				updateDefaultLineExtent();
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
				fireScrollPropertiesChanged(presentation::AbstractTwoAxes<bool>(presentation::_ipd = true, presentation::_bpd = false));
			}

			/// @see kernel#AccessibleRegionChangedSignal
			void TextViewport::documentAccessibleRegionChanged(const kernel::Document& document) {
				// TODO: Not implemented.
			}

			/// @internal Invokes @c TextViewportListener#viewportScrollPositionChanged
			inline void TextViewport::fireScrollPositionChanged(
					const presentation::AbstractTwoAxes<TextViewportScrollOffset>& positionsBeforeScroll,
					const VisualLine& firstVisibleLineBeforeScroll) BOOST_NOEXCEPT {
				static const decltype(frozenNotification_.count) minimumCount;
				if(frozenNotification_.count == minimumCount)
					listeners_.notify<const presentation::AbstractTwoAxes<TextViewportScrollOffset>&, const VisualLine&>(
						&TextViewportListener::viewportScrollPositionChanged, positionsBeforeScroll, firstVisibleLineBeforeScroll);
				else if(frozenNotification_.positionBeforeChanged == boost::none) {
					frozenNotification_.positionBeforeChanged = FrozenNotification::Position();
					boost::get(frozenNotification_.positionBeforeChanged).offsets = positionsBeforeScroll;
					boost::get(frozenNotification_.positionBeforeChanged).line = firstVisibleLineBeforeScroll;
				}
			}

			/// @internal Invokes @c TextViewportListener#viewportScrollPropertiesChanged
			inline void TextViewport::fireScrollPropertiesChanged(const presentation::AbstractTwoAxes<bool>& dimensions) BOOST_NOEXCEPT {
				static const decltype(frozenNotification_.count) minimumCount;
				if(frozenNotification_.count == minimumCount)
					listeners_.notify<const presentation::AbstractTwoAxes<bool>&>(
						&TextViewportListener::viewportScrollPropertiesChanged, dimensions);
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
				const bool horizontal = isHorizontal(textRenderer().computedBlockFlowDirection());
				Scalar bpd(horizontal ? geometry::dy(boundsInView()) : geometry::dx(boundsInView()));
				if(bpd <= 0)
					return 0;
//				bpd -= horizontal ? (spaceWidths().top() + spaceWidths().bottom()) : (spaceWidths().left() + spaceWidths().right());

				Index line = firstVisibleLine().line, nlines = 0;
				LineLayoutVector& layouts = const_cast<TextViewport*>(this)->textRenderer().layouts();
				const TextLayout* layout = &layouts.at(line, LineLayoutVector::USE_CALCULATED_LAYOUT);
				for(TextLayout::LineMetricsIterator lm(layout->lineMetrics(firstVisibleLine().subline)); ; ) {
					const Scalar lineExtent = lm.height();
					if(lineExtent >= bpd)
						return nlines + bpd / lineExtent;
					bpd -= lineExtent;
					++nlines;
					if(lm.line() == layout->numberOfLines() - 1) {
						if(line == textRenderer().presentation().document().numberOfLines() - 1)
							return static_cast<float>(nlines);
						layout = &layouts[++line];
						lm = layout->lineMetrics(0);
					} else
						++lm;
				}
			}

			/**
			 * Removes the viewport listener.
			 * @param listener The listener to be removed
			 * @throw std#invalid_argument @a listener is not registered
			 */
			void TextViewport::removeListener(TextViewportListener& listener) {
				listeners_.remove(listener);
			}

			void TextViewport::repairUncalculatedLayouts() {
				if(!repairingLayouts_) {
					ascension::detail::ValueSaver<bool> temp(repairingLayouts_);
					repairingLayouts_ = true;

					const Scalar extent = isHorizontal(textRenderer().computedBlockFlowDirection()) ? geometry::dy(boundsInView()) : geometry::dx(boundsInView());
					LineLayoutVector& layouts = textRenderer().layouts();
					VisualLine line(firstVisibleLine());
					const TextLayout* layout = &layouts.at(line.line, LineLayoutVector::USE_CALCULATED_LAYOUT);
					Scalar bpd = (line.subline > 0) ? -static_cast<Scalar>(layout->extent(boost::irange(static_cast<Index>(0), line.subline)).size()) : static_cast<Scalar>(0);

					for(const Index nlines = layouts.document().numberOfLines(); line.line < nlines && bpd < extent; layout = &layouts.at(++line.line, LineLayoutVector::USE_CALCULATED_LAYOUT)) {
						const auto lineExtent(layout->extent());
//						bpd += lineExtent.size();
						bpd += *lineExtent.begin() - *lineExtent.end();
					}
				}
			}

			namespace {
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
				void locateVisualLine(const TextViewport& viewport, TextViewport::ScrollOffset bpdFrom, const VisualLine& lineFrom,
						TextViewport::SignedScrollOffset bpd, bool dontModifyLayout, Scalar defaultLineExtent, VisualLine& line, TextViewport::ScrollOffset& offsetInVisualLine) {
					if(bpd == 0)
						return;
					const LineLayoutVector& layouts = viewport.textRenderer().layouts();
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
							if(line.line == viewport.textRenderer().presentation().document().numberOfLines()) {	// reached the last line
								--line.line;
								if(layout == nullptr)
									layout = !dontModifyLayout ? &layouts.at(line.line) : layouts.atIfCached(line.line);
								line.subline = (layout != nullptr) ? layout->numberOfLines() - 1 : 0;
								const Scalar lastLineExtent = (layout != nullptr) ? layout->extent(boost::irange(line.subline, line.subline + 1)).size() : defaultLineExtent;
								const Scalar pageSize = isHorizontal(viewport.textRenderer().computedBlockFlowDirection()) ? geometry::dy(viewport.boundsInView()) : geometry::dx(viewport.boundsInView());
								offsetInVisualLine = (lastLineExtent > pageSize) ? lastLineExtent - pageSize : 0;
								return;
							}
							if((layout = !dontModifyLayout ? &layouts.at(line.line) : layouts.atIfCached(line.line)) != nullptr) {
								const auto locatedLine(layout->locateLine(bpdToEat, boost::none));
								if(locatedLine.second == boost::none) {	// found in this layout
									offsetInVisualLine = bpdToEat - layout->extent(boost::irange<Index>(0, line.subline = locatedLine.first)).size();
									return;
								}
								assert(boost::get(locatedLine.second) == Direction::FORWARD);
								bpdToEat -= layout->extent().size();
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
							const Scalar logicalLineExtent = (layout != nullptr) ? layout->extent().size() : defaultLineExtent;
							if(logicalLineExtent > bpdToEat) {	// found in this logical line
								if(layout != nullptr) {
									const auto locatedLine(layout->locateLine(logicalLineExtent - bpdToEat, boost::none));
									assert(locatedLine.second == boost::none);
									offsetInVisualLine = logicalLineExtent - bpdToEat - layout->extent(boost::irange<Index>(0, line.subline = locatedLine.first)).size();
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
				std::tuple<VisualLine, TextViewportScrollOffset>&& locateVisualLine(const TextViewport& viewport,
						const boost::optional<TextViewportScrollOffset>& to, const boost::optional<VisualLine>& toLine,
						TextViewportScrollOffset from, const VisualLine& lineFrom) {
					assert((to != boost::none && toLine == boost::none) || (to == boost::none && toLine != boost::none));

					TextViewportScrollOffset bpd = from;
					VisualLine line(lineFrom);
					const TextLayout* layout = viewport.textRenderer().layouts().at(line.line);
					while((to != boost::none && boost::get(to) > bpd) || (toLine != boost::none && boost::get(toLine) > line)) {
						if(layout != nullptr) {
							if(line.subline < layout->numberOfLines() - 1) {
								++line.subline;
								++bpd;
								continue;
							}
						}
						if(line.line == viewport.textRenderer().presentation().document().numberOfLines() - 1)
							break;
						layout = viewport.textRenderer().layouts().at(++line.line);
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
						layout = viewport.textRenderer().layouts().at(--line.line);
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
			void TextViewport::scroll(const presentation::AbstractTwoAxes<TextViewportSignedScrollOffset>& offsets) {
				if(isScrollLocked())
					return;

				auto newPositions(scrollPositions());

				// inline dimension
				if(offsets.ipd() < 0)
					newPositions.ipd() = (static_cast<TextViewportScrollOffset>(-offsets.ipd()) < scrollPositions().ipd()) ? (scrollPositions().ipd() + offsets.ipd()) : 0;
				else if(offsets.ipd() > 0) {
					const bool vertical = presentation::isVertical(textRenderer().computedBlockFlowDirection());
					const Scalar maximumIpd = !vertical ? geometry::dx(boundsInView()) : geometry::dy(boundsInView());
					newPositions.ipd() = std::min(scrollPositions().ipd() + offsets.ipd(), static_cast<TextViewportScrollOffset>(contentMeasure() - maximumIpd));
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
					newPositions.bpd() += textRenderer().layouts().offsetVisualLine(newFirstVisibleLine, offsets.bpd(), LineLayoutVector::USE_CALCULATED_LAYOUT);
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
				swap(scrollPositions_, newPositions);
				swap(firstVisibleLine_, newFirstVisibleLine);
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
				swap(blockFlowScrollOffsetInFirstVisibleVisualLine_, newBlockFlowScrollOffsetInFirstVisibleVisualLine);
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD

				// notify
				if(firstVisibleLine_ != newFirstVisibleLine)
					fireScrollPositionChanged(newPositions, newFirstVisibleLine);
			}

			/**
			 * Scrolls the viewport by the specified offsets in physical dimensions.
			 * This method does nothing if scroll is locked.
			 * @param offsets The offsets to scroll in physical dimensions in user units
			 */
			void TextViewport::scroll(const PhysicalTwoAxes<TextViewportSignedScrollOffset>& offsets) {
				return scroll(mapPhysicalToAbstract(textRenderer().presentation().computeWritingMode(&textRenderer()), offsets));
			}

			/**
			 * Scrolls the viewport by the specified number of pages in block flow direction.
			 * This method does nothing if scroll is locked.
			 * @param pages The number of pages to scroll in block flow direction
			 */
			void TextViewport::scrollBlockFlowPage(TextViewportSignedScrollOffset pages) {
				if(isScrollLocked())
					return;

				const boost::integer_range<TextViewportScrollOffset> rangeBeforeScroll(scrollableRange<presentation::BlockFlowDirection>(*this));
				if(pages > 0) {
					const TextViewportNotificationLocker notificationLockGuard(this);
					for(; pages > 0 && scrollPositions().bpd() < rangeBeforeScroll.back(); --pages) {
						const presentation::AbstractTwoAxes<TextViewportSignedScrollOffset> delta(presentation::_bpd = pageSize<presentation::BlockFlowDirection>(*this), presentation::_ipd = 0);
						scroll(delta);
					}
				} else if(pages < 0) {
					auto newPositions(scrollPositions());
					decltype(firstVisibleLine_) newFirstVisibleLine;
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
					decltype(blockFlowScrollOffsetInFirstVisibleVisualLine_) newBlockFlowScrollOffsetInFirstVisibleVisualLine;
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
					static_assert(std::is_integral<TextViewportScrollOffset>::value, "");
					{
						const TextViewportNotificationLocker notificationLockGuard(this);	// the following code can change the layouts
						LineLayoutVector& layouts = textRenderer().layouts();
						const Scalar bpd = isHorizontal(textRenderer().computedBlockFlowDirection()) ? geometry::dy(boundsInView()) : geometry::dx(boundsInView());
						Index line = firstVisibleLine().line;
						const TextLayout* layout = &layouts.at(line, LineLayoutVector::USE_CALCULATED_LAYOUT);
						TextLayout::LineMetricsIterator lineMetrics(layout->lineMetrics(firstVisibleLine().subline));
						Scalar bpdInPage = 0;
						while(true) {
							if(lineMetrics.line() > 0) {
								if(bpdInPage += (--lineMetrics).height() > bpd)
									++lineMetrics;
							} else if(line > 0) {
								layout = &layouts.at(--line, LineLayoutVector::USE_CALCULATED_LAYOUT);
								if(bpdInPage += (lineMetrics = layout->lineMetrics(layout->numberOfLines() - 1)).height() > bpd) {
									layout = &layouts.at(++line, LineLayoutVector::USE_CALCULATED_LAYOUT);
									lineMetrics = layout->lineMetrics(0);
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
					swap(scrollPositions_, newPositions);
					swap(firstVisibleLine_, newFirstVisibleLine);
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
					swap(blockFlowScrollOffsetInFirstVisibleVisualLine_, newBlockFlowScrollOffsetInFirstVisibleVisualLine);
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD

					// notify
					if(newFirstVisibleLine != firstVisibleLine_)
						fireScrollPositionChanged(newPositions, newFirstVisibleLine);
				}
			}

			/**
			 * Scrolls the viewport to the specified position in abstract dimensions.
			 * This method does nothing if scroll is locked.
			 * @param positions The destination of scroll in abstract dimensions in user units
			 */
			void TextViewport::scrollTo(const presentation::AbstractTwoAxes<boost::optional<TextViewportScrollOffset>>& positions) {
				if(isScrollLocked())
					return;

				decltype(scrollPositions_) newPositions(
					presentation::_ipd = boost::get_optional_value_or(positions.ipd(), scrollPositions().ipd()),
					presentation::_bpd = boost::get_optional_value_or(positions.bpd(), scrollPositions().bpd()));

				// inline dimension
				if(positions.ipd() != boost::none) {
					const auto range(scrollableRange<presentation::ReadingDirection>(*this));
					newPositions.ipd() = std::min(std::max(newPositions.ipd(), *range.begin()), *range.end());
				}

				// block dimension
				decltype(firstVisibleLine_) newFirstVisibleLine;
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
				decltype(blockFlowScrollOffsetInFirstVisibleVisualLine_) newBlockFlowScrollOffsetInFirstVisibleVisualLine;
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
				if(positions.bpd() != boost::none) {
//					TextViewportNotificationLocker notificationLockGuard(this);	// this code can't change the layouts, unlike #scroll
					const auto range(scrollableRange<presentation::BlockFlowDirection>(*this));
					newPositions.bpd() = std::min(std::max(newPositions.bpd(), *range.begin()), *range.end());

					// locate the nearest visual line
					const Index numberOfLogicalLines = textRenderer().presentation().document().numberOfLines();
					TextViewportScrollOffset bpd;
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
						if(newPositions.bpd() - scrollPositions().bpd() < *range.end() - newPositions.bpd()) {
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
							bpd = scrollPositions().bpd() - blockFlowScrollOffsetInFirstVisibleVisualLine();
#else
							bpd = scrollPositions().bpd();
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
							line = firstVisibleLine();
						} else {
							if(const TextLayout* const lastLine = textRenderer().layouts().at(line.line = numberOfLogicalLines - 1)) {
								line.subline = lastLine->numberOfLines() - 1;
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
								bpd = *range.end() - lastLine->extent(boost::irange(line.subline, line.subline + 1)).size();
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
							} else {
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
								bpd = *range.end() - defaultLineExtent_;
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
								line.subline = 0;
							}
#ifndef ASCENSION_PIXELFUL_SCROLL_IN_BPD
							bpd = textRenderer().layouts().numberOfVisualLines() - 1;
#endif	// !ASCENSION_PIXELFUL_SCROLL_IN_BPD
						}
					}

#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
					locateVisualLine(*this,
						bpd, line, boost::get(positions.bpd()) - bpd, true, defaultLineExtent_,
						newFirstVisibleLine, newBlockFlowScrollOffsetInFirstVisibleVisualLine);
#else
					std::tie(line, newPositions.bpd()) = locateVisualLine(*this, newPositions.bpd(), boost::none, bpd, line);
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
				} else {
					newFirstVisibleLine = firstVisibleLine_;
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
					newBlockFlowScrollOffsetInFirstVisibleVisualLine = blockFlowScrollOffsetInFirstVisibleVisualLine_;
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD
				}

				// commit
				swap(scrollPositions_, newPositions);
				swap(firstVisibleLine_, newFirstVisibleLine);
#ifdef ASCENSION_PIXELFUL_SCROLL_IN_BPD
				swap(blockFlowScrollOffsetInFirstVisibleVisualLine_, newBlockFlowScrollOffsetInFirstVisibleVisualLine);
#endif	// ASCENSION_PIXELFUL_SCROLL_IN_BPD

				// notify
				if(firstVisibleLine_ != newFirstVisibleLine)
					fireScrollPositionChanged(newPositions, newFirstVisibleLine);
			}

			/**
			 * Scrolls the viewport to the specified position.
			 * @param positions
			 */
			void TextViewport::scrollTo(const PhysicalTwoAxes<boost::optional<TextViewportScrollOffset>>& positions) {
				return scrollTo(convertPhysicalScrollPositionsToAbstract(*this, positions));
			}

			void TextViewport::scrollTo(const VisualLine& line, TextViewportScrollOffset ipd) {
				// TODO: not implemented.
			}

			/**
			 * Resets the size of the viewport.
			 * @param bounds The new bounds to set, in viewer-local coordinates in pixels
			 * @see #boundsInView
			 */
			void TextViewport::setBoundsInView(const graphics::Rectangle& bounds) {
				const graphics::Rectangle oldBounds(boundsInView());
				// TODO: not implemented.
				listeners_.notify<const graphics::Rectangle&>(&TextViewportListener::viewportBoundsInViewChanged, oldBounds);
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
						listeners_.notify<const presentation::AbstractTwoAxes<bool>&>(&TextViewportListener::viewportScrollPropertiesChanged, frozenNotification_.dimensionsPropertiesChanged);
						frozenNotification_.dimensionsPropertiesChanged = presentation::AbstractTwoAxes<bool>(false, false);
					}
					if(frozenNotification_.positionBeforeChanged != boost::none) {
						listeners_.notify<const presentation::AbstractTwoAxes<TextViewportScrollOffset>&, const VisualLine&>(
							&TextViewportListener::viewportScrollPositionChanged,
							boost::get(frozenNotification_.positionBeforeChanged).offsets, boost::get(frozenNotification_.positionBeforeChanged).line);
						frozenNotification_.positionBeforeChanged = boost::none;
					}
					if(frozenNotification_.boundsBeforeChanged != boost::none) {
						listeners_.notify<const graphics::Rectangle&>(&TextViewportListener::viewportBoundsInViewChanged, boost::get(frozenNotification_.boundsBeforeChanged));
						frozenNotification_.boundsBeforeChanged = boost::none;
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

			/// @see VisualLinesListener#visualLinesDeleted
			void TextViewport::visualLinesDeleted(const boost::integer_range<Index>& lines, Index sublines, bool longestLineChanged) BOOST_NOEXCEPT {
				// see also TextViewer.visualLinesDeleted

				if(*lines.end() < firstVisibleLine_.line) {	// deleted logical lines before visible area
					firstVisibleLine_.line -= lines.size();
					scrollPositions_.bpd() -= sublines;
				} else if(includes(lines, firstVisibleLine_.line)) {	// deleted logical lines contain the first visible line
					firstVisibleLine_.subline = 0;
					adjustBpdScrollPositions();
				}
				fireScrollPropertiesChanged(presentation::AbstractTwoAxes<bool>(presentation::_ipd = longestLineChanged, presentation::_bpd = true));
			}

			/// @see VisualLinesListener#visualLinesInserted
			void TextViewport::visualLinesInserted(const boost::integer_range<Index>& lines) BOOST_NOEXCEPT {
				// see also TextViewer.visualLinesInserted

				if(*lines.end() < firstVisibleLine_.line) {	// inserted before visible area
					firstVisibleLine_.line += lines.size();
					scrollPositions_.bpd() += lines.size();
				} else if(*lines.begin() == firstVisibleLine_.line && firstVisibleLine_.subline > 0) {	// inserted around the first visible line
					firstVisibleLine_.line += lines.size();
					adjustBpdScrollPositions();
				}
				fireScrollPropertiesChanged(presentation::AbstractTwoAxes<bool>(presentation::_ipd = true/*longestLineChanged*/, presentation::_bpd = true));
				repairUncalculatedLayouts();
			}

			/// @see VisualLinesListener#visualLinesModified
			void TextViewport::visualLinesModified(const boost::integer_range<Index>& lines,
					SignedIndex sublinesDifference, bool documentChanged, bool longestLineChanged) BOOST_NOEXCEPT {
				// see also TextViewer.visualLinesModified

				if(sublinesDifference != 0)	 {
					if(*lines.end() < firstVisibleLine_.line)	// changed before visible area
						scrollPositions_.bpd() += sublinesDifference;
					else if(includes(lines, firstVisibleLine_.line) && firstVisibleLine_.subline > 0) {	// changed lines contain the first visible line
//						firstVisibleLine_.subline = 0;
						adjustBpdScrollPositions();
					}
				}
				fireScrollPropertiesChanged(presentation::AbstractTwoAxes<bool>(presentation::_ipd = longestLineChanged, presentation::_bpd = sublinesDifference != 0));
				repairUncalculatedLayouts();
			}

			namespace detail {
				/// @internal
				std::shared_ptr<TextViewport> createTextViewport(TextRenderer& textRenderer) {
//					return std::make_shared<TextViewport>(textRenderer);
					return std::shared_ptr<TextViewport>(new TextViewport(textRenderer));
				}
			}
		}
	}
}
