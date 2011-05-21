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

// local helpers
namespace {
	inline Dimension<> getCurrentCharacterSize(const TextViewer& viewer) {
		const Scalar cy = viewer.textRenderer().defaultFont()->metrics().cellHeight();
		const Caret& caret = viewer.caret();
		if(k::locations::isEndOfLine(caret))	// EOL
			return Dimension<>(viewer.textRenderer().defaultFont()->metrics().averageCharacterWidth(), cy);
		else {
			const TextLayout& layout = viewer.textRenderer().lineLayout(caret.line());
			const int leading = layout.location(caret.column(), TextLayout::LEADING).x;
			const int trailing = layout.location(caret.column(), TextLayout::TRAILING).x;
			return Dimension<>(static_cast<int>(detail::distance(leading, trailing)), cy);
		}
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
	rulerPainter_.reset(new RulerPainter(*this, true));

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
	rulerPainter_.reset(new RulerPainter(*this, true));

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
	renderer_->removeVisualLinesListener(*this);
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

/// @see ICaretListener#caretMoved
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
k::Position TextViewer::characterForClientXY(const Point<>& p, TextLayout::Edge edge,
		bool abortNoCharacter /* = false */, k::locations::CharacterUnit snapPolicy /* = k::locations::GRAPHEME_CLUSTER */) const {
	k::Position result;

	// determine the logical line
	length_t subline;
	bool outside;
	mapClientYToLine(p.y, &result.line, &subline, &outside);
	if(abortNoCharacter && outside)
		return k::Position();
	const TextLayout& layout = renderer_->lineLayout(result.line);

	// determine the column
	const Scalar x = p.x - getDisplayXOffset(result.line);
	if(edge == TextLayout::LEADING)
		result.column = layout.offset(
			Point<>(x, static_cast<Scalar>(renderer_->defaultFont()->metrics().linePitch() * subline)), &outside).first;
	else if(edge == TextLayout::TRAILING)
		result.column = layout.offset(
			Point<>(x, static_cast<Scalar>(renderer_->defaultFont()->metrics().linePitch() * subline)), &outside).second;
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
				else if(detail::distance(x, layout.location(result.column - 1).x)
						<= detail::distance(x, layout.location(result.column + 1).x))
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
					result.column = ((detail::distance(x, layout.location(backward.column).x)
						<= detail::distance(x, layout.location(forward.column).x)) ? backward : forward).column;
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
 *         result is 32767 (for upward) or -32768 (for downward)
 * @throw BadPositionException @a position is outside of the document
 * @throw WindowNotInitialized The window is not initialized
 * @see #characterForClientXY, #hitTest, layout#LineLayout#location
 */
Point<> TextViewer::clientXYForCharacter(const k::Position& position, bool fullSearchY, TextLayout::Edge edge) const {
	checkInitialization();
	const TextLayout& layout = renderer_->lineLayout(position.line);
	Point<> p(layout.location(position.column, edge));
	p.x += getDisplayXOffset(position.line);
	const int y = mapLineToClientY(position.line, fullSearchY);
	if(y == 32767 || y == -32768)
		p.y = y;
	else
		p.y += y;
	return p;
}

/// @see IDefaultFontListener#defaultFontChanged
void TextViewer::defaultFontChanged() /*throw()*/ {
	rulerPainter_->update();
	scrollInfo_.resetBars(*this, SB_BOTH, true);
	updateScrollBars();
	recreateCaret();
	redrawLine(0, true);
}

/// Implementation of @c #beep method. The subclasses can override to customize the behavior.
void TextViewer::doBeep() /*throw()*/ {
	::MessageBeep(MB_OK);
}

/// @see kernel#IDocumentStateListener#documentAccessibleRegionChanged
void TextViewer::documentAccessibleRegionChanged(const k::Document&) {
	if(document().isNarrowed())
		scrollTo(-1, -1, false);
	scheduleRedraw(false);
}

/// @see kernel#IDocumentListener#documentAboutToBeChanged
void TextViewer::documentAboutToBeChanged(const k::Document&) {
	// do nothing
}

/// @see kernel#IDocumentListener#documentChanged
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

/// @see kernel#IDocumentStateListener#documentModificationSignChanged
void TextViewer::documentModificationSignChanged(const k::Document&) {
	// do nothing
}

/// @see ascension#text#IDocumentStateListenerdocumentPropertyChanged
void TextViewer::documentPropertyChanged(const k::Document&, const k::DocumentPropertyKey&) {
	// do nothing
}

/// @see kernel#IDocumentStateListener#documentReadOnlySignChanged
void TextViewer::documentReadOnlySignChanged(const k::Document&) {
	// do nothing
}

/// @see kernel#IDocumentRollbackListener#documentUndoSequenceStarted
void TextViewer::documentUndoSequenceStarted(const k::Document&) {
	freeze();	// TODO: replace with AutoFreeze.
}

/// @see kernel#IDocumentRollbackListener#documentUndoSequenceStopped
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
void TextViewer::drawIndicatorMargin(length_t /* line */, Context& /* context */, const Rect<>& /* rect */) {
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
int TextViewer::getDisplayXOffset(length_t line) const {
	const Rect<> margins(textAreaMargins());
	const TextLayout& layout = renderer_->lineLayout(line);
	const TextAnchor alignment = resolveTextAlignment(layout.anchor(), layout.readingDirection());
	if(alignment == ALIGN_LEFT || alignment == JUSTIFY)	// TODO: this code ignores last visual line with justification.
		return margins.left() - scrollInfo_.x() * renderer_->defaultFont()->metrics().averageCharacterWidth();

	int indent;
	const Rect<> clientBounds(bounds(false));
	if(renderer_->maximumInlineProgressionDimension() + margins.left() + margins.right() > clientBounds.width()) {
		indent = renderer_->maximumInlineProgressionDimension() - layout.lineInlineProgressionDimension(0) + margins.left();
		indent += (clientBounds.width() - margins.left() - margins.right()) % renderer_->defaultFont()->metrics().averageCharacterWidth();
	} else
		indent = clientBounds.width() - layout.lineInlineProgressionDimension(0) - margins.right();
	if(alignment == ALIGN_CENTER)
		indent /= 2;
	else
		assert(alignment == ALIGN_RIGHT);
	return indent - static_cast<long>(scrollInfo_.x()) * renderer_->defaultFont()->metrics().averageCharacterWidth();
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

/// Hides the tool tip.
void TextViewer::hideToolTip() {
	checkInitialization();
	if(tipText_ == 0)
		tipText_ = new Char[1];
	wcscpy(tipText_, L"");
	::KillTimer(identifier().get(), TIMERID_CALLTIP);	// 念のため...
	::SendMessageW(toolTip_, TTM_UPDATE, 0, 0L);
}

/**
 * Determines which part is at the specified position.
 * @param p The position to hit test, in client coordinates
 * @return The result
 * @see TextViewer#HitTestResult
 */
TextViewer::HitTestResult TextViewer::hitTest(const Point<>& p) const {
	checkInitialization();
	const Rect<> clientBounds(bounds(false));
	if(!clientBounds.includes(p))
		return OUT_OF_VIEW;

	const RulerConfiguration& vrc = rulerConfiguration();
	const TextAnchor rulerAlignment = utils::computeRulerAlignment(*this);

	if(vrc.indicatorMargin.visible
			&& ((verticalRulerAlignment == ALIGN_LEFT && p.x < vrc.indicatorMargin.width)
			|| (verticalRulerAlignment == ALIGN_RIGHT && p.x >= clientBounds.right() - vrc.indicatorMargin.width)))
		return INDICATOR_MARGIN;
	else if(vrc.lineNumbers.visible
			&& ((verticalRulerAlignment == ALIGN_LEFT && p.x < rulerPainter_->width())
			|| (verticalRulerAlignment == ALIGN_RIGHT && p.x >= clientBounds.right() - rulerPainter_->width())))
		return LINE_NUMBERS;
	else if((vrc.alignment == ALIGN_LEFT && p.x < rulerPainter_->width() + configuration_.leadingMargin)
			|| (verticalRulerAlignment == ALIGN_RIGHT && p.x >= clientBounds.right() - rulerPainter_->width() - configuration_.leadingMargin))
		return LEADING_MARGIN;
	else if(p.y < textAreaMargins().top())
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
		const Point<>& position /* = Point<>(CW_USEDEFAULT, CW_USEDEFAULT) */,
		const Dimension<>& size /* = Dimension<>(CW_USEDEFAULT, CW_USEDEFAULT) */,
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
				current_.column = 0;
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
				++current_.column;
				update();
			}
		private:
			void update() {
				int temp = beginningIsBlackBack_ ? 0 : 1;
				temp += (current_.position() % 2 == 0) ? 0 : 1;
				auto_ptr<TextRunStyle> style(new TextRunStyle);
				style->foreground = (temp % 2 == 0) ? Color(0xff, 0x00, 0x00) : Color::fromCOLORREF(::GetSysColor(COLOR_WINDOWTEXT));
				style->background = (temp % 2 == 0) ? Color(0xff, 0xcc, 0xcc) : Color::fromCOLORREF(::GetSysColor(COLOR_WINDOW));
				current_.style.reset(style.release());
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
	renderer_->addVisualLinesListener(*this);

	// placement and display
//	setBounds(rect);
	if(visible)
		show();
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
void TextViewer::mapClientYToLine(int y, length_t* logicalLine, length_t* visualSublineOffset, bool* snapped /* = 0 */) const /*throw()*/ {
	if(logicalLine == 0 && visualSublineOffset == 0)
		return;
	const Rect<> margins(textAreaMargins());
	if(snapped != 0) {
		const Rect<> clientBounds(bounds(false));
		*snapped = y < clientBounds.top() + margins.top() || y >= clientBounds.bottom() - margins.bottom();
	}
	y -= margins.top();
	length_t line, subline;
	firstVisibleLine(&line, 0, &subline);
	renderer_->offsetVisualLine(line, subline, y / renderer_->defaultFont()->metrics().linePitch(), (snapped == 0 || *snapped) ? 0 : snapped);
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
 * @retval 32767 @a fullSearch is @c false and @a line is outside of the client area upward
 * @retval -32768 @a fullSearch is @c false and @a line is outside of the client area downward
 * @throw BadPositionException @a line is outside of the document
 * @see #mapClientYToLine, TextRenderer#offsetVisualLine
 */
int TextViewer::mapLineToClientY(length_t line, bool fullSearch) const {
	const Rect<> margins(textAreaMargins());
	if(line == scrollInfo_.firstVisibleLine) {
		if(scrollInfo_.firstVisibleSubline == 0)
			return margins.top();
		else
			return fullSearch ? margins.top()
				- static_cast<int>(renderer_->defaultFont()->metrics().linePitch() * scrollInfo_.firstVisibleSubline) : -32768;
	} else if(line > scrollInfo_.firstVisibleLine) {
		const int lineSpan = renderer_->defaultFont()->metrics().linePitch();
		const Rect<> clientBounds(bounds(false));
		int y = margins.top();
		y += lineSpan * static_cast<int>((renderer_->numberOfSublinesOfLine(scrollInfo_.firstVisibleLine) - scrollInfo_.firstVisibleSubline));
		for(length_t i = scrollInfo_.firstVisibleLine + 1; i < line; ++i) {
			y += lineSpan * static_cast<int>(renderer_->numberOfSublinesOfLine(i));
			if(y >= clientBounds.size().cy && !fullSearch)
				return 32767;
		}
		return y;
	} else if(!fullSearch)
		return -32768;
	else {
		const int linePitch = renderer_->defaultFont()->metrics().linePitch();
		int y = margins.top() - static_cast<int>(linePitch * scrollInfo_.firstVisibleSubline);
		for(length_t i = scrollInfo_.firstVisibleLine - 1; ; --i) {
			y -= static_cast<int>(renderer_->numberOfSublinesOfLine(i) * linePitch);
			if(i == line)
				break;
		}
		return y;
	}
}

/// @see ICaretStateListener#matchBracketsChanged
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

/// @see ICaretStateListener#overtypeModeChanged
void TextViewer::overtypeModeChanged(const Caret&) {
}

/// @see Window#paint
void TextViewer::paint(PaintContext& context) {
	if(isFrozen())	// skip if frozen
		return;
	const Rect<> scheduledBounds(context.boundsToPaint());
	if(scheduledBounds.width() == 0 || scheduledBounds.height() == 0)	// skip if the region to paint is empty
		return;

	const k::Document& doc = document();
	const Rect<> clientBounds(bounds(false));

//	Timer tm(L"onPaint");

	const length_t lines = doc.numberOfLines();
	const int linePitch = renderer_->defaultFont()->metrics().linePitch();

	// paint the ruler
	rulerPainter_->paint(context);

	// draw horizontal margins
	const Rect<> margins(textAreaMargins());
	const Color marginColor((configuration_.background != Color()) ?
		configuration_.background : Color::fromCOLORREF(::GetSysColor(COLOR_WINDOW)));
	if(margins.left() > 0) {
		const int vrWidth = (utils::computeRulerAlignment(*this) == ALIGN_LEFT) ? rulerPainter_->width() : 0;
		context.fillRectangle(
			Rect<>(
				Point<>(clientBounds.left() + vrWidth, scheduledBounds.top()),
				Dimension<>(margins.left() - vrWidth, scheduledBounds.height())),
			marginColor);
	}
	if(margins.right() > 0) {
		const int vrWidth = (utils::computeRulerAlignment(*this) == ALIGN_RIGHT) ? rulerPainter_->width() : 0;
		context.fillRectangle(
			Rect<>(
				Point<>(clientBounds.right() - margins.right(), scheduledBounds.top()),
				Dimension<>(margins.right() - vrWidth, scheduledBounds.height())),
			marginColor);
	}

	// draw lines
	const TextLayout::Selection selection(*caret_,
		configuration_.selectionForeground != Color() ? configuration_.selectionForeground :
			Color::fromCOLORREF(::GetSysColor(hasFocus() ? COLOR_HIGHLIGHTTEXT : COLOR_INACTIVECAPTIONTEXT)),
		configuration_.selectionBackground != Color() ? configuration_.selectionBackground :
			Color::fromCOLORREF(::GetSysColor(hasFocus() ? COLOR_HIGHLIGHT : COLOR_INACTIVECAPTION)));
	Rect<> lineBounds(clientBounds);
	lineBounds.left() += margins.left();
	lineBounds.top() += margins.top();
	lineBounds.right() -= margins.right();
	lineBounds.bottom() -= margins.bottom();
	length_t line, subline;
	mapClientYToLine(scheduledBounds.top(), &line, &subline);
	int y = mapLineToClientY(line, true);
	if(line < lines) {
		while(y < scheduledBounds.bottom() && line < lines) {
			// draw a logical line
			renderer_->renderLine(line, context, getDisplayXOffset(line), y, scheduledBounds, lineBounds, &selection);
			y += linePitch * static_cast<int>(renderer_->numberOfSublinesOfLine(line++));
			subline = 0;
		}
	}

	// paint behind the last
	if(scheduledBounds.bottom() > y && y > margins.top() + linePitch - 1)
		context.fillRectangle(
			Rect<>(
				Point<>(clientBounds.left() + margins.left(), y),
				Dimension<>(clientBounds.width() - margins.left() - margins.right(), scheduledBounds.bottom() - y)),
			marginColor);

	// draw top margin
	if(margins.top() > 0)
		context.fillRectangle(
			Rect<>(
				Point<>(clientBounds.left() + margins.left(), clientBounds.top()),
				Dimension<>(clientBounds.width() - margins.left() - margins.right(), margins.top())),
			marginColor);
}

/// Recreates and shows the caret. If the viewer does not have focus, nothing heppen.
void TextViewer::recreateCaret() {
	if(!hasFocus())
		return;
	::DestroyCaret();
	caretShape_.bitmap.reset();

	Dimension<> solidSize(0, 0);
	if(imeComposingCharacter_)
		solidSize = getCurrentCharacterSize(*this);
	else if(imeCompositionActivated_)
		solidSize.cx = solidSize.cy = 1;
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
		::CreateCaret(identifier().get(), 0, caretShape_.width = solidSize.cx, solidSize.cy);
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
			<< static_cast<ulong>(first) << L".." << static_cast<ulong>(last) << L"]\n";
#endif // _DEBUG

	Rect<> rect(bounds(false));

	// 上端
	rect.top() = max(mapLineToClientY(first, false), configuration_.topMargin);
	if(rect.size().cy <= 0)
		return;
	// 下端
	if(last != numeric_limits<length_t>::max()) {
		long bottom = rect.top() + static_cast<long>(renderer_->numberOfSublinesOfLine(first) * renderer_->defaultFont()->metrics().linePitch());
		for(length_t line = first + 1; line <= last; ++line) {
			bottom += static_cast<long>(renderer_->numberOfSublinesOfLine(line) * renderer_->defaultFont()->metrics().linePitch());
			if(bottom >= rect.bottom())
				break;
		}
		if(bottom < rect.bottom())
			rect.bottom() = bottom;
	}
	scheduleRedraw(rect, false);
}

/// Redraws the ruler.
void TextViewer::repaintRuler() {
	Rect<> r(bounds(false));
	if(rulerPainter_->configuration().alignment == ALIGN_LEFT)
		r.right() = r.left() + rulerPainter_->width();
	else
		r.left() = r.right() - rulerPainter_->width();
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
			renderer_->offsetVisualLine(scrollInfo_.firstVisibleLine, scrollInfo_.firstVisibleSubline, dy);
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
	const Rect<> margins(textAreaMargins());
	Rect<> clientBounds(bounds(false));
	Rect<> clipBounds(clientBounds);
	clipBounds.top() += margins.top();
	clipBounds.bottom() -= margins.bottom();
	if(static_cast<unsigned int>(abs(dy)) >= numberOfVisibleLines())
		scheduleRedraw(clipBounds, false);	// redraw all if the amount of the scroll is over a page
	else if(dx == 0) {	// only vertical
		const RECT temp(toNative(clipBounds));
		::ScrollWindowEx(identifier().get(),
			0, -dy * scrollRate(false) * renderer_->defaultFont()->metrics().linePitch(), 0, &temp, 0, 0, SW_INVALIDATE);
	} else {	// process the leading margin and the edit region independently
		// scroll the edit region
		clipBounds.left() += margins.left();
		clipBounds.right() -= margins.right();
		if(static_cast<unsigned int>(abs(dx)) >= numberOfVisibleColumns())
			scheduleRedraw(clipBounds, false);	// redraw all if the amount of the scroll is over a page
		else
			::ScrollWindowEx(identifier().get(),
				-dx * scrollRate(true) * renderer_->defaultFont()->metrics().averageCharacterWidth(),
				-dy * scrollRate(false) * renderer_->defaultFont()->metrics().linePitch(),
				0, &toNative(clipBounds), 0, 0, SW_INVALIDATE);
		// scroll the vertical ruler
		if(dy != 0) {
			if(rulerPainter_->configuration().alignment == ALIGN_LEFT) {
				clipBounds.left() = clientBounds.left();
				clipBounds.right() = clipBounds.left() + rulerPainter_->width();
			} else {
				clipBounds.right() = clientBounds.right();
				clipBounds.left() = clipBounds.right() - rulerPainter_->width();
			}
			const RECT temp(toNative(clipBounds));
			::ScrollWindowEx(identifier().get(),
				0, -dy * scrollRate(false) * renderer_->defaultFont()->metrics().linePitch(), 0, &temp, 0, 0, SW_INVALIDATE);
		}
	}

	// postprocess
	updateCaretPosition();
	if(redraw)
		redrawScheduledRegion();
	viewportListeners_.notify<bool, bool>(&ViewportListener::viewportChanged, dx != 0, dy != 0);
}

/// @see Widget#resized
void TextViewer::resized(State state, const Dimension<>&) {
	utils::closeCompletionProposalsPopup(*this);
	if(state == MINIMIZED)
		return;

	// notify the tooltip
	win32::AutoZeroSize<TOOLINFOW> ti;
	const Rect<> viewerBounds(bounds(false));
	ti.hwnd = identifier().get();
	ti.uId = 1;
	ti.rect = toNative(viewerBounds);
	::SendMessageW(toolTip_, TTM_NEWTOOLRECT, 0, reinterpret_cast<LPARAM>(&ti));

	if(renderer_.get() == 0)
		return;

	if(configuration_.lineWrap.wrapsAtWindowEdge())
		renderer_->invalidate();
	displaySizeListeners_.notify(&DisplaySizeListener::viewerDisplaySizeChanged);
	scrollInfo_.resetBars(*this, SB_BOTH, true);
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
			visualLine += renderer_->numberOfSublinesOfLine(i);
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
		renderer_->invalidate();

		if((oldReadingDirection == LEFT_TO_RIGHT && configuration_.readingDirection == RIGHT_TO_LEFT)
				|| (oldReadingDirection == RIGHT_TO_LEFT && configuration_.readingDirection == LEFT_TO_RIGHT))
			scrollInfo_.horizontal.position = scrollInfo_.horizontal.maximum
				- scrollInfo_.horizontal.pageSize - scrollInfo_.horizontal.position + 1;
		scrollInfo_.resetBars(*this, SB_BOTH, false);
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
void TextViewer::showToolTip(const String& text, ulong timeToWait /* = -1 */, ulong timeRemainsVisible /* = -1 */) {
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
 * @return The rectangle whose members correspond to each margins
 */
Rect<> TextViewer::textAreaMargins() const /*throw()*/ {
	Rect<> margins(Point<>(0, 0), Dimension<>(0, 0));
	((utils::computeRulerAlignment(*this) == ALIGN_LEFT) ? margins.left() : margins.right()) += rulerPainter_->width();
	const TextAnchor alignment = resolveTextAlignment(
		defaultTextAlignment(presentation()), configuration_.readingDirection);
	if(alignment == ALIGN_LEFT)
		margins.left() += configuration_.leadingMargin;
	else if(alignment == ALIGN_RIGHT)
		margins.right() += configuration_.leadingMargin;
	margins.top() += configuration_.topMargin;
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

	Point<> p(clientXYForCharacter(caret(), false, TextLayout::LEADING));
	const Rect<> margins(textAreaMargins());
	Rect<> textArea(bounds(false));
	textArea.left() += margins.left();
	textArea.top() += margins.top();
	textArea.right() -= margins.right() - 1;
	textArea.bottom() -= margins.bottom();

	if(!textArea.includes(p))
		p.y = -renderer_->defaultFont()->metrics().linePitch();	// "hide" the caret
	else if(caretShape_.readingDirection == RIGHT_TO_LEFT
			|| renderer_->lineLayout(caret().line()).bidiEmbeddingLevel(caret().column()) % 2 == 1)
		p.x -= caretShape_.width;
	::SetCaretPos(p.x, p.y);
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
		scrollInfo_.resetBars(*this, SB_HORZ, false);
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
		scrollInfo_.resetBars(*this, SB_HORZ, false);
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


// TextViewer.RulerConfiguration //////////////////////////////////////////////////////////////////

/// Default constructor.
TextViewer::RulerConfiguration::RulerConfiguration() /*throw()*/ : alignment(ANCHOR_START) {
}


// TextViewer.RulerConfiguration.LineNumbers //////////////////////////////////////////////////////

/// Constructor initializes the all members to their default values.
TextViewer::RulerConfiguration::LineNumbers::LineNumbers() /*throw()*/ : visible(false),
		anchor(ANCHOR_END), startValue(1), minimumDigits(4), leadingMargin(6), trailingMargin(1),
		borderColor(Color()), borderWidth(1), borderStyle(SOLID) {
}


// TextViewer.RulerConfiguration.IndicatorMargin //////////////////////////////////////////////////

/// Constructor initializes the all members to their default values.
TextViewer::RulerConfiguration::IndicatorMargin::IndicatorMargin() /*throw()*/ : visible(false), width(15) {
}


// TextViewer.Renderer ////////////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param viewer the text viewer
 */
TextViewer::Renderer::Renderer(TextViewer& viewer) :
		TextRenderer(viewer.presentation(), systemFonts(), true), viewer_(viewer) {
	// TODO: other IFontCollection object used?
#if 0
	// for test
	setSpecialCharacterRenderer(new DefaultSpecialCharacterRenderer, true);
#endif
}

/**
 * Copy-constructor with a parameter.
 * @param other the source object
 * @param viewer the text viewer
 */
TextViewer::Renderer::Renderer(const Renderer& other, TextViewer& viewer) : TextRenderer(other), viewer_(viewer),
		overrideReadingDirection_(other.overrideReadingDirection_), overrideTextAnchor_(other.overrideTextAnchor_) {
}

/**
 * Implements @c layout#ILayoutInformationProvider#defaultUIReadingDirection abstract method.
 * This returns the value of @c TextViewer#Configuration#readingDirection.
 */
Inheritable<ReadingDirection> TextViewer::Renderer::defaultUIReadingDirection() const /*throw()*/ {
	return viewer_.configuration().readingDirection;
}

/// @see layout#ILayoutInformationProvider#layoutSettings
const LayoutSettings& TextViewer::Renderer::layoutSettings() const /*throw()*/ {
	return viewer_.configuration();
}

/// @see layout#LineLayoutBuffer#renderingContext
auto_ptr<Context> TextViewer::Renderer::renderingContext() const {
	return viewer_.isWindow() ? viewer_.createGraphicContext() : Screen::instance().createGraphicContext();
}

/// Rewraps the visual lines at the window's edge.
void TextViewer::Renderer::rewrapAtWindowEdge() {
	if(viewer_.configuration().lineWrap.wrapsAtWindowEdge()) {
		const Rect<> clientBounds(viewer_.bounds(false));
		const Rect<> margins(viewer_.textAreaMargins());
		const int newWidth = clientBounds.width() - margins.left() - margins.right();
		for(Iterator i(firstCachedLine()), e(lastCachedLine()); i != e; ) {
			const TextLayout& layout = **i;
			++i;	// invalidate() may break iterator
			if(layout.numberOfLines() != 1
					|| layout.style().anchor == JUSTIFY || layout.maximumInlineProgressionDimension() > newWidth)
//				layout.rewrap();
				invalidate(layout.lineNumber(), layout.lineNumber() + 1);
		}
	}
}

/// @see layout#ILayoutInformationProvider#width
int TextViewer::Renderer::width() const /*throw()*/ {
	const LineWrapConfiguration& lwc = viewer_.configuration().lineWrap;
	if(!lwc.wraps()) {
		win32::AutoZeroSize<SCROLLINFO> si;
		si.fMask = SIF_RANGE;
		viewer_.scrollInformation(SB_HORZ, si);
		return (si.nMax + 1) * viewer_.textRenderer().defaultFont()->metrics().averageCharacterWidth();
	} else if(lwc.wrapsAtWindowEdge()) {
		const Rect<> clientBounds(viewer_.bounds(false));
		const Rect<> margins(viewer_.textAreaMargins());
		return clientBounds.width() - margins.left() - margins.right();
	} else
		return lwc.width;
}


// TextViewer.RulerPainter ////////////////////////////////////////////////////////////////////////

// some methods are defined in layout.cpp

/**
 * Constructor.
 * @param viewer the viewer
 * @param enableDoubleBuffering set @c true to use double-buffering for non-flicker drawing
 */
TextViewer::RulerPainter::RulerPainter(TextViewer& viewer, bool enableDoubleBuffering)
		: viewer_(viewer), width_(0), lineNumberDigitsCache_(0), enablesDoubleBuffering_(enableDoubleBuffering) {
	recalculateWidth();
}

/// Returns the maximum number of digits of line numbers.
uint8_t TextViewer::RulerPainter::maximumDigisForLineNumbers() const /*throw()*/ {
	uint8_t n = 1;
	length_t lines = viewer_.document().numberOfLines() + configuration_.lineNumbers.startValue - 1;
	while(lines >= 10) {
		lines /= 10;
		++n;
	}
	return static_cast<uchar>(n);	// hmm...
}

void TextViewer::RulerPainter::setConfiguration(const RulerConfiguration& configuration) {
	if(configuration.alignment != ALIGN_START && configuration.alignment != ALIGN_END
			&& configuration.alignment != ALIGN_LEFT && configuration.alignment != ALIGN_RIGHT)
		throw UnknownValueException("verticalRuler->alignment");
	if(configuration.lineNumbers.alignment != ALIGN_START && configuration.lineNumbers.alignment != ALIGN_END
			&& configuration.lineNumbers.alignment != ALIGN_LEFT && configuration.lineNumbers.alignment != ALIGN_RIGHT
			&& configuration.lineNumbers.alignment != ALIGN_CENTER && configuration.lineNumbers.alignment != JUSTIFY)
		throw UnknownValueException("verticalRuler->lineNumber.alignment");
	if(configuration.lineNumbers.readingDirection != LEFT_TO_RIGHT
			&& configuration.lineNumbers.readingDirection != RIGHT_TO_LEFT
			&& configuration.lineNumbers.readingDirection != INHERIT_READING_DIRECTION)
		throw UnknownValueException("verticalRuler->lineNumber.readingDirection");
	configuration_ = configuration;
	update();
}

void TextViewer::RulerPainter::update() /*throw()*/ {
	lineNumberDigitsCache_ = 0;
	recalculateWidth();
	updateGDIObjects();
	if(enablesDoubleBuffering_ && memoryBitmap_.get() != 0)
		memoryBitmap_.reset();
}

#undef RESTORE_HIDDEN_CURSOR


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

void TextViewer::ScrollInfo::resetBars(const TextViewer& viewer, int bars, bool pageSizeChanged) /*throw()*/ {
	// about horizontal
	if(bars == SB_HORZ || bars == SB_BOTH) {
		// テキストが左揃えでない場合は、スクロールボックスの位置を補正する必要がある
		// (ウィンドウが常に LTR である仕様のため)
	//	const TextAlignment alignment = resolveTextAlignment(viewer.configuration().alignment, viewer.configuration().readingDirection);
		const int dx = viewer.textRenderer().textMetrics().averageCharacterWidth();
		assert(dx > 0);
		const ulong columns = (!viewer.configuration().lineWrap.wrapsAtWindowEdge()) ? viewer.textRenderer().maximumInlineProgressionDimension() / dx : 0;
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
	if(bars == SB_VERT || bars == SB_BOTH) {
		const length_t lines = viewer.textRenderer().numberOfVisualLines();
		assert(lines > 0);
//		vertical.rate = static_cast<ulong>(lines) / numeric_limits<int>::max() + 1;
//		assert(vertical.rate != 0);
		vertical.maximum = max(static_cast<int>((lines - 1)/* / vertical.rate*/), 0/*static_cast<int>(viewer.numberOfVisibleLines() - 1)*/);
		if(pageSizeChanged)
			vertical.pageSize = static_cast<UINT>(viewer.numberOfVisibleLines());
	}
}

void TextViewer::ScrollInfo::updateVertical(const TextViewer& viewer) /*throw()*/ {
	vertical.maximum = static_cast<int>(viewer.textRenderer().numberOfVisualLines());
	firstVisibleLine = min(firstVisibleLine, viewer.document().numberOfLines() - 1);
	firstVisibleSubline = min(viewer.textRenderer().numberOfSublinesOfLine(firstVisibleLine) - 1, firstVisibleSubline);
	vertical.position = static_cast<int>(viewer.textRenderer().mapLogicalLineToVisualLine(firstVisibleLine) + firstVisibleSubline);
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
bool VirtualBox::isPointOver(const graphics::Point<>& p) const /*throw()*/ {
	assert(view_.isWindow());
	if(view_.hitTest(p) != TextViewer::TEXT_AREA)	// ignore if not in text area
		return false;
	const int leftMargin = view_.textAreaMargins().left();
	if(p.x < left() + leftMargin || p.x >= right() + leftMargin)	// about x-coordinate
		return false;

	// about y-coordinate
	const Point& top = beginning();
	const Point& bottom = end();
	length_t line, subline;
	view_.mapClientYToLine(p.y, &line, &subline);	// $friendly-access
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
		const TextLayout& layout = renderer.lineLayout(line);
		range = Range<length_t>(
			layout.offset(points_[0].x - renderer.lineIndent(line, 0),
				static_cast<Scalar>(renderer.defaultFont()->metrics().linePitch() * subline)).first,
			layout.offset(points_[1].x - renderer.lineIndent(line, 0),
				static_cast<Scalar>(renderer.defaultFont()->metrics().linePitch() * subline)).first);
		return !range.isEmpty();
	}
}

/**
 * Updates the rectangle of the virtual box.
 * @param region the region consists the rectangle
 */
void VirtualBox::update(const k::Region& region) /*throw()*/ {
	const TextRenderer& r = view_.textRenderer();
	const TextLayout* layout = &r.lineLayout(points_[0].line = region.first.line);
	graphics::Point<> location(layout->location(region.first.column));
	points_[0].x = location.x + r.lineIndent(points_[0].line, 0);
	points_[0].subline = location.y / r.defaultFont()->metrics().linePitch();
	layout = &r.lineLayout(points_[1].line = region.second.line);
	location = layout->location(region.second.column);
	points_[1].x = location.x + r.lineIndent(points_[1].line, 0);
	points_[1].subline = location.y / r.defaultFont()->metrics().linePitch();
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
void DefaultCaretShaper::shape(win32::Handle<HBITMAP>&, Dimension<>& solidSize, ReadingDirection& readingDirection) /*throw()*/ {
	DWORD width;
	if(::SystemParametersInfo(SPI_GETCARETWIDTH, 0, &width, 0) == 0)
		width = 1;	// NT4 does not support SPI_GETCARETWIDTH
	solidSize.cx = width;
	solidSize.cy = viewer_->textRenderer().textMetrics().cellHeight();
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
	inline BITMAPINFO* prepareCaretBitmap(const win32::Handle<HDC>& dc, ushort width, ushort height) {
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
	inline win32::Handle<HBITMAP> createSolidCaretBitmap(ushort width, ushort height, const RGBQUAD& color) {
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
	inline win32::Handle<HBITMAP> createRTLCaretBitmap(ushort height, bool bold, const RGBQUAD& color) {
		win32::Handle<HDC> dc(detail::screenDC());
		const RGBQUAD white = {0x00, 0x00, 0x00, 0x00};
		BITMAPINFO* info = prepareCaretBitmap(dc, 5, height);
		assert(height > 3);
		uninitialized_fill(info->bmiColors, info->bmiColors + 5 * height, white);
		info->bmiColors[0] = info->bmiColors[1] = info->bmiColors[2]
			= info->bmiColors[6] = info->bmiColors[7] = info->bmiColors[12] = color;
		for(ushort i = 0; i < height; ++i) {
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
	inline win32::Handle<HBITMAP> createTISCaretBitmap(ushort height, bool bold, const RGBQUAD& color) {
		win32::Handle<HDC> dc(detail::screenDC());
		const RGBQUAD white = {0x00, 0x00, 0x00, 0x00};
		const ushort width = max<ushort>(height / 8, 3);
		BITMAPINFO* info = prepareCaretBitmap(dc, width, height);
		assert(height > 3);
		uninitialized_fill(info->bmiColors, info->bmiColors + width * height, white);
		for(ushort y = 0; y < height - 1; ++y) {
			info->bmiColors[y * width] = color;
			if(bold) info->bmiColors[y * width + 1] = color;
		}
		if(bold)
			for(ushort x = 2; x < width; ++x)
				info->bmiColors[width * (height - 2) + x] = color;
		for(ushort x = 0; x < width; ++x)
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
		win32::Handle<HBITMAP>& bitmap, Dimension<>& solidSize, ReadingDirection& readingDirection) /*throw()*/ {
	const Caret& caret = updater_->textViewer().caret();
	const bool overtype = caret.isOvertypeMode() && isSelectionEmpty(caret);

	if(!overtype) {
		solidSize.cx = bold_ ? 2 : 1;	// this ignores the system setting...
		solidSize.cy = updater_->textViewer().textRenderer().defaultFont()->metrics().cellHeight();
	} else	// use the width of the glyph when overtype mode
		solidSize = getCurrentCharacterSize(updater_->textViewer());
	readingDirection = LEFT_TO_RIGHT;

	HIMC imc = ::ImmGetContext(updater_->textViewer().identifier().get());
	const bool imeOpened = win32::boole(::ImmGetOpenStatus(imc));
	::ImmReleaseContext(updater_->textViewer().identifier().get(), imc);
	if(imeOpened) {	// CJK and IME is open
		static const RGBQUAD red = {0xff, 0xff, 0x80, 0x00};
		bitmap = createSolidCaretBitmap(static_cast<uint16_t>(solidSize.cx), static_cast<uint16_t>(solidSize.cy), red);
	} else if(!overtype && solidSize.cy > 3) {
		static const RGBQUAD black = {0xff, 0xff, 0xff, 0x00};
		const WORD langID = PRIMARYLANGID(LOWORD(::GetKeyboardLayout(::GetCurrentThreadId())));
		if(isRTLLanguage(langID)) {	// RTL
			bitmap = createRTLCaretBitmap(static_cast<uint16_t>(solidSize.cy), bold_, black);
			readingDirection = RIGHT_TO_LEFT;
		} else if(isTISLanguage(langID)) {	// Thai relations
			bitmap = createTISCaretBitmap(static_cast<uint16_t>(solidSize.cy), bold_, black);
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
const ILineColorDirector::Priority CurrentLineHighlighter::LINE_COLOR_PRIORITY = 0x40;

/**
 * Constructor.
 * @param caret The caret
 * @param foreground The initial foreground color
 * @param background The initial background color
 */
CurrentLineHighlighter::CurrentLineHighlighter(Caret& caret,
		const Color& foreground, const Color& background) : caret_(&caret), foreground_(foreground), background_(background) {
	tr1::shared_ptr<ILineColorDirector> temp(this);
	caret.textViewer().presentation().addLineColorDirector(temp);
	caret.addListener(*this);
	caret.addStateListener(*this);
	caret.addLifeCycleListener(*this);
}

/// Destructor.
CurrentLineHighlighter::~CurrentLineHighlighter() /*throw()*/ {
	if(caret_ != 0) {
		caret_->removeListener(*this);
		caret_->removeStateListener(*this);
		caret_->textViewer().presentation().removeLineColorDirector(*this);
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
ILineColorDirector::Priority CurrentLineHighlighter::queryLineColors(length_t line, Color& foreground, Color& background) const {
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
		if(contentassist::ContentAssistant::CompletionProposalsUI* cpui = ca->getCompletionProposalsUI())
			cpui->close();
	}
}

/**
 * Computes the alignment of the vertical ruler.
 * @param viewer the text viewer
 * @return the alignment of the vertical ruler. @c ALIGN_LEFT or @c ALIGN_RIGHT
 */
TextAnchor utils::computeRulerAlignment(const TextViewer& viewer) {
	TextAlignment alignment = viewer.rulerConfiguration().alignment;
	switch(alignment) {
		case ALIGN_LEFT:
		case ALIGN_RIGHT:
			return alignment;
		case ALIGN_START:
		case ALIGN_END: {
			ReadingDirection readingDirection = INHERIT_READING_DIRECTION;
			tr1::shared_ptr<const LineStyle> defaultLineStyle(viewer.presentation().defaultLineStyle());
			if(defaultLineStyle.get() != 0)
				readingDirection = defaultLineStyle->readingDirection;
			if(readingDirection == INHERIT_READING_DIRECTION)
				readingDirection = viewer.textRenderer().defaultUIReadingDirection();
			if(readingDirection == INHERIT_READING_DIRECTION)
				readingDirection = ASCENSION_DEFAULT_TEXT_READING_DIRECTION;
			if(readingDirection != INHERIT_READING_DIRECTION)
				return resolveTextAlignment(alignment, readingDirection);
		}
	}
	throw UnknownValueException("viewer");
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
		POINT cursorPoint;
		::GetCursorPos(&cursorPoint);
		::ScreenToClient(viewer.handle().get(), &cursorPoint);
		const k::Position cursor = viewer.characterForClientXY(fromNative(cursorPoint), LineLayout::LEADING);
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
