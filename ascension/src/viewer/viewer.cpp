/**
 * @file viewer.cpp
 * @author exeal
 * @date 2003-2006 was EditView.cpp and EditViewWindowMessages.cpp
 * @date 2006-2012
 */

#include <ascension/rules.hpp>
#include <ascension/corelib/text/break-iterator.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/text-editor/command.hpp>
#include <ascension/text-editor/session.hpp>
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

#ifdef _DEBUG
bool DIAGNOSE_INHERENT_DRAWING = false;	// 余計な描画を行っていないか診断するフラグ
//#define TRACE_DRAWING_STRING	// テキスト (代替グリフ以外) のレンダリングをトレース
#endif // _DEBUG

namespace {
	inline Scalar mapLocalBpdToTextArea(const TextViewer& viewer, Scalar bpd) {
		const NativeRectangle textArea(viewer.textAreaAllocationRectangle());
		AbstractFourSides<Scalar> abstractBounds;
		mapPhysicalToAbstract(viewer.textRenderer().writingMode(), textArea, textArea, abstractBounds);
		return bpd -= abstractBounds.before();
	}
	inline Scalar mapTextAreaBpdToLocal(const TextViewer& viewer, Scalar bpd) {
		const NativeRectangle textArea(viewer.textAreaAllocationRectangle());
		AbstractFourSides<Scalar> abstractBounds;
		mapPhysicalToAbstract(viewer.textRenderer().writingMode(), textArea, textArea, abstractBounds);
		return bpd += abstractBounds.before();
	}
	inline NativePoint mapLocalToTextArea(const TextViewer& viewer, const NativePoint& p) {
		const NativeRectangle textArea(viewer.textAreaAllocationRectangle());
		NativePoint temp(p);
		return geometry::translate(temp, geometry::make<NativeSize>(-geometry::left(textArea), -geometry::top(textArea)));
	}
	inline NativePoint mapTextAreaToLocal(const TextViewer& viewer, const NativePoint& p) {
		const NativeRectangle textArea(viewer.textAreaAllocationRectangle());
		NativePoint temp(p);
		return geometry::translate(temp, geometry::make<NativeSize>(+geometry::left(textArea), +geometry::top(textArea)));
	}
}


// TextViewer /////////////////////////////////////////////////////////////////////////////////////

/**
 * @class ascension::viewers::TextViewer
 *
 * The view of Ascension framework. @c TextViewer displays the content of the document, manipulates
 * the document with the caret and selection, and provides other visual presentations.
 *
 * @c TextViewer provides two methods #freeze and #unfreeze to freeze of the screen of the window.
 * While the viewer is frozen, the window does not redraw the content.
 *
 * <h3>Duplication</h3>
 *
 * Unlike @c manah#win32#ui#Window class, the copy-constructor does not copy the window handle of
 * the original (attachement is not performed). This semantics is derived from
 * @c manah#win32#ui#CustomControl super class.
 *
 * So an object just created by copy-constructor does not have valid window handle. Call @c #create
 * method to construct as a window.
 *
 * <h3>Window styles related to bidirectional</h3>
 *
 * テキストを右寄せで表示するための @c WS_EX_RIGHT 、右から左に表示するための @c WS_EX_RTLREADING
 * はいずれも無視される。これらの設定には代わりに @c LayoutSettings の該当メンバを使用しなければならない
 *
 * @c WS_EX_LAYOUTRTL is also not supported. The result is undefined if you used.
 *
 * 垂直スクロールバーを左側に表示するにはクライアントが @c WS_EX_LEFTSCROLLBAR を設定しなければならない
 *
 * これらの設定を一括して変更する場合 @c #setTextDirection を使うことができる
 *
 * 垂直ルーラ (インジケータマージンと行番号) の位置はテキストが左寄せであれば左端、
 * 右寄せであれば右端になる
 *
 * <h3>Subclassing</h3>
 *
 * @c TextViewer and @c SourceViewer are intended to be subclassed. You can override the virtual
 * member functions in your derived class. Note that @c TextViewer implements interfaces defined
 * in Ascension by virtual functions. These are also overridable but you must call base
 * implementation. For example, you are overriding @c documentChanged:
 *
 * @code
 * void YourViewer::documentChanged(const Document& document, const DocumentChange& change) {
 *   // ...your own code
 *   TextViewer::documentChanged(document, change);
 * }
 * @endcode
 *
 * <h3>Windows specific features</h3>
 *
 * @c TextViewer supports drag-and-drop. If you want to enable this feature, call Win32
 * @c OleInitialize in your thread.
 *
 * If you want to enable tooltips, call Win32 @c InitCommonControlsEx.
 *
 * @see presentation#Presentation, Caret
 */

/**
 * Constructor.
 * @param presentation the presentation
 */
TextViewer::TextViewer(Presentation& presentation, Widget* parent /* = nullptr */, Style styles /* = WIDGET */)
		: ScrollableWidget(parent, styles), presentation_(presentation), tipText_(nullptr),
#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
		accessibleProxy_(nullptr),
#endif // !ASCENSION_NO_ACTIVE_ACCESSIBILITY
		mouseInputDisabledCount_(0) {
	renderer_.reset(new Renderer(*this));
	textRenderer().addComputedWritingModeListener(*this);
//	renderer_->addFontListener(*this);
//	renderer_->addVisualLinesListener(*this);
	caret_.reset(new Caret(*this));
	caret().addListener(*this);
	caret().addStateListener(*this);
	rulerPainter_.reset(new detail::RulerPainter(*this));

	document().addListener(*this);
	document().addRollbackListener(*this);

	// initializations of renderer_ and mouseInputStrategy_ are in initializeWindow()
}

/**
 * Copy-constructor. Unlike @c win32#Object class, this does not copy the window handle. For
 * more details, see the description of @c TextViewer.
 */
TextViewer::TextViewer(const TextViewer& other) : presentation_(other.presentation_), tipText_(nullptr)
#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
		, accessibleProxy_(nullptr)
#endif // !ASCENSION_NO_ACTIVE_ACCESSIBILITY
{
	renderer_.reset(new Renderer(*other.renderer_, *this));
	textRenderer().addComputedWritingModeListener(*this);
//	renderer_->addFontListener(*this);
//	renderer_->addVisualLinesListener(*this);
	caret_.reset(new Caret(*this));
	caret().addListener(*this);
	caret().addStateListener(*this);
	rulerPainter_.reset(new detail::RulerPainter(*this));

	modeState_ = other.modeState_;

	mouseInputDisabledCount_ = 0;
	document().addListener(*this);
	document().addRollbackListener(*this);
}

/// Destructor.
TextViewer::~TextViewer() {
	document().removeListener(*this);
	document().removeRollbackListener(*this);
	textRenderer().removeComputedWritingModeListener(*this);
	textRenderer().removeDefaultFontListener(*this);
	textRenderer().layouts().removeVisualLinesListener(*this);
	caret().removeListener(*this);
	caret().removeStateListener(*this);
	for(set<VisualPoint*>::iterator it = points_.begin(); it != points_.end(); ++it)
		(*it)->viewerDisposed();

	// 非共有データ
//	delete selection_;
	delete[] tipText_;
}

/// @see Widget#aboutToLoseFocus
void TextViewer::aboutToLoseFocus() {
	cursorVanisher_.restore();
	if(mouseInputStrategy_.get() != nullptr)
		mouseInputStrategy_->interruptMouseReaction(false);
/*	if(caret_->getMatchBracketsTrackingMode() != Caret::DONT_TRACK
			&& getCaret().getMatchBrackets().first != Position::INVALID_POSITION) {	// 対括弧の通知を終了
		FOR_EACH_LISTENERS()
			(*it)->onMatchBracketFoundOutOfView(Position::INVALID_POSITION);
	}
	if(completionWindow_->isWindow() && newWindow != completionWindow_->getSafeHwnd())
		closeCompletionProposalsPopup(*this);
*/	texteditor::abortIncrementalSearch(*this);
	static_cast<detail::InputEventHandler&>(caret()).abortInput();
//	if(currentWin32WindowMessage().wParam != get()) {
//		hideCaret();
//		::DestroyCaret();
//	}
	redrawLines(makeRange(line(caret().beginning()), line(caret().end()) + 1));
	redrawScheduledRegion();
}

/**
 * Registers the display size listener.
 * @param listener The listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
void TextViewer::addDisplaySizeListener(DisplaySizeListener& listener) {
	displaySizeListeners_.add(listener);
}

/**
 * Registers the viewport listener.
 * @param listener The listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
void TextViewer::addViewportListener(ViewportListener& listener) {
	viewportListeners_.add(listener);
}

/// @see CaretListener#caretMoved
void TextViewer::caretMoved(const Caret& self, const k::Region& oldRegion) {
	if(!isVisible())
		return;
	const k::Region newRegion(self.selectedRegion());
	bool changed = false;

	// redraw the selected region
	if(self.isSelectionRectangle()) {	// rectangle
		if(!oldRegion.isEmpty())
			redrawLines(makeRange(oldRegion.beginning().line, oldRegion.end().line + 1));
		if(!newRegion.isEmpty())
			redrawLines(makeRange(newRegion.beginning().line, newRegion.end().line + 1));
	} else if(newRegion != oldRegion) {	// the selection actually changed
		if(oldRegion.isEmpty()) {	// the selection was empty...
			if(!newRegion.isEmpty())	// the selection is not empty now
				redrawLines(makeRange(newRegion.beginning().line, newRegion.end().line + 1));
		} else {	// ...if the there is selection
			if(newRegion.isEmpty()) {	// the selection became empty
				redrawLines(makeRange(oldRegion.beginning().line, oldRegion.end().line + 1));
				if(!isFrozen())
					redrawScheduledRegion();
			} else if(oldRegion.beginning() == newRegion.beginning()) {	// the beginning point didn't change
				const Index i[2] = {oldRegion.end().line, newRegion.end().line};
				redrawLines(makeRange(min(i[0], i[1]), max(i[0], i[1]) + 1));
			} else if(oldRegion.end() == newRegion.end()) {	// the end point didn't change
				const Index i[2] = {oldRegion.beginning().line, newRegion.beginning().line};
				redrawLines(makeRange(min(i[0], i[1]), max(i[0], i[1]) + 1));
			} else {	// the both points changed
				if((oldRegion.beginning().line >= newRegion.beginning().line && oldRegion.beginning().line <= newRegion.end().line)
						|| (oldRegion.end().line >= newRegion.beginning().line && oldRegion.end().line <= newRegion.end().line)) {
					const Index i[2] = {
						min(oldRegion.beginning().line, newRegion.beginning().line), max(oldRegion.end().line, newRegion.end().line)
					};
					redrawLines(makeRange(min(i[0], i[1]), max(i[0], i[1]) + 1));
				} else {
					redrawLines(makeRange(oldRegion.beginning().line, oldRegion.end().line + 1));
					if(!isFrozen())
						redrawScheduledRegion();
					redrawLines(makeRange(newRegion.beginning().line, newRegion.end().line + 1));
				}
			}
		}
		changed = true;
	}

	if(changed && !isFrozen())
		redrawScheduledRegion();
}

/// @see ComputedWritingModeListener#computedWritingModeChanged
void TextViewer::computedWritingModeChanged(const presentation::WritingMode& used) {
	updateScrollBars();
}

/// @see DefaultFontListener#defaultFontChanged
void TextViewer::defaultFontChanged() /*throw()*/ {
	rulerPainter_->update();
	scrolls_.resetBars(*this, 'a', true);
	updateScrollBars();
	caret().resetVisualization();
	redrawLine(0, true);
}

