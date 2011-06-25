/**
 * @file viewer.cpp
 * @author exeal
 * @date 2003-2006 (was EditView.cpp and EditViewWindowMessages.cpp)
 * @date 2006-2010
 * @see user-input.cpp
 */

#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/viewer.hpp>
#include <ascension/rules.hpp>
#include <ascension/corelib/text/break-iterator.hpp>
#include <ascension/text-editor/command.hpp>
#include <ascension/text-editor/session.hpp>
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
 * @c TextViewer supports OLE drag-and-drop. If you want to enable this feature, call Win32
 * @c OleInitialize in your thread.
 *
 * If you want to enable tooltips, call Win32 @c InitCommonControlsEx.
 *
 * @see presentation#Presentation, Caret
 */

#define ASCENSION_RESTORE_VANISHED_CURSOR()	\
	if(modeState_.cursorVanished) {			\
		modeState_.cursorVanished = false;	\
		Cursor::show(true);					\
		releaseInput();						\
	}

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
	inline const hyperlink::Hyperlink* getPointedHyperlink(const TextViewer& viewer, const k::Position& at) {
		size_t numberOfHyperlinks;
		if(const hyperlink::Hyperlink* const* hyperlinks = viewer.presentation().getHyperlinks(at.line, numberOfHyperlinks)) {
			for(size_t i = 0; i < numberOfHyperlinks; ++i) {
				if(at.column >= hyperlinks[i]->region().beginning() && at.column <= hyperlinks[i]->region().end())
					return hyperlinks[i];
			}
		}
		return 0;
	}
	inline void toggleOrientation(TextViewer& viewer) /*throw()*/ {
		WritingMode<false> wm(viewer.textRenderer().defaultUIWritingMode());
		wm.inlineFlowDirection = (wm.inlineFlowDirection == LEFT_TO_RIGHT) ? RIGHT_TO_LEFT : LEFT_TO_RIGHT;
		viewer.textRenderer().setDefaultUIWritingMode(wm);
		viewer.synchronizeWritingModeUI();
//		if(config.lineWrap.wrapsAtWindowEdge()) {
//			win32::AutoZeroSize<SCROLLINFO> scroll;
//			viewer.getScrollInformation(SB_HORZ, scroll);
//			viewer.setScrollInformation(SB_HORZ, scroll);
//		}
	}
} // namespace @0

/**
 * Constructor.
 * @param presentation the presentation
 */
TextViewer::TextViewer(Presentation& presentation) : presentation_(presentation), tipText_(0),
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
	rulerPainter_.reset(new detail::RulerPainter(*this, true));

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
	rulerPainter_.reset(new detail::RulerPainter(*this, true));

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
#ifndef ASCENSION_NO_ACTIVE_ACCESSIBILITY
	if(accessibleProxy_ != 0)
		accessibleProxy_->Release();
#endif // !ASCENSION_NO_ACTIVE_ACCESSIBILITY
}

