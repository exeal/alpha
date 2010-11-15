/**
 * @file user-input.cpp Implements the members of the namespace @c ascension#viewers related to the
 * user input (keyboard and mouse).
 * @author exeal
 * @date 2009 separated from viewer.cpp
 * @see viewer.cpp
 */

#include <ascension/viewer.hpp>
#include <ascension/text-editor.hpp>	// texteditor.commands.*
#include <ascension/win32/ui/menu.hpp>
#include <zmouse.h>
using namespace ascension;
using namespace ascension::graphics;
using namespace ascension::presentation;
using namespace ascension::viewers;
using namespace manah;
using namespace std;
namespace k = ascension::kernel;


// TextViewer ///////////////////////////////////////////////////////////////

#define ASCENSION_RESTORE_VANISHED_CURSOR()	\
	if(modeState_.cursorVanished) {			\
		modeState_.cursorVanished = false;	\
		::ShowCursor(true);					\
		releaseCapture();					\
	}

// local helpers
namespace {
	inline void abortIncrementalSearch(TextViewer& viewer) /*throw()*/ {
		if(texteditor::Session* session = viewer.document().session()) {
			if(session->incrementalSearcher().isRunning())
				session->incrementalSearcher().abort();
		}
	}
	inline void endIncrementalSearch(TextViewer& viewer) /*throw()*/ {
		if(texteditor::Session* session = viewer.document().session()) {
			if(session->incrementalSearcher().isRunning())
				session->incrementalSearcher().end();
		}
	}
	inline const hyperlink::IHyperlink* getPointedHyperlink(const TextViewer& viewer, const Position& at) {
		size_t numberOfHyperlinks;
		if(const hyperlink::IHyperlink* const* hyperlinks = viewer.presentation().getHyperlinks(at.line, numberOfHyperlinks)) {
			for(size_t i = 0; i < numberOfHyperlinks; ++i) {
				if(at.column >= hyperlinks[i]->region().beginning() && at.column <= hyperlinks[i]->region().end())
					return hyperlinks[i];
			}
		}
		return 0;
	}
	inline void toggleOrientation(TextViewer& viewer) /*throw()*/ {
		TextViewer::Configuration configuration(viewer.configuration());
		configuration.readingDirection =
			(utils::computeUIReadingDirection(viewer) == LEFT_TO_RIGHT) ? RIGHT_TO_LEFT : LEFT_TO_RIGHT;
		viewer.setConfiguration(&configuration, 0, true);
//		if(config.lineWrap.wrapsAtWindowEdge()) {
//			win32::AutoZeroSize<SCROLLINFO> scroll;
//			viewer.getScrollInformation(SB_HORZ, scroll);
//			viewer.setScrollInformation(SB_HORZ, scroll);
//		}
	}
} // namespace @0

/// Handles @c WM_CHAR and @c WM_UNICHAR window messages.
void TextViewer::handleGUICharacterInput(CodePoint c) {
	// vanish the cursor when the GUI user began typing
	if(texteditor::commands::CharacterInputCommand(*this, c)() != 0
			&& !modeState_.cursorVanished
			&& configuration_.vanishesCursor
			&& hasFocus()) {
		// ignore if the cursor is not over a window belongs to the same thread
		POINT pt;
		::GetCursorPos(&pt);
		HWND pointedWindow = ::WindowFromPoint(pt);
		if(pointedWindow != 0
				&& ::GetWindowThreadProcessId(pointedWindow, 0) == ::GetWindowThreadProcessId(handle().get(), 0)) {
			modeState_.cursorVanished = true;
			::ShowCursor(false);
			::SetCapture(handle().get());
		}
	}
}

/**
 * Translates key down message to a command.
 *
 * This method provides a default implementtation of "key combination to command" map.
 * Default @c #onKeyDown calls this method.
 *
 * This method is not overiddable (not virtual).
 * To customize key bindings, the derevied class must override @c #onKeyDown method instead.
 * @param key the virtual-keycode of the key
 * @param controlPressed true if CTRL key is pressed
 * @param shiftPressed true if SHIFT key is pressed
 * @param altPressed true if ALT key is pressed
 * @return true if the key down was handled
 */
bool TextViewer::handleKeyDown(UINT key, bool controlPressed, bool shiftPressed, bool altPressed) /*throw()*/ {
	using namespace ascension::texteditor::commands;
//	if(altPressed) {
//		if(!shiftPressed || (key != VK_LEFT && key != VK_UP && key != VK_RIGHT && key != VK_DOWN))
//			return false;
//	}
	switch(key) {
	case VK_BACK:	// [BackSpace]
	case VK_F16:	// [F16]
		if(controlPressed)
			WordDeletionCommand(*this, Direction::BACKWARD)();
		else
			CharacterDeletionCommand(*this, Direction::BACKWARD)();
		return true;
	case VK_CLEAR:	// [Clear]
		if(controlPressed) {
			EntireDocumentSelectionCreationCommand(*this)();
			return true;
		}
		break;
	case VK_RETURN:	// [Enter]
		NewlineCommand(*this, controlPressed)();
		return true;
	case VK_SHIFT:	// [Shift]
		if(controlPressed
				&& ((toBoolean(::GetAsyncKeyState(VK_LSHIFT) & 0x8000) && configuration_.readingDirection == RIGHT_TO_LEFT)
				|| (toBoolean(::GetAsyncKeyState(VK_RSHIFT) & 0x8000) && configuration_.readingDirection == LEFT_TO_RIGHT))) {
			toggleOrientation(*this);
			return true;
		}
		return false;
	case VK_ESCAPE:	// [Esc]
		CancelCommand(*this)();
		return true;
	case VK_PRIOR:	// [PageUp]
		if(controlPressed)	onVScroll(SB_PAGEUP, 0, 0);
		else				CaretMovementCommand(*this, &k::locations::backwardPage, shiftPressed)();
		return true;
	case VK_NEXT:	// [PageDown]
		if(controlPressed)	onVScroll(SB_PAGEDOWN, 0, 0);
		else				CaretMovementCommand(*this, &k::locations::forwardPage, shiftPressed)();
		return true;
	case VK_HOME:	// [Home]
		if(controlPressed)
			CaretMovementCommand(*this, &k::locations::beginningOfDocument, shiftPressed)();
		else
			CaretMovementCommand(*this, &k::locations::beginningOfVisualLine, shiftPressed)();
		return true;
	case VK_END:	// [End]
		if(controlPressed)
			CaretMovementCommand(*this, &k::locations::endOfDocument, shiftPressed)();
		else
			CaretMovementCommand(*this, &k::locations::endOfVisualLine, shiftPressed)();
		return true;
	case VK_LEFT:	// [Left]
		if(altPressed && shiftPressed) {
			if(controlPressed)
				RowSelectionExtensionCommand(*this, &k::locations::leftWord)();
			else
				RowSelectionExtensionCommand(*this, &k::locations::leftCharacter)();
		} else {
			if(controlPressed)
				CaretMovementCommand(*this, &k::locations::leftWord, shiftPressed)();
			else
				CaretMovementCommand(*this, &k::locations::leftCharacter, shiftPressed)();
		}
		return true;
	case VK_UP:		// [Up]
		if(altPressed && shiftPressed && !controlPressed)
			RowSelectionExtensionCommand(*this, &k::locations::backwardVisualLine)();
		else if(controlPressed && !shiftPressed)
			scroll(0, -1, true);
		else
			CaretMovementCommand(*this, &k::locations::backwardVisualLine, shiftPressed)();
		return true;
	case VK_RIGHT:	// [Right]
		if(altPressed) {
			if(shiftPressed) {
				if(controlPressed)
					RowSelectionExtensionCommand(*this, &k::locations::rightWord)();
				else
					RowSelectionExtensionCommand(*this, &k::locations::rightCharacter)();
			} else
				CompletionProposalPopupCommand(*this)();
		} else {
			if(controlPressed)
				CaretMovementCommand(*this, &k::locations::rightWord, shiftPressed)();
			else
				CaretMovementCommand(*this, &k::locations::rightCharacter, shiftPressed)();
		}
		return true;
	case VK_DOWN:	// [Down]
		if(altPressed && shiftPressed && !controlPressed)
			RowSelectionExtensionCommand(*this, &k::locations::forwardVisualLine)();
		else if(controlPressed && !shiftPressed)
			onVScroll(SB_LINEDOWN, 0, 0);
		else
			CaretMovementCommand(*this, &k::locations::forwardVisualLine, shiftPressed)();
		return true;
	case VK_INSERT:	// [Insert]
		if(altPressed)
			break;
		else if(!shiftPressed) {
			if(controlPressed)	copySelection(caret(), true);
			else				OvertypeModeToggleCommand(*this)();
		} else if(controlPressed)
			PasteCommand(*this, false)();
		else						break;
		return true;
	case VK_DELETE:	// [Delete]
		if(!shiftPressed) {
			if(controlPressed)
				WordDeletionCommand(*this, Direction::FORWARD)();
			else
				CharacterDeletionCommand(*this, Direction::FORWARD)();
		} else if(!controlPressed)
			cutSelection(caret(), true);
		else
			break;
		return true;
	case 'A':	// ^A -> Select All
		if(controlPressed)
			return EntireDocumentSelectionCreationCommand(*this)(), true;
		break;
	case 'C':	// ^C -> Copy
		if(controlPressed)
			return copySelection(caret(), true), true;
		break;
	case 'H':	// ^H -> Backspace
		if(controlPressed)
			CharacterDeletionCommand(*this, Direction::BACKWARD)(), true;
		break;
	case 'I':	// ^I -> Tab
		if(controlPressed)
			return CharacterInputCommand(*this, 0x0009u)(), true;
		break;
	case 'J':	// ^J -> New Line
	case 'M':	// ^M -> New Line
		if(controlPressed)
			return NewlineCommand(*this, false)(), true;
		break;
	case 'V':	// ^V -> Paste
		if(controlPressed)
			return PasteCommand(*this, false)(), true;
		break;
	case 'X':	// ^X -> Cut
		if(controlPressed)
			return cutSelection(caret(), true), true;
		break;
	case 'Y':	// ^Y -> Redo
		if(controlPressed)
			return UndoCommand(*this, true)(), true;
		break;
	case 'Z':	// ^Z -> Undo
		if(controlPressed)
			return UndoCommand(*this, false)(), true;
		break;
	case VK_NUMPAD5:	// [Number Pad 5]
		if(controlPressed)
			return EntireDocumentSelectionCreationCommand(*this)(), true;
		break;
	case VK_F12:	// [F12]
		if(controlPressed && shiftPressed)
			return CodePointToCharacterConversionCommand(*this)(), true;
		break;
	}
	return false;
}

