/**
 * @file input-method.hpp
 * @author exeal
 * @date 2015-08-23 Created.
 */

#ifndef ASCENSION_INPUT_METHOD_HPP
#define ASCENSION_INPUT_METHOD_HPP
#include <ascension/corelib/string-piece.hpp>

namespace ascension {
	namespace viewer {
		class TextViewer;

		namespace detail {
			class InputMethodEvent {
				virtual void commitInputString(const StringPiece& text) = 0;
				virtual void preeditChanged() = 0;
				virtual void preeditEnded() = 0;
				virtual void preeditStarted() = 0;
				friend class TextViewer;
			};

			class InputMethodQueryEvent {
				virtual std::pair<const StringPiece, StringPiece::const_iterator> querySurroundingText() const = 0;
				friend class TextViewer;
			};

			void resetInputMethod(TextViewer& textViewer);
		}
	}
}

#endif // !ASCENSION_INPUT_METHOD_HPP
