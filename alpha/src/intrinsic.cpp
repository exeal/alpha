/**
 * @file intrinsic.cpp
 */

#include "intrinsic.hpp"
#include "application.hpp"
#include "buffer.hpp"
#include "editor-window.hpp"
#include "search.hpp"
#include "resource/messages.h"
#include "ascension/text-editor.hpp"
using namespace alpha::ambient;
using namespace alpha::ambient::intrinsic;
using namespace alpha;
namespace py = boost::python;
using namespace ascension;
using namespace ascension::texteditor::commands;

namespace {	// helpers
	inline EditorView& activeViewer() {
		return EditorWindows::instance().activePane().visibleView();
	}
}

namespace {	// function to implement Python ambient.intrinsic functions

	bool convertCharacterToCodePoint() {
		return CharacterToCodePointConversionCommand(activeViewer())() == 0;
	}

	bool convertCodePointToCharacter() {
		return CodePointToCharacterConversionCommand(activeViewer())() == 0;
	}

	void deleteBackwardCharacter(py::ssize_t n) {
		CharacterDeletionCommand(activeViewer(), Direction::BACKWARD).setNumericPrefix(static_cast<long>(n))();
	}

	void deleteCharacter(py::ssize_t n) {
		CharacterDeletionCommand(activeViewer(), Direction::FORWARD).setNumericPrefix(static_cast<long>(n))();
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

namespace {
	class Installer : virtual public ModuleInstaller {
	public:
		Installer() {
			ScriptEngine::instance().addModuleInstaller(*this);
		}
		void install() {
			{
				py::scope self(getNamedModule("intrinsic", "ambient.intrinsic"));

				py::def("convert_character_to_code_point", &convertCharacterToCodePoint);
				py::def("convert_code_point_to_character", &convertCodePointToCharacter);
				py::def("delete_backward_character", &deleteBackwardCharacter);
				py::def("delete_character", &deleteCharacter);
				py::def("paste", &paste);
				py::def("redo", &redo, py::arg("count") = 1);
				py::def("select_all", &selectAll);
				py::def("show_possible_completions", &showPossibleCompletions);
				py::def("undo", &undo, py::arg("count") = 1);
			}
		}
	} installer;
}

void intrinsic::showAboutDialogBox() {
}
