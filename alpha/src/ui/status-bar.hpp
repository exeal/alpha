/**
 * @file status-bar.hpp
 * @author exeal
 * @date 2003-2006 (was Alpha.h)
 * @date 2006-2009, 2013-2014 (was application.hpp)
 * @date 2014-03-29 Separated from application.hpp
 */

#ifndef ALPHA_STATUS_BAR_HPP
#define ALPHA_STATUS_BAR_HPP
#include <ascension/kernel/document.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#	include <gtkmm/label.h>
#	include <gtkmm/statusbar.h>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#	include <QStatusBar>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#	include "win32/status-bar.hpp"
#endif

namespace alpha {
	namespace ui {
		class MainWindow;

		/// The status bar for the application main window.
		class StatusBar : public
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			Gtk::Statusbar
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
			QStatusBar
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
			???
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			win32::StatusBar
#endif
		{
		public:
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			StatusBar();
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
			explicit StatusBar(QWidget* parent = Q_NULLPTR);
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
			???
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			explicit StatusBar(const Type& type);
#endif
		};
	}
}

#endif // !ALPHA_STATUS_BAR_HPP
