/**
 * @file inputs.hpp
 * @author exeal
 * @date 2006-2011 was text-editor.hpp
 * @date 2011-05-06
 * @date 2011-2013, 2015
 */

#ifndef ASCENSION_INPUTS_HPP
#define ASCENSION_INPUTS_HPP
#include <ascension/direction.hpp>
#include <ascension/corelib/text/code-point.hpp>
#include <ascension/text-editor/command.hpp>

namespace ascension {
	namespace viewer {
		class BlockProgressionDestinationProxy;
		class Caret;
		class TextViewer;
	}

	namespace texteditor {
		namespace commands {
			/**
			 * Inputs a character on the caret position, or appends to the end of the active
			 * incremental search pattern.
			 * @see viewer#Caret#inputCharacter
			 */
			class CharacterInputCommand : public Command {
			public:
				CharacterInputCommand(viewer::TextViewer& viewer, CodePoint c);
			private:
				bool perform();
				const CodePoint c_;
			};

			/// Inputs a character is at same position in the next/previous visual line.
			class CharacterInputFromNextLineCommand : public Command {
			public:
				CharacterInputFromNextLineCommand(viewer::TextViewer& viewer, bool fromPreviousLine) BOOST_NOEXCEPT;
			private:
				bool perform();
				const bool fromPreviousLine_;
			};

			/**
			 * Inserts a newline, or exits a mode.
			 * If the incremental search is running, exits the search. If the content assist is
			 * active, completes or aborts and breaks the line if no candidate matches exactly.
			 */
			class NewlineCommand : public Command {
			public:
				NewlineCommand(viewer::TextViewer& view, boost::optional<Direction> direction = boost::none) BOOST_NOEXCEPT;
			private:
				bool perform();
				const boost::optional<Direction> direction_;
			};

			/// Inputs a text.
			class TextInputCommand : public Command {
			public:
				TextInputCommand(viewer::TextViewer& view, const String& text) BOOST_NOEXCEPT;
			private:
				bool perform();
				String text_;
			};
		}
	}
}

#endif // !ASCENSION_INPUTS_HPP
