/**
 * @file command.cpp
 * @author exeal
 * @date 2006-2011 was text-editor.cpp
 * @date 2011-05-06
 * @date 2011-2012
 */

#include <ascension/content-assist/content-assist.hpp>
#include <ascension/corelib/text/break-iterator.hpp>	// text.WordBreakIterator
#include <ascension/text-editor/command.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/win32/ui/wait-cursor.hpp>

using namespace ascension;
using namespace ascension::kernel;
using namespace ascension::texteditor;
using namespace ascension::texteditor::commands;
using namespace ascension::viewers;
using namespace std;


// Command ////////////////////////////////////////////////////////////////////////////////////////

/**
 * Protected constructor.
 * @param viewer The target text viewer
 */
Command::Command(TextViewer& viewer) /*throw()*/ : viewer_(&viewer), numericPrefix_(1) {
}

/// Destructor.
Command::~Command() /*throw()*/ {
}


// commands.* /////////////////////////////////////////////////////////////////////////////////////

namespace {
	inline bool abortModes(TextViewer& target) {
		utils::closeCompletionProposalsPopup(target);
		return abortIncrementalSearch(target);
	}
}

#define ASCENSION_ASSERT_IFISWINDOW() assert(true/*target().isWindow()*/)

// the command can't perform and throw if the document is read only
#define ASCENSION_CHECK_DOCUMENT_READ_ONLY()	\
	if(target().document().isReadOnly()) return false

/**
 * Constructor.
 * @param viewer The target text viewer
 * @param region The region to operate on. If empty, the accessible region of the document. This
 *               region will be shrunk to the accessible region when the command performed
 */
BookmarkMatchLinesCommand::BookmarkMatchLinesCommand(TextViewer& viewer,
		const Region& region /* = Region() */) /*throw()*/ : Command(viewer), region_(region), numberOfMarkedLines_(0) {
}

/// Returns the number of the previously marked lines.
Index BookmarkMatchLinesCommand::numberOfMarkedLines() const /*throw()*/ {
	return numberOfMarkedLines_;
}

/**
 * Implements @c Command#perform.
 * @retval false The pattern to search was not set
 * @throw ... Any exceptions specified @c TextSearcher#search other than @c BadPositionException
 *            and @c IllegalStateException. If threw, the marking is interrupted
 */
bool BookmarkMatchLinesCommand::perform() {
	win32::WaitCursor wc;
	TextViewer& viewer = target();
	Document& document = viewer.document();
	const searcher::TextSearcher* s;
	if(const Session* const session = document.session())
		s = &session->textSearcher();
	else
		return true;	// TODO: prepares a default text searcher.
	if(!s->hasPattern())
		return false;

	numberOfMarkedLines_ = 0;
	Region scope(region_.isEmpty() ? document.accessibleRegion() : region_.getIntersection(document.accessibleRegion()));

	Bookmarker& bookmarker = document.bookmarker();
	Region matchedRegion;
	while(s->search(document,
			max<Position>(viewer.caret().beginning(), document.accessibleRegion().first),
			scope, Direction::FORWARD, matchedRegion)) {
		bookmarker.mark(matchedRegion.first.line);
		scope.first.line = matchedRegion.first.line + 1;
		scope.first.offsetInLine = 0;
		++numberOfMarkedLines_;
	}
	return true;
}

/**
 * Constructor.
 * @param viewer The target text viewer
 */
CancelCommand::CancelCommand(TextViewer& viewer) /*throw()*/ : Command(viewer) {
}

/**
 * Implements @c Command#perform.
 * @return true
 */
bool CancelCommand::perform() {
	ASCENSION_ASSERT_IFISWINDOW();
	abortModes(target());
	target().caret().clearSelection();
	return true;
}

/**
 * Constructor.
 * @param viewer The target text viewer
 * @param procedure A function gives a motion
 * @param direction The direction of motion
 * @param extendSelection Set @c true to extend the selection
 * @throw NullPointerException @a procedure is @c null
 */
template<typename ProcedureSignature>
CaretMovementCommand<ProcedureSignature>::CaretMovementCommand(
		TextViewer& viewer, ProcedureSignature* procedure,
		Direction direction, bool extendSelection /* = false */)
		: Command(viewer), procedure_(procedure), direction_(direction), extends_(extendSelection) {
	if(procedure == nullptr)
		throw NullPointerException("procedure");
}

namespace {
	template<typename CaretMovementProcedure>
	inline bool selectCompletionProposal(TextViewer&, CaretMovementProcedure*, Direction, long) {
		return false;
	}
	inline bool selectCompletionProposal(TextViewer& target, Position(*procedure)(const Point&, Direction, Index), Direction direction, long n) {
		if(procedure == &locations::nextLine) {
			if(contentassist::ContentAssistant* const ca = target.contentAssistant()) {
				if(contentassist::ContentAssistant::CompletionProposalsUI* const cpui = ca->completionProposalsUI())
					return cpui->nextProposal((direction == Direction::FORWARD) ? +n : -n), true;
			}
		}
		return false;
	}
	inline bool selectCompletionProposal(TextViewer& target, VisualDestinationProxy(*procedure)(const VisualPoint&, Direction, Index), Direction direction, long n) {
		if(procedure == &locations::nextVisualLine || procedure == &locations::nextPage) {
			if(contentassist::ContentAssistant* const ca = target.contentAssistant()) {
				if(contentassist::ContentAssistant::CompletionProposalsUI* const cpui = ca->completionProposalsUI()) {
					if(procedure == &locations::nextVisualLine)
						return cpui->nextProposal((direction == Direction::FORWARD) ? +n : -n), true;
					else if(procedure == &locations::nextPage)
						return cpui->nextPage((direction == Direction::FORWARD) ? +n : -n), true;
				}
			}
		}
		return false;
	}

	template<typename ProcedureSignature>
	inline bool moveToBoundOfSelection(Caret&, ProcedureSignature*, Direction) {
		return false;
	}
	inline bool moveToBoundOfSelection(Caret& caret, Position(*procedure)(const Point&, Direction direction, locations::CharacterUnit, Index), Direction direction) {
		if(procedure == &locations::nextCharacter)
			return caret.moveTo((direction == Direction::FORWARD) ? caret.end() : caret.beginning()), true;
		return false;
	}

