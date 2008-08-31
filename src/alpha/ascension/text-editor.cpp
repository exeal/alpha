/**
 * @file text-editor.cpp
 * @author exeal
 * @date 2006-2008
 */

#include "text-editor.hpp"
#include "content-assist.hpp"
#include "../../manah/win32/utility.hpp"
#include "../../manah/win32/ui/wait-cursor.hpp"
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

/**
 * Sets bookmarks.
 * @return the number of marked lines
 */
ulong BookmarkAllCommand::execute() {
	ABORT_MODES();
	const bool onlySelection = param_;
    if(onlySelection && target().caret().isSelectionEmpty())
		return 0;

	WaitCursor wc;
	TextViewer& viewer = target();
	Document& document = viewer.document();
	const searcher::TextSearcher* s;
	if(const Session* const session = document.session())
		s = &session->textSearcher();
	else
		return 0;	// TODO: prepares a default text searcher.

	ulong count = 0;
	Region scope(
		onlySelection ? max<Position>(viewer.caret().beginning(),
			document.accessibleRegion().first) : document.accessibleRegion().first,
		onlySelection ? min<Position>(viewer.caret().end(),
			document.accessibleRegion().second) : document.accessibleRegion().second);

	Bookmarker& bookmarker = document.bookmarker();
	Region matchedRegion;
	while(s->search(document,
			max<Position>(viewer.caret().beginning(), document.accessibleRegion().first),
			scope, FORWARD, matchedRegion)) {
		bookmarker.mark(matchedRegion.first.line);
		scope.first.line = matchedRegion.first.line + 1;
		scope.first.column = 0;
		++count;
	}
	return count;
}

/**
 * Removes all bookmarks or toggles the bookmark on the caret line.
 * @return 0
 */
ulong BookmarkCommand::execute() {
	ABORT_MODES();
	if(type_ == CLEAR_ALL)
		target().document().bookmarker().clear();
	else if(type_ == TOGGLE_CURRENT_LINE)
		target().document().bookmarker().toggle(target().caret().lineNumber());
	else
		assert(false);
	return 0;
}

/**
 * Clears the selection or abort the active incremental search explicitly.
 * @return 0
 */
ulong CancelCommand::execute() {
	ASSERT_IFISWINDOW();
	ABORT_MODES();
	target().caret().clearSelection();
	return 0;
}

/**
 * Moves the caret or extends the selection.
 * @retval 1 the type is any of @c MATCH_BRACKET, @c NEXT_BOOKMARK, or @c PREVIOUS_BOOKMARK and the next mark not found
 * @retval 0 otherwise
 */
