/**
 * @file viewer-gtk.cpp
 * @author exeal
 * @date 2013-11-10 Created.
 */

#include <ascension/viewer/viewer.hpp>
#ifdef ASCENSION_WINDOW_SYSTEM_GTK

#include <ascension/viewer/widgetapi/drag-and-drop.hpp>

using namespace ascension;
using namespace ascension::viewers;
using namespace std;


detail::TextViewerScrollableProperties::TextViewerScrollableProperties() :
		horizontalAdjustment_(*this, "hadjustment"), verticalAdjustment_(*this, "vadjustment"),
		horizontalScrollPolicy_(*this, "hscroll-policy", Gtk::SCROLL_NATURAL), verticalScrollPolicy_(*this, "vscroll-policy", Gtk::SCROLL_NATURAL) {
}

void TextViewer::doBeep() BOOST_NOEXCEPT {
#if 1
	::gdk_beep();
#else
	get_display()->beep();
#endif
}

/// Implements @c Gtk#Widget#get_preferred_height_for_width_vfunc
void TextViewer::get_preferred_height_for_width_vfunc(int, int& minimumHeight, int& naturalHeight) const {
	return get_preferred_height_vfunc(minimumHeight, naturalHeight);
}

/// Implements @c Gtk#Widget#get_preferred_height_vfunc
void TextViewer::get_preferred_height_vfunc(int& minimumHeight, int& naturalHeight) const {
	// TODO: Not implemented.
}

/// Implements @c Gtk#Widget#get_preferred_width_for_height_vfunc
void TextViewer::get_preferred_width_for_height_vfunc(int, int& minimumWidth, int& naturalWidth) const {
	return get_preferred_width_vfunc(minimumWidth, naturalWidth);
}

/// Implements @c Gtk#Widget#get_preferred_width_vfunc
void TextViewer::get_preferred_width_vfunc(int& minimumWidth, int& naturalWidth) const {
	// TODO: Not implemented.
}

/// Implements @c Gtk#Widget#get_request_mode_vfunc
Gtk::SizeRequestMode TextViewer::get_request_mode_vfunc() const {
	return Gtk::Widget::get_request_mode_vfunc();
}

void TextViewer::initializeNativeObjects(const TextViewer* other) {
//	drag_dest_set_target_list();
	property_hadjustment().signal_changed().connect();
}

/// @see Gtk#Widget#on_drag_drop
bool TextViewer::on_drag_drop(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, guint time) {
	if(mouseInputStrategy_.get() != nullptr) {
		if(const shared_ptr<widgetapi::DropTarget> dropTarget = mouseInputStrategy_->handleDropTarget()) {
			detail::DragEventAdapter adapter(*dropTarget);
			return adapter.adaptDropEvent(context, x, y, time);
		}
	}
	return false;
}

/// @see Gtk#Widget#on_drag_leave
void TextViewer::on_drag_leave(const Glib::RefPtr<Gdk::DragContext>& context, guint time) {
	if(mouseInputStrategy_.get() != nullptr) {
		if(const shared_ptr<widgetapi::DropTarget> dropTarget = mouseInputStrategy_->handleDropTarget()) {
			detail::DragEventAdapter adapter(*dropTarget);
			return adapter.adaptDragLeaveEvent(context, time);
		}
	}
}

/// @see Gtk#Widget#on_drag_motion
bool TextViewer::on_drag_motion(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, guint time) {
	if(mouseInputStrategy_.get() != nullptr) {
		if(const shared_ptr<widgetapi::DropTarget> dropTarget = mouseInputStrategy_->handleDropTarget()) {
			detail::DragEventAdapter adapter(*dropTarget);
			return adapter.adaptDragMoveEvent(context, x, y, time);
		}
	}
	return false;
}

#endif	// ASCENSION_WINDOW_SYSTEM_GTK
