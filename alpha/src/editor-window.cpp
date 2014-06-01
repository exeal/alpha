/**
 * @file editor-window.cpp
 * @author exeal
 * @date 2008 Separated from buffer.cpp
 * @date 2008-2009, 2014
 */

#include "application.hpp"
#include "buffer-list.hpp"
#include "editor-window.hpp"
#include "editor-view.hpp"
#include "function-pointer.hpp"
#include <ascension/graphics/font/text-viewport.hpp>
#include <boost/foreach.hpp>
#include <boost/range/algorithm/find.hpp>

namespace alpha {
	namespace {
		EditorPanes& editorPanes() BOOST_NOEXCEPT;
	}

	// EditorPane /////////////////////////////////////////////////////////////////////////////////////////////////////

	/**
	 * @typedef alpha::EditorPanes::BufferSelectionChangedSignal
	 * The signal which gets emitted when the active buffer was switched.
	 * @param editorPanes The editor panes widget
	 * @see #bufferSelectionChangedSignal, #selectedBuffer
	 */

	/// Constructor.
	EditorPane::EditorPane(std::unique_ptr<EditorView> initialViewer /* = std::unique_ptr<EditorView> */) : selectedViewer_(nullptr), lastSelectedViewer_(nullptr) {
		if(initialViewer.get() != nullptr)
			addView(std::move(initialViewer));
	}

	/// Copy-constructor.
	EditorPane::EditorPane(const EditorPane& other) {
		for(std::size_t i = 0, c = other.viewers_.size(); i != c; ++i) {
			std::unique_ptr<EditorView> newView(new EditorView(*other.viewers_[i]));
//			const bool succeeded = newViewer->create(other.viewers_[i]->getParent().use(), win32::ui::DefaultWindowRect(),
//				WS_CHILD | WS_CLIPCHILDREN | WS_HSCROLL | WS_VISIBLE | WS_VSCROLL, WS_EX_CLIENTEDGE);
//			assert(succeeded);
			newView->setConfiguration(&other.viewers_[i]->configuration(), 0, true);
			newView->textRenderer().viewport()->scrollTo(other.viewers_[i]->textRenderer().viewport()->scrollPositions());
			addView(std::move(newView));
			viewers_.push_back(std::move(newView));
			if(other.viewers_[i].get() == other.selectedViewer_)
				selectedViewer_ = viewers_[i].get();
			if(other.viewers_[i].get() == other.lastSelectedViewer_)
				lastSelectedViewer_ = viewers_[i].get();
		}
	}

	/**
	 * Adds the new viewer.
	 * @param viewer the viewer to add
	 * @throw ascension#NullPointerException @a viewer is @c null
	 */
	void EditorPane::addView(std::unique_ptr<EditorView> viewer) {
		if(viewer.get() == 0)
			throw ascension::NullPointerException("viewer");
		viewers_.push_back(std::move(viewer));
		if(viewers_.size() == 1)
			selectBuffer(viewer->document());
	}

	/// Removes all viewers.
	void EditorPane::removeAllViews() BOOST_NOEXCEPT {
		viewers_.clear();
		selectedViewer_ = lastSelectedViewer_ = nullptr;
	}

	/**
	 * Removes the viewer belongs to the specified buffer.
	 * @param buffer the buffer has the viewer to remove
	 */
	void EditorPane::removeBuffer(const Buffer& buffer) {
		for(std::vector<std::unique_ptr<EditorView>>::iterator viewer(std::begin(viewers_)), e(std::end(viewers_)); viewer != e; ++viewer) {
			if(&(*viewer)->document() == &buffer) {
				const std::unique_ptr<EditorView>& removing = *viewer;
				viewers_.erase(viewer);
				if(removing.get() == selectedViewer_) {
					selectedViewer_ = nullptr;
					if(removing.get() == lastSelectedViewer_)
						lastSelectedViewer_ = nullptr;
					if(viewers_.size() == 1 || lastSelectedViewer_ == nullptr)
						selectBuffer(viewers_.front()->document());
					else if(!viewers_.empty()) {
						selectBuffer(lastSelectedViewer_->document());
						lastSelectedViewer_ = nullptr;
					}
				}
				return;
			}
		}
	}

