/**
 * @file default-mouse-input-strategy.cpp
 * @author exeal
 * @date 2003-2006 was EditView.cpp and EditViewWindowMessages.cpp
 * @date 2006-2011 was viewer.cpp
 * @date 2011-10-04 separated from viewer.cpp
 */

#include <ascension/corelib/text/break-iterator.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/kernel/document-character-iterator.hpp>
#include <ascension/text-editor/session.hpp>	// texteditor.xxxIncrementalSearch
#include <ascension/viewer/base/cursor.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/default-mouse-input-strategy.hpp>
#include <ascension/viewer/viewer.hpp>
#include <limits>	// std.numeric_limit

using namespace ascension;
using namespace ascension::viewers;
using namespace ascension::viewers::base;
using namespace ascension::presentation;
using namespace ascension::graphics;
using namespace ascension::graphics::font;
using namespace std;
namespace k = ascension::kernel;


// DefaultMouseInputStrategy //////////////////////////////////////////////////////////////////////

namespace {
	/// Circled window displayed at which the auto scroll started.
	class AutoScrollOriginMark : public Widget {
		ASCENSION_NONCOPYABLE_TAG(AutoScrollOriginMark);
	public:
		/// Defines the type of the cursors obtained by @c #cursorForScrolling method.
		enum CursorType {
			CURSOR_NEUTRAL,	///< Indicates no scrolling.
			CURSOR_UPWARD,	///< Indicates scrolling upward.
			CURSOR_DOWNWARD	///< Indicates scrolling downward.
		};
	public:
		explicit AutoScrollOriginMark(const TextViewer& viewer) /*throw()*/;
		void initialize(const TextViewer& viewer);
		static const Cursor& cursorForScrolling(CursorType type);
	private:
		void paint(graphics::PaintContext& context);
#if defined(ASCENSION_WINDOW_SYSTEM_WIN32)
		void provideClassInformation(ClassInformation& classInformation) const {
			classInformation.style = CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW;
			classInformation.background = COLOR_WINDOW;
			classInformation.cursor = MAKEINTRESOURCEW(32513);	// IDC_IBEAM
		}
		basic_string<WCHAR> provideClassName() const {return L"AutoScrollOriginMark";}
#endif // ASCENSION_WINDOW_SYSTEM_WIN32
	private:
		static const Scalar WINDOW_WIDTH = 28;
	};
} // namespace @0

/**
 * Constructor.
 * @param viewer The text viewer
 */
AutoScrollOriginMark::AutoScrollOriginMark(const TextViewer& viewer) /*throw()*/ : Widget(viewer) {
	// TODO: Set transparency on window system other than Win32.
#if defined(ASCENSION_WINDOW_SYSTEM_WIN32)
	// calling CreateWindowExW with WS_EX_LAYERED will fail on NT 4.0
	::SetWindowLongW(identifier().get(), GWL_EXSTYLE,
		::GetWindowLongW(identifier().get(), GWL_EXSTYLE) | WS_EX_LAYERED);
#endif // ASCENSION_WINDOW_SYSTEM_WIN32

	resize(geometry::make<NativeSize>(WINDOW_WIDTH + 1, WINDOW_WIDTH + 1));
	win32::Handle<HRGN> rgn(::CreateEllipticRgn(0, 0, WINDOW_WIDTH + 1, WINDOW_WIDTH + 1), &::DeleteObject);
	setShape(rgn);
#if defined(ASCENSION_WINDOW_SYSTEM_WIN32)
	::SetLayeredWindowAttributes(identifier().get(), ::GetSysColor(COLOR_WINDOW), 0, LWA_COLORKEY);
#endif // ASCENSION_WINDOW_SYSTEM_WIN32
}

/**
 * Returns the cursor should be shown when the auto-scroll is active.
 * @param type The type of the cursor to obtain
 * @return The cursor. Do not destroy the returned value
 * @throw UnknownValueException @a type is unknown
 */
