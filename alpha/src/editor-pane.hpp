/**
 * @file editor-pane.hpp
 * @author exeal
 * @date 2009-2010, 2014 was editor-window.hpp
 * @date 2014-08-24 Renamed from editor-window.hpp
 */

#ifndef ALPHA_EDITOR_PANE_HPP
#define ALPHA_EDITOR_PANE_HPP
#include <list>
#include <memory>
#include <boost/core/noncopyable.hpp>
#include <gtkmm/stack.h>

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
		void add(std::unique_ptr<EditorView> viewer, bool select = true);
		std::size_t numberOfViews() const BOOST_NOEXCEPT;
		void remove(const EditorView& viewer);
		void removeAll() BOOST_NOEXCEPT;
		void select(const EditorView& viewer);
		EditorView& selectedView() BOOST_NOEXCEPT;
		const EditorView& selectedView() const BOOST_NOEXCEPT;
		/// @}

		/// @name Splitting Panes
		/// @{
		void split();
		void splitSideBySide();
		/// @}

	private:
		void split(Gtk::Orientation orientation);
		void touch(const EditorView& viewer);

	private:
		std::list<std::unique_ptr<EditorView>> viewers_;	// visible and invisible viewers
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
