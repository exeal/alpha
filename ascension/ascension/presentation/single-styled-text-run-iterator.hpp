/**
 * @file single-styled-text-run-iterator.hpp
 * Defines @c SingleStyledTextRunIterator class.
 * @author exeal
 * @see styled-text-run-iterator.hpp
 * @date 2015-09-09 Created.
 */

#ifndef ASCENSION_SINGLE_STYLED_TEXT_RUN_ITERATOR_HPP
#define ASCENSION_SINGLE_STYLED_TEXT_RUN_ITERATOR_HPP
#include <ascension/kernel/region.hpp>
#include <ascension/presentation/styled-text-run-iterator.hpp>

namespace ascension {
	namespace presentation {
		/**
		 * Implementation of @c StyledTextRunIterator interface covers the single segment in the document.
		 * @tparam Interface @c StyledTextRunIterator class template or its subclass
		 */
		template<typename Interface>
		class SingleStyledTextRunIterator : public Interface {
		public:
			/**
			 * Creates a @c SingleStyledTextRunIterator instance.
			 * @param region The target region this iterator covers
			 * @param style The style object this iterator returns
			 */
			SingleStyledTextRunIterator(const kernel::Region& region,
					typename Interface::StyleType style) : position_(region.beginning()), end_(region.end()), style_(style) {
			}
			/// @see StyledTextRunIterator#isDone
			bool isDone() const BOOST_NOEXCEPT override {
				return position_ == end_;
			}
			/// @see StyledTextRunIterator#next
			void next() override {
				if(isDone())
					throw NoSuchElementException();
				position_ = end_;
			}
			/// @see StyledTextRunIterator#position
			kernel::Position position() const BOOST_NOEXCEPT override {
				return position_;
			}
			/// @see StyledTextRunIterator#style
			typename Interface::StyleType style() const override {
				if(isDone())
					throw NoSuchElementException();
				return style_;
			}

		private:
			kernel::Position position_;
			const kernel::Position end_;
			const typename Interface::StyleType style_;
		};
	}
}

#endif // !ASCENSION_SINGLE_STYLED_TEXT_RUN_ITERATOR_HPP