const base::Cursor& AutoScrollOriginMark::cursorForScrolling(CursorType type) {
	static auto_ptr<Cursor> instances[3];
	if(type >= ASCENSION_COUNTOF(instances))
		throw UnknownValueException("type");
	if(instances[type].get() == 0) {
		static const uint8_t AND_LINE_3_TO_11[] = {
			0xff, 0xfe, 0x7f, 0xff,
			0xff, 0xfc, 0x3f, 0xff,
			0xff, 0xf8, 0x1f, 0xff,
			0xff, 0xf0, 0x0f, 0xff,
			0xff, 0xe0, 0x07, 0xff,
			0xff, 0xc0, 0x03, 0xff,
			0xff, 0x80, 0x01, 0xff,
			0xff, 0x00, 0x00, 0xff,
			0xff, 0x80, 0x01, 0xff
		};
		static const uint8_t XOR_LINE_3_TO_11[] = {
			0x00, 0x01, 0x80, 0x00,
			0x00, 0x02, 0x40, 0x00,
			0x00, 0x04, 0x20, 0x00,
			0x00, 0x08, 0x10, 0x00,
			0x00, 0x10, 0x08, 0x00,
			0x00, 0x20, 0x04, 0x00,
			0x00, 0x40, 0x02, 0x00,
			0x00, 0x80, 0x01, 0x00,
			0x00, 0x7f, 0xfe, 0x00
		};
		static const uint8_t AND_LINE_13_TO_18[] = {
			0xff, 0xfe, 0x7f, 0xff,
			0xff, 0xfc, 0x3f, 0xff,
			0xff, 0xf8, 0x1f, 0xff,
			0xff, 0xf8, 0x1f, 0xff,
			0xff, 0xfc, 0x3f, 0xff,
			0xff, 0xfe, 0x7f, 0xff,
		};
		static const uint8_t XOR_LINE_13_TO_18[] = {
			0x00, 0x01, 0x80, 0x00,
			0x00, 0x02, 0x40, 0x00,
			0x00, 0x04, 0x20, 0x00,
			0x00, 0x04, 0x20, 0x00,
			0x00, 0x02, 0x40, 0x00,
			0x00, 0x01, 0x80, 0x00
		};
		static const uint8_t AND_LINE_20_TO_28[] = {
			0xff, 0x80, 0x01, 0xff,
			0xff, 0x00, 0x00, 0xff,
			0xff, 0x80, 0x01, 0xff,
			0xff, 0xc0, 0x03, 0xff,
			0xff, 0xe0, 0x07, 0xff,
			0xff, 0xf0, 0x0f, 0xff,
			0xff, 0xf8, 0x1f, 0xff,
			0xff, 0xfc, 0x3f, 0xff,
			0xff, 0xfe, 0x7f, 0xff
		};
		static const uint8_t XOR_LINE_20_TO_28[] = {
			0x00, 0x7f, 0xfe, 0x00,
			0x00, 0x80, 0x01, 0x00,
			0x00, 0x40, 0x02, 0x00,
			0x00, 0x20, 0x04, 0x00,
			0x00, 0x10, 0x08, 0x00,
			0x00, 0x08, 0x10, 0x00,
			0x00, 0x04, 0x20, 0x00,
			0x00, 0x02, 0x40, 0x00,
			0x00, 0x01, 0x80, 0x00
		};
		uint8_t andBits[4 * 32], xorBits[4 * 32];
		// fill canvases
		memset(andBits, 0xff, 4 * 32);
		memset(xorBits, 0x00, 4 * 32);
		// draw lines
		if(type == CURSOR_NEUTRAL || type == CURSOR_UPWARD) {
			memcpy(andBits + 4 * 3, AND_LINE_3_TO_11, sizeof(AND_LINE_3_TO_11));
			memcpy(xorBits + 4 * 3, XOR_LINE_3_TO_11, sizeof(XOR_LINE_3_TO_11));
		}
		memcpy(andBits + 4 * 13, AND_LINE_13_TO_18, sizeof(AND_LINE_13_TO_18));
		memcpy(xorBits + 4 * 13, XOR_LINE_13_TO_18, sizeof(XOR_LINE_13_TO_18));
		if(type == CURSOR_NEUTRAL || type == CURSOR_DOWNWARD) {
			memcpy(andBits + 4 * 20, AND_LINE_20_TO_28, sizeof(AND_LINE_20_TO_28));
			memcpy(xorBits + 4 * 20, XOR_LINE_20_TO_28, sizeof(XOR_LINE_20_TO_28));
		}
#if defined(ASCENSION_OS_WINDOWS)
		instances[type].reset(new base::Cursor(win32::Handle<HCURSOR>(::CreateCursor(::GetModuleHandleW(0), 16, 16, 32, 32, andBits, xorBits), &::DestroyCursor)));
#else
		instances[type].reset(new base::Cursor(bitmap));
#endif
	}
	return *instances[type];
}

