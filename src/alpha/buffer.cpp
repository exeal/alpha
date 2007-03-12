/**
 * @file buffer.cpp
 * @author exeal
 * @date 2003-2006 (was AlphaDoc.cpp and BufferList.cpp)
 * @date 2006-2007
 */

#include "stdafx.h"
#include "ascension/text-editor.hpp"	// ascension::texteditor::commands::IncrementalSearchCommand
#include "buffer.hpp"
#include "application.hpp"
#include "command.hpp"
#include "mru-manager.hpp"
#include "new-file-format-dialog.hpp"
#include "save-some-buffers-dialog.hpp"
#include "code-pages-dialog.hpp"
#include "resource.h"
#include "../manah/win32/ui/wait-cursor.hpp"
#include "../manah/com/common.hpp"	// ComPtr, ComQIPtr
#include <commdlg.h>
#include <shlwapi.h>				// PathXxxx, StrXxxx, ...
#include <shlobj.h>					// IShellLink, ...
#include <dlgs.h>
using namespace alpha;
using namespace ascension;
using namespace ascension::text;
using namespace ascension::searcher;
using namespace ascension::presentation;
using namespace ascension::encodings;
using namespace ascension::viewers;
using namespace manah::win32;
using namespace manah::win32::ui;
using namespace std;

namespace {
	struct TextFileFormat {
		CodePage encoding;
		LineBreak lineBreak;
	};
	class FileIOCallback : virtual public IFileIOListener {
	public:
		FileIOCallback(alpha::Alpha& app, bool forLoading, const WCHAR* fileName, CodePage encoding) throw()
				: app_(app), forLoading_(forLoading), fileName_(fileName),
				encoding_(encoding), retryWithOtherCodePage_(false) {}
		bool doesUserWantToChangeCodePage() const throw() {
			return retryWithOtherCodePage_;
		}
		bool unconvertableCharacterFound() {
			const int answer = app_.messageBox(forLoading_ ?
				MSG_IO__UNCONVERTABLE_NATIVE_CHAR : MSG_IO__UNCONVERTABLE_UCS_CHAR, MB_YESNOCANCEL | MB_ICONEXCLAMATION,
				MARGS % fileName_ % Alpha::getInstance().getCodePageName(encoding_)->c_str());
			if(answer == IDYES)
				retryWithOtherCodePage_ = true;
			return answer == IDNO;
		}
		IFileIOProgressListener* queryProgressCallback() {return 0;}
	private:
		Alpha& app_;
		const bool forLoading_;
		const WCHAR* const fileName_;
		const CodePage encoding_;
		bool retryWithOtherCodePage_;
	};
} // namespace @0


// Buffer ///////////////////////////////////////////////////////////////////

/// Constructor.
Buffer::Buffer() {
	presentation_.reset(new Presentation(*this));
}

/// Destructor.
Buffer::~Buffer() {
}

/// @see ascension#Document#getFileName
const WCHAR* Buffer::getFileName() const throw() {
	static const wstring untitled = Alpha::getInstance().loadString(MSG_BUFFER__UNTITLED);
	const TCHAR* const buffer = Document::getFileName();
	return (buffer != 0) ? buffer : untitled.c_str();
}

/// Returns the presentation object of Ascension.
Presentation& Buffer::getPresentation() throw() {
	return *presentation_;
}

/// Returns the presentation object of Ascension.
const Presentation& Buffer::getPresentation() const throw() {
	return *presentation_;
}


// BufferList ///////////////////////////////////////////////////////////////

const wstring BufferList::READ_ONLY_SIGNATURE_;

/**
 *	コンストラクタ
 *	@param app		アプリケーション
 */
BufferList::BufferList(Alpha& app) : app_(app) {
	// エディタウィンドウの作成
	// (WS_CLIPCHILDREN を付けると分割ウィンドウのサイズ変更枠が不可視になる...)
	editorWindow_.create(app_.getMainWindow().get(), DefaultWindowRect(), WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE, 0, *(new EditorPane));
	assert(editorWindow_.isWindow());

	// 右クリックメニューの作成
	updateContextMenu();

	if(READ_ONLY_SIGNATURE_.empty())
		const_cast<wstring&>(READ_ONLY_SIGNATURE_).assign(app_.loadString(MSG_STATUS__READ_ONLY_CAPTION));
}

/// Destructor.
BufferList::~BufferList() {
	for(EditorWindow::Iterator it = editorWindow_.enumeratePanes(); !it.isEnd(); it.next())
		it.get().removeAll();
	for(size_t i = 0; i < buffers_.size(); ++i) {
		editorSession_.removeDocument(*buffers_[i]);
		delete buffers_[i];
	}
	if(icons_.isImageList()) {
		const int c = icons_.getNumberOfImages();
		for(int i = 0; i < c; ++i)
			::DestroyIcon(icons_.getIcon(i, ILD_NORMAL));
		icons_.destroy();
	}
}

/**
 * Opens the new empty buffer.
 * @param encoding the encoding (code page)
 * @param lineBreak the line break
 */
void BufferList::addNew(CodePage cp /* = CPEX_AUTODETECT_USERLANG */, LineBreak lineBreak /* = LB_AUTO */) {
/*	if(::GetCurrentThreadId() != app_.getMainWindow().getWindowThreadID()) {
		// ウィンドウの作成をメインスレッドに委譲
		struct X : virtual public ICallable {
			X(BufferList& buffers, CodePage cp, Ascension::LineBreak lineBreak, const wstring& docType)
				: buffers_(buffer), cp_(cp), bt_(lineBreak), doctype_(docType) {}
			void call() {buffers_.addNew(cp_, bt_, doctype_);}
			BufferList&					buffers_;
			const CodePage				cp_;
			const Ascension::LineBreak	bt_;
			const wstring				doctype_;
		} x(*this, cp, lineBreak, docType);
		app_.getMainWindow().sendMessage(MYWM_CALLOVERTHREAD, 0, reinterpret_cast<LPARAM>(static_cast<ICallable*>(&x)));
		return;
	}
*/
	Buffer* const buffer = new Buffer();
	editorSession_.addDocument(*buffer);

	if(cp != CPEX_AUTODETECT_USERLANG)
		buffer->setCodePage(cp);
	if(lineBreak != LB_AUTO)
		buffer->setLineBreak(lineBreak);
	buffers_.push_back(buffer);

	// 現在のペインの数だけビューを作成する
	::LOGFONTW font;
	app_.getTextEditorFont(font);
	EditorView* originalView = 0;
	for(EditorWindow::Iterator it = editorWindow_.enumeratePanes(); !it.isEnd(); it.next()) {
		EditorView* view = (originalView == 0) ? new EditorView(buffer->getPresentation()) : new EditorView(*originalView);
		view->create(editorWindow_.get(), DefaultWindowRect(),
			WS_CHILD | WS_CLIPCHILDREN | WS_HSCROLL | WS_VISIBLE | WS_VSCROLL, WS_EX_CLIENTEDGE);
		assert(view->isWindow());
		if(originalView == 0)
			originalView = view;
		it.get().addView(*view);
		view->getTextRenderer().setFont(font.lfFaceName, font.lfHeight, 0);
	}

//	view.addEventListener(app_);
	buffer->getPresentation().addTextViewerListListener(*this);
	buffer->addStateListener(*this);
	buffer->setUnexpectedFileTimeStampDirector(this);
//	view.getLayoutSetter().setFont(lf);

	// バッファバーにボタンを追加
	AutoZero<::TBBUTTON> button;
	button.idCommand = CMD_VIEW_BUFFERLIST_START + bufferBar_.getButtonCount();
	button.iBitmap = static_cast<int>(buffers_.size() - 1);
	button.fsState = TBSTATE_ENABLED;
	button.fsStyle = BTNS_AUTOSIZE | BTNS_BUTTON | BTNS_GROUP | BTNS_NOPREFIX;
	button.iString = reinterpret_cast<INT_PTR>(buffer->getFileName());
	bufferBar_.insertButton(bufferBar_.getButtonCount(), button);

	resetResources();
	setActive(*buffer);
//	app_.applyDocumentType(*buffer, docType);
}

/// [書式を指定して新規] ダイアログを開き、バッファを新規作成する
void BufferList::addNewDialog() {
	TextFileFormat format;
	format.encoding = app_.readIntegerProfile(L"File", L"defaultCodePage", ::GetACP());
	if(format.encoding == CP_ACP)
		format.encoding = ::GetACP();
	format.lineBreak = static_cast<LineBreak>(app_.readIntegerProfile(L"file", L"defaultBreakType", LB_AUTO));
	if(format.lineBreak == LB_AUTO)
		format.lineBreak = LB_CRLF;

	ui::NewFileFormatDialog dlg(format.encoding, format.lineBreak);
	if(dlg.doModal(app_.getMainWindow().get()) != IDOK)
		return;
	addNew(dlg.getEncoding(), dlg.getLineBreak());
}

