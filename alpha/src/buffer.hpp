/**
 * @file buffer.hpp
 * @author exeal
 * @date 2003-2009 (was AlphaDoc.h and BufferList.h)
 */

#ifndef ALPHA_BUFFER_HPP
#define ALPHA_BUFFER_HPP

#include "ambient.hpp"
#include <ascension/presentation.hpp>
#include <ascension/session.hpp>
#include <manah/win32/ui/menu.hpp>
#include <manah/win32/ui/common-controls.hpp>


namespace alpha {
	class Alpha;
	class BufferList;

	/// A buffer.
	class Buffer : public ascension::kernel::Document {
	public:
		// constructors
		Buffer() /*throw()*/;
		~Buffer() /*throw()*/;
		// attributes
		manah::com::ComPtr<IBuffer> asScript() const;
		const std::basic_string<WCHAR> name() const;
		// shortcuts
		ascension::presentation::Presentation& presentation() /*throw()*/;
		const ascension::presentation::Presentation& presentation() const /*throw()*/;
		ascension::kernel::fileio::TextFileDocumentInput& textFile() /*throw()*/;
		const ascension::kernel::fileio::TextFileDocumentInput& textFile() const /*throw()*/;
	private:
		manah::com::ComPtr<IBuffer> self_;
		std::auto_ptr<ascension::presentation::Presentation> presentation_;
		std::auto_ptr<ascension::kernel::fileio::TextFileDocumentInput> textFile_;
	};

	/// Manages a list of buffers. This class provides also buffer bar GUI.
	class BufferList :
			public ascension::kernel::IDocumentStateListener,
			public ascension::kernel::fileio::IFilePropertyListener,
			public ascension::kernel::fileio::IUnexpectedFileTimeStampDirector,
			public ascension::presentation::ITextViewerListListener {
		MANAH_NONCOPYABLE_TAG(BufferList);
	public:
		/// Results of @c #open and @c #reopen methods.
		enum OpenResult {
			OPENRESULT_SUCCEEDED,		///< Succeeded.
			OPENRESULT_FAILED,			///< Failed.
			OPENRESULT_USERCANCELED,	///< Canceled by user.
		};

		// constructors
		~BufferList();
		// instance
		static BufferList& instance();
		// attributes
		manah::com::ComPtr<IBufferList> asScript() const;
		Buffer& at(std::size_t index) const;
		HICON bufferIcon(std::size_t index) const;
		ascension::texteditor::Session& editorSession() /*throw()*/;
		const ascension::texteditor::Session& editorSession() const /*throw()*/;
		static std::wstring getDisplayName(const Buffer& buffer);
		const manah::win32::ui::Menu& listMenu() const /*throw()*/;
		std::size_t numberOfBuffers() const /*throw()*/;
		void setActive(std::size_t index);
		void setActive(const Buffer& buffer);
		// operations
		Buffer& addNew(const ascension::String& name = L"",
			const std::string& encoding = "UTF-8",
			ascension::kernel::Newline newline = ascension::kernel::NLF_RAW_VALUE);
		Buffer* addNewDialog(const ascension::String& name = L"");
		void close(Buffer& buffer);
		bool createBar(manah::win32::ui::Rebar& rebar);
		std::size_t find(const Buffer& buffer) const;
		std::size_t find(const std::basic_string<WCHAR>& fileName) const;
		LRESULT handleBufferBarNotification(NMTOOLBARW& nmhdr);
		LRESULT handleBufferBarPagerNotification(NMHDR& nmhdr);
		void move(std::size_t from, std::size_t to);
		Buffer* open(const std::basic_string<WCHAR>& fileName,
			const std::string& encoding = "UniversalAutoDetect", bool asReadOnly = false);
		bool openDialog(const std::wstring& initialDirectory = std::wstring());
		OpenResult reopen(std::size_t index, bool changeCodePage);
		bool save(std::size_t index, bool overwrite = true, bool addToMRU = true);
		bool saveSomeDialog();
		bool saveAll(bool addToMRU = true);
		void updateContextMenu();
	private:
		BufferList();
		void fireActiveBufferSwitched();
		Buffer& getConcreteDocument(ascension::kernel::Document& document) const;
		const Buffer& getConcreteDocument(const ascension::kernel::Document& document) const;
		bool handleFileIOError(const ::WCHAR* fileName, bool forLoading, ascension::kernel::fileio::IOException::Type result);
		static UINT_PTR CALLBACK openFileNameHookProc(::HWND window, ::UINT message, ::WPARAM wParam, ::LPARAM lParam);
		void recalculateBufferBarSize();
		void resetResources();
		// ascension.kernel.IDocumentStateListener
		void documentAccessibleRegionChanged(const ascension::kernel::Document& document);
		void documentModificationSignChanged(const ascension::kernel::Document& document);
		void documentPropertyChanged(const ascension::kernel::Document& document, const ascension::kernel::DocumentPropertyKey& key);
		void documentReadOnlySignChanged(const ascension::kernel::Document& document);
		// ascension.kernel.fileio.IFilePropertyListener
		void fileEncodingChanged(const ascension::kernel::fileio::TextFileDocumentInput& textFile);
		void fileNameChanged(const ascension::kernel::fileio::TextFileDocumentInput& textFile);
		// ascension.kernel.fileio.IUnexpectedFileTimeStampDerector
		bool queryAboutUnexpectedDocumentFileTimeStamp(ascension::kernel::Document& document,
				ascension::kernel::fileio::IUnexpectedFileTimeStampDirector::Context context) /*throw()*/;
		// ascension.presentation.ITextViewerListListener
		void textViewerListChanged(ascension::presentation::Presentation& presentation);
	private:
		manah::com::ComPtr<IBufferList> self_;
		ascension::texteditor::Session editorSession_;
		std::vector<Buffer*> buffers_;
		manah::win32::ui::Toolbar bufferBar_;
		manah::win32::ui::PagerCtrl bufferBarPager_;
		manah::win32::ui::ImageList icons_;
		manah::win32::ui::PopupMenu listMenu_, contextMenu_;
	};


