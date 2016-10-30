/**
 * @file text-area.hpp
 * Defines @c TextArea class.
 * @author exeal
 * @date 2015-03-18 Created.
 */

#ifndef ASCENSION_TEXT_AREA_HPP
#define ASCENSION_TEXT_AREA_HPP
#include <ascension/corelib/signals.hpp>
#include <ascension/graphics/font/line-rendering-options.hpp>
#include <ascension/graphics/font/text-renderer.hpp>
#include <ascension/graphics/font/text-viewport-base.hpp>
#include <ascension/graphics/font/visual-lines-listener.hpp>
#include <ascension/kernel/document-observers.hpp>
#include <ascension/presentation/flow-relative-two-axes.hpp>
#include <ascension/viewer/caret-painter.hpp>
#include <ascension/viewer/detail/weak-reference-for-points.hpp>
#include <ascension/viewer/text-viewer-component.hpp>

namespace ascension {
	namespace graphics {
		namespace font {
			class TextLayout;
			class TextRenderer;
		}
	}

	namespace viewer {
		class SelectedRegion;
		class TextAreaMouseInputStrategy;

		namespace widgetapi {
			class DropTarget;
		}

		class TextArea :
			public TextViewerComponent, public graphics::font::VisualLinesListener,
			public kernel::DocumentListener, public detail::WeakReferenceForPoints<TextArea>,
			private graphics::font::LineRenderingOptions, private boost::noncopyable {
		public:
			TextArea();
			~TextArea() BOOST_NOEXCEPT;

			/// @name Text Viewer
			/// @{
			/* BOOST_CONSTEXPR */ TextViewer& textViewer() BOOST_NOEXCEPT;
			BOOST_CONSTEXPR const TextViewer& textViewer() const BOOST_NOEXCEPT;
			/// @}

			/// @name Caret
			/// @{
			std::shared_ptr<Caret> caret() BOOST_NOEXCEPT;
			std::shared_ptr<const Caret> caret() const BOOST_NOEXCEPT;
			void hideCaret() BOOST_NOEXCEPT;
			void setCaretPainter(std::unique_ptr<CaretPainter> newCaretPainter) BOOST_NOEXCEPT;
			void showCaret() BOOST_NOEXCEPT;
			BOOST_CONSTEXPR bool showsCaret() const BOOST_NOEXCEPT;
			/// @}

			/// @name Geometry
			/// @{
			graphics::Rectangle allocationRectangle() const BOOST_NOEXCEPT;
			graphics::Rectangle contentRectangle() const BOOST_NOEXCEPT;
			typedef boost::signals2::signal<void(const TextArea&)> GeometryChangedSignal;
			SignalConnector<GeometryChangedSignal> allocationRectangleChangedSignal() BOOST_NOEXCEPT;
			SignalConnector<GeometryChangedSignal> contentRectangleChangedSignal() BOOST_NOEXCEPT;
			/// @}

			/// @name Text Renderer
			/// @{
			void setTextRenderer(std::unique_ptr<graphics::font::TextRenderer> newTextRenderer);
			std::shared_ptr<graphics::font::TextRenderer> textRenderer() BOOST_NOEXCEPT;
			std::shared_ptr<const graphics::font::TextRenderer> textRenderer() const BOOST_NOEXCEPT;
			/// @}

			/// @name Text Viewport
			/// @{
			std::shared_ptr<graphics::font::TextViewport> viewport() BOOST_NOEXCEPT;
			std::shared_ptr<const graphics::font::TextViewport> viewport() const BOOST_NOEXCEPT;
			/// @}

			/// @name Redraw
			/// @{
			void redrawLine(Index line, bool following = false);
			void redrawLines(const boost::integer_range<Index>& lines);
			/// @}

			/// @name Listeners and Strategies
			/// @{
			void setMouseInputStrategy(std::unique_ptr<TextAreaMouseInputStrategy> newStrategy);
			/// @}

			// TextViewerComponent
			std::weak_ptr<MouseInputStrategy> mouseInputStrategy() const override;
			void paint(graphics::PaintContext& context) override;

		protected:
			/// @name Overridable @c Caret Signal Slots
			/// @{
			virtual void caretMoved(const Caret& caret, const SelectedRegion& oldRegion);
			virtual void matchBracketsChanged(const Caret& caret,
				const boost::optional<std::pair<kernel::Position, kernel::Position>>& previouslyMatchedBrackets,
				bool outsideOfView);
			virtual void selectionShapeChanged(const Caret& caret);
			/// @}

			/// @name Overridable @c TextViewer Signal Slots
			/// @{
			virtual void focusChanged(const TextViewer& viewer);
			virtual void frozenStateChanged(const TextViewer& viewer);
			/// @}

			/// @ name Overridable Signal Slots
			/// @{
//			virtual void computedTextToplevelStyleChanged(
//				const presentation::Presentation& presentation,
//				const presentation::DeclaredTextToplevelStyle& previouslyDeclared,
//				const presentation::ComputedTextToplevelStyle& previouslyComputed);
			virtual void defaultFontChanged(const graphics::font::TextRenderer& textRenderer);
			/// @}

		private:
			std::unique_ptr<graphics::font::TextRenderer> createDefaultTextRenderer();
			void installTextRenderer();
			void uninstallTextRenderer();
			// TextViewerComponent
			void install(TextViewer& viewer, const Locator& locator) override;
			void relocated() override;
			void uninstall(TextViewer& viewer) override;
			// graphics.font.TextViewport signals
			void viewportResized(const graphics::Dimension& oldSize) BOOST_NOEXCEPT;
			void viewportScrolled(
				const presentation::FlowRelativeTwoAxes<graphics::font::TextViewportBase::ScrollOffset>& positionsBeforeScroll,
				const graphics::font::VisualLine& firstVisibleLineBeforeScroll) BOOST_NOEXCEPT;
			// graphics.font.VisualLinesListener
			void visualLinesDeleted(const boost::integer_range<Index>& lines,
				Index sublines, bool longestLineChanged) BOOST_NOEXCEPT override;
			void visualLinesInserted(const boost::integer_range<Index>& lines) BOOST_NOEXCEPT override;
			void visualLinesModified(const boost::integer_range<Index>& lines,
				SignedIndex sublinesDifference, bool documentChanged, bool longestLineChanged) BOOST_NOEXCEPT override;
			// kernel.DocumentListener
			void documentAboutToBeChanged(const kernel::Document& document) override;
			void documentChanged(const kernel::Document& document, const kernel::DocumentChange& change) override;
			// graphics.font.LineRenderingOptions
			std::unique_ptr<const graphics::font::InlineObject> endOfLine(Index line) const BOOST_NOEXCEPT override;
			void overrideTextPaint(Index line, std::vector<const graphics::font::OverriddenSegment>& segments) const BOOST_NOEXCEPT override;
			std::unique_ptr<const graphics::font::InlineObject> textWrappingMark(Index line) const BOOST_NOEXCEPT override;
		private:
			TextViewer* viewer_;
			const Locator* locator_;
			std::shared_ptr<Caret> caret_;
			std::shared_ptr<graphics::font::TextRenderer> renderer_;
			std::shared_ptr<graphics::font::TextViewport> viewport_;
			std::unique_ptr<CaretPainter> caretPainter_;
			boost::integer_range<Index> linesToRedraw_;
			std::shared_ptr<TextAreaMouseInputStrategy> mouseInputStrategy_;
			bool mouseInputStrategyIsInstalled_;
			std::shared_ptr<widgetapi::DropTarget> dropTargetHandler_;
			GeometryChangedSignal allocationRectangleChangedSignal_, contentRectangleChangedSignal_;
			boost::signals2::connection viewportResizedConnection_, viewportScrolledConnection_;
			boost::signals2::scoped_connection viewerFocusChangedConnection_, viewerFrozenStateChangedConnection_,
				caretMotionConnection_, defaultFontChangedConnection_, matchBracketsChangedConnection_, selectionShapeChangedConnection_;
		};

