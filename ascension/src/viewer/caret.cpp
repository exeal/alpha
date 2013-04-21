/**
 * @file caret.cpp
 * @author exeal
 * @date 2003-2008 was point.cpp
 * @date 2008-2010 separated from point.cpp
 * @date 2010-2013
 */

#include <ascension/corelib/text/break-iterator.hpp>
#include <ascension/corelib/text/character-property.hpp>
#include <ascension/corelib/text/utf.hpp>
#include <ascension/graphics/font/font-metrics.hpp>
#include <ascension/graphics/image.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/kernel/document-character-iterator.hpp>
#include <ascension/text-editor/input-sequence-checker.hpp>
#include <ascension/text-editor/session.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/default-caret-shaper.hpp>
#include <ascension/viewer/viewer.hpp>

using namespace ascension;
using namespace ascension::graphics;
using namespace ascension::viewers;
using namespace ascension::presentation;
using namespace ascension::text;
using namespace ascension::text::ucd;
using namespace std;
namespace k = ascension::kernel;


namespace {
	// copied from point.cpp
	inline const IdentifierSyntax& identifierSyntax(const k::Point& p) {
		return p.document().contentTypeInformation().getIdentifierSyntax(contentType(p));
	}
} // namespace @0


// Caret //////////////////////////////////////////////////////////////////////////////////////////

/// @internal Default constructor.
Caret::Shape::Shape() BOOST_NOEXCEPT : alignmentPoint() {
	// oh, this does nothing
}

/// @internal Default constructor.
Caret::Context::Context() BOOST_NOEXCEPT : yanking(false), leaveAnchorNext(false), leadingAnchor(false),
		typing(false), inputMethodCompositionActivated(false), inputMethodComposingCharacter(false) {
}

// TODO: rewrite this documentation.

/**
 * @class ascension::viewers::Caret
 *
 * @c Caret is an extension of @c VisualPoint. A caret has a selection on the text viewer. And
 * supports line selection, word selection, rectangle (box) selection, tracking match brackets, and
 * clipboard enhancement.
 *
 * A caret has one another point called "anchor" (or "mark"). The selection is a region between the
 * caret and the anchor. Anchor is @c VisualPoint but client can't operate this directly.
 *
 * Usually, the anchor will move adapting to the caret automatically. If you want to move the
 * anchor isolately, create the selection by using @c #select method or call @c #extendSelectionTo
 * method.
 *
 * When the caret moves, the text viewer will scroll automatically to show the caret. See the
 * description of @c #enableAutoShow and @c #isAutoShowEnabled.
 *
 * @c Caret hides @c Point#excludeFromRestriction and can't enter the inaccessible region of the
 * document. @c #isExcludedFromRestriction always returns @c true.
 *
 * @c Caret throws @c ReadOnlyDocumentException when tried to change the read-only document.
 *
 * このクラスの編集用のメソッドは @c EditPoint 、@c VisualPoint の編集用メソッドと異なり、
 * 積極的に連続編集とビューの凍結を使用する
 *
 * 行選択および単語選択は、選択の作成および拡張時にアンカーとキャレットを行境界や単語境界に束縛する機能で、
 * @c #extendSelectionTo メソッドで実際にこれらの点が移動する位置を制限する。
 * また、この場合 @c #extendSelectionTo を呼び出すとアンカーが自動的に移動する。
 * @c #beginLineSelection 、@c #beginWordSelection でこれらのモードに入ることができ、
 * @c #restoreSelectionMode で通常状態に戻ることができる。
 * また、これらのモードで @c #moveTo か @c #select を使っても通常状態に戻る
 *
 * 対括弧の検索はプログラムを編集しているときに役立つ機能で、キャレット位置に括弧があれば対応する括弧を検索する。
 * 括弧のペアを強調表示するのは、現時点ではビューの責任である
 *
 * To enter rectangle selection mode, call @c #beginRectangleSelection method. To exit, call
 * @c #endRectangleSelection method. You can get the information of the current rectangle selection
 * by using @c #boxForRectangleSelection method.
 *
 * This class does not accept @c IPointListener. Use @c ICaretListener interface instead.
 *
 * @note This class is not intended to subclass.
 */

/**
 * Constructor.
 * @param viewer The text viewer
 * @param position The initial position of the point
 * @throw BadPositionException @a position is outside of the document
 */
Caret::Caret(TextViewer& viewer, const k::Position& position /* = k::Position(0, 0) */) BOOST_NOEXCEPT : VisualPoint(viewer, position, nullptr),
		anchor_(new SelectionAnchor(viewer, position)),
#ifdef ASCENSION_OS_WINDOWS
		clipboardLocale_(::GetUserDefaultLCID()),
#endif // ASCENSION_OS_WINDOWS
		overtypeMode_(false), autoShow_(true), matchBracketsTrackingMode_(DONT_TRACK) {
	document().addListener(*this);
	textViewer().addDisplaySizeListener(*this);
	textViewer().addViewportListener(*this);
}

/// Destructor.
Caret::~Caret() BOOST_NOEXCEPT {
	if(!isDocumentDisposed())
		document().removeListener(*this);
	if(!isTextViewerDisposed()) {
		textViewer().removeDisplaySizeListener(*this);
		textViewer().removeViewportListener(*this);
	}
}

/// @see VisualPoint#aboutToMove
void Caret::aboutToMove(k::Position& to) {
	if(k::positions::isOutsideOfDocumentRegion(document(), to))
		throw k::BadPositionException(to, "Caret tried to move outside of document.");
	VisualPoint::aboutToMove(to);
}

/**
 * Registers the character input listener.
 * @param listener The listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
void Caret::addCharacterInputListener(CharacterInputListener& listener) {
	characterInputListeners_.add(listener);
}

/**
 * Registers the input property listener.
 * @param listener The listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
void Caret::addInputPropertyListener(InputPropertyListener& listener) {
	inputPropertyListeners_.add(listener);
}

/**
 * Registers the listener.
 * @param listener The listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
void Caret::addListener(CaretListener& listener) {
	listeners_.add(listener);
}

/**
 * Registers the state listener.
 * @param listener The listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
void Caret::addStateListener(CaretStateListener& listener) {
	stateListeners_.add(listener);
}

/**
 * Starts rectangular selection.
 * @see #endRectangleSelection, #isSelectionRectangle
 */
