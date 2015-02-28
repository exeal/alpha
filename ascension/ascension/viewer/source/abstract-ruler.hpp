/**
 * @file abstract-ruler.hpp
 * Defines @c AbstractRuler class.
 * @author exeal
 * @date 2015-01-17 Created.
 */

#ifndef ASCENSION_ABSTRACT_RULER_HPP
#define ASCENSION_ABSTRACT_RULER_HPP
#include <ascension/graphics/color.hpp>
#include <ascension/viewer/source/ruler.hpp>
#include <boost/core/noncopyable.hpp>

namespace ascension {
	namespace viewer {
		namespace source {
			class RulerDecorator;

			/**
			 * An abstract class implements @c Ruler interface partially. This class caches the installed
			 * @c SourceViewer. Client may derive this class to make concrete subclasses implement @c Ruler interface.
			 * @see Ruler
			 */
			class AbstractRuler : public Ruler, private boost::noncopyable {
			public:
				/// Default constructor. Post condition is "viewer() == nullptr".
				explicit AbstractRuler(const graphics::Color& backgroundColor = graphics::Color::OPAQUE_WHITE) BOOST_NOEXCEPT
					: viewer_(nullptr), backgroundColor_(backgroundColor) {}
				/// Destructor does nothing.
				virtual ~AbstractRuler() BOOST_NOEXCEPT {}
				/**
				 * Returns the 'background-color' style value.
				 * @see #setBackgroundColor
				 */
				const graphics::Color& backgroundColor() const BOOST_NOEXCEPT {
					return backgroundColor_;
				}
				/**
				 * Sets the 'background-color' style value.
				 * @param backgroundColor The 'background-color' style value
				 * @see #backgroundColor
				 */
				void setBackgroundColor(const graphics::Color& backgroundColor) BOOST_NOEXCEPT {
					backgroundColor_ = backgroundColor;
				}
				/// Returns the installed viewer, or @c null if not installed.
				const TextViewer* viewer() const {
					return viewer_;
				}

			protected:
				/// Implements @c Ruler#install method.
				virtual void install(const TextViewer& viewer) {
					if(viewer_ == nullptr)
						viewer_ = &viewer;
				}
				/// Implements @c Ruler#uninstall method.
				virtual void uninstall(const TextViewer& viewer) {
					if(&viewer == viewer_)
						viewer_ = nullptr;
				}

			private:
				const TextViewer* viewer_;
				graphics::Color backgroundColor_;
				friend class RulerDecorator;
			};
		}
	}
}

#endif // !ASCENSION_ABSTRACT_RULER_HPP
