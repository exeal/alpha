/**
 * @file editor-window.hpp
 * @author exeal
 * @date 2009
 */

#ifndef ALPHA_EDITOR_WINDOW_HPP
#define ALPHA_EDITOR_WINDOW_HPP
#include "ambient.hpp"
#include <ascension/viewer.hpp>
#include <ascension/searcher.hpp>	// ascension.searcher.IIncrementalSearchListener
#include <manah/win32/ui/splitter.hpp>

namespace alpha {
	class Buffer;

	/// A view of a text editor.
	class EditorView : public ascension::viewers::TextViewer,
		public ascension::kernel::IBookmarkListener, public ascension::searcher::IIncrementalSearchCallback {
	public:
		// constructors
		EditorView(ascension::presentation::Presentation& presentation);
		EditorView(const EditorView& rhs);
		~EditorView();
		// attributes
		boost::python::object asCaret() const;
		boost::python::object asTextEditor() const;
		const wchar_t* currentPositionString() const;
		Buffer& document() /*throw()*/;
		const Buffer& document() const /*throw()*/;
		ascension::length_t visualColumnStartValue() const /*throw()*/;
		void setVisualColumnStartValue() throw();
		// operations
		void beginIncrementalSearch(ascension::searcher::TextSearcher::Type type, ascension::Direction direction);
		// notification
		void updateStatusBar();

	private:
		// ascension.viewers.TextViewer (overrides)
		void drawIndicatorMargin(ascension::length_t line, manah::win32::gdi::DC& dc, const ::RECT& rect);
		// ascension.viewers.ICaretListener (overrides)
		void caretMoved(const ascension::viewers::Caret& self, const ascension::kernel::Region& oldRegion);
		void characterInputted(const ascension::viewers::Caret& self, ascension::CodePoint c);
		void matchBracketsChanged(const ascension::viewers::Caret& self,
			const std::pair<ascension::kernel::Position, ascension::kernel::Position>& oldPair, bool outsideOfView);
		void overtypeModeChanged(const ascension::viewers::Caret& self);
		void selectionShapeChanged(const ascension::viewers::Caret& self);
		// ascension.searcher.IIncrementalSearchCallback
		void incrementalSearchAborted(const ascension::kernel::Position& initialPosition);
		void incrementalSearchCompleted();
		void incrementalSearchPatternChanged(ascension::searcher::IIncrementalSearchCallback::Result result,
				const manah::Flags<ascension::searcher::IIncrementalSearchCallback::WrappingStatus>& wrappingStatus);
		void incrementalSearchStarted(const ascension::kernel::Document& document);
		// ascension.text.IBookmarkListener
		void bookmarkChanged(ascension::length_t line);
		void bookmarkCleared();
		// message handlers
		MANAH_DECLEAR_WINDOW_MESSAGE_MAP(EditorView);
		void onKeyDown(UINT vkey, UINT flags, bool& handled);
		void onKillFocus(HWND newWindow);
		void onSetFocus(HWND oldWindow);
	private:
		mutable boost::python::object asCaret_, asTextEditor_;
		ascension::length_t visualColumnStartValue_;
		static manah::win32::Object<HICON, ::DestroyIcon> narrowingIcon_;
	};

	/// A window pane for a text editor.
	class EditorWindow : public manah::win32::ui::AbstractPane {
		MANAH_UNASSIGNABLE_TAG(EditorWindow);
	public:
		// constructor
		EditorWindow(EditorView* initialView = 0);
		EditorWindow(const EditorWindow& rhs);
		~EditorWindow();
		// attributes
		std::size_t numberOfViews() const /*throw()*/;
		boost::python::object self() const;
		Buffer& visibleBuffer() const;
		EditorView& visibleView() const;
		// operations
		void addView(EditorView& view);
		void removeAll();
		void removeBuffer(const Buffer& buffer);
		void showBuffer(const Buffer& buffer);
		void split();
		void splitSideBySide();
		// manah.win32.ui.AbstractPane
		HWND getWindow() const /*throw()*/;
	private:
		mutable boost::python::object self_;
		std::vector<EditorView*> views_;
		std::size_t visibleIndex_, lastVisibleIndex_;
	};

	class IActiveBufferListener {
	private:
		/// The active buffer was switched.
		virtual void activeBufferSwitched() = 0;
		friend class EditorWindows;
	};

	/// A splittable editor window.
	class EditorWindows : public manah::win32::ui::Splitter<EditorWindow> {
	public:
		EditorWindows();
		Buffer& activeBuffer();
		const Buffer& activeBuffer() const;
		void addActiveBufferListener(IActiveBufferListener& listener);
		EditorWindow& at(std::size_t index);
		const EditorWindow& at(std::size_t index) const;
		bool contains(const EditorWindow& pane) const;
		static EditorWindows& instance();
		void removeActiveBufferListener(IActiveBufferListener& listener);
		boost::python::object self() const;
	private:
		void paneInserted(EditorWindow& pane);
		void paneRemoved(EditorWindow& pane);
	private:
		mutable boost::python::object self_;
		std::vector<EditorWindow*> windows_;
		std::list<IActiveBufferListener*> activeBufferListeners_;
	};


	/// Returns the script object corresponding to the text editor.
	inline boost::python::object EditorView::asCaret() const {
		if(asCaret_ == boost::python::object()) asCaret_ = boost::python::object(boost::python::ptr(&caret())); return asCaret_;}

	/// Returns the script object corresponding to the caret.
	inline boost::python::object EditorView::asTextEditor() const {
		if(asTextEditor_ == boost::python::object()) asTextEditor_ = boost::python::object(boost::python::ptr(this)); return asTextEditor_;}

	/// Returns the script object corresponding to the window.
	inline boost::python::object EditorWindow::self() const {
		if(self_ == boost::python::object()) self_ = boost::python::object(boost::python::ptr(this)); return self_;}

	/// @see ascension#viewers#TextViewer#document
	inline Buffer& EditorView::document() /*throw()*/ {return reinterpret_cast<Buffer&>(ascension::viewers::TextViewer::document());}

	/// @see ascension#viewers#TextViewer#document
	inline const Buffer& EditorView::document() const /*throw()*/ {
		return reinterpret_cast<const Buffer&>(ascension::viewers::TextViewer::document());}

	/// @see manah#windows#controls#AbstractPane#getWindow
	inline HWND EditorWindow::getWindow() const /*throw()*/ {return (visibleIndex_ != -1) ? views_[visibleIndex_]->get() : 0;}

	/// Returns the number of the viewers.
	inline std::size_t EditorWindow::numberOfViews() const /*throw()*/ {return views_.size();}

	/// Returns the visible buffer.
	inline Buffer& EditorWindow::visibleBuffer() const {return visibleView().document();}

	/// Returns the visible viewer.
	inline EditorView& EditorWindow::visibleView() const {
		if(visibleIndex_ == -1) throw std::logic_error("There no views."); return *views_[visibleIndex_];}

	/// Returns the script object corresponding to the windows.
	inline boost::python::object EditorWindows::self() const {
		if(self_ == boost::python::object()) self_ = boost::python::object(boost::python::ptr(this)); return self_;}

}

#endif // !ALPHA_EDITOR_WINDOW_HPP
