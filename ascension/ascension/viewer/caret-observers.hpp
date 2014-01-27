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
		 * Interface for objects which are interested in getting informed about changes of a caret.
		 * @see IPointListener, Caret#addStateListener, Caret#removeStateListener
		 */
		class CaretStateListener {
		private:
			/**
			 * The matched brackets were changed.
			 * @param caret The caret
			 * @param oldPair The pair of the brackets previously matched. May be @c boost#none
			 * @param outsideOfView The brackets newly matched are outside of the view
			 */
			virtual void matchBracketsChanged(const Caret& caret,
				const boost::optional<std::pair<kernel::Position, kernel::Position>>& oldPair, bool outsideOfView) = 0;
			/// The overtype mode of the caret is changed.
			virtual void overtypeModeChanged(const Caret& caret) = 0;
			/// The shape (linear or rectangle) of the selection is changed.
			virtual void selectionShapeChanged(const Caret& caret) = 0;
			friend class Caret;
		};

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
