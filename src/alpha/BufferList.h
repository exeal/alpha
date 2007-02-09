// BufferList.h
// (c) 2004-2006 exeal

#ifndef BUFFER_LIST_H_
#define BUFFER_LIST_H_

#include "AlphaView.h"
#include <string>
#include <vector>
#include <stdexcept>
#include <algorithm>	// std::find
#include "../Manah/Splitter.hpp"
#include "../Manah/Menu.hpp"
#include "../Manah/CommonControls.hpp"

interface IBuffers;
interface IEditorPane;

namespace Alpha {
	class AlphaApp;

	/// �����^�C�v���ƃt�@�C�����̑Ή�
	struct DocumentType {
		std::wstring	name;				///< �^�C�v��
		wchar_t			fileSpec[MAX_PATH];	///< ���C���h�J�[�h�B������ MAX_PATH �ȉ��łȂ���΂Ȃ�Ȃ�
		std::wstring	command;			///< [���s] �Ŏ��s����R�}���h�B"$F" �����݂̃t�@�C����\��
		bool			hidden;				///< �K�p�\���X�g����B��
	};
	typedef std::vector<DocumentType> DocTypeList;

	/**
	 *	@brief �����^�C�v�̊Ǘ�
	 *
 	 *	�����ɊǗ����镶���^�C�v���X�g�ɂ̓C���f�b�N�X�ŃA�N�Z�X����B
	 *	0�Ԗڂ̕����^�C�v�͍ŏ�����o�^����Ă��镶���^�C�v�ŁA
	 *	���O�͋󕶎���A�g���q�����̃f�t�H���g�����^�C�v�ł���B
	 *	���̕����^�C�v�� removeAll ���g���Ă��폜����Ȃ�
	 */
	class DocumentTypeManager : public Manah::Noncopyable {
		// �R���X�g���N�^
	private:
		DocumentTypeManager();

		// ���\�b�h
	public:
		void				add(const DocumentType& type);
		std::size_t			find(const std::wstring& name) const;
		const DocumentType&	getAt(std::size_t index) const;
		const DocumentType&	getByFileName(const std::basic_string<WCHAR>& fileName) const;
		std::size_t			getCount() const throw();
		void				removeAll();

		// �f�[�^�����o
	private:
		std::vector<DocumentType> documentTypes_;
		friend class BufferList;
	};

	/// �e�L�X�g�G�f�B�^�̃y�C��
	class EditorPane : virtual public Manah::Windows::Controls::AbstractPane, public Manah::Unassignable {
	public:
		// �R���X�g���N�^
		EditorPane(AlphaView* initialView = 0);
		EditorPane(const EditorPane& rhs);
		~EditorPane();

		// ����
		HRESULT		getAutomation(IEditorPane*& object) const throw();
		std::size_t	getCount() const throw();
		HWND		getWindow() const throw();
		AlphaDoc&	getVisibleBuffer() const;
		AlphaView&	getVisibleView() const;

		// ����
		void	addView(AlphaView& view);
		void	removeAll();
		void	removeBuffer(const AlphaDoc& buffer);
		void	showBuffer(const AlphaDoc& buffer);

		// �f�[�^�����o
	private:
		std::set<AlphaView*> views_;
		AlphaView* visibleView_;
		AlphaView* lastVisibleView_;
		IEditorPane* automation_;
	};

	/// �����\�ȃG�f�B�^�E�B���h�E
	typedef Manah::Windows::Controls::SplitterRoot<EditorPane> EditorWindow;

