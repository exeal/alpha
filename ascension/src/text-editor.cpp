/**
 * @file text-editor.cpp
 * @author exeal
 * @date 2006-2009
 */

#include <ascension/text-editor.hpp>
#include <ascension/content-assist.hpp>
#include <manah/win32/utility.hpp>
#include <manah/win32/ui/wait-cursor.hpp>
using namespace ascension;
using namespace ascension::kernel;
using namespace ascension::texteditor;
using namespace ascension::texteditor::commands;
using namespace ascension::texteditor::isc;
using namespace ascension::viewers;
using namespace manah;
using namespace manah::win32;
using namespace std;
using manah::win32::ui::WaitCursor;


// Command //////////////////////////////////////////////////////////////////

/**
 * Protected constructor enables the beep-on-error mode.
 * @param viewer the target text viewer
 */
Command::Command(TextViewer& viewer) /*throw()*/ : viewer_(&viewer), numericPrefix_(1), beepsOnError_(true) {
}

/// Destructor.
Command::~Command() /*throw()*/ {
}


// commands.* ///////////////////////////////////////////////////////////////

#define ASSERT_IFISWINDOW()	assert(target().isWindow())
#define ABORT_ISEARCH()												\
	if(Session* const session = target().document().session()) {	\
		if(session->incrementalSearcher().isRunning())				\
			session->incrementalSearcher().abort();					\
	}
#define END_ISEARCH()												\
	if(Session* const session = target().document().session()) {	\
		if(session->incrementalSearcher().isRunning())				\
			session->incrementalSearcher().end();					\
	}
#define CHECK_DOCUMENT_READONLY(retval)		\
	if(target().document().isReadOnly())	\
		return retval
#define CLOSE_COMPLETION_PROPOSAL_POPUP()																			\
	if(contentassist::IContentAssistant* const ca = target().contentAssistant()) {									\
		if(contentassist::IContentAssistant::ICompletionProposalsUI* const cpui = ca->getCompletionProposalsUI())	\
			cpui->close();																							\
	}
#define ABORT_MODES()					\
	CLOSE_COMPLETION_PROPOSAL_POPUP();	\
	ABORT_ISEARCH()

