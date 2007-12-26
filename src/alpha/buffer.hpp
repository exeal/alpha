/**
 * @file buffer.hpp
 * @author exeal
 * @date 2003-2007 (was AlphaDoc.h and BufferList.h)
 */

#ifndef ALPHA_BUFFER_HPP
#define ALPHA_BUFFER_HPP

#include "ascension/viewer.hpp"
#include "ascension/searcher.hpp"	// ascension.searcher.IIncrementalSearchListener
#include "ascension/session.hpp"
#include <objbase.h>
#include "../manah/win32/ui/splitter.hpp"
#include "../manah/win32/ui/menu.hpp"
#include "../manah/win32/ui/common-controls.hpp"



namespace alpha {
	class Alpha;
	class BufferList;
	namespace ambient {class Buffer;}

	/// A buffer.
	class Buffer : public ascension::kernel::Document {
	public:
		// constructors
		Buffer() throw();
		~Buffer() throw();
		// attributes
		ascension::kernel::fileio::TextFileDocumentInput&		textFile() throw();
		const ascension::kernel::fileio::TextFileDocumentInput&	textFile() const throw();
		const std::basic_string<::WCHAR>						name() const;
		ascension::presentation::Presentation&					presentation() throw();
		const ascension::presentation::Presentation&			presentation() const throw();

	private:
		std::auto_ptr<ascension::presentation::Presentation> presentation_;
		std::auto_ptr<ascension::kernel::fileio::TextFileDocumentInput> textFile_;
	};

	/// A view of a text editor.
	class EditorView : public ascension::viewers::TextViewer, virtual public ascension::kernel::fileio::IFilePropertyListener,
		virtual public ascension::kernel::IBookmarkListener, virtual public ascension::searcher::IIncrementalSearchCallback {
	public:
		// constructors
		EditorView(ascension::presentation::Presentation& presentation);
		EditorView(const EditorView& rhs);
		~EditorView();
		// attributes
		const wchar_t*		currentPositionString() const;
		Buffer&				document() throw();
		const Buffer&		document() const throw();
		ascension::length_t	visualColumnStartValue() const throw();
		void				setVisualColumnStartValue() throw();
		// operations
		void	beginIncrementalSearch(ascension::searcher::SearchType type, ascension::Direction direction);

	private:
		void	updateCurrentPositionOnStatusBar();
		void	updateNarrowingOnStatusBar();
		void	updateOvertypeModeOnStatusBar();
		void	updateTitleBar();
		// ascension.viewers.TextViewer (overrides)
		void	drawIndicatorMargin(ascension::length_t line, manah::win32::gdi::DC& dc, const ::RECT& rect);
		// ascension.kernel.IDocumentStateListener (overrides)
		void	documentAccessibleRegionChanged(const ascension::kernel::Document& document);
		void	documentModificationSignChanged(const ascension::kernel::Document& document);
		void	documentPropertyChanged(const ascension::kernel::Document& document, const ascension::kernel::DocumentPropertyKey& key);
		void	documentReadOnlySignChanged(const ascension::kernel::Document& document);
		// ascension.kernel.fileio.IFilePropertyListener
		void	fileEncodingChanged(const ascension::kernel::fileio::TextFileDocumentInput& textFile);
		void	fileNameChanged(const ascension::kernel::fileio::TextFileDocumentInput& textFile);
		// ascension.viewers.ICaretListener (overrides)
		void	caretMoved(const ascension::viewers::Caret& self, const ascension::kernel::Region& oldRegion);
		void	characterInputted(const ascension::viewers::Caret& self, ascension::CodePoint c);
		void	matchBracketsChanged(const ascension::viewers::Caret& self,
					const std::pair<ascension::kernel::Position, ascension::kernel::Position>& oldPair, bool outsideOfView);
		void	overtypeModeChanged(const ascension::viewers::Caret& self);
		void	selectionShapeChanged(const ascension::viewers::Caret& self);
		// ascension.searcher.IIncrementalSearchCallback
		void	incrementalSearchAborted(const ascension::kernel::Position& initialPosition);
		void	incrementalSearchCompleted();
		void	incrementalSearchPatternChanged(ascension::searcher::IIncrementalSearchCallback::Result result,
					const manah::Flags<ascension::searcher::IIncrementalSearchCallback::WrappingStatus>& wrappingStatus);
		void	incrementalSearchStarted(const ascension::kernel::Document& document);
		// ascension.text.IBookmarkListener
		void	bookmarkChanged(ascension::length_t line);
		void	bookmarkCleared();
		// message handlers
		MANAH_DECLEAR_WINDOW_MESSAGE_MAP(EditorView);
		void	onKeyDown(UINT vkey, UINT flags, bool& handled);
		void	onKillFocus(HWND newWindow);
		void	onSetFocus(HWND oldWindow);
	private:
		ascension::length_t visualColumnStartValue_;
		static manah::win32::Handle<HICON, ::DestroyIcon> narrowingIcon_;
	};