	/**
	 *	@brief �o�b�t�@���X�g�̊Ǘ�
	 *
	 *	���X�g�ɒǉ����ꂽ�o�b�t�@�͂��̃I�u�W�F�N�g���j�󂷂�B
	 *	�܂����̃N���X�̓o�b�t�@�o�[�Ɏg���A�C�R�����񋟂���
	 */
	class BufferList : public Manah::Noncopyable,
			virtual public Manah::Document<Ascension::DocumentUpdate>::IListener,
			virtual public Manah::FileBoundDocument<Ascension::DocumentUpdate>::IListener,
			virtual public Ascension::EditDoc::IStatusListener,
			virtual public Ascension::EditDoc::IUnexpectedFileTimeStampDerector {
	public:
		/// open �Areopen �̌���
		enum OpenResult {
			OPENRESULT_SUCCEEDED,		///< ����
			OPENRESULT_FAILED,			///< ���s
			OPENRESULT_USERCANCELED,	///< ���[�U���L�����Z������
		};
		/// �A�N�e�B�u�ȃo�b�t�@�Ɋւ���ύX���󂯎��
		class IActiveBufferListener {
		public:
			/// �f�X�g���N�^
			virtual ~IActiveBufferListener() {}
			/// �A�N�e�B�u�ȃo�b�t�@���؂�ւ����
			virtual void onChangedActiveBuffer() = 0;
			/// �A�N�e�B�u�ȃo�b�t�@�̃v���p�e�B���ω�����
			virtual void onChangedActiveBufferProperty() = 0;
		};

		// �R���X�g���N�^
		BufferList(AlphaApp& app);
		~BufferList();
		// ����
		AlphaDoc&								getActive() const;
		std::size_t								getActiveIndex() const;
		AlphaView&								getActiveView() const;
		AlphaDoc&								getAt(std::size_t index) const;
		HRESULT									getAutomation(IBuffers*& buffers) const throw();
		HICON									getBufferIcon(std::size_t index) const;
		std::size_t								getCount() const throw();
		static std::wstring						getDisplayName(const AlphaDoc& buffer);
		DocumentTypeManager&					getDocumentTypeManager() const throw();
		EditorWindow&							getEditorWindow() const throw();
		const Manah::Windows::Controls::Menu&	getListMenu() const throw();
		void									setActive(std::size_t index);
		void									setActive(const AlphaDoc& buffer);
		// ����
		void		addNew(Ascension::Encodings::CodePage cp = Ascension::Encodings::CPEX_AUTODETECT_USERLANG,
							Ascension::LineBreak lineBreak = Ascension::LB_AUTO, const std::wstring& docType = L"");
		void		addNewDialog();
		bool		close(std::size_t index, bool queryUser);
		bool		closeAll(bool queryUser, bool exceptActive = false);
		bool		createBar(Manah::Windows::Controls::Rebar& rebar);
		std::size_t	find(const AlphaDoc& buffer) const;
		std::size_t	find(const std::basic_string<WCHAR>& fileName) const;
		LRESULT		handleBufferBarNotification(NMTOOLBAR& nmhdr);
		LRESULT		handleBufferBarPagerNotification(NMHDR& nmhdr);
		void		move(std::size_t from, std::size_t to);
		OpenResult	open(const std::basic_string<WCHAR>& fileName,
						Ascension::Encodings::CodePage cp = Ascension::Encodings::CPEX_AUTODETECT_USERLANG,
						bool asReadOnly = false, bool addToMRU = true);
		OpenResult	openDialog(const WCHAR* initialDirectory = 0);
		OpenResult	reopen(std::size_t index, bool changeCodePage);
		bool		save(std::size_t index, bool overwrite = true, bool addToMRU = true);
		bool		saveAll(bool addToMRU = true);
		void		updateContextMenu();
		// Document::IListener
		void	onChangedViewList(Manah::Document<Ascension::DocumentUpdate>& document);
		void	onChangedModification(Manah::Document<Ascension::DocumentUpdate>& document);
		// FileBoundDocument::IListener
		void	onChangedFileName(Manah::FileBoundDocument<Ascension::DocumentUpdate>& document);
		// EditDoc::IStatusListener
		void	onDocumentChangedAccessibleRegion(Ascension::EditDoc& document);
		void	onDocumentChangedFormat(Ascension::EditDoc& document);
		void	onDocumentChangedReadOnlyState(Ascension::EditDoc& document);
		// EditDoc::IUnexpectedFileTimeStampDerector
		bool	queryAboutUnexpectedDocumentFileTimeStamp(
			Ascension::EditDoc& document, Ascension::EditDoc::IUnexpectedFileTimeStampDerector::Context context) throw();

	private:
		void						fireChangedActiveBuffer();
		AlphaDoc&					getConcreteDocument(Manah::Document<Ascension::DocumentUpdate>& document) const;
		bool						handleFileIOError(const WCHAR* fileName, bool forLoading, Ascension::EditDoc::FileIOResult result);
		static UINT_PTR CALLBACK	openFileNameHookProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
		void						recalculateBufferBarSize();
		void						resetResources();

		// �f�[�^�����o
	private:
		AlphaApp&							app_;
		std::vector<AlphaDoc*>				buffers_;
		DocumentTypeManager					documentTypes_;
		EditorWindow						editorWindow_;
		mutable IBuffers*					automation_;
		Manah::Windows::Controls::Toolbar	bufferBar_;
		Manah::Windows::Controls::PagerCtrl	bufferBarPager_;
		Manah::Windows::Controls::ImageList	icons_;
		Manah::Windows::Controls::Menu		listMenu_;
		Manah::Windows::Controls::Menu		contextMenu_;
		static const std::wstring			READ_ONLY_SIGNATURE_;
	};


