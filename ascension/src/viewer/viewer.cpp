/**
 * @file viewer.cpp
 * @author exeal
 * @date 2003-2006 (was EditView.cpp and EditViewWindowMessages.cpp)
 * @date 2006-2011
 */

#include <ascension/rules.hpp>
#include <ascension/corelib/text/break-iterator.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/text-editor/command.hpp>
#include <ascension/text-editor/session.hpp>
#include <ascension/viewer/base/cursor.hpp>
#include <ascension/viewer/caret.hpp>
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


// TextViewer ///////////////////////////////////////////////////////////////

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

// local helpers
namespace {
	inline void abortIncrementalSearch(TextViewer& viewer) /*throw()*/ {
		if(texteditor::Session* session = viewer.document().session()) {
			if(session->incrementalSearcher().isRunning())
				session->incrementalSearcher().abort();
		}
	}
	inline NativeSize getCurrentCharacterSize(const TextViewer& viewer) {
		const Scalar cy = viewer.textRenderer().defaultFont()->metrics().cellHeight();
		const Caret& caret = viewer.caret();
		if(k::locations::isEndOfLine(caret))	// EOL
			return geometry::make<NativeSize>(viewer.textRenderer().defaultFont()->metrics().averageCharacterWidth(), cy);
		else {
			const TextLayout& layout = viewer.textRenderer().layouts().at(caret.line());
			const Scalar leading = geometry::x(layout.location(caret.column(), TextLayout::LEADING));
			const Scalar trailing = geometry::x(layout.location(caret.column(), TextLayout::TRAILING));
			return geometry::make<NativeSize>(static_cast<Scalar>(detail::distance(leading, trailing)), cy);
		}
	}
	inline void endIncrementalSearch(TextViewer& viewer) /*throw()*/ {
		if(texteditor::Session* session = viewer.document().session()) {
			if(session->incrementalSearcher().isRunning())
				session->incrementalSearcher().end();
		}
	}
} // namespace @0

/**
 * Constructor.
 * @param presentation the presentation
 */
TextViewer::TextViewer(Presentation& presentation, Widget* parent /* = 0 */, Style styles /* = WIDGET */)
		: ScrollableWidget(parent, styles), presentation_(presentation), tipText_(0),
#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
		accessibleProxy_(0),
#endif // !ASCENSION_NO_ACTIVE_ACCESSIBILITY
		imeCompositionActivated_(false), imeComposingCharacter_(false), mouseInputDisabledCount_(0) {
	renderer_.reset(new Renderer(*this));
//	renderer_->addFontListener(*this);
//	renderer_->addVisualLinesListener(*this);
	caret_.reset(new Caret(*this));
	caret_->addListener(*this);
	caret_->addStateListener(*this);
	rulerPainter_.reset(new detail::RulerPainter(*this));

	document().addListener(*this);
	document().addStateListener(*this);
	document().addRollbackListener(*this);

	// initializations of renderer_ and mouseInputStrategy_ are in initializeWindow()
}

/**
 * Copy-constructor. Unlike @c win32#Object class, this does not copy the window handle. For
 * more details, see the description of @c TextViewer.
 */
TextViewer::TextViewer(const TextViewer& other) : presentation_(other.presentation_), tipText_(0)
#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
		, accessibleProxy_(0)
#endif // !ASCENSION_NO_ACTIVE_ACCESSIBILITY
{
	renderer_.reset(new Renderer(*other.renderer_, *this));
//	renderer_->addFontListener(*this);
//	renderer_->addVisualLinesListener(*this);
	caret_.reset(new Caret(*this));
	caret_->addListener(*this);
	caret_->addStateListener(*this);
	rulerPainter_.reset(new detail::RulerPainter(*this));

	modeState_ = other.modeState_;

	imeCompositionActivated_ = imeComposingCharacter_ = false;
	mouseInputDisabledCount_ = 0;
	document().addListener(*this);
	document().addStateListener(*this);
	document().addRollbackListener(*this);
}

/// Destructor.
TextViewer::~TextViewer() {
	document().removeListener(*this);
	document().removeStateListener(*this);
	document().removeRollbackListener(*this);
	renderer_->removeDefaultFontListener(*this);
	renderer_->layouts().removeVisualLinesListener(*this);
	caret_->removeListener(*this);
	caret_->removeStateListener(*this);
	for(set<VisualPoint*>::iterator it = points_.begin(); it != points_.end(); ++it)
		(*it)->viewerDisposed();

	// 非共有データ
//	delete selection_;
	delete[] tipText_;
}

/// @see Widget#aboutToLoseFocus
void TextViewer::aboutToLoseFocus() {
	cursorVanisher_.restore();
	if(mouseInputStrategy_.get() != 0)
		mouseInputStrategy_->interruptMouseReaction(false);
/*	if(caret_->getMatchBracketsTrackingMode() != Caret::DONT_TRACK
			&& getCaret().getMatchBrackets().first != Position::INVALID_POSITION) {	// 対括弧の通知を終了
		FOR_EACH_LISTENERS()
			(*it)->onMatchBracketFoundOutOfView(Position::INVALID_POSITION);
	}
	if(completionWindow_->isWindow() && newWindow != completionWindow_->getSafeHwnd())
		closeCompletionProposalsPopup(*this);
*/	abortIncrementalSearch(*this);
	if(imeCompositionActivated_) {	// stop IME input
#if defined(ASCENSION_WINDOW_SYSTEM_WIN32)
		win32::Handle<HIMC> imc(::ImmGetContext(identifier().get()),
			bind1st(ptr_fun(&::ImmReleaseContext), identifier().get()));
		::ImmNotifyIME(imc.get(), NI_COMPOSITIONSTR, CPS_CANCEL, 0);
#endif // ASCENSION_WINDOW_SYSTEM_WIN32
	}
//	if(currentWin32WindowMessage().wParam != get()) {
//		hideCaret();
//		::DestroyCaret();
//	}
	redrawLines(makeRange(caret().beginning().line(), caret().end().line() + 1));
	redrawScheduledRegion();
}

