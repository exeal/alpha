/**
 * @file caret-motions.hpp
 * @author exeal
 * @date 2006-2011 was text-editor.hpp
 * @date 2011-05-06
 * @date 2011-2013, 2015
 * @date 2016-11-04 Separated from command.hpp.
 */

#ifndef ASCENSION_CARET_MOTIONS_HPP
#define ASCENSION_CARET_MOTIONS_HPP
#include <ascension/direction.hpp>
#include <ascension/text-editor/command.hpp>

namespace ascension {
	namespace texteditor {
		namespace commands {
			/**
			 * Moves the caret or extends the selection.
			 * @c kernel#locations#CharacterUnit#GRAPHEME_CLUSTER is always used as character unit.
			 * @tparam ProcedureSignature Type of the function gives a motion
			 * @see CaretMovementToDefinedPositionCommand, viewer#Caret, kernel#locations
			 */
			template<typename ProcedureSignature>
			class CaretMovementCommand : public Command {
			public:
				CaretMovementCommand(viewer::TextViewer& viewer,
					ProcedureSignature* procedure, Direction direction, bool extendSelection = false);
			private:
				bool perform();
				ProcedureSignature* const procedure_;
				const Direction direction_;
				const bool extends_;
			};

			/**
			 * Moves the caret or extends the selection to a defined position.
			 * @tparam ProcedureSignature Type of the function gives a motion
			 * @see CaretMovementCommand, viewer#Caret, kernel#locations
			 */
			template<typename ProcedureSignature>
			class CaretMovementToDefinedPositionCommand : public Command {
			public:
				CaretMovementToDefinedPositionCommand(viewer::TextViewer& viewer,
					ProcedureSignature* procedure, bool extendSelection = false);
			private:
				bool perform();
				ProcedureSignature* const procedure_;
				const bool extends_;
			};

			/// Selects the entire document.
			class EntireDocumentSelectionCreationCommand : public Command {
			public:
				explicit EntireDocumentSelectionCreationCommand(viewer::TextViewer& view) BOOST_NOEXCEPT;
			private:
				bool perform();
			};

			/**
			 * Extends the selection and begins rectangular selection.
			 * @tparam ProcedureSignature Type of the function gives a motion
			 * @see RowSelectionExtensionToDefinedPositionCommand, viewer#Caret, kernel#locations
			 */
			template<typename ProcedureSignature>
			class RowSelectionExtensionCommand : public Command {
			public:
				RowSelectionExtensionCommand(viewer::TextViewer& viewer,
					ProcedureSignature* procedure, Direction direction);
			private:
				bool perform();
				ProcedureSignature* const procedure_;
				const Direction direction_;
			};
			/**
			 * Extends the selection to a defined position and begins rectangular selection.
			 * @tparam ProcedureSignature Type of the function gives a motion
			 * @see RowSelectionExtensionCommand, viewer#Caret, kernel#locations
			 */
			template<typename ProcedureSignature>
			class RowSelectionExtensionToDefinedPositionCommand : public Command {
			public:
				RowSelectionExtensionToDefinedPositionCommand(
					viewer::TextViewer& viewer, ProcedureSignature* procedure);
			private:
				bool perform();
				ProcedureSignature* const procedure_;
			};

			/// Selects the current word.
			class WordSelectionCreationCommand : public Command {
			public:
				explicit WordSelectionCreationCommand(viewer::TextViewer& view) BOOST_NOEXCEPT;
			private:
				bool perform();
			};

			template<typename ProcedureSignature>
			inline CaretMovementCommand<ProcedureSignature> makeCaretMovementCommand(
					viewer::TextViewer& viewer, ProcedureSignature* procedure,
					Direction direction, bool extendSelection = false) {
				return CaretMovementCommand<ProcedureSignature>(
					viewer, procedure, direction, extendSelection);
			}
			template<typename ProcedureSignature>
			inline CaretMovementToDefinedPositionCommand<ProcedureSignature> makeCaretMovementCommand(
					viewer::TextViewer& viewer, ProcedureSignature* procedure,
					bool extendSelection = false) {
				return CaretMovementToDefinedPositionCommand<ProcedureSignature>(
					viewer, procedure, extendSelection);
			}
			template<typename ProcedureSignature>
			inline RowSelectionExtensionCommand<ProcedureSignature> makeRowSelectionExtensionCommand(
					viewer::TextViewer& viewer, ProcedureSignature* procedure, Direction direction) {
				return RowSelectionExtensionCommand<ProcedureSignature>(viewer, procedure, direction);
			}
			template<typename ProcedureSignature>
			inline RowSelectionExtensionToDefinedPositionCommand<ProcedureSignature> makeRowSelectionExtensionCommand(
					viewer::TextViewer& viewer, ProcedureSignature* procedure) {
				return RowSelectionExtensionToDefinedPositionCommand<ProcedureSignature>(viewer, procedure);
			}
		}
	}
}

#endif // !ASCENSION_CARET_MOTIONS_HPP
