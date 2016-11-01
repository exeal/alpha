/**
 * @file editor-pane.hpp
 * @author exeal
 * @date 2009-2010, 2014 was editor-window.hpp
 * @date 2014-08-24 Renamed from editor-window.hpp
 */

#ifndef ALPHA_EDITOR_PANE_HPP
#define ALPHA_EDITOR_PANE_HPP
#include <boost/core/noncopyable.hpp>
#include <gtkmm/box.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/stack.h>
#include <list>
#include <memory>

namespace alpha {
	class Buffer;
	class EditorView;

	class EditorPane : public Gtk::Stack, private boost::noncopyable {
	public:
		explicit EditorPane(std::unique_ptr<EditorView> initialViewer = std::unique_ptr<EditorView>());
		EditorPane::EditorPane(const EditorPane& other);

		/// @name Buffer
		/// @{
		void removeBuffer(const Buffer& buffer);
		void selectBuffer(const Buffer& buffer);
		Buffer& selectedBuffer() BOOST_NOEXCEPT;
		const Buffer& selectedBuffer() const BOOST_NOEXCEPT;
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

		/// @name Splitting Panes
		/// @{
		void split();
		void splitSideBySide();
		/// @}

	private:
#ifdef _DEBUG
		bool on_event(GdkEvent* event) override;
		void on_realize() override;
		void on_size_allocate(Gtk::Allocation& allocation) override;
#endif
		void split(Gtk::Orientation orientation);
		void touch(const EditorView& viewer);

	private:
		typedef std::tuple<
			Gtk::Box*,	// child of EditorPane. managed by gtkmm
			Glib::RefPtr<Gtk::ScrolledWindow>,
			std::unique_ptr<EditorView>
//			std::unique_ptr<ModeLine>
		> Child;
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
		return *std::get<2>(children_.front());
	}

	/// Returns the visible viewer.
	inline const EditorView& EditorPane::selectedView() const {
		if(children_.empty())
			throw std::logic_error("There are no viewers.");
		return *std::get<2>(children_.front());
	}
}

#endif // !ALPHA_EDITOR_PANE_HPP
