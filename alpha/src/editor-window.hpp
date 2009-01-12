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
		manah::com::ComPtr<ITextEditor> asScript() const;
		manah::com::ComPtr<ICaret> caretObject() const;
		const wchar_t* currentPositionString() const;
		Buffer& document() /*throw()*/;
		const Buffer& document() const /*throw()*/;
		ascension::length_t visualColumnStartValue() const /*throw()*/;
		void setVisualColumnStartValue() throw();
		// operations
		void beginIncrementalSearch(ascension::searcher::SearchType type, ascension::Direction direction);
		// notification
		void updateStatusBar();

	private:
		void updateCurrentPositionOnStatusBar();
		void updateNarrowingOnStatusBar();
		void updateOvertypeModeOnStatusBar();
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
		manah::com::ComPtr<ITextEditor> self_;
		manah::com::ComPtr<ICaret> caretObject_;
		ascension::length_t visualColumnStartValue_;
		static manah::win32::Handle<HICON, ::DestroyIcon> narrowingIcon_;
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
		manah::com::ComPtr<IWindow> asScript() const;
		std::size_t numberOfViews() const /*throw()*/;
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
		manah::com::ComPtr<IWindow> self_;
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
		manah::com::ComPtr<IWindowList> asScript() const;
		EditorWindow& at(std::size_t index);
		const EditorWindow& at(std::size_t index) const;
		bool contains(const EditorWindow& pane) const;
		static EditorWindows& instance();
		void removeActiveBufferListener(IActiveBufferListener& listener);
	private:
		void paneInserted(EditorWindow& pane);
		void paneRemoved(EditorWindow& pane);
	private:
		manah::com::ComPtr<IWindowList> self_;
		std::vector<EditorWindow*> windows_;
		std::list<IActiveBufferListener*> activeBufferListeners_;
	};


	/// Returns the script object corresponding to the text editor.
	inline manah::com::ComPtr<ITextEditor> EditorView::asScript() const {return self_;}

	/// Returns the script object corresponding to the caret.
	inline manah::com::ComPtr<ICaret> EditorView::caretObject() const {return caretObject_;}

	/// Returns the script object corresponding to the window.
	inline manah::com::ComPtr<IWindow> EditorWindow::asScript() const {return self_;}

	/// @see ascension#viewers#TextViewer#document
	inline Buffer& EditorView::document() /*throw()*/ {return reinterpret_cast<Buffer&>(ascension::viewers::TextViewer::document());}

	/// @see ascension#viewers#TextViewer#document
	inline const Buffer& EditorView::document() const /*throw()*/ {
		return reinterpret_cast<const Buffer&>(ascension::viewers::TextViewer::document());}

	/// @see manah#windows#controls#AbstractPane#getWindow
	inline HWND EditorWindow::getWindow() const /*throw()*/ {return (visibleIndex_ != -1) ? views_[visibleIndex_]->getHandle() : 0;}

	/// Returns the number of the viewers.
	inline std::size_t EditorWindow::numberOfViews() const /*throw()*/ {return views_.size();}

	/// Returns the visible buffer.
	inline Buffer& EditorWindow::visibleBuffer() const {return visibleView().document();}

	/// Returns the visible viewer.
	inline EditorView& EditorWindow::visibleView() const {
		if(visibleIndex_ == -1) throw std::logic_error("There no views."); return *views_[visibleIndex_];}

	/// Returns the script object corresponding to the windows.
	inline manah::com::ComPtr<IWindowList> EditorWindows::asScript() const {return self_;}

}

#endif // !ALPHA_EDITOR_WINDOW_HPP
