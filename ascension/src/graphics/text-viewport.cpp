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
#include <ascension/graphics/text-renderer.hpp>
#include <ascension/graphics/text-viewport.hpp>
#include <ascension/kernel/document-character-iterator.hpp>
#include <ascension/presentation/text-style.hpp>

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

	const WritingMode writingMode(renderer.writingMode());
	Scalar viewportExtent;
	if(!tracksOutOfViewport() && n > 0)
		viewportExtent = isHorizontal(writingMode.blockFlowDirection) ?
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

	NativePoint newAxis(positionInViewport_);
	switch(writingMode.blockFlowDirection) {
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
	NativePoint axis;
	const NativeRectangle bounds(geometry::make<NativeRectangle>(
		geometry::make<NativePoint>(0, 0), geometry::size(viewport().boundsInView())));
	switch(viewport().textRenderer().writingMode().blockFlowDirection) {
		case HORIZONTAL_TB:
			axis = geometry::make<NativePoint>(0, geometry::top(bounds) + baseline);
			break;
		case VERTICAL_RL:
			axis = geometry::make<NativePoint>(geometry::right(bounds) - baseline, 0);
			break;
		case VERTICAL_LR:
			axis = geometry::make<NativePoint>(geometry::left(bounds) + baseline, 0);
			break;
		default:
			ASCENSION_ASSERT_NOT_REACHED();
	}

	// commit
	line_ = firstVisibleLine;
	distanceFromViewportBeforeEdge_ = baseline;
	positionInViewport_ = axis;
}

inline void BaselineIterator::invalidate() /*throw()*/ {
	geometry::x(positionInViewport_) = geometry::y(positionInViewport_) = 1;
}