	/// A pane for a text editor.
	class EditorPane : virtual public manah::win32::ui::AbstractPane {
		MANAH_UNASSIGNABLE_TAG(EditorPane);
	public:
		// constructor
		EditorPane(EditorView* initialView = 0);
		EditorPane(const EditorPane& rhs);
		~EditorPane();
		// attributes
		std::size_t	numberOfViews() const throw();
		Buffer&		visibleBuffer() const;
		EditorView&	visibleView() const;
		// operations
		void	addView(EditorView& view);
		void	removeAll();
		void	removeBuffer(const Buffer& buffer);
		void	showBuffer(const Buffer& buffer);
		// manah.win32.ui.AbstractPane
		::HWND	getWindow() const throw();
	private:
		std::set<EditorView*> views_;
		EditorView* visibleView_;
		EditorView* lastVisibleView_;
	};

	/// A splittable editor window.
	typedef manah::win32::ui::Splitter<EditorPane> EditorWindow;

	/**
	 * Manages a list of buffers.
	 * リストに追加されたバッファはこのオブジェクトが破壊する。
	 * またこのクラスはバッファバーに使うアイコンも提供する
	 */
	class BufferList :
			virtual public ascension::kernel::IDocumentStateListener,
			virtual public ascension::kernel::fileio::IFilePropertyListener,
			virtual public ascension::kernel::fileio::IUnexpectedFileTimeStampDirector,
			virtual public ascension::presentation::ITextViewerListListener {
		MANAH_NONCOPYABLE_TAG(BufferList);
	public:
		/// Results of @c #open and @c #reopen methods.
		enum OpenResult {
			OPENRESULT_SUCCEEDED,		///< Succeeded.
			OPENRESULT_FAILED,			///< Failed.
			OPENRESULT_USERCANCELED,	///< Canceled by user.
		};

		// constructors
		BufferList(Alpha& app);
		~BufferList();
		// attributes
		Buffer&									active() const;
		std::size_t								activeIndex() const;
		EditorView&								activeView() const;
		Buffer&									at(std::size_t index) const;
		::HICON									bufferIcon(std::size_t index) const;
		ascension::texteditor::Session&			editorSession() throw();
		const ascension::texteditor::Session&	editorSession() const throw();
		EditorWindow&							editorWindow() const throw();
		static std::wstring						getDisplayName(const Buffer& buffer);
		const manah::win32::ui::Menu&			listMenu() const throw();
		std::size_t								numberOfBuffers() const throw();
		void									setActive(std::size_t index);
		void									setActive(const Buffer& buffer);
		// operations
		void		addNew(const std::string& encoding = "UTF-8",
						ascension::kernel::Newline newline = ascension::kernel::NLF_RAW_VALUE);
		void		addNewDialog();
		bool		close(std::size_t index, bool queryUser);
		bool		closeAll(bool queryUser, bool exceptActive = false);
		bool		createBar(manah::win32::ui::Rebar& rebar);
		std::size_t	find(const Buffer& buffer) const;
		std::size_t	find(const std::basic_string<::WCHAR>& fileName) const;
		::LRESULT	handleBufferBarNotification(::NMTOOLBAR& nmhdr);
		::LRESULT	handleBufferBarPagerNotification(::NMHDR& nmhdr);
		void		move(std::size_t from, std::size_t to);
		OpenResult	open(const std::basic_string<::WCHAR>& fileName,
						const std::string& encoding = "UniversalAutoDetect", bool asReadOnly = false, bool addToMRU = true);
		OpenResult	openDialog(const ::WCHAR* initialDirectory = 0);
		OpenResult	reopen(std::size_t index, bool changeCodePage);
		bool		save(std::size_t index, bool overwrite = true, bool addToMRU = true);
		bool		saveAll(bool addToMRU = true);
		void		updateContextMenu();
		// ascension.kernel.IDocumentStateListener
		void	documentAccessibleRegionChanged(const ascension::kernel::Document& document);
		void	documentModificationSignChanged(const ascension::kernel::Document& document);
		void	documentPropertyChanged(const ascension::kernel::Document& document, const ascension::kernel::DocumentPropertyKey& key);
		void	documentReadOnlySignChanged(const ascension::kernel::Document& document);
		// ascension.kernel.fileio.IFilePropertyListener
		void	fileEncodingChanged(const ascension::kernel::fileio::TextFileDocumentInput& textFile);
		void	fileNameChanged(const ascension::kernel::fileio::TextFileDocumentInput& textFile);
		// ascension.kernel.fileio.IUnexpectedFileTimeStampDerector
		bool	queryAboutUnexpectedDocumentFileTimeStamp(ascension::kernel::Document& document,
					ascension::kernel::fileio::IUnexpectedFileTimeStampDirector::Context context) throw();
		// ascension.presentation.ITextViewerListListener
		void	textViewerListChanged(ascension::presentation::Presentation& presentation);

	private:
		void						fireActiveBufferSwitched();
		Buffer&						getConcreteDocument(ascension::kernel::Document& document) const;
		const Buffer&				getConcreteDocument(const ascension::kernel::Document& document) const;
		bool						handleFileIOError(const ::WCHAR* fileName, bool forLoading, ascension::kernel::fileio::IOException::Type result);
		static ::UINT_PTR CALLBACK	openFileNameHookProc(::HWND window, ::UINT message, ::WPARAM wParam, ::LPARAM lParam);
		void						recalculateBufferBarSize();
		void						resetResources();
	private:
		Alpha& app_;
		ascension::texteditor::Session editorSession_;
		std::vector<Buffer*> buffers_;
		EditorWindow editorWindow_;
		manah::win32::ui::Toolbar bufferBar_;
		manah::win32::ui::PagerCtrl bufferBarPager_;
		manah::win32::ui::ImageList icons_;
		manah::win32::ui::PopupMenu listMenu_, contextMenu_;
		static const std::wstring READ_ONLY_SIGNATURE_;
	};


