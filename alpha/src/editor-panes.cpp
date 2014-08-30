/**
 * @file editor-panes.cpp
 * @author exeal
 * @date 2008 Separated from buffer.cpp
 * @date 2008-2009, 2014 was editor-window.cpp
 * @date 2014-08-22 Renamed from editor-window.cpp
 * @date 2014-08-22 Separated from editor-pane.cpp
 */

#include "application.hpp"
#include "buffer-list.hpp"
#include "editor-panes.hpp"
#include "editor-view.hpp"
#include "function-pointer.hpp"
#include <boost/foreach.hpp>
//#include <boost/range/algorithm/find.hpp>

namespace alpha {
	/**
	 * @typedef alpha::EditorPanes::BufferSelectionChangedSignal
	 * The signal which gets emitted when the buffer selection was changed.
	 * @param editorPanes The editor panes
	 * @see #selectedBuffer, BufferList#BufferSelectionChangedSignal
	 */

	/// Default constructor.
	EditorPanes::EditorPanes() : Gtk::Paned(Gtk::ORIENTATION_VERTICAL) {
		BufferList& bufferList = BufferList::instance();
		bufferAboutToBeRemovedConnection_ =
			bufferList.bufferAboutToBeRemovedSignal().connect(
				std::bind(&EditorPanes::bufferAboutToBeRemoved, this, std::placeholders::_1, std::placeholders::_2));
		bufferAddedConnection_ =
			bufferList.bufferAddedSignal().connect(
				std::bind(&EditorPanes::bufferAdded, this, std::placeholders::_1, std::placeholders::_2));
		add1(*Gtk::manage(new EditorPane()));
		show_all_children();
	}

	/// Returns the iterator addresses the first editor pane.
	EditorPanes::Iterator EditorPanes::begin() BOOST_NOEXCEPT {
		return Iterator(firstPane());
	}

	/// Returns the iterator addresses the first editor pane.
	EditorPanes::ConstIterator EditorPanes::begin() const BOOST_NOEXCEPT {
		return cbegin();
	}

	/// @see BufferList#BufferAboutToBeRemoved
	void EditorPanes::bufferAboutToBeRemoved(BufferList& buffers, Buffer& buffer) {
		// remove the specified buffer from each panes
		BOOST_FOREACH(EditorPane& pane, *this)
			pane.removeBuffer(buffer);
	}

	/// @see BufferList#BufferAdded
	void EditorPanes::bufferAdded(BufferList& buffers, Buffer& buffer) {
		// create new views for each panes
		EditorView* originalView = nullptr;
		BOOST_FOREACH(EditorPane& pane, *this) {
			std::unique_ptr<EditorView> newView((originalView == nullptr) ? new EditorView(buffer.presentation()) : new EditorView(*originalView));
			newView->signal_focus_in_event().connect(sigc::mem_fun(*this, &EditorPanes::viewFocused));
			if(originalView == nullptr)
				originalView = newView.get();
			if(originalView != newView.get())
				newView->setConfiguration(&originalView->configuration(), 0, true);
			pane.add(std::move(newView));
		}
	}

	ascension::SignalConnector<EditorPanes::BufferSelectionChangedSignal> EditorPanes::bufferSelectionChangedSignal() BOOST_NOEXCEPT {
		return ascension::makeSignalConnector(bufferSelectionChangedSignal_);
	}

	/// Returns the iterator addresses the first editor pane.
	EditorPanes::ConstIterator EditorPanes::cbegin() const BOOST_NOEXCEPT {
		return ConstIterator(firstPane());
	}

	/// Returns the iterator addresses one past the last editor pane.
	EditorPanes::ConstIterator EditorPanes::cend() const BOOST_NOEXCEPT {
		const EditorPane* const p = lastPane();
		return (p != nullptr) ? ++ConstIterator(p) : ConstIterator(nullptr);
	}

