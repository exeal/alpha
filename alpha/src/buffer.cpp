/**
 * @file buffer.cpp
 * @author exeal
 * @date 2003-2006 (was AlphaDoc.cpp and BufferList.cpp)
 * @date 2006-2009
 */

#include "application.hpp"
#include "buffer.hpp"
#include "editor-window.hpp"
//#include "command.hpp"
//#include "mru-manager.hpp"
#include "new-file-format-dialog.hpp"
#include "save-some-buffers-dialog.hpp"
//#include "code-pages-dialog.hpp"
#include "../resource/messages.h"
#include <manah/win32/ui/wait-cursor.hpp>
#include <manah/com/common.hpp>	// ComPtr, ComQIPtr
#include <manah/com/exception.hpp>
#include <shlwapi.h>				// PathXxxx, StrXxxx, ...
#include <shlobj.h>					// IShellLink, ...
#include <dlgs.h>
using namespace alpha;
using namespace ambient;
using namespace manah;
using namespace std;
namespace a = ascension;
namespace e = ascension::encoding;
namespace f = ascension::kernel::fileio;
namespace k = ascension::kernel;
namespace py = boost::python;

namespace {
	struct TextFileFormat {
		string encoding;
		k::Newline newline;
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
	void saveBuffer(Buffer& buffer, const string& encoding = string(), k::Newline newlines = k::NLF_RAW_VALUE,
			e::Encoder::SubstitutionPolicy encodingSubstitutionPolicy = e::Encoder::DONT_SUBSTITUTE, bool writeUnicodeByteOrderMark = false) {
		if(buffer.textFile().isBoundToFile() && !buffer.isModified())
			return;

		f::WritingFormat format;
		format.encoding = !encoding.empty() ? encoding : buffer.textFile().encoding();
		format.encodingSubstitutionPolicy = encodingSubstitutionPolicy;
		format.newline = k::isLiteralNewline(newlines) ? newlines : buffer.textFile().newline();
		format.unicodeByteOrderMark = writeUnicodeByteOrderMark;

		try {
			buffer.textFile().write(format, 0);
		} catch(const f::UnmappableCharacterException& e) {
			::PyErr_SetString(PyExc_UnicodeDecodeError, e.what());
		} catch(const f::IOException& e) {
			::PyErr_SetFromWindowsErr(e.code());
		}
	}
} // namespace @0


// Buffer ///////////////////////////////////////////////////////////////////

/// Constructor.
Buffer::Buffer() {
	presentation_.reset(new a::presentation::Presentation(*this));
	textFile_.reset(new f::TextFileDocumentInput(*this));
}

/// Destructor.
Buffer::~Buffer() {
}

/**
 * Returns the name of the buffer.
 * @see BufferList#getDisplayName
 */
const basic_string<WCHAR> Buffer::name() const /*throw()*/ {
	static const wstring untitled(Alpha::instance().loadMessage(MSG_BUFFER__UNTITLED));
	return !textFile_->isBoundToFile() ? ::PathFindFileNameW(textFile_->location().c_str()) : untitled;
	
}

/// Returns the presentation object of Ascension.
a::presentation::Presentation& Buffer::presentation() /*throw()*/ {
	return *presentation_;
}

/// Returns the presentation object of Ascension.
const a::presentation::Presentation& Buffer::presentation() const /*throw()*/ {
	return *presentation_;
}


// BufferList ///////////////////////////////////////////////////////////////

/// Default constructor.
BufferList::BufferList() {
	// create the context menu
	updateContextMenu();
}

/// Destructor.
BufferList::~BufferList() {
//	for(EditorWindows::Iterator i(EditorWindows::instance().enumeratePanes()); !i.done(); i.next())
//		i.get().removeAll();
	for(size_t i = 0; i < buffers_.size(); ++i) {
		editorSession_.removeDocument(*buffers_[i]);
		delete buffers_[i];
	}
	if(icons_.get() != 0) {
		const int c = icons_.getNumberOfImages();
		for(int i = 0; i < c; ++i)
			::DestroyIcon(icons_.getIcon(i, ILD_NORMAL));
		icons_.destroy();
	}
}

void BufferList::activeBufferChanged() {
	const EditorWindow& window = EditorWindows::instance().activePane();
	const Buffer& buffer = window.visibleBuffer();
	EditorView& viewer = window.visibleView();

	// update the default active window
	for(EditorWindows::Iterator i(EditorWindows::instance().enumeratePanes()); !i.done(); i.next()) {
		if(&i.get().visibleView() == &viewer) {
			EditorWindows::instance().setDefaultActivePane(i.get());
			break;
		}
	}

	// title bar
	updateTitleBar();

	// buffer bar button
	const size_t button = find(buffer);
	if(button != -1)
		bufferBar_.checkButton(static_cast<int>(button));

	// scroll the buffer bar if the button was hidden
	if(bufferBarPager_.isVisible()) {
		const int pagerPosition = bufferBarPager_.getPosition();
		RECT buttonRect, pagerRect;
		bufferBar_.getItemRect(static_cast<int>(button), buttonRect);
		bufferBarPager_.getClientRect(pagerRect);
		if(buttonRect.left < pagerPosition)
			bufferBarPager_.setPosition(buttonRect.left);
		else if(buttonRect.right > pagerPosition + pagerRect.right)
			bufferBarPager_.setPosition(buttonRect.right - pagerRect.right);
	}

	Alpha::instance().statusBar().updateAll();
}

/**
 * Opens the new empty buffer. This method does not activate the new buffer.
 * @param name the name of the buffer
 * @param encoding the encoding
 * @param newline the newline
 * @return the buffer added
 * @throw ascension#encoding#UnsupportedEncodingException @a encoding is not supported
 * @throw ascension#UnknownValueException @a newline is invalid
 */
Buffer& BufferList::addNew(const ascension::String& name /* = L"" */,
		const string& encoding /* = "UTF-8" */, ascension::kernel::Newline newline /* = NLF_AUTO */) {
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
	auto_ptr<Buffer> buffer(new Buffer());

	buffer->textFile().setEncoding(encoding);
	if(isLiteralNewline(newline))
		buffer->textFile().setNewline(newline);

	editorSession_.addDocument(*buffer);
	buffer->setProperty(k::Document::TITLE_PROPERTY, !name.empty() ? name : L"*untitled*");
	buffers_.push_back(buffer.get());

	// 現在のペインの数だけビューを作成する
	LOGFONTW font;
	Alpha::instance().textEditorFont(font);
	EditorView* originalView = 0;
	for(EditorWindows::Iterator i(EditorWindows::instance().enumeratePanes()); !i.done(); i.next()) {
		EditorView* view = (originalView == 0) ? new EditorView(buffer->presentation()) : new EditorView(*originalView);
		view->create(EditorWindows::instance().use(), win32::ui::DefaultWindowRect(),
			WS_CHILD | WS_CLIPCHILDREN | WS_HSCROLL | WS_VISIBLE | WS_VSCROLL, WS_EX_CLIENTEDGE);
		assert(view->isWindow());
		if(originalView == 0)
			originalView = view;
		i.get().addView(*view);
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
	const basic_string<WCHAR> bufferName(buffer->name());
	win32::AutoZero<TBBUTTON> button;
	button.idCommand = bufferBar_.getButtonCount();
	button.iBitmap = static_cast<int>(buffers_.size() - 1);
	button.fsState = TBSTATE_ENABLED;
	button.fsStyle = BTNS_AUTOSIZE | BTNS_BUTTON | BTNS_GROUP | BTNS_NOPREFIX;
	button.dwData = reinterpret_cast<DWORD_PTR>(buffer.release());
	button.iString = reinterpret_cast<INT_PTR>(bufferName.c_str());
	bufferBar_.insertButton(bufferBar_.getButtonCount(), button);

	resetResources();
//	app_.applyDocumentType(*buffer, docType);

	return *buffers_.back();
}

/**
 * Shows "New with Format" dialog box and opens the new empty buffer.
 * @param name the name of the buffer
 * @return the buffer added or @c null if the user canceled
 */
Buffer* BufferList::addNewDialog(const ascension::String& name /* = L"" */) {
	Alpha& app = Alpha::instance();
	TextFileFormat format;
	format.encoding = e::Encoder::forMIB(e::fundamental::US_ASCII)->fromUnicode(app.readStringProfile(L"File", L"defaultEncoding"));
	if(!e::Encoder::supports(format.encoding))
		format.encoding = e::Encoder::defaultInstance().properties().name();
	format.newline = static_cast<k::Newline>(app.readIntegerProfile(L"file", L"defaultNewline", k::NLF_CR_LF));

	ui::NewFileFormatDialog dlg(format.encoding, format.newline);
	if(dlg.doModal(app.getMainWindow()) != IDOK)
		return 0;
	return &addNew(name, dlg.encoding(), dlg.newline());
}

/**
 * Closes the specified buffer.
 * @param buffer the buffer to close
 * @return true if the buffer was closed successfully
 * @throw std#out_of_range @a index is invalid
 */
void BufferList::close(Buffer& buffer) {
	const size_t index = find(buffer);
	if(index == -1)
		throw invalid_argument("buffer");
	if(buffers_.size() > 1) {
		bufferBar_.deleteButton(static_cast<int>(buffers_.size() - 1));
		for(EditorWindows::Iterator i(EditorWindows::instance().enumeratePanes()); !i.done(); i.next())
			i.get().removeBuffer(buffer);
		buffers_.erase(buffers_.begin() + index);
		editorSession_.removeDocument(buffer);
		buffer.textFile().removeListener(*this);
		buffer.textFile().unbind();
		delete &buffer;

		// reset the following buffer bar buttons
		for(size_t i = index, c = buffers_.size(); i < c; ++i) {
			bufferBar_.setCommandID(static_cast<int>(i), static_cast<WORD>(i));
			bufferBar_.setButtonText(static_cast<int>(i), getDisplayName(*buffers_[i]).c_str());
		}
		resetResources();
		recalculateBufferBarSize();
		activeBufferChanged();
	} else {	// the buffer is last one
		buffer.textFile().unbind();
		buffer.resetContent();
	}
}

/**
 * Reconstructs the buffer bar.
 * @param rebar the rebar on which the buffer bar set
 * @return success or not
 */
bool BufferList::createBar(win32::ui::Rebar& rebar) {
	if(bufferBarPager_.isWindow()) {
		rebar.deleteBand(rebar.idToIndex(IDC_BUFFERBARPAGER));
		bufferBar_.destroy();
		bufferBarPager_.destroy();
	}

	// バッファバーとページャを作成する
	if(!bufferBarPager_.create(rebar.use(), win32::ui::DefaultWindowRect(), 0, IDC_BUFFERBARPAGER,
			WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE | CCS_NORESIZE | PGS_HORZ))
		return false;
	if(!bufferBar_.create(bufferBarPager_.use(), win32::ui::DefaultWindowRect(), 0, IDC_BUFFERBAR,
			WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE
			| CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE | CCS_TOP
			| TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_REGISTERDROP | TBSTYLE_TOOLTIPS | TBSTYLE_TRANSPARENT, WS_EX_TOOLWINDOW)) {
		bufferBarPager_.destroy();
		return false;
	}
	HWND toolTips = bufferBar_.getToolTips();
	bufferBar_.setButtonStructSize();
	::SetWindowLongPtrW(toolTips, GWL_STYLE, ::GetWindowLongPtrW(toolTips, GWL_STYLE) | TTS_NOPREFIX);
	bufferBarPager_.setChild(bufferBar_.use());

	// レバーに乗せる
	win32::AutoZeroSize<REBARBANDINFOW> rbbi;
	const wstring caption = Alpha::instance().loadMessage(MSG_DIALOG__BUFFERBAR_CAPTION);
	rbbi.fMask = RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_ID | RBBIM_STYLE | RBBIM_TEXT;
	rbbi.fStyle = RBBS_BREAK | RBBS_GRIPPERALWAYS;
	rbbi.cxMinChild = 0;
	rbbi.cyMinChild = 22;
	rbbi.wID = IDC_BUFFERBAR;
	rbbi.lpText = const_cast<wchar_t*>(caption.c_str());
	rbbi.hwndChild = bufferBarPager_.use();
	if(!rebar.insertBand(rebar.getBandCount(), rbbi)) {
		bufferBar_.destroy();
		bufferBarPager_.destroy();
		return false;
	}
	return true;
}

/// @see ascension#text#IDocumentStateListener#documentAccessibleRegionChanged
void BufferList::documentAccessibleRegionChanged(const k::Document&) {
	Alpha::instance().statusBar().updateNarrowingStatus();
}

/// @see ascension#text#IDocumentStateListener#documentModificationSignChanged
void BufferList::documentModificationSignChanged(const k::Document& document) {
	const Buffer& buffer = getConcreteDocument(document);
	bufferBar_.setButtonText(static_cast<int>(find(buffer)), getDisplayName(buffer).c_str());
	recalculateBufferBarSize();
	updateTitleBar();
}

/// @see ascension#text#IDocumentStateListenerdocumentPropertyChanged
void BufferList::documentPropertyChanged(const k::Document&, const k::DocumentPropertyKey&) {
	// do nothing
}

/// @see ascension#text#IDocumentStateListenerdocumentReadOnlySignChanged
void BufferList::documentReadOnlySignChanged(const k::Document& document) {
	const Buffer& buffer = getConcreteDocument(document);
	bufferBar_.setButtonText(static_cast<int>(find(buffer)), getDisplayName(buffer).c_str());
	recalculateBufferBarSize();
	updateTitleBar();
}

/// @see ascension#kernel#fileio#IFilePropertyListener#fileNameChanged
void BufferList::fileNameChanged(const f::TextFileDocumentInput& textFile) {
	const Buffer& buffer = getConcreteDocument(textFile.document());
	// TODO: call mode-application.
	resetResources();
	bufferBar_.setButtonText(static_cast<int>(find(buffer)), getDisplayName(buffer).c_str());
	bufferBarPager_.recalcSize();
	updateTitleBar();
}

/// @see ascension#kernel#fileio#IFilePropertyListener#fileEncodingChanged
void BufferList::fileEncodingChanged(const f::TextFileDocumentInput& textFile) {
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

/// Implements _BufferList.for_filename method.
py::object BufferList::forFileName(const basic_string<WCHAR>& fileName) const {
	for(size_t i = 0, c = buffers_.size(); i < c; ++i) {
		if(buffers_[i]->textFile().isBoundToFile()
				&& f::comparePathNames(buffers_[i]->textFile().fileName().c_str(), fileName.c_str()))
			return buffers_[i]->self();
	}
	return py::object();
}

/**
 * Translates the abstract document into a @c Buffer.
 * @document the document
 * @throw std#invalid_argument @a document is not found
 * @see BufferList#find
 */
Buffer& BufferList::getConcreteDocument(k::Document& document) const {
	for(size_t i = 0; i < buffers_.size(); ++i) {
		if(buffers_[i] == &document)
			return *buffers_[i];
	}
	throw invalid_argument("The specified document is not in the list.");
}

/// Const-version of @c #getConcreteDocument(Document&).
const Buffer& BufferList::getConcreteDocument(const k::Document& document) const {
	return getConcreteDocument(const_cast<k::Document&>(document));
}

/**
 * タイトルバーやバッファバーに表示するバッファの名前を返す
 * @param buffer the buffer
 * @return the name
 */
wstring BufferList::getDisplayName(const Buffer& buffer) {
	wstring name(buffer.name());
	if(buffer.isModified())
		name.append(L" *");
	if(buffer.isReadOnly())
		name.append(L" #");
	return name;
}

/// Handles @c WM_NOTIFY message from the buffer bar.
LRESULT BufferList::handleBufferBarNotification(NMTOOLBARW& nmhdr) {
	if(nmhdr.hdr.code == NM_RCLICK) {	// right click -> context menu
		const NMMOUSE& mouse = *reinterpret_cast<NMMOUSE*>(&nmhdr.hdr);
		if(mouse.dwItemSpec != -1) {
			POINT pt = mouse.pt;
			bufferBar_.clientToScreen(pt);
			EditorWindows::instance().activePane().showBuffer(at(mouse.dwItemSpec));
			contextMenu_.trackPopup(TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON, pt.x, pt.y, Alpha::instance().getMainWindow().use());
			return true;
		}
	}

	else if(nmhdr.hdr.code == TTN_GETDISPINFOW) {	// show a tooltip
		assert(static_cast<int>(nmhdr.hdr.idFrom) < bufferBar_.getButtonCount());
		static wchar_t tipText[500];
		NMTTDISPINFOW& nmttdi = *reinterpret_cast<NMTTDISPINFOW*>(&nmhdr.hdr);

//		nmttdi->hinst = getHandle();
		const Buffer& buffer = at(nmttdi.hdr.idFrom);
		wcscpy(tipText, (buffer.textFile().isBoundToFile() ? buffer.textFile().location() : buffer.name()).c_str());
		nmttdi.lpszText = tipText;
		return true;
	}

	else if(nmhdr.hdr.code == TBN_ENDDRAG && bufferBar_.getButtonCount() > 1) {
		TBINSERTMARK mark;
		bufferBar_.getInsertMark(mark);
		if(mark.iButton != -1) {
			// move the button
			move(bufferBar_.commandToIndex(nmhdr.iItem),
				toBoolean(mark.dwFlags & TBIMHT_AFTER) ? mark.iButton + 1 : mark.iButton);
			// delete the insert mark
			mark.dwFlags = 0;
			mark.iButton = -1;
			bufferBar_.setInsertMark(mark);
		}
	}

	// drag -> switch the active buffer
	else if(nmhdr.hdr.code == TBN_GETOBJECT) {
		::NMOBJECTNOTIFY& n = *reinterpret_cast<::NMOBJECTNOTIFY*>(&nmhdr.hdr);
		if(n.iItem != -1) {
			EditorWindows::instance().activePane().showBuffer(at(bufferBar_.commandToIndex(n.iItem)));	// n.iItem は ID
			n.pObject = 0;
			n.hResult = E_NOINTERFACE;
		}
		return 0;
	}

	else if(nmhdr.hdr.code == TBN_HOTITEMCHANGE && bufferBar_.getButtonCount() > 1 && bufferBar_.get() == ::GetCapture()) {
		::NMTBHOTITEM& hotItem = *reinterpret_cast<::NMTBHOTITEM*>(&nmhdr.hdr);
		if(toBoolean(hotItem.dwFlags & HICF_MOUSE)) {	// dragging a button...
			::TBINSERTMARK mark;
			if(!toBoolean(hotItem.dwFlags & HICF_LEAVING)) {
				// move the insert mark
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

/// Returns the singleton instance.
BufferList& BufferList::instance() {
	static BufferList singleton;
	return singleton;
}

/**
 * バッファをリスト内で移動させる
 * @param from 移動するバッファの番号
 * @param to 移動先
 * @throw std::out_of_range @a from が不正なときスロー
 */
void BufferList::move(py::ssize_t from, py::ssize_t to) {
	if(from < 0 || to < 0 || static_cast<size_t>(from) >= buffers_.size() || static_cast<size_t>(to) > buffers_.size()) {
		::PyErr_SetString(PyExc_IndexError, "The specified index is out of range.");
		py::throw_error_already_set();
	} else if(from == to)
		return;

	// リスト内で移動
	Buffer* buffer = buffers_[from];
	buffers_.erase(buffers_.begin() + from);
	buffers_.insert((from < to) ? buffers_.begin() + to - 1 : buffers_.begin() + to, buffer);

	// バッファバーのボタンを並び替え
	const int end = min(static_cast<int>(max(from, to)), bufferBar_.getButtonCount() - 1);
	for(int i = static_cast<int>(min(from, to)); i <= end; ++i) {
		bufferBar_.setCommandID(i, i);
		bufferBar_.setButtonText(i, getDisplayName(*buffers_[i]).c_str());
	}
	EditorWindows::instance().activePane().showBuffer(*buffer);
	resetResources();
}

namespace {
	basic_string<WCHAR> resolveShortcut(const basic_string<WCHAR>& s) {
		const WCHAR* extension = ::PathFindExtensionW(s.c_str());
		if(wcslen(extension) != 0 && (
				(::StrCmpIW(extension + 1, L"lnk") == 0)
				/*|| (::StrCmpIW(extension + 1, L"url") == 0)*/)) {
			HRESULT hr;
			com::ComPtr<IShellLinkW> shellLink(CLSID_ShellLink, IID_IShellLinkW, CLSCTX_ALL, 0, &hr);
			if(SUCCEEDED(hr)) {
				com::ComPtr<IPersistFile> file;
				if(SUCCEEDED(hr = shellLink->QueryInterface(IID_IPersistFile, file.initializePPV()))) {
					if(SUCCEEDED(hr = file->Load(s.c_str(), STGM_READ))) {
						if(SUCCEEDED(hr = shellLink->Resolve(0, SLR_ANY_MATCH | SLR_NO_UI))) {
							WCHAR resolved[MAX_PATH];
							if(SUCCEEDED(hr = shellLink->GetPath(resolved, MAX_PATH, 0, 0)))
								return resolved;
						}
					}
				}
			}
			throw com::ComException(hr, IID_NULL, OLESTR("@0.resolveShortcut"));
		} else
			return f::canonicalizePathName(s.c_str());
	}
}
#if 0
/**
 * Opens the specified file.
 * This method may show a dialog to indicate the result.
 * @param fileName the name of the file to open
 * @param encoding the encoding. auto-detect if omitted
 * @param lockMode the lock mode
 * @param asReadOnly set true to open as read only
 * @return the opened buffer or @c None if failed
 */
py::object BufferList::open(const basic_string<WCHAR>& fileName,
		const string& encoding /* = "UniversalAutoDetect" */,
		e::Encoder::SubstitutionPolicy encodingSubstitutionPolicy,
		f::TextFileDocumentInput::LockType lockMode /* = DONT_LOCK */, bool asReadOnly /* = false */) {
	// TODO: this method is too complex.
	Alpha& app = Alpha::instance();

	// resolve shortcut
	basic_string<WCHAR> resolvedName;
	try {
		resolvedName = resolveShortcut(fileName);
	} catch(com::ComException&) {
		::PyErr_SetObject(PyExc_IOError, convertWideStringToUnicodeObject(
			app.loadMessage(MSG_IO__FAILED_TO_RESOLVE_SHORTCUT, MARGS % fileName)).ptr());
		py::throw_error_already_set();
	}

	// check if the file is already open with other text editor
	const size_t oldBuffer = find(resolvedName);
	if(oldBuffer != -1) {
		EditorWindows::instance().activePane().showBuffer(at(oldBuffer));
		return py::object();
	}

	Buffer* buffer = &EditorWindows::instance().activeBuffer();
	if(buffer->isModified() || buffer->textFile().isBoundToFile()) {	// open in the new container
		if(e::Encoder::supports(encoding))
			buffer = &addNew(L"", encoding);
		else
			buffer = &addNew();
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
	{
		win32::ui::WaitCursor wc;
		app.statusBar().setText(app.loadMessage(MSG_STATUS__LOADING_FILE, MARGS % resolvedName).c_str());
		app.getMainWindow().lockUpdate();

		try {
			// TODO: check the returned value.
			buffer->textFile().open(resolvedName, lockMode, encoding, encodingSubstitutionPolicy);
		} catch(const f::FileNotFoundException& e) {
			::PyErr_SetString(PyExc_IOError, e.what());
		} catch(const f::AccessDeniedException& e) {
			::PyErr_SetString(PyExc_IOError, e.what());
		} catch(const f::UnmappableCharacterException& e) {
			::PyErr_SetString(PyExc_UnicodeDecodeError, e.what());
		} catch(const f::MalformedInputException& e) {
			::PyErr_SetString(PyExc_UnicodeDecodeError, e.what());
		} catch(const f::PlatformDependentIOError&) {
			::PyErr_SetFromWindowsErr(0);
		} catch(f::IOException& e) {
			::PyErr_SetObject(PyExc_IOError, py::object(e.type()).ptr());
		}
		app.statusBar().setText(0);
		app.getMainWindow().unlockUpdate();
	}
	app.getMainWindow().show(app.getMainWindow().isVisible() ? SW_SHOW : SW_RESTORE);

	if(::PyErr_Occurred() != 0)
		py::throw_error_already_set();

	if(asReadOnly)
		buffer->setReadOnly();
	return buffers_.back()->self();
}
#endif
/// @see ascension#text#IUnexpectedFileTimeStampDirector::queryAboutUnexpectedTimeStamp
bool BufferList::queryAboutUnexpectedDocumentFileTimeStamp(
		k::Document& document, IUnexpectedFileTimeStampDirector::Context context) throw() {
	if(unexpectedFileTimeStampDirector == py::object())
		return false;
	try {
		py::object f(unexpectedFileTimeStampDirector.attr("query_about_unexpected_time_stamp"));
		const py::object result(f(getConcreteDocument(document), context));
		return ::PyObject_IsTrue(result.ptr()) == 1;
	} catch(py::error_already_set&) {
		Interpreter::instance().handleException();
		return false;
	}
}

/// Recalculates the size of the buffer bar.
void BufferList::recalculateBufferBarSize() {
	bufferBarPager_.recalcSize();

	// バッファバーの理想長さの再計算
	if(bufferBar_.isVisible()) {
		win32::AutoZero<REBARBANDINFOW> rbbi;
		win32::Borrowed<win32::ui::Rebar> rebar(bufferBarPager_.getParent()->get());
		RECT rect;
		rbbi.fMask = RBBIM_IDEALSIZE;
		bufferBar_.getItemRect(bufferBar_.getButtonCount() - 1, rect);
		rbbi.cxIdeal = rect.right;
		rebar->setBandInfo(rebar->idToIndex(IDC_BUFFERBAR), rbbi);
	}
}

#if 0
/**
 * Reopens the specified buffer.
 * @param index the index of the buffer to reopen
 * @param changeEncoding set true to change the encoding
 * @return the result. see the description of @c BufferList#OpenResult
 * @throw std#out_of_range @a index is invalid
 */
BufferList::OpenResult BufferList::reopen(size_t index, bool changeEncoding) {
	using namespace ascension::kernel::fileio;
	Alpha& app = Alpha::instance();
	Buffer& buffer = at(index);

	// ファイルが存在するか?
	if(!buffer.textFile().isOpen())
		return OPENRESULT_FAILED;

	// ユーザがキャンセル
	else if(buffer.isModified() && IDNO == app.messageBox(MSG_BUFFER__CONFIRM_REOPEN_EVEN_IF_DIRTY, MB_YESNO | MB_ICONQUESTION))
		return OPENRESULT_USERCANCELED;

	// エンコードを変更する場合はダイアログを出す
	string encoding(buffer.textFile().encoding());
	if(changeEncoding) {
		ui::EncodingsDialog dlg(encoding, true);
		if(dlg.doModal(app.getMainWindow()) != IDOK)
			return OPENRESULT_USERCANCELED;
		encoding = dlg.resultEncoding();
	}

	bool succeeded = true;
	IOException::Type errorType;
	while(true) {
		using namespace ascension::encoding;
		try {
			buffer.textFile().open(buffer.textFile().pathName(), buffer.textFile().lockMode(), encoding, Encoder::DONT_SUBSTITUTE);
		} catch(IOException& e) {
			succeeded = false;
			errorType = e.type();
		}
		if(!succeeded) {
			// alert the encoding error
			int userAnswer;
			if(errorType == IOException::UNMAPPABLE_CHARACTER)
				userAnswer = app.messageBox(MSG_IO__UNCONVERTABLE_NATIVE_CHAR, MB_YESNOCANCEL | MB_ICONEXCLAMATION,
					MARGS % buffer.textFile().pathName() % Encoder::forMIB(fundamental::US_ASCII)->toUnicode(encoding).c_str());
			else if(errorType == IOException::MALFORMED_INPUT)
				userAnswer = app.messageBox(MSG_IO__MALFORMED_INPUT_FILE, MB_OKCANCEL | MB_ICONEXCLAMATION,
					MARGS % buffer.textFile().pathName() % Encoder::forMIB(fundamental::US_ASCII)->toUnicode(encoding).c_str());
			else
				break;
			succeeded = true;
			if(userAnswer == IDYES || userAnswer == IDOK) {
				// the user want to change the encoding
				ui::EncodingsDialog dlg(encoding, true);
				if(dlg.doModal(app.getMainWindow()) != IDOK)
					return OPENRESULT_USERCANCELED;
				encoding = dlg.resultEncoding();
				continue;
			} else if(userAnswer == IDNO) {
				succeeded = true;
				try {
					buffer.textFile().open(buffer.textFile().pathName(),
						buffer.textFile().lockMode(), encoding, Encoder::REPLACE_UNMAPPABLE_CHARACTERS);
				} catch(IOException& e) {
					succeeded = false;
					if((errorType = e.type()) == IOException::MALFORMED_INPUT) {
						app.messageBox(MSG_IO__MALFORMED_INPUT_FILE, MB_OK | MB_ICONEXCLAMATION,
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
//		app.mruManager().add(buffer.textFile().pathName());
		return OPENRESULT_SUCCEEDED;
	} else
		return OPENRESULT_FAILED;
}
#endif

/// Reconstructs the image list and the menu according to the current buffer list.
void BufferList::resetResources() {
	if(icons_.get() != 0) {
		const int c = icons_.getNumberOfImages();
		for(int i = 0; i < c; ++i)
			::DestroyIcon(icons_.getIcon(i, ILD_NORMAL));
		icons_.destroy();
	}
	if(buffers_.empty())
		return;
	icons_ = win32::ui::ImageList::create(::GetSystemMetrics(SM_CXSMICON),
		::GetSystemMetrics(SM_CYSMICON), ILC_COLOR32 | ILC_MASK, 0, static_cast<int>(buffers_.size()));
	while(listMenu_.getNumberOfItems() != 0)
		listMenu_.erase<win32::ui::Menu::BY_POSITION>(0);

	SHFILEINFOW sfi;
	for(size_t i = 0; i < buffers_.size(); ++i) {
		::SHGetFileInfoW(
			(buffers_[i]->textFile().isBoundToFile()) ? buffers_[i]->textFile().fileName().c_str() : L"",
			0, &sfi, sizeof(SHFILEINFOW), SHGFI_ICON | SHGFI_SMALLICON);
		icons_.add(sfi.hIcon);
		listMenu_ << win32::ui::Menu::OwnerDrawnItem(static_cast<UINT>(i));
	}
	bufferBar_.setImageList(icons_.use());
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
#if 0
	Alpha& app = Alpha::instance();
	Buffer& buffer = at(index);

	// 保存の必要があるか?
	if(overwrite && buffer.textFile().isOpen() && !buffer.isModified())
		return true;

	WCHAR fileName[MAX_PATH + 1];
	TextFileFormat format = {buffer.textFile().encoding(), NLF_RAW_VALUE};
	bool newName = false;

	// 別名で保存 or ファイルが存在しない
	if(!overwrite || !buffer.textFile().isOpen() || !toBoolean(::PathFileExistsW(buffer.textFile().pathName().c_str()))) {
		win32::AutoZero<OSVERSIONINFOW> osVersion;
		const wstring filterSource(app.loadMessage(MSG_DIALOG__SAVE_FILE_FILTER));
		wchar_t* const filter = new wchar_t[filterSource.length() + 6];

		osVersion.dwOSVersionInfoSize = sizeof(::OSVERSIONINFOW);
		::GetVersionExW(&osVersion);
		filterSource.copy(filter, filterSource.length());
		wcsncpy(filter + filterSource.length(), L"\0*.*\0\0", 6);
		wcscpy(fileName, (buffer.textFile().pathName().c_str()));

		win32::AutoZeroSize<OPENFILENAMEW> newOfn;
		win32::AutoZeroSize<OPENFILENAME_NT4W> oldOfn;
		OPENFILENAMEW& ofn = (osVersion.dwMajorVersion > 4) ? newOfn : *reinterpret_cast<OPENFILENAMEW*>(&oldOfn);
		ofn.hwndOwner = app.getMainWindow().use();
		ofn.hInstance = ::GetModuleHandle(0);
		ofn.lpstrFilter = filter;
		ofn.lpstrFile = fileName;
		ofn.nMaxFile = MAX_PATH;
		ofn.Flags = OFN_ENABLEHOOK | OFN_ENABLESIZING | OFN_ENABLETEMPLATE
			| OFN_EXPLORER | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT /* | OFN_SHOWHELP*/;
		ofn.lCustData = reinterpret_cast<LPARAM>(&format);
		ofn.lpfnHook = openFileNameHookProc;
		ofn.lpTemplateName = MAKEINTRESOURCEW(IDD_DLG_SAVEFILE);

		bool succeeded = toBoolean(::GetSaveFileNameW(&ofn));
//		DWORD n = ::CommDlgExtendedError();
		delete[] filter;
		if(!succeeded)
			return false;

		// 既に開かれているファイルに上書きしようとしていないか?
		const size_t existing = find(fileName);
		if(existing != -1 && existing != index) {
			app.messageBox(MSG_BUFFER__SAVING_FILE_IS_OPENED, MB_ICONEXCLAMATION | MB_OK, MARGS % fileName);
			return false;
		}
		newName = true;
	} else
		wcscpy(fileName, buffer.textFile().pathName().c_str());

	using namespace ascension::encoding;
	MIBenum encodingMIB = MIB_UNKNOWN;
	{
		const auto_ptr<Encoder> temp(Encoder::forName(format.encoding));
		if(temp.get() != 0)
			encodingMIB = temp->properties().mibEnum();
	}
	const bool writeBOM =
		(encodingMIB == fundamental::UTF_8 && toBoolean(app.readIntegerProfile(L"File", L"writeBOMAsUTF8", 0)))
		|| (encodingMIB == fundamental::UTF_16LE && toBoolean(app.readIntegerProfile(L"File", L"writeBOMAsUTF16LE", 1)))
		|| (encodingMIB == fundamental::UTF_16BE && toBoolean(app.readIntegerProfile(L"File", L"writeBOMAsUTF16BE", 1)))
		|| (encodingMIB == fundamental::UTF_16 && toBoolean(app.readIntegerProfile(L"File", L"writeBOMAsUTF16", 1)))
		|| (encodingMIB == standard::UTF_32 && toBoolean(app.readIntegerProfile(L"File", L"writeBOMAsUTF32", 1)))
		|| (encodingMIB == standard::UTF_32LE && toBoolean(app.readIntegerProfile(L"File", L"writeBOMAsUTF32LE", 1)))
		|| (encodingMIB == standard::UTF_32BE && toBoolean(app.readIntegerProfile(L"File", L"writeBOMAsUTF32BE", 1)));

	using namespace kernel::fileio;
	IOException::Type errorType;
	bool succeeded = true;
	while(true) {
		TextFileDocumentInput::WriteParameters params;
		params.encoding = format.encoding;
		params.encodingSubstitutionPolicy = Encoder::DONT_SUBSTITUTE;
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
			const int userAnswer = app.messageBox(
				MSG_IO__UNCONVERTABLE_UCS_CHAR, MB_YESNOCANCEL | MB_ICONEXCLAMATION,
				MARGS % fileName % Encoder::forMIB(fundamental::US_ASCII)->toUnicode(params.encoding).c_str());
			if(userAnswer == IDYES) {
				// the user want to change the encoding
				ui::EncodingsDialog dlg(params.encoding, false);
				if(dlg.doModal(app.getMainWindow()) != IDOK)
					return false;
				params.encoding = dlg.resultEncoding();
				continue;
			} else if(userAnswer == IDNO) {
				succeeded = true;
				params.encodingSubstitutionPolicy = Encoder::REPLACE_UNMAPPABLE_CHARACTER;
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
//	if(succeeded && addToMRU && newName)
//		app_.mruManager().add(fileName);
	return succeeded;
#endif
	return false;
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
 */
bool BufferList::saveSomeDialog(py::tuple buffersToSave /* = py::tuple() */) {
	ui::SaveSomeBuffersDialog dialog;
	for(size_t i = 0, c = buffers_.size(); i < c; ++i) {
		ui::DirtyFile df;
		df.fileName = buffers_[i]->name();
		df.save = true;
		dialog.files_.push_back(df);
	}
	if(IDOK != dialog.doModal(Alpha::instance().getMainWindow()))
		return false;

	// save the checked buffers
	for(vector<ui::DirtyFile>::reverse_iterator it(dialog.files_.rbegin()); it != dialog.files_.rend(); ++it) {
		if(it->save) {
			py::object buffer(forFileName(it->fileName));
			if(buffer != py::object()) {
				py::extract<Buffer&> temp(buffer);
				if(temp.check())
					saveBuffer(static_cast<Buffer&>(temp));
			}
		}
	}
	return true;
}

/// @see ascension#presentation#ITextViewerListListener#textViewerListChanged
void BufferList::textViewerListChanged(a::presentation::Presentation& presentation) {
}

/// Reconstructs the context menu.
void BufferList::updateContextMenu() {
	while(contextMenu_.getNumberOfItems() > 0)
		contextMenu_.erase<win32::ui::Menu::BY_POSITION>(0);
//	contextMenu_ << Menu::StringItem(CMD_FILE_CLOSE, app_.commandManager().menuName(CMD_FILE_CLOSE).c_str())
//		<< Menu::StringItem(CMD_FILE_CLOSEOTHERS, app_.commandManager().menuName(CMD_FILE_CLOSEOTHERS).c_str());
//	contextMenu_.setDefault<Menu::BY_COMMAND>(CMD_FILE_CLOSE);
}

void BufferList::updateTitleBar() {
	win32::ui::Window& mainWindow = Alpha::instance().getMainWindow();
	if(mainWindow.isWindow()) {
		// show the display name of the active buffer and application credit
		static wstring titleCache;
		wstring title(getDisplayName(EditorWindows::instance().activePane().visibleBuffer()));
		if(title != titleCache) {
			titleCache = title;
//			title += L" - " IDS_APPFULLVERSION;
			title += L" - " IDS_APPNAME;
			mainWindow.setText(title.c_str());
		}
	}
}


namespace {
	py::object activeBuffer() {return EditorWindows::instance().activeBuffer().self();}
	void bindBufferToFile(Buffer& buffer, const wstring& fileName) {buffer.textFile().bind(fileName);}
	py::object bufferAt(const BufferList& buffers, py::ssize_t at) {
		try {
			return buffers.at(at).self();
		} catch(const out_of_range& e) {
			::PyErr_SetString(PyExc_IndexError, e.what());
			py::throw_error_already_set();
		}
		return py::object();
	}
	py::object buffers() {return BufferList::instance().self();}
	void closeBuffer(Buffer& buffer) {BufferList::instance().close(buffer);}
	string encodingOfBuffer(const Buffer& buffer) {return buffer.textFile().encoding();}
	k::Position insertString(Buffer& buffer, const k::Position& at, const a::String& text) {k::Position temp; k::insert(buffer, at, text, &temp); return temp;}
	bool isBufferActive(const Buffer& buffer) {return &buffer == &EditorWindows::instance().activeBuffer();}
	bool isBufferBoundToFile(const Buffer& buffer) {return buffer.textFile().isBoundToFile();}
	void lockFile(Buffer& buffer, f::TextFileDocumentInput::LockType type, bool onlyAsEditing) {
		f::TextFileDocumentInput::LockMode mode; mode.type = type; mode.onlyAsEditing = onlyAsEditing; buffer.textFile().lockFile(mode);}
	k::Newline newlineOfBuffer(const Buffer& buffer) {return buffer.textFile().newline();}
	k::Position replaceString(Buffer& buffer, const k::Region& region, const a::String& text) {k::Position temp; replace(buffer, region, text, &temp); return temp;}
	void revertBufferToFile(Buffer& buffer, const string& encoding,
		e::Encoder::SubstitutionPolicy encodingSubstitutionPolicy) {buffer.textFile().revert(encoding, encodingSubstitutionPolicy);}
	void setEncodingOfBuffer(Buffer& buffer, const string& encoding) {buffer.textFile().setEncoding(encoding);}
	void setNewlineOfBuffer(Buffer& buffer, k::Newline newline) {buffer.textFile().setNewline(newline);}
	void unbindBufferFromFile(Buffer& buffer) {buffer.textFile().unbind();}
	void unlockFile(Buffer& buffer) {buffer.textFile().unlockFile();}
	bool unicodeByteOrderMarkOfBuffer(const Buffer& buffer) {return buffer.textFile().unicodeByteOrderMark();}
	void writeBufferRegion(const Buffer& buffer, const k::Region& region, const wstring& fileName, bool append,
			const string& encoding, k::Newline newlines, e::Encoder::SubstitutionPolicy encodingSubstitutionPolicy, bool writeUnicodeByteOrderMark) {
		f::WritingFormat format;
		format.encoding = encoding;
		format.newline = newlines;
		format.encodingSubstitutionPolicy = encodingSubstitutionPolicy;
		format.unicodeByteOrderMark = writeUnicodeByteOrderMark;
		try {
			f::writeRegion(buffer, region, fileName, format, append);
		} catch(const f::UnmappableCharacterException& e) {
			::PyErr_SetString(PyExc_UnicodeDecodeError, e.what());
		} catch(const f::IOException& e) {
			::PyErr_SetFromWindowsErr(e.code());
		}
	}
}

ALPHA_EXPOSE_PROLOGUE(1)
	using namespace ascension;
	using namespace ascension::kernel;
	py::scope temp(Interpreter::instance().toplevelPackage());

	py::enum_<locations::CharacterUnit>("CharacterUnit")
		.value("utf16_code_unit", locations::UTF16_CODE_UNIT)
		.value("utf32_code_unit", locations::UTF32_CODE_UNIT)
		.value("grapheme_cluster", locations::GRAPHEME_CLUSTER)
		.value("glyph_cluster", locations::GLYPH_CLUSTER);
	py::enum_<e::Encoder::SubstitutionPolicy>("EncodingSubstitutionPolicy")
		.value("dont_substitute", e::Encoder::DONT_SUBSTITUTE)
		.value("replace_unmappable_characters", e::Encoder::REPLACE_UNMAPPABLE_CHARACTERS)
		.value("ignore_unmappable_characters", e::Encoder::IGNORE_UNMAPPABLE_CHARACTERS);
/*	py::enum_<f::IOException::Type>("FileIoError")
		.value("ok", static_cast<f::IOException::Type>(-1))
		.value("file_not_found", f::IOException::FILE_NOT_FOUND)
		.value("invalid_encoding", f::IOException::INVALID_ENCODING)
		.value("invalid_newline", f::IOException::INVALID_NEWLINE)
		.value("unmappable_character", f::IOException::UNMAPPABLE_CHARACTER)
		.value("malformed_input", f::IOException::MALFORMED_INPUT)
//		.value("out_of_memory", f::IOException::OUT_OF_MEMORY)
		.value("huge_file", f::IOException::HUGE_FILE)
//		.value("read_only_mode", f::IOException::READ_ONLY_MODE)
		.value("unwritable_file", f::IOException::UNWRITABLE_FILE)
		.value("cannot_create_temporary_file", f::IOException::CANNOT_CREATE_TEMPORARY_FILE)
		.value("lost_disk_file", f::IOException::LOST_DISK_FILE)
		.value("platform_dependent_error", f::IOException::PLATFORM_DEPENDENT_ERROR);
*/	py::enum_<f::TextFileDocumentInput::LockType>("FileLockMode")
		.value("no_lock", f::TextFileDocumentInput::NO_LOCK)
		.value("shared_lock", f::TextFileDocumentInput::SHARED_LOCK)
		.value("exclusive_lock", f::TextFileDocumentInput::EXCLUSIVE_LOCK);
	py::enum_<Newline>("Newline")
		.value("line_feed", NLF_LINE_FEED)
		.value("carriage_return", NLF_CARRIAGE_RETURN)
		.value("cr_lf", NLF_CR_LF)
		.value("next_line", NLF_NEXT_LINE)
		.value("line_separator", NLF_LINE_SEPARATOR)
		.value("paragraph_separator", NLF_PARAGRAPH_SEPARATOR)
		.value("raw_value", NLF_RAW_VALUE)
		.value("document_input", NLF_DOCUMENT_INPUT);
	py::enum_<f::IUnexpectedFileTimeStampDirector::Context>("UnexpectedFileTimeStampContext")
		.value("first_modification", f::IUnexpectedFileTimeStampDirector::FIRST_MODIFICATION)
		.value("overwrite_file", f::IUnexpectedFileTimeStampDirector::OVERWRITE_FILE)
		.value("client_invocation", f::IUnexpectedFileTimeStampDirector::CLIENT_INVOCATION);

	py::class_<Direction>("Direction", py::no_init)
		.def_readonly("forward", &Direction::FORWARD)
		.def_readonly("backward", &Direction::BACKWARD);
	py::class_<Position>("Position", py::init<>())
		.def(py::init<length_t, length_t>())
		.def_readwrite("line", &Position::line)
		.def_readwrite("column", &Position::column);
	py::class_<Region>("Region", py::init<>())
		.def(py::init<const Position&, const Position&>())
		.def(py::init<const Position&>())
		.def_readwrite("first", &Region::first)
		.def_readwrite("second", &Region::second)
		.def<Position& (Region::*)(void)>("beginning", &Region::beginning, py::return_value_policy<py::reference_existing_object>())
		.def("encompasses", &Region::encompasses)
		.def<Position& (Region::*)(void)>("end", &Region::end, py::return_value_policy<py::reference_existing_object>())
		.def("includes", &Region::includes)
		.def("intersection", &Region::getIntersection, py::return_value_policy<py::return_by_value>())
		.def("intersects_with", &Region::intersectsWith)
		.def("is_empty", &Region::isEmpty)
		.def("is_normalized", &Region::isNormalized)
		.def("normalize", &Region::normalize, py::return_value_policy<py::reference_existing_object>())
		.def("union", &Region::getUnion, py::return_value_policy<py::return_by_value>());
	py::class_<Bookmarker, boost::noncopyable>("_Bookmarker", py::no_init)
		.def("clear", &Bookmarker::clear)
		.def("is_marked", &Bookmarker::isMarked)
		.def("mark", &Bookmarker::mark, (py::arg("line"), py::arg("set") = true))
		.def("next", &Bookmarker::next, (py::arg("from"), py::arg("direction"), py::arg("wrap_around") = true, py::arg("marks") = 1))
		.def("toggle", &Bookmarker::toggle);
	py::class_<Buffer, boost::noncopyable>("_Buffer", py::no_init)
		.add_property("accessible_region", &Buffer::accessibleRegion)
		.add_property("bookmarker", py::make_function<
			Bookmarker& (Buffer::*)(void), py::return_value_policy<py::reference_existing_object>
			>(&Buffer::bookmarker, py::return_value_policy<py::reference_existing_object>()))
		.add_property("encoding", &encodingOfBuffer, &setEncodingOfBuffer)
		.add_property("name", &Buffer::name)
		.add_property("newline", &newlineOfBuffer, &setNewlineOfBuffer)
		.add_property("number_of_lines", &Buffer::numberOfLines)
		.add_property("number_of_redoable_changes", &Buffer::numberOfRedoableChanges)
		.add_property("number_of_undoable_changes", &Buffer::numberOfUndoableChanges)
		.add_property("read_only", &Buffer::isReadOnly, &Buffer::setReadOnly)
		.add_property("records_changes", &Buffer::isRecordingChanges, &Buffer::recordChanges)
		.add_property("region", &Buffer::region)
		.add_property("revision_number", &Buffer::revisionNumber)
		.add_property("unicode_byte_order_mark", &unicodeByteOrderMarkOfBuffer)
		.def("begin_compound_change", &Buffer::beginCompoundChange)
		.def("bind_file", &bindBufferToFile)
		.def("clear_undo_buffer", &Buffer::clearUndoBuffer)
		.def("close", &closeBuffer)
		.def("end_compound_change", &Buffer::endCompoundChange)
		.def<void (*)(Document&, const Region&)>("erase", &erase)
		.def("insert", &insertString)
		.def("insert_file_contents", &f::insertFileContents)
		.def("insert_undo_boundary", &Buffer::insertUndoBoundary)
		.def("is_active", &isBufferActive)
		.def("is_bound_to_file", &isBufferBoundToFile)
		.def("is_compound_changing", &Buffer::isCompoundChanging)
		.def("is_modified", &Buffer::isModified)
		.def("is_narrowed", &Buffer::isNarrowed)
		.def("length", &Buffer::length, py::arg("newline") = NLF_RAW_VALUE)
		.def("line", &Buffer::line, py::return_value_policy<py::copy_const_reference>())
		.def("lock_file", &lockFile)
		.def("mark_unmodified", &Buffer::markUnmodified)
		.def("narrow_to_region", &Buffer::narrowToRegion)
		.def("redo", &Buffer::redo, py::arg("n") = 1)
		.def("replace", &replaceString)
		.def("reset_content", &Buffer::resetContent)
		.def("revert_to_file", &revertBufferToFile)
		.def("unbind_file", &unbindBufferFromFile)
		.def("unlock_file", &unlockFile)
		.def("save", &saveBuffer,
			(py::arg("encoding") = string(), py::arg("newlines") = NLF_RAW_VALUE,
			py::arg("encoding_substitution_policy") = e::Encoder::DONT_SUBSTITUTE,
			py::arg("write_unicode_byte_order_mark") = false))
		.def("undo", &Buffer::undo, py::arg("n") = 1)
		.def("widen", &Buffer::widen)
		.def("write_region", &writeBufferRegion,
			(py::arg("region"), py::arg("filename"), py::arg("append") = false,
			py::arg("encoding") = string(), py::arg("newlines") = NLF_RAW_VALUE,
			py::arg("encoding_substitution_policy") = e::Encoder::DONT_SUBSTITUTE,
			py::arg("write_unicode_byte_order_mark") = false));
	py::class_<BufferList, boost::noncopyable>("_BufferList", py::no_init)
		.def_readwrite("unexpected_file_time_stamp_director", &BufferList::unexpectedFileTimeStampDirector)
//		.def("__contains__", &)
		.def("__getitem__", &bufferAt)
//		.def("__iter__", &)
		.def("__len__", &BufferList::numberOfBuffers)
		.def("add_new", &BufferList::addNew,
			(py::arg("name") = wstring(), py::arg("encoding") = "UTF-8", py::arg("newline") = NLF_RAW_VALUE),
			py::return_value_policy<py::reference_existing_object>())
		.def("add_new_dialog", &BufferList::addNewDialog,
			py::arg("name") = wstring(), py::return_value_policy<py::reference_existing_object>())
//		.def("close_all", &BufferList::closeAll)
		.def("for_filename", &BufferList::forFileName)
		.def("move", &BufferList::move)
//		.def("open", &BufferList::open,
//			(py::arg("filename"), py::arg("encoding") = "UniversalAutoDetect",
//			py::arg("lock_mode") = f::TextFileDocumentInput::DONT_LOCK, py::arg("as_read_only") = false))
		.def("save_all", &BufferList::saveAll);

	py::def("active_buffer", &activeBuffer);
	py::def("buffers", &buffers);
ALPHA_EXPOSE_EPILOGUE()
