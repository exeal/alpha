/**
 * @file editor-panes.hpp
 * @author exeal
 * @date 2009-2010, 2014 was editor-window.hpp
 * @date 2014-08-22 Renamed from editor-window.hpp
 * @date 2014-08-22 Separated from editor-pane.hpp
 */

#ifndef ALPHA_EDITOR_PANES_HPP
#define ALPHA_EDITOR_PANES_HPP
//#include "editor-pane.hpp"
#include <ascension/corelib/signals.hpp>
#include <boost/iterator/iterator_facade.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#	include <gtkmm/paned.h>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#	include <QSplitter>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#	include "win32/paned-widget.hpp"
#endif

namespace alpha {
	class Buffer;
	class BufferList;
	class EditorPane;
	class EditorView;

	class FocusChain {
	protected:
		/// Destructor.
		virtual ~FocusChain() BOOST_NOEXCEPT {}

	private:
		/**
		 * Sets the focused view.
		 * @param view The @c EditorView focused in
		 */
		virtual void focus(EditorView& view) = 0;
		friend class EditorView;
	};

	class EditorPanes : public FocusChain,
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
		public Gtk::Paned, private boost::noncopyable
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
		public QSplitter, private boost::noncopyable
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
		???
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		public win32::PanedWidget
#endif
		{	// children may be either native widget or EditorPane
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

		/// @name Paned Operations
		/// @{
		void remove(EditorPane& pane);
		void removeOthers(const EditorPane& pane);
		void split(EditorPane& pane);
		void splitSideBySide(EditorPane& pane);
		/// @}

		Buffer& selectedBuffer();
		const Buffer& selectedBuffer() const;

		/// @name Signals
		/// @{
		typedef boost::signals2::signal<void(EditorPanes&)> BufferSelectionChangedSignal;
		ascension::SignalConnector<BufferSelectionChangedSignal> bufferSelectionChangedSignal() BOOST_NOEXCEPT;
		/// @}

	private:
		EditorPane* firstPane() const;
		void initializeWidget();
		EditorPane* lastPane() const;
		void split(EditorPane& pane, bool sideBySide);
		// BufferList signals
		void bufferAboutToBeRemoved(BufferList& buffers, Buffer& buffer);
		void bufferAdded(BufferList& buffers, std::shared_ptr<Buffer> buffer);
		// FocusChain
		void focus(EditorView& view) override;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
		// Gtk.Widget
		bool on_focus_in_event(GdkEventFocus* event) override;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		// ascension.win32.CustomControl
		void realized(const Type& type) override;
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
	inline EditorPanes& EditorPanes::instance() BOOST_NOEXCEPT {
		static EditorPanes singleton;
		return singleton;
	}
}

#endif // !ALPHA_EDITOR_PANES_HPP
