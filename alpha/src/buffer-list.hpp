/**
 * @file buffer-list.hpp
 * @author exeal
 * @date 2003-2010 was AlphaDoc.h and BufferList.h
 * @date 2013-10-13 separated from buffer.hpp
 */

#ifndef ALPHA_BUFFER_LIST_HPP
#define ALPHA_BUFFER_LIST_HPP

#include "ambient.hpp"
#include <ascension/kernel/fileio.hpp>
#include <ascension/presentation/presentation.hpp>
#include <ascension/text-editor/session.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#	include <ascension/win32/ui/menu.hpp>
#	include <ascension/win32/ui/common-controls.hpp>
#else
#	include <gtkmm/menu.h>
#endif


namespace alpha {
	class Alpha;

	/// Manages a list of buffers. This class provides also buffer bar GUI.
	class BufferList :
			public ascension::kernel::fileio::FilePropertyListener,
			public ascension::kernel::fileio::UnexpectedFileTimeStampDirector,
			private boost::noncopyable {
	public:
		/// Results of @c #open and @c #reopen methods.
		enum OpenResult {
			OPENRESULT_SUCCEEDED,		///< Succeeded.
			OPENRESULT_FAILED,			///< Failed.
			OPENRESULT_USERCANCELED,	///< Canceled by user.
		};

		~BufferList();
		static BufferList& instance();
		boost::python::object self() const;

		/// @name
		/// @{
		Buffer& at(std::size_t index) const;
		Glib::ustring displayName(const Buffer& buffer) const BOOST_NOEXCEPT;
		boost::optional<std::size_t> find(const Buffer& buffer) const BOOST_NOEXCEPT;
		boost::python::object forName(const Glib::ustring& name) const;
		Glib::ustring makeUniqueName(const Glib::ustring& name) const;
		void move(boost::python::ssize_t from, boost::python::ssize_t to);
		std::size_t numberOfBuffers() const BOOST_NOEXCEPT;
		/// @}

		/// @name Open and Save
		/// @{
		Buffer& addNew(
			const Glib::ustring& name = Glib::ustring(), const std::string& encoding = "UTF-8",
			ascension::text::Newline newline = ascension::text::Newline::USE_INTRINSIC_VALUE);
//		Buffer* addNewDialog(const ascension::String& name = L"");
		void close(Buffer& buffer);
		bool saveSomeDialog(boost::python::tuple buffersToSave = boost::python::tuple());
		/// @}

		/// @name Other Attributes
		/// @{
		ascension::texteditor::Session& editorSession() BOOST_NOEXCEPT;
		const ascension::texteditor::Session& editorSession() const BOOST_NOEXCEPT;
		/// @}

		/// @name Signals
		/// @{
		typedef boost::signals2::signal<void(BufferList&, Buffer&)> BufferAboutToBeRemovedSignal;
		ascension::SignalConnector<BufferAboutToBeRemovedSignal> bufferAboutToBeRemovedSignal() BOOST_NOEXCEPT;
		typedef boost::signals2::signal<void(BufferList&, Buffer&)> BufferAddedSignal;
		ascension::SignalConnector<BufferAddedSignal> bufferAddedSignal() BOOST_NOEXCEPT;
		typedef boost::signals2::signal<void(BufferList&, Buffer&)> BufferRemovedSignal;
		ascension::SignalConnector<BufferRemovedSignal> bufferRemovedSignal() BOOST_NOEXCEPT;
		typedef boost::signals2::signal<void(const Buffer&)> DisplayNameChangedSignal;
		ascension::SignalConnector<DisplayNameChangedSignal> displayNameChangedSignal() BOOST_NOEXCEPT;
		/// @}

		void bufferSelectionChanged();

	private:
		BufferList();
		Buffer& getConcreteDocument(ascension::kernel::Document& document) const;
		const Buffer& getConcreteDocument(const ascension::kernel::Document& document) const;
		void fireDisplayNameChanged(const ascension::kernel::Document& buffer);
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		static UINT_PTR CALLBACK openFileNameHookProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
		void recalculateBufferBarSize();
#endif
		void resetResources();
		void updateTitleBar();
		// ascension.kernel.Document signals
		void documentModificationSignChanged(const ascension::kernel::Document& document);
		void documentReadOnlySignChanged(const ascension::kernel::Document& document);
		// ascension.kernel.fileio.FilePropertyListener
		void fileEncodingChanged(const ascension::kernel::fileio::TextFileDocumentInput& textFile);
		void fileNameChanged(const ascension::kernel::fileio::TextFileDocumentInput& textFile);
		// ascension.kernel.fileio.UnexpectedFileTimeStampDerector
		bool queryAboutUnexpectedDocumentFileTimeStamp(ascension::kernel::Document& document,
				ascension::kernel::fileio::UnexpectedFileTimeStampDirector::Context context) /*throw()*/;
	public:
		// for properties
		boost::python::object unexpectedFileTimeStampDirector;
	private:
		mutable boost::python::object self_;
		ascension::texteditor::Session editorSession_;
		struct BufferEntry : private boost::noncopyable {
			std::unique_ptr<Buffer> buffer;
			boost::signals2::connection modificationSignChangedConnection, readOnlySignChangedConnection;
			BufferEntry() {}
			BufferEntry(BufferEntry&& other) BOOST_NOEXCEPT : buffer(std::move(other.buffer)) {}
			~BufferEntry() BOOST_NOEXCEPT;
			BufferEntry& operator=(BufferEntry&& other) BOOST_NOEXCEPT;
		};
		std::vector<BufferEntry> buffers_;
		BufferAboutToBeRemovedSignal bufferAboutToBeRemovedSignal_;
		BufferAddedSignal bufferAddedSignal_;
		BufferRemovedSignal bufferRemovedSignal_;
		DisplayNameChangedSignal displayNameChangedSignal_;
		boost::signals2::scoped_connection bufferSelectionChangedConnection_;
	};


	/// Returns the viewer has the given index.
	inline Buffer& BufferList::at(std::size_t index) const {
		return *buffers_.at(index).buffer;
	}

	/// Returns the session of the text editor framework.
	inline ascension::texteditor::Session& BufferList::editorSession() BOOST_NOEXCEPT {
		return editorSession_;
	}

	/// Returns the session of the text editor framework.
	inline const ascension::texteditor::Session& BufferList::editorSession() const BOOST_NOEXCEPT {
		return editorSession_;
	}

	/// Returns the number of the buffers.
	inline std::size_t BufferList::numberOfBuffers() const BOOST_NOEXCEPT {
		return buffers_.size();
	}

	/// Returns the script object corresponding to the buffer list.
	inline boost::python::object BufferList::self() const {
		if(self_ == boost::python::object())
			self_ = boost::python::object(boost::python::ptr(this));
		return self_;
	}
}

#endif // !ALPHA_BUFFER_LIST_HPP
