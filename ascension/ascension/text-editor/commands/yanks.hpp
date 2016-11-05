/**
 * @file yanks.hpp
 * @author exeal
 * @date 2006-2011 was text-editor.hpp
 * @date 2011-05-06
 * @date 2011-2013, 2015
 * @date 2016-11-04 Separated from command.hpp.
 */

#ifndef ASCENSION_YANKS_HPP
#define ASCENSION_YANKS_HPP
#include <ascension/text-editor/command.hpp>

namespace ascension {
	namespace texteditor {
		namespace commands {
			/// Inserts the content of the kill ring or the clipboard at the caret position.
			class PasteCommand : public Command {
			public:
				PasteCommand(viewer::TextViewer& view, bool useKillRing) BOOST_NOEXCEPT;
				bool perform();
			private:
				const bool usesKillRing_;
			};
		}
	}
}

#endif // !ASCENSION_YANKS_HPP