/// @see CaretListener#caretMoved
void TextViewer::caretMoved(const Caret& self, const k::Region& oldRegion) {
	if(!isVisible())
		return;
	const k::Region newRegion(self.selectedRegion());
	bool changed = false;

	// adjust the caret
	if(!isFrozen() && (hasFocus() /*|| completionWindow_->hasFocus()*/))
		updateCaretPosition();

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
				const length_t i[2] = {oldRegion.end().line, newRegion.end().line};
				redrawLines(makeRange(min(i[0], i[1]), max(i[0], i[1]) + 1));
			} else if(oldRegion.end() == newRegion.end()) {	// the end point didn't change
				const length_t i[2] = {oldRegion.beginning().line, newRegion.beginning().line};
				redrawLines(makeRange(min(i[0], i[1]), max(i[0], i[1]) + 1));
			} else {	// the both points changed
				if((oldRegion.beginning().line >= newRegion.beginning().line && oldRegion.beginning().line <= newRegion.end().line)
						|| (oldRegion.end().line >= newRegion.beginning().line && oldRegion.end().line <= newRegion.end().line)) {
					const length_t i[2] = {
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

/**
 * Returns the document position nearest from the specified point.
 * @param p The coordinates of the point. Can be outside of the window
 * @param edge If set @c TextLayout#LEADING, the result is the leading of the character at @a pt.
 *             Otherwise the result is the position nearest @a pt
 * @param abortNoCharacter If set to @c true, this method returns @c Position#INVALID_POSITION
 *                         immediately when @a pt hovered outside of the text layout (e.g. far left
 *                         or right of the line, beyond the last line, ...)
 * @param snapPolicy Which character boundary the returned position snapped to
 * @return The document position
 * @throw UnknownValueException @a edge and/or snapPolicy are invalid
 * @see #localPointForCharacter, #hitTest, graphics#font#LineLayout#offset
 */
k::Position TextViewer::characterForLocalPoint(const NativePoint& p, TextLayout::Edge edge,
		bool abortNoCharacter /* = false */, k::locations::CharacterUnit snapPolicy /* = k::locations::GRAPHEME_CLUSTER */) const {
	k::Position result;

	// determine the logical line
	length_t subline;
	bool outside;
	mapViewportBpdToLine(geometry::y(p), &result.line, &subline, &outside);
	if(abortNoCharacter && outside)
		return k::Position();
	const TextLayout& layout = renderer_->layouts()[result.line];

	// determine the column
	const Scalar x = geometry::x(p) - getDisplayXOffset(result.line);
	if(edge == TextLayout::LEADING)
		result.column = layout.offset(geometry::make<NativePoint>(
			x, static_cast<Scalar>(renderer_->defaultFont()->metrics().linePitch() * subline)), &outside).first;
	else if(edge == TextLayout::TRAILING)
		result.column = layout.offset(geometry::make<NativePoint>(
			x, static_cast<Scalar>(renderer_->defaultFont()->metrics().linePitch() * subline)), &outside).second;
	else
		throw UnknownValueException("edge");
	if(abortNoCharacter && outside)
		return k::Position();

	// snap intervening position to the boundary
	if(result.column != 0 && snapPolicy != k::locations::UTF16_CODE_UNIT) {
		using namespace text;
		const String& s = document().line(result.line);
		const bool interveningSurrogates =
			surrogates::isLowSurrogate(s[result.column]) && surrogates::isHighSurrogate(s[result.column - 1]);
		if(snapPolicy == k::locations::UTF32_CODE_UNIT) {
			if(interveningSurrogates) {
				if(edge == TextLayout::LEADING)
					--result.column;
				else if(detail::distance<Scalar>(x, geometry::x(layout.location(result.column - 1)))
						<= detail::distance<Scalar>(x, geometry::x(layout.location(result.column + 1))))
					--result.column;
				else
					++result.column;
			}
		} else if(snapPolicy == k::locations::GRAPHEME_CLUSTER) {
			text::GraphemeBreakIterator<k::DocumentCharacterIterator> i(
				k::DocumentCharacterIterator(document(), k::Region(result.line, make_pair(0, s.length())), result));
			if(interveningSurrogates || !i.isBoundary(i.base())) {
				--i;
				if(edge == TextLayout::LEADING)
					result.column = i.base().tell().column;
				else {
					const k::Position backward(i.base().tell());
					const k::Position forward((++i).base().tell());
					result.column = ((detail::distance<Scalar>(x, geometry::x(layout.location(backward.column)))
						<= detail::distance<Scalar>(x, geometry::x(layout.location(forward.column)))) ? backward : forward).column;
				}
			}
		} else
			throw UnknownValueException("snapPolicy");
	}
	return result;
}

/**
 * Returns the content-rectangle, the portion in which the text content is placed.
 * @see #bounds, #spaceWidths
 */
NativeRectangle TextViewer::contentRectangle() const /*throw()*/ {
	const NativeRectangle window(bounds(false));
	const PhysicalFourSides<Scalar>& spaces = spaceWidths();
	PhysicalFourSides<Scalar> result = {
		geometry::left(window) + spaces.left, geometry::top(window) + spaces.top,
		geometry::right(window) + spaces.right, geometry::bottom(window) + spaces.bottom};
	switch(rulerPainter_->alignment()) {
		case detail::RulerPainter::LEFT:
			result.left += rulerPainter_->width();
			break;
		case detail::RulerPainter::TOP:
			result.top += rulerPainter_->width();
			break;
		case detail::RulerPainter::RIGHT:
			result.right -= rulerPainter_->width();
			break;
		case detail::RulerPainter::BOTTOM:
			result.bottom -= rulerPainter_->width();
			break;
		default:
			ASCENSION_ASSERT_NOT_REACHED();
	}
	return geometry::make<NativeRectangle>(
		geometry::make<NativePoint>(result.left, result.top),
		geometry::make<NativeSize>(result.right - result.left, result.bottom - result.top));
}

/// @see DefaultFontListener#defaultFontChanged
void TextViewer::defaultFontChanged() /*throw()*/ {
	rulerPainter_->update();
	scrollInfo_.resetBars(*this, 'b', true);
	updateScrollBars();
	recreateCaret();
	redrawLine(0, true);
}

/// @see kernel#DocumentStateListener#documentAccessibleRegionChanged
void TextViewer::documentAccessibleRegionChanged(const k::Document&) {
	if(document().isNarrowed())
		scrollTo(-1, -1, false);
	scheduleRedraw(false);
}

/// @see kernel#DocumentListener#documentAboutToBeChanged
void TextViewer::documentAboutToBeChanged(const k::Document&) {
	// do nothing
}

/// @see kernel#DocumentListener#documentChanged
void TextViewer::documentChanged(const k::Document&, const k::DocumentChange& change) {
	// cancel the active incremental search
	if(texteditor::Session* session = document().session()) {	// TODO: should TextViewer handle this? (I.S. would...)
		if(session->incrementalSearcher().isRunning())
			session->incrementalSearcher().abort();
	}

	// slide the frozen lines to be drawn
	if(isFrozen() && !freezeRegister_.linesToRedraw().isEmpty()) {
		length_t b = freezeRegister_.linesToRedraw().beginning();
		length_t e = freezeRegister_.linesToRedraw().end();
		if(change.erasedRegion().first.line != change.erasedRegion().second.line) {
			const length_t first = change.erasedRegion().first.line + 1, last = change.erasedRegion().second.line;
			if(b > last)
				b -= last - first + 1;
			else if(b > first)
				b = first;
			if(e != numeric_limits<length_t>::max()) {
				if(e > last)
					e -= last - first + 1;
				else if(e > first)
					e = first;
			}
		}
		if(change.insertedRegion().first.line != change.insertedRegion().second.line) {
			const length_t first = change.insertedRegion().first.line + 1, last = change.insertedRegion().second.line;
			if(b >= first)
				b += last - first + 1;
			if(e >= first && e != numeric_limits<length_t>::max())
				e += last - first + 1;
		}
		freezeRegister_.resetLinesToRedraw(makeRange(b, e));
	}
//	invalidateLines(region.beginning().line, !multiLine ? region.end().line : INVALID_INDEX);
	if(!isFrozen())
		rulerPainter_->update();
	if(scrollInfo_.changed)
		updateScrollBars();
}

/// @see kernel#DocumentStateListener#documentModificationSignChanged
void TextViewer::documentModificationSignChanged(const k::Document&) {
	// do nothing
}

/// @see kernel#DocumentStateListener#documentPropertyChanged
void TextViewer::documentPropertyChanged(const k::Document&, const k::DocumentPropertyKey&) {
	// do nothing
}

/// @see kernel#DocumentStateListener#documentReadOnlySignChanged
void TextViewer::documentReadOnlySignChanged(const k::Document&) {
	// do nothing
}

/// @see kernel#DocumentRollbackListener#documentUndoSequenceStarted
void TextViewer::documentUndoSequenceStarted(const k::Document&) {
	freeze();	// TODO: replace with AutoFreeze.
}

/// @see kernel#DocumentRollbackListener#documentUndoSequenceStopped
void TextViewer::documentUndoSequenceStopped(const k::Document&, const k::Position& resultPosition) {
	unfreeze();	// TODO: replace with AutoFreeze.
	if(resultPosition != k::Position() && hasFocus()) {
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
void TextViewer::drawIndicatorMargin(length_t /* line */, Context& /* context */, const NativeRectangle& /* rect */) {
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

/**
 * Returns the horizontal display offset from @c LineLayout coordinates to client coordinates.
 * @param line The line number
 * @return The offset
 */
Scalar TextViewer::getDisplayXOffset(length_t line) const {
	const PhysicalFourSides<Scalar>& spaces = spaceWidths();
	const TextLayout& layout = renderer_->layouts().at(line);
	const detail::PhysicalTextAnchor alignment(
		detail::computePhysicalTextAnchor(layout.anchor(), layout.writingMode().inlineFlowDirection));
	if(alignment == detail::LEFT /*|| ... != NO_JUSTIFICATION*/)	// TODO: this code ignores last visual line with justification.
		return spaces.left - scrollInfo_.x() * renderer_->defaultFont()->metrics().averageCharacterWidth();

	Scalar indent;
	const NativeRectangle clientBounds(bounds(false));
	if(renderer_->layouts().maximumInlineProgressionDimension() + spaces.left + spaces.right > geometry::dx(clientBounds)) {
		indent = renderer_->layouts().maximumInlineProgressionDimension() - layout.lineInlineProgressionDimension(0) + spaces.left;
		indent += (geometry::dx(clientBounds) - spaces.left - spaces.right) % renderer_->defaultFont()->metrics().averageCharacterWidth();
	} else
		indent = geometry::dx(clientBounds) - layout.lineInlineProgressionDimension(0) - spaces.right;
	if(alignment == detail::MIDDLE)
		indent /= 2;
	else
		assert(alignment == detail::RIGHT);
	return indent - static_cast<Scalar>(scrollInfo_.x()) * renderer_->defaultFont()->metrics().averageCharacterWidth();
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

	if(pos.column == document.getLineLength(pos.line))	// 指定位置に文字が無い
		return false;

	const LineLayout& layout = renderer_->getLineLayout(pos.line);
	const length_t subline = layout.getSubline(pos.column);
	const Char* const line = document.getLine(pos.line).data();
	const Char* const first = line + layout.getSublineOffset(subline);
	const Char* const last =
		line + ((subline < layout.getNumberOfSublines() - 1) ? layout.getSublineOffset(subline + 1) : document.getLineLength(pos.line));
	length_t linkLength;	// URIDetector の eatMailAddress 、eatUrlString で見つけたリンクテキストの長さ

	for(const Char* p = (pos.column > 200) ? first + pos.column - 200 : first; p <= first + pos.column; ) {
		if(p != first) {
			if((p[-1] >= L'A' && p[-1] <= L'Z')
					|| (p[-1] >= L'a' && p[-1] <= L'z')
					|| p[-1] == L'_') {
				++p;
				continue;
			}
		}
		if(0 != (linkLength = rules::URIDetector::eatURL(p, last, true) - p)) {
			if(p - first + linkLength > pos.column) {	// カーソル位置を越えた
				region.first.line = region.second.line = pos.line;
				region.first.column = p - line;
				region.second.column = region.first.column + linkLength;
				text.reset(new Char[linkLength + 1]);
				wmemcpy(text.get(), p, linkLength);
				text[linkLength] = 0;
				return true;
			}
			p += linkLength;	// 届かない場合は続行
		} else if(0 != (linkLength = rules::URIDetector::eatMailAddress(p, last, true) - p)) {
			if(p - first + linkLength > pos.column) {	// カーソル位置を越えた
				static const wchar_t MAILTO_PREFIX[] = L"mailto:";
				region.first.line = region.second.line = pos.line;
				region.first.column = p - line;
				region.second.column = region.first.column + linkLength;
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

/// @see Widget#focusGained
void TextViewer::focusGained() {
#ifdef ASCENSION_WINDOW_SYSTEM_WIN32
	// restore the scroll positions
	horizontalScrollBar().setPosition(scrollInfo_.horizontal.position);
	verticalScrollBar().setPosition(scrollInfo_.vertical.position);
#endif // ASCENSION_WINDOW_SYSTEM_WIN32

	// hmm...
//	if(/*sharedData_->options.appearance[SHOW_CURRENT_UNDERLINE] ||*/ !getCaret().isSelectionEmpty()) {
		redrawLines(makeRange(caret().beginning().line(), caret().end().line() + 1));
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
		return OUT_OF_VIEWPORT;

	const RulerConfiguration& rc = rulerConfiguration();
	if(rc.indicatorMargin.visible && geometry::includes(rulerPainter_->indicatorMarginBounds(), p))
		return INDICATOR_MARGIN;
	else if(rc.lineNumbers.visible && geometry::includes(rulerPainter_->lineNumbersBounds(), p))
		return LINE_NUMBERS;

	const PhysicalFourSides<Scalar>& spaces = spaceWidths();
	if(geometry::x(p) < geometry::left(localBounds) + spaces.left
			|| geometry::x(p) >= geometry::right(localBounds) - spaces.right
			|| geometry::y(p) < geometry::top(localBounds) + spaces.top
			|| geometry::y(p) >= geometry::bottom(localBounds) + spaces.bottom)
		return SIDE_SPACE;
	else
		return CONTENT_AREA;
}

/// @see Widget#keyPressed
void TextViewer::keyPressed(const KeyInput& input) {
	if(mouseInputStrategy_.get() != 0)
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
			CaretMovementCommand(*this, &k::locations::backwardPage, hasModifier<UserInput::SHIFT_DOWN>(input))();
		break;
	case keyboardcodes::NEXT_OR_PAGE_DOWN:	// [PageDown]
		if(hasModifier<UserInput::CONTROL_DOWN>(input))
			onVScroll(SB_PAGEDOWN, 0, win32::Handle<HWND>());
		else
			CaretMovementCommand(*this, &k::locations::forwardPage, hasModifier<UserInput::SHIFT_DOWN>(input))();
		break;
	case keyboardcodes::HOME:	// [Home]
		if(hasModifier<UserInput::CONTROL_DOWN>(input))
			CaretMovementCommand(*this, &k::locations::beginningOfDocument, hasModifier<UserInput::SHIFT_DOWN>(input))();
		else
			CaretMovementCommand(*this, &k::locations::beginningOfVisualLine, hasModifier<UserInput::SHIFT_DOWN>(input))();
		break;
	case keyboardcodes::END:	// [End]
		if(hasModifier<UserInput::CONTROL_DOWN>(input))
			CaretMovementCommand(*this, &k::locations::endOfDocument, hasModifier<UserInput::SHIFT_DOWN>(input))();
		else
			CaretMovementCommand(*this, &k::locations::endOfVisualLine, hasModifier<UserInput::SHIFT_DOWN>(input))();
		break;
	case keyboardcodes::LEFT:	// [Left]
		if(hasModifier<UserInput::ALT_DOWN>(input) && hasModifier<UserInput::SHIFT_DOWN>(input)) {
			if(hasModifier<UserInput::CONTROL_DOWN>(input))
				RowSelectionExtensionCommand(*this, &k::locations::leftWord)();
			else
				RowSelectionExtensionCommand(*this, &k::locations::leftCharacter)();
		} else {
			if(hasModifier<UserInput::CONTROL_DOWN>(input))
				CaretMovementCommand(*this, &k::locations::leftWord, hasModifier<UserInput::SHIFT_DOWN>(input))();
			else
				CaretMovementCommand(*this, &k::locations::leftCharacter, hasModifier<UserInput::SHIFT_DOWN>(input))();
		}
		break;
	case keyboardcodes::UP:		// [Up]
		if(hasModifier<UserInput::ALT_DOWN>(input)
				&& hasModifier<UserInput::SHIFT_DOWN>(input) && !hasModifier<UserInput::CONTROL_DOWN>(input))
			RowSelectionExtensionCommand(*this, &k::locations::backwardVisualLine)();
		else if(hasModifier<UserInput::CONTROL_DOWN>(input) && !hasModifier<UserInput::SHIFT_DOWN>(input))
			scroll(0, -1, true);
		else
			CaretMovementCommand(*this, &k::locations::backwardVisualLine, hasModifier<UserInput::SHIFT_DOWN>(input))();
		break;
	case keyboardcodes::RIGHT:	// [Right]
		if(hasModifier<UserInput::ALT_DOWN>(input)) {
			if(hasModifier<UserInput::SHIFT_DOWN>(input)) {
				if(hasModifier<UserInput::CONTROL_DOWN>(input))
					RowSelectionExtensionCommand(*this, &k::locations::rightWord)();
				else
					RowSelectionExtensionCommand(*this, &k::locations::rightCharacter)();
			} else
				CompletionProposalPopupCommand(*this)();
		} else {
			if(hasModifier<UserInput::CONTROL_DOWN>(input))
				CaretMovementCommand(*this, &k::locations::rightWord, hasModifier<UserInput::SHIFT_DOWN>(input))();
			else
				CaretMovementCommand(*this, &k::locations::rightCharacter, hasModifier<UserInput::SHIFT_DOWN>(input))();
		}
		break;
	case keyboardcodes::DOWN:	// [Down]
		if(hasModifier<UserInput::ALT_DOWN>(input)
				&& hasModifier<UserInput::SHIFT_DOWN>(input) && !hasModifier<UserInput::CONTROL_DOWN>(input))
			RowSelectionExtensionCommand(*this, &k::locations::forwardVisualLine)();
		else if(hasModifier<UserInput::CONTROL_DOWN>(input) && !hasModifier<UserInput::SHIFT_DOWN>(input))
			onVScroll(SB_LINEDOWN, 0, win32::Handle<HWND>());
		else
			CaretMovementCommand(*this, &k::locations::forwardVisualLine, hasModifier<UserInput::SHIFT_DOWN>(input))();
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
		if(mouseInputStrategy_.get() != 0)
			mouseInputStrategy_->interruptMouseReaction(true);
	}
}

/**
 * Converts the specified location in the document to a point in the viewport-local coordinates.
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
 * @return The point in local coordinates. The block-progression-dimension addresses the baseline
 *         of the line
 * @throw BadPositionException @a position is outside of the document
 * @see #characterForLocalPoint, #hitTest, graphics#font#LineLayout#location
 */
NativePoint TextViewer::localPointForCharacter(const k::Position& position,
		bool fullSearchBpd, TextLayout::Edge edge /* = TextLayout::LEADING */) const {
//	checkInitialization();
	const bool horizontal = WritingModeBase::isHorizontal(utils::writingMode(*this).blockFlowDirection);
	const TextLayout& layout = renderer_->layouts().at(position.line);
	const NativePoint offset(layout.location(position.column, edge));
	const Scalar ipd = (horizontal ? geometry::x(offset) : geometry::y(offset)) + displayOffset(position.line);
	const NativePoint alignmentPoint(BaselineIterator(*this, position.line, fullSearchBpd).position());
	Scalar bpd = horizontal ? geometry::y(alignmentPoint) : geometry::x(alignmentPoint);
	if(fullSearchBpd || (bpd != numeric_limits<Scalar>::max() && bpd != numeric_limits<Scalar>::min()))
		bpd += horizontal ? geometry::y(offset) : geometry::x(offset);
	return geometry::make<NativePoint>(horizontal ? ipd : bpd, horizontal ? bpd : ipd);
}

/**
 * @param unlock
 */
void TextViewer::lockScroll(bool unlock /* = false */) {
	if(!unlock)
		++scrollInfo_.lockCount;
	else if(scrollInfo_.lockCount != 0)
		--scrollInfo_.lockCount;
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
Scalar TextViewer::mapLineToViewportBpd(length_t line, bool fullSearch) const {
	const PhysicalFourSides<Scalar> spaces(spaceWidths());
	if(line == scrollInfo_.firstVisibleLine) {
		if(scrollInfo_.firstVisibleSubline == 0)
			return spaces.top;
		else
			return fullSearch ? spaces.top
				- static_cast<Scalar>(renderer_->defaultFont()->metrics().linePitch() * scrollInfo_.firstVisibleSubline) : numeric_limits<Scalar>::min();
	} else if(line > scrollInfo_.firstVisibleLine) {
		const Scalar lineSpan = renderer_->defaultFont()->metrics().linePitch();
		const NativeRectangle clientBounds(bounds(false));
		Scalar y = spaces.top;
		y += lineSpan * static_cast<Scalar>(
			renderer_->layouts().numberOfSublinesOfLine(scrollInfo_.firstVisibleLine) - scrollInfo_.firstVisibleSubline);
		for(length_t i = scrollInfo_.firstVisibleLine + 1; i < line; ++i) {
			y += lineSpan * static_cast<Scalar>(renderer_->layouts().numberOfSublinesOfLine(i));
			if(y >= geometry::dy(clientBounds) && !fullSearch)
				return numeric_limits<Scalar>::max();
		}
		return y;
	} else if(!fullSearch)
		return numeric_limits<Scalar>::min();
	else {
		const Scalar linePitch = renderer_->defaultFont()->metrics().linePitch();
		Scalar y = spaces.top - static_cast<Scalar>(linePitch * scrollInfo_.firstVisibleSubline);
		for(length_t i = scrollInfo_.firstVisibleLine - 1; ; --i) {
			y -= static_cast<Scalar>(renderer_->layouts().numberOfSublinesOfLine(i) * linePitch);
			if(i == line)
				break;
		}
		return y;
	}
}
#endif
/**
 * Converts the distance from the before-edge of the viewport into the logical line and visual
 * subline offset.
 * @param bpd The distance from the before-edge of the viewport in pixels
 * @param[out] line The logical line index. Can be @c null if not needed
 * @param[out] subline The offset from the first line in @a line. Can be @c null if not needed
 * @param[out] snapped @c true if there was not a line at @a bpd. Optional
 * @see #BaselineIterator, #mapLineToViewportBpd, TextRenderer#offsetVisualLine
 */
void TextViewer::mapLocalPointToLine(
		const graphics::NativePoint& p, length_t* line, length_t* subline, bool* snapped /* = 0 */) const /*throw()*/ {
	const NativeRectangle localBounds(bounds(false));
	switch(utils::writingMode(*this).blockFlowDirection) {
		case WritingModeBase::HORIZONTAL_TB:
			return mapViewportBpdToLine(geometry::y(p) - localBounds.top, line, subline, snapped);
		case WritingModeBase::VERTICAL_RL:
			return mapViewportBpdToLine(localBounds.right - geometry::x(p), line, subline, snapped);
		case WritingModeBase::VERTICAL_LR:
			return mapViewportBpdToLine(geometry::x(p) - localBounds.left, line, subline, snapped);
		default:
			ASCENSION_ASSERT_NOT_REACHED();
	}
}

/**
 * Converts the distance from the before-edge of the viewport into the logical line and visual
 * subline offset. The results are snapped to the first/last visible line in the viewport (this
 * includes partially visible line) if the given distance addresses outside of the viewport.
 * @param bpd The distance from the before-edge of the viewport in pixels
 * @param[out] line The logical line index. Can be @c null if not needed
 * @param[out] subline The offset from the first line in @a line. Can be @c null if not needed
 * @param[out] snapped @c true if there was not a line at @a bpd. Optional
 * @throw NullPointerException Both @a line and @a subline are @c null
 * @see #BaselineIterator, TextRenderer#offsetVisualLine
 */
void TextViewer::mapViewportBpdToLine(Scalar bpd, length_t* line, length_t* subline, bool* snapped /* = 0 */) const /*throw()*/ {
	if(line == 0 && subline == 0)
		throw NullPointerException("line and subline");
	const WritingMode<false> writingMode(utils::writingMode(*this));
	const PhysicalFourSides<Scalar>& physicalSpaces = spaceWidths();
	AbstractFourSides<Scalar> abstractSpaces;
	mapPhysicalToAbstract(writingMode, physicalSpaces, abstractSpaces);
	const Scalar before = abstractSpaces.before;
	const Scalar after = (WritingModeBase::isHorizontal(writingMode.blockFlowDirection) ?
		geometry::dy(bounds(false)) : geometry::dx(bounds(false))) - abstractSpaces.after;

	pair<length_t, length_t> result;	// 'first' for 'line', 'second' for 'subline'
	bool outside;						// for 'snapped'
	firstVisibleLine(&result.first, 0, &result.second);
	if(bpd <= before)
		outside = bpd != before;
	else {
		const bool beyondAfter = bpd >= after;
		if(beyondAfter)
			bpd = after;
		Scalar lineBefore = before;
		const TextLayout* layout = &textRenderer().layouts()[result.first];
		while(result.second > 0)	// back to the first subline
			lineBefore -= layout->lineMetrics(--result.second).height();
		while(true) {
			assert(bpd >= lineBefore);
			Scalar lineAfter = lineBefore;
			for(length_t sl = 0; sl < layout->numberOfLines(); ++sl)
				lineAfter += layout->lineMetrics(sl).height();
			if(bpd < lineAfter) {
				result.second = layout->locateLine(bpd - lineBefore, outside);
				if(!outside)
					break;	// bpd is this line
				assert(result.second == layout->numberOfLines() - 1);
			}
			layout = &textRenderer().layouts()[++result.first];
			lineBefore = lineAfter;
		}
		outside = beyondAfter;
	}
	if(line != 0)
		*line = result.first;
	if(subline != 0)
		*subline = result.second;
	if(snapped != 0)
		*snapped = outside;
}

/// @see CaretStateListener#matchBracketsChanged
void TextViewer::matchBracketsChanged(const Caret& self, const pair<k::Position, k::Position>& oldPair, bool outsideOfView) {
	const pair<k::Position, k::Position>& newPair = self.matchBrackets();
	if(newPair.first != k::Position()) {
		assert(newPair.second != k::Position());
		redrawLine(newPair.first.line);
		if(!isFrozen())
			redrawScheduledRegion();
		if(newPair.second.line != newPair.first.line) {
			redrawLine(newPair.second.line);
			if(!isFrozen())
				redrawScheduledRegion();
		}
		if(oldPair.first != k::Position()	// clear the previous highlight
				&& oldPair.first.line != newPair.first.line && oldPair.first.line != newPair.second.line) {
			redrawLine(oldPair.first.line);
			if(!isFrozen())
				redrawScheduledRegion();
		}
		if(oldPair.second != k::Position() && oldPair.second.line != newPair.first.line
				&& oldPair.second.line != newPair.second.line && oldPair.second.line != oldPair.first.line)
			redrawLine(oldPair.second.line);
	} else {
		if(oldPair.first != k::Position()) {	// clear the previous highlight
			assert(oldPair.second != k::Position());
			redrawLine(oldPair.first.line);
			if(!isFrozen())
				redrawScheduledRegion();
			if(oldPair.second.line != oldPair.first.line)
				redrawLine(oldPair.second.line);
		}
	}
}

/// @see Widget#mouseDoubleClicked
void TextViewer::mouseDoubleClicked(const MouseButtonInput& input) {
	if(allowsMouseInput() && mouseInputStrategy_.get() != 0)
		mouseInputStrategy_->mouseButtonInput(MouseInputStrategy::DOUBLE_CLICKED, input);
}

/// @see Widget#mouseMoved
void TextViewer::mouseMoved(const LocatedUserInput& input) {
	cursorVanisher_.restore();
	if(allowsMouseInput() && mouseInputStrategy_.get() != 0)
		mouseInputStrategy_->mouseMoved(input);
}

/// @see Widget#mousePressed
void TextViewer::mousePressed(const MouseButtonInput& input) {
	cursorVanisher_.restore();
	if(allowsMouseInput() && mouseInputStrategy_.get() != 0)
		mouseInputStrategy_->mouseButtonInput(MouseInputStrategy::PRESSED, input);
}

/// @see Widget#mouseReleased
void TextViewer::mouseReleased(const MouseButtonInput& input) {
	if(allowsMouseInput() || input.button() == UserInput::BUTTON3_DOWN)
		cursorVanisher_.restore();
	if(allowsMouseInput() && mouseInputStrategy_.get() != 0)
		mouseInputStrategy_->mouseButtonInput(MouseInputStrategy::RELEASED, input);
}

/// @see Widget#mouseWheelChanged
void TextViewer::mouseWheelChanged(const MouseWheelInput& input) {
	cursorVanisher_.restore();
	if(allowsMouseInput() && mouseInputStrategy_.get() != 0)
		mouseInputStrategy_->mouseWheelRotated(input);
}

/**
 * Returns the number of the drawable columns in the window.
 * @return The number of columns
 */
length_t TextViewer::numberOfVisibleColumns() const /*throw()*/ {
	const bool horizontalBlockFlow = presentation::WritingModeBase::isHorizontal(utils::writingMode(*this).blockFlowDirection);
	graphics::Scalar ipd(horizontalBlockFlow ? graphics::geometry::dx(bounds(false)) : graphics::geometry::dy(bounds(false)));
	if(ipd == 0)
		return 0;
	ipd -= horizontalBlockFlow ? (spaceWidths().left + spaceWidths().right) : (spaceWidths().top + spaceWidths().bottom);
	ipd -= rulerPainter_->width();
	return ipd /= renderer_->defaultFont()->metrics().averageCharacterWidth();
}

/**
 * Returns the number of the drawable lines in the window.
 * @return The number of lines
 */
length_t TextViewer::numberOfVisibleLines() const /*throw()*/ {
	const bool horizontalBlockFlow = presentation::WritingModeBase::isHorizontal(utils::writingMode(*this).blockFlowDirection);
	graphics::Scalar bpd(horizontalBlockFlow ? graphics::geometry::dy(bounds(false)) : graphics::geometry::dx(bounds(false)));
	if(bpd == 0)
		return 0;
	bpd -= horizontalBlockFlow ? (spaceWidths().top + spaceWidths().bottom) : (spaceWidths().left + spaceWidths().right);
	return bpd /= renderer_->defaultFont()->metrics().linePitch();
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

	const k::Document& doc = document();
	const NativeRectangle clientBounds(bounds(false));

//	Timer tm(L"onPaint");

	const length_t lines = doc.numberOfLines();
	const int linePitch = renderer_->defaultFont()->metrics().linePitch();

	// paint the ruler
	rulerPainter_->paint(context);

	// draw horizontal margins
	Paint marginPaint;
	{
		if(tr1::shared_ptr<const TextLineStyle> lineStyle = presentation().globalTextStyle().defaultLineStyle) {
			if(lineStyle->defaultRunStyle.get() != 0)
				marginPaint = lineStyle->defaultRunStyle->background;
		}
		if(marginPaint == Paint())
			marginPaint = Paint(SystemColors::get(SystemColors::WINDOW));
	}
	const PhysicalFourSides<Scalar>& spaces = spaceWidths();
	// TODO: This code can't handle vertical writing mode correctly.
/*	if(margins.left > 0) {
		const int vrWidth = utils::isRulerLeftAligned(*this) ? rulerPainter_->width() : 0;
		context.setFillStyle(Paint(marginColor));
		context.fillRectangle(
			geometry::make<NativeRectangle>(
				geometry::make<NativePoint>(geometry::left(clientBounds) + vrWidth, geometry::top(scheduledBounds)),
				geometry::make<NativeSize>(margins.left - vrWidth, geometry::dy(scheduledBounds))));
	}
	if(margins.right > 0) {
		const int vrWidth = !utils::isRulerLeftAligned(*this) ? rulerPainter_->width() : 0;
		context.setFillStyle(Paint(marginColor));
		context.fillRectangle(
			geometry::make<NativeRectangle>(
				geometry::make<NativePoint>(geometry::right(clientBounds) - margins.right, geometry::top(scheduledBounds)),
				geometry::make<NativeSize>(margins.right - vrWidth, geometry::dy(scheduledBounds))));
	}
*/
	// paint lines
	NativeRectangle lineBounds(clientBounds);
	assert(geometry::isNormalized(lineBounds));
	geometry::range<geometry::X_COORDINATE>(lineBounds) = makeRange(
		geometry::left(lineBounds) + spaces.left, geometry::right(lineBounds) - spaces.right);
	geometry::range<geometry::Y_COORDINATE>(lineBounds) = makeRange(
		geometry::top(lineBounds) + spaces.top, geometry::bottom(lineBounds) - spaces.bottom);
	if(!geometry::isNormalized(lineBounds))
		geometry::resize(lineBounds, geometry::make<NativeSize>(0, 0));
	length_t line, subline;
	mapViewportBpdToLine(mapPhysicalToAbstract(utils::writingMode(*this), bounds(false), scheduledBounds, temp).before), &line, &subline);
	Scalar y = BaselineIterator(line, true).position();
	if(line < lines) {
		while(y < geometry::bottom(scheduledBounds) && line < lines) {
			// paint a logical line
			renderer_->renderLine(line, context, geometry::make<NativePoint>(getDisplayXOffset(line), y), &selectionAndMatchHighlighter, &eol, &lwm);
			y += linePitch * static_cast<Scalar>(renderer_->layouts().numberOfSublinesOfLine(line++));
			subline = 0;
		}
	}

	// paint 'margin-after'
	if(geometry::bottom(scheduledBounds) > y && y > spaces.top + linePitch - 1) {
		context.setFillStyle(marginPaint);
		context.fillRectangle(
			geometry::make<NativeRectangle>(
				geometry::make<NativePoint>(geometry::left(clientBounds) + spaces.left, y),
				geometry::make<NativeSize>(geometry::dx(clientBounds) - spaces.left - spaces.right, geometry::bottom(scheduledBounds) - y)));
	}

	// paint 'margin-before'
	if(spaces.top > 0) {
		context.setFillStyle(marginPaint);
		context.fillRectangle(
			geometry::make<NativeRectangle>(
				geometry::make<NativePoint>(geometry::left(clientBounds) + spaces.left, geometry::top(clientBounds)),
				geometry::make<NativeSize>(geometry::dx(clientBounds) - spaces.left - spaces.right, spaces.top)));
	}
}

/// Recreates and shows the caret. If the viewer does not have focus, nothing heppen.
void TextViewer::recreateCaret() {
	if(!hasFocus())
		return;
	::DestroyCaret();
	caretShape_.bitmap.reset();

	NativeSize solidSize(geometry::make<NativeSize>(0, 0));
	if(imeComposingCharacter_)
		solidSize = getCurrentCharacterSize(*this);
	else if(imeCompositionActivated_)
		geometry::dx(solidSize) = geometry::dy(solidSize) = 1;
	else if(caretShape_.shaper.get() != 0)
		caretShape_.shaper->shape(caretShape_.bitmap, solidSize, caretShape_.readingDirection);
	else {
		DefaultCaretShaper s;
		CaretShapeUpdater u(*this);
		static_cast<CaretShaper&>(s).install(u);
		static_cast<CaretShaper&>(s).shape(caretShape_.bitmap, solidSize, caretShape_.readingDirection);
		static_cast<CaretShaper&>(s).uninstall();
	}

	if(caretShape_.bitmap.get() != 0) {
		::CreateCaret(identifier().get(), caretShape_.bitmap.get(), 0, 0);
		BITMAP bitmap;
		::GetObjectW(caretShape_.bitmap.get(), sizeof(HBITMAP), &bitmap);
		caretShape_.width = bitmap.bmWidth;
	} else
		::CreateCaret(identifier().get(), 0, caretShape_.width = geometry::dx(solidSize), geometry::dy(solidSize));
	::ShowCaret(identifier().get());
	updateCaretPosition();
}

/**
 * Redraws the specified line on the view.
 * If the viewer is frozen, redraws after unfrozen.
 * @param line The line to be redrawn
 * @param following Set @c true to redraw also the all lines follow to @a line
 */
void TextViewer::redrawLine(length_t line, bool following) {
	redrawLines(makeRange(line, following ? numeric_limits<length_t>::max() : line + 1));
}

/**
 * Redraws the specified lines on the view. If the viewer is frozen, redraws after unfrozen.
 * @param lines The lines to be redrawn. The last line (@a lines.end()) is exclusive and this line
 *              will not be redrawn. If this value is @c std#numeric_limits<length_t>#max(), this
 *              method redraws the first line (@a lines.beginning()) and the following all lines
 * @throw kernel#BadRegionException @a lines intersects outside of the document
 */
void TextViewer::redrawLines(const Range<length_t>& lines) {
//	checkInitialization();

	if(lines.end() != numeric_limits<length_t>::max() && lines.end() >= document().numberOfLines())
		throw k::BadRegionException(k::Region(k::Position(lines.beginning(), 0), k::Position(lines.end(), 0)));

	if(isFrozen()) {
		freezeRegister_.addLinesToRedraw(lines);
		return;
	}

	if(lines.end() - 1 < scrollInfo_.firstVisibleLine)
		return;

#ifdef _DEBUG
	if(DIAGNOSE_INHERENT_DRAWING)
		win32::DumpContext()
			<< L"@TextViewer.redrawLines invalidates lines ["
			<< static_cast<unsigned long>(lines.beginning())
			<< L".." << static_cast<unsigned long>(lines.end()) << L"]\n";
#endif // _DEBUG

	const WritingMode<false> writingMode = utils::writingMode(*this);
	const NativeRectangle viewport(bounds(false));
	AbstractFourSides<Scalar> abstractBounds;
	mapPhysicalToAbstract(writingMode, viewport, viewport, abstractBounds);

	// calculate before and after edges of a rectangle to redraw
	BaselineIterator baseline(*this, lines.beginning(), false);
	if(*baseline != numeric_limits<Scalar>::min())
		abstractBounds.before = *baseline - textRenderer().layouts().at(lines.beginning()).lineMetrics(0).ascent();
	baseline += lines.length();
	if(*baseline != numeric_limits<Scalar>::max())
		abstractBounds.after = *baseline + textRenderer().layouts().at(baseline.line()).extent().end();
	NativeRectangle boundsToRedraw(viewport);
	mapAbstractToPhysical(utils::writingMode(*this), viewport, abstractBounds, boundsToRedraw);

	scheduleRedraw(boundsToRedraw, false);
}

/// Redraws the ruler.
void TextViewer::repaintRuler() {
	NativeRectangle r(bounds(false));
	if(utils::isRulerLeftAligned(*this))
		geometry::range<geometry::X_COORDINATE>(r) =
			makeRange(geometry::left(r), geometry::left(r) + rulerPainter_->width());
	else
		geometry::range<geometry::X_COORDINATE>(r) =
			makeRange(geometry::right(r) - rulerPainter_->width(), geometry::right(r));
	scheduleRedraw(r, false);
}

/**
 * Scrolls the viewer.
 * @param dx The number of units to scroll horizontally. A positive value means rightward
 * @param dy The number of units to scroll vertically. A positive value means bottomward
 * @param redraw Set @c true if redraws the viewport after scroll
 */
void TextViewer::scroll(int dx, int dy, bool redraw) {
//	checkInitialization();
	if(scrollInfo_.lockCount != 0)
		return;

	// preprocess and update the scroll bars
	if(dx != 0) {
		dx = min<int>(dx, scrollInfo_.horizontal.maximum - scrollInfo_.horizontal.pageSize - scrollInfo_.horizontal.position + 1);
		dx = max(dx, -scrollInfo_.horizontal.position);
		if(dx != 0) {
			scrollInfo_.horizontal.position += dx;
			if(!isFrozen())
				horizontalScrollBar().setPosition(scrollInfo_.horizontal.position);
		}
	}
	if(dy != 0) {
		dy = min<int>(dy, scrollInfo_.vertical.maximum - scrollInfo_.vertical.pageSize - scrollInfo_.vertical.position + 1);
		dy = max(dy, -scrollInfo_.vertical.position);
		if(dy != 0) {
			scrollInfo_.vertical.position += dy;
			renderer_->layouts().offsetVisualLine(scrollInfo_.firstVisibleLine, scrollInfo_.firstVisibleSubline, dy);
			if(!isFrozen())
				verticalScrollBar().setPosition(scrollInfo_.vertical.position);
		}
	}
	if(dx == 0 && dy == 0)
		return;
	else if(isFrozen()) {
		scrollInfo_.changed = true;
		return;
	}
//	closeCompletionProposalsPopup(*this);
	hideToolTip();

	// calculate pixels to scroll
	const PhysicalFourSides<Scalar>& spaces = spaceWidths();
	const NativeRectangle viewport(bounds(false));
	NativeRectangle clipBounds(viewport);
	assert(geometry::isNormalized(clipBounds));
	geometry::range<geometry::Y_COORDINATE>(clipBounds) = makeRange(
		geometry::top(clipBounds) + spaces.top, geometry::bottom(clipBounds) - spaces.bottom);
	const WritingMode<false> writingMode(utils::writingMode(*this));
	NativeSize pixelsToScroll;
	// inline-progression-direction
	if(WritingModeBase::isHorizontal(writingMode.blockFlowDirection))
		geometry::dx(pixelsToScroll) = dx * scrollRate(true) * textRenderer().defaultFont()->metrics().averageCharacterWidth();
	else
		geometry::dy(pixelsToScroll) = dy * scrollRate(false) * textRenderer().defaultFont()->metrics().averageCharacterWidth();
	// block-progression-direction
	signed_length_t linesToScroll;
	if(WritingModeBase::isHorizontal(writingMode.blockFlowDirection))
		linesToScroll = dy;
	else {
		assert(WritingModeBase::isVertical(writingMode.blockFlowDirection));
		if(writingMode.blockFlowDirection == WritingModeBase::VERTICAL_RL)
			linesToScroll = -dx;
		else if(writingMode.blockFlowDirection == WritingModeBase::VERTICAL_LR)
			linesToScroll = +dx;
		else
			ASCENSION_ASSERT_NOT_REACHED();
	}
	int dbpd = 0;
	while(linesToScroll > 0) {
	}

	// scroll
	if(static_cast<unsigned int>(abs(dy)) >= numberOfVisibleLines())
		scheduleRedraw(clipBounds, false);	// redraw all if the amount of the scroll is over a page
	else if(dx == 0) {	// only vertical
		::ScrollWindowEx(identifier().get(),
			0, -dy * scrollRate(false) * renderer_->defaultFont()->metrics().linePitch(), 0, &clipBounds, 0, 0, SW_INVALIDATE);
	} else {	// process the leading margin and the edit region independently
		// scroll the edit region
		geometry::range<geometry::X_COORDINATE>(clipBounds) = makeRange(
			geometry::left(clipBounds) + spaces.left, geometry::right(clipBounds) - spaces.right);
		if(static_cast<unsigned int>(abs(dx)) >= numberOfVisibleColumns())
			scheduleRedraw(clipBounds, false);	// redraw all if the amount of the scroll is over a page
		else
			::ScrollWindowEx(identifier().get(),
				-dx * scrollRate(true) * renderer_->defaultFont()->metrics().averageCharacterWidth(),
				-dy * scrollRate(false) * renderer_->defaultFont()->metrics().linePitch(),
				0, &clipBounds, 0, 0, SW_INVALIDATE);
		// scroll the vertical ruler
		if(dy != 0) {
			if(rulerPainter_->configuration().alignment == ALIGN_LEFT)
				geometry::range<geometry::X_COORDINATE>(clipBounds) = makeRange(
					geometry::left(clipBounds), geometry::left(clipBounds) + rulerPainter_->width());
			else
				geometry::range<geometry::X_COORDINATE>(clipBounds) = makeRange(
					geometry::right(clipBounds) - rulerPainter_->width(), geometry::right(clientBounds));
			::ScrollWindowEx(identifier().get(),
				0, -dy * scrollRate(false) * renderer_->defaultFont()->metrics().linePitch(), 0, &clipBounds, 0, 0, SW_INVALIDATE);
		}
	}

	// postprocess
	updateCaretPosition();
	if(redraw)
		redrawScheduledRegion();
	viewportListeners_.notify<bool, bool>(&ViewportListener::viewportChanged, dx != 0, dy != 0);
}

/// @see Widget#resized
void TextViewer::resized(State state, const NativeSize&) {
	utils::closeCompletionProposalsPopup(*this);
	if(state == MINIMIZED)
		return;

	// notify the tooltip
	win32::AutoZeroSize<TOOLINFOW> ti;
	const NativeRectangle viewerBounds(bounds(false));
	ti.hwnd = identifier().get();
	ti.uId = 1;
	ti.rect = viewerBounds;
	::SendMessageW(toolTip_, TTM_NEWTOOLRECT, 0, reinterpret_cast<LPARAM>(&ti));

	if(renderer_.get() == 0)
		return;

	if(configuration_.lineWrap.wrapsAtWindowEdge())
		renderer_->layouts().invalidate();
	displaySizeListeners_.notify(&DisplaySizeListener::viewerDisplaySizeChanged);
	scrollInfo_.resetBars(*this, 'b', true);
	updateScrollBars();
	rulerPainter_->update();
	if(rulerPainter_->configuration().alignment != ALIGN_LEFT) {
		recreateCaret();
//		redrawVerticalRuler();
		scheduleRedraw(false);	// hmm...
	}
}

/**
 * Scrolls the viewer to the specified position.
 * @param x The visual line of the position. If set -1, does not scroll in this direction
 * @param y The column of the position. If set -1, does not scroll in this direction
 * @param redraw Set @c true to redraw the window after scroll
 * @see #scroll
 */
void TextViewer::scrollTo(int x, int y, bool redraw) {
//	checkInitialization();
	if(x != -1)
		x = max(min<int>(x, scrollInfo_.horizontal.maximum - scrollInfo_.horizontal.pageSize + 1), 0);
	if(y != -1)
		y = max(min<int>(y, scrollInfo_.vertical.maximum - scrollInfo_.vertical.pageSize + 1), 0);
	const int dx = (x != -1) ? x - scrollInfo_.horizontal.position : 0;
	const int dy = (y != -1) ? y - scrollInfo_.vertical.position : 0;
	if(dx != 0 || dy != 0)
		scroll(dx, dy, redraw);	// does not work if scroll is lock
}

/**
 * Scrolls the viewer to the specified line.
 * @param line the logical line
 * @param redraw set @c true to redraw the window after scroll
 * @throw BadPositionException @a line is outside of the document
 */
void TextViewer::scrollTo(length_t line, bool redraw) {
	// TODO: not implemented.
//	checkInitialization();
	if(scrollInfo_.lockCount != 0)
		return;
	if(line >= document().numberOfLines())
		throw k::BadPositionException(k::Position(line, 0));
	scrollInfo_.firstVisibleLine = line;
	scrollInfo_.firstVisibleSubline = 0;
	length_t visualLine;
	if(configuration_.lineWrap.wraps())
		visualLine = line;
	else {
		// TODO: this code can be more faster.
		visualLine = 0;
		for(length_t i = 0; i < line; ++i)
			visualLine += renderer_->layouts().numberOfSublinesOfLine(i);
	}
	viewportListeners_.notify<bool, bool>(&ViewportListener::viewportChanged, true, true);
}

/// @see CaretStateListener#selectionShapeChanged
void TextViewer::selectionShapeChanged(const Caret& self) {
	if(!isFrozen() && !isSelectionEmpty(self))
		redrawLines(self.beginning().line(), self.end().line());
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
	if(ruler != 0)
		rulerPainter_->setConfiguration(*ruler);
	if(general != 0) {
		const Inheritable<ReadingDirection> oldReadingDirection(configuration_.readingDirection);
		assert(!oldReadingDirection.inherits());
		configuration_ = *general;
		displaySizeListeners_.notify(&DisplaySizeListener::viewerDisplaySizeChanged);
		renderer_->layouts().invalidate();

		if((oldReadingDirection == LEFT_TO_RIGHT && configuration_.readingDirection == RIGHT_TO_LEFT)
				|| (oldReadingDirection == RIGHT_TO_LEFT && configuration_.readingDirection == LEFT_TO_RIGHT))
			scrollInfo_.horizontal.position = scrollInfo_.horizontal.maximum
				- scrollInfo_.horizontal.pageSize - scrollInfo_.horizontal.position + 1;
		scrollInfo_.resetBars(*this, 'b', false);
		updateScrollBars();

		if(!isFrozen() && (hasFocus() /*|| getHandle() == Viewer::completionWindow_->getSafeHwnd()*/)) {
			recreateCaret();
			updateCaretPosition();
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
void TextViewer::setContentAssistant(auto_ptr<contentassist::ContentAssistant> newContentAssistant) /*throw()*/ {
	if(contentAssistant_.get() != 0)
		contentAssistant_->uninstall();	// $friendly-access
	(contentAssistant_ = newContentAssistant)->install(*this);	// $friendly-access
}

/**
 * Sets the mouse input strategy. An instance of @c TextViewer has the default strategy implemented
 * by @c DefaultMouseInputStrategy class as the construction.
 * @param newStrategy the new strategy or @c null
 * @param delegateOwnership set @c true to transfer the ownership into the callee
 * @throw IllegalStateException the window is not created yet
 */
void TextViewer::setMouseInputStrategy(MouseInputStrategy* newStrategy, bool delegateOwnership) {
//	checkInitialization();
	if(mouseInputStrategy_.get() != 0) {
		mouseInputStrategy_->interruptMouseReaction(false);
		mouseInputStrategy_->uninstall();
	}
	if(newStrategy != 0)
		mouseInputStrategy_.reset(newStrategy, delegateOwnership);
	else
		mouseInputStrategy_.reset(new DefaultMouseInputStrategy(), true);	// TODO: the two parameters don't have rationales.
	mouseInputStrategy_->install(*this);
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
	::SetTimer(identifier().get(), TIMERID_CALLTIP, timeToWait, 0);
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
 * Revokes the frozen state of the viewer.
 * @throw WindowNotInitialized The window is not initialized
 * @see #freeze, #isFrozen
 */
void TextViewer::unfreeze() {
//	checkInitialization();
	if(freezeRegister_.isFrozen()) {
		const Range<length_t> linesToRedraw(freezeRegister_.unfreeze());
		if(!freezeRegister_.isFrozen()) {
			if(scrollInfo_.changed) {
				updateScrollBars();
				scheduleRedraw(false);
			} else if(!linesToRedraw.isEmpty)())
				redrawLines(linesToRedraw);

			rulerPainter_->update();

			caretMoved(caret(), caret().selectedRegion());
			redrawScheduledRegion();
		}
	}
}

/// Moves the caret to valid position with current position, scroll context, and the fonts.
void TextViewer::updateCaretPosition() {
	if(!hasFocus() || isFrozen())
		return;

	NativePoint p(clientXYForCharacter(caret(), false, TextLayout::LEADING));
	const PhysicalFourSides<Scalar> spaces(xspaceWidths());
	NativeRectangle textArea(bounds(false));
	assert(geometry::isNormalized(textArea));
	geometry::range<geometry::X_COORDINATE>(textArea) = makeRange(
		geometry::left(textArea) + spaces.left, geometry::right(textArea) - spaces.right - 1);
	geometry::range<geometry::Y_COORDINATE>(textArea) = makeRange(
		geometry::top(textArea) + spaces.top, geometry::bottom(textArea) - spaces.bottom);

	if(!geometry::includes(textArea, p))
		geometry::y(p) = -renderer_->defaultFont()->metrics().linePitch();	// "hide" the caret
	else if(caretShape_.readingDirection == RIGHT_TO_LEFT
			|| renderer_->layouts()[caret().line()].bidiEmbeddingLevel(caret().column()) % 2 == 1)
		geometry::x(p) -= caretShape_.width;
	::SetCaretPos(geometry::x(p), geometry::y(p));
	updateIMECompositionWindowPosition();
}

/// Updates the scroll information.
void TextViewer::updateScrollBars() {
//	checkInitialization();
	if(renderer_.get() == 0)
		return;

#define ASCENSION_GET_SCROLL_MINIMUM(s)	(s.maximum/* * s.rate*/ - s.pageSize + 1)

	// about horizontal scroll bar
	bool wasNeededScrollbar = ASCENSION_GET_SCROLL_MINIMUM(scrollInfo_.horizontal) > 0;
	// scroll to leftmost/rightmost before the scroll bar vanishes
	long minimum = ASCENSION_GET_SCROLL_MINIMUM(scrollInfo_.horizontal);
	if(wasNeededScrollbar && minimum <= 0) {
		scrollInfo_.horizontal.position = 0;
		if(!isFrozen()) {
			scheduleRedraw(false);
			updateCaretPosition();
		}
	} else if(scrollInfo_.horizontal.position > minimum)
		scrollTo(minimum, -1, true);
	assert(ASCENSION_GET_SCROLL_MINIMUM(scrollInfo_.horizontal) > 0 || scrollInfo_.horizontal.position == 0);
	if(!isFrozen()) {
		ScrollBar& scrollBar = horizontalScrollBar();
		scrollBar.setRange(makeRange(0, configuration_.lineWrap.wrapsAtWindowEdge() ? 0 : scrollInfo_.horizontal.maximum));
		scrollBar.setPageStep(scrollInfo_.horizontal.pageSize);
		scrollBar.setPosition(scrollInfo_.horizontal.position);
//		win32::AutoZeroSize<SCROLLINFO> scroll;
//		scroll.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
//		scroll.nMax = configuration_.lineWrap.wrapsAtWindowEdge() ? 0 : scrollInfo_.horizontal.maximum;
//		scroll.nPage = scrollInfo_.horizontal.pageSize;
//		scroll.nPos = scrollInfo_.horizontal.position;
//		setScrollInformation(SB_HORZ, scroll, true);
	}

	// about vertical scroll bar
	wasNeededScrollbar = ASCENSION_GET_SCROLL_MINIMUM(scrollInfo_.vertical) > 0;
	minimum = ASCENSION_GET_SCROLL_MINIMUM(scrollInfo_.vertical);
	// validate scroll position
	if(minimum <= 0) {
		scrollInfo_.vertical.position = 0;
		scrollInfo_.firstVisibleLine = scrollInfo_.firstVisibleSubline = 0;
		if(!isFrozen()) {
			scheduleRedraw(false);
			updateCaretPosition();
		}
	} else if(scrollInfo_.vertical.position > minimum)
		scrollTo(-1, minimum, true);
	assert(ASCENSION_GET_SCROLL_MINIMUM(scrollInfo_.vertical) > 0 || scrollInfo_.vertical.position == 0);
	if(!isFrozen()) {
		ScrollBar& scrollBar = verticalScrollBar();
		scrollBar.setRange(makeRange(0, scrollInfo_.vertical.maximum));
		scrollBar.setPageStep(scrollInfo_.vertical.pageSize);
		scrollBar.setPosition(scrollInfo_.vertical.position);
//		win32::AutoZeroSize<SCROLLINFO> scroll;
//		scroll.fMask = SIF_DISABLENOSCROLL | SIF_PAGE | SIF_POS | SIF_RANGE;
//		scroll.nMax = scrollInfo_.vertical.maximum;
//		scroll.nPage = scrollInfo_.vertical.pageSize;
//		scroll.nPos = scrollInfo_.vertical.position;
//		setScrollInformation(SB_VERT, scroll, true);
	}

	scrollInfo_.changed = isFrozen();

#undef ASCENSION_GET_SCROLL_MINIMUM
}

/// @see VisualLinesListener#visualLinesDeleted
void TextViewer::visualLinesDeleted(const Range<length_t>& lines, length_t sublines, bool longestLineChanged) /*throw()*/ {
	scrollInfo_.changed = true;
	if(lines.end() < scrollInfo_.firstVisibleLine) {	// 可視領域より前が削除された
		scrollInfo_.firstVisibleLine -= lines.length();
		scrollInfo_.vertical.position -= static_cast<int>(sublines);
		scrollInfo_.vertical.maximum -= static_cast<int>(sublines);
		repaintRuler();
	} else if(lines.beginning() > scrollInfo_.firstVisibleLine
			|| (lines.beginning() == scrollInfo_.firstVisibleLine && scrollInfo_.firstVisibleSubline == 0)) {	// 可視先頭行以降が削除された
		scrollInfo_.vertical.maximum -= static_cast<int>(sublines);
		redrawLine(lines.beginning(), true);
	} else {	// 可視先頭行を含む範囲が削除された
		scrollInfo_.firstVisibleLine = lines.beginning();
		scrollInfo_.updateVertical(*this);
		redrawLine(lines.beginning(), true);
	}
	if(longestLineChanged)
		scrollInfo_.resetBars(*this, 'h', false);
}

/// @see VisualLinesListener#visualLinesInserted
void TextViewer::visualLinesInserted(const Range<length_t>& lines) /*throw()*/ {
	scrollInfo_.changed = true;
	if(lines.end() < scrollInfo_.firstVisibleLine) {	// 可視領域より前に挿入された
		scrollInfo_.firstVisibleLine += lines.length();
		scrollInfo_.vertical.position += static_cast<int>(lines.length());
		scrollInfo_.vertical.maximum += static_cast<int>(lines.length());
		repaintRuler();
	} else if(lines.beginning() > scrollInfo_.firstVisibleLine
			|| (lines.beginning() == scrollInfo_.firstVisibleLine && scrollInfo_.firstVisibleSubline == 0)) {	// 可視先頭行以降に挿入された
		scrollInfo_.vertical.maximum += static_cast<int>(lines.length());
		redrawLine(lines.beginning(), true);
	} else {	// 可視先頭行の前後に挿入された
		scrollInfo_.firstVisibleLine += lines.length();
		scrollInfo_.updateVertical(*this);
		redrawLine(lines.beginning(), true);
	}
}

/// @see VisualLinesListener#visualLinesModified
void TextViewer::visualLinesModified(const Range<length_t>& lines,
		signed_length_t sublinesDifference, bool documentChanged, bool longestLineChanged) /*throw()*/ {
	if(sublinesDifference == 0)	// 表示上の行数が変化しなかった
		redrawLines(lines);
	else {
		scrollInfo_.changed = true;
		if(lines.end() < scrollInfo_.firstVisibleLine) {	// 可視領域より前が変更された
			scrollInfo_.vertical.position += sublinesDifference;
			scrollInfo_.vertical.maximum += sublinesDifference;
			repaintRuler();
		} else if(lines.beginning() > scrollInfo_.firstVisibleLine
				|| (lines.beginning() == scrollInfo_.firstVisibleLine && scrollInfo_.firstVisibleSubline == 0)) {	// 可視先頭行以降が変更された
			scrollInfo_.vertical.maximum += sublinesDifference;
			redrawLine(lines.beginning(), true);
		} else {	// 可視先頭行を含む範囲が変更された
			scrollInfo_.updateVertical(*this);
			redrawLine(lines.beginning(), true);
		}
	}
	if(longestLineChanged) {
		scrollInfo_.resetBars(*this, 'h', false);
		scrollInfo_.changed = true;
	}
	if(!documentChanged && scrollInfo_.changed)
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
	if(textViewer_ != 0)
		textViewer_->freeze();
}

/// Destructor calls @c TextViewer#unfreeze.
AutoFreeze::~AutoFreeze() /*throw()*/ {
	try {
		if(textViewer_ != 0)
			textViewer_->unfreeze();
	} catch(...) {
		// ignore
	}
}


// TextViewer.BaselineIterator ////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param viewer
 * @param line
 * @param trackOutOfViewport
 */
TextViewer::BaselineIterator::BaselineIterator(const TextViewer& viewer, length_t line,
		bool trackOutOfViewport) : viewer_(viewer), tracksOutOfViewport_(trackOutOfViewport) {
	initializeWithFirstVisibleLine();
	advance(line - this->line());
}

/// @see detail#IteratorAdapter#advance
void TextViewer::BaselineIterator::advance(difference_type n) {
	if(n == 0)
		return;
	else if(n > 0 && line() + n > viewer_.document().numberOfLines())
		throw invalid_argument("n");
	else if(n < 0 && static_cast<length_t>(-n) - 1 > line())
		throw invalid_argument("n");

	const length_t destination = line() + n;
	if(!tracksOutOfViewport()
			&& (baseline_.first == numeric_limits<Scalar>::min() || baseline_.first == numeric_limits<Scalar>::max())) {
		if((n > 0 && baseline_.first == numeric_limits<Scalar>::max())
				|| (n < 0 && baseline_.first == numeric_limits<Scalar>::min())) {
			line_ = destination;
			subline_ = 0;
			return;
		}
		swap(*this, BaselineIterator(viewer_, destination, tracksOutOfViewport()));
		return;
	}

	Scalar viewportExtent;
	if(!tracksOutOfViewport() && n > 0) {
		const NativeRectangle clientBounds(viewer_.bounds(false));
		const PhysicalFourSides<Scalar> spaces(viewer_.spaceWidths());
		viewportExtent = WritingModeBase::isHorizontal(utils::writingMode(viewer_).blockFlowDirection) ?
			(geometry::dy(clientBounds) - spaces.top - spaces.bottom) : (geometry::dx(clientBounds) - spaces.left - spaces.right);
	}

	length_t ln = line(), subline = subline_;
	Scalar newBaseline = baseline_.first;
	const TextLayout* layout = &viewer_.textRenderer().layouts()[line()];
	if(n > 0) {
		newBaseline += layout->lineMetrics(subline_).descent();
		for(length_t ln = line(), subline = subline_; ; ) {
			if(++subline == layout->numberOfLines()) {
				subline = 0;
				if(++ln == viewer_.document().numberOfLines()) {
					newBaseline = numeric_limits<Scalar>::max();
					break;
				}
				layout = &viewer_.textRenderer().layouts()[++ln];
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
		newBaseline -= layout->lineMetrics(subline_).ascent();
		for(length_t ln = line(), subline = subline_; ; ) {
			if(subline == 0) {
				if(ln-- == 0) {
					subline = 0;
					newBaseline = numeric_limits<Scalar>::min();
					break;
				}
				layout = &viewer_.textRenderer().layouts()[ln];
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

	NativePoint newAxis(baseline_.second);
	switch(utils::writingMode(viewer_).blockFlowDirection) {
		case WritingModeBase::HORIZONTAL_TB:
			geometry::y(newAxis) += newBaseline - baseline_.first;
			break;
		case WritingModeBase::VERTICAL_RL:
			geometry::x(newAxis) -= newBaseline - baseline_.first;
			break;
		case WritingModeBase::VERTICAL_LR:
			geometry::x(newAxis) += newBaseline - baseline_.first;
			break;
		default:
			ASCENSION_ASSERT_NOT_REACHED();
	}

	// commit
	line_ = destination;
	subline_ = 0;
	baseline_ = make_pair(newBaseline, newAxis);
}

/// @see detail#IteratorAdapter#current
const TextViewer::BaselineIterator::reference TextViewer::BaselineIterator::current() const {
	return baseline_.first;
}

/// @internal Moves this iterator to the first visible line in the viewport.
void TextViewer::BaselineIterator::initializeWithFirstVisibleLine() {
	length_t firstVisibleLine, firstVisibleSubline;
	viewer_.firstVisibleLine(&firstVisibleLine, 0, &firstVisibleSubline);
	const Scalar baseline = viewer_.textRenderer().layouts().at(firstVisibleLine).lineMetrics(firstVisibleSubline).ascent();
	NativePoint axis;
	const NativeRectangle clientBounds(viewer_.bounds(false));
	switch(utils::writingMode(viewer_).blockFlowDirection) {
		case WritingModeBase::HORIZONTAL_TB:
			axis = geometry::make<NativePoint>(0, geometry::top(clientBounds) + viewer_.spaceWidths().top + baseline);
			break;
		case WritingModeBase::VERTICAL_RL:
			axis = geometry::make<NativePoint>(geometry::right(clientBounds) - viewer_.spaceWidths().right - baseline, 0);
			break;
		case WritingModeBase::VERTICAL_LR:
			axis = geometry::make<NativePoint>(geometry::left(clientBounds) + viewer_.spaceWidths().left + baseline, 0);
			break;
		default:
			ASCENSION_ASSERT_NOT_REACHED();
	}

	// commit
	line_ = firstVisibleLine;
	subline_ = firstVisibleSubline;
	baseline_ = make_pair(baseline, axis);
}

inline void TextViewer::BaselineIterator::invalidate() /*throw()*/ {
	geometry::x(baseline_.second) = geometry::y(baseline_.second) = 1;
}

inline bool TextViewer::BaselineIterator::isValid() const /*throw()*/ {
	return geometry::x(baseline_.second) != 0 && geometry::y(baseline_.second) != 0;
}
#if 0
void TextViewer::BaselineIterator::move(length_t line) {
	if(line >= viewer_.document().numberOfLines())
		throw k::BadPositionException(k::Position(line, 0));
	Scalar newBaseline;
	if(!isValid()) {
		length_t firstVisibleLine, firstVisibleSubline;
		viewer_.firstVisibleLine(&firstVisibleLine, 0, &firstVisibleSubline);
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
				newBaseline = viewer_.textRenderer().layouts()[line].lineMetrics(0).ascent();
			else if(!tracksOutOfViewport())
				newBaseline = numeric_limits<Scalar>::min();
			else {
				const TextLayout& layout = viewer_.textRenderer().layouts()[line];
				newBaseline = 0;
				for(length_t subline = firstVisibleSubline - 1; ; --subline) {
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
			for(length_t ln = firstVisibleLine, subline = firstVisibleLine; ; ) {
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
			for(length_t ln = firstVisibleLine, subline = firstVisibleSubline; ; --subline) {
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

/// @see detail#IteratorAdapter#next
void TextViewer::BaselineIterator::next() {
	return advance(+1);
}

/// @see detail#IteratorAdapter#previous
void TextViewer::BaselineIterator::previous() {
	return advance(-1);
}


// TextViewer.CursorVanisher //////////////////////////////////////////////////////////////////////

TextViewer::CursorVanisher::CursorVanisher() /*throw()*/ : viewer_(0) {
}

TextViewer::CursorVanisher::~CursorVanisher() /*throw()*/ {
	restore();
}

void TextViewer::CursorVanisher::install(TextViewer& viewer) {
	assert(viewer_ == 0);
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


// TextViewer.SpacePainter ////////////////////////////////////////////////////////////////////////

TextViewer::SpacePainter::SpacePainter() /*throw()*/ : viewerBounds_(
		geometry::make<NativeRectangle>(geometry::make<NativePoint>(0, 0), geometry::make<NativeSize>(0, 0))) {
	computedValues_.left = computedValues_.top = computedValues_.right = computedValues_.bottom = 0;
}

TextViewer::SpacePainter::update() {
}


// TextViewer.Renderer ////////////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param viewer The text viewer
 * @param writingMode The initial writing mode
 */
TextViewer::Renderer::Renderer(TextViewer& viewer, const WritingMode<false>& writingMode) :
		TextRenderer(viewer.presentation(), systemFonts(), true), viewer_(viewer), defaultWritingMode_(writingMode) {
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
const WritingMode<false>& TextViewer::Renderer::defaultUIWritingMode() const /*throw()*/ {
	return defaultWritingMode_;
}

/// Rewraps the visual lines at the window's edge.
void TextViewer::Renderer::rewrapAtWindowEdge() {
	class Local {
	public:
		explicit Local(Scalar newMeasure) : newMeasure_(newMeasure) {}
		bool operator()(const LineLayoutVector::LineLayout& layout) const {
			return layout.second->numberOfLines() != 1
				|| layout.second->style().justification == NO_JUSTIFICATION
				|| layout.second->maximumInlineProgressionDimension() > newMeasure_;
		}
	private:
		const Scalar newMeasure_;
	};

	if(viewer_.configuration().lineWrap.wrapsAtWindowEdge()) {
		const NativeRectangle clientBounds(viewer_.bounds(false));
		const PhysicalFourSides<Scalar>& spaces(viewer_.spaceWidths());
		if(WritingModeBase::isHorizontal(utils::writingMode(viewer_).blockFlowDirection))
			layouts().invalidateIf(Local(geometry::dx(clientBounds) - spaces.left - spaces.right));
		else
			layouts().invalidateIf(Local(geometry::dy(clientBounds) - spaces.top - spaces.bottom));
	}
}

/**
 * Sets the default UI writing mode.
 * @param writingMode The writing mode
 */
void TextViewer::Renderer::setDefaultWritingMode(const WritingMode<false>& writingMode) /*throw()*/ {
	if(writingMode != defaultWritingMode_) {
		defaultWritingMode_ = writingMode;
		layouts().invalidate();
	}
}

/// @see TextRenderer#width
Scalar TextViewer::Renderer::width() const /*throw()*/ {
	const LineWrapConfiguration& lwc = viewer_.configuration().lineWrap;
	if(!lwc.wraps())
		return (viewer_.horizontalScrollBar().range().end() + 1) * viewer_.textRenderer().defaultFont()->metrics().averageCharacterWidth();
	else if(lwc.wrapsAtWindowEdge()) {
		const NativeRectangle clientBounds(viewer_.bounds(false));
		const PhysicalFourSides<Scalar>& spaces(viewer_.spaceWidths());
		return WritingModeBase::isHorizontal(utils::writingMode(viewer_).blockFlowDirection) ?
			(geometry::dx(clientBounds) - spaces.left - spaces.right) : (geometry::dy(clientBounds) - spaces.top - spaces.bottom);
	} else
		return lwc.width;
}


// TextViewer.Configuration /////////////////////////////////////////////////

/// Default constructor.
TextViewer::Configuration::Configuration() /*throw()*/ :
		readingDirection(LEFT_TO_RIGHT), usesRichTextClipboardFormat(false) {
#if(_WIN32_WINNT >= 0x0501)
	BOOL b;
	if(::SystemParametersInfo(SPI_GETMOUSEVANISH, 0, &b, 0) != 0)
		vanishesCursor = win32::boole(b);
	else
		vanishesCursor = false;
#else
	vanishesCursor = false;
#endif // _WIN32_WINNT >= 0x0501
}


// TextViewer.ScrollInfo ////////////////////////////////////////////////////

void TextViewer::ScrollInfo::resetBars(const TextViewer& viewer, char bars, bool pageSizeChanged) /*throw()*/ {
	// about horizontal
	if(bars == 'h' || bars == 'b') {
		// テキストが左揃えでない場合は、スクロールボックスの位置を補正する必要がある
		// (ウィンドウが常に LTR である仕様のため)
	//	const TextAlignment alignment = resolveTextAlignment(viewer.configuration().alignment, viewer.configuration().readingDirection);
		const int dx = viewer.textRenderer().defaultFont()->metrics().averageCharacterWidth();
		assert(dx > 0);
		const unsigned long columns = (!viewer.configuration().lineWrap.wrapsAtWindowEdge()) ?
			viewer.textRenderer().layouts().maximumInlineProgressionDimension() / dx : 0;
//		horizontal.rate = columns / numeric_limits<int>::max() + 1;
//		assert(horizontal.rate != 0);
		const int oldMaximum = horizontal.maximum;
		horizontal.maximum = max(static_cast<int>(columns/* / horizontal.rate*/), static_cast<int>(viewer.numberOfVisibleColumns() - 1));
	//	if(alignment == ALIGN_RIGHT)
	//		horizontal.position += horizontal.maximum - oldMaximum;
	//	else if(alignment == ALIGN_CENTER)
//	//		horizontal.position += (horizontal.maximum - oldMaximum) / 2;
	//		horizontal.position += horizontal.maximum / 2 - oldMaximum / 2;
		horizontal.position = max(horizontal.position, 0);
		if(pageSizeChanged) {
			const UINT oldPageSize = horizontal.pageSize;
			horizontal.pageSize = static_cast<UINT>(viewer.numberOfVisibleColumns());
	//		if(alignment == ALIGN_RIGHT)
	//			horizontal.position -= horizontal.pageSize - oldPageSize;
	//		else if(alignment == ALIGN_CENTER)
//	//			horizontal.position -= (horizontal.pageSize - oldPageSize) / 2;
	//			horizontal.position -= horizontal.pageSize / 2 - oldPageSize / 2;
			horizontal.position = max(horizontal.position, 0);
		}
	}
	// about vertical
	if(bars == 'v' || bars == 'b') {
		const length_t lines = viewer.textRenderer().layouts().numberOfVisualLines();
		assert(lines > 0);
//		vertical.rate = static_cast<ulong>(lines) / numeric_limits<int>::max() + 1;
//		assert(vertical.rate != 0);
		vertical.maximum = max(static_cast<int>((lines - 1)/* / vertical.rate*/), 0/*static_cast<int>(viewer.numberOfVisibleLines() - 1)*/);
		if(pageSizeChanged)
			vertical.pageSize = static_cast<UINT>(viewer.numberOfVisibleLines());
	}
}

void TextViewer::ScrollInfo::updateVertical(const TextViewer& viewer) /*throw()*/ {
	const LineLayoutVector& layouts = viewer.textRenderer().layouts();
	vertical.maximum = static_cast<int>(layouts.numberOfVisualLines());
	firstVisibleLine = min(firstVisibleLine, viewer.document().numberOfLines() - 1);
	firstVisibleSubline = min(layouts.numberOfSublinesOfLine(firstVisibleLine) - 1, firstVisibleSubline);
	vertical.position = static_cast<int>(layouts.mapLogicalLineToVisualLine(firstVisibleLine) + firstVisibleSubline);
}


// VirtualBox ///////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param viewer The viewer
 * @param region The region consists the rectangle
 */
VirtualBox::VirtualBox(const TextViewer& viewer, const k::Region& region) /*throw()*/ : viewer_(viewer) {
	update(region);
}

/**
 * Returns if the specified point is on the virtual box.
 * @param p The point in local coordinates
 * @return @c true If the point is on the virtual box
 */
bool VirtualBox::isPointOver(const graphics::NativePoint& p) const /*throw()*/ {
//	assert(viewer_.isWindow());
	if(viewer_.hitTest(p) != TextViewer::CONTENT_AREA)	// ignore if not in content area
		return false;
	const Scalar leftMargin = viewer_.textAreaMargins().left;
	if(geometry::x(p) < startEdge() + leftMargin || geometry::x(p) >= endEdge() + leftMargin)	// about x-coordinate
		return false;

	// about y-coordinate
	const Point& top = beginning();
	const Point& bottom = end();
	length_t line, subline;
	viewer_.mapClientYToLine(geometry::y(p), &line, &subline);	// $friendly-access
	if(line < top.line || (line == top.line && subline < top.subline))
		return false;
	else if(line > bottom.line || (line == bottom.line && subline > bottom.subline))
		return false;
	else
		return true;
}

/**
 * Returns the range which the box overlaps with in specified visual line.
 * @param line the logical line
 * @param subline the visual subline
 * @param[out] range the range
 * @return @c true if the box and the visual line overlap
 */
bool VirtualBox::overlappedSubline(length_t line, length_t subline, Range<length_t>& range) const /*throw()*/ {
	assert(viewer_.isWindow());
	const Point& top = beginning();
	const Point& bottom = end();
	if(line < top.line || (line == top.line && subline < top.subline)	// out of the region
			|| line > bottom.line || (line == bottom.line && subline > bottom.subline))
		return false;
	else {
		const TextRenderer& renderer = viewer_.textRenderer();
		const TextLayout& layout = renderer.layouts().at(line);
		range = Range<length_t>(
			layout.offset(geometry::make<NativePoint>(points_[0].ipd - renderer.lineIndent(line, 0),
				static_cast<Scalar>(renderer.defaultFont()->metrics().linePitch() * subline))).first,
			layout.offset(geometry::make<NativePoint>(points_[1].ipd - renderer.lineIndent(line, 0),
				static_cast<Scalar>(renderer.defaultFont()->metrics().linePitch() * subline))).first);
		return !range.isEmpty();
	}
}

/**
 * Updates the rectangle of the virtual box.
 * @param region the region consists the rectangle
 */
void VirtualBox::update(const k::Region& region) /*throw()*/ {
	const TextRenderer& r = viewer_.textRenderer();
	const TextLayout* layout = &r.layouts().at(points_[0].line = region.first.line);
	graphics::NativePoint location(layout->location(region.first.column));
	points_[0].ipd = geometry::x(location) + r.lineIndent(points_[0].line, 0);
	points_[0].subline = geometry::y(location) / r.defaultFont()->metrics().linePitch();
	layout = &r.layouts().at(points_[1].line = region.second.line);
	location = layout->location(region.second.column);
	points_[1].ipd = geometry::x(location) + r.lineIndent(points_[1].line, 0);
	points_[1].subline = geometry::y(location) / r.defaultFont()->metrics().linePitch();
}


// CaretShapeUpdater ////////////////////////////////////////////////////////

/**
 * Private constructor.
 * @param viewer the text viewer
 */
CaretShapeUpdater::CaretShapeUpdater(TextViewer& viewer) /*throw()*/ : viewer_(viewer) {
}

/// Notifies the text viewer to update the shape of the caret.
void CaretShapeUpdater::update() /*throw()*/ {
	viewer_.recreateCaret();	// $friendly-access
}

/// Returns the text viewer.
TextViewer& CaretShapeUpdater::textViewer() /*throw()*/ {
	return viewer_;
}


// DefaultCaretShaper ///////////////////////////////////////////////////////

/// Constructor.
DefaultCaretShaper::DefaultCaretShaper() /*throw()*/ : viewer_(0) {
}

/// @see CaretShaper#install
void DefaultCaretShaper::install(CaretShapeUpdater& updater) /*throw()*/ {
	viewer_ = &updater.textViewer();
}

/// @see CaretShaper#shape
void DefaultCaretShaper::shape(win32::Handle<HBITMAP>&, NativeSize& solidSize, ReadingDirection& readingDirection) /*throw()*/ {
	DWORD width;
	if(::SystemParametersInfo(SPI_GETCARETWIDTH, 0, &width, 0) == 0)
		width = 1;	// NT4 does not support SPI_GETCARETWIDTH
	solidSize = geometry::make<NativeSize>(width, viewer_->textRenderer().defaultFont()->metrics().cellHeight());
	readingDirection = LEFT_TO_RIGHT;	// no matter
}

/// @see CaretShaper#uninstall
void DefaultCaretShaper::uninstall() /*throw()*/ {
	viewer_ = 0;
}


// LocaleSensitiveCaretShaper ///////////////////////////////////////////////

namespace {
	/// Returns @c true if the specified language is RTL.
	inline bool isRTLLanguage(LANGID id) /*throw()*/ {
		return id == LANG_ARABIC || id == LANG_FARSI || id == LANG_HEBREW || id == LANG_SYRIAC || id == LANG_URDU;
	}
	/// Returns @c true if the specified language is Thai or Lao.
	inline bool isTISLanguage(LANGID id) /*throw()*/ {
#ifndef LANG_LAO
		const LANGID LANG_LAO = 0x54;
#endif // !LANG_LAO
		return id == LANG_THAI || id == LANG_LAO;
	}
	/**
	 * Returns the bitmap has specified size.
	 * @param dc The device context
	 * @param width The width of the bitmap
	 * @param height The height of the bitmap
	 * @return The bitmap. This value is allocated via the global @c operator @c new
	 */
	inline BITMAPINFO* prepareCaretBitmap(const win32::Handle<HDC>& dc, uint16_t width, uint16_t height) {
		BITMAPINFO* const info =
			static_cast<BITMAPINFO*>(::operator new(sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * width * height));
		BITMAPINFOHEADER& header = info->bmiHeader;
		memset(&header, 0, sizeof(BITMAPINFOHEADER));
		header.biSize = sizeof(BITMAPINFOHEADER);
		header.biWidth = width;
		header.biHeight = -height;
		header.biBitCount = sizeof(RGBQUAD) * 8;//::GetDeviceCaps(dc.get(), BITSPIXEL);
		header.biPlanes = static_cast<WORD>(::GetDeviceCaps(dc.get(), PLANES));
		return info;
	}
	/**
	 * Creates the bitmap for solid caret.
	 * @param width The width of the rectangle in pixels
	 * @param height The height of the rectangle in pixels
	 * @param color The color
	 * @return The bitmap
	 */
	inline win32::Handle<HBITMAP> createSolidCaretBitmap(uint16_t width, uint16_t height, const RGBQUAD& color) {
		win32::Handle<HDC> dc(detail::screenDC());
		BITMAPINFO* info = prepareCaretBitmap(dc, width, height);
		uninitialized_fill(info->bmiColors, info->bmiColors + width * height, color);
		win32::Handle<HBITMAP> result(::CreateDIBitmap(
			dc.get(), &info->bmiHeader, CBM_INIT, info->bmiColors, info, DIB_RGB_COLORS), &::DeleteObject);
		::operator delete(info);
		return result;
	}
	/**
	 * Creates the bitmap for RTL caret.
	 * @param height The height of the image in pixels
	 * @param bold Set @c true to create a bold shape
	 * @param color The color
	 * @return The bitmap
	 */
	inline win32::Handle<HBITMAP> createRTLCaretBitmap(uint16_t height, bool bold, const RGBQUAD& color) {
		win32::Handle<HDC> dc(detail::screenDC());
		const RGBQUAD white = {0x00, 0x00, 0x00, 0x00};
		BITMAPINFO* info = prepareCaretBitmap(dc, 5, height);
		assert(height > 3);
		uninitialized_fill(info->bmiColors, info->bmiColors + 5 * height, white);
		info->bmiColors[0] = info->bmiColors[1] = info->bmiColors[2]
			= info->bmiColors[6] = info->bmiColors[7] = info->bmiColors[12] = color;
		for(uint16_t i = 0; i < height; ++i) {
			info->bmiColors[i * 5 + 3] = color;
			if(bold)
				info->bmiColors[i * 5 + 4] = color;
		}
		win32::Handle<HBITMAP> result(::CreateDIBitmap(
			dc.get(), &info->bmiHeader, CBM_INIT, info->bmiColors, info, DIB_RGB_COLORS), &::DeleteObject);
		::operator delete(info);
		return result;
	}
	/**
	 * Creates the bitmap for Thai or Lao caret.
	 * @param height the height of the image in pixels
	 * @param bold set @c true to create a bold shape
	 * @param color the color
	 * @return The bitmap
	 */
	inline win32::Handle<HBITMAP> createTISCaretBitmap(uint16_t height, bool bold, const RGBQUAD& color) {
		win32::Handle<HDC> dc(detail::screenDC());
		const RGBQUAD white = {0x00, 0x00, 0x00, 0x00};
		const uint16_t width = max<uint16_t>(height / 8, 3);
		BITMAPINFO* info = prepareCaretBitmap(dc, width, height);
		assert(height > 3);
		uninitialized_fill(info->bmiColors, info->bmiColors + width * height, white);
		for(uint16_t y = 0; y < height - 1; ++y) {
			info->bmiColors[y * width] = color;
			if(bold) info->bmiColors[y * width + 1] = color;
		}
		if(bold)
			for(uint16_t x = 2; x < width; ++x)
				info->bmiColors[width * (height - 2) + x] = color;
		for(uint16_t x = 0; x < width; ++x)
			info->bmiColors[width * (height - 1) + x] = color;
		win32::Handle<HBITMAP> result(::CreateDIBitmap(
			dc.get(), &info->bmiHeader, CBM_INIT, info->bmiColors, info, DIB_RGB_COLORS), &::DeleteObject);
		::operator delete(info);
		return result;
	}
} // namespace @0

/// Constructor.
LocaleSensitiveCaretShaper::LocaleSensitiveCaretShaper(bool bold /* = false */) /*throw()*/ : updater_(0), bold_(bold) {
}

/// @see CaretListener#caretMoved
void LocaleSensitiveCaretShaper::caretMoved(const Caret& self, const k::Region&) {
	if(self.isOvertypeMode())
		updater_->update();
}

/// @see CaretShaper#install
void LocaleSensitiveCaretShaper::install(CaretShapeUpdater& updater) {
	updater_ = &updater;
}

/// @see CaretStateListener#matchBracketsChanged
void LocaleSensitiveCaretShaper::matchBracketsChanged(const Caret&, const std::pair<k::Position, k::Position>&, bool) {
}

/// @see CaretStateListener#overtypeModeChanged
void LocaleSensitiveCaretShaper::overtypeModeChanged(const Caret&) {
	updater_->update();
}

/// @see CaretShapeListener#selectionShapeChanged
void LocaleSensitiveCaretShaper::selectionShapeChanged(const Caret&) {
}

/// @see CaretShaper#shape
void LocaleSensitiveCaretShaper::shape(
		win32::Handle<HBITMAP>& bitmap, NativeSize& solidSize, ReadingDirection& readingDirection) /*throw()*/ {
	const Caret& caret = updater_->textViewer().caret();
	const bool overtype = caret.isOvertypeMode() && isSelectionEmpty(caret);

	if(!overtype) {
		geometry::dx(solidSize) = bold_ ? 2 : 1;	// this ignores the system setting...
		geometry::dy(solidSize) = updater_->textViewer().textRenderer().defaultFont()->metrics().cellHeight();
	} else	// use the width of the glyph when overtype mode
		solidSize = getCurrentCharacterSize(updater_->textViewer());
	readingDirection = LEFT_TO_RIGHT;

	HIMC imc = ::ImmGetContext(updater_->textViewer().identifier().get());
	const bool imeOpened = win32::boole(::ImmGetOpenStatus(imc));
	::ImmReleaseContext(updater_->textViewer().identifier().get(), imc);
	if(imeOpened) {	// CJK and IME is open
		static const RGBQUAD red = {0xff, 0xff, 0x80, 0x00};
		bitmap = createSolidCaretBitmap(
			static_cast<uint16_t>(geometry::dx(solidSize)), static_cast<uint16_t>(geometry::dy(solidSize)), red);
	} else if(!overtype && geometry::dy(solidSize) > 3) {
		static const RGBQUAD black = {0xff, 0xff, 0xff, 0x00};
		const WORD langID = PRIMARYLANGID(LOWORD(::GetKeyboardLayout(::GetCurrentThreadId())));
		if(isRTLLanguage(langID)) {	// RTL
			bitmap = createRTLCaretBitmap(static_cast<uint16_t>(geometry::dy(solidSize)), bold_, black);
			readingDirection = RIGHT_TO_LEFT;
		} else if(isTISLanguage(langID)) {	// Thai relations
			bitmap = createTISCaretBitmap(static_cast<uint16_t>(geometry::dy(solidSize)), bold_, black);
		}
	}
}

/// @see TextViewerInputStatusListener#textViewerIMEOpenStatusChanged
void LocaleSensitiveCaretShaper::textViewerIMEOpenStatusChanged() /*throw()*/ {
	updater_->update();
}

/// @see TextViewerInputStatusListener#textViewerInputLanguageChanged
void LocaleSensitiveCaretShaper::textViewerInputLanguageChanged() /*throw()*/ {
	updater_->update();
}

/// @see CaretShapeProvider#uninstall
void LocaleSensitiveCaretShaper::uninstall() {
	updater_ = 0;
}


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
 * @param type the type of the cursor to obtain
 * @return the cursor. do not destroy the returned value
 * @throw UnknownValueException @a type is unknown
 */
const base::Cursor& AutoScrollOriginMark::cursorForScrolling(CursorType type) {
	static Cursor instances[3];
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
		instances[type].reset(::CreateCursor(::GetModuleHandleW(0), 16, 16, 32, 32, andBits, xorBits), &::DestroyCursor);
#else
#endif
	}
	return instances[type];
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
	if(makeRange(geometry::top(clientBounds), geometry::top(clientBounds) + spaces.top).includes(geometry::y(p)))
		dy = -1;
	else if(geometry::y(p) >= geometry::bottom(clientBounds) - spaces.bottom && geometry::y(p) < geometry::bottom(clientBounds))
		dy = +1;
	if(makeRange(geometry::left(clientBounds), geometry::left(clientBounds) + spaces.left).includes(geometry::x(p)))
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
		const NativeRectangle clientBounds(viewer_->bounds(false));
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
				max<Scalar>(geometry::x(p), geometry::left(clientBounds) + spaces.left),
				geometry::right(clientBounds) - spaces.right),
			min<Scalar>(
				max<Scalar>(geometry::y(p), geometry::top(clientBounds) + spaces.top),
				geometry::bottom(clientBounds) - spaces.bottom));
		destination = viewer_->characterForClientXY(p, TextLayout::TRAILING);
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
	endIncrementalSearch(*viewer_);

	// select line(s)
	if(htr == TextViewer::INDICATOR_MARGIN || htr == TextViewer::LINE_NUMBERS) {
		const k::Position to(viewer_->characterForClientXY(position, TextLayout::LEADING));
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
				const k::Position p(viewer_->characterForClientXY(position, TextLayout::TRAILING, true));
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
			const k::Position to(viewer_->characterForClientXY(position, TextLayout::TRAILING));
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
		viewer_->caret().moveTo(viewer_->characterForClientXY(position, TextLayout::TRAILING));
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
			abortIncrementalSearch(*viewer_);
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
		const k::Position p(viewer_->characterForClientXY(position, TextLayout::TRAILING, true, k::locations::UTF16_CODE_UNIT));
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
	tr1::shared_ptr<TextLineColorDirector> temp(this);
	caret.textViewer().presentation().addTextLineColorDirector(temp);
	caret.addListener(*this);
	caret.addStateListener(*this);
	caret.addLifeCycleListener(*this);
}

/// Destructor.
CurrentLineHighlighter::~CurrentLineHighlighter() /*throw()*/ {
	if(caret_ != 0) {
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
		if(!isSelectionEmpty(*caret_) || caret_->line() != oldRegion.first.line)
			caret_->textViewer().redrawLine(oldRegion.first.line, false);
	}
	if(isSelectionEmpty(*caret_)) {
		if(!oldRegion.isEmpty() || caret_->line() != oldRegion.first.line)
			caret_->textViewer().redrawLine(caret_->line(), false);
	}
}

/// Returns the foreground color.
const Color& CurrentLineHighlighter::foreground() const /*throw()*/ {
	return foreground_;
}

/// @see CaretStateListener#matchBracketsChanged
void CurrentLineHighlighter::matchBracketsChanged(const Caret&, const pair<k::Position, k::Position>&, bool) {
}

/// @see CaretStateListener#overtypeModeChanged
void CurrentLineHighlighter::overtypeModeChanged(const Caret&) {
}

/// @see PointLifeCycleListener#pointDestroyed
void CurrentLineHighlighter::pointDestroyed() {
//	caret_->removeListener(*this);
//	caret_->removeStateListener(*this);
	caret_ = 0;
}

/// @see ILineColorDirector#queryLineColors
TextLineColorDirector::Priority CurrentLineHighlighter::queryLineColors(length_t line, Color& foreground, Color& background) const {
	if(caret_ != 0 && isSelectionEmpty(*caret_) && caret_->line() == line && caret_->textViewer().hasFocus()) {
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


// ascension.viewers.utils ////////////////////////////////////////////////////////////////////////

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
			if(at.column >= hyperlinks[i]->region().beginning() && at.column <= hyperlinks[i]->region().end())
				return hyperlinks[i];
		}
	}
	return 0;
}

/**
 * Toggles the inline flow direction of the text viewer.
 * @param viewer The text viewer
 */
void utils::toggleOrientation(TextViewer& viewer) /*throw()*/ {
	WritingMode<false> wm(viewer.textRenderer().defaultUIWritingMode());
	wm.inlineFlowDirection = (wm.inlineFlowDirection == LEFT_TO_RIGHT) ? RIGHT_TO_LEFT : LEFT_TO_RIGHT;
	viewer.textRenderer().setDefaultWritingMode(wm);
	viewer.synchronizeWritingModeUI();
//	if(config.lineWrap.wrapsAtWindowEdge()) {
//		win32::AutoZeroSize<SCROLLINFO> scroll;
//		viewer.getScrollInformation(SB_HORZ, scroll);
//		viewer.setScrollInformation(SB_HORZ, scroll);
//	}
}


// ascension.source free functions //////////////////////////////////////////

/**
 * Returns the identifier near the specified position in the document.
 * @param document The document
 * @param position The position
 * @param[out] startColumn The start of the identifier. can be @c null if not needed
 * @param[out] endColumn The end of the identifier. can be @c null if not needed
 * @return @c false If the identifier is not found (in this case, the values of the output
 *         parameters are undefined)
 * @see #getPointedIdentifier
 */
bool source::getNearestIdentifier(const k::Document& document, const k::Position& position, length_t* startColumn, length_t* endColumn) {
	using namespace text;
	static const length_t MAXIMUM_IDENTIFIER_HALF_LENGTH = 100;

	k::DocumentPartition partition;
	document.partitioner().partition(position, partition);
	const IdentifierSyntax& syntax = document.contentTypeInformation().getIdentifierSyntax(partition.contentType);
	length_t start = position.column, end = position.column;

	// find the start of the identifier
	if(startColumn != 0) {
		k::DocumentCharacterIterator i(document,
			k::Region(max(partition.region.beginning(), k::Position(position.line, 0)), position), position);
		do {
			i.previous();
			if(!syntax.isIdentifierContinueCharacter(i.current())) {
				i.next();
				start = i.tell().column;
				break;
			} else if(position.column - i.tell().column > MAXIMUM_IDENTIFIER_HALF_LENGTH)	// too long identifier
				return false;
		} while(i.hasPrevious());
		if(!i.hasPrevious())
			start = i.tell().column;
		if(startColumn!= 0)
			*startColumn = start;
	}

	// find the end of the identifier
	if(endColumn != 0) {
		k::DocumentCharacterIterator i(document, k::Region(position,
			min(partition.region.end(), k::Position(position.line, document.lineLength(position.line)))), position);
		while(i.hasNext()) {
			if(!syntax.isIdentifierContinueCharacter(i.current())) {
				end = i.tell().column;
				break;
			}
			i.next();
			if(i.tell().column - position.column > MAXIMUM_IDENTIFIER_HALF_LENGTH)	// too long identifier
				return false;
		}
		if(!i.hasNext())
			end = i.tell().column;
		if(endColumn != 0)
			*endColumn = end;
	}

	return true;
}

/**
 * Returns the identifier near the cursor.
 * @param viewer The text viewer
 * @param[out] startPosition The start of the identifier. can be @c null if not needed
 * @param[out] endPosition The end of the identifier. can be @c null if not needed
 * @return @c false if the identifier is not found (in this case, the values of the output
 *         parameters are undefined)
 * @see #getNearestIdentifier
 */
bool source::getPointedIdentifier(const TextViewer& viewer, k::Position* startPosition, k::Position* endPosition) {
//	if(viewer.isWindow()) {
		NativePoint cursorPoint;
		::GetCursorPos(&cursorPoint);
		viewer.mapFromGlobal(cursorPoint);
		const k::Position cursor = viewer.characterForClientXY(cursorPoint, TextLayout::LEADING);
		if(source::getNearestIdentifier(viewer.document(), cursor,
				(startPosition != 0) ? &startPosition->column : 0, (endPosition != 0) ? &endPosition->column : 0)) {
			if(startPosition != 0)
				startPosition->line = cursor.line;
			if(endPosition != 0)
				endPosition->line = cursor.line;
			return true;
		}
//	}
	return false;
}
