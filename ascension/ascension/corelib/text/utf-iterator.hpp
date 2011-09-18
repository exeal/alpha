/**
 * @file utf-iterator.hpp
 * @author exeal
 * @date 2011-08-21 created
 */

#ifndef ASCENSION_UTF_ITERATOR_HPP
#define ASCENSION_UTF_ITERATOR_HPP

#include <ascension/corelib/basic-exceptions.hpp>
#include <ascension/corelib/standard-iterator-adapter.hpp>	// IllegalStateException
#include <ascension/corelib/text/character.hpp>
#include <ascension/corelib/text/utf.hpp>

namespace ascension {
	namespace text {
		namespace utf {

			template<std::size_t codeUnitSize> struct DefaultByte;
			template<> struct DefaultByte<1> {typedef uint8_t Type;};
			template<> struct DefaultByte<2> {typedef Char Type;};
			template<> struct DefaultByte<4> {typedef CodePoint Type;};

			template<typename BaseIterator, typename UChar32 = CodePoint>
			class CharacterDecodeIterator : public detail::IteratorAdapter<
				CharacterDecodeIterator<BaseIterator, UChar32>,
				std::iterator<
					std::bidirectional_iterator_tag, UChar32,
					typename std::iterator_traits<BaseIterator>::difference_type,
					const UChar32*, const UChar32
				>
			> {
				ASCENSION_STATIC_ASSERT(sizeof(UChar32) == 4);
			public:
				/// Default constructor.
				CharacterDecodeIterator() : extractedBytes_(0) {}
				/**
				 * Constructor takes a position to start iteration. The ownership of the target text
				 * will not be transferred to this.
				 */
				CharacterDecodeIterator(BaseIterator first, BaseIterator last) :
					base_(first), first_(first), last_(last), extractedBytes_(0) {}
				/**
				 * Constructor takes a position to start iteration. The ownership of the target text
				 * will not be transferred to this.
				 */
				CharacterDecodeIterator(BaseIterator first, BaseIterator last,
					BaseIterator start) : base_(start), first_(first), last_(last), extractedBytes_(0) {}
				/// Copy constructor.
				CharacterDecodeIterator(const CharacterDecodeIterator& other) :
					base_(other.base_), first_(other.first_), last_(other.last_),
					extractedBytes_(other.extractedBytes_), cache_(other.cache_) {}
				/// Assignment operator.
				CharacterDecodeIterator& operator=(const CharacterDecodeIterator& other) {
					base_ = other.base_;
					first_ = other.first_;
					last_ = other.last_;
					extractedBytes_ = other.extractedBytes_;
					cache_ = other.cache_;
					return *this;
				}
				/// Returns beginning of the range this iterator can address.
				const BaseIterator& first() const /*throw()*/ {return first_;}
				/// Returns end of the range this iterator can address.
				const BaseIterator& last() const /*throw()*/ {return first_;}
				/// Sets if this iterator replaces the ill-formed code unit (sub)sequence.
				CharacterDecodeIterator& replaceMalformedInput(bool replace) /*throw()*/ {
					replacesMalformedInput_ = replace;
					return *this;
				}
				/**
				 * Returns @c true if this iterator replaces the ill-formed code unit
				 * (sub)sequence. The default value is @c false.
				 */
				bool replacesMalformedInput() const /*throw()*/ {return replacesMalformedInput_;}
				/// Returns the current position.
				BaseIterator tell() const {return base_;}
			private:
				template<std::size_t codeUnitSize> void decrement();
				template<> void decrement<1>() {
					BaseIterator i(base_);
					--i;
					std::size_t numberOfReadBytes = 1;
					for(; numberOfReadBytes <= 4; ++numberOfReadBytes, --i) {
						if(isLeadingByte(*i))
							break;
						else if(!isValidByte(*i)) {
							if(!replacesMalformedInput())
								throw MalformedInputException<BaseIterator>(i, 1);
							--base_;
							extractedBytes_ = 1;
							cache_ = REPLACEMENT_CHARACTER;
							return;
						}
						assert(maybeTrailingByte(*i));
					}
					if(length(*i) != numberOfReadBytes) {
						if(!replacesMalformedInput())
							throw MalformedInputException<BaseIterator>(i, numberOfReadBytes);
						base_ = i;
						extractedBytes_ = numberOfReadBytes;
						cache_ = REPLACEMENT_CHARACTER;
					} else
						base_ = i;
				}
				template<> void decrement<2>() {
					if(replacesMalformedInput()) {
						if(surrogates::isLowSurrogate(*--base_) && base_ != first()) {
							BaseIterator i(base_);
							if(surrogates::isHighSurrogate(*--i))
								base_ = i;
						}
					} else {
						BaseIterator i(base_);
						if(surrogates::isLowSurrogate(*--i)) {
							if(i == first() || !surrogates::isHighSurrogate(*--i))
								throw MalformedInputException<BaseIterator>(i, 1);
						}
						base_ = i;
					}
					extractedBytes_ = 0;
				}
				template<> void decrement<4>() {
					if(replacesMalformedInput())
						--base_;
					else {
						BaseIterator previous(base_);
						if(!isScalarValueException(*--previous))
							throw MalformedInputException<BaseIterator>(previous, 1);
						base_ = previous;
					}
					extractedBytes_ = 0;
				}
				void extract() const {
					static const std::size_t CODE_UNIT_SIZE = CodeUnitSizeOf<BaseIterator>::value;
					try {
						cache_ = checkedDecodeFirst(base_, last_);
					} catch(const MalformedInputException<BaseIterator>& e) {
						if(!replacesMalformedInput())
							throw;
						extractedBytes_ = e.maximalSubpartLength();
						cache_ = REPLACEMENT_CHARACTER;
						return;
					}
					extractedBytes_ = numberOfEncodedBytes<CODE_UNIT_SIZE>(cache_);
				}
				// detail.IteratorAdapter
				friend class detail::IteratorCoreAccess;
				value_type current() const {
					if(extractedBytes_ == 0) {
						if(base_ == last_)
							throw IllegalStateException("The iterator is last.");
						extract();
					}
					return cache_;
				}
				bool equals(const CharacterDecodeIterator<BaseIterator, UChar32>& other) const {
					return base_ == other.base_;
				}
				bool less(const CharacterDecodeIterator<BaseIterator, UChar32>& other) const {
					return base_ < other.base_;
				}
				void next() {
					if(extractedBytes_ == 0)
						current();
					std::advance(base_, extractedBytes_);
					extractedBytes_ = 0;
				}
				void previous() {
					if(base_ == first_)
						throw IllegalStateException("The iterator is first.");
					decrement<CodeUnitSizeOf<BaseIterator>::value>();
				}
			private:
				BaseIterator base_, first_, last_;
				bool replacesMalformedInput_ : 1;
				mutable uint8_t extractedBytes_ : 3;
				mutable UChar32 cache_ : 28;
			};

