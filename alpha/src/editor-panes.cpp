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
#include "editor-pane.hpp"
#include "editor-panes.hpp"
#include "editor-view.hpp"
#include "function-pointer.hpp"
#include "ui/main-window.hpp"
#include <ascension/log.hpp>
#include <boost/foreach.hpp>
//#include <boost/range/algorithm/find.hpp>

namespace alpha {
	namespace detail {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
		Paned::Paned(Paned* parentPaned) : Gtk::Paned(Gtk::ORIENTATION_HORIZONTAL), parent_(parentPaned)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
		Paned::Paned(Paned* parentPaned, QWidget* parent /* = Q_NULLPTR */) : QSplitter(Qt::Horizontal, parent), parent_(parentPaned)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		Paned::Paned(Paned* parentPaned) : parent_(parentPaned)
#endif
		{}
	}

	/**
	 * @typedef alpha::EditorPanes::BufferSelectionChangedSignal
	 * The signal which gets emitted when the buffer selection was changed.
	 * @param editorPanes The editor panes
	 * @see #selectedBuffer, BufferList#BufferSelectionChangedSignal
	 */

	/// Default constructor.
#if ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
	EditorPanes::EditorPanes(QWidget* parent /* = Q_NULLPTR */) : detail::Paned(std::weak_ptr<detail::Paned>(), parent)
#else
	EditorPanes::EditorPanes() : detail::Paned(nullptr)
#endif
	{
#if !ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		initializeWidget();
#endif
	}

	/// Returns the iterator addresses the first editor pane.
	EditorPanes::iterator EditorPanes::begin() BOOST_NOEXCEPT {
		return iterator::makeFirst(*this);
	}

	/// Returns the iterator addresses the first editor pane.
	EditorPanes::const_iterator EditorPanes::begin() const BOOST_NOEXCEPT {
		return cbegin();
	}

	/// @see BufferList#BufferAboutToBeRemoved
	void EditorPanes::bufferAboutToBeRemoved(BufferList& buffers, Buffer& buffer) {
		// remove the specified buffer from each panes
		BOOST_FOREACH(EditorPane& pane, *this)
			pane.removeBuffer(buffer);
	}

	/// @see BufferList#BufferAdded
	void EditorPanes::bufferAdded(BufferList& buffers, std::shared_ptr<Buffer> buffer) {
		// create new views for each panes
		EditorView* originalView = nullptr;
		BOOST_FOREACH(EditorPane& pane, *this) {
			std::unique_ptr<EditorView> newView;
			if(originalView == nullptr)
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				newView.reset(new EditorView(buffer));
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				newView.reset(new EditorView(buffer, &pane));
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
				???
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				newView.reset(new EditorView(buffer, ascension::win32::Window::Type::widget(pane.handle())));
#endif
			else
				newView.reset(new EditorView(originalView->document(), ascension::win32::Window::Type::widget(pane.handle())));

			if(originalView == nullptr)
				originalView = newView.get();
			if(originalView != newView.get())
				newView->setConfiguration(originalView->configuration(), true);
			pane.add(std::move(newView));
		}
	}

	ascension::SignalConnector<EditorPanes::BufferSelectionChangedSignal> EditorPanes::bufferSelectionChangedSignal() BOOST_NOEXCEPT {
		return ascension::makeSignalConnector(bufferSelectionChangedSignal_);
	}

	/// Returns the iterator addresses the first editor pane.
	EditorPanes::const_iterator EditorPanes::cbegin() const BOOST_NOEXCEPT {
		return const_iterator::makeFirst(*this);
	}

	/// Returns the iterator addresses one past the last editor pane.
	EditorPanes::const_iterator EditorPanes::cend() const BOOST_NOEXCEPT {
		return const_iterator::makeLast(*this);
	}

	/// Returns the iterator addresses one past the last editor pane.
	EditorPanes::iterator EditorPanes::end() BOOST_NOEXCEPT {
		return iterator::makeLast(*this);
	}