/// @see Widget#paint
void AutoScrollOriginMark::paint(PaintContext& context) {
	const Color color(SystemColors::get(SystemColors::APP_WORKSPACE));
	context.setStrokeStyle(Paint(color));
	context.setFillStyle(Paint(color));

	context
		.beginPath()
		.moveTo(geometry::make<NativePoint>(13, 3))
		.lineTo(geometry::make<NativePoint>(7, 9))
		.lineTo(geometry::make<NativePoint>(20, 9))
		.lineTo(geometry::make<NativePoint>(14, 3))
		.closePath()
		.fill();
	context
		.beginPath()
		.moveTo(geometry::make<NativePoint>(13, 24))
		.lineTo(geometry::make<NativePoint>(7, 18))
		.lineTo(geometry::make<NativePoint>(20, 18))
		.lineTo(geometry::make<NativePoint>(14, 24))
		.closePath()
		.fill();
	context
		.beginPath()
		.moveTo(geometry::make<NativePoint>(13, 12))
		.lineTo(geometry::make<NativePoint>(15, 12))
		.stroke();
	context
		.beginPath()
		.moveTo(geometry::make<NativePoint>(12, 13))
		.lineTo(geometry::make<NativePoint>(16, 13))
		.stroke();
	context
		.beginPath()
		.moveTo(geometry::make<NativePoint>(12, 14))
		.lineTo(geometry::make<NativePoint>(16, 14))
		.stroke();
	context
		.beginPath()
		.moveTo(geometry::make<NativePoint>(13, 15))
		.lineTo(geometry::make<NativePoint>(15, 15))
		.stroke();
}

/**
 * Standard implementation of @c MouseOperationStrategy interface.
 *
 * This class implements the standard behavior for the user's mouse input.
 *
 * - Begins drag-and-drop operation when the mouse moved with the left button down.
 * - Enters line-selection mode if the mouse left button pressed when the cursor is over the vertical ruler.
 * - When the cursor is over an invokable link, pressing the left button to open that link.
 * - Otherwise when the left button pressed, moves the caret to that position. Modifier keys change
 *   this behavior as follows: [Shift] The anchor will not move. [Control] Enters word-selection
 *   mode. [Alt] Enters rectangle-selection mode. These modifications can be combined.
 * - Double click of the left button selects the word under the cursor. And enters word-selection
 *   mode.
 * - Click the middle button to enter auto-scroll mode.
 * - If the mouse moved with the middle pressed, enters temporary auto-scroll mode. This mode
 *   automatically ends when the button was released.
 * - Changes the mouse cursor according to the position of the cursor (Arrow, I-beam and hand).
 */

const unsigned int DefaultMouseInputStrategy::SELECTION_EXPANSION_INTERVAL = 100;
const unsigned int DefaultMouseInputStrategy::DRAGGING_TRACK_INTERVAL = 100;

/**
 * Constructor.
 * @param dragAndDropSupportLevel The drag-and-drop feature support level
 */
DefaultMouseInputStrategy::DefaultMouseInputStrategy(
		DragAndDropSupport dragAndDropSupportLevel) : viewer_(0), state_(NONE), lastHoveredHyperlink_(0) {
#ifdef ASCENSION_WINDOW_SYSTEM_WIN32
	if((dnd_.supportLevel = dragAndDropSupportLevel) >= SUPPORT_DND_WITH_DRAG_IMAGE) {
		dnd_.dragSourceHelper.ComPtr<IDragSourceHelper>::ComPtr(CLSID_DragDropHelper, IID_IDragSourceHelper, CLSCTX_INPROC_SERVER);
		if(dnd_.dragSourceHelper.get() != 0) {
			dnd_.dropTargetHelper.ComPtr<IDropTargetHelper>::ComPtr(CLSID_DragDropHelper, IID_IDropTargetHelper, CLSCTX_INPROC_SERVER);
			if(dnd_.dropTargetHelper.get() == 0)
				dnd_.dragSourceHelper.reset();
		}
	}
#endif // ASCENSION_WINDOW_SYSTEM_WIN32
}
NativeSize DefaultMouseInputStrategy::calculateDnDScrollOffset(const TextViewer& viewer) {
	const NativePoint p = viewer.mapFromGlobal(Cursor::position());
	const NativeRectangle clientBounds(viewer.bounds(false));
	PhysicalFourSides<Scalar> spaces(viewer.spaceWidths());
	const Font::Metrics& fontMetrics = viewer.textRenderer().defaultFont()->metrics();
	spaces.left = max<Scalar>(fontMetrics.averageCharacterWidth(), spaces.left);
	spaces.top = max<Scalar>(fontMetrics.linePitch() / 2, spaces.top);
	spaces.right = max<Scalar>(fontMetrics.averageCharacterWidth(), spaces.right);
	spaces.bottom = max<Scalar>(fontMetrics.linePitch() / 2, spaces.bottom);

	// On Win32, oleidl.h defines the value named DD_DEFSCROLLINSET, but...

	geometry::Coordinate<NativeSize>::Type dx = 0, dy = 0;
	if(includes(makeRange(geometry::top(clientBounds), geometry::top(clientBounds) + spaces.top), geometry::y(p)))
		dy = -1;
	else if(geometry::y(p) >= geometry::bottom(clientBounds) - spaces.bottom && geometry::y(p) < geometry::bottom(clientBounds))
		dy = +1;
	if(includes(makeRange(geometry::left(clientBounds), geometry::left(clientBounds) + spaces.left), geometry::x(p)))
		dx = -3;	// viewer_->numberOfVisibleColumns()
	else if(geometry::x(p) >= geometry::right(clientBounds) - spaces.right && geometry::x(p) < geometry::right(clientBounds))
		dx = +3;	// viewer_->numberOfVisibleColumns()
	return geometry::make<NativeSize>(dx, dy);
}