ulong CaretMovementCommand::execute() {
	END_ISEARCH();
	Caret& caret = target().caret();

	if(!extend_) {
		if(type_ == NEXT_LINE || type_ == NEXT_VISUAL_LINE || type_ == PREVIOUS_LINE
				|| type_ == PREVIOUS_VISUAL_LINE || type_ == NEXT_PAGE || type_ == PREVIOUS_PAGE) {
			if(contentassist::IContentAssistant* const ca = target().contentAssistant()) {
				if(contentassist::IContentAssistant::ICompletionProposalsUI* const cpui = ca->getCompletionProposalsUI()) {
					switch(type_) {
					case NEXT_LINE:
					case NEXT_VISUAL_LINE:		cpui->nextProposal(+1); break;
					case PREVIOUS_LINE:
					case PREVIOUS_VISUAL_LINE:	cpui->nextProposal(-1); break;
					case NEXT_PAGE:				cpui->nextPage(+1); break;
					case PREVIOUS_PAGE:			cpui->nextPage(-1); break;
					}
					return 0;
				}
			}
		}
		caret.endRectangleSelection();
		if(!caret.isSelectionEmpty()) {	// just clear the selection
			const bool rtl = target().configuration().orientation == layout::RIGHT_TO_LEFT;
			if(type_ == FORWARD_CHARACTER
					|| (type_ == RIGHT_CHARACTER && !rtl)
					|| (type_ == LEFT_CHARACTER && rtl)) {
				caret.moveTo(caret.end());
				return 0;
			} else if(type_ == BACKWARD_CHARACTER
					|| (type_ == LEFT_CHARACTER && !rtl)
					|| (type_ == RIGHT_CHARACTER && rtl)) {
				caret.moveTo(caret.beginning());
				return 0;
			}
		}
	}

	if(type_ == MATCH_BRACKET) {
		Position foundPosition(caret.matchBrackets().first);

		if(foundPosition == Position::INVALID_POSITION) {
			target().beep();	// not found
			return 1;
		}
		if(!extend_)
			caret.moveTo(foundPosition);
		else if(foundPosition > caret)
			caret.select(caret, Position(foundPosition.line, foundPosition.column + 1));
		else
			caret.select(Position(caret.lineNumber(), caret.columnNumber() + 1), foundPosition);
	} else {
		int type = type_;
		switch(type) {
		case CONTEXTUAL_BEGINNING_OF_LINE:
			type = caret.isFirstPrintableCharacterOfLine() ? BEGINNING_OF_LINE : FIRST_PRINTABLE_CHARACTER_OF_LINE; break;
		case CONTEXTUAL_END_OF_LINE:
			type = caret.isLastPrintableCharacterOfLine() ? END_OF_LINE : LAST_PRINTABLE_CHARACTER_OF_LINE; break;
		case CONTEXTUAL_BEGINNING_OF_VISUAL_LINE:
			type = caret.isFirstPrintableCharacterOfVisualLine() ? BEGINNING_OF_VISUAL_LINE : FIRST_PRINTABLE_CHARACTER_OF_VISUAL_LINE; break;
		case CONTEXTUAL_END_OF_VISUAL_LINE:
			type = caret.isLastPrintableCharacterOfVisualLine() ? END_OF_VISUAL_LINE : LAST_PRINTABLE_CHARACTER_OF_VISUAL_LINE; break;
		}
		switch(type) {
#define ASCENSION_HANDLE_CARET_MOVEMENT(commandType, className, methodName)	\
	case commandType: extend_ ? caret.extendSelection(mem_fun(className::methodName), offset_) : caret.methodName(offset_); break
			ASCENSION_HANDLE_CARET_MOVEMENT(FORWARD_CHARACTER, EditPoint, forwardCharacter);
			ASCENSION_HANDLE_CARET_MOVEMENT(BACKWARD_CHARACTER, EditPoint, backwardCharacter);
			ASCENSION_HANDLE_CARET_MOVEMENT(LEFT_CHARACTER, VisualPoint, leftCharacter);
			ASCENSION_HANDLE_CARET_MOVEMENT(RIGHT_CHARACTER, VisualPoint, rightCharacter);
			ASCENSION_HANDLE_CARET_MOVEMENT(NEXT_WORD, VisualPoint, nextWord);
			ASCENSION_HANDLE_CARET_MOVEMENT(PREVIOUS_WORD, VisualPoint, previousWord);
			ASCENSION_HANDLE_CARET_MOVEMENT(LEFT_WORD, VisualPoint, leftWord);
			ASCENSION_HANDLE_CARET_MOVEMENT(RIGHT_WORD, VisualPoint, rightWord);
			ASCENSION_HANDLE_CARET_MOVEMENT(NEXT_WORDEND, VisualPoint, nextWordEnd);
			ASCENSION_HANDLE_CARET_MOVEMENT(PREVIOUS_WORDEND, VisualPoint, previousWordEnd);
			ASCENSION_HANDLE_CARET_MOVEMENT(LEFT_WORDEND, VisualPoint, leftWordEnd);
			ASCENSION_HANDLE_CARET_MOVEMENT(RIGHT_WORDEND, VisualPoint, rightWordEnd);
			ASCENSION_HANDLE_CARET_MOVEMENT(NEXT_LINE, VisualPoint, nextLine);
			ASCENSION_HANDLE_CARET_MOVEMENT(PREVIOUS_LINE, VisualPoint, previousLine);
			ASCENSION_HANDLE_CARET_MOVEMENT(NEXT_VISUAL_LINE, VisualPoint, nextVisualLine);
			ASCENSION_HANDLE_CARET_MOVEMENT(PREVIOUS_VISUAL_LINE, VisualPoint, previousVisualLine);
#undef ASCENSION_HANDLE_CARET_MOVEMENT
		case NEXT_PAGE:
			target().sendMessage(WM_VSCROLL, SB_PAGEDOWN);
			extend_ ? caret.extendSelection(mem_fun(VisualPoint::nextPage), offset_) : caret.nextPage(offset_);
			break;
		case PREVIOUS_PAGE:
			target().sendMessage(WM_VSCROLL, SB_PAGEUP);
			extend_ ? caret.extendSelection(mem_fun(VisualPoint::previousPage), offset_) : caret.previousPage(offset_);
			break;
#define ASCENSION_HANDLE_CARET_MOVEMENT(commandType, className, methodName)	\
	case commandType: extend_ ? caret.extendSelection(mem_fun(className::methodName)) : caret.methodName(); break
			ASCENSION_HANDLE_CARET_MOVEMENT(BEGINNING_OF_LINE, EditPoint, beginningOfLine);
			ASCENSION_HANDLE_CARET_MOVEMENT(END_OF_LINE, EditPoint, endOfLine);
			ASCENSION_HANDLE_CARET_MOVEMENT(FIRST_PRINTABLE_CHARACTER_OF_LINE, VisualPoint, firstPrintableCharacterOfLine);
			ASCENSION_HANDLE_CARET_MOVEMENT(LAST_PRINTABLE_CHARACTER_OF_LINE, VisualPoint, lastPrintableCharacterOfLine);
			ASCENSION_HANDLE_CARET_MOVEMENT(BEGINNING_OF_VISUAL_LINE, VisualPoint, beginningOfVisualLine);
			ASCENSION_HANDLE_CARET_MOVEMENT(END_OF_VISUAL_LINE, VisualPoint, endOfVisualLine);
			ASCENSION_HANDLE_CARET_MOVEMENT(FIRST_PRINTABLE_CHARACTER_OF_VISUAL_LINE, VisualPoint, firstPrintableCharacterOfVisualLine);
			ASCENSION_HANDLE_CARET_MOVEMENT(LAST_PRINTABLE_CHARACTER_OF_VISUAL_LINE, VisualPoint, lastPrintableCharacterOfVisualLine);
			ASCENSION_HANDLE_CARET_MOVEMENT(BEGINNING_OF_DOCUMENT, EditPoint, beginningOfDocument);
			ASCENSION_HANDLE_CARET_MOVEMENT(END_OF_DOCUMENT, EditPoint ,endOfDocument);
#undef ASCENSION_HANDLE_CARET_MOVEMENT
		case NEXT_BOOKMARK:
			return caret.nextBookmark() ? 0 : 1;
		case PREVIOUS_BOOKMARK:
			return caret.previousBookmark() ? 0 : 1;
		}
	}
	return 0;
}