/// @see WM_CAPTURECHANGED
void TextViewer::onCaptureChanged(HWND) {
	if(mouseInputStrategy_.get() != 0)
		mouseInputStrategy_->captureChanged();
}

/// @see WM_CHAR
void TextViewer::onChar(UINT ch, UINT) {
	handleGUICharacterInput(ch);
}

/// @see Window#onCommand
bool TextViewer::onCommand(WORD id, WORD, HWND) {
	using namespace ascension::texteditor::commands;
	switch(id) {
	case WM_UNDO:	// "Undo"
		UndoCommand(*this, false)();
		break;
	case WM_REDO:	// "Redo"
		UndoCommand(*this, true)();
		break;
	case WM_CUT:	// "Cut"
		cutSelection(caret(), true);
		break;
	case WM_COPY:	// "Copy"
		copySelection(caret(), true);
		break;
	case WM_PASTE:	// "Paste"
		PasteCommand(*this, false)();
		break;
	case WM_CLEAR:	// "Delete"
		CharacterDeletionCommand(*this, Direction::FORWARD)();
		break;
	case WM_SELECTALL:	// "Select All"
		EntireDocumentSelectionCreationCommand(*this)();
		break;
	case ID_RTLREADING:	// "Right to left Reading order"
		toggleOrientation(*this);
		break;
	case ID_DISPLAYSHAPINGCONTROLS: {	// "Show Unicode control characters"
		Configuration c(configuration());
		c.displaysShapingControls = !c.displaysShapingControls;
		setConfiguration(&c, 0, false);
		break;
	}
	case ID_INSERT_LRM:		CharacterInputCommand(*this, 0x200e)();	break;
	case ID_INSERT_RLM:		CharacterInputCommand(*this, 0x200f)();	break;
	case ID_INSERT_ZWJ:		CharacterInputCommand(*this, 0x200d)();	break;
	case ID_INSERT_ZWNJ:	CharacterInputCommand(*this, 0x200c)();	break;
	case ID_INSERT_LRE:		CharacterInputCommand(*this, 0x202a)();	break;
	case ID_INSERT_RLE:		CharacterInputCommand(*this, 0x202b)();	break;
	case ID_INSERT_LRO:		CharacterInputCommand(*this, 0x202d)();	break;
	case ID_INSERT_RLO:		CharacterInputCommand(*this, 0x202e)();	break;
	case ID_INSERT_PDF:		CharacterInputCommand(*this, 0x202c)();	break;
	case ID_INSERT_WJ:		CharacterInputCommand(*this, 0x2060)();	break;
	case ID_INSERT_NADS:	CharacterInputCommand(*this, 0x206e)();	break;
	case ID_INSERT_NODS:	CharacterInputCommand(*this, 0x206f)();	break;
	case ID_INSERT_ASS:		CharacterInputCommand(*this, 0x206b)();	break;
	case ID_INSERT_ISS:		CharacterInputCommand(*this, 0x206a)();	break;
	case ID_INSERT_AAFS:	CharacterInputCommand(*this, 0x206d)();	break;
	case ID_INSERT_IAFS:	CharacterInputCommand(*this, 0x206c)();	break;
	case ID_INSERT_RS:		CharacterInputCommand(*this, 0x001e)();	break;
	case ID_INSERT_US:		CharacterInputCommand(*this, 0x001f)();	break;
	case ID_INSERT_IAA:		CharacterInputCommand(*this, 0xfff9)();	break;
	case ID_INSERT_IAT:		CharacterInputCommand(*this, 0xfffa)();	break;
	case ID_INSERT_IAS:		CharacterInputCommand(*this, 0xfffb)();	break;
	case ID_INSERT_U0020:	CharacterInputCommand(*this, 0x0020)();	break;
	case ID_INSERT_NBSP:	CharacterInputCommand(*this, 0x00a0)();	break;
	case ID_INSERT_U1680:	CharacterInputCommand(*this, 0x1680)();	break;
	case ID_INSERT_MVS:		CharacterInputCommand(*this, 0x180e)();	break;
	case ID_INSERT_U2000:	CharacterInputCommand(*this, 0x2000)();	break;
	case ID_INSERT_U2001:	CharacterInputCommand(*this, 0x2001)();	break;
	case ID_INSERT_U2002:	CharacterInputCommand(*this, 0x2002)();	break;
	case ID_INSERT_U2003:	CharacterInputCommand(*this, 0x2003)();	break;
	case ID_INSERT_U2004:	CharacterInputCommand(*this, 0x2004)();	break;
	case ID_INSERT_U2005:	CharacterInputCommand(*this, 0x2005)();	break;
	case ID_INSERT_U2006:	CharacterInputCommand(*this, 0x2006)();	break;
	case ID_INSERT_U2007:	CharacterInputCommand(*this, 0x2007)();	break;
	case ID_INSERT_U2008:	CharacterInputCommand(*this, 0x2008)();	break;
	case ID_INSERT_U2009:	CharacterInputCommand(*this, 0x2009)();	break;
	case ID_INSERT_U200A:	CharacterInputCommand(*this, 0x200a)();	break;
	case ID_INSERT_ZWSP:	CharacterInputCommand(*this, 0x200b)();	break;
	case ID_INSERT_NNBSP:	CharacterInputCommand(*this, 0x202f)();	break;
	case ID_INSERT_MMSP:	CharacterInputCommand(*this, 0x205f)();	break;
	case ID_INSERT_U3000:	CharacterInputCommand(*this, 0x3000)();	break;
	case ID_INSERT_NEL:		CharacterInputCommand(*this, NEXT_LINE)();	break;
	case ID_INSERT_LS:		CharacterInputCommand(*this, LINE_SEPARATOR)();	break;
	case ID_INSERT_PS:		CharacterInputCommand(*this, PARAGRAPH_SEPARATOR)();	break;
	case ID_TOGGLEIMESTATUS:	// "Open IME" / "Close IME"
		InputMethodOpenStatusToggleCommand(*this)();
		break;
	case ID_TOGGLESOFTKEYBOARD:	// "Open soft keyboard" / "Close soft keyboard"
		InputMethodSoftKeyboardModeToggleCommand(*this)();
		break;
	case ID_RECONVERT:	// "Reconvert"
		ReconversionCommand(*this)();
		break;
	case ID_INVOKE_HYPERLINK:	// "Open <hyperlink>"
		if(const hyperlink::IHyperlink* const link = getPointedHyperlink(*this, caret()))
			link->invoke();
		break;
	default:
//		getParent()->sendMessage(WM_COMMAND, MAKEWPARAM(id, notifyCode), reinterpret_cast<LPARAM>(control));
		return true;
	}

	return false;
}

namespace {
	// replaces single "&" with "&&".
	template<typename CharType>
	basic_string<CharType> escapeAmpersands(const basic_string<CharType>& s) {
		static const ctype<CharType>& ct = use_facet<ctype<CharType> >(locale::classic());
		basic_string<CharType> result;
		result.reserve(s.length() * 2);
		for(basic_string<CharType>::size_type i = 0; i < s.length(); ++i) {
			result += s[i];
			if(s[i] == ct.widen('&'))
				result += s[i];
		}
		return result;
	}
} // namespace 0@

