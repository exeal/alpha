/**
 * @file intrinsic.cpp
 * Exposes intrinsic commands.
 */

#include "application.hpp"
#include "buffer.hpp"
#include "editor-view.hpp"
#include "editor-panes.hpp"
#include "function-pointer.hpp"
#include <ascension/graphics/font/text-layout.hpp>
#include <ascension/text-editor/command.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/text-viewer-utility.hpp>

namespace alpha {
#ifndef ALPHA_NO_AMBIENT
	namespace {	// helpers
		inline EditorView& activeViewer() {
			return EditorPanes::instance().activePane().selectedView();
		}

		inline EditorView& extractEditor(boost::python::object o) {
			if(o.is_none())
				return activeViewer();
			return boost::python::extract<EditorView&>(o);
		}
	}

	namespace {
		template<const ascension::Direction* direction>
		bool deleteCharacter(boost::python::object ed, boost::python::ssize_t n) {
			return ascension::texteditor::commands::CharacterDeletionCommand(
				extractEditor(ed), *direction).setNumericPrefix(
					static_cast<ascension::texteditor::Command::NumericPrefix>(n))();
		}

		template<const ascension::Direction* direction>
		bool deleteWord(boost::python::object ed, boost::python::ssize_t n) {
			return ascension::texteditor::commands::WordDeletionCommand(
				extractEditor(ed), *direction).setNumericPrefix(
					static_cast<ascension::texteditor::Command::NumericPrefix>(n))();
		}

		template<const ascension::Direction* direction>
		bool findNext(boost::python::object ed, boost::python::ssize_t n) {
			return ascension::texteditor::commands::FindNextCommand(
				extractEditor(ed), *direction).setNumericPrefix(
					static_cast<ascension::texteditor::Command::NumericPrefix>(n))();
		}

		template<bool fromPreviousLine>
		bool inputCharacterFromNextLine(boost::python::object ed) {
			return ascension::texteditor::commands::CharacterInputFromNextLineCommand(extractEditor(ed), fromPreviousLine)();
		}

		bool indent(boost::python::object ed, boost::python::ssize_t n);

		bool insertString(boost::python::object ed, const Glib::ustring& s, boost::python::ssize_t n) {
			try {
				return ascension::texteditor::commands::TextInputCommand(
					extractEditor(ed), ascension::fromGlibUstring(s)).setNumericPrefix(
						static_cast<ascension::texteditor::Command::NumericPrefix>(n))();
			} catch(ascension::kernel::DocumentCantChangeException&) {
				return false;	// TODO: Report the error to user.
			}
			return true;
		}

//		template<const Direction* direction>
//		bool isearch() {}

		template<typename Signature, Signature* procedure>
		void moveCaret(boost::python::object ed, bool extendSelection) {
			ascension::texteditor::commands::makeCaretMovementCommand(extractEditor(ed), procedure, extendSelection)();
		}

		template<typename Signature, Signature* procedure, const ascension::Direction* direction>
		void moveCaretN(boost::python::object ed, bool extendSelection, boost::python::ssize_t n) {
			ascension::texteditor::commands::makeCaretMovementCommand(
				extractEditor(ed), procedure, *direction, extendSelection).setNumericPrefix(
					static_cast<ascension::texteditor::Command::NumericPrefix>(n))();
		}

		template<const ascension::Direction* direction>
		bool newline(boost::python::object ed, boost::python::ssize_t n) {
			return ascension::texteditor::commands::NewlineCommand(
				extractEditor(ed), *direction).setNumericPrefix(
					static_cast<ascension::texteditor::Command::NumericPrefix>(n))();
		}

		template<bool(*procedure)(ascension::viewer::Caret&)>
		bool transpose(boost::python::object ed /*, boost::python::ssize_t n*/) {
			return ascension::texteditor::commands::TranspositionCommand(extractEditor(ed), procedure)();
		}

		void tryToBeginRectangleSelection(boost::python::object ed) {
			EditorView& viewer = extractEditor(ed);
			ascension::viewer::Caret& caret = viewer.caret();
			// the following code is copied from ascension/text-editor.cpp
			ascension::viewer::utils::closeCompletionProposalsPopup(viewer);
			if(ascension::texteditor::Session* const session = viewer.document().session()) {
				if(session->incrementalSearcher().isRunning())
					session->incrementalSearcher().end();
			}
			if(isSelectionEmpty(caret) && !caret.isSelectionRectangle())
				caret.beginRectangleSelection();
		}

