/**
 * @file command.cpp
 * @author exeal
 * @date 2006-2011 was text-editor.cpp
 * @date 2011-05-06
 * @date 2011-2012
 */

#include <ascension/text-editor/command.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/content-assist.hpp>
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
	inline bool abortIncrementalSearch(TextViewer& target) {
		if(Session* const session = target.document().session()) {
			if(session->incrementalSearcher().isRunning())
				return session->incrementalSearcher().abort(), true;
		}
		return false;
	}
	inline bool endIncrementalSearch(TextViewer& target) {
		if(Session* const session = target.document().session()) {
			if(session->incrementalSearcher().isRunning())
				return session->incrementalSearcher().end(), true;
		}
		return false;
	}
	inline bool abortModes(TextViewer& target) {
		utils::closeCompletionProposalsPopup(target);
		return abortIncrementalSearch(target);
	}
}

#define ASCENSION_ASSERT_IFISWINDOW() assert(target().isWindow())

// the command can't perform and throw if the document is read only
#define ASCENSION_CHECK_DOCUMENT_READ_ONLY()	\
	if(target().document().isReadOnly()) return false

namespace {
	typedef Position(*MovementProcedureP)(const Point&);
	typedef Position(*MovementProcedurePL)(const Point&, Index);
	typedef Position(*MovementProcedurePCL)(const Point&, locations::CharacterUnit, Index);
	typedef Position(*MovementProcedureV)(const VisualPoint&);
	typedef Position(*MovementProcedureVL)(const VisualPoint&, Index);
	typedef Position(*MovementProcedureVCL)(const VisualPoint&, locations::CharacterUnit, Index);
	typedef VerticalDestinationProxy(*MovementProcedureVLV)(const VisualPoint&, Index);

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
 * @param procedure A pointer to the member function defines the destination
 * @param extendSelection Set @c true to extend selection
 */
CaretMovementCommand::CaretMovementCommand(TextViewer& viewer,
		Position(*procedure)(const Point&), bool extendSelection /* = false */) :
		Command(viewer), extends_(extendSelection), procedureP_(procedure), procedurePL_(0), procedurePCL_(0),
		procedureV_(0), procedureVL_(0), procedureVCL_(0), procedureVLV_(0) {
	if(procedure == 0 || find(MOVEMENT_PROCEDURES_P,
			ASCENSION_ENDOF(MOVEMENT_PROCEDURES_P), procedure) == ASCENSION_ENDOF(MOVEMENT_PROCEDURES_P))
		throw invalid_argument("procedure");
}

/**
 * Constructor.
 * @param viewer The target text viewer
 * @param procedure A pointer to the member function defines the destination
 * @param extendSelection Set @c true to extend selection
 */
CaretMovementCommand::CaretMovementCommand(TextViewer& viewer,
		Position(*procedure)(const Point&, Index), bool extendSelection /* = false */) :
		Command(viewer), extends_(extendSelection), procedureP_(0), procedurePL_(procedure), procedurePCL_(0),
		procedureV_(0), procedureVL_(0), procedureVCL_(0), procedureVLV_(0) {
	if(procedure == 0 || find(MOVEMENT_PROCEDURES_PL,
			ASCENSION_ENDOF(MOVEMENT_PROCEDURES_PL), procedure) == ASCENSION_ENDOF(MOVEMENT_PROCEDURES_PL))
		throw invalid_argument("procedure");
}

/**
 * Constructor.
 * @param viewer The target text viewer
 * @param procedure A pointer to the member function defines the destination
 * @param extendSelection Set @c true to extend selection
 */
CaretMovementCommand::CaretMovementCommand(TextViewer& viewer,
		Position(*procedure)(const Point&, locations::CharacterUnit, Index), bool extendSelection /* = false */) :
		Command(viewer), extends_(extendSelection), procedureP_(0), procedurePL_(0), procedurePCL_(procedure),
		procedureV_(0), procedureVL_(0), procedureVCL_(0), procedureVLV_(0) {
	if(procedure == 0 || find(MOVEMENT_PROCEDURES_PCL,
			ASCENSION_ENDOF(MOVEMENT_PROCEDURES_PCL), procedure) == ASCENSION_ENDOF(MOVEMENT_PROCEDURES_PCL))
		throw invalid_argument("procedure");
}

/**
 * Constructor.
 * @param viewer The target text viewer
 * @param procedure A pointer to the member function defines the destination
 * @param extendSelection Set @c true to extend selection
 */
CaretMovementCommand::CaretMovementCommand(TextViewer& viewer,
		Position(*procedure)(const VisualPoint&), bool extendSelection /* = false */) :
		Command(viewer), extends_(extendSelection), procedureP_(0), procedurePL_(0), procedurePCL_(0),
		procedureV_(procedure), procedureVL_(0), procedureVCL_(0), procedureVLV_(0) {
	if(procedure == 0 || find(MOVEMENT_PROCEDURES_V,
			ASCENSION_ENDOF(MOVEMENT_PROCEDURES_V), procedure) == ASCENSION_ENDOF(MOVEMENT_PROCEDURES_V))
		throw invalid_argument("procedure");
}

/**
 * Constructor.
 * @param viewer The target text viewer
 * @param procedure A pointer to the member function defines the destination
 * @param extendSelection Set @c true to extend selection
 */
CaretMovementCommand::CaretMovementCommand(TextViewer& viewer,
		Position(*procedure)(const VisualPoint&, Index), bool extendSelection /* = false */) :
		Command(viewer), extends_(extendSelection), procedureP_(0), procedurePL_(0), procedurePCL_(0),
		procedureV_(0), procedureVL_(procedure), procedureVCL_(0), procedureVLV_(0) {
	if(procedure == 0 || find(MOVEMENT_PROCEDURES_VL,
			ASCENSION_ENDOF(MOVEMENT_PROCEDURES_VL), procedure) == ASCENSION_ENDOF(MOVEMENT_PROCEDURES_VL))
		throw invalid_argument("procedure");
}

/**
 * Constructor.
 * @param viewer The target text viewer
 * @param procedure A pointer to the member function defines the destination
 * @param extendSelection Set @c true to extend selection
 */
CaretMovementCommand::CaretMovementCommand(TextViewer& viewer,
		Position(*procedure)(const VisualPoint&, locations::CharacterUnit, Index), bool extendSelection /* = false */) :
		Command(viewer), extends_(extendSelection), procedureP_(0), procedurePL_(0), procedurePCL_(0),
		procedureV_(0), procedureVL_(0), procedureVCL_(procedure), procedureVLV_(0) {
	if(procedure == 0 || find(MOVEMENT_PROCEDURES_VCL,
			ASCENSION_ENDOF(MOVEMENT_PROCEDURES_VCL), procedure) == ASCENSION_ENDOF(MOVEMENT_PROCEDURES_VCL))
		throw invalid_argument("procedure");
}

/**
 * Constructor.
 * @param viewer The target text viewer
 * @param procedure A pointer to the member function defines the destination
 * @param extendSelection Set @c true to extend selection
 */
CaretMovementCommand::CaretMovementCommand(TextViewer& viewer,
		VerticalDestinationProxy(*procedure)(const VisualPoint&, Index), bool extendSelection /* = false */) :
		Command(viewer), extends_(extendSelection), procedureP_(0), procedurePL_(0), procedurePCL_(0),
		procedureV_(0), procedureVL_(0), procedureVCL_(0), procedureVLV_(procedure) {
	if(procedure == 0 || find(MOVEMENT_PROCEDURES_VLV,
			ASCENSION_ENDOF(MOVEMENT_PROCEDURES_VLV), procedure) == ASCENSION_ENDOF(MOVEMENT_PROCEDURES_VLV))
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
			if(contentassist::ContentAssistant* const ca = target().contentAssistant()) {
				if(contentassist::ContentAssistant::CompletionProposalsUI* const cpui = ca->completionProposalsUI()) {
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
			const bool rtl = defaultReadingDirection(target().presentation()) == presentation::RIGHT_TO_LEFT;
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
			} catch(const DocumentInput::ChangeRejectedException&) {
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

	if((fromPreviousLine_ && caret.line() == 0)
			|| (!fromPreviousLine_ && caret.line() >= document.numberOfLines() - 1))
		return false;
	
	const Position p((fromPreviousLine_ ? locations::backwardVisualLine(caret) : locations::forwardVisualLine(caret)).position());
	const String& line = document.line(caret.line() + (fromPreviousLine_ ? -1 : 1));
	if(p.offsetInLine >= line.length())
		return false;
	setNumericPrefix(1);
	return CharacterInputCommand(target(), text::surrogates::decodeFirst(line.begin() + p.offsetInLine, line.end()))();
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
	const Char* const line = document.line(eos.line()).data();
	const CodePoint cp = text::surrogates::decodeLast(line, line + eos.offsetInLine());
	Char buffer[7];
#if(_MSC_VER < 1400)
	swprintf(buffer, L"%lX", cp);
#else
	swprintf(buffer, ASCENSION_COUNTOF(buffer), L"%lX", cp);
#endif // _MSC_VER < 1400
	AutoFreeze af(&viewer);
	caret.select(Position(eos.line(), eos.offsetInLine() - ((cp > 0xffff) ? 2 : 1)), eos);
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
	const Char* const line = document.line(eos.line()).data();
	const Index offsetInLine = eos.offsetInLine();

	// accept /(?:[Uu]\+)?[0-9A-Fa-f]{1,6}/
	if(iswxdigit(line[offsetInLine - 1]) != 0) {
		Index i = offsetInLine - 1;
		while(i != 0) {
			if(offsetInLine - i == 7)
				return false;	// too long string
			else if(iswxdigit(line[i - 1]) == 0)
				break;
			--i;
		}

		Char buffer[7];
		wcsncpy(buffer, line + i, offsetInLine - i);
		buffer[offsetInLine - i] = 0;
		const CodePoint cp = wcstoul(buffer, 0, 16);
		if(text::isValidCodePoint(cp)) {
			buffer[1] = buffer[2] = 0;
			text::surrogates::encode(cp, buffer);
			if(i >= 2 && line[i - 1] == L'+' && (line[i - 2] == L'U' || line[i - 2] == L'u'))
				i -= 2;
			AutoFreeze af(&viewer);
			caret.select(Position(eos.line(), i), eos);
			try {
				caret.replaceSelection(StringPiece(buffer, (cp < 0x10000u ? 1 : 2)), false);
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
	long n = numericPrefix();
	if(n == 0)
		return 0;
	endIncrementalSearch(target());
	utils::closeCompletionProposalsPopup(target());

	win32::WaitCursor wc;
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
 * @param viewer The target text viewer
 * @param type The search type
 * @param direction The direction to search
 * @param callback The callback object for the incremental search. Can be @c null
 */
IncrementalFindCommand::IncrementalFindCommand(TextViewer& viewer, searcher::TextSearcher::Type type,
		Direction direction, searcher::IncrementalSearchCallback* callback /* = 0 */) /*throw()*/
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
	const long n = numericPrefix();
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
		indentByTabs(caret, caret.isSelectionRectangle(), n);
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
	if(HIMC imc = ::ImmGetContext(target().handle().get())) {
		const bool succeeded = win32::boole(::ImmSetOpenStatus(imc, !win32::boole(::ImmGetOpenStatus(imc))));
		::ImmReleaseContext(target().handle().get(), imc);
		return succeeded;
	}
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
	if(HIMC imc = ::ImmGetContext(target().handle().get())) {
		DWORD conversionMode, sentenceMode;
		if(win32::boole(::ImmGetConversionStatus(imc, &conversionMode, &sentenceMode))) {
			conversionMode = win32::boole(conversionMode & IME_CMODE_SOFTKBD) ?
				(conversionMode & ~IME_CMODE_SOFTKBD) : (conversionMode | IME_CMODE_SOFTKBD);
			const bool succeeded = win32::boole(::ImmSetConversionStatus(imc, conversionMode, sentenceMode));
			::ImmReleaseContext(target().handle().get(), imc);
			return succeeded;
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
	const Position matchBracket(caret.matchBrackets().first);
	if(matchBracket == Position())
		return false;	// not found
	caret.endRectangleSelection();
	if(!extends_)
		caret.moveTo(matchBracket);
	else if(matchBracket > caret)
		caret.select(caret, Position(matchBracket.line, matchBracket.offsetInLine + 1));
	else
		caret.select(Position(caret.line(), caret.offsetInLine() + 1), matchBracket);
	return true;
}

/**
 * Constructor.
 * @param viewer The target text viewer
 * @param insertPrevious Set @c true to insert on previous line. Otherwise on the current line
 */
NewlineCommand::NewlineCommand(TextViewer& viewer, bool insertPrevious) /*throw()*/ : Command(viewer), insertsPrevious_(insertPrevious) {
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

	if(endIncrementalSearch(viewer))
		return true;

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
			caret.moveTo(viewer.document().region().first);
		caret.enableAutoShow(autoShow);
	}

	try {
		viewer.document().insertUndoBoundary();
		breakLine(caret, false, numericPrefix());
	} catch(const DocumentInput::ChangeRejectedException&) {
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
		if(HIMC imc = ::ImmGetContext(viewer.handle().get())) {
			if(!win32::boole(::ImmGetOpenStatus(imc)))	// without this, IME may ignore us?
				::ImmSetOpenStatus(imc, true);

			// from NotePadView.pas of TNotePad (http://wantech.ikuto.com/)
			const bool multilineSelection = caret.line() != caret.anchor().line();
			const String s(multilineSelection ? selectedString(caret) : viewer.document().line(caret.line()));
			const DWORD bytes = static_cast<DWORD>(sizeof(RECONVERTSTRING) + sizeof(Char) * s.length());
			RECONVERTSTRING* rcs;
			try {
				rcs = static_cast<RECONVERTSTRING*>(::operator new(bytes));
			} catch(const bad_alloc&) {
				::ImmReleaseContext(viewer.handle().get(), imc);
				throw;	// failed to allocate the memory for RECONVERTSTRING
			}
			rcs->dwSize = bytes;
			rcs->dwVersion = 0;
			rcs->dwStrLen = static_cast<DWORD>(s.length());
			rcs->dwStrOffset = sizeof(RECONVERTSTRING);
			rcs->dwCompStrLen = rcs->dwTargetStrLen =
				static_cast<DWORD>(multilineSelection ? s.length() : (caret.end().offsetInLine() - caret.beginning().offsetInLine()));
			rcs->dwCompStrOffset = rcs->dwTargetStrOffset =
				multilineSelection ? 0 : static_cast<DWORD>(sizeof(Char) * caret.beginning().offsetInLine());
			s.copy(reinterpret_cast<Char*>(reinterpret_cast<char*>(rcs) + rcs->dwStrOffset), s.length());

			if(isSelectionEmpty(caret)) {
				// IME selects the composition target automatically if no selection
				if(win32::boole(::ImmSetCompositionStringW(imc, SCS_QUERYRECONVERTSTRING, rcs, rcs->dwSize, 0, 0))) {
					caret.select(
						Position(caret.line(), rcs->dwCompStrOffset / sizeof(Char)),
						Position(caret.line(), rcs->dwCompStrOffset / sizeof(Char) + rcs->dwCompStrLen));
					if(win32::boole(::ImmSetCompositionStringW(imc, SCS_SETRECONVERTSTRING, rcs, rcs->dwSize, 0, 0)))
						succeeded = true;
				}
			}
			::operator delete(rcs);
			::ImmReleaseContext(viewer.handle().get(), imc);
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
 * @param procedure A pointer to the member function defines the destination
 */
RowSelectionExtensionCommand::RowSelectionExtensionCommand(TextViewer& viewer,
		Position(*procedure)(const Point&)) : Command(viewer), procedureP_(procedure), procedurePL_(0), procedurePCL_(0),
		procedureV_(0), procedureVL_(0), procedureVCL_(0), procedureVLV_(0) {
	if(procedure == 0 || find(MOVEMENT_PROCEDURES_P,
			ASCENSION_ENDOF(MOVEMENT_PROCEDURES_P), procedure) == ASCENSION_ENDOF(MOVEMENT_PROCEDURES_P))
		throw invalid_argument("procedure");
}

/**
 * Constructor.
 * @param viewer The target text viewer
 * @param procedure A pointer to the member function defines the destination
 */
RowSelectionExtensionCommand::RowSelectionExtensionCommand(TextViewer& viewer,
		Position(*procedure)(const Point&, Index)) : Command(viewer), procedureP_(0), procedurePL_(procedure), procedurePCL_(0),
		procedureV_(0), procedureVL_(0), procedureVCL_(0), procedureVLV_(0) {
	if(procedure == 0 || find(MOVEMENT_PROCEDURES_PL,
			ASCENSION_ENDOF(MOVEMENT_PROCEDURES_PL), procedure) == ASCENSION_ENDOF(MOVEMENT_PROCEDURES_PL))
		throw invalid_argument("procedure");
}

/**
 * Constructor.
 * @param viewer The target text viewer
 * @param procedure A pointer to the member function defines the destination
 */
RowSelectionExtensionCommand::RowSelectionExtensionCommand(TextViewer& viewer,
		Position(*procedure)(const Point&, locations::CharacterUnit, Index)) : Command(viewer), procedureP_(0), procedurePL_(0), procedurePCL_(procedure),
		procedureV_(0), procedureVL_(0), procedureVCL_(0), procedureVLV_(0) {
	if(procedure == 0 || find(MOVEMENT_PROCEDURES_PCL,
			ASCENSION_ENDOF(MOVEMENT_PROCEDURES_PCL), procedure) == ASCENSION_ENDOF(MOVEMENT_PROCEDURES_PCL))
		throw invalid_argument("procedure");
}

/**
 * Constructor.
 * @param viewer The target text viewer
 * @param procedure A pointer to the member function defines the destination
 */
RowSelectionExtensionCommand::RowSelectionExtensionCommand(TextViewer& viewer,
		Position(*procedure)(const VisualPoint&)) : Command(viewer), procedureP_(0), procedurePL_(0), procedurePCL_(0),
		procedureV_(procedure), procedureVL_(0), procedureVCL_(0), procedureVLV_(0) {
	if(procedure == 0 || find(MOVEMENT_PROCEDURES_V,
			ASCENSION_ENDOF(MOVEMENT_PROCEDURES_V), procedure) == ASCENSION_ENDOF(MOVEMENT_PROCEDURES_V))
		throw invalid_argument("procedure");
}

/**
 * Constructor.
 * @param viewer The target text viewer
 * @param procedure A pointer to the member function defines the destination
 */
RowSelectionExtensionCommand::RowSelectionExtensionCommand(TextViewer& viewer,
		Position(*procedure)(const VisualPoint&, Index)) : Command(viewer), procedureP_(0), procedurePL_(0), procedurePCL_(0),
		procedureV_(0), procedureVL_(procedure), procedureVCL_(0), procedureVLV_(0) {
	if(procedure == 0 || find(MOVEMENT_PROCEDURES_VL,
			ASCENSION_ENDOF(MOVEMENT_PROCEDURES_VL), procedure) == ASCENSION_ENDOF(MOVEMENT_PROCEDURES_VL))
		throw invalid_argument("procedure");
}

/**
 * Constructor.
 * @param viewer The target text viewer
 * @param procedure A pointer to the member function defines the destination
 */
RowSelectionExtensionCommand::RowSelectionExtensionCommand(TextViewer& viewer,
		Position(*procedure)(const VisualPoint&, locations::CharacterUnit, Index)) : Command(viewer), procedureP_(0), procedurePL_(0), procedurePCL_(0),
		procedureV_(0), procedureVL_(0), procedureVCL_(procedure), procedureVLV_(0) {
	if(procedure == 0 || find(MOVEMENT_PROCEDURES_VCL,
			ASCENSION_ENDOF(MOVEMENT_PROCEDURES_VCL), procedure) == ASCENSION_ENDOF(MOVEMENT_PROCEDURES_VCL))
		throw invalid_argument("procedure");
}

/**
 * Constructor.
 * @param viewer The target text viewer
 * @param procedure A pointer to the member function defines the destination
 */
RowSelectionExtensionCommand::RowSelectionExtensionCommand(TextViewer& viewer,
		VerticalDestinationProxy(*procedure)(const VisualPoint&, Index)) : Command(viewer), procedureP_(0), procedurePL_(0), procedurePCL_(0),
		procedureV_(0), procedureVL_(0), procedureVCL_(0), procedureVLV_(procedure) {
	if(procedure == 0 || find(MOVEMENT_PROCEDURES_VLV,
			ASCENSION_ENDOF(MOVEMENT_PROCEDURES_VLV), procedure) == ASCENSION_ENDOF(MOVEMENT_PROCEDURES_VLV))
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
		return CaretMovementCommand(target(), procedureP_, true).setNumericPrefix(numericPrefix())();
	else if(procedurePL_ != 0)
		return CaretMovementCommand(target(), procedurePL_, true).setNumericPrefix(numericPrefix())();
	else if(procedurePCL_ != 0)
		return CaretMovementCommand(target(), procedurePCL_, true).setNumericPrefix(numericPrefix())();
	else if(procedureV_ != 0)
		return CaretMovementCommand(target(), procedureV_, true).setNumericPrefix(numericPrefix())();
	else if(procedureVL_ != 0)
		return CaretMovementCommand(target(), procedureVL_, true).setNumericPrefix(numericPrefix())();
	else if(procedureVCL_ != 0)
		return CaretMovementCommand(target(), procedureVCL_, true).setNumericPrefix(numericPrefix())();
	else
		return CaretMovementCommand(target(), procedureVLV_, true).setNumericPrefix(numericPrefix())();
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
	const long n = numericPrefix();
	if(n <= 0)
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