/// @see WM_CONTEXTMENU
bool TextViewer::onContextMenu(HWND, const POINT& pt) {
	using namespace win32::ui;

	if(!allowsMouseInput())	// however, may be invoked by other than the mouse...
		return true;
	utils::closeCompletionProposalsPopup(*this);
	abortIncrementalSearch(*this);

	Point<> menuPosition;

	// invoked by the keyboard
	if(pt.x == 0xffff && pt.y == 0xffff) {
		// MSDN says "the application should display the context menu at the location of the current selection."
		menuPosition = clientXYForCharacter(caret(), false);
		menuPosition.y += textRenderer().textMetrics().cellHeight() + 1;
		Rect<> clientBounds(bounds(false));
		const Rect<> margins(textAreaMargins());
		clientBounds.left() += margins.left();
		clientBounds.top() += margins.top();
		clientBounds.right() -= margins.right() - 1;
		clientBounds.bottom() -= margins.bottom();
		if(!clientBounds.includes(menuPosition))
			menuPosition.x = menuPosition.y = 1;
		clientToScreen(menuPosition);
	} else
		menuPosition = pt;

	// ignore if the point is over the scroll bars
	RECT rect;
	getClientRect(rect);
	clientToScreen(rect);
	if(!toBoolean(::PtInRect(&rect, menuPosition)))
		return false;

	const k::Document& doc = document();
	const bool hasSelection = !isSelectionEmpty(caret());
	const bool readOnly = doc.isReadOnly();
	const bool japanese = PRIMARYLANGID(userDefaultUILanguage()) == LANG_JAPANESE;

	static PopupMenu menu;
	static const WCHAR* captions[] = {
		L"&Undo",									L"\x5143\x306b\x623b\x3059(&U)",
		L"&Redo",									L"\x3084\x308a\x76f4\x3057(&R)",
		0,											0,
		L"Cu&t",									L"\x5207\x308a\x53d6\x308a(&T)",
		L"&Copy",									L"\x30b3\x30d4\x30fc(&C)",
		L"&Paste",									L"\x8cbc\x308a\x4ed8\x3051(&P)",
		L"&Delete",									L"\x524a\x9664(&D)",
		0,											0,
		L"Select &All",								L"\x3059\x3079\x3066\x9078\x629e(&A)",
		0,											0,
		L"&Right to left Reading order",			L"\x53f3\x304b\x3089\x5de6\x306b\x8aad\x3080(&R)",
		L"&Show Unicode control characters",		L"Unicode \x5236\x5fa1\x6587\x5b57\x306e\x8868\x793a(&S)",
		L"&Insert Unicode control character",		L"Unicode \x5236\x5fa1\x6587\x5b57\x306e\x633f\x5165(&I)",
		L"Insert Unicode &white space character",	L"Unicode \x7a7a\x767d\x6587\x5b57\x306e\x633f\x5165(&W)",
	};																	
#define GET_CAPTION(index)	captions[(index) * 2 + (japanese ? 1 : 0)]

	if(menu.getNumberOfItems() == 0) {	// first initialization
		menu << Menu::StringItem(WM_UNDO, GET_CAPTION(0))
			<< Menu::StringItem(WM_REDO, GET_CAPTION(1))
			<< Menu::SeparatorItem()
			<< Menu::StringItem(WM_CUT, GET_CAPTION(3))
			<< Menu::StringItem(WM_COPY, GET_CAPTION(4))
			<< Menu::StringItem(WM_PASTE, GET_CAPTION(5))
			<< Menu::StringItem(WM_CLEAR, GET_CAPTION(6))
			<< Menu::SeparatorItem()
			<< Menu::StringItem(WM_SELECTALL, GET_CAPTION(8))
			<< Menu::SeparatorItem()
			<< Menu::StringItem(ID_RTLREADING, GET_CAPTION(10))
			<< Menu::StringItem(ID_DISPLAYSHAPINGCONTROLS, GET_CAPTION(11))
			<< Menu::StringItem(0, GET_CAPTION(12))
			<< Menu::StringItem(0, GET_CAPTION(13));

		// under "Insert Unicode control character"
		PopupMenu subMenu;
		subMenu << Menu::StringItem(ID_INSERT_LRM, L"LRM\t&Left-To-Right Mark")
			<< Menu::StringItem(ID_INSERT_RLM, L"RLM\t&Right-To-Left Mark")
			<< Menu::StringItem(ID_INSERT_ZWJ, L"ZWJ\t&Zero Width Joiner")
			<< Menu::StringItem(ID_INSERT_ZWNJ, L"ZWNJ\tZero Width &Non-Joiner")
			<< Menu::StringItem(ID_INSERT_LRE, L"LRE\tLeft-To-Right &Embedding")
			<< Menu::StringItem(ID_INSERT_RLE, L"RLE\tRight-To-Left E&mbedding")
			<< Menu::StringItem(ID_INSERT_LRO, L"LRO\tLeft-To-Right &Override")
			<< Menu::StringItem(ID_INSERT_RLO, L"RLO\tRight-To-Left O&verride")
			<< Menu::StringItem(ID_INSERT_PDF, L"PDF\t&Pop Directional Formatting")
			<< Menu::StringItem(ID_INSERT_WJ, L"WJ\t&Word Joiner")
			<< Menu::StringItem(ID_INSERT_NADS, L"NADS\tN&ational Digit Shapes (deprecated)")
			<< Menu::StringItem(ID_INSERT_NODS, L"NODS\tNominal &Digit Shapes (deprecated)")
			<< Menu::StringItem(ID_INSERT_ASS, L"ASS\tActivate &Symmetric Swapping (deprecated)")
			<< Menu::StringItem(ID_INSERT_ISS, L"ISS\tInhibit S&ymmetric Swapping (deprecated)")
			<< Menu::StringItem(ID_INSERT_AAFS, L"AAFS\tActivate Arabic &Form Shaping (deprecated)")
			<< Menu::StringItem(ID_INSERT_IAFS, L"IAFS\tInhibit Arabic Form S&haping (deprecated)")
			<< Menu::StringItem(ID_INSERT_RS, L"RS\tRe&cord Separator")
			<< Menu::StringItem(ID_INSERT_US, L"US\tUnit &Separator")
			<< Menu::SeparatorItem()
			<< Menu::StringItem(ID_INSERT_IAA, L"IAA\tInterlinear Annotation Anchor")
			<< Menu::StringItem(ID_INSERT_IAT, L"IAT\tInterlinear Annotation Terminator")
			<< Menu::StringItem(ID_INSERT_IAS, L"IAS\tInterlinear Annotation Separator");
		menu.setChildPopup<Menu::BY_POSITION>(12, subMenu);

		// under "Insert Unicode white space character"
		subMenu.reset(win32::managed(::CreatePopupMenu()));
		subMenu << Menu::StringItem(ID_INSERT_U0020, L"U+0020\tSpace")
			<< Menu::StringItem(ID_INSERT_NBSP, L"NBSP\tNo-Break Space")
			<< Menu::StringItem(ID_INSERT_U1680, L"U+1680\tOgham Space Mark")
			<< Menu::StringItem(ID_INSERT_MVS, L"MVS\tMongolian Vowel Separator")
			<< Menu::StringItem(ID_INSERT_U2000, L"U+2000\tEn Quad")
			<< Menu::StringItem(ID_INSERT_U2001, L"U+2001\tEm Quad")
			<< Menu::StringItem(ID_INSERT_U2002, L"U+2002\tEn Space")
			<< Menu::StringItem(ID_INSERT_U2003, L"U+2003\tEm Space")
			<< Menu::StringItem(ID_INSERT_U2004, L"U+2004\tThree-Per-Em Space")
			<< Menu::StringItem(ID_INSERT_U2005, L"U+2005\tFour-Per-Em Space")
			<< Menu::StringItem(ID_INSERT_U2006, L"U+2006\tSix-Per-Em Space")
			<< Menu::StringItem(ID_INSERT_U2007, L"U+2007\tFigure Space")
			<< Menu::StringItem(ID_INSERT_U2008, L"U+2008\tPunctuation Space")
			<< Menu::StringItem(ID_INSERT_U2009, L"U+2009\tThin Space")
			<< Menu::StringItem(ID_INSERT_U200A, L"U+200A\tHair Space")
			<< Menu::StringItem(ID_INSERT_ZWSP, L"ZWSP\tZero Width Space")
			<< Menu::StringItem(ID_INSERT_NNBSP, L"NNBSP\tNarrow No-Break Space")
			<< Menu::StringItem(ID_INSERT_MMSP, L"MMSP\tMedium Mathematical Space")
			<< Menu::StringItem(ID_INSERT_U3000, L"U+3000\tIdeographic Space")
			<< Menu::SeparatorItem()
			<< Menu::StringItem(ID_INSERT_NEL, L"NEL\tNext Line")
			<< Menu::StringItem(ID_INSERT_LS, L"LS\tLine Separator")
			<< Menu::StringItem(ID_INSERT_PS, L"PS\tParagraph Separator");
		menu.setChildPopup<Menu::BY_POSITION>(13, subMenu);

		// check if the system supports bidi
		if(!supportsComplexScripts()) {
			menu.enable<Menu::BY_COMMAND>(ID_RTLREADING, false);
			menu.enable<Menu::BY_COMMAND>(ID_DISPLAYSHAPINGCONTROLS, false);
			menu.enable<Menu::BY_POSITION>(12, false);
			menu.enable<Menu::BY_POSITION>(13, false);
		}
	}
#undef GET_CAPTION

	// modify menu items
	menu.enable<Menu::BY_COMMAND>(WM_UNDO, !readOnly && doc.numberOfUndoableChanges() != 0);
	menu.enable<Menu::BY_COMMAND>(WM_REDO, !readOnly && doc.numberOfRedoableChanges() != 0);
	menu.enable<Menu::BY_COMMAND>(WM_CUT, !readOnly && hasSelection);
	menu.enable<Menu::BY_COMMAND>(WM_COPY, hasSelection);
	menu.enable<Menu::BY_COMMAND>(WM_PASTE, !readOnly && caret_->canPaste(false));
	menu.enable<Menu::BY_COMMAND>(WM_CLEAR, !readOnly && hasSelection);
	menu.enable<Menu::BY_COMMAND>(WM_SELECTALL, doc.numberOfLines() > 1 || doc.lineLength(0) > 0);
	menu.check<Menu::BY_COMMAND>(ID_RTLREADING, configuration_.readingDirection == RIGHT_TO_LEFT);
	menu.check<Menu::BY_COMMAND>(ID_DISPLAYSHAPINGCONTROLS, configuration_.displaysShapingControls);

	// IME commands
	HKL keyboardLayout = ::GetKeyboardLayout(::GetCurrentThreadId());
	if(//toBoolean(::ImmIsIME(keyboardLayout)) &&
			::ImmGetProperty(keyboardLayout, IGP_SENTENCE) != IME_SMODE_NONE) {
		HIMC imc = ::ImmGetContext(handle().get());
		WCHAR* openIme = japanese ? L"IME \x3092\x958b\x304f(&O)" : L"&Open IME";
		WCHAR* closeIme = japanese ? L"IME \x3092\x9589\x3058\x308b(&L)" : L"C&lose IME";
		WCHAR* openSftKbd = japanese ? L"\x30bd\x30d5\x30c8\x30ad\x30fc\x30dc\x30fc\x30c9\x3092\x958b\x304f(&E)" : L"Op&en soft keyboard";
		WCHAR* closeSftKbd = japanese ? L"\x30bd\x30d5\x30c8\x30ad\x30fc\x30dc\x30fc\x30c9\x3092\x9589\x3058\x308b(&F)" : L"Close so&ft keyboard";
		WCHAR* reconvert = japanese ? L"\x518d\x5909\x63db(&R)" : L"&Reconvert";

		menu << Menu::SeparatorItem()
			<< Menu::StringItem(ID_TOGGLEIMESTATUS, toBoolean(::ImmGetOpenStatus(imc)) ? closeIme : openIme);

		if(toBoolean(::ImmGetProperty(keyboardLayout, IGP_CONVERSION) & IME_CMODE_SOFTKBD)) {
			DWORD convMode;
			::ImmGetConversionStatus(imc, &convMode, 0);
			menu << Menu::StringItem(ID_TOGGLESOFTKEYBOARD, toBoolean(convMode & IME_CMODE_SOFTKBD) ? closeSftKbd : openSftKbd);
		}

		if(toBoolean(::ImmGetProperty(keyboardLayout, IGP_SETCOMPSTR) & SCS_CAP_SETRECONVERTSTRING))
			menu << Menu::StringItem(ID_RECONVERT, reconvert, (!readOnly && hasSelection) ? MFS_ENABLED : MFS_GRAYED);

		::ImmReleaseContext(handle().get(), imc);
	}

	// hyperlink
	if(const hyperlink::IHyperlink* link = getPointedHyperlink(*this, caret())) {
		const length_t len = (link->region().end() - link->region().beginning()) * 2 + 8;
		AutoBuffer<WCHAR> caption(new WCHAR[len]);	// TODO: this code can have buffer overflow in future
		swprintf(caption.get(),
#if(_MSC_VER < 1400)
#else
			len,
#endif // _MSC_VER < 1400
			japanese ? L"\x202a%s\x202c \x3092\x958b\x304f" : L"Open \x202a%s\x202c", escapeAmpersands(doc.line(
				caret().line()).substr(link->region().beginning(), link->region().end() - link->region().beginning())).c_str());
		menu << Menu::SeparatorItem() << Menu::StringItem(ID_INVOKE_HYPERLINK, caption.get());
	}

	menu.trackPopup(TPM_LEFTALIGN, menuPosition.x, menuPosition.y, handle().get());

	// ...finally erase all items
	int c = menu.getNumberOfItems();
	while(c > 13)
		menu.erase<Menu::BY_POSITION>(c--);

	return true;
}

