/**
 * @file editor-view.hpp
 * @author exeal
 * @date 2009-2010, 2014
 * @date 2014-02-15 Separated from editor-window.hpp
 */

#ifndef ALPHA_EDITOR_VIEW_HPP
#define ALPHA_EDITOR_VIEW_HPP
#include "ambient.hpp"
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/viewer.hpp>
#include <ascension/kernel/searcher.hpp>	// ascension.searcher.IncrementalSearchCallback

namespace alpha {
	class Buffer;

	/// A view of a text editor.
	class EditorView : public ascension::viewer::TextViewer,
		public ascension::kernel::BookmarkListener, public ascension::searcher::IncrementalSearchCallback {
	public:
		// constructors
		EditorView(ascension::presentation::Presentation& presentation);
		EditorView(const EditorView& rhs);
		~EditorView();
		// attributes
		boost::python::object asCaret() const;
		boost::python::object asTextEditor() const;
		const wchar_t* currentPositionString() const;
		Buffer& document() BOOST_NOEXCEPT;
		const Buffer& document() const BOOST_NOEXCEPT;
		ascension::Index visualColumnStartValue() const /*throw()*/;
		void setVisualColumnStartValue() throw();
		// operations
		void beginIncrementalSearch(ascension::searcher::TextSearcher::Type type, ascension::Direction direction);
		// notification
		void updateStatusBar();

	private:
		// ascension.viewer.TextViewer (overrides)
		void drawIndicatorMargin(ascension::Index line, ascension::graphics::PaintContext& context, const ascension::graphics::Rectangle& rect) override;
		void keyPressed(ascension::viewer::widgetapi::event::KeyInput& input) override;
		void focusAboutToBeLost(ascension::viewer::widgetapi::event::Event& event) override;
		void focusGained(ascension::viewer::widgetapi::event::Event& event) override;
#if 0
		// ascension.viewer.Caret signals
		void characterInputted(const ascension::viewer::Caret& self, ascension::CodePoint c);
		void matchBracketsChanged(const ascension::viewer::Caret& self,
			const boost::optional<std::pair<ascension::kernel::Position, ascension::kernel::Position>>& previouslyMatchedBrackets, bool outsideOfView);
		void overtypeModeChanged(const ascension::viewer::Caret& self);
		void selectionShapeChanged(const ascension::viewer::Caret& self);
#endif
		// ascension.searcher.IncrementalSearchCallback
		void incrementalSearchAborted(const ascension::kernel::Position& initialPosition);
		void incrementalSearchCompleted();
		void incrementalSearchPatternChanged(ascension::searcher::IncrementalSearchCallback::Result result, int wrappingStatus);
		void incrementalSearchStarted(const ascension::kernel::Document& document);
		// ascension.kernel.BookmarkListener
		void bookmarkChanged(ascension::Index line);
		void bookmarkCleared();
	private:
		mutable boost::python::object asCaret_, asTextEditor_;
		ascension::Index visualColumnStartValue_;
	};


	/// Returns the script object corresponding to the text editor.
	inline boost::python::object EditorView::asCaret() const {
		if(asCaret_ == boost::python::object())
			asCaret_ = boost::python::object(boost::python::ptr(&caret()));
		return asCaret_;
	}

	/// Returns the script object corresponding to the caret.
	inline boost::python::object EditorView::asTextEditor() const {
		if(asTextEditor_ == boost::python::object())
			asTextEditor_ = boost::python::object(boost::python::ptr(this));
		return asTextEditor_;
	}

	/// @see ascension#viewer#TextViewer#document
	inline Buffer& EditorView::document() BOOST_NOEXCEPT {
		return reinterpret_cast<Buffer&>(ascension::viewer::TextViewer::document());
	}

	/// @see ascension#viewer#TextViewer#document
	inline const Buffer& EditorView::document() const BOOST_NOEXCEPT {
		return reinterpret_cast<const Buffer&>(ascension::viewer::TextViewer::document());
	}
}

#endif // !ALPHA_EDITOR_VIEW_HPP