			template<typename BaseIterator, typename CodeUnit>
			class CharacterEncodeIterator : public detail::IteratorAdapter<
				CharacterEncodeIterator<BaseIterator, CodeUnit>,
				std::iterator<
					std::bidirectional_iterator_tag, CodeUnit,
					typename std::iterator_traits<BaseIterator>::difference_type,
					const CodeUnit*, const CodeUnit
				>
			> {
				ASCENSION_STATIC_ASSERT(CodeUnitSizeOf<BaseIterator>::value == 4);
			public:
				/// Default constructor.
				CharacterEncodeIterator() : positionInCache_(0) {
					std::fill(cache_, ASCENSION_ENDOF(cache)_, 0);
				}
				/**
				 * Constructor takes a position to start iteration. The ownership of the target text
				 * will not be transferred to this.
				 */
				CharacterEncodeIterator(BaseIterator start) : base_(start), positionInCache_(0) {
					std::fill(cache_, ASCENSION_ENDOF(cache_), 0);
				}
				/// Copy-constructor.
				CharacterEncodeIterator(const CharacterEncodeIterator& other) : base_(other.base_) {
					std::copy(other.cache_, ASCENSION_ENDOF(other.cache_), cache_);
					positionInCache_ = (other.positionInCache_ != 0) ?
						cache_ + (other.positionInCache_ - other.cache_) : 0;
				}
				/// Assignment operator.
				CharacterEncodeIterator& operator=(const CharacterEncodeIterator& other) {
					base_ = other.base_;
					std::copy(other.cache_, ASCENSION_ENDOF(other.cache_), cache_);
					positionInCache_ = (other.positionInCache_ != 0) ?
						cache_ + (other.positionInCache_ - other.cache_) : 0;
				}
				/// Returns the current position.
				BaseIterator tell() const {return base_;}
			private:
				void extract() const {
					CodeUnit* out = cache_;
					const std::size_t extractedBytes = checkedEncode(*base_, out);
					if(sizeof(CodeUnit) != 4)
						std::fill(cache_ + extractedBytes, ASCENSION_ENDOF(cache_), 0);
					positionInCache_ = cache_;
				}
			private:
				// detail.IteratorAdapter
				friend class detail::IteratorCoreAccess;
				value_type current() const {
					if(positionInCache_ == 0)
						extract();
					return *positionInCache_;
				}
				bool equals(const CharacterEncodeIterator& other) const {
					if(base_ != other.base_)
						return false;
					return (positionInCache_ == other.positionInCache_)
						|| (positionInCache_ == 0 && other.positionInCache_ == other.cache_)
						|| (positionInCache_ == cache_ && other.positionInCache_ == 0);
				}
				bool less(const CharacterEncodeIterator& other) const {
					return base_ < other.base_ || (base_ == other.base_ && positionInCache_ < other.positionInCache_);
				}
				void next() {
					if(positionInCache_ == 0)
						extract();
					if(*++positionInCache_ == 0) {
						++base_;
						positionInCache_ = 0;
					}
				}
				void previous() {
					if(positionInCache_ != 0 && positionInCache_ != cache_)
						--positionInCache_;
					else {
						--base_;
						extract();
						positionInCache_ = ASCENSION_ENDOF(cache_) - 1;
						while(positionInCache_ > cache_ && *positionInCache_ == 0)
							--positionInCache_;
					}
				}
			private:
				BaseIterator base_;
				mutable CodeUnit cache_[4 / sizeof(CodeUnit) + 1];
				mutable CodeUnit* positionInCache_;
			};

