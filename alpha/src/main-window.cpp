/**
 * @file main-window.cpp
 * Implements alpha#ui#MainWindow class.
 * @author exeal
 * @date 2014-06-04 Created.
 */

#include "buffer-list.hpp"
#include "editor-view.hpp"
#include "main-window.hpp"
#include <ascension/log.hpp>
#include <glibmm/i18n.h>

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

#ifdef _DEBUG
		bool MainWindow::on_event(GdkEvent* event) {
			ASCENSION_LOG_TRIVIAL(debug)
				<< "allocation = " << get_allocated_width() << "x" << get_allocated_height() << std::endl;
			if(event != nullptr)
				ASCENSION_LOG_TRIVIAL(debug) << event->type << std::endl;
			ASCENSION_LOG_TRIVIAL(debug) << get_focus() << std::endl;
			return Gtk::/*Application*/Window::on_event(event);
		}
#endif

		bool MainWindow::on_focus_in_event(GdkEventFocus* e) {
#if 0
			if(editorPanes().get_realized())
				set_focus_child(editorPanes().activePane().selectedView());
			return true;
#else
			return Gtk::Window::on_focus_in_event(e);
#endif
		}

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