/// @see MouseInputStrategy#captureChanged
void DefaultMouseInputStrategy::captureChanged() {
	timer_.stop();
	state_ = NONE;
}

/**
 * Ends the auto scroll.
 * @return true if the auto scroll was active
 */
bool DefaultMouseInputStrategy::endAutoScroll() {
	if(state_ == AUTO_SCROLL_DRAGGING || state_ == AUTO_SCROLL) {
		timer_.stop();
		state_ = NONE;
		autoScrollOriginMark_->hide();
		viewer_->releaseInput();
		return true;
	}
	return false;
}

/// Extends the selection to the current cursor position.
void DefaultMouseInputStrategy::extendSelection(const k::Position* to /* = 0 */) {
	if((state_ & SELECTION_EXTENDING_MASK) != SELECTION_EXTENDING_MASK)
		throw IllegalStateException("not extending the selection.");
	k::Position destination;
	if(to == 0) {
		const NativeRectangle viewport(viewer_->bounds(false));
		const PhysicalFourSides<Scalar> spaces(viewer_->spaceWidths());
		NativePoint p(viewer_->mapFromGlobal(Cursor::position()));
		Caret& caret = viewer_->caret();
		if(state_ != EXTENDING_CHARACTER_SELECTION) {
			const TextViewer::HitTestResult htr = viewer_->hitTest(p);
			if(state_ == EXTENDING_LINE_SELECTION && htr != TextViewer::INDICATOR_MARGIN && htr != TextViewer::LINE_NUMBERS)
				// end line selection
				state_ = EXTENDING_CHARACTER_SELECTION;
		}
		p = geometry::make<NativePoint>(
				min<Scalar>(
					max<Scalar>(geometry::x(p), geometry::left(viewport) + spaces.left),
					geometry::right(viewport) - spaces.right),
				min<Scalar>(
					max<Scalar>(geometry::y(p), geometry::top(viewport) + spaces.top),
					geometry::bottom(viewport) - spaces.bottom));
		destination = viewer_->characterForLocalPoint(p, TextLayout::TRAILING);
	} else
		destination = *to;

	const k::Document& document = viewer_->document();
	Caret& caret = viewer_->caret();
	if(state_ == EXTENDING_CHARACTER_SELECTION)
		caret.extendSelection(destination);
	else if(state_ == EXTENDING_LINE_SELECTION) {
		const length_t lines = document.numberOfLines();
		k::Region s;
		s.first.line = (destination.line >= selection_.initialLine) ? selection_.initialLine : selection_.initialLine + 1;
		s.first.column = (s.first.line > lines - 1) ? document.lineLength(--s.first.line) : 0;
		s.second.line = (destination.line >= selection_.initialLine) ? destination.line + 1 : destination.line;
		s.second.column = (s.second.line > lines - 1) ? document.lineLength(--s.second.line) : 0;
		caret.select(s);
	} else if(state_ == EXTENDING_WORD_SELECTION) {
		using namespace text;
		const IdentifierSyntax& id = document.contentTypeInformation().getIdentifierSyntax(caret.contentType());
		if(destination.line < selection_.initialLine
				|| (destination.line == selection_.initialLine
					&& destination.column < selection_.initialWordColumns.first)) {
			WordBreakIterator<k::DocumentCharacterIterator> i(
				k::DocumentCharacterIterator(document, destination), text::AbstractWordBreakIterator::BOUNDARY_OF_SEGMENT, id);
			--i;
			caret.select(k::Position(selection_.initialLine, selection_.initialWordColumns.second),
				(i.base().tell().line == destination.line) ? i.base().tell() : k::Position(destination.line, 0));
		} else if(destination.line > selection_.initialLine
				|| (destination.line == selection_.initialLine
					&& destination.column > selection_.initialWordColumns.second)) {
			text::WordBreakIterator<k::DocumentCharacterIterator> i(
				k::DocumentCharacterIterator(document, destination), text::AbstractWordBreakIterator::BOUNDARY_OF_SEGMENT, id);
			++i;
			caret.select(k::Position(selection_.initialLine, selection_.initialWordColumns.first),
				(i.base().tell().line == destination.line) ?
					i.base().tell() : k::Position(destination.line, document.lineLength(destination.line)));
		} else
			caret.select(k::Position(selection_.initialLine, selection_.initialWordColumns.first),
				k::Position(selection_.initialLine, selection_.initialWordColumns.second));
	}
}