		template<bool redo>
		bool undo(boost::python::object ed, boost::python::ssize_t n) {
			return ascension::texteditor::commands::UndoCommand(
				extractEditor(ed), redo).setNumericPrefix(
					static_cast<ascension::texteditor::Command::NumericPrefix>(n))();
		}
	} // namespace @0

	ALPHA_EXPOSE_PROLOGUE(10)
		boost::python::scope temp(ambient::Interpreter::instance().module("intrinsics"));

		boost::python::def("backward_bookmark",
			&moveCaretN<
				boost::optional<ascension::kernel::Position>(const ascension::kernel::Point&, ascension::Direction, ascension::Index),
				&ascension::kernel::locations::nextBookmark,
				&ascension::Direction::BACKWARD
			>, (boost::python::arg("ed") = boost::python::object(), boost::python::arg("extend_selection") = false, boost::python::arg("n") = 1));

		boost::python::def("backward_character",
			&moveCaretN<
				ascension::kernel::Position(const ascension::kernel::Point&, ascension::Direction, ascension::kernel::locations::CharacterUnit, ascension::Index),
				&ascension::kernel::locations::nextCharacter,
				&ascension::Direction::BACKWARD
			>, (boost::python::arg("ed") = boost::python::object(), boost::python::arg("extend_selection") = false, boost::python::arg("n") = 1));

		boost::python::def("backward_line",
			&moveCaretN<
				ascension::kernel::Position(const ascension::kernel::Point&, ascension::Direction, ascension::Index),
				&ascension::kernel::locations::nextLine,
				&ascension::Direction::BACKWARD
			>, (boost::python::arg("ed") = boost::python::object(), boost::python::arg("extend_selection") = false, boost::python::arg("n") = 1));

		boost::python::def("backward_page",
			&moveCaretN<
				ascension::viewer::VisualDestinationProxy(const ascension::viewer::VisualPoint&, ascension::Direction, ascension::Index),
				&ascension::kernel::locations::nextPage,
				&ascension::Direction::BACKWARD
			>, (boost::python::arg("ed") = boost::python::object(), boost::python::arg("extend_selection") = false, boost::python::arg("n") = 1));

		boost::python::def("backward_visual_line",
			&moveCaretN<
				ascension::viewer::VisualDestinationProxy(const ascension::viewer::VisualPoint&, ascension::Direction, ascension::Index),
				&ascension::kernel::locations::nextVisualLine,
				&ascension::Direction::BACKWARD
			>, (boost::python::arg("ed") = boost::python::object(), boost::python::arg("extend_selection") = false, boost::python::arg("n") = 1));

		boost::python::def("backward_word",
			&moveCaretN<
				ascension::kernel::Position(const ascension::kernel::Point&, ascension::Direction, ascension::Index),
				&ascension::kernel::locations::nextWord,
				&ascension::Direction::BACKWARD
			>, (boost::python::arg("ed") = boost::python::object(), boost::python::arg("extend_selection") = false, boost::python::arg("n") = 1));

		boost::python::def("backward_word_end",
			&moveCaretN<
				ascension::kernel::Position(const ascension::kernel::Point&, ascension::Direction, ascension::Index),
				&ascension::kernel::locations::nextWordEnd,
				&ascension::Direction::BACKWARD
			>, (boost::python::arg("ed") = boost::python::object(), boost::python::arg("extend_selection") = false, boost::python::arg("n") = 1));

		boost::python::def("beginning_of_buffer",
			&moveCaret<
				ascension::kernel::Position(const ascension::kernel::Point&),
				&ascension::kernel::locations::beginningOfDocument
			>, (boost::python::arg("ed") = boost::python::object(), boost::python::arg("extend_selection") = false));

		boost::python::def("beginning_of_line",
			&moveCaret<
				ascension::kernel::Position(const ascension::kernel::Point&),
				&ascension::kernel::locations::beginningOfLine
			>, (boost::python::arg("ed") = boost::python::object(), boost::python::arg("extend_selection") = false));

		boost::python::def("beginning_of_visual_line",
			&moveCaret<
				ascension::kernel::Position(const ascension::viewer::VisualPoint&),
				&ascension::kernel::locations::beginningOfVisualLine
			>, (boost::python::arg("ed") = boost::python::object(), boost::python::arg("extend_selection") = false));

