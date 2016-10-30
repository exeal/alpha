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
				SourceViewer* viewer() {
					return viewer_;
				}
				/// Returns the installed viewer, or @c null if not installed.
				const SourceViewer* viewer() const {
					return viewer_;
				}

			protected:
				/// Returns the installed @c RulerAllocationWidthSink object, or @c null if not installed.
				BOOST_CONSTEXPR RulerAllocationWidthSink* allocationWidthSink() const BOOST_NOEXCEPT {
					return allocationWidthSink_;
				}
				/// Implements @c Ruler#install method.
				virtual void install(SourceViewer& viewer,
						const Locator& locator, RulerAllocationWidthSink& allocationWidthSink) override {
					if(viewer_ == nullptr) {
						viewer_ = &viewer;
						allocationWidthSink_ = &allocationWidthSink;
						locator_ = &locator;
					}
				}
				/// Returns the installed @c TextViewerComponent#Locator, or @c null if not installed.
				const Locator* locator() const BOOST_NOEXCEPT {
					return locator_;
				}
				/// Implements @c Ruler#uninstall method.
				virtual void uninstall(SourceViewer& viewer) override {
					if(&viewer == viewer_) {
						viewer_ = nullptr;
						allocationWidthSink_ = nullptr;
						locator_ = nullptr;
					}
				}

			private:
				SourceViewer* viewer_;
				RulerAllocationWidthSink* allocationWidthSink_;
				const Locator* locator_;
				graphics::Color backgroundColor_;
				friend class RulerDecorator;
			};
		}
	}
}

#endif // !ASCENSION_ABSTRACT_RULER_HPP
