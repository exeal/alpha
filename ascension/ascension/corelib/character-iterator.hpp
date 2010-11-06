/**
 * @file character-iterator.hpp
 * Defines iterator classes traverse Unicode character sequence.
 * @author exeal
 * @date 2005-2010 (was unicode.hpp)
 * @date 2010
 * @see unicode.hpp
 */

#ifndef ASCENSION_CHARACTER_ITERATOR_HPP
#define ASCENSION_CHARACTER_ITERATOR_HPP
#include "basic-types.hpp"	// StandardConstBidirectionalIteratorAdapter
#include "internal.hpp"
#include "memory.hpp"	// AutoBuffer
#include "ustring.hpp"	// ustrlen
#include <cassert>
#include <stdexcept>
#include <iterator>
#include <set>
#include <bitset>
#include <map>
#include <locale>

#if ASCENSION_UNICODE_VERSION > 0x0510
#error These class definitions and implementations are based on old version of Unicode.
#endif
/// Tracking revision number of UAX #14 ("Line Breaking Properties")
#define ASCENSION_UAX14_REVISION_NUMBER 19	// 2006-05-23
/// Tracking revision number of UAX #29 ("Text Boundary")
#define ASCENSION_UAX29_REVISION_NUMBER 11	// 2006-10-12

namespace ascension {

	namespace text {

