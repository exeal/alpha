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
using namespace std;
using manah::win32::ui::WaitCursor;


// Command //////////////////////////////////////////////////////////////////

/**
 * Protected constructor.
 * @param viewer the target text viewer
 */
Command::Command(TextViewer& viewer) /*throw()*/ : viewer_(&viewer), numericPrefix_(1) {
}

/// Destructor.
Command::~Command() /*throw()*/ {
}


// commands.* ///////////////////////////////////////////////////////////////

namespace {
	inline void abortIncrementalSearch(TextViewer& target) {
		if(Session* const session = target.document().session()) {
			if(session->incrementalSearcher().isRunning())
				session->incrementalSearcher().abort();
		}
	}
	inline void endIncrementalSearch(TextViewer& target) {
		if(Session* const session = target.document().session()) {
			if(session->incrementalSearcher().isRunning())
				session->incrementalSearcher().abort();
		}
	}
	inline void abortModes(TextViewer& target) {
		utils::closeCompletionProposalsPopup(target);
		abortIncrementalSearch(target);
	}
}

#define ASCENSION_ASSERT_IFISWINDOW()	assert(target().isWindow())

// the command can't perform and throw if the document is read only
#define ASCENSION_CHECK_DOCUMENT_READ_ONLY()	\
	if(target().document().isReadOnly()) return false

namespace {
	typedef Position(*MovementProcedureP)(const Point&);
	typedef Position(*MovementProcedurePL)(const Point&, length_t);
	typedef Position(*MovementProcedurePCL)(const Point&, locations::CharacterUnit, length_t);
	typedef Position(*MovementProcedureV)(const VisualPoint&);
	typedef Position(*MovementProcedureVL)(const VisualPoint&, length_t);
	typedef Position(*MovementProcedureVCL)(const VisualPoint&, locations::CharacterUnit, length_t);
	typedef VerticalDestinationProxy(*MovementProcedureVLV)(const VisualPoint&, length_t);

	static MovementProcedureP MOVEMENT_PROCEDURES_P[] = {
		&locations::beginningOfDocument, &locations::beginningOfLine, &locations::endOfDocument, &locations::endOfLine};
	static MovementProcedurePL MOVEMENT_PROCEDURES_PL[] = {
		&locations::backwardBookmark, &locations::backwardLine, &locations::backwardWord, &locations::backwardWordEnd,
		&locations::forwardBookmark, &locations::forwardLine, &locations::forwardWord, &locations::forwardWordEnd};
	static MovementProcedurePCL MOVEMENT_PROCEDURES_PCL[] = {&locations::backwardCharacter, &locations::forwardCharacter};
	static MovementProcedureV MOVEMENT_PROCEDURES_V[] = {
		&locations::beginningOfVisualLine, &locations::contextualBeginningOfLine,
		&locations::contextualBeginningOfVisualLine, &locations::contextualEndOfLine, &locations::contextualEndOfVisualLine,
		&locations::endOfVisualLine, &locations::firstPrintableCharacterOfLine, &locations::firstPrintableCharacterOfVisualLine,
		&locations::lastPrintableCharacterOfLine, &locations::lastPrintableCharacterOfVisualLine};
	static MovementProcedureVL MOVEMENT_PROCEDURES_VL[] = {
		&locations::leftWord, &locations::leftWordEnd, &locations::rightWord, &locations::rightWordEnd};
	static MovementProcedureVCL MOVEMENT_PROCEDURES_VCL[] = {&locations::leftCharacter, &locations::rightCharacter};
	static MovementProcedureVLV MOVEMENT_PROCEDURES_VLV[] = {
		&locations::backwardPage, &locations::backwardVisualLine, &locations::forwardPage, &locations::forwardVisualLine};
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param region the region to operate on. if empty, the accessible region of the document. this
 * region will be shrunk to the accessible region when the command performed
 */
BookmarkMatchLinesCommand::BookmarkMatchLinesCommand(TextViewer& viewer,
		const Region& region /* = Region() */) /*throw()*/ : Command(viewer), region_(region), numberOfMarkedLines_(0) {
}

/// Returns the number of the previously marked lines.
length_t BookmarkMatchLinesCommand::numberOfMarkedLines() const /*throw()*/ {
	return numberOfMarkedLines_;
}

/**
 * Implements @c Command#perform.
 * @retval false the pattern to search was not set
 * @throw ... any exceptions specified @c TextSearcher#search other than @c BadPositionException
 *            and @c IllegalStateException. if threw, the marking is interrupted
 */
bool BookmarkMatchLinesCommand::perform() {
	WaitCursor wc;
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
		scope.first.column = 0;
		++numberOfMarkedLines_;
	}
	return true;
}

/**
 * Constructor.
 * @param viewer the target text viewer
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
 * @param viewer the target text viewer
 * @param procedure a pointer to the member function defines the destination
 * @param extendSelection set true to extend selection
 */
