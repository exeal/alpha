/**
 * @file utf-16.hpp
 * @author exeal
 * @date 2005-2010 was unicode.hpp
 * @date 2010 was character-iterator.hpp
 * @date 2010-11-06 separated from character-iterator.hpp to unicode-surrogates.hpp and
 *                  unicode-utf.hpp
 * @date 2011-08-20 joined unicode-surrogates.hpp and unicode-utf.hpp
 */

#ifndef ASCENSION_UTF_16_HPP
#define ASCENSION_UTF_16_HPP

#include <ascension/corelib/basic-exceptions.hpp>	// IllegalStateException
#include <ascension/corelib/standard-iterator-adapter.hpp>
#include <ascension/corelib/type-traits.hpp>		// detail.Select
#include <ascension/corelib/text/character.hpp>		// CodePoint, ASCENSION_STATIC_ASSERT, surrogates.*
#include <cassert>									// assert

#if ASCENSION_UNICODE_VERSION > 0x0510
#error These class definitions and implementations are based on old version of Unicode.
#endif

namespace ascension {
	namespace text {
		namespace utf16 {

			/**
			 * Converts the first surrogate pair in the given character sequence to the
			 * corresponding code point.
			 * @tparam InputIterator The input iterator represents a UTF-16 character sequence
			 * @param first The beginning of the character sequence
			 * @param last The end of the character sequence
			 * @return The code point
			 */
			template<typename InputIterator>
			inline CodePoint decodeFirst(InputIterator first, InputIterator last) /*throw()*/ {
				ASCENSION_STATIC_ASSERT(CodeUnitSizeOf<InputIterator>::value == 2);
				assert(first != last);
				const Char high = *first;
				return (++first != last) ? surrogates::decode(high, *first) : high;
			}

			/**
			 * Converts the last surrogate pair in the given character sequence to the
			 * corresponding code point.
			 * @tparam BidirectionalIterator The bidirectional iterator represents a UTF-16
			 *                               character sequence
			 * @param first The beginning of the character sequence
			 * @param last The end of the character sequence
			 * @return The code point
			 */
			template<typename BidirectionalIterator>
			inline CodePoint decodeLast(
					BidirectionalIterator first, BidirectionalIterator last) /*throw()*/ {
				ASCENSION_STATIC_ASSERT(CodeUnitSizeOf<BidirectionalIterator>::value == 2);
				assert(first != last);
				const Char low = *--last;
				return (last != first && surrogates::isLowSurrogate(low)
					&& surrogates::isHighSurrogate(*--last)) ? surrogates::decode(*last, low) : low;
			}

			/**
			 * Searches the next high-surrogate in the given character sequence.
			 * @tparam InputIterator The input iterator represents a UTF-16 character sequence
			 * @param start The start position to search
			 * @param last The end of the character sequence
			 * @return The next high-surrogate
			 */
			template<typename InputIterator>
			inline InputIterator next(InputIterator start, InputIterator last) /*throw()*/ {
				ASCENSION_STATIC_ASSERT(CodeUnitSizeOf<InputIterator>::value == 2);
				assert(start != last);
				return (surrogates::isHighSurrogate(*(start++))
					&& (start != last) && surrogates::isLowSurrogate(*start)) ? ++start : start;
			}

			/**
			 * Searches the previous high-surrogate in the given character sequence.
			 * @tparam BidirectionalIterator The bidirectional iterator represents a UTF-16
			 *                               character sequence
			 * @param first The beginning of the character sequence
			 * @param start The start position to search
			 * @return The previous high-surrogate
			 */
			template<typename BidirectionalIterator>
			inline BidirectionalIterator previous(
					BidirectionalIterator first, BidirectionalIterator start) /*throw()*/ {
				ASCENSION_STATIC_ASSERT(CodeUnitSizeOf<BidirectionalIterator>::value == 2);
				assert(first != start);
				return (!surrogates::isLowSurrogate(*--start)
					|| (start == first) || surrogates::isHighSurrogate(*--start)) ? start : ++start;
			}

		} // namespace utf16