		boost::python::def("bookmark_match_lines",
			ambient::makeFunctionPointer([](boost::python::object ed, bool onlySelection) -> boost::python::ssize_t {
				EditorView& editor = extractEditor(ed);
				ascension::texteditor::commands::BookmarkMatchLinesCommand temp(editor, onlySelection ? editor.caret().selectedRegion() : ascension::kernel::Region());
				temp();
				return temp.numberOfMarkedLines();
			}),
			(boost::python::arg("ed") = boost::python::object(), boost::python::arg("only_selection") = false));

		boost::python::def("cancel",
			ambient::makeFunctionPointer([](boost::python::object ed) {
				ascension::texteditor::commands::CancelCommand(extractEditor(ed))();
			}),
			boost::python::arg("ed") = boost::python::object());

//		boost::python::def("clear_all_bookmarks", &);

		boost::python::def("contextual_beginning_of_line",
			&moveCaret<
				ascension::kernel::Position(const ascension::viewer::VisualPoint&),
				&ascension::kernel::locations::contextualBeginningOfLine
			>, (boost::python::arg("ed") = boost::python::object(), boost::python::arg("extend_selection") = false));

		boost::python::def("contextual_beginning_of_visual_line",
			&moveCaret<
				ascension::kernel::Position(const ascension::viewer::VisualPoint&),
				&ascension::kernel::locations::contextualBeginningOfVisualLine
			>, (boost::python::arg("ed") = boost::python::object(), boost::python::arg("extend_selection") = false));

		boost::python::def("contextual_end_of_line",
			&moveCaret<
				ascension::kernel::Position(const ascension::viewer::VisualPoint&),
				&ascension::kernel::locations::contextualEndOfLine
			>, (boost::python::arg("ed") = boost::python::object(), boost::python::arg("extend_selection") = false));

		boost::python::def("contextual_end_of_visual_line",
			&moveCaret<
				ascension::kernel::Position(const ascension::viewer::VisualPoint&),
				&ascension::kernel::locations::contextualEndOfVisualLine
			>, (boost::python::arg("ed") = boost::python::object(), boost::python::arg("extend_selection") = false));

		boost::python::def("convert_character_to_code_point",
			ambient::makeFunctionPointer([](boost::python::object ed) -> bool {
				return ascension::texteditor::commands::CharacterToCodePointConversionCommand(extractEditor(ed))();
			}),
			boost::python::arg("ed") = boost::python::object());

		boost::python::def("convert_code_point_to_character",
			ambient::makeFunctionPointer([](boost::python::object ed) -> bool {
				return ascension::texteditor::commands::CodePointToCharacterConversionCommand(extractEditor(ed))();
			}),
			boost::python::arg("ed") = boost::python::object());

		boost::python::def("copy_selection",
			ambient::makeFunctionPointer([](boost::python::object ed, bool useKillRing) -> bool {
				try {
					ascension::viewer::copySelection(extractEditor(ed).caret(), useKillRing);
				} catch(const ascension::viewer::ClipboardException&) {
					return false;	// TODO: Report the error to user.
				}
				return true;
			}),
			(boost::python::arg("ed") = boost::python::object(), boost::python::arg("use_kill_ring") = true));

		boost::python::def("cut_selection",
			ambient::makeFunctionPointer([](boost::python::object ed, bool useKillRing) -> bool {
				try {
					ascension::viewer::cutSelection(extractEditor(ed).caret(), useKillRing);
				} catch(const ascension::viewer::ClipboardException&) {
					return false;	// TODO: Report the error to user.
				}
				return true;
			}),
			(boost::python::arg("ed") = boost::python::object(), boost::python::arg("use_kill_ring") = true));

		boost::python::def("delete_backward_character",
			&deleteCharacter<&ascension::Direction::BACKWARD>,
			(boost::python::arg("ed") = boost::python::object(), boost::python::arg("n") = 1));

		boost::python::def("delete_backward_word",
			&deleteWord<&ascension::Direction::BACKWARD>,
			(boost::python::arg("ed") = boost::python::object(), boost::python::arg("n") = 1));

		boost::python::def("delete_forward_character",
			&deleteCharacter<&ascension::Direction::FORWARD>,
			(boost::python::arg("ed") = boost::python::object(), boost::python::arg("n") = 1));

