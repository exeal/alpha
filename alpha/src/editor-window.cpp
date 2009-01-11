/**
 * @file
 * @author exeal
 * @date 2008 (separated from buffer.cpp)
 * @date 2008-2009
 */

#include "editor-window.hpp"
#include "application.hpp"
#include "../resource/messages.h"
#include <ascension/text-editor.hpp>	// ascension.texteditor.commands.IncrementalSearchCommand
using namespace alpha;
using namespace ascension;
using namespace ascension::kernel;
using namespace ascension::kernel::fileio;
using namespace ascension::presentation;
using namespace ascension::searcher;
using namespace ascension::viewers;
using namespace manah::win32;
using namespace manah::win32::ui;
using namespace std;
using manah::toBoolean;

#pragma comment(lib, "msimg32.lib")


// EditorWindow /////////////////////////////////////////////////////////////

/// Constructor.
EditorWindow::EditorWindow(EditorView* initialView /* = 0 */) : visibleIndex_(-1), lastVisibleIndex_(-1) {
	if(initialView != 0) {
		visibleIndex_ = 0;
		addView(*initialView);
	}
//	self_.reset(new EditorWindowProxy);
}

/// Copy-constructor.
EditorWindow::EditorWindow(const EditorWindow& rhs) {
	for(size_t i = 0, c = rhs.views_.size(); i != c; ++i) {
		auto_ptr<EditorView> newViewer(new EditorView(*rhs.views_[i]));
		const bool succeeded = newViewer->create(rhs.views_[i]->getParent().getHandle(), DefaultWindowRect(),
			WS_CHILD | WS_CLIPCHILDREN | WS_HSCROLL | WS_VISIBLE | WS_VSCROLL, WS_EX_CLIENTEDGE);
		assert(succeeded);
		newViewer->setConfiguration(&rhs.views_[i]->configuration(), 0);
		newViewer->scrollTo(rhs.views_[i]->getScrollPosition(SB_HORZ), rhs.views_[i]->getScrollPosition(SB_VERT), false);
		views_.push_back(newViewer.release());
		if(i == rhs.visibleIndex_)
			visibleIndex_ = i;
		if(i == lastVisibleIndex_)
			lastVisibleIndex_ = i;
	}
//	self_.reset(new EditorWindowProxy);
}

/// Destructor.
EditorWindow::~EditorWindow() {
	removeAll();
}

/**
 * Adds the new viewer
 * @param view the viewer to add
 */
void EditorWindow::addView(EditorView& view) {
	views_.push_back(&view);
	if(views_.size() == 1)
		showBuffer(view.document());
}

/// Removes all viewers.
void EditorWindow::removeAll() {
	for(vector<EditorView*>::iterator i(views_.begin()), e(views_.end()); i != e; ++i)
		delete *i;
	views_.clear();
	visibleIndex_ = lastVisibleIndex_ = -1;
}

/**
 * Removes the viewer belongs to the specified buffer.
 * @param buffer the buffer has the viewer to remove
 */
void EditorWindow::removeBuffer(const Buffer& buffer) {
	for(size_t i = 0, c = views_.size(); i < c; ++i) {
		if(&views_[i]->document() == &buffer) {
			EditorView* const removing = views_[i];
			views_.erase(views_.begin() + i);
			if(i == visibleIndex_) {
				visibleIndex_ = -1;
				if(i == lastVisibleIndex_)
					lastVisibleIndex_ = -1;
				if(views_.size() == 1 || lastVisibleIndex_ == -1)
					showBuffer((*views_.begin())->document());
				else if(!views_.empty()) {
					showBuffer(views_[lastVisibleIndex_]->document());
					lastVisibleIndex_ = -1;
				}
			}
			delete removing;
			return;
		}
	}
}

/**
 * Shows the specified buffer in the pane.
 * @param buffer the buffer to show
 * @throw std#invalid_argument @a buffer is not exist
 */
void EditorWindow::showBuffer(const Buffer& buffer) {
	if(visibleIndex_ != -1 && &visibleBuffer() == &buffer)
		return;
	for(size_t i = 0, c = views_.size(); i < c; ++i) {
		if(&views_[i]->document() == &buffer) {
			const bool hadFocus = visibleIndex_ == -1 || visibleView().hasFocus();
			lastVisibleIndex_ = visibleIndex_;
			visibleIndex_ = i;
			EditorWindows::instance().adjustPanes();
			visibleView().show(SW_SHOW);
			if(lastVisibleIndex_ != -1)
				views_[lastVisibleIndex_]->show(SW_HIDE);
			if(hadFocus)
				visibleView().setFocus();
			return;
		}
	}
	throw invalid_argument("Specified buffer is not contained in the pane.");
}

/// Splits this window.
void EditorWindow::split() {
	EditorWindows::instance().splitNS(*this, *(new EditorWindow(*this)));
}

/// Splits this window side-by-side.
void EditorWindow::splitSideBySide() {
	EditorWindows::instance().splitWE(*this, *(new EditorWindow(*this)));
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
//	self_.reset(new EditorViewProxy(*this));
//	caretObject_.reset(new CaretProxy(caret()));
}

/// Copy-constructor.
EditorView::EditorView(const EditorView& rhs) : TextViewer(rhs), visualColumnStartValue_(rhs.visualColumnStartValue_) {
	document().textFile().addListener(*this);
	document().bookmarker().addListener(*this);
//	self_.reset(EditorViewProxy(*this));
//	caretObject_.reset(new CaretProxy(caret()));
}

/// Destructor.
EditorView::~EditorView() {
	document().textFile().removeListener(*this);
	document().bookmarker().removeListener(*this);
}