/**
 * Converts the character on the caret to a corresponding code point or the string represents code point to a corresponding character.
 * @retval 0 succeeded
 * @retval 1 failed
 */
ulong CharacterCodePointConversionCommand::execute() {
	CHECK_DOCUMENT_READONLY(1);
	ABORT_MODES();

	using namespace ascension::text;

	TextViewer& viewer = target();
	const Document& document = viewer.document();
	const EditPoint& bottom = viewer.caret().end();

	if(bottom.isBeginningOfLine()
			|| (document.isNarrowed() && bottom.position() == document.accessibleRegion().first)) {	// 行頭以外でなければならぬ
		viewer.beep();
		return 1;
	}

	Caret& caret = viewer.caret();
	const Char* const line = document.line(bottom.lineNumber()).data();
	CodePoint cp;
	Char buffer[7];

	if(param_) {	// 文字 -> コードポイント
		cp = surrogates::decodeLast(line, line + bottom.columnNumber());
		swprintf(buffer, L"%lX", cp);
		viewer.freeze();
		caret.select(Position(bottom.lineNumber(), bottom.columnNumber() - ((cp > 0xFFFF) ? 2 : 1)), bottom);
		caret.replaceSelection(buffer, buffer + wcslen(buffer), false);
		viewer.unfreeze();
	} else {	// コードポイント -> 文字
		const length_t column = bottom.columnNumber();
		length_t i = column - 1;

		// 変換できるのは "N" 、"U+N" および "u+N" のいずれか (N は 6 桁以下の16進数)
		if(toBoolean(iswxdigit(line[column - 1]))) {
			while(i != 0) {
				if(column - i == 7) {
					viewer.beep();
					return 1;
				} else if(!toBoolean(iswxdigit(line[i - 1])))
					break;
				--i;
			}
			wcsncpy(buffer, line + i, column - i);
			buffer[column - i] = 0;
			cp = wcstoul(buffer, 0, 16);
			if(isValidCodePoint(cp)) {
				buffer[1] = buffer[2] = 0;
				surrogates::encode(cp, buffer);
				if(i >= 2 && line[i - 1] == L'+' && (line[i - 2] == L'U' || line[i - 2] == L'u'))
					i -= 2;
				viewer.freeze();
				caret.select(Position(bottom.lineNumber(), i), bottom);
				caret.replaceSelection(buffer, buffer + (cp < 0x10000U ? 1 : 2), false);
				viewer.unfreeze();
				return 0;
			}
		}
		viewer.beep();
		return 1;
	}
	return 0;
}

