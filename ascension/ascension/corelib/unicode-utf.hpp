/**
 * @file unicode-utf.hpp
 * Defines iterator classes traverse Unicode character sequence.
 * @author exeal
 * @date 2005-2010 (was unicode.hpp)
 * @date 2010 (was character-iterator.hpp)
 * @see unicode.hpp, unicode-surrogates.hpp
 */

#ifndef ASCENSION_UNICODE_UTF_HPP
#define ASCENSION_UNICODE_UTF_HPP

#include <ascension/internal.hpp>					// internal.Select
#include <ascension/corelib/basic-exceptions.hpp>	// IllegalStateException
#include <ascension/corelib/basic-types.hpp>
#include <ascension/corelib/unicode-surrogates.hpp>
#include <iterator>

#if ASCENSION_UNICODE_VERSION > 0x0510
#error These class definitions and implementations are based on old version of Unicode.
#endif

namespace ascension {
	namespace text {

		/**
		 * Base class for @c UTF16To32Iterator bidirectional iterator scans a UTF-16 character
		 * sequence as UTF-32.
		 *
		 * Scanned UTF-16 sequence is given by the template parameter @a BaseIterator.
		 *
		 * This supports four relation operators general bidirectional iterators don't have.
		 * These are available if @a BaseIterator have these facilities.
		 * @tparam BaseIterator The base bidirectional iterator presents UTF-16 character sequence
		 * @tparam ConcreteIterator Set to @c UTF16To32Iterator template class
		 * @see UTF16To32Iterator, UTF16To32IteratorUnsafe, UTF32To16Iterator, ToUTF32Sequence
		 */
		template<typename BaseIterator, typename ConcreteIterator>
		class UTF16To32IteratorBase : public std::iterator<std::bidirectional_iterator_tag,
				CodePoint, typename std::iterator_traits<BaseIterator>::difference_type,
				const CodePoint*, const CodePoint> {
		public:
			/// Assignment operator.
			ConcreteIterator& operator=(const UTF16To32IteratorBase& other) {
				p_ = other.p_;
				return getConcrete();
			}
			/// Dereference operator.
			CodePoint operator*() const {
				if(!getConcrete().hasNext())
					throw IllegalStateException("The iterator is last.");
				if(!surrogates::isHighSurrogate(*p_))
					return *p_;
				++const_cast<UTF16To32IteratorBase*>(this)->p_;
				const CodePoint next = getConcrete().hasNext() ? *p_ : INVALID_CODE_POINT;
				--const_cast<UTF16To32IteratorBase*>(this)->p_;
				return (next != INVALID_CODE_POINT) ?
					surrogates::decode(*p_, static_cast<Char>(next & 0xffffu)) : *p_;
			}
			/// Dereference operator.
			CodePoint operator->() const {return operator*();}
			/// Pre-fix increment operator.
			ConcreteIterator& operator++() {
				if(!getConcrete().hasNext())
					throw IllegalStateException("The iterator is last.");
				++p_;
				if(getConcrete().hasNext() && surrogates::isLowSurrogate(*p_))
					++p_;
				return getConcrete();
			}
			/// Post-fix increment operator.
			const ConcreteIterator operator++(int) {
				ConcreteIterator temp(getConcrete());
				++*this;
				return temp;
			}
			/// Pre-fix decrement operator.
			ConcreteIterator& operator--() {
				if(!getConcrete().hasPrevious())
					throw IllegalStateException("The iterator is first.");
				--p_;
				if(getConcrete().hasPrevious() && surrogates::isLowSurrogate(*p_)) --p_;
				return getConcrete();
			}
			/// Post-fix decrement operator.
			const ConcreteIterator operator--(int) {
				ConcreteIterator temp(*this);
				--*this;
				return temp;
			}
			/// Equality operator.
			bool operator==(const ConcreteIterator& other) const {return p_ == other.p_;}
			/// Inequality operator.
			bool operator!=(const ConcreteIterator& other) const {return p_ != other.p_;}
			/// Relational operator.
			bool operator<(const ConcreteIterator& other) const {return p_ < other.p_;}
			/// Relational operator.
			bool operator<=(const ConcreteIterator& other) const {return p_ <= other.p_;}
			/// Relational operator.
			bool operator>(const ConcreteIterator& other) const {return p_ > other.p_;}
			/// Relational operator.
			bool operator>=(const ConcreteIterator& other) const {return p_ >= other.p_;}
			/// Returns the current position.
			BaseIterator tell() const {return p_;}
		protected:
			/// Default constructor.
			UTF16To32IteratorBase() {}
			/// Copy-constructor.
			UTF16To32IteratorBase(const UTF16To32IteratorBase& other) : p_(other.p_) {}
			/// Constructor takes a position to start iteration.
			UTF16To32IteratorBase(BaseIterator start) : p_(start) {}
		private:
			ConcreteIterator& getConcrete() /*throw()*/ {
				return *static_cast<ConcreteIterator*>(this);}
			const ConcreteIterator& getConcrete() const /*throw()*/ {
				return *static_cast<const ConcreteIterator*>(this);}
			BaseIterator p_;
		};