	template<typename ProcedureSignature>
	inline void scrollTextViewer(TextViewer&, ProcedureSignature, Direction, long) {
	}
	inline void scrollTextViewer(TextViewer& target, VisualDestinationProxy(*procedure)(const VisualPoint&, Direction, Index), Direction direction, long n) {
		// TODO: consider the numeric prefix.
		if(procedure == &locations::nextPage) {
			graphics::font::TextViewport::SignedScrollOffset offset = (direction == Direction::FORWARD) ? n : -n;
			if(offset != 0) {
				presentation::AbstractTwoAxes<graphics::font::TextViewport::SignedScrollOffset> delta;
				delta.bpd() = offset;
				delta.ipd() = 0;
				target.textRenderer().viewport()->scroll(delta);
			}
		}
	}

	template<typename PointType>
	inline void moveCaret(Caret& caret, Position(*procedure)(const PointType&), Index, bool extend) {
		if(!extend)
			caret.moveTo((*procedure)(caret));
		else
			caret.extendSelectionTo((*procedure)(caret));
	}
	template<typename PointType>
	inline void moveCaret(Caret& caret, Position(*procedure)(const PointType&, Direction, Index), Direction direction, Index n, bool extend) {
		if(!extend)
			caret.moveTo((*procedure)(caret, direction, n));
		else
			caret.extendSelectionTo((*procedure)(caret, direction, n));
	}
	inline void moveCaret(Caret& caret, boost::optional<Position>(*procedure)(const Point&, Direction, Index), Direction direction, Index n, bool extend) {
		if(const boost::optional<Position> destination = (*procedure)(caret, direction, n)) {
			if(!extend)
				caret.moveTo(*destination);
			else
				caret.extendSelectionTo(*destination);
		}
	}
	template<typename PointType>
	inline void moveCaret(Caret& caret, Position(*procedure)(const PointType&, Direction, locations::CharacterUnit, Index), Direction direction, Index n, bool extend) {
		if(!extend)
			caret.moveTo((*procedure)(caret, direction, locations::GRAPHEME_CLUSTER, n));
		else
			caret.extendSelectionTo((*procedure)(caret, direction, locations::GRAPHEME_CLUSTER, n));
	}
	inline void moveCaret(Caret& caret, VisualDestinationProxy(*procedure)(const VisualPoint&, Direction, Index), Direction direction, Index n, bool extend) {
		if(!extend)
			caret.moveTo((*procedure)(caret, direction, n));
		else
			caret.extendSelectionTo((*procedure)(caret, direction, n));
	}
}

// explicit instantiations
template class CaretMovementCommand<Position(const Point&, Direction, Index)>;	// next(Line|Word|WordEnd)
template class CaretMovementCommand<boost::optional<Position>(const Point&, Direction, Index)>;	// nextBookmark
template class CaretMovementCommand<Position(const Point&, Direction, locations::CharacterUnit, Index)>;	// nextCharacter
template class CaretMovementCommand<VisualDestinationProxy(const VisualPoint&, Direction, Index)>;	// next(Page|VisualLine)

/**
 * Moves the caret or extends the selection.
 * @return true
 */
template<typename ProcedureSignature>
bool CaretMovementCommand<ProcedureSignature>::perform() {
	const NumericPrefix n = numericPrefix();
	endIncrementalSearch(target());
	if(n == 0)
		return true;
	Caret& caret = target().caret();

	if(!extends_) {
		if(selectCompletionProposal(target(), procedure_, direction_, n))
			return true;
		caret.endRectangleSelection();
		if(isSelectionEmpty(caret)) {	// just clear the selection
			if(moveToBoundOfSelection(caret, procedure_, direction_))
				return true;
		}
	}

	scrollTextViewer(target(), procedure_, direction_, n);
	moveCaret(caret, procedure_, direction_, n, extends_);
	return true;
}

/**
 * Constructor.
 * @param viewer The target text viewer
 * @param procedure A function gives a motion
 * @param extendSelection Set @c true to extend the selection
 * @throw NullPointerException @a procedure is @c null
 */
template<typename ProcedureSignature>
CaretMovementToDefinedPositionCommand<ProcedureSignature>::CaretMovementToDefinedPositionCommand(
		TextViewer& viewer, ProcedureSignature* procedure, bool extendSelection /* = false */)
		: Command(viewer), procedure_(procedure), extends_(extendSelection) {
	if(procedure == nullptr)
		throw NullPointerException("procedure");
}

// explicit instantiations
template class CaretMovementToDefinedPositionCommand<Position(const Point&)>;	// (beginning|end)Of(Document|Line)
template class CaretMovementToDefinedPositionCommand<Position(const VisualPoint&)>;	// contextual(Beginning|End)OfLine, (beginning|end|contextualBeginning|contextualEnd)OfVisualLine, (first|last)PrintableCharacterOf(Visual)?Line

/**
 * Moves the caret or extends the selection.
 * @return true
 */
template<typename ProcedureSignature>
bool CaretMovementToDefinedPositionCommand<ProcedureSignature>::perform() {
	endIncrementalSearch(target());
	if(!extends_)
		target().caret().moveTo((*procedure_)(target().caret()));
	else
		target().caret().extendSelectionTo((*procedure_)(target().caret()));
	return true;
}

/**
 * Constructor.
 * @param viewer The target text viewer
 * @param direction The direcion to delete
 */
CharacterDeletionCommand::CharacterDeletionCommand(TextViewer& viewer,
		Direction direction) /*throw()*/ : Command(viewer), direction_(direction) {
}

/**
 * Implements @c Command#perform.
 * @retval false If the incremental search was active, couldn't undo. Otherwise the document was
 *               read only or the region to delete was inaccessible
 */
