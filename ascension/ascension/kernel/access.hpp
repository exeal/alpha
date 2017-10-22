/**
 */

#ifndef ASCENSION_KERNEL_ACCESS_HPP
#define ASCENSION_KERNEL_ACCESS_HPP
#include <ascension/corelib/future/type-traits.hpp>
#include <ascension/kernel/position.hpp>

namespace ascension {
	namespace kernel {
		class Document;

		/**
		 * Metafunction to access the document.
		 * @tparam T The document-related object type
		 */
		template<typename T>
		struct DocumentAccess {
			/**
			 * Returns the document of the given document-related object.
			 * @param object The object to query
			 * @return The document
			 */
			static auto get(T& object) BOOST_NOEXCEPT -> decltype(object.document()) {
				return object.document();
			}
		};

		/**
		 * Metafunction to access the position.
		 * @tparam T The object type
		 */
		template<typename T>
		struct PositionAccess {
			/**
			 * Returns the position value of the given positional object.
			 * @param p The positional object
			 * @return The position value
			 */
			static inline auto get(T& p) BOOST_NOEXCEPT -> decltype(p.position()) {
				return p.position();
			}
		};

		template<>
		struct PositionAccess<Position> {
			static Position& get(Position& p) BOOST_NOEXCEPT {
				return p;
			}
		};

		template<>
		struct PositionAccess<const Position> {
			static const Position& get(const Position& p) BOOST_NOEXCEPT {
				return p;
			}
		};
		
		/**
		 * Returns the document of the given document-related object.
		 * @tparam T The type of @a object
		 * @param object The object to query
		 * @return A reference to the document
		 */
		template<typename T>
		inline auto document(T& object) BOOST_NOEXCEPT -> decltype(DocumentAccess<T>::get(object)) {
			return DocumentAccess<T>::get(object);
		}

		/**
		 * Returns the position value of the given positional object.
		 * @tparam T The type of @a p
		 * @param p The positional object
		 * @return The position value
		 */
		template<typename T>
		inline auto position(T& p) BOOST_NOEXCEPT -> decltype(PositionAccess<T>::get(p)) {
			return PositionAccess<T>::get(p);
		}

		/**
		 * Returns the line number of the given positional object.
		 * @tparam T The type of @a p. This should be able to pass to @c position function
		 * @param p The positional object
		 * @return The line number. Not a reference
		 */
		template<typename T>
		BOOST_CONSTEXPR inline Index line(const T& p) BOOST_NOEXCEPT {
			return position(p).line;
		}

		/**
		 * Returns the offset in the line of the given positional object.
		 * @tparam T The type of @a p. This should be able to pass to @c position function
		 * @param p The positional object
		 * @return The offset in the line. Not a reference
		 */
		template<typename T>
		BOOST_CONSTEXPR inline Index offsetInLine(const T& p) BOOST_NOEXCEPT {
			return position(p).offsetInLine;
		}

		/// @}
	}
}

#endif // !ASCENSION_KERNEL_ACCESS_HPP
