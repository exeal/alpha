/**
 * @file indentations.hpp
 * @author exeal
 * @date 2006-2011 was text-editor.hpp
 * @date 2011-05-06
 * @date 2011-2013, 2015
 */

#ifndef ASCENSION_INDENTATIONS_HPP
#define ASCENSION_INDENTATIONS_HPP
#include <ascension/text-editor/command.hpp>

namespace ascension {
	namespace texteditor {
		namespace commands {
			/**
			 * Makes/Deletes indents of the selected non-blank lines.
			 * @see viewer#Caret#spaceIndent, viewer#Caret#tabIndent
			 */
			class IndentationCommand : public Command {
			public:
				IndentationCommand(viewer::TextViewer& view, bool increase) BOOST_NOEXCEPT;
			private:
				bool perform();
				bool increases_;
			};
		}
	}
}

#endif // !ASCENSION_INDENTATIONS_HPP