bool CharacterDeletionCommand::perform() {
	NumericPrefix n = numericPrefix();
	if(n == 0)
		return true;
	TextViewer& viewer = target();
	if(/*caret.isAutoCompletionRunning() &&*/ direction_ == Direction::FORWARD)
		utils::closeCompletionProposalsPopup(viewer);

	Document& document = viewer.document();
	searcher::IncrementalSearcher* isearch = nullptr;
	if(Session* const session = document.session())
		isearch = &session->incrementalSearcher();
	if(isearch != nullptr && isearch->isRunning()) {
		if(direction_ == Direction::FORWARD)	// delete the entire pattern
			isearch->reset();
		else {	// delete the last N characters (undo)
			if(!isearch->canUndo())
				return false;
			else {
				isearch->undo();
				for(--n; n > 0 && isearch->canUndo(); --n)
					isearch->undo();
			}
		}
	} else {
		ASCENSION_CHECK_DOCUMENT_READ_ONLY();
		document.insertUndoBoundary();
		Caret& caret = viewer.caret();
		if(n == 1 && !isSelectionEmpty(caret)) {	// delete only the selected content
			try {
				eraseSelection(caret);;
			} catch(const DocumentInput::ChangeRejectedException&) {
				return false;
			}
		} else {
			AutoFreeze af((!isSelectionEmpty(caret) || n > 1) ? &viewer : nullptr);
			Region region(caret.selectedRegion());
			assert(region.isNormalized());
			if(direction_ == Direction::FORWARD)
				region.second = locations::nextCharacter(document, region.second,
					Direction::FORWARD, locations::GRAPHEME_CLUSTER, isSelectionEmpty(caret) ? n : (n - 1));
			else
				region.first = locations::nextCharacter(document, region.first,
					Direction::BACKWARD, locations::UTF32_CODE_UNIT, isSelectionEmpty(caret) ? n : (n - 1));
			try {
				erase(document, region);
			} catch(const DocumentInput::ChangeRejectedException&) {
				return false;
			}
		}
	}
	return true;
}

/**
 * Constructor.
 * @param viewer The target text viewer
 * @param c The code point of the character to input
 * @throw text#InvalidScalarValueException @a c is not valid Unicode scalar value
 */
CharacterInputCommand::CharacterInputCommand(TextViewer& viewer, CodePoint c) : Command(viewer), c_(c) {
	if(!text::isScalarValue(c))
		throw text::InvalidScalarValueException(c);
}

/**
 * Implements @c Command#perform.
 * @retval false Failed and the incremental search is not active
 * @retval 0 Otherwise
 * @see Caret#inputCharacter, TextViewer#onChar, TextViewer#onUniChar
 */
bool CharacterInputCommand::perform() {
	if(numericPrefix() == 1) {
		if(Session* const session = target().document().session()) {
			if(session->incrementalSearcher().isRunning()) {
				utils::closeCompletionProposalsPopup(target());
				if(c_ == 0x0009u || iswcntrl(static_cast<wint_t>(c_)) == 0)
					session->incrementalSearcher().addCharacter(c_);
				return true;
			}
		}
		try {
			return target().caret().inputCharacter(c_);
		} catch(const DocumentCantChangeException&) {
			return false;
		}
	} else {
		ASCENSION_CHECK_DOCUMENT_READ_ONLY();
		if(numericPrefix() > 0) {	// ...
			String s;
			text::utf::encode(c_, back_inserter(s));
			return TextInputCommand(target(), s).setNumericPrefix(numericPrefix())();
		}
		return true;
	}
}

/**
 * Constructor.
 * @param viewer The target text viewer
 * @param fromPreviousLine Set @c true to use a character on the previous visual line. Otherwise
 *                         one on the next visual line is used
 */
CharacterInputFromNextLineCommand::CharacterInputFromNextLineCommand(
		TextViewer& viewer, bool fromPreviousLine) /*throw()*/ : Command(viewer), fromPreviousLine_(fromPreviousLine) {
}

/**
 * Implements @c Command#perform.
 * @retval false The caret was the first/last line in the document and couldn't copy a character
 *               from the previous/next line. Or the next/previous line was too short to locate the
 *               character to copy. Or internal performance of @c CharacterInputCommand failed
 */
bool CharacterInputFromNextLineCommand::perform() {
	abortIncrementalSearch(target());
	ASCENSION_CHECK_DOCUMENT_READ_ONLY();

	// TODO: recognizes narrowing.

	const Document& document = target().document();
	const VisualPoint& caret = target().caret();

	if((fromPreviousLine_ && line(caret) == 0)
			|| (!fromPreviousLine_ && line(caret) >= document.numberOfLines() - 1))
		return false;
	
	const Position p(locations::nextVisualLine(caret, fromPreviousLine_ ? Direction::BACKWARD : Direction::FORWARD).position());
	const String& lineString = document.line(line(caret) + (fromPreviousLine_ ? -1 : 1));
	if(p.offsetInLine >= lineString.length())
		return false;
	setNumericPrefix(1);
	return CharacterInputCommand(target(), text::utf::decodeFirst(begin(lineString) + p.offsetInLine, end(lineString)))();
}

/**
 * Constructor.
 * @param viewer The target text viewer
 */
CharacterToCodePointConversionCommand::CharacterToCodePointConversionCommand(TextViewer& viewer) /*throw()*/ : Command(viewer) {
}

/**
 * Implements @c Command#perform.
 * @retval false The end of the selection is the beginning of the line and couldn't find the string
 *               to convert
 */
bool CharacterToCodePointConversionCommand::perform() {
	ASCENSION_CHECK_DOCUMENT_READ_ONLY();
	TextViewer& viewer = target();
	abortModes(viewer);
	const Document& document = viewer.document();
	const VisualPoint& eos = viewer.caret().end();
	if(locations::isBeginningOfLine(eos)
			|| (document.isNarrowed() && eos.position() == document.accessibleRegion().first))
		return false;

	Caret& caret = viewer.caret();
	const String& lineString = document.line(line(eos));
	const CodePoint c = text::utf::decodeLast(begin(lineString), begin(lineString) + offsetInLine(eos));
	Char buffer[7];
#if(_MSC_VER < 1400)
	swprintf(buffer, L"%lX", c);
#else
	swprintf(buffer, ASCENSION_COUNTOF(buffer), L"%lX", c);
#endif // _MSC_VER < 1400
	AutoFreeze af(&viewer);
	caret.select(Position(line(eos), offsetInLine(eos) - ((c > 0xffff) ? 2 : 1)), eos);
	try {
		caret.replaceSelection(buffer, false);
	} catch(const DocumentInput::ChangeRejectedException&) {
		return false;
	}
	return true;
}

