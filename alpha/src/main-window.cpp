/**
 * @file main-window.cpp
 * Implements alpha#ui#MainWindow class.
 * @author exeal
 * @date 2014-06-04 Created.
 */

#include "buffer-list.hpp"
#include "main-window.hpp"
#include <glibmm/i18n.h>

namespace alpha {
	namespace ui {
		/// Default constructor.
		MainWindow::MainWindow() /*: Gtk::Window(Gtk::WINDOW_TOPLEVEL)*/ {
			bufferSelectionChangedConnection_ = editorPanes_.bufferSelectionChangedSignal().connect([this](EditorPanes&) {
				this->updateTitle();
			});

			add(vbox_);
			vbox_.pack_start(editorPanes());
			vbox_.pack_start(statusBar(), Gtk::PACK_SHRINK);
//			vbox_.show_all_children();
			show_all_children();
		}

#ifdef _DEBUG
		bool MainWindow::on_event(GdkEvent* event) {
			return Gtk::ApplicationWindow::on_event(event);
		}
#endif

		/// Updates the text string of the title bar.
		void MainWindow::updateTitle() {
//			if(isWindow()) {
				// show the display name of the selected buffer and application credit
				static Glib::ustring titleCache;
				Glib::ustring title(BufferList::instance().displayName(EditorPanes::instance().selectedBuffer()));
				if(title != titleCache) {
					titleCache = title;
					title += " - ";
//					title += ALPHA_APPLICATION_FULL_NAME;
					title += _("Alpha");
					set_title(title.c_str());
				}
//			}
		}
	}
}