/// @see kernel#DocumentListener#documentAboutToBeChanged
void TextViewer::documentAboutToBeChanged(const k::Document&) {
	// do nothing
}

/// @see kernel#DocumentListener#documentChanged
void TextViewer::documentChanged(const k::Document&, const k::DocumentChange& change) {
	// cancel the active incremental search
	texteditor::abortIncrementalSearch(*this);	// TODO: should TextViewer handle this? (I.S. would...)

	// slide the frozen lines to be drawn
	if(isFrozen() && !isEmpty(freezeRegister_.linesToRedraw())) {
		Index b = freezeRegister_.linesToRedraw().beginning();
		Index e = freezeRegister_.linesToRedraw().end();
		if(change.erasedRegion().first.line != change.erasedRegion().second.line) {
			const Index first = change.erasedRegion().first.line + 1, last = change.erasedRegion().second.line;
			if(b > last)
				b -= last - first + 1;
			else if(b > first)
				b = first;
			if(e != numeric_limits<Index>::max()) {
				if(e > last)
					e -= last - first + 1;
				else if(e > first)
					e = first;
			}
		}
		if(change.insertedRegion().first.line != change.insertedRegion().second.line) {
			const Index first = change.insertedRegion().first.line + 1, last = change.insertedRegion().second.line;
			if(b >= first)
				b += last - first + 1;
			if(e >= first && e != numeric_limits<Index>::max())
				e += last - first + 1;
		}
		freezeRegister_.resetLinesToRedraw(makeRange(b, e));
	}
//	invalidateLines(region.beginning().line, !multiLine ? region.end().line : INVALID_INDEX);
	if(!isFrozen())
		rulerPainter_->update();
	if(scrolls_.changed)
		updateScrollBars();
}

/// @see kernel#DocumentRollbackListener#documentUndoSequenceStarted
void TextViewer::documentUndoSequenceStarted(const k::Document&) {
	freeze();	// TODO: replace with AutoFreeze.
}

/// @see kernel#DocumentRollbackListener#documentUndoSequenceStopped
void TextViewer::documentUndoSequenceStopped(const k::Document&, const k::Position& resultPosition) {
	unfreeze();	// TODO: replace with AutoFreeze.
	if(/*resultPosition != k::Position() &&*/ hasFocus()) {
		utils::closeCompletionProposalsPopup(*this);
		caret_->moveTo(resultPosition);
	}
}

/// @see Widget#dragEntered
void TextViewer::dragEntered(DragEnterInput& input) {
	if(dropTargetHandler_.get() == nullptr)
		return base::Widget::dragEntered(input);
	return dropTargetHandler_->dragEntered(input);
}

/// @see Widget#dragLeft
void TextViewer::dragLeft(DragLeaveInput& input) {
	if(dropTargetHandler_.get() == nullptr)
		return base::Widget::dragLeft(input);
	return dropTargetHandler_->dragLeft(input);
}

/// @see Widget#dragMoved
void TextViewer::dragMoved(DragMoveInput& input) {
	if(dropTargetHandler_.get() == nullptr)
		return base::Widget::dragMoved(input);
	return dropTargetHandler_->dragMoved(input);
}

/**
 * Additionally draws the indicator margin on the vertical ruler.
 * @param line The line number
 * @param context The graphics context
 * @param rect The rectangle to draw
 */
void TextViewer::drawIndicatorMargin(Index /* line */, Context& /* context */, const NativeRectangle& /* rect */) {
}

/// @see Widget#dropped
void TextViewer::dropped(DropInput& input) {
	if(dropTargetHandler_.get() == nullptr)
		return base::Widget::dropped(input);
	return dropTargetHandler_->dropped(input);
}

/**
 * Freezes the drawing of the viewer.
 * @throw WindowNotInitialized The window is not initialized
 * @see #isFrozen, #unfreeze, #AutoFreeze
 */
void TextViewer::freeze() {
//	checkInitialization();
	freezeRegister_.freeze();
}

#if 0
/**
 * Returns the text and the region of a link near the cursor.
 * @param[out] region the region of the link
 * @param[out] text the text of the link. if the link is mail address, "mailto:" will be added to the head
 * @return @c true if the cursor is on link
 * @deprecated 0.8
 */
bool TextViewer::getPointedLinkText(Region& region, AutoBuffer<Char>& text) const {
	checkInitialization();
	const Document& document = document();
	const Position pos = getCharacterForClientXY(getCursorPosition(), false);	// カーソル位置に最も近い文字位置

	if(pos.offsetInLine == document.getLineLength(pos.line))	// 指定位置に文字が無い
		return false;

	const LineLayout& layout = renderer_->getLineLayout(pos.line);
	const Index subline = layout.getSubline(pos.offsetInLine);
	const Char* const line = document.getLine(pos.line).data();
	const Char* const first = line + layout.getSublineOffset(subline);
	const Char* const last =
		line + ((subline < layout.getNumberOfSublines() - 1) ? layout.getSublineOffset(subline + 1) : document.getLineLength(pos.line));
	Index linkLength;	// URIDetector の eatMailAddress 、eatUrlString で見つけたリンクテキストの長さ

	for(const Char* p = (pos.offsetInLine > 200) ? first + pos.offsetInLine - 200 : first; p <= first + pos.offsetInLine; ) {
		if(p != first) {
			if((p[-1] >= L'A' && p[-1] <= L'Z')
					|| (p[-1] >= L'a' && p[-1] <= L'z')
					|| p[-1] == L'_') {
				++p;
				continue;
			}
		}
		if(0 != (linkLength = rules::URIDetector::eatURL(p, last, true) - p)) {
			if(p - first + linkLength > pos.offsetInLine) {	// カーソル位置を越えた
				region.first.line = region.second.line = pos.line;
				region.first.offsetInLine = p - line;
				region.second.offsetInLine = region.first.offsetInLine + linkLength;
				text.reset(new Char[linkLength + 1]);
				wmemcpy(text.get(), p, linkLength);
				text[linkLength] = 0;
				return true;
			}
			p += linkLength;	// 届かない場合は続行
		} else if(0 != (linkLength = rules::URIDetector::eatMailAddress(p, last, true) - p)) {
			if(p - first + linkLength > pos.offsetInLine) {	// カーソル位置を越えた
				static const wchar_t MAILTO_PREFIX[] = L"mailto:";
				region.first.line = region.second.line = pos.line;
				region.first.offsetInLine = p - line;
				region.second.offsetInLine = region.first.offsetInLine + linkLength;
				text.reset(new Char[linkLength + 7 + 1]);
				wmemcpy(text.get(), MAILTO_PREFIX, countof(MAILTO_PREFIX) - 1);
				wmemcpy(text.get() + countof(MAILTO_PREFIX) - 1, p, linkLength);
				text[countof(MAILTO_PREFIX) - 1 + linkLength] = 0;
				return true;
			}
			p += linkLength;	// 届かない場合は続行
		} else
			++p;
	}
	return false;
}
#endif

namespace {
	inline TextViewer::ScrollPosition reverseScrollPosition(
			const TextRenderer& textRenderer, TextViewer::ScrollPosition position) {
		return textRenderer.layouts().maximumMeasure()
			/ textRenderer.defaultFont()->metrics().averageCharacterWidth()
			- position
			- static_cast<TextViewer::ScrollPosition>(textRenderer.viewport()->numberOfVisibleCharactersInLine());
	}
	NativePoint physicalScrollPosition(const TextViewer& viewer) {
		const shared_ptr<const TextViewport> viewport(viewer.textRenderer().viewport());
		const Index bpd = viewport->firstVisibleLineInVisualNumber();
		const Index ipd = viewport->inlineProgressionOffset();
		const WritingMode writingMode(viewer.textRenderer().writingMode());
		TextViewer::ScrollPosition x, y;
		switch(writingMode.blockFlowDirection) {
			case HORIZONTAL_TB:
				x = (writingMode.inlineFlowDirection == LEFT_TO_RIGHT) ?
					ipd : reverseScrollPosition(viewer.textRenderer(), static_cast<TextViewer::ScrollPosition>(ipd));
				y = bpd;
				break;
			case VERTICAL_RL:
				x = reverseScrollPosition(viewer.textRenderer(), bpd);
				y = ipd;
				break;
			case VERTICAL_LR:
				x = bpd;
				y = ipd;
				break;
			default:
				ASCENSION_ASSERT_NOT_REACHED();
		}
		return geometry::make<NativePoint>(x /* / xScrollRate */, y /* / yScrollRate */);
	}
}

/// @see Widget#focusGained
void TextViewer::focusGained() {
#ifdef ASCENSION_WINDOW_SYSTEM_WIN32
	// restore the scroll positions
	const NativePoint scrollPosition(physicalScrollPosition(*this));
	horizontalScrollBar().setPosition(geometry::x(scrollPosition));
	verticalScrollBar().setPosition(geometry::y(scrollPosition));
#endif // ASCENSION_WINDOW_SYSTEM_WIN32

	// hmm...
//	if(/*sharedData_->options.appearance[SHOW_CURRENT_UNDERLINE] ||*/ !getCaret().isSelectionEmpty()) {
		redrawLines(makeRange(line(caret().beginning()), line(caret().end()) + 1));
		redrawScheduledRegion();
//	}

//	if(currentWin32WindowMessage().wParam != get()) {
//		// resurrect the caret
//		recreateCaret();
//		updateCaretPosition();
//		if(texteditor::Session* const session = document().session()) {
//			if(texteditor::InputSequenceCheckers* const isc = session->inputSequenceCheckers())
//				isc->setKeyboardLayout(::GetKeyboardLayout(::GetCurrentThreadId()));
//		}
//	}
}

/**
 * Determines which part is at the specified position.
 * @param p The position to hit test, in the viewer-local coordinates
 * @return The result
 * @see TextViewer#HitTestResult
 */
TextViewer::HitTestResult TextViewer::hitTest(const NativePoint& p) const {
//	checkInitialization();
	const NativeRectangle localBounds(bounds(false));
	if(!geometry::includes(localBounds, p))
		return OUT_OF_VIEWER;

	const RulerConfiguration& rc = rulerConfiguration();
	if(rc.indicatorMargin.visible && geometry::includes(rulerPainter_->indicatorMarginAllocationRectangle(), p))
		return INDICATOR_MARGIN;
	else if(rc.lineNumbers.visible && geometry::includes(rulerPainter_->lineNumbersAllocationRectangle(), p))
		return LINE_NUMBERS;
	else if(geometry::includes(textAreaContentRectangle(), p))
		return TEXT_AREA_CONTENT_RECTANGLE;
	else {
		assert(geometry::includes(textAreaAllocationRectangle(), p));
		return TEXT_AREA_PADDING_START;
	}
}