		boost::python::def("delete_forward_word",
			&deleteWord<&ascension::Direction::FORWARD>,
			(boost::python::arg("ed") = boost::python::object(), boost::python::arg("n") = 1));

//		boost::python::def("delete_line", &);

		boost::python::def("end_of_buffer",
			&moveCaret<
				ascension::kernel::Position(const ascension::kernel::Point&),
				&ascension::kernel::locations::endOfDocument
			>, (boost::python::arg("ed") = boost::python::object(), boost::python::arg("extend_selection") = false));

		boost::python::def("end_of_line",
			&moveCaret<
				ascension::kernel::Position(const ascension::kernel::Point&),
				&ascension::kernel::locations::endOfLine
			>, (boost::python::arg("ed") = boost::python::object(), boost::python::arg("extend_selection") = false));

		boost::python::def("end_of_visual_line",
			&moveCaret<
				ascension::kernel::Position(const ascension::viewer::VisualPoint&),
				&ascension::kernel::locations::endOfVisualLine
			>, (boost::python::arg("ed") = boost::python::object(), boost::python::arg("extend_selection") = false));

		boost::python::def("find_next",
			&findNext<&ascension::Direction::FORWARD>,
			(boost::python::arg("ed") = boost::python::object(), boost::python::arg("n") = 1));

		boost::python::def("find_previous",
			&findNext<&ascension::Direction::BACKWARD>,
			(boost::python::arg("ed") = boost::python::object(), boost::python::arg("n") = 1));

		boost::python::def("first_printable_character_of_line",
			&moveCaret<
				ascension::kernel::Position(const ascension::viewer::VisualPoint&),
				&ascension::kernel::locations::firstPrintableCharacterOfLine
			>, (boost::python::arg("ed") = boost::python::object(), boost::python::arg("extend_selection") = false));

		boost::python::def("first_printable_character_of_visual_line",
			&moveCaret<
				ascension::kernel::Position(const ascension::viewer::VisualPoint&),
				&ascension::kernel::locations::firstPrintableCharacterOfVisualLine
			>, (boost::python::arg("ed") = boost::python::object(), boost::python::arg("extend_selection") = false));

		boost::python::def("forward_bookmark",
			&moveCaretN<
				boost::optional<ascension::kernel::Position>(const ascension::kernel::Point&, ascension::Direction, ascension::Index),
				&ascension::kernel::locations::nextBookmark,
				&ascension::Direction::FORWARD
			>, (boost::python::arg("ed") = boost::python::object(), boost::python::arg("extend_selection") = false, boost::python::arg("n") = 1));

		boost::python::def("forward_character",
			&moveCaretN<
				ascension::kernel::Position(const ascension::kernel::Point&, ascension::Direction, ascension::kernel::locations::CharacterUnit, ascension::Index),
				&ascension::kernel::locations::nextCharacter,
				&ascension::Direction::FORWARD
			>, (boost::python::arg("ed") = boost::python::object(), boost::python::arg("extend_selection") = false, boost::python::arg("n") = 1));

		boost::python::def("forward_line",
			&moveCaretN<
				ascension::kernel::Position(const ascension::kernel::Point&, ascension::Direction, ascension::Index),
				&ascension::kernel::locations::nextLine,
				&ascension::Direction::FORWARD
			>, (boost::python::arg("ed") = boost::python::object(), boost::python::arg("extend_selection") = false, boost::python::arg("n") = 1));

		boost::python::def("forward_page",
			&moveCaretN<
				ascension::viewer::VisualDestinationProxy(const ascension::viewer::VisualPoint&, ascension::Direction, ascension::Index),
				&ascension::kernel::locations::nextPage,
				&ascension::Direction::FORWARD
			>, (boost::python::arg("ed") = boost::python::object(), boost::python::arg("extend_selection") = false, boost::python::arg("n") = 1));

		boost::python::def("forward_visual_line",
			&moveCaretN<
				ascension::viewer::VisualDestinationProxy(const ascension::viewer::VisualPoint&, ascension::Direction, ascension::Index),
				&ascension::kernel::locations::nextVisualLine,
				&ascension::Direction::FORWARD
			>, (boost::python::arg("ed") = boost::python::object(), boost::python::arg("extend_selection") = false, boost::python::arg("n") = 1));

