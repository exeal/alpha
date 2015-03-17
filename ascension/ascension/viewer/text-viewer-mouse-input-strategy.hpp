/**
 * @file text-viewer-mouse-input-strategy.hpp
 * @author exeal
 * @date 2011-03-30 separated from viewer.hpp
 * @date 2015-03-16 Renamed from viewer-observers.hpp
 */

#ifndef ASCENSION_TEXT_VIEWER_MOUSE_INPUT_STRATEGY_HPP
#define ASCENSION_TEXT_VIEWER_MOUSE_INPUT_STRATEGY_HPP
#include <ascension/viewer/mouse-input-strategy.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#	include <ascension/win32/windows.hpp>
#endif

namespace ascension {
	namespace viewer {
		class TextViewer;
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
		 * Interface of objects which define how the text editors react to the users' mouse input on the content area
		 * of @c TextViewer.
		 * @note An instance of @c TextViewerMouseInputStrategy can't be shared multiple text viewers.
		 * @see TextViewer#setMouseInputStrategy
		 */
		class TextViewerMouseInputStrategy : public MouseInputStrategy {
		public:
			/// Destructor.
			virtual ~TextViewerMouseInputStrategy() BOOST_NOEXCEPT {}
			/**
			 * Installs the strategy.
			 * @param viewer The text viewer uses the strategy. The window had been created at this time
			 */
			virtual void install(TextViewer& viewer) = 0;
			/// Uninstalls the strategy. The window is not destroyed yet at this time.
			virtual void uninstall() = 0;
		};

		namespace detail {
			class InputEventHandler {	// this is not an observer of caret...
			private:
				virtual void abortInput() = 0;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				virtual LRESULT handleInputEvent(UINT message, WPARAM wp, LPARAM lp, bool& consumed) = 0;
#endif
				friend class TextViewer;
			};
		}
	}
}

#endif // !ASCENSION_TEXT_VIEWER_MOUSE_INPUT_STRATEGY_HPP