/**
 * Returns an offset from left/top-edge of local-bounds to one of the content-area in pixels. This
 * algorithm considers the ruler, the scroll position and spaces around the content box.
 * @return The offset
 * @see TextLayout#lineStartEdge, TextRenderer#lineStartEdge
 */
Scalar TextViewer::inlineProgressionOffsetInViewport() const {
	const bool horizontal = isHorizontal(textRenderer().writingMode().blockFlowDirection);
	Scalar offset = 0;

	// scroll position
	const NativePoint scrollPosition(physicalScrollPosition(*this));
	offset -= (horizontal ? geometry::x(scrollPosition) : geometry::y(scrollPosition))
		* textRenderer().defaultFont()->metrics().averageCharacterWidth();

	// ruler width
	if((horizontal && rulerPainter_->alignment() == detail::RulerPainter::LEFT)
			|| (!horizontal && rulerPainter_->alignment() == detail::RulerPainter::TOP))
		offset += rulerPainter_->allocationWidth();

	return offset;

#if 0
	const detail::PhysicalTextAnchor alignment(
		detail::computePhysicalTextAnchor(layout.anchor(), layout.writingMode().inlineFlowDirection));
	if(alignment == detail::LEFT /*|| ... != NO_JUSTIFICATION*/)	// TODO: this code ignores last visual line with justification.
		return spaces.left - scrolls_.x() * renderer_->defaultFont()->metrics().averageCharacterWidth();

	Scalar indent;
	const NativeRectangle clientBounds(bounds(false));
	if(renderer_->layouts().maximumMeasure() + spaces.left + spaces.right > geometry::dx(clientBounds)) {
		indent = renderer_->layouts().maximumMeasure() - layout.measure(0) + spaces.left;
		indent += (geometry::dx(clientBounds) - spaces.left - spaces.right) % renderer_->defaultFont()->metrics().averageCharacterWidth();
	} else
		indent = geometry::dx(clientBounds) - layout.measure(0) - spaces.right;
	if(alignment == detail::MIDDLE)
		indent /= 2;
	else
		assert(alignment == detail::RIGHT);
	return indent - static_cast<Scalar>(scrolls_.x()) * renderer_->defaultFont()->metrics().averageCharacterWidth();
#endif
}

/// @see Widget#keyPressed
void TextViewer::keyPressed(const KeyInput& input) {
	if(mouseInputStrategy_.get() != nullptr)
		mouseInputStrategy_->interruptMouseReaction(true);

	// TODO: This code is temporary. The following code provides a default implementation of
	// TODO: "key combination to command" map.
	using namespace ascension::texteditor::commands;
//	if(hasModifier<UserInput::ALT_DOWN>(input)) {
//		if(!hasModifier<UserInput::SHIFT_DOWN>(input)
//				|| (input.keyboardCode() != VK_LEFT && input.keyboardCode() != VK_UP
//				&& input.keyboardCode() != VK_RIGHT && input.keyboardCode() != VK_DOWN))
//			return false;
//	}
	switch(input.keyboardCode()) {
	case keyboardcodes::BACK_SPACE:	// [BackSpace]
	case keyboardcodes::F16:	// [F16]
		if(hasModifier<UserInput::CONTROL_DOWN>(input))
			WordDeletionCommand(*this, Direction::BACKWARD)();
		else
			CharacterDeletionCommand(*this, Direction::BACKWARD)();
		break;
	case keyboardcodes::CLEAR:	// [Clear]
		if(hasModifier<UserInput::CONTROL_DOWN>(input))
			EntireDocumentSelectionCreationCommand(*this)();
		break;
	case keyboardcodes::ENTER_OR_RETURN:	// [Enter]
		NewlineCommand(*this, hasModifier<UserInput::CONTROL_DOWN>(input))();
		break;
	case keyboardcodes::SHIFT:	// [Shift]
		if(hasModifier<UserInput::CONTROL_DOWN>(input)
				&& (::GetKeyState(VK_LSHIFT) < 0 && configuration_.readingDirection == RIGHT_TO_LEFT)
				|| (::GetKeyState(VK_RSHIFT) < 0 && configuration_.readingDirection == LEFT_TO_RIGHT))
			utils::toggleOrientation(*this);
		break;
	case keyboardcodes::ESCAPE:	// [Esc]
		CancelCommand(*this)();
		break;
	case keyboardcodes::PRIOR_OR_PAGE_UP:	// [PageUp]
		if(hasModifier<UserInput::CONTROL_DOWN>(input))
			onVScroll(SB_PAGEUP, 0, win32::Handle<HWND>());
		else
			makeCaretMovementCommand(*this, &k::locations::backwardPage, hasModifier<UserInput::SHIFT_DOWN>(input))();
		break;
	case keyboardcodes::NEXT_OR_PAGE_DOWN:	// [PageDown]
		if(hasModifier<UserInput::CONTROL_DOWN>(input))
			onVScroll(SB_PAGEDOWN, 0, win32::Handle<HWND>());
		else
			makeCaretMovementCommand(*this, &k::locations::forwardPage, hasModifier<UserInput::SHIFT_DOWN>(input))();
		break;
	case keyboardcodes::HOME:	// [Home]
		if(hasModifier<UserInput::CONTROL_DOWN>(input))
			makeCaretMovementCommand(*this, &k::locations::beginningOfDocument, hasModifier<UserInput::SHIFT_DOWN>(input))();
		else
			makeCaretMovementCommand(*this, &k::locations::beginningOfVisualLine, hasModifier<UserInput::SHIFT_DOWN>(input))();
		break;
	case keyboardcodes::END:	// [End]
		if(hasModifier<UserInput::CONTROL_DOWN>(input))
			makeCaretMovementCommand(*this, &k::locations::endOfDocument, hasModifier<UserInput::SHIFT_DOWN>(input))();
		else
			makeCaretMovementCommand(*this, &k::locations::endOfVisualLine, hasModifier<UserInput::SHIFT_DOWN>(input))();
		break;
	case keyboardcodes::LEFT:	// [Left]
		if(hasModifier<UserInput::ALT_DOWN>(input) && hasModifier<UserInput::SHIFT_DOWN>(input)) {
			if(hasModifier<UserInput::CONTROL_DOWN>(input))
				makeRowSelectionExtensionCommand(*this, &k::locations::leftWord)();
			else
				makeRowSelectionExtensionCommand(*this, &k::locations::leftCharacter)();
		} else {
			if(hasModifier<UserInput::CONTROL_DOWN>(input))
				makeCaretMovementCommand(*this, &k::locations::leftWord, hasModifier<UserInput::SHIFT_DOWN>(input))();
			else
				makeCaretMovementCommand(*this, &k::locations::leftCharacter, hasModifier<UserInput::SHIFT_DOWN>(input))();
		}
		break;
	case keyboardcodes::UP:		// [Up]
		if(hasModifier<UserInput::ALT_DOWN>(input)
				&& hasModifier<UserInput::SHIFT_DOWN>(input) && !hasModifier<UserInput::CONTROL_DOWN>(input))
			makeRowSelectionExtensionCommand(*this, &k::locations::backwardVisualLine)();
		else if(hasModifier<UserInput::CONTROL_DOWN>(input) && !hasModifier<UserInput::SHIFT_DOWN>(input))
			textRenderer().viewport()->scroll(geometry::make<NativeSize>(0, -1), this);
		else
			makeCaretMovementCommand(*this, &k::locations::backwardVisualLine, hasModifier<UserInput::SHIFT_DOWN>(input))();
		break;
	case keyboardcodes::RIGHT:	// [Right]
		if(hasModifier<UserInput::ALT_DOWN>(input)) {
			if(hasModifier<UserInput::SHIFT_DOWN>(input)) {
				if(hasModifier<UserInput::CONTROL_DOWN>(input))
					makeRowSelectionExtensionCommand(*this, &k::locations::rightWord)();
				else
					makeRowSelectionExtensionCommand(*this, &k::locations::rightCharacter)();
			} else
				CompletionProposalPopupCommand(*this)();
		} else {
			if(hasModifier<UserInput::CONTROL_DOWN>(input))
				makeCaretMovementCommand(*this, &k::locations::rightWord, hasModifier<UserInput::SHIFT_DOWN>(input))();
			else
				makeCaretMovementCommand(*this, &k::locations::rightCharacter, hasModifier<UserInput::SHIFT_DOWN>(input))();
		}
		break;
	case keyboardcodes::DOWN:	// [Down]
		if(hasModifier<UserInput::ALT_DOWN>(input)
				&& hasModifier<UserInput::SHIFT_DOWN>(input) && !hasModifier<UserInput::CONTROL_DOWN>(input))
			makeRowSelectionExtensionCommand(*this, &k::locations::forwardVisualLine)();
		else if(hasModifier<UserInput::CONTROL_DOWN>(input) && !hasModifier<UserInput::SHIFT_DOWN>(input))
			textRenderer().viewport()->scroll(geometry::make<NativeSize>(0, +1), this);
		else
			makeCaretMovementCommand(*this, &k::locations::forwardVisualLine, hasModifier<UserInput::SHIFT_DOWN>(input))();
		break;
	case keyboardcodes::INSERT:	// [Insert]
		if(hasModifier<UserInput::ALT_DOWN>(input))
			break;
		else if(!hasModifier<UserInput::SHIFT_DOWN>(input)) {
			if(hasModifier<UserInput::CONTROL_DOWN>(input))
				copySelection(caret(), true);
			else
				OvertypeModeToggleCommand(*this)();
		} else if(hasModifier<UserInput::CONTROL_DOWN>(input))
			PasteCommand(*this, false)();
		else
			break;
		break;
	case keyboardcodes::DEL_OR_DELETE:	// [Delete]
		if(!hasModifier<UserInput::SHIFT_DOWN>(input)) {
			if(hasModifier<UserInput::CONTROL_DOWN>(input))
				WordDeletionCommand(*this, Direction::FORWARD)();
			else
				CharacterDeletionCommand(*this, Direction::FORWARD)();
		} else if(!hasModifier<UserInput::CONTROL_DOWN>(input))
			cutSelection(caret(), true);
		else
			break;
		break;
	case 'A':	// ^A -> Select All
		if(hasModifier<UserInput::CONTROL_DOWN>(input))
			EntireDocumentSelectionCreationCommand(*this)();
		break;
	case 'C':	// ^C -> Copy
		if(hasModifier<UserInput::CONTROL_DOWN>(input))
			copySelection(caret(), true);
		break;
	case 'H':	// ^H -> Backspace
		if(hasModifier<UserInput::CONTROL_DOWN>(input))
			CharacterDeletionCommand(*this, Direction::BACKWARD)(), true;
		break;
	case 'I':	// ^I -> Tab
		if(hasModifier<UserInput::CONTROL_DOWN>(input))
			CharacterInputCommand(*this, 0x0009u)();
		break;
	case 'J':	// ^J -> New Line
	case 'M':	// ^M -> New Line
		if(hasModifier<UserInput::CONTROL_DOWN>(input))
			NewlineCommand(*this, false)();
		break;
	case 'V':	// ^V -> Paste
		if(hasModifier<UserInput::CONTROL_DOWN>(input))
			PasteCommand(*this, false)();
		break;
	case 'X':	// ^X -> Cut
		if(hasModifier<UserInput::CONTROL_DOWN>(input))
			cutSelection(caret(), true);
		break;
	case 'Y':	// ^Y -> Redo
		if(hasModifier<UserInput::CONTROL_DOWN>(input))
			UndoCommand(*this, true)();
		break;
	case 'Z':	// ^Z -> Undo
		if(hasModifier<UserInput::CONTROL_DOWN>(input))
			UndoCommand(*this, false)();
		break;
	case keyboardcodes::NUMBER_PAD_5:	// [Number Pad 5]
		if(hasModifier<UserInput::CONTROL_DOWN>(input))
			EntireDocumentSelectionCreationCommand(*this)();
		break;
	case keyboardcodes::F12:	// [F12]
		if(hasModifier<UserInput::CONTROL_DOWN>(input) && hasModifier<UserInput::SHIFT_DOWN>(input))
			CodePointToCharacterConversionCommand(*this)();
		break;
	}
}