	/// Returns the iterator addresses one past the last editor pane.
	EditorPanes::const_iterator EditorPanes::end() const BOOST_NOEXCEPT {
		return cend();
	}

	namespace {
		EditorPane* findFirstPane(const EditorPanes& root, bool last) {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			for(const Gtk::Paned* paned = &root; ; ) {
				auto child(!last ? paned->get_child1() : paned->get_child2());
				if(child == nullptr && last)
					child = paned->get_child1();
				if(child == nullptr)
					break;
				else if(child->get_type() != Gtk::Paned::get_type())
					return const_cast<EditorPane*>(static_cast<const EditorPane*>(child));
				paned = static_cast<const Gtk::Paned*>(child);
			}
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
			for(const QSplitter* paned = &root; ; ) {
				auto child(paned->widget(!last ? 0 : 1));
				if(child == nullptr && last)
					child = paned->widget(0);
				if(child == nullptr)
					break;
				else if(const auto* const p = qobject_cast<const QSplitter*>(child))
					paned = p;
				else
					return const_cast<EditorPane*>(static_cast<const EditorPane*>(child));
			}
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
			// TODO: Not implemented.
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			for(const win32::PanedWidget* paned = &root; ; ) {
				auto child(!last ? paned->child<0>() : paned->child<1>());
				if(child.empty() && last)
					child = paned->child<0>();
				if(child.empty())
					break;
				else if(const auto* const p = boost::any_cast<std::shared_ptr<detail::Paned>>(&child))
					paned = p->get();
				else {
					const auto* const p2 = boost::any_cast<std::shared_ptr<EditorPane>>(&child);
					assert(p2 != nullptr);
					return p2->get();
				}
			}
#endif
			return nullptr;
		}
	}

	/// @internal Returns the first editor pane, or @c null if empty.
	inline EditorPane* EditorPanes::firstPane() const {
		return findFirstPane(*this, false);
	}

	/// @see FocusChain#focus
	void EditorPanes::focus(EditorView&) {
		BOOST_FOREACH(EditorPane& pane, *this) {
			if(ascension::viewer::widgetapi::hasFocus(pane.selectedView())) {
				lastActivePane_ = activePane_;
				activePane_ = &pane;
				break;
			}
		}
	}

	/// @internal Initializes the widget.
	void EditorPanes::initializeWidget() {
		BufferList& bufferList = BufferList::instance();
		bufferAboutToBeRemovedConnection_ =
			bufferList.bufferAboutToBeRemovedSignal().connect(
				std::bind(&EditorPanes::bufferAboutToBeRemoved, this, std::placeholders::_1, std::placeholders::_2));
		bufferAddedConnection_ =
			bufferList.bufferAddedSignal().connect(
				std::bind(&EditorPanes::bufferAdded, this, std::placeholders::_1, std::placeholders::_2));
		auto firstPane(std::make_shared<EditorPane>());
		activePane_ = firstPane.get();
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
		add1(*Gtk::manage(activePane_ = new EditorPane()));
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
		addWidget(firstPane.release());
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
		// TODO: Not implemented.
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		ascension::win32::realize(*firstPane, ascension::win32::Window::Type::widget(handle()));
		resetChild<0>(firstPane);
#endif
#ifdef ALPHA_NO_AMBIENT
		BufferList::instance().addNew();
#endif
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
		show_all_children();
#endif
	}

	/// @internal Returns the last editor pane, or @c null if empty.
	inline EditorPane* EditorPanes::lastPane() const {
		return findFirstPane(*this, true);
	}

#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
	bool EditorPanes::on_focus_in_event(GdkEventFocus*) {
		EditorPane& pane = activePane();
		if(pane.get_realized())
			set_focus_child(pane);
		return true;
	}
#endif

#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
	LRESULT EditorPanes::processMessage(UINT message, WPARAM wp, LPARAM lp, bool& consumed) {
		if(message == WM_SETFOCUS) {
			ascension::viewer::widgetapi::setFocus(activePane());
			consumed = true;
			return 0l;
		}
		return detail::Paned::processMessage(message, wp, lp, consumed);
	}