void Caret::beginRectangleSelection() {
	if(context_.selectedRectangle.get() == nullptr) {
		context_.selectedRectangle.reset(new VirtualBox(textViewer(), selectedRegion()));
		stateListeners_.notify<const Caret&>(&CaretStateListener::selectionShapeChanged, *this);
	}
}

/**
 * Returns @c true if a paste operation can be performed.
 * @note Even when this method returned @c true, the following @c #paste call can fail.
 * @param useKillRing Set @c true to get the content from the kill-ring of the session, not from
 *                    the system clipboard
 * @return true if the clipboard data is pastable
 */
bool Caret::canPaste(bool useKillRing) const {
	if(!useKillRing) {
		const UINT rectangleClipFormat = ::RegisterClipboardFormatW(ASCENSION_RECTANGLE_TEXT_MIME_FORMAT);
		if(rectangleClipFormat != 0 && win32::boole(::IsClipboardFormatAvailable(rectangleClipFormat)))
			return true;
		else if(win32::boole(::IsClipboardFormatAvailable(CF_UNICODETEXT)) || win32::boole(::IsClipboardFormatAvailable(CF_TEXT)))
			return true;
	} else {
		if(const texteditor::Session* const session = document().session())
			return session->killRing().numberOfKills() != 0;
	}
	return false;
}

/// 対括弧の追跡を更新する
void Caret::checkMatchBrackets() {
//	bool matched;
	boost::optional<pair<k::Position, k::Position>> oldPair(context_.matchBrackets);
	// TODO: implement matching brackets checking
/*	if(!isSelectionEmpty() || matchBracketsTrackingMode_ == DONT_TRACK)
		matched = false;
	else if(matched = getViewer().searchMatchBracket(getPosition(), matchBrackets_.first, true))
		matchBrackets_.second = getPosition();
	else if(matchBracketsTrackingMode_ == TRACK_FOR_SURROUND_CHARACTERS && !isStartOfLine()) {	// 1文字前も調べる
		const String& line = document()->getLine(lineNumber());
		GraphemeBreakIterator i(line.data(), line.data() + line.length(), line.data() + columnNumber());
		if(matched = getViewer().searchMatchBracket(Position(lineNumber(), (--i).tell() - line.data()), matchBrackets_.first, true))
			matchBrackets_.second = Position(lineNumber(), i.tell() - line.data());
	}
	if(!matched)
		matchBrackets_.first = matchBrackets_.second = Position::INVALID_POSITION;
*/	// TODO: check if the pair is out of view.
	if(context_.matchBrackets != oldPair)
		stateListeners_.notify<const Caret&, const boost::optional<pair<k::Position,
			k::Position>>&, bool>(&CaretStateListener::matchBracketsChanged, *this, oldPair, false);
}

/// Clears the selection. The anchor will move to the caret.
void Caret::clearSelection() {
	endRectangleSelection();
	context_.leaveAnchorNext = false;
	moveTo(*this);
}

/// @see kernel#DocumentListener#documentAboutToBeChanged
void Caret::documentAboutToBeChanged(const k::Document&) {
	// does nothing
}

/// @see kernel#DocumentListener#documentChanged
void Caret::documentChanged(const k::Document&, const k::DocumentChange&) {
	context_.yanking = false;
	if(context_.regionBeforeMoved)
		updateVisualAttributes();
}

/**
 * Ends the rectangular selection.
 * @see #beginRectangleSelection, #isSelectionRectangle
 */
void Caret::endRectangleSelection() {
	if(isTextViewerDisposed())
		throw TextViewerDisposedException();
	if(context_.selectedRectangle.get() != nullptr) {
		context_.selectedRectangle.reset();
		stateListeners_.notify<const Caret&>(&CaretStateListener::selectionShapeChanged, *this);
	}
}
#if 0
/**
 * Deletes the selected text. This method ends the rectangle selection mode.
 * @throw DocumentAccessViolationException The selected region intersects the inaccesible region
 * @throw ... Any exceptions @c Document#erase throws
 */
void Caret::eraseSelection() {
	verifyViewer();
	Document& doc = *document();
	if(isSelectionEmpty())
		return;
	else if(!isSelectionRectangle()) {	// the selection is linear
		const Position to(min<Position>(*anchor_, *this));
		doc.erase(*anchor_, *this);	// this can throw all exceptions this method throws
		moveTo(to);
	} else {	// the selection is rectangle
		DocumentLocker lock(*document());
		const Position resultPosition(beginning());
		const bool adapts = adaptsToDocument();
		adaptToDocument(false);
		const Index firstLine = beginning().lineNumber(), lastLine = end().lineNumber();
		pair<Index, Index> rangeInLine;
		bool interrupted = false;

		// rectangle deletion can't delete newline characters
		{
			AutoFreeze af(&textViewer());
			if(textViewer().configuration().lineWrap.wraps()) {	// ...and the lines are wrapped
				// hmmm..., this is heavy work
				vector<Point*> points;
				vector<Index> sizes;
				points.reserve((lastLine - firstLine) * 2);
				sizes.reserve((lastLine - firstLine) * 2);
				const TextRenderer& renderer = textViewer().textRenderer();
				for(Index line = resultPosition.line; line <= lastLine; ++line) {
					const LineLayout& layout = renderer.lineLayout(line);
					for(Index subline = 0; subline < layout.numberOfSublines(); ++subline) {
						box_->overlappedSubline(line, subline, rangeInLine.first, rangeInLine.second);
						points.push_back(new Point(doc, Position(line, rangeInLine.first)));
						sizes.push_back(rangeInLine.second - rangeInLine.first);
					}
				}
				const size_t sublines = points.size();
				for(size_t i = 0; i < sublines; ++i) {
					if(!interrupted) {
						try {
							doc.erase(Position(points[i]->lineNumber(), points[i]->columnNumber()),
								Position(points[i]->lineNumber(), points[i]->columnNumber() + sizes[i]));
						} catch(...) {
							if(i == 0) {
								while(i < sublines)
									delete points[i++];
								adaptToDocument(adapts);
								throw;
							}
						}
					}
					delete points[i];
				}
			} else {
				for(Index line = resultPosition.line; line <= lastLine; ++line) {
					box_->overlappedSubline(line, 0, rangeInLine.first, rangeInLine.second);
					try {
						doc.erase(Position(line, rangeInLine.first), Position(line, rangeInLine.second));
					} catch(...) {
						if(line == resultPosition.line) {
							adaptToDocument(adapts);
							throw;
						}
					}
				}
			}
		}
		adaptToDocument(adapts);
		endRectangleSelection();
		moveTo(resultPosition);
	}
}
#endif
/**
 * Moves to the specified position without the anchor adapting.
 * @param to The destination position
 */