/**
 * Closes the specified buffer.
 * @param index the index of the buffer to close
 * @param queryUser set true to query user if the buffer is modified
 * @return true if the buffer was closed successfully
 * @throw std#out_of_range @a index is invalid
 */
bool BufferList::close(size_t index, bool queryUser) {
	Buffer& buffer = getAt(index);

	if(queryUser && buffer.isModified()) {
		setActive(buffer);	// 対象のバッファをユーザに見えるようにする
		const int answer = app_.messageBox(MSG_BUFFER__BUFFER_IS_DIRTY, MB_YESNOCANCEL | MB_ICONEXCLAMATION, MARGS % buffer.getFileName());
		if(answer == IDCANCEL)
			return false;
		else if(answer == IDYES && !save(index, true))
			return false;
	}

	if(buffers_.size() > 1) {
		bufferBar_.deleteButton(static_cast<int>(buffers_.size() - 1));
		for(EditorWindow::Iterator it = editorWindow_.enumeratePanes(); !it.isEnd(); it.next())
			it.get().removeBuffer(buffer);
		buffers_.erase(buffers_.begin() + index);
		editorSession_.removeDocument(buffer);
		delete &buffer;

		for(size_t i = index ; i < buffers_.size(); ++i)
			bufferBar_.setButtonText(static_cast<int>(CMD_VIEW_BUFFERLIST_START + i), getDisplayName(*buffers_[i]).c_str());
		resetResources();
		recalculateBufferBarSize();
	} else {	// 最後の1つの場合
		if(!buffer.isBoundToFile()) {
			buffer.resetContent();			// 新規バッファの場合、resetContent() でもファイル名が変わらないため
//			app_.applyDocumentType(buffer);	// 文書タイプの変更イベントが発行されない
			// TODO: call mode-application.
		} else
			buffer.resetContent();
	}
	return true;
}

/**
 *	全て、或いは非アクティブな全てのバッファを閉じる
 *	@param queryUser	未保存のバッファがある場合、ユーザに問い合わせる場合 true
 *	@param exceptActive	アクティブなバッファを残す場合 true
 *	@return				1つもユーザに拒否されなければ true
 */
bool BufferList::closeAll(bool queryUser, bool exceptActive /* = false */) {
	const size_t active = getActiveIndex();

	app_.getMainWindow().lockUpdate();

	// 先に保存の必要の無いバッファを全部閉じる
	for(size_t i = buffers_.size(); i != 0; --i) {
		if(i - 1 == active && exceptActive)
			continue;
		if(!buffers_[i - 1]->isModified())
			close(i - 1, false);
	}

	app_.getMainWindow().unlockUpdate();

	// 未保存のバッファが無ければ終了
	if(buffers_.size() == 1) {
		if(exceptActive || !buffers_[0]->isModified())
			return true;
	}

	// 未保存のバッファが1つなら通常の確認ダイアログを出して終わり
	if(buffers_.size() - (exceptActive ? 1 : 0) == 1) {
		const size_t dirty = !exceptActive ? 0 : ((active == 0) ? 1 : 0);

		if(buffers_[dirty]->isModified()) {
			if(exceptActive)
				setActive(dirty);
			return close(getActiveIndex(), queryUser);
		}
	}

	// 複数のバッファを保存するかどうか確認ダイアログを出す
	ui::SaveSomeBuffersDialog dlg;
	for(size_t i = 0; i < buffers_.size(); ++i) {
		if(exceptActive && i == active)
			continue;
		ui::DirtyFile df;
		df.index = static_cast<uint>(i);
		df.fileName = buffers_[i]->getFileName();
		df.save = true;
		dlg.files_.push_back(df);
	}
	if(IDOK != dlg.doModal(app_.getMainWindow().get()))
		return false;

	// 保存する
	for(vector<ui::DirtyFile>::reverse_iterator it = dlg.files_.rbegin(); it != dlg.files_.rend(); ++it) {
		if(it->save) {
			if(!save(it->index, true))
				return false;
		}
		if(!close(it->index, false))
			return false;
	}

	return true;
}

/**
 * Reconstructs the buffer bar.
 * @param rebar the rebar on which the buffer bar set
 * @return success or not
 */
bool BufferList::createBar(Rebar& rebar) {
	if(bufferBarPager_.isWindow()) {
		rebar.deleteBand(rebar.idToIndex(IDC_BUFFERBARPAGER));
		bufferBar_.destroy();
		bufferBarPager_.destroy();
	}

	// バッファバーとページャを作成する
	if(!bufferBarPager_.create(rebar.get(), DefaultWindowRect(), 0, IDC_BUFFERBARPAGER,
			WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE | CCS_NORESIZE | PGS_HORZ))
		return false;
	if(!bufferBar_.create(bufferBarPager_.get(), DefaultWindowRect(), 0, IDC_BUFFERBAR,
			WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE
			| CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE | CCS_TOP
			| TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_REGISTERDROP | TBSTYLE_TOOLTIPS | TBSTYLE_TRANSPARENT, WS_EX_TOOLWINDOW)) {
		bufferBarPager_.destroy();
		return false;
	}
	HWND toolTips = bufferBar_.getToolTips();
	bufferBar_.setButtonStructSize();
	::SetWindowLongPtrW(toolTips, GWL_STYLE, ::GetWindowLongPtrW(toolTips, GWL_STYLE) | TTS_NOPREFIX);
	bufferBarPager_.setChild(bufferBar_.get());

	// レバーに乗せる
	AutoZeroCB<::REBARBANDINFOW> rbbi;
	const wstring caption = app_.loadString(MSG_DIALOG__BUFFERBAR_CAPTION);
	rbbi.fMask = RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_ID | RBBIM_STYLE | RBBIM_TEXT;
	rbbi.fStyle = RBBS_BREAK | RBBS_GRIPPERALWAYS;
	rbbi.cxMinChild = 0;
	rbbi.cyMinChild = 22;
	rbbi.wID = IDC_BUFFERBAR;
	rbbi.lpText = const_cast<wchar_t*>(caption.c_str());
	rbbi.hwndChild = bufferBarPager_.get();
	if(!rebar.insertBand(rebar.getBandCount(), rbbi)) {
		bufferBar_.destroy();
		bufferBarPager_.destroy();
		return false;
	}
	return true;
}

/**
 * Finds the buffer in the list.
 * @param buffer the buffer to find
 * @return the index of the buffer or -1 if not found
 */
size_t BufferList::find(const Buffer& buffer) const {
	for(size_t i = 0; i < buffers_.size(); ++i) {
		if(buffers_[i] == &buffer)
			return i;
	}
	return -1;
}

/**
 * Finds the buffer in the list.
 * @param fileName the name of the buffer to find
 * @return the index of the buffer or -1 if not found
 */
size_t BufferList::find(const basic_string<WCHAR>& fileName) const {
	for(size_t i = 0; i < buffers_.size(); ++i) {
		if(buffers_[i]->isBoundToFile() && Document::areSamePathNames(buffers_[i]->getFilePathName(), fileName.c_str()))
			return i;
	}
	return -1;
}

/// IActiveBufferListener::activeBufferSwitched のトリガ
void BufferList::fireActiveBufferSwitched() {
	const size_t activeBuffer = getActiveIndex();
	const EditorView& activeView = getActiveView();

	bufferBar_.checkButton(static_cast<int>(activeBuffer) + CMD_VIEW_BUFFERLIST_START);
	for(EditorWindow::Iterator it = editorWindow_.enumeratePanes(); !it.isEnd(); it.next()) {
		if(it.get().getCount() > 0 && &it.get().getVisibleView() == &activeView) {
			editorWindow_.setDefaultActivePane(it.get());
			break;
		}
	}

	// アクティブなバッファのボタンが隠れていたらスクロールする
	if(bufferBarPager_.isVisible()) {
		const int pagerPos = bufferBarPager_.getPosition();
		RECT buttonRect, pagerRect;
		bufferBar_.getItemRect(static_cast<int>(activeBuffer), buttonRect);
		bufferBarPager_.getClientRect(pagerRect);
		if(buttonRect.left < pagerPos)
			bufferBarPager_.setPosition(buttonRect.left);
		else if(buttonRect.right > pagerPos + pagerRect.right)
			bufferBarPager_.setPosition(buttonRect.right - pagerRect.right);
	}
}

/**
 * Translates the abstract document into a @c Buffer.
 * @document the document
 * @throw std#invalid_argument @a document is not found
 * @see BufferList#find
 */
