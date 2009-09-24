/**
 * @file intrinsic.cpp
 */

#include "application.hpp"
#include "buffer.hpp"
#include "editor-window.hpp"
#include <ascension/text-editor.hpp>
using namespace alpha;
using namespace alpha::ambient;
namespace py = boost::python;
using namespace ascension::texteditor::commands;
namespace a = ascension;
namespace k = ascension::kernel;
namespace v = ascension::viewers;

namespace {	// helpers
	inline EditorView& activeViewer() {
		return EditorWindows::instance().activePane().visibleView();
	}

	inline EditorView& extractEditor(py::object o) {
		if(o == py::object())
			return activeViewer();
		return py::extract<EditorView&>(o);
	}
}

namespace {
	py::ssize_t bookmarkMatchLines(py::object ed, bool onlySelection) {
		EditorView& editor = extractEditor(ed);
		BookmarkMatchLinesCommand temp(editor, onlySelection ? editor.caret().selectedRegion() : k::Region());
		temp();
		return temp.numberOfMarkedLines();
	}

	void cancel(py::object ed) {(CancelCommand(extractEditor(ed)))();}

	bool convertCharacterToCodePoint(py::object ed) {return CharacterToCodePointConversionCommand(extractEditor(ed))();}

	bool convertCodePointToCharacter(py::object ed) {return CodePointToCharacterConversionCommand(extractEditor(ed))();}

/*	bool copySelection(py::object ed, bool useKillRing) {
		try {
			extractEditor(ed).caret().copySelection(useKillRing);
		} catch(viewers::ClipboardException&) {
			return false;
		}
		return true;
	}

	bool cutSelection(py::object ed, bool useKillRing) {
		try {
			extractEditor(ed).caret().cutSelection(useKillRing);
		} catch(viewers::ClipboardException&) {
			return false;
		}
		return true;
	}
*/
	template<const a::Direction* direction> bool deleteCharacter(py::object ed, py::ssize_t n) {
		return CharacterDeletionCommand(extractEditor(ed), *direction).setNumericPrefix(static_cast<long>(n))();}

	template<const a::Direction* direction> bool deleteWord(py::object ed, py::ssize_t n) {
		return WordDeletionCommand(extractEditor(ed), *direction).setNumericPrefix(static_cast<long>(n))();}

	template<const a::Direction* direction> bool findNext(py::object ed, py::ssize_t n) {
		return FindNextCommand(extractEditor(ed), *direction).setNumericPrefix(static_cast<long>(n))();}

	bool inputCharacter(py::object ed, int character) {return CharacterInputCommand(extractEditor(ed), character)();}

	template<bool previous> bool inputCharacterFromNextLine(py::object ed) {return CharacterInputFromNextLineCommand(extractEditor(ed), previous)();}

	bool indent(py::object ed, py::ssize_t n);

	bool insertString(py::object ed, const a::String& s, py::ssize_t n) {
		try {
			return TextInputCommand(extractEditor(ed), s).setNumericPrefix(static_cast<long>(n))();
		} catch(k::DocumentCantChangeException&) {
			return false;
		}
		return true;
	}

//	template<const Direction* direction> bool isearch() {}

	template<typename Signature, Signature* procedure>
	void moveCaret(py::object ed, bool extendSelection) {CaretMovementCommand(extractEditor(ed), procedure, extendSelection)();}

	template<typename Signature, Signature* procedure> void moveCaretN(py::object ed, bool extendSelection, py::ssize_t n) {
		CaretMovementCommand(extractEditor(ed), procedure, extendSelection).setNumericPrefix(static_cast<long>(n))();}

	template<bool previous> bool newline(py::object ed, py::ssize_t n) {
		return NewlineCommand(extractEditor(ed), previous).setNumericPrefix(static_cast<long>(n))();}

	bool paste(py::object ed, bool useKillRing) {return PasteCommand(extractEditor(ed), useKillRing)();}

	bool reconvert(py::object ed) {return (ReconversionCommand(extractEditor(ed)))();}

	void selectAll(py::object ed) {(EntireDocumentSelectionCreationCommand(extractEditor(ed)))();}

	void selectWord(py::object ed) {(WordSelectionCreationCommand(extractEditor(ed)))();}

	bool showCompletionProposalsPopup(py::object ed) {return (CompletionProposalPopupCommand(extractEditor(ed)))();}

	bool toggleIMEStatus(py::object ed) {return (InputMethodOpenStatusToggleCommand(extractEditor(ed)))();}

