/**
 * @file source-viewer.hpp
 * Defines @c SourceViewer class.
 * @author exeal
 * @date 2015-03-01 Created.
 */

#ifndef ASCENSION_SOURCE_VIEWER_HPP
#define ASCENSION_SOURCE_VIEWER_HPP
#include <ascension/viewer/text-viewer.hpp>
#include <ascension/viewer/source/ruler-allocation-width-sink.hpp>
#include <ascension/viewer/source/ruler-locator.hpp>

namespace ascension {
	namespace viewer {
		/// Provides stuffs for source code editors.
		namespace source {
			class Ruler;

			class SourceViewer : public TextViewer, private RulerAllocationWidthSink, private RulerLocator {
			public:
				/// @name Ruler
				/// @{
				std::shared_ptr<source::Ruler>& ruler() const BOOST_NOEXCEPT;
				graphics::font::TextAlignment rulerAbstractAlignment() const BOOST_NOEXCEPT;
				graphics::PhysicalDirection rulerPhysicalAlignment() const BOOST_NOEXCEPT;
				void setRuler(std::unique_ptr<source::Ruler> ruler);
				void setRulerAlignment(graphics::font::TextAlignment alignment);
				/// @}

			protected:
				virtual graphics::Rectangle doTextAreaAllocationRectangle() const BOOST_NOEXCEPT override;
				virtual void unfrozen(const boost::integer_range<Index>& linesToRedraw) override;

				/// @name Overridable Widget Events
				/// @{
				virtual void paint(graphics::PaintContext& context) override;
				virtual void resized(const graphics::Dimension& newSize) override;
				/// @}

			private:
				// RulerAllocationWidthSink
				void updateRulerAllocationWidth(const Ruler& ruler) override;
				// RulerLocator
				graphics::Rectangle locateRuler(const Ruler& ruler) const override;
			private:
				std::unique_ptr<const Ruler> ruler_;
				graphics::font::TextAlignment rulerAbstractAlignment_;
				graphics::PhysicalDirection rulerPhysicalAlignment_;
			};
		}
	}
}

#endif // !ASCENSION_SOURCE_VIEWER_HPP