Buffer& BufferList::getConcreteDocument(Document& document) const {
	for(size_t i = 0; i < buffers_.size(); ++i) {
		if(buffers_[i] == &document)
			return *buffers_[i];
	}
	throw invalid_argument("The specified document is not in the list.");
}

/**
 * タイトルバーやバッファバーに表示するバッファの名前を返す
 * @param buffer the buffer
 * @return the name
 */
wstring BufferList::getDisplayName(const Buffer& buffer) {
	const std::wstring str = buffer.getFileName();
	if(buffer.isModified())
		return buffer.isReadOnly() ? str + L" * " + READ_ONLY_SIGNATURE_ : str + L" *";
	else
		return buffer.isReadOnly() ? str + L" " + READ_ONLY_SIGNATURE_ : str;
}

/// Handles @c WM_NOTIFY message from the buffer bar.
LRESULT BufferList::handleBufferBarNotification(::NMTOOLBAR& nmhdr) {
	if(nmhdr.hdr.code == NM_RCLICK) {	// 右クリック
		const NMMOUSE& mouse = *reinterpret_cast<::NMMOUSE*>(&nmhdr.hdr);
		if(mouse.dwItemSpec != -1) {
			::POINT pt = mouse.pt;
			bufferBar_.clientToScreen(pt);
			setActive(mouse.dwItemSpec - CMD_VIEW_BUFFERLIST_START);
			contextMenu_.trackPopup(TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON, pt.x, pt.y, Alpha::getInstance().getMainWindow().get());
			return true;
		}
	}

	else if(nmhdr.hdr.code == TTN_GETDISPINFOW) {	// ツールチップ
		assert(nmhdr.hdr.idFrom >= CMD_VIEW_BUFFERLIST_START && nmhdr.hdr.idFrom < CMD_VIEW_BUFFERLIST_END);
		static wchar_t tipText[500];
		::NMTTDISPINFOW& nmttdi = *reinterpret_cast<::NMTTDISPINFOW*>(&nmhdr.hdr);

//		nmttdi->hinst = getHandle();
		const Buffer& buffer = getAt(nmttdi.hdr.idFrom - CMD_VIEW_BUFFERLIST_START);
		wcscpy(tipText, (buffer.getFilePathName() != 0) ? buffer.getFilePathName() : buffer.getFileName());
		nmttdi.lpszText = tipText;
		return true;
	}

	else if(nmhdr.hdr.code == TBN_ENDDRAG && bufferBar_.getButtonCount() > 1) {
		::TBINSERTMARK mark;
		bufferBar_.getInsertMark(mark);
		if(mark.iButton != -1) {
			// ボタンを移動する
			move(bufferBar_.commandToIndex(nmhdr.iItem),
				toBoolean(mark.dwFlags & TBIMHT_AFTER) ? mark.iButton + 1 : mark.iButton);
			// 挿入マークを消す
			mark.dwFlags = 0;
			mark.iButton = -1;
			bufferBar_.setInsertMark(mark);
		}
	}

	// ドラッグ -> アクティブなバッファを切り替える
	else if(nmhdr.hdr.code == TBN_GETOBJECT) {
		::NMOBJECTNOTIFY& n = *reinterpret_cast<::NMOBJECTNOTIFY*>(&nmhdr.hdr);
		if(n.iItem != -1) {
			setActive(bufferBar_.commandToIndex(n.iItem));	// n.iItem は ID
			n.pObject = 0;
			n.hResult = E_NOINTERFACE;
		}
		return 0;
	}

	else if(nmhdr.hdr.code == TBN_HOTITEMCHANGE && bufferBar_.getButtonCount() > 1 && bufferBar_.get() == ::GetCapture()) {
		::NMTBHOTITEM& hotItem = *reinterpret_cast<::NMTBHOTITEM*>(&nmhdr.hdr);
		if(toBoolean(hotItem.dwFlags & HICF_MOUSE)) {	// ボタンをドラッグ中
			::TBINSERTMARK mark;
			if(!toBoolean(hotItem.dwFlags & HICF_LEAVING)) {
				// 挿入マークを移動させる
				mark.dwFlags = 0;
				mark.iButton = bufferBar_.commandToIndex(hotItem.idNew);
			} else {
				mark.dwFlags = TBIMHT_AFTER;
				mark.iButton = bufferBar_.getButtonCount() - 1;
			}
			bufferBar_.setInsertMark(mark);
		}
	}

	return false;
}

/// Handles @c WM_NOTIFY message from the pager of the buffer bar.
LRESULT BufferList::handleBufferBarPagerNotification(NMHDR& nmhdr) {
	if(nmhdr.code == PGN_CALCSIZE) {	// ページャサイズの計算
		LPNMPGCALCSIZE p = reinterpret_cast<LPNMPGCALCSIZE>(&nmhdr);
		if(p->dwFlag == PGF_CALCWIDTH) {
			SIZE size;
			bufferBar_.getMaxSize(size);
			p->iWidth = size.cx;
		} else if(p->dwFlag == PGF_CALCHEIGHT) {
			SIZE size;
			bufferBar_.getMaxSize(size);
			p->iHeight = size.cy;
		}
		return true;
	}

	else if(nmhdr.code == PGN_SCROLL) {	// ページャのスクロール量の設定
		NMPGSCROLL* p = reinterpret_cast<NMPGSCROLL*>(&nmhdr);
		p->iScroll = 20;
		if(toBoolean(p->fwKeys & PGK_SHIFT))	// 逆方向
			p->iScroll *= -1;
		if(toBoolean(p->fwKeys & PGK_CONTROL))	// 倍速
			p->iScroll *= 2;
		return true;
	}

	return false;
}

/**
 * ファイルを開いたり保存したりするのに失敗したときの処理
 * @param fileName 処理中のファイル名 (このバッファがアクティブになっていなければならない)
 * @param forLoading 呼び出し元がファイルを開こうとしたとき true
 * @param result エラー内容
 * @return 結果的にエラーである場合 false
 */
bool BufferList::handleFileIOError(const WCHAR* fileName, bool forLoading, Document::FileIOResult result) {
	assert(fileName != 0);
	if(result == Document::FIR_OK)
		return true;
	else if(result == Document::FIR_STANDARD_WIN32_ERROR) {
		void* buffer = 0;
		wstring message = fileName;
		::FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			0, ::GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<wchar_t*>(&buffer), 0, 0);
		message += L"\n\n";
		message += static_cast<wchar_t*>(buffer);
		::LocalFree(buffer);
		app_.getMainWindow().messageBox(message.c_str(), IDS_APPNAME, MB_ICONEXCLAMATION);
	} else if(result != Document::FIR_CALLER_ABORTED) {
		DWORD messageID;
		switch(result) {
		case Document::FIR_INVALID_CODE_PAGE:				messageID = MSG_IO__INVALID_ENCODE; break;
		case Document::FIR_INVALID_LINE_BREAK:				messageID = MSG_IO__INVALID_LINE_BREAK; break;
		case Document::FIR_OUT_OF_MEMORY:					messageID = MSG_ERROR__OUT_OF_MEMORY; break;
		case Document::FIR_HUGE_FILE:						messageID = MSG_IO__HUGE_FILE; break;
		case Document::FIR_READ_ONLY_MODE:					messageID = MSG_IO__FAILED_TO_WRITE_FOR_READONLY; break;
		case Document::FIR_CANNOT_CREATE_TEMPORARY_FILE:	messageID = MSG_IO__CANNOT_CREATE_TEMP_FILE; break;
		case Document::FIR_LOST_DISK_FILE:					messageID = MSG_IO__LOST_DISK_FILE; break;
		}
		app_.messageBox(messageID, MB_ICONEXCLAMATION, MARGS % fileName);
	}
	if(forLoading)
		close(getActiveIndex(), false);
	return false;
}

/**
 * バッファをリスト内で移動させる
 * @param from 移動するバッファの番号
 * @param to 移動先
 * @throw std::out_of_range @a from が不正なときスロー
 */
void BufferList::move(size_t from, size_t to) {
	if(from >= buffers_.size() || to > buffers_.size())
		throw out_of_range("The specified index is out of range.");
	else if(from == to)
		return;

	// リスト内で移動
	Buffer* buffer = buffers_[from];
	buffers_.erase(buffers_.begin() + from);
	buffers_.insert((from < to) ? buffers_.begin() + to - 1 : buffers_.begin() + to, buffer);

	// バッファバーのボタンを並び替え
	const int end = min(static_cast<int>(max(from, to)), bufferBar_.getButtonCount() - 1);
	for(int i = static_cast<int>(min(from, to)); i <= end; ++i)
		bufferBar_.setButtonText(CMD_VIEW_BUFFERLIST_START + i, getDisplayName(*buffers_[i]).c_str());
	setActive(*buffer);
	resetResources();
}

