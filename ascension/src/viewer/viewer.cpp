/**
 * @file viewer.cpp
 * @author exeal
 * @date 2003-2006 was EditView.cpp and EditViewWindowMessages.cpp
 * @date 2006-2013
 */

#include <ascension/content-assist/content-assist.hpp>
#include <ascension/corelib/text/break-iterator.hpp>
#include <ascension/graphics/font/font-metrics.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/presentation/writing-mode-mappings.hpp>
#include <ascension/rules.hpp>
#include <ascension/text-editor/command.hpp>
#include <ascension/text-editor/session.hpp>
#include <ascension/viewer/widgetapi/cursor.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/default-mouse-input-strategy.hpp>
#include <ascension/viewer/viewer.hpp>
#include <limits>	// std.numeric_limit
#include <boost/foreach.hpp>

using namespace ascension;
using namespace ascension::viewers;
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
		const graphics::Rectangle textArea(viewer.textAreaAllocationRectangle());
		FlowRelativeFourSides<Scalar> abstractBounds;
		mapPhysicalToFlowRelative(viewer.presentation().computeWritingMode(&viewer.textRenderer()), textArea, textArea, abstractBounds);
		return bpd -= abstractBounds.before();
	}
	inline Scalar mapTextAreaBpdToLocal(const TextViewer& viewer, Scalar bpd) {
		const graphics::Rectangle textArea(viewer.textAreaAllocationRectangle());
		FlowRelativeFourSides<Scalar> abstractBounds;
		mapPhysicalToFlowRelative(viewer.presentation().computeWritingMode(&viewer.textRenderer()), textArea, textArea, abstractBounds);
		return bpd += abstractBounds.before();
	}
	/// @internal Maps the given point in viewer-local coordinates into a point in text-area coordinates.
	inline Point mapLocalToTextArea(const TextViewer& viewer, const Point& p) {
		const graphics::Rectangle textArea(viewer.textAreaAllocationRectangle());
		Point temp(p);
		return geometry::translate(temp, Dimension(geometry::_dx = -geometry::left(textArea), geometry::_dy = -geometry::top(textArea)));
	}
	/// @internal Maps the given point in text-area coordinates into a point in viewer-local coordinates.
	inline Point mapTextAreaToLocal(const TextViewer& viewer, const Point& p) {
		const graphics::Rectangle textArea(viewer.textAreaAllocationRectangle());
		Point temp(p);
		return geometry::translate(temp, Dimension(geometry::_dx = +geometry::left(textArea), geometry::_dy = +geometry::top(textArea)));
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
 * @param presentation The presentation object
 */
TextViewer::TextViewer(Presentation& presentation) : presentation_(presentation), mouseInputDisabledCount_(0) {
	initialize(nullptr);

	// initializations of renderer_ and mouseInputStrategy_ are in initializeWindow()
}

/**
 * Copy-constructor. Unlike @c win32#Object class, this does not copy the window handle. For
 * more details, see the description of @c TextViewer.
 */
TextViewer::TextViewer(const TextViewer& other) : presentation_(other.presentation_), mouseInputDisabledCount_(0) {
	initialize(&other);
	modeState_ = other.modeState_;
}

/// Destructor.
TextViewer::~TextViewer() {
	document().removeListener(*this);
	document().removeRollbackListener(*this);
	textRenderer().removeComputedBlockFlowDirectionListener(*this);
	textRenderer().removeDefaultFontListener(*this);
	textRenderer().layouts().removeVisualLinesListener(*this);
	caret().removeListener(*this);
	caret().removeStateListener(*this);
	BOOST_FOREACH(VisualPoint* p, points_)
		p->viewerDisposed();

	// 非共有データ
//	delete selection_;
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
	redrawLines(boost::irange(line(caret().beginning()), line(caret().end()) + 1));
	widgetapi::redrawScheduledRegion(*this);
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
	if(!widgetapi::isVisible(*this))
		return;
	const k::Region newRegion(self.selectedRegion());
	bool changed = false;

	// redraw the selected region
	if(self.isSelectionRectangle()) {	// rectangle
		if(!oldRegion.isEmpty())
			redrawLines(boost::irange(oldRegion.beginning().line, oldRegion.end().line + 1));
		if(!newRegion.isEmpty())
			redrawLines(boost::irange(newRegion.beginning().line, newRegion.end().line + 1));
	} else if(newRegion != oldRegion) {	// the selection actually changed
		if(oldRegion.isEmpty()) {	// the selection was empty...
			if(!newRegion.isEmpty())	// the selection is not empty now
				redrawLines(boost::irange(newRegion.beginning().line, newRegion.end().line + 1));
		} else {	// ...if the there is selection
			if(newRegion.isEmpty()) {	// the selection became empty
				redrawLines(boost::irange(oldRegion.beginning().line, oldRegion.end().line + 1));
				if(!isFrozen())
					widgetapi::redrawScheduledRegion(*this);
			} else if(oldRegion.beginning() == newRegion.beginning()) {	// the beginning point didn't change
				const Index i[2] = {oldRegion.end().line, newRegion.end().line};
				redrawLines(boost::irange(min(i[0], i[1]), max(i[0], i[1]) + 1));
			} else if(oldRegion.end() == newRegion.end()) {	// the end point didn't change
				const Index i[2] = {oldRegion.beginning().line, newRegion.beginning().line};
				redrawLines(boost::irange(min(i[0], i[1]), max(i[0], i[1]) + 1));
			} else {	// the both points changed
				if((oldRegion.beginning().line >= newRegion.beginning().line && oldRegion.beginning().line <= newRegion.end().line)
						|| (oldRegion.end().line >= newRegion.beginning().line && oldRegion.end().line <= newRegion.end().line)) {
					const Index i[2] = {
						min(oldRegion.beginning().line, newRegion.beginning().line), max(oldRegion.end().line, newRegion.end().line)
					};
					redrawLines(boost::irange(min(i[0], i[1]), max(i[0], i[1]) + 1));
				} else {
					redrawLines(boost::irange(oldRegion.beginning().line, oldRegion.end().line + 1));
					if(!isFrozen())
						widgetapi::redrawScheduledRegion(*this);
					redrawLines(boost::irange(newRegion.beginning().line, newRegion.end().line + 1));
				}
			}
		}
		changed = true;
	}

	if(changed && !isFrozen())
		widgetapi::redrawScheduledRegion(*this);
}

/// @see ComputedWritingModeListener#computedBlockFlowDirectionChanged
void TextViewer::computedBlockFlowDirectionChanged(BlockFlowDirection used) {
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
	if(isFrozen() && !freezeRegister_.linesToRedraw().empty()) {
		Index b = *freezeRegister_.linesToRedraw().begin();
		Index e = *freezeRegister_.linesToRedraw().end();
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
		freezeRegister_.resetLinesToRedraw(boost::irange(b, e));
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
	if(/*resultPosition != k::Position() &&*/ widgetapi::hasFocus(*this)) {
		utils::closeCompletionProposalsPopup(*this);
		caret_->moveTo(resultPosition);
	}
}

/**
 * Additionally draws the indicator margin on the vertical ruler.
 * @param line The line number
 * @param context The graphics context
 * @param rect The rectangle to draw
 */
void TextViewer::drawIndicatorMargin(Index /* line */, PaintContext& /* context */, const graphics::Rectangle& /* rect */) {
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
	inline widgetapi::NativeScrollPosition reverseScrollPosition(
			const TextViewer& textViewer, widgetapi::NativeScrollPosition position) {
		const TextRenderer& textRenderer = textViewer.textRenderer();
		return static_cast<widgetapi::NativeScrollPosition>(textRenderer.layouts().maximumMeasure()
			/ widgetapi::createRenderingContext(textViewer)->fontMetrics(textRenderer.defaultFont())->averageCharacterWidth())
			- position
			- static_cast<widgetapi::NativeScrollPosition>(textRenderer.viewport()->numberOfVisibleCharactersInLine());
	}
	boost::geometry::model::d2::point_xy<widgetapi::NativeScrollPosition>&& physicalScrollPosition(const TextViewer& textViewer) {
		const TextRenderer& textRenderer = textViewer.textRenderer();
		const shared_ptr<const TextViewport> viewport(textRenderer.viewport());
		const WritingMode writingMode(textViewer.presentation().computeWritingMode(&textRenderer));
		const Index bpd = viewport->firstVisibleLineInVisualNumber();
		const Index ipd = viewport->inlineProgressionOffset();
		widgetapi::NativeScrollPosition x, y;
		switch(writingMode.blockFlowDirection) {
			case HORIZONTAL_TB:
				x = (writingMode.inlineFlowDirection == LEFT_TO_RIGHT) ?
					ipd : reverseScrollPosition(textViewer, static_cast<widgetapi::NativeScrollPosition>(ipd));
				y = bpd;
				break;
			case VERTICAL_RL:
				x = reverseScrollPosition(textViewer, bpd);
				y = ipd;
				break;
			case VERTICAL_LR:
				x = bpd;
				y = ipd;
				break;
			default:
				ASCENSION_ASSERT_NOT_REACHED();
		}
		return boost::geometry::make<boost::geometry::model::d2::point_xy<widgetapi::NativeScrollPosition>>(x /* / xScrollRate */, y /* / yScrollRate */);
	}
}

namespace {
	void configureScrollBar(TextViewer& viewer, size_t coordinate, boost::optional<widgetapi::NativeScrollPosition> position,
			boost::optional<const boost::integer_range<widgetapi::NativeScrollPosition>> range, boost::optional<widgetapi::NativeScrollPosition> pageSize) {
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
		Glib::RefPtr<Gtk::Adjustment> adjustment = (coordinate == geometry::X_COORDINATE) ? viewer.get_hadjustment() : viewer.get_vadjustment();
		if(range != boost::none) {
			adjustment->set_lower(range->beginning());
			adjustment->set_upper(range->end());
		}
		adjustment->set_step_increment(1);
		if(pageSize != boost::none) {
			adjustment->set_page_increment(*pageSize);
			adjustment->set_page_size(*pageSize);
		}
		if(position != boost::none)
			adjustment->set_value(*position);
#elif defined(ASCENSION_WINDOW_SYSTEM_QT)
		QScrollBar* const scrollBar = (coordinate == geometry::X_COORDINATE) ? viewer.horizontalScrollBar() : viewer.verticalScrollBar();
		if(range != boost::none)
			scrollBar->setRange(range->beginning(), range->end());
		scrollBar->setSingleStep(1);
		if(pageSize != boost::none)
			scrollBar->setPageStep(*pageSize);
		if(position != boost::none)
			scrollBar->setSliderPosition(*position);
#elif defined(ASCENSION_WINDOW_SYSTEM_QUARTZ)
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
		win32::AutoZeroSize<SCROLLINFO> si;
		if(range != boost::none) {
			si.fMask |= SIF_RANGE;
			si.nMin = *range->begin();
			si.nMax = *range->end();
		}
		if(pageSize != boost::none) {
			si.fMask |= SIF_PAGE;
			si.nPage = *pageSize;
		}
		if(position != boost::none) {
			si.fMask |= SIF_POS;
			si.nPos = *position;
		}
		::SetScrollInfo(viewer.handle().get(), (coordinate == 0) ? SB_HORZ : SB_VERT, &si, true);
#endif
	}
}

/// @see Widget#focusGained
void TextViewer::focusGained() {
#ifdef ASCENSION_WINDOW_SYSTEM_WIN32
	// restore the scroll positions
	const boost::geometry::model::d2::point_xy<widgetapi::NativeScrollPosition> scrollPosition(physicalScrollPosition(*this));
	configureScrollBar(*this, 0, boost::geometry::get<0>(scrollPosition), boost::none, boost::none);
	configureScrollBar(*this, 1, boost::geometry::get<1>(scrollPosition), boost::none, boost::none);
#endif // ASCENSION_WINDOW_SYSTEM_WIN32

	// hmm...
//	if(/*sharedData_->options.appearance[SHOW_CURRENT_UNDERLINE] ||*/ !getCaret().isSelectionEmpty()) {
		redrawLines(boost::irange(line(caret().beginning()), line(caret().end()) + 1));
		widgetapi::redrawScheduledRegion(*this);
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
TextViewer::HitTestResult TextViewer::hitTest(const Point& p) const {
//	checkInitialization();
	const graphics::Rectangle localBounds(widgetapi::bounds(*this, false));
	if(!boost::geometry::within(p, localBounds))
		return OUT_OF_VIEWER;

	const RulerStyles& rulerStyles = rulerPainter_->declaredStyles();
	if(indicatorMargin(rulerStyles)->visible && boost::geometry::within(p, rulerPainter_->indicatorMarginAllocationRectangle()))
		return INDICATOR_MARGIN;
	else if(lineNumbers(rulerStyles)->visible && boost::geometry::within(p, rulerPainter_->lineNumbersAllocationRectangle()))
		return LINE_NUMBERS;
	else if(boost::geometry::within(p, textAreaContentRectangle()))
		return TEXT_AREA_CONTENT_RECTANGLE;
	else {
		assert(boost::geometry::within(p, textAreaAllocationRectangle()));
		return TEXT_AREA_PADDING_START;
	}
}

/// @internal Called by constructors.
void TextViewer::initialize(const TextViewer* other) {
	renderer_.reset((other == nullptr) ? new Renderer(*this) : new Renderer(*other->renderer_, *this));
	textRenderer().addComputedBlockFlowDirectionListener(*this);
//	renderer_->addFontListener(*this);
//	renderer_->addVisualLinesListener(*this);
	caret_.reset(new Caret(*this));
	caret().addListener(*this);
	caret().addStateListener(*this);
	rulerPainter_.reset(new detail::RulerPainter(*this));

	document().addListener(*this);
	document().addRollbackListener(*this);

	//	scrollInfo_.updateVertical(*this);
	updateScrollBars();
	setMouseInputStrategy(shared_ptr<MouseInputStrategy>());

#ifdef ASCENSION_TEST_TEXT_STYLES
	RulerConfiguration rc;
	rc.lineNumbers.visible = true;
	rc.indicatorMargin.visible = true;
	rc.lineNumbers.foreground = Paint(Color(0x00, 0x80, 0x80));
	rc.lineNumbers.background = Paint(Color(0xff, 0xff, 0xff));
	rc.lineNumbers.borderEnd.color = Color(0x00, 0x80, 0x80);
	rc.lineNumbers.borderEnd.style = Border::DOTTED;
	rc.lineNumbers.borderEnd.width = Length(1);
	setConfiguration(nullptr, &rc, false);

#if 0
	// this is JavaScript partitioning and lexing settings for test
	using namespace contentassist;
	using namespace rules;
	using namespace text;

	static const ContentType JS_MULTILINE_DOC_COMMENT = 140,
		JS_MULTILINE_COMMENT = 142, JS_SINGLELINE_COMMENT = 143, JS_DQ_STRING = 144, JS_SQ_STRING = 145;

	class JSContentTypeInformation : public IContentTypeInformationProvider {
	public:
		JSContentTypeInformation()  {
			jsIDs_.overrideIdentifierStartCharacters(L"_", L""); jsdocIDs_.overrideIdentifierStartCharacters(L"$@", L"");}
		const IdentifierSyntax& getIdentifierSyntax(ContentType contentType) const {
			return (contentType != JS_MULTILINE_DOC_COMMENT) ? jsIDs_ : jsdocIDs_;}
	private:
		IdentifierSyntax jsIDs_, jsdocIDs_;
	};
	JSContentTypeInformation* cti = new JSContentTypeInformation;

	TransitionRule* rules[12];
	rules[0] = new LiteralTransitionRule(DEFAULT_CONTENT_TYPE, JS_MULTILINE_DOC_COMMENT, L"/**");
	rules[1] = new LiteralTransitionRule(JS_MULTILINE_DOC_COMMENT, DEFAULT_CONTENT_TYPE, L"*/");
	rules[2] = new LiteralTransitionRule(DEFAULT_CONTENT_TYPE, JS_MULTILINE_COMMENT, L"/*");
	rules[3] = new LiteralTransitionRule(JS_MULTILINE_COMMENT, DEFAULT_CONTENT_TYPE, L"*/");
	rules[4] = new LiteralTransitionRule(DEFAULT_CONTENT_TYPE, JS_SINGLELINE_COMMENT, L"//");
	rules[5] = new LiteralTransitionRule(JS_SINGLELINE_COMMENT, DEFAULT_CONTENT_TYPE, L"", L'\\');
	rules[6] = new LiteralTransitionRule(DEFAULT_CONTENT_TYPE, JS_DQ_STRING, L"\"");
	rules[7] = new LiteralTransitionRule(JS_DQ_STRING, DEFAULT_CONTENT_TYPE, L"\"", L'\\');
	rules[8] = new LiteralTransitionRule(JS_DQ_STRING, DEFAULT_CONTENT_TYPE, L"");
	rules[9] = new LiteralTransitionRule(DEFAULT_CONTENT_TYPE, JS_SQ_STRING, L"\'");
	rules[10] = new LiteralTransitionRule(JS_SQ_STRING, DEFAULT_CONTENT_TYPE, L"\'", L'\\');
	rules[11] = new LiteralTransitionRule(JS_SQ_STRING, DEFAULT_CONTENT_TYPE, L"");
	LexicalPartitioner* p = new LexicalPartitioner();
	p->setRules(rules, ASCENSION_ENDOF(rules));
	for(size_t i = 0; i < ASCENSION_COUNTOF(rules); ++i)
		delete rules[i];
	document().setPartitioner(unique_ptr<DocumentPartitioner>(p));

	PresentationReconstructor* pr = new PresentationReconstructor(presentation());

	// JSDoc syntax highlight test
	static const Char JSDOC_ATTRIBUTES[] = L"@addon @argument @author @base @class @constructor @deprecated @exception @exec @extends"
		L" @fileoverview @final @ignore @link @member @param @private @requires @return @returns @see @throws @type @version";
	{
		unique_ptr<const WordRule> jsdocAttributes(new WordRule(220, JSDOC_ATTRIBUTES, ASCENSION_ENDOF(JSDOC_ATTRIBUTES) - 1, L' ', true));
		unique_ptr<LexicalTokenScanner> scanner(new LexicalTokenScanner(JS_MULTILINE_DOC_COMMENT));
		scanner->addWordRule(jsdocAttributes);
		scanner->addRule(unique_ptr<Rule>(new URIRule(219, URIDetector::defaultIANAURIInstance(), false)));
		map<Token::ID, const TextStyle> jsdocStyles;
		jsdocStyles.insert(make_pair(Token::DEFAULT_TOKEN, TextStyle(Colors(Color(0x00, 0x80, 0x00)))));
		jsdocStyles.insert(make_pair(219, TextStyle(Colors(Color(0x00, 0x80, 0x00)), false, false, false, SOLID_UNDERLINE)));
		jsdocStyles.insert(make_pair(220, TextStyle(Colors(Color(0x00, 0x80, 0x00)), true)));
		unique_ptr<IPartitionPresentationReconstructor> ppr(
			new LexicalPartitionPresentationReconstructor(document(), unique_ptr<ITokenScanner>(scanner.release()), jsdocStyles));
		pr->setPartitionReconstructor(JS_MULTILINE_DOC_COMMENT, ppr);
	}

	// JavaScript syntax highlight test
	static const Char JS_KEYWORDS[] = L"Infinity break case catch continue default delete do else false finally for function"
		L" if in instanceof new null return switch this throw true try typeof undefined var void while with";
	static const Char JS_FUTURE_KEYWORDS[] = L"abstract boolean byte char class double enum extends final float goto"
		L" implements int interface long native package private protected public short static super synchronized throws transient volatile";
	{
		unique_ptr<const WordRule> jsKeywords(new WordRule(221, JS_KEYWORDS, ASCENSION_ENDOF(JS_KEYWORDS) - 1, L' ', true));
		unique_ptr<const WordRule> jsFutureKeywords(new WordRule(222, JS_FUTURE_KEYWORDS, ASCENSION_ENDOF(JS_FUTURE_KEYWORDS) - 1, L' ', true));
		unique_ptr<LexicalTokenScanner> scanner(new LexicalTokenScanner(DEFAULT_CONTENT_TYPE));
		scanner->addWordRule(jsKeywords);
		scanner->addWordRule(jsFutureKeywords);
		scanner->addRule(unique_ptr<const Rule>(new NumberRule(223)));
		map<Token::ID, const TextStyle> jsStyles;
		jsStyles.insert(make_pair(Token::DEFAULT_TOKEN, TextStyle()));
		jsStyles.insert(make_pair(221, TextStyle(Colors(Color(0x00, 0x00, 0xff)))));
		jsStyles.insert(make_pair(222, TextStyle(Colors(Color(0x00, 0x00, 0xff)), false, false, false, DASHED_UNDERLINE)));
		jsStyles.insert(make_pair(223, TextStyle(Colors(Color(0x80, 0x00, 0x00)))));
		pr->setPartitionReconstructor(DEFAULT_CONTENT_TYPE,
			unique_ptr<IPartitionPresentationReconstructor>(new LexicalPartitionPresentationReconstructor(document(),
				unique_ptr<ITokenScanner>(scanner.release()), jsStyles)));
	}

	// other contents
	pr->setPartitionReconstructor(JS_MULTILINE_COMMENT, unique_ptr<IPartitionPresentationReconstructor>(
		new SingleStyledPartitionPresentationReconstructor(TextStyle(Colors(Color(0x00, 0x80, 0x00))))));
	pr->setPartitionReconstructor(JS_SINGLELINE_COMMENT, unique_ptr<IPartitionPresentationReconstructor>(
		new SingleStyledPartitionPresentationReconstructor(TextStyle(Colors(Color(0x00, 0x80, 0x00))))));
	pr->setPartitionReconstructor(JS_DQ_STRING, unique_ptr<IPartitionPresentationReconstructor>(
		new SingleStyledPartitionPresentationReconstructor(TextStyle(Colors(Color(0x00, 0x00, 0x80))))));
	pr->setPartitionReconstructor(JS_SQ_STRING, unique_ptr<IPartitionPresentationReconstructor>(
		new SingleStyledPartitionPresentationReconstructor(TextStyle(Colors(Color(0x00, 0x00, 0x80))))));
	new CurrentLineHighlighter(*caret_, Colors(Color(), Color::fromCOLORREF(::GetSysColor(COLOR_INFOBK))));

	// URL hyperlinks test
	unique_ptr<hyperlink::CompositeHyperlinkDetector> hld(new hyperlink::CompositeHyperlinkDetector);
	hld->setDetector(JS_MULTILINE_DOC_COMMENT, unique_ptr<hyperlink::IHyperlinkDetector>(
		new hyperlink::URIHyperlinkDetector(URIDetector::defaultIANAURIInstance(), false)));
	presentation().setHyperlinkDetector(hld.release(), true);

	// content assist test
	class JSDocProposals : public IdentifiersProposalProcessor {
	public:
		JSDocProposals(const IdentifierSyntax& ids) : IdentifiersProposalProcessor(JS_MULTILINE_DOC_COMMENT, ids) {}
		void computeCompletionProposals(const Caret& caret, bool& incremental,
				Region& replacementRegion, set<ICompletionProposal*>& proposals) const {
			basic_istringstream<Char> s(JSDOC_ATTRIBUTES);
			String p;
			while(s >> p)
				proposals.insert(new CompletionProposal(p));
			IdentifiersProposalProcessor::computeCompletionProposals(caret, incremental = true, replacementRegion, proposals);
		}
		bool isCompletionProposalAutoActivationCharacter(CodePoint c) const /*throw()*/ {return c == L'@';}
	};
	class JSProposals : public IdentifiersProposalProcessor {
	public:
		JSProposals(const IdentifierSyntax& ids) : IdentifiersProposalProcessor(DEFAULT_CONTENT_TYPE, ids) {}
		void computeCompletionProposals(const Caret& caret, bool& incremental,
				Region& replacementRegion, set<ICompletionProposal*>& proposals) const {
			basic_istringstream<Char> s(JS_KEYWORDS);
			String p;
			while(s >> p)
				proposals.insert(new CompletionProposal(p));
			IdentifiersProposalProcessor::computeCompletionProposals(caret, incremental = true, replacementRegion, proposals);
		}
		bool isCompletionProposalAutoActivationCharacter(CodePoint c) const /*throw()*/ {return c == L'.';}
	};
	unique_ptr<contentassist::ContentAssistant> ca(new contentassist::ContentAssistant());
	ca->setContentAssistProcessor(JS_MULTILINE_DOC_COMMENT, unique_ptr<contentassist::IContentAssistProcessor>(new JSDocProposals(cti->getIdentifierSyntax(JS_MULTILINE_DOC_COMMENT))));
	ca->setContentAssistProcessor(DEFAULT_CONTENT_TYPE, unique_ptr<contentassist::IContentAssistProcessor>(new JSProposals(cti->getIdentifierSyntax(DEFAULT_CONTENT_TYPE))));
	setContentAssistant(unique_ptr<contentassist::IContentAssistant>(ca));
	document().setContentTypeInformation(unique_ptr<IContentTypeInformationProvider>(cti));
#endif // 1

	class ZebraTextRunStyleTest : public TextRunStyleDirector {
	private:
		class Iterator : public StyledTextRunIterator {
		public:
			Iterator(Index lineLength, bool beginningIsBlackBack) : length_(lineLength), beginningIsBlackBack_(beginningIsBlackBack) {
				current_ = StyledTextRun(0, current_.style());
				update();
			}
			StyledTextRun current() const {
				if(!hasNext())
					throw IllegalStateException("");
				return current_;
			}
			bool hasNext() const {
				return current_.position() != length_;
			}
			void next() {
				if(!hasNext())
					throw IllegalStateException("");
				current_ = StyledTextRun(current_.position() + 1, current_.style());
				update();
			}
		private:
			void update() {
				int temp = beginningIsBlackBack_ ? 0 : 1;
				temp += (current_.position() % 2 == 0) ? 0 : 1;
				shared_ptr<TextRunStyle> style(make_shared<TextRunStyle>());
				style->foreground = Paint((temp % 2 == 0) ?
					Color(0xff, 0x00, 0x00) : SystemColors::get(SystemColors::WINDOW_TEXT));
				style->background = Paint((temp % 2 == 0) ?
					Color(0xff, 0xcc, 0xcc) : SystemColors::get(SystemColors::WINDOW));
				current_ = StyledTextRun(current_.position(), style);
			}
		private:
			const Index length_;
			const bool beginningIsBlackBack_;
			StyledTextRun current_;
		};
	public:
		ZebraTextRunStyleTest(const k::Document& document) : document_(document) {
		}
		unique_ptr<StyledTextRunIterator> queryTextRunStyle(Index line) const {
			return unique_ptr<StyledTextRunIterator>(new Iterator(document_.lineLength(line), line % 2 == 0));
		}
	private:
		const k::Document& document_;
	};
	presentation().setTextRunStyleDirector(
		shared_ptr<TextRunStyleDirector>(new ZebraTextRunStyleTest(document())));
#endif // ASCENSION_TEST_TEXT_STYLES
	
	renderer_->addDefaultFontListener(*this);
	renderer_->layouts().addVisualLinesListener(*this);

	initializeNativeObjects(other);
}

/**
 * Returns an offset from left/top-edge of local-bounds to one of the content-area in user units.
 * This algorithm considers the ruler, the scroll position and spaces around the content box.
 * @return The offset
 * @see TextLayout#lineStartEdge, TextRenderer#lineStartEdge
 */
Scalar TextViewer::inlineProgressionOffsetInViewport() const {
	const bool horizontal = isHorizontal(textRenderer().computedBlockFlowDirection());
	Scalar offset = 0;

	// scroll position
	const boost::geometry::model::d2::point_xy<widgetapi::NativeScrollPosition> scrollPosition(physicalScrollPosition(*this));
	offset -= inlineProgressionScrollOffsetInUserUnits(*textRenderer().viewport(),
		horizontal ? boost::geometry::get<0>(scrollPosition) : boost::geometry::get<1>(scrollPosition));

	// ruler width
	if((horizontal && rulerPainter_->alignment() == PhysicalDirection::LEFT)
			|| (!horizontal && rulerPainter_->alignment() == PhysicalDirection::TOP))
		offset += rulerPainter_->allocationWidth();

	return offset;

#if 0
	const detail::PhysicalTextAnchor alignment(
		detail::computePhysicalTextAnchor(layout.anchor(), layout.writingMode().inlineFlowDirection));
	if(alignment == detail::LEFT /*|| ... != NO_JUSTIFICATION*/)	// TODO: this code ignores last visual line with justification.
		return spaces.left - scrolls_.x() * renderer_->defaultFont()->metrics().averageCharacterWidth();

	Scalar indent;
	const graphics::Rectangle clientBounds(bounds(false));
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

namespace {
	void handleDirectionalKey(TextViewer& viewer, PhysicalDirection direction, widgetapi::UserInput::ModifierKey modifiers) {
		using namespace ascension::texteditor::commands;
		using widgetapi::UserInput;
		static k::Position(*const nextCharacterLocation)(const k::Point&, Direction, k::locations::CharacterUnit, Index) = k::locations::nextCharacter;

		const WritingMode writingMode(viewer.presentation().computeWritingMode(&viewer.textRenderer()));
		const FlowRelativeDirection abstractDirection = mapPhysicalToFlowRelative(writingMode, direction);
		const Direction logicalDirection = (abstractDirection == FlowRelativeDirection::AFTER || abstractDirection == FlowRelativeDirection::END) ? Direction::FORWARD : Direction::BACKWARD;
		switch(abstractDirection) {
			case FlowRelativeDirection::BEFORE:
			case FlowRelativeDirection::AFTER:
				if((modifiers & ~(UserInput::SHIFT_DOWN | UserInput::ALT_DOWN)) == 0) {
					if((modifiers & UserInput::ALT_DOWN) == 0)
						makeCaretMovementCommand(viewer, &k::locations::nextVisualLine,
							logicalDirection, (modifiers & UserInput::SHIFT_DOWN) != 0)();
					else
						makeRowSelectionExtensionCommand(viewer, &k::locations::nextVisualLine, logicalDirection)();
				}
				break;
			case FlowRelativeDirection::START:
			case FlowRelativeDirection::END:
				if((modifiers & ~(UserInput::CONTROL_DOWN | UserInput::SHIFT_DOWN | UserInput::ALT_DOWN)) == 0) {
					if((modifiers & UserInput::ALT_DOWN) == 0) {
						if((modifiers & UserInput::CONTROL_DOWN) != 0)
							makeCaretMovementCommand(viewer, &k::locations::nextWord,
								logicalDirection, (modifiers & UserInput::SHIFT_DOWN) != 0)();
						else
							makeCaretMovementCommand(viewer, nextCharacterLocation,
								logicalDirection, (modifiers & UserInput::SHIFT_DOWN) != 0)();
					} else if((modifiers & UserInput::SHIFT_DOWN) != 0) {
						if((modifiers & UserInput::CONTROL_DOWN) != 0)
							makeRowSelectionExtensionCommand(viewer, &k::locations::nextWord, logicalDirection)();
						else
							makeRowSelectionExtensionCommand(viewer, nextCharacterLocation, logicalDirection)();
					}
				}
				break;
			default:
				ASCENSION_ASSERT_NOT_REACHED();
		}
	}
}

/// @see Widget#keyPressed
void TextViewer::keyPressed(const widgetapi::KeyInput& input) {
	if(mouseInputStrategy_.get() != nullptr)
		mouseInputStrategy_->interruptMouseReaction(true);

	// TODO: This code is temporary. The following code provides a default implementation of
	// TODO: "key combination to command" map.
	using namespace ascension::viewers::widgetapi;
	using namespace ascension::texteditor::commands;
	static k::Position(*const nextCharacterLocation)(const k::Point&, Direction, k::locations::CharacterUnit, Index) = k::locations::nextCharacter;
//	if(hasModifier<UserInput::ALT_DOWN>(input)) {
//		if(!hasModifier<UserInput::SHIFT_DOWN>(input)
//				|| (input.keyboardCode() != VK_LEFT && input.keyboardCode() != VK_UP
//				&& input.keyboardCode() != VK_RIGHT && input.keyboardCode() != VK_DOWN))
//			return false;
//	}
	switch(input.keyboardCode()) {
	case keyboardcodes::BACK_SPACE:	// [BackSpace]
	case keyboardcodes::F16:	// [F16]
		switch(input.modifiers()) {
			case 0:
			case UserInput::SHIFT_DOWN:
				CharacterDeletionCommand(*this, Direction::BACKWARD)();
				break;
			case UserInput::CONTROL_DOWN:
				WordDeletionCommand(*this, Direction::BACKWARD)();
				break;
			case UserInput::ALT_DOWN:
			case UserInput::SHIFT_DOWN | UserInput::ALT_DOWN:
				UndoCommand(*this, hasModifier<UserInput::SHIFT_DOWN>(input))();
				break;
		}
		break;
	case keyboardcodes::CLEAR:	// [Clear]
		if(input.modifiers() == UserInput::CONTROL_DOWN)
			EntireDocumentSelectionCreationCommand(*this)();
		break;
	case keyboardcodes::ENTER_OR_RETURN:	// [Enter]
		switch(input.modifiers()) {
		case 0:
		case UserInput::SHIFT_DOWN:
			NewlineCommand(*this)();
			break;
		case UserInput::CONTROL_DOWN:
			NewlineCommand(*this, Direction::BACKWARD)();
			break;
		case UserInput::CONTROL_DOWN | UserInput::SHIFT_DOWN:
			NewlineCommand(*this, Direction::FORWARD)();
			break;
		}
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
			onVScroll(SB_PAGEUP, 0, nullptr);
		else
			makeCaretMovementCommand(*this, &k::locations::nextPage, Direction::BACKWARD, hasModifier<UserInput::SHIFT_DOWN>(input))();
		break;
	case keyboardcodes::NEXT_OR_PAGE_DOWN:	// [PageDown]
		if(hasModifier<UserInput::CONTROL_DOWN>(input))
			onVScroll(SB_PAGEDOWN, 0, nullptr);
		else
			makeCaretMovementCommand(*this, &k::locations::nextPage, Direction::FORWARD, hasModifier<UserInput::SHIFT_DOWN>(input))();
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
		handleDirectionalKey(*this, PhysicalDirection::LEFT, input.modifiers());
		break;
	case keyboardcodes::UP:		// [Up]
		handleDirectionalKey(*this, PhysicalDirection::TOP, input.modifiers());
		break;
	case keyboardcodes::RIGHT:	// [Right]
		handleDirectionalKey(*this, PhysicalDirection::RIGHT, input.modifiers());
		break;
	case keyboardcodes::DOWN:	// [Down]
		handleDirectionalKey(*this, PhysicalDirection::BOTTOM, input.modifiers());
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
void TextViewer::keyReleased(const widgetapi::KeyInput& input) {
	if(widgetapi::hasModifier<widgetapi::UserInput::ALT_DOWN>(input)) {
		cursorVanisher_.restore();
		if(mouseInputStrategy_.get() != nullptr)
			mouseInputStrategy_->interruptMouseReaction(true);
	}
}

#if 0
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
		const graphics::Rectangle clientBounds(bounds(false));
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
			widgetapi::redrawScheduledRegion(*this);
		if(newPair->second.line != newPair->first.line) {
			redrawLine(newPair->second.line);
			if(!isFrozen())
				widgetapi::redrawScheduledRegion(*this);
		}
		if(oldPair	// clear the previous highlight
				&& oldPair->first.line != newPair->first.line && oldPair->first.line != newPair->second.line) {
			redrawLine(oldPair->first.line);
			if(!isFrozen())
				widgetapi::redrawScheduledRegion(*this);
		}
		if(oldPair && oldPair->second.line != newPair->first.line
				&& oldPair->second.line != newPair->second.line && oldPair->second.line != oldPair->first.line)
			redrawLine(oldPair->second.line);
	} else {
		if(oldPair) {	// clear the previous highlight
			redrawLine(oldPair->first.line);
			if(!isFrozen())
				widgetapi::redrawScheduledRegion(*this);
			if(oldPair->second.line != oldPair->first.line)
				redrawLine(oldPair->second.line);
		}
	}
}

/// @see Widget#mouseDoubleClicked
void TextViewer::mouseDoubleClicked(const widgetapi::MouseButtonInput& input) {
	if(allowsMouseInput() && mouseInputStrategy_.get() != nullptr)
		mouseInputStrategy_->mouseButtonInput(MouseInputStrategy::DOUBLE_CLICKED, input);
}

/// @see Widget#mouseMoved
void TextViewer::mouseMoved(const widgetapi::LocatedUserInput& input) {
	cursorVanisher_.restore();
	if(allowsMouseInput() && mouseInputStrategy_.get() != nullptr)
		mouseInputStrategy_->mouseMoved(input);
}

/// @see Widget#mousePressed
void TextViewer::mousePressed(const widgetapi::MouseButtonInput& input) {
	cursorVanisher_.restore();
	if(allowsMouseInput() && mouseInputStrategy_.get() != nullptr)
		mouseInputStrategy_->mouseButtonInput(MouseInputStrategy::PRESSED, input);
}

/// @see Widget#mouseReleased
void TextViewer::mouseReleased(const widgetapi::MouseButtonInput& input) {
	if(allowsMouseInput() || input.button() == widgetapi::UserInput::BUTTON3_DOWN)
		cursorVanisher_.restore();
	if(allowsMouseInput() && mouseInputStrategy_.get() != nullptr)
		mouseInputStrategy_->mouseButtonInput(MouseInputStrategy::RELEASED, input);
}

/// @see Widget#mouseWheelChanged
void TextViewer::mouseWheelChanged(const widgetapi::MouseWheelInput& input) {
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
	graphics::Rectangle scheduledBounds(context.boundsToPaint());
	if(geometry::isEmpty(geometry::normalize(scheduledBounds)))	// skip if the region to paint is empty
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
	redrawLines(boost::irange(line, following ? numeric_limits<Index>::max() : line + 1));
}

/**
 * Redraws the specified lines on the view. If the viewer is frozen, redraws after unfrozen.
 * @param lines The lines to be redrawn. The last line (@a lines.end()) is exclusive and this line
 *              will not be redrawn. If this value is @c std#numeric_limits<Index>#max(), this
 *              method redraws the first line (@a lines.beginning()) and the following all lines
 * @throw kernel#BadRegionException @a lines intersects outside of the document
 */
void TextViewer::redrawLines(const boost::integer_range<Index>& lines) {
//	checkInitialization();

	if(*lines.end() != numeric_limits<Index>::max() && *lines.end() >= document().numberOfLines())
		throw k::BadRegionException(k::Region(k::Position(*lines.begin(), 0), k::Position(*lines.end(), 0)));

	if(isFrozen()) {
		freezeRegister_.addLinesToRedraw(lines);
		return;
	}

	if(*lines.end() - 1 < textRenderer().viewport()->firstVisibleLineInLogicalNumber())
		return;

#ifdef _DEBUG
	if(DIAGNOSE_INHERENT_DRAWING)
		win32::DumpContext()
			<< L"@TextViewer.redrawLines invalidates lines ["
			<< static_cast<unsigned long>(*lines.begin())
			<< L".." << static_cast<unsigned long>(*lines.end()) << L"]\n";
#endif // _DEBUG

	const WritingMode writingMode(presentation().computeWritingMode(&textRenderer()));
	const graphics::Rectangle viewport(widgetapi::bounds(*this, false));
	FlowRelativeFourSides<Scalar> abstractBounds;
	mapPhysicalToFlowRelative(writingMode, viewport, viewport, abstractBounds);

	// calculate before and after edges of a rectangle to redraw
	BaselineIterator baseline(*textRenderer().viewport(), lines.front(), false);
	if(*baseline != numeric_limits<Scalar>::min())
		abstractBounds.before() = mapTextAreaBpdToLocal(*this, *baseline)
			- textRenderer().layouts().at(lines.front()).lineMetrics(0).ascent();
	baseline += lines.size();
	if(*baseline != numeric_limits<Scalar>::max())
		abstractBounds.after() = mapTextAreaBpdToLocal(*this, *baseline)
			+ *textRenderer().layouts().at(baseline.line()).extent().end();
	graphics::Rectangle boundsToRedraw(viewport);
	mapFlowRelativeToPhysical(writingMode, viewport, abstractBounds, boundsToRedraw);

	widgetapi::scheduleRedraw(*this, boundsToRedraw, false);
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
	graphics::Rectangle r(widgetapi::bounds(*this, false));
	if(utils::isRulerLeftAligned(*this))
		geometry::range<0>(r) = boost::irange(geometry::left(r), geometry::left(r) + rulerPainter_->allocationWidth());
	else
		geometry::range<1>(r) = boost::irange(geometry::right(r) - rulerPainter_->allocationWidth(), geometry::right(r));
	widgetapi::scheduleRedraw(*this, r, false);
}

/// @see Widget#resized
void TextViewer::resized(const Dimension&) {
	utils::closeCompletionProposalsPopup(*this);
	if(widgetapi::isMinimized(*this))
		return;
	if(renderer_.get() == nullptr)
		return;
	textRenderer().viewport()->setBoundsInView(textAreaContentRectangle());
#ifdef ASCENSION_WINDOW_SYSTEM_WIN32
	// notify the tooltip
	win32::AutoZeroSize<TOOLINFOW> ti;
	const graphics::Rectangle viewerBounds(widgetapi::bounds(*this, false));
	ti.hwnd = handle().get();
	ti.uId = 1;
	ti.rect = viewerBounds;
	::SendMessageW(toolTip_.get(), TTM_NEWTOOLRECT, 0, reinterpret_cast<LPARAM>(&ti));
#endif // ASCENSION_WINDOW_SYSTEM_WIN32
	textRenderer().setTextWrapping(textRenderer().textWrapping(), widgetapi::createRenderingContext(*this).get());
	scrolls_.resetBars(*this, 'a', true);
	updateScrollBars();
	rulerPainter_->update();
	if(rulerPainter_->alignment() != PhysicalDirection::LEFT && rulerPainter_->alignment() != PhysicalDirection::TOP) {
//		recreateCaret();
//		redrawVerticalRuler();
		widgetapi::scheduleRedraw(*this, false);	// hmm...
	}
	if(contentAssistant() != 0)
		contentAssistant()->viewerBoundsChanged();
}

/// @see CaretStateListener#selectionShapeChanged
void TextViewer::selectionShapeChanged(const Caret& self) {
	if(!isFrozen() && !isSelectionEmpty(self))
		redrawLines(boost::irange(line(self.beginning()), line(self.end()) + 1));
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
void TextViewer::setConfiguration(const Configuration* general, shared_ptr<const RulerStyles> ruler, bool synchronizeUI) {
	if(ruler != nullptr)
		rulerPainter_->setStyles(ruler);
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

		if(!isFrozen() && (widgetapi::hasFocus(*this) /*|| handle() == Viewer::completionWindow_->getSafeHwnd()*/)) {
			caret().resetVisualization();
			caret().updateLocation();
		}
		if(synchronizeUI) {
			LONG style = ::GetWindowLongW(handle().get(), GWL_EXSTYLE);
			if(configuration_.readingDirection == LEFT_TO_RIGHT) {
				style &= ~(WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR);
				style |= WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR;
			} else {
				style &= ~(WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR);
				style |= WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR;
			}
			::SetWindowLongW(handle().get(), GWL_EXSTYLE, style);
		}
	}
	widgetapi::scheduleRedraw(*this, false);
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

	hideToolTip();
	if(timeToWait == -1)
		timeToWait = ::GetDoubleClickTime();
	tipText_.assign(text);
	::SetTimer(handle().get(), TIMERID_CALLTIP, timeToWait, nullptr);
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
graphics::Rectangle TextViewer::textAreaAllocationRectangle() const BOOST_NOEXCEPT {
	const graphics::Rectangle window(widgetapi::bounds(*this, false));
	PhysicalFourSides<Scalar> result(window);
	switch(rulerPainter_->alignment()) {
		case PhysicalDirection::LEFT:
			result.left() += rulerPainter_->allocationWidth();
			break;
		case PhysicalDirection::TOP:
			result.top() += rulerPainter_->allocationWidth();
			break;
		case PhysicalDirection::RIGHT:
			result.right() -= rulerPainter_->allocationWidth();
			break;
		case PhysicalDirection::BOTTOM:
			result.bottom() -= rulerPainter_->allocationWidth();
			break;
		default:
			ASCENSION_ASSERT_NOT_REACHED();
	}
	return geometry::make<graphics::Rectangle>(result);
}

/**
 * Returns the 'content-rectangle' of the text area, in local-coordinates.
 * @see #bounds, #textAreaAllocationRectangle
 */
graphics::Rectangle TextViewer::textAreaContentRectangle() const /*throw()*/ {
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
		const boost::integer_range<Index> linesToRedraw(freezeRegister_.unfreeze());
		if(!freezeRegister_.isFrozen()) {
			if(scrolls_.changed) {
				updateScrollBars();
				widgetapi::scheduleRedraw(*this, false);
			} else if(!linesToRedraw.empty())
				redrawLines(linesToRedraw);

			rulerPainter_->update();

			caretMoved(caret(), caret().selectedRegion());
			widgetapi::redrawScheduledRegion(*this);
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
	const boost::geometry::model::d2::point_xy<widgetapi::NativeScrollPosition> positions(physicalScrollPosition(*this));
	Point endPositions(geometry::_x = layouts.maximumMeasure(), geometry::_y = static_cast<boost::geometry::coordinate_type<Point>::type>(layouts.numberOfVisualLines()));
	Point pageSteps(
		geometry::_x = static_cast<boost::geometry::coordinate_type<Point>::type>(viewport->numberOfVisibleCharactersInLine()),
		geometry::_y = static_cast<boost::geometry::coordinate_type<Point>::type>(viewport->numberOfVisibleLines()));
	if(isVertical(textRenderer().computedBlockFlowDirection())) {
		geometry::transpose(endPositions);
		geometry::transpose(pageSteps);
	}

#define ASCENSION_GET_SCROLL_MINIMUM(endPosition, pageStep)	(endPosition/* * rate*/ - pageStep + 1)

	// about horizontal scroll bar
	bool wasNeededScrollbar = ASCENSION_GET_SCROLL_MINIMUM(geometry::x(endPositions), geometry::x(pageSteps)) > 0;
	// scroll to leftmost/rightmost before the scroll bar vanishes
	widgetapi::NativeScrollPosition minimum = static_cast<widgetapi::NativeScrollPosition>(ASCENSION_GET_SCROLL_MINIMUM(geometry::x(endPositions), geometry::x(pageSteps)));
	if(wasNeededScrollbar && minimum <= 0) {
//		scrolls_.horizontal.position = 0;
		if(!isFrozen()) {
			widgetapi::scheduleRedraw(*this, false);
			caret().updateLocation();
		}
	} else if(static_cast<widgetapi::NativeScrollPosition>(geometry::x(positions)) > minimum)
		viewport->scrollTo(PhysicalTwoAxes<boost::optional<TextViewport::ScrollOffset>>(minimum, boost::none));
	assert(ASCENSION_GET_SCROLL_MINIMUM(geometry::x(endPositions), geometry::x(pageSteps)) > 0 || geometry::x(positions) == 0);
	if(!isFrozen())
		configureScrollBar(*this, 0, geometry::x(positions),
			boost::irange<widgetapi::NativeScrollPosition>(0, geometry::x(endPositions)),
			static_cast<widgetapi::NativeScrollPosition>(geometry::x(pageSteps)));

	// about vertical scroll bar
	wasNeededScrollbar = ASCENSION_GET_SCROLL_MINIMUM(geometry::y(endPositions), geometry::y(pageSteps)) > 0;
	minimum = ASCENSION_GET_SCROLL_MINIMUM(geometry::y(endPositions), geometry::y(pageSteps));
	// validate scroll position
	if(minimum <= 0) {
//		scrolls_.vertical.position = 0;
//		scrolls_.firstVisibleLine = VisualLine(0, 0);
		if(!isFrozen()) {
			widgetapi::scheduleRedraw(*this, false);
			caret().updateLocation();
		}
	} else if(static_cast<widgetapi::NativeScrollPosition>(geometry::y(positions)) > minimum)
		viewport->scrollTo(PhysicalTwoAxes<boost::optional<TextViewport::ScrollOffset>>(boost::none, minimum));
	assert(ASCENSION_GET_SCROLL_MINIMUM(geometry::y(endPositions), geometry::y(pageSteps)) > 0 || geometry::y(positions) == 0);
	if(!isFrozen())
		configureScrollBar(*this, 1, geometry::y(positions),
			boost::irange<widgetapi::NativeScrollPosition>(0, geometry::y(endPositions)),
			static_cast<widgetapi::NativeScrollPosition>(geometry::y(pageSteps)));

	scrolls_.changed = isFrozen();

#undef ASCENSION_GET_SCROLL_MINIMUM
}

/// @see TextViewport#viewportBoundsInViewChanged
void TextViewer::viewportBoundsInViewChanged(const graphics::Rectangle& oldBounds) /*throw()*/ {
	// does nothing
}

namespace {
	void scrollBarParameters(const TextViewer& viewer,
			size_t coordinate, widgetapi::NativeScrollPosition* position,
			boost::integer_range<widgetapi::NativeScrollPosition>* range, widgetapi::NativeScrollPosition* pageSize) {
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
		Glib::RefPtr<Gtk::Adjustment> adjustment = (coordinate == geometry::X_COORDINATE) ? viewer.get_hadjustment() : viewer.get_vadjustment();
		if(range != nullptr)
			*range = makeRange(adjustment->get_lower(), adjustment->get_upper());
		if(pageSize != nullptr)
			*pageSize = adjustment->get_page_increment();
//			*pageSize = adjustment->get_page_size();
		if(position != nullptr)
			*position = adjustment->get_value();
#elif defined(ASCENSION_WINDOW_SYSTEM_QT)
		const QScrollBar* const scrollBar = (coordinate == geometry::X_COORDINATE) ? viewer.horizontalScrollBar() : viewer.verticalScrollBar();
		if(range != nullptr)
			*range = makeRange(scrollBar->minimum(), scrollBar->maximum());
		if(pageSize != nullptr)
			*pageSize = scrollBar->pageStep();
		if(position != nullptr)
			*position = scrollBar->sliderPosition();
#elif defined(ASCENSION_WINDOW_SYSTEM_QUARTZ)
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
		win32::AutoZeroSize<SCROLLINFO> si;
		si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
		if(win32::boole(::GetScrollInfo(viewer.handle().get(), (coordinate == 0) ? SB_HORZ : SB_VERT, &si)))
			throw makePlatformError();
		if(range != nullptr)
			*range = boost::irange<widgetapi::NativeScrollPosition>(si.nMin, si.nMax);
		if(pageSize != nullptr)
			*pageSize = si.nPage;
		if(position != nullptr)
			*position = si.nPos;
#endif
	}
}

/// @see TextViewportListener#viewportScrollPositionChanged
void TextViewer::viewportScrollPositionChanged(
		const AbstractTwoAxes<TextViewport::SignedScrollOffset>& offsets,
		const VisualLine& oldLine, TextViewport::ScrollOffset oldInlineProgressionOffset) {
	if(isFrozen()) {
		scrolls_.changed = true;
		return;
	}

	// 1. update the scroll positions
	const shared_ptr<const TextViewport> viewport(textRenderer().viewport());
	// TODO: this code ignores orientation in vertical layout.
	switch(textRenderer().computedBlockFlowDirection()) {
		case HORIZONTAL_TB:
			configureScrollBar(*this, 0, viewport->inlineProgressionOffset(), boost::none, boost::none);
			configureScrollBar(*this, 1, viewport->firstVisibleLineInVisualNumber(), boost::none, boost::none);
			break;
		case VERTICAL_RL: {
			boost::integer_range<widgetapi::NativeScrollPosition> range(0, 0);
			widgetapi::NativeScrollPosition pageStep;
			scrollBarParameters(*this, 0, nullptr, &range, &pageStep);
			configureScrollBar(*this, 0, *range.end()
				- pageStep - viewport->firstVisibleLineInVisualNumber(), boost::none, boost::none);
			configureScrollBar(*this, 1, viewport->inlineProgressionOffset(), boost::none, boost::none);
			break;
		}
		case VERTICAL_LR:
			configureScrollBar(*this, 0, viewport->firstVisibleLineInVisualNumber(), boost::none, boost::none);
			configureScrollBar(*this, 1, viewport->inlineProgressionOffset(), boost::none, boost::none);
			break;
		default:
			ASCENSION_ASSERT_NOT_REACHED();
	}
//	closeCompletionProposalsPopup(*this);
	hideToolTip();

	// 2. calculate pixels to scroll
	const BlockFlowDirection writingMode = textRenderer().computedBlockFlowDirection();
	geometry::BasicDimension<int16_t> scrollOffsetsInPixels;
	geometry::dx(scrollOffsetsInPixels) = static_cast<int16_t>(inlineProgressionScrollOffsetInUserUnits(*viewport, offsets.ipd()));
	geometry::dy(scrollOffsetsInPixels) = static_cast<int16_t>(textRenderer().baselineDistance(
		boost::irange(oldLine, VisualLine(viewport->firstVisibleLineInLogicalNumber(), viewport->firstVisibleSublineInLogicalLine()))));
	geometry::dy(scrollOffsetsInPixels) -= static_cast<int16_t>(
		*textRenderer().layouts().at(oldLine.line).lineMetrics(oldLine.subline).extent().begin());
	geometry::dy(scrollOffsetsInPixels) += static_cast<int16_t>(
		*textRenderer().layouts().at(viewport->firstVisibleLineInLogicalNumber()).lineMetrics(viewport->firstVisibleSublineInLogicalLine()).extent().begin());
	if(isVertical(writingMode)) {
		geometry::transpose(scrollOffsetsInPixels);
		if(writingMode == VERTICAL_RL)
			geometry::dx(scrollOffsetsInPixels) = -geometry::dx(scrollOffsetsInPixels);
	}

	// 3. scroll the graphics device
	const graphics::Rectangle& boundsToScroll = viewport->boundsInView();
	if(abs(offsets.bpd()) >= static_cast<SignedIndex>(viewport->numberOfVisibleLines())
			|| abs(offsets.ipd()) >= static_cast<SignedIndex>(viewport->numberOfVisibleCharactersInLine()))
		widgetapi::scheduleRedraw(*this, boundsToScroll, false);	// repaint all if the amount of the scroll is over a page
	else {
		// scroll image by BLIT
		// TODO: direct call of Win32 API.
		::ScrollWindowEx(handle().get(),
			-geometry::dx(scrollOffsetsInPixels), -geometry::dy(scrollOffsetsInPixels), nullptr, &boundsToScroll, nullptr, nullptr, SW_INVALIDATE);
		// invalidate bounds newly entered into the viewport
		if(geometry::dx(scrollOffsetsInPixels) > 0)
			widgetapi::scheduleRedraw(*this, graphics::Rectangle(
				geometry::topLeft(boundsToScroll),
				Dimension(geometry::_dx = geometry::dx(scrollOffsetsInPixels), geometry::_dy = geometry::dy(boundsToScroll))), false);
		else if(geometry::dx(scrollOffsetsInPixels) < 0)
			widgetapi::scheduleRedraw(*this, graphics::Rectangle(
				geometry::topRight(boundsToScroll),
				Dimension(geometry::_dx = geometry::dx(scrollOffsetsInPixels), geometry::_dy = geometry::dy(boundsToScroll))), false);
		if(geometry::dy(scrollOffsetsInPixels) > 0)
			widgetapi::scheduleRedraw(*this, graphics::Rectangle(
				geometry::topLeft(boundsToScroll),
				Dimension(geometry::_dx = geometry::dx(boundsToScroll), geometry::_dy = geometry::dy(scrollOffsetsInPixels))), false);
		else if(geometry::dy(scrollOffsetsInPixels) < 0)
			widgetapi::scheduleRedraw(*this, graphics::Rectangle(
				geometry::bottomLeft(boundsToScroll),
				Dimension(geometry::_dx = geometry::dx(boundsToScroll), geometry::_dy = geometry::dy(scrollOffsetsInPixels))), false);
	}

	// 4. scroll the ruler
	rulerPainter_->scroll(oldLine);

	// 5. repaint
	widgetapi::redrawScheduledRegion(*this);
}

/// @see VisualLinesListener#visualLinesDeleted
void TextViewer::visualLinesDeleted(const boost::integer_range<Index>& lines, Index sublines, bool longestLineChanged) /*throw()*/ {
	scrolls_.changed = true;
	const shared_ptr<const TextViewport> viewport(textRenderer().viewport());
	if(*lines.end() < viewport->firstVisibleLineInLogicalNumber()) {	// deleted before visible area
//		scrolls_.firstVisibleLine.line -= length(lines);
//		scrolls_.vertical.position -= static_cast<int>(sublines);
//		scrolls_.vertical.maximum -= static_cast<int>(sublines);
		repaintRuler();
	} else if(*lines.begin() > viewport->firstVisibleLineInLogicalNumber()	// deleted the first visible line and/or after it
			|| (*lines.begin() == viewport->firstVisibleLineInLogicalNumber() && viewport->firstVisibleSublineInLogicalLine() == 0)) {
//		scrolls_.vertical.maximum -= static_cast<int>(sublines);
		redrawLine(*lines.begin(), true);
	} else {	// deleted lines contain the first visible line
//		scrolls_.firstVisibleLine.line = lines.beginning();
//		scrolls_.updateVertical(*this);
		redrawLine(*lines.begin(), true);
	}
	if(longestLineChanged)
		scrolls_.resetBars(*this, 'i', false);
}

/// @see VisualLinesListener#visualLinesInserted
void TextViewer::visualLinesInserted(const boost::integer_range<Index>& lines) /*throw()*/ {
	scrolls_.changed = true;
	const shared_ptr<const TextViewport> viewport(textRenderer().viewport());
	if(*lines.end() < viewport->firstVisibleLineInLogicalNumber()) {	// inserted before visible area
//		scrolls_.firstVisibleLine.line += length(lines);
//		scrolls_.vertical.position += static_cast<int>(length(lines));
//		scrolls_.vertical.maximum += static_cast<int>(length(lines));
		repaintRuler();
	} else if(*lines.begin() > viewport->firstVisibleLineInLogicalNumber()	// inserted at or after the first visible line
			|| (*lines.begin() == viewport->firstVisibleLineInLogicalNumber() && viewport->firstVisibleSublineInLogicalLine() == 0)) {
//		scrolls_.vertical.maximum += static_cast<int>(length(lines));
		redrawLine(*lines.begin(), true);
	} else {	// inserted around the first visible line
//		scrolls_.firstVisibleLine.line += length(lines);
//		scrolls_.updateVertical(*this);
		redrawLine(*lines.begin(), true);
	}
}

/// @see VisualLinesListener#visualLinesModified
void TextViewer::visualLinesModified(const boost::integer_range<Index>& lines,
		SignedIndex sublinesDifference, bool documentChanged, bool longestLineChanged) /*throw()*/ {
	if(sublinesDifference == 0)	// number of visual lines was not changed
		redrawLines(lines);
	else {
		const shared_ptr<const TextViewport> viewport(textRenderer().viewport());
		scrolls_.changed = true;
		if(*lines.end() < viewport->firstVisibleLineInLogicalNumber()) {	// changed before visible area
//			scrolls_.vertical.position += sublinesDifference;
//			scrolls_.vertical.maximum += sublinesDifference;
			repaintRuler();
		} else if(*lines.begin() > viewport->firstVisibleLineInLogicalNumber()	// changed at or after the first visible line
				|| (*lines.begin() == viewport->firstVisibleLineInLogicalNumber() && viewport->firstVisibleSublineInLogicalLine() == 0)) {
//			scrolls_.vertical.maximum += sublinesDifference;
			redrawLine(*lines.begin(), true);
		} else {	// changed lines contain the first visible line
//			scrolls_.updateVertical(*this);
			redrawLine(*lines.begin(), true);
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
		widgetapi::Cursor::show();
		widgetapi::releaseInput(*viewer_);
		vanished_ = false;
	}
}

void TextViewer::CursorVanisher::vanish() {
	if(!vanished_ && viewer_->configuration().vanishesCursor && widgetapi::hasFocus(*viewer_)) {
		vanished_ = true;
		widgetapi::Cursor::hide();
		widgetapi::grabInput(*viewer_);
	}
}

bool TextViewer::CursorVanisher::vanished() const {
	return vanished_;
}


// TextViewer.Renderer ////////////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param viewer The text viewer
 */
TextViewer::Renderer::Renderer(TextViewer& viewer) :
		TextRenderer(viewer.presentation(), systemFonts(),
		geometry::size(viewer.textAreaContentRectangle())), viewer_(viewer) {
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
TextViewer::Renderer::Renderer(const Renderer& other, TextViewer& viewer) : TextRenderer(other), viewer_(viewer) {
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
		const graphics::Rectangle clientBounds(viewer_.bounds(false));
		const PhysicalFourSides<Scalar>& spaces(viewer_.spaceWidths());
		if(isHorizontal(viewer_.textRenderer().writingMode().blockFlowDirection))
			layouts().invalidateIf(Local(geometry::dx(clientBounds) - spaces.left - spaces.right));
		else
			layouts().invalidateIf(Local(geometry::dy(clientBounds) - spaces.top - spaces.bottom));
	}
}
#endif // ASCENSION_ABANDONED_AT_VERSION_08

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
/// @see TextRenderer#width
Scalar TextViewer::Renderer::width() const /*throw()*/ {
	const LineWrapConfiguration& lwc = viewer_.configuration().lineWrap;
	if(!lwc.wraps())
		return (viewer_.horizontalScrollBar().range().end() + 1) * viewer_.textRenderer().defaultFont()->metrics().averageCharacterWidth();
	else if(lwc.wrapsAtWindowEdge()) {
		const graphics::Rectangle clientBounds(viewer_.bounds(false));
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


// CurrentLineHighlighter /////////////////////////////////////////////////////////////////////////

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
const TextLineColorSpecifier::Priority CurrentLineHighlighter::LINE_COLOR_PRIORITY = 0x40;

/**
 * Constructor.
 * @param caret The caret
 * @param foreground The initial foreground color
 * @param background The initial background color
 */
CurrentLineHighlighter::CurrentLineHighlighter(Caret& caret,
		const boost::optional<Color>& foreground, const boost::optional<Color>& background)
		: caret_(&caret), foreground_(foreground), background_(background) {
	shared_ptr<TextLineColorSpecifier> temp(this);
	caret.textViewer().presentation().addTextLineColorSpecifier(temp);
	caret.addListener(*this);
	caret.addStateListener(*this);
	caret.addLifeCycleListener(*this);
}

/// Destructor.
CurrentLineHighlighter::~CurrentLineHighlighter() /*noexcept*/ {
	if(caret_ != nullptr) {
		caret_->removeListener(*this);
		caret_->removeStateListener(*this);
		caret_->textViewer().presentation().removeTextLineColorSpecifier(*this);
	}
}

/// Returns the background color.
const boost::optional<Color>& CurrentLineHighlighter::background() const /*noexcept*/ {
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
const boost::optional<Color>& CurrentLineHighlighter::foreground() const /*noexcept*/ {
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

/// @see TextLineColorSpecifier#specifyTextLineColors
TextLineColorSpecifier::Priority CurrentLineHighlighter::specifyTextLineColors(Index line,
		boost::optional<Color>& foreground, boost::optional<Color>& background) const {
	if(caret_ != nullptr && isSelectionEmpty(*caret_) && k::line(*caret_) == line && widgetapi::hasFocus(caret_->textViewer())) {
		foreground = foreground_;
		background = background_;
		return LINE_COLOR_PRIORITY;
	} else {
		foreground = background = boost::none;
		return 0;
	}
}

/// @see CaretStateListener#selectionShapeChanged
void CurrentLineHighlighter::selectionShapeChanged(const Caret&) {
}

/**
 * Sets the background color and redraws the window.
 * @param color The background color to set
 */
void CurrentLineHighlighter::setBackground(const boost::optional<Color>& color) /*noexcept*/ {
	background_ = color;
}

/**
 * Sets the foreground color and redraws the window.
 * @param color The foreground color to set
 */
void CurrentLineHighlighter::setForeground(const boost::optional<Color>& color) /*noexcept*/ {
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
			if(at.offsetInLine >= *hyperlinks[i]->region().begin() && at.offsetInLine <= *hyperlinks[i]->region().end())
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
	viewer.textRenderer().setDirection(!viewer.presentation().computeWritingMode(&viewer.textRenderer()).inlineFlowDirection);
//	viewer.synchronizeWritingModeUI();
//	if(config.lineWrap.wrapsAtWindowEdge()) {
//		win32::AutoZeroSize<SCROLLINFO> scroll;
//		viewer.getScrollInformation(SB_HORZ, scroll);
//		viewer.setScrollInformation(SB_HORZ, scroll);
//	}
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
		return boost::make_optional(k::Region(position.line, boost::irange(offsetsInLine.first, offsetsInLine.second)));
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
		return source::getNearestIdentifier(
			viewer.document(), viewToModel(*viewer.textRenderer().viewport(),
			widgetapi::mapFromGlobal(viewer, widgetapi::Cursor::position())).characterIndex());
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