void Caret::extendSelectionTo(const k::Position& to) {
	context_.leaveAnchorNext = true;
	try {
		moveTo(to);
	} catch(...) {
		context_.leaveAnchorNext = false;
		throw;
	}
	context_.leaveAnchorNext = false;
}

/**
 * Moves to the specified position without the anchor adapting.
 * @param to The destination position
 */
void Caret::extendSelectionTo(const VisualDestinationProxy& to) {
	context_.leaveAnchorNext = true;
	try {
		moveTo(to);
	} catch(...) {
		context_.leaveAnchorNext = false;
		throw;
	}
	context_.leaveAnchorNext = false;
}

inline void Caret::fireCaretMoved(const k::Region& oldRegion) {
	if(!isTextViewerDisposed() && !textViewer().isFrozen() && (widgetapi::hasFocus(textViewer()) /*|| widgetapi::hasFocus(*completionWindow_)*/))
		updateLocation();
	listeners_.notify<const Caret&, const k::Region&>(&CaretListener::caretMoved, *this, oldRegion);
}

namespace {
	/**
	 * @internal Deletes the forward one character and inserts the specified text.
	 * This function emulates keyboard overtyping input.
	 * @param caret The caret
	 * @param text The text to insert
	 * @param keepNewline Set @c false to overwrite a newline characer
	 * @throw NullPointerException @a first is @c null
	 * @throw DocumentDisposedException
	 * @throw TextViewerDisposedException
	 * @throw ... Any exceptions @c Document#replace throws
	 */
	void destructiveInsert(Caret& caret, const StringPiece& text, bool keepNewline = true) {
		if(text.begin() == nullptr)
			throw NullPointerException("text");
		const bool adapts = caret.adaptsToDocument();
		caret.adaptToDocument(false);
		k::Position e((keepNewline && k::locations::isEndOfLine(caret)) ?
			caret.position() : k::locations::nextCharacter(caret, Direction::FORWARD, k::locations::GRAPHEME_CLUSTER));
		if(e != caret.position()) {
			try {
				caret.document().replace(k::Region(caret.position(), e), text, &e);
			} catch(...) {
				caret.adaptToDocument(adapts);
				throw;
			}
			caret.moveTo(e);
		}
		caret.adaptToDocument(adapts);
	}
} // namespace @0

/**
 * Inputs the specified character at current position.
 * <p>If the selection is not empty, replaces the selected region. Otherwise if in overtype mode,
 * replaces a character at current position (but this does not erase newline character).</p>
 * <p>This method may insert undo boundaries for compound typing.</p>
 * @param character The code point of the character to input
 * @param validateSequence Set @c true to perform input sequence check using the active ISC. See
 *                         @c texteditor#InputSequenceCheckers
 * @param blockControls Set @c true to refuse any ASCII control characters except HT (U+0009),
                        RS (U+001E) and US (U+001F)
 * @retval true Succeeded
 * @retval false The input was rejected by the input sequence validation (when @a validateSequence
 *               was @c true)
 * @return false @c character was control character and blocked (when @a blockControls was @c true)
 * @throw ... Any exceptions @c Document#insertUndoBoundary and @c Caret#replaceSelection throw
 * @see #isOvertypeMode, #setOvertypeMode, texteditor#commands#TextInputCommand
 */
bool Caret::inputCharacter(CodePoint character, bool validateSequence /* = true */, bool blockControls /* = true */) {
	// check blockable control character
	static const CodePoint SAFE_CONTROLS[] = {0x0009u, 0x001eu, 0x001fu};
	if(blockControls && character <= 0x00ffu
			&& (iscntrl(static_cast<int>(character)) != 0)
			&& !binary_search(SAFE_CONTROLS, ASCENSION_ENDOF(SAFE_CONTROLS), character))
		return false;

	// check the input sequence
	k::Document& doc = document();
	if(validateSequence) {
		if(const texteditor::Session* const session = doc.session()) {
			if(const texteditor::InputSequenceCheckers* const checker = session->inputSequenceCheckers()) {
				const Char* const lineString = doc.line(line(beginning())).data();
				if(!checker->check(StringPiece(lineString, offsetInLine(beginning())), character)) {
					eraseSelection(*this);
					return false;	// invalid sequence
				}
			}
		}
	}

	Char buffer[2];
	{
		Char* out = buffer;
		utf::checkedEncode(character, out);
	}
	if(!isSelectionEmpty(*this)) {	// just replace if the selection is not empty
		doc.insertUndoBoundary();
		replaceSelection(StringPiece(buffer, (character < 0x10000u) ? 1 : 2));
		doc.insertUndoBoundary();
	} else if(overtypeMode_) {
		prechangeDocument();
		doc.insertUndoBoundary();
		destructiveInsert(*this, StringPiece(buffer, (character < 0x10000u) ? 1 : 2));
		doc.insertUndoBoundary();
	} else {
		const bool alpha = identifierSyntax(*this).isIdentifierContinueCharacter(character);
		if(context_.lastTypedPosition && (!alpha || context_.lastTypedPosition != position())) {
			// end sequential typing
			doc.insertUndoBoundary();
			context_.lastTypedPosition = boost::none;
		}
		if(alpha && !context_.lastTypedPosition)
			// (re)start sequential typing
			doc.insertUndoBoundary();

		detail::ValueSaver<bool> lock(context_.typing);
		context_.typing = true;
		replaceSelection(StringPiece(buffer, (character < 0x10000u) ? 1 : 2));	// this may throw
		if(alpha)
			context_.lastTypedPosition = position();
	}

	characterInputListeners_.notify<const Caret&, CodePoint>(
		&CharacterInputListener::characterInput, *this, character);
	return true;
}