/// @see ascension#presentation#ITextViewerListListener#textViewerListChanged
void BufferList::textViewerListChanged(Presentation& presentation) {
}

/// @see ascension#text#IDocumentStateListener#documentModificationSignChanged
void BufferList::documentModificationSignChanged(Document& document) {
	const Buffer& buffer = getConcreteDocument(document);
	bufferBar_.setButtonText(static_cast<int>(CMD_VIEW_BUFFERLIST_START + find(buffer)), getDisplayName(buffer).c_str());
	recalculateBufferBarSize();
}

/// @see ascension#text#IDocumentStateListener#documentFileNameChanged
void BufferList::documentFileNameChanged(Document& document) {
	Buffer& buffer = getConcreteDocument(document);
	// TODO: call mode-application.
	resetResources();
	bufferBar_.setButtonText(static_cast<int>(CMD_VIEW_BUFFERLIST_START + find(buffer)), getDisplayName(buffer).c_str());
	bufferBarPager_.recalcSize();
}

/// @see ascension#text#IDocumentStateListener#documentAccessibleRegionChanged
void BufferList::documentAccessibleRegionChanged(Document& document) {
}

/// @see ascension#text#IDocumentStateListener#documentEncodingChanged
void BufferList::documentEncodingChanged(Document& document) {
}

/// @see ascension#text#IDocumentStateListenerdocumentReadOnlySignChanged
void BufferList::documentReadOnlySignChanged(Document& document) {
	const Buffer& buffer = getConcreteDocument(document);
	bufferBar_.setButtonText(static_cast<int>(CMD_VIEW_BUFFERLIST_START + find(buffer)), getDisplayName(buffer).c_str());
	recalculateBufferBarSize();
}

/**
 * Opens the specified file.
 * This method may show a dialog to indicate the result.
 * @param fileName the name of the file to open
 * @param encoding the encoding (code page). auto-detect if omitted
 * @param asReadOnly set true to open as read only
 * @param addToMRU set true to add the file to MRU
 * @return the result. see the description of @c BufferList#OpenResult
 */
BufferList::OpenResult BufferList::open(const basic_string<WCHAR>& fileName,
		CodePage encoding /* = CPEX_AUTOSELECT */, bool asReadOnly /* = false */, bool addToMRU /* = true */) {
	WCHAR resolvedName[MAX_PATH];

	// ショートカットの解決
	const WCHAR* extension = ::PathFindExtensionW(fileName.c_str());
	if(wcslen(extension) != 0 && (
			(::StrCmpIW(extension + 1, L"lnk") == 0)
			/*|| (::StrCmpIW(extension + 1, L"url") == 0)*/)) {
		manah::com::ComPtr<IShellLinkW> shellLink;
		manah::com::ComQIPtr<IPersistFile> file;
		HRESULT hr;

		try {
			if(FAILED(hr = shellLink.createInstance(CLSID_ShellLink)))
				throw hr;
			if(FAILED(hr = shellLink->QueryInterface(IID_IPersistFile, &file)))
				throw hr;
			if(FAILED(hr = file->Load(fileName.c_str(), STGM_READ)))
				throw hr;
			if(FAILED(hr = shellLink->Resolve(0, SLR_ANY_MATCH | SLR_NO_UI)))
				throw hr;
			if(FAILED(hr = shellLink->GetPath(resolvedName, MAX_PATH, 0, 0)))
				throw hr;
		} catch(HRESULT /*hr_*/) {
			app_.messageBox(MSG_IO__FAILED_TO_RESOLVE_SHORTCUT, MB_ICONHAND, MARGS % fileName);
			return OPENRESULT_FAILED;
		}
	} else if(!Document::canonicalizePathName(fileName.c_str(), resolvedName))
		wcscpy(resolvedName, fileName.c_str());

	// (テキストエディタで) 既に開かれているか調べる
	const size_t oldBuffer = find(resolvedName);
	if(oldBuffer != -1) {
		setActive(oldBuffer);
		return OPENRESULT_SUCCEEDED;
	}

	Buffer* buffer = &getActive();
	Document::FileLockMode lockMode;
	lockMode.onlyAsEditing = false;
	switch(app_.readIntegerProfile(L"File", L"shareMode", 0)) {
	case 0:	lockMode.type = Document::FileLockMode::LOCK_TYPE_NONE; break;
	case 1:	lockMode.type = Document::FileLockMode::LOCK_TYPE_SHARED; break;
	case 2:	lockMode.type = Document::FileLockMode::LOCK_TYPE_EXCLUSIVE; break;
	}

	if(buffer->isModified() || buffer->getFilePathName() != 0) {	// 新しいコンテナで開く
		addNew(encoding);
		buffer = &getActive();
	}

	if(!EncoderFactory::getInstance().isCodePageForAutoDetection(encoding)) {
//		try {
			buffer->setCodePage(encoding);
//		} catch(invalid_argument&) {
//			if(IDNO == app_.messageBox(MSG_ILLEGAL_CODEPAGE, MB_YESNO | MB_ICONEXCLAMATION))
//				return OPENRESULT_USERCANCELED;
//			encoding = ::GetACP();
//			buffer->setCodePage(encoding);
//		}
	}

	const wstring s = app_.loadString(MSG_STATUS__LOADING_FILE, MARGS % resolvedName);
	Document::FileIOResult result;
	do {
		WaitCursor wc;
		FileIOCallback callback(app_, true, resolvedName, encoding);

		app_.setStatusText(s.c_str());
		app_.getMainWindow().lockUpdate();

		// 準備ができたのでファイルを開く
		result = buffer->load(resolvedName, lockMode, encoding, &callback);
		app_.setStatusText(0);
		app_.getMainWindow().unlockUpdate();

		// ユーザがコードページの変更を要求していた
		if(callback.doesUserWantToChangeCodePage()) {
			assert(result == Document::FIR_CALLER_ABORTED);
			ui::CodePagesDialog dlg(encoding, true);
			if(dlg.doModal(app_.getMainWindow().get()) == IDOK) {
				encoding = dlg.getCodePage();
				continue;	// コードページを変えて再挑戦
			}
		}
		break;
	} while(true);

	app_.getMainWindow().show(app_.getMainWindow().isVisible() ? SW_SHOW : SW_RESTORE);

	if(handleFileIOError(resolvedName, true, result)) {
		if(asReadOnly)
			buffer->setReadOnly();
		if(addToMRU)
			app_.getMRUManager().add(buffer->getFilePathName(), buffer->getCodePage());
		return OPENRESULT_SUCCEEDED;
	}
	return (result != Document::FIR_CALLER_ABORTED) ? OPENRESULT_FAILED : OPENRESULT_USERCANCELED;
}

/**
 * Shows "Open" dialog box and opens file(s).
 * @param initialDirectory the directory name to show in the dialog first. if @c null, the directory
 * of the active buffer. if the active buffer is not bound to a file, the system default
 * @return the result. see the description of @c BufferList::OpenResult
 */
