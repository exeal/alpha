/**
 * @file text-viewport.cpp
 * @author exeal
 * @date 2003-2006 was LineLayout.cpp
 * @date 2006-2012
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

using namespace ascension;
using namespace ascension::graphics;
using namespace ascension::graphics::font;
using namespace ascension::presentation;
using namespace std;
namespace k = ascension::kernel;


// BaselineIterator ///////////////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param viewport The text viewport
 * @param line The line number this iterator addresses
 * @param trackOutOfViewport Set @c true to 
 */
BaselineIterator::BaselineIterator(const TextViewport& viewport, Index line,
		bool trackOutOfViewport) : viewport_(&viewport), tracksOutOfViewport_(trackOutOfViewport) {
	initializeWithFirstVisibleLine();
	advance(line - this->line());
}

/// @see boost#iterator_facade#advance
void BaselineIterator::advance(BaselineIterator::difference_type n) {
	const TextRenderer& renderer = viewport().textRenderer();
	if(n == 0)
		return;
	else if(n > 0 && line() + n > renderer.presentation().document().numberOfLines())
		throw invalid_argument("n");
	else if(n < 0 && static_cast<Index>(-n) - 1 > line())
		throw invalid_argument("n");

	const Index destination = line() + n;
	if(!tracksOutOfViewport()
			&& (distanceFromViewportBeforeEdge_ == numeric_limits<Scalar>::min()
			|| distanceFromViewportBeforeEdge_ == numeric_limits<Scalar>::max())) {
		if((n > 0 && distanceFromViewportBeforeEdge_ == numeric_limits<Scalar>::max())
				|| (n < 0 && distanceFromViewportBeforeEdge_ == numeric_limits<Scalar>::min())) {
			line_ = VisualLine(destination, 0);
			return;
		}
		swap(*this, BaselineIterator(viewport(), destination, tracksOutOfViewport()));
		return;
	}

	const BlockFlowDirection blockFlowDirection(renderer.computedBlockFlowDirection());
	Scalar viewportExtent;
	if(!tracksOutOfViewport() && n > 0)
		viewportExtent = isHorizontal(blockFlowDirection) ?
			(geometry::dy(viewport().boundsInView())) : (geometry::dx(viewport().boundsInView()));

	VisualLine i(line_);
	Scalar newBaseline = distanceFromViewportBeforeEdge_;
	const TextLayout* layout = &renderer.layouts()[line()];
	if(n > 0) {
		newBaseline += layout->lineMetrics(line_.subline).descent();
		for(Index ln = line(), subline = line_.subline; ; ) {
			if(++subline == layout->numberOfLines()) {
				subline = 0;
				if(++ln == renderer.presentation().document().numberOfLines()) {
					newBaseline = numeric_limits<Scalar>::max();
					break;
				}
				layout = &renderer.layouts()[++ln];
			}
			newBaseline += layout->lineMetrics(subline).ascent();
			if(ln == destination && subline == 0)
				break;
			newBaseline += layout->lineMetrics(subline).descent();
			if(!tracksOutOfViewport() && newBaseline >= viewportExtent) {
				newBaseline = numeric_limits<Scalar>::max();
				break;
			}
		}
	} else {	// n < 0
		newBaseline -= layout->lineMetrics(line_.subline).ascent();
		for(Index ln = line(), subline = line_.subline; ; ) {
			if(subline == 0) {
				if(ln-- == 0) {
					subline = 0;
					newBaseline = numeric_limits<Scalar>::min();
					break;
				}
				layout = &renderer.layouts()[ln];
				subline = layout->numberOfLines() - 1;
			} else
				--subline;
			newBaseline -= layout->lineMetrics(subline).descent();
			if(ln == destination && subline == 0)
				break;
			newBaseline -= layout->lineMetrics(subline).ascent();
			if(!tracksOutOfViewport() && newBaseline < 0) {
				newBaseline = numeric_limits<Scalar>::min();
				break;
			}
		}
	}

	Point newAxis(positionInViewport_);
	switch(blockFlowDirection) {
		case HORIZONTAL_TB:
			geometry::y(newAxis) += newBaseline - distanceFromViewportBeforeEdge_;
			break;
		case VERTICAL_RL:
			geometry::x(newAxis) -= newBaseline - distanceFromViewportBeforeEdge_;
			break;
		case VERTICAL_LR:
			geometry::x(newAxis) += newBaseline - distanceFromViewportBeforeEdge_;
			break;
		default:
			ASCENSION_ASSERT_NOT_REACHED();
	}

	// commit
	line_ = VisualLine(destination, 0);
	distanceFromViewportBeforeEdge_ = newBaseline;
	positionInViewport_ = newAxis;
}

/// @see boost#iterator_facade#decrement
void BaselineIterator::decrement() {
	return advance(-1);
}

