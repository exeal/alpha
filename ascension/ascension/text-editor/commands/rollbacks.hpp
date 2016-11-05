/**
 * @file rollbacks.hpp
 * @author exeal
 * @date 2006-2011 was text-editor.hpp
 * @date 2011-05-06
 * @date 2011-2013, 2015
 */

#ifndef ASCENSION_ROLLBACKS_HPP
#define ASCENSION_ROLLBACKS_HPP
#include <ascension/text-editor/command.hpp>

namespace ascension {
	namespace texteditor {
		namespace commands {
			/// Performs undo or redo.
			class UndoCommand : public Command {
			public:
				UndoCommand(viewer::TextViewer& view, bool redo) BOOST_NOEXCEPT;
				bool isLastActionIncompleted() const;
			private:
				bool perform();
				const bool redo_;
				enum {COMPLETED, INCOMPLETED, INDETERMINATE} lastResult_;
			};
		}
	}
}

#endif // !ASCENSION_ROLLBACKS_HPP
