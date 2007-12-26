/**
 * @file buffer.cpp
 * @author exeal
 * @date 2003-2006 (was AlphaDoc.cpp and BufferList.cpp)
 * @date 2006-2007
 */

#include "stdafx.h"
#include "ascension/text-editor.hpp"	// ascension.texteditor.commands.IncrementalSearchCommand
#include "buffer.hpp"
#include "application.hpp"
#include "command.hpp"
#include "mru-manager.hpp"
#include "new-file-format-dialog.hpp"
#include "save-some-buffers-dialog.hpp"
#include "code-pages-dialog.hpp"
#include "resource/messages.h"
#include "../manah/win32/ui/wait-cursor.hpp"
#include "../manah/com/common.hpp"	// ComPtr, ComQIPtr
#include <commdlg.h>
#include <shlwapi.h>				// PathXxxx, StrXxxx, ...
#include <shlobj.h>					// IShellLink, ...
#include <dlgs.h>
using namespace alpha;
using namespace ascension;
using namespace ascension::kernel;
using namespace ascension::kernel::fileio;
using namespace ascension::searcher;
using namespace ascension::presentation;
using namespace ascension::encoding;
using namespace ascension::viewers;
using namespace manah::win32;
using namespace manah::win32::ui;
using namespace std;

#pragma comment(lib, "msimg32.lib")

namespace {
	struct TextFileFormat {
		std::string encoding;
		Newline newline;
	};
/*	class FileIOCallback : virtual public IFileIOListener {
	public:
		FileIOCallback(alpha::Alpha& app, bool forLoading, const WCHAR* fileName, CodePage encoding) throw()
				: app_(app), forLoading_(forLoading), fileName_(fileName),
				encoding_(encoding), retryWithOtherCodePage_(false) {}
		bool doesUserWantToChangeCodePage() const throw() {
			return retryWithOtherCodePage_;
		}
		bool unconvertableCharacterFound() {
			const DWORD id = (encoding_ < 0x10000) ?
				(encoding_ + MSGID_ENCODING_START) : (encoding_ - 60000 + MSGID_EXTENDED_ENCODING_START);
			const int answer = app_.messageBox(forLoading_ ?
				MSG_IO__UNCONVERTABLE_NATIVE_CHAR : MSG_IO__UNCONVERTABLE_UCS_CHAR, MB_YESNOCANCEL | MB_ICONEXCLAMATION,
				MARGS % fileName_ % Alpha::getInstance().loadMessage(id).c_str());
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
	};*/
} // namespace @0


// Buffer ///////////////////////////////////////////////////////////////////

/// Constructor.
Buffer::Buffer() {
	presentation_.reset(new Presentation(*this));
	textFile_.reset(new fileio::TextFileDocumentInput(*this));
}

/// Destructor.
Buffer::~Buffer() {
}

/// Returns the name of the buffer.
const basic_string<::WCHAR> Buffer::name() const throw() {
	static const wstring untitled(Alpha::instance().loadMessage(MSG_BUFFER__UNTITLED));
	if(textFile_->isOpen())
		return textFile_->name();
	return untitled.c_str();
}

/// Returns the presentation object of Ascension.
Presentation& Buffer::presentation() throw() {
	return *presentation_;
}