		/// Concrete class derived from @c UTF16To32IteratorBase does not check boundary at all.
		template<typename BaseIterator = const Char*>
		class UTF16To32IteratorUnsafe :
			public UTF16To32IteratorBase<BaseIterator, UTF16To32IteratorUnsafe<BaseIterator> > {
		public:
			/// Default constructor.
			UTF16To32IteratorUnsafe() {}
			/**
			 * Constructor takes a position to start iteration. The ownership of the target text
			 * will not be transferred to this.
			 */
			UTF16To32IteratorUnsafe(BaseIterator i) :
				UTF16To32IteratorBase<BaseIterator, UTF16To32IteratorUnsafe<BaseIterator> >(i) {}
			/// Returns true.
			bool hasNext() const /*throw()*/ {return true;}
			/// Returns true.
			bool hasPrevious() const /*throw()*/ {return true;}
		};

		/**
		 * Concrete class derived from @c UTF16To32IteratorBase.
		 * @see makeUTF16To32Iterator
		 */
		template<typename BaseIterator = const Char*>
		class UTF16To32Iterator :
			public UTF16To32IteratorBase<BaseIterator, UTF16To32Iterator<BaseIterator> > {
		private:
			typedef UTF16To32IteratorBase<BaseIterator, UTF16To32Iterator<BaseIterator> > Base;
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
			UTF16To32Iterator(const UTF16To32Iterator& other) :
				Base(other), first_(other.first_), last_(other.last_) {}
			/// Assignment operator.
			UTF16To32Iterator& operator=(const UTF16To32Iterator& other) {
				Base::operator=(other); first_ = other.first_; last_ = other.last_; return *this;}
			/// Returns true if the iterator is not at the last.
			bool hasNext() const {return Base::tell() != last_;}
			/// Returns true if the iterator is not at the first.
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
			template<class> class AdaptionIterator = UTF16To32Iterator>
		struct ToUTF32Sequence {
			typedef typename internal::Select<
				CodeUnitSizeOf<CodeUnitSequence>::result == 4,
				CodeUnitSequence, AdaptionIterator<CodeUnitSequence> >::Result Result;
		};

		/**
		 * Bidirectional iterator scans UTF-32 character sequence as UTF-16.
		 *
		 * UTF-32 sequence scanned by this is given by the template parameter @a BaseIterator.
		 *
		 * This supports four relation operators general bidirectional iterators don't have.
		 * These are available if @a BaseIterator have these facilities.
		 * @tparam BaseIterator The base bidirectional iterator presents UTF-32 character sequence
		 * @see UTF16To32Iterator
		 */
		template<class BaseIterator = const CodePoint*>
		class UTF32To16Iterator : public std::iterator<
			std::bidirectional_iterator_tag, Char,
			typename std::iterator_traits<BaseIterator>::difference_type,
			const Char*, const Char> {
		public:
			/// Default constructor.
			UTF32To16Iterator() {}
			/**
			 * Constructor takes a position to start iteration.
			 * The ownership of the target text will not be transferred to this.
			 */
			UTF32To16Iterator(BaseIterator start) : p_(start), high_(true) {}
			/// Assignment operator.
			UTF32To16Iterator& operator=(const UTF32To16Iterator& other) {
				p_ = other.p_;
				high_ = other.high_;
			}
			/// Dereference operator.
			Char operator*() const {
				if(*p_ < 0x10000ul)
					return static_cast<Char>(*p_ & 0xffffu);
				else {
					Char text[2];
					surrogates::encode(*p_, text);
					return text[high_ ? 0 : 1];
				}
			}
			/// Dereference operator.
			Char operator->() const {return operator*();}
			/// Pre-fix increment operator.
			UTF32To16Iterator& operator++() {
				if(!high_) {
					high_ = true;
					++p_;
				} else if(*p_ < 0x10000ul)
					++p_;
				else
					high_ = false;
				return *this;
			}
			/// Post-fix increment operator.
			const UTF32To16Iterator operator++(int) {
				UTF32To16Iterator temp(*this);
				++*this;
				return temp;
			}
		  	/// Pre-fix decrement operator.
			UTF32To16Iterator& operator--() {
				if(!high_)
					high_ = true;
				else {
					--p_;
					high_ = *p_ < 0x10000ul;
				}
				return *this;
			}
			/// Post-fix decrement operator.
			const UTF32To16Iterator operator--(int) {
				UTF32To16Iterator temp(*this);
				--*this;
				return temp;
			}
			/// Equality operator.
			bool operator==(const UTF32To16Iterator& other) const {
				return p_ == other.p_ && high_ == other.high_;
			}
			/// Inequality operator.
			bool operator!=(const UTF32To16Iterator& other) const {
				return p_ != other.p_ || high_ != other.high_;
			}
			/// Relational operator.
			bool operator<(const UTF32To16Iterator<BaseIterator>& other) const {
				return p_ < other.p_ || (p_ == other.p_ && high_ && !other.high_);
			}
			/// Relational operator.
			bool operator<=(const UTF32To16Iterator<BaseIterator>& other) const {
				return p_ < other.p_ || (p_ == other.p_ && high_ == other.high_);
			}
			/// Relational operator.
			bool operator>(const UTF32To16Iterator<BaseIterator>& other) const {
				return p_ > other.p_ || (p_ == other.p_ && !high_ && other.high_);
			}
			/// Relational operator.
			bool operator>=(const UTF32To16Iterator<BaseIterator>& other) const {
				return p_ > other.p_ || (p_ == other.p_ && high_ == other.high_);
			}
			/// Returns the current position.
			BaseIterator tell() const {return p_;}
		private:
			BaseIterator p_;
			bool high_;
		};

	}
} // namespace ascension.text

#endif // !ASCENSION_CHARACTER_ITERATOR_HPP