namespace {
	typedef Position(Caret::*MovementProcedure0)(void) const;
	static MovementProcedure0 MOVEMENT_PROCEDURES_0[] = {
		&Caret::beginningOfDocument, &Caret::beginningOfLine, &Caret::beginningOfVisualLine,
		&Caret::contextualBeginningOfLine, &Caret::contextualBeginningOfVisualLine, &Caret::contextualEndOfVisualLine,
		&Caret::contextualEndOfLine, &Caret::endOfDocument, &Caret::endOfLine, &Caret::endOfVisualLine,
		&Caret::firstPrintableCharacterOfLine, &Caret::firstPrintableCharacterOfVisualLine,
		&Caret::lastPrintableCharacterOfLine, &Caret::lastPrintableCharacterOfVisualLine
	};
	typedef Position(Caret::*const MovementProcedure1)(length_t) const;
	static MovementProcedure1 MOVEMENT_PROCEDURES_1[] = {
		&Caret::backwardBookmark, &Caret::backwardCharacter, &Caret::backwardLine, &Caret::backwardWord,
		&Caret::backwardWordEnd, &Caret::forwardBookmark, &Caret::forwardCharacter, &Caret::forwardLine,
		&Caret::forwardWord, &Caret::forwardWordEnd, &Caret::leftCharacter, &Caret::leftWord, &Caret::leftWordEnd,
		&Caret::rightCharacter, &Caret::rightWord, &Caret::rightWordEnd
	};
	typedef VerticalDestinationProxy(Caret::*MovementProcedureV1)(length_t) const;
	static MovementProcedureV1 MOVEMENT_PROCEDURES_V_1[] = {
		&Caret::backwardPage, &Caret::backwardVisualLine, &Caret::forwardPage, &Caret::forwardVisualLine
	};
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param region the region to operate on. if empty, the accessible region of the document. this
 * region will be shrunk to the accessible region when the command performed
 */
BookmarkMatchLinesCommand::BookmarkMatchLinesCommand(TextViewer& viewer,
		const Region& region /* = Region() */) /*throw()*/ : Command(viewer), region_(region) {
}

/**
 * @see Command#perform
 * @return the number of marked lines
 */
ulong BookmarkMatchLinesCommand::perform() {
	WaitCursor wc;
	TextViewer& viewer = target();
	Document& document = viewer.document();
	const searcher::TextSearcher* s;
	if(const Session* const session = document.session())
		s = &session->textSearcher();
	else
		return 0;	// TODO: prepares a default text searcher.

	ulong count = 0;
	Region scope(region_.isEmpty() ? document.accessibleRegion() : region_.getIntersection(document.accessibleRegion()));

	Bookmarker& bookmarker = document.bookmarker();
	Region matchedRegion;
	while(s->search(document,
			max<Position>(viewer.caret().beginning(), document.accessibleRegion().first),
			scope, Direction::FORWARD, matchedRegion)) {
		bookmarker.mark(matchedRegion.first.line);
		scope.first.line = matchedRegion.first.line + 1;
		scope.first.column = 0;
		++count;
	}
	return count;
}

/**
 * Constructor.
 * @param viewer the target text viewer
 */
CancelCommand::CancelCommand(TextViewer& viewer) /*throw()*/ : Command(viewer) {
}

/**
 * @see Command#perform
 * @return 0
 */
ulong CancelCommand::perform() {
	ASSERT_IFISWINDOW();
	ABORT_MODES();
	target().caret().clearSelection();
	return 0;
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param procedure a pointer to the member function defines the destination
 * @param extendSelection set true to extend selection
 */
CaretMovementCommand::CaretMovementCommand(TextViewer& viewer,
		Position(Caret::*procedure)(void) const, bool extendSelection /* = false */) :
		Command(viewer), extends_(extendSelection), procedure0_(procedure), procedure1_(0), procedureV1_(0) {
	if(procedure == 0 || find(MOVEMENT_PROCEDURES_0,
			MANAH_ENDOF(MOVEMENT_PROCEDURES_0), procedure) == MANAH_ENDOF(MOVEMENT_PROCEDURES_0))
		throw invalid_argument("procedure");
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param procedure a pointer to the member function defines the destination
 * @param extendSelection set true to extend selection
 */
CaretMovementCommand::CaretMovementCommand(TextViewer& viewer,
		Position(Caret::*procedure)(length_t) const, bool extendSelection /* = false */) :
		Command(viewer), extends_(extendSelection), procedure0_(0), procedure1_(procedure), procedureV1_(0) {
	if(procedure == 0 || find(MOVEMENT_PROCEDURES_1,
			MANAH_ENDOF(MOVEMENT_PROCEDURES_1), procedure) == MANAH_ENDOF(MOVEMENT_PROCEDURES_1))
		throw invalid_argument("procedure");
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param procedure a pointer to the member function defines the destination
 * @param extendSelection set true to extend selection
 */
CaretMovementCommand::CaretMovementCommand(TextViewer& viewer,
		VerticalDestinationProxy(Caret::*procedure)(length_t) const, bool extendSelection /* = false */) :
		Command(viewer), extends_(extendSelection), procedure0_(0), procedure1_(0), procedureV1_(procedure) {
	if(procedure == 0 || find(MOVEMENT_PROCEDURES_V_1,
			MANAH_ENDOF(MOVEMENT_PROCEDURES_V_1), procedure) == MANAH_ENDOF(MOVEMENT_PROCEDURES_V_1))
		throw invalid_argument("procedure");
}

/**
 * Moves the caret or extends the selection.
 * @retval 1 the type is any of @c MATCH_BRACKET, @c NEXT_BOOKMARK, or @c PREVIOUS_BOOKMARK and the next mark not found
 * @retval 0 otherwise
 */
ulong CaretMovementCommand::perform() {
	const long n = numericPrefix();
	END_ISEARCH();
	if(n == 0)
		return 0;
	Caret& caret = target().caret();

	if(!extends_) {
		if(procedure1_ == &Caret::forwardLine || procedure1_ == &Caret::backwardLine
				|| procedureV1_ == &Caret::forwardVisualLine || procedureV1_ == &Caret::backwardVisualLine
				|| procedureV1_ == &Caret::forwardPage || procedureV1_ == &Caret::backwardPage) {
			if(contentassist::IContentAssistant* const ca = target().contentAssistant()) {
				if(contentassist::IContentAssistant::ICompletionProposalsUI* const cpui = ca->getCompletionProposalsUI()) {
					if(procedure1_ == &Caret::forwardLine || procedureV1_ == &Caret::forwardVisualLine)
						cpui->nextProposal(n);
					else if(procedure1_ == &Caret::backwardLine || procedureV1_ == &Caret::backwardVisualLine)
						cpui->nextProposal(n);
					else if(procedureV1_ == &Caret::forwardPage)
						cpui->nextPage(n);
					else if(procedureV1_ == &Caret::backwardPage)
						cpui->nextPage(n);
					return 0;
				}
			}
		}
		caret.endRectangleSelection();
		if(!caret.isSelectionEmpty()) {	// just clear the selection
			const bool rtl = target().configuration().orientation == layout::RIGHT_TO_LEFT;
			if(procedure1_ == &Caret::forwardCharacter
					|| (procedure1_ == &Caret::rightCharacter && !rtl)
					|| (procedure1_ == &Caret::leftCharacter && rtl)) {
				caret.moveTo(caret.end());
				return 0;
			} else if(procedure1_ == &Caret::backwardCharacter
					|| (procedure1_ == &Caret::leftCharacter && !rtl)
					|| (procedure1_ == &Caret::rightCharacter && rtl)) {
				caret.moveTo(caret.beginning());
				return 0;
			}
		}
	}

	// TODO: consider the numeric prefix.
	if(procedureV1_ == &Caret::forwardPage)
		target().sendMessage(WM_VSCROLL, SB_PAGEDOWN);
	else if(procedureV1_ == &Caret::backwardPage)
		target().sendMessage(WM_VSCROLL, SB_PAGEUP);

	if(procedureV1_ != 0) {
		if(!extends_)
			caret.moveTo((caret.*procedureV1_)(n));
		else
			caret.extendSelection((caret.*procedureV1_)(n));
	} else {
		const Position destination((procedure0_ != 0) ? (caret.*procedure0_)() : (caret.*procedure1_)(n));
		if(!extends_)
			caret.moveTo(destination);
		else
			caret.extendSelection(destination);
	}
	return 0;
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param direction the direcion to delete
 */
CharacterDeletionCommand::CharacterDeletionCommand(TextViewer& viewer,
		Direction direction) /*throw()*/ : Command(viewer), direction_(direction) {
}

/**
 * @see Command#perform
 * @retval 0 succeeded
 * @retval 1 failed
 */
ulong CharacterDeletionCommand::perform() {
	long n = numericPrefix();
	if(n == 0)
		return 0;
	CHECK_DOCUMENT_READONLY(1);
	TextViewer& viewer = target();
	Caret& caret = viewer.caret();
	const bool forward = (direction_ == Direction::FORWARD && n > 0) || (direction_ == Direction::BACKWARD && n < 0);
	n = abs(n);
	if(/*caret.isAutoCompletionRunning() &&*/ forward)
		CLOSE_COMPLETION_PROPOSAL_POPUP();

	Document& document = viewer.document();
	searcher::IncrementalSearcher* isearch = 0;
	if(Session* const session = document.session())
		isearch = &session->incrementalSearcher();
	if(isearch != 0 && isearch->isRunning()) {
		if(forward)
			isearch->reset();
		else {
			if(!isearch->canUndo()) {
				if(beepsOnError())
					viewer.beep();
			} else {
				isearch->undo();
				for(--n; n > 0 && isearch->canUndo(); --n)
					isearch->undo();
			}
		}
	} else {
		bool succeeded;
		document.insertUndoBoundary();
		if(n == 1 && !caret.isSelectionEmpty())
			succeeded = caret.eraseSelection();
		else {
			const bool frozen = !caret.isSelectionEmpty() || n > 1;
			if(frozen)
				viewer.freeze();

			try {
				// first of all, delete the selected text
				if(!caret.isSelectionEmpty()) {
					caret.eraseSelection();
					--n;
				}
				if(n > 0) {
					if(forward)
						caret.erase(n, EditPoint::GRAPHEME_CLUSTER);
					else
						caret.erase(-1, EditPoint::UTF32_CODE_UNIT);
				}
			} catch(...) {
			}

			if(frozen)
				viewer.unfreeze();
		}
	}
	return 0;
}

/**
 * Constructor.
 * @param viewer the target text viewer
 */
CharacterToCodePointConversionCommand::CharacterToCodePointConversionCommand(TextViewer& viewer) /*throw()*/ : Command(viewer) {
}

/**
 * @see Command#perform
 * @retval 0 succeeded
 * @retval 1 failed
 */
ulong CharacterToCodePointConversionCommand::perform() {
	CHECK_DOCUMENT_READONLY(1);
	ABORT_MODES();

	TextViewer& viewer = target();
	const Document& document = viewer.document();
	const EditPoint& bottom = viewer.caret().end();
	if(bottom.isBeginningOfLine()
			|| (document.isNarrowed() && bottom.position() == document.accessibleRegion().first)) {	// 行頭以外でなければならぬ
		if(beepsOnError())
			viewer.beep();
		return 1;
	}

	Caret& caret = viewer.caret();
	const Char* const line = document.line(bottom.lineNumber()).data();
	const CodePoint cp = text::surrogates::decodeLast(line, line + bottom.columnNumber());
	Char buffer[7];
#if(_MSC_VER < 1400)
	swprintf(buffer, L"%lX", cp);
#else
	swprintf(buffer, MANAH_COUNTOF(buffer), L"%lX", cp);
#endif // _MSC_VER < 1400
	viewer.freeze();
	try {
		caret.select(Position(bottom.lineNumber(), bottom.columnNumber() - ((cp > 0xFFFF) ? 2 : 1)), bottom);
		caret.replaceSelection(buffer, buffer + wcslen(buffer), false);
	} catch(...) {
		viewer.unfreeze();
		return 1;
	}
	viewer.unfreeze();
	return 0;
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param c the code point of the character to input
 * @throw invalid_argument @a c is not valid Unicode scalar value
 */
CharacterInputCommand::CharacterInputCommand(TextViewer& viewer, CodePoint c) : Command(viewer), c_(c) {
	if(!text::isScalarValue(c))
		throw invalid_argument("the given code point is not valid Unicode scalar value.");
}

/**
 * @see Command#perform
 * @retval 1 failed and the incremental search is not active
 * @retval 0 otherwise
 * @see Caret#inputCharacter, TextViewer#onChar, TextViewer#onUniChar
 */
ulong CharacterInputCommand::perform() {
	const long n = numericPrefix();
	if(n <= 0)
		return 0;
	else if(n == 1) {
		if(Session* const session = target().document().session()) {
			if(session->incrementalSearcher().isRunning()) {
				CLOSE_COMPLETION_PROPOSAL_POPUP();
				if(c_ == 0x0009 || !toBoolean(iswcntrl(static_cast<wint_t>(c_))))
					session->incrementalSearcher().addCharacter(c_);
				return 0;
			}
		}
		return target().caret().inputCharacter(c_) ? 1 : 0;
	} else {
		String s;
		text::surrogates::encode(c_, back_inserter(s));
		return TextInputCommand(target(), s).setNumericPrefix(numericPrefix())();
	}
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param fromPreviousLine set true to use a character on the previous visual line. otherwise one
 * on the next visual line is used
 */
CharacterInputFromNextLineCommand::CharacterInputFromNextLineCommand(
		TextViewer& viewer, bool fromPreviousLine) /*throw()*/ : Command(viewer), fromPreviousLine_(fromPreviousLine) {
}

/**
 * @see Command#execute
 * @retval 0 succeeded
 * @retval 1 failed
 */
ulong CharacterInputFromNextLineCommand::perform() {
	ABORT_ISEARCH();
	CHECK_DOCUMENT_READONLY(1);

	// TODO: recognizes narrowing.

	const Document& document = target().document();
	const VisualPoint& caret = target().caret();

	if((fromPreviousLine_ && caret.lineNumber() > 0)
			|| (!fromPreviousLine_ && caret.lineNumber() < document.numberOfLines() - 1)) {
		// calculate column position
		VisualPoint p(caret);
		p.adaptToDocument(false);
		if(fromPreviousLine_)
			p.backwardVisualLine();
		else
			p.forwardVisualLine();

		const length_t column = p.columnNumber();
		const String& line = document.line(caret.lineNumber() + (fromPreviousLine_ ? -1 : 1));
		if(column < line.length())
			return CharacterInputCommand(target(), text::surrogates::decodeFirst(line.begin() + column, line.end()))();
	}
	if(beepsOnError())
		target().beep();
	return 1;
}

/**
 * Constructor.
 * @param viewer the target text viewer
 */
CodePointToCharacterConversionCommand::CodePointToCharacterConversionCommand(TextViewer& viewer) /*throw()*/ : Command(viewer) {
}

/**
 * @see Command#perform
 * @retval 0 succeeded
 * @retval 1 failed
 */
ulong CodePointToCharacterConversionCommand::perform() {
	CHECK_DOCUMENT_READONLY(1);
	ABORT_MODES();

	TextViewer& viewer = target();
	const Document& document = viewer.document();
	const EditPoint& bottom = viewer.caret().end();
	if(bottom.isBeginningOfLine()
			|| (document.isNarrowed() && bottom.position() == document.accessibleRegion().first)) {	// 行頭以外でなければならぬ
		if(beepsOnError())
			viewer.beep();
		return 1;
	}

	Caret& caret = viewer.caret();
	const Char* const line = document.line(bottom.lineNumber()).data();
	const length_t column = bottom.columnNumber();

	// accept /(?:[Uu]\+)?[0-9A-Fa-f]{1,6}/
	if(toBoolean(iswxdigit(line[column - 1]))) {
		length_t i = column - 1;
		while(i != 0) {
			if(column - i == 7) {
				if(beepsOnError())
					viewer.beep();
				return 1;
			} else if(!toBoolean(iswxdigit(line[i - 1])))
				break;
			--i;
		}

		Char buffer[7];
		wcsncpy(buffer, line + i, column - i);
		buffer[column - i] = 0;
		const CodePoint cp = wcstoul(buffer, 0, 16);
		if(text::isValidCodePoint(cp)) {
			buffer[1] = buffer[2] = 0;
			text::surrogates::encode(cp, buffer);
			if(i >= 2 && line[i - 1] == L'+' && (line[i - 2] == L'U' || line[i - 2] == L'u'))
				i -= 2;
			viewer.freeze();
			bool succeeded = true;
			try {
				caret.select(Position(bottom.lineNumber(), i), bottom);
				caret.replaceSelection(buffer, buffer + (cp < 0x10000U ? 1 : 2), false);
			} catch(...) {
				succeeded = false;
			}
			viewer.unfreeze();
			if(succeeded)
				return 0;
		}
	}
	if(beepsOnError())
		viewer.beep();
	return 1;
}

/**
 * Constructor.
 * @param viewer the target text viewer
 */
CompletionProposalPopupCommand::CompletionProposalPopupCommand(TextViewer& viewer) /*throw()*/ : Command(viewer) {
}

/**
 * @see Command#perform
 * @retval 0 succeeded
 * @retval 1 failed
 */
ulong CompletionProposalPopupCommand::perform() {
	CHECK_DOCUMENT_READONLY(1);
//	CHECK_GUI_EDITABILITY(1);
	ABORT_ISEARCH();
	if(contentassist::IContentAssistant* ca = target().contentAssistant()) {
		ca->showPossibleCompletions();
		return 0;
	}
	if(beepsOnError())
		target().beep();
	return 1;
}

/**
 * Constructor.
 * @param viewer the target text viewer
 */
EntireDocumentSelectionCreationCommand::EntireDocumentSelectionCreationCommand(TextViewer& viewer) /*throw()*/ : Command(viewer) {
}

/**
 * @see Command#perform
 * @return 0
 */
ulong EntireDocumentSelectionCreationCommand::perform() {
	END_ISEARCH();
	target().caret().endRectangleSelection();
	target().caret().select(target().document().accessibleRegion());
	return 0;
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param direction the direction to search
 */
FindNextCommand::FindNextCommand(TextViewer& viewer, Direction direction) /*throw()*/ : Command(viewer), direction_(direction) {
}

/**
 * @see Command#perform
 * @return 1 if no text matched or the command failed. otherwise 0
 */
ulong FindNextCommand::perform() {
	long n = numericPrefix();
	if(n == 0)
		return 0;
	END_ISEARCH();
	CLOSE_COMPLETION_PROPOSAL_POPUP();

	WaitCursor wc;
	Document& document = target().document();
	const searcher::TextSearcher* s;
	if(const Session* const session = document.session())
		s = &session->textSearcher();
	else
		return 1;	// TODO: prepares a default text searcher.
	const bool forward = (direction_ == Direction::FORWARD && n > 0) || (direction_ == Direction::BACKWARD && n < 0);
	n = abs(n);

	Caret& caret = target().caret();
	const Region scope(document.accessibleRegion());
	Region matchedRegion(caret.selectionRegion());
	bool foundOnce = false;
	for(; n > 0; --n) {	// search N times
		if(!s->search(document, forward ?
				max<Position>(matchedRegion.end(), scope.first)
				: min<Position>(matchedRegion.beginning(), scope.second),
				scope, forward ? Direction::FORWARD : Direction::BACKWARD, matchedRegion))
			break;
		foundOnce = true;
	}

	if(foundOnce) {
		caret.select(matchedRegion);
//		viewer.highlightMatchTexts();
		return 0;
	} else {
//		viewer.highlightMatchTexts(false);
		return 1;
	}
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param direction the direction to search
 * @param callback the callback object for the incremental search. can be @c null
 */
IncrementalFindCommand::IncrementalFindCommand(TextViewer& viewer, Direction direction,
		searcher::IIncrementalSearchCallback* callback /* = 0 */) /*throw()*/ : Command(viewer), direction_(direction), callback_(callback) {
}

/**
 * @see Command#perform
 * @retval 0
 */
ulong IncrementalFindCommand::perform() {
	long n = numericPrefix();
	if(n == 0)
		return 0;
	CLOSE_COMPLETION_PROPOSAL_POPUP();
	if(Session* const session = target().document().session()) {
		const Direction realDirection = (n > 0) ? direction_ : !direction_;
		n = abs(n);
		searcher::IncrementalSearcher& isearch = session->incrementalSearcher();
		if(!isearch.isRunning()) {	// begin the search if not running
			isearch.start(target().document(), target().caret(), session->textSearcher(), realDirection, callback_);
			--n;
		}
		for(; n > 0; --n) {	// jump N times
			if(!isearch.next(realDirection)) {
				if(beepsOnError())
					target().beep();
				break;
			}
		}
	}
	return 1;
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param increase set true to increase the indentation
 */
IndentationCommand::IndentationCommand(TextViewer& viewer, bool increase) /*throw()*/ : Command(viewer), increases_(increase) {
}

/**
 * @see Command#perform
 * @retval 0 succeeded
 * @retval 1 failed
 */
ulong IndentationCommand::perform() {
	const long n = numericPrefix();
	if(n == 0)
		return 0;
	CHECK_DOCUMENT_READONLY(1);
//	CHECK_GUI_EDITABILITY(1);
	END_ISEARCH();
	CLOSE_COMPLETION_PROPOSAL_POPUP();

	TextViewer& viewer = target();
	Caret& caret = viewer.caret();
	viewer.document().insertUndoBoundary();
	viewer.freeze();
	const Position anchorResult(caret.tabIndent(caret.anchor(), caret.isSelectionRectangle(), n));
	viewer.document().insertUndoBoundary();
	caret.select(anchorResult, caret);
	viewer.unfreeze();

	return 0;
}

/**
 * Constructor.
 * @param viewer the target text viewer
 */
InputMethodOpenStatusToggleCommand::InputMethodOpenStatusToggleCommand(TextViewer& viewer) /*throw()*/ : Command(viewer) {
}

/**
 * @see Command#perform
 * @retval 0 succeeded
 * @retval 1 failed
 */
ulong InputMethodOpenStatusToggleCommand::perform() {
	if(HIMC imc = ::ImmGetContext(target().get())) {
		const bool succeeded = toBoolean(::ImmSetOpenStatus(imc, !toBoolean(::ImmGetOpenStatus(imc))));
		::ImmReleaseContext(target().get(), imc);
		return succeeded ? 0 : 1;
	}
	return 1;
}

/**
 * Constructor.
 * @param viewer the target text viewer
 */
InputMethodSoftKeyboardModeToggleCommand::InputMethodSoftKeyboardModeToggleCommand(TextViewer& viewer) /*throw()*/ : Command(viewer) {
}

/**
 * @see Command#perform
 * @retval 0 succeeded
 * @retval 1 failed
 */
ulong InputMethodSoftKeyboardModeToggleCommand::perform() {
	if(HIMC imc = ::ImmGetContext(target().get())) {
		DWORD conversionMode, sentenceMode;
		if(toBoolean(::ImmGetConversionStatus(imc, &conversionMode, &sentenceMode))) {
			conversionMode = toBoolean(conversionMode & IME_CMODE_SOFTKBD) ?
				(conversionMode & ~IME_CMODE_SOFTKBD) : (conversionMode | IME_CMODE_SOFTKBD);
			const bool succeeded = toBoolean(::ImmSetConversionStatus(imc, conversionMode, sentenceMode));
			::ImmReleaseContext(target().get(), imc);
			return succeeded ? 0 : 1;
		}
	}
	return 1;
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param extendSelection set true to extend the selection
 */
MatchBracketCommand::MatchBracketCommand(TextViewer& viewer, bool extendSelection) /*throw()*/ : Command(viewer), extends_(extendSelection) {
}

/**
 * @see Command#perform
 * @retval 0 succeeded
 * @retval 1 failed
 */
ulong MatchBracketCommand::perform() {
	END_ISEARCH();
	Caret& caret = target().caret();
	const Position matchBracket(caret.matchBrackets().first);
	if(matchBracket == Position::INVALID_POSITION) {
		if(beepsOnError())
			target().beep();	// not found
		return 1;
	}
	caret.endRectangleSelection();
	if(!extends_)
		caret.moveTo(matchBracket);
	else if(matchBracket > caret)
		caret.select(caret, Position(matchBracket.line, matchBracket.column + 1));
	else
		caret.select(Position(caret.lineNumber(), caret.columnNumber() + 1), matchBracket);
	return 0;
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param insertPrevious set true to insert on previous line. otherwise on the current line
 */
NewlineCommand::NewlineCommand(TextViewer& viewer, bool insertPrevious) /*throw()*/ : Command(viewer), insertsPrevious_(insertPrevious) {
}

/**
 * @see Command#perform
 * @retval 0 succeeded
 * @retval 1 failed
 */
ulong NewlineCommand::perform() {
	if(numericPrefix() <= 0)
		return 0;
	TextViewer& viewer = target();

	if(contentassist::IContentAssistant* const ca = target().contentAssistant()) {
		if(contentassist::IContentAssistant::ICompletionProposalsUI* cpui = ca->getCompletionProposalsUI()) {
			if(cpui->complete())
				return 0;
		}
	}

	if(Session* const session = viewer.document().session()) {
		if(session->incrementalSearcher().isRunning()) {
			session->incrementalSearcher().end();
			return 0;
		}
	}

	CHECK_DOCUMENT_READONLY(1);
//	CHECK_GUI_EDITABILITY(1);

	Caret& caret = viewer.caret();

	if(insertsPrevious_) {
		caret.enableAutoShow(false);
		if(caret.beginning().lineNumber() != 0)
			caret.moveTo(Position(caret.beginning().lineNumber() - 1, INVALID_INDEX));
		else
			caret.moveTo(Position::ZERO_POSITION);
		caret.enableAutoShow(true);
	}

	viewer.freeze();
	viewer.document().insertUndoBoundary();
	if(!caret.isSelectionEmpty())
		caret.eraseSelection();
	caret.newLine(false, numericPrefix());
	viewer.document().insertUndoBoundary();
	caret.moveTo(caret.anchor());
	viewer.unfreeze();
	return 0;
}

/**
 * Constructor.
 * @param viewer the target text viewer
 */
OvertypeModeToggleCommand::OvertypeModeToggleCommand(TextViewer& viewer) /*throw()*/ : Command(viewer) {
}

/**
 * @see Command#perform
 * @retval 0 succeeded
 * @retval 1 failed
 */
ulong OvertypeModeToggleCommand::perform() {
	Caret& caret = target().caret();
	caret.setOvertypeMode(!caret.isOvertypeMode());
	CLOSE_COMPLETION_PROPOSAL_POPUP();
	return 0;
}

/**
 * Constructor.
 * @param viewer the target text viewer
 */
PasteCommand::PasteCommand(TextViewer& viewer, bool useKillRing) /*throw()*/ : Command(viewer), usesKillRing_(useKillRing) {
}

/**
 * @see Command#perform
 * @retval 0 succeeded
 * @retval 1 failed
 */
ulong PasteCommand::perform() {
	ASSERT_IFISWINDOW();
	CHECK_DOCUMENT_READONLY(1);
	CLOSE_COMPLETION_PROPOSAL_POPUP();
	try {
		target().caret().pasteToSelection(usesKillRing_);
	} catch(...) {
		if(beepsOnError())
			target().beep();
		return 1;
	}
	return 0;
}

/**
 * Constructor.
 * @param viewer the target text viewer
 */
ReconversionCommand::ReconversionCommand(TextViewer& viewer) /*throw()*/ : Command(viewer) {
}

/**
 * @retval 0 the command succeeded
 * @retval 1 the command failed because of the empty or rectangle selection or system error
 * @see viewers#TextViewer#onIMERequest
 */
ulong ReconversionCommand::perform() {
	END_ISEARCH();
	CHECK_DOCUMENT_READONLY(1);
//	CHECK_GUI_EDITABILITY(1);

	ulong result = 1;
	TextViewer& viewer = target();
	Caret& caret = viewer.caret();
	if(!caret.isSelectionRectangle()) {
		if(HIMC imc = ::ImmGetContext(viewer.get())) {
			if(!toBoolean(::ImmGetOpenStatus(imc)))	// without this, IME may ignore us?
				::ImmSetOpenStatus(imc, true);

			// from NotePadView.pas of TNotePad (http://wantech.ikuto.com/)
			const bool multilineSelection = caret.lineNumber() != caret.anchor().lineNumber();
			const String s = multilineSelection ? caret.selectionText() : viewer.document().line(caret.lineNumber());
			const DWORD bytes = static_cast<DWORD>(sizeof(RECONVERTSTRING) + sizeof(Char) * s.length());
			RECONVERTSTRING* const rcs = static_cast<RECONVERTSTRING*>(::operator new(bytes));
			rcs->dwSize = bytes;
			rcs->dwVersion = 0;
			rcs->dwStrLen = static_cast<DWORD>(s.length());
			rcs->dwStrOffset = sizeof(RECONVERTSTRING);
			rcs->dwCompStrLen = rcs->dwTargetStrLen = static_cast<DWORD>(multilineSelection ? s.length() :
				(caret.end().columnNumber() - caret.beginning().columnNumber()));
			rcs->dwCompStrOffset = rcs->dwTargetStrOffset =
				multilineSelection ? 0 : static_cast<DWORD>(sizeof(Char) * caret.beginning().columnNumber());
			s.copy(reinterpret_cast<Char*>(reinterpret_cast<char*>(rcs) + rcs->dwStrOffset), s.length());

			bool succeeded = false;
			if(caret.isSelectionEmpty()) {
				// IME selects the composition target automatically if no selection
				if(toBoolean(::ImmSetCompositionStringW(imc, SCS_QUERYRECONVERTSTRING, rcs, rcs->dwSize, 0, 0))) {
					caret.select(
						Position(caret.lineNumber(), rcs->dwCompStrOffset / sizeof(Char)),
						Position(caret.lineNumber(), rcs->dwCompStrOffset / sizeof(Char) + rcs->dwCompStrLen));
					if(toBoolean(::ImmSetCompositionStringW(imc, SCS_SETRECONVERTSTRING, rcs, rcs->dwSize, 0, 0)))
						result = 0;
				}
			}
			::operator delete(rcs);
			::ImmReleaseContext(viewer.get(), imc);
		}
	}

	CLOSE_COMPLETION_PROPOSAL_POPUP();
	if(result != 0 && beepsOnError())
		viewer.beep();
	return result;
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param onlySelection
 * @param callback
 */
ReplaceAllCommand::ReplaceAllCommand(TextViewer& viewer, bool onlySelection,
		searcher::IInteractiveReplacementCallback* callback) /*throw()*/
		: Command(viewer), onlySelection_(onlySelection), callback_(callback) {
}

/**
 * Replaces all matched texts. This does not freeze the text viewer.
 * @return the number of replced strings
 */
ulong ReplaceAllCommand::perform() {
	ABORT_MODES();
    if(onlySelection_ && target().caret().isSelectionEmpty())
		return 0;

	WaitCursor wc;
	TextViewer& viewer = target();
	Document& document = viewer.document();
	const searcher::TextSearcher* s;
	if(const Session* const session = document.session())
		s = &session->textSearcher();
	else
		return 0;	// TODO: prepares a default text searcher.

	Region scope(
		onlySelection_ ? max<Position>(viewer.caret().beginning(),
			document.accessibleRegion().first) : document.accessibleRegion().first,
		onlySelection_ ? min<Position>(viewer.caret().end(),
			document.accessibleRegion().second) : document.accessibleRegion().second);

	// mark to restore the selection later
	kernel::Point oldAnchor(document, viewer.caret().anchor());
	kernel::Point oldCaret(document, viewer.caret());

	ulong c = 0;
	try {
		c = static_cast<ulong>(s->replaceAll(document, scope, callback_));
	} catch(...) {
		if(c != 0)
			viewer.caret().select(oldAnchor, oldCaret);
		viewer.unfreeze();
		throw;
	}
	if(c != 0)
		viewer.caret().select(oldAnchor, oldCaret);
	return c;
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param procedure a pointer to the member function defines the destination
 */
RowSelectionExtensionCommand::RowSelectionExtensionCommand(TextViewer& viewer,
		Position(Caret::*procedure)(void) const) : Command(viewer), procedure0_(procedure), procedure1_(0), procedureV1_(0) {
	if(procedure == 0 || find(MOVEMENT_PROCEDURES_0,
			MANAH_ENDOF(MOVEMENT_PROCEDURES_0), procedure) == MANAH_ENDOF(MOVEMENT_PROCEDURES_0))
		throw invalid_argument("procedure");
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param procedure a pointer to the member function defines the destination
 */
RowSelectionExtensionCommand::RowSelectionExtensionCommand(TextViewer& viewer,
		Position(Caret::*procedure)(length_t) const) : Command(viewer), procedure0_(0), procedure1_(procedure), procedureV1_(0) {
	if(procedure == 0 || find(MOVEMENT_PROCEDURES_1,
			MANAH_ENDOF(MOVEMENT_PROCEDURES_1), procedure) == MANAH_ENDOF(MOVEMENT_PROCEDURES_1))
		throw invalid_argument("procedure");
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param procedure a pointer to the member function defines the destination
 */
RowSelectionExtensionCommand::RowSelectionExtensionCommand(TextViewer& viewer,
		VerticalDestinationProxy(Caret::*procedure)(length_t) const) :
		Command(viewer), procedure0_(0), procedure1_(0), procedureV1_(procedure) {
	if(procedure == 0 || find(MOVEMENT_PROCEDURES_V_1,
			MANAH_ENDOF(MOVEMENT_PROCEDURES_V_1), procedure) == MANAH_ENDOF(MOVEMENT_PROCEDURES_V_1))
		throw invalid_argument("procedure");
}

/**
 * @see Command#perform
 * @return 0
 */
ulong RowSelectionExtensionCommand::perform() {
	CLOSE_COMPLETION_PROPOSAL_POPUP();
	END_ISEARCH();

	Caret& caret = target().caret();
	if(caret.isSelectionEmpty() && !caret.isSelectionRectangle())
		caret.beginRectangleSelection();
	if(procedure0_ != 0)
		return CaretMovementCommand(target(), procedure0_, true)();
	else if(procedure1_ != 0)
		return CaretMovementCommand(target(), procedure1_, true)();
	else
		return CaretMovementCommand(target(), procedureV1_, true)();
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param untabify set true to untabify rather than tabify
 */
TabifyCommand::TabifyCommand(TextViewer& viewer, bool untabify) /*throw()*/ : Command(viewer), untabify_(untabify) {
}

/**
 * @see Command#perform
 * @retval 0 succeeded
 * @retval 1 failed
 */
ulong TabifyCommand::perform() {
	CHECK_DOCUMENT_READONLY(1);
//	CHECK_GUI_EDITABILITY(1);
	ABORT_MODES();
	// TODO: not implemented.
	return 1;
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param text the text to input. this can be empty or ill-formed UTF-16 sequence
 */
TextInputCommand::TextInputCommand(TextViewer& viewer, const String& text) /*throw()*/ : Command(viewer), text_(text) {
}

/**
 * Inserts a text. If the incremental search is active, appends a string to the end of the pattern.
 * @retval 0 succeeded
 * @retval 1 failed
 */
ulong TextInputCommand::perform() {
	const long n = numericPrefix();
	if(n <= 0)
		return 0;

	if(Session* const session = target().document().session()) {
		if(session->incrementalSearcher().isRunning()) {
			if(n > 1) {
				basic_stringbuf<Char> b;
				for(long i = 0; i < n; ++i)
					b.sputn(text_.data(), static_cast<streamsize>(text_.length()));
				session->incrementalSearcher().addString(b.str());
			} else
				session->incrementalSearcher().addString(text_);
			return 0;
		}
	}

	CHECK_DOCUMENT_READONLY(1);
//	CHECK_GUI_EDITABILITY(1);
	if(n > 1) {
		basic_stringbuf<Char> b;
		for(long i = 0; i < n; ++i)
			b.sputn(text_.data(), static_cast<streamsize>(text_.length()));
		target().caret().replaceSelection(b.str());
	} else
		target().caret().replaceSelection(text_);
	return 0;
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param procedure indicates what to transpose. this must be one of:
 * @c EditPoint#transposeCharacters, @c EditPoint#transposeWords, @c EditPoint#transposeLines
 * @throw std#invalid_argument
 */
TranspositionCommand::TranspositionCommand(TextViewer& viewer, bool(EditPoint::*procedure)(void)) : Command(viewer), procedure_(procedure) {
	if(procedure_ != &EditPoint::transposeCharacters
			&& procedure_ != &EditPoint::transposeWords
			&& procedure_ != &EditPoint::transposeLines)
		throw invalid_argument("procedure");
}

/**
 * @see Command#perform
 * @retval 0 succeeded
 * @retval 1 failed
 */
ulong TranspositionCommand::perform() {
	CHECK_DOCUMENT_READONLY(1);
//	CHECK_GUI_EDITABILITY(1);
	END_ISEARCH();
	CLOSE_COMPLETION_PROPOSAL_POPUP();

	TextViewer& viewer = target();
	Caret& caret = viewer.caret();
	viewer.freeze();
	viewer.document().insertUndoBoundary();
	const bool succeeded = (caret.*procedure_)();
	viewer.document().insertUndoBoundary();
	viewer.unfreeze();

	if(succeeded) {
		if(beepsOnError())
			viewer.beep();
		return 0;
	}
	return 1;
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param redo set true to perform redo, rather than undo
 */
UndoCommand::UndoCommand(TextViewer& viewer, bool redo) /*throw()*/ : Command(viewer), redo_(redo) {
}

/**
 * Undo or redo.
 * @retval 0 succeeded
 * @retval 1 failed
 * @retval 2 partialy done
 * @see Document#undo, Document#redo
 */
ulong UndoCommand::perform() {
	CHECK_DOCUMENT_READONLY(1);
//	CHECK_GUI_EDITABILITY(1);

	if(numericPrefix() < 0)
		setNumericPrefix(0);	// currently, this is no-op

	WaitCursor wc;
	Document& document = target().document();
	if(!redo_)
		return document.undo(min(static_cast<size_t>(numericPrefix()), document.numberOfUndoableChanges())) ? 0 : 2;
	else
		return document.redo(min(static_cast<size_t>(numericPrefix()), document.numberOfRedoableChanges())) ? 0 : 2;
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param direction the direcion to delete
 */
WordDeletionCommand::WordDeletionCommand(TextViewer& viewer, Direction direction) /*throw()*/ : Command(viewer), direction_(direction) {
}

/**
 * @see Command#perform
 * @retval 0 succeeded
 * @retval 1 failed
 */
ulong WordDeletionCommand::perform() {
	long n = numericPrefix();
	if(n == 0)
		return 0;
	CHECK_DOCUMENT_READONLY(1);
	ABORT_ISEARCH();

	TextViewer& viewer = target();
	Caret& caret = viewer.caret();
	const bool forward = (direction_ == Direction::FORWARD && n > 0) || (direction_ == Direction::BACKWARD && n < 0);
	n = abs(n);
	if(/*caret.isAutoCompletionRunning() &&*/ forward)
		CLOSE_COMPLETION_PROPOSAL_POPUP();

	Document& document = viewer.document();
	const Position from = forward ? caret.beginning() : caret.end();
	text::WordBreakIterator<DocumentCharacterIterator> to(
		DocumentCharacterIterator(document, forward ? caret.end() : caret.beginning()),
		text::AbstractWordBreakIterator::START_OF_SEGMENT,
			viewer.document().contentTypeInformation().getIdentifierSyntax(caret.getContentType()));
	for(Position p(to.base().tell()); n > 0; --n) {
		if(p == (forward ? ++to : --to).base().tell())
			break;
		p = to.base().tell();
	}
	if(to.base().tell() != from) {
		viewer.freeze();
		document.insertUndoBoundary();
		document.erase(from, to.base().tell());
		caret.moveTo(min(from, to.base().tell()));
		document.insertUndoBoundary();
		viewer.unfreeze();
	}
	return 0;
}

/**
 * Constructor.
 * @param viewer the target text viewer
 */
WordSelectionCreationCommand::WordSelectionCreationCommand(TextViewer& viewer) /*throw()*/ : Command(viewer) {
}

/**
 * @see Command#perform
 * @return 0
 */
ulong WordSelectionCreationCommand::perform() {
	END_ISEARCH();
	target().caret().endRectangleSelection();
	target().caret().selectWord();
	return 0;
}

#undef ASSERT_IFISWINDOW
#undef ABORT_ISEARCH
#undef END_ISEARCH
#undef CHECK_DOCUMENT_READONLY
#undef CHECK_GUI_EDITABILITY
#undef CLOSE_COMPLETION_WINDOW


// isc.AinuInputSequenceChecker /////////////////////////////////////////////

/// @see InputSequenceChecker#check
bool AinuInputSequenceChecker::check(HKL, const Char* first, const Char* last, CodePoint cp) const {
	// 結合可能な半濁点のペアが正しいか調べるだけ
	return cp != 0x309A || (first < last && (
		last[-1] == L'\x30BB'		// セ
		|| last[-1] == L'\x30C4'	// ツ
		|| last[-1] == L'\x30C8'	// ト
		|| last[-1] == L'\x31F7'));	// 小さいフ
}


// isc.ThaiInputSequenceChecker /////////////////////////////////////////////

const ThaiInputSequenceChecker::CharacterClass ThaiInputSequenceChecker::charClasses_[] = {
/* U+0E00 */	CTRL, CONS, CONS, CONS, CONS, CONS, CONS, CONS,
				CONS, CONS, CONS, CONS, CONS, CONS, CONS, CONS,
/* U+0E10 */	CONS, CONS, CONS, CONS, CONS, CONS, CONS, CONS,
				CONS, CONS, CONS, CONS, CONS, CONS, CONS, CONS,
/* U+0E20 */	CONS, CONS, CONS, CONS, FV3,  CONS, FV3,  CONS,
				CONS, CONS, CONS, CONS, CONS, CONS, CONS, NON,
/* U+0E30 */	FV1,  AV2,  FV1,  FV1,  AV1,  AV3,  AV2,  AV3,
				BV1,  BV2,  BD,   CTRL, CTRL, CTRL, CTRL, NON,
/* U+0E40 */	LV,   LV,   LV,   LV,   LV,   FV2,  NON,  AD2,
				TONE, TONE, TONE, TONE, AD1,  AD1,  AD3,  NON,
/* U+0E50 */	NON,  NON,  NON,  NON,  NON,  NON,  NON,  NON,
				NON,  NON,  NON,  NON,  CTRL, CTRL, CTRL, CTRL,
};
const char ThaiInputSequenceChecker::checkMap_[] =
/* CTRL */	"XAAAAAA" "RRRRRRRRRR"
/* NON */	"XAAASSA" "RRRRRRRRRR"
/* CONS */	"XAAAASA" "CCCCCCCCCC"
/* LV */	"XSASSSS" "RRRRRRRRRR"
/* FV1 */	"XSASASA" "RRRRRRRRRR"
/* FV2 */	"XAAAASA" "RRRRRRRRRR"
/* FV3 */	"XAAASAS" "RRRRRRRRRR"
/* BV1 */	"XAAAASA" "RRRCCRRRRR"
/* BV2 */	"XAAASSA" "RRRCRRRRRR"
/* BD */	"XAAASSA" "RRRRRRRRRR"
/* TONE */	"XAAAAAA" "RRRRRRRRRR"
/* AD1 */	"XAAASSA" "RRRRRRRRRR"
/* AD2 */	"XAAASSA" "RRRRRRRRRR"
/* AD3 */	"XAAASSA" "RRRRRRRRRR"
/* AV1 */	"XAAASSA" "RRRCCRRRRR"
/* AV2 */	"XAAASSA" "RRRCRRRRRR"
/* AV3 */	"XAAASSA" "RRRCRCRRRR";

/// @see InputSequenceChecker#check
bool ThaiInputSequenceChecker::check(HKL, const Char* first, const Char* last, CodePoint cp) const {
	// WTT 2.0 で規格化されている
	// - http://mozart.inet.co.th/cyberclub/trin/thairef/wtt2/char-class.pdf
	// - http://www.nectec.or.th/it-standards/keyboard_layout/thai-key.htm
	if(mode_ == PASS_THROUGH)
		return true;
	return doCheck(
		(first != last) ? getCharacterClass(last[-1]) : CTRL,	// 先行する文字が無い場合は制御文字があるとする
		getCharacterClass((cp != 0x0E33) ? cp : 0x0E4D),		// Sara Am -> Nikhahit + Sara Aa
		mode_ == STRICT_MODE);
}


// isc.VietnameseInputSequenceChecker ///////////////////////////////////////

/// @see InputSequenceChecker#check
bool VietnameseInputSequenceChecker::check(HKL keyboardLayout, const Char* first, const Char* last, CodePoint cp) const {
	// ベトナム語の文字「クオック・グー」では12個の母音字、5個の声調記号、その他の子音を使う。
	// ここでは声調記号の入力が <1文字の母音字>+<1文字以下の声調記号>
	// というパターンに矛盾していないかを調べる。
	// 独自のスクリプトを持たないため、入力ロケールがベトナム語でないときはチェックしない。
	// 詳細は http://www.asahi-net.or.jp/~ez3k-msym/charsets/cjk-v.htm を参照
	// (Uniscribe と同じく母音字が合成文字の場合は無視)
	static const CodePoint VOWELS[24] = {
		L'A', L'E', L'I', L'O', L'U', L'Y', L'a', L'e', L'i', L'o', L'u', L'y',
		0x00C2, 0x00CA, 0x00D4, 0x00E2, 0x00EA, 0x00F4, 0x0102, 0x0103, 0x01A0, 0x01A1, 0x01AF, 0x01B0,
	};
	static CodePoint TONE_MARKS[5] = {0x0300, 0x0301, 0x0303, 0x309, 0x0323};

	using std::binary_search;
	if(PRIMARYLANGID(LOWORD(keyboardLayout)) != LANG_VIETNAMESE)
		return true;
	else if(first < last && binary_search(TONE_MARKS, MANAH_ENDOF(TONE_MARKS), cp))
		return binary_search(VOWELS, MANAH_ENDOF(VOWELS), last[-1]);
	return true;
}