/**
 * Inputs a character. If the incremental search is active, appends a character to the end of the pattern.
 * @retval 1 failed and the incremental search is not active
 * @retval 0 otherwise
 * @see Caret#inputCharacter, TextViewer#onChar, TextViewer#onUniChar
 */
ulong CharacterInputCommand::execute() {
	// インクリメンタル検索中 -> 検索式に追加
	if(Session* const session = target().document().session()) {
		if(session->incrementalSearcher().isRunning()) {
			CLOSE_COMPLETION_PROPOSAL_POPUP();
			if(param_ == 0x0009 || !toBoolean(iswcntrl(static_cast<wint_t>(param_))))
				session->incrementalSearcher().addCharacter(param_);
			return 0;
		}
	}
	return target().caret().inputCharacter(param_) ? 1 : 0;
}

/**
 * Inputs a character on same column in next or previous visual line.
 * @retval 0 succeeded
 * @retval 1 failed
 */
ulong CharacterInputFromNextLineCommand::execute() {
	ABORT_ISEARCH();
	CHECK_DOCUMENT_READONLY(1);

	// TODO: recognizes narrowing.

	const Document& document = target().document();
	const VisualPoint& caret = target().caret();
	const bool& fromPrevious = !param_;	// 名前が分かりにくいんで...

	if((caret.lineNumber() == 0 && fromPrevious)
			|| (caret.lineNumber() == document.numberOfLines() - 1 && !fromPrevious)) {
		target().beep();
		return 1;
	}

	// 編集点を作って位置を計算させる
	VisualPoint p(caret);
	p.adaptToDocument(false);
	if(fromPrevious)
		p.previousVisualLine();
	else
		p.nextVisualLine();

	const length_t column = p.columnNumber();
	const String& line = document.line(caret.lineNumber() + (fromPrevious ? -1 : 1));
	if(column >= line.length()) {
		target().beep();
		return 1;
	}
	return CharacterInputCommand(target(), text::surrogates::decodeFirst(line.begin() + column, line.end())).execute();
}

/**
 * Clipboard related operation.
 * @retval 0 succeeded
 * @retval 1 failed
 */
ulong ClipboardCommand::execute() {
	if(type_ == CUT || type_ == PASTE) {
		ASSERT_IFISWINDOW();
		CHECK_DOCUMENT_READONLY(1);
		CLOSE_COMPLETION_PROPOSAL_POPUP();
		if(type_ == CUT)
			ABORT_ISEARCH();
	}
	try {
		if(type_ == COPY)
			target().caret().copySelection(performClipboardRing_);
		else if(type_ == CUT)
			target().caret().cutSelection(performClipboardRing_);
		else if(type_ == PASTE)
			target().caret().pasteToSelection(performClipboardRing_);
	} catch(...) {
		return 1;
	}
	return 0;
}

/**
 * Show the completion proposal popup.
 * @retval 0 succeeded
 * @retval 1 failed
 */
ulong CompletionProposalPopupCommand::execute() {
	CHECK_DOCUMENT_READONLY(1);
//	CHECK_GUI_EDITABILITY(1);
	ABORT_ISEARCH();
	if(contentassist::IContentAssistant* ca = target().contentAssistant()) {
		ca->showPossibleCompletions();
		return 0;
	} else {
		target().beep();
		return 1;
	}
}

/**
 * Deletes a character, a word, a whole line, or the incremental search pattern.
 * @retval 0 succeeded
 * @retval 1 failed
 */
