/**
 * @file virtual-box.cpp
 * @author exeal
 * @date 2003-2006 was EditView.cpp and EditViewWindowMessages.cpp
 * @date 2006-2013 was viewer.cpp
 * @date 2013-04-29 separated from viewer.cpp
 */

#include <ascension/viewer/viewer.hpp>
#include <ascension/viewer/virtual-box.hpp>

using namespace ascension;
using namespace ascension::viewers;
using namespace ascension::graphics;
using namespace ascension::graphics::font;
using namespace std;
namespace k = ascension::kernel;


// VirtualBox /////////////////////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param viewer The viewer
 * @param region The region consists the rectangle
 */
VirtualBox::VirtualBox(const TextViewer& viewer, const k::Region& region) BOOST_NOEXCEPT : viewer_(viewer), lines_(VisualLine(), VisualLine()), ipds_(0, 0) {
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
 * @return The character range in @a line.line, or @c boost#none if @a line is out side of the box.
 *         The range can be empty
 * @see #includes, viewers#selectedRangeOnLine, viewers#selectedRangeOnVisualLine
 */
boost::optional<boost::integer_range<Index>> VirtualBox::characterRangeInVisualLine(const VisualLine& line) const BOOST_NOEXCEPT {
	if(line < *lines_.begin() || line > *lines_.end())
		return boost::none;	// out of the region

	const TextLayout& layout = viewer_.textRenderer().layouts().at(line.line);
	const Scalar baseline = layout.lineMetrics(line.subline).baselineOffset();
	Scalar first = *ipds_.begin(), second = *ipds_.end();
	const Scalar lineStartOffset = viewer_.textRenderer().lineStartEdge(VisualLine(line.line, 0));
	first -= lineStartOffset;
	first = mapTextRendererInlineProgressionDimensionToLineLayout(layout.writingMode(), first);
	second -= lineStartOffset;
	second = mapTextRendererInlineProgressionDimensionToLineLayout(layout.writingMode(), second);

	const boost::integer_range<Index> result(ordered(boost::irange(
		layout.hitTestCharacter(presentation::AbstractTwoAxes<Scalar>(
			presentation::_ipd = min(first, second), presentation::_bpd = baseline)).insertionIndex(),		
		layout.hitTestCharacter(presentation::AbstractTwoAxes<Scalar>(
			presentation::_ipd = max(first, second), presentation::_bpd = baseline)).insertionIndex())));
	assert(layout.lineAt(*result.begin()) == line.subline);
	assert(result.empty() || layout.lineAt(*result.end()) == line.subline);
	return result;
}

/**
 * Returns if the specified point is on the virtual box.
 * @param p The point in viewer-local coordinates (not viewport nor @c TextRenderer coordinates)
 * @return @c true If the point is on the virtual box
 * @see #characterRangeInVisualLine
 */
bool VirtualBox::includes(const graphics::Point& p) const BOOST_NOEXCEPT {
	// TODO: This code can't handle vertical writing-mode.
//	assert(viewer_.isWindow());
	if(viewer_.hitTest(p) != TextViewer::TEXT_AREA_CONTENT_RECTANGLE)
		return false;	// ignore if not in text area

	const shared_ptr<const TextViewport> viewport(viewer_.textRenderer().viewport());

	// compute inline-progression-dimension in the TextRenderer for 'p'
	Scalar ipd = font::inlineProgressionScrollOffsetInUserUnits(*viewport);
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
	geometry::translate(pointInViewport, Dimension(
		geometry::_dx = geometry::left(viewport->boundsInView()), geometry::_dy = geometry::top(viewport->boundsInView())));
	const VisualLine line(locateLine(*viewport, pointInViewport));
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
void VirtualBox::update(const k::Region& region) BOOST_NOEXCEPT {
	pair<VisualLine, VisualLine> lines;	// components correspond to 'region'
	pair<Scalar, Scalar> ipds;

	// first
	const TextLayout* layout = &viewer_.textRenderer().layouts().at(lines.first.line = region.first.line);
	ipds.first = layout->hitToPoint(TextHit<>::leading(region.first.offsetInLine)).ipd();
	ipds.first = mapLineLayoutInlineProgressionDimensionToTextRenderer(layout->writingMode(), ipds.first);
	ipds.first += viewer_.textRenderer().lineStartEdge(VisualLine(lines.first.line, 0));
	lines.first.subline = layout->lineAt(region.first.offsetInLine);

	// second
	layout = &viewer_.textRenderer().layouts().at(lines.second.line = region.second.line);
	ipds.second = layout->hitToPoint(TextHit<>::leading(region.second.offsetInLine)).ipd();
	ipds.second = mapLineLayoutInlineProgressionDimensionToTextRenderer(layout->writingMode(), ipds.second);
	ipds.second += viewer_.textRenderer().lineStartEdge(VisualLine(lines.second.line, 0));
	lines.second.subline = layout->lineAt(region.second.offsetInLine);

	// commit
	lines_ = ordered(boost::irange(lines.first, lines.second));
	ipds_ = ordered(boost::irange(ipds.first, ipds.second));
}
