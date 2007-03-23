/**
 * @file buffer.hpp
 * @author exeal
 * @date 2003-2007 (was AlphaDoc.h and BufferList.h)
 */

#ifndef ALPHA_BUFFER_HPP
#define ALPHA_BUFFER_HPP

#include "ascension/viewer.hpp"
#include "ascension/searcher.hpp"	// ascension::searcher::IIncrementalSearchListener
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

	/// �o�b�t�@
	class Buffer : public ascension::text::Document {
	public:
		// �R���X�g���N�^
		Buffer() throw();
		~Buffer() throw();
		// ���\�b�h
		const TCHAR*									getFileName() const;
		ascension::presentation::Presentation&			getPresentation() throw();
		const ascension::presentation::Presentation&	getPresentation() const throw();

	private:
		std::auto_ptr<ascension::presentation::Presentation> presentation_;
	};

	/// �e�L�X�g�G�f�B�^�̃r���[
	class EditorView : public ascension::viewers::SourceViewer,
		virtual public ascension::text::IDocumentStateListener,
		virtual public ascension::viewers::ICaretListener,
		virtual public ascension::searcher::IIncrementalSearchListener {
	public:
		// �R���X�g���N�^
		EditorView(ascension::presentation::Presentation& presentation);
		EditorView(const EditorView& rhs);
		~EditorView();
		// ���\�b�h
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
		// ascension::text::IDocumentStateListener (overrides)
		void	documentAccessibleRegionChanged(ascension::text::Document& document);
		void	documentEncodingChanged(ascension::text::Document& document);
		void	documentFileNameChanged(ascension::text::Document& document);
		void	documentModificationSignChanged(ascension::text::Document& document);
		void	documentReadOnlySignChanged(ascension::text::Document& document);
		// ascension::viewers::ICaretListener (overrides)
		void	caretMoved(const ascension::viewers::Caret& self, const ascension::text::Region& oldRegion);
		void	matchBracketsChanged(const ascension::viewers::Caret& self,
					const std::pair<ascension::text::Position, ascension::text::Position>& oldPair, bool outsideOfView);
		void	overtypeModeChanged(const ascension::viewers::Caret& self);
		void	selectionShapeChanged(const ascension::viewers::Caret& self);
		// ascension::searcher::IIncrementalSearchListener
		void	incrementalSearchAborted(const ascension::text::Position& initialPosition);
		void	incrementalSearchCompleted();
		void	incrementalSearchPatternChanged(ascension::searcher::IIncrementalSearchListener::Result result);
		void	incrementalSearchStarted(const ascension::text::Document& document);
		// ���b�Z�[�W�n���h��
		MANAH_DECLEAR_WINDOW_MESSAGE_MAP(EditorView);
		void	onKeyDown(UINT vkey, UINT flags, bool& handled);
		void	onKillFocus(HWND newWindow);
		void	onSetFocus(HWND oldWindow);
	private:
		ascension::length_t visualColumnStartValue_;
		static manah::win32::Handle<HICON, ::DestroyIcon> narrowingIcon_;
	};

	/// �e�L�X�g�G�f�B�^�̃y�C��
	class EditorPane : virtual public manah::win32::ui::AbstractPane, public manah::Unassignable {
	public:
		// �R���X�g���N�^
		EditorPane(EditorView* initialView = 0);
		EditorPane(const EditorPane& rhs);
		~EditorPane();

		// ����
		std::size_t	getCount() const throw();
		HWND		getWindow() const throw();
		Buffer&		getVisibleBuffer() const;
		EditorView&	getVisibleView() const;

		// ����
		void	addView(EditorView& view);
		void	removeAll();
		void	removeBuffer(const Buffer& buffer);
		void	showBuffer(const Buffer& buffer);

		// �f�[�^�����o
	private:
		std::set<EditorView*> views_;
		EditorView* visibleView_;
		EditorView* lastVisibleView_;
	};

	/// �����\�ȃG�f�B�^�E�B���h�E
	typedef manah::win32::ui::Splitter<EditorPane> EditorWindow;

	/**
	 * @brief �o�b�t�@���X�g�̊Ǘ�
	 *
	 * ���X�g�ɒǉ����ꂽ�o�b�t�@�͂��̃I�u�W�F�N�g���j�󂷂�B
	 * �܂����̃N���X�̓o�b�t�@�o�[�Ɏg���A�C�R�����񋟂���
	 */
	class BufferList : public manah::Noncopyable,
			virtual public ascension::text::IDocumentStateListener,
			virtual public ascension::text::IUnexpectedFileTimeStampDirector,
			virtual public ascension::presentation::ITextViewerListListener {
	public:
		/// @c #open �A@c #reopen �̌���
		enum OpenResult {
			OPENRESULT_SUCCEEDED,		///< ����
			OPENRESULT_FAILED,			///< ���s
			OPENRESULT_USERCANCELED,	///< ���[�U���L�����Z������
		};

		// �R���X�g���N�^
		BufferList(Alpha& app);
		~BufferList();
		// ����
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
		// ����
		void		addNew(
						ascension::encodings::CodePage encoding = ascension::encodings::CPEX_AUTODETECT_USERLANG,
						ascension::text::LineBreak lineBreak = ascension::text::LB_AUTO);
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
						ascension::encodings::CodePage encoding = ascension::encodings::CPEX_AUTODETECT_USERLANG,
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

		// �f�[�^�����o
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



	/// �r���[�̑�����Ԃ�
	inline std::size_t EditorPane::getCount() const throw() {return views_.size();}

	/// @see manah#windows#controls#AbstractPane#getWindow
	inline HWND EditorPane::getWindow() const throw() {return (visibleView_ != 0) ? visibleView_->getHandle() : 0;}

	/// �\������Ă���o�b�t�@��Ԃ�
	inline Buffer& EditorPane::getVisibleBuffer() const {return getVisibleView().getDocument();}

	/// �\������Ă���r���[��Ԃ�
	inline EditorView& EditorPane::getVisibleView() const {if(visibleView_ == 0) throw std::logic_error("There no views."); return *visibleView_;}

	/// �A�N�e�B�u�ȃo�b�t�@��Ԃ�
	inline Buffer& BufferList::getActive() const {return editorWindow_.getActivePane().getVisibleBuffer();}

	/// �A�N�e�B�u�ȃo�b�t�@�̈ʒu��Ԃ�
	inline std::size_t BufferList::getActiveIndex() const {return find(getActive());}

	/// �A�N�e�B�u�ȃr���[��Ԃ�
	inline EditorView& BufferList::getActiveView() const {return editorWindow_.getActivePane().getVisibleView();}

	/// �w��ʒu�̃o�b�t�@��Ԃ�
	inline Buffer& BufferList::getAt(std::size_t index) const {return *buffers_.at(index);}

	/// �h�L�������g�̐���Ԃ�
	inline std::size_t BufferList::getCount() const throw() {return buffers_.size();}

	/// �o�b�t�@�̃A�C�R����Ԃ�
	inline HICON BufferList::getBufferIcon(std::size_t index) const {
		if(index >= getCount()) throw std::out_of_range("Index is invalid."); return icons_.getIcon(static_cast<int>(index));}

	/// �e�L�X�g�G�f�B�^�̃Z�b�V������Ԃ�
	inline ascension::texteditor::Session& BufferList::getEditorSession() throw() {return editorSession_;}

	/// �e�L�X�g�G�f�B�^�̃Z�b�V������Ԃ�
	inline const ascension::texteditor::Session& BufferList::getEditorSession() const throw() {return editorSession_;}

	/// �G�f�B�^�E�B���h�E��Ԃ�
	inline EditorWindow& BufferList::getEditorWindow() const throw() {return const_cast<BufferList*>(this)->editorWindow_;}

	/// �o�b�t�@���X�g�̃��j���[��Ԃ�
	inline const manah::win32::ui::Menu& BufferList::getListMenu() const throw() {return listMenu_;}

	/// @see ascension#viewers#TextViewer#getDocument
	inline Buffer& EditorView::getDocument() throw() {return reinterpret_cast<Buffer&>(ascension::viewers::SourceViewer::getDocument());}

	/// @see ascension#viewers#TextViewer#getDocument
	inline const Buffer& EditorView::getDocument() const throw() {return reinterpret_cast<const Buffer&>(ascension::viewers::SourceViewer::getDocument());}

}

#endif /* !ALPHA_BUFFER_HPP */
