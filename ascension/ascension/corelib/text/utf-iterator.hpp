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

			template<typename Derived, typename BaseIterator, typename UChar32 = CodePoint>
			class CharacterDecodeIteratorBase : public detail::IteratorAdapter<
				Derived,
				std::iterator<
					std::bidirectional_iterator_tag, UChar32,
					typename std::iterator_traits<BaseIterator>::difference_type,
					const UChar32*, const UChar32
				>
			> {
			public:
				/// Default constructor.
				CharacterDecodeIteratorBase() : extractedBytes_(0) {}
				/// Copy-constructor.
				CharacterDecodeIteratorBase(const CharacterDecodeIteratorBase& other) :
					base_(other.base_), extractedBytes_(other.extractedBytes_), cache_(other.cache_) {}
				/// Constructor takes a position to start iteration.
				CharacterDecodeIteratorBase(BaseIterator start) : base_(start), extractedBytes_(0) {}
				/// Assignment operator.
				CharacterDecodeIteratorBase& operator=(const CharacterDecodeIteratorBase& other) {
					base_ = other.base_;
					extractedBytes_ = other.extractedBytes_;
					cache_ = other.cache_;
					return *this;
				}
				/// Returns the current position.
				BaseIterator tell() const {return base_;}
			private:
				template<std::size_t codeUnitSize> void decrement();
				template<> void decrement<1>() {
				}
				template<> void decrement<2>() {
					if(surrogates::isLowSurrogate(*--base_) && derived().canDecrement())
						--base_;
					extractedBytes_ = 0;
				}
				template<> void decrement<4>() {
					--base_;
					extractedBytes_ = 0;
				}
				Derived& derived() {return *static_cast<Derived*>(this);}
				const Derived& derived() const {return *static_cast<const Derived*>(this);}
				template<std::size_t codeUnitSize> void extract() const;
				template<> void extract<1>() const {	// UTF-8
					uint8_t bytes[4];
					std::size_t nbytes = utf8::length(bytes[0] = *base_);
					if(nbytes != 0) {
						Derived p(derived());
						for(std::size_t i = 1; i < nbytes; ++i) {
							++p.base_;
							if(!p.canIncrement()) {
								nbytes = 0;
								break;
							}
							bytes[i] = *p.base_;
						}
					}
					if(nbytes == 0) {	// ill-formed
						extractedBytes_ = 1;
						cache_ = REPLACEMENT_CHARACTER;
						return;
					}
					cache_ = utf8::decodeUnsafe(bytes);
					extractedBytes_ = nbytes;
				}
				template<> void extract<2>() const {	// UTF-16
					value_type c(*base_);
					if(surrogates::isHighSurrogate(c)) {
						Derived low(derived());
						++low.base_;
						if(low.canIncrement() && surrogates::isLowSurrogate(*low.base_))
							c = surrogates::decode(static_cast<Char>(c), *low.base_);
//						else
//							c = REPLACEMENT_CHARACTER;
					}
					cache_ = c;
					extractedBytes_ = (c < 0x10000ul) ? 1 : 2;
				}
				template<> void extract<4>() const {	// UTF-32
					extractedBytes_ = 1;
					cache_ = *base_;
				}
				// detail.IteratorAdapter
				friend class detail::IteratorCoreAccess;
				value_type current() const {
					if(extractedBytes_ == 0) {
						if(!derived().canIncrement())
							throw IllegalStateException("The iterator is last.");
						extract<CodeUnitSizeOf<BaseIterator>::value>();
					}
					return cache_;
				}
				bool equals(const CharacterDecodeIteratorBase<Derived, BaseIterator, UChar32>& other) const {
					return base_ == other.base_;
				}
				bool less(const CharacterDecodeIteratorBase<Derived, BaseIterator, UChar32>& other) const {
					return base_ < other.base_;
				}
				void next() {
					if(extractedBytes_ == 0)
						current();
					std::advance(base_, extractedBytes_);
					extractedBytes_ = 0;
				}
				void previous() {
					if(!derived().canDecrement())
						throw IllegalStateException("The iterator is first.");
					decrement<CodeUnitSizeOf<BaseIterator>::value>();
				}
			private:
				BaseIterator base_;
				mutable uint8_t extractedBytes_ : 3;
				mutable UChar32 cache_ : 29;
			};

			template<typename BaseIterator, typename UChar32 = CodePoint>
			class CharacterDecodeIteratorUnsafe : public CharacterDecodeIteratorBase<
				CharacterDecodeIteratorUnsafe<BaseIterator, UChar32>, BaseIterator, UChar32> {
			private:
				typedef CharacterDecodeIteratorBase<
					CharacterDecodeIteratorUnsafe<BaseIterator, UChar32>, BaseIterator, UChar32> Base;
			public:
				/// Default constructor.
				CharacterDecodeIteratorUnsafe() {}
				/// Constructor takes a position to start iteration.
				CharacterDecodeIteratorUnsafe(BaseIterator start) : Base(start) {}
				/// Copy-constructor.
				CharacterDecodeIteratorUnsafe(const CharacterDecodeIteratorUnsafe& other) : Base(other) {}
			private:
				friend class Base;
				bool canDecrement() const /*throw()*/ {return true;}
				bool canIncrement() const /*throw()*/ {return true;}
			};

			template<typename BaseIterator, typename UChar32 = CodePoint>
			class CharacterDecodeIterator : public CharacterDecodeIteratorBase<
				CharacterDecodeIterator<BaseIterator, UChar32>, BaseIterator, UChar32> {
			private:
				typedef CharacterDecodeIteratorBase<
					CharacterDecodeIterator<BaseIterator, UChar32>, BaseIterator, UChar32> Base;
			public:
				/// Default constructor.
				CharacterDecodeIterator() {}
				/**
				 * Constructor takes a position to start iteration. The ownership of the target text
				 * will not be transferred to this.
				 */
				CharacterDecodeIterator(BaseIterator first, BaseIterator last) :
					Base(first), first_(first), last_(last) {}
				/**
				 * Constructor takes a position to start iteration. The ownership of the target text
				 * will not be transferred to this.
				 */
				CharacterDecodeIterator(BaseIterator first, BaseIterator last,
					BaseIterator start) : Base(start), first_(first), last_(last) {}
				/// Copy constructor.
				CharacterDecodeIterator(const CharacterDecodeIterator& other) :
					Base(other), first_(other.first_), last_(other.last_) {}
				/// Assignment operator.
				CharacterDecodeIterator& operator=(const CharacterDecodeIterator& other) {
					Base::operator=(other);
					first_ = other.first_;
					last_ = other.last_;
					return *this;
				}
			private:
				friend class Base;
				bool canDecrement() const {return tell() != first_;}
				bool canIncrement() const {return tell() != last_;}
			private:
				BaseIterator first_, last_;
			};

			template<std::size_t codeUnitSize> struct DefaultByte;
			template<> struct DefaultByte<1> {typedef uint8_t Type;};
			template<> struct DefaultByte<2> {typedef Char Type;};
			template<> struct DefaultByte<4> {typedef CodePoint Type;};

			template<typename BaseIterator,
				typename Byte = typename DefaultByte<CodeUnitSizeOf<BaseIterator>::value>::Type>
			class CharacterEncodeIterator : public detail::IteratorAdapter<
				CharacterEncodeIterator<BaseIterator, Byte>,
				std::iterator<
					std::bidirectional_iterator_tag, Byte,
					typename std::iterator_traits<BaseIterator>::difference_type,
					const Byte*, const Byte
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
				/// Returns the current position.
				BaseIterator tell() const {return base_;}
			private:
				template<std::size_t codeUnitSize> void doExtract() const;
				template<> void doExtract<1>() const {
				}
				template<> void doExtract<2>() const {
					const CodePoint c = *base_;
					std::size_t extractedBytes;
#if 0
					if(isScalarValue(c))
#else
					if(isValidCodePoint())
#endif
						extractedBytes = utf16::uncheckedEncode(c, cache_);
					else {
						cache_[0] = REPLACEMENT_CHARACTER;
						extractedBytes = 1;
					}
					std::fill(cache_ + extractedBytes, ASCENSION_ENDOF(cache_), 0);
				}
				template<> void doExtract<4>() const {cache_[0] = *base_;}
				void extract() const {
					doExtract<CodeUnitSizeOf<BaseIterator>::value>();
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
					return base_ == other.base_ && positionInCache_ == other.positionInCache_;
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
				mutable Byte cache_[4 / CodeUnitSizeOf<BaseIterator>::value + 1];
				mutable Byte* positionInCache_;
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
				/// Assignment operator.
				void operator=(CodePoint c) {write<CodeUnitSizeOf<BaseIterator>::value>();}
				/// Returns the current position.
				BaseIterator tell() const {return base_;}
			private:
				template<std::size_t codeUnitSize> void write(CodePoint c);
				template<> void write<1>(CodePoint c) {utf8::encode(c, base_);}
				template<> void write<2>(CodePoint c) {utf16::encode(c, base_);}
				template<> void write<4>(CodePoint c) {*base_ = c; ++base_;}
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
			template<typename BaseIterator>
			inline CharacterDecodeIteratorUnsafe<BaseIterator> makeCharacterDecodeIteratorUnsafe(BaseIterator start) {
				return CharacterDecodeIteratorUnsafe<BaseIterator>(start);
			}
			template<typename BaseIterator>
			inline CharacterEncodeIterator<BaseIterator> makeCharacterEncodeIterator(BaseIterator start) {
				return CharacterEncodeIterator<BaseIterator>(start);
			}

		}
	}
}

#endif // !ASCENSION_UTF_ITERATOR_HPP
