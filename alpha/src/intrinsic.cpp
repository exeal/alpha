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
using namespace ascension;
using namespace ascension::texteditor::commands;

namespace {	// helpers
	inline EditorView& activeViewer() {
		return EditorWindows::instance().activePane().visibleView();
	}
}

namespace {
	py::ssize_t bookmarkMatchLines(EditorView& ed, bool onlySelection) {
		BookmarkMatchLinesCommand temp(ed, onlySelection ? ed.caret().selectedRegion() : kernel::Region());
		temp();
		return temp.numberOfMarkedLines();
	}

	void cancel(EditorView& ed) {(CancelCommand(ed))();}

	bool convertCharacterToCodePoint(EditorView& ed) {return CharacterToCodePointConversionCommand(ed)();}

	bool convertCodePointToCharacter(EditorView& ed) {return CodePointToCharacterConversionCommand(ed)();}

/*	bool copySelection(EditorView& ed, bool useKillRing) {
		try {
			ed.caret().copySelection(useKillRing);
		} catch(viewers::ClipboardException&) {
			return false;
		}
		return true;
	}

	bool cutSelection(EditorView& ed, bool useKillRing) {
		try {
			ed.caret().cutSelection(useKillRing);
		} catch(viewers::ClipboardException&) {
			return false;
		}
		return true;
	}
*/
	template<const Direction* direction> bool deleteCharacter(EditorView& ed, py::ssize_t n) {return CharacterDeletionCommand(ed, *direction).setNumericPrefix(static_cast<long>(n))();}

	template<const Direction* direction> bool deleteWord(EditorView& ed, py::ssize_t n) {return WordDeletionCommand(ed, *direction).setNumericPrefix(static_cast<long>(n))();}

	template<const Direction* direction> bool findNext(EditorView& ed, py::ssize_t n) {return FindNextCommand(ed, *direction).setNumericPrefix(static_cast<long>(n))();}

	bool inputCharacter(EditorView& ed, int character) {return CharacterInputCommand(ed, character)();}

	template<bool previous> bool inputCharacterFromNextLine(EditorView& ed) {return CharacterInputFromNextLineCommand(ed, previous)();}

	bool indent(EditorView& ed, py::ssize_t n);

	bool insertString(EditorView& ed, const String& s, py::ssize_t n) {
		try {
			return TextInputCommand(ed, s).setNumericPrefix(static_cast<long>(n))();
		} catch(kernel::DocumentCantChangeException&) {
			return false;
		}
		return true;
	}

//	template<const Direction* direction> bool isearch() {}

	template<bool previous> bool newline(EditorView& ed, py::ssize_t n) {return NewlineCommand(ed, previous).setNumericPrefix(static_cast<long>(n))();}

	bool paste(EditorView& ed, bool useKillRing) {return PasteCommand(ed, useKillRing)();}

	bool reconvert(EditorView& ed) {return (ReconversionCommand(ed))();}

//	bool redo(EditorView& ed, py::ssize_t n) {
//		return UndoCommand(activeViewer(), true).setNumericPrefix(static_cast<long>(n))() == 0;
//	}

	void selectAll(EditorView& ed) {(EntireDocumentSelectionCreationCommand(ed))();}

	void selectWord(EditorView& ed) {(WordSelectionCreationCommand(ed))();}

	bool showCompletionProposalsPopup(EditorView& ed) {return (CompletionProposalPopupCommand(ed))();}

	bool toggleIMEStatus(EditorView& ed) {return (InputMethodOpenStatusToggleCommand(ed))();}

	bool toggleOvertypeMode(EditorView& ed) {return (OvertypeModeToggleCommand(ed))();}

	bool toggleSoftKeyboardMode(EditorView& ed) {return (InputMethodSoftKeyboardModeToggleCommand(ed))();}

	template<bool(*procedure)(viewers::Caret&)> bool transpose(EditorView& ed /*, py::ssize_t n*/) {return TranspositionCommand(ed, procedure)();}

	bool undo(py::ssize_t n) {
		return UndoCommand(activeViewer(), false).setNumericPrefix(static_cast<long>(n))() == 0;
	}
} // namespace @0

ALPHA_EXPOSE_PROLOGUE()
py::scope temp(ambient::Interpreter::instance().module("intrinsics"));

	py::def("bookmark_match_lines", &bookmarkMatchLines);
	py::def("cancel", &cancel);
//	py::def("clear_all_bookmarks", &);
	py::def("convert_character_to_code_point", &convertCharacterToCodePoint);
	py::def("convert_code_point_to_character", &convertCodePointToCharacter);
//	py::def("copy_selection", &copySelection, (py::arg("ed"), py::arg("use_kill_ring") = true));
//	py::def("cut_selection", &cutSelection, (py::arg("ed"), py::arg("use_kill_ring") = true));
	py::def("delete_backward_character", &deleteCharacter<&Direction::BACKWARD>, (py::arg("ed"), py::arg("n") = 1));
	py::def("delete_backward_word", &deleteWord<&Direction::BACKWARD>, (py::arg("ed"), py::arg("n") = 1));
	py::def("delete_forward_character", &deleteCharacter<&Direction::FORWARD>, (py::arg("ed"), py::arg("n") = 1));
	py::def("delete_forward_word", &deleteWord<&Direction::FORWARD>, (py::arg("ed"), py::arg("n") = 1));
//	py::def("delete_line", &);
	py::def("find_next", &findNext<&Direction::FORWARD>, (py::arg("ed"), py::arg("n") = 1));
	py::def("find_previous", &findNext<&Direction::BACKWARD>, (py::arg("ed"), py::arg("n") = 1));
	py::def("input_character", &inputCharacter);
	py::def("input_character_from_next_line", &inputCharacterFromNextLine<false>);
	py::def("input_character_from_previous_line", &inputCharacterFromNextLine<true>);
//	py::def("indent", &indent);
	py::def("insert_previous_line", &newline<true>, (py::arg("ed"), py::arg("n") = 1));
	py::def("insert_string", &insertString);
//	py::def("isearch_backward", &);
//	py::def("isearch_forward", &);
	py::def("newline", &newline<false>, (py::arg("ed"), py::arg("n") = 1));
	py::def("paste", &paste);
	py::def("reconvert", &reconvert);
//	py::def("redo", &redo, (py::arg("ed"), py::arg("n") = 1));
//	py::def("replace_all", &);
	py::def("select_all", &selectAll);
	py::def("select_word", &selectWord);
	py::def("show_completion_proposals_popup", &showCompletionProposalsPopup);
//	py::def("toggle_bookmark", &);
	py::def("toggle_ime_status", &toggleIMEStatus);
	py::def("toggle_overtype_mode", &toggleOvertypeMode);
	py::def("toggle_soft_keyboard_mode", &toggleSoftKeyboardMode);
	py::def("transpose_characters", &transpose<&viewers::transposeCharacters>/*, (py::arg("ed"), py::arg("n") = 1)*/);
	py::def("transpose_lines", &transpose<&viewers::transposeLines>/*, (py::arg("ed"), py::arg("n") = 1)*/);
	py::def("transpose_words", &transpose<&viewers::transposeWords>/*, (py::arg("ed"), py::arg("n") = 1)*/);
//	py::def("undo", Invoker<UndoCommand, >, (py::arg("ed"), py::arg("n") = 1));
//	py::def("unindent", &unindent);
ALPHA_EXPOSE_EPILOGUE()