	bool toggleOvertypeMode(py::object ed) {return (OvertypeModeToggleCommand(extractEditor(ed)))();}

	bool toggleSoftKeyboardMode(py::object ed) {return (InputMethodSoftKeyboardModeToggleCommand(extractEditor(ed)))();}

	template<bool(*procedure)(v::Caret&)> bool transpose(py::object ed /*, py::ssize_t n*/) {return TranspositionCommand(extractEditor(ed), procedure)();}

	template<bool redo> bool undo(py::object ed, py::ssize_t n) {return UndoCommand(extractEditor(ed), redo).setNumericPrefix(static_cast<long>(n))();}
} // namespace @0

ALPHA_EXPOSE_PROLOGUE(10)
	py::scope temp(ambient::Interpreter::instance().module("intrinsics"));

	py::def("backward_bookmark",
		&moveCaretN<k::Position(const k::Point&, a::length_t), &k::locations::backwardBookmark>,
		(py::arg("ed") = py::object(), py::arg("extend_selection") = false, py::arg("n") = 1));
	py::def("backward_character",
		&moveCaretN<k::Position(const k::Point&, k::locations::CharacterUnit, a::length_t), &k::locations::backwardCharacter>,
		(py::arg("ed") = py::object(), py::arg("extend_selection") = false, py::arg("n") = 1));
	py::def("backward_line",
		&moveCaretN<k::Position(const k::Point&, a::length_t), &k::locations::backwardLine>,
		(py::arg("ed") = py::object(), py::arg("extend_selection") = false, py::arg("n") = 1));
	py::def("backward_page",
		&moveCaretN<v::VerticalDestinationProxy(const v::VisualPoint&, a::length_t), &k::locations::backwardPage>,
		(py::arg("ed") = py::object(), py::arg("extend_selection") = false, py::arg("n") = 1));
	py::def("backward_visual_line",
		&moveCaretN<v::VerticalDestinationProxy(const v::VisualPoint&, a::length_t), &k::locations::backwardVisualLine>,
		(py::arg("ed") = py::object(), py::arg("extend_selection") = false, py::arg("n") = 1));
	py::def("backward_word",
		&moveCaretN<k::Position(const k::Point&, a::length_t), &k::locations::backwardWord>,
		(py::arg("ed") = py::object(), py::arg("extend_selection") = false, py::arg("n") = 1));
	py::def("backward_word_end",
		&moveCaretN<k::Position(const k::Point&, a::length_t), &k::locations::backwardWordEnd>,
		(py::arg("ed") = py::object(), py::arg("extend_selection") = false, py::arg("n") = 1));
	py::def("beginning_of_buffer",
		&moveCaret<k::Position(const k::Point&), &k::locations::beginningOfDocument>,
		(py::arg("ed") = py::object(), py::arg("extend_selection") = false));
	py::def("beginning_of_line",
		&moveCaret<k::Position(const k::Point&), &k::locations::beginningOfLine>,
		(py::arg("ed") = py::object(), py::arg("extend_selection") = false));
	py::def("beginning_of_visual_line",
		&moveCaret<k::Position(const v::VisualPoint&), &k::locations::beginningOfVisualLine>,
		(py::arg("ed") = py::object(), py::arg("extend_selection") = false));
	py::def("bookmark_match_lines", &bookmarkMatchLines, (py::arg("ed") = py::object(), py::arg("only_selection") = false));
	py::def("cancel", &cancel, py::arg("ed") = py::object());
//	py::def("clear_all_bookmarks", &);
	py::def("contextual_beginning_of_line",
		&moveCaret<k::Position(const v::VisualPoint&), &k::locations::contextualBeginningOfLine>,
		(py::arg("ed") = py::object(), py::arg("extend_selection") = false));
	py::def("contextual_beginning_of_visual_line",
		&moveCaret<k::Position(const v::VisualPoint&), &k::locations::contextualBeginningOfVisualLine>,
		(py::arg("ed") = py::object(), py::arg("extend_selection") = false));
	py::def("contextual_end_of_line",
		&moveCaret<k::Position(const v::VisualPoint&), &k::locations::contextualEndOfLine>,
		(py::arg("ed") = py::object(), py::arg("extend_selection") = false));
	py::def("contextual_end_of_visual_line",
		&moveCaret<k::Position(const v::VisualPoint&), &k::locations::contextualEndOfVisualLine>,
		(py::arg("ed") = py::object(), py::arg("extend_selection") = false));
	py::def("convert_character_to_code_point", &convertCharacterToCodePoint, py::arg("ed") = py::object());
	py::def("convert_code_point_to_character", &convertCodePointToCharacter, py::arg("ed") = py::object());