/// @see WM_IME_COMPOSITION
void TextViewer::onIMEComposition(WPARAM wParam, LPARAM lParam, bool& handled) {
	if(document().isReadOnly())
		return;
	else if(/*lParam == 0 ||*/ toBoolean(lParam & GCS_RESULTSTR)) {	// completed
		if(HIMC imc = ::ImmGetContext(handle().get())) {
			if(const length_t len = ::ImmGetCompositionStringW(imc, GCS_RESULTSTR, 0, 0) / sizeof(WCHAR)) {
				// this was not canceled
				const AutoBuffer<Char> text(new Char[len + 1]);
				::ImmGetCompositionStringW(imc, GCS_RESULTSTR, text.get(), static_cast<DWORD>(len * sizeof(WCHAR)));
				text[len] = 0;
				if(!imeComposingCharacter_)
					texteditor::commands::TextInputCommand(*this, text.get())();
				else {
					k::Document& doc = document();
					try {
						doc.insertUndoBoundary();
						doc.replace(k::Region(*caret_,
							static_cast<k::DocumentCharacterIterator&>(k:DocumentCharacterIterator(doc, caret()).next()).tell()),
							String(1, static_cast<Char>(wParam)));:
						doc.insertUndoBoundary();
					} catch(const DocumentCantChangeException&) {
					}
					imeComposingCharacter_ = false;
					recreateCaret();
				}
			}
//			updateIMECompositionWindowPosition();
			::ImmReleaseContext(handle().get(), imc);
			handled = true;	// prevent to be send WM_CHARs
		}
	} else if(toBoolean(GCS_COMPSTR & lParam)) {
		if(toBoolean(lParam & CS_INSERTCHAR)) {
			k::Document& doc = document();
			const k::Position temp(*caret_);
			try {
				if(imeComposingCharacter_)
					doc.replace(k::Region(*caret_,
						static_cast<k::DocumentCharacterIterator&>(k::DocumentCharacterIterator(doc, caret()).next()).tell()),
						String(1, static_cast<Char>(wParam)));
				else
					insert(doc, *caret_, String(1, static_cast<Char>(wParam)));
				imeComposingCharacter_ = true;
				if(toBoolean(lParam & CS_NOMOVECARET))
					caret_->moveTo(temp);
			} catch(...) {
			}
			handled = true;
			recreateCaret();
		}
	}
}

/// @see WM_IME_ENDCOMPOSITION
void TextViewer::onIMEEndComposition() {
	imeCompositionActivated_ = false;
	recreateCaret();
}

/// @see WM_IME_NOTIFY
LRESULT TextViewer::onIMENotify(WPARAM command, LPARAM, bool&) {
	if(command == IMN_SETOPENSTATUS)
		inputStatusListeners_.notify(&ITextViewerInputStatusListener::textViewerIMEOpenStatusChanged);
	return 0L;
}

/// @see WM_IME_REQUEST
LRESULT TextViewer::onIMERequest(WPARAM command, LPARAM lParam, bool& handled) {
	const k::Document& doc = document();

	// this command will be sent two times when reconversion is invoked
	if(command == IMR_RECONVERTSTRING) {
		if(doc.isReadOnly() || caret().isSelectionRectangle()) {
			beep();
			return 0L;
		}
		handled = true;
		if(isSelectionEmpty(*caret_)) {	// IME selects the composition target automatically if no selection
			if(RECONVERTSTRING* const rcs = reinterpret_cast<RECONVERTSTRING*>(lParam)) {
				const String& line = doc.line(caret().line());
				rcs->dwStrLen = static_cast<DWORD>(line.length());
				rcs->dwStrOffset = sizeof(RECONVERTSTRING);
				rcs->dwTargetStrOffset = rcs->dwCompStrOffset = static_cast<DWORD>(sizeof(Char) * caret().column());
				rcs->dwTargetStrLen = rcs->dwCompStrLen = 0;
				line.copy(reinterpret_cast<Char*>(reinterpret_cast<char*>(rcs) + rcs->dwStrOffset), rcs->dwStrLen);
			}
			return sizeof(RECONVERTSTRING) + sizeof(Char) * doc.lineLength(caret().line());
		} else {
			const String selection(selectedString(caret(), k::NLF_RAW_VALUE));
			if(RECONVERTSTRING* const rcs = reinterpret_cast<RECONVERTSTRING*>(lParam)) {
				rcs->dwStrLen = rcs->dwTargetStrLen = rcs->dwCompStrLen = static_cast<DWORD>(selection.length());
				rcs->dwStrOffset = sizeof(RECONVERTSTRING);
				rcs->dwTargetStrOffset = rcs->dwCompStrOffset = 0;
				selection.copy(reinterpret_cast<Char*>(reinterpret_cast<char*>(rcs) + rcs->dwStrOffset), rcs->dwStrLen);
			}
			return sizeof(RECONVERTSTRING) + sizeof(Char) * selection.length();
		}
	}

	// before reconversion. a RECONVERTSTRING contains the ranges of the composition
	else if(command == IMR_CONFIRMRECONVERTSTRING) {
		if(RECONVERTSTRING* const rcs = reinterpret_cast<RECONVERTSTRING*>(lParam)) {
			const k::Region region(doc.accessibleRegion());
			if(!isSelectionEmpty(caret())) {
				// reconvert the selected region. the selection may be multi-line
				if(rcs->dwCompStrLen < rcs->dwStrLen)	// the composition region was truncated.
					rcs->dwCompStrLen = rcs->dwStrLen;	// IME will alert and reconversion will not be happen if do this
														// (however, NotePad narrows the selection...)
			} else {
				// reconvert the region IME passed if no selection (and create the new selection).
				// in this case, reconversion across multi-line (prcs->dwStrXxx represents the entire line)
				if(doc.isNarrowed() && caret().line() == region.first.line) {	// the document is narrowed
					if(rcs->dwCompStrOffset / sizeof(Char) < region.first.column) {
						rcs->dwCompStrLen += static_cast<DWORD>(sizeof(Char) * region.first.column - rcs->dwCompStrOffset);
						rcs->dwTargetStrLen = rcs->dwCompStrOffset;
						rcs->dwCompStrOffset = rcs->dwTargetStrOffset = static_cast<DWORD>(sizeof(Char) * region.first.column);
					} else if(rcs->dwCompStrOffset / sizeof(Char) > region.second.column) {
						rcs->dwCompStrOffset -= rcs->dwCompStrOffset - sizeof(Char) * region.second.column;
						rcs->dwTargetStrOffset = rcs->dwCompStrOffset;
						rcs->dwCompStrLen = rcs->dwTargetStrLen
							= static_cast<DWORD>(sizeof(Char) * region.second.column - rcs->dwCompStrOffset);
					}
				}
				caret().select(
					k::Position(caret().line(), rcs->dwCompStrOffset / sizeof(Char)),
					k::Position(caret().line(), rcs->dwCompStrOffset / sizeof(Char) + rcs->dwCompStrLen));
			}
			handled = true;
			return true;
		}
	}

	// queried position of the composition window
	else if(command == IMR_QUERYCHARPOSITION)
		return false;	// handled by updateIMECompositionWindowPosition...

	// queried document content for higher conversion accuracy
	else if(command == IMR_DOCUMENTFEED) {
		if(caret().line() == caret().anchor().line()) {
			handled = true;
			if(RECONVERTSTRING* const rcs = reinterpret_cast<RECONVERTSTRING*>(lParam)) {
				rcs->dwStrLen = static_cast<DWORD>(doc.lineLength(caret().line()));
				rcs->dwStrOffset = sizeof(RECONVERTSTRING);
				rcs->dwCompStrLen = rcs->dwTargetStrLen = 0;
				rcs->dwCompStrOffset = rcs->dwTargetStrOffset = sizeof(Char) * static_cast<DWORD>(caret().beginning().column());
				doc.line(caret().line()).copy(reinterpret_cast<Char*>(reinterpret_cast<char*>(rcs) + rcs->dwStrOffset), rcs->dwStrLen);
			}
			return sizeof(RECONVERTSTRING) + sizeof(Char) * doc.lineLength(caret().line());
		}
	}

	return 0L;
}

