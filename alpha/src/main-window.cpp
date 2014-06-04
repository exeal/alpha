/**
 * @file main-window.cpp
 * Implements alpha#ui#MainWindow class.
 * @author exeal
 * @date 2014-06-04 Created.
 */

#include "main-window.hpp"

namespace alpha {
	namespace ui {
		/// Default constructor.
		MainWindow::MainWindow() : Gtk::Window(Gtk::WINDOW_TOPLEVEL) {
			add(vbox_);
			vbox_.pack_start(editorPanes());
			vbox_.pack_start(statusBar());
		}
	}
}
