/**
 * @file drag-and-drop-gtk.cpp
 * Implements 
 * @author exeal
 * @date 2013-2014
 * @date 2013-11-04 Created.
 */

#include <ascension/viewer/widgetapi/drag-and-drop.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)

#include <ascension/graphics/image.hpp>
#include <ascension/graphics/geometry/point-xy.hpp>
#include <boost/foreach.hpp>

namespace ascension {
	namespace viewer {
		namespace widgetapi {
			DropAction DragContext::defaultAction() const {
				if(!context_)
					throw IllegalStateException("");
				return context_->get_suggested_action();
			}

			DropAction DragContext::execute(const DropActions& supportedActions, int mouseButton, GdkEvent* event) {
				if(mimeData_.get() == nullptr)
					throw IllegalStateException("DragContext.setMimeData is not called.");
				std::vector<MimeData::Format> formats;
				mimeData_->formats(formats);
				std::vector<Gtk::TargetEntry> targetEntries;
				BOOST_FOREACH(auto format, formats)
					targetEntries.push_back(Gtk::TargetEntry(format));
				Glib::RefPtr<Gtk::TargetList> targets(Gtk::TargetList::create(targetEntries));
				Glib::RefPtr<Gdk::DragContext> context(source_.drag_begin(targets, supportedActions, mouseButton, event));
				return context->get_selected_action();
			}

			void DragContext::setImage(const graphics::Image& image, const boost::geometry::model::d2::point_xy<uint32_t>& hotspot) {
#if ASCENSION_SELECTS_GRAPHICS_SYSTEM(CAIRO)
				Glib::RefPtr<Gdk::Pixbuf> icon(Gdk::Pixbuf::create(image.asNativeObject(), 0, 0, image.width(), image.height()));
#else
				Glib::RefPtr<Gdk::Pixbuf> icon(Gdk::Pixbuf::create_from_data(
					image.pixels().begin(), Gdk::COLORSPACE_RGB, true, image.depth(), image.width(), image.height(), image.stride()));
#endif
				if(!icon)
					throw makePlatformError();
				if(!context_) {
					icon_ = icon;
					iconHotspotX_ = graphics::geometry::x(hotspot);
					iconHotspotY_ = graphics::geometry::y(hotspot);
				} else
					context_->set_icon(icon, graphics::geometry::x(hotspot), graphics::geometry::y(hotspot));
			}

			void DragContext::setMimeData(std::shared_ptr<const MimeData> data) {
				mimeData_ = data;
			}

			DropActions DragContext::supportedActions() const {
				if(!context_)
					throw IllegalStateException("");
				return fromNative<DropActions>(context_->get_actions());
			}
		}

		namespace detail {
			void DragEventAdapter::adaptDragLeaveEvent(const Glib::RefPtr<Gdk::DragContext>& context, guint time) {
				// TODO: Not implemented.
			}

			bool DragEventAdapter::adaptDragMoveEvent(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, guint time) {
				return false;	// TODO: Not implemented.
			}

			bool DragEventAdapter::adaptDropEvent(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, guint time) {
				return false;	// TODO: Not implemented.
			}
		}
	}
}

#endif	// ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
