/**
 * @file buffer.hpp
 * @author exeal
 * @date 2003-2010 (was AlphaDoc.h and BufferList.h)
 * @date 2013-2014
 */

#ifndef ALPHA_BUFFER_HPP
#define ALPHA_BUFFER_HPP

#include "ambient.hpp"
#include <ascension/kernel/fileio.hpp>
#include <ascension/presentation/presentation.hpp>
#include <ascension/text-editor/session.hpp>

namespace alpha {
	/// A buffer.
	class Buffer : public ascension::kernel::Document {
	public:
		explicit Buffer(const Glib::ustring& name);
		boost::python::object self() const;

		/// @name Attributes
		/// @{
		const Glib::ustring& name() const BOOST_NOEXCEPT;
		void rename(const Glib::ustring& newName, bool unique = false);
		/// @}

		void save(const Glib::ustring& fileName);

		/// @name Shortcuts
		/// @{
		ascension::presentation::Presentation& presentation() BOOST_NOEXCEPT;
		const ascension::presentation::Presentation& presentation() const BOOST_NOEXCEPT;
		ascension::kernel::fileio::TextFileDocumentInput& textFile() BOOST_NOEXCEPT;
		const ascension::kernel::fileio::TextFileDocumentInput& textFile() const BOOST_NOEXCEPT;
		/// @}

	private:
		mutable boost::python::object self_;
		std::unique_ptr<ascension::presentation::Presentation> presentation_;
		std::unique_ptr<ascension::kernel::fileio::TextFileDocumentInput> textFile_;
		Glib::ustring name_;
	};


	/// Returns the name of the buffer.
	inline const Glib::ustring& Buffer::name() const BOOST_NOEXCEPT {
		return name_;
	}

	/// Returns the script object corresponding to the buffer.
	inline boost::python::object Buffer::self() const {
		if(self_ == boost::python::object())
			self_ = boost::python::object(boost::python::ptr(this));
		return self_;
	}

	/// Returns the input text file.
	inline ascension::kernel::fileio::TextFileDocumentInput& Buffer::textFile() BOOST_NOEXCEPT {
		return *textFile_;
	}

	/// Returns the input text file.
	inline const ascension::kernel::fileio::TextFileDocumentInput& Buffer::textFile() const BOOST_NOEXCEPT {
		return *textFile_;
	}
}

#endif // !ALPHA_BUFFER_HPP
