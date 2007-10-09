/**
 * @file buffer.hpp
 * @author exeal
 * @date 2003-2007 (was AlphaDoc.h and BufferList.h)
 */

#ifndef ALPHA_BUFFER_HPP
#define ALPHA_BUFFER_HPP

#include "ascension/viewer.hpp"
#include "ascension/searcher.hpp"	// ascension.searcher.IIncrementalSearchListener
#include <objbase.h>
#include "../manah/win32/ui/splitter.hpp"
#include "../manah/win32/ui/menu.hpp"
#include "../manah/win32/ui/common-controls.hpp"



namespace alpha {
	class Alpha;
	class BufferList;
	namespace ambient {
		class Buffer;
	}

	/// A buffer.
	class Buffer : public ascension::text::Document {
	public:
		// constructors
		Buffer() throw();
		~Buffer() throw();
		// methods
		const TCHAR*									getFileName() const;
		ascension::presentation::Presentation&			getPresentation() throw();
		const ascension::presentation::Presentation&	getPresentation() const throw();

	private:
		std::auto_ptr<ascension::presentation::Presentation> presentation_;
	};

	/// A view of a text editor.
	class EditorView : public ascension::viewers::TextViewer,
		virtual public ascension::text::IBookmarkListener, virtual public ascension::searcher::IIncrementalSearchCallback {
	public:
		// constructors
		EditorView(ascension::presentation::Presentation& presentation);
		EditorView(const EditorView& rhs);
		~EditorView();
		// methods
		void				beginIncrementalSearch(ascension::searcher::SearchType type, ascension::Direction direction);
		const wchar_t*		getCurrentPositionString() const;
		Buffer&				getDocument() throw();
		const Buffer&		getDocument() const throw();
		ascension::length_t	getVisualColumnStartValue() const throw();
		void				setVisualColumnStartValue() throw();

	private:
		void	updateCurrentPositionOnStatusBar();
		void	updateNarrowingOnStatusBar();
		void	updateOvertypeModeOnStatusBar();
		void	updateTitleBar();
		// ascension.viewers.TextViewer (overrides)
		void	drawIndicatorMargin(ascension::length_t line, manah::win32::gdi::DC& dc, const ::RECT& rect);
		// ascension.text.IDocumentStateListener (overrides)
		void	documentAccessibleRegionChanged(ascension::text::Document& document);
		void	documentEncodingChanged(ascension::text::Document& document);
		void	documentFileNameChanged(ascension::text::Document& document);
		void	documentModificationSignChanged(ascension::text::Document& document);
		void	documentReadOnlySignChanged(ascension::text::Document& document);
		// ascension.viewers.ICaretListener (overrides)
		void	caretMoved(const ascension::viewers::Caret& self, const ascension::text::Region& oldRegion);
		void	characterInputted(const ascension::viewers::Caret& self, ascension::CodePoint c);
		void	matchBracketsChanged(const ascension::viewers::Caret& self,
					const std::pair<ascension::text::Position, ascension::text::Position>& oldPair, bool outsideOfView);
		void	overtypeModeChanged(const ascension::viewers::Caret& self);
		void	selectionShapeChanged(const ascension::viewers::Caret& self);
		// ascension.searcher.IIncrementalSearchCallback
		void	incrementalSearchAborted(const ascension::text::Position& initialPosition);
		void	incrementalSearchCompleted();
		void	incrementalSearchPatternChanged(ascension::searcher::IIncrementalSearchCallback::Result result,
					const manah::Flags<ascension::searcher::IIncrementalSearchCallback::WrappingStatus>& wrappingStatus);
		void	incrementalSearchStarted(const ascension::text::Document& document);
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
	class EditorPane : virtual public manah::win32::ui::AbstractPane, public manah::Unassignable {
	public:
		// constructor
		EditorPane(EditorView* initialView = 0);
		EditorPane(const EditorPane& rhs);
		~EditorPane();
		// attributes
		std::size_t	getCount() const throw();
		HWND		getWindow() const throw();
		Buffer&		getVisibleBuffer() const;
		EditorView&	getVisibleView() const;
		// operations
		void	addView(EditorView& view);
		void	removeAll();
		void	removeBuffer(const Buffer& buffer);
		void	showBuffer(const Buffer& buffer);
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
	class BufferList : private manah::Noncopyable,
			virtual public ascension::text::IDocumentStateListener,
			virtual public ascension::text::IUnexpectedFileTimeStampDirector,
			virtual public ascension::presentation::ITextViewerListListener {
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
		Buffer&									getActive() const;
		std::size_t								getActiveIndex() const;
		EditorView&								getActiveView() const;
		Buffer&									getAt(std::size_t index) const;
		HICON									getBufferIcon(std::size_t index) const;
		std::size_t								getCount() const throw();
		static std::wstring						getDisplayName(const Buffer& buffer);
		ascension::texteditor::Session&			getEditorSession() throw();
		const ascension::texteditor::Session&	getEditorSession() const throw();
		EditorWindow&							getEditorWindow() const throw();
		const manah::win32::ui::Menu&			getListMenu() const throw();
		void									setActive(std::size_t index);
		void									setActive(const Buffer& buffer);
		// operations
		void		addNew(
						ascension::encoding::MIBenum encoding = ascension::encoding::fundamental::MIB_UNICODE_UTF8,
						ascension::text::Newline newline = ascension::text::NLF_AUTO);
		void		addNewDialog();
		bool		close(std::size_t index, bool queryUser);
		bool		closeAll(bool queryUser, bool exceptActive = false);
		bool		createBar(manah::win32::ui::Rebar& rebar);
		std::size_t	find(const Buffer& buffer) const;
		std::size_t	find(const std::basic_string<WCHAR>& fileName) const;
		LRESULT		handleBufferBarNotification(::NMTOOLBAR& nmhdr);
		LRESULT		handleBufferBarPagerNotification(::NMHDR& nmhdr);
		void		move(std::size_t from, std::size_t to);
		OpenResult	open(const std::basic_string<WCHAR>& fileName,
						ascension::encoding::MIBenum encoding = ascension::encoding::EncodingDetector::UNIVERSAL_DETECTOR,
						bool asReadOnly = false, bool addToMRU = true);
		OpenResult	openDialog(const WCHAR* initialDirectory = 0);
		OpenResult	reopen(std::size_t index, bool changeCodePage);
		bool		save(std::size_t index, bool overwrite = true, bool addToMRU = true);
		bool		saveAll(bool addToMRU = true);
		void		updateContextMenu();
		// ascension::text::IDocumentStateListener
		void	documentAccessibleRegionChanged(ascension::text::Document& document);
		void	documentEncodingChanged(ascension::text::Document& document);
		void	documentFileNameChanged(ascension::text::Document& document);
		void	documentModificationSignChanged(ascension::text::Document& document);
		void	documentReadOnlySignChanged(ascension::text::Document& document);
		// ascension::text::IUnexpectedFileTimeStampDerector
		bool	queryAboutUnexpectedDocumentFileTimeStamp(
			ascension::text::Document& document, ascension::text::IUnexpectedFileTimeStampDirector::Context context) throw();
		// ascension::presentation::ITextViewerListListener
		void	textViewerListChanged(ascension::presentation::Presentation& presentation);

	private:
		void						fireActiveBufferSwitched();
		Buffer&						getConcreteDocument(ascension::text::Document& document) const;
		bool						handleFileIOError(const WCHAR* fileName, bool forLoading, ascension::text::Document::FileIOResult result);
		static UINT_PTR CALLBACK	openFileNameHookProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
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



	/// Returns the number of the viewers.
	inline std::size_t EditorPane::getCount() const throw() {return views_.size();}

	/// @see manah#windows#controls#AbstractPane#getWindow
	inline HWND EditorPane::getWindow() const throw() {return (visibleView_ != 0) ? visibleView_->getHandle() : 0;}

	/// Returns the visible buffer.
	inline Buffer& EditorPane::getVisibleBuffer() const {return getVisibleView().getDocument();}

	/// Returns the visible viewer.
	inline EditorView& EditorPane::getVisibleView() const {if(visibleView_ == 0) throw std::logic_error("There no views."); return *visibleView_;}

	/// Returns the active buffer.
	inline Buffer& BufferList::getActive() const {return editorWindow_.getActivePane().getVisibleBuffer();}

	/// Returns the index of the active buffer.
	inline std::size_t BufferList::getActiveIndex() const {return find(getActive());}

	/// Returns the active viewer.
	inline EditorView& BufferList::getActiveView() const {return editorWindow_.getActivePane().getVisibleView();}

	/// Returns the viewer has the given index.
	inline Buffer& BufferList::getAt(std::size_t index) const {return *buffers_.at(index);}

	/// Returns the number of the buffers.
	inline std::size_t BufferList::getCount() const throw() {return buffers_.size();}

	/// Returns the icon of the specified buffer.
	inline HICON BufferList::getBufferIcon(std::size_t index) const {
		if(index >= getCount()) throw std::out_of_range("Index is invalid."); return icons_.getIcon(static_cast<int>(index));}

	/// Returns the session of the text editor framework.
	inline ascension::texteditor::Session& BufferList::getEditorSession() throw() {return editorSession_;}

	/// Returns the session of the text editor framework.
	inline const ascension::texteditor::Session& BufferList::getEditorSession() const throw() {return editorSession_;}

	/// Returns the text editor window.
	inline EditorWindow& BufferList::getEditorWindow() const throw() {return const_cast<BufferList*>(this)->editorWindow_;}

	/// Returns the menu for the buffer bar.
	inline const manah::win32::ui::Menu& BufferList::getListMenu() const throw() {return listMenu_;}

	/// @see ascension#viewers#TextViewer#getDocument
	inline Buffer& EditorView::getDocument() throw() {return reinterpret_cast<Buffer&>(ascension::viewers::TextViewer::getDocument());}

	/// @see ascension#viewers#TextViewer#getDocument
	inline const Buffer& EditorView::getDocument() const throw() {return reinterpret_cast<const Buffer&>(ascension::viewers::TextViewer::getDocument());}
}

#endif /* !ALPHA_BUFFER_HPP */
