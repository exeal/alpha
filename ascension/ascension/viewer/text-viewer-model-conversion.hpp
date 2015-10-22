/**
 * @file text-viewer-model-conversion.hpp
 * @author exeal
 * @see text-viewport.hpp
 * @date 2015-10-17 Created.
 */

#ifndef ASCENSION_TEXT_VIEWER_MODEL_CONVERSION_HPP
#define ASCENSION_TEXT_VIEWER_MODEL_CONVERSION_HPP
#include <ascension/graphics/geometry/point.hpp>
#include <ascension/graphics/font/text-hit.hpp>
#include <ascension/kernel/point.hpp>

namespace ascension {
	namespace graphics {
		namespace font {
			class UseCalculatedLayoutTag;
			struct VisualLine;
		}
	}

	namespace viewer {
		class TextViewer;

		/// @defgroup model_and_viewer_coordinates_conversions Model and Viewer Coordinates Conversions
		/// @see model_and_viewport_coordinates_conversions
		/// @{
		graphics::Point lineStartEdge(const TextViewer& textViewer, const graphics::font::VisualLine& line);
		graphics::Point lineStartEdge(TextViewer& textViewer,
			const graphics::font::VisualLine& line, const graphics::font::UseCalculatedLayoutTag&);
		graphics::font::VisualLine locateLine(const TextViewer& textViewer,
			const graphics::Point& p, bool* snapped = nullptr) BOOST_NOEXCEPT;
		graphics::Point modelToView(const TextViewer& textViewer,
			const graphics::font::TextHit<kernel::Position>& position/*, bool fullSearchBpd*/);
		graphics::font::TextHit<kernel::Position> viewToModel(
			const TextViewer& textViewer, const graphics::Point& pointInView,
			kernel::locations::CharacterUnit snapPolicy = kernel::locations::GRAPHEME_CLUSTER);
		boost::optional<graphics::font::TextHit<kernel::Position>> viewToModelInBounds(
			const TextViewer& textViewer, const graphics::Point& pointInView,
			kernel::locations::CharacterUnit snapPolicy = kernel::locations::GRAPHEME_CLUSTER);
		/// @}
	}
}

#endif // !ASCENSION_TEXT_VIEWER_MODEL_CONVERSION_HPP