//	py::def("copy_selection", &copySelection, (py::arg("ed") = py::object(), py::arg("use_kill_ring") = true));
//	py::def("cut_selection", &cutSelection, (py::arg("ed") = py::object(), py::arg("use_kill_ring") = true));
	py::def("delete_backward_character", &deleteCharacter<&a::Direction::BACKWARD>, (py::arg("ed") = py::object(), py::arg("n") = 1));
	py::def("delete_backward_word", &deleteWord<&a::Direction::BACKWARD>, (py::arg("ed") = py::object(), py::arg("n") = 1));
	py::def("delete_forward_character", &deleteCharacter<&a::Direction::FORWARD>, (py::arg("ed") = py::object(), py::arg("n") = 1));
	py::def("delete_forward_word", &deleteWord<&a::Direction::FORWARD>, (py::arg("ed") = py::object(), py::arg("n") = 1));
//	py::def("delete_line", &);
	py::def("end_of_buffer",
		&moveCaret<k::Position(const k::Point&), &k::locations::endOfDocument>,
		(py::arg("ed") = py::object(), py::arg("extend_selection") = false));
	py::def("end_of_line",
		&moveCaret<k::Position(const k::Point&), &k::locations::endOfLine>,
		(py::arg("ed") = py::object(), py::arg("extend_selection") = false));
	py::def("end_of_visual_line",
		&moveCaret<k::Position(const v::VisualPoint&), &k::locations::endOfVisualLine>,
		(py::arg("ed") = py::object(), py::arg("extend_selection") = false));
	py::def("find_next", &findNext<&a::Direction::FORWARD>, (py::arg("ed") = py::object(), py::arg("n") = 1));
	py::def("find_previous", &findNext<&a::Direction::BACKWARD>, (py::arg("ed") = py::object(), py::arg("n") = 1));
	py::def("first_printable_character_of_line",
		&moveCaret<k::Position(const v::VisualPoint&), &k::locations::firstPrintableCharacterOfLine>,
		(py::arg("ed") = py::object(), py::arg("extend_selection") = false));
	py::def("first_printable_character_of_visual_line",
		&moveCaret<k::Position(const v::VisualPoint&), &k::locations::firstPrintableCharacterOfVisualLine>,
		(py::arg("ed") = py::object(), py::arg("extend_selection") = false));
	py::def("forward_bookmark",
		&moveCaretN<k::Position(const k::Point&, a::length_t), &k::locations::forwardBookmark>,
		(py::arg("ed") = py::object(), py::arg("extend_selection") = false, py::arg("n") = 1));
	py::def("forward_character",
		&moveCaretN<k::Position(const k::Point&, k::locations::CharacterUnit, a::length_t), &k::locations::forwardCharacter>,
		(py::arg("ed") = py::object(), py::arg("extend_selection") = false, py::arg("n") = 1));
	py::def("forward_line",
		&moveCaretN<k::Position(const k::Point&, a::length_t), &k::locations::forwardLine>,
		(py::arg("ed") = py::object(), py::arg("extend_selection") = false, py::arg("n") = 1));
	py::def("forward_page",
		&moveCaretN<v::VerticalDestinationProxy(const v::VisualPoint&, a::length_t), &k::locations::forwardPage>,
		(py::arg("ed") = py::object(), py::arg("extend_selection") = false, py::arg("n") = 1));
	py::def("forward_visual_line",
		&moveCaretN<v::VerticalDestinationProxy(const v::VisualPoint&, a::length_t), &k::locations::forwardVisualLine>,
		(py::arg("ed") = py::object(), py::arg("extend_selection") = false, py::arg("n") = 1));
	py::def("forward_word",
		&moveCaretN<k::Position(const k::Point&, a::length_t), &k::locations::forwardWord>,
		(py::arg("ed") = py::object(), py::arg("extend_selection") = false, py::arg("n") = 1));
	py::def("forward_word_end",
		&moveCaretN<k::Position(const k::Point&, a::length_t), &k::locations::forwardWordEnd>,
		(py::arg("ed") = py::object(), py::arg("extend_selection") = false, py::arg("n") = 1));
	py::def("input_character", &inputCharacter, (py::arg("ed") = py::object(), py::arg("character")));
	py::def("input_character_from_next_line", &inputCharacterFromNextLine<false>, py::arg("ed") = py::object());
	py::def("input_character_from_previous_line", &inputCharacterFromNextLine<true>, py::arg("ed") = py::object());