/// @see VisualPoint#moved
void Caret::moved(const k::Position& from) {
	context_.regionBeforeMoved = boost::make_optional(k::Region(
		anchor_->isInternalUpdating() ? anchor_->positionBeforeInternalUpdate() : anchor_->position(), from));
	if(context_.leaveAnchorNext)
		context_.leaveAnchorNext = false;
	else {
		context_.leadingAnchor = true;
		anchor_->moveTo(position());
		context_.leadingAnchor = false;
	}
	VisualPoint::moved(from);
	if(!document().isChanging())
		updateVisualAttributes();
}

/// @see PointListener#pointMoved
void Caret::pointMoved(const Point& self, const k::Position& oldPosition) {
	assert(&self == &*anchor_);
	context_.yanking = false;
	if(context_.leadingAnchor)	// calling anchor_->moveTo in this->moved
		return;
	if((oldPosition == position()) != isSelectionEmpty(*this))
		checkMatchBrackets();
	fireCaretMoved(k::Region(oldPosition, position()));
}

/// @internal Should be called before change the document.
inline void Caret::prechangeDocument() {
	if(context_.lastTypedPosition && !context_.typing) {
		document().insertUndoBoundary();
		context_.lastTypedPosition = boost::none;
	}
}

/**
 * Removes the character input listener
 * @param listener The listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
void Caret::removeCharacterInputListener(CharacterInputListener& listener) {
	characterInputListeners_.remove(listener);
}

/**
 * Removes the input property listener
 * @param listener The listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
void Caret::removeInputPropertyListener(InputPropertyListener& listener) {
	inputPropertyListeners_.remove(listener);
}

/**
 * Removes the listener.
 * @param listener The listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
void Caret::removeListener(CaretListener& listener) {
	listeners_.remove(listener);
}

/**
 * Removes the state listener
 * @param listener The listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
void Caret::removeStateListener(CaretStateListener& listener) {
	stateListeners_.remove(listener);
}

/**
 * Replaces the selected region with the specified text.
 * If the selection is empty, inserts the text at current position.
 * @param text The text to insert
 * @param rectangleInsertion Set @c true to insert text as rectangle
 * @throw NullPointerException @a first and/or @last is @c null
 * @throw std#invalid_argument @a first &gt; @a last
 * @throw ... Any exceptions @c Document#insert and @c Document#erase throw
 */
void Caret::replaceSelection(const StringPiece& text, bool rectangleInsertion /* = false */) {
	k::Position e;
	prechangeDocument();
	if(!isSelectionRectangle() && !rectangleInsertion)
		document().replace(selectedRegion(), text, &e);
	else {
		// TODO: not implemented.
	}
	moveTo(e);
}

/**
 * Recreates and shows the caret. If the text viewer does not have focus, nothing heppen.
 * @see #updateLocation
 */
void Caret::resetVisualization() {
	TextViewer& viewer = textViewer();
	if(!widgetapi::hasFocus(viewer))
		return;

	unique_ptr<Image> image;
	geometry::BasicPoint<uint16_t> alignmentPoint;

	if(context_.inputMethodComposingCharacter) {
		const bool horizontal = isHorizontal(textViewer().textRenderer().computedBlockFlowDirection());
		const graphics::Rectangle bounds(currentCharacterLogicalBounds(*this));
		image.reset(new Image(geometry::BasicDimension<uint16_t>(geometry::dx(bounds), geometry::dy(bounds)), Image::RGB_16));
		geometry::x(alignmentPoint) = static_cast<uint16_t>(geometry::left(bounds));
		geometry::y(alignmentPoint) = static_cast<uint16_t>(geometry::top(bounds));
	} else if(context_.inputMethodCompositionActivated) {
		image.reset(new Image(geometry::BasicDimension<uint16_t>(0, 0), Image::RGB_16));
		alignmentPoint = boost::geometry::make_zero<Point>();
	} else if(shaper_.get() != nullptr)
		shaper_->shape(image, alignmentPoint);
	else {
		DefaultCaretShaper s;
		CaretShapeUpdater u(*this);
		static_cast<CaretShaper&>(s).install(u);
		static_cast<CaretShaper&>(s).shape(image, alignmentPoint);
		static_cast<CaretShaper&>(s).uninstall();
	}
	assert(image.get() != nullptr);

#if defined(ASCENSION_WINDOW_SYSTEM_WIN32)
	::DestroyCaret();
	::CreateCaret(viewer.handle().get(), image->asNativeObject().get(), 0, 0);
	::ShowCaret(viewer.handle().get());
#endif

	shapeCache_.image = move(image);
	shapeCache_.alignmentPoint = alignmentPoint;
	updateLocation();
}

/**
 * Selects the specified region. The active selection mode will be cleared.
 * @param anchor The position where the anchor moves to
 * @param caret The position where the caret moves to
 * @throw BadPositionException @a anchor or @a caret is outside of the document
 */