	void EditorPanes::realized(const Type& type) {
		win32::PanedWidget::realized(type);
		initializeWidget();
	}
#endif

	/**
	 * Deletes the specified pane
	 * @param pane The pane to remove. If @c null, the selected pane is removed
	 * @throw ascension#NoSuchElementException @a pane is not valid
	 */
	void EditorPanes::remove(EditorPane& pane) {
	}

	/**
	 * Deletes all panes other except the specified one.
	 * @param pane
	 * @param root
	 */
	void EditorPanes::removeOthers(const EditorPane& pane) {
	}

	/// Returns the selected buffer.
	Buffer& EditorPanes::selectedBuffer() {
		return activePane().selectedBuffer();
	}

	/// Returns the selected buffer.
	const Buffer& EditorPanes::selectedBuffer() const {
		return activePane().selectedBuffer();
	}

#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
	namespace {
		template<std::size_t position>
		std::pair<win32::PanedWidget*, std::size_t> findParentPaned(win32::PanedWidget& root, EditorPane& pane) {
			if(!root.child<position>().empty()) {
				auto& child = root.child<position>();
				if(auto* const p = boost::any_cast<std::shared_ptr<EditorPane>>(&child)) {
					if(p->get() == &pane)
						return std::make_pair(&root, position);
				} else if(auto* const p = boost::any_cast<std::shared_ptr<detail::Paned>>(&child))
					return findParentPaned(**p, pane);
			}
			return std::make_pair<win32::PanedWidget*, std::size_t>(nullptr, 0u);
		}

		std::pair<win32::PanedWidget*, std::size_t> findParentPaned(win32::PanedWidget& root, EditorPane& pane) {
			auto temp(findParentPaned<0>(root, pane));
			if(std::get<0>(temp) == nullptr)
				temp = findParentPaned<1>(root, pane);
			return temp;
		}
	}
#endif

	/**
	 * @param pane
	 * @param sideBySide
	 */
	void EditorPanes::split(EditorPane& pane, bool sideBySide) {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
		const bool primary = paned->get_child1() == this;
		assert(primary || paned->get_child2() == this);
		std::unique_ptr<Gtk::Paned> newPaned(new Gtk::Paned(orientation));
		std::unique_ptr<EditorPane> newPane(new EditorPane(*this));
		panedParent->remove(*this);
		newPaned->add1(*this);
		newPaned->add2(*Gtk::manage(newPane.release()));
		if(primary)
			panedParent->add1(*Gtk::manage(newPaned.release()));
		else
			panedParent->add2(*Gtk::manage(newPaned.release()));
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		const auto paned(findParentPaned(*this, pane));
		if(std::get<0>(paned) == nullptr)
			throw std::invalid_argument("'pane' is not a descendant of this EditorPanes.");
		std::shared_ptr<EditorPane> newPane(pane.clone());
		if(std::get<1>(paned) == 0 && std::get<0>(paned)->child<1>().empty())
			std::get<0>(paned)->resetChild<1>(newPane);
		else if(std::get<1>(paned) == 1 && std::get<0>(paned)->child<0>().empty())
			std::get<0>(paned)->resetChild<0>(newPane);
		else {
			auto newPaned(std::make_shared<win32::PanedWidget>());
			ascension::win32::realize(*newPaned, ascension::win32::Window::Type::widget(std::get<0>(paned)->handle()));
			newPaned->resetChild<1>(newPane);
			std::shared_ptr<EditorPane> original(boost::any_cast<std::shared_ptr<EditorPane>>((std::get<1>(paned) == 0) ? std::get<0>(paned)->child<0>() : std::get<0>(paned)->child<1>()));
			if(std::get<1>(paned) == 0) {
				newPaned->resetChild<0>(boost::any_cast<std::shared_ptr<EditorPane>>(std::get<0>(paned)->child<0>()));
				std::get<0>(paned)->resetChild<0>(newPaned);
			} else {
				newPaned->resetChild<0>(boost::any_cast<std::shared_ptr<EditorPane>>(std::get<0>(paned)->child<1>()));
				std::get<0>(paned)->resetChild<1>(newPaned);
			}
		}
#endif
	}