/// @see Widget#keyReleased
void TextViewer::keyReleased(const KeyInput& input) {
	if(hasModifier<UserInput::ALT_DOWN>(input)) {
		cursorVanisher_.restore();
		if(mouseInputStrategy_.get() != nullptr)
			mouseInputStrategy_->interruptMouseReaction(true);
	}
}

/**
 * @internal
 * @see #mapViewportIpdToLineLayout
 */
inline Scalar TextViewer::mapLineLayoutIpdToViewport(Index line, Scalar ipd) const {
	return ipd + lineStartEdge(textRenderer().layouts().at(line),
		textRenderer().viewport()->contentMeasure()) + inlineProgressionOffsetInViewport();
}

/**
 * @internal
 * @see #mapLineLayoutIpdToViewport
 */
inline Scalar TextViewer::mapViewportIpdToLineLayout(Index line, Scalar ipd) const {
	return ipd - lineStartEdge(textRenderer().layouts().at(line),
		textRenderer().viewport()->contentMeasure()) - inlineProgressionOffsetInViewport();
}

#if 0
/**
 * Returns the distance from the before-edge of the viewport to the baseline of the specified line.
 * @param line The logical line number
 * @param fullSearch @c false to return special value for the line outside of the viewport
 * @return The distance from the viewport's edge to the line in pixels
 * @retval std#numeric_limits&lt;Scalar&gt;::max() @a fullSearch is @c false and @a line is outside
 *                                                 of the after-edge of the viewport
 * @retval std#numeric_limits&lt;Scalar&gt;::min() @a fullSearch is @c false and @a line is outside
 *                                                 of the before-edge of the viewport
 * @throw kernel#BadPositionException @a line is outside of the document
 * @see #BaseIterator, #mapViewportBpdToLine, TextRenderer#offsetVisualLine
 */
Scalar TextViewer::mapLineToViewportBpd(Index line, bool fullSearch) const {
	const PhysicalFourSides<Scalar> spaces(spaceWidths());
	if(line == scrolls_.firstVisibleLine.line) {
		if(scrolls_.firstVisibleLine.subline == 0)
			return spaces.top;
		else
			return fullSearch ? spaces.top
				- static_cast<Scalar>(renderer_->defaultFont()->metrics().linePitch() * scrolls_.firstVisibleLine.subline) : numeric_limits<Scalar>::min();
	} else if(line > scrolls_.firstVisibleLine.line) {
		const Scalar lineSpan = renderer_->defaultFont()->metrics().linePitch();
		const NativeRectangle clientBounds(bounds(false));
		Scalar y = spaces.top;
		y += lineSpan * static_cast<Scalar>(
			renderer_->layouts().numberOfSublinesOfLine(scrolls_.firstVisibleLine.line) - scrolls_.firstVisibleLine.subline);
		for(Index i = scrolls_.firstVisibleLine.line + 1; i < line; ++i) {
			y += lineSpan * static_cast<Scalar>(renderer_->layouts().numberOfSublinesOfLine(i));
			if(y >= geometry::dy(clientBounds) && !fullSearch)
				return numeric_limits<Scalar>::max();
		}
		return y;
	} else if(!fullSearch)
		return numeric_limits<Scalar>::min();
	else {
		const Scalar linePitch = renderer_->defaultFont()->metrics().linePitch();
		Scalar y = spaces.top - static_cast<Scalar>(linePitch * scrolls_.firstVisibleLine.subline);
		for(Index i = scrolls_.firstVisibleLine.line - 1; ; --i) {
			y -= static_cast<Scalar>(renderer_->layouts().numberOfSublinesOfLine(i) * linePitch);
			if(i == line)
				break;
		}
		return y;
	}
}
#endif

/// @see CaretStateListener#matchBracketsChanged
void TextViewer::matchBracketsChanged(const Caret& self, const boost::optional<pair<k::Position, k::Position>>& oldPair, bool outsideOfView) {
	const boost::optional<pair<k::Position, k::Position>>& newPair = self.matchBrackets();
	if(newPair) {
		redrawLine(newPair->first.line);
		if(!isFrozen())
			redrawScheduledRegion();
		if(newPair->second.line != newPair->first.line) {
			redrawLine(newPair->second.line);
			if(!isFrozen())
				redrawScheduledRegion();
		}
		if(oldPair	// clear the previous highlight
				&& oldPair->first.line != newPair->first.line && oldPair->first.line != newPair->second.line) {
			redrawLine(oldPair->first.line);
			if(!isFrozen())
				redrawScheduledRegion();
		}
		if(oldPair && oldPair->second.line != newPair->first.line
				&& oldPair->second.line != newPair->second.line && oldPair->second.line != oldPair->first.line)
			redrawLine(oldPair->second.line);
	} else {
		if(oldPair) {	// clear the previous highlight
			redrawLine(oldPair->first.line);
			if(!isFrozen())
				redrawScheduledRegion();
			if(oldPair->second.line != oldPair->first.line)
				redrawLine(oldPair->second.line);
		}
	}
}

/// @see Widget#mouseDoubleClicked
void TextViewer::mouseDoubleClicked(const MouseButtonInput& input) {
	if(allowsMouseInput() && mouseInputStrategy_.get() != nullptr)
		mouseInputStrategy_->mouseButtonInput(MouseInputStrategy::DOUBLE_CLICKED, input);
}

/// @see Widget#mouseMoved
void TextViewer::mouseMoved(const LocatedUserInput& input) {
	cursorVanisher_.restore();
	if(allowsMouseInput() && mouseInputStrategy_.get() != nullptr)
		mouseInputStrategy_->mouseMoved(input);
}

/// @see Widget#mousePressed
void TextViewer::mousePressed(const MouseButtonInput& input) {
	cursorVanisher_.restore();
	if(allowsMouseInput() && mouseInputStrategy_.get() != nullptr)
		mouseInputStrategy_->mouseButtonInput(MouseInputStrategy::PRESSED, input);
}

/// @see Widget#mouseReleased
void TextViewer::mouseReleased(const MouseButtonInput& input) {
	if(allowsMouseInput() || input.button() == UserInput::BUTTON3_DOWN)
		cursorVanisher_.restore();
	if(allowsMouseInput() && mouseInputStrategy_.get() != nullptr)
		mouseInputStrategy_->mouseButtonInput(MouseInputStrategy::RELEASED, input);
}

/// @see Widget#mouseWheelChanged
void TextViewer::mouseWheelChanged(const MouseWheelInput& input) {
	cursorVanisher_.restore();
	if(allowsMouseInput() && mouseInputStrategy_.get() != nullptr)
		mouseInputStrategy_->mouseWheelRotated(input);
}

/// @see CaretStateListener#overtypeModeChanged
void TextViewer::overtypeModeChanged(const Caret&) {
}

/// @see Widget#paint
void TextViewer::paint(PaintContext& context) {
	if(isFrozen())	// skip if frozen
		return;
	const NativeRectangle scheduledBounds(geometry::normalize(context.boundsToPaint()));
	if(geometry::isEmpty(scheduledBounds))	// skip if the region to paint is empty
		return;

//	Timer tm(L"TextViewer.paint");

	// paint the ruler
	rulerPainter_->paint(context);

	// paint the text area
	textRenderer().paint(context);
}

/**
 * Redraws the specified line on the view.
 * If the viewer is frozen, redraws after unfrozen.
 * @param line The line to be redrawn
 * @param following Set @c true to redraw also the all lines follow to @a line
 */
void TextViewer::redrawLine(Index line, bool following) {
	redrawLines(makeRange(line, following ? numeric_limits<Index>::max() : line + 1));
}

/**
 * Redraws the specified lines on the view. If the viewer is frozen, redraws after unfrozen.
 * @param lines The lines to be redrawn. The last line (@a lines.end()) is exclusive and this line
 *              will not be redrawn. If this value is @c std#numeric_limits<Index>#max(), this
 *              method redraws the first line (@a lines.beginning()) and the following all lines
 * @throw kernel#BadRegionException @a lines intersects outside of the document
 */
void TextViewer::redrawLines(const Range<Index>& lines) {
//	checkInitialization();

	if(lines.end() != numeric_limits<Index>::max() && lines.end() >= document().numberOfLines())
		throw k::BadRegionException(k::Region(k::Position(lines.beginning(), 0), k::Position(lines.end(), 0)));

	if(isFrozen()) {
		freezeRegister_.addLinesToRedraw(lines);
		return;
	}

	if(lines.end() - 1 < textRenderer().viewport()->firstVisibleLineInLogicalNumber())
		return;

#ifdef _DEBUG
	if(DIAGNOSE_INHERENT_DRAWING)
		win32::DumpContext()
			<< L"@TextViewer.redrawLines invalidates lines ["
			<< static_cast<unsigned long>(lines.beginning())
			<< L".." << static_cast<unsigned long>(lines.end()) << L"]\n";
#endif // _DEBUG

	const WritingMode writingMode(textRenderer().writingMode());
	const NativeRectangle viewport(bounds(false));
	AbstractFourSides<Scalar> abstractBounds;
	mapPhysicalToAbstract(writingMode, viewport, viewport, abstractBounds);

	// calculate before and after edges of a rectangle to redraw
	BaselineIterator baseline(*textRenderer().viewport(), lines.beginning(), false);
	if(*baseline != numeric_limits<Scalar>::min())
		abstractBounds.before() = mapTextAreaBpdToLocal(*this, *baseline)
			- textRenderer().layouts().at(lines.beginning()).lineMetrics(0).ascent();
	baseline += length(lines);
	if(*baseline != numeric_limits<Scalar>::max())
		abstractBounds.after() = mapTextAreaBpdToLocal(*this, *baseline)
			+ textRenderer().layouts().at(baseline.line()).extent().end();
	NativeRectangle boundsToRedraw(viewport);
	mapAbstractToPhysical(writingMode, viewport, abstractBounds, boundsToRedraw);

	scheduleRedraw(boundsToRedraw, false);
}