void Caret::select(const k::Position& anchor, const k::Position& caret) {
	if(isTextViewerDisposed())
		throw TextViewerDisposedException();
	else if(k::positions::isOutsideOfDocumentRegion(document(), anchor))
		throw k::BadPositionException(anchor);
	else if(k::positions::isOutsideOfDocumentRegion(document(), caret))
		throw k::BadPositionException(caret);
	context_.yanking = false;
	if(anchor != anchor_->position() || caret != position()) {
		const k::Region oldRegion(selectedRegion());
		context_.leadingAnchor = true;
		anchor_->moveTo(anchor);
		context_.leadingAnchor = false;
		context_.leaveAnchorNext = true;
		try {
			VisualPoint::moveTo(caret);	// TODO: this may throw...
		} catch(...) {
			context_.leaveAnchorNext = false;
			throw;
		}
		if(isSelectionRectangle())
			context_.selectedRectangle->update(selectedRegion());
		if(autoShow_)
			utils::show(*this);
		fireCaretMoved(oldRegion);
	}
	checkMatchBrackets();
}

/**
 * Sets character input mode.
 * @param overtype Set @c true to set to overtype mode, @c false to set to insert mode
 * @return This caret
 * @see #inputCharacter, #isOvertypeMode
 */
Caret& Caret::setOvertypeMode(bool overtype) BOOST_NOEXCEPT {
	if(overtype != overtypeMode_) {
		overtypeMode_ = overtype;
		stateListeners_.notify<const Caret&>(&CaretStateListener::overtypeModeChanged, *this);
	}
	return *this;
}

/// @see Point#update
void Caret::update(const k::DocumentChange& change) {
	// notify the movement of the anchor and the caret concurrently when the document was changed
	context_.leaveAnchorNext = context_.leadingAnchor = true;
	anchor_->beginInternalUpdate(change);
	Point::update(change);
	anchor_->endInternalUpdate();
	context_.leaveAnchorNext = context_.leadingAnchor = false;
}

/**
 * Moves the caret to valid position with current position, scroll context, and the fonts.
 * @see #resetVisualization
 */
void Caret::updateLocation() {
	const TextViewer& viewer = textViewer();
	if(!widgetapi::hasFocus(viewer) || viewer.isFrozen())
		return;

	const shared_ptr<const font::TextViewport> viewport(viewer.textRenderer().viewport());
	graphics::Point p(modelToView(*viewport, font::TextHit<k::Position>::leading(*this), false));
	const graphics::Rectangle contentRectangle(viewer.textAreaContentRectangle());
	assert(geometry::isNormalized(contentRectangle));

	boost::geometry::model::d2::point_xy<int> newLocation(static_cast<int>(geometry::x(p)), static_cast<int>(geometry::y(p)));
	if(!boost::geometry::within(p, contentRectangle)) {
		// "hide" the caret
		const int linePitch = static_cast<int>(widgetapi::createRenderingContext(viewer)->fontMetrics(viewer.textRenderer().defaultFont())->linePitch());
		if(isHorizontal(textViewer().textRenderer().computedBlockFlowDirection()))
			boost::geometry::assign_values(newLocation, static_cast<int>(geometry::x(p)), -linePitch);
		else
			boost::geometry::assign_values(newLocation, -linePitch, static_cast<int>(geometry::y(p)));
	} else
		boost::geometry::assign_values(newLocation,
			static_cast<int>(geometry::x(p)) - geometry::x(shapeCache_.alignmentPoint),
			static_cast<int>(geometry::y(p)) - geometry::y(shapeCache_.alignmentPoint));
#if defined(ASCENSION_WINDOW_SYSTEM_WIN32)
	::SetCaretPos(boost::geometry::get<0>(newLocation), boost::geometry::get<1>(newLocation));
#endif
	adjustInputMethodCompositionWindow();
}

inline void Caret::updateVisualAttributes() {
	if(isSelectionRectangle())
		context_.selectedRectangle->update(selectedRegion());
	if((context_.regionBeforeMoved->first != position() || context_.regionBeforeMoved->second != position()))
		fireCaretMoved(*context_.regionBeforeMoved);
	if(autoShow_)
		utils::show(*this);
	checkMatchBrackets();
	context_.regionBeforeMoved = boost::none;
}

/// @see DisplaySizeListener#viewerDisplaySizeChanged
void Caret::viewerDisplaySizeChanged() {
//	if(textViewer().rulerConfiguration().alignment != ALIGN_LEFT)
		resetVisualization();
}

/// @see ViewportListener#viewportChanged
void Caret::viewportChanged(bool, bool) {
	updateLocation();
}


// viewers free functions /////////////////////////////////////////////////////////////////////////

/**
 * Returns @c true if the specified point is over the selection.
 * @param p The client coordinates of the point
 * @return true if the point is over the selection
 * @throw kernel#DocumentDisposedException The document @a caret connecting to has been disposed
 * @throw TextViewerDisposedException The text viewer @a caret connecting to has been disposed
 */
bool viewers::isPointOverSelection(const Caret& caret, const Point& p) {
	if(!isSelectionEmpty(caret)) {
		if(caret.isSelectionRectangle())
			return caret.boxForRectangleSelection().includes(p);
		if(caret.textViewer().hitTest(p) == TextViewer::TEXT_AREA_CONTENT_RECTANGLE) {	// ignore if on the margin
			const graphics::Rectangle viewerBounds(widgetapi::bounds(caret.textViewer(), false));
			if(geometry::x(p) <= geometry::right(viewerBounds) && geometry::y(p) <= geometry::bottom(viewerBounds)) {
				const boost::optional<font::TextHit<k::Position>> hit(
					viewToModelInBounds(*caret.textViewer().textRenderer().viewport(), p));
				return hit != boost::none && hit->characterIndex() >= caret.beginning() && hit->characterIndex() <= caret.end();
			}
		}
	}
	return false;
}

/**
 * Returns the selected range on the specified logical line.
 * This function returns a logical range, and does not support rectangular selection.
 * @param caret The caret gives a selection
 * @param line The logical line
 * @param[out] range The selected range. If the selection continued to the next line,
 *                   @c range.end() returns the position of the end of line + 1
 * @return @c true if there is selected range on the line
 * @throw kernel#DocumentDisposedException The document @a caret connecting to has been disposed
 * @throw kernel#BadPositionException @a line is outside of the document
 * @see #selectedRangeOnVisualLine
 */
