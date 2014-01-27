/**
 * @file caret-observers.hpp
 * @author exeal
 * @date 2011-03-30 separated from caret.hpp
 * @date 2011-2013
 */

#ifndef ASCENSION_CARET_OBSERVERS_HPP
#define ASCENSION_CARET_OBSERVERS_HPP
#include <ascension/ascension/corelib/basic-types.hpp>	// CodePoint
#include <utility>	// std.pair
#include <boost/optional.hpp>

namespace ascension {

	namespace kernel {
		class Position;
		class Region;
	}

	namespace viewers {

		class Caret;

		/**
		 * Interface for objects which are interested in change of input's properties of a
		 * @c TextViewer.
		 * @see CaretListener#overtypeModeChanged, TextViewer#addInputPropertyListener,
		 *      TextViewer#removeInputPropertyListener
		 */
		class InputPropertyListener {
		private:
			/**
			 * The text viewer's input locale had been changed (ex. @c WM_INPUTLANGCHANGE of
			 * Win32).
			 */
			virtual void inputLocaleChanged() BOOST_NOEXCEPT = 0;
			/// The text viewer's input method open status had been changed.
			virtual void inputMethodOpenStatusChanged() BOOST_NOEXCEPT = 0;
			friend class Caret;
		};
	}
}

#endif // !ASCENSION_CARET_OBSERVERS_HPP
