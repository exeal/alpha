/**
 * @file transpositions.hpp
 * @author exeal
 * @date 2006-2011 was text-editor.hpp
 * @date 2011-05-06
 * @date 2011-2013, 2015
 * @date 2016-11-05 Separated from command.hpp.
 */

#ifndef ASCENSION_TRANSPOSITIONS_HPP
#define ASCENSION_TRANSPOSITIONS_HPP
#include <ascension/text-editor/command.hpp>

namespace ascension {
	namespace viewer {
		class Caret;
	}

	namespace texteditor {
		namespace commands {
			/// Transposes (swaps) the two text elements.
			class TranspositionCommand : public Command {
			public:
				TranspositionCommand(viewer::TextViewer& view, bool(*procedure)(viewer::Caret&));
			private:
				bool perform();
				bool(*procedure_)(viewer::Caret&);
			};
		}
	}
}

#endif // !ASCENSION_TRANSPOSITIONS_HPP
