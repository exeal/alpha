/**
 * @file main-window.hpp
 * @author exeal
 * @date 2003-2006 (was Alpha.h)
 * @date 2006-2009, 2013-2014
 * @date 2014-06-04 Separated from application.hpp
 */

#ifndef ALPHA_MAIN_WINDOW_HPP
#define ALPHA_MAIN_WINDOW_HPP
//#include "buffer.hpp"
#include "editor-panes.hpp"
//#include "search.hpp"	// ui.SearchDialog
#include "status-bar.hpp"
#include <gtkmm/hvbox.h>
#include <gtkmm/window.h>
#include <memory>	// std.unique_ptr

namespace alpha {
	namespace ui {
//		class SearchDialog;
//		class BookmarkDialog;

		class MainWindow : public Gtk::ApplicationWindow {
		public:
			MainWindow();

			void updateTitle();

			/// @name Children
			/// @{
			EditorPanes& editorPanes() const BOOST_NOEXCEPT;
//			SearchDialog& searchDialog() const BOOST_NOEXCEPT;
			StatusBar& statusBar() const BOOST_NOEXCEPT;
			/// @}

		private:
#ifdef _DEBUG
			bool on_event(GdkEvent* event) override;
#endif
		private:
			Gtk::VBox vbox_;
			EditorPanes editorPanes_;
//			std::unique_ptr<ui::SearchDialog> searchDialog_;
			StatusBar statusBar_;
			boost::signals2::scoped_connection bufferSelectionChangedConnection_;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			manah::win32::ui::Rebar rebar_;		// rebar
			manah::win32::ui::Toolbar toolbar_;	// standard toolbar
#endif
		};

		/// Returns the editor panes.
		inline EditorPanes& MainWindow::editorPanes() const BOOST_NOEXCEPT {
			return const_cast<MainWindow*>(this)->editorPanes_;
		}

//		/// Returns the search dialog box.
//		inline SearchDialog& MainWindow::searchDialog() const BOOST_NOEXCEPT {
//			return *searchDialog_;
//		}

		/// Returns the status bar.
		inline StatusBar& MainWindow::statusBar() const BOOST_NOEXCEPT {
			return const_cast<MainWindow*>(this)->statusBar_;
		}
	}
} // namespace alpha

#endif // !ALPHA_MAIN_WINDOW_HPP
