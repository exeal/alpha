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
		if(initialViewer.get() != nullptr)
			add(std::move(initialViewer));
	}

	/// Copy-constructor.
	EditorPane::EditorPane(const EditorPane& other) {
		BOOST_FOREACH(const std::unique_ptr<EditorView>& p, other.viewers_) {
			std::unique_ptr<EditorView> newView(new EditorView(*p));
//			const bool succeeded = newViewer->create(p->getParent().use(), win32::ui::DefaultWindowRect(),
//				WS_CHILD | WS_CLIPCHILDREN | WS_HSCROLL | WS_VISIBLE | WS_VSCROLL, WS_EX_CLIENTEDGE);
//			assert(succeeded);
			newView->setConfiguration(&p->configuration(), 0, true);
			newView->textRenderer().viewport()->scrollTo(p->textRenderer().viewport()->scrollPositions());
			add(std::move(newView));
		}
	}

	/**
	 * Adds the new viewer.
	 * @param viewer the viewer to add
	 * @param select If set to @c true, @a viewer is selected automatically
	 * @throw ascension#NullPointerException @a viewer is @c null
	 */
	void EditorPane::add(std::unique_ptr<EditorView> viewer, bool select /* = true */) {
		if(viewer.get() == nullptr)
			throw ascension::NullPointerException("viewer");
		if(viewers_.empty()) {
			viewers_.push_front(std::move(viewer));
			this->select(*viewers_.front());
		} else
			viewers_.push_back(std::move(viewer));
	}

	/**
	 * Removes the specified viewer from this @c EditorPane.
	 * @param viewer The viewer to remove
	 * @throw ascension#NoSuchElementException @a viewer is not exist
	 */
	void EditorPane::remove(const EditorView& viewer) {
		try {
			return removeBuffer(viewer.document());
		} catch(const ascension::NoSuchElementException&) {
			throw ascension::NoSuchElementException("viewer");
		}
	}

	/// Removes all viewers from this @c EditorPane.
	void EditorPane::removeAll() BOOST_NOEXCEPT {
		viewers_.clear();
	}

	/**
	 * Removes the viewer belongs to the specified buffer from this @c EditorPane.
	 * @param buffer The buffer has the viewer to remove
	 * @throw ascension#NoSuchElementException @a buffer is not exist
	 */
	void EditorPane::removeBuffer(const Buffer& buffer) {
		if(!viewers_.empty()) {
			auto i(std::begin(viewers_));
			if(&(*(i++))->document() == &buffer) {
				if(i != std::end(viewers_))
					select(**i);
				viewers_.erase(--i);
				return;
			} else {
				for(const auto e(std::end(viewers_)); i != e; ++i) {
					if(&(*i)->document() == &buffer) {
						viewers_.erase(i);
						return;
					}
				}
			}
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
			return selectBuffer(viewer.document());
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
		const bool hadFocus = !viewers_.empty() && selectedView().has_focus();

		if(viewers_.size() > 1) {
			// bring to the front of the list
			const auto e(std::end(viewers_));
			for(auto i(std::next(std::begin(viewers_))); i != e; ++i) {
				if(&(*i)->document() == &buffer) {
					std::unique_ptr<EditorView> temp(std::move(*i));
					viewers_.erase(i);
					viewers_.push_front(std::move(temp));
				}
			}
		}

		// show and focus
		bool found = false;
		BOOST_FOREACH(const std::unique_ptr<EditorView>& viewer, viewers_) {
			if(&viewer->document() == &buffer) {
				viewer->show();
				if(hadFocus)
					viewer->grab_focus();
				found = true;
			}
		}
		if(!found)
			throw ascension::NoSuchElementException("buffer");

		// hide the others
		BOOST_FOREACH(const std::unique_ptr<EditorView>& viewer, viewers_) {
			if(&viewer->document() != &buffer)
				viewer->hide();
		}
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
