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
#include <gtkmm/label.h>
#include <gtkmm/statusbar.h>

namespace alpha {
	class EditorPanes;

	namespace ui {
		/// The status bar for the application main window.
		class StatusBar : public Gtk::Statusbar {
		public:
			StatusBar();
			bool isSimple() const BOOST_NOEXCEPT;
			void setSimple(bool simple);
#if 0
			void updateAll();
			void updateCaretPosition();
			void updateNarrowingStatus();
			void updateOvertypeMode();
			void updateTemporaryMacroRecordingStatus();
#endif

		private:
#ifdef _DEBUG
			bool on_event(GdkEvent* event) override;
			void on_realize() override;
#endif
			// ascension.kernel.Document signals
			void selectedBufferAccessibleRegionChanged(const ascension::kernel::Document& document);
			// EditorPanes signals
			void bufferSelectionChanged(EditorPanes& panes);

		private:
			Gtk::Label caretPositionLabel_, narrowingStatusLabel_, overtypeModeLabel_, temporaryMacroRecordingStatusLabel_;
			ascension::Index columnStartValue_;
			boost::signals2::connection selectedBufferAccessibleRegionChangedConnection_, bufferSelectionChangedConnection_;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			ascension::win32::Handle<HFONT> defaultFont_;
			manah::win32::gdi::Font font_;
			manah::win32::Object<HICON, ::DestroyIcon> narrowingIcon_;
#endif
		};
	}
} // namespace alpha

#endif // !ALPHA_STATUS_BAR_HPP
