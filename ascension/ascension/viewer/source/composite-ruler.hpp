/**
 * @file composite-ruler.hpp
 * Defines @c CompositeRuler class.
 * @author exeal
 * @date 2015-01-12 Created.
 */

#ifndef ASCENSION_COMPOSITE_RULER_HPP
#define ASCENSION_COMPOSITE_RULER_HPP
#include <ascension/viewer/source/ruler.hpp>
#include <boost/core/noncopyable.hpp>
#include <memory>
#include <vector>

namespace ascension {
	namespace viewers {
		namespace source {
			/**
			 * This ruler does not have a visual representation of its own. The representation comes from the
			 * configurable list of ruler columns. Such columns must implement the @c Ruler interface, too.
			 * @see Ruler, TextViewer
			 */
			class CompositeRuler : public Ruler, private boost::noncopyable {
			public:
				CompositeRuler() BOOST_NOEXCEPT;
				void addDecorator(std::size_t position, std::unique_ptr<Ruler> rulerColumn);
				void removeDecorator(std::size_t position);

			private:
				// Ruler interface
				void paint(graphics::PaintContext& context) override;
				graphics::Scalar width() const BOOST_NOEXCEPT override;
				void install(const TextViewer& viewer) override;
				void uninstall(const TextViewer& viewer) override;
			private:
				const TextViewer* textViewer_;
				std::vector<std::unique_ptr<Ruler>> columns_;
			};
		}
	}
}

#endif // !ASCENSION_COMPOSITE_RULER_HPP