inline bool BaselineIterator::isValid() const /*throw()*/ {
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
		const WritingMode writingMode(viewport.textRenderer().writingMode());
		const PhysicalFourSides<Scalar>& physicalSpaces = viewport.textRenderer().spaceWidths();
		FlowRelativeFourSides<Scalar> abstractSpaces;
		mapPhysicalToFlowRelative(writingMode, physicalSpaces, abstractSpaces);
		const Scalar spaceBefore = abstractSpaces.before();
		const Scalar spaceAfter = abstractSpaces.after();
		const Scalar borderBefore = 0, borderAfter = 0, paddingBefore = 0, paddingAfter = 0;
		const Scalar before = spaceBefore + borderBefore + paddingBefore;
		const Scalar after = (isHorizontal(writingMode.blockFlowDirection) ?
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
					result.subline = layout->locateLine(bpd - lineBefore, outside);
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

	inline Scalar mapLineLayoutIpdToViewport(const TextViewport& viewport, Index line, Scalar ipd) {
		return ipd + viewport.inlineProgressionOffset()
			+ lineStartEdge(viewport.textRenderer().layouts().at(line), viewport.contentMeasure());
	}
	inline Scalar mapViewportIpdToLineLayout(const TextViewport& viewport, Index line, Scalar ipd) {
		return ipd - viewport.inlineProgressionOffset()
			- lineStartEdge(viewport.textRenderer().layouts().at(line), viewport.contentMeasure());
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
 * @return The measure of the 'allocation-rectangle' in pixels
 * @see #contentMeasure
 */
Scalar TextViewport::allocationMeasure() const /*throw()*/ {
	const TextRenderer& renderer = textRenderer();
	const bool horizontal = isHorizontal(renderer.writingMode().blockFlowDirection);
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
 * @return The measure of the 'content-rectangle' in pixels
 * @see #allocationMeasure
 */
Scalar TextViewport::contentMeasure() const /*throw()*/ {
	return max(
		textRenderer().layouts().maximumMeasure(),
		static_cast<Scalar>(isHorizontal(textRenderer().writingMode().blockFlowDirection) ?
			geometry::dx(boundsInView()) : geometry::dy(boundsInView())));
}

/**
 * Returns the number of the drawable columns in the window.
 * @return The number of columns
 */
float TextViewport::numberOfVisibleCharactersInLine() const /*throw()*/ {
	const bool horizontal = isHorizontal(textRenderer().writingMode().blockFlowDirection);
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
	const bool horizontal = isHorizontal(textRenderer().writingMode().blockFlowDirection);
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
	switch(textRenderer().writingMode().blockFlowDirection) {
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
void TextViewport::setBoundsInView(const NativeRectangle& bounds) {
	const NativeRectangle oldBounds(boundsInView());
	// TODO: not implemented.
	listeners_.notify<const NativeRectangle&>(&TextViewportListener::viewportBoundsInViewChanged, oldBounds);
}

/// @see VisualLinesListener#visualLinesDeleted
void TextViewport::visualLinesDeleted(const Range<Index>& lines, Index sublines, bool longestLineChanged) /*throw()*/ {
//	scrolls_.changed = true;
	if(lines.end() < firstVisibleLine_.line) {	// deleted before visible area
		firstVisibleLine_.line -= length(lines);
		scrollOffsets_.bpd() -= sublines;
//		scrolls_.vertical.maximum -= static_cast<int>(sublines);
//		repaintRuler();
	} else if(lines.beginning() > firstVisibleLine_.line	// deleted the first visible line and/or after it
			|| (lines.beginning() == firstVisibleLine_.line && firstVisibleLine_.subline == 0)) {
//		scrolls_.vertical.maximum -= static_cast<int>(sublines);
//		redrawLine(lines.beginning(), true);
	} else {	// deleted lines contain the first visible line
		firstVisibleLine_.line = lines.beginning();
		adjustBpdScrollPositions();
//		redrawLine(lines.beginning(), true);
	}
//	if(longestLineChanged)
//		scrolls_.resetBars(*this, 'i', false);
	visualLinesListeners_.notify<const Range<Index>&, Index, bool>(&VisualLinesListener::visualLinesDeleted, lines, sublines, longestLineChanged);
}

/// @see VisualLinesListener#visualLinesInserted
void TextViewport::visualLinesInserted(const Range<Index>& lines) /*throw()*/ {
//	scrolls_.changed = true;
	if(lines.end() < firstVisibleLine_.line) {	// inserted before visible area
		firstVisibleLine_.line += length(lines);
		scrollOffsets_.bpd() += length(lines);
//		scrolls_.vertical.maximum += static_cast<int>(length(lines));
//		repaintRuler();
	} else if(lines.beginning() > firstVisibleLine_.line	// inserted at or after the first visible line
			|| (lines.beginning() == firstVisibleLine_.line && firstVisibleLine_.subline == 0)) {
//		scrolls_.vertical.maximum += static_cast<int>(length(lines));
//		redrawLine(lines.beginning(), true);
	} else {	// inserted around the first visible line
		firstVisibleLine_.line += length(lines);
		adjustBpdScrollPositions();
//		redrawLine(lines.beginning(), true);
	}
	visualLinesListeners_.notify<const Range<Index>&>(&VisualLinesListener::visualLinesInserted, lines);
}

/// @see VisualLinesListener#visualLinesModified
void TextViewport::visualLinesModified(const Range<Index>& lines,
		SignedIndex sublinesDifference, bool documentChanged, bool longestLineChanged) /*throw()*/ {
	if(sublinesDifference == 0)	// number of visual lines was not changed
/*		redrawLines(lines)*/;
	else {
//		scrolls_.changed = true;
		if(lines.end() < firstVisibleLine_.line) {	// changed before visible area
			scrollOffsets_.bpd() += sublinesDifference;
//			scrolls_.vertical.maximum += sublinesDifference;
//			repaintRuler();
		} else if(lines.beginning() > firstVisibleLine_.line	// changed at or after the first visible line
				|| (lines.beginning() == firstVisibleLine_.line && firstVisibleLine_.subline == 0)) {
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
	visualLinesListeners_.notify<const Range<Index>&, SignedIndex, bool, bool>(
		&VisualLinesListener::visualLinesModified, lines, sublinesDifference, documentChanged, longestLineChanged);
}


// free functions /////////////////////////////////////////////////////////////////////////////////

PhysicalTwoAxes<boost::optional<TextViewport::ScrollOffset>>
convertFlowRelativeScrollPositionsToPhysical(const TextViewport& viewport,
		const AbstractTwoAxes<boost::optional<TextViewport::ScrollOffset>>& positions) {
	switch(viewport.textRenderer().writingMode().blockFlowDirection) {
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
	switch(viewport.textRenderer().writingMode().blockFlowDirection) {
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
 * Converts an inline progression scroll offset into pixels.
 * @param viewport The viewport
 * @param scrollOffset The inline progression scroll offset
 * @return A scroll offset in pixels
 */
Scalar font::inlineProgressionScrollOffsetInPixels(const TextViewport& viewport, TextViewport::ScrollOffset scrollOffset) {
	return viewport.textRenderer().defaultFont()->metrics().averageCharacterWidth() * scrollOffset;
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
	switch(layout.anchor()) {
		case TEXT_ANCHOR_START:
			return 0;
		case TEXT_ANCHOR_MIDDLE:
			return (contentMeasure - layout.measure(subline)) / 2;
		case TEXT_ANCHOR_END:
			return contentMeasure - layout.measure(subline);
		default:
			ASCENSION_ASSERT_NOT_REACHED();
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
 * Converts the point in the viewport into the logical line number and visual subline offset.
 * @param p The point in the viewport in pixels
 * @param[out] snapped @c true if there was not a line at @a p. Optional
 * @see #location, #mapBpdToLine, TextLayout#locateLine, TextLayout#offset
 */
VisualLine font::locateLine(const TextViewport& viewport, const NativePoint& p, bool* snapped /* = nullptr */) /*throw()*/ {
	const NativeRectangle bounds(geometry::make<NativeRectangle>(
		geometry::make<NativePoint>(0, 0), geometry::size(viewport.boundsInView())));
	switch(viewport.textRenderer().writingMode().blockFlowDirection) {
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
 * @param edge The edge of the character. If this is @c graphics#font#TextLayout#LEADING, the
 *             returned point is the leading edge if the character (left if the character is
 *             left-to-right), otherwise returned point is the trailing edge (right if the
 *             character is left-to-right)
 * @return The point in view-coordinates (not viewport-coordinates) in pixels. The
 *         block-progression-dimension addresses the baseline of the line
 * @throw BadPositionException @a position is outside of the document
 * @see #modelToViewInBounds, #viewToModel, TextLayout#location
 */
NativePoint font::modelToView(const TextViewport& viewport,
		const k::Position& position, bool fullSearchBpd, TextLayout::Edge edge /* = TextLayout::LEADING */) {
	// get alignment-point
	const BaselineIterator baseline(viewport, position.line, fullSearchBpd);
	NativePoint p(baseline.positionInViewport());
	const bool horizontal = isHorizontal(viewport.textRenderer().writingMode().blockFlowDirection);

	// apply offset in line layout
	const NativePoint offset(viewport.textRenderer().layouts().at(position.line).location(position.offsetInLine, edge));
	if(fullSearchBpd || horizontal
			|| (*baseline != numeric_limits<Scalar>::max() && *baseline != numeric_limits<Scalar>::min())) {
//		assert(geometry::x(p) != numeric_limits<Scalar>::max() && geometry::x(p) != numeric_limits<Scalar>::min());
		geometry::x(p) += geometry::x(offset);
	}
	if(fullSearchBpd || !horizontal
			|| (*baseline != numeric_limits<Scalar>::max() && *baseline != numeric_limits<Scalar>::min())) {
//		assert(geometry::y(p) != numeric_limits<Scalar>::max() && geometry::y(p) != numeric_limits<Scalar>::min()));
		geometry::y(p) += geometry::y(offset);
	}

	// apply viewport offset in inline-progression-direction
	if(horizontal)
		geometry::x(p) = mapLineLayoutIpdToViewport(viewport, position.line, geometry::x(p));
	else
		geometry::y(p) = mapLineLayoutIpdToViewport(viewport, position.line, geometry::y(p));

	return geometry::translate(p, geometry::make<NativeSize>(
		geometry::left(viewport.boundsInView()), geometry::top(viewport.boundsInView())));
}

template<>
TextViewport::SignedScrollOffset font::pageSize<geometry::X_COORDINATE>(const TextViewport& viewport) {
	switch(viewport.textRenderer().writingMode().blockFlowDirection) {
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
TextViewport::SignedScrollOffset font::pageSize<geometry::Y_COORDINATE>(const TextViewport& viewport) {
	switch(viewport.textRenderer().writingMode().blockFlowDirection) {
		case HORIZONTAL_TB:
			return static_cast<TextViewport::SignedScrollOffset>(viewport.numberOfVisibleLines());
		case VERTICAL_RL:
		case VERTICAL_LR:
			return static_cast<TextViewport::SignedScrollOffset>(viewport.numberOfVisibleCharactersInLine());
		default:
			ASCENSION_ASSERT_NOT_REACHED();
	}
}

template TextViewport::SignedScrollOffset font::pageSize<geometry::X_COORDINATE>(const TextViewport& viewport);
template TextViewport::SignedScrollOffset font::pageSize<geometry::Y_COORDINATE>(const TextViewport& viewport);

namespace {
	// implements viewToModel and viewToModelInBounds in font namespace.
	boost::optional<k::Position> internalViewToModel(const TextViewport& viewport,
			const NativePoint& pointInView, TextLayout::Edge edge,
			bool abortNoCharacter, k::locations::CharacterUnit snapPolicy) {
		NativePoint p(pointInView);
		geometry::translate(p, geometry::make<NativeSize>(
			geometry::left(viewport.boundsInView()), geometry::top(viewport.boundsInView())));
		k::Position result;

		// locate the logical line
		Index subline;
		bool outside;
		{
			const VisualLine temp(locateLine(viewport, p, &outside));
			result.line = temp.line;
			subline = temp.subline;
		}
		if(abortNoCharacter && outside)
			return boost::none;
		const TextLayout& layout = viewport.textRenderer().layouts()[result.line];
		const BaselineIterator baseline(viewport, result.line, true);

		// locate the position in the line
		const bool horizontal = isHorizontal(layout.writingMode().blockFlowDirection);
		NativePoint lineLocalPoint(horizontal ?
			geometry::make<NativePoint>(
				mapViewportIpdToLineLayout(viewport, result.line, geometry::x(p)),
				geometry::y(p) + geometry::y(baseline.positionInViewport()))
			: geometry::make<NativePoint>(
				geometry::x(p) + geometry::x(baseline.positionInViewport()),
				mapViewportIpdToLineLayout(viewport, result.line, geometry::y(p))));
		if(edge == TextLayout::LEADING)
			result.offsetInLine = layout.offset(lineLocalPoint, &outside).first;
		else if(edge == TextLayout::TRAILING)
			result.offsetInLine = layout.offset(lineLocalPoint, &outside).second;
		else
			throw UnknownValueException("edge");
		if(abortNoCharacter && outside)
			return boost::none;

		// snap intervening position to the boundary
		if(result.offsetInLine != 0 && snapPolicy != k::locations::UTF16_CODE_UNIT) {
			using namespace text;
			const k::Document& document = viewport.textRenderer().presentation().document();
			const String& s = document.line(result.line);
			const bool interveningSurrogates =
				surrogates::isLowSurrogate(s[result.offsetInLine]) && surrogates::isHighSurrogate(s[result.offsetInLine - 1]);
			const Scalar ipd = horizontal ? static_cast<Scalar>(geometry::x(lineLocalPoint)) : geometry::y(lineLocalPoint);
			if(snapPolicy == k::locations::UTF32_CODE_UNIT) {
				if(interveningSurrogates) {
					if(edge == TextLayout::LEADING)
						--result.offsetInLine;
					else {
						const NativePoint leading(layout.location(result.offsetInLine - 1));
						const NativePoint trailing(layout.location(result.offsetInLine + 1));
						const Scalar leadingIpd = horizontal ? geometry::x(leading) : geometry::y(leading);
						const Scalar trailingIpd = horizontal ? geometry::x(trailing) : geometry::y(trailing);
						(detail::distance<Scalar>(ipd, leadingIpd)
							<= detail::distance<Scalar>(ipd, trailingIpd)) ? --result.offsetInLine : ++result.offsetInLine;
					}
				}
			} else if(snapPolicy == k::locations::GRAPHEME_CLUSTER) {
				text::GraphemeBreakIterator<k::DocumentCharacterIterator> i(
					k::DocumentCharacterIterator(document, k::Region(result.line, make_pair(0, s.length())), result));
				if(interveningSurrogates || !i.isBoundary(i.base())) {
					--i;
					if(edge == TextLayout::LEADING)
						result.offsetInLine = i.base().tell().offsetInLine;
					else {
						const k::Position backward(i.base().tell()), forward((++i).base().tell());
						const NativePoint leading(layout.location(backward.offsetInLine)), trailing(layout.location(forward.offsetInLine));
						const Scalar backwardIpd = horizontal ? geometry::x(leading) : geometry::y(leading);
						const Scalar forwardIpd = horizontal ? geometry::x(trailing) : geometry::y(trailing);
						result.offsetInLine = ((detail::distance<Scalar>(ipd, backwardIpd)
							<= detail::distance<Scalar>(ipd, forwardIpd)) ? backward : forward).offsetInLine;
					}
				}
			} else
				throw UnknownValueException("snapPolicy");
		}
		return result;
	}
}

/**
 * Returns the document position nearest from the specified point.
 * @param pointInView The point in view-coordinates (not viewport-coordinates). This can be outside
 *                    of the view
 * @param edge If set @c TextLayout#LEADING, the result is the leading of the character at
 *             @a pointInView. Otherwise the result is the position nearest @a pointInView
 * @param abortNoCharacter If set to @c true, this method returns @c boost#none immediately when
 *                         @a p hovered outside of the text layout (e.g. far left or right of the
 *                         line, beyond the last line, ...)
 * @param snapPolicy Which character boundary the returned position snapped to
 * @return The document position, or @c boost#none if @a abortNoCharacter was @c true and
 *         @a pointInView is outside of the layout
 * @throw UnknownValueException @a edge and/or snapPolicy are invalid
 * @see #modelToView, #viewToModelInBounds, TextLayout#offset
 */
k::Position font::viewToModel(const TextViewport& viewport,
		const NativePoint& pointInView, TextLayout::Edge edge,
		k::locations::CharacterUnit snapPolicy /* = kernel::locations::GRAPHEME_CLUSTER */) {
	return *internalViewToModel(viewport, pointInView, edge, false, snapPolicy);
}


/**
 * Returns the document position nearest from the specified point.
 * @param pointInView The point in view-coordinates (not viewport-coordinates). This can be outside
 *                    of the view
 * @param edge If set @c TextLayout#LEADING, the result is the leading of the character at
 *             @a pointInView. Otherwise the result is the position nearest @a pointInView
 * @param abortNoCharacter If set to @c true, this method returns @c boost#none immediately when
 *                         @a p hovered outside of the text layout (e.g. far left or right of the
 *                         line, beyond the last line, ...)
 * @param snapPolicy Which character boundary the returned position snapped to
 * @return The document position, or @c boost#none if @a abortNoCharacter was @c true and
*          @a pointInView is outside of the layout
 * @throw UnknownValueException @a edge and/or snapPolicy are invalid
 * @see #modelToView, #viewToModel, TextLayout#offset
 */
boost::optional<k::Position> font::viewToModelInBounds(const TextViewport& viewport,
		const NativePoint& pointInView, TextLayout::Edge edge,
		k::locations::CharacterUnit snapPolicy /* = k::locations::GRAPHEME_CLUSTER */) {
	return internalViewToModel(viewport, pointInView, edge, true, snapPolicy);
}