	/// Returns the script object corresponding to the buffer.
	inline manah::com::ComPtr<IBuffer> Buffer::asScript() const {return self_;}

	/// Returns the input text file.
	inline ascension::kernel::fileio::TextFileDocumentInput& Buffer::textFile() /*throw()*/ {return *textFile_;}

	/// Returns the input text file.
	inline const ascension::kernel::fileio::TextFileDocumentInput& Buffer::textFile() const /*throw()*/ {return *textFile_;}

	/// Returns the script object corresponding to the buffer list.
	inline manah::com::ComPtr<IBufferList> BufferList::asScript() const {return self_;}

	/// Returns the viewer has the given index.
	inline Buffer& BufferList::at(std::size_t index) const {return *buffers_.at(index);}

	/// Returns the icon of the specified buffer.
	inline HICON BufferList::bufferIcon(std::size_t index) const {
		if(index >= numberOfBuffers()) throw std::out_of_range("Index is invalid."); return icons_.getIcon(static_cast<int>(index));}

	/// Returns the session of the text editor framework.
	inline ascension::texteditor::Session& BufferList::editorSession() /*throw()*/ {return editorSession_;}

	/// Returns the session of the text editor framework.
	inline const ascension::texteditor::Session& BufferList::editorSession() const /*throw()*/ {return editorSession_;}

	/// Returns the menu for the buffer bar.
	inline const manah::win32::ui::Menu& BufferList::listMenu() const /*throw()*/ {return listMenu_;}

	/// Returns the number of the buffers.
	inline std::size_t BufferList::numberOfBuffers() const /*throw()*/ {return buffers_.size();}
}

#endif // !ALPHA_BUFFER_HPP
