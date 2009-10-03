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
using namespace ambient;
using namespace ascension;
using namespace ascension::kernel;
using namespace ascension::kernel::fileio;
using namespace ascension::presentation;
using namespace ascension::searcher;
using namespace ascension::viewers;
using namespace manah;
using namespace manah::com;
using namespace std;
namespace py = boost::python;

#pragma comment(lib, "msimg32.lib")


// EditorWindow /////////////////////////////////////////////////////////////

/// Constructor.
EditorWindow::EditorWindow(EditorView* initialView /* = 0 */) : visibleIndex_(-1), lastVisibleIndex_(-1) {
	if(initialView != 0) {
		visibleIndex_ = 0;
		addView(*initialView);
	}
}

/// Copy-constructor.
EditorWindow::EditorWindow(const EditorWindow& rhs) {
	for(size_t i = 0, c = rhs.views_.size(); i != c; ++i) {
		auto_ptr<EditorView> newViewer(new EditorView(*rhs.views_[i]));
		const bool succeeded = newViewer->create(rhs.views_[i]->getParent()->use(), win32::ui::DefaultWindowRect(),
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

win32::Handle<HICON, ::DestroyIcon> EditorView::narrowingIcon_;

/// Constructor.
EditorView::EditorView(Presentation& presentation) : TextViewer(presentation), visualColumnStartValue_(1) {
	document().bookmarker().addListener(*this);
//	self_.reset(new EditorViewProxy(*this));
//	caretObject_.reset(new CaretProxy(caret()));
}

/// Copy-constructor.
EditorView::EditorView(const EditorView& rhs) : TextViewer(rhs), visualColumnStartValue_(rhs.visualColumnStartValue_) {
	document().bookmarker().addListener(*this);
//	self_.reset(EditorViewProxy(*this));
//	caretObject_.reset(new CaretProxy(caret()));
}

/// Destructor.
EditorView::~EditorView() {
	document().bookmarker().removeListener(*this);
}

/// Begins incremental search.
void EditorView::beginIncrementalSearch(TextSearcher::Type type, ascension::Direction direction) {
	texteditor::commands::IncrementalFindCommand(*this, type, direction, this)();
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
	if(&EditorWindows::instance().activePane().visibleView() == this)
		Alpha::instance().statusBar().updateCaretPosition();
}

/// @see TextViewer#drawIndicatorMargin
void EditorView::drawIndicatorMargin(length_t line, manah::win32::gdi::DC& dc, const ::RECT& rect) {
	if(document().bookmarker().isMarked(line)) {
		// draw a bookmark indication mark
		const COLORREF selColor = ::GetSysColor(COLOR_HIGHLIGHT);
		const COLORREF selTextColor = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
//		dc.fillSolidRect(rect, selectionColor);
		TRIVERTEX vertex[2];
		vertex[0].x = rect.left + 2;
		vertex[0].y = (rect.top * 2 + rect.bottom) / 3;
		vertex[0].Red = static_cast<COLOR16>((selColor >> 0) & 0xFF) << 8;
		vertex[0].Green = static_cast<COLOR16>((selColor >> 8) & 0xFF) << 8;
		vertex[0].Blue = static_cast<COLOR16>((selColor >> 16) & 0xFF) << 8;
		vertex[0].Alpha = 0x0000;
		vertex[1].x = rect.right - 2;
		vertex[1].y = (rect.top + rect.bottom * 2) / 3;
		vertex[1].Red = static_cast<COLOR16>((selTextColor >> 0) & 0xFF) << 8;
		vertex[1].Green = static_cast<COLOR16>((selTextColor >> 8) & 0xFF) << 8;
		vertex[1].Blue = static_cast<COLOR16>((selTextColor >> 16) & 0xFF) << 8;
		vertex[1].Alpha = 0x0000;
		GRADIENT_RECT mesh;
		mesh.UpperLeft = 0;
		mesh.LowerRight = 1;
		::GradientFill(dc.get(), vertex, MANAH_COUNTOF(vertex), &mesh, 1, GRADIENT_FILL_RECT_H);
	}
}

/// @see IIncrementalSearchListener#incrementalSearchAborted
void EditorView::incrementalSearchAborted(const Position& initialPosition) {
	incrementalSearchCompleted();
	caret().moveTo(initialPosition);
}

/// @see IIncrementalSearchListener#incrementalSearchCompleted
void EditorView::incrementalSearchCompleted() {
	Alpha::instance().statusBar().setText(0);
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
		app.statusBar().setText(app.loadMessage(messageID).c_str(),
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
	app.statusBar().setText(prompt.c_str(),
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
	// disable default keyboard bindings
	handled = true;
//	return TextViewer::onKeyDown(vkey, flags, handled);
}

/// @see Window#onKillFocus
void EditorView::onKillFocus(HWND newWindow) {
	TextViewer::onKillFocus(newWindow);
	BufferList::instance().editorSession().incrementalSearcher().end();
}

/// @see Window#onSetFocus
void EditorView::onSetFocus(HWND oldWindow) {
	TextViewer::onSetFocus(oldWindow);
	BufferList::instance().activeBufferChanged();
}

/// @see ICaretListener#overtypeModeChanged
void EditorView::overtypeModeChanged(const Caret&) {
	if(&EditorWindows::instance().activePane().visibleView() == this)
		Alpha::instance().statusBar().updateOvertypeMode();
}

/// @see ICaretListener#selectionShapeChanged
void EditorView::selectionShapeChanged(const Caret&) {
}


// EditorWindows ////////////////////////////////////////////////////////////

/// Default constructor.
EditorWindows::EditorWindows() {
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


namespace {
	void activateWindow(EditorWindow& window) {
		if(::SetFocus(window.getWindow()) == 0)
			Interpreter::instance().raiseLastWin32Error();
	}
	py::object activeBuffer() {
		return EditorWindows::instance().activeBuffer().self();
	}
	py::object activeWindow() {
		return EditorWindows::instance().activePane().self();
	}

	py::object bufferOfPoint(const Point& p) {
		return static_cast<const Buffer&>(p.document()).self();
	}
	py::object bufferOfTextEditor(const EditorView& editor) {
		return editor.document().self();
	}
	void closeWindow(EditorWindow& window) {
		activateWindow(window);
		EditorWindows::instance().removeActivePane();
	}

	void extendSelection(Caret& caret, py::object to) {
		if(py::extract<const Position&>(to).check())
			caret.extendSelection(static_cast<Position>(py::extract<Position>(to)));
		else if(py::extract<const VerticalDestinationProxy&>(to).check())
			caret.extendSelection(static_cast<VerticalDestinationProxy>(py::extract<VerticalDestinationProxy>(to)));
		else if(py::extract<const Point&>(to).check())
			caret.extendSelection(static_cast<const Point&>(py::extract<const Point&>(to)).position());
		else {
			::PyErr_BadArgument();
			py::throw_error_already_set();
		}
	}

	py::ssize_t numberOfVisibleColumns(const EditorWindow& window) {return window.visibleView().numberOfVisibleColumns();}

	py::ssize_t numberOfVisibleLines(const EditorWindow& window) {return window.visibleView().numberOfVisibleLines();}

	template<const VisualPoint& (Caret::*procedure)() const>
	Position positionOfCaret(const Caret& c) {return (c.*procedure)().position();}

	void scroll(EditorWindow& window, py::ssize_t lines, py::ssize_t columns) {
		window.visibleView().scroll(static_cast<int>(columns), static_cast<int>(lines), true);
	}

	py::object selectedBuffer(const EditorWindow& window) {
		return window.visibleBuffer().self();
	}
	py::object selectedTextEditor(const EditorWindow& window) {
		return window.visibleView().asTextEditor();
	}
	void selectInWindow(EditorWindow& window, py::object o) {
		Buffer* buffer = 0;	// buffer to select
		if(toBoolean(PyUnicode_Check(o.ptr()))) {
			const wstring name(ambient::convertUnicodeObjectToWideString(o.ptr()));
			py::object buf(BufferList::instance().forFileName(name));
			if(buf == py::object()) {
				::PyErr_BadArgument();
				py::throw_error_already_set();
			}
			buffer = static_cast<Buffer*>(py::extract<Buffer*>(buf));
		} else if(py::extract<Buffer*>(o).check())
			buffer = static_cast<Buffer*>(py::extract<Buffer*>(o));
		else if(py::extract<EditorView&>(o).check())
			buffer = &static_cast<EditorView&>(py::extract<EditorView&>(o)).document();
		else {
			::PyErr_BadArgument();
			py::throw_error_already_set();
		}
		window.showBuffer(*buffer);
	}
	py::object windowAt(const EditorWindows& windows, py::ssize_t at) {
		for(EditorWindows::Iterator i(windows.enumeratePanes()); !i.done(); i.next(), --at) {
			if(at == 0)
				return i.get().self();
		}
		::PyErr_BadArgument();
		py::throw_error_already_set();
		return py::object();
	}
	py::object windows() {
		return EditorWindows::instance().self();
	}
}

ALPHA_EXPOSE_PROLOGUE(2)
	py::scope temp(Interpreter::instance().toplevelPackage());

	py::class_<VerticalDestinationProxy>("_VerticalDestinationProxy", py::no_init);
	py::class_<Point>("Point", py::init<Buffer&, const Position&>())
		.add_property("adapts_to_buffer", &Point::adaptsToDocument,
			py::make_function(&Point::adaptToDocument, py::return_value_policy<py::reference_existing_object>()))
		.add_property("buffer", &bufferOfPoint)
		.add_property("column", &Point::column)
//		.add_property("excluded_from_restriction", &Point::isExcludedFromRestriction,
//			py::make_function(&Point::excludeFromRestriction, py::return_value_policy<py::reference_existing_object>()))
		.add_property("gravity", &Point::gravity,
			py::make_function(&Point::setGravity, py::return_value_policy<py::reference_existing_object>()))
		.add_property("line", &Point::line)
		.add_property("position", py::make_function(&Point::position, py::return_value_policy<py::copy_const_reference>()))
		.def("is_buffer_deleted", &Point::isDocumentDisposed)
		.def<void (Point::*)(const Position&)>("move_to", &Point::moveTo);
	py::class_<Caret, py::bases<Point>, boost::noncopyable>("_Caret", py::no_init)
		.add_property("anchor", &positionOfCaret<&Caret::anchor>)
		.add_property("beginning", &positionOfCaret<&Caret::beginning>)
		.add_property("end", &positionOfCaret<&Caret::end>)
		.add_property("selected_region", &Caret::selectedRegion)
		.def("begin_rectangle_selection", &Caret::beginRectangleSelection)
		.def("can_paste", &Caret::canPaste, py::arg("use_killring") = false)
		.def("clear_selection", &Caret::clearSelection)
		.def("copy_selection", &copySelection)
		.def("cut_selection", &cutSelection)
		.def("delete_selection", &eraseSelection)
		.def("end_rectangle_selection", &Caret::endRectangleSelection)
		.def("extend_selection", &extendSelection)
		.def("input_character", &Caret::inputCharacter,
			(py::arg("character"), py::arg("validate_sequence") = true, py::arg("block_controls") = true))
		.def("is_overtype_mode", &Caret::isOvertypeMode)
		.def("is_selection_empty", &isSelectionEmpty)
		.def("is_selection_rectangle", &Caret::isSelectionRectangle)
		.def("paste", &Caret::paste, py::arg("use_killring") = false)
		.def("replace_selection", &replaceSelection, (py::arg("text"), py::arg("rectangle_insertion") = false))
		.def<void (Caret::*)(const Region&)>("select", &Caret::select)
		.def("select_word", &selectWord)
		.def<ascension::String (*)(const Caret&, Newline)>("selected_string", &selectedString, py::arg("newline") = NLF_RAW_VALUE)
		.def("set_overtype_mode", &Caret::setOvertypeMode, py::arg("set") = true, py::return_value_policy<py::reference_existing_object>())
/*		.def("show_automatically", &Caret::showAutomatically)
		.def("shows_automatically", &Caret::showsAutomatically)*/;
	py::class_<EditorView, boost::noncopyable>("_TextEditor", py::no_init)
		.add_property("buffer", &bufferOfTextEditor)
		.add_property("caret", &EditorView::asCaret);
	py::class_<EditorWindow, boost::noncopyable>("_Window", py::no_init)
		.add_property("number_of_visible_columns", &numberOfVisibleColumns)
		.add_property("number_of_visible_lines", &numberOfVisibleLines)
		.add_property("selected_buffer", &selectedBuffer)
		.add_property("selected_editor", &selectedTextEditor)
		.def("activate", &activateWindow)
		.def("close", &closeWindow)
		.def("scroll", &scroll)
		.def("select", &selectInWindow)
		.def("split", &EditorWindow::split)
		.def("split_side_by_side", &EditorWindow::splitSideBySide);
	py::class_<EditorWindows, boost::noncopyable>("_WindowList", py::no_init)
//		.def("__contains__", &)
		.def("__getitem__", &windowAt)
//		.def("__iter__", &)
		.def("__len__", &EditorWindows::numberOfPanes)
		.def("activate_next", &EditorWindows::activateNextPane)
		.def("activate_previous", &EditorWindows::activatePreviousPane)
		.def("unsplit_all", &EditorWindows::removeInactivePanes);

	py::def("active_window", &activeWindow);
	py::def("windows", &windows);
ALPHA_EXPOSE_EPILOGUE()