//	py::def("indent", &indent);
	py::def("insert_previous_line", &newline<true>, (py::arg("ed") = py::object(), py::arg("n") = 1));
	py::def("insert_string", &insertString, (py::arg("ed") = py::object(), py::arg("string"), py::arg("n") = 1));
//	py::def("isearch_backward", &);
//	py::def("isearch_forward", &);
	py::def("last_printable_character_of_line",
		&moveCaret<k::Position(const v::VisualPoint&), &k::locations::lastPrintableCharacterOfLine>,
		(py::arg("ed") = py::object(), py::arg("extend_selection") = false));
	py::def("last_printable_character_of_visual_line",
		&moveCaret<k::Position(const v::VisualPoint&), &k::locations::lastPrintableCharacterOfVisualLine>,
		(py::arg("ed") = py::object(), py::arg("extend_selection") = false));
	py::def("left_character",
		&moveCaretN<k::Position(const v::VisualPoint&, k::locations::CharacterUnit, a::length_t), &k::locations::leftCharacter>,
		(py::arg("ed") = py::object(), py::arg("extend_selection") = false, py::arg("n") = 1));
	py::def("left_word",
		&moveCaretN<k::Position(const v::VisualPoint&, a::length_t), &k::locations::leftWord>,
		(py::arg("ed") = py::object(), py::arg("extend_selection") = false, py::arg("n") = 1));
	py::def("right_word_end",
		&moveCaretN<k::Position(const v::VisualPoint&, a::length_t), &k::locations::leftWordEnd>,
		(py::arg("ed") = py::object(), py::arg("extend_selection") = false, py::arg("n") = 1));
	py::def("newline", &newline<false>, (py::arg("ed") = py::object(), py::arg("n") = 1));
	py::def("paste", &paste, (py::arg("ed") = py::object(), py::arg("use_killring") = true));
	py::def("reconvert", &reconvert, py::arg("ed") = py::object());
	py::def("redo", &undo<true>, (py::arg("ed") = py::object(), py::arg("n") = 1));
//	py::def("replace_all", &);
	py::def("right_character",
		&moveCaretN<k::Position(const v::VisualPoint&, k::locations::CharacterUnit, a::length_t), &k::locations::rightCharacter>,
		(py::arg("ed") = py::object(), py::arg("extend_selection") = false, py::arg("n") = 1));
	py::def("right_word",
		&moveCaretN<k::Position(const v::VisualPoint&, a::length_t), &k::locations::rightWord>,
		(py::arg("ed") = py::object(), py::arg("extend_selection") = false, py::arg("n") = 1));
	py::def("right_word_end",
		&moveCaretN<k::Position(const v::VisualPoint&, a::length_t), &k::locations::rightWordEnd>,
		(py::arg("ed") = py::object(), py::arg("extend_selection") = false, py::arg("n") = 1));
	py::def("select_all", &selectAll, py::arg("ed") = py::object());
	py::def("select_word", &selectWord, py::arg("ed") = py::object());
	py::def("show_completion_proposals_popup", &showCompletionProposalsPopup, py::arg("ed") = py::object());
//	py::def("toggle_bookmark", &);
	py::def("toggle_ime_status", &toggleIMEStatus, py::arg("ed") = py::object());
	py::def("toggle_overtype_mode", &toggleOvertypeMode, py::arg("ed") = py::object());
	py::def("toggle_soft_keyboard_mode", &toggleSoftKeyboardMode, py::arg("ed") = py::object());
	py::def("transpose_characters", &transpose<&v::transposeCharacters>/*, (py::arg("ed") = py::object(), py::arg("n") = 1)*/);
	py::def("transpose_lines", &transpose<&v::transposeLines>/*, (py::arg("ed") = py::object(), py::arg("n") = 1)*/);
	py::def("transpose_words", &transpose<&v::transposeWords>/*, (py::arg("ed") = py::object(), py::arg("n") = 1)*/);
	py::def("undo", &undo<false>, (py::arg("ed") = py::object(), py::arg("n") = 1));
//	py::def("unindent", &unindent);
ALPHA_EXPOSE_EPILOGUE()