/**
 * Removes the display size listener.
 * @param listener The listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
void TextViewer::removeDisplaySizeListener(DisplaySizeListener& listener) {
	displaySizeListeners_.remove(listener);
}

/**
 * Removes the viewport listener.
 * @param listener The listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
void TextViewer::removeViewportListener(ViewportListener& listener) {
	viewportListeners_.remove(listener);
}

/// Redraws the ruler.
void TextViewer::repaintRuler() {
	NativeRectangle r(bounds(false));
	if(utils::isRulerLeftAligned(*this))
		geometry::range<geometry::X_COORDINATE>(r) =
			makeRange(geometry::left(r), geometry::left(r) + rulerPainter_->allocationWidth());
	else
		geometry::range<geometry::X_COORDINATE>(r) =
			makeRange(geometry::right(r) - rulerPainter_->allocationWidth(), geometry::right(r));
	scheduleRedraw(r, false);
}

/// @see Widget#resized
void TextViewer::resized(State state, const NativeSize&) {
	utils::closeCompletionProposalsPopup(*this);
	if(state == MINIMIZED)
		return;
	if(renderer_.get() == nullptr)
		return;
	textRenderer().viewport()->setBoundsInView(textAreaContentRectangle());
}

/// @see CaretStateListener#selectionShapeChanged
void TextViewer::selectionShapeChanged(const Caret& self) {
	if(!isFrozen() && !isSelectionEmpty(self))
		redrawLines(makeRange(line(self.beginning()), line(self.end()) + 1));
}

/**
 * Updates the configurations.
 * @param general The general configurations. @c null to unchange
 * @param ruler The configurations about the ruler. @c null to unchange
 * @param synchronizeUI Set @c true to change the window style according to the new style. This
 *                      sets @c WS_EX_LEFTSCROLLBAR, @c WS_EX_RIGHTSCROLLBAR, @c WS_EX_LTRREADING
 *                      and @c WS_EX_RTLREADING styles
 * @throw UnknownValueException The content of @a verticalRuler is invalid
 */
void TextViewer::setConfiguration(const Configuration* general, const RulerConfiguration* ruler, bool synchronizeUI) {
	if(ruler != nullptr)
		rulerPainter_->setConfiguration(*ruler);
	if(general != nullptr) {
		const Inheritable<ReadingDirection> oldReadingDirection(configuration_.readingDirection);
		assert(!oldReadingDirection.inherits());
		configuration_ = *general;
		displaySizeListeners_.notify(&DisplaySizeListener::viewerDisplaySizeChanged);
		renderer_->layouts().invalidate();

//		if((oldReadingDirection == LEFT_TO_RIGHT && configuration_.readingDirection == RIGHT_TO_LEFT)
//				|| (oldReadingDirection == RIGHT_TO_LEFT && configuration_.readingDirection == LEFT_TO_RIGHT))
//			scrolls_.horizontal.position = scrolls_.horizontal.maximum
//				- scrolls_.horizontal.pageSize - scrolls_.horizontal.position + 1;
		scrolls_.resetBars(*this, 'a', false);
		updateScrollBars();

		if(!isFrozen() && (hasFocus() /*|| getHandle() == Viewer::completionWindow_->getSafeHwnd()*/)) {
			caret().resetVisualization();
			caret().updateLocation();
		}
		if(synchronizeUI) {
			LONG style = ::GetWindowLongW(identifier().get(), GWL_EXSTYLE);
			if(configuration_.readingDirection == LEFT_TO_RIGHT) {
				style &= ~(WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR);
				style |= WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR;
			} else {
				style &= ~(WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR);
				style |= WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR;
			}
			::SetWindowLongW(identifier().get(), GWL_EXSTYLE, style);
		}
	}
	scheduleRedraw(false);
}

/**
 * Sets the new content assistant.
 * @param newContentAssistant the content assistant to set. the ownership will be transferred to the callee.
 */
void TextViewer::setContentAssistant(unique_ptr<contentassist::ContentAssistant> newContentAssistant) /*throw()*/ {
	if(contentAssistant_.get() != nullptr)
		contentAssistant_->uninstall();	// $friendly-access
	(contentAssistant_ = std::move(newContentAssistant))->install(*this);	// $friendly-access
}

/**
 * Sets the mouse input strategy. An instance of @c TextViewer has the default strategy implemented
 * by @c DefaultMouseInputStrategy class as the construction.
 * @param newStrategy The new strategy or @c null
 * @param delegateOwnership Set @c true to transfer the ownership into the callee
 * @throw IllegalStateException The window is not created yet
 */
void TextViewer::setMouseInputStrategy(shared_ptr<MouseInputStrategy> newStrategy) {
//	checkInitialization();
	if(mouseInputStrategy_.get() != nullptr) {
		mouseInputStrategy_->interruptMouseReaction(false);
		mouseInputStrategy_->uninstall();
		dropTargetHandler_.reset();
	}
	if(newStrategy.get() != nullptr)
		mouseInputStrategy_ = newStrategy;
	else
		mouseInputStrategy_.reset(new DefaultMouseInputStrategy());	// TODO: the two parameters don't have rationales.
	mouseInputStrategy_->install(*this);
	dropTargetHandler_ = mouseInputStrategy_->handleDropTarget();
}

/**
 * Shows the tool tip at the cursor position.
 * @param text the text to be shown. CRLF represents a line break. this can not contain any NUL character
 * @param timeToWait the time to wait in milliseconds. -1 to use the system default value
 * @param timeRemainsVisible the time remains visible in milliseconds. -1 to use the system default value
 */
void TextViewer::showToolTip(const String& text, unsigned long timeToWait /* = -1 */, unsigned long timeRemainsVisible /* = -1 */) {
//	checkInitialization();

	delete[] tipText_;
	tipText_ = new wchar_t[text.length() + 1];
	hideToolTip();
	if(timeToWait == -1)
		timeToWait = ::GetDoubleClickTime();
	wcscpy(tipText_, text.c_str());
	::SetTimer(identifier().get(), TIMERID_CALLTIP, timeToWait, nullptr);
}

#ifndef ASCENSION_NO_TEXT_SERVICES_FRAMEWORK
HRESULT TextViewer::startTextServices() {
	assertValid();
	ComPtr<ITfThreadMgr> threadManager;
	HRESULT hr = threadManager.createInstance(CLSID_TF_ThreadMgr, 0, CLSCTX_INPROC_SERVER);
	if(FAILED(hr))
		return hr;
	ComPtr<ITfDocumentMgr> documentManager;
	hr = threadManager->CreateDocumentMgr(&documentManager);
	if(FAILED(hr))
		return hr;
	ComPtr<ITfContext> context;
	documentManager->CreateContext(...);
	...
}
#endif // !ASCENSION_NO_TEXT_SERVICES_FRAMEWORK

/**
 * Returns the 'allocation-rectangle' of the text area, in local-coordinates.
 * @see #bounds, #textAreaContentRectangle
 */
NativeRectangle TextViewer::textAreaAllocationRectangle() const /*throw()*/ {
	const NativeRectangle window(bounds(false));
	PhysicalFourSides<Scalar> result(window);
	switch(rulerPainter_->alignment()) {
		case detail::RulerPainter::LEFT:
			result.left() += rulerPainter_->allocationWidth();
			break;
		case detail::RulerPainter::TOP:
			result.top() += rulerPainter_->allocationWidth();
			break;
		case detail::RulerPainter::RIGHT:
			result.right() -= rulerPainter_->allocationWidth();
			break;
		case detail::RulerPainter::BOTTOM:
			result.bottom() -= rulerPainter_->allocationWidth();
			break;
		default:
			ASCENSION_ASSERT_NOT_REACHED();
	}
	return geometry::make<NativeRectangle>(
		geometry::make<NativePoint>(result.left(), result.top()),
		geometry::make<NativeSize>(result.right() - result.left(), result.bottom() - result.top()));
}

/**
 * Returns the 'content-rectangle' of the text area, in local-coordinates.
 * @see #bounds, #textAreaAllocationRectangle
 */
NativeRectangle TextViewer::textAreaContentRectangle() const /*throw()*/ {
	// TODO: Consider 'padding-start' setting.
	return textAreaAllocationRectangle();
}

/**
 * Revokes the frozen state of the viewer.
 * @throw WindowNotInitialized The window is not initialized
 * @see #freeze, #isFrozen
 */
void TextViewer::unfreeze() {
//	checkInitialization();
	if(freezeRegister_.isFrozen()) {
		const Range<Index> linesToRedraw(freezeRegister_.unfreeze());
		if(!freezeRegister_.isFrozen()) {
			if(scrolls_.changed) {
				updateScrollBars();
				scheduleRedraw(false);
			} else if(!isEmpty(linesToRedraw))
				redrawLines(linesToRedraw);

			rulerPainter_->update();

			caretMoved(caret(), caret().selectedRegion());
			redrawScheduledRegion();
		}
	}
}