/**
 * Handles double click action of the left button.
 * @param position same as @c IMouseInputStrategy#mouseButtonInput
 * @param keyState same as @c IMouseInputStrategy#mouseButtonInput
 * @return true if processed the input. in this case, the original behavior of
 * @c DefaultMouseInputStrategy is suppressed. the default implementation returns false
 */
bool DefaultMouseInputStrategy::handleLeftButtonDoubleClick(const NativePoint& position, int modifiers) {
	return false;
}

/// Handles @c WM_LBUTTONDOWN.
void DefaultMouseInputStrategy::handleLeftButtonPressed(const NativePoint& position, int modifiers) {
	bool boxDragging = false;
	Caret& caret = viewer_->caret();
	const TextViewer::HitTestResult htr = viewer_->hitTest(position);

	utils::closeCompletionProposalsPopup(*viewer_);
	texteditor::endIncrementalSearch(*viewer_);

	// select line(s)
	if(htr == TextViewer::INDICATOR_MARGIN || htr == TextViewer::LINE_NUMBERS) {
		const k::Position to(viewer_->characterForLocalPoint(position, TextLayout::LEADING));
		const bool extend = win32::boole(modifiers & MK_SHIFT) && to.line != caret.anchor().line();
		state_ = EXTENDING_LINE_SELECTION;
		selection_.initialLine = extend ? caret.anchor().line() : to.line;
		viewer_->caret().endRectangleSelection();
		extendSelection(&to);
		viewer_->grabInput();
		timer_.start(SELECTION_EXPANSION_INTERVAL, *this);
	}

	// approach drag-and-drop
	else if(dnd_.supportLevel >= SUPPORT_DND && !isSelectionEmpty(caret) && isPointOverSelection(caret, position)) {
		state_ = APPROACHING_DND;
		dragApproachedPosition_ = position;
		if(caret.isSelectionRectangle())
			boxDragging = true;
	}

	else {
		// try hyperlink
		bool hyperlinkInvoked = false;
		if(win32::boole(modifiers & MK_CONTROL)) {
			if(!isPointOverSelection(caret, position)) {
				const k::Position p(viewer_->characterForLocalPoint(position, TextLayout::TRAILING, true));
				if(p != k::Position()) {
					if(const hyperlink::Hyperlink* link = utils::getPointedHyperlink(*viewer_, p)) {
						link->invoke();
						hyperlinkInvoked = true;
					}
				}
			}
		}

		if(!hyperlinkInvoked) {
			// modification keys and result
			//
			// shift => keep the anchor and move the caret to the cursor position
			// ctrl  => begin word selection
			// alt   => begin rectangle selection
			const k::Position to(viewer_->characterForLocalPoint(position, TextLayout::TRAILING));
			state_ = EXTENDING_CHARACTER_SELECTION;
			if((modifiers & (UserInput::CONTROL_DOWN | UserInput::SHIFT_DOWN)) != 0) {
				if((modifiers & UserInput::CONTROL_DOWN) != 0) {
					// begin word selection
					state_ = EXTENDING_WORD_SELECTION;
					caret.moveTo((modifiers & UserInput::SHIFT_DOWN) != 0 ? caret.anchor() : to);
					selectWord(caret);
					selection_.initialLine = caret.line();
					selection_.initialWordColumns = make_pair(caret.beginning().column(), caret.end().column());
				}
				if((modifiers & UserInput::SHIFT_DOWN) != 0)
					extendSelection(&to);
			} else
				caret.moveTo(to);
			if((modifiers & UserInput::ALT_DOWN) != 0)	// make the selection reactangle
				caret.beginRectangleSelection();
			else
				caret.endRectangleSelection();
			viewer_->grabInput();
			timer_.start(SELECTION_EXPANSION_INTERVAL, *this);
		}
	}

//	if(!caret.isSelectionRectangle() && !boxDragging)
//		viewer_->redrawLine(caret.line());
	viewer_->setFocus();
}

