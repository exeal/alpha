/**
 * @file buffer.cpp
 * @author exeal
 * @date 2003-2006 (was AlphaDoc.cpp and BufferList.cpp)
 * @date 2006-2008
 */

#include "application.hpp"
#include "buffer.hpp"
#include "editor-window.hpp"
//#include "command.hpp"
//#include "mru-manager.hpp"
#include "new-file-format-dialog.hpp"
#include "save-some-buffers-dialog.hpp"
#include "code-pages-dialog.hpp"
#include "../resource/messages.h"
#include <manah/win32/ui/wait-cursor.hpp>
#include <manah/com/common.hpp>	// ComPtr, ComQIPtr
#include <commdlg.h>
#include <shlwapi.h>				// PathXxxx, StrXxxx, ...
#include <shlobj.h>					// IShellLink, ...
#include <dlgs.h>
using namespace alpha;
//using namespace manah::win32;
using namespace manah::win32::ui;
using namespace manah::com;
using namespace manah::com::ole;
using namespace std;
using manah::toBoolean;
namespace a = ascension;
namespace k = ascension::kernel;

namespace {
	struct TextFileFormat {
		std::string encoding;
		ascension::kernel::Newline newline;
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

namespace {
	class AutoVARIANT : public tagVARIANT {
		MANAH_UNASSIGNABLE_TAG(AutoVARIANT);
	public:
		AutoVARIANT() /*throw()*/ {::VariantInit(this);}
		AutoVARIANT(const AutoVARIANT& other) {::VariantInit(this); ::VariantCopy(this, &other);}
		~AutoVARIANT() throw() {::VariantClear(this);}
		HRESULT coerce(VARTYPE type, USHORT flags) /*throw()*/ {
			AutoVARIANT temp; const HRESULT hr = ::VariantChangeType(&temp, this, flags, type);
			if(FAILED(hr)) return hr; swap(temp); return S_OK;}
		HRESULT coerce(VARTYPE type, USHORT flags, LCID lcid) /*throw()*/ {
			AutoVARIANT temp; const HRESULT hr = ::VariantChangeTypeEx(&temp, this, lcid, flags, type);
			if(FAILED(hr)) return hr; swap(temp); return S_OK;}
		void swap(AutoVARIANT& other) /*throw()*/ {std::swap(static_cast<tagVARIANT&>(*this), static_cast<tagVARIANT&>(other));}
	};

	class IEnumVARIANTStaticImpl : public IUnknownImpl<manah::typelist::Cat<MANAH_INTERFACE_SIGNATURE(IEnumVARIANT)> > {
		MANAH_UNASSIGNABLE_TAG(IEnumVARIANTStaticImpl);
	public:
		IEnumVARIANTStaticImpl(manah::AutoBuffer<VARIANT> array, std::size_t length) : data_(new SharedData) {
			data_->refs = 1;
			data_->array = array.release();
			data_->length = length;
		}
		~IEnumVARIANTStaticImpl() throw() {
			if(--data_->refs == 0) {
				for(size_t i = 0; i < data_->length; ++i)
					::VariantClear(data_->array + i);
				delete[] data_->array;
				delete data_;
			}
		}
		// IEnumVARIANT
		STDMETHODIMP Next(ULONG numberOfElements, VARIANT* values, ULONG* numberOfFetchedElements) {
			MANAH_VERIFY_POINTER(values);
			ULONG fetched;
			for(fetched = 0; fetched < numberOfElements && current_ + fetched < data_->array + data_->length; ++fetched) {
				const HRESULT hr = ::VariantCopy(values + fetched, current_ + fetched);
				if(FAILED(hr)) {
					for(ULONG i = 0; i < fetched; ++i)
						::VariantClear(current_ + i);
					return hr;
				}
			}
			if(numberOfFetchedElements != 0)
				*numberOfFetchedElements = fetched;
			return (fetched == numberOfElements) ? S_OK : S_FALSE;
		}
		STDMETHODIMP Skip(ULONG numberOfElements) {
			const VARIANT* const previous = current_;
			current_ = min(current_ + numberOfElements, data_->array + data_->length);
			return (current_ - previous == numberOfElements) ? S_OK : S_FALSE;
		}
		STDMETHODIMP Reset() {
			current_ = data_->array;
			return S_OK;
		}
		STDMETHODIMP Clone(IEnumVARIANT** enumerator) {
			MANAH_VERIFY_POINTER(enumerator);
			if(*enumerator = new(nothrow) IEnumVARIANTStaticImpl(*this))
				return S_OK;
			return E_OUTOFMEMORY;
		}
	private:
		IEnumVARIANTStaticImpl(const IEnumVARIANTStaticImpl& rhs) : data_(rhs.data_), current_(rhs.current_) {
			++data_->refs;
		}
		struct SharedData {
			std::size_t refs;
			VARIANT* array;
			std::size_t length;
		} * data_;
		VARIANT* current_;
	};

#define AMBIENT_CHECK_PROXY() if(!check()) return CO_E_OBJNOTCONNECTED

	class AutomationPosition : public k::Position, public ambient::SingleAutomationObject<IPosition, &IID_IPosition> {
	public:
		explicit AutomationPosition(const k::Position& position);
		// IPosition
		STDMETHODIMP get_Column(long* column);
		STDMETHODIMP get_Line(long* line);
		STDMETHODIMP put_Column(long column);
		STDMETHODIMP put_Line(long line);
	};

	class AutomationRegion : public k::Region, public ambient::SingleAutomationObject<IRegion, &IID_IRegion> {
	public:
		explicit AutomationRegion(const k::Region& region);
		// IRegion
		STDMETHODIMP get_Beginning(IPosition** beginning);
		STDMETHODIMP get_End(IPosition** end);
		STDMETHODIMP Encompasses(IRegion* other, VARIANT_BOOL* result);
		STDMETHODIMP GetIntersection(IRegion* other, IRegion** result);
		STDMETHODIMP GetUnion(IRegion* other, IRegion** result);
		STDMETHODIMP Includes(IPosition* p, VARIANT_BOOL* result);
		STDMETHODIMP IntersectsWith(IRegion* other, VARIANT_BOOL* result);
		STDMETHODIMP IsEmpty(VARIANT_BOOL* result);
	};
/*
	template<typename Implementation>
	class PointProxyBase : public AutomationProxy<Implementation, ICaret> {
	public:
		explicit PointProxyBase(Implementation& impl);
		// IPoint
		STDMETHODIMP get_AdaptsToBuffer(VARIANT_BOOL* adapts);
		STDMETHODIMP get_Buffer(IBuffer** buffer);
		STDMETHODIMP get_Column(long* column);
		STDMETHODIMP get_ExcludedFromRestriction(VARIANT_BOOL* excluded);
		STDMETHODIMP get_Gravity(Direction *gravity);
		STDMETHODIMP get_Position(IPosition** position);
		STDMETHODIMP put_AdaptsToBuffer(VARIANT_BOOL adapt);
		STDMETHODIMP put_ExcludedFromRestriction(VARIANT_BOOL excluded);
		STDMETHODIMP put_Gravity(Direction gravity);
		STDMETHODIMP IsBufferDeleted(VARIANT_BOOL* result);
		STDMETHODIMP MoveTo(IPosition* to);
	};

	class PointProxy : public AutomationObject<&IID_IPoint, InterfaceEntry<IPoint, &IID_IPoint>, PointProxyBase> {};
*/
	class BookmarkerProxy : public ambient::AutomationProxy<k::Bookmarker, ambient::SingleAutomationObject<IBookmarker, &IID_IBookmarker> > {
	public:
		explicit BookmarkerProxy(k::Bookmarker& impl);
		STDMETHODIMP Clear();
		STDMETHODIMP IsMarked(long line, VARIANT_BOOL* result);
		STDMETHODIMP Mark(long line, VARIANT_BOOL set);
		STDMETHODIMP Next(long from, Direction direction, VARIANT_BOOL wrapAround, long marks, long* result);
		STDMETHODIMP Toggle(long line);
	};

	class BufferProxy : public ambient::AutomationProxy<Buffer, ambient::SingleAutomationObject<IBuffer, &IID_IBuffer> > {
	public:
		explicit BufferProxy(Buffer& impl);
		// IBuffer
		STDMETHODIMP get_AccessibleRegion(IRegion** region);
		STDMETHODIMP get_Bookmarker(IBookmarker** bookmarker);
		STDMETHODIMP get_BoundToFile(VARIANT_BOOL* bound);
		STDMETHODIMP get_Encoding(BSTR* encoding);
		STDMETHODIMP get_InCompoundChanging(VARIANT_BOOL* compound);
		STDMETHODIMP get_Length(Newline newline, long* length);
		STDMETHODIMP get_Line(long line, BSTR* s);
		STDMETHODIMP get_Modified(VARIANT_BOOL* modified);
		STDMETHODIMP get_Name(BSTR* name);
		STDMETHODIMP get_Narrowed(VARIANT_BOOL* narrowed);
		STDMETHODIMP get_Newline(Newline* newline);
		STDMETHODIMP get_ReadOnly(VARIANT_BOOL* readOnly);
		STDMETHODIMP get_RecordsChanges(VARIANT_BOOL* records);
		STDMETHODIMP get_Region(IRegion** region);
		STDMETHODIMP get_RevisionNumber(long* revisionNumber);
		STDMETHODIMP get_UnicodeByteOrderMark(VARIANT_BOOL* p);
		STDMETHODIMP put_Encoding(BSTR encoding);
		STDMETHODIMP put_Newline(Newline newline);
		STDMETHODIMP put_ReadOnly(VARIANT_BOOL readOnly);
		STDMETHODIMP put_RecordsChanges(VARIANT_BOOL record);
		STDMETHODIMP BeginCompoundChange();
		STDMETHODIMP ClearUndoBuffer();
		STDMETHODIMP Close();
		STDMETHODIMP EndCompoundChange();
		STDMETHODIMP Erase(IRegion* region, VARIANT_BOOL* result);
		STDMETHODIMP Insert(IPosition* position, BSTR text, VARIANT_BOOL* result);
		STDMETHODIMP InsertUndoBoundary();
		STDMETHODIMP MarkUnmodified();
		STDMETHODIMP NarrowToRegion(IRegion* region);
		STDMETHODIMP Redo(long n, VARIANT_BOOL* result);
		STDMETHODIMP ResetContent();
		STDMETHODIMP Undo(long n, VARIANT_BOOL* result);
		STDMETHODIMP Widen();
	};