	/**
	 * Split the specified pane in the @c EditorPanes.
	 * @param pane The pane to split
	 */
	void EditorPanes::split(EditorPane& pane) {
		return split(pane, false);
	}

	/**
	 * Split the specified pane in the @c EditorPanes side-by-side.
	 * @param pane The pane to split
	 */
	void EditorPanes::splitSideBySide(EditorPane& pane) {
		return split(pane, true);
	}


	// EditorPanes.InternalIterator ///////////////////////////////////////////////////////////////////////////////////

	template<typename Value>
	bool EditorPanes::InternalIterator<Value>::equal(const InternalIterator<Value>& other) const {
		return current_ == other.current_;
	}

	template<typename Value>
	typename EditorPanes::InternalIterator<Value>::iterator_facade_::reference EditorPanes::InternalIterator<Value>::dereference() const {
		if(current_ == nullptr)
			throw ascension::NoSuchElementException();
		return *current_;
	}

	template<typename Value>
	void EditorPanes::InternalIterator<Value>::increment() {
		if(current_.get() == nullptr || parent_ == nullptr)
			throw ascension::NoSuchElementException();

		const bool isConst = std::is_const<Value>::value;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
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
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		if(auto* const primary = boost::any_cast<std::shared_ptr<EditorPane>>(&parent_->child<0>())) {
			if(*primary == current_) {
				// current is the primary
				if(auto* const secondary = boost::any_cast<std::shared_ptr<EditorPane>>(&parent_->child<1>())) {
					current_ = *secondary;
					return;
				} else if(auto* const secondary = boost::any_cast<std::shared_ptr<detail::Paned>>(&parent_->child<1>())) {
					// find the leftmost under the secondary
					*this = make<0>(**secondary);
					return;
				}
			}
		}

		// find branch not reached yet
		for(auto* paned = parent_->parent(); paned != nullptr; paned = paned->parent()) {
			if(auto* const primary = boost::any_cast<std::shared_ptr<detail::Paned>>(&paned->child<0>())) {
				if(primary->get() == parent_) {
					if(auto* const secondary = boost::any_cast<std::shared_ptr<detail::Paned>>(&paned->child<1>())) {
						*this = make<0>(**secondary);
						return;
					}
				}
			}
		}
#endif
		current_ = nullptr;
	}

	template<typename Value>
	template<std::size_t branch>
	inline EditorPanes::InternalIterator<Value> EditorPanes::InternalIterator<Value>::make(Paned& root) {
		InternalIterator i;
		i.parent_ = const_cast<detail::Paned*>(&root);
		while(true) {
			if(auto* const p = boost::any_cast<std::shared_ptr<detail::Paned>>(&i.parent_->child<branch>()))
				i.parent_ = p->get();
			else if(auto* const p = boost::any_cast<std::shared_ptr<EditorPane>>(&i.parent_->child<branch>())) {
				i.current_ = *p;
				break;
			} else
				break;
		}
		return i;
	}

	template<typename Value>
	EditorPanes::InternalIterator<Value> EditorPanes::InternalIterator<Value>::makeFirst(Paned& root) {
		return make<0>(root);
	}

	template<typename Value>
	EditorPanes::InternalIterator<Value> EditorPanes::InternalIterator<Value>::makeLast(Paned& root) {
		return make<1>(root);
	}

#ifndef ALPHA_NO_AMBIENT
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
#endif // !ALPHA_NO_AMBIENT
}