		/**
		 * @c surrogates namespace collects low level procedures handle UTF-16 surrogate pair.
		 * @see UTF16To32Iterator, UTF32To16Iterator
		 */
		namespace surrogates {
			/**
			 * Returns @c true if the specified code point is supplemental (out of BMP).
			 * @param c the code point
			 * @return true if @a c is supplemental
			 */
			inline bool isSupplemental(CodePoint c) /*throw()*/ {
				return (c & 0xffff0000ul) != 0;
			}
			/**
			 * Returns @c true if the specified code unit is high (leading)-surrogate.
			 * @param c the code point
			 * @return true if @a c is high-surrogate
			 */
			inline bool isHighSurrogate(CodePoint c) /*throw()*/ {
				return (c & 0xfffffc00ul) == 0xd800u;
			}
			/**
			 * Returns @c true if the specified code unit is low (trailing)-surrogate.
			 * @param c the code point
			 * @return true if @a c is low-surrogate
			 */
			inline bool isLowSurrogate(CodePoint c) /*throw()*/ {
				return (c & 0xfffffc00ul) == 0xdc00u;
			}
			/**
			 * Returns @c true if the specified code unit is surrogate.
			 * @param c the code point
			 * @return true if @a c is surrogate
			 */
			inline bool isSurrogate(CodePoint c) /*throw()*/ {
				return (c & 0xfffff800ul) == 0xd800u;
			}
			/**
			 * Returns high (leading)-surrogate for the specified code point.
			 * @note If @a c is in BMP, the behavior is undefined.
			 * @param c the code point
			 * @return the high-surrogate code unit for @a c
			 */
			inline Char highSurrogate(CodePoint c) /*throw()*/ {
				return static_cast<Char>((c >> 10) & 0xffffu) + 0xd7c0u;
			}
			/**
			 * Returns low (trailing)-surrogate for the specified code point.
			 * @note If @a c is in BMP, the behavior is undefined.
			 * @param c the code point
			 * @return the low-surrogate code unit for @a c
			 */
			inline Char lowSurrogate(CodePoint c) /*throw()*/ {
				return static_cast<Char>(c & 0x03ffu) | 0xdc00u;
			}
			/**
			 * Converts the specified surrogate pair to a corresponding code point.
			 * @param high the high-surrogate
			 * @param low the low-surrogate
			 * @return the code point or the value of @a high if the pair is not valid
			 */
			inline CodePoint decode(Char high, Char low) /*throw()*/ {
				return (isHighSurrogate(high) && isLowSurrogate(low)) ?
					0x10000ul + (high - 0xd800u) * 0x0400u + low - 0xdc00u : high;
			}
			/**
			 * Converts the first surrogate pair in the given character sequence to the
			 * corresponding code point.
			 * @tparam InputIterator the input iterator represents a UTF-16 character sequence
			 * @param first the beginning of the character sequence
			 * @param last the end of the character sequence
			 * @return the code point
			 */
			template<typename InputIterator>
			inline CodePoint decodeFirst(InputIterator first, InputIterator last) /*throw()*/ {
				ASCENSION_STATIC_ASSERT(CodeUnitSizeOf<InputIterator>::result == 2);
				assert(first != last);
				const Char high = *first;
				return (++first != last) ? decode(high, *first) : high;
			}
			/**
			 * Converts the last surrogate pair in the given character sequence to the
			 * corresponding code point.
			 * @tparam BidirectionalIterator the bidirectional iterator represents a UTF-16
			 *                               character sequence
			 * @param first the beginning of the character sequence
			 * @param last the end of the character sequence
			 * @return the code point
			 */
			template<typename BidirectionalIterator>
			inline CodePoint decodeLast(
					BidirectionalIterator first, BidirectionalIterator last) /*throw()*/ {
				ASCENSION_STATIC_ASSERT(CodeUnitSizeOf<BidirectionalIterator>::result == 2);
				assert(first != last);
				const Char low = *--last;
				return (last != first && isLowSurrogate(low)
					&& isHighSurrogate(*--last)) ? decode(*last, low) : low;
			}
			/**
			 * Converts the specified code point to a corresponding surrogate pair.
			 * @tparam OutputIterator the output iterator represents a UTF-16 character sequence
			 * @param c the code point
			 * @param[out] dest the surrogate pair
			 * @retval 0 @a c is a surrogate. in this case, @a *dest will be @a c
			 * @retval 1 @a c is in BMP
			 * @retval 2 @a c is out of BMP
			 * @throw std#invalid_argument @a c can't be expressed by UTF-16
			 */
			template<typename OutputIterator>
			inline length_t encode(CodePoint c, OutputIterator dest) {
				ASCENSION_STATIC_ASSERT(CodeUnitSizeOf<OutputIterator>::result == 2);
				if(c < 0x00010000ul) {
					*dest = static_cast<Char>(c & 0xffffu);
					return !isSurrogate(c) ? 1 : 0;
				} else if(c <= 0x0010fffful) {
					*dest = highSurrogate(c);
					*++dest = lowSurrogate(c);
					return 2;
				}
				throw std::invalid_argument("the specified code point is not valid.");
			}
			/**
			 * Searches the next high-surrogate in the given character sequence.
			 * @tparam InputIterator the input iterator represents a UTF-16 character sequence
			 * @param start the start position to search
			 * @param last the end of the character sequence
			 * @return the next high-surrogate
			 */
			template<typename InputIterator>
			inline InputIterator next(InputIterator start, InputIterator last) /*throw()*/ {
				ASCENSION_STATIC_ASSERT(CodeUnitSizeOf<InputIterator>::result == 2);
				assert(start != last);
				return (isHighSurrogate(*(start++))
					&& (start != last) && isLowSurrogate(*start)) ? ++start : start;
			}
			/**
			 * Searches the previous high-surrogate in the given character sequence.
			 * @tparam BidirectionalIterator the bidirectional iterator represents a UTF-16
			 *                               character sequence
			 * @param first the beginning of the character sequence
			 * @param start the start position to search
			 * @return the previous high-surrogate
			 */
			template<typename BidirectionalIterator>
			inline BidirectionalIterator previous(
					BidirectionalIterator first, BidirectionalIterator start) /*throw()*/ {
				ASCENSION_STATIC_ASSERT(CodeUnitSizeOf<BidirectionalIterator>::result == 2);
				assert(first != start);
				return (!isLowSurrogate(*--start)
					|| (start == first) || isHighSurrogate(*--start)) ? start : ++start;
			}
			/**
			 * Searches an isolated surrogate character in the given UTF-16 code unit sequence.
			 * @note About UTF-32 code unit sequence, use <code>std#find_if(,,
			 *       std::ptr_fun(isSurrogate))</code> instead.
			 * @tparam InputIterator the input iterator represents a UTF-16 character sequence
			 * @param first the beginning of the character sequence
			 * @param last the end of the sequence
			 * @return the isolated surrogate or @a last if not found
			 */
			template<typename InputIterator>
			inline InputIterator searchIsolatedSurrogate(
					InputIterator first, InputIterator last) /*throw()*/ {
				ASCENSION_STATIC_ASSERT(CodeUnitSizeOf<InputIterator>::result == 2);
				while(first != last) {
					if(isLowSurrogate(*first))
						break;
					else if(isHighSurrogate(*first)) {
						const InputIterator high(first);
						if(++first == last || !isLowSurrogate(*first))
							return high;
					}
					++first;
				}
				return first;
			}
		} // namespace surrogates

