/**
 * @file source-viewer.hpp
 * Defines @c SourceViewer class.
 * @author exeal
 * @date 2015-03-01 Created.
 */

#ifndef ASCENSION_SOURCE_VIEWER_HPP
#define ASCENSION_SOURCE_VIEWER_HPP
#include <ascension/graphics/font/text-alignment.hpp>
#include <ascension/graphics/physical-directions-dimensions.hpp>
#include <ascension/viewer/text-viewer.hpp>
#include <ascension/viewer/source/ruler-allocation-width-sink.hpp>
#include <ascension/viewer/source/ruler-locator.hpp>

namespace ascension {
	namespace viewer {
		/// Provides stuffs for source code editors.
		namespace source {
			class Ruler;
			class CompositeRuler;

			class SourceViewer : public TextViewer, private RulerAllocationWidthSink {
			public:
				virtual const TextViewerComponent* hitTest(const graphics::Point& location) const BOOST_NOEXCEPT override;

				/// @name Ruler
				/// @{
				std::shared_ptr<source::Ruler>& ruler() const BOOST_NOEXCEPT;
				graphics::font::TextAlignment rulerAbstractAlignment() const BOOST_NOEXCEPT;
				graphics::PhysicalDirection rulerPhysicalAlignment() const BOOST_NOEXCEPT;
				void setRuler(std::unique_ptr<source::Ruler> ruler);
				void setRuler(std::unique_ptr<source::CompositeRuler> ruler);
				void setRulerAlignment(graphics::font::TextAlignment alignment);
				/// @}

			protected:
				// TextViewerComponent.Locator
				virtual graphics::Rectangle locateComponent(const TextViewerComponent& ruler) const override;

				/// @name Overridable Widget Events
				/// @{
				virtual void keyPressed(widgetapi::event::KeyInput& input) override;
				virtual void keyReleased(widgetapi::event::KeyInput& input) override;
				virtual void paint(graphics::PaintContext& context) override;
				virtual void resized(const graphics::Dimension& newSize) override;
				/// @}

			private:
				// RulerAllocationWidthSink
				void updateRulerAllocationWidth(const Ruler& ruler) override;
			private:
				std::unique_ptr<Ruler> ruler_;
				bool rulerIsComposite_;
				graphics::font::TextAlignment rulerAbstractAlignment_;
				graphics::PhysicalDirection rulerPhysicalAlignment_;
			};
		}
	}
}

#endif // !ASCENSION_SOURCE_VIEWER_HPP
