/**
 * @file kernel/point-proxy.hpp
 * Defines @c kernel#locations#PointProxy class.
 * @author exeal
 * @date 2017-10-17 Separated from visual-locations.hpp.
 * @see kernel/point-proxy.hpp
 */

#ifndef ASCENSION_KERNEL_POINT_PROXY_HPP
#define ASCENSION_KERNEL_POINT_PROXY_HPP
#include <ascension/kernel/access.hpp>

namespace ascension {
	namespace kernel {
		class Document;

		namespace locations {
			/**
			 * Describes a position in the document.
			 * @see viewer#locations#PointProxy
			 */
			struct PointProxy {
				/// The document.
				const Document& document;
				/// The position.
				const Position position;
				/// Creates a @c PointProxy instance.
				PointProxy(const Document& document, const Position& position) BOOST_NOEXCEPT : document(document), position(position) {}
				/// Returns a reference to @c #position.
				operator const Position&() const BOOST_NOEXCEPT {return position;}
			};
		}

		template<>
		struct DocumentAccess<const locations::PointProxy> {
			static const Document& get(const locations::PointProxy& p) BOOST_NOEXCEPT {
				return p.document;
			}
		};

		template<>
		struct PositionAccess<const locations::PointProxy> {
			static const Position& get(const locations::PointProxy& p) BOOST_NOEXCEPT {
				return p.position;
			}
		};
	}
}

#endif // !ASCENSION_KERNEL_POINT_PROXY_HPP