/// Returns the presentation object of Ascension.
const Presentation& Buffer::presentation() const throw() {
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
	editorWindow_.create(app_.getMainWindow().getHandle(),
		DefaultWindowRect(), WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE, 0, *(new EditorPane));
	assert(editorWindow_.isWindow());

	// 右クリックメニューの作成
	updateContextMenu();

	if(READ_ONLY_SIGNATURE_.empty())
		const_cast<wstring&>(READ_ONLY_SIGNATURE_).assign(app_.loadMessage(MSG_STATUS__READ_ONLY_CAPTION));
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
 * @param encoding the encoding
 * @param newline the newline
 */
void BufferList::addNew(const string& encoding /* = "UTF-8" */, Newline newline /* = NLF_AUTO */) {
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
	if(Encoder::forName(encoding) == 0)
		throw invalid_argument("unsupported encoding.");

	Buffer* const buffer = new Buffer();
	editorSession_.addDocument(*buffer);

	buffer->textFile().setEncoding(encoding);
	if((newline & NLF_SPECIAL_VALUE_MASK) == 0)
		buffer->textFile().setNewline(newline);
	buffers_.push_back(buffer);

	// 現在のペインの数だけビューを作成する
	::LOGFONTW font;
	app_.textEditorFont(font);
	EditorView* originalView = 0;
	for(EditorWindow::Iterator it = editorWindow_.enumeratePanes(); !it.isEnd(); it.next()) {
		EditorView* view = (originalView == 0) ? new EditorView(buffer->presentation()) : new EditorView(*originalView);
		view->create(editorWindow_.getHandle(), DefaultWindowRect(),
			WS_CHILD | WS_CLIPCHILDREN | WS_HSCROLL | WS_VISIBLE | WS_VSCROLL, WS_EX_CLIENTEDGE);
		assert(view->isWindow());
		if(originalView == 0)
			originalView = view;
		it.get().addView(*view);
		view->textRenderer().setFont(font.lfFaceName, font.lfHeight, 0);
		if(originalView != view)
			view->setConfiguration(&originalView->configuration(), 0);
	}

//	view.addEventListener(app_);
	buffer->presentation().addTextViewerListListener(*this);
	buffer->addStateListener(*this);
	buffer->textFile().addListener(*this);
//	view.getLayoutSetter().setFont(lf);

	// バッファバーにボタンを追加
	const basic_string<::WCHAR> name(buffer->name());
	MANAH_AUTO_STRUCT(::TBBUTTON, button);
	button.idCommand = CMD_SPECIAL_BUFFERSSTART + bufferBar_.getButtonCount() - CMD_SPECIAL_START;
	button.iBitmap = static_cast<int>(buffers_.size() - 1);
	button.fsState = TBSTATE_ENABLED;
	button.fsStyle = BTNS_AUTOSIZE | BTNS_BUTTON | BTNS_GROUP | BTNS_NOPREFIX;
	button.iString = reinterpret_cast<::INT_PTR>(name.c_str());
	bufferBar_.insertButton(bufferBar_.getButtonCount(), button);

	resetResources();
	setActive(*buffer);
//	app_.applyDocumentType(*buffer, docType);
}

/// [書式を指定して新規] ダイアログを開き、バッファを新規作成する
void BufferList::addNewDialog() {
	TextFileFormat format;
	format.encoding = Encoder::forMIB(fundamental::US_ASCII)->fromUnicode(app_.readStringProfile(L"File", L"defaultEncoding"));
	if(Encoder::forName(format.encoding) == 0)
		format.encoding = Encoder::getDefault().name();
	format.newline = static_cast<Newline>(app_.readIntegerProfile(L"file", L"defaultNewline", NLF_CR_LF));

	ui::NewFileFormatDialog dlg(format.encoding, format.newline);
	if(dlg.doModal(app_.getMainWindow()) != IDOK)
		return;
	addNew(dlg.encoding(), dlg.newline());
}

/**
 * Closes the specified buffer.
 * @param index the index of the buffer to close
 * @param queryUser set true to query user if the buffer is modified
 * @return true if the buffer was closed successfully
 * @throw std#out_of_range @a index is invalid
 */
bool BufferList::close(size_t index, bool queryUser) {
	Buffer& buffer = at(index);

	if(queryUser && buffer.isModified()) {
		setActive(buffer);	// 対象のバッファをユーザに見えるようにする
		const int answer = app_.messageBox(MSG_BUFFER__BUFFER_IS_DIRTY, MB_YESNOCANCEL | MB_ICONEXCLAMATION, MARGS % buffer.name());
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
		buffer.textFile().removeListener(*this);
		buffer.textFile().close();
		delete &buffer;

		for(size_t i = index ; i < buffers_.size(); ++i)
			bufferBar_.setButtonText(static_cast<int>(CMD_SPECIAL_BUFFERSSTART - CMD_SPECIAL_START + i), getDisplayName(*buffers_[i]).c_str());
		resetResources();
		recalculateBufferBarSize();
		fireActiveBufferSwitched();
	} else {	// 最後の 1 つの場合
		buffer.textFile().close();
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
	const size_t active = activeIndex();

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

	// 未保存のバッファが 1 つなら通常の確認ダイアログを出して終わり
	if(buffers_.size() - (exceptActive ? 1 : 0) == 1) {
		const size_t dirty = !exceptActive ? 0 : ((active == 0) ? 1 : 0);

		if(buffers_[dirty]->isModified()) {
			if(exceptActive)
				setActive(dirty);
			return close(activeIndex(), queryUser);
		}
	}

	// 複数のバッファを保存するかどうか確認ダイアログを出す
	ui::SaveSomeBuffersDialog dlg;
	for(size_t i = 0; i < buffers_.size(); ++i) {
		if(exceptActive && i == active)
			continue;
		ui::DirtyFile df;
		df.index = static_cast<uint>(i);
		df.fileName = buffers_[i]->name();
		df.save = true;
		dlg.files_.push_back(df);
	}
	if(IDOK != dlg.doModal(app_.getMainWindow()))
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
	if(!bufferBarPager_.create(rebar.getHandle(), DefaultWindowRect(), 0, IDC_BUFFERBARPAGER,
			WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE | CCS_NORESIZE | PGS_HORZ))
		return false;
	if(!bufferBar_.create(bufferBarPager_.getHandle(), DefaultWindowRect(), 0, IDC_BUFFERBAR,
			WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE
			| CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE | CCS_TOP
			| TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_REGISTERDROP | TBSTYLE_TOOLTIPS | TBSTYLE_TRANSPARENT, WS_EX_TOOLWINDOW)) {
		bufferBarPager_.destroy();
		return false;
	}
	::HWND toolTips = bufferBar_.getToolTips();
	bufferBar_.setButtonStructSize();
	::SetWindowLongPtrW(toolTips, GWL_STYLE, ::GetWindowLongPtrW(toolTips, GWL_STYLE) | TTS_NOPREFIX);
	bufferBarPager_.setChild(bufferBar_.getHandle());

	// レバーに乗せる
	MANAH_AUTO_STRUCT_SIZE(::REBARBANDINFOW, rbbi);
	const wstring caption = app_.loadMessage(MSG_DIALOG__BUFFERBAR_CAPTION);
	rbbi.fMask = RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_ID | RBBIM_STYLE | RBBIM_TEXT;
	rbbi.fStyle = RBBS_BREAK | RBBS_GRIPPERALWAYS;
	rbbi.cxMinChild = 0;
	rbbi.cyMinChild = 22;
	rbbi.wID = IDC_BUFFERBAR;
	rbbi.lpText = const_cast<wchar_t*>(caption.c_str());
	rbbi.hwndChild = bufferBarPager_.getHandle();
	if(!rebar.insertBand(rebar.getBandCount(), rbbi)) {
		bufferBar_.destroy();
		bufferBarPager_.destroy();
		return false;
	}
	return true;
}

/// @see ascension#text#IDocumentStateListener#documentAccessibleRegionChanged
void BufferList::documentAccessibleRegionChanged(const Document&) {
	// do nothing
}

/// @see ascension#text#IDocumentStateListener#documentModificationSignChanged
void BufferList::documentModificationSignChanged(const Document& document) {
	const Buffer& buffer = getConcreteDocument(document);
	bufferBar_.setButtonText(static_cast<int>(CMD_SPECIAL_BUFFERSSTART + find(buffer) - CMD_SPECIAL_START), getDisplayName(buffer).c_str());
	recalculateBufferBarSize();
}

/// @see ascension#text#IDocumentStateListenerdocumentPropertyChanged
void BufferList::documentPropertyChanged(const Document&, const DocumentPropertyKey&) {
	// do nothing
}

/// @see ascension#text#IDocumentStateListenerdocumentReadOnlySignChanged
void BufferList::documentReadOnlySignChanged(const Document& document) {
	const Buffer& buffer = getConcreteDocument(document);
	bufferBar_.setButtonText(static_cast<int>(CMD_SPECIAL_BUFFERSSTART + find(buffer) - CMD_SPECIAL_START), getDisplayName(buffer).c_str());
	recalculateBufferBarSize();
}

/// @see ascension#kernel#fileio#IFilePropertyListener#fileNameChanged
void BufferList::fileNameChanged(const TextFileDocumentInput& textFile) {
	const Buffer& buffer = getConcreteDocument(textFile.document());
	// TODO: call mode-application.
	resetResources();
	bufferBar_.setButtonText(static_cast<int>(CMD_SPECIAL_BUFFERSSTART + find(buffer) - CMD_SPECIAL_START), getDisplayName(buffer).c_str());
	bufferBarPager_.recalcSize();
}

/// @see ascension#kernel#fileio#IFilePropertyListener#fileEncodingChanged
void BufferList::fileEncodingChanged(const TextFileDocumentInput& textFile) {
	// do nothing
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
size_t BufferList::find(const basic_string<::WCHAR>& fileName) const {
	for(size_t i = 0; i < buffers_.size(); ++i) {
		if(buffers_[i]->textFile().isOpen() && comparePathNames(buffers_[i]->textFile().pathName().c_str(), fileName.c_str()))
			return i;
	}
	return -1;
}

/// IActiveBufferListener::activeBufferSwitched のトリガ
void BufferList::fireActiveBufferSwitched() {
	const size_t buffer = activeIndex();
	const EditorView& view = activeView();

	bufferBar_.checkButton(static_cast<int>(buffer) + CMD_SPECIAL_BUFFERSSTART - CMD_SPECIAL_START);
	for(EditorWindow::Iterator i(editorWindow_.enumeratePanes()); !i.isEnd(); i.next()) {
		if(i.get().numberOfViews() > 0 && &i.get().visibleView() == &view) {
			editorWindow_.setDefaultActivePane(i.get());
			break;
		}
	}

	// アクティブなバッファのボタンが隠れていたらスクロールする
	if(bufferBarPager_.isVisible()) {
		const int pagerPos = bufferBarPager_.getPosition();
		::RECT buttonRect, pagerRect;
		bufferBar_.getItemRect(static_cast<int>(buffer), buttonRect);
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

/// Const-version of @c #getConcreteDocument(Document&).
const Buffer& BufferList::getConcreteDocument(const Document& document) const {
	return getConcreteDocument(const_cast<Document&>(document));
}

/**
 * タイトルバーやバッファバーに表示するバッファの名前を返す
 * @param buffer the buffer
 * @return the name
 */
wstring BufferList::getDisplayName(const Buffer& buffer) {
	const std::wstring str(buffer.name());
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
			setActive(mouse.dwItemSpec - (CMD_SPECIAL_BUFFERSSTART - CMD_SPECIAL_START));
			contextMenu_.trackPopup(TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON, pt.x, pt.y, Alpha::instance().getMainWindow().getHandle());
			return true;
		}
	}

	else if(nmhdr.hdr.code == TTN_GETDISPINFOW) {	// ツールチップ
		assert(nmhdr.hdr.idFrom >= CMD_SPECIAL_BUFFERSSTART - CMD_SPECIAL_START && nmhdr.hdr.idFrom < CMD_SPECIAL_BUFFERSEND - CMD_SPECIAL_START);
		static wchar_t tipText[500];
		::NMTTDISPINFOW& nmttdi = *reinterpret_cast<::NMTTDISPINFOW*>(&nmhdr.hdr);

//		nmttdi->hinst = getHandle();
		const Buffer& buffer = at(nmttdi.hdr.idFrom - (CMD_SPECIAL_BUFFERSSTART - CMD_SPECIAL_START));
		wcscpy(tipText, (buffer.textFile().isOpen() ? buffer.textFile().location() : buffer.name()).c_str());
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

	else if(nmhdr.hdr.code == TBN_HOTITEMCHANGE && bufferBar_.getButtonCount() > 1 && bufferBar_.getHandle() == ::GetCapture()) {
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
bool BufferList::handleFileIOError(const ::WCHAR* fileName, bool forLoading, IOException::Type result) {
	assert(fileName != 0);
	if(result == IOException::FILE_NOT_FOUND) {
		::SetLastError(ERROR_FILE_NOT_FOUND);
		result = IOException::PLATFORM_DEPENDENT_ERROR;
	}
	if(result == IOException::PLATFORM_DEPENDENT_ERROR) {
		void* buffer = 0;
		wstring message(fileName);
		::FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			0, ::GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<wchar_t*>(&buffer), 0, 0);
		message += L"\n\n";
		message += static_cast<wchar_t*>(buffer);
		::LocalFree(buffer);
		app_.getMainWindow().messageBox(message.c_str(), IDS_APPNAME, MB_ICONEXCLAMATION);
	} else if(result != IOException::UNMAPPABLE_CHARACTER && result != IOException::MALFORMED_INPUT) {
		::DWORD messageID;
		switch(result) {
		case IOException::INVALID_ENCODING:				messageID = MSG_IO__INVALID_ENCODING; break;
		case IOException::INVALID_NEWLINE:				messageID = MSG_IO__INVALID_NEWLINE; break;
		case IOException::OUT_OF_MEMORY:				messageID = MSG_ERROR__OUT_OF_MEMORY; break;
		case IOException::HUGE_FILE:					messageID = MSG_IO__HUGE_FILE; break;
		case IOException::READ_ONLY_MODE:
		case IOException::UNWRITABLE_FILE:				messageID = MSG_IO__FAILED_TO_WRITE_FOR_READONLY; break;
		case IOException::CANNOT_CREATE_TEMPORARY_FILE:	messageID = MSG_IO__CANNOT_CREATE_TEMP_FILE; break;
		case IOException::LOST_DISK_FILE:				messageID = MSG_IO__LOST_DISK_FILE; break;
		}
		app_.messageBox(messageID, MB_ICONEXCLAMATION, MARGS % fileName);
	}
	if(forLoading)
		close(activeIndex(), false);
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
		bufferBar_.setButtonText(CMD_SPECIAL_BUFFERSSTART + i - CMD_SPECIAL_START, getDisplayName(*buffers_[i]).c_str());
	setActive(*buffer);
	resetResources();
}

/**
 * Opens the specified file.
 * This method may show a dialog to indicate the result.
 * @param fileName the name of the file to open
 * @param encoding the encoding. auto-detect if omitted
 * @param asReadOnly set true to open as read only
 * @param addToMRU set true to add the file to MRU
 * @return the result. see the description of @c BufferList#OpenResult
 */
BufferList::OpenResult BufferList::open(const basic_string<WCHAR>& fileName,
		const string& encoding /* = "UTF-8" */, bool asReadOnly /* = false */, bool addToMRU /* = true */) {
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
	} else
		wcscpy(resolvedName, canonicalizePathName(fileName.c_str()).c_str());

	// (テキストエディタで) 既に開かれているか調べる
	const size_t oldBuffer = find(resolvedName);
	if(oldBuffer != -1) {
		setActive(oldBuffer);
		return OPENRESULT_SUCCEEDED;
	}

	Buffer* buffer = &active();
	TextFileDocumentInput::LockMode lockMode;
	lockMode.onlyAsEditing = false;
	switch(app_.readIntegerProfile(L"File", L"shareMode", 0)) {
	case 0:	lockMode.type = TextFileDocumentInput::LockMode::DONT_LOCK; break;
	case 1:	lockMode.type = TextFileDocumentInput::LockMode::SHARED_LOCK; break;
	case 2:	lockMode.type = TextFileDocumentInput::LockMode::EXCLUSIVE_LOCK; break;
	}

	if(buffer->isModified() || buffer->textFile().isOpen()) {	// 新しいコンテナで開く
		addNew(encoding);
		buffer = &active();
	}
/*
	if(Encoder::supports(encoding)) {
//		try {
			buffer->setEncoding(encoding);
//		} catch(invalid_argument&) {
//			if(IDNO == app_.messageBox(MSG_ILLEGAL_CODEPAGE, MB_YESNO | MB_ICONEXCLAMATION))
//				return OPENRESULT_USERCANCELED;
//			encoding = ::GetACP();
//			buffer->setCodePage(encoding);
//		}
	}
*/
	string modifiedEncoding(encoding);
	bool succeeded = true;
	IOException::Type errorType;
	const wstring s(app_.loadMessage(MSG_STATUS__LOADING_FILE, MARGS % resolvedName));
	while(true) {
		WaitCursor wc;
		app_.setStatusText(s.c_str());
		app_.getMainWindow().lockUpdate();

		// 準備ができたのでファイルを開く
		try {
			// TODO: 戻り値を調べる必要がある。
			buffer->textFile().open(resolvedName, lockMode, modifiedEncoding, Encoder::NO_POLICY);
		} catch(IOException& e) {
			succeeded = false;
			errorType = e.type();
		}
		app_.setStatusText(0);
		app_.getMainWindow().unlockUpdate();
		if(!succeeded) {
			// alert the encoding error
			int userAnswer;
			if(errorType == IOException::UNMAPPABLE_CHARACTER)
				userAnswer = Alpha::instance().messageBox(
					MSG_IO__UNCONVERTABLE_NATIVE_CHAR, MB_YESNOCANCEL | MB_ICONEXCLAMATION,
					MARGS % resolvedName % Encoder::forMIB(fundamental::US_ASCII)->toUnicode(modifiedEncoding).c_str());
			else if(errorType == IOException::MALFORMED_INPUT)
				userAnswer = Alpha::instance().messageBox(
					MSG_IO__MALFORMED_INPUT_FILE, MB_OKCANCEL | MB_ICONEXCLAMATION,
					MARGS % resolvedName % Encoder::forMIB(fundamental::US_ASCII)->toUnicode(modifiedEncoding).c_str());
			else
				break;
			succeeded = true;
			if(userAnswer == IDYES || userAnswer == IDOK) {
				// the user want to change the encoding
				ui::EncodingsDialog dlg(modifiedEncoding, true);
				if(dlg.doModal(app_.getMainWindow()) != IDOK)
					return OPENRESULT_USERCANCELED;
				modifiedEncoding = dlg.resultEncoding();
				continue;
			} else if(userAnswer == IDNO) {
				succeeded = true;
				try {
					buffer->textFile().open(resolvedName, lockMode, modifiedEncoding, Encoder::REPLACE_UNMAPPABLE_CHARACTER);
				} catch(IOException& e) {
					succeeded = false;
					if((errorType = e.type()) == IOException::MALFORMED_INPUT) {
						Alpha::instance().messageBox(
							MSG_IO__MALFORMED_INPUT_FILE, MB_OK | MB_ICONEXCLAMATION,
							MARGS % resolvedName % Encoder::forMIB(fundamental::US_ASCII)->toUnicode(modifiedEncoding).c_str());
						return OPENRESULT_FAILED;
					}
				}
			} else
				return OPENRESULT_USERCANCELED;
		}
		break;
	}

	app_.getMainWindow().show(app_.getMainWindow().isVisible() ? SW_SHOW : SW_RESTORE);

	if(succeeded || handleFileIOError(resolvedName, true, errorType)) {
		if(asReadOnly)
			buffer->setReadOnly();
		if(addToMRU)
			app_.mruManager().add(buffer->textFile().pathName());
		return OPENRESULT_SUCCEEDED;
	}
	return OPENRESULT_FAILED;
}

/**
 * Shows "Open" dialog box and opens file(s).
 * @param initialDirectory the directory name to show in the dialog first. if @c null, the directory
 * of the active buffer. if the active buffer is not bound to a file, the system default
 * @return the result. see the description of @c BufferList::OpenResult
 */
BufferList::OpenResult BufferList::openDialog(const WCHAR* initialDirectory /* = 0 */) {
	wstring filterSource = app_.readStringProfile(L"File", L"filter", app_.loadMessage(MSG_DIALOG__DEFAULT_OPENFILE_FILTER).c_str());
	wchar_t* filter = new wchar_t[filterSource.length() + 2];
	WCHAR fileName[MAX_PATH + 1] = L"";
	wstring errorMessage;

	// フィルタを整形
	replace_if(filterSource.begin(), filterSource.end(), bind2nd(equal_to<wchar_t>(), L':'), L'\0');
	filterSource.copy(filter, filterSource.length());
	filter[filterSource.length()] = L'\0';
	filter[filterSource.length() + 1] = L'\0';

	MANAH_AUTO_STRUCT(::OSVERSIONINFOW, osVersion);
	osVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
	::GetVersionEx(&osVersion);

	WCHAR* activeBufferDir = 0;
	if(initialDirectory == 0 && active().textFile().isOpen()) {
		activeBufferDir = new WCHAR[MAX_PATH];
		wcscpy(activeBufferDir, active().textFile().pathName().c_str());
		*::PathFindFileNameW(activeBufferDir) = 0;
		if(activeBufferDir[0] == 0) {
			delete[] activeBufferDir;
			activeBufferDir = 0;
		}
	}

	TextFileFormat format = {Encoder::getDefault().name(), NLF_RAW_VALUE};
	MANAH_AUTO_STRUCT_SIZE(::OPENFILENAMEW, newOfn);
	MANAH_AUTO_STRUCT_SIZE(::OPENFILENAME_NT4W, oldOfn);
	::OPENFILENAMEW& ofn = (osVersion.dwMajorVersion > 4) ? newOfn : *reinterpret_cast<::OPENFILENAMEW*>(&oldOfn);
	ofn.hwndOwner = app_.getMainWindow().getHandle();
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

		if(directory.length() > ofn.nFileOffset)	// 開くファイルは 1 つ
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
		// changed "Encoding"
		if(LOWORD(wParam) == IDC_COMBO_ENCODING && HIWORD(wParam) == CBN_SELCHANGE) {
			ComboBox newlineCombobox(::GetDlgItem(window, IDC_COMBO_NEWLINE));
			if(!newlineCombobox.isWindow())
				break;
			ComboBox encodingCombobox(::GetDlgItem(window, IDC_COMBO_ENCODING));

			const wstring keepNLF = Alpha::instance().loadMessage(MSG_DIALOG__KEEP_NEWLINE);
			const MIBenum encoding = static_cast<MIBenum>(encodingCombobox.getItemData(encodingCombobox.getCurSel()));
			const int newline = (newlineCombobox.getCount() != 0) ? newlineCombobox.getCurSel() : 0;

			if(encoding == extended::UTF_5 || encoding == extended::UTF_7
					|| encoding == fundamental::UTF_8
					|| encoding == fundamental::UTF_16LE || encoding == fundamental::UTF_16BE
					|| encoding == extended::UTF_32LE || encoding == extended::UTF_32BE) {
				if(newlineCombobox.getCount() != 7) {
					newlineCombobox.resetContent();
					newlineCombobox.setItemData(newlineCombobox.addString(keepNLF.c_str()), NLF_RAW_VALUE);
					newlineCombobox.setItemData(newlineCombobox.addString(IDS_BREAK_CRLF), NLF_CR_LF);
					newlineCombobox.setItemData(newlineCombobox.addString(IDS_BREAK_LF), NLF_LINE_FEED);
					newlineCombobox.setItemData(newlineCombobox.addString(IDS_BREAK_CR), NLF_CARRIAGE_RETURN);
					newlineCombobox.setItemData(newlineCombobox.addString(IDS_BREAK_NEL), NLF_NEXT_LINE);
					newlineCombobox.setItemData(newlineCombobox.addString(IDS_BREAK_LS), NLF_LINE_SEPARATOR);
					newlineCombobox.setItemData(newlineCombobox.addString(IDS_BREAK_PS), NLF_PARAGRAPH_SEPARATOR);
					newlineCombobox.setCurSel(newline);
				}
			} else {
				if(newlineCombobox.getCount() != 4) {
					newlineCombobox.resetContent();
					newlineCombobox.setItemData(newlineCombobox.addString(keepNLF.c_str()), NLF_RAW_VALUE);
					newlineCombobox.setItemData(newlineCombobox.addString(IDS_BREAK_CRLF), NLF_CR_LF);
					newlineCombobox.setItemData(newlineCombobox.addString(IDS_BREAK_LF), NLF_LINE_FEED);
					newlineCombobox.setItemData(newlineCombobox.addString(IDS_BREAK_CR), NLF_CARRIAGE_RETURN);
					newlineCombobox.setCurSel((newline < 4) ? newline : 0);
				}
			}
		}
		break;
	case WM_INITDIALOG: {
		::OPENFILENAMEW& ofn = *reinterpret_cast<::OPENFILENAMEW*>(lParam);
		HWND dialog = ::GetParent(window);
		ComboBox encodingCombobox(::GetDlgItem(window, IDC_COMBO_ENCODING));
		Static encodingLabel(::GetDlgItem(window, IDC_STATIC_1));
		ComboBox newlineCombobox(::GetDlgItem(window, IDC_COMBO_NEWLINE));
		Static newlineLabel(::GetDlgItem(window, IDC_STATIC_2));
		HFONT guiFont = reinterpret_cast<HFONT>(::SendMessage(dialog, WM_GETFONT, 0, 0L));

		// ダイアログテンプレートのコントロールの位置合わせなど
		::POINT pt;
		::RECT rect;
		::GetWindowRect(window, &rect);
		pt.x = rect.left;
		pt.y = rect.top;

		// ラベル
		::GetWindowRect(::GetDlgItem(dialog, stc2), &rect);
		long x = rect.left;
		encodingLabel.getRect(rect);
		encodingLabel.setPosition(0, x - pt.x, rect.top - pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		encodingLabel.setFont(guiFont);
		if(newlineLabel.isWindow()) {
			newlineLabel.getRect(rect);
			newlineLabel.setPosition(0, x - pt.x, rect.top - pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
			newlineLabel.setFont(guiFont);
		}

		// コンボボックス
		::GetWindowRect(::GetDlgItem(dialog, cmb1), &rect);
		x = rect.left;
		encodingCombobox.getRect(rect);
		encodingCombobox.setPosition(0, x - pt.x, rect.top - pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		encodingCombobox.setFont(guiFont);
		if(newlineCombobox.isWindow()) {
			newlineCombobox.getRect(rect);
			newlineCombobox.setPosition(0, x - pt.x, rect.top - pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
			newlineCombobox.setFont(guiFont);
		}

		const TextFileFormat& format = *reinterpret_cast<TextFileFormat*>(ofn.lCustData);
		const Encoder* asciiEncoder = Encoder::forMIB(fundamental::US_ASCII);
		assert(asciiEncoder != 0);
		vector<string> encodings;
		Encoder::availableNames(back_inserter(encodings));

		for(vector<string>::const_iterator encoding(encodings.begin()), e(encodings.end()); encoding != e; ++encoding) {
			const wstring name(asciiEncoder->toUnicode(*encoding));
			if(!name.empty())
				encodingCombobox.setItemData(encodingCombobox.addString(name.c_str()),
					matchEncodingNames(name.begin(), name.end(), format.encoding.begin(), format.encoding.end()) ? 1: 0);
		}
		if(!newlineCombobox.isWindow()) {
			encodings.clear();
			EncodingDetector::availableNames(back_inserter(encodings));
			for(vector<string>::const_iterator encoding(encodings.begin()), e(encodings.end()); encoding != e; ++encoding) {
				const wstring name(asciiEncoder->toUnicode(*encoding));
				if(!name.empty())
					encodingCombobox.setItemData(encodingCombobox.addString(name.c_str()),
						matchEncodingNames(name.begin(), name.end(), format.encoding.begin(), format.encoding.end()) ? 1: 0);
			}
		}

		encodingCombobox.setCurSel(0);
		for(::UINT i = 0, c = encodingCombobox.getCount(); i < c; ++i) {
			if(encodingCombobox.getItemData(i) == 1) {
				encodingCombobox.setCurSel(i);
				break;
			}
		}

		if(newlineCombobox.isWindow()) {
			switch(format.newline) {
			case NLF_RAW_VALUE:				newlineCombobox.setCurSel(0);	break;
			case NLF_CR_LF:					newlineCombobox.setCurSel(1);	break;
			case NLF_LINE_FEED:				newlineCombobox.setCurSel(2);	break;
			case NLF_CARRIAGE_RETURN:		newlineCombobox.setCurSel(3);	break;
			case NLF_NEXT_LINE:				newlineCombobox.setCurSel(4);	break;
			case NLF_LINE_SEPARATOR:		newlineCombobox.setCurSel(5);	break;
			case NLF_PARAGRAPH_SEPARATOR:	newlineCombobox.setCurSel(6);	break;
			}
			::SendMessageW(window, WM_COMMAND, MAKEWPARAM(IDC_COMBO_ENCODING, CBN_SELCHANGE), 0);
		}
	}
		break;
	case WM_NOTIFY: {
		::OFNOTIFYW& ofn = *reinterpret_cast<OFNOTIFYW*>(lParam);
		if(ofn.hdr.code == CDN_FILEOK) {	// [開く]/[保存]
			ComboBox encodingCombobox(::GetDlgItem(window, IDC_COMBO_ENCODING));
			ComboBox newlineCombobox(::GetDlgItem(window, IDC_COMBO_NEWLINE));
			Button readOnlyCheckbox(::GetDlgItem(::GetParent(window), chx1));
			TextFileFormat& format = *reinterpret_cast<TextFileFormat*>(ofn.lpOFN->lCustData);

			wstring encodingName(encodingCombobox.getText());
			format.encoding = Encoder::forMIB(fundamental::US_ASCII)->fromUnicode(encodingName);
			if(Encoder::forName(format.encoding) == 0) {
				// reject for invalid encoding name
				Alpha::instance().messageBox(MSG_IO__UNSUPPORTED_ENCODING, MB_OK | MB_ICONEXCLAMATION);
				::SetWindowLongPtrW(window, DWLP_MSGRESULT, true);
				return true;
			}
			if(newlineCombobox.isWindow()) {
				switch(newlineCombobox.getCurSel()) {
				case 0:	format.newline = NLF_RAW_VALUE;				break;
				case 1:	format.newline = NLF_CR_LF;					break;
				case 2:	format.newline = NLF_LINE_FEED;				break;
				case 3:	format.newline = NLF_CARRIAGE_RETURN;		break;
				case 4:	format.newline = NLF_NEXT_LINE;				break;
				case 5:	format.newline = NLF_LINE_SEPARATOR;		break;
				case 6:	format.newline = NLF_PARAGRAPH_SEPARATOR;	break;
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
	const size_t a = activeIndex();
	setActive(buffer);
	switch(context) {
	case IUnexpectedFileTimeStampDirector::FIRST_MODIFICATION:
		return app_.messageBox(MSG_BUFFER__FILE_IS_MODIFIED_AND_EDIT, MB_YESNO | MB_ICONQUESTION, MARGS % buffer.textFile().pathName()) == IDYES;
	case IUnexpectedFileTimeStampDirector::OVERWRITE_FILE:
		return app_.messageBox(MSG_BUFFER__FILE_IS_MODIFIED_AND_SAVE, MB_YESNO | MB_ICONQUESTION, MARGS % buffer.textFile().pathName()) == IDYES;
	case IUnexpectedFileTimeStampDirector::CLIENT_INVOCATION:
		if(IDYES == app_.messageBox(MSG_BUFFER__FILE_IS_MODIFIED_AND_REOPEN, MB_YESNO | MB_ICONQUESTION, MARGS % buffer.textFile().pathName()))
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
		MANAH_AUTO_STRUCT(::REBARBANDINFOW, rbbi);
		Rebar rebar(bufferBarPager_.getParent().getHandle());
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
 * @param changeEncoding set true to change the encoding
 * @return the result. see the description of @c BufferList#OpenResult
 * @throw std#out_of_range @a index is invalid
 */
BufferList::OpenResult BufferList::reopen(size_t index, bool changeEncoding) {
	Buffer& buffer = at(index);

	// ファイルが存在するか?
	if(!buffer.textFile().isOpen())
		return OPENRESULT_FAILED;

	// ユーザがキャンセル
	else if(buffer.isModified() && IDNO == app_.messageBox(MSG_BUFFER__CONFIRM_REOPEN_EVEN_IF_DIRTY, MB_YESNO | MB_ICONQUESTION))
		return OPENRESULT_USERCANCELED;

	// エンコードを変更する場合はダイアログを出す
	string encoding(buffer.textFile().encoding());
	if(changeEncoding) {
		ui::EncodingsDialog dlg(encoding, true);
		if(dlg.doModal(app_.getMainWindow()) != IDOK)
			return OPENRESULT_USERCANCELED;
		encoding = dlg.resultEncoding();
	}

	bool succeeded = true;
	IOException::Type errorType;
	while(true) {
		try {
			buffer.textFile().open(buffer.textFile().pathName(), buffer.textFile().lockMode(), encoding, Encoder::NO_POLICY);
		} catch(IOException& e) {
			succeeded = false;
			errorType = e.type();
		}
		if(!succeeded) {
			// alert the encoding error
			int userAnswer;
			if(errorType == IOException::UNMAPPABLE_CHARACTER)
				userAnswer = Alpha::instance().messageBox(
					MSG_IO__UNCONVERTABLE_NATIVE_CHAR, MB_YESNOCANCEL | MB_ICONEXCLAMATION,
					MARGS % buffer.textFile().pathName() % Encoder::forMIB(fundamental::US_ASCII)->toUnicode(encoding).c_str());
			else if(errorType == IOException::MALFORMED_INPUT)
				userAnswer = Alpha::instance().messageBox(
					MSG_IO__MALFORMED_INPUT_FILE, MB_OKCANCEL | MB_ICONEXCLAMATION,
					MARGS % buffer.textFile().pathName() % Encoder::forMIB(fundamental::US_ASCII)->toUnicode(encoding).c_str());
			else
				break;
			succeeded = true;
			if(userAnswer == IDYES || userAnswer == IDOK) {
				// the user want to change the encoding
				ui::EncodingsDialog dlg(encoding, true);
				if(dlg.doModal(app_.getMainWindow()) != IDOK)
					return OPENRESULT_USERCANCELED;
				encoding = dlg.resultEncoding();
				continue;
			} else if(userAnswer == IDNO) {
				succeeded = true;
				try {
					buffer.textFile().open(buffer.textFile().pathName(), buffer.textFile().lockMode(), encoding, Encoder::REPLACE_UNMAPPABLE_CHARACTER);
				} catch(IOException& e) {
					succeeded = false;
					if((errorType = e.type()) == IOException::MALFORMED_INPUT) {
						Alpha::instance().messageBox(
							MSG_IO__MALFORMED_INPUT_FILE, MB_OK | MB_ICONEXCLAMATION,
							MARGS % buffer.textFile().pathName() % Encoder::forMIB(fundamental::US_ASCII)->toUnicode(encoding).c_str());
						return OPENRESULT_FAILED;
					}
				}
			} else
				return OPENRESULT_USERCANCELED;
		}
		break;
	}

	if(succeeded || handleFileIOError(buffer.textFile().pathName().c_str(), true, errorType)) {
		app_.mruManager().add(buffer.textFile().pathName());
		return OPENRESULT_SUCCEEDED;
	} else
		return OPENRESULT_FAILED;
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
			(buffers_[i]->textFile().isOpen()) ? buffers_[i]->textFile().pathName().c_str() : L"",
			0, &sfi, sizeof(::SHFILEINFOW), SHGFI_ICON | SHGFI_SMALLICON);
		icons_.add(sfi.hIcon);
		listMenu_ << Menu::OwnerDrawnItem(static_cast<UINT>(i) + CMD_SPECIAL_BUFFERSSTART - CMD_SPECIAL_START);
	}
	bufferBar_.setImageList(icons_.getHandle());
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
	Buffer& buffer = at(index);

	// 保存の必要があるか?
	if(overwrite && buffer.textFile().isOpen() && !buffer.isModified())
		return true;

	::WCHAR fileName[MAX_PATH + 1];
	TextFileFormat format = {buffer.textFile().encoding(), NLF_RAW_VALUE};
	bool newName = false;

	// 別名で保存 or ファイルが存在しない
	if(!overwrite || !buffer.textFile().isOpen() || !toBoolean(::PathFileExistsW(buffer.textFile().pathName().c_str()))) {
		MANAH_AUTO_STRUCT(::OSVERSIONINFOW, osVersion);
		const wstring filterSource(app_.loadMessage(MSG_DIALOG__SAVE_FILE_FILTER));
		wchar_t* const filter = new wchar_t[filterSource.length() + 6];

		osVersion.dwOSVersionInfoSize = sizeof(::OSVERSIONINFOW);
		::GetVersionEx(&osVersion);
		filterSource.copy(filter, filterSource.length());
		wcsncpy(filter + filterSource.length(), L"\0*.*\0\0", 6);
		wcscpy(fileName, (buffer.textFile().pathName().c_str()));

		MANAH_AUTO_STRUCT_SIZE(::OPENFILENAMEW, newOfn);
		MANAH_AUTO_STRUCT_SIZE(::OPENFILENAME_NT4W, oldOfn);
		::OPENFILENAMEW& ofn = (osVersion.dwMajorVersion > 4) ? newOfn : *reinterpret_cast<::OPENFILENAMEW*>(&oldOfn);
		ofn.hwndOwner = app_.getMainWindow().getHandle();
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
		wcscpy(fileName, buffer.textFile().pathName().c_str());

	MIBenum encodingMIB = MIB_UNKNOWN;
	if(const Encoder* encoder = Encoder::forName(format.encoding))
		encodingMIB = encoder->mibEnum();
	const bool writeBOM =
		(encodingMIB == fundamental::UTF_8 && toBoolean(app_.readIntegerProfile(L"File", L"writeBOMAsUTF8", 0)))
		|| (encodingMIB == fundamental::UTF_16LE && toBoolean(app_.readIntegerProfile(L"File", L"writeBOMAsUTF16LE", 1)))
		|| (encodingMIB == fundamental::UTF_16BE && toBoolean(app_.readIntegerProfile(L"File", L"writeBOMAsUTF16BE", 1)))
		|| (encodingMIB == extended::UTF_32LE && toBoolean(app_.readIntegerProfile(L"File", L"writeBOMAsUTF32LE", 1)))
		|| (encodingMIB == extended::UTF_32BE && toBoolean(app_.readIntegerProfile(L"File", L"writeBOMAsUTF32BE", 1)));

	bool succeeded = true;
	IOException::Type errorType;
	while(true) {
		TextFileDocumentInput::WriteParameters params;
		params.encoding = format.encoding;
		params.encodingPolicy = Encoder::NO_POLICY;
		params.newline = format.newline;
		if(writeBOM)
			params.options = TextFileDocumentInput::WriteParameters::WRITE_UNICODE_BYTE_ORDER_SIGNATURE;

		try {
			buffer.textFile().write(fileName, params);
		} catch(IOException& e) {
			succeeded = false;
			errorType = e.type();
		}
		if(!succeeded && errorType == IOException::UNMAPPABLE_CHARACTER) {
			// alert the encoding error
			const int userAnswer = Alpha::instance().messageBox(
				MSG_IO__UNCONVERTABLE_UCS_CHAR, MB_YESNOCANCEL | MB_ICONEXCLAMATION,
				MARGS % fileName % Encoder::forMIB(fundamental::US_ASCII)->toUnicode(params.encoding).c_str());
			if(userAnswer == IDYES) {
				// the user want to change the encoding
				ui::EncodingsDialog dlg(params.encoding, false);
				if(dlg.doModal(app_.getMainWindow()) != IDOK)
					return false;
				params.encoding = dlg.resultEncoding();
				continue;
			} else if(userAnswer == IDNO) {
				succeeded = true;
				params.encodingPolicy = Encoder::REPLACE_UNMAPPABLE_CHARACTER;
				try {
					buffer.textFile().write(fileName, params);
				} catch(IOException& e) {
					succeeded = false;
					errorType = e.type();
				}
				break;
			} else
				return false;
		}
		break;
	}

	succeeded = succeeded || handleFileIOError(fileName, false, errorType);
	if(succeeded && addToMRU && newName)
		app_.mruManager().add(fileName);
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
	editorWindow_.getActivePane().showBuffer(at(index));
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

/// @see ascension#presentation#ITextViewerListListener#textViewerListChanged
void BufferList::textViewerListChanged(Presentation& presentation) {
}

/// Reconstructs the context menu.
void BufferList::updateContextMenu() {
	while(contextMenu_.getNumberOfItems() > 0)
		contextMenu_.erase<Menu::BY_POSITION>(0);
	contextMenu_ << Menu::StringItem(CMD_FILE_CLOSE, app_.commandManager().menuName(CMD_FILE_CLOSE).c_str())
		<< Menu::StringItem(CMD_FILE_CLOSEOTHERS, app_.commandManager().menuName(CMD_FILE_CLOSEOTHERS).c_str());
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
	for(set<EditorView*>::const_iterator i(rhs.views_.begin()), e(rhs.views_.end()); i != e; ++i) {
		EditorView* view = new EditorView(**i);
		const bool succeeded = view->create((*i)->getParent().getHandle(), DefaultWindowRect(),
			WS_CHILD | WS_CLIPCHILDREN | WS_HSCROLL | WS_VISIBLE | WS_VSCROLL, WS_EX_CLIENTEDGE);
		assert(succeeded);
		view->setConfiguration(&(*i)->configuration(), 0);
		view->scrollTo((*i)->getScrollPosition(SB_HORZ), (*i)->getScrollPosition(SB_VERT), false);
		views_.insert(view);
		if(*i == rhs.visibleView_)
			visibleView_ = view;
		if(*i == rhs.lastVisibleView_)
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
		showBuffer(view.document());
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
		if(&(*it)->document() == &buffer) {
			EditorView& removing = **it;
			views_.erase(it);
			if(&removing == visibleView_) {
				visibleView_ = 0;
				if(&removing == lastVisibleView_)
					lastVisibleView_ = 0;
				if(views_.size() == 1 || lastVisibleView_ == 0)
					showBuffer((*views_.begin())->document());
				else if(!views_.empty()) {
					showBuffer(lastVisibleView_->document());
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
	if(visibleView_ != 0 && &visibleView_->document() == &buffer)
		return;
	for(set<EditorView*>::iterator it = views_.begin(); it != views_.end(); ++it) {
		if(&(*it)->document() == &buffer) {
			const bool hadFocus = visibleView_ == 0 || visibleView_->hasFocus();
			lastVisibleView_ = visibleView_;
			visibleView_ = *it;
			Alpha::instance().bufferList().editorWindow().adjustPanes();
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
EditorView::EditorView(Presentation& presentation) : TextViewer(presentation), visualColumnStartValue_(1) {
	document().textFile().addListener(*this);
	document().bookmarker().addListener(*this);
}

/// Copy-constructor.
EditorView::EditorView(const EditorView& rhs) : TextViewer(rhs), visualColumnStartValue_(rhs.visualColumnStartValue_) {
	document().textFile().addListener(*this);
	document().bookmarker().addListener(*this);
}

/// Destructor.
EditorView::~EditorView() {
	document().bookmarker().removeListener(*this);
}

/// Begins incremental search.
void EditorView::beginIncrementalSearch(SearchType type, Direction direction) {
	TextSearcher& searcher = Alpha::instance().bufferList().editorSession().textSearcher();
	SearchOptions options = searcher.options();
	options.type = type;
	searcher.setOptions(options);
	texteditor::commands::IncrementalSearchCommand(*this, direction, this).execute();
}

/// @see IBookmarkListener#bookmarkChanged
void EditorView::bookmarkChanged(length_t line) {
	redrawLine(line);
}

/// @see IBookmarkListener#bookmarkCleared
void EditorView::bookmarkCleared() {
	invalidate();
}

/// @see ICaretListener#caretMoved
void EditorView::caretMoved(const Caret& self, const Region& oldRegion) {
	TextViewer::caretMoved(self, oldRegion);
	updateCurrentPositionOnStatusBar();
}

/// @see IDocumentStateListener#documentAccessibleRegionChanged
void EditorView::documentAccessibleRegionChanged(const Document& document) {
	TextViewer::documentAccessibleRegionChanged(document);
	updateNarrowingOnStatusBar();
}

/// @see IDocumentStateListener#documentModificationSignChanged
void EditorView::documentModificationSignChanged(const Document& document) {
	TextViewer::documentModificationSignChanged(document);
	updateTitleBar();
}

/// @see ascension#text#IDocumentStateListenerdocumentPropertyChanged
void EditorView::documentPropertyChanged(const Document& document, const DocumentPropertyKey& key) {
	TextViewer::documentPropertyChanged(document, key);
}

/// @see IDocumentStateListener#documentReadOnlySignChanged
void EditorView::documentReadOnlySignChanged(const Document& document) {
	TextViewer::documentReadOnlySignChanged(document);
	updateTitleBar();
}

/// @see TextViewer#drawIndicatorMargin
void EditorView::drawIndicatorMargin(length_t line, manah::win32::gdi::DC& dc, const ::RECT& rect) {
	if(document().bookmarker().isMarked(line)) {
		// draw a bookmark indication mark
		const ::COLORREF selColor = ::GetSysColor(COLOR_HIGHLIGHT);
		const ::COLORREF selTextColor = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
//		dc.fillSolidRect(rect, selectionColor);
		::TRIVERTEX vertex[2];
		vertex[0].x = rect.left + 2;
		vertex[0].y = (rect.top * 2 + rect.bottom) / 3;
		vertex[0].Red = static_cast<::COLOR16>((selColor >> 0) & 0xFF) << 8;
		vertex[0].Green = static_cast<::COLOR16>((selColor >> 8) & 0xFF) << 8;
		vertex[0].Blue = static_cast<::COLOR16>((selColor >> 16) & 0xFF) << 8;
		vertex[0].Alpha = 0x0000;
		vertex[1].x = rect.right - 2;
		vertex[1].y = (rect.top + rect.bottom * 2) / 3;
		vertex[1].Red = static_cast<::COLOR16>((selTextColor >> 0) & 0xFF) << 8;
		vertex[1].Green = static_cast<::COLOR16>((selTextColor >> 8) & 0xFF) << 8;
		vertex[1].Blue = static_cast<::COLOR16>((selTextColor >> 16) & 0xFF) << 8;
		vertex[1].Alpha = 0x0000;
		::GRADIENT_RECT mesh;
		mesh.UpperLeft = 0;
		mesh.LowerRight = 1;
		::GradientFill(dc.getHandle(), vertex, countof(vertex), &mesh, 1, GRADIENT_FILL_RECT_H);
	}
}

/// @see IFilePropertyListener#fileEncodingChanged
void EditorView::fileEncodingChanged(const TextFileDocumentInput&) {
	// do nothing
}

/// @see IFilePropertyListener#fileNameChanged
void EditorView::fileNameChanged(const TextFileDocumentInput&) {
	updateTitleBar();
}

/// @see IIncrementalSearchListener#incrementalSearchAborted
void EditorView::incrementalSearchAborted(const Position& initialPosition) {
	incrementalSearchCompleted();
	caret().moveTo(initialPosition);
}

/// @see IIncrementalSearchListener#incrementalSearchCompleted
void EditorView::incrementalSearchCompleted() {
	Alpha::instance().setStatusText(0);
}

/// @see IIncrementalSearchListener#incrementalSearchPatternChanged
void EditorView::incrementalSearchPatternChanged(Result result, const manah::Flags<WrappingStatus>&) {
	::UINT messageID;
	Alpha& app = Alpha::instance();
	const IncrementalSearcher& isearch = Alpha::instance().bufferList().editorSession().incrementalSearcher();
	const bool forward = isearch.direction() == FORWARD;

	if(result == IIncrementalSearchCallback::EMPTY_PATTERN) {
		caret().select(isearch.matchedRegion());
		messageID = forward ? MSG_STATUS__ISEARCH_EMPTY_PATTERN : MSG_STATUS__RISEARCH_EMPTY_PATTERN;
		app.setStatusText(app.loadMessage(messageID).c_str(),
			toBoolean(app.readIntegerProfile(L"View", L"applyMainFontToSomeControls", 1)) ? textRenderer().font() : 0);
		return;
	} else if(result == IIncrementalSearchCallback::FOUND) {
		caret().select(isearch.matchedRegion());
		messageID = forward ? MSG_STATUS__ISEARCH : MSG_STATUS__RISEARCH;
	} else {
		if(result == IIncrementalSearchCallback::NOT_FOUND)
			messageID = forward ? MSG_STATUS__ISEARCH_NOT_FOUND : MSG_STATUS__RISEARCH_NOT_FOUND;
		else
			messageID = forward ? MSG_STATUS__ISEARCH_BAD_PATTERN : MSG_STATUS__RISEARCH_BAD_PATTERN;
		beep();
	}

	ascension::String prompt(app.loadMessage(messageID, MARGS % isearch.pattern()));
	replace_if(prompt.begin(), prompt.end(), bind2nd(equal_to<wchar_t>(), L'\t'), L' ');
	app.setStatusText(prompt.c_str(),
			toBoolean(app.readIntegerProfile(L"View", L"applyMainFontToSomeControls", 1)) ? textRenderer().font() : 0);
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
	return TextViewer::onKeyDown(vkey, flags, handled);
}

/// @see Window#onKillFocus
void EditorView::onKillFocus(HWND newWindow) {
	TextViewer::onKillFocus(newWindow);
	Alpha::instance().bufferList().editorSession().incrementalSearcher().end();
}

/// @see Window#onSetFocus
void EditorView::onSetFocus(HWND oldWindow) {
	TextViewer::onSetFocus(oldWindow);
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
		static manah::AutoBuffer<::WCHAR> message, messageFormat;
		static size_t formatLength = 0;
		if(messageFormat.get() == 0) {
			void* messageBuffer;
			if(0 != ::FormatMessageW(
					FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE,
					::GetModuleHandle(0), MSG_STATUS__CARET_POSITION, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					reinterpret_cast<wchar_t*>(&messageBuffer), 0, 0)) {
				formatLength = wcslen(static_cast<::WCHAR*>(messageBuffer));
				messageFormat.reset(new ::WCHAR[formatLength + 1]);
				wcscpy(messageFormat.get(), static_cast<::WCHAR*>(messageBuffer));
				::LocalFree(messageBuffer);
			} else {
				messageFormat.reset(new ::WCHAR[1]);
				messageFormat[0] = 0;
			}
			message.reset(new ::WCHAR[formatLength + 100]);
		}
		if(formatLength != 0) {
			length_t messageArguments[3];
			MANAH_AUTO_STRUCT(::SCROLLINFO, si);
			getScrollInformation(SB_VERT, si, SIF_POS | SIF_RANGE);
			messageArguments[0] = caret().lineNumber() + verticalRulerConfiguration().lineNumbers.startValue;
			messageArguments[1] = caret().visualColumnNumber() + visualColumnStartValue_;
			messageArguments[2] = caret().columnNumber() + visualColumnStartValue_;
			::FormatMessageW(FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_FROM_STRING, messageFormat.get(),
				0, 0, message.get(), static_cast<::DWORD>(formatLength) + 100, reinterpret_cast<va_list*>(messageArguments));
			// show in the status bar
			Alpha::instance().statusBar().setText(1, message.get());
		}
	}
}

void EditorView::updateNarrowingOnStatusBar() {
	if(hasFocus()) {
		const bool narrow = document().isNarrowed();
		Alpha& app = Alpha::instance();
		if(narrowingIcon_.getHandle() == 0)
			narrowingIcon_.reset(static_cast<::HICON>(app.loadImage(IDR_ICON_NARROWING, IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR)));
		StatusBar& statusBar = app.statusBar();
//		statusBar.setText(4, narrow ? app.loadMessage(MSG_STATUS__NARROWING).c_str() : L"");
		statusBar.setTipText(4, narrow ? app.loadMessage(MSG_STATUS__NARROWING).c_str() : L"");
		statusBar.setIcon(4, narrow ? narrowingIcon_.getHandle() : 0);
	}
}

void EditorView::updateOvertypeModeOnStatusBar() {
	if(hasFocus())
		Alpha::instance().statusBar().setText(3,
			Alpha::instance().loadMessage(caret().isOvertypeMode() ? MSG_STATUS__OVERTYPE_MODE : MSG_STATUS__INSERT_MODE).c_str());
}

/// Updates the title bar text according to the current state.
void EditorView::updateTitleBar() {
	static wstring titleCache;
	Window& mainWindow = Alpha::instance().getMainWindow();
	if(!mainWindow.isWindow())
		return;
	wstring title(BufferList::getDisplayName(document()));
	if(title != titleCache) {
		titleCache = title;
//		title += L" - " IDS_APPFULLVERSION;
		title += L" - " IDS_APPNAME;
		mainWindow.setText(title.c_str());
	}
}
