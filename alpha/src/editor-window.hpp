/**
 * @file editor-window.hpp
 * @author exeal
 * @date 2009-2010, 2014
 */

#ifndef ALPHA_EDITOR_WINDOW_HPP
#define ALPHA_EDITOR_WINDOW_HPP
#include "ambient.hpp"
#include <list>
#include <memory>
#include <ascension/corelib/signals.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <gtkmm/paned.h>

namespace alpha {
	class Buffer;
	class BufferList;
	class EditorView;

	class EditorPane : public Gtk::Container, private boost::noncopyable {
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

	class EditorPanes : public Gtk::Paned, private boost::noncopyable {	// children may be either Gtk.Paned or EditorPane
	private:
		template<typename Derived, typename Value>
		class InternalIterator : public boost::iterator_facade<Derived, Value, boost::bidirectional_traversal_tag> {
		public:
			InternalIterator(pointer pane);
		private:
			friend class boost::iterator_core_access;
			void decrement();
			reference dereference() const;
			bool equal(const InternalIterator& other) const;
			void increment();
		private:
			pointer current_;
			bool end_;
		};

	public:
		class Iterator : public InternalIterator<Iterator, EditorPane> {
		public:
			Iterator(pointer pane) : InternalIterator<Iterator, EditorPane>(pane) {}
		};
		class ConstIterator : public InternalIterator<ConstIterator, const EditorPane> {
		public:
			ConstIterator(pointer pane) : InternalIterator<ConstIterator, const EditorPane>(pane) {}
		};
		typedef Iterator iterator;
		typedef ConstIterator const_iterator;
		typedef std::ptrdiff_t difference_type;

	public:
		static EditorPanes& instance() BOOST_NOEXCEPT;

		/// @name Pane Access
		/// @{
		EditorPane& activePane() BOOST_NOEXCEPT;
		const EditorPane& activePane() const BOOST_NOEXCEPT;
		Iterator begin() BOOST_NOEXCEPT;
		ConstIterator begin() const BOOST_NOEXCEPT;
		ConstIterator cbegin() const BOOST_NOEXCEPT;
		ConstIterator cend() const BOOST_NOEXCEPT;
		Iterator end() BOOST_NOEXCEPT;
		ConstIterator end() const BOOST_NOEXCEPT;
		/// @}

		/// @name Removing Panes
		/// @{
		void remove(EditorPane* pane);
		void removeOthers(const EditorPane* pane, Gtk::Paned* root = nullptr);
		/// @}

		Buffer& selectedBuffer() BOOST_NOEXCEPT;
		const Buffer& selectedBuffer() const BOOST_NOEXCEPT;

		/// @name Signals
		/// @{
		typedef boost::signals2::signal<void(EditorPanes&)> BufferSelectionChangedSignal;
		ascension::SignalConnector<BufferSelectionChangedSignal> bufferSelectionChangedSignal() BOOST_NOEXCEPT;
		/// @}

	private:
		EditorPane* firstPane() const;
		EditorPane* lastPane() const;
		// BufferList signals
		void bufferAboutToBeRemoved(BufferList& buffers, Buffer& buffer);
		void bufferAdded(BufferList& buffers, Buffer& buffer);
		// EditorView signals
		bool viewFocused(GdkEventFocus* event);

	private:
		EditorPane* activePane_;
		EditorPane* lastActivePane_;
		BufferSelectionChangedSignal bufferSelectionChangedSignal_;
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

	/// Returns the active editor pane.
	inline EditorPane& EditorPanes::activePane() BOOST_NOEXCEPT {
		return *activePane_;
	}

	/// Returns the active editor pane.
	inline const EditorPane& EditorPanes::activePane() const BOOST_NOEXCEPT {
		return *activePane_;
	}

	/// Returns the singleton @c EditorPanes object.
	inline EditorPanes& EditorPanes::instance() {
		static EditorPanes singleton;
		return singleton;
	}
}

#endif // !ALPHA_EDITOR_WINDOW_HPP