ulong DeletionCommand::execute() {
	CHECK_DOCUMENT_READONLY(1);
	if(type_ != NEXT_CHARACTER && type_ != PREVIOUS_CHARACTER)
		ABORT_ISEARCH();

	TextViewer& viewer = target();
	Caret& caret = viewer.caret();
	if(/*caret.isAutoCompletionRunning() &&*/ type_ != PREVIOUS_CHARACTER)
		CLOSE_COMPLETION_PROPOSAL_POPUP();

	Document& document = viewer.document();
	searcher::IncrementalSearcher* isearch = 0;
	if(Session* const session = document.session())
		isearch = &session->incrementalSearcher();
	if(isearch != 0 && isearch->isRunning()) {
		if(type_ == NEXT_CHARACTER)
			isearch->reset();
		else if(type_ == PREVIOUS_CHARACTER) {
			if(!isearch->canUndo())
				viewer.beep();
			else
				isearch->undo();
		}
	} else if(type_ == NEXT_WORD || type_ == PREVIOUS_WORD) {
		const Position from = (type_ == NEXT_WORD) ? caret.beginning() : caret.end();
		text::WordBreakIterator<DocumentCharacterIterator> to(
			DocumentCharacterIterator(document, (type_ == NEXT_WORD) ? caret.end() : caret.beginning()),
			text::AbstractWordBreakIterator::START_OF_SEGMENT,
				viewer.document().contentTypeInformation().getIdentifierSyntax(caret.getContentType()));
		(type_ == NEXT_WORD) ? ++to : --to;
		if(to.base().tell() != from) {
			viewer.freeze();
			document.beginSequentialEdit();
			caret.moveTo(document.erase(from, to.base().tell()));
			document.endSequentialEdit();
			viewer.unfreeze();
		}
	} else if(!caret.isSelectionEmpty()) {	// 選択を削除
		viewer.freeze();
		document.beginSequentialEdit();
		caret.eraseSelection();
		document.endSequentialEdit();
		viewer.unfreeze();
	} else if(type_ == NEXT_CHARACTER) {
		document.endSequentialEdit();
		caret.erase(1, EditPoint::GRAPHEME_CLUSTER);
	} else if(type_ == PREVIOUS_CHARACTER) {
		document.endSequentialEdit();
		caret.erase(-1, EditPoint::UTF32_CODE_UNIT);
	} else if(type_ == WHOLE_LINE) {
		const length_t line = caret.lineNumber();
		document.endSequentialEdit();
		if(line != document.numberOfLines() - 1)	// 最終行でない場合
			caret.nextLine();
		document.erase(Position(line, 0), Position(line, INVALID_INDEX));
	} else
		assert(false);
	return 0;
}

/**
 * Searches and selects the next matched text.
 * @return 1 if no text matched or the command failed. otherwise 0
 */
ulong FindNextCommand::execute() {
	END_ISEARCH();
	CLOSE_COMPLETION_PROPOSAL_POPUP();

	using namespace ascension::searcher;

	WaitCursor wc;
	TextViewer& viewer = target();
	Document& document = viewer.document();
	Caret& caret = viewer.caret();
	const TextSearcher* s;
	if(const Session* const session = document.session())
		s = &session->textSearcher();
	else
		return 0;	// TODO: prepares a default text searcher.

	const Region scope(document.accessibleRegion());
	Region matchedRegion;
	bool found = s->search(document,
		(param_ == FORWARD) ? max<Position>(caret.end(), scope.first) : min<Position>(caret.beginning(), scope.second),
		scope, param_, matchedRegion);

	if(found) {
		caret.select(matchedRegion);
//		viewer.highlightMatchTexts();
		return 0;
	} else {
//		viewer.highlightMatchTexts(false);
		return 1;
	}
}

/**
 * Starts the incremental search, or jumps to the next matched position if active.
 * @return 0
 */
ulong IncrementalSearchCommand::execute() {
	CLOSE_COMPLETION_PROPOSAL_POPUP();
	if(Session* const session = target().document().session()) {
		searcher::IncrementalSearcher& isearch = session->incrementalSearcher();
		if(!isearch.isRunning())	// 開始
			isearch.start(target().document(), target().caret(), session->textSearcher(), direction_, callback_);
		else {	// 次の一致位置へジャンプ (検索方向のみ有効。type_ は無視される)
			if(!isearch.next(direction_))
				target().beep();
		}
	}
	return 0;
}

/**
 * Indents the selected lines.
 * @retval 0 succeeded
 * @retval 1 failed
 */
ulong IndentationCommand::execute() {
	CHECK_DOCUMENT_READONLY(1);
//	CHECK_GUI_EDITABILITY(1);
	END_ISEARCH();
	CLOSE_COMPLETION_PROPOSAL_POPUP();

	TextViewer& viewer = target();
	Caret& caret = viewer.caret();
	viewer.document().beginSequentialEdit();
	viewer.freeze();
	Position anchorResult = tabIndent_ ?
		caret.tabIndent(caret.anchor(), caret.isSelectionRectangle(), level_ * (indent_ ? 1 : -1))
		: caret.spaceIndent(caret.anchor(), caret.isSelectionRectangle(), level_ * (indent_ ? 1 : -1));
	viewer.document().endSequentialEdit();
	caret.select(anchorResult, caret);
	viewer.unfreeze();

	return 0;
}