		boost::python::def("forward_word",
			&moveCaretN<
				ascension::kernel::Position(const ascension::kernel::Point&, ascension::Direction, ascension::Index),
				&ascension::kernel::locations::nextWord,
				&ascension::Direction::FORWARD
			>, (boost::python::arg("ed") = boost::python::object(), boost::python::arg("extend_selection") = false, boost::python::arg("n") = 1));

		boost::python::def("forward_word_end",
			&moveCaretN<
				ascension::kernel::Position(const ascension::kernel::Point&, ascension::Direction, ascension::Index),
				&ascension::kernel::locations::nextWordEnd,
				&ascension::Direction::FORWARD
			>, (boost::python::arg("ed") = boost::python::object(), boost::python::arg("extend_selection") = false, boost::python::arg("n") = 1));

		boost::python::def("input_character",
			ambient::makeFunctionPointer([](boost::python::object ed, int character) -> bool {
				return ascension::texteditor::commands::CharacterInputCommand(extractEditor(ed), character)();
			}),
			(boost::python::arg("ed") = boost::python::object(), boost::python::arg("character")));

		boost::python::def("input_character_from_next_line",
			&inputCharacterFromNextLine<false>, boost::python::arg("ed") = boost::python::object());

		boost::python::def("input_character_from_previous_line",
			&inputCharacterFromNextLine<true>, boost::python::arg("ed") = boost::python::object());

//		boost::python::def("indent", &indent);

		boost::python::def("insert_previous_line", &newline<&ascension::Direction::BACKWARD>,
			(boost::python::arg("ed") = boost::python::object(), boost::python::arg("n") = 1));

		boost::python::def("insert_string", &insertString,
			(boost::python::arg("ed") = boost::python::object(), boost::python::arg("string"), boost::python::arg("n") = 1));

//		boost::python::def("isearch_backward", &);

//		boost::python::def("isearch_forward", &);

		boost::python::def("last_printable_character_of_line",
			&moveCaret<
				ascension::kernel::Position(const ascension::viewer::VisualPoint&),
				&ascension::kernel::locations::lastPrintableCharacterOfLine
			>, (boost::python::arg("ed") = boost::python::object(), boost::python::arg("extend_selection") = false));

		boost::python::def("last_printable_character_of_visual_line",
			&moveCaret<
				ascension::kernel::Position(const ascension::viewer::VisualPoint&),
				&ascension::kernel::locations::lastPrintableCharacterOfVisualLine
			>, (boost::python::arg("ed") = boost::python::object(), boost::python::arg("extend_selection") = false));
/*
		boost::python::def("left_character",
			&moveCaretN<
				ascension::kernel::Position(const ascension::viewer::VisualPoint&, ascension::kernel::locations::CharacterUnit, ascension::Index),
				&ascension::kernel::locations::leftCharacter
			>, (boost::python::arg("ed") = boost::python::object(), boost::python::arg("extend_selection") = false, boost::python::arg("n") = 1));

		boost::python::def("left_word",
			&moveCaretN<
				ascension::kernel::Position(const ascension::viewer::VisualPoint&, ascension::Index),
				&ascension::kernel::locations::leftWord
			>, (boost::python::arg("ed") = boost::python::object(), boost::python::arg("extend_selection") = false, boost::python::arg("n") = 1));

		boost::python::def("left_word_end",
			&moveCaretN<
				ascension::kernel::Position(const ascension::viewer::VisualPoint&, ascension::Index),
				&ascension::kernel::locations::leftWordEnd
			>, (boost::python::arg("ed") = boost::python::object(), boost::python::arg("extend_selection") = false, boost::python::arg("n") = 1));
*/
		boost::python::def("matching_paren",
			ambient::makeFunctionPointer([](boost::python::object ed, bool extendSelection) -> bool {
				return ascension::texteditor::commands::MatchBracketCommand(extractEditor(ed), extendSelection)();
			}),
			(boost::python::arg("ed") = boost::python::object(), boost::python::arg("extend_selection") = false));

		boost::python::def("newline",
			&newline<&ascension::Direction::FORWARD>, (boost::python::arg("ed") = boost::python::object(), boost::python::arg("n") = 1));

		boost::python::def("paste",
			ambient::makeFunctionPointer([](boost::python::object ed, bool useKillRing) -> bool {
				return ascension::texteditor::commands::PasteCommand(extractEditor(ed), useKillRing)();
			}),
			(boost::python::arg("ed") = boost::python::object(), boost::python::arg("use_killring") = true));