/**
 * Constructor.
 * @param viewer The target text viewer
 */
CodePointToCharacterConversionCommand::CodePointToCharacterConversionCommand(TextViewer& viewer) /*throw()*/ : Command(viewer) {
}

/**
 * Implements @c Command#perform.
 * @retval false The end of the selection is the beginning of the line and couldn't find the string
 *               to convert
 */
bool CodePointToCharacterConversionCommand::perform() {
	ASCENSION_CHECK_DOCUMENT_READ_ONLY();
	TextViewer& viewer = target();
	abortModes(viewer);
	const Document& document = viewer.document();
	const VisualPoint& eos = viewer.caret().end();
	if(locations::isBeginningOfLine(eos)
			|| (document.isNarrowed() && eos.position() == document.accessibleRegion().first))
		return false;

	Caret& caret = viewer.caret();
	const String& lineString = document.line(line(eos));
	const Index offsetInLine = kernel::offsetInLine(eos);

	// accept /(?:[Uu]\+)?[0-9A-Fa-f]{1,6}/
	if(iswxdigit(lineString[offsetInLine - 1]) != 0) {
		Index i = offsetInLine - 1;
		while(i != 0) {
			if(offsetInLine - i == 7)
				return false;	// too long string
			else if(iswxdigit(lineString[i - 1]) == 0)
				break;
			--i;
		}

		const CodePoint c = wcstoul(lineString.substr(i, offsetInLine - i).c_str(), nullptr, 16);
		if(text::isValidCodePoint(c)) {
			String s;
			text::utf::encode(c, back_inserter(s));
			if(i >= 2 && lineString[i - 1] == L'+' && (lineString[i - 2] == L'U' || lineString[i - 2] == L'u'))
				i -= 2;
			AutoFreeze af(&viewer);
			caret.select(Position(line(eos), i), eos);
			try {
				caret.replaceSelection(s, false);
			} catch(const DocumentInput::ChangeRejectedException&) {
				return false;
			}
			return true;
		}
	}
	return false;	// invalid code point string and can't convert
}

/**
 * Constructor.
 * @param viewer The target text viewer
 */
CompletionProposalPopupCommand::CompletionProposalPopupCommand(TextViewer& viewer) /*throw()*/ : Command(viewer) {
}

/**
 * Implements @c Command#perform.
 * @retval false The text viewer didn't have the content assistant
 */
bool CompletionProposalPopupCommand::perform() {
	ASCENSION_CHECK_DOCUMENT_READ_ONLY();
//	ASCENSION_CHECK_GUI_EDITABILITY();
	abortIncrementalSearch(target());
	if(contentassist::ContentAssistant* ca = target().contentAssistant()) {
		ca->showPossibleCompletions();
		return true;
	}
	return false;	// the viewer does not have a content assistant
}

/**
 * Constructor.
 * @param viewer The target text viewer
 */
EntireDocumentSelectionCreationCommand::EntireDocumentSelectionCreationCommand(TextViewer& viewer) /*throw()*/ : Command(viewer) {
}

/**
 * @see Command#perform
 * @return true
 */
bool EntireDocumentSelectionCreationCommand::perform() {
	endIncrementalSearch(target());
	target().caret().endRectangleSelection();
	target().caret().select(target().document().accessibleRegion());
	return true;
}

/**
 * Constructor.
 * @param viewer The target text viewer
 * @param direction The direction to search
 */
FindNextCommand::FindNextCommand(TextViewer& viewer, Direction direction) /*throw()*/ : Command(viewer), direction_(direction) {
}

/**
 * @see Command#perform
 * @return false If no text matched
 * @throw ... Any exceptions @c searcher#TextSearcher#search throws
 */
bool FindNextCommand::perform() {
	if(numericPrefix() == 0)
		return false;
	endIncrementalSearch(target());
	utils::closeCompletionProposalsPopup(target());

	win32::WaitCursor wc;	// TODO: code depends on Win32.
	Document& document = target().document();
	const searcher::TextSearcher* s;
	if(const Session* const session = document.session())
		s = &session->textSearcher();
	else
		return false;	// TODO: prepares a default text searcher.

	Caret& caret = target().caret();
	const Region scope(document.accessibleRegion());
	Region matchedRegion(caret.selectedRegion());
	bool foundOnce = false;
	for(NumericPrefix n(numericPrefix()); n > 0; --n) {	// search N times
		if(!s->search(document, (direction_ == Direction::FORWARD) ?
				max<Position>(matchedRegion.end(), scope.first)
				: min<Position>(matchedRegion.beginning(), scope.second), scope, direction_, matchedRegion))
			break;
		foundOnce = true;
	}

	if(foundOnce) {
		caret.select(matchedRegion);
//		viewer.highlightMatchTexts();
	} else
/*		viewer.highlightMatchTexts(false)*/;
	return foundOnce;
}

/**
 * Constructor.
 * @param viewer The target text viewer
 * @param type The search type
 * @param direction The direction to search
 * @param callback The callback object for the incremental search. Can be @c null
 */
IncrementalFindCommand::IncrementalFindCommand(TextViewer& viewer, searcher::TextSearcher::Type type,
		Direction direction, searcher::IncrementalSearchCallback* callback /* = nullptr */) /*throw()*/
		: Command(viewer), type_(type), direction_(direction), callback_(callback) {
}

/**
 * Implements Command#perform.
 * @return false If no text matched
 * @throw ... Any exceptions @c IncrementalSearcher#start and @c IncrementalSearcher#next throw
 */
