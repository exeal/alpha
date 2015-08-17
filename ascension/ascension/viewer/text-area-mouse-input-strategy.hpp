/**
 * @file text-area-mouse-input-strategy.hpp
 * @author exeal
 * @date 2011-03-30 separated from viewer.hpp
 * @date 2015-03-16 Renamed from viewer-observers.hpp
 * @date 2015-04-12 Renamed from text-viewer-mouse-input-strategy.hpp
 */

#ifndef ASCENSION_TEXT_AREA_MOUSE_INPUT_STRATEGY_HPP
#define ASCENSION_TEXT_AREA_MOUSE_INPUT_STRATEGY_HPP
#include <ascension/viewer/mouse-input-strategy.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#	include <ascension/win32/windows.hpp>
#endif

namespace ascension {
	namespace viewer {
		class TextArea;
#if 0
		/**
		 * Interface of objects which define how the text editors react to the users' keyboard
		 * input. Ascension also provides the standard implementation of this interface
		 * @c DefaultKeyboardInputStrategy.
		 * @see TextViewer#set
		 */
		class KeyboardInputStrategy {
		};
#endif

		/**
		 * Interface of objects which define how the text editors react to the users' mouse input on the @c TextArea.
		 * @note An instance of @c TextAreaMouseInputStrategy can't be shared multiple text areas.
		 * @see TextArea#setMouseInputStrategy
		 */
		class TextAreaMouseInputStrategy : virtual public MouseInputStrategy {
		public:
			/// Destructor.
			virtual ~TextAreaMouseInputStrategy() BOOST_NOEXCEPT {}
			/**
			 * Installs the strategy.
			 * @param viewer The text area uses the strategy. The window had been created at this time
			 */
			virtual void install(TextArea& viewer) = 0;
			/// Uninstalls the strategy. The window is not destroyed yet at this time.
			virtual void uninstall() = 0;
		};
	}
}

#endif // !ASCENSION_TEXT_AREA_MOUSE_INPUT_STRATEGY_HPP
