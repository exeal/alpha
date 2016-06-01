/**
 * @file visual-destination-proxy.hpp
 * Defines @c TextHit type and @c VisualDestinationProxy class.
 * @author exeal
 * @date 2003-xx-xx Created.
 * @date 2008-xx-xx Separated from point.hpp.
 * @date 2011-10-02 Separated from caret.hpp.
 * @date 2016-05-22 Separated from visual-point.hpp.
 */

#ifndef ASCENSION_VISUAL_DESTINATION_PROXY_HPP
#define ASCENSION_VISUAL_DESTINATION_PROXY_HPP
#include <ascension/graphics/font/text-hit.hpp>
#include <ascension/kernel/position.hpp>

namespace ascension {
	namespace viewer {
		namespace detail {
			class VisualDestinationProxyMaker;
		}

		/// Used by procedures which move @c VisualPoint.
		typedef graphics::font::TextHit<kernel::Position> TextHit;

		/// See the documentation of @c kernel#locations namespace.
		class VisualDestinationProxy : public TextHit {
		public:
			bool crossesVisualLines() const BOOST_NOEXCEPT {
				return crossesVisualLines_;
			}
		private:
			VisualDestinationProxy(const TextHit& p, bool crossesVisualLines) : TextHit(p), crossesVisualLines_(crossesVisualLines) {}
			const bool crossesVisualLines_;
			friend class detail::VisualDestinationProxyMaker;
		};
	}
}

#endif // !ASCENSION_VISUAL_DESTINATION_PROXY_HPP