bool IncrementalFindCommand::perform() {
	NumericPrefix n(numericPrefix());
	if(n == 0)
		return false;
	utils::closeCompletionProposalsPopup(target());
	if(Session* const session = target().document().session()) {
		searcher::IncrementalSearcher& isearch = session->incrementalSearcher();
		if(!isearch.isRunning()) {	// begin the search if not running
			isearch.start(target().document(), target().caret(), session->textSearcher(), type_, direction_, callback_);
			--n;
		}
		for(; n > 0; --n) {	// jump N times
			if(!isearch.next(direction_))
				return false;	// it is not able to jump anymore in the active incremental search
		}
	}
	return true;
}

/**
 * Constructor.
 * @param viewer The target text viewer
 * @param increase Set @c true to increase the indentation
 */
IndentationCommand::IndentationCommand(TextViewer& viewer, bool increase) /*throw()*/ : Command(viewer), increases_(increase) {
}

/**
 * @see Command#perform
 * @retval false The document's input rejected the change
 */
bool IndentationCommand::perform() {
	const NumericPrefix n(numericPrefix());
	if(n == 0)
		return true;
	ASCENSION_CHECK_DOCUMENT_READ_ONLY();
//	ASCENSION_CHECK_GUI_EDITABILITY();
	TextViewer& viewer = target();
	endIncrementalSearch(viewer);
	utils::closeCompletionProposalsPopup(viewer);

	try {
		Caret& caret = viewer.caret();
		viewer.document().insertUndoBoundary();
		AutoFreeze af(&viewer);
		const long tabs = n;
		indentByTabs(caret, caret.isSelectionRectangle(), increases_ ? +tabs : -tabs);
		viewer.document().insertUndoBoundary();
	} catch(const DocumentInput::ChangeRejectedException&) {
		return false;
	}

	return true;
}

/**
 * Constructor.
 * @param viewer the target text viewer
 */
InputMethodOpenStatusToggleCommand::InputMethodOpenStatusToggleCommand(TextViewer& viewer) /*throw()*/ : Command(viewer) {
}

/**
 * Implements @c Command#perform.
 * @retval false The system didn't support the input method
 */
bool InputMethodOpenStatusToggleCommand::perform() {
	if(win32::Handle<HIMC> imc = win32::inputMethod(target()))
		return win32::boole(::ImmSetOpenStatus(imc.get(), !win32::boole(::ImmGetOpenStatus(imc.get()))));
	return false;
}

/**
 * Constructor.
 * @param viewer The target text viewer
 */
InputMethodSoftKeyboardModeToggleCommand::InputMethodSoftKeyboardModeToggleCommand(TextViewer& viewer) /*throw()*/ : Command(viewer) {
}

/**
 * @see Command#perform
 * @retval false The system didn't support the input method
 */
bool InputMethodSoftKeyboardModeToggleCommand::perform() {
	if(win32::Handle<HIMC> imc = win32::inputMethod(target())) {
		DWORD conversionMode, sentenceMode;
		if(win32::boole(::ImmGetConversionStatus(imc.get(), &conversionMode, &sentenceMode))) {
			conversionMode = win32::boole(conversionMode & IME_CMODE_SOFTKBD) ?
				(conversionMode & ~IME_CMODE_SOFTKBD) : (conversionMode | IME_CMODE_SOFTKBD);
			return win32::boole(::ImmSetConversionStatus(imc.get(), conversionMode, sentenceMode));
		}
	}
	return false;
}

/**
 * Constructor.
 * @param viewer The target text viewer
 * @param extendSelection Set @c true to extend the selection
 */
MatchBracketCommand::MatchBracketCommand(TextViewer& viewer, bool extendSelection) /*throw()*/ : Command(viewer), extends_(extendSelection) {
}

/**
 * Implements @c Command#perform.
 * @retval false The match bracket was not found
 */
bool MatchBracketCommand::perform() {
	endIncrementalSearch(target());
	Caret& caret = target().caret();
	if(const boost::optional<pair<Position, Position>> matchBrackets = caret.matchBrackets()) {
		caret.endRectangleSelection();
		if(!extends_)
			caret.moveTo(matchBrackets->first);
		else if(matchBrackets->first > caret)
			caret.select(caret, Position(matchBrackets->first.line, matchBrackets->first.offsetInLine + 1));
		else
			caret.select(Position(line(caret), offsetInLine(caret) + 1), matchBrackets->first);
		return true;
	} else
		return false;	// not found
}

/**
 * Constructor.
 * @param viewer The target text viewer
 * @param direction Set @c boost#none to break current line at the caret position. Otherwise,
 *                  this command inserts newline(s) at the beginning of the next
 *                  (@c Direction#FORWARD) or the previous (@c Direction#BACKWARD) line. In this
 *                  case, the command ends the active mode and inserts newline character(s)
 */
NewlineCommand::NewlineCommand(TextViewer& viewer, boost::optional<Direction> direction) /*throw()*/ : Command(viewer), direction_(direction) {
}

/**
 * Implements @c Command#perform.
 * @retval false The document was read only or the change was rejected
 */
bool NewlineCommand::perform() {
	if(numericPrefix() <= 0)
		return true;
	TextViewer& viewer = target();

	if(contentassist::ContentAssistant* const ca = target().contentAssistant()) {
		if(contentassist::ContentAssistant::CompletionProposalsUI* cpui = ca->completionProposalsUI()) {
			if(cpui->complete())
				return true;
		}
	}

	if(endIncrementalSearch(viewer) && direction_ == boost::none)
		return true;

	ASCENSION_CHECK_DOCUMENT_READ_ONLY();
//	ASCENSION_CHECK_GUI_EDITABILITY(1);

	Caret& caret = viewer.caret();
	const Region oldSelection(caret.selectedRegion());
	Document& document = viewer.document();
	AutoFreeze af(&viewer);

	if(direction_ != boost::none) {
		Position p;
		if(*direction_ == Direction::FORWARD)
			p = locations::endOfVisualLine(caret);
		else if(line(caret) != document.region().beginning().line)
			p.offsetInLine = document.lineLength(p.line = line(caret) - 1);
		else
			p = document.region().beginning();
		if(p < document.accessibleRegion().beginning() || p > document.accessibleRegion().end())
			return false;
		const bool autoShow = caret.isAutoShowEnabled();
		caret.enableAutoShow(false);
		caret.moveTo(p);
		caret.enableAutoShow(autoShow);
	}

	try {
		document.insertUndoBoundary();
		breakLine(caret, false, numericPrefix());
	} catch(const DocumentInput::ChangeRejectedException&) {
		document.insertUndoBoundary();
		caret.select(oldSelection);
		return false;
	}
	document.insertUndoBoundary();
	caret.moveTo(caret.anchor());
	return true;
}

