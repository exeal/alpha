/**
 * @file interprocess-data-gtk.cpp
 * Implements @c InterprocessData class on GTK+3 window system.
 * @author exeal
 * @date 2013-11-04 Created.
 * @date 2016-12-28 Separated from drag-and-drop-gtk.cpp.
 */

#include <ascension/corelib/interprocess-data.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)

#include <boost/range/algorithm/find.hpp>

namespace ascension {
	/**
	 * Constructor.
	 * @param targets The list of targets returned by @c Gdk#DragContext#list_targets method
	 */
	InterprocessDataFormats::InterprocessDataFormats(std::vector<std::string>&& targets) : targets_(targets) {
	}

	void InterprocessDataFormats::formats(std::vector<Format>& out) const {
		std::vector<Format> temp(targets_);
		std::swap(out, temp);
	}

	bool InterprocessDataFormats::hasFormat(Format format) const BOOST_NOEXCEPT {
		return boost::find(targets_, format) != boost::end(targets_);
	}

	bool InterprocessDataFormats::hasText() const BOOST_NOEXCEPT {
		return boost::find(targets_, "text/plain") != boost::end(targets_);
	}

	bool InterprocessDataFormats::hasURIs() const BOOST_NOEXCEPT {
		return boost::find(targets_, "text/uri-list") != boost::end(targets_);
	}


	InterprocessData::InterprocessData() : impl_(std::make_shared<Gtk::SelectionData>()) {
	}

	InterprocessData::InterprocessData(Gtk::SelectionData& impl) : impl_(&impl, boost::null_deleter()) {
	}

	void InterprocessData::data(Format format, std::vector<std::uint8_t>& out) const {
		if(format != impl_->get_target())
			throw 0;
		const guchar* const p = impl_->get_data();
		std::vector<std::uint8_t> temp(p, p + impl_->get_length());
		std::swap(out, temp);
	}

	void InterprocessData::formats(std::vector<MimeDataFormats::Format>& out) const {
		std::vector<std::string> temp(impl_->get_targets());
		std::swap(out, temp);
	}

	bool InterprocessData::hasFormat(Format format) const BOOST_NOEXCEPT {
		try {
			const std::vector<std::string> targets(impl_->get_targets());
			return boost::find(targets, format) != boost::end(targets);
		} catch(...) {
			return false;
		}
	}

	bool InterprocessData::hasText() const BOOST_NOEXCEPT {
		return impl_->targets_include_text();
	}

	bool InterprocessData::hasURIs() const BOOST_NOEXCEPT {
		return impl_->targets_include_uri();
	}

	void InterprocessData::setData(Format format, const boost::iterator_range<const std::uint8_t*>& range) {
		impl_->set(format, 8, boost::const_begin(range), boost::size(range));
//		impl_->set(::gdk_atom_intern(format.c_str(), true), 8, boost::begin(range), boost::size(range));
	}

	void InterprocessData::setText(const StringPiece& text) {
		impl_->set_text(toGlibUstring(text));
	}

	String InterprocessData::text() const {
		return fromGlibUstring(impl_->get_text());
	}
}

#endif	// ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
