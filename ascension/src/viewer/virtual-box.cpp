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
VirtualBox::VirtualBox(const TextViewer& viewer, const k::Region& region) /*throw()*/ : viewer_(viewer) {
	update(region);
}

/**
 * Returns the character range in specified visual line overlaps with the box.
 * @param line The line
 * @param[out] range The character range in @a line.line
 * @return @c true if the box and the visual line overlap
 * @see #includes
 */
bool VirtualBox::characterRangeInVisualLine(const VisualLine& line, boost::integer_range<Index>& range) const /*throw()*/ {
//	assert(viewer_.isWindow());
	const Point& top = beginning();
	const Point& bottom = end();
	if(line < top.line || line > bottom.line)	// out of the region
		return false;
	else {
		const TextRenderer& renderer = viewer_.textRenderer();
		const TextLayout& layout = renderer.layouts().at(line.line);
		const Scalar bpd = layout.lineMetrics(line.subline).baselineOffset();
		range = boost::irange(
			layout.hitTestCharacter(AbstractTwoAxes<Scalar>(
				viewer_.mapViewportIpdToLineLayout(line.line, points_[0].ipd), bpd)).characterIndex(),
			layout.hitTestCharacter(AbstractTwoAxes<Scalar>(
				viewer_.mapViewportIpdToLineLayout(line.line, points_[1].ipd), bpd)).characterIndex());
		return !range.empty();
	}
}

/**
 * Returns if the specified point is on the virtual box.
 * @param p The point in local coordinates
 * @return @c true If the point is on the virtual box
 * @see #characterRangeInVisualLine
 */
bool VirtualBox::includes(const graphics::Point& p) const /*throw()*/ {
	// TODO: This code can't handle vertical writing-mode.
//	assert(viewer_.isWindow());
	if(viewer_.hitTest(p) == TextViewer::TEXT_AREA_CONTENT_RECTANGLE) {	// ignore if not in text area
		// about inline-progression-direction
		const bool horizontal = isHorizontal(viewer_.textRenderer().computedBlockFlowDirection());
		const Scalar ipd = (horizontal ? geometry::x(p) : geometry::y(p)) - viewer_.inlineProgressionOffsetInViewport();	// $friendly-access
		if(ascension::includes(boost::irange(startEdge(), endEdge()), ipd)) {
			// about block-progression-direction
			const shared_ptr<const TextViewport> viewport(viewer_.textRenderer().viewport());
			graphics::Point pointInViewport(p);
			const graphics::Rectangle viewportBoundsInView(viewport->boundsInView());
			geometry::translate(pointInViewport, Dimension(
				geometry::_dx = geometry::left(viewportBoundsInView), geometry::_dy = geometry::top(viewportBoundsInView)));
			const VisualLine line(locateLine(*viewport, pointInViewport));
			return line >= beginning().line && line <= end().line;
		}
	}
	return false;
}

/**
 * Updates the rectangle of the virtual box.
 * @param region The region consists the rectangle
 */
void VirtualBox::update(const k::Region& region) /*throw()*/ {
	array<Point, 2> newPoints;

	// first
	const TextLayout* layout = &viewer_.textRenderer().layouts().at(newPoints[0].line.line = region.first.line);
	newPoints[0].ipd = viewer_.mapLineLayoutIpdToViewport(newPoints[0].line.line, layout->hitToPoint(TextHit<>::leading(region.first.offsetInLine)).ipd());
	newPoints[0].line.subline = layout->lineAt(region.first.offsetInLine);

	// second
	layout = &viewer_.textRenderer().layouts().at(newPoints[1].line.line = region.second.line);
	newPoints[1].ipd = viewer_.mapLineLayoutIpdToViewport(newPoints[1].line.line, layout->hitToPoint(TextHit<>::leading(region.second.offsetInLine)).ipd());
	newPoints[1].line.subline = layout->lineAt(region.second.offsetInLine);

	// commit
	points_[0] = newPoints[0];
	points_[1] = newPoints[1];
}