/**
 * Toggles any mode of IME, overtyping, or soft keyboard.
 * @return 0
 */
ulong InputStatusToggleCommand::execute() {
	if(type_ == IME_STATUS) {
		assert(target().isWindow());
		::HIMC imc = ::ImmGetContext(target().getHandle());
		::ImmSetOpenStatus(imc, !toBoolean(::ImmGetOpenStatus(imc)));
		::ImmReleaseContext(target().getHandle(), imc);
	} else if(type_ == OVERTYPE_MODE) {
		Caret& caret = target().caret();
		caret.setOvertypeMode(!caret.isOvertypeMode());
		CLOSE_COMPLETION_PROPOSAL_POPUP();
	} else if(type_ == SOFT_KEYBOARD) {
		assert(target().isWindow());
		::HIMC imc = ::ImmGetContext(target().getHandle());
		::DWORD conversionMode, sentenceMode;
		::ImmGetConversionStatus(imc, &conversionMode, &sentenceMode);
		conversionMode = toBoolean(conversionMode & IME_CMODE_SOFTKBD) ?
			(conversionMode & ~IME_CMODE_SOFTKBD) : (conversionMode | IME_CMODE_SOFTKBD);
		::ImmSetConversionStatus(imc, conversionMode, sentenceMode);
		::ImmReleaseContext(target().getHandle(), imc);
	} else
		assert(false);
	return 0;
}

/**
 * Inserts a newline, or exits a mode.
 *
 * If the incremental search is active, exits the search.
 *
 * If the auto completion is active, completes. Or aborts and breaks the line if no candidate matches exactly.
 * @retval 0 the command succeeded
 * @retval 1 the command failed
 */
ulong NewlineCommand::execute() {
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

	if(param_) {
		caret.enableAutoShow(false);
		if(caret.beginning().lineNumber() != 0)
			caret.moveTo(Position(caret.beginning().lineNumber() - 1, INVALID_INDEX));
		else
			caret.moveTo(Position::ZERO_POSITION);
		caret.enableAutoShow(true);
	}

	viewer.freeze();
	viewer.document().beginSequentialEdit();
	if(caret.isSelectionEmpty()) {
		viewer.document().endSequentialEdit();
		caret.newLine(false);
	} else {
		caret.eraseSelection();
		caret.newLine(false);
	}
	caret.moveTo(caret.anchor());
	viewer.unfreeze();
	return 0;
}

/**
 * Reconverts the selected content.
 * @retval 0 the command succeeded
 * @retval 1 the command failed because of the empty or rectangle selection
 * @see viewers#TextViewer#onIMERequest
 */
