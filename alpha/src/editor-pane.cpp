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
#include "editor-panes.hpp"
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

	/// Creates a @c EditorPane instance.
	EditorPane::EditorPane()
#if ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
			: QStackedWidget(EditorPanes::instance())
#endif
	{
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
		pack_start(stack_, Gtk::PACK_EXPAND_WIDGET);
		pack_end(modeLine_, Gtk::PACK_SHRINK);
		stack_.set_homogeneous(false);
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
		std::unique_ptr<QVBoxLayout> layout(new QVBoxLayout);
		layout->addWidget(&stack_);
		layout->addWidget(&modeLine_, -1);
		setLayout(layout.release());
#endif
	}

	/**
	 * Adds the new viewer.
	 * @param viewer The viewer to add
	 * @throw ascension#IllegalStateException This EditorPane is not realized
	 * @throw ascension#NullPointerException @a viewer is @c null
	 */
	void EditorPane::add(std::unique_ptr<EditorView> viewer) {
		if(!ascension::viewer::widgetapi::isRealized(*this) || !ascension::viewer::widgetapi::isRealized(stack_))
			throw ascension::IllegalStateException("This EditorPane is not realized.");
		if(viewer.get() == nullptr)
			throw ascension::NullPointerException("viewer");

#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
		Gtk::ScrolledWindow scroller;
		scroller.add(*viewer);
		std::ostringstream oss;
		oss << std::hex << reinterpret_cast<std::uintptr_t>(boost::addressof(*viewer));
		const Glib::ustring name(oss.str());	// dummy
		stack_.add(*Gtk::manage(scroller), name);
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
		stack_.addWidget(viewer.get());
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		stack_.addWidget(*viewer);
#endif
		viewers_.push_back(std::move(viewer));
		ascension::viewer::widgetapi::show(viewers_.back());
	}

	/**
	 * Clones this object.
	 * @return A cloned object
	 */
	std::unique_ptr<EditorPane> EditorPane::clone() const {
		// TODO: Not implemented.
		std::unique_ptr<EditorPane> p(new EditorPane());
		return p;
	}

#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
	/// @see ascension::win32::CustomControl#realized
	void EditorPane::realized(const Type& type) {
		win32::VerticalContainer::realized(type);
		stack_.setHorizontallyHomogeneous(false);
		stack_.setVerticallyHomogeneous(false);
		ascension::win32::realize(stack_, ascension::win32::Window::Type::widget(handle()));
//		ascension::win32::realize(modeLine_, ascension::win32::Window::Type::widget(handle()));
		pushBack(std::shared_ptr<ascension::win32::Window>(&stack_, boost::null_deleter()), std::make_pair(win32::Container::EXPAND_WIDGET, win32::Container::FILL));
//		pushBack(std::shared_ptr<ascension::win32::Window>(&modeLine_, boost::null_deleter()), std::make_pair(win32::Container::SHRINK, win32::Container::FILL));
		BOOST_FOREACH(auto& viewer, viewers_)
			ascension::win32::realize(*viewer, ascension::win32::Window::Type::widget(handle()));
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
		BOOST_FOREACH(auto& viewer, viewers_)
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			stack_.remove(*viewer_);
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			::SetParent(viewer->handle().get(), nullptr);
#endif
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
			if((*(i++))->document().get() == &buffer) {
				if(i != std::end(viewers_))
					select(**i);
				--i;
			} else {
				for(const auto e(std::end(viewers_)); i != e; ++i) {
					if((*i)->document().get() == &buffer)
						break;
				}
			}
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			stack_.remove(**i);
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			::SetParent((*i)->handle().get(), nullptr);
#endif
			viewers_.erase(i);
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
		const bool hadFocus = !viewers_.empty() && ascension::viewer::widgetapi::hasFocus(selectedView());

		if(viewers_.size() > 1) {
			// bring to the front of the list
			const auto e(std::end(viewers_));
			for(auto i(std::next(std::begin(viewers_))); i != e; ++i) {
				if((*i)->document().get() == &buffer) {
					auto temp(*i);
					viewers_.erase(i);
					viewers_.push_front(temp);
				}
			}
		}

		// show and focus
		bool found = false;
		BOOST_FOREACH(const auto& viewer, viewers_) {
			if(viewer->document().get() == &buffer) {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				ascension::viewer::widgetapi::show(container);
				assert(ascension::viewer::widgetapi::isVisible(container));
				set_visible_child(container);
				container.show_all_children();
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				stack_.setCurrentWidget(*viewer);
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
}
