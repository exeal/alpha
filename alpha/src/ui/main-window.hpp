/**
 * @file main-window.hpp
 * @author exeal
 * @date 2003-2006 (was Alpha.h)
 * @date 2006-2009, 2013-2014
 * @date 2014-06-04 Separated from application.hpp
 */

#ifndef ALPHA_MAIN_WINDOW_HPP
#define ALPHA_MAIN_WINDOW_HPP
#include "editor-panes.hpp"
#include "status-bar.hpp"
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#	include <gtkmm/box.h>
#	include <gtkmm/window.h>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#	include <ascension/win32/window/custom-control.hpp>
#endif

namespace alpha {
	namespace ui {
		class MainWindow : public
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			Gtk::/*Application*/Window
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			ascension::win32::CustomControl<MainWindow>
#endif
		{
		public:
			MainWindow();

			void updateTitle();

			/// @name Children
			/// @{
			EditorPanes& editorPanes() const BOOST_NOEXCEPT;
			StatusBar& statusBar() const BOOST_NOEXCEPT;
			/// @}

		private:
			EditorPanes editorPanes_;
			StatusBar statusBar_;
			boost::signals2::scoped_connection bufferSelectionChangedConnection_;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			Gtk::Box box_;	// TODO: Replace by Gtk.Grid.
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			void windowClass(ascension::win32::WindowClass& out) const BOOST_NOEXCEPT override;
//			manah::win32::ui::Rebar rebar_;		// rebar
//			manah::win32::ui::Toolbar toolbar_;	// standard toolbar
#endif
		};

		/// Returns the editor panes.
		inline EditorPanes& MainWindow::editorPanes() const BOOST_NOEXCEPT {
			return const_cast<MainWindow*>(this)->editorPanes_;
		}

		/// Returns the status bar.
		inline StatusBar& MainWindow::statusBar() const BOOST_NOEXCEPT {
			return const_cast<MainWindow*>(this)->statusBar_;
		}
	}
} // namespace alpha

#endif // !ALPHA_MAIN_WINDOW_HPP
