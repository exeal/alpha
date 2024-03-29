/**
 * @file virtual-box.cpp
 * @author exeal
 * @date 2003-2006 was EditView.cpp and EditViewWindowMessages.cpp
 * @date 2006-2013 was viewer.cpp
 * @date 2013-04-29 separated from viewer.cpp
 * @date 2013-2016
 */

#include <ascension/corelib/numeric-range-algorithm/encompasses.hpp>
#include <ascension/corelib/numeric-range-algorithm/order.hpp>
#include <ascension/graphics/font/line-layout-vector.hpp>
#include <ascension/graphics/font/text-layout.hpp>
#include <ascension/graphics/font/text-viewport.hpp>
#include <ascension/graphics/geometry/point-xy.hpp>
#include <ascension/graphics/geometry/rectangle-sides.hpp>
#include <ascension/viewer/text-area.hpp>
#include <ascension/viewer/text-viewer.hpp>
#include <ascension/viewer/text-viewer-model-conversion.hpp>
#include <ascension/viewer/virtual-box.hpp>

namespace ascension {
	namespace viewer {
		// VirtualBox /////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * Constructor.
		 * @param textArea The text area
		 * @param region The region consists the rectangle
		 */
		VirtualBox::VirtualBox(const TextArea& textArea, const kernel::Region& region) BOOST_NOEXCEPT
				: textArea_(textArea), lines_(graphics::font::VisualLine(), graphics::font::VisualLine()), ipds_(0, 0) {
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
			if(!encompasses(lines_, line))
				return boost::none;	// out of the region

			const auto renderer(textArea_.textRenderer());
			const graphics::font::TextLayout* layout = renderer->layouts().at(line.line);
			std::unique_ptr<const graphics::font::TextLayout> isolatedLayout;
			if(layout == nullptr)
				layout = (isolatedLayout = renderer->layouts().createIsolatedLayout(line.line)).get();
			const presentation::WritingMode writingMode(graphics::font::writingMode(*layout));
			const graphics::Scalar baseline = graphics::font::TextLayout::LineMetricsIterator(*layout, line.subline).baselineOffset();
			const graphics::Scalar lineStartOffset = renderer->lineStartEdge(graphics::font::VisualLine(line.line, 0));
			auto ipds(ipds_);
			ipds.advance_begin(-lineStartOffset);
			ipds.advance_end(-lineStartOffset);
			ipds = boost::irange(
				mapTextRendererInlineProgressionDimensionToLineLayout(writingMode, *boost::const_begin(ipds)),
				mapTextRendererInlineProgressionDimensionToLineLayout(writingMode, *boost::const_end(ipds)));

			const auto result(
				boost::irange(
					layout->hitTestCharacter(presentation::FlowRelativeTwoAxes<graphics::Scalar>(
						presentation::_ipd = *boost::const_begin(ipds), presentation::_bpd = baseline)).insertionIndex(),		
					layout->hitTestCharacter(presentation::FlowRelativeTwoAxes<graphics::Scalar>(
						presentation::_ipd = *boost::const_end(ipds), presentation::_bpd = baseline)).insertionIndex())
				| adaptors::ordered());
			assert(layout->lineAt(graphics::font::TextHit<>::afterOffset(*boost::const_begin(result))) == line.subline);
			assert(boost::empty(result) || layout->lineAt(graphics::font::TextHit<>::beforeOffset(*boost::const_end(result))) == line.subline);
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
			if(textArea_.textViewer().hitTest(p) != &textArea_)
				return false;	// ignore if not in text area

			// compute inline-progression-dimension in the TextRenderer for 'p'
			graphics::Scalar ipd = graphics::font::inlineProgressionOffsetInViewerGeometry(*textArea_.viewport());
			switch(textArea_.textRenderer()->lineRelativeAlignment()) {
				case TextRenderer::LEFT:
				default:
					ipd = geometry::x(p) - geometry::left(textArea_.contentRectangle());
					break;
				case TextRenderer::RIGHT:
					ipd = geometry::x(p) - geometry::right(textArea_.contentRectangle());
					break;
				case TextRenderer::HORIZONTAL_CENTER:
					ipd = geometry::x(p) - (geometry::left(textArea_.contentRectangle())) + geometry::right(textArea_.contentRectangle()) / 2;
					break;
				case TextRenderer::TOP:
					ipd = geometry::y(p) - geometry::top(textArea_.contentRectangle());
					break;
				case TextRenderer::BOTTOM:
					ipd = geometry::y(p) - geometry::bottom(textArea_.contentRectangle());
					break;
				case TextRenderer::VERTICAL_CENTER:
					ipd = geometry::y(p) - (geometry::top(textArea_.contentRectangle())) + geometry::bottom(textArea_.contentRectangle()) / 2;
					break;
//				default:
//					ASCENSION_ASSERT_NOT_REACHED();
			}
			if(!ascension::includes(ipds_, ipd))
				return false;

			const graphics::font::VisualLine line(locateLine(textArea_.textViewer(), p));
			return line >= *boost::const_begin(lines_) && line <= *boost::const_end(lines_);	// 'lines_' is inclusive
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
			const auto renderer(textArea_.textRenderer());

			// first
			const graphics::font::TextLayout* layout = renderer->layouts().at(std::get<0>(lines).line = kernel::line(*boost::const_begin(region)));
			const presentation::WritingMode writingMode(graphics::font::writingMode(*layout));
			std::get<0>(ipds) = layout->hitToPoint(graphics::font::TextHit<>::leading(kernel::offsetInLine(*boost::const_begin(region)))).ipd();
			std::get<0>(ipds) = mapLineLayoutInlineProgressionDimensionToTextRenderer(writingMode, std::get<0>(ipds));
			std::get<0>(ipds) += renderer->lineStartEdge(graphics::font::VisualLine(std::get<0>(lines).line, 0));
			std::get<0>(lines).subline = layout->lineAt(graphics::font::TextHit<>::afterOffset(kernel::offsetInLine(*boost::const_begin(region))));

			// second
			if(!boost::empty(region)) {
				layout = renderer->layouts().at(std::get<1>(lines).line = kernel::line(*boost::const_end(region)));
				const auto e(graphics::font::TextHit<>::beforeOffset(kernel::offsetInLine(*boost::const_end(region))));
				std::get<1>(ipds) = layout->hitToPoint(e).ipd();
				std::get<1>(ipds) = mapLineLayoutInlineProgressionDimensionToTextRenderer(writingMode, std::get<1>(ipds));
				std::get<1>(ipds) += renderer->lineStartEdge(graphics::font::VisualLine(std::get<1>(lines).line, 0));
				std::get<1>(lines).subline = layout->lineAt(e);
			} else {
				std::get<1>(ipds) = std::get<0>(ipds);
				std::get<1>(lines) = std::get<0>(lines);
			}

			// commit
			lines_ = boost::irange(std::get<0>(lines), std::get<1>(lines)) | adaptors::ordered();
			ipds_ = nrange(std::get<0>(ipds), std::get<1>(ipds)) | adaptors::ordered();
		}
	}
}