/// @see WM_IME_STARTCOMPOSITION
void TextViewer::onIMEStartComposition() {
	imeCompositionActivated_ = true;
	updateIMECompositionWindowPosition();
	utils::closeCompletionProposalsPopup(*this);
}

/// @see WM_KEYDOWN
void TextViewer::onKeyDown(UINT vkey, UINT, bool& handled) {
	if(mouseInputStrategy_.get() != 0)
		mouseInputStrategy_->interruptMouseReaction(true);
	handled = handleKeyDown(vkey, toBoolean(::GetKeyState(VK_CONTROL) & 0x8000), toBoolean(::GetKeyState(VK_SHIFT) & 0x8000), false);
}

/// @see WM_KILLFOCUS
void TextViewer::onKillFocus(HWND newWindow) {
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
		HIMC imc = ::ImmGetContext(get());
		::ImmNotifyIME(imc, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
		::ImmReleaseContext(get(), imc);
	}
	if(newWindow != get()) {
		hideCaret();
		::DestroyCaret();
	}
	redrawLines(caret().beginning().line(), caret().end().line());
	update();
}

/// @see WM_LBUTTONDBLCLK
void TextViewer::onLButtonDblClk(UINT keyState, const POINT& pt, bool& handled) {
	handled = (allowsMouseInput() && mouseInputStrategy_.get() != 0) ?
		handled = mouseInputStrategy_->mouseButtonInput(
			IMouseInputStrategy::LEFT_BUTTON, IMouseInputStrategy::DOUBLE_CLICKED, pt, keyState) : false;
}

/// @see WM_LBUTTONDOWN
void TextViewer::onLButtonDown(UINT keyState, const POINT& pt, bool& handled) {
	ASCENSION_RESTORE_VANISHED_CURSOR();
	handled = (allowsMouseInput() && mouseInputStrategy_.get() != 0) ?
		handled = mouseInputStrategy_->mouseButtonInput(
			IMouseInputStrategy::LEFT_BUTTON, IMouseInputStrategy::PRESSED, pt, keyState) : false;
}

/// @see WM_LBUTTONUP
void TextViewer::onLButtonUp(UINT keyState, const POINT& pt, bool& handled) {
	ASCENSION_RESTORE_VANISHED_CURSOR();
	handled = (allowsMouseInput() && mouseInputStrategy_.get() != 0) ?
		handled = mouseInputStrategy_->mouseButtonInput(
			IMouseInputStrategy::LEFT_BUTTON, IMouseInputStrategy::RELEASED, pt, keyState) : false;
}

/// @see WM_MBUTTONDBLCLK
void TextViewer::onMButtonDblClk(UINT keyState, const POINT& pt, bool& handled) {
	handled = (allowsMouseInput() && mouseInputStrategy_.get() != 0) ?
		mouseInputStrategy_->mouseButtonInput(
			IMouseInputStrategy::MIDDLE_BUTTON, IMouseInputStrategy::DOUBLE_CLICKED, pt, keyState) : false;
}

/// @see WM_MBUTTONDOWN
void TextViewer::onMButtonDown(UINT keyState, const POINT& pt, bool& handled) {
	ASCENSION_RESTORE_VANISHED_CURSOR();
	handled = (allowsMouseInput() && mouseInputStrategy_.get() != 0) ?
		mouseInputStrategy_->mouseButtonInput(
			IMouseInputStrategy::MIDDLE_BUTTON, IMouseInputStrategy::PRESSED, pt, keyState) : false;
}

/// @see WM_MBUTTONUP
void TextViewer::onMButtonUp(UINT keyState, const POINT& pt, bool& handled) {
	ASCENSION_RESTORE_VANISHED_CURSOR();
	handled = (allowsMouseInput() && mouseInputStrategy_.get() != 0) ? 
		mouseInputStrategy_->mouseButtonInput(
			IMouseInputStrategy::MIDDLE_BUTTON, IMouseInputStrategy::RELEASED, pt, keyState) : false;
}

/// @see WM_MOUSEMOVE
void TextViewer::onMouseMove(UINT keyState, const POINT& position) {
	ASCENSION_RESTORE_VANISHED_CURSOR();
	if(allowsMouseInput() && mouseInputStrategy_.get() != 0)
		mouseInputStrategy_->mouseMoved(position, keyState);
}

/// @see WM_MOUSEWHEEL
void TextViewer::onMouseWheel(UINT keyState, short delta, const POINT& pt) {
	ASCENSION_RESTORE_VANISHED_CURSOR();
	if(allowsMouseInput() && mouseInputStrategy_.get() != 0)
		mouseInputStrategy_->mouseWheelRotated(delta, pt, keyState);
}

/// @see WM_RBUTTONDBLCLK
void TextViewer::onRButtonDblClk(UINT keyState, const POINT& pt, bool& handled) {
	handled = (allowsMouseInput() && mouseInputStrategy_.get() != 0) ?
		mouseInputStrategy_->mouseButtonInput(
			IMouseInputStrategy::RIGHT_BUTTON, IMouseInputStrategy::DOUBLE_CLICKED, pt, keyState) : false;
}

/// @see WM_RBUTTONDOWN
void TextViewer::onRButtonDown(UINT keyState, const POINT& pt, bool& handled) {
	ASCENSION_RESTORE_VANISHED_CURSOR();
	handled = (allowsMouseInput() && mouseInputStrategy_.get() != 0) ?
		mouseInputStrategy_->mouseButtonInput(
			IMouseInputStrategy::RIGHT_BUTTON, IMouseInputStrategy::PRESSED, pt, keyState) : false;
}

/// @see WM_RBUTTONUP
void TextViewer::onRButtonUp(UINT keyState, const POINT& pt, bool& handled) {
	if(allowsMouseInput()) {
		ASCENSION_RESTORE_VANISHED_CURSOR();
		handled = (mouseInputStrategy_.get() != 0) ? mouseInputStrategy_->mouseButtonInput(
			IMouseInputStrategy::RIGHT_BUTTON, IMouseInputStrategy::RELEASED, pt, keyState) : false;
	} else
		handled = false;
}

/// @see WM_SETCURSOR
bool TextViewer::onSetCursor(HWND, UINT, UINT) {
	ASCENSION_RESTORE_VANISHED_CURSOR();
	return (mouseInputStrategy_.get() != 0) ? mouseInputStrategy_->showCursor(getCursorPosition()) : false;
}

/// @see WM_SETFOCUS
void TextViewer::onSetFocus(HWND oldWindow) {
	// restore the scroll positions
	setScrollPosition(SB_HORZ, scrollInfo_.horizontal.position, false);
	setScrollPosition(SB_VERT, scrollInfo_.vertical.position, true);

	// hmm...
//	if(/*sharedData_->options.appearance[SHOW_CURRENT_UNDERLINE] ||*/ !getCaret().isSelectionEmpty()) {
		redrawLines(caret().beginning().line(), caret().end().line());
		update();
//	}

	if(oldWindow != get()) {
		// resurrect the caret
		recreateCaret();
		updateCaretPosition();
		if(texteditor::Session* const session = document().session()) {
			if(texteditor::InputSequenceCheckers* const isc = session->inputSequenceCheckers())
				isc->setKeyboardLayout(::GetKeyboardLayout(::GetCurrentThreadId()));
		}
	}
}

/// @see WM_SYSCHAR
void TextViewer::onSysChar(UINT, UINT) {
	ASCENSION_RESTORE_VANISHED_CURSOR();
}

/// @see WM_SYSKEYDOWN
bool TextViewer::onSysKeyDown(UINT vkey, UINT) {
	if(mouseInputStrategy_.get() != 0)
		mouseInputStrategy_->interruptMouseReaction(true);
	return handleKeyDown(vkey, toBoolean(::GetKeyState(VK_CONTROL) & 0x8000), toBoolean(::GetKeyState(VK_SHIFT) & 0x8000), true);;
}

/// @see WM_SYSKEYUP
bool TextViewer::onSysKeyUp(UINT, UINT) {
	ASCENSION_RESTORE_VANISHED_CURSOR();
	if(mouseInputStrategy_.get() != 0)
		mouseInputStrategy_->interruptMouseReaction(true);
	return false;
}

#ifdef WM_UNICHAR
/// @see WM_UNICHAR
void TextViewer::onUniChar(UINT ch, UINT) {
	if(ch != UNICODE_NOCHAR)
		handleGUICharacterInput(ch);
}
#endif // WM_UNICHAR

/// @see WM_XBUTTONDBLCLK
bool TextViewer::onXButtonDblClk(WORD xButton, WORD keyState, const POINT& pt) {
	if(allowsMouseInput() && mouseInputStrategy_.get() != 0)
		return mouseInputStrategy_->mouseButtonInput((xButton == XBUTTON1) ?
			IMouseInputStrategy::X1_BUTTON : IMouseInputStrategy::X2_BUTTON, IMouseInputStrategy::DOUBLE_CLICKED, pt, keyState);
	return false;
}

