/**
 * @file main-window-gtk.cpp
 * Implements alpha#ui#MainWindow class.
 * @author exeal
 * @date 2014-06-04 Created.
 * @date 2017-01-21 Renamed from main-window.cpp.
 */

#include "main-window.hpp"
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#	include "buffer-list.hpp"
#	include "editor-view.hpp"
#	include <ascension/log.hpp>
#	include <glibmm/i18n.h>

namespace alpha {
	namespace ui {
		/// Default constructor.
		MainWindow::MainWindow() : /*Gtk::Window(Gtk::WINDOW_TOPLEVEL), */ box_(Gtk::ORIENTATION_VERTICAL) {
			bufferSelectionChangedConnection_ = editorPanes_.bufferSelectionChangedSignal().connect([this](EditorPanes&) {
				this->updateTitle();
			});

			add(box_);
			box_.pack_start(editorPanes(), Gtk::PACK_EXPAND_WIDGET);
			box_.pack_end(statusBar(), Gtk::PACK_SHRINK);
//			box_.show_all_children();
			show_all_children();
		}

		/// Updates the text string of the title bar.
		void MainWindow::updateTitle() {
//			if(isWindow()) {
				// show the display name of the selected buffer and application credit
				static PlatformString titleCache;
				PlatformString title(BufferList::instance().displayName(EditorPanes::instance().selectedBuffer()));
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

#endif
