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

	namespace detail {
		class Paned :
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			public Gtk::Paned, private boost::noncopyable
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
			public QSplitter, private boost::noncopyable
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
			???
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			public win32::PanedWidget
#endif
		{
		public:
			Paned* parent() const BOOST_NOEXCEPT {
				return parent_;
			}
		protected:
#if ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
			explicit Paned(Paned* parentPaned, QWidget* parent = Q_NULLPTR);
#else
			explicit Paned(Paned* parentPaned);
#endif
		private:
			Paned* parent_;
		};
	}

	class EditorPanes : public FocusChain, public detail::Paned {	// children may be either Paned or EditorPane
	private:
		template<typename Value>
		class InternalIterator : public boost::iterators::iterator_facade<InternalIterator<Value>, Value, boost::iterators::bidirectional_traversal_tag> {
		public:
			typedef detail::Paned MutablePaned;
			typedef typename std::conditional<std::is_const<Value>::value, const MutablePaned, MutablePaned>::type Paned;
			static InternalIterator makeFirst(Paned& root);
			static InternalIterator makeLast(Paned& root);
		private:
			template<std::size_t branch> static InternalIterator make(Paned& root);
			// boost.iterators.iterator_facade
			friend class boost::iterators::iterator_core_access;
			void decrement();
			typename iterator_facade_::reference dereference() const;
			bool equal(const InternalIterator& other) const;
			void increment();
		private:
			std::shared_ptr<Value> current_;
			detail::Paned* parent_;
		};

	public:
		typedef InternalIterator<EditorPane> iterator;
		typedef InternalIterator<const EditorPane> const_iterator;
		typedef std::ptrdiff_t difference_type;

	public:
#if ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
		EditorPanes(QWidget* parent = Q_NULLPTR);
#else
		EditorPanes();
#endif

		/// @name Pane Access
		/// @{
		EditorPane& activePane() BOOST_NOEXCEPT;
		const EditorPane& activePane() const BOOST_NOEXCEPT;
		iterator begin() BOOST_NOEXCEPT;
		const_iterator begin() const BOOST_NOEXCEPT;
		const_iterator cbegin() const BOOST_NOEXCEPT;
		const_iterator cend() const BOOST_NOEXCEPT;
		iterator end() BOOST_NOEXCEPT;
		const_iterator end() const BOOST_NOEXCEPT;
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
		LRESULT processMessage(UINT message, WPARAM wp, LPARAM lp, bool& consumed) override;
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
}

#endif // !ALPHA_EDITOR_PANES_HPP