bool viewers::selectedRangeOnLine(const Caret& caret, Index line, boost::integer_range<Index>& range) {
	const k::Position bos(caret.beginning());
	if(bos.line > line)
		return false;
	const k::Position eos(caret.end());
	if(eos.line < line)
		return false;
	range = boost::irange(
		(line == bos.line) ? bos.offsetInLine : 0,
		(line == eos.line) ? eos.offsetInLine : caret.document().lineLength(line) + 1);
	return true;
}

/**
 * Returns the selected range on the specified visual line.
 * @param caret The caret gives a selection
 * @param line The logical line
 * @param subline The visual subline
 * @param[out] range The selected range. if the selection continued to the next logical line,
 *                   @c range.end() returns the position of the end of line + 1
 * @return @c true if there is selected range on the line
 * @throw kernel#DocumentDisposedException The document @a caret connecting to has been disposed
 * @throw TextViewerDisposedException The text viewer @a caret connecting to has been disposed
 * @throw kernel#BadPositionException @a line or @a subline is outside of the document
 * @see #selectedRangeOnLine
 */
bool viewers::selectedRangeOnVisualLine(const Caret& caret, Index line, Index subline, boost::integer_range<Index>& range) {
	if(!caret.isSelectionRectangle()) {
		if(!selectedRangeOnLine(caret, line, range))
			return false;
		const font::TextLayout& layout = caret.textViewer().textRenderer().layouts().at(line);
		const Index sublineOffset = layout.lineOffset(subline);
		range = boost::irange(
			max(*range.begin(), sublineOffset),
			min(*range.end(), sublineOffset + layout.lineLength(subline) + ((subline < layout.numberOfLines() - 1) ? 0 : 1)));
		return !range.empty();
	} else
		return caret.boxForRectangleSelection().characterRangeInVisualLine(font::VisualLine(line, subline), range);
}

/**
 * Writes the selected string into the specified output stream.
 * @param caret The caret gives a selection
 * @param[out] out The output stream
 * @param newline The newline representation for multiline selection. If the selection is
 *                rectangular, this value is ignored and the document's newline is used instead
 * @return @a out
 */
basic_ostream<Char>& viewers::selectedString(const Caret& caret, basic_ostream<Char>& out, const Newline& newline /* = Newline::USE_INTRINSIC_VALUE */) {
	if(!isSelectionEmpty(caret)) {
		if(!caret.isSelectionRectangle())
			writeDocumentToStream(out, caret.document(), caret.selectedRegion(), newline);
		else {
			const k::Document& document = caret.document();
			const Index lastLine = line(caret.end());
			boost::integer_range<Index> selection(0, 0);
			for(Index line = kernel::line(caret.beginning()); line <= lastLine; ++line) {
				const k::Document::Line& ln = document.getLineInformation(line);
				caret.boxForRectangleSelection().characterRangeInVisualLine(font::VisualLine(line, 0), selection);	// TODO: Recognize wrap (second parameter).
				out.write(ln.text().data() + *selection.begin(), static_cast<streamsize>(selection.size()));
				const String newlineString(ln.newline().asString());
				out.write(newlineString.data(), static_cast<streamsize>(newlineString.length()));
			}
		}
	}
	return out;
}

/**
 * Selects the word at the caret position. This creates a linear selection.
 * If the caret is nowhere, this function does nothing.
 */
void viewers::selectWord(Caret& caret) {
	WordBreakIterator<k::DocumentCharacterIterator> i(
		k::DocumentCharacterIterator(caret.document(), caret.position()),
		AbstractWordBreakIterator::BOUNDARY_OF_SEGMENT, identifierSyntax(caret));
	caret.endRectangleSelection();
	if(k::locations::isEndOfLine(caret)) {
		if(k::locations::isBeginningOfLine(caret))	// an empty line
			caret.moveTo(caret);
		else	// eol
			caret.select((--i).base().tell(), caret);
	} else if(k::locations::isBeginningOfLine(caret))	// bol
		caret.select(caret, (++i).base().tell());
	else {
		const k::Position p((++i).base().tell());
		i.base().seek(k::Position(line(caret), offsetInLine(caret) + 1));
		caret.select((--i).base().tell(), p);
	}
}


// viewers free functions /////////////////////////////////////////////////////////////////////////

/**
 * Breaks the line at the caret position and moves the caret to the end of the inserted string.
 * @param caret The caret
 * @param inheritIndent Set @c true to inherit the indent of the line @c{at.line}
 * @param newlines The number of newlines to insert
 * @throw DocumentDisposedException The document @a caret connecting to has been disposed
 * @throw ... Any exceptions @c Document#insert throws
 */
void viewers::breakLine(Caret& caret, bool inheritIndent, size_t newlines /* = 1 */) {
	if(newlines == 0)
		return;

	shared_ptr<const k::DocumentInput> documentInput(caret.document().input().lock());
	Newline newline((documentInput.get() != nullptr) ? documentInput->newline() : ASCENSION_DEFAULT_NEWLINE);
	documentInput.reset();
	String s(newline.asString());

	if(inheritIndent) {	// simple auto-indent
		const String& currentLine = caret.document().line(line(caret));
		const Index len = identifierSyntax(caret).eatWhiteSpaces(
			currentLine.data(), currentLine.data() + offsetInLine(caret), true) - currentLine.data();
		s += currentLine.substr(0, len);
	}

	if(newlines > 1) {
		basic_stringbuf<Char> sb;
		for(size_t i = 0; i < newlines; ++i)
			sb.sputn(s.data(), static_cast<streamsize>(s.length()));
		s.assign(sb.str());
	}
	return caret.replaceSelection(s);
}

/**
 * Deletes the selected region.
 * If @a caret is nowhere, this function does nothing.
 * @param caret The caret provides a selection
 * @throw ... Any exceptions @c Document#insert and @c Document#erase throw
 */
void viewers::eraseSelection(Caret& caret) {
	return caret.replaceSelection(0, 0);
}

