/**
 * @file drag-and-drop-gtk.cpp
 * Implements 
 * @author exeal
 * @date 2013-11-04 Created.
 */

#include <ascension/viewer/widgetapi/drag-and-drop.hpp>
#ifdef ASCENSION_WINDOW_SYSTEM_GTK

#include <ascension/graphics/image.hpp>
#include <boost/foreach.hpp>

using namespace ascension;
using namespace ascension::viewers::widgetapi;
using namespace std;


// MimeDataFormats ////////////////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param targets The list of targets returned by @c Gdk#DragContext#list_targets method
 */
MimeDataFormats::MimeDataFormats(vector<string>&& targets) : targets_(targets) {
}

list<MimeDataFormats::Format>&& MimeDataFormats::formats() const {
	return list<MimeDataFormats::Format>(begin(targets_), end(targets_));
}

bool MimeDataFormats::hasText() const BOOST_NOEXCEPT {
	return find(begin(targets_), end(targets_), "text/plain") != end(targets_);
}

bool MimeDataFormats::hasURIs() const BOOST_NOEXCEPT {
	return find(begin(targets_), end(targets_), "text/uri-list") != end(targets_);
}


// MimeData ///////////////////////////////////////////////////////////////////////////////////////

MimeData::MimeData() : impl_(make_shared<Gtk::SelectionData>()) {
}

MimeData::MimeData(Gtk::SelectionData& impl) : impl_(&impl, detail::NullDeleter()) {
}

list<MimeDataFormats::Format>&& MimeData::formats() const {
	const vector<string> targets(impl_->get_targets());
	return list<MimeDataFormats::Format>(begin(targets), end(targets));
}

bool MimeData::hasText() const BOOST_NOEXCEPT {
	return impl_->targets_include_text();
}

bool MimeData::hasURIs() const BOOST_NOEXCEPT {
	return impl_->targets_include_uri();
}

void MimeData::setText(const StringPiece& text) {
	impl_->set_text(toGlibUstring(text));
}

String MimeData::text() const {
	return fromGlibUstring(impl_->get_text());
}


// DragContext ////////////////////////////////////////////////////////////////////////////////////

namespace {
	inline DropAction actionsFromGdkmm(Gdk::DragAction from) BOOST_NOEXCEPT {
		DropAction to = 0;
		if((from & Gdk::ACTION_COPY) != 0)
			to |= DROP_ACTION_COPY;
		if((from & Gdk::ACTION_MOVE) != 0)
			to |= DROP_ACTION_MOVE;
		if((from & Gdk::ACTION_LINK) != 0)
			to |= DROP_ACTION_LINK;
		return to;
	}

	inline Gdk::DragAction actionsToGdkmm(DropAction from) BOOST_NOEXCEPT {
		Gdk::DragAction to = static_cast<Gdk::DragAction>(0);
		if((from & DROP_ACTION_COPY) != 0)
			to |= Gdk::ACTION_COPY;
		if((from & DROP_ACTION_MOVE) != 0)
			to |= Gdk::ACTION_MOVE;
		if((from & DROP_ACTION_LINK) != 0)
			to |= Gdk::ACTION_LINK;
		return to;
	}
}

DropAction DragContext::defaultAction() const {
	if(!context_)
		throw IllegalStateException("");
	return actionsFromGdkmm(context_->get_suggested_action());
}

DropAction DragContext::execute(DropAction supportedActions, int mouseButton, GdkEvent* event) {
	if(mimeData_.get() == nullptr)
		throw IllegalStateException("DragContext.setMimeData is not called.");
	const list<MimeData::Format> formats(mimeData_->formats());
	vector<Gtk::TargetEntry> targetEntries;
	BOOST_FOREACH(auto format, formats)
		targetEntries.push_back(Gtk::TargetEntry(format));
	Glib::RefPtr<Gtk::TargetList> targets(Gtk::TargetList::create(targetEntries));
	Glib::RefPtr<Gdk::DragContext> context(source_.drag_begin(targets, actionsToGdkmm(supportedActions), mouseButton, event));
	return actionsFromGdkmm(context->get_selected_action());
}

void DragContext::setImage(const graphics::Image& image, const graphics::geometry::BasicPoint<uint32_t>& hotspot) {
#ifdef ASCENSION_GRAPHICS_SYSTEM_CAIRO
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

void DragContext::setMimeData(shared_ptr<const MimeData> data) {
	mimeData_ = data;
}

DropAction DragContext::supportedActions() const {
	if(!context_)
		throw IllegalStateException("");
	return actionsFromGdkmm(context_->get_actions());
}

#endif	// ASCENSION_WINDOW_SYSTEM_GTK