BufferList::OpenResult BufferList::openDialog(const WCHAR* initialDirectory /* = 0 */) {
	wstring filterSource = app_.readStringProfile(L"File", L"filter", app_.loadString(MSG_DIALOG__DEFAULT_OPENFILE_FILTER).c_str());
	wchar_t* filter = new wchar_t[filterSource.length() + 2];
	WCHAR fileName[MAX_PATH + 1] = L"";
	wstring errorMessage;

	// フィルタを整形
	replace_if(filterSource.begin(), filterSource.end(), bind2nd(equal_to<wchar_t>(), L':'), L'\0');
	filterSource.copy(filter, filterSource.length());
	filter[filterSource.length()] = L'\0';
	filter[filterSource.length() + 1] = L'\0';

	AutoZero<::OSVERSIONINFOW> osVersion;
	osVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
	::GetVersionEx(&osVersion);

	WCHAR* activeBufferDir = 0;
	if(initialDirectory == 0 && getActive().getFilePathName() != 0) {
		activeBufferDir = new WCHAR[MAX_PATH];
		wcscpy(activeBufferDir, getActive().getFilePathName());
		*::PathFindFileNameW(activeBufferDir) = 0;
		if(activeBufferDir[0] == 0) {
			delete[] activeBufferDir;
			activeBufferDir = 0;
		}
	}

	TextFileFormat format = {CPEX_AUTODETECT_USERLANG, LB_AUTO};
	AutoZeroLS<::OPENFILENAMEW> newOfn;
	AutoZeroLS<::OPENFILENAME_NT4W> oldOfn;
	::OPENFILENAMEW& ofn = (osVersion.dwMajorVersion > 4) ? newOfn : *reinterpret_cast<::OPENFILENAMEW*>(&oldOfn);
	ofn.hwndOwner = app_.getMainWindow().get();
	ofn.hInstance = ::GetModuleHandle(0);
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = fileName;
	ofn.lpstrInitialDir = (initialDirectory != 0) ? initialDirectory : activeBufferDir;
	ofn.nFilterIndex = app_.readIntegerProfile(L"File", L"activeFilter", 0);
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_ALLOWMULTISELECT | OFN_ENABLEHOOK | OFN_ENABLESIZING | OFN_ENABLETEMPLATE
		| OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST/* | OFN_SHOWHELP*/;
	ofn.lCustData = reinterpret_cast<LPARAM>(&format);
	ofn.lpfnHook = openFileNameHookProc;
	ofn.lpTemplateName = MAKEINTRESOURCE(IDD_DLG_OPENFILE);

	const bool succeeded = toBoolean(::GetOpenFileNameW(&ofn));
	delete[] filter;
	delete[] activeBufferDir;
	app_.writeIntegerProfile(L"File", L"activeFilter", ofn.nFilterIndex);	// 最後に使ったフィルタを保存

	if(succeeded) {
		const wstring directory = ofn.lpstrFile;

		if(directory.length() > ofn.nFileOffset)	// 開くファイルは1つ
			return open(directory, format.encoding, toBoolean(ofn.Flags & OFN_READONLY));	// pOfn->lpstrFile は完全ファイル名
		else {	// 複数のファイルを開く場合
			wchar_t* fileNames = ofn.lpstrFile + ofn.nFileOffset;
			bool failedOnce = false;

			while(*fileNames != 0) {
				const size_t len = wcslen(fileNames);
				if(open(directory + L"\\" + wstring(fileNames, len),
						format.encoding, toBoolean(ofn.Flags & OFN_READONLY)) != OPENRESULT_SUCCEEDED)
					failedOnce = true;
				fileNames += len + 1;
			}
			return failedOnce ? OPENRESULT_FAILED : OPENRESULT_SUCCEEDED;
		}
	} else
		return OPENRESULT_FAILED;
}