		/// @addtogroup shortcuts_to_main_objects
		/// @{
		kernel::Document& document(TextArea& textArea);
		const kernel::Document& document(const TextArea& textArea);
		/// @}

		
		/// Returns the caret, or @c nullptr if not installed.
		inline std::shared_ptr<Caret> TextArea::caret() BOOST_NOEXCEPT {
			return caret_;
		}
		
		/// Returns the caret, or @c nullptr if not installed.
		inline std::shared_ptr<const Caret> TextArea::caret() const BOOST_NOEXCEPT {
			return caret_;
		}

		/**
		 * Returns @c true if the caret is shown.
		 * @see #hideCaret, showCaret
		 */
		inline BOOST_CONSTEXPR bool TextArea::showsCaret() const BOOST_NOEXCEPT {
			return caretPainter_.get() != nullptr && static_cast<const detail::CaretPainterBase&>(*caretPainter_).shows();
		}
		
		/// Returns the text renderer, or @c nullptr if not installed.
		inline std::shared_ptr<graphics::font::TextRenderer> TextArea::textRenderer() BOOST_NOEXCEPT {
			return renderer_;
		}
		
		/// Returns the text renderer, or @c nullptr if not installed.
		inline std::shared_ptr<const graphics::font::TextRenderer> TextArea::textRenderer() const BOOST_NOEXCEPT {
			return renderer_;
		}
		
		/// Returns the text viewer.
		inline /* BOOST_CONSTEXPR */ TextViewer& TextArea::textViewer() BOOST_NOEXCEPT {
			return *viewer_;
		}
		
		/// Returns the text viewer.
		inline BOOST_CONSTEXPR const TextViewer& TextArea::textViewer() const BOOST_NOEXCEPT {
			return *viewer_;
		}
		
		/// Returns the text viewport, or @c nullptr if not installed.
		inline std::shared_ptr<graphics::font::TextViewport> TextArea::viewport() BOOST_NOEXCEPT {
			return viewport_;
		}
		
		/// Returns the text viewport, or @c nullptr if not installed.
		inline std::shared_ptr<const graphics::font::TextViewport> TextArea::viewport() const BOOST_NOEXCEPT {
			return viewport_;
		}
	}
}

#endif // !ASCENSION_TEXT_AREA_HPP