	class BufferListProxy : public ambient::AutomationProxy<BufferList, ambient::SingleAutomationObject<IBufferList, &IID_IBufferList> > {
	public:
		explicit BufferListProxy(BufferList& impl);
		// IBufferList
		STDMETHODIMP get__NewEnum(IUnknown** enumerator);
		STDMETHODIMP get_Item(long index, IBuffer** buffer);
		STDMETHODIMP get_Length(long* length);
		STDMETHODIMP AddNew(BSTR name, BSTR encoding, Newline newline, IBuffer** result);
		STDMETHODIMP AddNewDialog(BSTR name, IBuffer** result);
		STDMETHODIMP Open(BSTR fileName, BSTR encoding, FileLockMode lockMode, VARIANT_BOOL asReadOnly, IBuffer** result);
		STDMETHODIMP OpenDialog(BSTR initialDirectory, VARIANT_BOOL* succeeded);
		STDMETHODIMP SaveSomeDialog(VARIANT_BOOL* ok);
	};
} // namespace @0


// Buffer ///////////////////////////////////////////////////////////////////

/// Constructor.
Buffer::Buffer() {
//	typedef manah::typelist::Cat<MANAH_INTERFACE_SIGNATURE(IDispatch), manah::typelist::Cat<MANAH_INTERFACE_SIGNATURE(IPosition)> > T;
//	typedef ImplementsAll<InterfacesFromSignatures<T>::Result> U;
//	IUnknownImpl<U>* p1;
//	ambient::IUnknownDispatchImpl<manah::typelist::Cat<MANAH_INTERFACE_SIGNATURE(IPosition)> >* p2;
//	static_cast<IPosition*>(p1);
//	static_cast<IDispatch*>(p1);
//	static_cast<IPosition*>(p2);
//	static_cast<IDispatch*>(p2);
	self_.reset(new BufferProxy(*this));
	presentation_.reset(new a::presentation::Presentation(*this));
	textFile_.reset(new k::fileio::TextFileDocumentInput(*this));
}

/// Destructor.
Buffer::~Buffer() {
}

/**
 * Returns the name of the buffer.
 * @see BufferList#getDisplayName
 */
const basic_string<WCHAR> Buffer::name() const throw() {
	static const wstring untitled(Alpha::instance().loadMessage(MSG_BUFFER__UNTITLED));
	if(textFile_->isOpen())
		return textFile_->name();
	return untitled.c_str();
}

/// Returns the presentation object of Ascension.
a::presentation::Presentation& Buffer::presentation() throw() {
	return *presentation_;
}

/// Returns the presentation object of Ascension.
const a::presentation::Presentation& Buffer::presentation() const throw() {
	return *presentation_;
}


// BufferList ///////////////////////////////////////////////////////////////

/// Default constructor.
BufferList::BufferList() {
	self_.reset(new BufferListProxy(*this));

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
	if(icons_.isImageList()) {
		const int c = icons_.getNumberOfImages();
		for(int i = 0; i < c; ++i)
			::DestroyIcon(icons_.getIcon(i, ILD_NORMAL));
		icons_.destroy();
	}
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
	if((newline & k::NLF_SPECIAL_VALUE_MASK) == 0)
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
		view->create(EditorWindows::instance().getHandle(), DefaultWindowRect(),
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
	MANAH_AUTO_STRUCT(TBBUTTON, button);
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
	using namespace a::encoding;
	Alpha& app = Alpha::instance();
	TextFileFormat format;
	format.encoding = Encoder::forMIB(fundamental::US_ASCII)->fromUnicode(app.readStringProfile(L"File", L"defaultEncoding"));
	if(!Encoder::supports(format.encoding))
		format.encoding = Encoder::getDefault().properties().name();
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
		buffer.textFile().close();
		delete &buffer;

		// reset the following buffer bar buttons
		for(size_t i = index, c = buffers_.size(); i < c; ++i) {
			bufferBar_.setCommandID(static_cast<int>(i), static_cast<WORD>(i));
			bufferBar_.setButtonText(static_cast<int>(i), getDisplayName(*buffers_[i]).c_str());
		}
		resetResources();
		recalculateBufferBarSize();
		fireActiveBufferSwitched();
	} else {	// the buffer is last one
		buffer.textFile().close();
		buffer.resetContent();
	}
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
	HWND toolTips = bufferBar_.getToolTips();
	bufferBar_.setButtonStructSize();
	::SetWindowLongPtrW(toolTips, GWL_STYLE, ::GetWindowLongPtrW(toolTips, GWL_STYLE) | TTS_NOPREFIX);
	bufferBarPager_.setChild(bufferBar_.getHandle());

	// レバーに乗せる
	MANAH_AUTO_STRUCT_SIZE(REBARBANDINFOW, rbbi);
	const wstring caption = Alpha::instance().loadMessage(MSG_DIALOG__BUFFERBAR_CAPTION);
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
void BufferList::documentAccessibleRegionChanged(const k::Document&) {
	// do nothing
}

/// @see ascension#text#IDocumentStateListener#documentModificationSignChanged
void BufferList::documentModificationSignChanged(const k::Document& document) {
	const Buffer& buffer = getConcreteDocument(document);
	bufferBar_.setButtonText(static_cast<int>(find(buffer)), getDisplayName(buffer).c_str());
	recalculateBufferBarSize();
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
}

/// @see ascension#kernel#fileio#IFilePropertyListener#fileNameChanged
void BufferList::fileNameChanged(const k::fileio::TextFileDocumentInput& textFile) {
	const Buffer& buffer = getConcreteDocument(textFile.document());
	// TODO: call mode-application.
	resetResources();
	bufferBar_.setButtonText(static_cast<int>(find(buffer)), getDisplayName(buffer).c_str());
	bufferBarPager_.recalcSize();
}

/// @see ascension#kernel#fileio#IFilePropertyListener#fileEncodingChanged
void BufferList::fileEncodingChanged(const k::fileio::TextFileDocumentInput& textFile) {
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
size_t BufferList::find(const basic_string<WCHAR>& fileName) const {
	for(size_t i = 0; i < buffers_.size(); ++i) {
		if(buffers_[i]->textFile().isOpen()
				&& k::fileio::comparePathNames(buffers_[i]->textFile().pathName().c_str(), fileName.c_str()))
			return i;
	}
	return -1;
}

/// Invokes @c IActiveBufferListener#activeBufferSwitched.
void BufferList::fireActiveBufferSwitched() {
	const EditorView& view = EditorWindows::instance().activePane().visibleView();
	int activeBufferIndex = -1;

	// find the buffer bar button for the new active buffer
	MANAH_AUTO_STRUCT_SIZE(TBBUTTON, button);
	for(int i = 0, c = bufferBar_.getButtonCount(); i < c; ++i) {
		if(bufferBar_.getButton(i, button) && reinterpret_cast<Buffer*>(button.dwData) == &view.document()) {
			activeBufferIndex = i;
			break;
		}
	}
	assert(activeBufferIndex != -1);

	bufferBar_.checkButton(activeBufferIndex);
	for(EditorWindows::Iterator i(EditorWindows::instance().enumeratePanes()); !i.done(); i.next()) {
		if(i.get().numberOfViews() > 0 && &i.get().visibleView() == &view) {
			EditorWindows::instance().setDefaultActivePane(i.get());
			break;
		}
	}

	// アクティブなバッファのボタンが隠れていたらスクロールする
	if(bufferBarPager_.isVisible()) {
		const int pagerPos = bufferBarPager_.getPosition();
		RECT buttonRect, pagerRect;
		bufferBar_.getItemRect(activeBufferIndex, buttonRect);
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
			setActive(mouse.dwItemSpec);
			contextMenu_.trackPopup(TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON, pt.x, pt.y, Alpha::instance().getMainWindow().getHandle());
			return true;
		}
	}

	else if(nmhdr.hdr.code == TTN_GETDISPINFOW) {	// show a tooltip
		assert(static_cast<int>(nmhdr.hdr.idFrom) < bufferBar_.getButtonCount());
		static wchar_t tipText[500];
		NMTTDISPINFOW& nmttdi = *reinterpret_cast<NMTTDISPINFOW*>(&nmhdr.hdr);

//		nmttdi->hinst = getHandle();
		const Buffer& buffer = at(nmttdi.hdr.idFrom);
		wcscpy(tipText, (buffer.textFile().isOpen() ? buffer.textFile().location() : buffer.name()).c_str());
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
			setActive(bufferBar_.commandToIndex(n.iItem));	// n.iItem は ID
			n.pObject = 0;
			n.hResult = E_NOINTERFACE;
		}
		return 0;
	}

	else if(nmhdr.hdr.code == TBN_HOTITEMCHANGE && bufferBar_.getButtonCount() > 1 && bufferBar_.getHandle() == ::GetCapture()) {
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

/**
 * ファイルを開いたり保存したりするのに失敗したときの処理
 * @param fileName 処理中のファイル名 (このバッファがアクティブになっていなければならない)
 * @param forLoading 呼び出し元がファイルを開こうとしたとき true
 * @param result エラー内容
 * @return 結果的にエラーである場合 false
 */
bool BufferList::handleFileIOError(const WCHAR* fileName, bool forLoading, k::fileio::IOException::Type result) {
	using k::fileio::IOException;
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
		Alpha::instance().getMainWindow().messageBox(message.c_str(), IDS_APPNAME, MB_ICONEXCLAMATION);
	} else if(result != IOException::UNMAPPABLE_CHARACTER && result != IOException::MALFORMED_INPUT) {
		DWORD messageID;
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
		Alpha::instance().messageBox(messageID, MB_ICONEXCLAMATION, MARGS % fileName);
	}
	if(forLoading)
		close(EditorWindows::instance().activeBuffer());
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
	for(int i = static_cast<int>(min(from, to)); i <= end; ++i) {
		bufferBar_.setCommandID(i, i);
		bufferBar_.setButtonText(i, getDisplayName(*buffers_[i]).c_str());
	}
	setActive(*buffer);
	resetResources();
}

/**
 * Opens the specified file.
 * This method may show a dialog to indicate the result.
 * @param fileName the name of the file to open
 * @param encoding the encoding. auto-detect if omitted
 * @param asReadOnly set true to open as read only
 * @return the opened buffer or @c None if failed
 */
Buffer* BufferList::open(const basic_string<WCHAR>& fileName,
		const string& encoding /* = "UniversalAutoDetect" */, bool asReadOnly /* = false */) {
	// TODO: this method is too complex.
	using namespace k::fileio;
	Alpha& app = Alpha::instance();
	WCHAR resolvedName[MAX_PATH];

	// ショートカットの解決
	const WCHAR* extension = ::PathFindExtensionW(fileName.c_str());
	if(wcslen(extension) != 0 && (
			(::StrCmpIW(extension + 1, L"lnk") == 0)
			/*|| (::StrCmpIW(extension + 1, L"url") == 0)*/)) {
		HRESULT hr;
		ComPtr<IShellLinkW> shellLink(CLSID_ShellLink, IID_IShellLinkW, CLSCTX_ALL, 0, &hr);
		ComPtr<IPersistFile> file;

		try {
			if(FAILED(hr))
				throw hr;
			if(FAILED(hr = shellLink->QueryInterface(IID_IPersistFile, file.initializePPV())))
				throw hr;
			if(FAILED(hr = file->Load(fileName.c_str(), STGM_READ)))
				throw hr;
			if(FAILED(hr = shellLink->Resolve(0, SLR_ANY_MATCH | SLR_NO_UI)))
				throw hr;
			if(FAILED(hr = shellLink->GetPath(resolvedName, MAX_PATH, 0, 0)))
				throw hr;
		} catch(HRESULT /*hr_*/) {
			app.messageBox(MSG_IO__FAILED_TO_RESOLVE_SHORTCUT, MB_ICONHAND, MARGS % fileName);
			return 0;
		}
	} else
		wcscpy(resolvedName, canonicalizePathName(fileName.c_str()).c_str());

	// (テキストエディタで) 既に開かれているか調べる
	const size_t oldBuffer = find(resolvedName);
	if(oldBuffer != -1) {
		setActive(oldBuffer);
		return 0;
	}

	Buffer* buffer = &EditorWindows::instance().activeBuffer();
	TextFileDocumentInput::LockMode lockMode;
	switch(app.readIntegerProfile(L"File", L"shareMode", 0)) {
	case 0:	lockMode = TextFileDocumentInput::DONT_LOCK; break;
	case 1:	lockMode = TextFileDocumentInput::SHARED_LOCK; break;
	case 2:	lockMode = TextFileDocumentInput::EXCLUSIVE_LOCK; break;
	}

	if(buffer->isModified() || buffer->textFile().isOpen()) {	// 新しいコンテナで開く
		if(a::encoding::Encoder::supports(encoding))
			addNew(L"", encoding);
		else
			addNew();
		buffer = &EditorWindows::instance().activeBuffer();
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
	const wstring s(app.loadMessage(MSG_STATUS__LOADING_FILE, MARGS % resolvedName));
	while(true) {
		using namespace a::encoding;
		WaitCursor wc;
		app.setStatusText(s.c_str());
		app.getMainWindow().lockUpdate();

		// 準備ができたのでファイルを開く
		try {
			// TODO: 戻り値を調べる必要がある。
			buffer->textFile().open(resolvedName, lockMode, modifiedEncoding, Encoder::DONT_SUBSTITUTE);
		} catch(IOException& e) {
			succeeded = false;
			errorType = e.type();
		}
		app.setStatusText(0);
		app.getMainWindow().unlockUpdate();
		if(!succeeded) {
			// alert the encoding error
			int userAnswer;
			modifiedEncoding = buffer->textFile().encoding();
			if(errorType == IOException::UNMAPPABLE_CHARACTER)
				userAnswer = app.messageBox(MSG_IO__UNCONVERTABLE_NATIVE_CHAR, MB_YESNOCANCEL | MB_ICONEXCLAMATION,
					MARGS % resolvedName % Encoder::forMIB(fundamental::US_ASCII)->toUnicode(modifiedEncoding).c_str());
			else if(errorType == IOException::MALFORMED_INPUT)
				userAnswer = app.messageBox(MSG_IO__MALFORMED_INPUT_FILE, MB_OKCANCEL | MB_ICONEXCLAMATION,
					MARGS % resolvedName % Encoder::forMIB(fundamental::US_ASCII)->toUnicode(modifiedEncoding).c_str());
			else
				break;
			succeeded = true;
			if(userAnswer == IDYES || userAnswer == IDOK) {
				// the user want to change the encoding
				ui::EncodingsDialog dlg(modifiedEncoding, true);
				if(dlg.doModal(app.getMainWindow()) != IDOK)
					return 0;	// the user canceled
				modifiedEncoding = dlg.resultEncoding();
				continue;
			} else if(userAnswer == IDNO) {
				succeeded = true;
				try {
					buffer->textFile().open(resolvedName, lockMode, modifiedEncoding, Encoder::REPLACE_UNMAPPABLE_CHARACTER);
				} catch(IOException& e) {
					succeeded = false;
					if((errorType = e.type()) == IOException::MALFORMED_INPUT) {
						app.messageBox(MSG_IO__MALFORMED_INPUT_FILE, MB_OK | MB_ICONEXCLAMATION,
							MARGS % resolvedName % Encoder::forMIB(fundamental::US_ASCII)->toUnicode(modifiedEncoding).c_str());
						return 0;
					}
				}
			} else
				return 0;	// the user canceled
		}
		break;
	}

	app.getMainWindow().show(app.getMainWindow().isVisible() ? SW_SHOW : SW_RESTORE);

	if(succeeded || handleFileIOError(resolvedName, true, errorType)) {
		if(asReadOnly)
			buffer->setReadOnly();
		return buffers_.back();
	}
	return 0;
}

/**
 * Shows "Open" dialog box and opens file(s).
 * @param initialDirectory the directory name to show in the dialog first. if an empty string, the
 * directory of the active buffer. if the active buffer is not bound to a file, the system default
 * @return false if failed or the user canceled
 */
bool BufferList::openDialog(const wstring& initialDirectory /* = wstring() */) {
	Alpha& app = Alpha::instance();
	wstring filterSource = app.readStringProfile(L"File", L"filter", app.loadMessage(MSG_DIALOG__DEFAULT_OPENFILE_FILTER).c_str());
	wchar_t* filter = new wchar_t[filterSource.length() + 2];
	WCHAR fileName[MAX_PATH + 1] = L"";
	wstring errorMessage;

	// フィルタを整形
	replace_if(filterSource.begin(), filterSource.end(), bind2nd(equal_to<wchar_t>(), L':'), L'\0');
	filterSource.copy(filter, filterSource.length());
	filter[filterSource.length()] = L'\0';
	filter[filterSource.length() + 1] = L'\0';

	MANAH_AUTO_STRUCT(OSVERSIONINFOW, osVersion);
	osVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
	::GetVersionExW(&osVersion);

	WCHAR* activeBufferDir = 0;
	if(initialDirectory.empty() && EditorWindows::instance().activeBuffer().textFile().isOpen()) {
		activeBufferDir = new WCHAR[MAX_PATH];
		wcscpy(activeBufferDir, EditorWindows::instance().activeBuffer().textFile().pathName().c_str());
		*::PathFindFileNameW(activeBufferDir) = 0;
		if(activeBufferDir[0] == 0) {
			delete[] activeBufferDir;
			activeBufferDir = 0;
		}
	}

	TextFileFormat format = {a::encoding::Encoder::getDefault().properties().name(), k::NLF_RAW_VALUE};
	MANAH_AUTO_STRUCT_SIZE(OPENFILENAMEW, newOfn);
	MANAH_AUTO_STRUCT_SIZE(OPENFILENAME_NT4W, oldOfn);
	OPENFILENAMEW& ofn = (osVersion.dwMajorVersion > 4) ? newOfn : *reinterpret_cast<OPENFILENAMEW*>(&oldOfn);
	ofn.hwndOwner = app.getMainWindow().getHandle();
	ofn.hInstance = ::GetModuleHandle(0);
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = fileName;
	ofn.lpstrInitialDir = !initialDirectory.empty() ? initialDirectory.c_str() : activeBufferDir;
	ofn.nFilterIndex = app.readIntegerProfile(L"File", L"activeFilter", 0);
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_ALLOWMULTISELECT | OFN_ENABLEHOOK | OFN_ENABLESIZING | OFN_ENABLETEMPLATE
		| OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST/* | OFN_SHOWHELP*/;
	ofn.lCustData = reinterpret_cast<LPARAM>(&format);
	ofn.lpfnHook = openFileNameHookProc;
	ofn.lpTemplateName = MAKEINTRESOURCEW(IDD_DLG_OPENFILE);

	const bool succeeded = toBoolean(::GetOpenFileNameW(&ofn));
	delete[] filter;
	delete[] activeBufferDir;
	app.writeIntegerProfile(L"File", L"activeFilter", ofn.nFilterIndex);	// save the used filter

	if(succeeded) {
		const wstring directory = ofn.lpstrFile;

		if(directory.length() > ofn.nFileOffset)	// the number of files to open is 1
			// ofn.lpstrFile is full name
			return open(directory, format.encoding, toBoolean(ofn.Flags & OFN_READONLY)) != 0;
		else {	// open multiple files
			wchar_t* fileNames = ofn.lpstrFile + ofn.nFileOffset;
			bool failedOnce = false;

			while(*fileNames != 0) {
				const size_t len = wcslen(fileNames);
				if(open(directory + L"\\" + wstring(fileNames, len), format.encoding, toBoolean(ofn.Flags & OFN_READONLY)) == 0)
					failedOnce = true;
				fileNames += len + 1;
			}
			return !failedOnce;
		}
	} else
		return false;
}

/// Hook procedure for @c GetOpenFileNameW and @c GetSaveFileNameW.
UINT_PTR CALLBACK BufferList::openFileNameHookProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
	using namespace a::encoding;
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

			// TODO:
			if(/*encoding == minority::UTF_5 ||*/ encoding == standard::UTF_7
					|| encoding == fundamental::UTF_8
					|| encoding == fundamental::UTF_16LE || encoding == fundamental::UTF_16BE || encoding == fundamental::UTF_16
					|| encoding == standard::UTF_32 || encoding == standard::UTF_32LE || encoding == standard::UTF_32BE) {
				if(newlineCombobox.getCount() != 7) {
					newlineCombobox.resetContent();
					newlineCombobox.setItemData(newlineCombobox.addString(keepNLF.c_str()), k::NLF_RAW_VALUE);
					newlineCombobox.setItemData(newlineCombobox.addString(IDS_BREAK_CRLF), k::NLF_CR_LF);
					newlineCombobox.setItemData(newlineCombobox.addString(IDS_BREAK_LF), k::NLF_LINE_FEED);
					newlineCombobox.setItemData(newlineCombobox.addString(IDS_BREAK_CR), k::NLF_CARRIAGE_RETURN);
					newlineCombobox.setItemData(newlineCombobox.addString(IDS_BREAK_NEL), k::NLF_NEXT_LINE);
					newlineCombobox.setItemData(newlineCombobox.addString(IDS_BREAK_LS), k::NLF_LINE_SEPARATOR);
					newlineCombobox.setItemData(newlineCombobox.addString(IDS_BREAK_PS), k::NLF_PARAGRAPH_SEPARATOR);
					newlineCombobox.setCurSel(newline);
				}
			} else {
				if(newlineCombobox.getCount() != 4) {
					newlineCombobox.resetContent();
					newlineCombobox.setItemData(newlineCombobox.addString(keepNLF.c_str()), k::NLF_RAW_VALUE);
					newlineCombobox.setItemData(newlineCombobox.addString(IDS_BREAK_CRLF), k::NLF_CR_LF);
					newlineCombobox.setItemData(newlineCombobox.addString(IDS_BREAK_LF), k::NLF_LINE_FEED);
					newlineCombobox.setItemData(newlineCombobox.addString(IDS_BREAK_CR), k::NLF_CARRIAGE_RETURN);
					newlineCombobox.setCurSel((newline < 4) ? newline : 0);
				}
			}
		}
		break;
	case WM_INITDIALOG: {
		OPENFILENAMEW& ofn = *reinterpret_cast<OPENFILENAMEW*>(lParam);
		HWND dialog = ::GetParent(window);
		ComboBox encodingCombobox(::GetDlgItem(window, IDC_COMBO_ENCODING));
		Static encodingLabel(::GetDlgItem(window, IDC_STATIC_1));
		ComboBox newlineCombobox(::GetDlgItem(window, IDC_COMBO_NEWLINE));
		Static newlineLabel(::GetDlgItem(window, IDC_STATIC_2));
		HFONT guiFont = reinterpret_cast<HFONT>(::SendMessageW(dialog, WM_GETFONT, 0, 0L));

		// ダイアログテンプレートのコントロールの位置合わせなど
		POINT pt;
		RECT rect;
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
		const auto_ptr<Encoder> asciiEncoder(Encoder::forMIB(fundamental::US_ASCII));
		assert(asciiEncoder.get() != 0);

		vector<pair<size_t, const IEncodingProperties*> > encodings;
		Encoder::availableEncodings(back_inserter(encodings));
		for(vector<pair<size_t, const IEncodingProperties*> >::const_iterator encoding(encodings.begin()), e(encodings.end()); encoding != e; ++encoding) {
			const wstring name(asciiEncoder->toUnicode(encoding->second->displayName(locale::classic())));
			if(!name.empty()) {
				const int item = encodingCombobox.addString(name.c_str());
				if(item >= 0) {
					encodingCombobox.setItemData(item, static_cast<DWORD>(encoding->first));
					const string internalName(encoding->second->name());
					if(compareEncodingNames(internalName.begin(), internalName.end(), format.encoding.begin(), format.encoding.end()) == 0)
						encodingCombobox.setCurSel(item);
				}
			}
		}

		if(!newlineCombobox.isWindow()) {
			vector<string> detectors;
			EncodingDetector::availableNames(back_inserter(detectors));
			for(vector<string>::const_iterator detector(detectors.begin()), e(detectors.end()); detector != e; ++detector) {
				const wstring name(asciiEncoder->toUnicode(*detector));
				if(!name.empty()) {
					const int item = encodingCombobox.addString(name.c_str());
					encodingCombobox.setItemData(item, 0xFFFFFFFFU);
					if(compareEncodingNames(name.begin(), name.end(), format.encoding.begin(), format.encoding.end()) == 0)
						encodingCombobox.setCurSel(item);
				}
			}
		}

		if(encodingCombobox.getCurSel() == CB_ERR)
			encodingCombobox.setCurSel(0);

		if(newlineCombobox.isWindow()) {
			switch(format.newline) {
			case k::NLF_RAW_VALUE:				newlineCombobox.setCurSel(0);	break;
			case k::NLF_CR_LF:					newlineCombobox.setCurSel(1);	break;
			case k::NLF_LINE_FEED:				newlineCombobox.setCurSel(2);	break;
			case k::NLF_CARRIAGE_RETURN:		newlineCombobox.setCurSel(3);	break;
			case k::NLF_NEXT_LINE:				newlineCombobox.setCurSel(4);	break;
			case k::NLF_LINE_SEPARATOR:			newlineCombobox.setCurSel(5);	break;
			case k::NLF_PARAGRAPH_SEPARATOR:	newlineCombobox.setCurSel(6);	break;
			}
			::SendMessageW(window, WM_COMMAND, MAKEWPARAM(IDC_COMBO_ENCODING, CBN_SELCHANGE), 0);
		}
	}
		break;
	case WM_NOTIFY: {
		OFNOTIFYW& ofn = *reinterpret_cast<OFNOTIFYW*>(lParam);
		if(ofn.hdr.code == CDN_FILEOK) {	// [開く]/[保存]
			ComboBox encodingCombobox(::GetDlgItem(window, IDC_COMBO_ENCODING));
			ComboBox newlineCombobox(::GetDlgItem(window, IDC_COMBO_NEWLINE));
			Button readOnlyCheckbox(::GetDlgItem(::GetParent(window), chx1));
			TextFileFormat& format = *reinterpret_cast<TextFileFormat*>(ofn.lpOFN->lCustData);

			format.encoding.erase();
			const int encodingCurSel = encodingCombobox.getCurSel();
			if(encodingCurSel != CB_ERR) {
				const DWORD id = encodingCombobox.getItemData(encodingCurSel);
				if(id != 0xFFFFFFFFU)
					format.encoding = Encoder::forID(id)->properties().name();
			}
			if(format.encoding.empty()) {
				const wstring encodingName(encodingCombobox.getText());
				format.encoding = Encoder::forMIB(fundamental::US_ASCII)->fromUnicode(encodingName);
			}
			if(!Encoder::supports(format.encoding) && EncodingDetector::forName(format.encoding) == 0) {
				// reject for invalid encoding name
				Alpha::instance().messageBox(MSG_IO__UNSUPPORTED_ENCODING, MB_OK | MB_ICONEXCLAMATION);
				::SetWindowLongPtrW(window, DWLP_MSGRESULT, true);
				return true;
			}
			if(newlineCombobox.isWindow()) {
				switch(newlineCombobox.getCurSel()) {
				case 0:	format.newline = k::NLF_RAW_VALUE;				break;
				case 1:	format.newline = k::NLF_CR_LF;					break;
				case 2:	format.newline = k::NLF_LINE_FEED;				break;
				case 3:	format.newline = k::NLF_CARRIAGE_RETURN;		break;
				case 4:	format.newline = k::NLF_NEXT_LINE;				break;
				case 5:	format.newline = k::NLF_LINE_SEPARATOR;			break;
				case 6:	format.newline = k::NLF_PARAGRAPH_SEPARATOR;	break;
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
		k::Document& document, IUnexpectedFileTimeStampDirector::Context context) throw() {
	const Buffer& buffer = getConcreteDocument(document);
	const Buffer& activeBuffer = EditorWindows::instance().activeBuffer();
	setActive(buffer);
	switch(context) {
	case IUnexpectedFileTimeStampDirector::FIRST_MODIFICATION:
		return Alpha::instance().messageBox(MSG_BUFFER__FILE_IS_MODIFIED_AND_EDIT,
			MB_YESNO | MB_ICONQUESTION, MARGS % buffer.textFile().pathName()) == IDYES;
	case IUnexpectedFileTimeStampDirector::OVERWRITE_FILE:
		return Alpha::instance().messageBox(MSG_BUFFER__FILE_IS_MODIFIED_AND_SAVE,
			MB_YESNO | MB_ICONQUESTION, MARGS % buffer.textFile().pathName()) == IDYES;
	case IUnexpectedFileTimeStampDirector::CLIENT_INVOCATION:
		if(IDYES == Alpha::instance().messageBox(MSG_BUFFER__FILE_IS_MODIFIED_AND_REOPEN,
				MB_YESNO | MB_ICONQUESTION, MARGS % buffer.textFile().pathName()))
			reopen(find(buffer), false);
		else
			setActive(activeBuffer);
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
		RECT rect;
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
	using namespace k::fileio;
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
		using namespace a::encoding;
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
						buffer.textFile().lockMode(), encoding, Encoder::REPLACE_UNMAPPABLE_CHARACTER);
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

	SHFILEINFOW sfi;
	for(size_t i = 0; i < buffers_.size(); ++i) {
		::SHGetFileInfoW(
			(buffers_[i]->textFile().isOpen()) ? buffers_[i]->textFile().pathName().c_str() : L"",
			0, &sfi, sizeof(::SHFILEINFOW), SHGFI_ICON | SHGFI_SMALLICON);
		icons_.add(sfi.hIcon);
		listMenu_ << Menu::OwnerDrawnItem(static_cast<UINT>(i));
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
	Alpha& app = Alpha::instance();
	Buffer& buffer = at(index);

	// 保存の必要があるか?
	if(overwrite && buffer.textFile().isOpen() && !buffer.isModified())
		return true;

	WCHAR fileName[MAX_PATH + 1];
	TextFileFormat format = {buffer.textFile().encoding(), k::NLF_RAW_VALUE};
	bool newName = false;

	// 別名で保存 or ファイルが存在しない
	if(!overwrite || !buffer.textFile().isOpen() || !toBoolean(::PathFileExistsW(buffer.textFile().pathName().c_str()))) {
		MANAH_AUTO_STRUCT(OSVERSIONINFOW, osVersion);
		const wstring filterSource(app.loadMessage(MSG_DIALOG__SAVE_FILE_FILTER));
		wchar_t* const filter = new wchar_t[filterSource.length() + 6];

		osVersion.dwOSVersionInfoSize = sizeof(::OSVERSIONINFOW);
		::GetVersionExW(&osVersion);
		filterSource.copy(filter, filterSource.length());
		wcsncpy(filter + filterSource.length(), L"\0*.*\0\0", 6);
		wcscpy(fileName, (buffer.textFile().pathName().c_str()));

		MANAH_AUTO_STRUCT_SIZE(OPENFILENAMEW, newOfn);
		MANAH_AUTO_STRUCT_SIZE(OPENFILENAME_NT4W, oldOfn);
		OPENFILENAMEW& ofn = (osVersion.dwMajorVersion > 4) ? newOfn : *reinterpret_cast<OPENFILENAMEW*>(&oldOfn);
		ofn.hwndOwner = app.getMainWindow().getHandle();
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

	using namespace a::encoding;
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

	using namespace k::fileio;
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
bool BufferList::saveSomeDialog() {
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
			if(!save(find(it->fileName), true))
				return false;
		}
	}
	return true;
}

/**
 * Activates the specified buffer in the active pane.
 * @param index the index of the buffer to activate
 * @throw std#out_of_range @a index is invalid
 */
void BufferList::setActive(size_t index) {
	EditorWindows::instance().activePane().showBuffer(at(index));
	fireActiveBufferSwitched();
}

/**
 * Activates the specified buffer in the active pane.
 * @param buffer the buffer to activate
 * @throw std#invalid_argument @a buffer is not exist
 */
void BufferList::setActive(const Buffer& buffer) {
	EditorWindows::instance().activePane().showBuffer(buffer);
	fireActiveBufferSwitched();
}

/// @see ascension#presentation#ITextViewerListListener#textViewerListChanged
void BufferList::textViewerListChanged(a::presentation::Presentation& presentation) {
}

/// Reconstructs the context menu.
void BufferList::updateContextMenu() {
	while(contextMenu_.getNumberOfItems() > 0)
		contextMenu_.erase<Menu::BY_POSITION>(0);
//	contextMenu_ << Menu::StringItem(CMD_FILE_CLOSE, app_.commandManager().menuName(CMD_FILE_CLOSE).c_str())
//		<< Menu::StringItem(CMD_FILE_CLOSEOTHERS, app_.commandManager().menuName(CMD_FILE_CLOSEOTHERS).c_str());
//	contextMenu_.setDefault<Menu::BY_COMMAND>(CMD_FILE_CLOSE);
}


// AutomationPosition ///////////////////////////////////////////////////////

/// Constructor.
AutomationPosition::AutomationPosition(const Position& position) : Position(position) {
}

/// @see IPosition#get_Column
STDMETHODIMP AutomationPosition::get_Column(long* n) {
	MANAH_VERIFY_POINTER(n);
	return ::VarI4FromUI8(column, n);
}

/// @see IPosition#get_Line
STDMETHODIMP AutomationPosition::get_Line(long* n) {
	MANAH_VERIFY_POINTER(n);
	return ::VarI4FromUI8(line, n);
}

/// @see IPosition#put_Column
STDMETHODIMP AutomationPosition::put_Column(long n) {
	if(n < 0)
		return E_INVALIDARG;
	return column = n, S_OK;
}

/// @see IPosition#put_Line
STDMETHODIMP AutomationPosition::put_Line(long n) {
	if(n < 0)
		return E_INVALIDARG;
	return line = n, S_OK;
}


// AutomationRegion /////////////////////////////////////////////////////////

namespace {
	inline k::Position extract(IPosition& position) /*throw()*/ {
		pair<long, long> temp;
		position.get_Line(&temp.first);
		position.get_Column(&temp.second);
		return k::Position(temp.first, temp.second);
	}
	inline k::Region extract(IRegion& region) /*throw()*/ {
		pair<ComPtr<IPosition>, ComPtr<IPosition> > temp;
		region.get_Beginning(temp.first.initialize());
		region.get_End(temp.second.initialize());
		return k::Region(extract(*temp.first.get()), extract(*temp.second.get()));
	}
} // namespace

/// Constructor.
AutomationRegion::AutomationRegion(const k::Region& region) : k::Region(region) {
}

/// @see IRegion#get_Beginning
STDMETHODIMP AutomationRegion::get_Beginning(IPosition** p) {
	MANAH_VERIFY_POINTER(p);
	try {
		(*p = new AutomationPosition(beginning()))->AddRef();
	} catch(bad_alloc&) {
		return (*p = 0), E_OUTOFMEMORY;
	}
	return S_OK;
}

/// @see IRegion#get_End
STDMETHODIMP AutomationRegion::get_End(IPosition** p) {
	MANAH_VERIFY_POINTER(p);
	try {
		(*p = new AutomationPosition(end()))->AddRef();
	} catch(bad_alloc&) {
		return (*p = 0), E_OUTOFMEMORY;
	}
	return S_OK;
}

/// @see IRegion#Encompasses
STDMETHODIMP AutomationRegion::Encompasses(IRegion* other, VARIANT_BOOL* result) {
	MANAH_VERIFY_POINTER(other);
	if(result != 0)
		*result = toVariantBoolean(encompasses(extract(*other)));
	return S_OK;
}

/// @see IRegion#GetIntersection
STDMETHODIMP AutomationRegion::GetIntersection(IRegion* other, IRegion** result) {
	MANAH_VERIFY_POINTER(other);
	if(result != 0) {
		const Region temp(getIntersection(extract(*other)));
		try {
			(*result = new AutomationRegion(temp))->AddRef();
		} catch(bad_alloc&) {
			return (*result = 0), E_OUTOFMEMORY;
		}
	}
	return S_OK;
}

/// @see IRegion#GetUnion
STDMETHODIMP AutomationRegion::GetUnion(IRegion* other, IRegion** result) {
	MANAH_VERIFY_POINTER(other);
	if(result != 0) {
		const Region temp(getUnion(extract(*other)));
		try {
			(*result = new AutomationRegion(temp))->AddRef();
		} catch(bad_alloc&) {
			return (*result = 0), E_OUTOFMEMORY;
		}
	}
	return S_OK;
}

/// @see IRegion#Includes
STDMETHODIMP AutomationRegion::Includes(IPosition* p, VARIANT_BOOL* result) {
	MANAH_VERIFY_POINTER(p);
	if(result != 0)
		*result = toVariantBoolean(includes(extract(*p)));
	return S_OK;
}

/// @see IRegion#IntersectsWith
STDMETHODIMP AutomationRegion::IntersectsWith(IRegion* other, VARIANT_BOOL* result) {
	MANAH_VERIFY_POINTER(other);
	if(result != 0)
		*result = toVariantBoolean(intersectsWith(extract(*other)));
	return S_OK;
}

/// @see IRegion#IsEmpty
STDMETHODIMP AutomationRegion::IsEmpty(VARIANT_BOOL* result) {
	if(result != 0)
		*result = toVariantBoolean(isEmpty());
	return S_OK;
}


// PointProxyBase ///////////////////////////////////////////////////////////
/*
PointProxyBase::PointProxyBase(k::Point& impl) {
	reset(impl);
}

/// @see IPoint#get_AdaptsToBuffer
STDMETHODIMP PointProxyBase::get_AdaptsToBuffer(VARIANT_BOOL* adapts) {
	MANAH_VERIFY_POINTER(adapts);
	AMBIENT_CHECK_PROXY();
	return (*adapts = toVariantBoolean(impl().adaptsToDocument())), S_OK;
}

/// @see IPoint#get_Buffer
STDMETHODIMP PointProxyBase::get_Buffer(IBuffer** buffer) {
	MANAH_VERIFY_POINTER(buffer);
	AMBIENT_CHECK_PROXY();
	(*buffer = static_cast<Buffer*>(impl().document())->asScript().get())->AddRef();
	return S_OK;
}

/// @see IPoint#get_Column
STDMETHODIMP PointProxyBase::get_Column(long* column) {
	MANAH_VERIFY_POINTER(column);
	AMBIENT_CHECK_PROXY();
	return ::VarI4FromUI8(impl().columnNumber(), column);
}

/// @see IPoint#get_Gravity
STDMETHODIMP PointProxyBase::get_Gravity(Direction* gravity) {
	MANAH_VERIFY_POINTER(gravity);
	AMBIENT_CHECK_PROXY();
	return (*gravity = (impl().gravity() == a::Direction::FORWARD) ? Forward : Backward), S_OK;
}

/// @see IPoint#get_Position
STDMETHODIMP PointProxyBase::get_Position(IPosition** position) {
	MANAH_VERIFY_POINTER(position);
	AMBIENT_CHECK_PROXY();
	try {
		(*position = new AutomationPosition(impl().position()))->AddRef();
	} catch(const bad_alloc&) {
		*position = 0;
		return E_OUTOFMEMORY;
	}
	return S_OK;
}

/// @see IPoint#IsBufferDeleted
STDMETHODIMP PointProxyBase::IsBufferDeleted(VARIANT_BOOL* result) {
	AMBIENT_CHECK_PROXY();
	if(result != 0)
		*result = toVariantBoolean(impl().isDocumentDisposed());
	return S_OK;
}

/// @see IPoint#MoveTo
STDMETHODIMP PointProxyBase::MoveTo(IPosition* to) {
	MANAH_VERIFY_POINTER(to);
	AMBIENT_CHECK_PROXY();
	try {
		impl().moveTo(extract(*to));
	} catch(k::BadPositionException&) {
		ComException e(E_INVALIDARG, IID_IPoint, OLESTR("ambient.Point.MoveTo"), OLESTR("BadPositionError"));
		e.throwLogicalThreadError();
		return e.getSCode();
	}
	return S_OK;
}

/// @see IPoint#put_AdaptsToBuffer
STDMETHODIMP PointProxyBase::put_AdaptsToBuffer(VARIANT_BOOL adapts) {
	AMBIENT_CHECK_PROXY();
	return impl().adaptToDocument(toBoolean(adapts)), S_OK;
}

/// @see IPoint#put_ExcludedFromRestriction
STDMETHODIMP PointProxyBase::put_ExcludedFromRestriction(VARIANT_BOOL excluded) {
	AMBIENT_CHECK_PROXY();
	return impl().excludeFromRestriction(toBoolean(excluded)), S_OK;
}

/// @see IPoint#put_Gravity
STDMETHODIMP PointProxyBase::put_Gravity(Direction gravity) {
	AMBIENT_CHECK_PROXY();
	return impl().setGravity((gravity == Forward) ? a::Direction::FORWARD : a::Direction::BACKWARD), S_OK;
}
*/

// BookmarkerProxy //////////////////////////////////////////////////////////

BookmarkerProxy::BookmarkerProxy(k::Bookmarker& impl) : AutomationProxy(impl) {
}

/// @see IBookmarker#Clear
STDMETHODIMP BookmarkerProxy::Clear() {
	AMBIENT_CHECK_PROXY();
	return impl().clear(), S_OK;
}

/// @see IBookmarker#IsMarked
STDMETHODIMP BookmarkerProxy::IsMarked(long line, VARIANT_BOOL* result) {
	AMBIENT_CHECK_PROXY();
	if(result != 0) {
		try {
			if(line < 0)
				throw k::BadPositionException(k::Position::INVALID_POSITION);
			*result = toVariantBoolean(impl().isMarked(line));
		} catch(const k::BadPositionException&) {
			return E_INVALIDARG;
		}
	}
	return S_OK;
}

/// @see IBookmarker#Mark
STDMETHODIMP BookmarkerProxy::Mark(long line, VARIANT_BOOL set) {
	AMBIENT_CHECK_PROXY();
	try {
		if(line < 0)
			throw k::BadPositionException(k::Position::INVALID_POSITION);
		impl().mark(line, toBoolean(set));
	} catch(const k::BadPositionException&) {
		return E_INVALIDARG;
	}
	return S_OK;
}

/// @see IBookmarker#Next
STDMETHODIMP BookmarkerProxy::Next(long from, Direction direction, VARIANT_BOOL wrapAround, long marks, long* result) {
	AMBIENT_CHECK_PROXY();
	if(marks < 0)
		return E_INVALIDARG;
	if(result != 0) {
		try {
			if(from < -1L)
				throw k::BadPositionException(k::Position::INVALID_POSITION);
			const a::length_t temp = impl().next((from != -1L) ? from : a::INVALID_INDEX,
				(direction == Forward) ? a::Direction::FORWARD : a::Direction::BACKWARD, toBoolean(wrapAround), marks);
			return ::VarI4FromUI8(temp, result);
		} catch(const k::BadPositionException&) {
			return E_INVALIDARG;
		}
	}
	return S_OK;
}

/// @see IBookmarker#Toggle
STDMETHODIMP BookmarkerProxy::Toggle(long line) {
	AMBIENT_CHECK_PROXY();
	try {
		if(line < 0)
			throw k::BadPositionException(k::Position::INVALID_POSITION);
		impl().toggle(line);
	} catch(const k::BadPositionException&) {
		return E_INVALIDARG;
	}
	return S_OK;
}


// BufferProxy //////////////////////////////////////////////////////////////

BufferProxy::BufferProxy(Buffer& impl) : AutomationProxy(impl) {
}

/// @see IBuffer#BeginCompoundChange
STDMETHODIMP BufferProxy::BeginCompoundChange() {
	AMBIENT_CHECK_PROXY();
	try {
		impl().beginCompoundChange();
	} catch(k::ReadOnlyDocumentException&) {
		return E_INVALIDARG;
	}
	return S_OK;
}

/// @see IBuffer#ClearUndoBuffer
STDMETHODIMP BufferProxy::ClearUndoBuffer() {
	AMBIENT_CHECK_PROXY();
	return impl().clearUndoBuffer(), S_OK;
}

/// @see IBuffer#Close
STDMETHODIMP BufferProxy::Close() {
	if(check()) {
		try {
			BufferList::instance().close(impl());
		} catch(const invalid_argument&) {
		}
	}
	return S_OK;
}

/// @see IBuffer#EndCompoundChange
STDMETHODIMP BufferProxy::EndCompoundChange() {
	AMBIENT_CHECK_PROXY();
	try {
		impl().endCompoundChange();
	} catch(a::IllegalStateException&) {
		return E_INVALIDARG;;
	}
	return S_OK;
}

/// @see IBuffer#Erase
STDMETHODIMP BufferProxy::Erase(IRegion* region, VARIANT_BOOL* result) {
	MANAH_VERIFY_POINTER(region);
	AMBIENT_CHECK_PROXY();
	bool temp;
	try {
		temp = impl().erase(extract(*region));
	} catch(const k::ReadOnlyDocumentException&) {
		return E_ACCESSDENIED;
	} catch(const k::DocumentAccessViolationException&) {
		return E_ACCESSDENIED;
	}
	if(result != 0)
		*result = toVariantBoolean(temp);
	return S_OK;
}

/// @see IBuffer#get_AccessibleRegion
STDMETHODIMP BufferProxy::get_AccessibleRegion(IRegion** region) {
	MANAH_VERIFY_POINTER(region);
	AMBIENT_CHECK_PROXY();
	try {
		(*region = new AutomationRegion(impl().accessibleRegion()))->AddRef();
	} catch(const bad_alloc&) {
		return (*region = 0), E_OUTOFMEMORY;
	}
	return S_OK;
}

/// @see IBuffer#get_Bookmarker
STDMETHODIMP BufferProxy::get_Bookmarker(IBookmarker** bookmarker) {
	MANAH_VERIFY_POINTER(bookmarker);
	AMBIENT_CHECK_PROXY();
	*bookmarker = new(nothrow) BookmarkerProxy(impl().bookmarker());
	if(*bookmarker == 0)
		return E_OUTOFMEMORY;
	return (*bookmarker)->AddRef(), S_OK;
}

/// @see IBuffer#get_BoundToFile
STDMETHODIMP BufferProxy::get_BoundToFile(VARIANT_BOOL* bound) {
	MANAH_VERIFY_POINTER(bound);
	AMBIENT_CHECK_PROXY();
	return (*bound = toVariantBoolean(impl().textFile().isOpen())), S_OK;
}

/// @see IBuffer#get_Encoding
STDMETHODIMP BufferProxy::get_Encoding(BSTR* encoding) {
	MANAH_VERIFY_POINTER(encoding);
	AMBIENT_CHECK_PROXY();
	const string ascii(impl().textFile().encoding());
	*encoding = ::SysAllocStringLen(0, static_cast<UINT>(ascii.length()));
	if(*encoding == 0)
		return E_OUTOFMEMORY;
	wcscpy(*encoding, a::encoding::Encoder::forMIB(a::encoding::fundamental::US_ASCII)->toUnicode(ascii).c_str());
	return S_OK;
}

/// @see IBuffer#get_InCompoundChanging
STDMETHODIMP BufferProxy::get_InCompoundChanging(VARIANT_BOOL* compound) {
	MANAH_VERIFY_POINTER(compound);
	AMBIENT_CHECK_PROXY();
	return (*compound = toVariantBoolean(impl().isCompoundChanging())), S_OK;
}

/// @see IBuffer#get_Length
STDMETHODIMP BufferProxy::get_Length(Newline newline, long* length) {
	MANAH_VERIFY_POINTER(length);
	AMBIENT_CHECK_PROXY();
	try {
		return ::VarI4FromUI8(impl().length(static_cast<k::Newline>(newline)), length);
	} catch(const a::UnknownValueException&) {
		return E_INVALIDARG;
	}
	return E_FAIL;	// hmm...
}

/// @see IBuffer#get_Line
STDMETHODIMP BufferProxy::get_Line(long line, BSTR* s) {
	MANAH_VERIFY_POINTER(s);
	AMBIENT_CHECK_PROXY();
	if(line < 0)
		return E_INVALIDARG;
	try {
		*s = ::SysAllocString(impl().line(line).c_str());
	} catch(const k::BadPositionException&) {
		return E_INVALIDARG;;
	}
	if(*s == 0)
		return E_OUTOFMEMORY;
	return S_OK;
}

/// @see IBuffer#get_Modified
STDMETHODIMP BufferProxy::get_Modified(VARIANT_BOOL* modified) {
	MANAH_VERIFY_POINTER(modified);
	AMBIENT_CHECK_PROXY();
	return (*modified = toVariantBoolean(impl().isModified())), S_OK;
}

/// @see IBuffer#get_Name
STDMETHODIMP BufferProxy::get_Name(BSTR* name) {
	MANAH_VERIFY_POINTER(name);
	AMBIENT_CHECK_PROXY();
	*name = ::SysAllocString(impl().name().c_str());
	return (*name != 0) ? S_OK : E_OUTOFMEMORY;
}

/// @see IBuffer#get_Narrowed
STDMETHODIMP BufferProxy::get_Narrowed(VARIANT_BOOL* narrowed) {
	MANAH_VERIFY_POINTER(narrowed);
	AMBIENT_CHECK_PROXY();
	return (*narrowed = toVariantBoolean(impl().isNarrowed())), S_OK;
}

/// @see IBuffer#get_Newline
STDMETHODIMP BufferProxy::get_Newline(Newline* newline) {
	MANAH_VERIFY_POINTER(newline);
	AMBIENT_CHECK_PROXY();
	return (*newline = static_cast<Newline>(impl().textFile().newline())), S_OK;
}

/// @see IBuffer#get_ReadOnly
STDMETHODIMP BufferProxy::get_ReadOnly(VARIANT_BOOL* readOnly) {
	MANAH_VERIFY_POINTER(readOnly);
	AMBIENT_CHECK_PROXY();
	return *readOnly = toVariantBoolean(impl().isReadOnly()), S_OK;
}

/// @see IBuffer#get_RecordsChanges
STDMETHODIMP BufferProxy::get_RecordsChanges(VARIANT_BOOL* records) {
	MANAH_VERIFY_POINTER(records);
	AMBIENT_CHECK_PROXY();
	return *records = toVariantBoolean(impl().isRecordingChanges()), S_OK;
}

/// @see IBuffer#get_Region
STDMETHODIMP BufferProxy::get_Region(IRegion** region) {
	MANAH_VERIFY_POINTER(region);
	AMBIENT_CHECK_PROXY();
	try {
		(*region = new AutomationRegion(impl().region()))->AddRef();
	} catch(const bad_alloc&) {
		return (*region = 0), E_OUTOFMEMORY;
	}
	return S_OK;
}

/// @see IBuffer#get_RevisionNumber
STDMETHODIMP BufferProxy::get_RevisionNumber(long* number) {
	MANAH_VERIFY_POINTER(number);
	AMBIENT_CHECK_PROXY();
	return ::VarI4FromUI8(impl().revisionNumber(), number);
}

/// @see IBuffer#get_UnicodeByteOrderMark
STDMETHODIMP BufferProxy::get_UnicodeByteOrderMark(VARIANT_BOOL* mark) {
	MANAH_VERIFY_POINTER(mark);
	AMBIENT_CHECK_PROXY();
	return (*mark = impl().textFile().unicodeByteOrderMark()), S_OK;
}

/// @see IBuffer#Insert
STDMETHODIMP BufferProxy::Insert(IPosition* at, BSTR text, VARIANT_BOOL* result) {
	MANAH_VERIFY_POINTER(at);
	AMBIENT_CHECK_PROXY();
	if(const UINT len = ::SysStringLen(text)) {
		bool succeeded;
		try {
			succeeded = impl().insert(extract(*at), text, text + len);
		} catch(const k::BadPositionException&) {
			return E_INVALIDARG;;
		} catch(const k::ReadOnlyDocumentException&) {
			return E_ACCESSDENIED;
		} catch(const k::DocumentAccessViolationException&) {
			return E_ACCESSDENIED;
		}
		if(result != 0)
			*result = toVariantBoolean(succeeded);
	} else if(result != 0)
		*result = VARIANT_TRUE;
	return S_OK;
}

/// @see IBuffer#InsertUndoBoundary
STDMETHODIMP BufferProxy::InsertUndoBoundary() {
	AMBIENT_CHECK_PROXY();
	try {
		return impl().insertUndoBoundary(), S_OK;
	} catch(const k::ReadOnlyDocumentException&) {
		return E_ACCESSDENIED;
	}
}

/// @see IBuffer#MarkUnmodified
STDMETHODIMP BufferProxy::MarkUnmodified() {
	AMBIENT_CHECK_PROXY();
	return impl().markUnmodified(), S_OK;
}

/// @see IBuffer#Narrow
STDMETHODIMP BufferProxy::NarrowToRegion(IRegion* region) {
	MANAH_VERIFY_POINTER(region);
	AMBIENT_CHECK_PROXY();
	try {
		return impl().narrowToRegion(extract(*region)), S_OK;
	} catch(const k::BadRegionException&) {
		return E_INVALIDARG;
	}
	return E_FAIL;
}

/// @see IBuffer#put_Encoding
STDMETHODIMP BufferProxy::put_Encoding(BSTR encoding) {
	AMBIENT_CHECK_PROXY();
	try {
		if(encoding == 0)
			throw a::encoding::UnsupportedEncodingException("encoding");
		impl().textFile().setEncoding(a::encoding::encodingNameFromUnicode(encoding));
	} catch(const a::encoding::UnsupportedEncodingException&) {
		return E_INVALIDARG;
	}
	return S_OK;
}

/// @see IBuffer#put_Newline
STDMETHODIMP BufferProxy::put_Newline(Newline newline) {
	AMBIENT_CHECK_PROXY();
	try {
		return impl().textFile().setNewline(static_cast<k::Newline>(newline)), S_OK;
	} catch(const a::UnknownValueException&) {
		return E_INVALIDARG;
	}
	return E_FAIL;
}

/// @see IBuffer#put_ReadOnly
STDMETHODIMP BufferProxy::put_ReadOnly(VARIANT_BOOL readOnly) {
	AMBIENT_CHECK_PROXY();
	return impl().setReadOnly(toBoolean(readOnly)), S_OK;
}

/// @see IBuffer#put_RecordsChanges
STDMETHODIMP BufferProxy::put_RecordsChanges(VARIANT_BOOL record) {
	AMBIENT_CHECK_PROXY();
	return impl().recordChanges(toBoolean(record)), S_OK;
}

/// @see IBuffer#Redo
STDMETHODIMP BufferProxy::Redo(long n, VARIANT_BOOL* result) {
	AMBIENT_CHECK_PROXY();
	try {
		if(n < 0)
			throw invalid_argument("n");
		const bool temp = impl().redo(n);
		if(result != 0)
			*result = toVariantBoolean(temp);
		return S_OK;
	} catch(const k::ReadOnlyDocumentException&) {
		return E_ACCESSDENIED;
	} catch(const invalid_argument&) {
		return E_INVALIDARG;
	}
	return E_FAIL;
}

/// @see IBuffer#ResetContent
STDMETHODIMP BufferProxy::ResetContent() {
	AMBIENT_CHECK_PROXY();
	return impl().resetContent(), S_OK;
}

/// @see IBuffer#Undo
STDMETHODIMP BufferProxy::Undo(long n, VARIANT_BOOL* result) {
	AMBIENT_CHECK_PROXY();
	try {
		if(n < 0)
			throw invalid_argument("n");
		const bool temp = impl().undo(n);
		if(result != 0)
			*result = toVariantBoolean(temp);
		return S_OK;
	} catch(const k::ReadOnlyDocumentException&) {
		return E_ACCESSDENIED;
	} catch(const invalid_argument&) {
		return E_INVALIDARG;
	}
	return E_FAIL;
}

/// @see IBuffer#Widen
STDMETHODIMP BufferProxy::Widen() {
	AMBIENT_CHECK_PROXY();
	return impl().widen(), S_OK;
}


// BufferListProxy //////////////////////////////////////////////////////////

/// @see IScriptSystem#get_Buffers
STDMETHODIMP ambient::ScriptSystem::get_Buffers(IBufferList** buffers) {
	MANAH_VERIFY_POINTER(buffers);
	(*buffers = BufferList::instance().asScript().get())->AddRef();
	return S_OK;
}

/// @see IScriptSystem#Position
STDMETHODIMP ambient::ScriptSystem::Position(SAFEARRAY* parameters, IPosition** newInstance) {
	MANAH_VERIFY_POINTER(parameters);
	if(newInstance != 0) {
		*newInstance = 0;
		if(::SafeArrayGetDim(parameters) != 1)
			return E_INVALIDARG;
		VARTYPE arrayType;
		::SafeArrayGetVartype(parameters, &arrayType);
		if(arrayType != VT_VARIANT)
			return E_INVALIDARG;
		HRESULT hr;
		long lbound, ubound;
		if(FAILED(hr = ::SafeArrayGetLBound(parameters, 1, &lbound))
				|| FAILED(hr = ::SafeArrayGetUBound(parameters, 1, &ubound)))
			return E_INVALIDARG;
		try {
			switch(ubound - lbound + 1) {
			case 0:
				(*newInstance = new AutomationPosition(k::Position()))->AddRef();
				break;
			case 1: {
				AutoVARIANT other;
				hr = ::SafeArrayGetElement(parameters, &ubound, &other);
				if(other.vt != VT_UNKNOWN && other.vt != VT_DISPATCH)
					return DISP_E_TYPEMISMATCH;
				ComQIPtr<IPosition, &IID_IPosition> rhs((other.vt == VT_UNKNOWN) ? other.punkVal : other.pdispVal);
				if(rhs.get() == 0)
					return DISP_E_TYPEMISMATCH;
				(*newInstance = new AutomationPosition(extract(*rhs.get())))->AddRef();
				break;
				}
			case 2: {
				AutoVARIANT line, column;
				if(FAILED(hr = ::SafeArrayGetElement(parameters, &lbound, &line))
						|| FAILED(hr = ::SafeArrayGetElement(parameters, &ubound, &column)))
					return E_INVALIDARG;
				if(FAILED(hr = line.coerce(VT_UI8, VARIANT_NOVALUEPROP)) || FAILED(hr = column.coerce(VT_UI8, VARIANT_NOVALUEPROP)))
					return hr;
				(*newInstance = new AutomationPosition(k::Position(V_UI8(&line), V_UI8(&column))))->AddRef();
				break;
			}
			default:
				return DISP_E_BADPARAMCOUNT;
			}
		} catch(const bad_alloc&) {
			return E_OUTOFMEMORY;
		}
	}
	return S_OK;
}

/// @see IScriptSystem#Region
STDMETHODIMP ambient::ScriptSystem::Region(SAFEARRAY* parameters, IRegion** newInstance) {
	MANAH_VERIFY_POINTER(parameters);
	if(newInstance != 0) {
		*newInstance = 0;
		if(::SafeArrayGetDim(parameters) != 1)
			return E_INVALIDARG;
		VARTYPE arrayType;
		::SafeArrayGetVartype(parameters, &arrayType);
		if(arrayType != VT_VARIANT)
			return E_INVALIDARG;
		HRESULT hr;
		long lbound, ubound;
		if(FAILED(hr = ::SafeArrayGetLBound(parameters, 1, &lbound))
				|| FAILED(hr = ::SafeArrayGetUBound(parameters, 1, &ubound)))
			return E_INVALIDARG;
		try {
			switch(ubound - lbound + 1) {
			case 0:
				return (*newInstance = new AutomationRegion(k::Region()))->AddRef(), S_OK;
			case 1: {
				AutoVARIANT temp;
				hr = ::SafeArrayGetElement(parameters, &ubound, &temp);
				if(V_VT(&temp) != VT_UNKNOWN && V_VT(&temp) != VT_DISPATCH)
					return DISP_E_TYPEMISMATCH;
				ComQIPtr<IPosition, &IID_IPosition> p((V_VT(&temp) == VT_UNKNOWN) ? V_UNKNOWN(&temp) : V_DISPATCH(&temp));
				if(p.get() != 0)
					return (*newInstance = new AutomationRegion(k::Region(extract(*p.get()))))->AddRef(), S_OK;
				ComQIPtr<IRegion, &IID_IRegion> r((V_VT(&temp) == VT_UNKNOWN) ? V_UNKNOWN(&temp) : V_DISPATCH(&temp));
				if(r.get() != 0)
					return (*newInstance = new AutomationRegion(extract(*r.get())))->AddRef(), S_OK;
				return DISP_E_TYPEMISMATCH;
			}
			case 2: {
				AutoVARIANT v[2];
				if(FAILED(hr = ::SafeArrayGetElement(parameters, &lbound, &v[0]))
						|| FAILED(hr = ::SafeArrayGetElement(parameters, &ubound, &v[1])))
					return E_INVALIDARG;
				if((V_VT(&v[0]) != VT_UNKNOWN && V_VT(&v[0]) != VT_DISPATCH)
						|| (V_VT(&v[1]) != VT_UNKNOWN && V_VT(&v[1]) != VT_DISPATCH))
					return DISP_E_TYPEMISMATCH;
				ComQIPtr<IPosition, &IID_IPosition> p1((V_VT(&v[0]) == VT_UNKNOWN) ? V_UNKNOWN(&v[0]) : V_DISPATCH(&v[0]));
				ComQIPtr<IPosition, &IID_IPosition> p2((V_VT(&v[1]) == VT_UNKNOWN) ? V_UNKNOWN(&v[1]) : V_DISPATCH(&v[1]));
				if(p1.get() == 0 || p2.get() == 0)
					return DISP_E_TYPEMISMATCH;
				return (*newInstance = new AutomationRegion(k::Region(extract(*p1.get()), extract(*p2.get()))))->AddRef(), S_OK;
			}
			default:
				return DISP_E_BADPARAMCOUNT;
			}
		} catch(const bad_alloc&) {
			return E_OUTOFMEMORY;
		}
	}
	return S_OK;
}

/// Constructor.
BufferListProxy::BufferListProxy(BufferList& impl) : AutomationProxy(impl) {
}

/// @see IBufferList#get__NewEnum
STDMETHODIMP BufferListProxy::get__NewEnum(IUnknown** enumerator) {
	AMBIENT_CHECK_PROXY();
	MANAH_VERIFY_POINTER(enumerator);
	const size_t c = impl().numberOfBuffers();
	VARIANT* buffers = new(nothrow) VARIANT[c];
	if(buffers == 0)
		return E_OUTOFMEMORY;
	for(size_t i = 0; i < c; ++i) {
		::VariantInit(buffers + i);
		buffers[i].vt = VT_DISPATCH;
		(buffers[i].pdispVal = impl().at(i).asScript().get())->AddRef();
	}
	manah::AutoBuffer<VARIANT> array(buffers);
	*enumerator = new(nothrow) IEnumVARIANTStaticImpl(array, c);
	if(*enumerator == 0) {
		for(size_t i = 0; i < c; ++i)
			::VariantClear(buffers + i);
		return E_OUTOFMEMORY;
	}
	(*enumerator)->AddRef();
	return S_OK;
}

/// @see IBufferList#get_Item
STDMETHODIMP BufferListProxy::get_Item(long index, IBuffer** buffer) {
	AMBIENT_CHECK_PROXY();
	MANAH_VERIFY_POINTER(buffer);
	if(index < 0 || static_cast<size_t>(index) >= impl().numberOfBuffers())
		return DISP_E_BADINDEX;
	return (*buffer = impl().at(index).asScript().get()), S_OK;
}

/// @see IBufferList#get_Length
STDMETHODIMP BufferListProxy::get_Length(long* length) {
	AMBIENT_CHECK_PROXY();
	MANAH_VERIFY_POINTER(length);
	if(impl().numberOfBuffers() > static_cast<size_t>(numeric_limits<long>::max()))
		return DISP_E_OVERFLOW;
	*length = static_cast<long>(impl().numberOfBuffers());
	return S_OK;
}

/// @see IBufferList#AddNew
STDMETHODIMP BufferListProxy::AddNew(BSTR name, BSTR encoding, Newline newline, IBuffer** result) {
	AMBIENT_CHECK_PROXY();
	try {
		Buffer& newInstance = impl().addNew((name != 0) ? name : L"",
			a::encoding::encodingNameFromUnicode((encoding != 0) ? encoding : L""), static_cast<k::Newline>(newline));
		if(result != 0)
			(*result = newInstance.asScript().get())->AddRef();
	} catch(invalid_argument&) {
		return E_INVALIDARG;
	}
	return S_OK;
}

/// @see IBufferList#AddNewDialog
STDMETHODIMP BufferListProxy::AddNewDialog(BSTR name, IBuffer** result) {
	AMBIENT_CHECK_PROXY();
	try {
		Buffer* const newInstance = impl().addNewDialog((name != 0) ? name : L"");
		if(result != 0) {
			if(newInstance != 0)
				(*result = newInstance->asScript().get())->AddRef();
			else
				*result = 0;
		}
	} catch(invalid_argument&) {
		return E_INVALIDARG;
	}
	return S_OK;
}

/// @see IBufferList#Open
STDMETHODIMP BufferListProxy::Open(BSTR fileName, BSTR encoding, FileLockMode lockMode, VARIANT_BOOL asReadOnly, IBuffer** result) {
	AMBIENT_CHECK_PROXY();
	return E_NOTIMPL;
}

/// @see IBufferList#OpenDialog
STDMETHODIMP BufferListProxy::OpenDialog(BSTR initialDirectory, VARIANT_BOOL* succeeded) {
	AMBIENT_CHECK_PROXY();
	const bool b = impl().openDialog((initialDirectory != 0) ? initialDirectory : L"");
	if(succeeded != 0)
		*succeeded = toVariantBoolean(b);
	return S_OK;
}

/// @see IBufferList#SaveSomeDialog
STDMETHODIMP BufferListProxy::SaveSomeDialog(VARIANT_BOOL* ok) {
	AMBIENT_CHECK_PROXY();
	const bool temp = impl().saveSomeDialog();
	if(ok != 0)
		*ok = toVariantBoolean(temp);
	return S_OK;
}