/// Begins incremental search.
void EditorView::beginIncrementalSearch(SearchType type, ascension::Direction direction) {
	TextSearcher& searcher = BufferList::instance().editorSession().textSearcher();
	SearchOptions options = searcher.options();
	options.type = type;
	searcher.setOptions(options);
	texteditor::commands::IncrementalFindCommand(*this, direction, this)();
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
		::GradientFill(dc.getHandle(), vertex, MANAH_COUNTOF(vertex), &mesh, 1, GRADIENT_FILL_RECT_H);
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
	const IncrementalSearcher& isearch = BufferList::instance().editorSession().incrementalSearcher();
	const bool forward = isearch.direction() == Direction::FORWARD;

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
	// ä˘íËÇÃÉLÅ[äÑÇËìñÇƒÇëSÇƒñ≥å¯Ç…Ç∑ÇÈ
//	handled = true;
	return TextViewer::onKeyDown(vkey, flags, handled);
}

/// @see Window#onKillFocus
void EditorView::onKillFocus(HWND newWindow) {
	TextViewer::onKillFocus(newWindow);
	BufferList::instance().editorSession().incrementalSearcher().end();
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
	StatusBar& statusBar = Alpha::instance().statusBar();
	if(statusBar.isWindow() && hasFocus()) {
		// build the current position indication string
		static manah::AutoBuffer<WCHAR> message, messageFormat;
		static size_t formatLength = 0;
		if(messageFormat.get() == 0) {
			void* messageBuffer;
			if(0 != ::FormatMessageW(
					FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE,
					::GetModuleHandle(0), MSG_STATUS__CARET_POSITION, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					reinterpret_cast<wchar_t*>(&messageBuffer), 0, 0)) {
				formatLength = wcslen(static_cast<WCHAR*>(messageBuffer));
				messageFormat.reset(new WCHAR[formatLength + 1]);
				wcscpy(messageFormat.get(), static_cast<::WCHAR*>(messageBuffer));
				::LocalFree(messageBuffer);
			} else {
				messageFormat.reset(new WCHAR[1]);
				messageFormat[0] = 0;
			}
			message.reset(new WCHAR[formatLength + 100]);
		}
		if(formatLength != 0) {
			length_t messageArguments[3];
			MANAH_AUTO_STRUCT(SCROLLINFO, si);
			getScrollInformation(SB_VERT, si, SIF_POS | SIF_RANGE);
			messageArguments[0] = caret().lineNumber() + verticalRulerConfiguration().lineNumbers.startValue;
			messageArguments[1] = caret().visualColumnNumber() + visualColumnStartValue_;
			messageArguments[2] = caret().columnNumber() + visualColumnStartValue_;
			::FormatMessageW(FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_FROM_STRING, messageFormat.get(),
				0, 0, message.get(), static_cast<DWORD>(formatLength) + 100, reinterpret_cast<va_list*>(messageArguments));
			// show in the status bar
			statusBar.setText(1, message.get());
		}
	}
}

void EditorView::updateNarrowingOnStatusBar() {
	StatusBar& statusBar = Alpha::instance().statusBar();
	if(statusBar.isWindow() && hasFocus()) {
		const bool narrow = document().isNarrowed();
		Alpha& app = Alpha::instance();
		if(narrowingIcon_.getHandle() == 0)
			narrowingIcon_.reset(static_cast<HICON>(app.loadImage(IDR_ICON_NARROWING, IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR)));
//		statusBar.setText(4, narrow ? app.loadMessage(MSG_STATUS__NARROWING).c_str() : L"");
		statusBar.setTipText(4, narrow ? app.loadMessage(MSG_STATUS__NARROWING).c_str() : L"");
		statusBar.setIcon(4, narrow ? narrowingIcon_.getHandle() : 0);
	}
}

void EditorView::updateOvertypeModeOnStatusBar() {
	StatusBar& statusBar = Alpha::instance().statusBar();
	if(statusBar.isWindow() && hasFocus())
		statusBar.setText(3,
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


// EditorWindows ////////////////////////////////////////////////////////////

/// Default constructor.
EditorWindows::EditorWindows() {
//	self_.reset(new EditorWindowsProxy(*this));
}

/// Returns the active buffer.
Buffer& EditorWindows::activeBuffer() {return activePane().visibleBuffer();}

/// Returns the active buffer.
const Buffer& EditorWindows::activeBuffer() const {return activePane().visibleBuffer();}

/// Returns the editor window at the given position.
EditorWindow& EditorWindows::at(size_t index) {
	return *windows_.at(index);
}

/// Returns the editor window at the given position.
const EditorWindow& EditorWindows::at(size_t index) const {
	return *windows_.at(index);
}

/// Returns true if the given @a pane belongs to the editor windows.
bool EditorWindows::contains(const EditorWindow& pane) const {
	return std::find(windows_.begin(), windows_.end(), &pane) != windows_.end();
}

/// Returns the singleton instance.
EditorWindows& EditorWindows::instance() {
	static EditorWindows singleton;
	return singleton;
}

/// @see Splitter#paneInserted
void EditorWindows::paneInserted(EditorWindow& pane) {
	windows_.push_back(&pane);
}

/// @see Splitter#paneRemoved
void EditorWindows::paneRemoved(EditorWindow& pane) {
	for(vector<EditorWindow*>::iterator i(windows_.begin()), e(windows_.end()); i != e; ++i) {
		if(*i == &pane) {
			windows_.erase(i);
			break;
		}
	}
}


STDMETHODIMP ambient::ScriptSystem::get_Windows(IWindowList** windows) {return E_NOTIMPL;}