		boost::python::def("reconvert",
			ambient::makeFunctionPointer([](boost::python::object ed) -> bool {
				return ascension::texteditor::commands::ReconversionCommand(extractEditor(ed))();
			}),
			boost::python::arg("ed") = boost::python::object());

		boost::python::def("redo", &undo<true>, (boost::python::arg("ed") = boost::python::object(), boost::python::arg("n") = 1));

//		boost::python::def("replace_all", &);
/*
		boost::python::def("right_character",
			&moveCaretN<
				ascension::kernel::Position(const ascension::viewer::VisualPoint&, ascension::kernel::locations::CharacterUnit, ascension::Index),
				&ascension::kernel::locations::rightCharacter
			>, (boost::python::arg("ed") = boost::python::object(), boost::python::arg("extend_selection") = false, boost::python::arg("n") = 1));

		boost::python::def("right_word",
			&moveCaretN<
				ascension::kernel::Position(const ascension::viewer::VisualPoint&, ascension::Index),
				&ascension::kernel::locations::rightWord
			>, (boost::python::arg("ed") = boost::python::object(), boost::python::arg("extend_selection") = false, boost::python::arg("n") = 1));

		boost::python::def("right_word_end",
			&moveCaretN<
				ascension::kernel::Position(const ascension::viewer::VisualPoint&, ascension::Index),
				&ascension::kernel::locations::rightWordEnd
			>, (boost::python::arg("ed") = boost::python::object(), boost::python::arg("extend_selection") = false, boost::python::arg("n") = 1));
*/
		boost::python::def("select_all",
			ambient::makeFunctionPointer([](boost::python::object ed) {
				ascension::texteditor::commands::EntireDocumentSelectionCreationCommand(extractEditor(ed))();
			}),
			boost::python::arg("ed") = boost::python::object());

		boost::python::def("select_word",
			ambient::makeFunctionPointer([](boost::python::object ed) {
				ascension::texteditor::commands::WordSelectionCreationCommand(extractEditor(ed))();
			}),
			boost::python::arg("ed") = boost::python::object());

		boost::python::def("show_completion_proposals_popup",
			ambient::makeFunctionPointer([](boost::python::object ed) -> bool {
				return ascension::texteditor::commands::CompletionProposalPopupCommand(extractEditor(ed))();
			}),
			boost::python::arg("ed") = boost::python::object());

//		boost::python::def("toggle_bookmark", &);

		boost::python::def("toggle_ime_status",
			ambient::makeFunctionPointer([](boost::python::object ed) -> bool {
				return ascension::texteditor::commands::InputMethodOpenStatusToggleCommand(extractEditor(ed))();
			}),
			boost::python::arg("ed") = boost::python::object());

		boost::python::def("toggle_overtype_mode",
			ambient::makeFunctionPointer([](boost::python::object ed) -> bool {
				return ascension::texteditor::commands::OvertypeModeToggleCommand(extractEditor(ed))();
			}),
			boost::python::arg("ed") = boost::python::object());

		boost::python::def("toggle_soft_keyboard_mode",
			ambient::makeFunctionPointer([](boost::python::object ed) -> bool {
				return ascension::texteditor::commands::InputMethodSoftKeyboardModeToggleCommand(extractEditor(ed))();
			}),
			boost::python::arg("ed") = boost::python::object());

		boost::python::def("transpose_characters",
			&transpose<&ascension::viewer::transposeCharacters>/*,
			(boost::python::arg("ed") = boost::python::object(), boost::python::arg("n") = 1)*/);

		boost::python::def("transpose_lines",
			&transpose<&ascension::viewer::transposeLines>/*,
			(boost::python::arg("ed") = boost::python::object(), boost::python::arg("n") = 1)*/);

		boost::python::def("transpose_words",
			&transpose<&ascension::viewer::transposeWords>/*,
			(boost::python::arg("ed") = boost::python::object(), boost::python::arg("n") = 1)*/);

		boost::python::def("try_to_begin_rectangle_selection",
			&tryToBeginRectangleSelection, boost::python::arg("ed") = boost::python::object());

		boost::python::def("undo", &undo<false>,
			(boost::python::arg("ed") = boost::python::object(), boost::python::arg("n") = 1));

//		boost::python::def("unindent", &unindent);
	ALPHA_EXPOSE_EPILOGUE()
#endif // !ALPHA_NO_AMBIENT
}
