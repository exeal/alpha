/**
 * @file viewer-observers.hpp
 * @author exeal
 * @date 2011-03-30 separated from viewer.hpp
 */

#ifndef VIEWER_OBSERVERS_HPP
#define VIEWER_OBSERVERS_HPP

namespace ascension {
	namespace viewers {

		class TextViewer;

		/**
		 * Interface for objects which are interested in change of scroll positions of a
		 * @c TextViewer.
		 * @see TextViewer#addViewportListener, TextViewer#removeViewportListener
		 */
		class ViewportListener {
		private:
			/**
			 * The scroll positions of the viewer were changed.
			 * @param horizontal @c true if the horizontal scroll position is changed
			 * @param vertical @c true if the vertical scroll position is changed
			 * @see TextViewer#firstVisibleLine
			 */
			virtual void viewportChanged(bool horizontal, bool vertical) = 0;
			friend class TextViewer;
		};
		
		/**
		 * Interface for objects which are interested in change of size of a @c TextViewer.
		 * @see TextViewer#addDisplaySizeListener, TextViewer#removeDisplaySizeListener
		 */
		class DisplaySizeListener {
		private:
			/// The size of the viewer was changed.
			virtual void viewerDisplaySizeChanged() = 0;
			friend class TextViewer;
		};

	}

	namespace detail {
		class InputEventHandler {	// this is not an observer of caret...
		private:
			virtual void abortInput() = 0;
#if defined(ASCENSION_WINDOW_SYSTEM_WIN32)
			virtual LRESULT handleInputEvent(UINT message, WPARAM wp, LPARAM lp, bool& consumed) = 0;
#endif
			friend class viewers::TextViewer;
		};
	}
}

#endif // !VIEWER_OBSERVERS_HPP