/// Handles @c WM_LBUTTONUP.
void DefaultMouseInputStrategy::handleLeftButtonReleased(const NativePoint& position, int) {
	// cancel if drag-and-drop approaching
	if(dnd_.supportLevel >= SUPPORT_DND
			&& (state_ == APPROACHING_DND
			|| state_ == DND_SOURCE)) {	// TODO: this should handle only case APPROACHING_DND?
		state_ = NONE;
		viewer_->caret().moveTo(viewer_->characterForLocalPoint(position, TextLayout::TRAILING));
		::SetCursor(::LoadCursor(0, IDC_IBEAM));	// hmm...
	}

	timer_.stop();
	if((state_ & SELECTION_EXTENDING_MASK) == SELECTION_EXTENDING_MASK) {
		state_ = NONE;
		// if released the button when extending the selection, the scroll may not reach the caret position
		utils::show(viewer_->caret());
	}
	viewer_->releaseInput();
}

/**
 * Handles the right button.
 * @param action Same as @c MouseInputStrategy#mouseButtonInput
 * @param position Same as @c MouseInputStrategy#mouseButtonInput
 * @param modifiers Same as @c MouseInputStrategy#mouseButtonInput
 * @return Same as @c MouseInputStrategy#mouseButtonInput. The default implementation returns @c false
 */
bool DefaultMouseInputStrategy::handleRightButton(Action action, const NativePoint& position, int modifiers) {
	return false;
}

/**
 * Handles the first X1 button.
 * @param action Same as @c MouseInputStrategy#mouseButtonInput
 * @param position Same as @c MouseInputStrategy#mouseButtonInput
 * @param modifiers Same as @c MouseInputStrategy#mouseButtonInput
 * @return Same as @c MouseInputStrategy#mouseButtonInput. The default implementation returns @c false
 */
bool DefaultMouseInputStrategy::handleX1Button(Action action, const NativePoint& position, int modifiers) {
	return false;
}

/**
 * Handles the first X2 button.
 * @param action Same as @c MouseInputStrategy#mouseButtonInput
 * @param position Same as @c MouseInputStrategy#mouseButtonInput
 * @param modifiers Same as @c MouseInputStrategy#mouseButtonInput
 * @return Same as @c MouseInputStrategy#mouseButtonInput. The default implementation returns @c false
 */
bool DefaultMouseInputStrategy::handleX2Button(Action action, const NativePoint& position, int modifiers) {
	return false;
}

/// @see MouseInputStrategy#install
void DefaultMouseInputStrategy::install(TextViewer& viewer) {
	if(viewer_ != 0)
		uninstall();
#ifdef ASCENSION_WINDOW_SYSTEM_WIN32
	::RegisterDragDrop((viewer_ = &viewer)->identifier().get(), this);
#endif // ASCENSION_WINDOW_SYSTEM_WIN32
	state_ = NONE;

	// create the window for the auto scroll origin mark
	autoScrollOriginMark_.reset(new AutoScrollOriginMark(*viewer_));
}

/// @see MouseInputStrategy#interruptMouseReaction
void DefaultMouseInputStrategy::interruptMouseReaction(bool forKeyboardInput) {
	if(state_ == AUTO_SCROLL_DRAGGING || state_ == AUTO_SCROLL)
		endAutoScroll();
}

/// @see MouseInputStrategy#mouseButtonInput
bool DefaultMouseInputStrategy::mouseButtonInput(Action action, const base::MouseButtonInput& input) {
	if(action != RELEASED && endAutoScroll())
		return true;
	switch(input.button()) {
	case UserInput::BUTTON1_DOWN:
		if(action == PRESSED)
			handleLeftButtonPressed(input.location(), input.modifiers());
		else if(action == RELEASED)
			handleLeftButtonReleased(input.location(), input.modifiers());
		else if(action == DOUBLE_CLICKED) {
			texteditor::abortIncrementalSearch(*viewer_);
			if(handleLeftButtonDoubleClick(input.location(), input.modifiers()))
				return true;
			const TextViewer::HitTestResult htr = viewer_->hitTest(viewer_->mapFromGlobal(Cursor::position()));
			if(htr == TextViewer::SIDE_SPACE || htr == TextViewer::CONTENT_AREA) {
				// begin word selection
				Caret& caret = viewer_->caret();
				selectWord(caret);
				state_ = EXTENDING_WORD_SELECTION;
				selection_.initialLine = caret.line();
				selection_.initialWordColumns = make_pair(caret.anchor().column(), caret.column());
				viewer_->grabInput();
				timer_.start(SELECTION_EXPANSION_INTERVAL, *this);
				return true;
			}
		}
		break;
	case UserInput::BUTTON2_DOWN:
		if(action == PRESSED) {
			if(viewer_->document().numberOfLines() > viewer_->numberOfVisibleLines()) {
				state_ = APPROACHING_AUTO_SCROLL;
				dragApproachedPosition_ = input.location();
				const NativePoint p(viewer_->mapToGlobal(input.location()));
				viewer_->setFocus();
				// show the indicator margin
				NativeRectangle rect(autoScrollOriginMark_->bounds(true));
				autoScrollOriginMark_->move(
					geometry::make<NativePoint>(geometry::x(p) - geometry::dx(rect) / 2, geometry::y(p) - geometry::dy(rect) / 2));
				autoScrollOriginMark_->show();
				autoScrollOriginMark_->raise();
				viewer_->grabInput();
				showCursor(input.location());
				return true;
			}
		} else if(action == RELEASED) {
			if(state_ == APPROACHING_AUTO_SCROLL) {
				state_ = AUTO_SCROLL;
				timer_.start(0, *this);
			} else if(state_ == AUTO_SCROLL_DRAGGING)
				endAutoScroll();
		}
		break;
	case UserInput::BUTTON3_DOWN:
		return handleRightButton(action, input.location(), input.modifiers());
	case UserInput::BUTTON4_DOWN:
		return handleX1Button(action, input.location(), input.modifiers());
	case UserInput::BUTTON5_DOWN:
		return handleX2Button(action, input.location(), input.modifiers());
	}
	return false;
}