		/**
		 * Returns the size of a code unit of the specified code unit sequence in bytes.
		 * @tparam CodeUnitSequence the type represents a code unit sequence
		 * @see ToUTF32Sequence
		 */
		template<typename CodeUnitSequence> struct CodeUnitSizeOf {
			enum {
				/// Byte size of the code unit.
				result = sizeof(typename std::iterator_traits<CodeUnitSequence>::value_type)
			};
		};
		template<typename T> struct CodeUnitSizeOf<std::back_insert_iterator<T> > {
			enum {result = sizeof(T::value_type)};
		};
		template<typename T> struct CodeUnitSizeOf<std::front_insert_iterator<T> > {
			enum {result = sizeof(T::value_type)};
		};
		template<typename T, typename U> struct CodeUnitSizeOf<std::ostream_iterator<T, U> > {
			enum {result = sizeof(T)};
		};

		class CharacterIterator {
		public:
			static const CodePoint DONE;
		public:
			/// Destructor.
			virtual ~CharacterIterator() /*throw()*/ {}
			// attributes
			/// Returns @c true if the iterator equals @a other.
			bool equals(const CharacterIterator& other) const {
				verifyOther(other);
				return doEquals(other);
			}
			/// Returns c true if the iterator is less than @a other.
			bool less(const CharacterIterator& other) const {
				verifyOther(other);
				return doLess(other);
			}
			/// Returns the position in the character sequence.
			std::ptrdiff_t offset() const /*throw()*/ {return offset_;}

			// operations
			/// Assigns the other iterator.
			CharacterIterator& assign(const CharacterIterator& other) {
				verifyOther(other);
				doAssign(other);
				return *this;
			}
			/// Creates a copy of the iterator.
			std::auto_ptr<CharacterIterator> clone() const {
				std::auto_ptr<CharacterIterator> p(doClone());
				if(p.get() != 0)
					p->offset_ = offset_;
				return p;
			}
			/// Moves to the start of the character sequence.
			CharacterIterator& first() {
				doFirst();
				offset_ = 0;
				return *this;
			}
			/// Moves to the end of the character sequence.
			CharacterIterator& last() {
				doLast();
				offset_ = 0;
				return *this;
			}
			/// Moves to the next code unit.
			CharacterIterator& next() {
				doNext();
				++offset_;
				return *this;
			}
			/// Moves to the previous code unit.
			CharacterIterator& previous() {
				doPrevious();
				--offset_;
				return *this;
			}

