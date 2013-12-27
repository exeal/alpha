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

namespace {
}

void TextViewer::initializeNativeObjects(const TextViewer* other) {
	set_can_focus(true);
	set_redraw_on_allocate(false);
//	drag_dest_set_target_list();
	property_hadjustment().signal_changed().connect([]() {
	});
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

/// @see Gtk#Widget#on_realize
void TextViewer::on_realize() {
	set_realized();

	const Gtk::Allocation allocation(get_allocation());
	GdkWindowAttr attributes;
	memset(&attributes, 0, sizeof(decltype(attributes)));
	static const gint attributesMask = GDK_WA_X | GDK_WA_Y/* | GDK_WA_VISUAL*/;
	attributes.x = allocation.get_x();
	attributes.y = allocation.get_y();
	attributes.width = allocation.get_width();
	attributes.height = allocation.get_height();
	attributes.event_mask = get_events() | Gdk::EXPOSURE_MASK;
//	attributes.visual = ::gtk_widget_get_visual(gobj());
	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.wclass = GDK_INPUT_OUTPUT;
	const Glib::RefPtr<Gdk::Window> window(Gdk::Window::create(get_parent_window(), &attributes, attributesMask));
	set_window(window);

	assert(!rulerWindow_);
	attributes.event_mask |= Gdk::SCROLL_MASK | Gdk::SMOOTH_SCROLL_MASK
		| Gdk::KEY_PRESS_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK
		| Gdk::POINTER_MOTION_MASK | Gdk::POINTER_MOTION_HINT_MASK;
	rulerWindow_ = Gdk::Window::create(window, &attributes, attributesMask);
	rulerWindow_->show();
#if 0
	register_window(rulerWindow_);
#else
	rulerWindow_->set_user_data(Gtk::Widget::gobj());
#endif

	assert(!textAreaWindow_);
//	attributes.event_mask = Gdk::
	textAreaWindow_ = Gdk::Window::create(window, &attributes, attributesMask);
	textAreaWindow_->show();
#if 0
	register_window(textAreaWindow_);
#else
	rulerWindow_->set_user_data(Gtk::Widget::gobj());
#endif
}

/// @see Gtk#Widget#on_size_allocate
void TextViewer::on_size_allocate(Gtk::Allocation& allocation) {
	set_allocation(allocation);
	if(textAreaWindow_ && get_realized()) {
		graphics::Rectangle r(textAreaAllocationRectangle());
		textAreaWindow_->move_resize(
			static_cast<int>(graphics::geometry::left(r)), static_cast<int>(graphics::geometry::top(r)),
			static_cast<int>(graphics::geometry::dx(r)), static_cast<int>(graphics::geometry::dy(r)));

		const RulerStyles& rulerStyles = rulerPainter_->declaredStyles();
		r = boost::geometry::make_zero<graphics::Rectangle>();
		if(indicatorMargin(rulerStyles)->visible)
			boost::geometry::union_(r, rulerPainter_->indicatorMarginAllocationRectangle(), r);
		if(lineNumbers(rulerStyles)->visible)
			boost::geometry::union_(r, rulerPainter_->lineNumbersAllocationRectangle(), r);
		textAreaWindow_->move_resize(
			static_cast<int>(graphics::geometry::left(r)), static_cast<int>(graphics::geometry::top(r)),
			static_cast<int>(graphics::geometry::dx(r)), static_cast<int>(graphics::geometry::dy(r)));
	}
}

/// @see Gtk#Widget#on_unrealize
void TextViewer::on_unrealize() {
	textAreaWindow_.reset();
	rulerWindow_.reset();
	Gtk::Widget::on_unrealize();
}

#endif	// ASCENSION_WINDOW_SYSTEM_GTK
