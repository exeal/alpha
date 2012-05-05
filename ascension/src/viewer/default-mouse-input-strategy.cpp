/**
 * @file default-mouse-input-strategy.cpp
 * @author exeal
 * @date 2003-2006 was EditView.cpp and EditViewWindowMessages.cpp
 * @date 2006-2011 was viewer.cpp
 * @date 2011-10-04 separated from viewer.cpp
 * @date 2011-2012
 */

#include <ascension/corelib/text/break-iterator.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/kernel/document-character-iterator.hpp>
#include <ascension/text-editor/session.hpp>	// texteditor.xxxIncrementalSearch
#include <ascension/viewer/widgetapi/cursor.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/default-mouse-input-strategy.hpp>
#include <ascension/viewer/viewer.hpp>
#include <limits>	// std.numeric_limit

using namespace ascension;
using namespace ascension::viewers;
using namespace ascension::presentation;
using namespace ascension::graphics;
using namespace ascension::graphics::font;
using namespace std;
namespace k = ascension::kernel;


// DefaultMouseInputStrategy //////////////////////////////////////////////////////////////////////

namespace {
	typedef
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
		Gtk::Widget
#elif defined(ASCENSION_WINDOW_SYSTEM_QT)
		QWidget
#elif defined(ASCENSION_WINDOW_SYSTEM_QUARTZ)
		NSView
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
		win32::CustomControl
#endif
		AutoScrollOriginMarkBase;

