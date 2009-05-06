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

	void cancel(EditorView& ed) {
		(CancelCommand(ed))();
	}

	bool convertCharacterToCodePoint(EditorView& ed) {
		return CharacterToCodePointConversionCommand(ed)();
	}

	bool convertCodePointToCharacter(EditorView& ed) {
		return CodePointToCharacterConversionCommand(ed)();
	}

	bool deleteBackwardCharacter(EditorView& ed, py::ssize_t n) {
		return CharacterDeletionCommand(ed, Direction::BACKWARD).setNumericPrefix(static_cast<long>(n))();
	}

	bool deleteBackwardWord(EditorView& ed, py::ssize_t n) {
		return WordDeletionCommand(ed, Direction::BACKWARD).setNumericPrefix(static_cast<long>(n))();
	}

	bool deleteForwardCharacter(EditorView& ed, py::ssize_t n) {
		return CharacterDeletionCommand(ed, Direction::FORWARD).setNumericPrefix(static_cast<long>(n))();
	}

	bool deleteForwardWord(EditorView& ed, py::ssize_t n) {
		return WordDeletionCommand(ed, Direction::FORWARD).setNumericPrefix(static_cast<long>(n))();
	}

	bool paste(bool useKillRing) {
		return PasteCommand(activeViewer(), useKillRing)() == 0;
	}

	bool redo(py::ssize_t n) {
		return UndoCommand(activeViewer(), true).setNumericPrefix(static_cast<long>(n))() == 0;
	}

	void selectAll() {
		EntireDocumentSelectionCreationCommand temp(activeViewer());
		temp();
	}

	void showPossibleCompletions() {
		EditorView& viewer = EditorWindows::instance().activePane().visibleView();
		if(contentassist::IContentAssistant* ca = viewer.contentAssistant())
			ca->showPossibleCompletions();
		else
			viewer.beep();
	}

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
//	py::def("copy_selection", &);
//	py::def("cut_selection", &);
	py::def("delete_backward_character", &deleteBackwardCharacter, (py::arg("ed"), py::arg("n") = 1));
	py::def("delete_backward_word", &deleteBackwardWord, (py::arg("ed"), py::arg("n") = 1));
	py::def("delete_forward_character", &deleteForwardCharacter, (py::arg("ed"), py::arg("n") = 1));
	py::def("delete_forward_word", &deleteForwardWord, (py::arg("ed"), py::arg("n") = 1));
/*	py::def("delete_line", &);
	py::def("find_next", &findNext, (py::arg("ed"), py::arg("n") = 1));
	py::def("find_previous", &findPrevious, (py::arg("ed"), py::arg("n") = 1));
	py::def("input_character", &);
	py::def("input_character_from_next_line", &);
	py::def("input_character_from_previous_line", &);
	py::def("indent", &);
	py::def("insert_previous_line", &);
	py::def("insert_string", &);
	py::def("isearch_backward", &);
	py::def("isearch_forward", &);
	py::def("newline", &newline, (py::arg("ed"), py::arg("n") = 1));
	py::def("paste", &);
	py::def("reconvert", &);
	py::def("redo", &redo, (py::arg("ed"), py::arg("n") = 1));
	py::def("replace_all", &);
	py::def("select_all", &);
	py::def("select_word", &);
	py::def("show_completion_proposals_popup", &);
	py::def("toggle_bookmark", &);
	py::def("toggle_ime_status", &);
	py::def("toggle_overtype_mode", &);
	py::def("toggle_soft_keyboard_mode", &);
	py::def("transpose_characters", &);
	py::def("transpose_lines", &);
	py::def("transpose_words", &);
	py::def("undo", &undo, (py::arg("ed"), py::arg("n") = 1));
	py::def("unindent", &);*/
ALPHA_EXPOSE_EPILOGUE()