	/// �o�^����Ă��镶���^�C�v�̑�����Ԃ�
	inline std::size_t DocumentTypeManager::getCount() const throw() {return documentTypes_.size();}

	/// �r���[�̑�����Ԃ�
	inline std::size_t EditorPane::getCount() const throw() {return views_.size();}

	/// @see AbstractPane::getWindow
	inline HWND EditorPane::getWindow() const throw() {return (visibleView_ != 0) ? visibleView_->getSafeHwnd() : 0;}

	/// �\������Ă���o�b�t�@��Ԃ�
	inline AlphaDoc& EditorPane::getVisibleBuffer() const {return getVisibleView().getDocument();}

	/// �\������Ă���r���[��Ԃ�
	inline AlphaView& EditorPane::getVisibleView() const {if(visibleView_ == 0) throw std::logic_error("There no views."); return *visibleView_;}

	/// �A�N�e�B�u�ȃo�b�t�@��Ԃ�
	inline AlphaDoc& BufferList::getActive() const {return editorWindow_.getActivePane().getVisibleBuffer();}

	/// �A�N�e�B�u�ȃo�b�t�@�̈ʒu��Ԃ�
	inline std::size_t BufferList::getActiveIndex() const {return find(getActive());}

	/// �A�N�e�B�u�ȃr���[��Ԃ�
	inline AlphaView& BufferList::getActiveView() const {return editorWindow_.getActivePane().getVisibleView();}

	/// �w��ʒu�̃o�b�t�@��Ԃ�
	inline AlphaDoc& BufferList::getAt(std::size_t index) const {return *buffers_.at(index);}

	/// �h�L�������g�̐���Ԃ�
	inline std::size_t BufferList::getCount() const throw() {return buffers_.size();}

	/// �����^�C�v�}�l�[�W����Ԃ�
	inline DocumentTypeManager& BufferList::getDocumentTypeManager() const throw() {return const_cast<BufferList*>(this)->documentTypes_;}

	/// �o�b�t�@�̃A�C�R����Ԃ�
	inline HICON BufferList::getBufferIcon(std::size_t index) const {
		if(index >= getCount()) throw std::out_of_range("Index is invalid."); return icons_.getIcon(static_cast<int>(index));}

	/// �G�f�B�^�E�B���h�E��Ԃ�
	inline EditorWindow& BufferList::getEditorWindow() const throw() {return const_cast<BufferList*>(this)->editorWindow_;}

	/// �o�b�t�@���X�g�̃��j���[��Ԃ�
	inline const Manah::Windows::Controls::Menu& BufferList::getListMenu() const throw() {return listMenu_;}

} // namespace Alpha

#endif /* BUFFER_LIST_H_ */

/* [EOF] */