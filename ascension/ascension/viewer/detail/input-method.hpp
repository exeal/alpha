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

		namespace widgetapi {
			namespace event {
				class InputMethodEvent;
				class InputMethodQueryEvent;
			}
		}

		namespace detail {
			class InputMethodEventHandler {
				virtual void commitString(widgetapi::event::InputMethodEvent& event) BOOST_NOEXCEPT = 0;
				virtual void preeditChanged(widgetapi::event::InputMethodEvent& event) BOOST_NOEXCEPT = 0;
				virtual void preeditEnded() BOOST_NOEXCEPT = 0;
				virtual void preeditStarted() BOOST_NOEXCEPT = 0;
				friend class TextViewer;
			};

			class InputMethodQueryEventHandler {
				virtual std::pair<const StringPiece, StringPiece::const_iterator> querySurroundingText() const BOOST_NOEXCEPT = 0;
				friend class TextViewer;
			};

			void resetInputMethod(TextViewer& textViewer);
		}
	}
}

#endif // !ASCENSION_INPUT_METHOD_HPP