/// @see Widget#aboutToLoseFocus
void TextViewer::aboutToLoseFocus() {
	ASCENSION_RESTORE_VANISHED_CURSOR();
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
	redrawLines(caret().beginning().line(), caret().end().line());
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
			redrawLines(oldRegion.beginning().line, oldRegion.end().line);
		if(!newRegion.isEmpty())
			redrawLines(newRegion.beginning().line, newRegion.end().line);
	} else if(newRegion != oldRegion) {	// the selection actually changed
		if(oldRegion.isEmpty()) {	// the selection was empty...
			if(!newRegion.isEmpty())	// the selection is not empty now
				redrawLines(newRegion.beginning().line, newRegion.end().line);
		} else {	// ...if the there is selection
			if(newRegion.isEmpty()) {	// the selection became empty
				redrawLines(oldRegion.beginning().line, oldRegion.end().line);
				if(!isFrozen())
					redrawScheduledRegion();
			} else if(oldRegion.beginning() == newRegion.beginning()) {	// the beginning point didn't change
				const length_t i[2] = {oldRegion.end().line, newRegion.end().line};
				redrawLines(min(i[0], i[1]), max(i[0], i[1]));
			} else if(oldRegion.end() == newRegion.end()) {	// the end point didn't change
				const length_t i[2] = {oldRegion.beginning().line, newRegion.beginning().line};
				redrawLines(min(i[0], i[1]), max(i[0], i[1]));
			} else {	// the both points changed
				if((oldRegion.beginning().line >= newRegion.beginning().line && oldRegion.beginning().line <= newRegion.end().line)
						|| (oldRegion.end().line >= newRegion.beginning().line && oldRegion.end().line <= newRegion.end().line)) {
					const length_t i[2] = {
						min(oldRegion.beginning().line, newRegion.beginning().line), max(oldRegion.end().line, newRegion.end().line)
					};
					redrawLines(min(i[0], i[1]), max(i[0], i[1]));
				} else {
					redrawLines(oldRegion.beginning().line, oldRegion.end().line);
					if(!isFrozen())
						redrawScheduledRegion();
					redrawLines(newRegion.beginning().line, newRegion.end().line);
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
 * @see #clientXYForCharacter, #hitTest, layout#LineLayout#offset
 */
k::Position TextViewer::characterForClientXY(const NativePoint& p, TextLayout::Edge edge,
		bool abortNoCharacter /* = false */, k::locations::CharacterUnit snapPolicy /* = k::locations::GRAPHEME_CLUSTER */) const {
	k::Position result;

	// determine the logical line
	length_t subline;
	bool outside;
	mapClientYToLine(geometry::y(p), &result.line, &subline, &outside);
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

void TextViewer::checkInitialization() const {
	if(!isWindow())
		throw WindowNotInitializedException();
}

/**
 * Returns the point nearest from the specified document position.
 * @param position The document position. can be outside of the window
 * @param fullSearchY If this is @c false, this method stops at top or bottom of the client area.
 *                    Otherwise, the calculation of y-coordinate is performed completely. But in
 *                    this case, may be very slow. see the description of return value
 * @param edge The edge of the character
 * @return The client coordinates of the point. About the y-coordinate of the point, if
 *         @a fullSearchY is @c false and @a position.line is outside of the client area, the
 *         result is @c std#numeric_limits&lt;Scalar&gt;::max() (for upward) or
 *         @c std#numeric_limits&lt;Scalar&gt;::min() (for downward)
 * @throw BadPositionException @a position is outside of the document
 * @throw WindowNotInitialized The window is not initialized
 * @see #characterForClientXY, #hitTest, layout#LineLayout#location
 */
NativePoint TextViewer::clientXYForCharacter(const k::Position& position, bool fullSearchY, TextLayout::Edge edge) const {
	checkInitialization();
	const TextLayout& layout = renderer_->layouts().at(position.line);
	NativePoint p(layout.location(position.column, edge));
	geometry::x(p) += getDisplayXOffset(position.line);
	const Scalar y = mapLineToClientY(position.line, fullSearchY);
	if(y == numeric_limits<Scalar>::max() || y == numeric_limits<Scalar>::min())
		geometry::y(p) = y;
	else
		geometry::y(p) += y;
	return p;
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
	if(isFrozen() && freezeInfo_.invalidLines.first != INVALID_INDEX) {
		if(change.erasedRegion().first.line != change.erasedRegion().second.line) {
			const length_t first = change.erasedRegion().first.line + 1, last = change.erasedRegion().second.line;
			if(freezeInfo_.invalidLines.first > last)
				freezeInfo_.invalidLines.first -= last - first + 1;
			else if(freezeInfo_.invalidLines.first > first)
				freezeInfo_.invalidLines.first = first;
			if(freezeInfo_.invalidLines.second != numeric_limits<length_t>::max()) {
				if(freezeInfo_.invalidLines.second > last)
					freezeInfo_.invalidLines.second -= last - first + 1;
				else if(freezeInfo_.invalidLines.second > first)
					freezeInfo_.invalidLines.second = first;
			}
		}
		if(change.insertedRegion().first.line != change.insertedRegion().second.line) {
			const length_t first = change.insertedRegion().first.line + 1, last = change.insertedRegion().second.line;
			if(freezeInfo_.invalidLines.first >= first)
				freezeInfo_.invalidLines.first += last - first + 1;
			if(freezeInfo_.invalidLines.second >= first && freezeInfo_.invalidLines.second != numeric_limits<length_t>::max())
				freezeInfo_.invalidLines.second += last - first + 1;
		}
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
	checkInitialization();
	++freezeInfo_.count;
}

/**
 * Returns the horizontal display offset from @c LineLayout coordinates to client coordinates.
 * @param line The line number
 * @return The offset
 */
Scalar TextViewer::getDisplayXOffset(length_t line) const {
	const Margins margins(textAreaMargins());
	const TextLayout& layout = renderer_->layouts().at(line);
	const detail::PhysicalTextAnchor alignment(
		detail::computePhysicalTextAnchor(layout.anchor(), layout.writingMode().inlineFlowDirection));
	if(alignment == detail::LEFT /*|| ... != NO_JUSTIFICATION*/)	// TODO: this code ignores last visual line with justification.
		return margins.left - scrollInfo_.x() * renderer_->defaultFont()->metrics().averageCharacterWidth();

	Scalar indent;
	const NativeRectangle clientBounds(bounds(false));
	if(renderer_->layouts().maximumInlineProgressionDimension() + margins.left + margins.right > geometry::dx(clientBounds)) {
		indent = renderer_->layouts().maximumInlineProgressionDimension() - layout.lineInlineProgressionDimension(0) + margins.left;
		indent += (geometry::dx(clientBounds) - margins.left - margins.right) % renderer_->defaultFont()->metrics().averageCharacterWidth();
	} else
		indent = geometry::dx(clientBounds) - layout.lineInlineProgressionDimension(0) - margins.right;
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
	// restore the scroll positions
	setScrollPosition(SB_HORZ, scrollInfo_.horizontal.position, false);
	setScrollPosition(SB_VERT, scrollInfo_.vertical.position, true);

	// hmm...
//	if(/*sharedData_->options.appearance[SHOW_CURRENT_UNDERLINE] ||*/ !getCaret().isSelectionEmpty()) {
		redrawLines(caret().beginning().line(), caret().end().line());
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
 * @param p The position to hit test, in client coordinates
 * @return The result
 * @see TextViewer#HitTestResult
 */
TextViewer::HitTestResult TextViewer::hitTest(const NativePoint& p) const {
	checkInitialization();
	const NativeRectangle clientBounds(bounds(false));
	if(!geometry::includes(clientBounds, p))
		return OUT_OF_VIEW;

	const RulerConfiguration& vrc = rulerConfiguration();
	const bool rulerIsLeft = utils::isRulerLeftAligned(*this);

	if(vrc.indicatorMargin.visible
			&& ((rulerIsLeft && geometry::x(p) < vrc.indicatorMargin.width)
			|| (!rulerIsLeft && geometry::x(p) >= geometry::right(clientBounds) - vrc.indicatorMargin.width)))
		return INDICATOR_MARGIN;
	else if(vrc.lineNumbers.visible
			&& ((rulerIsLeft && geometry::x(p) < rulerPainter_->width())
			|| (!rulerIsLeft && geometry::x(p) >= geometry::right(clientBounds) - rulerPainter_->width())))
		return LINE_NUMBERS;
	else if((vrc.alignment == ALIGN_LEFT && geometry::x(p) < rulerPainter_->width() + configuration_.leadingMargin)
			|| (!rulerIsLeft && geometry::x(p) >= geometry::right(clientBounds) - rulerPainter_->width() - configuration_.leadingMargin))
		return LEADING_MARGIN;
	else if(geometry::y(p) < textAreaMargins().top)
		return TOP_MARGIN;
	else
		return TEXT_AREA;
}

/**
 * Creates the window of the viewer.
 * @param parent A handle to the parent or owner window of the window
 * @param position The position of the window
 * @param size The size of the window
 * @param style The style of the window
 * @param extendedStyle The extended style of the window
 * @see manah#windows#controls#Window#create
 */
void TextViewer::initialize(const win32::Handle<HWND>& parent,
		const NativePoint& position /* = geometry::make<NativePoint>(CW_USEDEFAULT, CW_USEDEFAULT) */,
		const NativeSize& size /* = geometry::make<NativeSize>(CW_USEDEFAULT, CW_USEDEFAULT) */,
		DWORD style /* = 0 */, DWORD extendedStyle /* = 0 */) {
	const bool visible = win32::boole(style & WS_VISIBLE);
	style &= ~WS_VISIBLE;	// 後で足す
	win32::Window::initialize(parent, position, size, style, extendedStyle);

	scrollInfo_.updateVertical(*this);
	updateScrollBars();

	// create the tooltip belongs to the window
	toolTip_ = ::CreateWindowExW(
		WS_EX_TOOLWINDOW | WS_EX_TOPMOST, TOOLTIPS_CLASSW, 0, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, identifier().get(), 0,
		reinterpret_cast<HINSTANCE>(static_cast<HANDLE_PTR>(::GetWindowLongPtr(identifier().get(), GWLP_HINSTANCE))), 0);
	if(toolTip_ != 0) {
		win32::AutoZeroSize<TOOLINFOW> ti;
		RECT margins = {1, 1, 1, 1};
		ti.hwnd = identifier().get();
		ti.lpszText = LPSTR_TEXTCALLBACKW;
		ti.uFlags = TTF_SUBCLASS;
		ti.uId = 1;
		::SetRect(&ti.rect, 0, 0, 0, 0);
		::SendMessageW(toolTip_, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&ti));
		::SendMessageW(toolTip_, TTM_SETDELAYTIME, TTDT_AUTOPOP, 30000);	// 30 秒間 (根拠なし) 表示されるように
//		::SendMessageW(toolTip_, TTM_SETDELAYTIME, TTDT_INITIAL, 1500);
		::SendMessageW(toolTip_, TTM_SETMARGIN, 0, reinterpret_cast<LPARAM>(&margins));
		::SendMessageW(toolTip_, TTM_ACTIVATE, true, 0L);
	}

	setMouseInputStrategy(0, true);

	RulerConfiguration rc;
	rc.lineNumbers.visible = true;
	rc.indicatorMargin.visible = true;
	rc.lineNumbers.foreground = Color(0x00, 0x80, 0x80);
	rc.lineNumbers.background = Color(0xff, 0xff, 0xff);
	rc.lineNumbers.borderColor = Color(0x00, 0x80, 0x80);
	rc.lineNumbers.borderStyle = RulerConfiguration::LineNumbers::DOTTED;
	rc.lineNumbers.borderWidth = 1;
	setConfiguration(0, &rc, false);

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
	document().setPartitioner(auto_ptr<DocumentPartitioner>(p));

	PresentationReconstructor* pr = new PresentationReconstructor(presentation());

	// JSDoc syntax highlight test
	static const Char JSDOC_ATTRIBUTES[] = L"@addon @argument @author @base @class @constructor @deprecated @exception @exec @extends"
		L" @fileoverview @final @ignore @link @member @param @private @requires @return @returns @see @throws @type @version";
	{
		auto_ptr<const WordRule> jsdocAttributes(new WordRule(220, JSDOC_ATTRIBUTES, ASCENSION_ENDOF(JSDOC_ATTRIBUTES) - 1, L' ', true));
		auto_ptr<LexicalTokenScanner> scanner(new LexicalTokenScanner(JS_MULTILINE_DOC_COMMENT));
		scanner->addWordRule(jsdocAttributes);
		scanner->addRule(auto_ptr<Rule>(new URIRule(219, URIDetector::defaultIANAURIInstance(), false)));
		map<Token::ID, const TextStyle> jsdocStyles;
		jsdocStyles.insert(make_pair(Token::DEFAULT_TOKEN, TextStyle(Colors(Color(0x00, 0x80, 0x00)))));
		jsdocStyles.insert(make_pair(219, TextStyle(Colors(Color(0x00, 0x80, 0x00)), false, false, false, SOLID_UNDERLINE)));
		jsdocStyles.insert(make_pair(220, TextStyle(Colors(Color(0x00, 0x80, 0x00)), true)));
		auto_ptr<IPartitionPresentationReconstructor> ppr(
			new LexicalPartitionPresentationReconstructor(document(), auto_ptr<ITokenScanner>(scanner.release()), jsdocStyles));
		pr->setPartitionReconstructor(JS_MULTILINE_DOC_COMMENT, ppr);
	}

	// JavaScript syntax highlight test
	static const Char JS_KEYWORDS[] = L"Infinity break case catch continue default delete do else false finally for function"
		L" if in instanceof new null return switch this throw true try typeof undefined var void while with";
	static const Char JS_FUTURE_KEYWORDS[] = L"abstract boolean byte char class double enum extends final float goto"
		L" implements int interface long native package private protected public short static super synchronized throws transient volatile";
	{
		auto_ptr<const WordRule> jsKeywords(new WordRule(221, JS_KEYWORDS, ASCENSION_ENDOF(JS_KEYWORDS) - 1, L' ', true));
		auto_ptr<const WordRule> jsFutureKeywords(new WordRule(222, JS_FUTURE_KEYWORDS, ASCENSION_ENDOF(JS_FUTURE_KEYWORDS) - 1, L' ', true));
		auto_ptr<LexicalTokenScanner> scanner(new LexicalTokenScanner(DEFAULT_CONTENT_TYPE));
		scanner->addWordRule(jsKeywords);
		scanner->addWordRule(jsFutureKeywords);
		scanner->addRule(auto_ptr<const Rule>(new NumberRule(223)));
		map<Token::ID, const TextStyle> jsStyles;
		jsStyles.insert(make_pair(Token::DEFAULT_TOKEN, TextStyle()));
		jsStyles.insert(make_pair(221, TextStyle(Colors(Color(0x00, 0x00, 0xff)))));
		jsStyles.insert(make_pair(222, TextStyle(Colors(Color(0x00, 0x00, 0xff)), false, false, false, DASHED_UNDERLINE)));
		jsStyles.insert(make_pair(223, TextStyle(Colors(Color(0x80, 0x00, 0x00)))));
		pr->setPartitionReconstructor(DEFAULT_CONTENT_TYPE,
			auto_ptr<IPartitionPresentationReconstructor>(new LexicalPartitionPresentationReconstructor(document(),
				auto_ptr<ITokenScanner>(scanner.release()), jsStyles)));
	}

	// other contents
	pr->setPartitionReconstructor(JS_MULTILINE_COMMENT, auto_ptr<IPartitionPresentationReconstructor>(
		new SingleStyledPartitionPresentationReconstructor(TextStyle(Colors(Color(0x00, 0x80, 0x00))))));
	pr->setPartitionReconstructor(JS_SINGLELINE_COMMENT, auto_ptr<IPartitionPresentationReconstructor>(
		new SingleStyledPartitionPresentationReconstructor(TextStyle(Colors(Color(0x00, 0x80, 0x00))))));
	pr->setPartitionReconstructor(JS_DQ_STRING, auto_ptr<IPartitionPresentationReconstructor>(
		new SingleStyledPartitionPresentationReconstructor(TextStyle(Colors(Color(0x00, 0x00, 0x80))))));
	pr->setPartitionReconstructor(JS_SQ_STRING, auto_ptr<IPartitionPresentationReconstructor>(
		new SingleStyledPartitionPresentationReconstructor(TextStyle(Colors(Color(0x00, 0x00, 0x80))))));
	new CurrentLineHighlighter(*caret_, Colors(Color(), Color::fromCOLORREF(::GetSysColor(COLOR_INFOBK))));

	// URL hyperlinks test
	auto_ptr<hyperlink::CompositeHyperlinkDetector> hld(new hyperlink::CompositeHyperlinkDetector);
	hld->setDetector(JS_MULTILINE_DOC_COMMENT, auto_ptr<hyperlink::IHyperlinkDetector>(
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
	auto_ptr<contentassist::ContentAssistant> ca(new contentassist::ContentAssistant());
	ca->setContentAssistProcessor(JS_MULTILINE_DOC_COMMENT, auto_ptr<contentassist::IContentAssistProcessor>(new JSDocProposals(cti->getIdentifierSyntax(JS_MULTILINE_DOC_COMMENT))));
	ca->setContentAssistProcessor(DEFAULT_CONTENT_TYPE, auto_ptr<contentassist::IContentAssistProcessor>(new JSProposals(cti->getIdentifierSyntax(DEFAULT_CONTENT_TYPE))));
	setContentAssistant(auto_ptr<contentassist::IContentAssistant>(ca));
	document().setContentTypeInformation(auto_ptr<IContentTypeInformationProvider>(cti));
#endif // 1

	class ZebraTextRunStyleTest : public TextRunStyleDirector {
	private:
		class Iterator : public StyledTextRunIterator {
		public:
			Iterator(length_t lineLength, bool beginningIsBlackBack) : length_(lineLength), beginningIsBlackBack_(beginningIsBlackBack) {
				current_ = StyledTextRun(0, current_.style());
				update();
			}
			void current(StyledTextRun& sr) const {
				if(!hasNext())
					throw IllegalStateException("");
				sr = current_;
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
				auto_ptr<TextRunStyle> style(new TextRunStyle);
				style->foreground = Paint((temp % 2 == 0) ?
					Color(0xff, 0x00, 0x00) : Color::fromCOLORREF(::GetSysColor(COLOR_WINDOWTEXT)));
				style->background = Paint((temp % 2 == 0) ?
					Color(0xff, 0xcc, 0xcc) : Color::fromCOLORREF(::GetSysColor(COLOR_WINDOW)));
				current_ = StyledTextRun(current_.position(), style);
			}
		private:
			const length_t length_;
			const bool beginningIsBlackBack_;
			StyledTextRun current_;
		};
	public:
		ZebraTextRunStyleTest(const k::Document& document) : document_(document) {
		}
		auto_ptr<StyledTextRunIterator> queryTextRunStyle(length_t line) const {
			return auto_ptr<StyledTextRunIterator>(new Iterator(document_.lineLength(line), line % 2 == 0));
		}
	private:
		const k::Document& document_;
	};
	presentation().setTextRunStyleDirector(
		tr1::shared_ptr<TextRunStyleDirector>(new ZebraTextRunStyleTest(document())));
	
	renderer_->addDefaultFontListener(*this);
	renderer_->layouts().addVisualLinesListener(*this);

	// placement and display
//	setBounds(rect);
	if(visible)
		show();
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
			toggleOrientation(*this);
		break;
	case keyboardcodes::ESCAPE:	// [Esc]
		CancelCommand(*this)();
		break;
	case keyboardcodes::PAGE_UP_OR_PRIOR:	// [PageUp]
		if(hasModifier<UserInput::CONTROL_DOWN>(input))
			onVScroll(SB_PAGEUP, 0, win32::Handle<HWND>());
		else
			CaretMovementCommand(*this, &k::locations::backwardPage, hasModifier<UserInput::SHIFT_DOWN>(input))();
		break;
	case keyboardcodes::PAGE_DOWN_OR_NEXT:	// [PageDown]
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
	case keyboardcodes::DELETE:	// [Delete]
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
		ASCENSION_RESTORE_VANISHED_CURSOR();
		if(mouseInputStrategy_.get() != 0)
			mouseInputStrategy_->interruptMouseReaction(true);
	}
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

/**
 * Converts the distance from the window top to the logical line.
 * @param y The distance
 * @param[out] logicalLine The logical line index. can be @c null if not needed
 * @param[out] visualSublineOffset The offset from the first line in @a logicalLine. Can be @c null if not needed
 * @param[out] snapped @c true if there was not a line at @a y. optional
 * @see #mapLineToClientY, TextRenderer#offsetVisualLine
 */
void TextViewer::mapClientYToLine(Scalar y, length_t* logicalLine, length_t* visualSublineOffset, bool* snapped /* = 0 */) const /*throw()*/ {
	if(logicalLine == 0 && visualSublineOffset == 0)
		return;
	const Margins margins(textAreaMargins());
	if(snapped != 0) {
		const NativeRectangle clientBounds(bounds(false));
		*snapped = y < geometry::top(clientBounds) + margins.top || y >= geometry::bottom(clientBounds) - margins.bottom;
	}
	y -= margins.top;
	length_t line, subline;
	firstVisibleLine(&line, 0, &subline);
	renderer_->layouts().offsetVisualLine(
		line, subline, y / renderer_->defaultFont()->metrics().linePitch(), (snapped == 0 || *snapped) ? 0 : snapped);
	if(logicalLine != 0)
		*logicalLine = line;
	if(visualSublineOffset != 0)
		*visualSublineOffset = subline;
}

/**
 * Returns the client y-coordinate of the logical line.
 * @param line The logical line number
 * @param fullSearch @c false to return special value for the line outside of the client area
 * @return The y-coordinate of the top of the line
 * @retval std#numeric_limits&lt;Scalar&gt;::max() @a fullSearch is @c false and @a line is outside
 *                                                 of the client area upward
 * @retval std#numeric_limits&lt;Scalar&gt;::min() @a fullSearch is @c false and @a line is outside
 *                                                 of the client area downward
 * @throw BadPositionException @a line is outside of the document
 * @see #mapClientYToLine, TextRenderer#offsetVisualLine
 */
Scalar TextViewer::mapLineToClientY(length_t line, bool fullSearch) const {
	const Margins margins(textAreaMargins());
	if(line == scrollInfo_.firstVisibleLine) {
		if(scrollInfo_.firstVisibleSubline == 0)
			return margins.top;
		else
			return fullSearch ? margins.top
				- static_cast<Scalar>(renderer_->defaultFont()->metrics().linePitch() * scrollInfo_.firstVisibleSubline) : numeric_limits<Scalar>::min();
	} else if(line > scrollInfo_.firstVisibleLine) {
		const Scalar lineSpan = renderer_->defaultFont()->metrics().linePitch();
		const NativeRectangle clientBounds(bounds(false));
		Scalar y = margins.top;
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
		Scalar y = margins.top - static_cast<Scalar>(linePitch * scrollInfo_.firstVisibleSubline);
		for(length_t i = scrollInfo_.firstVisibleLine - 1; ; --i) {
			y -= static_cast<Scalar>(renderer_->layouts().numberOfSublinesOfLine(i) * linePitch);
			if(i == line)
				break;
		}
		return y;
	}
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
	ASCENSION_RESTORE_VANISHED_CURSOR();
	if(allowsMouseInput() && mouseInputStrategy_.get() != 0)
		mouseInputStrategy_->mouseMoved(input);
}

/// @see Widget#mousePressed
void TextViewer::mousePressed(const MouseButtonInput& input) {
	ASCENSION_RESTORE_VANISHED_CURSOR();
	if(allowsMouseInput() && mouseInputStrategy_.get() != 0)
		mouseInputStrategy_->mouseButtonInput(MouseInputStrategy::PRESSED, input);
}

/// @see Widget#mouseReleased
void TextViewer::mouseReleased(const MouseButtonInput& input) {
	if(allowsMouseInput() || input.button() == UserInput::BUTTON3_DOWN)
		ASCENSION_RESTORE_VANISHED_CURSOR();
	if(allowsMouseInput() && mouseInputStrategy_.get() != 0)
		mouseInputStrategy_->mouseButtonInput(MouseInputStrategy::RELEASED, input);
}

/// @see Widget#mouseWheelChanged
void TextViewer::mouseWheelChanged(const MouseWheelInput& input) {
	ASCENSION_RESTORE_VANISHED_CURSOR();
	if(allowsMouseInput() && mouseInputStrategy_.get() != 0)
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

	const k::Document& doc = document();
	const NativeRectangle clientBounds(bounds(false));

//	Timer tm(L"onPaint");

	const length_t lines = doc.numberOfLines();
	const int linePitch = renderer_->defaultFont()->metrics().linePitch();

	// paint the ruler
	rulerPainter_->paint(context);

	// draw horizontal margins
	const Margins margins(textAreaMargins());
	const Color marginColor((configuration_.background != Color()) ?
		configuration_.background : Color::fromCOLORREF(::GetSysColor(COLOR_WINDOW)));
	if(margins.left > 0) {
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

	// draw lines
	const TextLayout::Selection selection(*caret_,
		configuration_.selectionForeground != Color() ? configuration_.selectionForeground :
			Color::fromCOLORREF(::GetSysColor(hasFocus() ? COLOR_HIGHLIGHTTEXT : COLOR_INACTIVECAPTIONTEXT)),
		configuration_.selectionBackground != Color() ? configuration_.selectionBackground :
			Color::fromCOLORREF(::GetSysColor(hasFocus() ? COLOR_HIGHLIGHT : COLOR_INACTIVECAPTION)));
	NativeRectangle lineBounds(clientBounds);
	assert(geometry::isNormalized(lineBounds));
	geometry::range<geometry::X_COORDINATE>(lineBounds) = makeRange(
		geometry::left(lineBounds) + margins.left, geometry::right(lineBounds) - margins.right);
	geometry::range<geometry::Y_COORDINATE>(lineBounds) = makeRange(
		geometry::top(lineBounds) + margins.top, geometry::bottom(lineBounds) - margins.bottom);
	if(!geometry::isNormalized(lineBounds))
		geometry::resize(lineBounds, geometry::make<NativeSize>(0, 0));
	length_t line, subline;
	mapClientYToLine(geometry::top(scheduledBounds), &line, &subline);
	Scalar y = mapLineToClientY(line, true);
	if(line < lines) {
		while(y < geometry::bottom(scheduledBounds) && line < lines) {
			// draw a logical line
			renderer_->renderLine(line, context, getDisplayXOffset(line), y, scheduledBounds, lineBounds, &selection);
			y += linePitch * static_cast<Scalar>(renderer_->layouts().numberOfSublinesOfLine(line++));
			subline = 0;
		}
	}

	// paint behind the last
	if(geometry::bottom(scheduledBounds) > y && y > margins.top + linePitch - 1) {
		context.setFillStyle(Paint(marginColor));
		context.fillRectangle(
			geometry::make<NativeRectangle>(
				geometry::make<NativePoint>(geometry::left(clientBounds) + margins.left, y),
				geometry::make<NativeSize>(geometry::dx(clientBounds) - margins.left - margins.right, geometry::bottom(scheduledBounds) - y)));
	}

	// draw top margin
	if(margins.top > 0) {
		context.setFillStyle(Paint(marginColor));
		context.fillRectangle(
			geometry::make<NativeRectangle>(
				geometry::make<NativePoint>(geometry::left(clientBounds) + margins.left, geometry::top(clientBounds)),
				geometry::make<NativeSize>(geometry::dx(clientBounds) - margins.left - margins.right, margins.top)));
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
 * @param line the line to be redrawn
 * @param following @c true to redraw also the all lines follow to @a line
 */
void TextViewer::redrawLine(length_t line, bool following) {
	redrawLines(line, following ? numeric_limits<length_t>::max() : line);
}

/**
 * Redraws the specified lines on the view. If the viewer is frozen, redraws after unfrozen.
 * @param first The start of the lines to be redrawn
 * @param last The end of the lines to be redrawn. This value is inclusive and this line will be
 *             redrawn. If this value is @c std#numeric_limits<length_t>#max(), redraws the
 *             @a first line and the below lines
 * @throw std#invalid_argument @a first is gretaer than @a last
 */
void TextViewer::redrawLines(length_t first, length_t last) {
	if(first > last)
		throw invalid_argument("first is greater than last.");
	checkInitialization();

	if(isFrozen()) {
		freezeInfo_.invalidLines.first =
			(freezeInfo_.invalidLines.first == INVALID_INDEX) ? first : min(first, freezeInfo_.invalidLines.first);
		freezeInfo_.invalidLines.second = 
			(freezeInfo_.invalidLines.second == INVALID_INDEX) ? last : max(last, freezeInfo_.invalidLines.second);
		return;
	}

	const length_t lines = document().numberOfLines();
	if(first >= lines || last < scrollInfo_.firstVisibleLine)
		return;

#ifdef _DEBUG
	if(DIAGNOSE_INHERENT_DRAWING)
		win32::DumpContext() << L"@TextViewer.redrawLines invalidates lines ["
			<< static_cast<unsigned long>(first) << L".." << static_cast<unsigned long>(last) << L"]\n";
#endif // _DEBUG

	NativeRectangle rect(bounds(false));
	assert(geometry::isNormalized(rect));

	// 上端
	geometry::set<0>(rect, geometry::make<NativePoint>(
		geometry::x(geometry::get<0>(rect)), max(mapLineToClientY(first, false), configuration_.topMargin)));
	if(geometry::dy(rect) <= 0)
		return;
	// 下端
	if(last != numeric_limits<length_t>::max()) {
		Scalar bottom = geometry::top(rect) + static_cast<Scalar>(
			renderer_->layouts().numberOfSublinesOfLine(first) * renderer_->defaultFont()->metrics().linePitch());
		for(length_t line = first + 1; line <= last; ++line) {
			bottom += static_cast<Scalar>(
				renderer_->layouts().numberOfSublinesOfLine(line) * renderer_->defaultFont()->metrics().linePitch());
			if(bottom >= geometry::bottom(rect))
				break;
		}
		if(bottom < geometry::bottom(rect))
			geometry::set<1>(rect, geometry::make<NativePoint>(geometry::x(geometry::get<1>(rect)), bottom));
	}
	scheduleRedraw(rect, false);
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
 * @param dx the number of columns to scroll horizontally
 * @param dy the number of visual lines to scroll vertically
 * @param redraw if redraws after scroll
 */
void TextViewer::scroll(int dx, int dy, bool redraw) {
	checkInitialization();
	if(scrollInfo_.lockCount != 0)
		return;

	// preprocess and update the scroll bars
	if(dx != 0) {
		dx = min<int>(dx, scrollInfo_.horizontal.maximum - scrollInfo_.horizontal.pageSize - scrollInfo_.horizontal.position + 1);
		dx = max(dx, -scrollInfo_.horizontal.position);
		if(dx != 0) {
			scrollInfo_.horizontal.position += dx;
			if(!isFrozen())
				setScrollPosition(SB_HORZ, scrollInfo_.horizontal.position, true);
		}
	}
	if(dy != 0) {
		dy = min<int>(dy, scrollInfo_.vertical.maximum - scrollInfo_.vertical.pageSize - scrollInfo_.vertical.position + 1);
		dy = max(dy, -scrollInfo_.vertical.position);
		if(dy != 0) {
			scrollInfo_.vertical.position += dy;
			renderer_->layouts().offsetVisualLine(scrollInfo_.firstVisibleLine, scrollInfo_.firstVisibleSubline, dy);
			if(!isFrozen())
				setScrollPosition(SB_VERT, scrollInfo_.vertical.position, true);
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

	// scroll
	const Margins margins(textAreaMargins());
	NativeRectangle clientBounds(bounds(false));
	NativeRectangle clipBounds(clientBounds);
	assert(geometry::isNormalized(clipBounds));
	geometry::range<geometry::Y_COORDINATE>(clipBounds) = makeRange(
		geometry::top(clipBounds) + margins.top, geometry::bottom(clipBounds) - margins.bottom);
	if(static_cast<unsigned int>(abs(dy)) >= numberOfVisibleLines())
		scheduleRedraw(clipBounds, false);	// redraw all if the amount of the scroll is over a page
	else if(dx == 0) {	// only vertical
		::ScrollWindowEx(identifier().get(),
			0, -dy * scrollRate(false) * renderer_->defaultFont()->metrics().linePitch(), 0, &clipBounds, 0, 0, SW_INVALIDATE);
	} else {	// process the leading margin and the edit region independently
		// scroll the edit region
		geometry::range<geometry::X_COORDINATE>(clipBounds) = makeRange(
			geometry::left(clipBounds) + margins.left, geometry::right(clipBounds) - margins.right);
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
 * @param x the visual line of the position. if set -1, does not scroll in this direction
 * @param y the column of the position. if set -1, does not scroll in this direction
 * @param redraw @c true to redraw the window after scroll
 * @see #scroll
 */
void TextViewer::scrollTo(int x, int y, bool redraw) {
	checkInitialization();
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
	checkInitialization();
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

/// @see ICaretStateListener#selectionShapeChanged
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
	checkInitialization();
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
	checkInitialization();

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
 * Returns the margins of text area.
 * @return The margins
 */
TextViewer::Margins TextViewer::textAreaMargins() const /*throw()*/ {
	Margins margins = {0, 0, 0, 0};
	(utils::isRulerLeftAligned(*this) ? margins.left : margins.right) += rulerPainter_->width();
	const detail::PhysicalTextAnchor alignment = detail::computePhysicalTextAnchor(
		presentation().globalTextStyle()->defaultLineStyle->anchor, configuration_.readingDirection);
	if(alignment == detail::LEFT)
		margins.left += configuration_.leadingMargin;
	else if(alignment == detail::RIGHT)
		margins.right += configuration_.leadingMargin;
	margins.top += configuration_.topMargin;
	return margins;
}

/**
 * Revokes the frozen state of the viewer.
 * @throw WindowNotInitialized The window is not initialized
 * @see #freeze, #isFrozen
 */
void TextViewer::unfreeze() {
	checkInitialization();
	if(freezeInfo_.count > 0 && --freezeInfo_.count == 0) {
		if(scrollInfo_.changed) {
			updateScrollBars();
			scheduleRedraw(false);
		} else if(freezeInfo_.invalidLines.first != INVALID_INDEX)
			redrawLines(freezeInfo_.invalidLines.first, freezeInfo_.invalidLines.second);
		freezeInfo_.invalidLines.first = freezeInfo_.invalidLines.second = INVALID_INDEX;

		rulerPainter_->update();

		caretMoved(caret(), caret().selectedRegion());
		redrawScheduledRegion();
	}
}

/// Moves the caret to valid position with current position, scroll context, and the fonts.
void TextViewer::updateCaretPosition() {
	if(!hasFocus() || isFrozen())
		return;

	NativePoint p(clientXYForCharacter(caret(), false, TextLayout::LEADING));
	const Margins margins(textAreaMargins());
	NativeRectangle textArea(bounds(false));
	assert(geometry::isNormalized(textArea));
	geometry::range<geometry::X_COORDINATE>(textArea) = makeRange(
		geometry::left(textArea) + margins.left, geometry::right(textArea) - margins.right - 1);
	geometry::range<geometry::Y_COORDINATE>(textArea) = makeRange(
		geometry::top(textArea) + margins.top, geometry::bottom(textArea) - margins.bottom);

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
	checkInitialization();
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
		win32::AutoZeroSize<SCROLLINFO> scroll;
		scroll.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
		scroll.nMax = configuration_.lineWrap.wrapsAtWindowEdge() ? 0 : scrollInfo_.horizontal.maximum;
		scroll.nPage = scrollInfo_.horizontal.pageSize;
		scroll.nPos = scrollInfo_.horizontal.position;
		setScrollInformation(SB_HORZ, scroll, true);
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
		win32::AutoZeroSize<SCROLLINFO> scroll;
		scroll.fMask = SIF_DISABLENOSCROLL | SIF_PAGE | SIF_POS | SIF_RANGE;
		scroll.nMax = scrollInfo_.vertical.maximum;
		scroll.nPage = scrollInfo_.vertical.pageSize;
		scroll.nPos = scrollInfo_.vertical.position;
		setScrollInformation(SB_VERT, scroll, true);
	}

	scrollInfo_.changed = isFrozen();

#undef ASCENSION_GET_SCROLL_MINIMUM
}

/// @see VisualLinesListener#visualLinesDeleted
void TextViewer::visualLinesDeleted(length_t first, length_t last, length_t sublines, bool longestLineChanged) /*throw()*/ {
	scrollInfo_.changed = true;
	if(last < scrollInfo_.firstVisibleLine) {	// 可視領域より前が削除された
		scrollInfo_.firstVisibleLine -= last - first;
		scrollInfo_.vertical.position -= static_cast<int>(sublines);
		scrollInfo_.vertical.maximum -= static_cast<int>(sublines);
		repaintRuler();
	} else if(first > scrollInfo_.firstVisibleLine
			|| (first == scrollInfo_.firstVisibleLine && scrollInfo_.firstVisibleSubline == 0)) {	// 可視先頭行以降が削除された
		scrollInfo_.vertical.maximum -= static_cast<int>(sublines);
		redrawLine(first, true);
	} else {	// 可視先頭行を含む範囲が削除された
		scrollInfo_.firstVisibleLine = first;
		scrollInfo_.updateVertical(*this);
		redrawLine(first, true);
	}
	if(longestLineChanged)
		scrollInfo_.resetBars(*this, 'h', false);
}

/// @see VisualLinesListener#visualLinesInserted
void TextViewer::visualLinesInserted(length_t first, length_t last) /*throw()*/ {
	scrollInfo_.changed = true;
	if(last < scrollInfo_.firstVisibleLine) {	// 可視領域より前に挿入された
		scrollInfo_.firstVisibleLine += last - first;
		scrollInfo_.vertical.position += static_cast<int>(last - first);
		scrollInfo_.vertical.maximum += static_cast<int>(last - first);
		repaintRuler();
	} else if(first > scrollInfo_.firstVisibleLine
			|| (first == scrollInfo_.firstVisibleLine && scrollInfo_.firstVisibleSubline == 0)) {	// 可視先頭行以降に挿入された
		scrollInfo_.vertical.maximum += static_cast<int>(last - first);
		redrawLine(first, true);
	} else {	// 可視先頭行の前後に挿入された
		scrollInfo_.firstVisibleLine += last - first;
		scrollInfo_.updateVertical(*this);
		redrawLine(first, true);
	}
}

/// @see VisualLinesListener#visualLinesModified
void TextViewer::visualLinesModified(length_t first, length_t last,
		signed_length_t sublinesDifference, bool documentChanged, bool longestLineChanged) /*throw()*/ {
	if(sublinesDifference == 0)	// 表示上の行数が変化しなかった
		redrawLines(first, last - 1);
	else {
		scrollInfo_.changed = true;
		if(last < scrollInfo_.firstVisibleLine) {	// 可視領域より前が変更された
			scrollInfo_.vertical.position += sublinesDifference;
			scrollInfo_.vertical.maximum += sublinesDifference;
			repaintRuler();
		} else if(first > scrollInfo_.firstVisibleLine
				|| (first == scrollInfo_.firstVisibleLine && scrollInfo_.firstVisibleSubline == 0)) {	// 可視先頭行以降が変更された
			scrollInfo_.vertical.maximum += sublinesDifference;
			redrawLine(first, true);
		} else {	// 可視先頭行を含む範囲が変更された
			scrollInfo_.updateVertical(*this);
			redrawLine(first, true);
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


// TextViewer.Renderer ////////////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param viewer The text viewer
 * @param writingMode The initial writing mode
 */
TextViewer::Renderer::Renderer(TextViewer& viewer, const presentation::WritingMode& writingMode) :
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
const WritingMode& TextViewer::Renderer::defaultUIWritingMode() const /*throw()*/ {
	return defaultWritingMode_;
}

/// Rewraps the visual lines at the window's edge.
void TextViewer::Renderer::rewrapAtWindowEdge() {
	class Local {
	public:
		explicit Local(Scalar newWidth) : newWidth_(newWidth) {}
		bool operator()(const LineLayoutVector::LineLayout& layout) const {
			return layout.second->numberOfLines() != 1
				|| layout.second->style().anchor == JUSTIFY
				|| layout.second->maximumInlineProgressionDimension() > newWidth_;
		}
	private:
		const Scalar newWidth_;
	};

	if(viewer_.configuration().lineWrap.wrapsAtWindowEdge()) {
		const NativeRectangle clientBounds(viewer_.bounds(false));
		const Margins margins(viewer_.textAreaMargins());
		layouts().invalidateIf(Local(geometry::dx(clientBounds) - margins.left - margins.right));
	}
}

/// @see TextRenderer#width
Scalar TextViewer::Renderer::width() const /*throw()*/ {
	const LineWrapConfiguration& lwc = viewer_.configuration().lineWrap;
	if(!lwc.wraps()) {
		win32::AutoZeroSize<SCROLLINFO> si;
		si.fMask = SIF_RANGE;
		viewer_.scrollInformation(SB_HORZ, si);
		return (si.nMax + 1) * viewer_.textRenderer().defaultFont()->metrics().averageCharacterWidth();
	} else if(lwc.wrapsAtWindowEdge()) {
		const NativeRectangle clientBounds(viewer_.bounds(false));
		const Margins margins(viewer_.textAreaMargins());
		return geometry::dx(clientBounds) - margins.left - margins.right;
	} else
		return lwc.width;
}


// TextViewer.Configuration /////////////////////////////////////////////////

/// Default constructor.
TextViewer::Configuration::Configuration() /*throw()*/ :
		readingDirection(LEFT_TO_RIGHT), leadingMargin(5), topMargin(1), usesRichTextClipboardFormat(false) {
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
 * @param view the viewer
 * @param region the region consists the rectangle
 */
VirtualBox::VirtualBox(const TextViewer& view, const k::Region& region) /*throw()*/ : view_(view) {
	update(region);
}

/**
 * Returns if the specified point is on the virtual box.
 * @param p The client coordinates of the point
 * @return @c true If the point is on the virtual box
 */
bool VirtualBox::isPointOver(const graphics::NativePoint& p) const /*throw()*/ {
	assert(view_.isWindow());
	if(view_.hitTest(p) != TextViewer::TEXT_AREA)	// ignore if not in text area
		return false;
	const Scalar leftMargin = view_.textAreaMargins().left;
	if(geometry::x(p) < startEdge() + leftMargin || geometry::x(p) >= endEdge() + leftMargin)	// about x-coordinate
		return false;

	// about y-coordinate
	const Point& top = beginning();
	const Point& bottom = end();
	length_t line, subline;
	view_.mapClientYToLine(geometry::y(p), &line, &subline);	// $friendly-access
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
	assert(view_.isWindow());
	const Point& top = beginning();
	const Point& bottom = end();
	if(line < top.line || (line == top.line && subline < top.subline)	// out of the region
			|| line > bottom.line || (line == bottom.line && subline > bottom.subline))
		return false;
	else {
		const TextRenderer& renderer = view_.textRenderer();
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
	const TextRenderer& r = view_.textRenderer();
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


// DefaultMouseInputStrategy ////////////////////////////////////////////////

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
		void provideClassInformation(win32::ClassInformation& classInfomation) const {
			classInformation.style = CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW;
			background = COLOR_WINDOW;
			cursor = MAKEINTRESOURCEW(32513);	// IDC_IBEAM
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
const win32::Handle<HCURSOR>& AutoScrollOriginMark::cursorForScrolling(CursorType type) {
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
 * - Begins OLE drag-and-drop operation when the mouse moved with the left button down.
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

map<UINT_PTR, DefaultMouseInputStrategy*> DefaultMouseInputStrategy::timerTable_;
const UINT DefaultMouseInputStrategy::SELECTION_EXPANSION_INTERVAL = 100;
const UINT DefaultMouseInputStrategy::OLE_DRAGGING_TRACK_INTERVAL = 100;

/**
 * Constructor.
 * @param enableOLEDragAndDrop set true to enable OLE Drag-and-Drop feature.
 * @param showDraggingImage set true to display OLE dragging image
 */
DefaultMouseInputStrategy::DefaultMouseInputStrategy(
		OLEDragAndDropSupport oleDragAndDropSupportLevel) : viewer_(0), state_(NONE), lastHoveredHyperlink_(0) {
	if((dnd_.supportLevel = oleDragAndDropSupportLevel) >= SUPPORT_OLE_DND_WITH_DRAG_IMAGE) {
		dnd_.dragSourceHelper.ComPtr<IDragSourceHelper>::ComPtr(CLSID_DragDropHelper, IID_IDragSourceHelper, CLSCTX_INPROC_SERVER);
		if(dnd_.dragSourceHelper.get() != 0) {
			dnd_.dropTargetHelper.ComPtr<IDropTargetHelper>::ComPtr(CLSID_DragDropHelper, IID_IDropTargetHelper, CLSCTX_INPROC_SERVER);
			if(dnd_.dropTargetHelper.get() == 0)
				dnd_.dragSourceHelper.reset();
		}
	}
}

/// 
void DefaultMouseInputStrategy::beginTimer(UINT interval) {
	endTimer();
	if(const UINT_PTR id = ::SetTimer(0, 0, interval, timeElapsed))
		timerTable_[id] = this;
}

/// @see MouseInputStrategy#captureChanged
void DefaultMouseInputStrategy::captureChanged() {
	endTimer();
	state_ = NONE;
}

namespace {
	HRESULT createSelectionImage(const TextViewer& viewer, const NativePoint& cursorPosition, bool highlightSelection, SHDRAGIMAGE& image) {
		using namespace win32::gdi;

		win32::Handle<HDC> dc(::CreateCompatibleDC(0), &::DeleteDC);
		if(dc.get() == 0)
			return E_FAIL;

		win32::AutoZero<BITMAPV5HEADER> bh;
		bh.bV5Size = sizeof(BITMAPV5HEADER);
		bh.bV5Planes = 1;
		bh.bV5BitCount = 32;
		bh.bV5Compression = BI_BITFIELDS;
		bh.bV5RedMask = 0x00ff0000ul;
		bh.bV5GreenMask = 0x0000ff00ul;
		bh.bV5BlueMask = 0x000000fful;
		bh.bV5AlphaMask = 0xff000000ul;

		// determine the range to draw
		const k::Region selectedRegion(viewer.caret());
		length_t firstLine, firstSubline;
		viewer.firstVisibleLine(&firstLine, 0, &firstSubline);

		// calculate the size of the image
		const NativeRectangle clientBounds(viewer.bounds(false));
		const TextRenderer& renderer = viewer.textRenderer();
		NativeRectangle selectionBounds(geometry::make<NativeRectangle>(
			geometry::make<NativePoint>(numeric_limits<Scalar>::max(), 0),
			geometry::make<NativeSize>(numeric_limits<Scalar>::min(), 0)));
		for(length_t line = selectedRegion.beginning().line, e = selectedRegion.end().line; line <= e; ++line) {
			selectionBounds.bottom += static_cast<LONG>(renderer.defaultFont()->metrics().linePitch() * renderer.layouts()[line].numberOfLines());
			if(geometry::dy(selectionBounds) > geometry::dy(clientBounds))
				return S_FALSE;	// overflow
			const TextLayout& layout = renderer.layouts()[line];
			const Scalar indent = renderer.lineIndent(line);
			Range<length_t> range;
			for(length_t subline = 0, sublines = layout.numberOfLines(); subline < sublines; ++subline) {
				if(selectedRangeOnVisualLine(viewer.caret(), line, subline, range)) {
					range = Range<length_t>(
						range.beginning(),
						min(viewer.document().lineLength(line), range.end()));
					const NativeRectangle sublineBounds(layout.bounds(range));
					selectionBounds.left() = min(geometry::left(sublineBounds) + indent, geometry::left(selectionBounds));
					selectionBounds.right() = max(geometry::right(sublineBounds) + indent, geometry::right(selectionBounds));
					if(geometry::dx(selectionBounds) > geometry::dx(clientBounds))
						return S_FALSE;	// overflow
				}
			}
		}
		bh.bV5Width = geometry::dx(selectionBounds);
		bh.bV5Height = geometry::dy(selectionBounds);

		// create a mask
		win32::Handle<HBITMAP> mask(::CreateBitmap(bh.bV5Width, bh.bV5Height, 1, 1, 0), &::DeleteObject);	// monochrome
		if(mask.get() == 0)
			return E_FAIL;
		HBITMAP oldBitmap = ::SelectObject(dc.get(), mask.get());
		dc.fillSolidRect(0, 0, bh.bV5Width, bh.bV5Height, RGB(0x00, 0x00, 0x00));
		int y = 0;
		for(length_t line = selectedRegion.beginning().line, e = selectedRegion.end().line; line <= e; ++line) {
			const TextLayout& layout = renderer.layouts()[line];
			const int indent = renderer.lineIndent(line);
			Range<length_t> range;
			for(length_t subline = 0, sublines = layout.numberOfLines(); subline < sublines; ++subline) {
				if(selectedRangeOnVisualLine(viewer.caret(), line, subline, range)) {
					range = Range<length_t>(
						range.beginning(),
						min(viewer.document().lineLength(line), range.end()));
					win32::Handle<HRGN> rgn(layout.blackBoxBounds(range.beginning(), range.end()));
					::OffsetRgn(rgn.get(), indent - geometry::left(selectionBounds), y - geometry::top(selectionBounds));
					::FillRgn(dc.get(), rgn.get(), Brush::getStockObject(WHITE_BRUSH).use());
				}
				y += renderer.defaultFont()->metrics().linePitch();
			}
		}
		::SelectObject(dc.get(), oldBitmap);
		BITMAPINFO* bi = 0;
		AutoBuffer<byte> maskBuffer;
		uint8_t* maskBits;
		BYTE alphaChunnels[2] = {0xff, 0x01};
		try {
			bi = static_cast<BITMAPINFO*>(::operator new(sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 2));
			memset(&bi->bmiHeader, 0, sizeof(BITMAPINFOHEADER));
			bi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			int r = ::GetDIBits(dc.get(), mask.get(), 0, bh.bV5Height, 0, bi, DIB_RGB_COLORS);
			if(r == 0 || r == ERROR_INVALID_PARAMETER)
				throw runtime_error("");
			assert(bi->bmiHeader.biBitCount == 1 && bi->bmiHeader.biClrUsed == 2);
			maskBuffer.reset(new uint8_t[bi->bmiHeader.biSizeImage + sizeof(DWORD)]);
			maskBits = maskBuffer.get() + sizeof(DWORD) - reinterpret_cast<ULONG_PTR>(maskBuffer.get()) % sizeof(DWORD);
			r = ::GetDIBits(dc.get(), mask.get(), 0, bh.bV5Height, maskBits, bi, DIB_RGB_COLORS);
			if(r == 0 || r == ERROR_INVALID_PARAMETER)
				throw runtime_error("");
			if(bi->bmiColors[0].rgbRed == 0xff && bi->bmiColors[0].rgbGreen == 0xff && bi->bmiColors[0].rgbBlue == 0xff)
				swap(alphaChunnels[0], alphaChunnels[1]);
		} catch(const bad_alloc&) {
			return E_OUTOFMEMORY;
		} catch(const runtime_error&) {
			::operator delete(bi);
			return E_FAIL;
		}
		::operator delete(bi);

		// create the result bitmap
		void* bits;
		win32::Handle<HBITMAP> bitmap(::CreateDIBSection(dc.get(), *reinterpret_cast<BITMAPINFO*>(&bh), DIB_RGB_COLORS, bits));
		if(bitmap.get() == 0)
			return E_FAIL;
		// render the lines
		oldBitmap = ::SelectObject(dc.get(), bitmap.get());
		NativeRectangle selectionExtent(selectionBounds);
		geometry::translate(selectionExtent, geometry::negate(geometry::topLeft(selectionExtent)));
		y = selectionBounds.top;
		const TextLayout::Selection selection(viewer.caret());
		for(length_t line = selectedRegion.beginning().line, e = selectedRegion.end().line; line <= e; ++line) {
			renderer.renderLine(line, dc,
				renderer.lineIndent(line) - geometry::left(selectionBounds), y,
				selectionExtent, selectionExtent, highlightSelection ? &selection : 0);
			y += static_cast<int>(renderer.defaultFont()->metrics().linePitch() * renderer.numberOfSublinesOfLine(line));
		}
		::SelectObject(dc.get(), oldBitmap);

		// set alpha chunnel
		const uint8_t* maskByte = maskBits;
		for(LONG y = 0; y < bh.bV5Height; ++y) {
			for(LONG x = 0; ; ) {
				RGBQUAD& pixel = static_cast<RGBQUAD*>(bits)[x + bh.bV5Width * y];
				pixel.rgbReserved = alphaChunnels[(*maskByte & (1 << ((8 - x % 8) - 1))) ? 0 : 1];
				if(x % 8 == 7)
					++maskByte;
				if(++x == bh.bV5Width) {
					if(x % 8 != 0)
						++maskByte;
					break;
				}
			}
			if(reinterpret_cast<ULONG_PTR>(maskByte) % sizeof(DWORD) != 0)
				maskByte += sizeof(DWORD) - reinterpret_cast<ULONG_PTR>(maskByte) % sizeof(DWORD);
		}

		// locate the hotspot of the image based on the cursor position
		const Margins margins(viewer.textAreaMargins());
		NativePoint hotspot(cursorPosition);
		geometry::x(hotspot) -= margins.left - viewer.scrollPosition(SB_HORZ) * renderer.defaultFont()->metrics().averageCharacterWidth() + geometry::left(selectionBounds);
		geometry::y(hotspot) -= geometry::y(viewer.clientXYForCharacter(k::Position(selectedRegion.beginning().line, 0), true));

		memset(&image, 0, sizeof(SHDRAGIMAGE));
		image.sizeDragImage.cx = bh.bV5Width;
		image.sizeDragImage.cy = bh.bV5Height;
		image.ptOffset = hotspot;
		image.hbmpDragImage = static_cast<HBITMAP>(bitmap.release());
		image.crColorKey = CLR_NONE;

		return S_OK;
	}
}

HRESULT DefaultMouseInputStrategy::doDragAndDrop() {
	win32::com::ComPtr<IDataObject> draggingContent;
	const Caret& caret = viewer_->caret();
	HRESULT hr;

	if(FAILED(hr = utils::createTextObjectForSelectedString(viewer_->caret(), true, *draggingContent.initialize())))
		return hr;
	if(!caret.isSelectionRectangle())
		dnd_.numberOfRectangleLines = 0;
	else {
		const Region selection(caret.selectedRegion());
		dnd_.numberOfRectangleLines = selection.end().line - selection.beginning().line + 1;
	}

	// setup drag-image
	if(dnd_.dragSourceHelper.get() != 0) {
		SHDRAGIMAGE image;
		if(SUCCEEDED(hr = createSelectionImage(*viewer_,
				dragApproachedPosition_, dnd_.supportLevel >= SUPPORT_OLE_DND_WITH_SELECTED_DRAG_IMAGE, image))) {
			if(FAILED(hr = dnd_.dragSourceHelper->InitializeFromBitmap(&image, draggingContent.get())))
				::DeleteObject(image.hbmpDragImage);
		}
	}

	// operation
	state_ = OLE_DND_SOURCE;
	DWORD effectOwn;	// dummy
	hr = ::DoDragDrop(draggingContent.get(), this, DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_SCROLL, &effectOwn);
	state_ = NONE;
	if(viewer_->isVisible())
		viewer_->setFocus();
	return hr;
}

/// @see IDropTarget#DragEnter
STDMETHODIMP DefaultMouseInputStrategy::DragEnter(IDataObject* data, DWORD keyState, POINTL pt, DWORD* effect) {
	if(data == 0)
		return E_INVALIDARG;
	MANAH_VERIFY_POINTER(effect);
	*effect = DROPEFFECT_NONE;

	HRESULT hr;

#ifdef _DEBUG
	{
		win32::DumpContext dout;
		com::ComPtr<IEnumFORMATETC> formats;
		if(SUCCEEDED(hr = data->EnumFormatEtc(DATADIR_GET, formats.initialize()))) {
			FORMATETC format;
			ULONG fetched;
			dout << L"DragEnter received a data object exposes the following formats.\n";
			for(formats->Reset(); formats->Next(1, &format, &fetched) == S_OK; ) {
				WCHAR name[256];
				if(::GetClipboardFormatNameW(format.cfFormat, name, MANAH_COUNTOF(name) - 1) != 0)
					dout << L"\t" << name << L"\n";
				else
					dout << L"\t" << L"(unknown format : " << format.cfFormat << L")\n";
				if(format.ptd != 0)
					::CoTaskMemFree(format.ptd);
			}
		}
	}
#endif // _DEBUG

	if(dnd_.supportLevel == DONT_SUPPORT_OLE_DND || viewer_->document().isReadOnly() || !viewer_->allowsMouseInput())
		return S_OK;

	// validate the dragged data if can drop
	FORMATETC fe = {CF_UNICODETEXT, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	if((hr = data->QueryGetData(&fe)) != S_OK) {
		fe.cfFormat = CF_TEXT;
		if(SUCCEEDED(hr = data->QueryGetData(&fe) != S_OK))
			return S_OK;	// can't accept
	}

	if(state_ != OLE_DND_SOURCE) {
		assert(state_ == NONE);
		// retrieve number of lines if text is rectangle
		dnd_.numberOfRectangleLines = 0;
		fe.cfFormat = static_cast<CLIPFORMAT>(::RegisterClipboardFormatW(ASCENSION_RECTANGLE_TEXT_CLIP_FORMAT));
		if(fe.cfFormat != 0 && data->QueryGetData(&fe) == S_OK) {
			const TextAlignment alignment = defaultTextAlignment(viewer_->presentation());
			const ReadingDirection readingDirection = defaultReadingDirection(viewer_->presentation());
			if(alignment == ALIGN_END
					|| (alignment == ALIGN_LEFT && readingDirection == RIGHT_TO_LEFT)
					|| (alignment == ALIGN_RIGHT && readingDirection == LEFT_TO_RIGHT))
				return S_OK;	// TODO: support alignments other than ALIGN_LEFT.
			pair<HRESULT, String> text(utils::getTextFromDataObject(*data));
			if(SUCCEEDED(text.first))
				dnd_.numberOfRectangleLines = calculateNumberOfLines(text.second) - 1;
		}
		state_ = OLE_DND_TARGET;
	}

	viewer_->setFocus();
	beginTimer(OLE_DRAGGING_TRACK_INTERVAL);
	if(dnd_.dropTargetHelper.get() != 0) {
		POINT p = {pt.x, pt.y};
		hr = dnd_.dropTargetHelper->DragEnter(viewer_->identifier().get(), data, &p, *effect);
	}
	return DragOver(keyState, pt, effect);
}

/// @see IDropTarget#DragLeave
STDMETHODIMP DefaultMouseInputStrategy::DragLeave() {
	::SetFocus(0);
	endTimer();
	if(dnd_.supportLevel >= SUPPORT_OLE_DND) {
		if(state_ == OLE_DND_TARGET)
			state_ = NONE;
		if(dnd_.dropTargetHelper.get() != 0)
			dnd_.dropTargetHelper->DragLeave();
	}
	return S_OK;
}

namespace {
	inline NativeSize calculateDnDScrollOffset(const TextViewer& viewer) {
		const NativePoint p = viewer.mapFromGlobal(Cursor::position());
		const NativeRectangle clientBounds(viewer.bounds(false));
		TextViewer::Margins margins(viewer.textAreaMargins());
		const Font::Metrics& fontMetrics = viewer.textRenderer().defaultFont()->metrics();
		margins.left = max<Scalar>(fontMetrics.averageCharacterWidth(), margins.left);
		margins.top = max<Scalar>(fontMetrics.linePitch() / 2, margins.top);
		margins.right = max<Scalar>(fontMetrics.averageCharacterWidth(), margins.right);
		margins.bottom = max<Scalar>(fontMetrics.linePitch() / 2, margins.bottom);

		// oleidl.h defines the value named DD_DEFSCROLLINSET, but...

		typename geometry::Coordinate<NativeSize>::Type dx = 0, dy = 0;
		if(makeRange(geometry::top(clientBounds), geometry::top(clientBounds) + margins.top).includes(geometry::y(p)))
			dy = -1;
		else if(geometry::y(p) >= geometry::bottom(clientBounds) - margins.bottom && geometry::y(p) < geometry::bottom(clientBounds))
			dy = +1;
		if(makeRange(geometry::left(clientBounds), geometry::left(clientBounds) + margins.left).includes(geometry::x(p)))
			dx = -3;	// viewer_->numberOfVisibleColumns()
		else if(geometry::x(p) >= geometry::right(clientBounds) - margins.right && geometry::x(p) < geometry::right(clientBounds))
			dx = +3;	// viewer_->numberOfVisibleColumns()
		return geometry::make<NativeSize>(dx, dy);
	}
} // namespace @0

/// @see IDropTarget#DragOver
STDMETHODIMP DefaultMouseInputStrategy::DragOver(DWORD keyState, POINTL pt, DWORD* effect) {
	MANAH_VERIFY_POINTER(effect);
	*effect = DROPEFFECT_NONE;
	bool acceptable = true;

	if((state_ != OLE_DND_SOURCE && state_ != OLE_DND_TARGET) || viewer_->document().isReadOnly() || !viewer_->allowsMouseInput())
		acceptable = false;
	else {
		const NativePoint caretPoint(viewer_->mapFromGlobal(geometry::make<NativePoint>(pt.x, pt.y)));
		const k::Position p(viewer_->characterForClientXY(caretPoint, TextLayout::TRAILING));
		viewer_->setCaretPosition(viewer_->clientXYForCharacter(p, true, TextLayout::LEADING));

		// drop rectangle text into bidirectional line is not supported...
		if(dnd_.numberOfRectangleLines != 0) {
			const length_t lines = min(viewer_->document().numberOfLines(), p.line + dnd_.numberOfRectangleLines);
			for(length_t line = p.line; line < lines; ++line) {
				if(viewer_->textRenderer().layouts()[line].isBidirectional()) {
					acceptable = false;
					break;
				}
			}
		}
	}

	if(acceptable) {
		*effect = ((keyState & MK_CONTROL) != 0) ? DROPEFFECT_COPY : DROPEFFECT_MOVE;
		const NativeSize scrollOffset(calculateDnDScrollOffset(*viewer_));
		if(geometry::dx(scrollOffset) != 0 || geometry::dy(scrollOffset) != 0) {
			*effect |= DROPEFFECT_SCROLL;
			// only one direction to scroll
			if(geometry::dy(scrollOffset) != 0)
				viewer_->scroll(0, geometry::dy(scrollOffset), true);
			else
				viewer_->scroll(geometry::dx(scrollOffset), 0, true);
		}
	}
	if(dnd_.dropTargetHelper.get() != 0) {
		viewer_->lockScroll();
		dnd_.dropTargetHelper->DragOver(&geometry::make<NativePoint>(pt.x, pt.y), *effect);	// damn! IDropTargetHelper scrolls the view
		viewer_->lockScroll(true);
	}
	return S_OK;
}

/// @see IDropTarget#Drop
STDMETHODIMP DefaultMouseInputStrategy::Drop(IDataObject* data, DWORD keyState, POINTL pt, DWORD* effect) {
	if(dnd_.dropTargetHelper.get() != 0) {
		POINT p = {pt.x, pt.y};
		dnd_.dropTargetHelper->Drop(data, &p, *effect);
	}
	if(data == 0)
		return E_INVALIDARG;
	MANAH_VERIFY_POINTER(effect);
	*effect = DROPEFFECT_NONE;
/*
	FORMATETC fe = {::RegisterClipboardFormatA("Rich Text Format"), 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	STGMEDIUM stg;
	data->GetData(&fe, &stg);
	const char* bytes = static_cast<char*>(::GlobalLock(stg.hGlobal));
	manah::win32::DumpContext dout;
	dout << bytes;
	::GlobalUnlock(stg.hGlobal);
	::ReleaseStgMedium(&stg);
*/
	k::Document& document = viewer_->document();
	if(dnd_.supportLevel == DONT_SUPPORT_OLE_DND || document.isReadOnly() || !viewer_->allowsMouseInput())
		return S_OK;
	Caret& ca = viewer_->caret();
	const NativePoint caretPoint(viewer_->mapFromGlobal(geometry::make<NativePoint>(pt.x, pt.y)));
	const k::Position destination(viewer_->characterForClientXY(caretPoint, TextLayout::TRAILING));

	if(!document.accessibleRegion().includes(destination))
		return S_OK;

	if(state_ == OLE_DND_TARGET) {	// dropped from the other widget
		endTimer();
		ca.moveTo(destination);

		bool rectangle;
		pair<HRESULT, String> content(utils::getTextFromDataObject(*data, &rectangle));
		if(SUCCEEDED(content.first)) {
			AutoFreeze af(viewer_);
			bool failed = false;
			ca.moveTo(destination);
			try {
				ca.replaceSelection(content.second, rectangle);
			} catch(...) {
				failed = true;
			}
			if(!failed) {
				if(rectangle)
					ca.beginRectangleSelection();
				ca.select(destination, ca);
				*effect = DROPEFFECT_COPY;
			}
		}
		state_ = NONE;
	} else {	// drop from the same widget
		assert(state_ == OLE_DND_SOURCE);
		String text(selectedString(ca, text::NLF_RAW_VALUE));

		// can't drop into the selection
		if(isPointOverSelection(ca, caretPoint)) {
			ca.moveTo(destination);
			state_ = NONE;
		} else {
			const bool rectangle = ca.isSelectionRectangle();
			document.insertUndoBoundary();
			AutoFreeze af(viewer_);
			if(win32::boole(keyState & MK_CONTROL)) {	// copy
//				viewer_->redrawLines(ca.beginning().line(), ca.end().line());
				bool failed = false;
				ca.enableAutoShow(false);
				ca.moveTo(destination);
				try {
					ca.replaceSelection(text, rectangle);
				} catch(...) {
					failed = true;
				}
				ca.enableAutoShow(true);
				if(!failed) {
					ca.select(destination, ca);
					*effect = DROPEFFECT_COPY;
				}
			} else {	// move as a rectangle or linear
				bool failed = false;
				pair<k::Point, k::Point> oldSelection(make_pair(k::Point(ca.anchor()), k::Point(ca)));
				ca.enableAutoShow(false);
				ca.moveTo(destination);
				try {
					ca.replaceSelection(text, rectangle);
				} catch(...) {
					failed = true;
				}
				if(!failed) {
					ca.select(destination, ca);
					if(rectangle)
						ca.beginRectangleSelection();
					try {
						erase(ca.document(), oldSelection.first, oldSelection.second);
					} catch(...) {
						failed = true;
					}
				}
				ca.enableAutoShow(true);
				if(!failed)
					*effect = DROPEFFECT_MOVE;
			}
			document.insertUndoBoundary();
		}
	}
	return S_OK;
}

/**
 * Ends the auto scroll.
 * @return true if the auto scroll was active
 */
bool DefaultMouseInputStrategy::endAutoScroll() {
	if(state_ == AUTO_SCROLL_DRAGGING || state_ == AUTO_SCROLL) {
		endTimer();
		state_ = NONE;
		autoScrollOriginMark_->show(SW_HIDE);
		viewer_->releaseInput();
		return true;
	}
	return false;
}

///
void DefaultMouseInputStrategy::endTimer() {
	for(map<UINT_PTR, DefaultMouseInputStrategy*>::iterator i = timerTable_.begin(); i != timerTable_.end(); ++i) {
		if(i->second == this) {
			::KillTimer(0, i->first);
			timerTable_.erase(i);
			return;
		}
	}
}

/// Extends the selection to the current cursor position.
void DefaultMouseInputStrategy::extendSelection(const k::Position* to /* = 0 */) {
	if((state_ & SELECTION_EXTENDING_MASK) != SELECTION_EXTENDING_MASK)
		throw IllegalStateException("not extending the selection.");
	k::Position destination;
	if(to == 0) {
		const NativeRectangle clientBounds(viewer_->bounds(false));
		const TextViewer::Margins margins(viewer_->textAreaMargins());
		NativePoint p(viewer_->mapFromGlobal(Cursor::position()));
		Caret& caret = viewer_->caret();
		if(state_ != EXTENDING_CHARACTER_SELECTION) {
			const TextViewer::HitTestResult htr = viewer_->hitTest(p);
			if(state_ == EXTENDING_LINE_SELECTION && htr != TextViewer::INDICATOR_MARGIN && htr != TextViewer::LINE_NUMBERS)
				// end line selection
				state_ = EXTENDING_CHARACTER_SELECTION;
		}
		p = geometry::make<NativePoint>(
			min(max<Scalar>(geometry::x(p), geometry::left(clientBounds) + margins.left), geometry::right(clientBounds) - margins.right),
			min(max<Scalar>(geometry::y(p), geometry::top(clientBounds) + margins.top), geometry::bottom(clientBounds) - margins.bottom));
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

/// @see IDropSource#GiveFeedback
STDMETHODIMP DefaultMouseInputStrategy::GiveFeedback(DWORD) {
	return DRAGDROP_S_USEDEFAULTCURSORS;	// use the system default cursor
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
		viewer_->setCapture();
		beginTimer(SELECTION_EXPANSION_INTERVAL);
	}

	// approach OLE drag-and-drop
	else if(dnd_.supportLevel >= SUPPORT_OLE_DND && !isSelectionEmpty(caret) && isPointOverSelection(caret, position)) {
		state_ = APPROACHING_OLE_DND;
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
					if(const hyperlink::Hyperlink* link = getPointedHyperlink(*viewer_, p)) {
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
			if(win32::boole(modifiers & (MK_CONTROL | MK_SHIFT))) {
				if(win32::boole(keyState & MK_CONTROL)) {
					// begin word selection
					state_ = EXTENDING_WORD_SELECTION;
					caret.moveTo(win32::boole(keyState & MK_SHIFT) ? caret.anchor() : to);
					selectWord(caret);
					selection_.initialLine = caret.line();
					selection_.initialWordColumns = make_pair(caret.beginning().column(), caret.end().column());
				}
				if(win32::boole(keyState & MK_SHIFT))
					extendSelection(&to);
			} else
				caret.moveTo(to);
			if(win32::boole(::GetKeyState(VK_MENU) & 0x8000))	// make the selection reactangle
				caret.beginRectangleSelection();
			else
				caret.endRectangleSelection();
			viewer_->setCapture();
			beginTimer(SELECTION_EXPANSION_INTERVAL);
		}
	}

//	if(!caret.isSelectionRectangle() && !boxDragging)
//		viewer_->redrawLine(caret.line());
	viewer_->setFocus();
}

/// Handles @c WM_LBUTTONUP.
void DefaultMouseInputStrategy::handleLeftButtonReleased(const NativePoint& position, int) {
	// cancel if OLE drag-and-drop approaching
	if(dnd_.supportLevel >= SUPPORT_OLE_DND
			&& (state_ == APPROACHING_OLE_DND
			|| state_ == OLE_DND_SOURCE)) {	// TODO: this should handle only case APPROACHING_OLE_DND?
		state_ = NONE;
		viewer_->caret().moveTo(viewer_->characterForClientXY(position, LineLayout::TRAILING));
		::SetCursor(::LoadCursor(0, IDC_IBEAM));	// hmm...
	}

	endTimer();
	if((state_ & SELECTION_EXTENDING_MASK) == SELECTION_EXTENDING_MASK) {
		state_ = NONE;
		// if released the button when extending the selection, the scroll may not reach the caret position
		utils::show(viewer_->caret());
	}
	viewer_->releaseCapture();
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

/// @see IMouseInputStrategy#install
void DefaultMouseInputStrategy::install(TextViewer& viewer) {
	if(viewer_ != 0)
		uninstall();
	(viewer_ = &viewer)->registerDragDrop(*this);
	state_ = NONE;

	// create the window for the auto scroll origin mark
	auto_ptr<AutoScrollOriginMark> temp(new AutoScrollOriginMark);
	temp->create(viewer);
	autoScrollOriginMark_ = temp;
}

/// @see MouseInputStrategy#interruptMouseReaction
void DefaultMouseInputStrategy::interruptMouseReaction(bool forKeyboardInput) {
	if(state_ == AUTO_SCROLL_DRAGGING || state_ == AUTO_SCROLL)
		endAutoScroll();
}

/// @see IMouseInputStrategy#mouseButtonInput
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
			if(htr == TextViewer::LEADING_MARGIN || htr == TextViewer::TOP_MARGIN || htr == TextViewer::TEXT_AREA) {
				// begin word selection
				Caret& caret = viewer_->caret();
				selectWord(caret);
				state_ = EXTENDING_WORD_SELECTION;
				selection_.initialLine = caret.line();
				selection_.initialWordColumns = make_pair(caret.anchor().column(), caret.column());
				viewer_->grabInput();
				beginTimer(SELECTION_EXPANSION_INTERVAL);
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
				autoScrollOriginMark_->setPosition(HWND_TOP,
					geometry::x(p) - geometry::dx(rect) / 2, geometry::y(p) - geometry::dy(rect) / 2,
					0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_SHOWWINDOW);
				viewer_->grabInput();
				showCursor(input.location());
				return true;
			}
		} else if(action == RELEASED) {
			if(state_ == APPROACHING_AUTO_SCROLL) {
				state_ = AUTO_SCROLL;
				beginTimer(0);
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
			|| (dnd_.supportLevel >= SUPPORT_OLE_DND && state_ == APPROACHING_OLE_DND)) {	// OLE dragging starts?
		if(state_ == APPROACHING_OLE_DND && isSelectionEmpty(viewer_->caret()))
			state_ = NONE;	// approaching... => cancel
		else {
			// the following code can be replaced with DragDetect in user32.lib
			const int cxDragBox = ::GetSystemMetrics(SM_CXDRAG);
			const int cyDragBox = ::GetSystemMetrics(SM_CYDRAG);
			if((geometry::x(input.location()) > geometry::x(dragApproachedPosition_) + cxDragBox / 2)
					|| (geometry::x(input.location()) < geometry::x(dragApproachedPosition_) - cxDragBox / 2)
					|| (geometry::y(input.location()) > geometry::y(dragApproachedPosition_) + cyDragBox / 2)
					|| (geometry::y(input.location()) < geometry::y(dragApproachedPosition_) - cyDragBox / 2)) {
				if(state_ == APPROACHING_OLE_DND)
					doDragAndDrop();
				else {
					state_ = AUTO_SCROLL_DRAGGING;
					beginTimer(0);
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

/// @see IDropSource#QueryContinueDrag
STDMETHODIMP DefaultMouseInputStrategy::QueryContinueDrag(BOOL escapePressed, DWORD keyState) {
	if(win32::boole(escapePressed) || win32::boole(keyState & MK_RBUTTON))	// cancel
		return DRAGDROP_S_CANCEL;
	if(!win32::boole(keyState & MK_LBUTTON))	// drop
		return DRAGDROP_S_DROP;
	return S_OK;
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
	else if(dnd_.supportLevel >= SUPPORT_OLE_DND && !isSelectionEmpty(viewer_->caret()) && isPointOverSelection(viewer_->caret(), position))
		cursorName = IDC_ARROW;
	else if(htr == TextViewer::TEXT_AREA) {
		// on a hyperlink?
		const k::Position p(viewer_->characterForClientXY(position, TextLayout::TRAILING, true, k::locations::UTF16_CODE_UNIT));
		if(p != k::Position())
			newlyHoveredHyperlink = getPointedHyperlink(*viewer_, p);
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
void CALLBACK DefaultMouseInputStrategy::timeElapsed(HWND, UINT, UINT_PTR eventID, DWORD) {
	map<UINT_PTR, DefaultMouseInputStrategy*>::iterator i = timerTable_.find(eventID);
	if(i == timerTable_.end())
		return;
	DefaultMouseInputStrategy& self = *i->second;
	if((self.state_ & SELECTION_EXTENDING_MASK) == SELECTION_EXTENDING_MASK) {	// scroll automatically during extending the selection
		const NativePoint p(self.viewer_->mapFromGlobal(Cursor::position()));
		const NativeRectangle rc(self.viewer_->bounds(false));
		const TextViewer::Margins margins(self.viewer_->textAreaMargins());
		// 以下のスクロール量には根拠は無い
		if(geometry::y(p) < geometry::top(rc) + margins.top)
			self.viewer_->scroll(0, (geometry::y(p) - (geometry::top(rc) + margins.top)) / self.viewer_->textRenderer().defaultFont()->metrics().linePitch() - 1, true);
		else if(geometry::y(p) >= geometry::bottom(rc) - margins.bottom)
			self.viewer_->scroll(0, (geometry::y(p) - (geometry::bottom(rc) - margins.bottom)) / self.viewer_->textRenderer().defaultFont()->metrics().linePitch() + 1, true);
		else if(geometry::x(p) < geometry::left(rc) + margins.left)
			self.viewer_->scroll((geometry::x(p) - (geometry::left(rc) + margins.left)) / self.viewer_->textRenderer().defaultFont()->metrics().averageCharacterWidth() - 1, 0, true);
		else if(geometry::x(p) >= geometry::right(rc) - margins.right)
			self.viewer_->scroll((geometry::x(p) - (geometry::right(rc) - margins.right)) / self.viewer_->textRenderer().defaultFont()->metrics().averageCharacterWidth() + 1, 0, true);
		self.extendSelection();
	} else if(self.state_ == AUTO_SCROLL_DRAGGING || self.state_ == AUTO_SCROLL) {
		self.endTimer();
		TextViewer& viewer = *self.viewer_;
		const NativePoint p(self.viewer_->mapFromGlobal(Cursor::position()));
		const Scalar yScrollDegree = (geometry::y(p) - geometry::y(self.dragApproachedPosition_)) / viewer.textRenderer().defaultFont()->metrics().linePitch();
//		const Scalar xScrollDegree = (geometry::x(p) - geometry::x(self.dragApproachedPosition_)) / viewer.presentation().textMetrics().lineHeight();
//		const Scalar scrollDegree = max(abs(yScrollDegree), abs(xScrollDegree));

		if(yScrollDegree != 0 /*&& abs(yScrollDegree) >= abs(xScrollDegree)*/)
			viewer.scroll(0, yScrollDegree > 0 ? +1 : -1, true);
//		else if(xScrollDegree != 0)
//			viewer.scroll(xScrollDegree > 0 ? +1 : -1, 0, true);

		if(yScrollDegree != 0) {
			self.beginTimer(500 / static_cast<uint>((pow(2.0f, abs(yScrollDegree) / 2))));
			::SetCursor(AutoScrollOriginMark::cursorForScrolling(
				(yScrollDegree > 0) ? AutoScrollOriginMark::CURSOR_DOWNWARD : AutoScrollOriginMark::CURSOR_UPWARD).get());
		} else {
			self.beginTimer(300);
			::SetCursor(AutoScrollOriginMark::cursorForScrolling(AutoScrollOriginMark::CURSOR_NEUTRAL).get());
		}
#if 0
	} else if(self.dnd_.enabled && (self.state_ & OLE_DND_MASK) == OLE_DND_MASK) {	// scroll automatically during OLE dragging
		const SIZE scrollOffset = calculateDnDScrollOffset(*self.viewer_);
		if(scrollOffset.cy != 0)
			self.viewer_->scroll(0, scrollOffset.cy, true);
		else if(scrollOffset.cx != 0)
			self.viewer_->scroll(scrollOffset.cx, 0, true);
#endif
	}
}

/// @see IMouseInputStrategy#uninstall
void DefaultMouseInputStrategy::uninstall() {
	endTimer();
	if(autoScrollOriginMark_.get() != 0) {
		autoScrollOriginMark_->destroy();
		autoScrollOriginMark_.reset();
	}
	viewer_->revokeDragDrop();
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


// ascension.viewers.utils //////////////////////////////////////////////////

/// Closes the opened completion proposals popup immediately.
void utils::closeCompletionProposalsPopup(TextViewer& viewer) /*throw()*/ {
	if(contentassist::ContentAssistant* ca = viewer.contentAssistant()) {
		if(contentassist::ContentAssistant::CompletionProposalsUI* cpui = ca->completionProposalsUI())
			cpui->close();
	}
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
 * @param viewer the text viewer
 * @param[out] startPosition the start of the identifier. can be @c null if not needed
 * @param[out] endPosition the end of the identifier. can be @c null if not needed
 * @return @c false if the identifier is not found (in this case, the values of the output
 *         parameters are undefined)
 * @see #getNearestIdentifier
 */
bool source::getPointedIdentifier(const TextViewer& viewer, k::Position* startPosition, k::Position* endPosition) {
	if(viewer.isWindow()) {
		NativePoint cursorPoint;
		::GetCursorPos(&cursorPoint);
		::ScreenToClient(viewer.identifier().get(), &cursorPoint);
		const k::Position cursor = viewer.characterForClientXY(cursorPoint, TextLayout::LEADING);
		if(source::getNearestIdentifier(viewer.document(), cursor,
				(startPosition != 0) ? &startPosition->column : 0, (endPosition != 0) ? &endPosition->column : 0)) {
			if(startPosition != 0)
				startPosition->line = cursor.line;
			if(endPosition != 0)
				endPosition->line = cursor.line;
			return true;
		}
	}
	return false;
}
