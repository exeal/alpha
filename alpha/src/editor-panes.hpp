/**
 * @file editor-panes.hpp
 * @author exeal
 * @date 2009-2010, 2014 was editor-window.hpp
 * @date 2014-08-22 Renamed from editor-window.hpp
 * @date 2014-08-22 Separated from editor-pane.hpp
 */

#ifndef ALPHA_EDITOR_PANES_HPP
#define ALPHA_EDITOR_PANES_HPP
#include "editor-pane.hpp"
#include <ascension/corelib/signals.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <gtkmm/paned.h>

namespace alpha {
	class BufferList;
	class EditorPane;

	class EditorPanes : public Gtk::Paned, private boost::noncopyable {	// children may be either Gtk.Paned or EditorPane
	private:
		template<typename Derived, typename Value>
		class InternalIterator : public boost::iterators::iterator_facade<Derived, Value, boost::iterators::bidirectional_traversal_tag> {
		public:
			InternalIterator(typename iterator_facade_::pointer pane);
		private:
			friend class boost::iterators::iterator_core_access;
			void decrement();
			typename iterator_facade_::reference dereference() const;
			bool equal(const InternalIterator& other) const;
			void increment();
		private:
			typename iterator_facade_::pointer current_;
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
		EditorPanes();
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
		// Gtk.Widget
		bool on_focus_in_event(GdkEventFocus* event) override;
#ifdef _DEBUG
		bool on_event(GdkEvent* event) override;
		void on_realize() override;
#endif

	private:
		EditorPane* activePane_;
		EditorPane* lastActivePane_;
		boost::signals2::connection bufferAboutToBeRemovedConnection_, bufferAddedConnection_;
		BufferSelectionChangedSignal bufferSelectionChangedSignal_;
	};

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

#endif // !ALPHA_EDITOR_PANES_HPP
