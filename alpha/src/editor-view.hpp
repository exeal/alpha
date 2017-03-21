/**
 * @file editor-view.hpp
 * @author exeal
 * @date 2009-2010, 2014
 * @date 2014-02-15 Separated from editor-window.hpp
 */

#ifndef ALPHA_EDITOR_VIEW_HPP
#define ALPHA_EDITOR_VIEW_HPP
#include <ascension/kernel/bookmarker.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/text-viewer.hpp>
#ifndef ALPHA_NO_AMBIENT
#	include <boost/python.hpp>
#endif

namespace alpha {
	class Buffer;

	/// A view of a text editor.
	class EditorView : public ascension::viewer::TextViewer, public ascension::kernel::BookmarkListener {
	public:
		// constructors
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
		explicit EditorView(std::shared_ptr<Buffer> buffer);
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
		explicit EditorView(std::shared_ptr<Buffer> buffer, QWidget* parent = Q_NULLPTR);
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
		???
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		explicit EditorView(std::shared_ptr<Buffer> buffer, const Type& type);
#endif
		~EditorView();
		// attributes
#ifndef ALPHA_NO_AMBIENT
		boost::python::object asCaret() const;
		boost::python::object asTextEditor() const;
#endif
		std::shared_ptr<Buffer> document() BOOST_NOEXCEPT;
		std::shared_ptr<const Buffer> document() const BOOST_NOEXCEPT;

	private:
		// ascension.viewer.TextViewer (overrides)
		void drawIndicatorMargin(ascension::Index line, ascension::graphics::PaintContext& context, const ascension::graphics::Rectangle& rect) override;
		void keyPressed(ascension::viewer::widgetapi::event::KeyInput& input) override;
		void focusAboutToBeLost(ascension::viewer::widgetapi::event::Event& event) override;
		void focusGained(ascension::viewer::widgetapi::event::Event& event) override;
		void realized() BOOST_NOEXCEPT override;
#if 0
		// ascension.viewer.Caret signals
		void characterInputted(const ascension::viewer::Caret& self, ascension::CodePoint c);
		void matchBracketsChanged(const ascension::viewer::Caret& self,
			const boost::optional<std::pair<ascension::kernel::Position, ascension::kernel::Position>>& previouslyMatchedBrackets, bool outsideOfView);
		void overtypeModeChanged(const ascension::viewer::Caret& self);
		void selectionShapeChanged(const ascension::viewer::Caret& self);
#endif
		// ascension.kernel.BookmarkListener
		void bookmarkChanged(ascension::Index line) override;
		void bookmarkCleared() override;
	private:
#ifndef ALPHA_NO_AMBIENT
		mutable boost::python::object asCaret_, asTextEditor_;
#endif
		std::shared_ptr<Buffer> buffer_;	// for .document
	};


#ifndef ALPHA_NO_AMBIENT
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
#endif

	/// @see ascension#viewer#TextViewer#document
	inline std::shared_ptr<Buffer> EditorView::document() BOOST_NOEXCEPT {
		return buffer_;
	}

	/// @see ascension#viewer#TextViewer#document
	inline std::shared_ptr<const Buffer> EditorView::document() const BOOST_NOEXCEPT {
		return buffer_;
	}
}

#endif // !ALPHA_EDITOR_VIEW_HPP