/// @see boost#iterator_facade#dereference
const BaselineIterator::reference BaselineIterator::dereference() const {
	return distanceFromViewportBeforeEdge_;
}

/// @see boost#iterator_facade#increment
void BaselineIterator::increment() {
	return advance(+1);
}

/// @internal Moves this iterator to the first visible line in the viewport.
void BaselineIterator::initializeWithFirstVisibleLine() {
	const VisualLine firstVisibleLine(
		viewport().firstVisibleLineInLogicalNumber(), viewport().firstVisibleSublineInLogicalLine());
	const Scalar baseline = viewport().textRenderer().layouts().at(firstVisibleLine.line).lineMetrics(firstVisibleLine.subline).ascent();
	Point axis;
	const Rectangle bounds(boost::geometry::make_zero<Point>(), geometry::size(viewport().boundsInView()));
	switch(viewport().textRenderer().computedBlockFlowDirection()) {
		case HORIZONTAL_TB:
			axis = Point(geometry::_x = 0, geometry::_y = geometry::top(bounds) + baseline);
			break;
		case VERTICAL_RL:
			axis = Point(geometry::_x = geometry::right(bounds) - baseline, geometry::_y = 0);
			break;
		case VERTICAL_LR:
			axis = Point(geometry::_x = geometry::left(bounds) + baseline, geometry::_y = 0);
			break;
		default:
			ASCENSION_ASSERT_NOT_REACHED();
	}

	// commit
	line_ = firstVisibleLine;
	distanceFromViewportBeforeEdge_ = baseline;
	positionInViewport_ = axis;
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


// TextViewport ///////////////////////////////////////////////////////////////////////////////////

namespace {
	/**
	 * @internal Converts the distance from the 'before-edge' of the 'allocation-rectangle' into
	 * the logical line and visual subline offset. The results are snapped to the first/last
	 * visible line in the viewport (this includes partially visible line) if the given distance
	 * addresses outside of the viewport.
	 * @param bpd The distance from the 'before-edge' of the 'allocation-rectangle' in pixels
	 * @param[out] snapped @c true if there was not a line at @a bpd. Optional
	 * @return The logical and visual line numbers
	 * @see #BaselineIterator, TextViewer#mapLocalBpdToLine
	 */
	VisualLine mapBpdToLine(const TextViewport& viewport, Scalar bpd, bool* snapped = nullptr) /*throw()*/ {
		const BlockFlowDirection blockFlowDirection = viewport.textRenderer().computedBlockFlowDirection();
		const PhysicalFourSides<Scalar>& physicalSpaces = viewport.textRenderer().spaceWidths();
		Scalar spaceBefore, spaceAfter;
		switch(blockFlowDirection) {
			case HORIZONTAL_TB:
				spaceBefore = physicalSpaces.top();
				spaceAfter = physicalSpaces.bottom();
				break;
			case VERTICAL_RL:
				spaceBefore = physicalSpaces.right();
				spaceAfter = physicalSpaces.left();
				break;
			case VERTICAL_LR:
				spaceBefore = physicalSpaces.left();
				spaceAfter = physicalSpaces.right();
				break;
			default:
				ASCENSION_ASSERT_NOT_REACHED();
		}
		const Scalar borderBefore = 0, borderAfter = 0, paddingBefore = 0, paddingAfter = 0;
		const Scalar before = spaceBefore + borderBefore + paddingBefore;
		const Scalar after = (isHorizontal(blockFlowDirection) ?
			geometry::dy(viewport.boundsInView()) : geometry::dx(viewport.boundsInView())) - spaceAfter - borderAfter - paddingBefore;
	
		VisualLine result(viewport.firstVisibleLineInLogicalNumber(), viewport.firstVisibleSublineInLogicalLine());
		bool outside;	// for 'snapped'
		if(bpd <= before)
			outside = bpd != before;
		else {
			const bool beyondAfter = bpd >= after;
			if(beyondAfter)
				bpd = after;
			Scalar lineBefore = before;
			const TextLayout* layout = &viewport.textRenderer().layouts()[result.line];
			while(result.subline > 0)	// back to the first subline
				lineBefore -= layout->lineMetrics(--result.subline).height();
			while(true) {
				assert(bpd >= lineBefore);
				Scalar lineAfter = lineBefore;
				for(Index sl = 0; sl < layout->numberOfLines(); ++sl)
					lineAfter += layout->lineMetrics(sl).height();
				if(bpd < lineAfter) {
					result.subline = layout->locateLine(bpd - lineBefore, boost::none, outside);	// TODO: Should i use the second argument?
					if(!outside)
						break;	// bpd is this line
					assert(result.subline == layout->numberOfLines() - 1);
				}
				layout = &viewport.textRenderer().layouts()[++result.line];
				lineBefore = lineAfter;
			}
			outside = beyondAfter;
		}
		if(snapped != nullptr)
			*snapped = outside;
		return result;
	}
}

/**
 * Private constructor.
 * @param textRenderer
 */
TextViewport::TextViewport(TextRenderer& textRenderer) : textRenderer_(textRenderer) {
	documentAccessibleRegionChangedConnection_ =
		this->textRenderer().presentation().document().accessibleRegionChangedSignal().connect(
			boost::bind(&TextViewport::documentAccessibleRegionChanged, this, _1));
}

inline void TextViewport::adjustBpdScrollPositions() /*throw()*/ {
	const LineLayoutVector& layouts = textRenderer().layouts();
	firstVisibleLine_.line = min(firstVisibleLine_.line, textRenderer().presentation().document().numberOfLines() - 1);
	firstVisibleLine_.subline = min(layouts.numberOfSublinesOfLine(firstVisibleLine_.line) - 1, firstVisibleLine_.subline);
	scrollOffsets_.bpd() = layouts.mapLogicalLineToVisualLine(firstVisibleLine_.line) + firstVisibleLine_.subline;
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
	return max(renderer.layouts().maximumMeasure() + spaces + borders + paddings,
		static_cast<Scalar>(horizontal ? geometry::dx(boundsInView()) : geometry::dy(boundsInView())));
}

/**
 * Returns the measure of the 'content-rectangle'.
 * @return The measure of the 'content-rectangle' in user units
 * @see #allocationMeasure
 */
Scalar TextViewport::contentMeasure() const BOOST_NOEXCEPT {
	return max(
		textRenderer().layouts().maximumMeasure(),
		static_cast<Scalar>(isHorizontal(textRenderer().computedBlockFlowDirection()) ?
			geometry::dx(boundsInView()) : geometry::dy(boundsInView())));
}

/**
 * Returns the number of the drawable columns in the window.
 * @return The number of columns
 */
float TextViewport::numberOfVisibleCharactersInLine() const /*throw()*/ {
	const bool horizontal = isHorizontal(textRenderer().computedBlockFlowDirection());
	Scalar ipd(horizontal ? geometry::dx(boundsInView()) : geometry::dy(boundsInView()));
	if(ipd == 0)
		return 0;
//	ipd -= horizontal ? (spaceWidths().left() + spaceWidths().right()) : (spaceWidths().top() + spaceWidths().bottom());
	return static_cast<float>(ipd) / textRenderer().defaultFont()->metrics().averageCharacterWidth();
}

/**
 * Returns the number of the drawable visual lines in the viewport.
 * @return The number of visual lines
 */
float TextViewport::numberOfVisibleLines() const /*throw()*/ {
	const bool horizontal = isHorizontal(textRenderer().computedBlockFlowDirection());
	Scalar bpd(horizontal ? geometry::dy(boundsInView()) : geometry::dx(boundsInView()));
	if(bpd <= 0)
		return 0;
//	bpd -= horizontal ? (spaceWidths().top() + spaceWidths().bottom()) : (spaceWidths().left() + spaceWidths().right());

	VisualLine line(firstVisibleLineInLogicalNumber(), firstVisibleSublineInLogicalLine());
	const TextLayout* layout = &textRenderer().layouts().at(line.line);
	float lines = 0;
	while(true) {
		const Scalar lineHeight = layout->lineMetrics(line.subline).height();
		if(lineHeight >= bpd)
			return lines += bpd / lineHeight;
		bpd -= lineHeight;
		++lines;
		if(line.subline == layout->numberOfLines() - 1) {
			if(line.line == textRenderer().presentation().document().numberOfLines() - 1)
				return lines;
			layout = &textRenderer().layouts()[++line.line];
			line.subline = 0;
		} else
			++line.subline;
	}
}

/**
 * 
 * This method does nothing if scroll is locked.
 * @param offsets The offsets to scroll in visual lines in block-progression-dimension and in
 *                characters in inline-progression-dimension
 */
void TextViewport::scroll(const AbstractTwoAxes<TextViewport::SignedScrollOffset>& offsets) {
	if(lockCount_ != 0)
		return;

	// 1. preprocess parameters and update positions
	AbstractTwoAxes<TextViewport::SignedScrollOffset> delta(offsets);
	const VisualLine oldLine = firstVisibleLine_;
	const ScrollOffset oldInlineProgressionOffset = inlineProgressionOffset();
	if(offsets.bpd() != 0) {
		const ScrollOffset maximumBpd = textRenderer().layouts().numberOfVisualLines() - static_cast<ScrollOffset>(numberOfVisibleLines());
		delta.bpd() = max(min(offsets.bpd(),
			static_cast<SignedScrollOffset>(maximumBpd - firstVisibleLineInVisualNumber()) + 1),
				-static_cast<SignedScrollOffset>(firstVisibleLineInVisualNumber()));
		if(delta.bpd() != 0) {
			scrollOffsets_.bpd() += delta.bpd();
			textRenderer().layouts().offsetVisualLine(firstVisibleLine_, delta.bpd());
		}
	}
	if(offsets.ipd() != 0) {
		const ScrollOffset maximumIpd = contentMeasure()
			/ textRenderer().defaultFont()->metrics().averageCharacterWidth()
			- static_cast<ScrollOffset>(numberOfVisibleCharactersInLine());
		delta.ipd() = max(min(offsets.ipd(),
			static_cast<SignedScrollOffset>(maximumIpd - inlineProgressionOffset()) + 1),
			-static_cast<SignedScrollOffset>(inlineProgressionOffset()));
		if(delta.ipd() != 0)
			scrollOffsets_.ipd() += delta.ipd();
	}
	if(delta.bpd() == 0 && delta.ipd() == 0)
		return;
	listeners_.notify<const AbstractTwoAxes<SignedScrollOffset>&, const VisualLine&, ScrollOffset>(
		&TextViewportListener::viewportScrollPositionChanged, delta, oldLine, oldInlineProgressionOffset);
}

/***/
void TextViewport::scroll(const PhysicalTwoAxes<TextViewport::SignedScrollOffset>& offsets) {
	AbstractTwoAxes<SignedScrollOffset> delta;
	switch(textRenderer().computedBlockFlowDirection()) {
		case HORIZONTAL_TB:
			delta.bpd() = offsets.y();
			delta.ipd() = offsets.x();
			break;
		case VERTICAL_RL:
			delta.bpd() = -offsets.x();
			delta.ipd() = offsets.y();
			break;
		case VERTICAL_LR:
			delta.bpd() = +offsets.x();
			delta.ipd() = offsets.y();
			break;
		default:
			ASCENSION_ASSERT_NOT_REACHED();
	}
	return scroll(delta);
}

/**
 * Scrolls the viewport to the specified position.
 * @param positions
 */
void TextViewport::scrollTo(const AbstractTwoAxes<boost::optional<TextViewport::ScrollOffset>>& positions) {
	AbstractTwoAxes<SignedScrollOffset> delta;
	if(positions.bpd() != boost::none) {
		const ScrollOffset maximumBpd =
			textRenderer().layouts().numberOfVisualLines() - static_cast<ScrollOffset>(numberOfVisibleLines()) + 1;
		delta.bpd() = max<ScrollOffset>(min(*positions.bpd(), maximumBpd), 0) - firstVisibleLineInVisualNumber();
	} else
		delta.bpd() = 0;
	if(positions.ipd() != boost::none) {
		const ScrollOffset maximumIpd =
			static_cast<ScrollOffset>(contentMeasure()
				/ textRenderer().defaultFont()->metrics().averageCharacterWidth())
				- static_cast<ScrollOffset>(numberOfVisibleCharactersInLine()) + 1;
		delta.ipd() = max<ScrollOffset>(min(*positions.ipd(), maximumIpd), 0) - inlineProgressionOffset();
	} else
		delta.ipd() = 0;
	if(delta.bpd() != 0 || delta.ipd() != 0)
		scroll(delta);
}

/**
 * Scrolls the viewport to the specified position.
 * @param positions
 */
void TextViewport::scrollTo(const PhysicalTwoAxes<boost::optional<TextViewport::ScrollOffset>>& positions) {
	return scrollTo(convertPhysicalScrollPositionsToAbstract(*this, positions));
}

void TextViewport::scrollTo(const VisualLine& line, TextViewport::ScrollOffset ipd) {
	// TODO: not implemented.
}

/**
 * Resets the size of the viewport.
 * @param bounds The new bounds to set, in viewer-local coordinates in pixels
 */
void TextViewport::setBoundsInView(const graphics::Rectangle& bounds) {
	const graphics::Rectangle oldBounds(boundsInView());
	// TODO: not implemented.
	listeners_.notify<const graphics::Rectangle&>(&TextViewportListener::viewportBoundsInViewChanged, oldBounds);
}

/// @see VisualLinesListener#visualLinesDeleted
void TextViewport::visualLinesDeleted(const boost::integer_range<Index>& lines, Index sublines, bool longestLineChanged) /*throw()*/ {
//	scrolls_.changed = true;
	if(*lines.end() < firstVisibleLine_.line) {	// deleted before visible area
		firstVisibleLine_.line -= lines.size();
		scrollOffsets_.bpd() -= sublines;
//		scrolls_.vertical.maximum -= static_cast<int>(sublines);
//		repaintRuler();
	} else if(lines.front() > firstVisibleLine_.line	// deleted the first visible line and/or after it
			|| (lines.front() == firstVisibleLine_.line && firstVisibleLine_.subline == 0)) {
//		scrolls_.vertical.maximum -= static_cast<int>(sublines);
//		redrawLine(lines.beginning(), true);
	} else {	// deleted lines contain the first visible line
		firstVisibleLine_.line = lines.front();
		adjustBpdScrollPositions();
//		redrawLine(lines.beginning(), true);
	}
//	if(longestLineChanged)
//		scrolls_.resetBars(*this, 'i', false);
	visualLinesListeners_.notify<const boost::integer_range<Index>&, Index, bool>(&VisualLinesListener::visualLinesDeleted, lines, sublines, longestLineChanged);
}

/// @see VisualLinesListener#visualLinesInserted
void TextViewport::visualLinesInserted(const boost::integer_range<Index>& lines) BOOST_NOEXCEPT {
//	scrolls_.changed = true;
	if(*lines.end() < firstVisibleLine_.line) {	// inserted before visible area
		firstVisibleLine_.line += lines.size();
		scrollOffsets_.bpd() += lines.size();
//		scrolls_.vertical.maximum += static_cast<int>(length(lines));
//		repaintRuler();
	} else if(*lines.begin() > firstVisibleLine_.line	// inserted at or after the first visible line
			|| (*lines.begin() == firstVisibleLine_.line && firstVisibleLine_.subline == 0)) {
//		scrolls_.vertical.maximum += static_cast<int>(length(lines));
//		redrawLine(lines.beginning(), true);
	} else {	// inserted around the first visible line
		firstVisibleLine_.line += lines.size();
		adjustBpdScrollPositions();
//		redrawLine(lines.beginning(), true);
	}
	visualLinesListeners_.notify<const boost::integer_range<Index>&>(&VisualLinesListener::visualLinesInserted, lines);
}

/// @see VisualLinesListener#visualLinesModified
void TextViewport::visualLinesModified(const boost::integer_range<Index>& lines,
		SignedIndex sublinesDifference, bool documentChanged, bool longestLineChanged) BOOST_NOEXCEPT {
	if(sublinesDifference == 0)	// number of visual lines was not changed
/*		redrawLines(lines)*/;
	else {
//		scrolls_.changed = true;
		if(*lines.end() < firstVisibleLine_.line) {	// changed before visible area
			scrollOffsets_.bpd() += sublinesDifference;
//			scrolls_.vertical.maximum += sublinesDifference;
//			repaintRuler();
		} else if(*lines.begin() > firstVisibleLine_.line	// changed at or after the first visible line
				|| (*lines.begin() == firstVisibleLine_.line && firstVisibleLine_.subline == 0)) {
//			scrolls_.vertical.maximum += sublinesDifference;
//			redrawLine(lines.beginning(), true);
		} else {	// changed lines contain the first visible line
			adjustBpdScrollPositions();
//			redrawLine(lines.beginning(), true);
		}
	}
	if(longestLineChanged) {
//		scrolls_.resetBars(*this, 'i', false);
//		scrolls_.changed = true;
	}
//	if(!documentChanged && scrolls_.changed)
//		updateScrollBars();
	visualLinesListeners_.notify<const boost::integer_range<Index>&, SignedIndex, bool, bool>(
		&VisualLinesListener::visualLinesModified, lines, sublinesDifference, documentChanged, longestLineChanged);
}


// free functions /////////////////////////////////////////////////////////////////////////////////

PhysicalTwoAxes<boost::optional<TextViewport::ScrollOffset>>
convertFlowRelativeScrollPositionsToPhysical(const TextViewport& viewport,
		const AbstractTwoAxes<boost::optional<TextViewport::ScrollOffset>>& positions) {
	switch(viewport.textRenderer().computedBlockFlowDirection()) {
		case HORIZONTAL_TB:
			return PhysicalTwoAxes<boost::optional<TextViewport::ScrollOffset>>(positions.ipd(), positions.bpd());
		case VERTICAL_RL:
			return PhysicalTwoAxes<boost::optional<TextViewport::ScrollOffset>>(
				(positions.bpd() != boost::none) ?
					boost::make_optional(viewport.textRenderer().layouts().numberOfVisualLines() - *positions.bpd() - 1) : boost::none,
				positions.ipd());
		case VERTICAL_LR:
			return PhysicalTwoAxes<boost::optional<TextViewport::ScrollOffset>>(positions.bpd(), positions.ipd());
		default:
			ASCENSION_ASSERT_NOT_REACHED();
	}
}

AbstractTwoAxes<boost::optional<TextViewport::ScrollOffset>>
convertPhysicalScrollPositionsToAbstract(const TextViewport& viewport,
		const PhysicalTwoAxes<boost::optional<TextViewport::ScrollOffset>>& positions) {
	AbstractTwoAxes<boost::optional<TextViewport::ScrollOffset>> result;
	switch(viewport.textRenderer().computedBlockFlowDirection()) {
		case HORIZONTAL_TB:
			result.bpd() = positions.y();
			result.ipd() = positions.x();
			break;
		case VERTICAL_RL:
			result.bpd() = (positions.x() != boost::none) ?
				boost::make_optional(viewport.textRenderer().layouts().numberOfVisualLines() - *positions.x() - 1) : boost::none;
			result.ipd() = positions.y();
			break;
		case VERTICAL_LR:
			result.bpd() = positions.x();
			result.ipd() = positions.y();
			break;
		default:
			ASCENSION_ASSERT_NOT_REACHED();
	}
	return result;
}

/**
 * Converts an inline progression scroll offset into user units.
 * @param viewport The viewport
 * @param scrollOffset The inline progression scroll offset
 * @return A scroll offset in user units
 */
Scalar font::inlineProgressionScrollOffsetInUserUnits(const TextViewport& viewport, const boost::optional<TextViewport::ScrollOffset>& scrollOffset /* = boost::none */) {
	const TextViewport::ScrollOffset offset = scrollOffset ? *scrollOffset : viewport.inlineProgressionOffset();
	return viewport.textRenderer().defaultFont()->metrics().averageCharacterWidth() * offset;
}

/**
 * Returns distance from the edge of content-area to the edge of the specified visual line in
 * pixels. The edges are the start side of @a layout (ex. left if left-to-right, bottom if
 * bottom-to-top).
 * @param layout The layout of the line
 * @param contentMeasure The measure of 'content-rectangle' of the content-area in pixels
 * @param subline The visual subline number
 * @return The indentation in pixels
 * @throw IndexOutOfBoundsException @a subline is invalid
 * @see font#lineStartEdge
 */
Scalar font::lineIndent(const TextLayout& layout, Scalar contentMeasure, Index subline /* = 0 */) {
	// TODO: ??? Is this same as TextLayout.lineStartEdge method?
	switch(layout.anchor(subline)) {
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
 * Returns distance from left/top-edge of the content-area to 'start-edge' of the specified line in
 * pixels.
 * @param layout The layout of the line
 * @param contentMeasure The measure of 'content-rectangle' of the content-area in pixels
 * @param subline The visual subline number
 * @return Distance from left/top-edge of the content-area to start-edge of @a line in pixels
 * @throw IndexOutOfBoundsException @a subline is invalid
 * @see font#lineIndent, TextLayout#lineStartEdge, TextViewer#inlineProgressionOffsetInViewport
 */
Scalar font::lineStartEdge(const TextLayout& layout, Scalar contentMeasure, Index subline /* = 0 */) {
	const Scalar indent = lineIndent(layout, contentMeasure, subline);
	return (layout.writingMode().inlineFlowDirection == LEFT_TO_RIGHT) ? indent : contentMeasure - indent;
}

/**
 * Returns the start egde of the specified line in viewport-coordinates.
 * @param viewport The viewport
 * @param line The line numbers
 * @return The start edge of the @a line in viewport-coordinates, in user units. If the writing
 *         mode of the layout is horizontal, the y-coordinate is zero. Otherwise if vertical, the
 *         x-coordinate is zero
 * @throw IndexOutOfBoundsException @a line is invalid
 */
Point font::lineStartEdge(const TextViewport& viewport, const VisualLine& line) {
	const TextRenderer& renderer = viewport.textRenderer();
	const AbstractTwoAxes<Scalar> lineStart(_ipd = renderer.lineStartEdge(line) - inlineProgressionScrollOffsetInUserUnits(viewport), _bpd = 0);
	const TextLayout& layout = renderer.layouts().at(line.line);	// this may throw IndexOutOfBoundsException
	Point result(mapAbstractToPhysical(layout.writingMode(), lineStart));

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
	return result;
}

/**
 * Converts the point in the viewport into the logical line number and visual subline offset.
 * @param p The point in the viewport in user coordinates
 * @param[out] snapped @c true if there was not a line at @a p. Optional
 * @return The visual line numbers
 * @see #location, TextLayout#hitTestCharacter, TextLayout#locateLine
 */
VisualLine font::locateLine(const TextViewport& viewport, const Point& p, bool* snapped /* = nullptr */) BOOST_NOEXCEPT {
	const graphics::Rectangle bounds(boost::geometry::make_zero<Point>(), geometry::size(viewport.boundsInView()));
	switch(viewport.textRenderer().computedBlockFlowDirection()) {
		case HORIZONTAL_TB:
			return mapBpdToLine(viewport, geometry::y(p) - geometry::top(bounds), snapped);
		case VERTICAL_RL:
			return mapBpdToLine(viewport, geometry::right(bounds) - geometry::x(p), snapped);
		case VERTICAL_LR:
			return mapBpdToLine(viewport, geometry::x(p) - geometry::left(bounds), snapped);
		default:
			ASCENSION_ASSERT_NOT_REACHED();
	}
}

/**
 * Converts the specified position in the document to a point in the view-coordinates.
 * @param viewport The viewport
 * @param position The document position
 * @param fullSearchBpd If this is @c false, this method stops at before- or after-edge of the
 *                      viewport. If this happened, the block-progression-dimension of the returned
 *                      point is @c std#numeric_limits&lt;Scalar&gt;::max() (for the before-edge)
 *                      or @c std#numeric_limits&lt;Scalar&gt;::min() (for the after-edge). If this
 *                      is @c true, the calculation is performed completely and returns an exact
 *                      location will (may be very slow)
 * @param edge The edge of the character. If this is @c CharacterEdge#LEADING, the returned point
 *             is the leading edge if the character (left if the character is left-to-right),
 *             otherwise returned point is the trailing edge (right if the character is
 *             left-to-right)
 * @return The point in view-coordinates (not viewport-coordinates) in pixels. The
 *         block-progression-dimension addresses the baseline of the line
 * @throw BadPositionException @a position is outside of the document
 * @see #viewToModel, #viewToModelInBounds, TextLayout#location
 */
Point font::modelToView(const TextViewport& viewport, const TextHit<k::Position>& position, bool fullSearchBpd) {
	// compute alignment-point of the line
	const BaselineIterator baseline(viewport, position.characterIndex().line, fullSearchBpd);
	const Point lineStart(lineStartEdge(viewport, VisualLine(position.characterIndex().line, 0)));
	Point p(
		geometry::_x = geometry::x(baseline.positionInViewport()) + geometry::x(lineStart),
		geometry::_y = geometry::y(baseline.positionInViewport()) + geometry::y(lineStart));
	const bool horizontal = isHorizontal(viewport.textRenderer().computedBlockFlowDirection());

	// compute offset in the line layout
	const TextLayout& lineLayout = viewport.textRenderer().layouts().at(position.characterIndex().line);
	const TextHit<> hitInLine(position.isLeadingEdge() ?
		TextHit<>::leading(position.characterIndex().offsetInLine) : TextHit<>::trailing(position.characterIndex().offsetInLine));
	const AbstractTwoAxes<Scalar> abstractOffset(lineLayout.hitToPoint(hitInLine));
	const Point physicalOffset(mapAbstractToPhysical(lineLayout.writingMode(), abstractOffset));

	// compute the result
	if(horizontal)
		geometry::x(p) += geometry::x(physicalOffset);
	else
		geometry::y(p) += geometry::y(physicalOffset);
	if(fullSearchBpd
			|| (*baseline != numeric_limits<Scalar>::max() && *baseline != numeric_limits<Scalar>::min())) {
		if(horizontal)
			geometry::y(p) += geometry::y(physicalOffset);
		else
			geometry::x(p) += geometry::x(physicalOffset);
	}

	return p;
}

template<>
TextViewport::SignedScrollOffset font::pageSize<0>(const TextViewport& viewport) {
	switch(viewport.textRenderer().computedBlockFlowDirection()) {
		case HORIZONTAL_TB:
			return static_cast<TextViewport::SignedScrollOffset>(viewport.numberOfVisibleCharactersInLine());
		case VERTICAL_RL:
			return -static_cast<TextViewport::SignedScrollOffset>(viewport.numberOfVisibleLines());
		case VERTICAL_LR:
			return +static_cast<TextViewport::SignedScrollOffset>(viewport.numberOfVisibleLines());
		default:
			ASCENSION_ASSERT_NOT_REACHED();
	}
}

template<>
TextViewport::SignedScrollOffset font::pageSize<1>(const TextViewport& viewport) {
	switch(viewport.textRenderer().computedBlockFlowDirection()) {
		case HORIZONTAL_TB:
			return static_cast<TextViewport::SignedScrollOffset>(viewport.numberOfVisibleLines());
		case VERTICAL_RL:
		case VERTICAL_LR:
			return static_cast<TextViewport::SignedScrollOffset>(viewport.numberOfVisibleCharactersInLine());
		default:
			ASCENSION_ASSERT_NOT_REACHED();
	}
}

//template TextViewport::SignedScrollOffset font::pageSize<0>(const TextViewport& viewport);
//template TextViewport::SignedScrollOffset font::pageSize<1>(const TextViewport& viewport);

namespace {
	inline Scalar mapViewportIpdToLineLayout(const TextViewport& viewport, Index line, Scalar ipd) {
		return ipd - viewport.inlineProgressionOffset()
			- lineStartEdge(viewport.textRenderer().layouts().at(line), viewport.contentMeasure());
	}

	// implements viewToModel and viewToModelInBounds in font namespace.
	boost::optional<TextHit<k::Position>> internalViewToModel(const TextViewport& viewport,
			const Point& pointInView, bool abortNoCharacter, k::locations::CharacterUnit snapPolicy) {
		Point p(pointInView);
		geometry::translate(p, Dimension(geometry::_dx = geometry::left(viewport.boundsInView()), geometry::_dy = geometry::top(viewport.boundsInView())));

		// locate the logical line
		bool outside;
		const VisualLine line(locateLine(viewport, p, &outside));
		if(abortNoCharacter && outside)
			return boost::none;
		const TextLayout& layout = viewport.textRenderer().layouts()[line.line];
		const BaselineIterator baseline(viewport, line.line, true);
		// locate the position in the line
		const bool horizontal = isHorizontal(layout.writingMode().blockFlowDirection);
		const PhysicalTwoAxes<Scalar> lineLocalPoint(horizontal ?
			Point(
				geometry::_x = mapViewportIpdToLineLayout(viewport, line.line, geometry::x(p)),
				geometry::_y = geometry::y(p) + geometry::y(baseline.positionInViewport()))
			: Point(
				geometry::_x = geometry::x(p) + geometry::x(baseline.positionInViewport()),
				geometry::_y = mapViewportIpdToLineLayout(viewport, line.line, geometry::y(p))));
		TextHit<> hitInLine(layout.hitTestCharacter(mapPhysicalToAbstract(layout.writingMode(), lineLocalPoint), &outside));
		if(abortNoCharacter && outside)
			return boost::none;

		// snap intervening position to the boundary
		if(hitInLine.characterIndex() != 0 && snapPolicy != k::locations::UTF16_CODE_UNIT) {
			using namespace text;
			const k::Document& document = viewport.textRenderer().presentation().document();
			const String& s = document.line(line.line);
			const bool interveningSurrogates =
				surrogates::isLowSurrogate(s[hitInLine.characterIndex()]) && surrogates::isHighSurrogate(s[hitInLine.characterIndex() - 1]);
			const Scalar ipd = horizontal ? lineLocalPoint.x() : lineLocalPoint.y();
			if(snapPolicy == k::locations::UTF32_CODE_UNIT) {
				if(interveningSurrogates) {
					const Index index = hitInLine.characterIndex() - 1;
					const TextHit<> leading(TextHit<>::leading(index)), trailing(TextHit<>::trailing(index));
					const Point leadingPoint(leading), trailingPoint(trailing);
					const Scalar leadingIpd = horizontal ? geometry::x(leadingPoint) : geometry::y(leadingPoint);
					const Scalar trailingIpd = horizontal ? geometry::x(trailingPoint) : geometry::y(trailingPoint);
					hitInLine = (abs(ipd - leadingIpd) <= abs(ipd - trailingIpd)) ? leading : trailing;
				}
			} else if(snapPolicy == k::locations::GRAPHEME_CLUSTER) {
				text::GraphemeBreakIterator<k::DocumentCharacterIterator> i(
					k::DocumentCharacterIterator(document, k::Region(line.line, boost::irange<Index>(0, s.length())), k::Position(line.line, hitInLine.characterIndex())));
				if(interveningSurrogates || !i.isBoundary(i.base())) {
					--i;
					const TextHit<> leading(TextHit<>::leading(i.base().tell().offsetInLine));
					const TextHit<> trailing(TextHit<>::trailing((++i).base().tell().offsetInLine));
					const Point leadingPoint(layout.hitToPoint(leading)), trailingPoint(layout.hitToPoint(trailing));
					const Scalar leadingIpd = horizontal ? geometry::x(leadingPoint) : geometry::y(leadingPoint);
					const Scalar trailingIpd = horizontal ? geometry::x(trailingPoint) : geometry::y(trailingPoint);
					hitInLine = (abs(ipd - leadingIpd) <= abs(ipd - trailingIpd)) ? leading : trailing;
				}
			} else
				throw UnknownValueException("snapPolicy");
		}
		return hitInLine.isLeadingEdge() ?
			TextHit<k::Position>::leading(k::Position(line.line, hitInLine.characterIndex()))
			: TextHit<k::Position>::trailing(k::Position(line.line, hitInLine.characterIndex()));
	}
}

/**
 * Returns the document position nearest from the specified point.
 * @param pointInView The point in view-coordinates (not viewport-coordinates). This can be outside
 *                    of the view
 * @param snapPolicy Which character boundary the returned position snapped to
 * @return The document position
 * @throw UnknownValueException @a snapPolicy is invalid
 * @see #modelToView, #viewToModelInBounds, TextLayout#hitTestCharacter
 */
TextHit<k::Position>&& font::viewToModel(const TextViewport& viewport,
		const Point& pointInView, k::locations::CharacterUnit snapPolicy /* = kernel::locations::GRAPHEME_CLUSTER */) {
	return move(*internalViewToModel(viewport, pointInView, false, snapPolicy));
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
 * @see #modelToView, #viewToModel, TextLayout#hitTestCharacter
 */
boost::optional<TextHit<k::Position>> font::viewToModelInBounds(const TextViewport& viewport,
		const Point& pointInView, k::locations::CharacterUnit snapPolicy /* = k::locations::GRAPHEME_CLUSTER */) {
	return internalViewToModel(viewport, pointInView, true, snapPolicy);
}