namespace {
	/**
	 * @internal Indents the region selected by the caret.
	 * @param caret The caret gives the region to indent
	 * @param character A character to make indents
	 * @param rectangle Set @c true for rectangular indents (will be ignored level is negative)
	 * @param level The level of the indentation
	 * @deprecated 0.8
	 */
	void indent(Caret& caret, Char character, bool rectangle, long level) {
		// TODO: this code is not exception-safe.
		if(level == 0)
			return;
		const String indent(abs(level), character);
		const k::Region region(caret.selectedRegion());

		if(region.beginning().line == region.end().line) {
			// number of selected lines is one -> just insert tab character(s)
			caret.replaceSelection(indent);
			return;
		}

		const k::Position oldPosition(caret.position());
		k::Position otherResult(caret.anchor());
		Index line = region.beginning().line;

		// indent/unindent the first line
		k::Document& document = caret.document();
		if(level > 0) {
			insert(document, k::Position(line, rectangle ? region.beginning().offsetInLine : 0), indent);
			if(line == otherResult.line && otherResult.offsetInLine != 0)
				otherResult.offsetInLine += level;
			if(line == kernel::line(caret) && offsetInLine(caret) != 0)
				caret.moveTo(k::Position(kernel::line(caret), offsetInLine(caret) + level));
		} else {
			const String& s = document.line(line);
			Index indentLength;
			for(indentLength = 0; indentLength < s.length(); ++indentLength) {
				// this assumes that all white space characters belong to BMP
				if(s[indentLength] == '\t' && GeneralCategory::of(s[indentLength]) != GeneralCategory::SPACE_SEPARATOR)
					break;
			}
			if(indentLength > 0) {
				const Index deleteLength = min<Index>(-level, indentLength);
				erase(document, k::Position(line, 0), k::Position(line, deleteLength));
				if(line == otherResult.line && otherResult.offsetInLine != 0)
					otherResult.offsetInLine -= deleteLength;
				if(line == kernel::line(caret) && offsetInLine(caret) != 0)
					caret.moveTo(k::Position(kernel::line(caret), offsetInLine(caret) - deleteLength));
			}
		}

		// indent/unindent the following selected lines
		if(level > 0) {
			for(++line; line <= region.end().line; ++line) {
				if(document.lineLength(line) != 0 && (line != region.end().line || region.end().offsetInLine > 0)) {
					Index insertPosition = 0;
					if(rectangle) {
						boost::integer_range<Index> dummy(0, 0);
						caret.boxForRectangleSelection().characterRangeInVisualLine(font::VisualLine(line, 0), dummy);	// TODO: recognize wrap (second parameter).
						insertPosition = dummy.front();
					}
					insert(document, k::Position(line, insertPosition), indent);
					if(line == otherResult.line && otherResult.offsetInLine != 0)
						otherResult.offsetInLine += level;
					if(line == kernel::line(caret) && offsetInLine(caret) != 0)
						caret.moveTo(k::Position(kernel::line(caret), offsetInLine(caret) + level));
				}
			}
		} else {
			for(++line; line <= region.end().line; ++line) {
				const String& s = document.line(line);
				Index indentLength;
				for(indentLength = 0; indentLength < s.length(); ++indentLength) {
				// this assumes that all white space characters belong to BMP
					if(s[indentLength] == '\t' && GeneralCategory::of(s[indentLength]) != GeneralCategory::SPACE_SEPARATOR)
						break;
				}
				if(indentLength > 0) {
					const Index deleteLength = min<Index>(-level, indentLength);
					erase(document, k::Position(line, 0), k::Position(line, deleteLength));
					if(line == otherResult.line && otherResult.offsetInLine != 0)
						otherResult.offsetInLine -= deleteLength;
					if(line == kernel::line(caret) && offsetInLine(caret) != 0)
						caret.moveTo(k::Position(kernel::line(caret), offsetInLine(caret) - deleteLength));
				}
			}
		}
	}
} // namespace @0

/**
 * Indents the region selected by the caret by using spaces.
 * @param caret The caret gives the region to indent
 * @param rectangle Set @c true for rectangular indents (will be ignored level is negative)
 * @param level The level of the indentation
 * @throw ...
 * @deprecated 0.8
 */
void viewers::indentBySpaces(Caret& caret, bool rectangle, long level /* = 1 */) {
	return indent(caret, ' ', rectangle, level);
}

/**
 * Indents the region selected by the caret by using horizontal tabs.
 * @param caret The caret gives the region to indent
 * @param rectangle Set @c true for rectangular indents (will be ignored level is negative)
 * @param level The level of the indentation
 * @throw ...
 * @deprecated 0.8
 */
void viewers::indentByTabs(Caret& caret, bool rectangle, long level /* = 1 */) {
	return indent(caret, '\t', rectangle, level);
}

/**
 * Transposes the character (grapheme cluster) addressed by the caret and the previous character,
 * and moves the caret to the end of them.
 * If the characters to transpose are not inside of the accessible region, this method fails and
 * returns @c false
 * @param caret The caret
 * @return false if there is not a character to transpose in the line, or the point is not the
 *               beginning of a grapheme
 * @throw DocumentDisposedException The document the caret connecting to has been disposed
 * @throw ... Any exceptions @c Document#replace throws other than @c DocumentAccessViolationException
 */
