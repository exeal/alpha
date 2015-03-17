/**
 * @file ruler-decorator.hpp
 * Defines @c RulerDecorator class.
 * @author exeal
 * @date 2015-01-17 Created.
 */

#ifndef ASCENSION_RULER_DECORATOR_HPP
#define ASCENSION_RULER_DECORATOR_HPP
#include <ascension/viewer/source/abstract-ruler.hpp>
#include <ascension/viewer/source/ruler-locator.hpp>
#include <memory>

namespace ascension {
	namespace viewer {
		namespace source {
			/// Base class of @Ruler interface decorator.
			class RulerDecorator : public AbstractRuler, public TextViewerComponent::Locator {
			protected:
				/**
				 * Creates @c RulerDecorator object with the specified decoratee.
				 * @param decoratee The decoratee
				 * @throw NullPointerException @a decoratee is @c null
				 */
				explicit RulerDecorator(std::unique_ptr<AbstractRuler> decoratee) : decoratee_(std::move(decoratee)) {
					if(decoratee_.get() == nullptr)
						throw NullPointerException("decoratee");
				}
				/// Destructor.
				virtual ~RulerDecorator() BOOST_NOEXCEPT {}
				/// Returns the decoratee.
				AbstractRuler& decoratee() const BOOST_NOEXCEPT {
					return *decoratee_;
				}
				/// @see Ruler#install
				virtual void install(SourceViewer& viewer, const Locator& locator, RulerAllocationWidthSink& allocationWidthSink) override {
					return decoratee_->install(viewer, *(locator_ = &locator), allocationWidthSink);
				}
				/***/
				virtual graphics::Rectangle locate(const Locator& parentLocator) const = 0;
				/// @see Ruler#uninstall
				virtual void uninstall(SourceViewer& viewer) override {
					locator_ = nullptr;
					return decoratee_->uninstall(viewer);
				}

			private:
				graphics::Rectangle locateComponent(const TextViewerComponent& component) const override {
					if(&component != &decoratee())
						throw std::invalid_argument("ruler");
					if(locator_ != nullptr)
						return locate(*locator_);
					return boost::geometry::make_zero<graphics::Rectangle>();
				}
				std::unique_ptr<AbstractRuler> decoratee_;
				const Locator* locator_;
			};
		}
	}
}

#endif // !ASCENSION_ABSTRACT_RULER_HPP
