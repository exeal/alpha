/**
 * @file weak-reference-for-points.hpp
 * @author exeal
 * @date 2003-2006 (was EditDoc.h)
 * @date 2006-2012, 2014-2015
 * @date 2015-04-29 Separated from document.hpp.
 */

#ifndef ASCENSION_WEAK_REFERENCE_FOR_POINTS_HPP
#define ASCENSION_WEAK_REFERENCE_FOR_POINTS_HPP
#include <memory>

namespace ascension {
	namespace viewer {
		namespace detail {
			template<typename Derived>
			class WeakReferenceForPoints {
			public:
				class Proxy /* : private std::enable_shared_from_this<Proxy> */ {
				public:
					explicit Proxy(Derived& object) BOOST_NOEXCEPT : object_(&object) {}
					Derived* get() const BOOST_NOEXCEPT {return object_;}
					void reset() BOOST_NOEXCEPT {object_ = nullptr;}
				private:
					Derived* object_;
				};
			public:
				virtual ~WeakReferenceForPoints() BOOST_NOEXCEPT {
					if(proxy_.get() != nullptr)
						proxy_->reset();
				}
				std::shared_ptr<const Proxy> referByPoint() {
					if(proxy_.get() == nullptr)
						proxy_ = std::make_shared<Proxy>(*static_cast<Derived*>(this));
					return proxy_;
				}

			private:
				std::shared_ptr<Proxy> proxy_;
			};
		}
	}
}

#endif // !ASCENSION_WEAK_REFERENCE_FOR_POINTS_HPP