			// virtual methods the concrete class should implement
		public:
			/// Returns the current code point value.
			virtual CodePoint current() const /*throw()*/ = 0;
			/// Returns @c true if the iterator is not last.
			virtual bool hasNext() const /*throw()*/ = 0;
			/// Returns @c true if the iterator is not first.
			virtual bool hasPrevious() const /*throw()*/ = 0;
		protected:
			/// Identifies a concrete type of the derived class for relational operations.
			struct ConcreteTypeTag {};
		private:
			/// Called by @c #assign method.
			virtual void doAssign(const CharacterIterator& other) = 0;
			/// Called by @c #clone method.
			virtual std::auto_ptr<CharacterIterator> doClone() const = 0;
			/// Called by @c #equals method.
			virtual bool doEquals(const CharacterIterator& other) const = 0;
			/// Called by @c #first method.
			virtual void doFirst() = 0;
			/// Called by @c #last method.
			virtual void doLast() = 0;
			/// Called by @c #less method.
			virtual bool doLess(const CharacterIterator& other) const = 0;
			/// Called by @c #next method.
			virtual void doNext() = 0;
			/// Called by @c #previous method.
			virtual void doPrevious() = 0;

		protected:
			/// Protected constructor.
			explicit CharacterIterator(const ConcreteTypeTag& classID) /*throw()*/
				: offset_(0), classID_(&classID) {}
			/// Protected copy-constructor.
			CharacterIterator(const CharacterIterator& other) /*throw()*/
				: offset_(other.offset_), classID_(other.classID_) {}
			/// Protected assignment operator.
			CharacterIterator& operator=(const CharacterIterator& other) {
				offset_ = other.offset_; return *this;
			}
		private:
			void verifyOther(const CharacterIterator& other) const {
				if(classID_ != other.classID_) throw std::invalid_argument("type mismatch.");}
			std::ptrdiff_t offset_;
			const ConcreteTypeTag* const classID_;
		};

		/**
		 * Implementation of @c CharacterIterator for C string or @c String.
		 * @note This class is not intended to be subclassed.
		 */
		class StringCharacterIterator : public CharacterIterator,
			public StandardConstBidirectionalIteratorAdapter<StringCharacterIterator, CodePoint> {
		public:
			StringCharacterIterator() /*throw()*/;
			StringCharacterIterator(const StringPiece& text);
			StringCharacterIterator(const Range<const Char*>& text, const Char* start);
			StringCharacterIterator(const String& s, String::const_iterator start);
			StringCharacterIterator(const StringCharacterIterator& other) /*throw()*/;

			// attributes
			/// Returns the beginning position.
			const Char* beginning() const /*throw()*/ {return first_;}
			/// Returns the end position.
			const Char* end() const /*throw()*/ {return last_;}
			/// Returns the current position.
			const Char* tell() const /*throw()*/ {return current_;}

			// CharacterIterator
			/// @see CharacterIterator#current
			CodePoint current() const /*throw()*/ {
				return (current_ != last_) ? surrogates::decodeFirst(current_, last_) : DONE;
			}
			/// @see CharacterIterator#hasNext
			bool hasNext() const /*throw()*/ {return current_ != last_;}
			/// @see CharacterIterator#hasPrevious
			bool hasPrevious() const /*throw()*/ {return current_ != first_;}
		private:
			void doAssign(const CharacterIterator& other);
			std::auto_ptr<CharacterIterator> doClone() const;
			void doFirst();
			bool doEquals(const CharacterIterator& other) const;
			void doLast();
			bool doLess(const CharacterIterator& other) const;
			void doNext();
			void doPrevious();
		private:
			static const ConcreteTypeTag CONCRETE_TYPE_TAG_;
			const Char* current_;
			const Char* first_;
			const Char* last_;
		};

		/**
		 * Base class for @c UTF16To32Iterator bidirectional iterator scans a UTF-16 character
		 * sequence as UTF-32.
		 *
		 * Scanned UTF-16 sequence is given by the template parameter @a BaseIterator.
		 *
		 * This supports four relation operators general bidirectional iterators don't have.
		 * These are available if @a BaseIterator have these facilities.
		 * @tparam BaseIterator the base bidirectional iterator presents UTF-16 character sequence
		 * @tparam ConcreteIterator set to @c UTF16To32Iterator template class
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
		 * @tparam BaseIterator the base bidirectional iterator presents UTF-32 character sequence
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
