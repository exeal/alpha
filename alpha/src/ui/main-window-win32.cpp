/**
 * @file main-window-win32.cpp
 * Implements alpha#ui#MainWindow class.
 * @author exeal
 * @date 2014-06-04 Created.
 * @date 2017-01-21 Renamed from main-window.cpp.
 */

#include "ui/main-window.hpp"
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#	include "buffer-list.hpp"
#	include <ascension/viewer/widgetapi/widget.hpp>

namespace alpha {
	namespace ui {
		/// Default constructor.
		MainWindow::MainWindow() : ascension::win32::CustomControl<MainWindow>(ascension::win32::Window::TOPLEVEL) {
			ascension::viewer::widgetapi::setParentWindow(statusBar_, *this);
			bufferSelectionChangedConnection_ = editorPanes_.bufferSelectionChangedSignal().connect([this](EditorPanes&) {
				this->updateTitle();
			});
		}

		/// @see CustomControl#windowClass
		void MainWindow::windowClass(ascension::win32::WindowClass& out) const BOOST_NOEXCEPT {
			out.name = L"alpha.MainWindow";
			out.styles = CS_HREDRAW | CS_VREDRAW;
		}

		/// Updates the text string of the title bar.
		void MainWindow::updateTitle() {
//			if(isWindow()) {
				// show the display name of the selected buffer and application credit
				static PlatformString titleCache;
				PlatformString title(BufferList::instance().displayName(EditorPanes::instance().selectedBuffer()));
				if(title != titleCache) {
					titleCache = title;
					title += L" - ";
//					title += ALPHA_APPLICATION_FULL_NAME;
					title += L"Alpha";
					::SetWindowTextW(handle().get(), title.c_str());
				}
//			}
		}
	}
}

#endif