/// Updates the scroll information.
void TextViewer::updateScrollBars() {
//	checkInitialization();
	if(renderer_.get() == nullptr || isFrozen())
		return;
	const shared_ptr<TextViewport> viewport(textRenderer().viewport());
	const LineLayoutVector& layouts = textRenderer().layouts();
	const NativePoint positions(physicalScrollPosition(*this));
	NativePoint endPositions(geometry::make<NativePoint>(layouts.maximumMeasure(),
		static_cast<geometry::Coordinate<NativePoint>::Type>(layouts.numberOfVisualLines())));
	NativePoint pageSteps(geometry::make<NativePoint>(
		static_cast<geometry::Coordinate<NativePoint>::Type>(viewport->numberOfVisibleCharactersInLine()),
		static_cast<geometry::Coordinate<NativePoint>::Type>(viewport->numberOfVisibleLines())));
	if(isVertical(textRenderer().writingMode().blockFlowDirection)) {
		geometry::transpose(endPositions);
		geometry::transpose(pageSteps);
	}

#define ASCENSION_GET_SCROLL_MINIMUM(endPosition, pageStep)	(endPosition/* * rate*/ - pageStep + 1)

	// about horizontal scroll bar
	bool wasNeededScrollbar = ASCENSION_GET_SCROLL_MINIMUM(geometry::x(endPositions), geometry::x(pageSteps)) > 0;
	// scroll to leftmost/rightmost before the scroll bar vanishes
	ScrollPosition minimum = ASCENSION_GET_SCROLL_MINIMUM(geometry::x(endPositions), geometry::x(pageSteps));
	if(wasNeededScrollbar && minimum <= 0) {
//		scrolls_.horizontal.position = 0;
		if(!isFrozen()) {
			scheduleRedraw(false);
			caret().updateLocation();
		}
	} else if(static_cast<ScrollPosition>(geometry::x(positions)) > minimum)
		viewport->scrollTo(geometry::make<NativePoint>(minimum, geometry::y(positions)), this);
	assert(ASCENSION_GET_SCROLL_MINIMUM(geometry::x(endPositions), geometry::x(pageSteps)) > 0 || geometry::x(positions) == 0);
	if(!isFrozen()) {
		ScrollProperties<ScrollPosition>& scrollBar = horizontalScrollBar();
		scrollBar.setRange(makeRange<ScrollPosition>(0, geometry::x(endPositions)));
		scrollBar.setPageStep(geometry::x(pageSteps));
		scrollBar.setPosition(geometry::x(positions));
	}

	// about vertical scroll bar
	wasNeededScrollbar = ASCENSION_GET_SCROLL_MINIMUM(geometry::y(endPositions), geometry::y(pageSteps)) > 0;
	minimum = ASCENSION_GET_SCROLL_MINIMUM(geometry::y(endPositions), geometry::y(pageSteps));
	// validate scroll position
	if(minimum <= 0) {
//		scrolls_.vertical.position = 0;
//		scrolls_.firstVisibleLine = VisualLine(0, 0);
		if(!isFrozen()) {
			scheduleRedraw(false);
			caret().updateLocation();
		}
	} else if(static_cast<ScrollPosition>(geometry::y(positions)) > minimum)
		viewport->scrollTo(geometry::make<NativePoint>(geometry::x(positions), minimum), this);
	assert(ASCENSION_GET_SCROLL_MINIMUM(geometry::y(endPositions), geometry::y(pageSteps)) > 0 || geometry::y(positions) == 0);
	if(!isFrozen()) {
		ScrollProperties<ScrollPosition>& scrollBar = verticalScrollBar();
		scrollBar.setRange(makeRange<ScrollPosition>(0, geometry::y(endPositions)));
		scrollBar.setPageStep(geometry::y(pageSteps));
		scrollBar.setPosition(geometry::y(positions));
	}

	scrolls_.changed = isFrozen();

#undef ASCENSION_GET_SCROLL_MINIMUM
}

/// @see TextViewportListener#viewportPositionChanged
void TextViewer::viewportPositionChanged(const VisualLine& oldLine, Index oldInlineProgressionOffset) {
	if(isFrozen()) {
		scrolls_.changed = true;
		return;
	}
	const shared_ptr<const TextViewport> viewport(textRenderer().viewport());
	// TODO: this code ignores orientation in vertical layout.
	switch(textRenderer().writingMode().blockFlowDirection) {
		case HORIZONTAL_TB:
			horizontalScrollBar().setPosition(viewport->inlineProgressionOffset());
			verticalScrollBar().setPosition(viewport->firstVisibleLineInVisualNumber());
			break;
		case VERTICAL_RL:
			horizontalScrollBar().setPosition(horizontalScrollBar().range().end()
				- horizontalScrollBar().pageStep() - viewport->firstVisibleLineInVisualNumber());
			verticalScrollBar().setPosition(viewport->inlineProgressionOffset());
			break;
		case VERTICAL_LR:
			horizontalScrollBar().setPosition(viewport->firstVisibleLineInVisualNumber());
			verticalScrollBar().setPosition(viewport->inlineProgressionOffset());
			break;
		default:
			ASCENSION_ASSERT_NOT_REACHED();
	}

//	closeCompletionProposalsPopup(*this);
	hideToolTip();

	// scroll the ruler
	rulerPainter_->scroll(oldLine);

	// repaint
	redrawScheduledRegion();
}

/// @see TextViewportListener#viewportSizeChanged
void TextViewer::viewportSizeChanged(const NativeSize& oldSize) {
#ifdef ASCENSION_WINDOW_SYSTEM_WIN32
	// notify the tooltip
	win32::AutoZeroSize<TOOLINFOW> ti;
	const NativeRectangle viewerBounds(bounds(false));
	ti.hwnd = identifier().get();
	ti.uId = 1;
	ti.rect = viewerBounds;
	::SendMessageW(toolTip_, TTM_NEWTOOLRECT, 0, reinterpret_cast<LPARAM>(&ti));
#endif // ASCENSION_WINDOW_SYSTEM_WIN32
	textRenderer().setTextWrapping(textRenderer().textWrapping(), createRenderingContext().get());
	scrolls_.resetBars(*this, 'a', true);
	updateScrollBars();
	rulerPainter_->update();
	if(rulerPainter_->alignment() != detail::RulerPainter::LEFT && rulerPainter_->alignment() != detail::RulerPainter::TOP) {
//		recreateCaret();
//		redrawVerticalRuler();
		scheduleRedraw(false);	// hmm...
	}
}

/// @see VisualLinesListener#visualLinesDeleted
void TextViewer::visualLinesDeleted(const Range<Index>& lines, Index sublines, bool longestLineChanged) /*throw()*/ {
	scrolls_.changed = true;
	const shared_ptr<const TextViewport> viewport(textRenderer().viewport());
	if(lines.end() < viewport->firstVisibleLineInLogicalNumber()) {	// deleted before visible area
//		scrolls_.firstVisibleLine.line -= length(lines);
//		scrolls_.vertical.position -= static_cast<int>(sublines);
//		scrolls_.vertical.maximum -= static_cast<int>(sublines);
		repaintRuler();
	} else if(lines.beginning() > viewport->firstVisibleLineInLogicalNumber()	// deleted the first visible line and/or after it
			|| (lines.beginning() == viewport->firstVisibleLineInLogicalNumber() && viewport->firstVisibleSublineInLogicalLine() == 0)) {
//		scrolls_.vertical.maximum -= static_cast<int>(sublines);
		redrawLine(lines.beginning(), true);
	} else {	// deleted lines contain the first visible line
//		scrolls_.firstVisibleLine.line = lines.beginning();
//		scrolls_.updateVertical(*this);
		redrawLine(lines.beginning(), true);
	}
	if(longestLineChanged)
		scrolls_.resetBars(*this, 'i', false);
}

/// @see VisualLinesListener#visualLinesInserted
void TextViewer::visualLinesInserted(const Range<Index>& lines) /*throw()*/ {
	scrolls_.changed = true;
	const shared_ptr<const TextViewport> viewport(textRenderer().viewport());
	if(lines.end() < viewport->firstVisibleLineInLogicalNumber()) {	// inserted before visible area
//		scrolls_.firstVisibleLine.line += length(lines);
//		scrolls_.vertical.position += static_cast<int>(length(lines));
//		scrolls_.vertical.maximum += static_cast<int>(length(lines));
		repaintRuler();
	} else if(lines.beginning() > viewport->firstVisibleLineInLogicalNumber()	// inserted at or after the first visible line
			|| (lines.beginning() == viewport->firstVisibleLineInLogicalNumber() && viewport->firstVisibleSublineInLogicalLine() == 0)) {
//		scrolls_.vertical.maximum += static_cast<int>(length(lines));
		redrawLine(lines.beginning(), true);
	} else {	// inserted around the first visible line
//		scrolls_.firstVisibleLine.line += length(lines);
//		scrolls_.updateVertical(*this);
		redrawLine(lines.beginning(), true);
	}
}

/// @see VisualLinesListener#visualLinesModified
void TextViewer::visualLinesModified(const Range<Index>& lines,
		SignedIndex sublinesDifference, bool documentChanged, bool longestLineChanged) /*throw()*/ {
	if(sublinesDifference == 0)	// number of visual lines was not changed
		redrawLines(lines);
	else {
		const shared_ptr<const TextViewport> viewport(textRenderer().viewport());
		scrolls_.changed = true;
		if(lines.end() < viewport->firstVisibleLineInLogicalNumber()) {	// changed before visible area
//			scrolls_.vertical.position += sublinesDifference;
//			scrolls_.vertical.maximum += sublinesDifference;
			repaintRuler();
		} else if(lines.beginning() > viewport->firstVisibleLineInLogicalNumber()	// changed at or after the first visible line
				|| (lines.beginning() == viewport->firstVisibleLineInLogicalNumber() && viewport->firstVisibleSublineInLogicalLine() == 0)) {
//			scrolls_.vertical.maximum += sublinesDifference;
			redrawLine(lines.beginning(), true);
		} else {	// changed lines contain the first visible line
//			scrolls_.updateVertical(*this);
			redrawLine(lines.beginning(), true);
		}
	}
	if(longestLineChanged) {
		scrolls_.resetBars(*this, 'i', false);
		scrolls_.changed = true;
	}
	if(!documentChanged && scrolls_.changed)
		updateScrollBars();
}


// AutoFreeze /////////////////////////////////////////////////////////////////////////////////////

/**
 * @class ascension::viewers::AutoFreeze
 *
 * Calls automatically @c TextViewer#freeze and @c TextViewer#unfreeze.
 *
 * @code
 * extern TextViewer* target;
 * AutoFreeze af(target);
 * target-&gt;mayThrow();
 * // target-&gt;unfreeze() will be called automatically
 * @endcode
 *
 * @note This class is not intended to be subclassed.
 */

/**
 * Constructor calls @c TextViewer#freeze.
 * @param textViewer The text viewer this object manages. If this is @c null, the object does
 *                   nothing at all
 * @throw ... Any exceptions @c TextViewer#freeze throws
 */
AutoFreeze::AutoFreeze(TextViewer* textViewer) : textViewer_(textViewer) {
	if(textViewer_ != nullptr)
		textViewer_->freeze();
}

/// Destructor calls @c TextViewer#unfreeze.
AutoFreeze::~AutoFreeze() /*throw()*/ {
	try {
		if(textViewer_ != nullptr)
			textViewer_->unfreeze();
	} catch(...) {
		// ignore
	}
}


// TextViewer.CursorVanisher //////////////////////////////////////////////////////////////////////

TextViewer::CursorVanisher::CursorVanisher() /*throw()*/ : viewer_(nullptr) {
}

TextViewer::CursorVanisher::~CursorVanisher() /*throw()*/ {
	restore();
}

void TextViewer::CursorVanisher::install(TextViewer& viewer) {
	assert(viewer_ == nullptr);
	viewer_ = &viewer;
}

void TextViewer::CursorVanisher::restore() {
	if(vanished_) {
		base::Cursor::show();
		viewer_->releaseInput();
		vanished_ = false;
	}
}

void TextViewer::CursorVanisher::vanish() {
	if(!vanished_ && viewer_->configuration().vanishesCursor && viewer_->hasFocus()) {
		vanished_ = true;
		base::Cursor::hide();
		viewer_->grabInput();
	}
}

bool TextViewer::CursorVanisher::vanished() const {
	return vanished_;
}


// TextViewer.Renderer ////////////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param viewer The text viewer
 * @param writingMode The initial writing mode
 */
TextViewer::Renderer::Renderer(TextViewer& viewer, const WritingMode& writingMode) :
		TextRenderer(viewer.presentation(), systemFonts(),
		geometry::size(viewer.textAreaContentRectangle())), viewer_(viewer), defaultWritingMode_(writingMode) {
	// TODO: other FontCollection object used?
#if 0
	// for test
	setSpecialCharacterRenderer(new DefaultSpecialCharacterRenderer, true);
#endif
}