	/// Returns the input text file.
	inline ascension::kernel::fileio::TextFileDocumentInput& Buffer::textFile() throw() {return *textFile_;}

	/// Returns the input text file.
	inline const ascension::kernel::fileio::TextFileDocumentInput& Buffer::textFile() const throw() {return *textFile_;}

	/// @see manah#windows#controls#AbstractPane#getWindow
	inline ::HWND EditorPane::getWindow() const throw() {return (visibleView_ != 0) ? visibleView_->getHandle() : 0;}

	/// Returns the number of the viewers.
	inline std::size_t EditorPane::numberOfViews() const throw() {return views_.size();}

	/// Returns the visible buffer.
	inline Buffer& EditorPane::visibleBuffer() const {return visibleView().document();}

	/// Returns the visible viewer.
	inline EditorView& EditorPane::visibleView() const {if(visibleView_ == 0) throw std::logic_error("There no views."); return *visibleView_;}

	/// Returns the active buffer.
	inline Buffer& BufferList::active() const {return editorWindow_.getActivePane().visibleBuffer();}

	/// Returns the index of the active buffer.
	inline std::size_t BufferList::activeIndex() const {return find(active());}

	/// Returns the active viewer.
	inline EditorView& BufferList::activeView() const {return editorWindow_.getActivePane().visibleView();}

	/// Returns the viewer has the given index.
	inline Buffer& BufferList::at(std::size_t index) const {return *buffers_.at(index);}

	/// Returns the icon of the specified buffer.
	inline HICON BufferList::bufferIcon(std::size_t index) const {
		if(index >= numberOfBuffers()) throw std::out_of_range("Index is invalid."); return icons_.getIcon(static_cast<int>(index));}

	/// Returns the session of the text editor framework.
	inline ascension::texteditor::Session& BufferList::editorSession() throw() {return editorSession_;}

	/// Returns the session of the text editor framework.
	inline const ascension::texteditor::Session& BufferList::editorSession() const throw() {return editorSession_;}

	/// Returns the text editor window.
	inline EditorWindow& BufferList::editorWindow() const throw() {return const_cast<BufferList*>(this)->editorWindow_;}

	/// Returns the menu for the buffer bar.
	inline const manah::win32::ui::Menu& BufferList::listMenu() const throw() {return listMenu_;}

	/// Returns the number of the buffers.
	inline std::size_t BufferList::numberOfBuffers() const throw() {return buffers_.size();}

	/// @see ascension#viewers#TextViewer#document
	inline Buffer& EditorView::document() throw() {return reinterpret_cast<Buffer&>(ascension::viewers::TextViewer::document());}

	/// @see ascension#viewers#TextViewer#document
	inline const Buffer& EditorView::document() const throw() {return reinterpret_cast<const Buffer&>(ascension::viewers::TextViewer::document());}
}

#endif /* !ALPHA_BUFFER_HPP */
