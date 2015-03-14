/**
 * @file virtual-box.cpp
 * @author exeal
 * @date 2003-2006 was EditView.cpp and EditViewWindowMessages.cpp
 * @date 2006-2013 was viewer.cpp
 * @date 2013-04-29 separated from viewer.cpp
 * @date 2013-2015
 */

#include <ascension/graphics/font/text-viewport.hpp>
#include <ascension/viewer/text-viewer.hpp>
#include <ascension/viewer/virtual-box.hpp>

namespace ascension {
	namespace viewer {
		// VirtualBox /////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * Constructor.
		 * @param viewer The viewer
		 * @param region The region consists the rectangle
		 */
		VirtualBox::VirtualBox(const TextViewer& viewer, const kernel::Region& region) BOOST_NOEXCEPT
				: viewer_(viewer), lines_(graphics::font::VisualLine(), graphics::font::VisualLine()), ipds_(0, 0) {
			update(region);
		}

		namespace {
			template<typename T>
			inline T mapTextRendererInlineProgressionDimensionToLineLayout(const presentation::WritingMode& writingMode, T ipd) {
				bool negative = writingMode.inlineFlowDirection == presentation::RIGHT_TO_LEFT;
				if(isVertical(writingMode.blockFlowDirection) && presentation::resolveTextOrientation(writingMode) == presentation::SIDEWAYS_LEFT)
					negative = !negative;
				return !negative ? ipd : -ipd;
			}
		}

		/**
		 * Returns the character range in specified visual line overlaps with the box.
		 * @param line The line
		 * @return The character range in @a line.line, or @c boost#none if @a line is out side of the box. The range
		 *         can be empty
		 * @see #includes, viewer#selectedRangeOnLine, viewer#selectedRangeOnVisualLine
		 */
		boost::optional<boost::integer_range<Index>> VirtualBox::characterRangeInVisualLine(const graphics::font::VisualLine& line) const BOOST_NOEXCEPT {
			if(line < *lines_.begin() || line > *lines_.end())
				return boost::none;	// out of the region

			const graphics::font::TextLayout* layout = viewer_.textRenderer().layouts().at(line.line);
			std::unique_ptr<const graphics::font::TextLayout> isolatedLayout;
			if(layout == nullptr)
				layout = (isolatedLayout = viewer_.textRenderer().layouts().createIsolatedLayout(line.line)).get();
			const presentation::WritingMode writingMode(graphics::font::writingMode(*layout));
			const graphics::Scalar baseline = graphics::font::TextLayout::LineMetricsIterator(*layout, line.subline).baselineOffset();
			graphics::Scalar first = *ipds_.begin(), second = *ipds_.end();
			const graphics::Scalar lineStartOffset = viewer_.textRenderer().lineStartEdge(graphics::font::VisualLine(line.line, 0));
			first -= lineStartOffset;
			first = mapTextRendererInlineProgressionDimensionToLineLayout(writingMode, first);
			second -= lineStartOffset;
			second = mapTextRendererInlineProgressionDimensionToLineLayout(writingMode, second);

			const boost::integer_range<Index> result(ordered(boost::irange(
				layout->hitTestCharacter(presentation::FlowRelativeTwoAxes<graphics::Scalar>(
					presentation::_ipd = std::min(first, second), presentation::_bpd = baseline)).insertionIndex(),		
				layout->hitTestCharacter(presentation::FlowRelativeTwoAxes<graphics::Scalar>(
					presentation::_ipd = std::max(first, second), presentation::_bpd = baseline)).insertionIndex())));
			assert(layout->lineAt(*result.begin()) == line.subline);
			assert(result.empty() || layout->lineAt(*result.end()) == line.subline);
			return result;
		}