/// @see MouseInputStrategy#mouseMoved
void DefaultMouseInputStrategy::mouseMoved(const base::LocatedUserInput& input) {
	if(state_ == APPROACHING_AUTO_SCROLL
			|| (dnd_.supportLevel >= SUPPORT_DND && state_ == APPROACHING_DND)) {	// dragging starts?
		if(state_ == APPROACHING_DND && isSelectionEmpty(viewer_->caret()))
			state_ = NONE;	// approaching... => cancel
		else {
			// the following code can be replaced with DragDetect in user32.lib
			const int cxDragBox = ::GetSystemMetrics(SM_CXDRAG);
			const int cyDragBox = ::GetSystemMetrics(SM_CYDRAG);
			if((geometry::x(input.location()) > geometry::x(dragApproachedPosition_) + cxDragBox / 2)
					|| (geometry::x(input.location()) < geometry::x(dragApproachedPosition_) - cxDragBox / 2)
					|| (geometry::y(input.location()) > geometry::y(dragApproachedPosition_) + cyDragBox / 2)
					|| (geometry::y(input.location()) < geometry::y(dragApproachedPosition_) - cyDragBox / 2)) {
				if(state_ == APPROACHING_DND)
					doDragAndDrop();
				else {
					state_ = AUTO_SCROLL_DRAGGING;
					timer_.start(0, *this);
				}
			}
		}
	} else if((state_ & SELECTION_EXTENDING_MASK) == SELECTION_EXTENDING_MASK)
		extendSelection();
}

/// @see MouseInputStrategy#mouseWheelRotated
void DefaultMouseInputStrategy::mouseWheelRotated(const base::MouseWheelInput& input) {
	if(!endAutoScroll()) {
		// use system settings
		UINT lines;	// the number of lines to scroll
		if(!win32::boole(::SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &lines, 0)))
			lines = 3;
		if(lines == WHEEL_PAGESCROLL)
			lines = static_cast<UINT>(viewer_->numberOfVisibleLines());
		viewer_->scroll(0, -geometry::dy(input.rotation()) * static_cast<short>(lines) / WHEEL_DELTA, true);
	}
}

/// @see MouseInputStrategy#showCursor
bool DefaultMouseInputStrategy::showCursor(const NativePoint& position) {
	using namespace hyperlink;
	LPCTSTR cursorName = 0;
	const Hyperlink* newlyHoveredHyperlink = 0;

	// on the vertical ruler?
	const TextViewer::HitTestResult htr = viewer_->hitTest(position);
	if(htr == TextViewer::INDICATOR_MARGIN || htr == TextViewer::LINE_NUMBERS)
		cursorName = IDC_ARROW;
	// on a draggable text selection?
	else if(dnd_.supportLevel >= SUPPORT_DND && !isSelectionEmpty(viewer_->caret()) && isPointOverSelection(viewer_->caret(), position))
		cursorName = IDC_ARROW;
	else if(htr == TextViewer::CONTENT_AREA) {
		// on a hyperlink?
		const k::Position p(viewer_->characterForLocalPoint(position, TextLayout::TRAILING, true, k::locations::UTF16_CODE_UNIT));
		if(p != k::Position())
			newlyHoveredHyperlink = utils::getPointedHyperlink(*viewer_, p);
		if(newlyHoveredHyperlink != 0 && win32::boole(::GetAsyncKeyState(VK_CONTROL) & 0x8000))
			cursorName = IDC_HAND;
	}

	if(cursorName != 0) {
		::SetCursor(static_cast<HCURSOR>(::LoadImage(
			0, cursorName, IMAGE_CURSOR, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE | LR_SHARED)));
		return true;
	}
	if(newlyHoveredHyperlink != 0) {
		if(newlyHoveredHyperlink != lastHoveredHyperlink_)
			viewer_->showToolTip(newlyHoveredHyperlink->description(), 1000, 30000);
	} else
		viewer_->hideToolTip();
	lastHoveredHyperlink_ = newlyHoveredHyperlink;
	return false;
}