/**
 * Copy-constructor with a parameter.
 * @param other The source object
 * @param viewer The text viewer
 */
TextViewer::Renderer::Renderer(const Renderer& other, TextViewer& viewer) :
		TextRenderer(other), viewer_(viewer), defaultWritingMode_(other.defaultWritingMode_) {
}

/// @see TextRenderer#defaultUIWritingMode
const WritingMode& TextViewer::Renderer::defaultUIWritingMode() const /*throw()*/ {
	return defaultWritingMode_;
}

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
/// Rewraps the visual lines at the window's edge.
void TextViewer::Renderer::rewrapAtWindowEdge() {
	class Local {
	public:
		explicit Local(Scalar newMeasure) : newMeasure_(newMeasure) {}
		bool operator()(const LineLayoutVector::LineLayout& layout) const {
			return layout.second->numberOfLines() != 1
				|| layout.second->style().justification == NO_JUSTIFICATION
				|| layout.second->measure() > newMeasure_;
		}
	private:
		const Scalar newMeasure_;
	};

	if(viewer_.configuration().lineWrap.wrapsAtWindowEdge()) {
		const NativeRectangle clientBounds(viewer_.bounds(false));
		const PhysicalFourSides<Scalar>& spaces(viewer_.spaceWidths());
		if(isHorizontal(viewer_.textRenderer().writingMode().blockFlowDirection))
			layouts().invalidateIf(Local(geometry::dx(clientBounds) - spaces.left - spaces.right));
		else
			layouts().invalidateIf(Local(geometry::dy(clientBounds) - spaces.top - spaces.bottom));
	}
}
#endif // ASCENSION_ABANDONED_AT_VERSION_08

/**
 * Sets the default UI writing mode.
 * @param writingMode The writing mode
 */
void TextViewer::Renderer::setDefaultWritingMode(const WritingMode& writingMode) /*throw()*/ {
	if(writingMode != defaultWritingMode_) {
		defaultWritingMode_ = writingMode;
		layouts().invalidate();
	}
}

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
/// @see TextRenderer#width
Scalar TextViewer::Renderer::width() const /*throw()*/ {
	const LineWrapConfiguration& lwc = viewer_.configuration().lineWrap;
	if(!lwc.wraps())
		return (viewer_.horizontalScrollBar().range().end() + 1) * viewer_.textRenderer().defaultFont()->metrics().averageCharacterWidth();
	else if(lwc.wrapsAtWindowEdge()) {
		const NativeRectangle clientBounds(viewer_.bounds(false));
		const PhysicalFourSides<Scalar>& spaces(viewer_.spaceWidths());
		return isHorizontal(viewer_.textRenderer().writingMode().blockFlowDirection) ?
			(geometry::dx(clientBounds) - spaces.left - spaces.right) : (geometry::dy(clientBounds) - spaces.top - spaces.bottom);
	} else
		return lwc.width;
}
#endif // ASCENSION_ABANDONED_AT_VERSION_08


// TextViewer.Configuration /////////////////////////////////////////////////

/// Default constructor.
TextViewer::Configuration::Configuration() /*throw()*/ :
		readingDirection(LEFT_TO_RIGHT), usesRichTextClipboardFormat(false) {
#if(_WIN32_WINNT >= 0x0501)
	BOOL b;
	if(::SystemParametersInfoW(SPI_GETMOUSEVANISH, 0, &b, 0) != 0)
		vanishesCursor = win32::boole(b);
	else
		vanishesCursor = false;
#else
	vanishesCursor = false;
#endif // _WIN32_WINNT >= 0x0501
}


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
bool VirtualBox::characterRangeInVisualLine(const VisualLine& line, Range<Index>& range) const /*throw()*/ {
//	assert(viewer_.isWindow());
	const Point& top = beginning();
	const Point& bottom = end();
	if(line < top.line || line > bottom.line)	// out of the region
		return false;
	else {
		const TextRenderer& renderer = viewer_.textRenderer();
		const TextLayout& layout = renderer.layouts().at(line.line);
		const Scalar bpd = layout.baseline(line.subline);
		range = makeRange(
			layout.offset(geometry::make<NativePoint>(
				viewer_.mapViewportIpdToLineLayout(line.line, points_[0].ipd), bpd)).first,
			layout.offset(geometry::make<NativePoint>(
				viewer_.mapViewportIpdToLineLayout(line.line, points_[1].ipd), bpd)).first);
		return !isEmpty(range);
	}
}

/**
 * Returns if the specified point is on the virtual box.
 * @param p The point in local coordinates
 * @return @c true If the point is on the virtual box
 * @see #characterRangeInVisualLine
 */