	/// Returns the iterator addresses one past the last editor pane.
	EditorPanes::Iterator EditorPanes::end() BOOST_NOEXCEPT {
		EditorPane* const p = lastPane();
		return (p != nullptr) ? ++Iterator(p) : Iterator(nullptr);
	}

	/// Returns the iterator addresses one past the last editor pane.
	EditorPanes::ConstIterator EditorPanes::end() const BOOST_NOEXCEPT {
		return cend();
	}

	/// @internal Returns the first editor pane, or @c null if empty.
	EditorPane* EditorPanes::firstPane() const {
		for(const Gtk::Paned* paned = this; ; ) {
			const Gtk::Widget* const child = paned->get_child1();
			if(child == nullptr)
				break;
			else if(child->get_type() != Gtk::Paned::get_type())
				return const_cast<EditorPane*>(static_cast<const EditorPane*>(child));
			paned = static_cast<const Gtk::Paned*>(child);
		}
		return nullptr;
	}

	/// @internal Returns the last editor pane, or @c null if empty.
	EditorPane* EditorPanes::lastPane() const {
		for(const Gtk::Paned* paned = this; ; ) {
			const Gtk::Widget* child = paned->get_child2();
			if(child == nullptr)
				child = paned->get_child1();
			if(child == nullptr)
				break;
			if(child->get_type() != Gtk::Paned::get_type())
				return const_cast<EditorPane*>(static_cast<const EditorPane*>(child));
			paned = static_cast<const Gtk::Paned*>(child);
		}
		return nullptr;
	}

#ifdef _DEBUG
	bool EditorPanes::on_event(GdkEvent* event) {
		return Gtk::Paned::on_event(event);
	}

	void EditorPanes::on_realize() {
		return Gtk::Paned::on_realize();
	}
#endif

	/**
	 * Deletes the specified pane
	 * @param pane The pane to remove. If @c null, the selected pane is removed
	 * @throw ascension#NoSuchElementException @a pane is not valid
	 */
	void EditorPanes::remove(EditorPane* pane) {
	}

	/**
	 * Deletes all panes other except the specified one.
	 * @param pane
	 * @param root
	 */
	void EditorPanes::removeOthers(const EditorPane* pane, Gtk::Paned* root /* = nullptr */) {
	}

	/// Returns the selected buffer.
	Buffer& EditorPanes::selectedBuffer() {
		return activePane().selectedBuffer();
	}

	/// Returns the selected buffer.
	const Buffer& EditorPanes::selectedBuffer() const {
		return activePane().selectedBuffer();
	}

	/// @see 
	bool EditorPanes::viewFocused(GdkEventFocus*) {
		BOOST_FOREACH(EditorPane& pane, *this) {
			if(pane.selectedView().has_focus()) {
				lastActivePane_ = activePane_;
				activePane_ = &pane;
				break;
			}
		}
		return false;
	}


	// EditorPanes.InternalIterator ///////////////////////////////////////////////////////////////////////////////////

	template<typename Derived, typename Reference>
	EditorPanes::InternalIterator<Derived, Reference>::InternalIterator(pointer pane) : current_(pane), end_(pane == nullptr) {
	}

	template<typename Derived, typename Reference>
	bool EditorPanes::InternalIterator<Derived, Reference>::equal(const InternalIterator<Derived, Reference>& other) const {
		return current_ == other.current_ && end_ == other.end_;
	}

	template<typename Derived, typename Reference>
	typename EditorPanes::InternalIterator<Derived, Reference>::reference EditorPanes::InternalIterator<Derived, Reference>::dereference() const {
		if(end_ /* || current_ == nullptr*/)
			throw ascension::NoSuchElementException();
		return *current_;
	}

