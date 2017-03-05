/**
 * @file editor-pane.cpp
 * @author exeal
 * @date 2008 Separated from buffer.cpp
 * @date 2008-2009, 2014 was editor-window.cpp
 * @date 2014-08-24 Renamed from editor-window.cpp
 */

#include "application.hpp"
#include "buffer-list.hpp"
#include "editor-pane.hpp"
#include "editor-view.hpp"
#include "function-pointer.hpp"
#include <ascension/graphics/font/text-viewport.hpp>
#include <ascension/log.hpp>
#include <ascension/viewer/text-area.hpp>
#include <boost/core/addressof.hpp>
#include <boost/foreach.hpp>


namespace alpha {
	/**
	 * @typedef alpha::EditorPanes::BufferSelectionChangedSignal
	 * The signal which gets emitted when the active buffer was switched.
	 * @param editorPanes The editor panes widget
	 * @see #bufferSelectionChangedSignal, #selectedBuffer;
	 */

	/// Constructor.
	EditorPane::EditorPane(std::unique_ptr<EditorView> initialViewer /* = std::unique_ptr<EditorView> */) {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
		set_homogeneous(false);
#endif
		if(initialViewer.get() != nullptr)
			add(std::move(initialViewer));
	}

	/// Copy-constructor.
	EditorPane::EditorPane(const EditorPane& other) {
		BOOST_FOREACH(const Child& child, other.children_) {
			const EditorView& src = *child.viewer;
			std::unique_ptr<EditorView> newView(new EditorView(src));
			newView->setConfiguration(src.configuration(), true);
			newView->textArea()->viewport()->scrollTo(src.textArea()->viewport()->scrollPositions());
			add(std::move(newView));
		}
	}

