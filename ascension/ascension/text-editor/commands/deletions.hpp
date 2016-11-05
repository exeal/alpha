/**
 * @file deletions.hpp
 * @author exeal
 * @date 2006-2011 was text-editor.hpp
 * @date 2011-05-06
 * @date 2011-2013, 2015
 * @date 2016-11-04 Separated from command.hpp.
 */

#ifndef ASCENSION_DELETIONS_HPP
#define ASCENSION_DELETIONS_HPP
#include <ascension/direction.hpp>
#include <ascension/text-editor/command.hpp>

namespace ascension {
	namespace texteditor {
		namespace commands {
			/**
			 * Deletes the forward/backward N character(s). If the incremental search is active,
			 * deletes the entire pattern (direction is @c Direction#forward()) or the last N character(s)
			 * (direction is @c Direction#backward()).
			 * @see WordDeletionCommand
			 */
			class CharacterDeletionCommand : public Command {
			public:
				CharacterDeletionCommand(viewer::TextViewer& viewer, Direction direction) BOOST_NOEXCEPT;
			private:
				bool perform();
				const Direction direction_;
			};

			/// Deletes the forward/backward N word(s).
			class WordDeletionCommand : public Command {
			public:
				WordDeletionCommand(viewer::TextViewer& viewer, Direction direction) BOOST_NOEXCEPT;
			private:
				bool perform();
				const Direction direction_;
			};
		}
	}
}

#endif // !ASCENSION_DELETIONS_HPP