/**
 * Constructor.
 * @param viewer The target text viewer
 */
OvertypeModeToggleCommand::OvertypeModeToggleCommand(TextViewer& viewer) /*throw()*/ : Command(viewer) {
}

/**
 * @see Command#perform
 * @return true
 */
bool OvertypeModeToggleCommand::perform() {
	Caret& caret = target().caret();
	caret.setOvertypeMode(!caret.isOvertypeMode());
	utils::closeCompletionProposalsPopup(target());
	return true;
}

/**
 * Constructor.
 * @param viewer The target text viewer
 */
PasteCommand::PasteCommand(TextViewer& viewer, bool useKillRing) /*throw()*/ : Command(viewer), usesKillRing_(useKillRing) {
}

/**
 * @see Command#perform
 * @return false the internal call of @c Caret#paste threw
 */
bool PasteCommand::perform() {
	ASCENSION_ASSERT_IFISWINDOW();
	ASCENSION_CHECK_DOCUMENT_READ_ONLY();
	utils::closeCompletionProposalsPopup(target());
	try {
		target().caret().paste(usesKillRing_);
	} catch(...) {
		return false;
	}
	return true;
}

/**
 * Constructor.
 * @param viewer The target text viewer
 */
ReconversionCommand::ReconversionCommand(TextViewer& viewer) /*throw()*/ : Command(viewer) {
}

/**
 * Implements @c Command#perform.
 * @return false The selection was empty or rectangle. Or the system didn't support IME
 *               reconversion
 * @throw std#bad_alloc Out of memory
 * @see viewers#TextViewer#onIMERequest
 */
bool ReconversionCommand::perform() {
	TextViewer& viewer = target();
	endIncrementalSearch(viewer);
	ASCENSION_CHECK_DOCUMENT_READ_ONLY();
//	ASCENSION_CHECK_GUI_EDITABILITY();

	bool succeeded = false;
	Caret& caret = viewer.caret();
	if(!caret.isSelectionRectangle()) {
		if(win32::Handle<HIMC> imc = win32::inputMethod(viewer)) {
			if(!win32::boole(::ImmGetOpenStatus(imc.get))))	// without this, IME may ignore us?
				::ImmSetOpenStatus(imc.get(), true);

			// from NotePadView.pas of TNotePad (http://wantech.ikuto.com/)
			const bool multilineSelection = line(caret) != line(caret.anchor());
			const String s(multilineSelection ? selectedString(caret) : viewer.document().line(line(caret)));
			const DWORD bytes = static_cast<DWORD>(sizeof(RECONVERTSTRING) + sizeof(Char) * s.length());
			RECONVERTSTRING* const rcs = static_cast<RECONVERTSTRING*>(::operator new(bytes));
			rcs->dwSize = bytes;
			rcs->dwVersion = 0;
			rcs->dwStrLen = static_cast<DWORD>(s.length());
			rcs->dwStrOffset = sizeof(RECONVERTSTRING);
			rcs->dwCompStrLen = rcs->dwTargetStrLen =
				static_cast<DWORD>(multilineSelection ? s.length() : (offsetInLine(caret.end()) - offsetInLine(caret.beginning())));
			rcs->dwCompStrOffset = rcs->dwTargetStrOffset =
				multilineSelection ? 0 : static_cast<DWORD>(sizeof(Char) * offsetInLine(caret.beginning()));
			s.copy(reinterpret_cast<Char*>(reinterpret_cast<char*>(rcs) + rcs->dwStrOffset), s.length());

			if(isSelectionEmpty(caret)) {
				// IME selects the composition target automatically if no selection
				if(win32::boole(::ImmSetCompositionStringW(imc, SCS_QUERYRECONVERTSTRING, rcs, rcs->dwSize, nullptr, 0))) {
					caret.select(
						Position(line(caret), rcs->dwCompStrOffset / sizeof(Char)),
						Position(line(caret), rcs->dwCompStrOffset / sizeof(Char) + rcs->dwCompStrLen));
					if(win32::boole(::ImmSetCompositionStringW(imc, SCS_SETRECONVERTSTRING, rcs, rcs->dwSize, nullptr, 0)))
						succeeded = true;
				}
			}
			::operator delete(rcs);
		}
	}

	utils::closeCompletionProposalsPopup(viewer);
	return succeeded;
}

/**
 * Constructor.
 * @param viewer The target text viewer
 * @param onlySelection
 * @param callback
 */
ReplaceAllCommand::ReplaceAllCommand(TextViewer& viewer, bool onlySelection,
		const String& replacement, searcher::InteractiveReplacementCallback* callback) /*throw()*/
		: Command(viewer), onlySelection_(onlySelection), replacement_(replacement), callback_(callback) {
}

/**
 * Replaces all matched texts. This does not freeze the text viewer.
 * @return The number of replced strings
 * @throw ... Any exceptions @c searcher::TextSearcher::replaceAll throws other than
 *            @c ReplacementInterruptedException&lt;IDocumentInput#ChangeRejectedException&gt;
 */
