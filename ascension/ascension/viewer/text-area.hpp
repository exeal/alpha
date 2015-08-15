/**
 * @file text-area.hpp
 * Defines @c TextArea class.
 * @author exeal
 * @date 2015-03-18 Created.
 */

#ifndef ASCENSION_TEXT_AREA_HPP
#define ASCENSION_TEXT_AREA_HPP
#include <ascension/graphics/font/text-renderer.hpp>
#include <ascension/graphics/font/text-viewport-listener.hpp>
#include <ascension/graphics/font/visual-lines-listener.hpp>
#include <ascension/kernel/document-observers.hpp>
#include <ascension/viewer/detail/weak-reference-for-points.hpp>
#include <ascension/viewer/text-viewer-component.hpp>

namespace ascension {
	namespace graphics {
		namespace font {
			class TextLayout;
		}
	}

	namespace viewer {
		class Caret;
		class TextAreaMouseInputStrategy;

		namespace widgetapi {
			class DropTarget;
		}

		class TextArea : public TextViewerComponent,
			public graphics::font::TextViewportListener, public graphics::font::VisualLinesListener,
			public kernel::DocumentListener, public detail::WeakReferenceForPoints<TextArea> {
		public:
			TextArea();
			~TextArea() BOOST_NOEXCEPT;

			/// @name Text Viewer
			/// @{
			BOOST_CONSTEXPR TextViewer& textViewer() BOOST_NOEXCEPT;
			BOOST_CONSTEXPR const TextViewer& textViewer() const BOOST_NOEXCEPT;
			/// @}

			/**
			 * @name Implementation of @c graphics#font#TextRenderer
			 * @{
			 */
			/// Implementation of @c graphics#font#TextRenderer for @c TextArea.
			class Renderer : public graphics::font::TextRenderer {
			public:
				explicit Renderer(TextViewer& viewer);
				Renderer(const Renderer& other, TextViewer& viewer);
				void displayShapingControls(bool display);
				bool displaysShapingControls() const BOOST_NOEXCEPT;
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
				void rewrapAtWindowEdge();
#endif // ASCENSION_ABANDONED_AT_VERSION_08
				// TextRenderer
				std::unique_ptr<const graphics::font::TextLayout> createLineLayout(Index line) const;
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
				graphics::Scalar width() const BOOST_NOEXCEPT;
#endif // ASCENSION_ABANDONED_AT_VERSION_08

			private:
				TextViewer& viewer_;
				bool displaysShapingControls_;
			};
			BOOST_CONSTEXPR Renderer& textRenderer() BOOST_NOEXCEPT;
			BOOST_CONSTEXPR const Renderer& textRenderer() const BOOST_NOEXCEPT;
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
			virtual void caretMoved(const Caret& caret, const kernel::Region& oldRegion);
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
			void paintCaret(graphics::PaintContext& context);
			// TextViewerComponent
			void install(TextViewer& viewer, const Locator& locator) override;
			void relocated() override;
			void uninstall(TextViewer& viewer) override;
			// graphics.font.TextViewportListener
			void viewportBoundsInViewChanged(const graphics::Rectangle& oldBounds) BOOST_NOEXCEPT override;
			void viewportScrollPositionChanged(
				const presentation::FlowRelativeTwoAxes<graphics::font::TextViewportScrollOffset>& positionsBeforeScroll,
				const graphics::font::VisualLine& firstVisibleLineBeforeScroll) BOOST_NOEXCEPT override;
			void viewportScrollPropertiesChanged(const presentation::FlowRelativeTwoAxes<bool>& changedDimensions) override;
			// graphics.font.VisualLinesListener
			void visualLinesDeleted(const boost::integer_range<Index>& lines,
				Index sublines, bool longestLineChanged) BOOST_NOEXCEPT override;
			void visualLinesInserted(const boost::integer_range<Index>& lines) BOOST_NOEXCEPT override;
			void visualLinesModified(const boost::integer_range<Index>& lines,
				SignedIndex sublinesDifference, bool documentChanged, bool longestLineChanged) BOOST_NOEXCEPT override;
			// kernel.DocumentListener
			void documentAboutToBeChanged(const kernel::Document& document) override;
			void documentChanged(const kernel::Document& document, const kernel::DocumentChange& change) override;
		private:
			TextViewer* viewer_;
			const Locator* locator_;
			std::unique_ptr<Renderer> renderer_;
			boost::integer_range<Index> linesToRedraw_;
			std::shared_ptr<TextAreaMouseInputStrategy> mouseInputStrategy_;
			bool mouseInputStrategyIsInstalled_;
			std::shared_ptr<widgetapi::DropTarget> dropTargetHandler_;
			boost::signals2::scoped_connection caretMotionConnection_,
				defaultFontChangedConnection_, matchBracketsChangedConnection_, selectionShapeChangedConnection_;
		};
		
		/// Returns the text renderer.
		inline TextArea::Renderer& TextArea::textRenderer() BOOST_NOEXCEPT {
			return *renderer_;
		}
		
		/// Returns the text renderer.
		inline const TextArea::Renderer& TextArea::textRenderer() const BOOST_NOEXCEPT {
			return *renderer_;
		}
		
		/// Returns the text viewer.
		inline TextViewer& TextArea::textViewer() BOOST_NOEXCEPT {
			return *viewer_;
		}
		
		/// Returns the text viewer.
		inline const TextViewer& TextArea::textViewer() const BOOST_NOEXCEPT {
			return *viewer_;
		}
	}
}

#endif // !ASCENSION_TEXT_AREA_HPP
