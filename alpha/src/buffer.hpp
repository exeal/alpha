/**
 * @file buffer.hpp
 * @author exeal
 * @date 2003-2010 (was AlphaDoc.h and BufferList.h)
 * @date 2013-2014
 */

#ifndef ALPHA_BUFFER_HPP
#define ALPHA_BUFFER_HPP
#include <ascension/kernel/fileio/text-file-document-input.hpp>
#include <ascension/presentation/presentation.hpp>
#include <ascension/text-editor/session.hpp>

namespace alpha {
	/// A buffer.
	class Buffer : public ascension::kernel::Document {
	public:
		explicit Buffer(const Glib::ustring& name);

		/// @name Attributes
		/// @{
		const Glib::ustring& name() const BOOST_NOEXCEPT;
		void rename(const Glib::ustring& newName, bool unique = false);
		/// @}

		void save(const Glib::ustring& fileName);

		/// @name Shortcuts
		/// @{
		std::shared_ptr<ascension::presentation::Presentation> presentation() BOOST_NOEXCEPT;
		std::shared_ptr<const ascension::presentation::Presentation> presentation() const BOOST_NOEXCEPT;
		ascension::kernel::fileio::TextFileDocumentInput& textFile() BOOST_NOEXCEPT;
		const ascension::kernel::fileio::TextFileDocumentInput& textFile() const BOOST_NOEXCEPT;
		/// @}

		/// @name Signals
		/// @{
		typedef boost::signals2::signal<void(const Buffer&)> NameChangedSignal;
		ascension::SignalConnector<NameChangedSignal> nameChangedSignal() BOOST_NOEXCEPT;
		/// @}

	private:
		std::shared_ptr<ascension::presentation::Presentation> presentation_;
		std::unique_ptr<ascension::kernel::fileio::TextFileDocumentInput> textFile_;
		Glib::ustring name_;
		NameChangedSignal nameChangedSignal_;
	};


	/// Returns the name of the buffer.
	inline const Glib::ustring& Buffer::name() const BOOST_NOEXCEPT {
		return name_;
	}

	/// Returns the presentation object of Ascension.
	inline std::shared_ptr<ascension::presentation::Presentation> Buffer::presentation() BOOST_NOEXCEPT {
		return presentation_;
	}

	/// Returns the presentation object of Ascension.
	inline std::shared_ptr<const ascension::presentation::Presentation> Buffer::presentation() const BOOST_NOEXCEPT {
		return presentation_;
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
