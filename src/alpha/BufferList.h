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

	/// 文書タイプ名とファイル名の対応
	struct DocumentType {
		std::wstring	name;				///< タイプ名
		wchar_t			fileSpec[MAX_PATH];	///< ワイルドカード。長さは MAX_PATH 以下でなければならない
		std::wstring	command;			///< [実行] で実行するコマンド。"$F" が現在のファイルを表す
		bool			hidden;				///< 適用可能リストから隠す
	};
	typedef std::vector<DocumentType> DocTypeList;

	/**
	 *	@brief 文書タイプの管理
	 *
 	 *	内部に管理する文書タイプリストにはインデックスでアクセスする。
	 *	0番目の文書タイプは最初から登録されている文書タイプで、
	 *	名前は空文字列、拡張子無しのデフォルト文書タイプである。
	 *	この文書タイプは removeAll を使っても削除されない
	 */
	class DocumentTypeManager : public Manah::Noncopyable {
		// コンストラクタ
	private:
		DocumentTypeManager();

		// メソッド
	public:
		void				add(const DocumentType& type);
		std::size_t			find(const std::wstring& name) const;
		const DocumentType&	getAt(std::size_t index) const;
		const DocumentType&	getByFileName(const std::basic_string<WCHAR>& fileName) const;
		std::size_t			getCount() const throw();
		void				removeAll();

		// データメンバ
	private:
		std::vector<DocumentType> documentTypes_;
		friend class BufferList;
	};

	/// テキストエディタのペイン
	class EditorPane : virtual public Manah::Windows::Controls::AbstractPane, public Manah::Unassignable {
	public:
		// コンストラクタ
		EditorPane(AlphaView* initialView = 0);
		EditorPane(const EditorPane& rhs);
		~EditorPane();

		// 属性
		HRESULT		getAutomation(IEditorPane*& object) const throw();
		std::size_t	getCount() const throw();
		HWND		getWindow() const throw();
		AlphaDoc&	getVisibleBuffer() const;
		AlphaView&	getVisibleView() const;

		// 操作
		void	addView(AlphaView& view);
		void	removeAll();
		void	removeBuffer(const AlphaDoc& buffer);
		void	showBuffer(const AlphaDoc& buffer);

		// データメンバ
	private:
		std::set<AlphaView*> views_;
		AlphaView* visibleView_;
		AlphaView* lastVisibleView_;
		IEditorPane* automation_;
	};

	/// 分割可能なエディタウィンドウ
	typedef Manah::Windows::Controls::SplitterRoot<EditorPane> EditorWindow;

	/**
	 *	@brief バッファリストの管理
	 *
	 *	リストに追加されたバッファはこのオブジェクトが破壊する。
	 *	またこのクラスはバッファバーに使うアイコンも提供する
	 */
	class BufferList : public Manah::Noncopyable,
			virtual public Manah::Document<Ascension::DocumentUpdate>::IListener,
			virtual public Manah::FileBoundDocument<Ascension::DocumentUpdate>::IListener,
			virtual public Ascension::EditDoc::IStatusListener,
			virtual public Ascension::EditDoc::IUnexpectedFileTimeStampDerector {
	public:
		/// open 、reopen の結果
		enum OpenResult {
			OPENRESULT_SUCCEEDED,		///< 成功
			OPENRESULT_FAILED,			///< 失敗
			OPENRESULT_USERCANCELED,	///< ユーザがキャンセルした
		};
		/// アクティブなバッファに関する変更を受け取る
		class IActiveBufferListener {
		public:
			/// デストラクタ
			virtual ~IActiveBufferListener() {}
			/// アクティブなバッファが切り替わった
			virtual void onChangedActiveBuffer() = 0;
			/// アクティブなバッファのプロパティが変化した
			virtual void onChangedActiveBufferProperty() = 0;
		};

		// コンストラクタ
		BufferList(AlphaApp& app);
		~BufferList();
		// 属性
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
		// 操作
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

		// データメンバ
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


	/// 登録されている文書タイプの総数を返す
	inline std::size_t DocumentTypeManager::getCount() const throw() {return documentTypes_.size();}

	/// ビューの総数を返す
	inline std::size_t EditorPane::getCount() const throw() {return views_.size();}

	/// @see AbstractPane::getWindow
	inline HWND EditorPane::getWindow() const throw() {return (visibleView_ != 0) ? visibleView_->getSafeHwnd() : 0;}

	/// 表示されているバッファを返す
	inline AlphaDoc& EditorPane::getVisibleBuffer() const {return getVisibleView().getDocument();}

	/// 表示されているビューを返す
	inline AlphaView& EditorPane::getVisibleView() const {if(visibleView_ == 0) throw std::logic_error("There no views."); return *visibleView_;}

	/// アクティブなバッファを返す
	inline AlphaDoc& BufferList::getActive() const {return editorWindow_.getActivePane().getVisibleBuffer();}

	/// アクティブなバッファの位置を返す
	inline std::size_t BufferList::getActiveIndex() const {return find(getActive());}

	/// アクティブなビューを返す
	inline AlphaView& BufferList::getActiveView() const {return editorWindow_.getActivePane().getVisibleView();}

	/// 指定位置のバッファを返す
	inline AlphaDoc& BufferList::getAt(std::size_t index) const {return *buffers_.at(index);}

	/// ドキュメントの数を返す
	inline std::size_t BufferList::getCount() const throw() {return buffers_.size();}

	/// 文書タイプマネージャを返す
	inline DocumentTypeManager& BufferList::getDocumentTypeManager() const throw() {return const_cast<BufferList*>(this)->documentTypes_;}

	/// バッファのアイコンを返す
	inline HICON BufferList::getBufferIcon(std::size_t index) const {
		if(index >= getCount()) throw std::out_of_range("Index is invalid."); return icons_.getIcon(static_cast<int>(index));}

	/// エディタウィンドウを返す
	inline EditorWindow& BufferList::getEditorWindow() const throw() {return const_cast<BufferList*>(this)->editorWindow_;}

	/// バッファリストのメニューを返す
	inline const Manah::Windows::Controls::Menu& BufferList::getListMenu() const throw() {return listMenu_;}

} // namespace Alpha

#endif /* BUFFER_LIST_H_ */

/* [EOF] */