	/**
	 * Shows the specified buffer in the pane.
	 * @param buffer the buffer to show
	 * @throw std#invalid_argument @a buffer is not exist
	 */
	void EditorPane::selectBuffer(const Buffer& buffer) {
		if(selectedViewer_ != nullptr && &selectedBuffer() == &buffer)
			return;
		BOOST_FOREACH(std::unique_ptr<EditorView>& viewer, viewers_) {
			if(&viewer->document() == &buffer) {
				const bool hadFocus = selectedViewer_ == nullptr || selectedView().has_focus();
				lastSelectedViewer_ = selectedViewer_;
				selectedViewer_ = viewer.get();
//				EditorWindows::instance().adjustPanes();
				selectedView().show();
				if(lastSelectedViewer_ != nullptr)
					lastSelectedViewer_->hide();
				if(hadFocus)
					selectedView().grab_focus();
				return;
			}
		}
		throw std::invalid_argument("Specified buffer is not contained in the pane.");
	}

	/// Returns the selected buffer.
	Buffer& EditorPane::selectedBuffer() {
		return selectedView().document();
	}

	/// Returns the selected buffer.
	const Buffer& EditorPane::selectedBuffer() const {
		return selectedView().document();
	}

	/// @internal Implements @c #split(void) and @c #splitSideBySide methods.
	void EditorPane::split(Gtk::Orientation orientation) {
		Gtk::Container* const parent = get_parent();
		assert(parent->get_type() == Gtk::Paned::get_type());
		Gtk::Paned* const panedParent = static_cast<Gtk::Paned*>(parent);

		const bool primary = panedParent->get_child1() == this;
		assert(primary || panedParent->get_child2() == this);
		std::unique_ptr<Gtk::Paned> newPaned(new Gtk::Paned(orientation));
		std::unique_ptr<EditorPane> newPane(new EditorPane(*this));
		panedParent->remove(*this);
		newPaned->add1(*this);
		newPaned->add2(*Gtk::manage(newPane.release()));
		if(primary)
			panedParent->add1(*Gtk::manage(newPaned.release()));
		else
			panedParent->add2(*Gtk::manage(newPaned.release()));
	}

	/// Splits this pane.
	void EditorPane::split() {
		return split(Gtk::ORIENTATION_VERTICAL);
	}

	/// Splits this pane side-by-side.
	void EditorPane::splitSideBySide() {
		return split(Gtk::ORIENTATION_HORIZONTAL);
	}


	// EditorPanes ////////////////////////////////////////////////////////////////////////////////////////////////////

	/**
	 * @typedef alpha::EditorPanes::BufferSelectionChangedSignal
	 * The signal which gets emitted when the buffer selection was changed.
	 * @param editorPanes The editor panes
	 * @see #selectedBuffer
	 */

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
			pane.addView(std::move(newView));
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
		boost::python::object currentBuffer(boost::python::object o) {
			EditorPane* pane = boost::python::extract<EditorPane*>(o);
			if(pane == nullptr) {
				EditorPanes* panes = boost::python::extract<EditorPanes*>(o);
				if(panes == nullptr)
					panes = &editorPanes();
				pane = &panes->activePane();
			}
			return pane->selectedBuffer().self();
		}

		void selectBuffer(EditorPane& pane, boost::python::object o) {
			if(o == boost::python::object()) {
				pane.grab_focus();
				return;
			}
			if(boost::python::extract<Buffer*>(o).check())
				pane.selectBuffer(boost::python::extract<Buffer&>(o));
			else if(boost::python::extract<EditorView&>(o).check())
				pane.selectBuffer(boost::python::extract<EditorView&>(o));
			else {
				::PyErr_BadArgument();
				boost::python::throw_error_already_set();
			}
		}
	}

	ALPHA_EXPOSE_PROLOGUE(2)
		boost::python::scope scope(ambient::Interpreter::instance().toplevelPackage());

		boost::python::class_<EditorPane, boost::noncopyable>("_Window", boost::python::no_init)
			.add_property("current_buffer", &currentBuffer)
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
			.def("delete", &EditorPanes::remove, boost::python::arg("pane") = nullptr)
			.def("delete_others", &EditorPanes::removeOthers, boost::python::arg("pane") = nullptr, boost::python::arg("root") = nullptr);

		boost::python::def("current_buffer", &currentBuffer, boost::python::arg("pane_or_panes") = boost::python::object());

		boost::python::def("selected_window", ambient::makeFunctionPointer([]() {
			return editorPanes().activePane().self();
		}));

		boost::python::def("windows", ambient::makeFunctionPointer([]() {
			return editorPanes().self();
		}));
	ALPHA_EXPOSE_EPILOGUE()
}