/// @see WM_XBUTTONDOWN
bool TextViewer::onXButtonDown(WORD xButton, WORD keyState, const POINT& pt) {
	ASCENSION_RESTORE_VANISHED_CURSOR();
	if(allowsMouseInput() && mouseInputStrategy_.get() != 0)
		return mouseInputStrategy_->mouseButtonInput((xButton == XBUTTON1) ?
			IMouseInputStrategy::X1_BUTTON : IMouseInputStrategy::X2_BUTTON, IMouseInputStrategy::PRESSED, pt, keyState);
	return false;
}

/// @see WM_XBUTTONUP
bool TextViewer::onXButtonUp(WORD xButton, WORD keyState, const POINT& pt) {
	ASCENSION_RESTORE_VANISHED_CURSOR();
	if(allowsMouseInput() && mouseInputStrategy_.get() != 0)
		return mouseInputStrategy_->mouseButtonInput((xButton == XBUTTON1) ?
			IMouseInputStrategy::X1_BUTTON : IMouseInputStrategy::X2_BUTTON, IMouseInputStrategy::RELEASED, pt, keyState);
	return false;
}

/// Moves the IME form to valid position.
void TextViewer::updateIMECompositionWindowPosition() {
	check();
	if(!imeCompositionActivated_)
		return;
	else if(HIMC imc = ::ImmGetContext(get())) {
		// composition window placement
		COMPOSITIONFORM cf;
		getClientRect(cf.rcArea);
		const Rect<> margins(textAreaMargins());
		cf.rcArea.left += margins.left();
		cf.rcArea.top += margins.top();
		cf.rcArea.right -= margins.right();
		cf.rcArea.bottom -= margins.bottom();
		cf.dwStyle = CFS_POINT;
		cf.ptCurrentPos = clientXYForCharacter(caret().beginning(), false, LineLayout::LEADING);
		if(cf.ptCurrentPos.y == 32767 || cf.ptCurrentPos.y == -32768)
			cf.ptCurrentPos.y = (cf.ptCurrentPos.y == -32768) ? cf.rcArea.top : cf.rcArea.bottom;
		else
			cf.ptCurrentPos.y = max(cf.ptCurrentPos.y, cf.rcArea.top);
		::ImmSetCompositionWindow(imc, &cf);
		cf.dwStyle = CFS_RECT;
		::ImmSetCompositionWindow(imc, &cf);

		// composition font
		LOGFONTW font;
		::GetObjectW(renderer_->primaryFont()->handle().get(), sizeof(LOGFONTW), &font);
		::ImmSetCompositionFontW(imc, &font);	// this may be ineffective for IME settings
		
		::ImmReleaseContext(get(), imc);
	}
}


// DefaultMouseInputStrategy ////////////////////////////////////////////////

namespace {
	/// Circled window displayed at which the auto scroll started.
	class AutoScrollOriginMark : public win32::Window {
		ASCENSION_NONCOPYABLE_TAG(AutoScrollOriginMark);
	public:
		/// Defines the type of the cursors obtained by @c #cursorForScrolling method.
		enum CursorType {
			CURSOR_NEUTRAL,	///< Indicates no scrolling.
			CURSOR_UPWARD,	///< Indicates scrolling upward.
			CURSOR_DOWNWARD	///< Indicates scrolling downward.
		};
	public:
		AutoScrollOriginMark() /*throw()*/;
		bool create(const TextViewer& view);
		static const win32::Handle<HCURSOR> cursorForScrolling(CursorType type);
	private:
		void paint(graphics::PaintContext& context);
		void provideClassInformation(win32::ClassInformation& classInfomation) const {
			classInformation.style = CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW;
			background = COLOR_WINDOW;
			cursor = MAKEINTRESOURCEW(32513);	// IDC_IBEAM
		}
		basic_string<WCHAR> provideClassName() const {return L"AutoScrollOriginMark";}
	private:
		static const long WINDOW_WIDTH = 28;
	};
} // namespace @0

/// Default constructor.
AutoScrollOriginMark::AutoScrollOriginMark() /*throw()*/ {
}

/**
 * Creates the window.
 * @param view the viewer
 * @return succeeded or not
 * @see Window#create
 */
bool AutoScrollOriginMark::create(const TextViewer& view) {
	RECT rc = {0, 0, WINDOW_WIDTH + 1, WINDOW_WIDTH + 1};

	if(!win32::ui::CustomControl<AutoScrollOriginMark>::create(view.use(),
			rc, 0, WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP, WS_EX_TOOLWINDOW))
		return false;
	modifyStyleEx(0, WS_EX_LAYERED);	// いきなり CreateWindowEx(WS_EX_LAYERED) とすると NT 4.0 で失敗する

	HRGN rgn = ::CreateEllipticRgn(0, 0, WINDOW_WIDTH + 1, WINDOW_WIDTH + 1);
	setRegion(rgn, false);
	::DeleteObject(rgn);
	setLayeredAttributes(::GetSysColor(COLOR_WINDOW), 0, LWA_COLORKEY);

	return true;
}

/**
 * Returns the cursor should be shown when the auto-scroll is active.
 * @param type the type of the cursor to obtain
 * @return the cursor. do not destroy the returned value
 * @throw UnknownValueException @a type is unknown
 */
const win32::Handle<HCURSOR>& AutoScrollOriginMark::cursorForScrolling(CursorType type) {
	static win32::Handle<HCURSOR> instances[3];
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
		instances[type].reset(::CreateCursor(::GetModuleHandleW(0), 16, 16, 32, 32, andBits, xorBits), &::DestroyCursor);
	}
	return instances[type];
}

/// @see Window#onPaint
void AutoScrollOriginMark::paint(PaintContext& context) {
	const COLORREF color = ::GetSysColor(COLOR_APPWORKSPACE);
	HPEN pen = ::CreatePen(PS_SOLID, 1, color), oldPen = dc.selectObject(pen);
	HBRUSH brush = ::CreateSolidBrush(color), oldBrush = dc.selectObject(brush);
	Point<> points[4];

	points[0].x = 13; points[0].y = 3;
	points[1].x = 7; points[1].y = 9;
	points[2].x = 20; points[2].y = 9;
	points[3].x = 14; points[3].y = 3;
	dc.polygon(points, 4);

	points[0].x = 13; points[0].y = 24;
	points[1].x = 7; points[1].y = 18;
	points[2].x = 20; points[2].y = 18;
	points[3].x = 14; points[3].y = 24;
	dc.polygon(points, 4);

	dc.moveTo(13, 12); dc.lineTo(15, 12);
	dc.moveTo(12, 13); dc.lineTo(16, 13);
	dc.moveTo(12, 14); dc.lineTo(16, 14);
	dc.moveTo(13, 15); dc.lineTo(15, 15);

	dc.selectObject(oldPen);
	dc.selectObject(oldBrush);
	::DeleteObject(pen);
	::DeleteObject(brush);
}