	/**
	 * Adds the new viewer.
	 * @param viewer The viewer to add
	 * @throw ascension#NullPointerException @a viewer is @c null
	 */
	void EditorPane::add(std::unique_ptr<EditorView> viewer) {
		if(viewer.get() == nullptr)
			throw ascension::NullPointerException("viewer");

#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
		std::ostringstream oss;
		oss << std::hex << reinterpret_cast<std::uintptr_t>(boost::addressof(*viewer));
		const Glib::ustring name(oss.str());	// dummy
		std::unique_ptr<Gtk::Box> container(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
		std::unique_ptr<Gtk::ScrolledWindow> scroller(new Gtk::ScrolledWindow());
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		std::unique_ptr<Container> container(new Container);
#endif
//		std::unique_ptr<ModeLine> modeLine(new ModeLine());

		children_.push_back(Child());
		Child& newChild = children_.back();
		try {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			box->pack_start(*scroller, Gtk::PACK_EXPAND_WIDGET);
			newChild.scroller = Glib::RefPtr<Gtk::ScrolledWindow>(scroller.release());
			newChild.scroller->add(*viewer);
#endif
			newChild.viewer = std::move(viewer);
//			box->pack_end(*modeLine, Gtk::PACK_SHRINK);
		} catch(...) {
			children_.pop_back();
			throw;
		}
		newChild.container = std::move(container);
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
		Gtk::Stack::add(*Gtk::manage(container.release()), name);
#else
		ascension::viewer::widgetapi::setParentWidget(*container, *this);
#endif
//		if(children_.size() == 1)
//			this->select(*children_.front().viewer);
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
		show_all_children();
#else
		ascension::viewer::widgetapi::show(*container);
#endif
		ascension::viewer::widgetapi::show(*this);
	}

	/**
	 * Removes the specified viewer from this @c EditorPane.
	 * @param viewer The viewer to remove
	 * @throw ascension#NoSuchElementException @a viewer is not exist
	 */
	void EditorPane::remove(const EditorView& viewer) {
		try {
			return removeBuffer(*viewer.document());
		} catch(const ascension::NoSuchElementException&) {
			throw ascension::NoSuchElementException("viewer");
		}
	}

	/// Removes all viewers from this @c EditorPane.
	void EditorPane::removeAll() BOOST_NOEXCEPT {
		BOOST_FOREACH(Child& child, children_)
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			Gtk::Stack::remove(*child.container);
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			::SetParent(child.container->handle().get(), nullptr);
#endif
		children_.clear();
	}

	/**
	 * Removes the viewer belongs to the specified buffer from this @c EditorPane.
	 * @param buffer The buffer has the viewer to remove
	 * @throw ascension#NoSuchElementException @a buffer is not exist
	 */
	void EditorPane::removeBuffer(const Buffer& buffer) {
		if(!children_.empty()) {
			auto i(std::begin(children_));
			if((i++)->viewer->document().get() == &buffer) {
				if(i != std::end(children_))
					select(*i->viewer);
				--i;
			} else {
				for(const auto e(std::end(children_)); i != e; ++i) {
					if(i->viewer->document().get() == &buffer)
						break;
				}
			}
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			Gtk::Stack::remove(*i->viewer);
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			::SetParent(i->viewer->handle().get(), nullptr);
#endif
			children_.erase(i);
			return;
		}
		throw ascension::NoSuchElementException("buffer");
	}

	/**
	 * Shows the specified viewer in this @c EditorPane.
	 * @param viewer The viewer to show
	 * @throw ascension#NoSuchElementException @a viewer is not exist
	 */
	void EditorPane::select(const EditorView& viewer) {
		try {
			return selectBuffer(*viewer.document());
		} catch(const ascension::NoSuchElementException&) {
			throw ascension::NoSuchElementException("viewer");
		}
	}

	/**
	 * Shows the specified buffer in this @c EditorPane.
	 * @param buffer The buffer to show
	 * @throw ascension#NoSuchElementException @a buffer is not exist
	 */
	void EditorPane::selectBuffer(const Buffer& buffer) {
		const bool hadFocus = !children_.empty() && ascension::viewer::widgetapi::hasFocus(selectedView());

		if(children_.size() > 1) {
			// bring to the front of the list
			const auto e(std::end(children_));
			for(auto i(std::next(std::begin(children_))); i != e; ++i) {
				if(i->viewer->document().get() == &buffer) {
					Child temp(std::move(*i));
					children_.erase(i);
					children_.push_front(std::move(temp));
				}
			}
		}

		// show and focus
		bool found = false;
		BOOST_FOREACH(const Child& child, children_) {
			auto& container = *child.container;
			const std::unique_ptr<EditorView>& viewer = child.viewer;
			if(viewer->document().get() == &buffer) {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				ascension::viewer::widgetapi::show(container);
				assert(ascension::viewer::widgetapi::isVisible(container));
				set_visible_child(container);
				container.show_all_children();
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				setCurrentWidget(container);
#endif
				if(hadFocus)
					ascension::viewer::widgetapi::setFocus(*viewer);
				found = true;
			}
		}
		if(!found)
			throw ascension::NoSuchElementException("buffer");

//		// hide the others
//		BOOST_FOREACH(const std::unique_ptr<EditorView>& viewer, viewers_) {
//			if(&viewer->document() != &buffer)
//				viewer->hide();
//		}
	}

	/// Returns the selected buffer.
	Buffer& EditorPane::selectedBuffer() {
		return *selectedView().document();
	}

	/// Returns the selected buffer.
	const Buffer& EditorPane::selectedBuffer() const {
		return *selectedView().document();
	}

#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
	EditorPane::Container::Container() : ascension::win32::CustomControl<Container>(ascension::win32::Window::WIDGET) {
	}

	LRESULT EditorPane::Container::processMessage(UINT message, WPARAM wp, LPARAM lp, bool& consumed) {
		return 0l;
	}

	void EditorPane::Container::windowClass(ascension::win32::WindowClass& out) BOOST_NOEXCEPT {
		out.name = L"alpha.Container";
		out.styles = CS_HREDRAW | CS_VREDRAW;
	}
#endif
}