/// Hook procedure for @c GetOpenFileNameW and @c GetSaveFileNameW.
UINT_PTR CALLBACK BufferList::openFileNameHookProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
	switch(message) {
	case WM_COMMAND:
		// [コードページ] が変更された
		if(LOWORD(wParam) == IDC_COMBO_CHARCODE && HIWORD(wParam) == CBN_SELCHANGE) {
			ComboBox breakCodeCombobox(::GetDlgItem(window, IDC_COMBO_BREAKCODE));
			if(!breakCodeCombobox.isWindow())
				break;
			ComboBox codePageCombobox(::GetDlgItem(window, IDC_COMBO_CHARCODE));

			const wstring unchange = Alpha::getInstance().loadString(MSG_DIALOG__UNCHANGE_LINE_BREAK);
			const CodePage cp = codePageCombobox.getItemData(codePageCombobox.getCurSel());
			const int breakCode = (breakCodeCombobox.getCount() != 0) ? breakCodeCombobox.getCurSel() : 0;

			if(cp == CPEX_UNICODE_UTF5 || cp == CP_UTF7 || cp == CP_UTF8
					|| cp == CPEX_UNICODE_UTF16LE || cp == CPEX_UNICODE_UTF16BE
					|| cp == CPEX_UNICODE_UTF32LE || cp == CPEX_UNICODE_UTF32BE) {
				if(breakCodeCombobox.getCount() != 7) {
					breakCodeCombobox.resetContent();
					breakCodeCombobox.setItemData(breakCodeCombobox.addString(unchange.c_str()), LB_AUTO);
					breakCodeCombobox.setItemData(breakCodeCombobox.addString(IDS_BREAK_CRLF), LB_CRLF);
					breakCodeCombobox.setItemData(breakCodeCombobox.addString(IDS_BREAK_LF), LB_LF);
					breakCodeCombobox.setItemData(breakCodeCombobox.addString(IDS_BREAK_CR), LB_CR);
					breakCodeCombobox.setItemData(breakCodeCombobox.addString(IDS_BREAK_NEL), LB_NEL);
					breakCodeCombobox.setItemData(breakCodeCombobox.addString(IDS_BREAK_LS), LB_LS);
					breakCodeCombobox.setItemData(breakCodeCombobox.addString(IDS_BREAK_PS), LB_PS);
					breakCodeCombobox.setCurSel(breakCode);
				}
			} else {
				if(breakCodeCombobox.getCount() != 4) {
					breakCodeCombobox.resetContent();
					breakCodeCombobox.setItemData(breakCodeCombobox.addString(unchange.c_str()), LB_AUTO);
					breakCodeCombobox.setItemData(breakCodeCombobox.addString(IDS_BREAK_CRLF), LB_CRLF);
					breakCodeCombobox.setItemData(breakCodeCombobox.addString(IDS_BREAK_LF), LB_LF);
					breakCodeCombobox.setItemData(breakCodeCombobox.addString(IDS_BREAK_CR), LB_CR);
					breakCodeCombobox.setCurSel((breakCode < 4) ? breakCode : 0);
				}
			}
		}
		break;
	case WM_INITDIALOG: {
		OPENFILENAMEW& ofn = *reinterpret_cast<OPENFILENAMEW*>(lParam);
		HWND dialog = ::GetParent(window);
		ComboBox codePageCombobox(::GetDlgItem(window, IDC_COMBO_CHARCODE));
		Static codePageLabel(::GetDlgItem(window, IDC_STATIC_1));
		ComboBox breakCodeCombobox(::GetDlgItem(window, IDC_COMBO_BREAKCODE));
		Static breakCodeLabel(::GetDlgItem(window, IDC_STATIC_2));
		HFONT guiFont = reinterpret_cast<HFONT>(::SendMessage(dialog, WM_GETFONT, 0, 0L));

		// ダイアログテンプレートのコントロールの位置合わせなど
		POINT pt;
		RECT rect;
		::GetWindowRect(window, &rect);
		pt.x = rect.left;
		pt.y = rect.top;

		// ラベル
		::GetWindowRect(::GetDlgItem(dialog, stc2), &rect);
		long x = rect.left;
		codePageLabel.getRect(rect);
		codePageLabel.setPosition(0, x - pt.x, rect.top - pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		codePageLabel.setFont(guiFont);
		if(breakCodeLabel.isWindow()) {
			breakCodeLabel.getRect(rect);
			breakCodeLabel.setPosition(0, x - pt.x, rect.top - pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
			breakCodeLabel.setFont(guiFont);
		}

		// コンボボックス
		::GetWindowRect(::GetDlgItem(dialog, cmb1), &rect);
		x = rect.left;
		codePageCombobox.getRect(rect);
		codePageCombobox.setPosition(0, x - pt.x, rect.top - pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		codePageCombobox.setFont(guiFont);
		if(breakCodeCombobox.isWindow()) {
			breakCodeCombobox.getRect(rect);
			breakCodeCombobox.setPosition(0, x - pt.x, rect.top - pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
			breakCodeCombobox.setFont(guiFont);
		}

		const EncoderFactory& encoders = EncoderFactory::getInstance();
		set<CodePage> codePages;
		encoders.enumCodePages(codePages);
		for(set<CodePage>::const_iterator it = codePages.begin(); it != codePages.end(); ++it) {
			if(breakCodeCombobox.isWindow()
					&& (encoders.isCodePageForAutoDetection(*it) || encoders.isCodePageForAutoDetection(*it)))
				continue;
			else if(const wstring* name = Alpha::getInstance().getCodePageName(*it))
				codePageCombobox.setItemData(codePageCombobox.addString(name->c_str()), *it);
		}

		const UINT c = codePageCombobox.getCount();
		codePageCombobox.setCurSel(0);
		for(UINT i = 0; i < c; ++i) {
			if(reinterpret_cast<TextFileFormat*>(ofn.lCustData)->encoding == codePageCombobox.getItemData(i)) {
				codePageCombobox.setCurSel(i);
				break;
			}
		}

		if(breakCodeCombobox.isWindow()) {
			switch(reinterpret_cast<TextFileFormat*>(ofn.lCustData)->lineBreak) {
			case LB_AUTO:	breakCodeCombobox.setCurSel(0);	break;
			case LB_CRLF:	breakCodeCombobox.setCurSel(1);	break;
			case LB_LF:		breakCodeCombobox.setCurSel(2);	break;
			case LB_CR:		breakCodeCombobox.setCurSel(3);	break;
			case LB_NEL:	breakCodeCombobox.setCurSel(4);	break;
			case LB_LS:		breakCodeCombobox.setCurSel(5);	break;
			case LB_PS:		breakCodeCombobox.setCurSel(6);	break;
			}
			::SendMessageW(window, WM_COMMAND, MAKEWPARAM(IDC_COMBO_CHARCODE, CBN_SELCHANGE), 0);
		}
	}
		break;
	case WM_NOTIFY: {
		OFNOTIFYW& ofn = *reinterpret_cast<OFNOTIFYW*>(lParam);
		if(ofn.hdr.code == CDN_FILEOK) {	// [開く]/[保存]
			ComboBox codePageCombobox(::GetDlgItem(window, IDC_COMBO_CHARCODE));
			ComboBox breakCodeCombobox(::GetDlgItem(window, IDC_COMBO_BREAKCODE));
			Button readOnlyCheckbox(::GetDlgItem(::GetParent(window), chx1));
			TextFileFormat& format = *reinterpret_cast<TextFileFormat*>(ofn.lpOFN->lCustData);

			format.encoding = codePageCombobox.getItemData(codePageCombobox.getCurSel());
			if(breakCodeCombobox.isWindow()) {
				switch(breakCodeCombobox.getCurSel()) {
				case 0:	format.lineBreak = LB_AUTO;	break;
				case 1:	format.lineBreak = LB_CRLF;	break;
				case 2:	format.lineBreak = LB_LF;	break;
				case 3:	format.lineBreak = LB_CR;	break;
				case 4:	format.lineBreak = LB_NEL;	break;
				case 5:	format.lineBreak = LB_LS;	break;
				case 6:	format.lineBreak = LB_PS;	break;
				}
			}
			if(readOnlyCheckbox.isWindow()) {
				// 複数ファイルの場合、チェックボックスの状態が無視される
				// (意図的かもしれない)
				if(readOnlyCheckbox.getCheck() == BST_CHECKED)
					ofn.lpOFN->Flags |= OFN_READONLY;
				else
					ofn.lpOFN->Flags &= ~OFN_READONLY;
			}
		}
	}
		break;
	}

	return 0L;
}

/// @see ascension#text#IUnexpectedFileTimeStampDirector::queryAboutUnexpectedTimeStamp
bool BufferList::queryAboutUnexpectedDocumentFileTimeStamp(
		Document& document, IUnexpectedFileTimeStampDirector::Context context) throw() {
	const Buffer& buffer = getConcreteDocument(document);
	const size_t a = getActiveIndex();
	setActive(buffer);
	switch(context) {
	case IUnexpectedFileTimeStampDirector::FIRST_MODIFICATION:
		return app_.messageBox(MSG_BUFFER__FILE_IS_MODIFIED_AND_EDIT, MB_YESNO | MB_ICONQUESTION, MARGS % buffer.getFilePathName()) == IDYES;
	case IUnexpectedFileTimeStampDirector::OVERWRITE_FILE:
		return app_.messageBox(MSG_BUFFER__FILE_IS_MODIFIED_AND_SAVE, MB_YESNO | MB_ICONQUESTION, MARGS % buffer.getFilePathName()) == IDYES;
	case IUnexpectedFileTimeStampDirector::CLIENT_INVOCATION:
		if(IDYES == app_.messageBox(MSG_BUFFER__FILE_IS_MODIFIED_AND_REOPEN, MB_YESNO | MB_ICONQUESTION, MARGS % buffer.getFilePathName()))
			reopen(find(buffer), false);
		else
			setActive(a);
		return true;
	}
	return false;
}

/// Recalculates the size of the buffer bar.
void BufferList::recalculateBufferBarSize() {
	bufferBarPager_.recalcSize();

	// バッファバーの理想長さの再計算
	if(bufferBar_.isVisible()) {
		AutoZero<::REBARBANDINFOW> rbbi;
		Rebar rebar(bufferBarPager_.getParent()->get());
		::RECT rect;
		rbbi.fMask = RBBIM_IDEALSIZE;
		bufferBar_.getItemRect(bufferBar_.getButtonCount() - 1, rect);
		rbbi.cxIdeal = rect.right;
		rebar.setBandInfo(rebar.idToIndex(IDC_BUFFERBAR), rbbi);
	}
}

/**
 * Reopens the specified buffer.
 * @param index the index of the buffer to reopen
 * @param changeCodePage set true to change the encoding
 * @return the result. see the description of @c BufferList#OpenResult
 * @throw std#out_of_range @a index is invalid
 */
BufferList::OpenResult BufferList::reopen(size_t index, bool changeCodePage) {
	Buffer& buffer = getAt(index);

	// ファイルが存在するか?
	if(!buffer.isBoundToFile())
		return OPENRESULT_FAILED;

	// ユーザがキャンセル
	else if(buffer.isModified() && IDNO == app_.messageBox(MSG_BUFFER__CONFIRM_REOPEN_EVEN_IF_DIRTY, MB_YESNO | MB_ICONQUESTION))
		return OPENRESULT_USERCANCELED;

	// コードページを変更する場合はダイアログを出す
	CodePage cp = buffer.getCodePage();
	if(changeCodePage) {
		ui::CodePagesDialog dlg(cp, true);
		if(dlg.doModal(app_.getMainWindow().get()) != IDOK)
			return OPENRESULT_USERCANCELED;
		cp = dlg.getCodePage();
	}

	Document::FileIOResult result;
	do {
		FileIOCallback callback(app_, true, buffer.getFilePathName(), cp);

		result = buffer.load(buffer.getFilePathName(), buffer.getLockMode(), cp, &callback);
		if(callback.doesUserWantToChangeCodePage()) {
			assert(result == Document::FIR_CALLER_ABORTED);
			ui::CodePagesDialog dlg(cp, true);
			if(dlg.doModal(app_.getMainWindow().get()) != IDOK)
				return OPENRESULT_USERCANCELED;
			cp = dlg.getCodePage();
			continue;
		}
		break;
	} while(true);

	if(handleFileIOError(buffer.getFilePathName(), true, result)) {
		app_.getMRUManager().add(buffer.getFilePathName(), buffer.getCodePage());
		return OPENRESULT_SUCCEEDED;
	} else
		return (result != Document::FIR_CALLER_ABORTED) ? OPENRESULT_FAILED : OPENRESULT_USERCANCELED;
}

/// Reconstructs the image list and the menu according to the current buffer list.
void BufferList::resetResources() {
	if(icons_.isImageList()) {
		const int c = icons_.getNumberOfImages();
		for(int i = 0; i < c; ++i)
			::DestroyIcon(icons_.getIcon(i, ILD_NORMAL));
		icons_.destroy();
	}
	if(buffers_.empty())
		return;
	icons_.create(::GetSystemMetrics(SM_CXSMICON),
		::GetSystemMetrics(SM_CYSMICON), ILC_COLOR32 | ILC_MASK, 0, static_cast<int>(buffers_.size()));
	while(listMenu_.getNumberOfItems() != 0)
		listMenu_.erase<Menu::BY_POSITION>(0);

	::SHFILEINFOW sfi;
	for(size_t i = 0; i < buffers_.size(); ++i) {
		::SHGetFileInfoW(
			(buffers_[i]->getFilePathName() != 0) ? buffers_[i]->getFilePathName() : L"",
			0, &sfi, sizeof(::SHFILEINFOW), SHGFI_ICON | SHGFI_SMALLICON);
		icons_.add(sfi.hIcon);
		listMenu_.append(static_cast<UINT>(i) + CMD_VIEW_BUFFERLIST_START, 0U);
	}
	bufferBar_.setImageList(icons_.get());
	if(bufferBar_.isVisible())
		bufferBar_.invalidateRect(0);
}

/**
 * Saves (overwrites) the specified buffer.
 * @param index the index of the buffer to save
 * @param overwrite set false to save with another name (a file dialog will be shown)
 * @param addToMRU set true to add the file to MRU. this is effective only if the file was not exist or renamed
 * @retval true saved successfully or not needed to
 * @throw std#out_of_range @a index is invalid
 */
bool BufferList::save(size_t index, bool overwrite /* = true */, bool addToMRU /* = true */) {
	Buffer& buffer = getAt(index);

	// 保存の必要があるか?
	if(overwrite && buffer.isBoundToFile() && !buffer.isModified())
		return true;

	WCHAR fileName[MAX_PATH + 1];
	TextFileFormat format = {buffer.getCodePage(), LB_AUTO};
	bool newName = false;

	// 別名で保存 or ファイルが存在しない
	if(!overwrite || buffer.getFilePathName() == 0 || !toBoolean(::PathFileExistsW(buffer.getFilePathName()))) {
		AutoZero<::OSVERSIONINFOW> osVersion;
		const wstring filterSource = app_.loadString(MSG_DIALOG__SAVE_FILE_FILTER);
		wchar_t* filter = new wchar_t[filterSource.length() + 6];

		osVersion.dwOSVersionInfoSize = sizeof(::OSVERSIONINFOW);
		::GetVersionEx(&osVersion);
		filterSource.copy(filter, filterSource.length());
		wcsncpy(filter + filterSource.length(), L"\0*.*\0\0", 6);
		wcscpy(fileName, (buffer.getFilePathName() != 0) ? buffer.getFilePathName() : L"");

		AutoZeroLS<::OPENFILENAMEW> newOfn;
		AutoZeroLS<::OPENFILENAME_NT4W> oldOfn;
		::OPENFILENAMEW& ofn = (osVersion.dwMajorVersion > 4) ? newOfn : *reinterpret_cast<::OPENFILENAMEW*>(&oldOfn);
		ofn.hwndOwner = app_.getMainWindow().get();
		ofn.hInstance = ::GetModuleHandle(0);
		ofn.lpstrFilter = filter;
		ofn.lpstrFile = fileName;
		ofn.nMaxFile = MAX_PATH;
		ofn.Flags = OFN_ENABLEHOOK | OFN_ENABLESIZING | OFN_ENABLETEMPLATE
			| OFN_EXPLORER | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT /* | OFN_SHOWHELP*/;
		ofn.lCustData = reinterpret_cast<LPARAM>(&format);
		ofn.lpfnHook = openFileNameHookProc;
		ofn.lpTemplateName = MAKEINTRESOURCE(IDD_DLG_SAVEFILE);

		bool succeeded = toBoolean(::GetSaveFileNameW(&ofn));
//		DWORD n = ::CommDlgExtendedError();
		delete[] filter;
		if(!succeeded)
			return false;

		// 既に開かれているファイルに上書きしようとしていないか?
		const size_t existing = find(fileName);
		if(existing != -1 && existing != index) {
			app_.messageBox(MSG_BUFFER__SAVING_FILE_IS_OPENED, MB_ICONEXCLAMATION | MB_OK, MARGS % fileName);
			return false;
		}
		newName = true;
	} else
		wcscpy(fileName, buffer.getFilePathName());

	const bool writeBOM =
		(format.encoding == CP_UTF8 && toBoolean(app_.readIntegerProfile(L"File", L"writeBOMAsUTF8", 0)))
		|| (format.encoding == CPEX_UNICODE_UTF16LE && toBoolean(app_.readIntegerProfile(L"File", L"writeBOMAsUTF16LE", 1)))
		|| (format.encoding == CPEX_UNICODE_UTF16BE && toBoolean(app_.readIntegerProfile(L"File", L"writeBOMAsUTF16BE", 1)))
		|| (format.encoding == CPEX_UNICODE_UTF32LE && toBoolean(app_.readIntegerProfile(L"File", L"writeBOMAsUTF32LE", 1)))
		|| (format.encoding == CPEX_UNICODE_UTF32BE && toBoolean(app_.readIntegerProfile(L"File", L"writeBOMAsUTF32BE", 1)));
	Document::FileIOResult result;

	do {
		FileIOCallback callback(app_, false, fileName, format.encoding);
		Document::SaveParameters params;
		params.codePage = format.encoding;
		params.lineBreak = format.lineBreak;
		if(writeBOM)
			params.options = Document::SaveParameters::WRITE_UNICODE_BOM;

		result = buffer.save(fileName, params, &callback);
		if(callback.doesUserWantToChangeCodePage()) {
			ui::CodePagesDialog dlg(format.encoding, false);
			assert(result == Document::FIR_CALLER_ABORTED);
			if(dlg.doModal(app_.getMainWindow().get()) == IDOK) {
				format.encoding = dlg.getCodePage();
				continue;
			}
		}
		break;
	} while(true);

	const bool succeeded = handleFileIOError(fileName, false, result);
	if(succeeded && addToMRU && newName)
		app_.getMRUManager().add(fileName, format.encoding);
	return succeeded;
}

/**
 * Saves all buffers.
 * @param addToMRU set true to add the files to MRU.
 * @return true if all buffers were saved successfully
 */
bool BufferList::saveAll(bool addToMRU /* = true */) {
	for(size_t i = 0; i < buffers_.size(); ++i) {
		if(!save(i, true, addToMRU))
			return false;
	}
	return true;
}

/**
 * Activates the specified buffer in the active pane.
 * @param index the index of the buffer to activate
 * @throw std#out_of_range @a index is invalid
 */
void BufferList::setActive(size_t index) {
	editorWindow_.getActivePane().showBuffer(getAt(index));
	fireActiveBufferSwitched();
}

/**
 * Activates the specified buffer in the active pane.
 * @param buffer the buffer to activate
 * @throw std#invalid_argument @a buffer is not exist
 */
void BufferList::setActive(const Buffer& buffer) {
	editorWindow_.getActivePane().showBuffer(buffer);
	fireActiveBufferSwitched();
}

/// Reconstructs the context menu.
void BufferList::updateContextMenu() {
	while(contextMenu_.getNumberOfItems() > 0)
		contextMenu_.erase<Menu::BY_POSITION>(0);
	contextMenu_ << Menu::StringItem(CMD_FILE_CLOSE, app_.getCommandManager().getMenuName(CMD_FILE_CLOSE).c_str())
		<< Menu::StringItem(CMD_FILE_CLOSEOTHERS, app_.getCommandManager().getMenuName(CMD_FILE_CLOSEOTHERS).c_str());
	contextMenu_.setDefault<Menu::BY_COMMAND>(CMD_FILE_CLOSE);
}


// EditorPane ///////////////////////////////////////////////////////////////

/// Constructor.
EditorPane::EditorPane(EditorView* initialView /* = 0 */) : visibleView_(initialView), lastVisibleView_(0) {
	if(initialView != 0)
		addView(*initialView);
}

/// Copy-constructor.
EditorPane::EditorPane(const EditorPane& rhs) {
	for(set<EditorView*>::const_iterator it = rhs.views_.begin(); it != rhs.views_.end(); ++it) {
		EditorView* view = new EditorView(*(*it));
		view->detach();	// <-- 重要
		const bool succeeded = view->create((*it)->getParent()->get(), DefaultWindowRect(),
			WS_CHILD | WS_CLIPCHILDREN | WS_HSCROLL | WS_VISIBLE | WS_VSCROLL, WS_EX_CLIENTEDGE);
		assert(succeeded);
		views_.insert(view);
		if(*it == rhs.visibleView_)
			visibleView_ = view;
		if(*it == rhs.lastVisibleView_)
			lastVisibleView_ = view;
	}
}

/// Destructor.
EditorPane::~EditorPane() {
	removeAll();
}

/**
 * Adds the new viewer
 * @param view the viewer to add
 */
void EditorPane::addView(EditorView& view) {
	views_.insert(&view);
	if(views_.size() == 1)
		showBuffer(view.getDocument());
}

/// Removes all viewers.
void EditorPane::removeAll() {
	for(set<EditorView*>::iterator it = views_.begin(); it != views_.end(); ++it)
		delete *it;
	views_.clear();
	visibleView_ = lastVisibleView_ = 0;
}

/**
 * Removes the viewer belongs to the specified buffer.
 * @param buffer the buffer has the viewer to remove
 */
void EditorPane::removeBuffer(const Buffer& buffer) {
	for(set<EditorView*>::iterator it = views_.begin(); it != views_.end(); ++it) {
		if(&(*it)->getDocument() == &buffer) {
			EditorView& removing = **it;
			views_.erase(it);
			if(&removing == visibleView_) {
				visibleView_ = 0;
				if(&removing == lastVisibleView_)
					lastVisibleView_ = 0;
				if(views_.size() == 1 || lastVisibleView_ == 0)
					showBuffer((*views_.begin())->getDocument());
				else if(!views_.empty()) {
					showBuffer(lastVisibleView_->getDocument());
					lastVisibleView_ = 0;
				}
			}
			delete &removing;
			return;
		}
	}
}

/**
 * Shows the specified buffer in the pane.
 * @param buffer the buffer to show
 * @throw std#invalid_argument @a buffer is not exist
 */
void EditorPane::showBuffer(const Buffer& buffer) {
	if(visibleView_ != 0 && &visibleView_->getDocument() == &buffer)
		return;
	for(set<EditorView*>::iterator it = views_.begin(); it != views_.end(); ++it) {
		if(&(*it)->getDocument() == &buffer) {
			const bool hadFocus = visibleView_ == 0 || visibleView_->hasFocus();
			lastVisibleView_ = visibleView_;
			visibleView_ = *it;
			Alpha::getInstance().getBufferList().getEditorWindow().adjustPanes();
			visibleView_->show(SW_SHOW);
			if(lastVisibleView_ != 0)
				lastVisibleView_->show(SW_HIDE);
			if(hadFocus)
				visibleView_->setFocus();
			return;
		}
	}
	throw invalid_argument("Specified buffer is not contained in the pane.");
}


// EditorView ///////////////////////////////////////////////////////////////

MANAH_BEGIN_WINDOW_MESSAGE_MAP(EditorView, TextViewer)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_KEYDOWN)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_KILLFOCUS)
	MANAH_WINDOW_MESSAGE_ENTRY(WM_SETFOCUS)
MANAH_END_WINDOW_MESSAGE_MAP()

Handle<HICON, ::DestroyIcon> EditorView::narrowingIcon_;

/// Constructor.
EditorView::EditorView(Presentation& presentation) : SourceViewer(presentation), visualColumnStartValue_(1) {
}

/// Copy-constructor.
EditorView::EditorView(const EditorView& rhs) : SourceViewer(rhs), visualColumnStartValue_(rhs.visualColumnStartValue_) {
}

/// Destructor.
EditorView::~EditorView() {
}

/// Begins incremental search.
void EditorView::beginIncrementalSearch(SearchType type, Direction direction) {
	texteditor::commands::IncrementalSearchCommand(*this, type, direction, this).execute();
}

/// @see ICaretListener#caretMoved
void EditorView::caretMoved(const Caret& self, const Region& oldRegion) {
	SourceViewer::caretMoved(self, oldRegion);
	updateCurrentPositionOnStatusBar();
}

/// @see IDocumentStateListener#documentAccessibleRegionChanged
void EditorView::documentAccessibleRegionChanged(Document& document) {
	SourceViewer::documentAccessibleRegionChanged(document);
	updateNarrowingOnStatusBar();
}

/// @see IDocumentStateListener#documentEncodingChanged
void EditorView::documentEncodingChanged(Document& document) {
	SourceViewer::documentEncodingChanged(document);
}

/// @see IDocumentStateListener#documentFileNameChanged
void EditorView::documentFileNameChanged(Document& document) {
	SourceViewer::documentFileNameChanged(document);
	updateTitleBar();
}

/// @see IDocumentStateListener#documentModificationSignChanged
void EditorView::documentModificationSignChanged(Document& document) {
	SourceViewer::documentModificationSignChanged(document);
	updateTitleBar();
}

/// @see IDocumentStateListener#documentReadOnlySignChanged
void EditorView::documentReadOnlySignChanged(Document& document) {
	SourceViewer::documentReadOnlySignChanged(document);
	updateTitleBar();
}

/// @see IIncrementalSearchListener#incrementalSearchAborted
void EditorView::incrementalSearchAborted(const Position& initialPosition) {
	incrementalSearchCompleted();
	getCaret().moveTo(initialPosition);
}

/// @see IIncrementalSearchListener#incrementalSearchCompleted
void EditorView::incrementalSearchCompleted() {
	Alpha::getInstance().setStatusText(0);
}

/// @see IIncrementalSearchListener#incrementalSearchPatternChanged
void EditorView::incrementalSearchPatternChanged(Result result) {
	UINT messageID;
	Alpha& app = Alpha::getInstance();
	const IncrementalSearcher& isearch = Alpha::getInstance().getBufferList().getEditorSession().getIncrementalSearcher();
	const bool forward = isearch.getDirection() == FORWARD;

	if(result == IIncrementalSearchListener::EMPTY_PATTERN) {
		getCaret().select(isearch.getMatchedRegion());
		messageID = forward ? MSG_STATUS__ISEARCH_EMPTY_PATTERN : MSG_STATUS__RISEARCH_EMPTY_PATTERN;
		app.setStatusText(app.loadString(messageID).c_str(),
			toBoolean(app.readIntegerProfile(L"View", L"applyMainFontToSomeControls", 1)) ? getTextRenderer().getFont() : 0);
		return;
	} else if(result == IIncrementalSearchListener::FOUND) {
		getCaret().select(isearch.getMatchedRegion());
		messageID = forward ? MSG_STATUS__ISEARCH : MSG_STATUS__RISEARCH;
	} else {
		if(result == IIncrementalSearchListener::NOT_FOUND)
			messageID = forward ? MSG_STATUS__ISEARCH_NOT_FOUND : MSG_STATUS__RISEARCH_NOT_FOUND;
		else
			messageID = forward ? MSG_STATUS__ISEARCH_BAD_PATTERN : MSG_STATUS__RISEARCH_BAD_PATTERN;
		beep();
	}

	String prompt = app.loadString(messageID, MARGS % isearch.getPattern());
	replace_if(prompt.begin(), prompt.end(), bind2nd(equal_to<wchar_t>(), L'\t'), L' ');
	app.setStatusText(prompt.c_str(),
			toBoolean(app.readIntegerProfile(L"View", L"applyMainFontToSomeControls", 1)) ? getTextRenderer().getFont() : 0);
}

/// @see IIncrementalSearchListener#incrementalSearchStarted
void EditorView::incrementalSearchStarted(const Document&) {
}

/// @see ICaretListener#matchBracketsChanged
void EditorView::matchBracketsChanged(const Caret& self, const pair<Position, Position>& oldPair, bool outsideOfView) {
	// TODO: indicate if the pair is outside of the viewer.
}

/// @see Window#onKeyDown
void EditorView::onKeyDown(UINT vkey, UINT flags, bool& handled) {
	// 既定のキー割り当てを全て無効にする
//	handled = true;
	return SourceViewer::onKeyDown(vkey, flags, handled);
}

/// @see Window#onKillFocus
void EditorView::onKillFocus(HWND newWindow) {
	SourceViewer::onKillFocus(newWindow);
	Alpha::getInstance().getBufferList().getEditorSession().getIncrementalSearcher().end();
}

/// @see Window#onSetFocus
void EditorView::onSetFocus(HWND oldWindow) {
	SourceViewer::onSetFocus(oldWindow);
	updateTitleBar();
	updateCurrentPositionOnStatusBar();
	updateNarrowingOnStatusBar();
	updateOvertypeModeOnStatusBar();
}

/// @see ICaretListener#overtypeModeChanged
void EditorView::overtypeModeChanged(const Caret&) {
	updateOvertypeModeOnStatusBar();
}

/// @see ICaretListener#selectionShapeChanged
void EditorView::selectionShapeChanged(const Caret&) {
}

void EditorView::updateCurrentPositionOnStatusBar() {
	if(hasFocus()) {
		// build the current position indication string
		static wchar_t format[256] = {0};
		if(format[0] == 0)
			Alpha::getInstance().loadString(MSG_STATUS__CARET_POSITION, format, countof(format));
		wchar_t text[countof(format)];
		AutoZero<::SCROLLINFO> si;
		getScrollInformation(SB_VERT, si, SIF_POS | SIF_RANGE);
		swprintf(text, format,
			getCaret().getLineNumber() + getVerticalRulerConfiguration().lineNumbers.startValue,
			getCaret().getVisualColumnNumber() + visualColumnStartValue_,
			getCaret().getColumnNumber() + visualColumnStartValue_);
		// show in the status bar
		Alpha::getInstance().getStatusBar().setText(1, text);
	}
}

void EditorView::updateNarrowingOnStatusBar() {
	if(hasFocus()) {
		const bool narrow = getDocument().isNarrowed();
		Alpha& app = Alpha::getInstance();
		if(narrowingIcon_.get() == 0)
			narrowingIcon_.reset(static_cast<HICON>(app.loadImage(IDR_ICON_NARROWING, IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR)));
		StatusBar& statusBar = app.getStatusBar();
		statusBar.setText(4, narrow ? app.loadString(MSG_STATUS__NARROWING).c_str() : L"");
		statusBar.setTipText(4, narrow ? app.loadString(MSG_STATUS__NARROWING).c_str() : L"");
		statusBar.setIcon(4, narrow ? narrowingIcon_.get() : 0);
	}
}

void EditorView::updateOvertypeModeOnStatusBar() {
	if(hasFocus())
		Alpha::getInstance().getStatusBar().setText(3,
			Alpha::getInstance().loadString(getCaret().isOvertypeMode() ? MSG_STATUS__OVERTYPE_MODE : MSG_STATUS__INSERT_MODE).c_str());
}

/// Updates the title bar text according to the current state.
void EditorView::updateTitleBar() {
	static wstring titleCache;
	Window& mainWindow = Alpha::getInstance().getMainWindow();
	if(!mainWindow.isWindow())
		return;
	wstring title = BufferList::getDisplayName(getDocument());
	if(title != titleCache) {
		titleCache = title;
//		title += L" - " IDS_APPFULLVERSION;
		title += L" - " IDS_APPNAME;
		mainWindow.setText(title.c_str());
	}
}