/**
 * Standard implementation of @c IMouseOperationStrategy interface.
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

/// @see IMouseInputStrategy#captureChanged
void DefaultMouseInputStrategy::captureChanged() {
	endTimer();
	state_ = NONE;
}

namespace {
	HRESULT createSelectionImage(const TextViewer& viewer, const Point<>& cursorPosition, bool highlightSelection, SHDRAGIMAGE& image) {
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
		const Region selectedRegion(viewer.caret());
		length_t firstLine, firstSubline;
		viewer.firstVisibleLine(&firstLine, 0, &firstSubline);

		// calculate the size of the image
		const Rect<> clientBounds(viewer.bounds(false));
		const TextRenderer& renderer = viewer.textRenderer();
		Rect<> selectionBounds(
			Point<>(numeric_limits<Scalar>::max(), 0),
			Dimension<>(numeric_limits<Scalar>::min(), 0));
		for(length_t line = selectedRegion.beginning().line, e = selectedRegion.end().line; line <= e; ++line) {
			selectionBounds.bottom += static_cast<LONG>(renderer.textMetrics().linePitch() * renderer.lineLayout(line).numberOfSublines());
			if(selectionBounds.height() > clientBounds.height())
				return S_FALSE;	// overflow
			const LineLayout& layout = renderer.lineLayout(line);
			const int indent = renderer.lineIndent(line);
			Range<length_t> range;
			for(length_t subline = 0, sublines = layout.numberOfSublines(); subline < sublines; ++subline) {
				if(selectedRangeOnVisualLine(viewer.caret(), line, subline, range)) {
					range = Range<length_t>(
						range.beginning(),
						min(viewer.document().lineLength(line), range.end()));
					const RECT sublineBounds(layout.bounds(range.beginning(), range.end()));
					selectionBounds.left() = min(sublineBounds.left() + indent, selectionBounds.left());
					selectionBounds.right() = max(sublineBounds.right() + indent, selectionBounds.right());
					if(selectionBounds.right - selectionBounds.left > clientRect.width())
						return S_FALSE;	// overflow
				}
			}
		}
		bh.bV5Width = selectionBounds.width();
		bh.bV5Height = selectionBounds.height();

		// create a mask
		win32::Handle<HBITMAP> mask(::CreateBitmap(bh.bV5Width, bh.bV5Height, 1, 1, 0), &::DeleteObject);	// monochrome
		if(mask.get() == 0)
			return E_FAIL;
		HBITMAP oldBitmap = ::SelectObject(dc.get(), mask.get());
		dc.fillSolidRect(0, 0, bh.bV5Width, bh.bV5Height, RGB(0x00, 0x00, 0x00));
		int y = 0;
		for(length_t line = selectedRegion.beginning().line, e = selectedRegion.end().line; line <= e; ++line) {
			const LineLayout& layout = renderer.lineLayout(line);
			const int indent = renderer.lineIndent(line);
			Range<length_t> range;
			for(length_t subline = 0, sublines = layout.numberOfSublines(); subline < sublines; ++subline) {
				if(selectedRangeOnVisualLine(viewer.caret(), line, subline, range)) {
					range = Range<length_t>(
						range.beginning(),
						min(viewer.document().lineLength(line), range.end()));
					win32::Handle<HRGN> rgn(layout.blackBoxBounds(range.beginning(), range.end()));
					::OffsetRgn(rgn.get(), indent - selectionBounds.left, y - selectionBounds.top);
					::FillRgn(dc.get(), rgn.get(), Brush::getStockObject(WHITE_BRUSH).use());
				}
				y += renderer.textMetrics().linePitch();
			}
		}
		::SelectObject(dc.get(), oldBitmap);
		BITMAPINFO* bi = 0;
		AutoBuffer<manah::byte> maskBuffer;
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
		Rect<> selectionExtent(selectionBounds);
		selectionExtent.translate(-selectionExtent.left, -selectionExtent.top);
		y = selectionBounds.top;
		const LineLayout::Selection selection(viewer.caret());
		for(length_t line = selectedRegion.beginning().line, e = selectedRegion.end().line; line <= e; ++line) {
			renderer.renderLine(line, dc,
				renderer.lineIndent(line) - selectionBounds.left, y,
				selectionExtent, selectionExtent, highlightSelection ? &selection : 0);
			y += static_cast<int>(renderer.textMetrics().linePitch() * renderer.numberOfSublinesOfLine(line));
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
		const Rect<> margins(viewer.textAreaMargins());
		Point<> hotspot(cursorPosition);
		hotspot.x -= margins.left() - viewer.scrollPosition(SB_HORZ) * renderer.textMetrics().averageCharacterWidth() + selectionBounds.left();
		hotspot.y -= viewer.clientXYForCharacter(k::Position(selectedRegion.beginning().line, 0), true).y;

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
		hr = dnd_.dropTargetHelper->DragEnter(viewer_->get(), data, &p, *effect);
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
	inline SIZE calculateDnDScrollOffset(const TextViewer& viewer) {
		const POINT pt = viewer.getCursorPosition();
		RECT clientRect;
		viewer.getClientRect(clientRect);
		RECT margins = viewer.textAreaMargins();
		const TextRenderer& renderer = viewer.textRenderer();
		margins.left = max<long>(renderer.textMetrics().averageCharacterWidth(), margins.left);
		margins.top = max<long>(renderer.textMetrics().linePitch() / 2, margins.top);
		margins.right = max<long>(renderer.textMetrics().averageCharacterWidth(), margins.right);
		margins.bottom = max<long>(renderer.textMetrics().linePitch() / 2, margins.bottom);

		// oleidl.h defines the value named DD_DEFSCROLLINSET, but...

		SIZE result = {0, 0};
		if(pt.y >= clientRect.top && pt.y < clientRect.top + margins.top)
			result.cy = -1;
		else if(pt.y >= clientRect.bottom - margins.bottom && pt.y < clientRect.bottom)
			result.cy = +1;
		if(pt.x >= clientRect.left && pt.x < clientRect.left + margins.left)
			result.cx = -3;	// viewer_->numberOfVisibleColumns()
		else if(pt.x >= clientRect.right - margins.right && pt.y < clientRect.right)
			result.cx = +3;	// viewer_->numberOfVisibleColumns()
		return result;
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
		POINT caretPoint = {pt.x, pt.y};
		viewer_->screenToClient(caretPoint);
		const Position p(viewer_->characterForClientXY(caretPoint, LineLayout::TRAILING));
		viewer_->setCaretPosition(viewer_->clientXYForCharacter(p, true, LineLayout::LEADING));

		// drop rectangle text into bidirectional line is not supported...
		if(dnd_.numberOfRectangleLines != 0) {
			const length_t lines = min(viewer_->document().numberOfLines(), p.line + dnd_.numberOfRectangleLines);
			for(length_t line = p.line; line < lines; ++line) {
				if(viewer_->textRenderer().lineLayout(line).isBidirectional()) {
					acceptable = false;
					break;
				}
			}
		}
	}

	if(acceptable) {
		*effect = ((keyState & MK_CONTROL) != 0) ? DROPEFFECT_COPY : DROPEFFECT_MOVE;
		const SIZE scrollOffset = calculateDnDScrollOffset(*viewer_);
		if(scrollOffset.cx != 0 || scrollOffset.cy != 0) {
			*effect |= DROPEFFECT_SCROLL;
			// only one direction to scroll
			if(scrollOffset.cy != 0)
				viewer_->scroll(0, scrollOffset.cy, true);
			else
				viewer_->scroll(scrollOffset.cx, 0, true);
		}
	}
	if(dnd_.dropTargetHelper.get() != 0) {
		POINT p = {pt.x, pt.y};
		viewer_->lockScroll();
		dnd_.dropTargetHelper->DragOver(&p, *effect);	// damn! IDropTargetHelper scrolls the view
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
	Document& document = viewer_->document();
	if(dnd_.supportLevel == DONT_SUPPORT_OLE_DND || document.isReadOnly() || !viewer_->allowsMouseInput())
		return S_OK;
	Caret& ca = viewer_->caret();
	POINT caretPoint = {pt.x, pt.y};
	viewer_->screenToClient(caretPoint);
	const Position destination(viewer_->characterForClientXY(caretPoint, LineLayout::TRAILING));

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
		String text(selectedString(ca, NLF_RAW_VALUE));

		// can't drop into the selection
		if(isPointOverSelection(ca, caretPoint)) {
			ca.moveTo(destination);
			state_ = NONE;
		} else {
			const bool rectangle = ca.isSelectionRectangle();
			document.insertUndoBoundary();
			AutoFreeze af(viewer_);
			if(toBoolean(keyState & MK_CONTROL)) {	// copy
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
				pair<Point, Point> oldSelection(make_pair(Point(ca.anchor()), Point(ca)));
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
		viewer_->releaseCapture();
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
void DefaultMouseInputStrategy::extendSelection(const Position* to /* = 0 */) {
	if((state_ & SELECTION_EXTENDING_MASK) != SELECTION_EXTENDING_MASK)
		throw IllegalStateException("not extending the selection.");
	Position destination;
	if(to == 0) {
		RECT rc;
		const RECT margins = viewer_->textAreaMargins();
		viewer_->getClientRect(rc);
		POINT p = viewer_->getCursorPosition();
		Caret& caret = viewer_->caret();
		if(state_ != EXTENDING_CHARACTER_SELECTION) {
			const TextViewer::HitTestResult htr = viewer_->hitTest(p);
			if(state_ == EXTENDING_LINE_SELECTION && htr != TextViewer::INDICATOR_MARGIN && htr != TextViewer::LINE_NUMBERS)
				// end line selection
				state_ = EXTENDING_CHARACTER_SELECTION;
		}
		p.x = min(max(p.x, rc.left + margins.left), rc.right - margins.right);
		p.y = min(max(p.y, rc.top + margins.top), rc.bottom - margins.bottom);
		destination = viewer_->characterForClientXY(p, LineLayout::TRAILING);
	} else
		destination = *to;

	const Document& document = viewer_->document();
	Caret& caret = viewer_->caret();
	if(state_ == EXTENDING_CHARACTER_SELECTION)
		caret.extendSelection(destination);
	else if(state_ == EXTENDING_LINE_SELECTION) {
		const length_t lines = document.numberOfLines();
		Region s;
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
			WordBreakIterator<DocumentCharacterIterator> i(
				DocumentCharacterIterator(document, destination), AbstractWordBreakIterator::BOUNDARY_OF_SEGMENT, id);
			--i;
			caret.select(Position(selection_.initialLine, selection_.initialWordColumns.second),
				(i.base().tell().line == destination.line) ? i.base().tell() : Position(destination.line, 0));
		} else if(destination.line > selection_.initialLine
				|| (destination.line == selection_.initialLine
					&& destination.column > selection_.initialWordColumns.second)) {
			WordBreakIterator<DocumentCharacterIterator> i(
				DocumentCharacterIterator(document, destination), AbstractWordBreakIterator::BOUNDARY_OF_SEGMENT, id);
			++i;
			caret.select(Position(selection_.initialLine, selection_.initialWordColumns.first),
				(i.base().tell().line == destination.line) ?
					i.base().tell() : Position(destination.line, document.lineLength(destination.line)));
		} else
			caret.select(Position(selection_.initialLine, selection_.initialWordColumns.first),
				Position(selection_.initialLine, selection_.initialWordColumns.second));
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
bool DefaultMouseInputStrategy::handleLeftButtonDoubleClick(const POINT& position, uint keyState) {
	return false;
}

/// Handles @c WM_LBUTTONDOWN.
void DefaultMouseInputStrategy::handleLeftButtonPressed(const POINT& position, uint keyState) {
	bool boxDragging = false;
	Caret& caret = viewer_->caret();
	const TextViewer::HitTestResult htr = viewer_->hitTest(position);

	utils::closeCompletionProposalsPopup(*viewer_);
	endIncrementalSearch(*viewer_);

	// select line(s)
	if(htr == TextViewer::INDICATOR_MARGIN || htr == TextViewer::LINE_NUMBERS) {
		const Position to(viewer_->characterForClientXY(position, LineLayout::LEADING));
		const bool extend = toBoolean(keyState & MK_SHIFT) && to.line != caret.anchor().line();
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
		if(toBoolean(keyState & MK_CONTROL)) {
			if(!isPointOverSelection(caret, position)) {
				const Position p(viewer_->characterForClientXY(position, LineLayout::TRAILING, true));
				if(p != Position()) {
					if(const hyperlink::IHyperlink* link = getPointedHyperlink(*viewer_, p)) {
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
			const Position to(viewer_->characterForClientXY(position, LineLayout::TRAILING));
			state_ = EXTENDING_CHARACTER_SELECTION;
			if(toBoolean(keyState & (MK_CONTROL | MK_SHIFT))) {
				if(toBoolean(keyState & MK_CONTROL)) {
					// begin word selection
					state_ = EXTENDING_WORD_SELECTION;
					caret.moveTo(toBoolean(keyState & MK_SHIFT) ? caret.anchor() : to);
					selectWord(caret);
					selection_.initialLine = caret.line();
					selection_.initialWordColumns = make_pair(caret.beginning().column(), caret.end().column());
				}
				if(toBoolean(keyState & MK_SHIFT))
					extendSelection(&to);
			} else
				caret.moveTo(to);
			if(toBoolean(::GetKeyState(VK_MENU) & 0x8000))	// make the selection reactangle
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
void DefaultMouseInputStrategy::handleLeftButtonReleased(const POINT& position, uint) {
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
 * @param action same as @c IMouseInputStrategy#mouseButtonInput
 * @param position same as @c IMouseInputStrategy#mouseButtonInput
 * @param keyState same as @c IMouseInputStrategy#mouseButtonInput
 * @return same as @c IMouseInputStrategy#mouseButtonInput. the default implementation returns false
 */
bool DefaultMouseInputStrategy::handleRightButton(Action action, const POINT& position, uint keyState) {
	return false;
}

/**
 * Handles the first X1 button.
 * @param action same as @c IMouseInputStrategy#mouseButtonInput
 * @param position same as @c IMouseInputStrategy#mouseButtonInput
 * @param keyState same as @c IMouseInputStrategy#mouseButtonInput
 * @return same as @c IMouseInputStrategy#mouseButtonInput. the default implementation returns false
 */
bool DefaultMouseInputStrategy::handleX1Button(Action action, const POINT& position, uint keyState) {
	return false;
}

/**
 * Handles the first X2 button.
 * @param action same as @c IMouseInputStrategy#mouseButtonInput
 * @param position same as @c IMouseInputStrategy#mouseButtonInput
 * @param keyState same as @c IMouseInputStrategy#mouseButtonInput
 * @return same as @c IMouseInputStrategy#mouseButtonInput. the default implementation returns false
 */
bool DefaultMouseInputStrategy::handleX2Button(Action action, const POINT& position, uint keyState) {
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

/// @see IMouseInputStrategy#interruptMouseReaction
void DefaultMouseInputStrategy::interruptMouseReaction(bool forKeyboardInput) {
	if(state_ == AUTO_SCROLL_DRAGGING || state_ == AUTO_SCROLL)
		endAutoScroll();
}

/// @see IMouseInputStrategy#mouseButtonInput
bool DefaultMouseInputStrategy::mouseButtonInput(Button button, Action action, const POINT& position, uint keyState) {
	if(action != RELEASED && endAutoScroll())
		return true;
	switch(button) {
	case LEFT_BUTTON:
		if(action == PRESSED)
			handleLeftButtonPressed(position, keyState);
		else if(action == RELEASED)
			handleLeftButtonReleased(position, keyState);
		else if(action == DOUBLE_CLICKED) {
			abortIncrementalSearch(*viewer_);
			if(handleLeftButtonDoubleClick(position, keyState))
				return true;
			const TextViewer::HitTestResult htr = viewer_->hitTest(viewer_->getCursorPosition());
			if(htr == TextViewer::LEADING_MARGIN || htr == TextViewer::TOP_MARGIN || htr == TextViewer::TEXT_AREA) {
				// begin word selection
				Caret& caret = viewer_->caret();
				selectWord(caret);
				state_ = EXTENDING_WORD_SELECTION;
				selection_.initialLine = caret.line();
				selection_.initialWordColumns = make_pair(caret.anchor().column(), caret.column());
				viewer_->setCapture();
				beginTimer(SELECTION_EXPANSION_INTERVAL);
				return true;
			}
		}
		break;
	case MIDDLE_BUTTON:
		if(action == PRESSED) {
			if(viewer_->document().numberOfLines() > viewer_->numberOfVisibleLines()) {
				state_ = APPROACHING_AUTO_SCROLL;
				dragApproachedPosition_ = position;
				POINT p(position);
				viewer_->clientToScreen(p);
				viewer_->setFocus();
				// show the indicator margin
				RECT rect;
				autoScrollOriginMark_->getRect(rect);
				autoScrollOriginMark_->setPosition(HWND_TOP,
					p.x - (rect.right - rect.left) / 2, p.y - (rect.bottom - rect.top) / 2,
					0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_SHOWWINDOW);
				viewer_->setCapture();
				showCursor(position);
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
	case RIGHT_BUTTON:
		return handleRightButton(action, position, keyState);
	case X1_BUTTON:
		return handleX1Button(action, position, keyState);
	case X2_BUTTON:
		return handleX2Button(action, position, keyState);
	}
	return false;
}

/// @see IMouseInputStrategy#mouseMoved
void DefaultMouseInputStrategy::mouseMoved(const POINT& position, uint) {
	if(state_ == APPROACHING_AUTO_SCROLL
			|| (dnd_.supportLevel >= SUPPORT_OLE_DND && state_ == APPROACHING_OLE_DND)) {	// OLE dragging starts?
		if(state_ == APPROACHING_OLE_DND && isSelectionEmpty(viewer_->caret()))
			state_ = NONE;	// approaching... => cancel
		else {
			// the following code can be replaced with DragDetect in user32.lib
			const int cxDragBox = ::GetSystemMetrics(SM_CXDRAG);
			const int cyDragBox = ::GetSystemMetrics(SM_CYDRAG);
			if((position.x > dragApproachedPosition_.x + cxDragBox / 2)
					|| (position.x < dragApproachedPosition_.x - cxDragBox / 2)
					|| (position.y > dragApproachedPosition_.y + cyDragBox / 2)
					|| (position.y < dragApproachedPosition_.y - cyDragBox / 2)) {
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

/// @see IMouseInputStrategy#mouseWheelRotated
void DefaultMouseInputStrategy::mouseWheelRotated(short delta, const POINT&, uint) {
	if(!endAutoScroll()) {
		// use system settings
		UINT lines;	// the number of lines to scroll
		if(!toBoolean(::SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &lines, 0)))
			lines = 3;
		if(lines == WHEEL_PAGESCROLL)
			lines = static_cast<UINT>(viewer_->numberOfVisibleLines());
		delta *= static_cast<short>(lines);
		viewer_->scroll(0, -delta / WHEEL_DELTA, true);
	}
}

/// @see IDropSource#QueryContinueDrag
STDMETHODIMP DefaultMouseInputStrategy::QueryContinueDrag(BOOL escapePressed, DWORD keyState) {
	if(toBoolean(escapePressed) || toBoolean(keyState & MK_RBUTTON))	// cancel
		return DRAGDROP_S_CANCEL;
	if(!toBoolean(keyState & MK_LBUTTON))	// drop
		return DRAGDROP_S_DROP;
	return S_OK;
}

/// @see IMouseInputStrategy#showCursor
bool DefaultMouseInputStrategy::showCursor(const POINT& position) {
	using namespace hyperlink;
	LPCTSTR cursorName = 0;
	const IHyperlink* newlyHoveredHyperlink = 0;

	// on the vertical ruler?
	const TextViewer::HitTestResult htr = viewer_->hitTest(position);
	if(htr == TextViewer::INDICATOR_MARGIN || htr == TextViewer::LINE_NUMBERS)
		cursorName = IDC_ARROW;
	// on a draggable text selection?
	else if(dnd_.supportLevel >= SUPPORT_OLE_DND && !isSelectionEmpty(viewer_->caret()) && isPointOverSelection(viewer_->caret(), position))
		cursorName = IDC_ARROW;
	else if(htr == TextViewer::TEXT_AREA) {
		// on a hyperlink?
		const Position p(viewer_->characterForClientXY(position, LineLayout::TRAILING, true, k::locations::UTF16_CODE_UNIT));
		if(p != Position())
			newlyHoveredHyperlink = getPointedHyperlink(*viewer_, p);
		if(newlyHoveredHyperlink != 0 && toBoolean(::GetAsyncKeyState(VK_CONTROL) & 0x8000))
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
		const POINT pt = self.viewer_->getCursorPosition();
		RECT rc;
		const RECT margins = self.viewer_->textAreaMargins();
		self.viewer_->getClientRect(rc);
		// 以下のスクロール量には根拠は無い
		if(pt.y < rc.top + margins.top)
			self.viewer_->scroll(0, (pt.y - (rc.top + margins.top)) / self.viewer_->textRenderer().textMetrics().linePitch() - 1, true);
		else if(pt.y >= rc.bottom - margins.bottom)
			self.viewer_->scroll(0, (pt.y - (rc.bottom - margins.bottom)) / self.viewer_->textRenderer().textMetrics().linePitch() + 1, true);
		else if(pt.x < rc.left + margins.left)
			self.viewer_->scroll((pt.x - (rc.left + margins.left)) / self.viewer_->textRenderer().textMetrics().averageCharacterWidth() - 1, 0, true);
		else if(pt.x >= rc.right - margins.right)
			self.viewer_->scroll((pt.x - (rc.right - margins.right)) / self.viewer_->textRenderer().textMetrics().averageCharacterWidth() + 1, 0, true);
		self.extendSelection();
	} else if(self.state_ == AUTO_SCROLL_DRAGGING || self.state_ == AUTO_SCROLL) {
		self.endTimer();
		TextViewer& viewer = *self.viewer_;
		const POINT pt = self.viewer_->getCursorPosition();
		const long yScrollDegree = (pt.y - self.dragApproachedPosition_.y) / viewer.textRenderer().textMetrics().linePitch();
//		const long xScrollDegree = (pt.x - self.dragApproachedPosition.x) / viewer.presentation().textMetrics().lineHeight();
//		const long scrollDegree = max(abs(yScrollDegree), abs(xScrollDegree));

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