ulong ReconversionCommand::execute() {
	END_ISEARCH();
	CHECK_DOCUMENT_READONLY(1);
//	CHECK_GUI_EDITABILITY(1);

	TextViewer& viewer = target();
	Caret& caret = viewer.caret();
	if(caret.isSelectionRectangle()) {
		viewer.beep();
		return 1;
	}
	::HIMC imc = ::ImmGetContext(viewer.getHandle());
	if(!toBoolean(::ImmGetOpenStatus(imc)))	// without this, IME may ignore us?
		::ImmSetOpenStatus(imc, true);

	// from NotePadView.pas of TNotePad (http://wantech.ikuto.com/)
	const bool multilineSelection = caret.lineNumber() != caret.anchor().lineNumber();
	const String s = multilineSelection ? caret.selectionText() : viewer.document().line(caret.lineNumber());
	const ::DWORD bytes = static_cast<::DWORD>(sizeof(::RECONVERTSTRING) + sizeof(Char) * s.length());
	::RECONVERTSTRING* const rcs = static_cast<::RECONVERTSTRING*>(::operator new(bytes));
	rcs->dwSize = bytes;
	rcs->dwVersion = 0;
	rcs->dwStrLen = static_cast<::DWORD>(s.length());
	rcs->dwStrOffset = sizeof(::RECONVERTSTRING);
	rcs->dwCompStrLen = rcs->dwTargetStrLen = static_cast<::DWORD>(multilineSelection ? s.length() :
		(caret.end().columnNumber() - caret.beginning().columnNumber()));
	rcs->dwCompStrOffset = rcs->dwTargetStrOffset =
		multilineSelection ? 0 : static_cast<::DWORD>(sizeof(Char) * caret.beginning().columnNumber());
	s.copy(reinterpret_cast<Char*>(reinterpret_cast<char*>(rcs) + rcs->dwStrOffset), s.length());
	if(caret.isSelectionEmpty()) {
		// IME selects the composition target automatically if no selection
		if(0 == ::ImmSetCompositionStringW(imc, SCS_QUERYRECONVERTSTRING, rcs, rcs->dwSize, 0, 0)) {
			::operator delete(rcs);
			viewer.beep();
			return 1;
		}
		caret.select(
			Position(caret.lineNumber(), rcs->dwCompStrOffset / sizeof(Char)),
			Position(caret.lineNumber(), rcs->dwCompStrOffset / sizeof(Char) + rcs->dwCompStrLen));
	}
	::ImmSetCompositionStringW(imc, SCS_SETRECONVERTSTRING, rcs, rcs->dwSize, 0, 0);
	::operator delete(rcs);
	::ImmReleaseContext(viewer.getHandle(), imc);

	CLOSE_COMPLETION_PROPOSAL_POPUP();
	return 0;
}

/**
 * Replaces all matched texts. This does not freeze the text viewer.
 * @return the number of replced strings
 */