bool ReplaceAllCommand::perform() {
	abortModes(target());
    if(onlySelection_ && isSelectionEmpty(target().caret()))
		return false;

	using namespace searcher;
	win32::WaitCursor wc;
	TextViewer& viewer = target();
	Document& document = viewer.document();
	TextSearcher* s;
	if(Session* const session = document.session())
		s = &session->textSearcher();
	else
		return false;	// TODO: prepares a default text searcher.

	Region scope(
		onlySelection_ ? max<Position>(viewer.caret().beginning(),
			document.accessibleRegion().first) : document.accessibleRegion().first,
		onlySelection_ ? min<Position>(viewer.caret().end(),
			document.accessibleRegion().second) : document.accessibleRegion().second);

	// mark to restore the selection later
	Point oldAnchor(document, viewer.caret().anchor());
	Point oldCaret(document, viewer.caret());

	AutoFreeze af(&viewer);
	try {
		numberOfLastReplacements_ = s->replaceAll(document, scope, replacement_, callback_);
	} catch(const ReplacementInterruptedException<DocumentInput::ChangeRejectedException>& e) {
		numberOfLastReplacements_ = e.numberOfReplacements();
		throw;
	} catch(const ReplacementInterruptedException<bad_alloc>& e) {
		numberOfLastReplacements_ = e.numberOfReplacements();
		throw;
	}
	if(numberOfLastReplacements_ != 0)
		viewer.caret().select(oldAnchor, oldCaret);
	return true;
}

/**
 * Constructor.
 * @param viewer The target text viewer
 * @param procedure A function gives a motion
 * @param direction The direction of motion
 * @throw NullPointerException @a procedure is @c null
 */
template<typename ProcedureSignature>
RowSelectionExtensionCommand<ProcedureSignature>::RowSelectionExtensionCommand(
		TextViewer& viewer, ProcedureSignature* procedure, Direction direction)
		: Command(viewer), procedure_(procedure), direction_(direction) {
	if(procedure == nullptr)
		throw NullPointerException("procedure");
}

/**
 * @see Command#perform
 * @return true
 */
template<typename ProcedureSignature>
bool RowSelectionExtensionCommand<ProcedureSignature>::perform() {
	utils::closeCompletionProposalsPopup(target());
	endIncrementalSearch(target());

	Caret& caret = target().caret();
	if(isSelectionEmpty(caret) && !caret.isSelectionRectangle())
		caret.beginRectangleSelection();
	return CaretMovementCommand<ProcedureSignature>(target(), procedure_, direction_, true).setNumericPrefix(numericPrefix())();
}

// explicit instantiations
template class RowSelectionExtensionCommand<Position(const Point&, Direction, Index)>;	// next(Line|Word|WordEnd)
template class RowSelectionExtensionCommand<boost::optional<Position>(const Point&, Direction, Index)>;	// nextBookmark
template class RowSelectionExtensionCommand<Position(const Point&, Direction, locations::CharacterUnit, Index)>;	// nextCharacter
template class RowSelectionExtensionCommand<VisualDestinationProxy(const VisualPoint&, Direction, Index)>;	// next(Page|VisualLine)

/**
 * Constructor.
 * @param viewer The target text viewer
 * @param procedure A function gives a motion
 * @throw NullPointerException @a procedure is @c null
 * 
 */
template<typename ProcedureSignature>
RowSelectionExtensionToDefinedPositionCommand<ProcedureSignature>::RowSelectionExtensionToDefinedPositionCommand(
		TextViewer& viewer, ProcedureSignature* procedure) : Command(viewer), procedure_(procedure) {
	if(procedure == nullptr)
		throw NullPointerException("procedure");
}

// explicit instantiations
template class RowSelectionExtensionToDefinedPositionCommand<Position(const Point&)>;	// (beginning|end)Of(Document|Line)
template class RowSelectionExtensionToDefinedPositionCommand<Position(const VisualPoint&)>;	// contextual(Beginning|End)OfLine, (beginning|end|contextualBeginning|contextualEnd)OfVisualLine, (first|last)PrintableCharacterOf(Visual)?Line

/**
 * Moves the caret or extends the selection.
 * @return true
 */
template<typename ProcedureSignature>
bool RowSelectionExtensionToDefinedPositionCommand<ProcedureSignature>::perform() {
	utils::closeCompletionProposalsPopup(target());
	endIncrementalSearch(target());

	Caret& caret = target().caret();
	if(isSelectionEmpty(caret) && !caret.isSelectionRectangle())
		caret.beginRectangleSelection();
	return CaretMovementToDefinedPositionCommand<ProcedureSignature>(target(), procedure_, true).setNumericPrefix(numericPrefix())();
}

/**
 * Constructor.
 * @param viewer The target text viewer
 * @param untabify Set @c true to untabify rather than tabify
 */
TabifyCommand::TabifyCommand(TextViewer& viewer, bool untabify) /*throw()*/ : Command(viewer), untabify_(untabify) {
}

/**
 * Implements @c Command#perform.
 * @note Not implemented.
 */
bool TabifyCommand::perform() {
	ASCENSION_CHECK_DOCUMENT_READ_ONLY();
//	ASCENSION_CHECK_GUI_EDITABILITY(1);
	abortModes(target());
	// TODO: not implemented.
	return false;
}

/**
 * Constructor.
 * @param viewer The target text viewer
 * @param text The text to input. This can be empty or ill-formed UTF-16 sequence
 */
TextInputCommand::TextInputCommand(TextViewer& viewer, const String& text) /*throw()*/ : Command(viewer), text_(text) {
}

namespace {
	inline String multiplyString(const String& s, size_t n) {
		const size_t len = s.length();
		String temp;
		temp.reserve(n * len);
		for(size_t i = 0; i < n; ++i)
			temp.append(s.data(), len);
		return temp;
	}
}

/**
 * Inserts a text. If the incremental search is active, appends a string to the end of the pattern.
 * @retval false The change was rejected
 * @throw ... Any exceptions @c searcher#IncrementalSearcher#addString and
 *            @c viewers#replaceSelection throw other than
 *            @c kernel#IDocumentInput#ChangeRejectedException
 */
