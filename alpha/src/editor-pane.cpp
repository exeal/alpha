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
	 * @see #bufferSelectionChangedSignal, #selectedBuffer
	 */

	/// Constructor.
	EditorPane::EditorPane(std::unique_ptr<EditorView> initialViewer /* = std::unique_ptr<EditorView> */) {
		set_homogeneous(false);
		if(initialViewer.get() != nullptr)
			add(std::move(initialViewer));
	}

	/// Copy-constructor.
	EditorPane::EditorPane(const EditorPane& other) {
		BOOST_FOREACH(const Child& child, other.children_) {
			const EditorView& src = *std::get<2>(child);
			std::unique_ptr<EditorView> newView(new EditorView(src));
//			const bool succeeded = newViewer->create(p->getParent().use(), win32::ui::DefaultWindowRect(),
//				WS_CHILD | WS_CLIPCHILDREN | WS_HSCROLL | WS_VISIBLE | WS_VSCROLL, WS_EX_CLIENTEDGE);
//			assert(succeeded);
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
		std::ostringstream oss;
		oss << std::hex << reinterpret_cast<std::uintptr_t>(boost::addressof(*viewer));
		const Glib::ustring name(oss.str());	// dummy

		std::unique_ptr<Gtk::Box> box(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
		std::unique_ptr<Gtk::ScrolledWindow> scroller(new Gtk::ScrolledWindow());
//		std::unique_ptr<ModeLine> modeLine(new ModeLine());

		children_.push_back(Child());
		Child& newChild = children_.back();
		try {
			box->pack_start(*scroller, Gtk::PACK_EXPAND_WIDGET);
			std::get<1>(newChild) = Glib::RefPtr<Gtk::ScrolledWindow>(scroller.release());
			std::get<1>(newChild)->add(*viewer);
			std::get<2>(newChild) = std::move(viewer);
//			box->pack_end(*modeLine, Gtk::PACK_SHRINK);
		} catch(...) {
			children_.pop_back();
			throw;
		}
		std::get<0>(newChild) = box.get();
		Gtk::Stack::add(*Gtk::manage(box.release()), name);
//		if(children_.size() == 1)
//			this->select(*std::get<2>(children_.front()));
		/*std::get<0>(newChild)->*/show_all_children();
		/*std::get<0>(newChild)->*/show();
	}

#ifdef _DEBUG
	bool EditorPane::on_event(GdkEvent* event) {
//		ASCENSION_LOG_TRIVIAL(debug)
//			<< "allocation = " << get_allocated_width() << "x" << get_allocated_height() << std::endl;
//		if(event != nullptr)
//			ASCENSION_LOG_TRIVIAL(debug) << event->type << std::endl;
		return Gtk::Stack::on_event(event);
	}

	void EditorPane::on_realize() {
		return Gtk::Stack::on_realize();
	}

	void EditorPane::on_size_allocate(Gtk::Allocation& allocation) {
		if(Gtk::Widget* const child = get_visible_child())
			child->size_allocate(allocation);
		return Gtk::Stack::on_size_allocate(allocation);
	}
#endif

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
			Gtk::Stack::remove(*std::get<0>(child));
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
			if(std::get<2>(*(i++))->document().get() == &buffer) {
				if(i != std::end(children_))
					select(*std::get<2>(*i));
				--i;
			} else {
				for(const auto e(std::end(children_)); i != e; ++i) {
					if(std::get<2>(*i)->document().get() == &buffer)
						break;
				}
			}
			Gtk::Stack::remove(*std::get<2>(*i));
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
		const bool hadFocus = !children_.empty() && selectedView().has_focus();

		if(children_.size() > 1) {
			// bring to the front of the list
			const auto e(std::end(children_));
			for(auto i(std::next(std::begin(children_))); i != e; ++i) {
				if(std::get<2>(*i)->document().get() == &buffer) {
					Child temp(std::move(*i));
					children_.erase(i);
					children_.push_front(std::move(temp));
				}
			}
		}

		// show and focus
		bool found = false;
		BOOST_FOREACH(const Child& child, children_) {
			Gtk::Box& box = *std::get<0>(child);
			const std::unique_ptr<EditorView>& viewer = std::get<2>(child);
			if(viewer->document().get() == &buffer) {
				box.show();
				assert(box.get_visible());
				set_visible_child(box);
				box.show_all_children();
				if(hadFocus)
					viewer->grab_focus();
				found = true;
#ifdef _DEBUG
				bool visible = get_visible();
				bool has_window = get_has_window();
				int width = get_width(), height = get_height();
				visible = std::get<0>(child)->get_visible();
				has_window = std::get<0>(child)->get_has_window();
				width = std::get<0>(child)->get_width();
				height = std::get<0>(child)->get_height();
				visible = std::get<1>(child)->get_visible();
				has_window = std::get<1>(child)->get_has_window();
				width = std::get<1>(child)->get_width();
				height = std::get<1>(child)->get_height();
				visible = viewer->get_visible();
				has_window = viewer->get_has_window();
				width = viewer->get_width();
				height = viewer->get_height();
#endif
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

	/**
	 * Splits this pane.
	 * @see #splitSideBySide
	 */
	void EditorPane::split() {
		return split(Gtk::ORIENTATION_VERTICAL);
	}

	/**
	 * Splits this pane side-by-side.
	 * @see #split
	 */
	void EditorPane::splitSideBySide() {
		return split(Gtk::ORIENTATION_HORIZONTAL);
	}
}