		/**
		 * Returns if the specified point is on the virtual box.
		 * @param p The point in viewer-local coordinates (not viewport nor @c TextRenderer coordinates)
		 * @return @c true If the point is on the virtual box
		 * @see #characterRangeInVisualLine
		 */
		bool VirtualBox::includes(const graphics::Point& p) const BOOST_NOEXCEPT {
			namespace geometry = graphics::geometry;
			using graphics::font::TextRenderer;
			// TODO: This code can't handle vertical writing-mode.
//			assert(viewer_.isWindow());
			if(viewer_.hitTest(p) != TextViewer::TEXT_AREA_CONTENT_RECTANGLE)
				return false;	// ignore if not in text area

			const std::shared_ptr<const graphics::font::TextViewport> viewport(viewer_.textRenderer().viewport());

			// compute inline-progression-dimension in the TextRenderer for 'p'
			graphics::Scalar ipd = graphics::font::inlineProgressionOffsetInViewerGeometry(*viewport);
			switch(viewer_.textRenderer().lineRelativeAlignment()) {
				case TextRenderer::LEFT:
					ipd = geometry::x(p) - geometry::left(viewport->boundsInView());
					break;
				case TextRenderer::RIGHT:
					ipd = geometry::x(p) - geometry::right(viewport->boundsInView());
					break;
				case TextRenderer::HORIZONTAL_CENTER:
					ipd = geometry::x(p) - (geometry::left(viewport->boundsInView())) + geometry::right(viewport->boundsInView()) / 2;
					break;
				case TextRenderer::TOP:
					ipd = geometry::y(p) - geometry::top(viewport->boundsInView());
					break;
				case TextRenderer::BOTTOM:
					ipd = geometry::y(p) - geometry::bottom(viewport->boundsInView());
					break;
				case TextRenderer::VERTICAL_CENTER:
					ipd = geometry::y(p) - (geometry::top(viewport->boundsInView())) + geometry::bottom(viewport->boundsInView()) / 2;
					break;
				default:
					ASCENSION_ASSERT_NOT_REACHED();
			}
			if(!ascension::includes(ipds_, ipd))
				return false;

			graphics::Point pointInViewport(p);
			geometry::translate(pointInViewport, graphics::Dimension(
				geometry::_dx = geometry::left(viewport->boundsInView()), geometry::_dy = geometry::top(viewport->boundsInView())));
			const graphics::font::VisualLine line(locateLine(*viewport, pointInViewport));
			return line >= *lines_.begin() && line <= *lines_.end();	// 'lines_' is inclusive
		}

		namespace {
			template<typename T>
			inline T mapLineLayoutInlineProgressionDimensionToTextRenderer(const presentation::WritingMode& writingMode, T ipd) {
				return mapTextRendererInlineProgressionDimensionToLineLayout(writingMode, ipd);
			}
		}

		/**
		 * Updates the rectangle of the virtual box.
		 * @param region The region consists the rectangle
		 */
		void VirtualBox::update(const kernel::Region& region) BOOST_NOEXCEPT {
			std::pair<graphics::font::VisualLine, graphics::font::VisualLine> lines;	// components correspond to 'region'
			std::pair<graphics::Scalar, graphics::Scalar> ipds;

			// first
			const graphics::font::TextLayout* layout = viewer_.textRenderer().layouts().at(lines.first.line = region.first.line);
			const presentation::WritingMode writingMode(graphics::font::writingMode(*layout));
			ipds.first = layout->hitToPoint(graphics::font::TextHit<>::leading(region.first.offsetInLine)).ipd();
			ipds.first = mapLineLayoutInlineProgressionDimensionToTextRenderer(writingMode, ipds.first);
			ipds.first += viewer_.textRenderer().lineStartEdge(graphics::font::VisualLine(lines.first.line, 0));
			lines.first.subline = layout->lineAt(region.first.offsetInLine);

			// second
			layout = viewer_.textRenderer().layouts().at(lines.second.line = region.second.line);
			ipds.second = layout->hitToPoint(graphics::font::TextHit<>::leading(region.second.offsetInLine)).ipd();
			ipds.second = mapLineLayoutInlineProgressionDimensionToTextRenderer(writingMode, ipds.second);
			ipds.second += viewer_.textRenderer().lineStartEdge(graphics::font::VisualLine(lines.second.line, 0));
			lines.second.subline = layout->lineAt(region.second.offsetInLine);

			// commit
			lines_ = ordered(boost::irange(lines.first, lines.second));
			ipds_ = ordered(boost::irange(ipds.first, ipds.second));
		}
	}
}
