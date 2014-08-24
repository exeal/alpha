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
#include <boost/foreach.hpp>
#include <boost/range/algorithm/find.hpp>

namespace ascension {
	namespace viewers {
		namespace widgetapi {
			// MimeDataFormats ////////////////////////////////////////////////////////////////////////////////////////

			/**
			 * Constructor.
			 * @param targets The list of targets returned by @c Gdk#DragContext#list_targets method
			 */
			MimeDataFormats::MimeDataFormats(std::vector<std::string>&& targets) : targets_(targets) {
			}

			std::list<MimeDataFormats::Format>&& MimeDataFormats::formats() const {
				return std::list<MimeDataFormats::Format>(std::begin(targets_), std::end(targets_));
			}

			bool MimeDataFormats::hasFormat(Format format) const BOOST_NOEXCEPT {
				return boost::find(targets_, format) != boost::end(targets_);
			}

			bool MimeDataFormats::hasText() const BOOST_NOEXCEPT {
				return boost::find(targets_, "text/plain") != boost::end(targets_);
			}

			bool MimeDataFormats::hasURIs() const BOOST_NOEXCEPT {
				return boost::find(targets_, "text/uri-list") != boost::end(targets_);
			}


			// MimeData ///////////////////////////////////////////////////////////////////////////////////////////////

			MimeData::MimeData() : impl_(std::make_shared<Gtk::SelectionData>()) {
			}

			MimeData::MimeData(Gtk::SelectionData& impl) : impl_(&impl, boost::null_deleter()) {
			}

			std::vector<std::uint8_t>&& MimeData::data(Format format) const {
				if(format != impl_->get_target())
					throw 0;
				const guchar* const p = impl_->get_data();
				return std::vector<std::uint8_t>(p, p + impl_->get_length());
			}

			std::list<MimeDataFormats::Format>&& MimeData::formats() const {
				const std::vector<std::string> targets(impl_->get_targets());
				return std::list<MimeDataFormats::Format>(std::begin(targets), std::end(targets));
			}

			bool MimeData::hasFormat(Format format) const BOOST_NOEXCEPT {
				try {
					const std::vector<std::string> targets(impl_->get_targets());
					return boost::find(targets, format) != boost::end(targets);
				} catch(...) {
					return false;
				}
			}

			bool MimeData::hasText() const BOOST_NOEXCEPT {
				return impl_->targets_include_text();
			}

			bool MimeData::hasURIs() const BOOST_NOEXCEPT {
				return impl_->targets_include_uri();
			}

			void MimeData::setData(Format format, const boost::iterator_range<const std::uint8_t*>& range) {
				impl_->set(format, 8, range.begin(), range.size());
//				impl_->set(::gdk_atom_intern(format.c_str(), true), 8, range.begin(), range.size());
			}

			void MimeData::setText(const StringPiece& text) {
				impl_->set_text(toGlibUstring(text));
			}

			String MimeData::text() const {
				return fromGlibUstring(impl_->get_text());
			}


			// DragContext ////////////////////////////////////////////////////////////////////////////////////////////

			DropAction DragContext::defaultAction() const {
				if(!context_)
					throw IllegalStateException("");
				return context_->get_suggested_action();
			}

			DropAction DragContext::execute(DropAction supportedActions, int mouseButton, GdkEvent* event) {
				if(mimeData_.get() == nullptr)
					throw IllegalStateException("DragContext.setMimeData is not called.");
				const std::list<MimeData::Format> formats(mimeData_->formats());
				std::vector<Gtk::TargetEntry> targetEntries;
				BOOST_FOREACH(auto format, formats)
					targetEntries.push_back(Gtk::TargetEntry(format));
				Glib::RefPtr<Gtk::TargetList> targets(Gtk::TargetList::create(targetEntries));
				Glib::RefPtr<Gdk::DragContext> context(source_.drag_begin(targets, supportedActions, mouseButton, event));
				return context->get_selected_action();
			}

			void DragContext::setImage(const graphics::Image& image, const graphics::geometry::BasicPoint<uint32_t>& hotspot) {
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

			DropAction DragContext::supportedActions() const {
				if(!context_)
					throw IllegalStateException("");
				return context_->get_actions();
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
