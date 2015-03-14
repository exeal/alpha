/**
 * @file ruler-border-decorator.hpp
 * Defines @c RulerBorderDecorator class.
 * @author exeal
 * @date 2015-01-17 Created.
 */

#ifndef ASCENSION_RULER_BORDER_DECORATOR_HPP
#define ASCENSION_RULER_BORDER_DECORATOR_HPP
#include <ascension/graphics/font/actual-text-styles.hpp>
#include <ascension/viewer/source/ruler-decorator.hpp>
#include <memory>

namespace ascension {
	namespace viewer {
		namespace source {
			/// Decorator class of @Ruler interface which displays a border-end.
			class RulerBorderDecorator : public RulerDecorator {
			public:
				RulerBorderDecorator(std::unique_ptr<AbstractRuler> decoratee,
					const graphics::font::ActualBorderSide& borderEnd = graphics::font::ActualBorderSide());
				void setBorderEnd(const graphics::font::ActualBorderSide& borderEnd);

			private:
				// Ruler
				void paint(graphics::PaintContext& context);
				graphics::Scalar width() const BOOST_NOEXCEPT;
				// RulerLocator
				graphics::Rectangle locateRuler(const Ruler& ruler) const override;
			private:
				graphics::font::ActualBorderSide borderEnd_;
			};
		}
	}
}

#endif // !ASCENSION_ABSTRACT_RULER_HPP
