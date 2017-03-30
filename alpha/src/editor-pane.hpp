/**
 * @file editor-pane.hpp
 * @author exeal
 * @date 2009-2010, 2014 was editor-window.hpp
 * @date 2014-08-24 Renamed from editor-window.hpp
 */

#ifndef ALPHA_EDITOR_PANE_HPP
#define ALPHA_EDITOR_PANE_HPP
#include <boost/core/noncopyable.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#	include <gtkmm/box.h>
#	include <gtkmm/paned.hpp>
#	include <gtkmm/scrolledwindow.h>
#	include <gtkmm/stack.h>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#	include <QSplitter>
#	include <QStackedWidget>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#	include "win32/container.hpp"
#	include "win32/stacked-widget.hpp"
#endif
#include <list>
#include <memory>

namespace alpha {
	class Buffer;
	class EditorView;

	class EditorPane :
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
		public Gtk::Paned, private boost::noncopyable
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
		public QSplitter
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
		???
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		public win32::VerticalContainer
#endif
	{
	public:
		EditorPane();
		std::unique_ptr<EditorPane> clone() const;

		/// @name Buffer
		/// @{
		void removeBuffer(const Buffer& buffer);
		void selectBuffer(const Buffer& buffer);
		Buffer& selectedBuffer();
		const Buffer& selectedBuffer() const;
		/// @}

		/// @name Viewer
		/// @{
		void add(std::unique_ptr<EditorView> viewer);
		std::size_t numberOfViews() const BOOST_NOEXCEPT;
		void remove(const EditorView& viewer);
		void removeAll() BOOST_NOEXCEPT;
		void select(const EditorView& viewer);
		EditorView& selectedView();
		const EditorView& selectedView() const;
		/// @}

	private:
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		void realized(const Type& type) override;
#endif
		void touch(const EditorView& viewer);
	private:
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
		Gtk::Stack stack_;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
		QStackedWidget stack_;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
		??? stack_;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		win32::StackedWidget stack_;
#endif
//		std::unique_ptr<ModeLine> modeLine_;
		std::list<std::shared_ptr<EditorView>> viewers_;
	};

	/// Returns the number of the viewers.
	inline std::size_t EditorPane::numberOfViews() const BOOST_NOEXCEPT {
		return viewers_.size();
	}

	/// Returns the visible viewer.
	inline EditorView& EditorPane::selectedView() {
		if(viewers_.empty())
			throw std::logic_error("There are no viewers.");
		return *viewers_.front();
	}

	/// Returns the visible viewer.
	inline const EditorView& EditorPane::selectedView() const {
		if(viewers_.empty())
			throw std::logic_error("There are no viewers.");
		return *viewers_.front();
	}
}

#endif // !ALPHA_EDITOR_PANE_HPP