CaretMovementCommand::CaretMovementCommand(TextViewer& viewer,
		Position(*procedure)(const Point&), bool extendSelection /* = false */) :
		Command(viewer), extends_(extendSelection), procedureP_(procedure), procedurePL_(0), procedurePCL_(0),
		procedureV_(0), procedureVL_(0), procedureVCL_(0), procedureVLV_(0) {
	if(procedure == 0 || find(MOVEMENT_PROCEDURES_P,
			MANAH_ENDOF(MOVEMENT_PROCEDURES_P), procedure) == MANAH_ENDOF(MOVEMENT_PROCEDURES_P))
		throw invalid_argument("procedure");
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param procedure a pointer to the member function defines the destination
 * @param extendSelection set true to extend selection
 */
CaretMovementCommand::CaretMovementCommand(TextViewer& viewer,
		Position(*procedure)(const Point&, length_t), bool extendSelection /* = false */) :
		Command(viewer), extends_(extendSelection), procedureP_(0), procedurePL_(procedure), procedurePCL_(0),
		procedureV_(0), procedureVL_(0), procedureVCL_(0), procedureVLV_(0) {
	if(procedure == 0 || find(MOVEMENT_PROCEDURES_PL,
			MANAH_ENDOF(MOVEMENT_PROCEDURES_PL), procedure) == MANAH_ENDOF(MOVEMENT_PROCEDURES_PL))
		throw invalid_argument("procedure");
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param procedure a pointer to the member function defines the destination
 * @param extendSelection set true to extend selection
 */
CaretMovementCommand::CaretMovementCommand(TextViewer& viewer,
		Position(*procedure)(const Point&, locations::CharacterUnit, length_t), bool extendSelection /* = false */) :
		Command(viewer), extends_(extendSelection), procedureP_(0), procedurePL_(0), procedurePCL_(procedure),
		procedureV_(0), procedureVL_(0), procedureVCL_(0), procedureVLV_(0) {
	if(procedure == 0 || find(MOVEMENT_PROCEDURES_PCL,
			MANAH_ENDOF(MOVEMENT_PROCEDURES_PCL), procedure) == MANAH_ENDOF(MOVEMENT_PROCEDURES_PCL))
		throw invalid_argument("procedure");
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param procedure a pointer to the member function defines the destination
 * @param extendSelection set true to extend selection
 */
CaretMovementCommand::CaretMovementCommand(TextViewer& viewer,
		Position(*procedure)(const VisualPoint&), bool extendSelection /* = false */) :
		Command(viewer), extends_(extendSelection), procedureP_(0), procedurePL_(0), procedurePCL_(0),
		procedureV_(procedure), procedureVL_(0), procedureVCL_(0), procedureVLV_(0) {
	if(procedure == 0 || find(MOVEMENT_PROCEDURES_V,
			MANAH_ENDOF(MOVEMENT_PROCEDURES_V), procedure) == MANAH_ENDOF(MOVEMENT_PROCEDURES_V))
		throw invalid_argument("procedure");
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param procedure a pointer to the member function defines the destination
 * @param extendSelection set true to extend selection
 */
CaretMovementCommand::CaretMovementCommand(TextViewer& viewer,
		Position(*procedure)(const VisualPoint&, length_t), bool extendSelection /* = false */) :
		Command(viewer), extends_(extendSelection), procedureP_(0), procedurePL_(0), procedurePCL_(0),
		procedureV_(0), procedureVL_(procedure), procedureVCL_(0), procedureVLV_(0) {
	if(procedure == 0 || find(MOVEMENT_PROCEDURES_VL,
			MANAH_ENDOF(MOVEMENT_PROCEDURES_VL), procedure) == MANAH_ENDOF(MOVEMENT_PROCEDURES_VL))
		throw invalid_argument("procedure");
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param procedure a pointer to the member function defines the destination
 * @param extendSelection set true to extend selection
 */
CaretMovementCommand::CaretMovementCommand(TextViewer& viewer,
		Position(*procedure)(const VisualPoint&, locations::CharacterUnit, length_t), bool extendSelection /* = false */) :
		Command(viewer), extends_(extendSelection), procedureP_(0), procedurePL_(0), procedurePCL_(0),
		procedureV_(0), procedureVL_(0), procedureVCL_(procedure), procedureVLV_(0) {
	if(procedure == 0 || find(MOVEMENT_PROCEDURES_VCL,
			MANAH_ENDOF(MOVEMENT_PROCEDURES_VCL), procedure) == MANAH_ENDOF(MOVEMENT_PROCEDURES_VCL))
		throw invalid_argument("procedure");
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param procedure a pointer to the member function defines the destination
 * @param extendSelection set true to extend selection
 */
CaretMovementCommand::CaretMovementCommand(TextViewer& viewer,
		VerticalDestinationProxy(*procedure)(const VisualPoint&, length_t), bool extendSelection /* = false */) :
		Command(viewer), extends_(extendSelection), procedureP_(0), procedurePL_(0), procedurePCL_(0),
		procedureV_(0), procedureVL_(0), procedureVCL_(0), procedureVLV_(procedure) {
	if(procedure == 0 || find(MOVEMENT_PROCEDURES_VLV,
			MANAH_ENDOF(MOVEMENT_PROCEDURES_VLV), procedure) == MANAH_ENDOF(MOVEMENT_PROCEDURES_VLV))
		throw invalid_argument("procedure");
}

/**
 * Moves the caret or extends the selection.
 * @return true
 */
bool CaretMovementCommand::perform() {
	const long n = numericPrefix();
	endIncrementalSearch(target());
	if(n == 0)
		return true;
	Caret& caret = target().caret();

	if(!extends_) {
		if(procedurePL_ == &locations::forwardLine || procedurePL_ == &locations::backwardLine
				|| procedureVLV_ == &locations::forwardVisualLine || procedureVLV_ == &locations::backwardVisualLine
				|| procedureVLV_ == &locations::forwardPage || procedureVLV_ == &locations::backwardPage) {
			if(contentassist::IContentAssistant* const ca = target().contentAssistant()) {
				if(contentassist::IContentAssistant::ICompletionProposalsUI* const cpui = ca->getCompletionProposalsUI()) {
					if(procedurePL_ == &locations::forwardLine || procedureVLV_ == &locations::forwardVisualLine)
						cpui->nextProposal(n);
					else if(procedurePL_ == &locations::backwardLine || procedureVLV_ == &locations::backwardVisualLine)
						cpui->nextProposal(n);
					else if(procedureVLV_ == &locations::forwardPage)
						cpui->nextPage(n);
					else if(procedureVLV_ == &locations::backwardPage)
						cpui->nextPage(n);
					return true;
				}
			}
		}
		caret.endRectangleSelection();
		if(!isSelectionEmpty(caret)) {	// just clear the selection
			const bool rtl = target().configuration().orientation == layout::RIGHT_TO_LEFT;
			if(procedurePCL_ == &locations::forwardCharacter
					|| (procedureVCL_ == &locations::rightCharacter && !rtl)
					|| (procedureVCL_ == &locations::leftCharacter && rtl)) {
				caret.moveTo(caret.end());
				return true;
			} else if(procedurePCL_ == &locations::backwardCharacter
					|| (procedureVCL_ == &locations::leftCharacter && !rtl)
					|| (procedureVCL_ == &locations::rightCharacter && rtl)) {
				caret.moveTo(caret.beginning());
				return true;
			}
		}
	}

	// TODO: consider the numeric prefix.
	if(procedureVLV_ == &locations::forwardPage)
		target().sendMessage(WM_VSCROLL, SB_PAGEDOWN);
	else if(procedureVLV_ == &locations::backwardPage)
		target().sendMessage(WM_VSCROLL, SB_PAGEUP);

	if(procedureVLV_ == 0) {
		Position destination;
		if(procedureP_ != 0)
			destination = (*procedureP_)(caret);
		else if(procedurePL_ != 0)
			destination = (*procedurePL_)(caret, n);
		else if(procedurePCL_ != 0)
			destination = (*procedurePCL_)(caret, locations::GRAPHEME_CLUSTER, n);
		else if(procedureV_ != 0)
			destination = (*procedureV_)(caret);
		else if(procedureVL_ != 0)
			destination = (*procedureVL_)(caret, n);
		else if(procedureVCL_ != 0)
			destination = (*procedureVCL_)(caret, locations::GRAPHEME_CLUSTER, n);
		else
			assert(false);
		if(!extends_)
			caret.moveTo(destination);
		else
			caret.extendSelection(destination);
	} else {
		if(!extends_)
			caret.moveTo((*procedureVLV_)(caret, n));
		else
			caret.extendSelection((*procedureVLV_)(caret, n));
	}
	return true;
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
 * Implements @c Command#perform.
 * @retval false if the incremental search was active, couldn't undo. otherwise the document was
 *               read only or the region to delete was inaccessible
 */
bool CharacterDeletionCommand::perform() {
	long n = numericPrefix();
	if(n == 0)
		return 0;
	TextViewer& viewer = target();
	const bool forward = (direction_ == Direction::FORWARD && n > 0) || (direction_ == Direction::BACKWARD && n < 0);
	n = abs(n);
	if(/*caret.isAutoCompletionRunning() &&*/ forward)
		utils::closeCompletionProposalsPopup(viewer);

	Document& document = viewer.document();
	searcher::IncrementalSearcher* isearch = 0;
	if(Session* const session = document.session())
		isearch = &session->incrementalSearcher();
	if(isearch != 0 && isearch->isRunning()) {
		if(forward)	// delete the entire pattern
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
			} catch(const IDocumentInput::ChangeRejectedException&) {
				return false;
			}
		} else {
			AutoFreeze af((!isSelectionEmpty(caret) || n > 1) ? &viewer : 0);
			Region region(caret.selectedRegion());
			assert(region.isNormalized());
			if(forward)
				region.second = locations::nextCharacter(document, region.second,
					Direction::FORWARD, locations::GRAPHEME_CLUSTER, isSelectionEmpty(caret) ? n : (n - 1));
			else
				region.first = locations::nextCharacter(document, region.first,
					Direction::BACKWARD, locations::UTF32_CODE_UNIT, isSelectionEmpty(caret) ? n : (n - 1));
			try {
				erase(document, region);
			} catch(const IDocumentInput::ChangeRejectedException&) {
				return false;
			}
		}
	}
	return true;
}

/**
 * Constructor.
 * @param viewer the target text viewer
 */
CharacterToCodePointConversionCommand::CharacterToCodePointConversionCommand(TextViewer& viewer) /*throw()*/ : Command(viewer) {
}

/**
 * Implements @c Command#perform.
 * @retval false the end of the selection is the beginning of the line and couldn't find the string
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
	const Char* const line = document.line(eos.line()).data();
	const CodePoint cp = text::surrogates::decodeLast(line, line + eos.column());
	Char buffer[7];
#if(_MSC_VER < 1400)
	swprintf(buffer, L"%lX", cp);
#else
	swprintf(buffer, MANAH_COUNTOF(buffer), L"%lX", cp);
#endif // _MSC_VER < 1400
	AutoFreeze af(&viewer);
	caret.select(Position(eos.line(), eos.column() - ((cp > 0xffff) ? 2 : 1)), eos);
	try {
		caret.replaceSelection(buffer, buffer + wcslen(buffer), false);
	} catch(const IDocumentInput::ChangeRejectedException&) {
		return false;
	}
	return true;
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
 * Implements @c Command#perform.
 * @retval false failed and the incremental search is not active
 * @retval 0 otherwise
 * @see Caret#inputCharacter, TextViewer#onChar, TextViewer#onUniChar
 */
bool CharacterInputCommand::perform() {
	if(numericPrefix() == 1) {
		if(Session* const session = target().document().session()) {
			if(session->incrementalSearcher().isRunning()) {
				utils::closeCompletionProposalsPopup(target());
				if(c_ == 0x0009u || !toBoolean(iswcntrl(static_cast<wint_t>(c_))))
					session->incrementalSearcher().addCharacter(c_);
				return true;
			}
		}
		return target().caret().inputCharacter(c_);
	} else {
		ASCENSION_CHECK_DOCUMENT_READ_ONLY();
		if(numericPrefix() > 0) {
			String s;
			text::surrogates::encode(c_, back_inserter(s));
			return TextInputCommand(target(), s).setNumericPrefix(numericPrefix())();
		}
		return true;
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
 * Implements @c Command#perform.
 * @retval false the caret was the first/last line in the document and couldn't copy a character
 *               from the previous/next line. or the next/previous line was too short to locate the
 *               character to copy. or internal performance of @c CharacterInputCommand failed
 */
bool CharacterInputFromNextLineCommand::perform() {
	abortIncrementalSearch(target());
	ASCENSION_CHECK_DOCUMENT_READ_ONLY();

	// TODO: recognizes narrowing.

	const Document& document = target().document();
	const VisualPoint& caret = target().caret();

	if((fromPreviousLine_ && caret.line() == 0)
			|| (!fromPreviousLine_ && caret.line() >= document.numberOfLines() - 1))
		return false;
	
	const Position p((fromPreviousLine_ ? locations::backwardVisualLine(caret) : locations::forwardVisualLine(caret)).position());
	const String& line = document.line(caret.line() + (fromPreviousLine_ ? -1 : 1));
	if(p.column >= line.length())
		return false;
	return CharacterInputCommand(target(), text::surrogates::decodeFirst(line.begin() + p.column, line.end()))();
}

/**
 * Constructor.
 * @param viewer the target text viewer
 */
CodePointToCharacterConversionCommand::CodePointToCharacterConversionCommand(TextViewer& viewer) /*throw()*/ : Command(viewer) {
}

/**
 * Implements @c Command#perform.
 * @retval false the end of the selection is the beginning of the line and couldn't find the string
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
	const Char* const line = document.line(eos.line()).data();
	const length_t column = eos.column();

	// accept /(?:[Uu]\+)?[0-9A-Fa-f]{1,6}/
	if(toBoolean(iswxdigit(line[column - 1]))) {
		length_t i = column - 1;
		while(i != 0) {
			if(column - i == 7)
				return false;	// too long string
			else if(!toBoolean(iswxdigit(line[i - 1])))
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
			AutoFreeze af(&viewer);
			caret.select(Position(eos.line(), i), eos);
			try {
				caret.replaceSelection(buffer, buffer + (cp < 0x10000u ? 1 : 2), false);
			} catch(const IDocumentInput::ChangeRejectedException&) {
				return false;
			}
			return true;
		}
	}
	return false;	// invalid code point string and can't convert
}

/**
 * Constructor.
 * @param viewer the target text viewer
 */
CompletionProposalPopupCommand::CompletionProposalPopupCommand(TextViewer& viewer) /*throw()*/ : Command(viewer) {
}

/**
 * Implements @c Command#perform.
 * @retval false the text viewer didn't have the content assistant
 */
bool CompletionProposalPopupCommand::perform() {
	ASCENSION_CHECK_DOCUMENT_READ_ONLY();
//	ASCENSION_CHECK_GUI_EDITABILITY();
	abortIncrementalSearch(target());
	if(contentassist::IContentAssistant* ca = target().contentAssistant()) {
		ca->showPossibleCompletions();
		return true;
	}
	return false;	// the viewer does not have a content assistant
}

/**
 * Constructor.
 * @param viewer the target text viewer
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
 * @param viewer the target text viewer
 * @param direction the direction to search
 */
FindNextCommand::FindNextCommand(TextViewer& viewer, Direction direction) /*throw()*/ : Command(viewer), direction_(direction) {
}

/**
 * @see Command#perform
 * @return false if no text matched
 * @throw ... any exceptions @c searcher#TextSearcher#search throws
 */
bool FindNextCommand::perform() {
	long n = numericPrefix();
	if(n == 0)
		return 0;
	endIncrementalSearch(target());
	utils::closeCompletionProposalsPopup(target());

	WaitCursor wc;
	Document& document = target().document();
	const searcher::TextSearcher* s;
	if(const Session* const session = document.session())
		s = &session->textSearcher();
	else
		return false;	// TODO: prepares a default text searcher.
	const bool forward = (direction_ == Direction::FORWARD && n > 0) || (direction_ == Direction::BACKWARD && n < 0);
	n = abs(n);

	Caret& caret = target().caret();
	const Region scope(document.accessibleRegion());
	Region matchedRegion(caret.selectedRegion());
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
	} else
/*		viewer.highlightMatchTexts(false)*/;
	return foundOnce;
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param type the search type
 * @param direction the direction to search
 * @param callback the callback object for the incremental search. can be @c null
 */
IncrementalFindCommand::IncrementalFindCommand(TextViewer& viewer, searcher::TextSearcher::Type type,
		Direction direction, searcher::IIncrementalSearchCallback* callback /* = 0 */) /*throw()*/
		: Command(viewer), type_(type), direction_(direction), callback_(callback) {
}

/**
 * @see Command#perform
 * @return 0
 */
bool IncrementalFindCommand::perform() {
	long n = numericPrefix();
	if(n == 0)
		return 0;
	utils::closeCompletionProposalsPopup(target());
	if(Session* const session = target().document().session()) {
		const Direction realDirection = (n > 0) ? direction_ : !direction_;
		n = abs(n);
		searcher::IncrementalSearcher& isearch = session->incrementalSearcher();
		if(!isearch.isRunning()) {	// begin the search if not running
			isearch.start(target().document(), target().caret(), session->textSearcher(), type_, realDirection, callback_);
			--n;
		}
		for(; n > 0; --n) {	// jump N times
			if(!isearch.next(realDirection))
				return false;	// it is not able to jump anymore in the active incremental search
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
 * @retval false the document's input rejected the change
 */
bool IndentationCommand::perform() {
	const long n = numericPrefix();
	if(n == 0)
		return 0;
	ASCENSION_CHECK_DOCUMENT_READ_ONLY();
//	ASCENSION_CHECK_GUI_EDITABILITY();
	TextViewer& viewer = target();
	endIncrementalSearch(viewer);
	utils::closeCompletionProposalsPopup(viewer);

	try {
		Caret& caret = viewer.caret();
		viewer.document().insertUndoBoundary();
		AutoFreeze af(&viewer);
		indentByTabs(caret, caret.isSelectionRectangle(), n);
		viewer.document().insertUndoBoundary();
	} catch(const IDocumentInput::ChangeRejectedException&) {
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
 * @retval false the system didn't support the input method
 */
bool InputMethodOpenStatusToggleCommand::perform() {
	if(HIMC imc = ::ImmGetContext(target().get())) {
		const bool succeeded = toBoolean(::ImmSetOpenStatus(imc, !toBoolean(::ImmGetOpenStatus(imc))));
		::ImmReleaseContext(target().get(), imc);
		return succeeded;
	}
	return false;
}

/**
 * Constructor.
 * @param viewer the target text viewer
 */
InputMethodSoftKeyboardModeToggleCommand::InputMethodSoftKeyboardModeToggleCommand(TextViewer& viewer) /*throw()*/ : Command(viewer) {
}

/**
 * @see Command#perform
 * @retval false the system didn't support the input method
 */
bool InputMethodSoftKeyboardModeToggleCommand::perform() {
	if(HIMC imc = ::ImmGetContext(target().get())) {
		DWORD conversionMode, sentenceMode;
		if(toBoolean(::ImmGetConversionStatus(imc, &conversionMode, &sentenceMode))) {
			conversionMode = toBoolean(conversionMode & IME_CMODE_SOFTKBD) ?
				(conversionMode & ~IME_CMODE_SOFTKBD) : (conversionMode | IME_CMODE_SOFTKBD);
			const bool succeeded = toBoolean(::ImmSetConversionStatus(imc, conversionMode, sentenceMode));
			::ImmReleaseContext(target().get(), imc);
			return succeeded;
		}
	}
	return false;
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param extendSelection set true to extend the selection
 */
MatchBracketCommand::MatchBracketCommand(TextViewer& viewer, bool extendSelection) /*throw()*/ : Command(viewer), extends_(extendSelection) {
}

/**
 * Implements @c Command#perform.
 * @retval false the match bracket was not found
 */
bool MatchBracketCommand::perform() {
	endIncrementalSearch(target());
	Caret& caret = target().caret();
	const Position matchBracket(caret.matchBrackets().first);
	if(matchBracket == Position::INVALID_POSITION)
		return false;	// not found
	caret.endRectangleSelection();
	if(!extends_)
		caret.moveTo(matchBracket);
	else if(matchBracket > caret)
		caret.select(caret, Position(matchBracket.line, matchBracket.column + 1));
	else
		caret.select(Position(caret.line(), caret.column() + 1), matchBracket);
	return true;
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
 * @retval true succeeded
 * @retval false failed
 */
bool NewlineCommand::perform() {
	if(numericPrefix() <= 0)
		return true;
	TextViewer& viewer = target();

	if(contentassist::IContentAssistant* const ca = target().contentAssistant()) {
		if(contentassist::IContentAssistant::ICompletionProposalsUI* cpui = ca->getCompletionProposalsUI()) {
			if(cpui->complete())
				return true;
		}
	}

	if(Session* const session = viewer.document().session()) {
		if(session->incrementalSearcher().isRunning()) {
			session->incrementalSearcher().end();
			return true;
		}
	}

	ASCENSION_CHECK_DOCUMENT_READ_ONLY();
//	ASCENSION_CHECK_GUI_EDITABILITY(1);

	Caret& caret = viewer.caret();
	const Region oldSelection(caret.selectedRegion());
	AutoFreeze af(&viewer);

	if(insertsPrevious_) {
		const bool autoShow = caret.isAutoShowEnabled();
		caret.enableAutoShow(false);
		if(oldSelection.first.line != 0)
			caret.moveTo(Position(oldSelection.first.line - 1, INVALID_INDEX));
		else
			caret.moveTo(Position::ZERO_POSITION);
		caret.enableAutoShow(autoShow);
	}

	try {
		viewer.document().insertUndoBoundary();
		breakLine(caret, false, numericPrefix());
	} catch(const IDocumentInput::ChangeRejectedException&) {
		viewer.document().insertUndoBoundary();
		caret.select(oldSelection);
		return false;
	}
	viewer.document().insertUndoBoundary();
	caret.moveTo(caret.anchor());
	return true;
}

/**
 * Constructor.
 * @param viewer the target text viewer
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
 * @param viewer the target text viewer
 */
PasteCommand::PasteCommand(TextViewer& viewer, bool useKillRing) /*throw()*/ : Command(viewer), usesKillRing_(useKillRing) {
}

/**
 * @see Command#perform
 * @return false the internal call of @c Caret#pasteToSelection threw
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
 * @param viewer the target text viewer
 */
ReconversionCommand::ReconversionCommand(TextViewer& viewer) /*throw()*/ : Command(viewer) {
}

/**
 * Implements @c Command#perform.
 * @return false the selection was empty or rectangle. or the system didn't support IME
 *               reconversion
 * @throw std::bad_alloc out of memory
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
		if(HIMC imc = ::ImmGetContext(viewer.get())) {
			if(!toBoolean(::ImmGetOpenStatus(imc)))	// without this, IME may ignore us?
				::ImmSetOpenStatus(imc, true);

			// from NotePadView.pas of TNotePad (http://wantech.ikuto.com/)
			const bool multilineSelection = caret.line() != caret.anchor().line();
			const String s(multilineSelection ? selectedString(caret) : viewer.document().line(caret.line()));
			const DWORD bytes = static_cast<DWORD>(sizeof(RECONVERTSTRING) + sizeof(Char) * s.length());
			RECONVERTSTRING* rcs;
			try {
				rcs = static_cast<RECONVERTSTRING*>(::operator new(bytes));
			} catch(const bad_alloc&) {
				::ImmReleaseContext(viewer.get(), imc);
				throw;	// failed to allocate the memory for RECONVERTSTRING
			}
			rcs->dwSize = bytes;
			rcs->dwVersion = 0;
			rcs->dwStrLen = static_cast<DWORD>(s.length());
			rcs->dwStrOffset = sizeof(RECONVERTSTRING);
			rcs->dwCompStrLen = rcs->dwTargetStrLen =
				static_cast<DWORD>(multilineSelection ? s.length() : (caret.end().column() - caret.beginning().column()));
			rcs->dwCompStrOffset = rcs->dwTargetStrOffset =
				multilineSelection ? 0 : static_cast<DWORD>(sizeof(Char) * caret.beginning().column());
			s.copy(reinterpret_cast<Char*>(reinterpret_cast<char*>(rcs) + rcs->dwStrOffset), s.length());

			if(isSelectionEmpty(caret)) {
				// IME selects the composition target automatically if no selection
				if(toBoolean(::ImmSetCompositionStringW(imc, SCS_QUERYRECONVERTSTRING, rcs, rcs->dwSize, 0, 0))) {
					caret.select(
						Position(caret.line(), rcs->dwCompStrOffset / sizeof(Char)),
						Position(caret.line(), rcs->dwCompStrOffset / sizeof(Char) + rcs->dwCompStrLen));
					if(toBoolean(::ImmSetCompositionStringW(imc, SCS_SETRECONVERTSTRING, rcs, rcs->dwSize, 0, 0)))
						succeeded = true;
				}
			}
			::operator delete(rcs);
			::ImmReleaseContext(viewer.get(), imc);
		}
	}

	utils::closeCompletionProposalsPopup(viewer);
	return succeeded;
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
bool ReplaceAllCommand::perform() {
	abortModes(target());
    if(onlySelection_ && isSelectionEmpty(target().caret()))
		return false;

	WaitCursor wc;
	TextViewer& viewer = target();
	Document& document = viewer.document();
	const searcher::TextSearcher* s;
	if(const Session* const session = document.session())
		s = &session->textSearcher();
	else
		return false;	// TODO: prepares a default text searcher.

	Region scope(
		onlySelection_ ? max<Position>(viewer.caret().beginning(),
			document.accessibleRegion().first) : document.accessibleRegion().first,
		onlySelection_ ? min<Position>(viewer.caret().end(),
			document.accessibleRegion().second) : document.accessibleRegion().second);

	// mark to restore the selection later
	kernel::Point oldAnchor(document, viewer.caret().anchor());
	kernel::Point oldCaret(document, viewer.caret());

	AutoFreeze af(&viewer);
	try {
		numberOfLastReplacements_ = s->replaceAll(document, scope, callback_);
	} catch(...) {
		numberOfLastReplacements_ = 0;
		throw;
	}
	if(numberOfLastReplacements_ != 0)
		viewer.caret().select(oldAnchor, oldCaret);
	return true;
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param procedure a pointer to the member function defines the destination
 */
RowSelectionExtensionCommand::RowSelectionExtensionCommand(TextViewer& viewer,
		Position(*procedure)(const Point&)) : Command(viewer), procedureP_(procedure), procedurePL_(0), procedurePCL_(0),
		procedureV_(0), procedureVL_(0), procedureVCL_(0), procedureVLV_(0) {
	if(procedure == 0 || find(MOVEMENT_PROCEDURES_P,
			MANAH_ENDOF(MOVEMENT_PROCEDURES_P), procedure) == MANAH_ENDOF(MOVEMENT_PROCEDURES_P))
		throw invalid_argument("procedure");
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param procedure a pointer to the member function defines the destination
 */
RowSelectionExtensionCommand::RowSelectionExtensionCommand(TextViewer& viewer,
		Position(*procedure)(const Point&, length_t)) : Command(viewer), procedureP_(0), procedurePL_(procedure), procedurePCL_(0),
		procedureV_(0), procedureVL_(0), procedureVCL_(0), procedureVLV_(0) {
	if(procedure == 0 || find(MOVEMENT_PROCEDURES_PL,
			MANAH_ENDOF(MOVEMENT_PROCEDURES_PL), procedure) == MANAH_ENDOF(MOVEMENT_PROCEDURES_PL))
		throw invalid_argument("procedure");
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param procedure a pointer to the member function defines the destination
 */
RowSelectionExtensionCommand::RowSelectionExtensionCommand(TextViewer& viewer,
		Position(*procedure)(const Point&, locations::CharacterUnit, length_t)) : Command(viewer), procedureP_(0), procedurePL_(0), procedurePCL_(procedure),
		procedureV_(0), procedureVL_(0), procedureVCL_(0), procedureVLV_(0) {
	if(procedure == 0 || find(MOVEMENT_PROCEDURES_PCL,
			MANAH_ENDOF(MOVEMENT_PROCEDURES_PCL), procedure) == MANAH_ENDOF(MOVEMENT_PROCEDURES_PCL))
		throw invalid_argument("procedure");
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param procedure a pointer to the member function defines the destination
 */
RowSelectionExtensionCommand::RowSelectionExtensionCommand(TextViewer& viewer,
		Position(*procedure)(const VisualPoint&)) : Command(viewer), procedureP_(0), procedurePL_(0), procedurePCL_(0),
		procedureV_(procedure), procedureVL_(0), procedureVCL_(0), procedureVLV_(0) {
	if(procedure == 0 || find(MOVEMENT_PROCEDURES_V,
			MANAH_ENDOF(MOVEMENT_PROCEDURES_V), procedure) == MANAH_ENDOF(MOVEMENT_PROCEDURES_V))
		throw invalid_argument("procedure");
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param procedure a pointer to the member function defines the destination
 */
RowSelectionExtensionCommand::RowSelectionExtensionCommand(TextViewer& viewer,
		Position(*procedure)(const VisualPoint&, length_t)) : Command(viewer), procedureP_(0), procedurePL_(0), procedurePCL_(0),
		procedureV_(0), procedureVL_(procedure), procedureVCL_(0), procedureVLV_(0) {
	if(procedure == 0 || find(MOVEMENT_PROCEDURES_VL,
			MANAH_ENDOF(MOVEMENT_PROCEDURES_VL), procedure) == MANAH_ENDOF(MOVEMENT_PROCEDURES_VL))
		throw invalid_argument("procedure");
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param procedure a pointer to the member function defines the destination
 */
RowSelectionExtensionCommand::RowSelectionExtensionCommand(TextViewer& viewer,
		Position(*procedure)(const VisualPoint&, locations::CharacterUnit, length_t)) : Command(viewer), procedureP_(0), procedurePL_(0), procedurePCL_(0),
		procedureV_(0), procedureVL_(0), procedureVCL_(procedure), procedureVLV_(0) {
	if(procedure == 0 || find(MOVEMENT_PROCEDURES_VCL,
			MANAH_ENDOF(MOVEMENT_PROCEDURES_VCL), procedure) == MANAH_ENDOF(MOVEMENT_PROCEDURES_VCL))
		throw invalid_argument("procedure");
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param procedure a pointer to the member function defines the destination
 */
RowSelectionExtensionCommand::RowSelectionExtensionCommand(TextViewer& viewer,
		VerticalDestinationProxy(*procedure)(const VisualPoint&, length_t)) : Command(viewer), procedureP_(0), procedurePL_(0), procedurePCL_(0),
		procedureV_(0), procedureVL_(0), procedureVCL_(0), procedureVLV_(procedure) {
	if(procedure == 0 || find(MOVEMENT_PROCEDURES_VLV,
			MANAH_ENDOF(MOVEMENT_PROCEDURES_VLV), procedure) == MANAH_ENDOF(MOVEMENT_PROCEDURES_VLV))
		throw invalid_argument("procedure");
}

/**
 * @see Command#perform
 * @return true
 */
bool RowSelectionExtensionCommand::perform() {
	utils::closeCompletionProposalsPopup(target());
	endIncrementalSearch(target());

	Caret& caret = target().caret();
	if(isSelectionEmpty(caret) && !caret.isSelectionRectangle())
		caret.beginRectangleSelection();
	if(procedureP_ != 0)
		return CaretMovementCommand(target(), procedureP_, true)();
	else if(procedurePL_ != 0)
		return CaretMovementCommand(target(), procedurePL_, true)();
	else if(procedurePCL_ != 0)
		return CaretMovementCommand(target(), procedurePCL_, true)();
	else if(procedureV_ != 0)
		return CaretMovementCommand(target(), procedureV_, true)();
	else if(procedureVL_ != 0)
		return CaretMovementCommand(target(), procedureVL_, true)();
	else if(procedureVCL_ != 0)
		return CaretMovementCommand(target(), procedureVCL_, true)();
	else
		return CaretMovementCommand(target(), procedureVLV_, true)();
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param untabify set true to untabify rather than tabify
 */
TabifyCommand::TabifyCommand(TextViewer& viewer, bool untabify) /*throw()*/ : Command(viewer), untabify_(untabify) {
}

/**
 * Implements @c Command#perform.
 * @note not implemented.
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
 * @param viewer the target text viewer
 * @param text the text to input. this can be empty or ill-formed UTF-16 sequence
 */
TextInputCommand::TextInputCommand(TextViewer& viewer, const String& text) /*throw()*/ : Command(viewer), text_(text) {
}

/**
 * Inserts a text. If the incremental search is active, appends a string to the end of the pattern.
 * @retval false 
 * @throw ... any exceptions @c searcher#IncrementalSearcher#addString and
 *            @c viewers#replaceSelection throw
 */
bool TextInputCommand::perform() {
	const long n = numericPrefix();
	if(n <= 0)
		return true;

	if(Session* const session = target().document().session()) {
		if(session->incrementalSearcher().isRunning()) {
			if(n > 1) {
				basic_stringbuf<Char> b;
				for(long i = 0; i < n; ++i)
					b.sputn(text_.data(), static_cast<streamsize>(text_.length()));
				session->incrementalSearcher().addString(b.str());
			} else
				session->incrementalSearcher().addString(text_);
			return true;
		}
	}

	ASCENSION_CHECK_DOCUMENT_READ_ONLY();
//	ASCENSION_CHECK_GUI_EDITABILITY();
	if(n > 1) {
		basic_stringbuf<Char> b;
		for(long i = 0; i < n; ++i)
			b.sputn(text_.data(), static_cast<streamsize>(text_.length()));
		try {
			replaceSelection(target().caret(), b.str());
		} catch(const IDocumentInput::ChangeRejectedException&) {
			return false;
		}
	} else {
		try {
			replaceSelection(target().caret(), text_);
		} catch(const IDocumentInput::ChangeRejectedException&) {
			return false;
		}
	}
	return true;
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param procedure indicates what to transpose. this must be one of:
 * @c EditPoint#transposeCharacters, @c EditPoint#transposeWords, @c EditPoint#transposeLines
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
 * @return false the internal transposition method call returned false
 * @throw ... any exceptions the transposition method returns other than
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
	} catch(const IDocumentInput::ChangeRejectedException&) {
		return false;
	}
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param redo set true to perform redo, rather than undo
 */
UndoCommand::UndoCommand(TextViewer& viewer, bool redo) /*throw()*/ : Command(viewer), redo_(redo), incompleted_(false) {
}

/**
 * Returns @ true if the last performance was done incompletely.
 * @see Document#undo, Document#redo
 */
bool UndoCommand::isLastActionIncompleted() const /*throw()*/ {
	return incompleted_;
}

/**
 * Undo or redo.
 * @return a boolean value returned by @c Document#undo or @c Document#redo.
 */
bool UndoCommand::perform() {
	ASCENSION_CHECK_DOCUMENT_READ_ONLY();
//	ASCENSION_CHECK_GUI_EDITABILITY(1);

	if(numericPrefix() < 0)
		setNumericPrefix(0);	// currently, this is no-op

	WaitCursor wc;
	Document& document = target().document();
	bool (Document::*f)(size_t) = !redo_ ? &Document::undo : &Document::redo;
	incompleted_ = (document.*f)(min(static_cast<size_t>(numericPrefix()), document.numberOfUndoableChanges()));
	return true;
}

/**
 * Constructor.
 * @param viewer the target text viewer
 * @param direction the direcion to delete
 */
WordDeletionCommand::WordDeletionCommand(TextViewer& viewer, Direction direction) /*throw()*/ : Command(viewer), direction_(direction) {
}

/**
 * Implements @c Command#perform.
 * @retval false the document's input rejected the change
 */
bool WordDeletionCommand::perform() {
	long n = numericPrefix();
	if(n == 0)
		return true;
	ASCENSION_CHECK_DOCUMENT_READ_ONLY();
	TextViewer& viewer = target();
	abortIncrementalSearch(viewer);

	Caret& caret = viewer.caret();
	const bool forward = (direction_ == Direction::FORWARD && n > 0) || (direction_ == Direction::BACKWARD && n < 0);
	n = abs(n);
	if(/*caret.isAutoCompletionRunning() &&*/ forward)
		utils::closeCompletionProposalsPopup(viewer);

	Document& document = viewer.document();
	const Position from(forward ? caret.beginning() : caret.end());
	text::WordBreakIterator<DocumentCharacterIterator> to(
		DocumentCharacterIterator(document, forward ? caret.end() : caret.beginning()),
		text::AbstractWordBreakIterator::START_OF_SEGMENT,
			viewer.document().contentTypeInformation().getIdentifierSyntax(caret.contentType()));
	for(Position p(to.base().tell()); n > 0; --n) {
		if(p == (forward ? ++to : --to).base().tell())
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
		} catch(const IDocumentInput::ChangeRejectedException&) {
			return false;
		}
	}
	return true;
}

/**
 * Constructor.
 * @param viewer the target text viewer
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


// isc.AinuInputSequenceChecker /////////////////////////////////////////////

/// @see InputSequenceChecker#check
bool AinuInputSequenceChecker::check(HKL, const Char* first, const Char* last, CodePoint cp) const {
	// only check a pair consists of combining semi-voiced sound mark is valid
	return cp != 0x309au || (first < last && (
		last[-1] == L'\x30bb'		// se (セ)
		|| last[-1] == L'\x30c4'	// tu (ツ)
		|| last[-1] == L'\x30c8'	// to (ト)
		|| last[-1] == L'\x31f7'));	// small fu (小さいフ)
}


// isc.ThaiInputSequenceChecker /////////////////////////////////////////////

const ThaiInputSequenceChecker::CharacterClass ThaiInputSequenceChecker::charClasses_[] = {
	CTRL, CONS, CONS, CONS, CONS, CONS, CONS, CONS,	// U+0E00
	CONS, CONS, CONS, CONS, CONS, CONS, CONS, CONS,
	CONS, CONS, CONS, CONS, CONS, CONS, CONS, CONS,	// U+0E10
	CONS, CONS, CONS, CONS, CONS, CONS, CONS, CONS,
	CONS, CONS, CONS, CONS, FV3,  CONS, FV3,  CONS,	// U+0E20
	CONS, CONS, CONS, CONS, CONS, CONS, CONS, NON,
	FV1,  AV2,  FV1,  FV1,  AV1,  AV3,  AV2,  AV3,	// U+0E30
	BV1,  BV2,  BD,   CTRL, CTRL, CTRL, CTRL, NON,
	LV,   LV,   LV,   LV,   LV,   FV2,  NON,  AD2,	// U+0E40
	TONE, TONE, TONE, TONE, AD1,  AD1,  AD3,  NON,
	NON,  NON,  NON,  NON,  NON,  NON,  NON,  NON,	// U+0E50
	NON,  NON,  NON,  NON,  CTRL, CTRL, CTRL, CTRL
};

const char ThaiInputSequenceChecker::checkMap_[] =
	"XAAAAAA" "RRRRRRRRRR"	// CTRL
	"XAAASSA" "RRRRRRRRRR"	// NON
	"XAAAASA" "CCCCCCCCCC"	// CONS
	"XSASSSS" "RRRRRRRRRR"	// LV
	"XSASASA" "RRRRRRRRRR"	// FV1
	"XAAAASA" "RRRRRRRRRR"	// FV2
	"XAAASAS" "RRRRRRRRRR"	// FV3
	"XAAAASA" "RRRCCRRRRR"	// BV1
	"XAAASSA" "RRRCRRRRRR"	// BV2
	"XAAASSA" "RRRRRRRRRR"	// BD 
	"XAAAAAA" "RRRRRRRRRR"	// TONE
	"XAAASSA" "RRRRRRRRRR"	// AD1	
	"XAAASSA" "RRRRRRRRRR"	// AD2	
	"XAAASSA" "RRRRRRRRRR"	// AD3	
	"XAAASSA" "RRRCCRRRRR"	// AV1	
	"XAAASSA" "RRRCRRRRRR"	// AV2	
	"XAAASSA" "RRRCRCRRRR";	// AV3

/// @see InputSequenceChecker#check
bool ThaiInputSequenceChecker::check(HKL, const Char* first, const Char* last, CodePoint cp) const {
	// standardized by WTT 2.0:
	// - http://mozart.inet.co.th/cyberclub/trin/thairef/wtt2/char-class.pdf
	// - http://www.nectec.or.th/it-standards/keyboard_layout/thai-key.htm
	if(mode_ == PASS_THROUGH)
		return true;
	return doCheck(
		(first != last) ? getCharacterClass(last[-1]) : CTRL,	// if there is not a preceding character, as if a control is
		getCharacterClass((cp != 0x0e33u) ? cp : 0x0e4du),		// Sara Am -> Nikhahit + Sara Aa
		mode_ == STRICT_MODE);
}


// isc.VietnameseInputSequenceChecker ///////////////////////////////////////

/// @see InputSequenceChecker#check
bool VietnameseInputSequenceChecker::check(HKL keyboardLayout, const Char* first, const Char* last, CodePoint cp) const {
	// The Vietnamese alphabet (quốc ngữ) has 12 vowels, 5 tone marks and other consonants. This
	// code checks if the input is conflicting the pattern <vowel> + <0 or 1 tone mark>. Does not
	// check when the input locale is not Vietnamese, because Vietnamese does not have own script
	// Like Uniscribe, ignored if the vowel is a composite.
	// 
	// Reference:
	// - Vietnamese alphabet (http://en.wikipedia.org/wiki/Vietnamese_alphabet)
	// - Vietnamese Writing System (http://www.cjvlang.com/Writing/writviet.html)
	static const CodePoint VOWELS[24] = {
		L'A', L'E', L'I', L'O', L'U', L'Y', L'a', L'e', L'i', L'o', L'u', L'y',
		0x00c2u, 0x00cau, 0x00d4u, 0x00e2u, 0x00eau, 0x00f4u, 0x0102u, 0x0103u, 0x01a0u, 0x01a1u, 0x01afu, 0x01b0u
	};
	static const CodePoint TONE_MARKS[5] = {0x0300u, 0x0301u, 0x0303u, 0x0309u, 0x0323u};

	if(PRIMARYLANGID(LOWORD(keyboardLayout)) != LANG_VIETNAMESE)
		return true;
	else if(first < last && binary_search(TONE_MARKS, MANAH_ENDOF(TONE_MARKS), cp))
		return binary_search(VOWELS, MANAH_ENDOF(VOWELS), last[-1]);
	return true;
}