		/**
		 * Base class for @c UTF16To32Iterator bidirectional iterator scans a UTF-16 character
		 * sequence as UTF-32.
		 * @par Scanned UTF-16 sequence is given by the template parameter @a BaseIterator.
		 * @par This supports four relation operators general bidirectional iterators don't have.
		 *      These are available if @a BaseIterator have these facilities.
		 * @tparam Derived Set to @c UTF16To32Iterator template class
		 * @tparam BaseIterator The base bidirectional iterator presents UTF-16 character sequence
		 * @tparam UChar32 The type of 32-bit code unit
		 * @see UTF16To32Iterator, UTF16To32IteratorUnsafe, UTF32To16Iterator, ToUTF32Sequence
		 */
		template<typename Derived, typename BaseIterator, typename UChar32 = CodePoint>
		class UTF16To32IteratorBase : public detail::IteratorAdapter<
			Derived,
			std::iterator<
				std::bidirectional_iterator_tag, UChar32,
				typename std::iterator_traits<BaseIterator>::difference_type,
				const UChar32*, const UChar32
			>
		> {
		public:
			/// Assignment operator.
			Derived& operator=(const UTF16To32IteratorBase<Derived, BaseIterator, UChar32>& other) {
				p_ = other.p_;
				return derived();
			}
			/// Returns the current position.
			BaseIterator tell() const {return p_;}
		protected:
			/// Default constructor.
			UTF16To32IteratorBase() {}
			/// Copy-constructor.
			UTF16To32IteratorBase(const UTF16To32IteratorBase<Derived, BaseIterator, UChar32>& other) : p_(other.p_) {}
			/// Constructor takes a position to start iteration.
			UTF16To32IteratorBase(BaseIterator start) : p_(start) {}
		private:
			// detail.IteratorAdapter
			friend class detail::IteratorCoreAccess;
			value_type current() const {
				if(!derived().hasNext())
					throw IllegalStateException("The iterator is last.");
				if(!surrogates::isHighSurrogate(*p_))
					return *p_;
				++const_cast<UTF16To32IteratorBase<Derived, BaseIterator, UChar32>*>(this)->p_;
				const value_type next = derived().hasNext() ? *p_ : INVALID_CODE_POINT;
				--const_cast<UTF16To32IteratorBase<Derived, BaseIterator, UChar32>*>(this)->p_;
				return (next != INVALID_CODE_POINT) ?
					surrogates::decode(*p_, static_cast<Char>(next & 0xffffu)) : *p_;
			}
			void next() {
				if(!derived().hasNext())
					throw IllegalStateException("The iterator is last.");
				++p_;
				if(derived().hasNext() && surrogates::isLowSurrogate(*p_))
					++p_;
			}
			void previous() {
				if(!derived().hasPrevious())
					throw IllegalStateException("The iterator is first.");
				--p_;
				if(derived().hasPrevious() && surrogates::isLowSurrogate(*p_))
					--p_;
			}
			bool equals(const UTF16To32IteratorBase<Derived, BaseIterator, UChar32>& other) const {
				return p_ == other.p_;
			}
			bool less(const UTF16To32IteratorBase<Derived, BaseIterator, UChar32>& other) const {
				return p_ < other.p_;
			}
		private:
			Derived& derived() /*throw()*/ {return *static_cast<Derived*>(this);}
			const Derived& derived() const /*throw()*/ {return *static_cast<const Derived*>(this);}
			BaseIterator p_;
		};

		/// Concrete class derived from @c UTF16To32IteratorBase does not check boundary at all.
		template<typename BaseIterator = const Char*, typename UChar32 = CodePoint>
		class UTF16To32IteratorUnsafe : public UTF16To32IteratorBase<
			UTF16To32IteratorUnsafe<BaseIterator, UChar32>, BaseIterator, UChar32> {
		public:
			/// Default constructor.
			UTF16To32IteratorUnsafe() {}
			/// Copy-constructor.
			UTF16To32IteratorUnsafe(const UTF16To32IteratorUnsafe<BaseIterator, UChar32>& other) :
				UTF16To32IteratorBase<UTF16To32IteratorUnsafe<BaseIterator, UChar32>, BaseIterator, UChar32>(other) {}
			/**
			 * Constructor takes a position to start iteration. The ownership of the target text
			 * will not be transferred to this.
			 */
			UTF16To32IteratorUnsafe(BaseIterator i) :
				UTF16To32IteratorBase<UTF16To32IteratorUnsafe<BaseIterator, UChar32>, BaseIterator, UChar32>(i) {}
			/// Returns true.
			bool hasNext() const /*throw()*/ {return true;}
			/// Returns true.
			bool hasPrevious() const /*throw()*/ {return true;}
		};

		/**
		 * Concrete class derived from @c UTF16To32IteratorBase.
		 * @see makeUTF16To32Iterator
		 */
		template<typename BaseIterator = const Char*, typename UChar32 = CodePoint>
		class UTF16To32Iterator :
			public UTF16To32IteratorBase<UTF16To32Iterator<BaseIterator, UChar32>, BaseIterator, UChar32> {
		private:
			typedef UTF16To32IteratorBase<UTF16To32Iterator<BaseIterator, UChar32>, BaseIterator, UChar32> Base;
		public:
			/// Default constructor.
			UTF16To32Iterator() {}
			/**
			 * Constructor takes a position to start iteration. The ownership of the target text
			 * will not be transferred to this.
			 */
			UTF16To32Iterator(BaseIterator first, BaseIterator last) :
				Base(first), first_(first), last_(last) {}
			/**
			 * Constructor takes a position to start iteration. The ownership of the target text
			 * will not be transferred to this.
			 */
			UTF16To32Iterator(BaseIterator first, BaseIterator last,
				BaseIterator start) : Base(start), first_(first), last_(last) {}
			/// Copy constructor.
			UTF16To32Iterator(const UTF16To32Iterator<BaseIterator, UChar32>& other) :
				Base(other), first_(other.first_), last_(other.last_) {}
			/// Assignment operator.
			UTF16To32Iterator& operator=(const UTF16To32Iterator<BaseIterator, UChar32>& other) {
				Base::operator=(other);
				first_ = other.first_;
				last_ = other.last_;
				return *this;
			}
			/// Returns @c true if the iterator is not at the last.
			bool hasNext() const {return Base::tell() != last_;}
			/// Returns @c true if the iterator is not at the first.
			bool hasPrevious() const {return Base::tell() != first_;}
		private:
			BaseIterator first_, last_;
		};