///
void DefaultMouseInputStrategy::timeElapsed(Timer& timer) {
	if((state_ & SELECTION_EXTENDING_MASK) == SELECTION_EXTENDING_MASK) {	// scroll automatically during extending the selection
		const NativePoint p(viewer_->mapFromGlobal(Cursor::position()));
		const NativeRectangle rc(viewer_->bounds(false));
		const PhysicalFourSides<Scalar> spaces(viewer_->spaceWidths());
		// 以下のスクロール量には根拠は無い
		if(geometry::y(p) < geometry::top(rc) + spaces.top)
			viewer_->scroll(0, (geometry::y(p) - (geometry::top(rc) + spaces.top)) / viewer_->textRenderer().defaultFont()->metrics().linePitch() - 1, true);
		else if(geometry::y(p) >= geometry::bottom(rc) - spaces.bottom)
			viewer_->scroll(0, (geometry::y(p) - (geometry::bottom(rc) - spaces.bottom)) / viewer_->textRenderer().defaultFont()->metrics().linePitch() + 1, true);
		else if(geometry::x(p) < geometry::left(rc) + spaces.left)
			viewer_->scroll((geometry::x(p) - (geometry::left(rc) + spaces.left)) / viewer_->textRenderer().defaultFont()->metrics().averageCharacterWidth() - 1, 0, true);
		else if(geometry::x(p) >= geometry::right(rc) - spaces.right)
			viewer_->scroll((geometry::x(p) - (geometry::right(rc) - spaces.right)) / viewer_->textRenderer().defaultFont()->metrics().averageCharacterWidth() + 1, 0, true);
		extendSelection();
	} else if(state_ == AUTO_SCROLL_DRAGGING || state_ == AUTO_SCROLL) {
		timer.stop();
		const NativePoint p(viewer_->mapFromGlobal(Cursor::position()));
		const Scalar yScrollDegree = (geometry::y(p) - geometry::y(dragApproachedPosition_)) / viewer_->textRenderer().defaultFont()->metrics().linePitch();
//		const Scalar xScrollDegree = (geometry::x(p) - geometry::x(dragApproachedPosition_)) / viewer_->presentation().textMetrics().lineHeight();
//		const Scalar scrollDegree = max(abs(yScrollDegree), abs(xScrollDegree));

		if(yScrollDegree != 0 /*&& abs(yScrollDegree) >= abs(xScrollDegree)*/)
			viewer_->scroll(0, yScrollDegree > 0 ? +1 : -1, true);
//		else if(xScrollDegree != 0)
//			viewer.scroll(xScrollDegree > 0 ? +1 : -1, 0, true);

		if(yScrollDegree != 0) {
			timer_.start(500 / static_cast<unsigned int>((pow(2.0f, abs(yScrollDegree) / 2))), *this);
			::SetCursor(AutoScrollOriginMark::cursorForScrolling(
				(yScrollDegree > 0) ? AutoScrollOriginMark::CURSOR_DOWNWARD : AutoScrollOriginMark::CURSOR_UPWARD).get());
		} else {
			timer_.start(300, *this);
			::SetCursor(AutoScrollOriginMark::cursorForScrolling(AutoScrollOriginMark::CURSOR_NEUTRAL).get());
		}
#if 0
	} else if(self.dnd_.enabled && (self.state_ & DND_MASK) == DND_MASK) {	// scroll automatically during dragging
		const SIZE scrollOffset = calculateDnDScrollOffset(*self.viewer_);
		if(scrollOffset.cy != 0)
			self.viewer_->scroll(0, scrollOffset.cy, true);
		else if(scrollOffset.cx != 0)
			self.viewer_->scroll(scrollOffset.cx, 0, true);
#endif
	}
}

/// @see MouseInputStrategy#uninstall
void DefaultMouseInputStrategy::uninstall() {
	timer_.stop();
	if(autoScrollOriginMark_.get() != 0)
		autoScrollOriginMark_.reset();
#ifdef ASCENSION_WINDOW_SYSTEM_WIN32
	::RevokeDragDrop(viewer_->identifier().get());
#endif // ASCENSION_WINDOW_SYSTEM_WIN32
	viewer_ = 0;
}