	/// Circled window displayed at which the auto scroll started.
	class AutoScrollOriginMark : public AutoScrollOriginMarkBase
	{
		ASCENSION_NONCOPYABLE_TAG(AutoScrollOriginMark);
	public:
		/// Defines the type of the cursors obtained by @c #cursorForScrolling method.
		enum CursorType {
			CURSOR_NEUTRAL,	///< Indicates no scrolling.
			CURSOR_UPWARD,	///< Indicates scrolling upward.
			CURSOR_DOWNWARD	///< Indicates scrolling downward.
		};
	public:
		explicit AutoScrollOriginMark(TextViewer& viewer) /*throw()*/;
		void initialize(const TextViewer& viewer);
		static const widgetapi::Cursor& cursorForScrolling(CursorType type);
	private:
		void paint(graphics::PaintContext& context);
#if defined(ASCENSION_WINDOW_SYSTEM_WIN32)
		void provideClassInformation(win32::CustomControl::ClassInformation& classInformation) const {
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
 * @param viewer The text viewer. The widget becomes the child of this viewer
 */
AutoScrollOriginMark::AutoScrollOriginMark(TextViewer& viewer) /*throw()*/ {
	// TODO: Set transparency on window system other than Win32.
#if defined(ASCENSION_WINDOW_SYSTEM_WIN32)
	// calling CreateWindowExW with WS_EX_LAYERED will fail on NT 4.0
	::SetWindowLongW(handle().get(), GWL_EXSTYLE,
		::GetWindowLongW(handle().get(), GWL_EXSTYLE) | WS_EX_LAYERED);
#endif // ASCENSION_WINDOW_SYSTEM_WIN32

	widgetapi::resize(*this, geometry::make<NativeSize>(WINDOW_WIDTH + 1, WINDOW_WIDTH + 1));
	NativeRegion rgn(::CreateEllipticRgn(0, 0, WINDOW_WIDTH + 1, WINDOW_WIDTH + 1), &::DeleteObject);
	widgetapi::setShape(*this, rgn);
#if defined(ASCENSION_WINDOW_SYSTEM_WIN32)
	::SetLayeredWindowAttributes(handle().get(), ::GetSysColor(COLOR_WINDOW), 0, LWA_COLORKEY);
#endif // ASCENSION_WINDOW_SYSTEM_WIN32
	widgetapi::setParent(viewer);
}

/**
 * Returns the cursor should be shown when the auto-scroll is active.
 * @param type The type of the cursor to obtain
 * @return The cursor. Do not destroy the returned value
 * @throw UnknownValueException @a type is unknown
 */
const widgetapi::Cursor& AutoScrollOriginMark::cursorForScrolling(CursorType type) {
	static unique_ptr<widgetapi::Cursor> instances[3];
	if(type >= ASCENSION_COUNTOF(instances))
		throw UnknownValueException("type");
	if(instances[type].get() == nullptr) {
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
		instances[type].reset(new widgetapi::Cursor(win32::Handle<HCURSOR>(
			::CreateCursor(::GetModuleHandleW(nullptr), 16, 16, 32, 32, andBits, xorBits), &::DestroyCursor)));
#else
		instances[type].reset(new widgetapi::Cursor(bitmap));
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

/// Default constructor.
DefaultMouseInputStrategy::DefaultMouseInputStrategy() : viewer_(nullptr), state_(NONE), lastHoveredHyperlink_(nullptr) {
}

void DefaultMouseInputStrategy::beginDragAndDrop() {
	const Caret& caret = viewer_->caret();
	HRESULT hr;

	win32::com::SmartPointer<IDataObject> draggingContent(
		utils::createMimeDataForSelectedString(viewer_->caret(), true));
	if(!caret.isSelectionRectangle())
		dnd_.numberOfRectangleLines = 0;
	else {
		const k::Region selection(caret.selectedRegion());
		dnd_.numberOfRectangleLines = selection.end().line - selection.beginning().line + 1;
	}

	// setup drag-image
	if(dnd_.dragSourceHelper.get() != nullptr) {
		boost::optional<SHDRAGIMAGE> image(createSelectionImage(*viewer_, dragApproachedPosition_, true));
		if(image != boost::none && FAILED(hr = dnd_.dragSourceHelper->InitializeFromBitmap(&image.get(), draggingContent.get())))
			::DeleteObject(image->hbmpDragImage);
	}

	// operation
	state_ = DND_SOURCE;
	DWORD possibleEffects = DROPEFFECT_COPY | DROPEFFECT_SCROLL, resultEffect;
	if(!viewer_->document().isReadOnly())
		possibleEffects |= DROPEFFECT_MOVE;
	hr = ::DoDragDrop(draggingContent.get(), this, possibleEffects, &resultEffect);
	state_ = NONE;
	if(widgetapi::isVisible(*viewer_))
		widgetapi::setFocus(*viewer_);
}

/// @see MouseInputStrategy#captureChanged
void DefaultMouseInputStrategy::captureChanged() {
	timer_.stop();
	state_ = NONE;
}

namespace {
	bool isMimeDataAcceptable(const widgetapi::NativeMimeData& data, bool onlyRectangle) {
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
		return (data.get_target() == ASCENSION_RECTANGLE_TEXT_MIME_FORMAT) || (!onlyRectangle && data.targets_include_text());
#elif defined(ASCENSION_WINDOW_SYSTEM_QT)
		return data.hasFormat(ASCENSION_RECTANGLE_TEXT_MIME_FORMAT) || (!onlyRectangle && data.hasText());
#elif defined(ASCENSION_WINDOW_SYSTEM_QUARTZ)
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
		const array<CLIPFORMAT, 3> formats = {
			static_cast<CLIPFORMAT>(::RegisterClipboardFormatW(ASCENSION_RECTANGLE_TEXT_MIME_FORMAT)), CF_UNICODETEXT, CF_TEXT
		};
		FORMATETC format = {0, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
		for(size_t i = 0; i < (onlyRectangle ? 1 : formats.size()); ++i) {
			if((format.cfFormat = formats[i]) != 0) {
				const HRESULT hr = const_cast<widgetapi::NativeMimeData&>(data).QueryGetData(&format);
				if(hr == S_OK)
					return true;
			}
		}
		return false;
#endif
	}
}

/// @see DropTarget#dragEntered
void DefaultMouseInputStrategy::dragEntered(widgetapi::DragEnterInput& input) {
	input.setDropAction(widgetapi::DROP_ACTION_IGNORE);
	if(/*dnd_.supportLevel == DONT_SUPPORT_DND ||*/ viewer_->document().isReadOnly() || !viewer_->allowsMouseInput())
		return input.ignore();

	// validate the dragged data if can drop
	if(!isMimeDataAcceptable(input.mimeData(), false))
		return input.ignore();

	if(state_ != DND_SOURCE) {
		assert(state_ == NONE);
		// retrieve number of lines if text is rectangle
		dnd_.numberOfRectangleLines = 0;
		if(isMimeDataAcceptable(input.mimeData(), true)) {
			const TextAnchor anchor = defaultTextAnchor(viewer_->presentation());
			const ReadingDirection readingDirection = defaultReadingDirection(viewer_->presentation());
			if((anchor == TEXT_ANCHOR_START && readingDirection == RIGHT_TO_LEFT)
					|| (anchor == TEXT_ANCHOR_END && readingDirection == LEFT_TO_RIGHT))
				return input.ignore();	// TODO: support alignments other than ALIGN_LEFT.
			pair<HRESULT, String> text(utils::getTextFromMimeData(input.mimeData()));
			if(SUCCEEDED(text.first))
				dnd_.numberOfRectangleLines = text::calculateNumberOfLines(text.second) - 1;
		}
		state_ = DND_TARGET;
	}

	widgetapi::setFocus(*viewer_);
	timer_.start(DRAGGING_TRACK_INTERVAL, *this);
	return dragMoved(input);
}

/// @see DropTarget#dragLeft
void DefaultMouseInputStrategy::dragLeft(widgetapi::DragLeaveInput& input) {
	widgetapi::setFocus(nullptr);
	timer_.stop();
//	if(dnd_.supportLevel >= SUPPORT_DND) {
		if(state_ == DND_TARGET)
			state_ = NONE;
//	}
	input.consume();
}

namespace {
	PhysicalTwoAxes<TextViewport::SignedScrollOffset> calculateDnDScrollOffset(const TextViewer& viewer) {
		const NativePoint p(widgetapi::mapFromGlobal(viewer, widgetapi::Cursor::position()));
		const NativeRectangle localBounds(widgetapi::bounds(viewer, false));
		NativeRectangle inset(viewer.textAreaContentRectangle());
		const Font::Metrics& fontMetrics = viewer.textRenderer().defaultFont()->metrics();
		geometry::range<geometry::X_COORDINATE>(inset) = makeRange(
			geometry::left(inset) + fontMetrics.averageCharacterWidth(), geometry::right(inset) - fontMetrics.averageCharacterWidth());
		geometry::range<geometry::Y_COORDINATE>(inset) = makeRange(
			geometry::top(inset) + fontMetrics.linePitch() / 2, geometry::bottom(inset) - fontMetrics.linePitch() / 2);

		// On Win32, oleidl.h defines the value named DD_DEFSCROLLINSET, but...

		geometry::Coordinate<NativeSize>::Type dx = 0, dy = 0;
		if(includes(makeRange(geometry::top(localBounds), geometry::top(inset)), geometry::y(p)))
			dy = -1;
		else if(includes(makeRange(geometry::bottom(localBounds), geometry::bottom(inset)), geometry::y(p)))
			dy = +1;
		if(includes(makeRange(geometry::left(localBounds), geometry::left(inset)), geometry::x(p)))
			dx = -3;
		else if(includes(makeRange(geometry::right(localBounds), geometry::right(inset)), geometry::y(p)))
			dx = +3;
		return PhysicalTwoAxes<TextViewport::SignedScrollOffset>(dx, dy);
	}
}

/// @see DropTarget#dragMoved
void DefaultMouseInputStrategy::dragMoved(widgetapi::DragMoveInput& input) {
	widgetapi::DropAction dropAction = widgetapi::DROP_ACTION_IGNORE;
	bool acceptable = false;

	if((state_ == DND_SOURCE || state_ == DND_TARGET)
			&& !viewer_->document().isReadOnly() && viewer_->allowsMouseInput()) {
		const NativePoint caretPoint(widgetapi::mapFromGlobal(*viewer_, input.location()));
		const k::Position p(viewToModel(*viewer_->textRenderer().viewport(), caretPoint, TextLayout::TRAILING));
//		viewer_->setCaretPosition(viewer_->localPointForCharacter(p, true, TextLayout::LEADING));

		// drop rectangle text into bidirectional line is not supported...
		if(dnd_.numberOfRectangleLines == 0)
			acceptable = true;
		else {
			const Index lines = min(viewer_->document().numberOfLines(), p.line + dnd_.numberOfRectangleLines);
			bool bidirectional = false;
			for(Index line = p.line; line < lines; ++line) {
				if(viewer_->textRenderer().layouts()[line].isBidirectional()) {
					bidirectional = true;
					break;
				}
			}
			acceptable = !bidirectional;
		}
	}

	if(acceptable) {
		dropAction = widgetapi::hasModifier<widgetapi::UserInput::CONTROL_DOWN>(input) ? widgetapi::DROP_ACTION_COPY : widgetapi::DROP_ACTION_MOVE;
		const PhysicalTwoAxes<TextViewport::SignedScrollOffset> scrollOffset(calculateDnDScrollOffset(*viewer_));
		if(scrollOffset.x() != 0 || scrollOffset.y() != 0) {
#ifdef ASCENSION_WINDOW_SYSTEM_WIN32
			dropAction |= widgetapi::DROP_ACTION_WIN32_SCROLL;
#endif // ASCENSION_WINDOW_SYSTEM_WIN32
			// only one direction to scroll
			if(scrollOffset.x() != 0)
				viewer_->textRenderer().viewport()->scroll(PhysicalTwoAxes<TextViewport::SignedScrollOffset>(0, scrollOffset.y()));
			else
				viewer_->textRenderer().viewport()->scroll(PhysicalTwoAxes<TextViewport::SignedScrollOffset>(scrollOffset.x(), 0));
		}
	}
	input.setDropAction(dropAction);
	input.consume();
}

/// @see DropTarget#drop
void DefaultMouseInputStrategy::dropped(widgetapi::DropInput& input) {
	k::Document& document = viewer_->document();
	input.setDropAction(widgetapi::DROP_ACTION_IGNORE);
	if(/*dnd_.supportLevel == DONT_SUPPORT_DND ||*/ document.isReadOnly() || !viewer_->allowsMouseInput())
		return input.ignore();
	Caret& caret = viewer_->caret();
	const NativePoint caretPoint(input.location());
	const k::Position destination(viewToModel(*viewer_->textRenderer().viewport(), caretPoint, TextLayout::TRAILING));

	if(!document.accessibleRegion().includes(destination))
		return input.ignore();

	if(state_ == DND_TARGET) {	// dropped from the other widget
		timer_.stop();
		if((input.possibleActions() & widgetapi::DROP_ACTION_COPY) != 0) {
			caret.moveTo(destination);

			pair<String, bool> content(utils::getTextFromMimeData(input.mimeData()));
			if(SUCCEEDED(content.first)) {
				AutoFreeze af(viewer_);
				bool failed = false;
				try {
					caret.replaceSelection(content.first, content.second);
				} catch(...) {
					failed = true;
				}
				if(!failed) {
					if(content.second)
						caret.beginRectangleSelection();
					caret.select(destination, caret);
					input.setDropAction(widgetapi::DROP_ACTION_COPY);
				}
			}
		}
		state_ = NONE;
	} else {	// drop from the same widget
		assert(state_ == DND_SOURCE);
		String text(selectedString(caret, text::NLF_RAW_VALUE));

		// can't drop into the selection
		if(isPointOverSelection(caret, caretPoint)) {
			caret.moveTo(destination);
			state_ = NONE;
		} else {
			const bool rectangle = caret.isSelectionRectangle();
			bool failed = false;
			if(widgetapi::hasModifier<widgetapi::UserInput::CONTROL_DOWN>(input)) {	// copy
				if((input.possibleActions() & widgetapi::DROP_ACTION_COPY) != 0) {
					document.insertUndoBoundary();
					AutoFreeze af(viewer_);
//					viewer_->redrawLines(ca.beginning().line(), ca.end().line());
					caret.enableAutoShow(false);
					caret.moveTo(destination);
					try {
						caret.replaceSelection(text, rectangle);
					} catch(...) {
						failed = true;
					}
					caret.enableAutoShow(true);
					if(!failed) {
						caret.select(destination, caret);
						input.setDropAction(widgetapi::DROP_ACTION_COPY);
					}
					document.insertUndoBoundary();
				}
			} else {	// move as a rectangle or linear
				if((input.possibleActions() & widgetapi::DROP_ACTION_MOVE) != 0) {
					document.insertUndoBoundary();
					AutoFreeze af(viewer_);
					pair<k::Point, k::Point> oldSelection(make_pair(k::Point(caret.anchor()), k::Point(caret)));
					caret.enableAutoShow(false);
					caret.moveTo(destination);
					try {
						caret.replaceSelection(text, rectangle);
					} catch(...) {
						failed = true;
					}
					if(!failed) {
						caret.select(destination, caret);
						if(rectangle)
							caret.beginRectangleSelection();
						try {
							erase(caret.document(), oldSelection.first, oldSelection.second);
						} catch(...) {
							failed = true;
						}
					}
					caret.enableAutoShow(true);
					if(!failed)
						input.setDropAction(widgetapi::DROP_ACTION_MOVE);
					document.insertUndoBoundary();
				}
			}
		}
	}
}

/**
 * Ends the auto scroll.
 * @return true if the auto scroll was active
 */
bool DefaultMouseInputStrategy::endAutoScroll() {
	if(state_ == AUTO_SCROLL_DRAGGING || state_ == AUTO_SCROLL) {
		timer_.stop();
		state_ = NONE;
		widgetapi::hide(*autoScrollOriginMark_);
		widgetapi::releaseInput(*viewer_);
		return true;
	}
	return false;
}

/// Extends the selection to the current cursor position.
void DefaultMouseInputStrategy::extendSelectionTo(const k::Position* to /* = nullptr */) {
	if((state_ & SELECTION_EXTENDING_MASK) != SELECTION_EXTENDING_MASK)
		throw IllegalStateException("not extending the selection.");
	k::Position destination;
	if(to == nullptr) {
		NativePoint p(widgetapi::mapFromGlobal(*viewer_, widgetapi::Cursor::position()));
		Caret& caret = viewer_->caret();
		if(state_ != EXTENDING_CHARACTER_SELECTION) {
			const TextViewer::HitTestResult htr = viewer_->hitTest(p);
			if(state_ == EXTENDING_LINE_SELECTION && (htr & TextViewer::RULER_MASK) == 0)
				// end line selection
				state_ = EXTENDING_CHARACTER_SELECTION;
		}
		// snap cursor position into 'content-rectangle' of the text area
		const NativeRectangle contentRectangle(viewer_->textAreaContentRectangle());
		p = geometry::make<NativePoint>(
				min<Scalar>(
					max<Scalar>(geometry::x(p), geometry::left(contentRectangle)),
					geometry::right(contentRectangle)),
				min<Scalar>(
					max<Scalar>(geometry::y(p), geometry::top(contentRectangle)),
					geometry::bottom(contentRectangle)));
		destination = viewToModel(*viewer_->textRenderer().viewport(), p, TextLayout::TRAILING);
	} else
		destination = *to;

	const k::Document& document = viewer_->document();
	Caret& caret = viewer_->caret();
	if(state_ == EXTENDING_CHARACTER_SELECTION)
		caret.extendSelectionTo(destination);
	else if(state_ == EXTENDING_LINE_SELECTION) {
		const Index lines = document.numberOfLines();
		k::Region s;
		s.first.line = (destination.line >= selection_.initialLine) ? selection_.initialLine : selection_.initialLine + 1;
		s.first.offsetInLine = (s.first.line > lines - 1) ? document.lineLength(--s.first.line) : 0;
		s.second.line = (destination.line >= selection_.initialLine) ? destination.line + 1 : destination.line;
		s.second.offsetInLine = (s.second.line > lines - 1) ? document.lineLength(--s.second.line) : 0;
		caret.select(s);
	} else if(state_ == EXTENDING_WORD_SELECTION) {
		using namespace text;
		const IdentifierSyntax& id = document.contentTypeInformation().getIdentifierSyntax(contentType(caret));
		if(destination.line < selection_.initialLine
				|| (destination.line == selection_.initialLine
					&& destination.offsetInLine < selection_.initialWordColumns.first)) {
			WordBreakIterator<k::DocumentCharacterIterator> i(
				k::DocumentCharacterIterator(document, destination), text::AbstractWordBreakIterator::BOUNDARY_OF_SEGMENT, id);
			--i;
			caret.select(k::Position(selection_.initialLine, selection_.initialWordColumns.second),
				(i.base().tell().line == destination.line) ? i.base().tell() : k::Position(destination.line, 0));
		} else if(destination.line > selection_.initialLine
				|| (destination.line == selection_.initialLine
					&& destination.offsetInLine > selection_.initialWordColumns.second)) {
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

#ifdef ASCENSION_WINDOW_SYSTEM_WIN32
/// @see IDropSource#GiveFeedback
STDMETHODIMP DefaultMouseInputStrategy::GiveFeedback(DWORD) {
	return DRAGDROP_S_USEDEFAULTCURSORS;	// use the system default cursor
}
#endif // ASCENSION_WINDOW_SYSTEM_WIN32

/// @see MouseInputStrategy#handleDropTarget
shared_ptr<widgetapi::DropTarget> DefaultMouseInputStrategy::handleDropTarget() const {
	const widgetapi::DropTarget* const self = this;
	return shared_ptr<widgetapi::DropTarget>(const_cast<widgetapi::DropTarget*>(self), detail::NullDeleter());
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
	if((htr & TextViewer::RULER_MASK) != 0) {
		const k::Position to(viewToModel(*viewer_->textRenderer().viewport(), position, TextLayout::LEADING));
		const bool extend = win32::boole(modifiers & MK_SHIFT) && to.line != line(caret.anchor());
		state_ = EXTENDING_LINE_SELECTION;
		selection_.initialLine = extend ? line(caret.anchor()) : to.line;
		viewer_->caret().endRectangleSelection();
		extendSelectionTo(&to);
		widgetapi::grabInput(*viewer_);
		timer_.start(SELECTION_EXPANSION_INTERVAL, *this);
	}

	// approach drag-and-drop
	else if(/*dnd_.supportLevel >= SUPPORT_DND &&*/ !isSelectionEmpty(caret) && isPointOverSelection(caret, position)) {
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
				if(const boost::optional<k::Position> p = viewToModelInBounds(*viewer_->textRenderer().viewport(), position, TextLayout::TRAILING)) {
					if(const hyperlink::Hyperlink* link = utils::getPointedHyperlink(*viewer_, *p)) {
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
			if(const boost::optional<k::Position> to = viewToModelInBounds(*viewer_->textRenderer().viewport(), position, TextLayout::TRAILING)) {
				state_ = EXTENDING_CHARACTER_SELECTION;
				if((modifiers & (widgetapi::UserInput::CONTROL_DOWN | widgetapi::UserInput::SHIFT_DOWN)) != 0) {
					if((modifiers & widgetapi::UserInput::CONTROL_DOWN) != 0) {
						// begin word selection
						state_ = EXTENDING_WORD_SELECTION;
						caret.moveTo((modifiers & widgetapi::UserInput::SHIFT_DOWN) != 0 ? caret.anchor() : *to);
						selectWord(caret);
						selection_.initialLine = line(caret);
						selection_.initialWordColumns = make_pair(offsetInLine(caret.beginning()), offsetInLine(caret.end()));
					}
					if((modifiers & widgetapi::UserInput::SHIFT_DOWN) != 0)
						extendSelectionTo(&*to);
				} else
					caret.moveTo(*to);
				if((modifiers & widgetapi::UserInput::ALT_DOWN) != 0)	// make the selection reactangle
					caret.beginRectangleSelection();
				else
					caret.endRectangleSelection();
				widgetapi::grabInput(*viewer_);
				timer_.start(SELECTION_EXPANSION_INTERVAL, *this);
			}
		}
	}

//	if(!caret.isSelectionRectangle() && !boxDragging)
//		viewer_->redrawLine(caret.line());
	widgetapi::setFocus(*viewer_);
}

/// Handles @c WM_LBUTTONUP.
void DefaultMouseInputStrategy::handleLeftButtonReleased(const NativePoint& position, int) {
	// cancel if drag-and-drop approaching
	if(/*dnd_.supportLevel >= SUPPORT_DND
			&&*/ (state_ == APPROACHING_DND
			|| state_ == DND_SOURCE)) {	// TODO: this should handle only case APPROACHING_DND?
		state_ = NONE;
		viewer_->caret().moveTo(viewToModel(*viewer_->textRenderer().viewport(), position, TextLayout::TRAILING));
		::SetCursor(::LoadCursor(nullptr, IDC_IBEAM));	// hmm...
	}

	timer_.stop();
	if((state_ & SELECTION_EXTENDING_MASK) == SELECTION_EXTENDING_MASK) {
		state_ = NONE;
		// if released the button when extending the selection, the scroll may not reach the caret position
		utils::show(viewer_->caret());
	}
	widgetapi::releaseInput(*viewer_);
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
	if(viewer_ != nullptr)
		uninstall();
	viewer_ = &viewer;
#ifdef ASCENSION_WINDOW_SYSTEM_WIN32
	if(dnd_.dragSourceHelper.get() == nullptr)
		dnd_.dragSourceHelper = win32::com::SmartPointer<IDragSourceHelper>::create(CLSID_DragDropHelper, IID_IDragSourceHelper, CLSCTX_INPROC_SERVER);
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
bool DefaultMouseInputStrategy::mouseButtonInput(Action action, const widgetapi::MouseButtonInput& input) {
	if(action != RELEASED && endAutoScroll())
		return true;
	switch(input.button()) {
	case widgetapi::UserInput::BUTTON1_DOWN:
		if(action == PRESSED)
			handleLeftButtonPressed(input.location(), input.modifiers());
		else if(action == RELEASED)
			handleLeftButtonReleased(input.location(), input.modifiers());
		else if(action == DOUBLE_CLICKED) {
			texteditor::abortIncrementalSearch(*viewer_);
			if(handleLeftButtonDoubleClick(input.location(), input.modifiers()))
				return true;
			const TextViewer::HitTestResult htr = viewer_->hitTest(widgetapi::mapFromGlobal(*viewer_, widgetapi::Cursor::position()));
			if((htr & TextViewer::TEXT_AREA_MASK) != 0) {
				// begin word selection
				Caret& caret = viewer_->caret();
				selectWord(caret);
				state_ = EXTENDING_WORD_SELECTION;
				selection_.initialLine = line(caret);
				selection_.initialWordColumns = make_pair(offsetInLine(caret.anchor()), offsetInLine(caret));
				widgetapi::grabInput(*viewer_);
				timer_.start(SELECTION_EXPANSION_INTERVAL, *this);
				return true;
			}
		}
		break;
	case widgetapi::UserInput::BUTTON2_DOWN:
		if(action == PRESSED) {
			if(viewer_->document().numberOfLines() > viewer_->textRenderer().viewport()->numberOfVisibleLines()) {
				state_ = APPROACHING_AUTO_SCROLL;
				dragApproachedPosition_ = input.location();
				const NativePoint p(widgetapi::mapToGlobal(*viewer_, input.location()));
				widgetapi::setFocus(*viewer_);
				// show the indicator margin
				NativeRectangle rect(widgetapi::bounds(*autoScrollOriginMark_, true));
				widgetapi::move(*autoScrollOriginMark_,
					geometry::make<NativePoint>(geometry::x(p) - geometry::dx(rect) / 2, geometry::y(p) - geometry::dy(rect) / 2));
				widgetapi::show(*autoScrollOriginMark_);
				widgetapi::raise(*autoScrollOriginMark_);
				widgetapi::grabInput(*viewer_);
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
	case widgetapi::UserInput::BUTTON3_DOWN:
		return handleRightButton(action, input.location(), input.modifiers());
	case widgetapi::UserInput::BUTTON4_DOWN:
		return handleX1Button(action, input.location(), input.modifiers());
	case widgetapi::UserInput::BUTTON5_DOWN:
		return handleX2Button(action, input.location(), input.modifiers());
	}
	return false;
}

/// @see MouseInputStrategy#mouseMoved
void DefaultMouseInputStrategy::mouseMoved(const widgetapi::LocatedUserInput& input) {
	if(state_ == APPROACHING_AUTO_SCROLL
			|| (/*dnd_.supportLevel >= SUPPORT_DND &&*/ state_ == APPROACHING_DND)) {	// dragging starts?
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
					beginDragAndDrop();
				else {
					state_ = AUTO_SCROLL_DRAGGING;
					timer_.start(0, *this);
				}
			}
		}
	} else if((state_ & SELECTION_EXTENDING_MASK) == SELECTION_EXTENDING_MASK)
		extendSelectionTo();
}

/// @see MouseInputStrategy#mouseWheelRotated
void DefaultMouseInputStrategy::mouseWheelRotated(const widgetapi::MouseWheelInput& input) {
	if(!endAutoScroll()) {
		const shared_ptr<TextViewport> viewport(viewer_->textRenderer().viewport());
		// use system settings
		UINT lines;	// the number of lines to scroll
		if(!win32::boole(::SystemParametersInfoW(SPI_GETWHEELSCROLLLINES, 0, &lines, 0)))
			lines = 3;
		if(lines == WHEEL_PAGESCROLL) {
			// TODO: calculate precise page size.
			lines = static_cast<UINT>(viewport->numberOfVisibleLines());
		}
		viewport->scroll(PhysicalTwoAxes<TextViewport::SignedScrollOffset>(
			0, -geometry::dy(input.rotation()) * static_cast<short>(lines) / WHEEL_DELTA));
	}
}

#ifdef ASCENSION_WINDOW_SYSTEM_WIN32
/// Implements @c IDropSource#QueryContinueDrag method.
STDMETHODIMP DefaultMouseInputStrategy::QueryContinueDrag(BOOL escapePressed, DWORD keyState) {
	if(win32::boole(escapePressed) || win32::boole(keyState & MK_RBUTTON))	// cancel
		return DRAGDROP_S_CANCEL;
	if(!win32::boole(keyState & MK_LBUTTON))	// drop
		return DRAGDROP_S_DROP;
	return S_OK;
}
#endif // ASCENSION_WINDOW_SYSTEM_WIN32

/// @see MouseInputStrategy#showCursor
bool DefaultMouseInputStrategy::showCursor(const NativePoint& position) {
	using namespace hyperlink;
	LPCTSTR cursorName = nullptr;
	const Hyperlink* newlyHoveredHyperlink = nullptr;

	// on the vertical ruler?
	const TextViewer::HitTestResult htr = viewer_->hitTest(position);
	if((htr & TextViewer::RULER_MASK) != 0)
		cursorName = IDC_ARROW;
	// on a draggable text selection?
	else if(/*dnd_.supportLevel >= SUPPORT_DND &&*/ !isSelectionEmpty(viewer_->caret()) && isPointOverSelection(viewer_->caret(), position))
		cursorName = IDC_ARROW;
	else if(htr == TextViewer::TEXT_AREA_CONTENT_RECTANGLE) {
		// on a hyperlink?
		if(const boost::optional<k::Position> p =
				viewToModelInBounds(*viewer_->textRenderer().viewport(), position, TextLayout::TRAILING, k::locations::UTF16_CODE_UNIT))
			newlyHoveredHyperlink = utils::getPointedHyperlink(*viewer_, *p);
		if(newlyHoveredHyperlink != nullptr && win32::boole(::GetAsyncKeyState(VK_CONTROL) & 0x8000))
			cursorName = IDC_HAND;
	}

	if(cursorName != nullptr) {
		::SetCursor(static_cast<HCURSOR>(::LoadImageW(
			nullptr, cursorName, IMAGE_CURSOR, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE | LR_SHARED)));
		return true;
	}
	if(newlyHoveredHyperlink != nullptr) {
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
		const shared_ptr<TextViewport> viewport(viewer_->textRenderer().viewport());
		const NativePoint p(widgetapi::mapFromGlobal(*viewer_, widgetapi::Cursor::position()));
		const NativeRectangle contentRectangle(viewer_->textAreaContentRectangle());
		NativeSize scrollUnits(geometry::make<NativeSize>(
			inlineProgressionScrollOffsetInPixels(*viewport, 1),
			viewer_->textRenderer().defaultFont()->metrics().linePitch()));
		if(isVertical(viewer_->textRenderer().writingMode().blockFlowDirection))
			geometry::transpose(scrollUnits);

		PhysicalTwoAxes<TextViewport::SignedScrollOffset> scrollOffsets(0, 0);
		// no rationale about these scroll amounts
		if(geometry::y(p) < geometry::top(contentRectangle))
			scrollOffsets.y() = (geometry::y(p) - (geometry::top(contentRectangle))) / geometry::dy(scrollUnits) - 1;
		else if(geometry::y(p) >= geometry::bottom(contentRectangle))
			scrollOffsets.y() = (geometry::y(p) - (geometry::bottom(contentRectangle))) / geometry::dy(scrollUnits) + 1;
		else if(geometry::x(p) < geometry::left(contentRectangle))
			scrollOffsets.x() = (geometry::x(p) - (geometry::left(contentRectangle))) / geometry::dx(scrollUnits) - 1;
		else if(geometry::x(p) >= geometry::right(contentRectangle))
			scrollOffsets.x() = (geometry::x(p) - (geometry::right(contentRectangle))) / geometry::dx(scrollUnits) + 1;
		if(scrollOffsets.x() != 0 || scrollOffsets.y() != 0)
			viewport->scroll(scrollOffsets);
		extendSelectionTo();
	} else if(state_ == AUTO_SCROLL_DRAGGING || state_ == AUTO_SCROLL) {
		const shared_ptr<TextViewport> viewport(viewer_->textRenderer().viewport());
		timer.stop();
		const NativePoint p(widgetapi::mapFromGlobal(*viewer_, widgetapi::Cursor::position()));
		NativeSize scrollUnits(geometry::make<NativeSize>(
			inlineProgressionScrollOffsetInPixels(*viewport, 1),
			viewer_->textRenderer().defaultFont()->metrics().linePitch()));
		if(isVertical(viewer_->textRenderer().writingMode().blockFlowDirection))
			geometry::transpose(scrollUnits);
		const NativeSize scrollOffsets(geometry::make<NativeSize>(
			(geometry::x(p) - geometry::x(dragApproachedPosition_)) / geometry::dx(scrollUnits),
			(geometry::y(p) - geometry::y(dragApproachedPosition_)) / geometry::dy(scrollUnits)));
//		const Scalar scrollDegree = max(abs(yScrollDegree), abs(xScrollDegree));

		if(geometry::dy(scrollOffsets) != 0 /*&& abs(geometry::dy(scrollOffsets)) >= abs(geometry::dx(scrollOffsets))*/)
			viewport->scroll(PhysicalTwoAxes<TextViewport::SignedScrollOffset>(0, (geometry::dy(scrollOffsets) > 0) ? +1 : -1));
//		else if(geometry::dx(scrollOffsets) != 0)
//			viewport->scroll(PhysicalTwoAxes<TextViewport::SignedScrollOffset>((geometry::dx(scrollOffsets) > 0) ? +1 : -1, 0));

		if(geometry::dy(scrollOffsets) != 0) {
			timer_.start(500 / static_cast<unsigned int>((pow(2.0f, abs(geometry::dy(scrollOffsets)) / 2))), *this);
			::SetCursor(AutoScrollOriginMark::cursorForScrolling(
				(geometry::dy(scrollOffsets) > 0) ? AutoScrollOriginMark::CURSOR_DOWNWARD : AutoScrollOriginMark::CURSOR_UPWARD).asNativeObject().get());
		} else {
			timer_.start(300, *this);
			::SetCursor(AutoScrollOriginMark::cursorForScrolling(AutoScrollOriginMark::CURSOR_NEUTRAL).asNativeObject().get());
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
	if(autoScrollOriginMark_.get() != nullptr)
		autoScrollOriginMark_.reset();
	viewer_ = nullptr;
}


// window system-dependent implementations ////////////////////////////////////////////////////////

#if defined(ASCENSION_WINDOWS_SYSTEM_GTK)
#elif defined(ASCENSION_WINDOWS_SYSTEM_QT)
#elif defined(ASCENSION_WINDOWS_SYSTEM_QUARTZ)
#elif defined(ASCENSION_WINDOWS_SYSTEM_WIN32)
#endif
