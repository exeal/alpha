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
#	include <gtkmm/scrolledwindow.h>
#	include <gtkmm/stack.h>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#	include <QStackedWidget>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#	include "win32/stacked-widget.hpp"
#endif
#include <list>
#include <memory>

namespace alpha {
	class Buffer;
	class EditorView;

	class EditorPane :
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
		public Gtk::Stack, private boost::noncopyable
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
		public QStackedWidget
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
		???
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		public win32::StackedWidget
#endif
	{
	public:
		explicit EditorPane(std::unique_ptr<EditorView> initialViewer = std::unique_ptr<EditorView>());
		EditorPane::EditorPane(const EditorPane& other);

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
		void touch(const EditorView& viewer);

	private:
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		class Container : public ascension::win32::CustomControl<Container> {
		public:
			Container();
			static void windowClass(ascension::win32::WindowClass& out) BOOST_NOEXCEPT;
		private:
			LRESULT processMessage(UINT message, WPARAM wp, LPARAM lp, bool& consumed) override;
		};
#endif
		struct Child {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			Gtk::Box* container;	// child of EditorPane. managed by gtkmm
			Glib::RefPtr<Gtk::ScrolledWindow> scroller;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			std::unique_ptr<Container> container;
#endif
			std::unique_ptr<EditorView> viewer;
//			std::unique_ptr<ModeLine> modeLine;
		};
		std::list<Child> children_;	// visible and invisible children
	};

	/// Returns the number of the viewers.
	inline std::size_t EditorPane::numberOfViews() const BOOST_NOEXCEPT {
		return children_.size();
	}

	/// Returns the visible viewer.
	inline EditorView& EditorPane::selectedView() {
		if(children_.empty())
			throw std::logic_error("There are no viewers.");
		return *children_.front().viewer;
	}

	/// Returns the visible viewer.
	inline const EditorView& EditorPane::selectedView() const {
		if(children_.empty())
			throw std::logic_error("There are no viewers.");
		return *children_.front().viewer;
	}
}

#endif // !ALPHA_EDITOR_PANE_HPP
