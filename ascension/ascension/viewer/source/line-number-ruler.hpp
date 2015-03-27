/**
 * @file line-number-ruler.hpp
 * Defines @c LineNumberRuler class.
 * @author exeal
 * @date 2015-01-13 Created.
 */

#ifndef ASCENSION_LINE_NUMBER_RULER_HPP
#define ASCENSION_LINE_NUMBER_RULER_HPP
#include <ascension/graphics/font/font.hpp>
#include <ascension/graphics/font/line-layout-vector.hpp>
#include <ascension/graphics/font/number-substitution.hpp>
#include <ascension/graphics/font/text-alignment.hpp>
#include <ascension/graphics/font/text-viewport-listener.hpp>
#include <ascension/presentation/writing-mode.hpp>
#include <ascension/viewer/source/abstract-ruler.hpp>
#include <boost/optional.hpp>

namespace ascension {
	namespace viewer {
		namespace source {
			/// A ruler (column) displaying line numbers.
			class LineNumberRuler : public AbstractRuler,
				public graphics::font::TextViewportListener, public graphics::font::VisualLinesListener {
			public:
				LineNumberRuler();

				/// @name Styles
				/// @{
				void setAlignment(graphics::font::TextAlignment alignment,
					graphics::font::TextJustification justification = graphics::font::TextJustification::AUTO);
				void setColor(const graphics::Color& color);
				void setDirection(presentation::ReadingDirection direction);
				void setFont(std::shared_ptr<const graphics::font::Font> font);
				void setNumberSubstitution(const graphics::font::NumberSubstitution& numberSubstitution);
				void setPaddings(graphics::Scalar paddingStart, graphics::Scalar paddingEnd);
				void setStartValue(Index startValue);
				/// @}

			private:
				std::uint8_t computeNumberOfDigits() const BOOST_NOEXCEPT;
				void invalidate();
				bool updateNumberOfDigits() BOOST_NOEXCEPT;
				void updateWidth();
				// Ruler
				void paint(graphics::PaintContext& context) override;
				graphics::Scalar width() const BOOST_NOEXCEPT override;
				// graphics.font.TextViewportListener
				void viewportBoundsInViewChanged(const graphics::Rectangle& oldBounds) BOOST_NOEXCEPT override;
				void viewportScrollPositionChanged(
					const presentation::FlowRelativeTwoAxes<graphics::font::TextViewportScrollOffset>& positionsBeforeScroll,
					const graphics::font::VisualLine& firstVisibleLineBeforeScroll) BOOST_NOEXCEPT override;
				void viewportScrollPropertiesChanged(
					const presentation::FlowRelativeTwoAxes<bool>& changedDimensions) BOOST_NOEXCEPT override;
				// graphics.font.VisualLinesListener
				void visualLinesDeleted(const boost::integer_range<Index>& lines,
					Index sublines, bool longestLineChanged) BOOST_NOEXCEPT override;
				void visualLinesInserted(const boost::integer_range<Index>& lines) BOOST_NOEXCEPT override;
				void visualLinesModified(const boost::integer_range<Index>& lines,
					SignedIndex sublinesDifference, bool documentChanged, bool longestLineChanged) BOOST_NOEXCEPT override;
			private:
				graphics::font::TextAlignment alignment_;
				graphics::font::TextJustification justification_;
				graphics::Color color_;
				presentation::ReadingDirection direction_;
				std::shared_ptr<const graphics::font::Font> font_;
				graphics::font::NumberSubstitution numberSubstitution_;
				graphics::Scalar paddingStart_, paddingEnd_;
				Index startValue_;
				boost::optional<std::uint8_t> numberOfDigits_;
				boost::optional<graphics::Scalar> width_;
			};
		}
	}
}

#endif // !ASCENSION_LINE_NUMBER_RULER_HPP