bool TextInputCommand::perform() {
	const NumericPrefix n(numericPrefix());
	if(n == 0)
		return true;

	if(Session* const session = target().document().session()) {
		if(session->incrementalSearcher().isRunning()) {
			session->incrementalSearcher().addString((n > 1) ? multiplyString(text_, static_cast<size_t>(n)) : text_);
			return true;
		}
	}

	ASCENSION_CHECK_DOCUMENT_READ_ONLY();
//	ASCENSION_CHECK_GUI_EDITABILITY();
	if(n > 1) {
		try {
			target().caret().replaceSelection(multiplyString(text_, static_cast<size_t>(n)));
		} catch(const DocumentInput::ChangeRejectedException&) {
			return false;
		}
	} else {
		try {
			target().caret().replaceSelection(text_);
		} catch(const DocumentInput::ChangeRejectedException&) {
			return false;
		}
	}
	return true;
}

/**
 * Constructor.
 * @param viewer The target text viewer
 * @param procedure Indicates what to transpose. This must be one of:
 *                  @c EditPoint#transposeCharacters, @c EditPoint#transposeWords,
 *                  @c EditPoint#transposeLines
 * @throw std#invalid_argument
 */
TranspositionCommand::TranspositionCommand(TextViewer& viewer, bool(*procedure)(Caret&)) : Command(viewer), procedure_(procedure) {
	if(procedure_ != &transposeCharacters
			&& procedure_ != &transposeWords
			&& procedure_ != &transposeLines)
		throw invalid_argument("procedure");
}

/**
 * Implements @c Command#perform by using a transposition method of @c viewers#Caret class. 
 * @return false The internal transposition method call returned @c false
 * @throw ... Any exceptions the transposition method returns other than
 *            @c kernel#ReadOnlyDocumentException and @c kernel#DocumentCantChangeException
 */
bool TranspositionCommand::perform() {
	ASCENSION_CHECK_DOCUMENT_READ_ONLY();
//	ASCENSION_CHECK_GUI_EDITABILITY();
	TextViewer& viewer = target();
	endIncrementalSearch(viewer);
	utils::closeCompletionProposalsPopup(viewer);

	Caret& caret = viewer.caret();
	try {
		AutoFreeze af(&viewer);
		viewer.document().insertUndoBoundary();
		const bool succeeded = (*procedure_)(caret);
		viewer.document().insertUndoBoundary();
		return succeeded;
	} catch(const DocumentInput::ChangeRejectedException&) {
		return false;
	}
}

/**
 * Constructor.
 * @param viewer The target text viewer
 * @param redo Set @c true to perform redo, rather than undo
 */
UndoCommand::UndoCommand(TextViewer& viewer, bool redo) /*throw()*/ : Command(viewer), redo_(redo), lastResult_(INDETERMINATE) {
}

/**
 * Returns @c true if the last performance was done incompletely.
 * @throw IllegalStateException The command has never performed
 * @see Document#undo, Document#redo
 */
bool UndoCommand::isLastActionIncompleted() const {
	if(lastResult_ == INDETERMINATE)
		throw IllegalStateException("this command has never performed.");
	return lastResult_ == INCOMPLETED;
}

/**
 * Undo or redo.
 * @retval false The change was rejected
 */
bool UndoCommand::perform() {
//	ASCENSION_CHECK_GUI_EDITABILITY(1);
	if(numericPrefix() < 0)
		setNumericPrefix(0);	// currently, this is no-op

	win32::WaitCursor wc;
	Document& document = target().document();
	bool (Document::*performance)(size_t) = !redo_ ? &Document::undo : &Document::redo;
	size_t (Document::*number)() const = !redo_ ? &Document::numberOfUndoableChanges : &Document::numberOfRedoableChanges;
	try {
		lastResult_ = (document.*performance)(min(static_cast<size_t>(numericPrefix()), (document.*number)())) ? COMPLETED : INCOMPLETED;
	} catch(DocumentCantChangeException&) {
		return false;
	}
	return true;
}

/**
 * Constructor.
 * @param viewer The target text viewer
 * @param direction The direcion to delete
 */
WordDeletionCommand::WordDeletionCommand(TextViewer& viewer, Direction direction) /*throw()*/ : Command(viewer), direction_(direction) {
}

/**
 * Implements @c Command#perform.
 * @retval false The document's input rejected the change
 */
bool WordDeletionCommand::perform() {
	long n = numericPrefix();
	if(n == 0)
		return true;
	ASCENSION_CHECK_DOCUMENT_READ_ONLY();
	TextViewer& viewer = target();
	abortIncrementalSearch(viewer);

	Caret& caret = viewer.caret();
	if(/*caret.isAutoCompletionRunning() &&*/ direction_ == Direction::FORWARD)
		utils::closeCompletionProposalsPopup(viewer);

	Document& document = viewer.document();
	const Position from((direction_ == Direction::FORWARD) ? caret.beginning() : caret.end());
	text::WordBreakIterator<DocumentCharacterIterator> to(
		DocumentCharacterIterator(document, (direction_ == Direction::FORWARD) ? caret.end() : caret.beginning()),
		text::AbstractWordBreakIterator::START_OF_SEGMENT,
			viewer.document().contentTypeInformation().getIdentifierSyntax(contentType(caret)));
	for(Position p(to.base().tell()); n > 0; --n) {
		if(p == ((direction_ == Direction::FORWARD) ? ++to : --to).base().tell())
			break;
		p = to.base().tell();
	}
	if(to.base().tell() != from) {
		try {
			AutoFreeze af(&viewer);
			document.insertUndoBoundary();
			erase(document, from, to.base().tell());
			caret.moveTo(min(from, to.base().tell()));
			document.insertUndoBoundary();
		} catch(const DocumentInput::ChangeRejectedException&) {
			return false;
		}
	}
	return true;
}

/**
 * Constructor.
 * @param viewer The target text viewer
 */
WordSelectionCreationCommand::WordSelectionCreationCommand(TextViewer& viewer) /*throw()*/ : Command(viewer) {
}

/**
 * Implements @c Command#perform.
 * @return true
 */
bool WordSelectionCreationCommand::perform() {
	endIncrementalSearch(target());
	target().caret().endRectangleSelection();
	selectWord(target().caret());
	return 0;
}

#undef ASCENSION_ASSERT_IFISWINDOW
#undef ASCENSION_CHECK_DOCUMENT_READ_ONLY
//#undef ASCENSION_CHECK_GUI_EDITABILITY