		/// Returns a @c UTF16To32Iterator instance iterates elements of the given container.
		template<typename Container>
		inline UTF16To32Iterator<typename Container::const_iterator>
			makeUTF16To32Iterator(const Container& c) {
			return UTF16To32Iterator<typename Container::const_iterator>(c.begin(), c.end());
		}
		/// Returns a @c UTF16To32Iterator instance iterates elements of the given container.
		template<typename Container>
		inline UTF16To32Iterator<typename Container::const_iterator> makeUTF16To32Iterator(
				const Container& c, typename Container::const_iterator start) {
			return UTF16To32Iterator<typename Container::const_iterator>(c.begin(), c.end(), start);
		}

		/// Converts the code unit sequence into UTF-32. This does not accept UTF-8.
		template<typename CodeUnitSequence,
			template<class, class> class AdaptionIterator = UTF16To32Iterator, typename UChar32 = CodePoint>
		struct ToUTF32Sequence {
			typedef typename detail::Select<
				CodeUnitSizeOf<CodeUnitSequence>::result == 4,
				CodeUnitSequence, AdaptionIterator<CodeUnitSequence, UChar32> >::Type Type;
		};

		/**
		 * Bidirectional iterator scans UTF-32 character sequence as UTF-16.
		 * @par UTF-32 sequence scanned by this is given by the template parameter @a BaseIterator.
		 * @par This supports four relation operators general bidirectional iterators don't have.
		 *      These are available if @a BaseIterator have these facilities.
		 * @tparam BaseIterator The base bidirectional iterator presents UTF-32 character sequence
		 * @tparam UChar16 The type of 16-bit character
		 * @see UTF16To32Iterator
		 */
		template<class BaseIterator = const CodePoint*, typename UChar16 = Char
//			, typename std::tr1::enable_if<CodeUnitSizeOf<BaseIterator>::value == 4 && sizeof(UChar16) == 2>::type* = 0
		>
		class UTF32To16Iterator : public detail::IteratorAdapter<
			UTF32To16Iterator<BaseIterator, UChar16>,
			std::iterator<
				std::bidirectional_iterator_tag, UChar16,
				typename std::iterator_traits<BaseIterator>::difference_type,
				const UChar16*, const UChar16
			>
		> {
			ASCENSION_STATIC_ASSERT(CodeUnitSizeOf<BaseIterator>::value == 4);
			ASCENSION_STATIC_ASSERT(sizeof(UChar16) == 2);
		public:
			/// Default constructor.
			UTF32To16Iterator() {}
			/**
			 * Constructor takes a position to start iteration.
			 * The ownership of the target text will not be transferred to this.
			 */
			UTF32To16Iterator(BaseIterator start) : p_(start), high_(true) {}
			/// Assignment operator.
			UTF32To16Iterator& operator=(const UTF32To16Iterator<BaseIterator, UChar16>& other) {
				p_ = other.p_;
				high_ = other.high_;
			}
			/// Returns the current position.
			BaseIterator tell() const {return p_;}
		private:
			// detail.IteratorAdapter
			friend class detail::IteratorCoreAccess;
			value_type current() const {
				if(*p_ < 0x10000ul)
					return static_cast<value_type>(*p_ & 0xffffu);
				else {
					value_type text[2];
					surrogates::encode(*p_, text);
					return text[high_ ? 0 : 1];
				}
			}
			void next() {
				if(!high_) {
					high_ = true;
					++p_;
				} else if(*p_ < 0x10000ul)
					++p_;
				else
					high_ = false;
			}
			void previous() {
				if(!high_)
					high_ = true;
				else {
					--p_;
					high_ = *p_ < 0x10000ul;
				}
			}
			bool equals(const UTF32To16Iterator<BaseIterator, UChar16>& other) const {
				return p_ == other.p_ && high_ == other.high_;
			}
			bool less(const UTF32To16Iterator<BaseIterator, UChar16>& other) const {
				return p_ < other.p_ || (p_ == other.p_ && high_ && !other.high_);
			}
		private:
			BaseIterator p_;
			bool high_;
		};

	}
} // namespace ascension.text

#endif // !ASCENSION_UTF_16_HPP