bool VirtualBox::includes(const NativePoint& p) const /*throw()*/ {
	// TODO: This code can't handle vertical writing-mode.
//	assert(viewer_.isWindow());
	if(viewer_.hitTest(p) == TextViewer::TEXT_AREA_CONTENT_RECTANGLE) {	// ignore if not in text area
		// about inline-progression-direction
		const bool horizontal = isHorizontal(viewer_.textRenderer().writingMode().blockFlowDirection);
		const Scalar ipd = (horizontal ? geometry::x(p) : geometry::y(p)) - viewer_.inlineProgressionOffsetInViewport();	// $friendly-access
		if(ascension::includes(makeRange(startEdge(), endEdge()), ipd)) {
			// about block-progression-direction
			const shared_ptr<const TextViewport> viewport(viewer_.textRenderer().viewport());
			graphics::NativePoint pointInViewport(p);
			const graphics::NativeRectangle viewportBoundsInView(viewport->boundsInView());
			geometry::translate(pointInViewport, geometry::make<NativeSize>(
				geometry::left(viewportBoundsInView), geometry::top(viewportBoundsInView)));
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
	Point newPoints[2];
	const TextRenderer& r = viewer_.textRenderer();
	const bool horizontal = isHorizontal(viewer_.textRenderer().writingMode().blockFlowDirection);

	// first
	const TextLayout* layout = &r.layouts().at(newPoints[0].line.line = region.first.line);
	graphics::NativePoint location(layout->location(region.first.offsetInLine));
	newPoints[0].ipd = viewer_.mapLineLayoutIpdToViewport(
		newPoints[0].line.line, horizontal ? static_cast<Scalar>(geometry::x(location)) : geometry::y(location));
	newPoints[0].line.subline = layout->lineAt(region.first.offsetInLine);

	// second
	layout = &r.layouts().at(newPoints[1].line.line = region.second.line);
	location = layout->location(region.second.offsetInLine);
	newPoints[1].ipd = viewer_.mapLineLayoutIpdToViewport(
		newPoints[1].line.line, horizontal ? static_cast<Scalar>(geometry::x(location)) : geometry::y(location));
	newPoints[1].line.subline = layout->lineAt(region.second.offsetInLine);

	// commit
	points_[0] = newPoints[0];
	points_[1] = newPoints[1];
}


// CurrentLineHighlighter ///////////////////////////////////////////////////

/**
 * @class ascension::presentation::CurrentLineHighlighter
 * Highlights a line the caret is on with the specified background color.
 *
 * Because an instance automatically registers itself as a line color director, you should not call
 * @c Presentation#addLineColorDirector method. Usual usage is as follows.
 *
 * @code
 * Caret& caret = ...;
 * new CurrentLineHighlighter(caret);
 * @endcode
 *
 * When the caret has a selection, highlight is canceled.
 */

/// The priority value this class returns.
const TextLineColorDirector::Priority CurrentLineHighlighter::LINE_COLOR_PRIORITY = 0x40;

/**
 * Constructor.
 * @param caret The caret
 * @param foreground The initial foreground color
 * @param background The initial background color
 */
CurrentLineHighlighter::CurrentLineHighlighter(Caret& caret,
		const Color& foreground, const Color& background) : caret_(&caret), foreground_(foreground), background_(background) {
	shared_ptr<TextLineColorDirector> temp(this);
	caret.textViewer().presentation().addTextLineColorDirector(temp);
	caret.addListener(*this);
	caret.addStateListener(*this);
	caret.addLifeCycleListener(*this);
}

/// Destructor.
CurrentLineHighlighter::~CurrentLineHighlighter() /*throw()*/ {
	if(caret_ != nullptr) {
		caret_->removeListener(*this);
		caret_->removeStateListener(*this);
		caret_->textViewer().presentation().removeTextLineColorDirector(*this);
	}
}

/// Returns the background color.
const Color& CurrentLineHighlighter::background() const /*throw()*/ {
	return background_;
}

/// @see CaretListener#caretMoved
void CurrentLineHighlighter::caretMoved(const Caret&, const k::Region& oldRegion) {
	if(oldRegion.isEmpty()) {
		if(!isSelectionEmpty(*caret_) || line(*caret_) != oldRegion.first.line)
			caret_->textViewer().redrawLine(oldRegion.first.line, false);
	}
	if(isSelectionEmpty(*caret_)) {
		if(!oldRegion.isEmpty() || line(*caret_) != oldRegion.first.line)
			caret_->textViewer().redrawLine(line(*caret_), false);
	}
}

/// Returns the foreground color.
const Color& CurrentLineHighlighter::foreground() const /*throw()*/ {
	return foreground_;
}

/// @see CaretStateListener#matchBracketsChanged
void CurrentLineHighlighter::matchBracketsChanged(const Caret&, const boost::optional<pair<k::Position, k::Position>>&, bool) {
}

/// @see CaretStateListener#overtypeModeChanged
void CurrentLineHighlighter::overtypeModeChanged(const Caret&) {
}

/// @see PointLifeCycleListener#pointDestroyed
void CurrentLineHighlighter::pointDestroyed() {
//	caret_->removeListener(*this);
//	caret_->removeStateListener(*this);
	caret_ = nullptr;
}

/// @see ILineColorDirector#queryLineColors
TextLineColorDirector::Priority CurrentLineHighlighter::queryLineColors(Index line, Color& foreground, Color& background) const {
	if(caret_ != nullptr && isSelectionEmpty(*caret_) && k::line(*caret_) == line && caret_->textViewer().hasFocus()) {
		foreground = foreground_;
		background = background_;
		return LINE_COLOR_PRIORITY;
	} else {
		foreground = background = Color();
		return 0;
	}
}

/// @see ICaretStateListener#selectionShapeChanged
void CurrentLineHighlighter::selectionShapeChanged(const Caret&) {
}

/**
 * Sets the background color and redraws the window.
 * @param color The background color to set
 */
void CurrentLineHighlighter::setBackground(const Color& color) /*throw()*/ {
	background_ = color;
}

/**
 * Sets the foreground color and redraws the window.
 * @param color The foreground color to set
 */
void CurrentLineHighlighter::setForeground(const Color& color) /*throw()*/ {
	foreground_ = color;
}


// ascension.viewers.utils free functions /////////////////////////////////////////////////////////

/// Closes the opened completion proposals popup immediately.
void utils::closeCompletionProposalsPopup(TextViewer& viewer) /*throw()*/ {
	if(contentassist::ContentAssistant* ca = viewer.contentAssistant()) {
		if(contentassist::ContentAssistant::CompletionProposalsUI* cpui = ca->completionProposalsUI())
			cpui->close();
	}
}

const hyperlink::Hyperlink* utils::getPointedHyperlink(const TextViewer& viewer, const k::Position& at) {
	size_t numberOfHyperlinks;
	if(const hyperlink::Hyperlink* const* hyperlinks = viewer.presentation().getHyperlinks(at.line, numberOfHyperlinks)) {
		for(size_t i = 0; i < numberOfHyperlinks; ++i) {
			if(at.offsetInLine >= hyperlinks[i]->region().beginning() && at.offsetInLine <= hyperlinks[i]->region().end())
				return hyperlinks[i];
		}
	}
	return nullptr;
}

/**
 * Toggles the inline flow direction of the text viewer.
 * @param viewer The text viewer
 */
void utils::toggleOrientation(TextViewer& viewer) /*throw()*/ {
	WritingMode writingMode(viewer.textRenderer().defaultUIWritingMode());
	writingMode.inlineFlowDirection =
		(writingMode.inlineFlowDirection == LEFT_TO_RIGHT) ? RIGHT_TO_LEFT : LEFT_TO_RIGHT;
	viewer.textRenderer().setDefaultWritingMode(writingMode);
//	viewer.synchronizeWritingModeUI();
//	if(config.lineWrap.wrapsAtWindowEdge()) {
//		win32::AutoZeroSize<SCROLLINFO> scroll;
//		viewer.getScrollInformation(SB_HORZ, scroll);
//		viewer.setScrollInformation(SB_HORZ, scroll);
//	}
}


// graphics.font.TextViewport /////////////////////////////////////////////////////////////////////

/**
 * 
 * This method does nothing if scroll is locked.
 * @param dbpd An offset to scroll in visual lines in block-progression-dimension
 * @param dipd An offset tp scroll in characters in inline-progression-dimension
 * @param widget The widget to scroll. This method schedules repainting about this widget. Can be
 *               @c nullptr
 */
void TextViewport::scroll(SignedIndex dbpd, SignedIndex dipd, Widget* widget) {
	if(lockCount_ != 0)
		return;

	// 1. preprocess parameters and update positions
	const VisualLine oldLine = firstVisibleLine_;
	const Index oldInlineProgressionOffset = inlineProgressionOffset();
	if(dbpd != 0) {
		const Index maximumBpd = textRenderer().layouts().numberOfVisualLines() - static_cast<Index>(numberOfVisibleLines());
		dbpd = max(min(dbpd,
			static_cast<SignedIndex>(maximumBpd - firstVisibleLineInVisualNumber()) + 1),
				-static_cast<SignedIndex>(firstVisibleLineInVisualNumber()));
		if(dbpd != 0) {
			scrollOffsets_.bpd += dbpd;
			textRenderer().layouts().offsetVisualLine(firstVisibleLine_, dbpd);
		}
	}
	if(dipd != 0) {
		const Index maximumIpd = contentMeasure()
			/ textRenderer().defaultFont()->metrics().averageCharacterWidth()
			- static_cast<Index>(numberOfVisibleCharactersInLine());
		dipd = max(min(dipd,
			static_cast<SignedIndex>(maximumIpd - inlineProgressionOffset()) + 1),
			-static_cast<SignedIndex>(inlineProgressionOffset()));
		if(dipd != 0)
			scrollOffsets_.ipd += dipd;
	}
	if(dbpd == 0 && dipd == 0)
		return;
	listeners_.notify<const VisualLine&, Index>(
		&TextViewportListener::viewportScrollPositionChanged, oldLine, oldInlineProgressionOffset);

	// 2. calculate numbers of pixels to scroll
	if(widget == nullptr)
		return;
	dipd *= textRenderer().defaultFont()->metrics().averageCharacterWidth();
	NativeSize pixelsToScroll;
	switch(textRenderer().writingMode().blockFlowDirection) {
		case HORIZONTAL_TB:
			pixelsToScroll = geometry::make<NativeSize>(dipd * textRenderer().defaultFont()->metrics().averageCharacterWidth(), dbpd);
			break;
		case VERTICAL_RL:
			pixelsToScroll = geometry::make<NativeSize>(-dbpd, dipd * textRenderer().defaultFont()->metrics().averageCharacterWidth());
			break;
		case VERTICAL_LR:
			pixelsToScroll = geometry::make<NativeSize>(+dbpd, dipd * textRenderer().defaultFont()->metrics().averageCharacterWidth());
			break;
		default:
			ASCENSION_ASSERT_NOT_REACHED();
	}

	// 3. scroll the graphics device
	const NativeRectangle& boundsToScroll = boundsInView();
	if(abs(dbpd) >= static_cast<SignedIndex>(numberOfVisibleLines())
			|| abs(dipd) >= static_cast<SignedIndex>(numberOfVisibleCharactersInLine()))
		widget->scheduleRedraw(boundsToScroll, false);	// repaint all if the amount of the scroll is over a page
	else {
		// scroll image by BLIT
		// TODO: direct call of Win32 API.
		::ScrollWindowEx(widget->identifier().get(),
			-geometry::dx(pixelsToScroll), -geometry::dy(pixelsToScroll), nullptr, &boundsToScroll, nullptr, nullptr, SW_INVALIDATE);
		// invalidate bounds newly entered into the viewport
		if(geometry::dx(pixelsToScroll) > 0)
			widget->scheduleRedraw(geometry::make<NativeRectangle>(
				geometry::topLeft(boundsToScroll),
				geometry::make<NativeSize>(geometry::dx(pixelsToScroll), geometry::dy(boundsToScroll))), false);
		else if(geometry::dx(pixelsToScroll) < 0)
			widget->scheduleRedraw(geometry::make<NativeRectangle>(
				geometry::topRight(boundsToScroll),
				geometry::make<NativeSize>(geometry::dx(pixelsToScroll), geometry::dy(boundsToScroll))), false);
		if(geometry::dy(pixelsToScroll) > 0)
			widget->scheduleRedraw(geometry::make<NativeRectangle>(
				geometry::topLeft(boundsToScroll),
				geometry::make<NativeSize>(geometry::dx(boundsToScroll), geometry::dy(pixelsToScroll))), false);
		else if(geometry::dy(pixelsToScroll) < 0)
			widget->scheduleRedraw(geometry::make<NativeRectangle>(
				geometry::bottomLeft(boundsToScroll),
				geometry::make<NativeSize>(geometry::dx(boundsToScroll), geometry::dy(pixelsToScroll))), false);
	}
}


// ascension.viewers.source free functions ////////////////////////////////////////////////////////

/**
 * Returns the identifier near the specified position in the document.
 * @param document The document
 * @param position The position
 * @param[out] startOffsetInLine The start offset in the line, of the identifier. Can be @c nullptr
 *                               if not needed
 * @param[out] endOffsetInLine The end offset in the line, of the identifier. Can be @c nullptr if
 *                             not needed
 * @return @c true if an identifier was found. @c false if not found and output paramter valuess
 *         are not defined in this case
 * @see #getPointedIdentifier
 */
bool source::getNearestIdentifier(const k::Document& document,
		const k::Position& position, Index* startOffsetInLine, Index* endOffsetInLine) {
	using namespace text;
	static const Index MAXIMUM_IDENTIFIER_HALF_LENGTH = 100;

	k::DocumentPartition partition;
	document.partitioner().partition(position, partition);
	const IdentifierSyntax& syntax = document.contentTypeInformation().getIdentifierSyntax(partition.contentType);
	Index start = position.offsetInLine, end = position.offsetInLine;

	// find the start of the identifier
	if(startOffsetInLine != nullptr) {
		k::DocumentCharacterIterator i(document,
			k::Region(max(partition.region.beginning(), k::Position(position.line, 0)), position), position);
		do {
			i.previous();
			if(!syntax.isIdentifierContinueCharacter(i.current())) {
				i.next();
				start = i.tell().offsetInLine;
				break;
			} else if(position.offsetInLine - i.tell().offsetInLine > MAXIMUM_IDENTIFIER_HALF_LENGTH)	// too long identifier
				return false;
		} while(i.hasPrevious());
		if(!i.hasPrevious())
			start = i.tell().offsetInLine;
		*startOffsetInLine = start;
	}

	// find the end of the identifier
	if(endOffsetInLine != nullptr) {
		k::DocumentCharacterIterator i(document, k::Region(position,
			min(partition.region.end(), k::Position(position.line, document.lineLength(position.line)))), position);
		while(i.hasNext()) {
			if(!syntax.isIdentifierContinueCharacter(i.current())) {
				end = i.tell().offsetInLine;
				break;
			}
			i.next();
			if(i.tell().offsetInLine - position.offsetInLine > MAXIMUM_IDENTIFIER_HALF_LENGTH)	// too long identifier
				return false;
		}
		if(!i.hasNext())
			end = i.tell().offsetInLine;
		*endOffsetInLine = end;
	}

	return true;
}
/**
 * Returns the identifier near the specified position in the document.
 * @param document The document
 * @param position The position
 * @return The found identifier or @c boost#none if not found
 * @see #getPointedIdentifier
 */
boost::optional<k::Region> source::getNearestIdentifier(const k::Document& document, const k::Position& position) {
	pair<Index, Index> offsetsInLine;
	if(getNearestIdentifier(document, position, &offsetsInLine.first, &offsetsInLine.second))
		return boost::make_optional(k::Region(position.line, offsetsInLine));
	else
		return boost::none;
}

/**
 * Returns the identifier near the cursor.
 * @param viewer The text viewer
 * @return The found identifier or @c boost#none if not found
 * @see #getNearestIdentifier
 */
boost::optional<k::Region> source::getPointedIdentifier(const TextViewer& viewer) {
//	if(viewer.isWindow()) {
		NativePoint cursorPoint;
		::GetCursorPos(&cursorPoint);
		viewer.mapFromGlobal(cursorPoint);
		return source::getNearestIdentifier(viewer.document(),
			viewToModel(*viewer.textRenderer().viewport(), cursorPoint, TextLayout::LEADING));
//	}
	return boost::none;
}

/**
 * Calls @c IncrementalSearcher#abort from @a viewer.
 * @return true if the incremental search was running
 */
bool texteditor::abortIncrementalSearch(TextViewer& viewer) /*throw()*/ {
	if(texteditor::Session* session = viewer.document().session()) {
		if(session->incrementalSearcher().isRunning())
			return session->incrementalSearcher().abort(), true;
	}
	return false;
}

/**
 * Calls @c IncrementalSearcher#end from @a viewer.
 * @return true if the incremental search was running
 */
bool texteditor::endIncrementalSearch(TextViewer& viewer) /*throw()*/ {
	if(texteditor::Session* session = viewer.document().session()) {
		if(session->incrementalSearcher().isRunning())
			return session->incrementalSearcher().end(), true;
	}
	return false;
}