			template<typename BaseIterator>
			class CharacterOutputIterator : public detail::IteratorAdapter<
				CharacterOutputIterator<BaseIterator>,
				std::iterator<std::output_iterator_tag, void, void, CodePoint*, CodePoint&>
			> {
			public:
				/// Constructor takes base output iterator.
				CharacterOutputIterator(const BaseIterator& base) : base_(base) {}
				/// Copy-constructor.
				CharacterOutputIterator(const CharacterOutputIterator& other) : base_(other.base_) {}
				/// Assignment operator.
				CharacterOutputIterator& operator=(const CharacterOutputIterator& other) {
					base_ = other.base_;
					return *this;
				}
				/**
				 * Assignment operator.
				 * @param c The code point of the character to write
				 * @throw InvalidCodePointException @a c is invalid
				 * @throw InvalidScalarValueException @a c is invalid
				 */
				void operator=(CodePoint c) {utf::checkedEncode(c, base_);}
				/// Returns the current position.
				BaseIterator tell() const {return base_;}
			private:
				// detail.IteratorAdapter
				friend class detail::IteratorCoreAccess;
				CharacterOutputIterator& current() const {return *this;}
				void next() {}
			private:
				BaseIterator base_;
			};

			template<typename BaseIterator>
			inline CharacterDecodeIterator<BaseIterator> makeCharacterDecodeIterator(BaseIterator first, BaseIterator last) {
				return CharacterDecodeIterator<BaseIterator>(first, last);
			}
			template<typename BaseIterator>
			inline CharacterDecodeIterator<BaseIterator> makeCharacterDecodeIterator(BaseIterator first, BaseIterator last, BaseIterator start) {
				return CharacterDecodeIterator<BaseIterator>(first, last, start);
			}
			template<typename CodeUnit, typename BaseIterator>
			inline CharacterEncodeIterator<BaseIterator, CodeUnit> makeCharacterEncodeIterator(BaseIterator start) {
				return CharacterEncodeIterator<BaseIterator, CodeUnit>(start);
			}

		}
	}
}

#endif // !ASCENSION_UTF_ITERATOR_HPP
