/**
 * @file text-viewer-model-conversion.cpp
 * @author exeal
 * @date 2015-10-20 Created.
 */

#include <ascension/graphics/font/text-viewport.hpp>
#include <ascension/viewer/text-area.hpp>
#include <ascension/viewer/text-viewer.hpp>
#include <ascension/viewer/text-viewer-model-conversion.hpp>

namespace ascension {
	namespace viewer {
		namespace {
			inline std::shared_ptr<const graphics::font::TextViewport> viewport(const TextViewer& textViewer) BOOST_NOEXCEPT {
				return textViewer.textArea().textRenderer().viewport();
			}

			template<typename Point>	// a 'Point' should have the copy-constructor
			inline Point viewToView(const TextViewer& textViewer, const Point& pointInViewport, bool toViewer) {
				Point pointInViewer(pointInViewport);
				auto offset(graphics::geometry::origin(textViewer.textArea().contentRectangle()));
				if(!toViewer)
					graphics::geometry::negate(offset);
				graphics::geometry::translate(
					pointInViewer,
					graphics::geometry::BasicDimension<typename boost::geometry::coordinate_type<Point>::type>(
						graphics::geometry::_dx = graphics::geometry::x(offset),
						graphics::geometry::_dy = graphics::geometry::y(offset)));
				return pointInViewer;
			}

			template<typename Point>
			inline Point viewerToViewport(const TextViewer& textViewer, const Point& pointInViewer) {
				return viewToView(textViewer, pointInViewer, false);
			}

			template<typename Point>
			inline Point viewportToViewer(const TextViewer& textViewer, const Point& pointInViewport) {
				return viewToView(textViewer, pointInViewport, true);
			}
		}

		graphics::font::VisualLine locateLine(const TextViewer& textViewer, const graphics::Point& p, bool* snapped /* = nullptr */) BOOST_NOEXCEPT {
			return graphics::font::locateLine(*viewport(textViewer), viewerToViewport(textViewer, p), snapped);
		}

		graphics::Point modelToView(const TextViewer& textViewer, const graphics::font::TextHit<kernel::Position>& position/*, bool fullSearchBpd*/) {
			return viewportToViewer(textViewer, graphics::font::modelToView(*viewport(textViewer), position));
		}

		graphics::font::TextHit<kernel::Position> viewToModel(
				const TextViewer& textViewer, const graphics::Point& pointInView,
				kernel::locations::CharacterUnit snapPolicy /* = kernel::locations::GRAPHEME_CLUSTER */) {
			return graphics::font::viewToModel(*viewport(textViewer), viewerToViewport(textViewer, pointInView), snapPolicy);
		}

		boost::optional<graphics::font::TextHit<kernel::Position>> viewToModelInBounds(
				const TextViewer& textViewer, const graphics::Point& pointInView,
				kernel::locations::CharacterUnit snapPolicy /* = kernel::locations::GRAPHEME_CLUSTER */) {
			return graphics::font::viewToModelInBounds(*viewport(textViewer), viewerToViewport(textViewer, pointInView), snapPolicy);
		}
	}
}