bool viewers::transposeCharacters(Caret& caret) {
	// TODO: handle the case when the caret intervened a grapheme cluster.

	// As transposing characters in string "ab":
	//
	//  a b -- transposing clusters 'a' and 'b'. result is "ba"
	// ^ ^ ^
	// | | next-cluster (named pos[2])
	// | middle-cluster (named pos[1]; usually current-position)
	// previous-cluster (named pos[0])

	k::Position pos[3];
	const k::Region region(caret.document().accessibleRegion());

	if(text::ucd::BinaryProperty::is<text::ucd::BinaryProperty::GRAPHEME_EXTEND>(k::locations::characterAt(caret)))	// not the start of a grapheme
		return false;
	else if(!region.includes(caret.position()))	// inaccessible
		return false;

	if(offsetInLine(caret) == 0 || caret.position() == region.first) {
		GraphemeBreakIterator<k::DocumentCharacterIterator> i(
			k::DocumentCharacterIterator(caret.document(), pos[0] = caret.position()));
		pos[1] = (++i).base().tell();
		if(pos[1].line != pos[0].line || pos[1] == pos[0] || !region.includes(pos[1]))
			return false;
		pos[2] = (++i).base().tell();
		if(pos[2].line != pos[1].line || pos[2] == pos[1] || !region.includes(pos[2]))
			return false;
	} else if(offsetInLine(caret) == caret.document().lineLength(line(caret)) || caret.position() == region.second) {
		GraphemeBreakIterator<k::DocumentCharacterIterator> i(
			k::DocumentCharacterIterator(caret.document(), pos[2] = caret.position()));
		pos[1] = (--i).base().tell();
		if(pos[1].line != pos[2].line || pos[1] == pos[2] || !region.includes(pos[1]))
			return false;
		pos[0] = (--i).base().tell();
		if(pos[0].line != pos[1].line || pos[0] == pos[1] || !region.includes(pos[0]))
			return false;
	} else {
		GraphemeBreakIterator<k::DocumentCharacterIterator> i(
			k::DocumentCharacterIterator(caret.document(), pos[1] = caret.position()));
		pos[2] = (++i).base().tell();
		if(pos[2].line != pos[1].line || pos[2] == pos[1] || !region.includes(pos[2]))
			return false;
		i.base().seek(pos[1]);
		pos[0] = (--i).base().tell();
		if(pos[0].line != pos[1].line || pos[0] == pos[1] || !region.includes(pos[0]))
			return false;
	}

	basic_ostringstream<Char> ss;
	writeDocumentToStream(ss, caret.document(), k::Region(pos[1], pos[2]), Newline::LINE_SEPARATOR);
	writeDocumentToStream(ss, caret.document(), k::Region(pos[0], pos[1]), Newline::LINE_SEPARATOR);
	try {
		caret.document().replace(k::Region(pos[0], pos[2]), ss.str());
	} catch(k::DocumentAccessViolationException&) {
		return false;
	}
	assert(caret.position() == pos[2]);
	return true;
}

/**
 * Transposes the line addressed by the caret and the next line, and moves the caret to the same
 * offset in the next line.
 * If the caret is the last line in the document, transposes with the previous line.
 * The intervening newline character is not moved.
 * If the lines to transpose are not inside of the accessible region, this method fails and returns
 * @c false
 * @param caret The caret
 * @return false if there is not a line to transpose
 * @throw DocumentDisposedException The document the caret connecting to has been disposed
 * @throw ... Any exceptions @c Document#replace throws other than @c DocumentAccessViolationException
 */
bool viewers::transposeLines(Caret& caret) {
	if(caret.document().numberOfLines() == 1)	// there is just one line
		return false;

	k::Document& document = caret.document();
	const k::Position old(caret.position());
	const Index firstLine = (old.line != document.numberOfLines() - 1) ? old.line : old.line - 1;
	String s(document.line(firstLine + 1));
	s += document.getLineInformation(firstLine).newline().asString();
	s += document.line(firstLine);

	try {
		document.replace(k::Region(
			k::Position(firstLine, 0), k::Position(firstLine + 1, document.lineLength(firstLine + 1))), s);
		caret.moveTo(k::Position((old.line != document.numberOfLines() - 1) ? firstLine + 1 : firstLine, old.offsetInLine));
	} catch(const k::DocumentAccessViolationException&) {
		return false;
	}
	return true;
}

/**
 * Transposes the word addressed by the caret and the next word, and moves the caret to the end of
 * them.
 * If the words to transpose are not inside of the accessible region, this method fails and returns
 * @c false
 * @param caret The caret
 * @return false if there is not a word to transpose
 * @throw DocumentDisposedException The document the caret connecting to has been disposed
 * @throw ... Any exceptions @c Document#replace throws other than @c DocumentAccessViolationException
 */
bool viewers::transposeWords(Caret& caret) {
	// As transposing words in string "(\w+)[^\w*](\w+)":
	//
	//  abc += xyz -- transposing words "abc" and "xyz". result is "xyz+=abc"
	// ^   ^  ^   ^
	// |   |  |   2nd-word-end (named pos[3])
	// |   |  2nd-word-start (named pos[2])
	// |   1st-word-end (named pos[1])
	// 1st-word-start (named pos[0])

	WordBreakIterator<k::DocumentCharacterIterator> i(
		k::DocumentCharacterIterator(caret.document(), caret),
		AbstractWordBreakIterator::START_OF_ALPHANUMERICS, identifierSyntax(caret));
	k::Position pos[4];

	// find the backward word (1st-word-*)...
	pos[0] = (--i).base().tell();
	i.setComponent(AbstractWordBreakIterator::END_OF_ALPHANUMERICS);
	pos[1] = (++i).base().tell();
	if(pos[1] == pos[0])	// the word is empty
		return false;

	// ...and then backward one (2nd-word-*)
	i.base().seek(caret);
	i.setComponent(AbstractWordBreakIterator::START_OF_ALPHANUMERICS);
	pos[2] = (++i).base().tell();
	if(pos[2] == caret.position())
		return false;
	pos[3] = (++i).base().tell();
	if(pos[2] == pos[3])	// the word is empty
		return false;

	// replace
	basic_ostringstream<Char> ss;
	writeDocumentToStream(ss, caret.document(), k::Region(pos[2], pos[3]), Newline::USE_INTRINSIC_VALUE);
	writeDocumentToStream(ss, caret.document(), k::Region(pos[1], pos[2]), Newline::USE_INTRINSIC_VALUE);
	writeDocumentToStream(ss, caret.document(), k::Region(pos[0], pos[1]), Newline::USE_INTRINSIC_VALUE);
	k::Position e;
	try {
		caret.document().replace(k::Region(pos[0], pos[3]), ss.str(), &e);
	} catch(const k::DocumentAccessViolationException&) {
		return false;
	}
	return caret.moveTo(e), true;
}