ulong ReplaceAllCommand::execute() {
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
 * Starts box selection, or extends the selection if the selection is exist.
 * @return 0
 */
ulong RowSelectionExtensionCommand::execute() {
	CLOSE_COMPLETION_PROPOSAL_POPUP();
	END_ISEARCH();

	Caret& caret = target().caret();
	static const int commandMap[] = {
		FORWARD_CHARACTER, CaretMovementCommand::FORWARD_CHARACTER,
		BACKWARD_CHARACTER, CaretMovementCommand::BACKWARD_CHARACTER,
		LEFT_CHARACTER, CaretMovementCommand::LEFT_CHARACTER,
		RIGHT_CHARACTER, CaretMovementCommand::RIGHT_CHARACTER,
		NEXT_WORD, CaretMovementCommand::NEXT_WORD,
		PREVIOUS_WORD, CaretMovementCommand::PREVIOUS_WORD,
		LEFT_WORD, CaretMovementCommand::LEFT_WORD,
		RIGHT_WORD, CaretMovementCommand::RIGHT_WORD,
		NEXT_WORDEND, CaretMovementCommand::NEXT_WORDEND,
		PREVIOUS_WORDEND, CaretMovementCommand::PREVIOUS_WORDEND,
		LEFT_WORDEND, CaretMovementCommand::LEFT_WORDEND,
		RIGHT_WORDEND, CaretMovementCommand::RIGHT_WORDEND,
		NEXT_LINE, CaretMovementCommand::NEXT_LINE,
		PREVIOUS_LINE, CaretMovementCommand::PREVIOUS_LINE,
		NEXT_VISUAL_LINE, CaretMovementCommand::NEXT_VISUAL_LINE,
		PREVIOUS_VISUAL_LINE, CaretMovementCommand::PREVIOUS_VISUAL_LINE,
		BEGINNING_OF_LINE, CaretMovementCommand::BEGINNING_OF_LINE,
		END_OF_LINE, CaretMovementCommand::END_OF_LINE,
		FIRST_PRINTABLE_CHARACTER_OF_LINE, CaretMovementCommand::FIRST_PRINTABLE_CHARACTER_OF_LINE,
		LAST_PRINTABLE_CHARACTER_OF_LINE, CaretMovementCommand::LAST_PRINTABLE_CHARACTER_OF_LINE,
		CONTEXTUAL_BEGINNING_OF_LINE, CaretMovementCommand::CONTEXTUAL_BEGINNING_OF_LINE,
		CONTEXTUAL_END_OF_LINE, CaretMovementCommand::CONTEXTUAL_END_OF_LINE,
		BEGINNING_OF_VISUAL_LINE, CaretMovementCommand::BEGINNING_OF_VISUAL_LINE,
		END_OF_VISUAL_LINE, CaretMovementCommand::END_OF_VISUAL_LINE,
		FIRST_PRINTABLE_CHARACTER_OF_VISUAL_LINE, CaretMovementCommand::FIRST_PRINTABLE_CHARACTER_OF_VISUAL_LINE,
		LAST_PRINTABLE_CHARACTER_OF_VISUAL_LINE, CaretMovementCommand::LAST_PRINTABLE_CHARACTER_OF_VISUAL_LINE,
		CONTEXTUAL_BEGINNING_OF_VISUAL_LINE, CaretMovementCommand::CONTEXTUAL_BEGINNING_OF_VISUAL_LINE,
		CONTEXTUAL_END_OF_VISUAL_LINE, CaretMovementCommand::CONTEXTUAL_END_OF_VISUAL_LINE
	};

	if(caret.isSelectionEmpty() && !caret.isSelectionRectangle())
		caret.beginRectangleSelection();
	assert(type_ >= 0 && type_ < MANAH_COUNTOF(commandMap));
	for(int i = 0; i < MANAH_COUNTOF(commandMap); i += 2) {
		if(commandMap[i] == type_) {
			CaretMovementCommand(target(), static_cast<CaretMovementCommand::Type>(commandMap[i + 1]), true).execute();
			break;
		}
	}
	return 0;
}

/**
 * Selects a whole document, or selects the word near the caret.
 * @return 0
 */
ulong SelectionCreationCommand::execute() {
	END_ISEARCH();

	target().caret().endRectangleSelection();
	if(type_ == ALL)
		target().caret().select(target().document().accessibleRegion());
	else if(type_ == CURRENT_WORD)
		target().caret().selectWord();
	else
		assert(false);
	return 0;
}

/**
 * Tabifies or untabifies.
 * @retval 0 succeeded
 * @retval 1 failed
 */
ulong TabifyCommand::execute() {
	CHECK_DOCUMENT_READONLY(1);
//	CHECK_GUI_EDITABILITY(1);
	ABORT_MODES();
	// TODO: not implemented.
	return 1;
}

/**
 * Inserts a text. If the incremental search is active, appends a string to the end of the pattern.
 * @retval 0 succeeded
 * @retval 1 failed
 */
ulong TextInputCommand::execute() {
	// インクリメンタル検索中 -> 検索式に追加
	if(Session* const session = target().document().session()) {
		if(session->incrementalSearcher().isRunning()) {
			session->incrementalSearcher().addString(param_);
			return 0;
		}
	}

	CHECK_DOCUMENT_READONLY(1);
//	CHECK_GUI_EDITABILITY(1);
	target().caret().replaceSelection(param_);
	return 0;
}

/**
 * Transposes two characters, words, lines, paragraphs, or sentences.
 * @retval 0 succeeded
 * @retval 1 failed
 * @see VisualPoint#transposeCharacters, VisualPoint#transposeWords,
 * VisualPoint#transposeLines, VisualPoint#transposeParagraphs, VisualPoint#transposeSentences
 */
ulong TranspositionCommand::execute() {
	CHECK_DOCUMENT_READONLY(1);
//	CHECK_GUI_EDITABILITY(1);
	END_ISEARCH();
	CLOSE_COMPLETION_PROPOSAL_POPUP();

	TextViewer& viewer = target();
	Caret& caret = viewer.caret();
	bool succeeded = false;
	viewer.freeze();
	viewer.document().beginSequentialEdit();
	switch(type_) {
	case CHARACTERS:	succeeded = caret.transposeCharacters();	break;
	case WORDS:			succeeded = caret.transposeWords();			break;
	case LINES:			succeeded = caret.transposeLines();			break;
//	case SENTENCES:		succeeded = caret.transposeSentences();		break;
//	case PARAGRAPHS:	succeeded = caret.transposeParagraphs();	break;
	}
	if(!succeeded)
		viewer.beep();
	viewer.document().endSequentialEdit();
	viewer.unfreeze();

	return succeeded ? 0 : 1;
}

/**
 * Undo or redo.
 * @retval 0 succeeded
 * @retval 1 failed
 * @see Document#undo, Document#redo
 */
ulong UndoCommand::execute() {
	CHECK_DOCUMENT_READONLY(1);
//	CHECK_GUI_EDITABILITY(1);

	if((param_ && target().document().numberOfUndoableEdits() == 0)
			|| (!param_ && target().document().numberOfRedoableEdits() == 0))
		return 1;

	WaitCursor wc;
	if(param_)	target().document().undo();
	else		target().document().redo();
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
