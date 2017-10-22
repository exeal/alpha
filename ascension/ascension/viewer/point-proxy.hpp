/**
 * @file viewer/point-proxy.hpp
 * Defines @c viewer#locations#PointProxy class.
 * @author exeal
 * @date 2017-10-17 Separated from visual-locations.hpp.
 * @see kernel/point-proxy.hpp
 */

#ifndef ASCENSION_VIEWER_POINT_PROXY_HPP
#define ASCENSION_VIEWER_POINT_PROXY_HPP
#include <ascension/kernel/point-proxy.hpp>
#include <ascension/viewer/text-hit.hpp>

namespace ascension {
	namespace viewer {
		class TextArea;

		namespace locations {
			/**
			 * Describes a position in the @c TextArea.
			 * @see kernel#locations#PointProxy
			 */
			struct PointProxy {
				/// The text area.
				const TextArea& textArea;
				/// The text hit.
				const TextHit hit;
				/// Creates a @c PointProxy instance.
				PointProxy(const TextArea& textArea, const TextHit& hit) BOOST_NOEXCEPT : textArea(textArea), hit(hit) {}
				/// Returns a reference to @c #hit.
				operator const TextHit&() const BOOST_NOEXCEPT {return hit;}
			};
		}

		kernel::locations::PointProxy insertionPosition(const locations::PointProxy& p);
	}

	namespace kernel {
		template<>
		struct DocumentAccess<const viewer::locations::PointProxy> {
			static std::shared_ptr<const Document> get(const viewer::locations::PointProxy& p) BOOST_NOEXCEPT;
		};
		template<>
		struct PositionAccess<const viewer::locations::PointProxy> {
			static Position get(const viewer::locations::PointProxy& p) BOOST_NOEXCEPT;
		};
	}
}

#endif // !ASCENSION_VIEWER_POINT_PROXY_HPP