	template<typename Derived, typename Reference>
	void EditorPanes::InternalIterator<Derived, Reference>::increment() {
		if(end_ || current_ == nullptr)
			throw ascension::NoSuchElementException();

		const bool isConst = std::is_const<Reference>::value;
		typedef std::conditional<!isConst, Gtk::Paned, const Gtk::Paned>::type PanedType;
		auto parent = static_cast<PanedType*>(current_->get_parent());
		assert(parent->get_type() == Gtk::Paned::get_type());
		std::conditional<!isConst, Gtk::Widget, const Gtk::Widget>::type* child = &**this;
		while(parent->get_child1() != child) {
			assert(parent->get_child2() == child);
			child = parent;
			parent = static_cast<PanedType*>(child->get_parent());
			assert(parent->get_type() == Gtk::Paned::get_type());
		}

		child = parent->get_child2();
		while(child != nullptr) {
			if(child->get_type() == Gtk::Paned::get_type()) {
				parent = static_cast<PanedType*>(child);
				child = parent->get_child1();
			} else
				break;
		}

		if(child != nullptr)
			current_ = static_cast<pointer>(child);
		else
			end_ = true;
	}


	namespace {
		Buffer& currentBuffer(boost::python::object o) {
			EditorPane* pane = boost::python::extract<EditorPane*>(o);
			if(pane == nullptr) {
				EditorPanes* panes = boost::python::extract<EditorPanes*>(o);
				if(panes == nullptr)
					panes = &Application::instance()->window().editorPanes();
				pane = &panes->activePane();
			}
			return pane->selectedBuffer();
		}

		void selectBuffer(EditorPane& pane, boost::python::object o) {
			if(o == boost::python::object()) {
				pane.grab_focus();
				return;
			}
			if(boost::python::extract<Buffer*>(o).check())
				pane.selectBuffer(boost::python::extract<Buffer&>(o));
			else if(boost::python::extract<EditorView&>(o).check())
				pane.select(boost::python::extract<EditorView&>(o));
			else {
				::PyErr_BadArgument();
				boost::python::throw_error_already_set();
			}
		}
	}

	ALPHA_EXPOSE_PROLOGUE(6)
		boost::python::scope scope(ambient::Interpreter::instance().toplevelPackage());

		boost::python::class_<EditorPane, boost::noncopyable>("_Window", boost::python::no_init)
			.add_property("current_buffer", boost::python::make_function(&currentBuffer, boost::python::return_internal_reference<>()))
//			.add_property("selected_editor", &selectedTextEditor)
			.def("select", &selectBuffer, boost::python::arg("object") = boost::python::object())
			.def<void (EditorPane::*)(void)>("split", &EditorPane::split)
			.def("split_side_by_side", &EditorPane::splitSideBySide);

		boost::python::class_<EditorPanes, boost::noncopyable>("_WindowList", boost::python::no_init)
//			.def("__contains__", [](const EditorPanes& panes, const EditorPane& pane) {
//				return boost::find(panes, pane) != boost::end(panes);
//			})
			.def("__del__", &EditorPanes::remove)
			.def("__iter__", boost::python::iterator<EditorPanes>())
//			.def("__len__", [](const EditorPanes& panes) {
//				return std::distance(std::begin(panes), std::end(panes));
//			})
			.def("delete", &EditorPanes::remove, boost::python::arg("pane") = boost::python::object())
			.def("delete_others", &EditorPanes::removeOthers, boost::python::arg("pane") = boost::python::object(), boost::python::arg("root") = boost::python::object());

		boost::python::def("current_buffer", &currentBuffer,
			boost::python::arg("pane_or_panes") = boost::python::object(), boost::python::return_internal_reference<>());

		boost::python::def("selected_window", boost::python::make_function(
			ambient::makeFunctionPointer([]() -> EditorPane& {
				return Application::instance()->window().editorPanes().activePane();
			}),
			boost::python::return_value_policy<boost::python::reference_existing_object>()));

		boost::python::def("windows", boost::python::make_function(
			ambient::makeFunctionPointer([]() -> EditorPanes& {
				return Application::instance()->window().editorPanes();
			}),
			boost::python::return_value_policy<boost::python::reference_existing_object>()));
	ALPHA_EXPOSE_EPILOGUE()
}
