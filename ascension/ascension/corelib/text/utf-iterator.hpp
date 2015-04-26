/**
 * @file utf-iterator.hpp
 * @author exeal
 * @date 2011-08-21 created
 */

#ifndef ASCENSION_UTF_ITERATOR_HPP
#define ASCENSION_UTF_ITERATOR_HPP

#include <ascension/corelib/basic-exceptions.hpp>
#include <ascension/corelib/text/character.hpp>
#include <ascension/corelib/text/utf.hpp>
#include <array>
#include <utility>	// std.advance
#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/iterator_traits.hpp>

namespace ascension {
	namespace text {
		namespace utf {

			/**
			 * Provides the default type for Unicode code unit.
			 * @tparam codeUnitSize The byte size of code unit
			 */
			template<std::size_t codeUnitSize> struct DefaultByte;
			template<> struct DefaultByte<1> {typedef std::uint8_t Type;};
			template<> struct DefaultByte<2> {typedef Char Type;};
			template<> struct DefaultByte<4> {typedef CodePoint Type;};

			/**
			 * Converts UTF-x character sequence into UCS-4.
			 * @tparam BaseIterator The base iterator represents UTF-x character sequence
			 * @tparam UChar32 The returned UCS-4 character type
			 * @see CharacterEncodeIterator, CharacterOutputIterator, makeCharacterDecodeIterator
			 */
			template<typename BaseIterator, typename UChar32 = CodePoint>
			class CharacterDecodeIterator : public boost::iterators::iterator_facade<
				CharacterDecodeIterator<BaseIterator, UChar32>, UChar32,
				boost::iterators::bidirectional_traversal_tag, const UChar32,
				typename std::iterator_traits<BaseIterator>::difference_type
			> {
				static_assert(sizeof(UChar32) == 4, "UChar32 should be UCS-4 character type.");
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
				const BaseIterator& first() const BOOST_NOEXCEPT {return first_;}
				/// Returns end of the range this iterator can address.
				const BaseIterator& last() const BOOST_NOEXCEPT {return first_;}
				/// Sets if this iterator replaces the ill-formed code unit (sub)sequence.
				CharacterDecodeIterator& replaceMalformedInput(bool replace) BOOST_NOEXCEPT {
					replacesMalformedInput_ = replace;
					return *this;
				}
				/**
				 * Returns @c true if this iterator replaces the ill-formed code unit
				 * (sub)sequence. The default value is @c false.
				 */
				bool replacesMalformedInput() const BOOST_NOEXCEPT {return replacesMalformedInput_;}
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
				// boost.iterator_facade
				friend class boost::iterators::iterator_core_access;
				void decrement() {
					if(base_ == first_)
						throw IllegalStateException("The iterator is first.");
					decrement<CodeUnitSizeOf<BaseIterator>::value>();
				}
				typename boost::iterators::iterator_value<CharacterDecodeIterator>::type dereference() const {
					if(extractedBytes_ == 0) {
						if(base_ == last_)
							throw IllegalStateException("The iterator is last.");
						extract();
					}
					return cache_;
				}
				typename boost::iterators::iterator_difference<CharacterDecodeIterator>::type distance_to(const CharacterDecodeIterator<BaseIterator, UChar32>& other) const {
					return base_ - other.base_;
				}
				bool equal(const CharacterDecodeIterator<BaseIterator, UChar32>& other) const {
					return base_ == other.base_;
				}
				void increment() {
					if(extractedBytes_ == 0)
						dereference();
					std::advance(base_, extractedBytes_);
					extractedBytes_ = 0;
				}
			private:
				BaseIterator base_, first_, last_;
				bool replacesMalformedInput_ : 1;
				mutable std::uint8_t extractedBytes_ : 3;
				mutable UChar32 cache_ : 28;
			};

			/**
			 * Converts UCS-4 into UTF-x character sequence.
			 * @tparam BaseIterator The base iterator. This should represents UCS-4 or UTF-32 character sequence
			 * @tparam CodeUnit The returned code unit type
			 * @see CharacterDecodeIterator, CharacterOutputIterator, makeCharacterEncodeIterator
			 */
			template<typename BaseIterator, typename CodeUnit>
			class CharacterEncodeIterator : public boost::iterators::iterator_facade<
				CharacterEncodeIterator<BaseIterator, CodeUnit>, CodeUnit,
				boost::bidirectional_traversal_tag, const CodeUnit,
				typename std::iterator_traits<BaseIterator>::difference_type
			> {
				static_assert(CodeUnitSizeOf<BaseIterator>::value == 4,
					"BaseIterator should be UCS-4 or UTF-32 character sequence.");
			public:
				/// Default constructor.
				CharacterEncodeIterator() {
					cache_.fill(0);
					positionInCache_ = cache_.end();
				}
				/**
				 * Constructor takes a position to start iteration. The ownership of the target text
				 * will not be transferred to this.
				 */
				CharacterEncodeIterator(BaseIterator start) : base_(start) {
					cache_.fill(0);
					positionInCache_ = cache_.end();
				}
				/// Copy-constructor.
				CharacterEncodeIterator(const CharacterEncodeIterator& other) : base_(other.base_), cache_(other.cache_) {
					positionInCache_ = (other.positionInCache_ != other.cache_.end()) ?
						cache_.begin() + (other.positionInCache_ - other.cache_.begin()) : cache_.end();
				}
				/// Assignment operator.
				CharacterEncodeIterator& operator=(const CharacterEncodeIterator& other) {
					base_ = other.base_;
					cache_ = other.cache_;
					positionInCache_ = (other.positionInCache_ != other.cache_.end()) ?
						cache_.begin() + (other.positionInCache_ - other.cache_) : 0;
				}
				/// Returns the current position.
				BaseIterator tell() const {return base_;}

			private:
				void extract() const {
					CodeUnit* out = cache_.data();
					const std::size_t extractedBytes = checkedEncode(*base_, out);
					if(sizeof(CodeUnit) != 4)
						std::fill(cache_.begin() + extractedBytes, cache_.end(), 0);
					positionInCache_ = cache_.begin();
				}
			private:
				// boost.iterator_facade
				friend class boost::iterators::iterator_core_access;
				void decrement() {
					if(positionInCache_ != cache_.end() && positionInCache_ != cache_.begin())
						--positionInCache_;
					else {
						--base_;
						extract();
						positionInCache_ = --cache_.end();
						while(positionInCache_ != cache_.begin() && *positionInCache_ == 0)
							--positionInCache_;
					}
				}
				typename boost::iterators::iterator_value<CharacterEncodeIterator>::type dereference() const {
					if(positionInCache_ == cache_.end())
						extract();
					return *positionInCache_;
				}
				bool equal(const CharacterEncodeIterator& other) const {
					if(base_ != other.base_)
						return false;
					const std::ptrdiff_t offsets[2] = {
						(positionInCache_ != cache_.end()) ? positionInCache_ - cache_.begin() : 2,
						(other.positionInCache_ != other.cache_.end()) ? other.positionInCache_ - other.cache_.begin() : 2
					};
					return ((offsets[0] - offsets[1]) & 1) == 0;
				}
				typename boost::iterators::iterator_difference<CharacterEncodeIterator>::type distance_to(const CharacterEncodeIterator& other) const {
					return base_ - other.base_;
				}
				void increment() {
					if(positionInCache_ == cache_.end())
						extract();
					if(*++positionInCache_ == 0) {
						++base_;
						positionInCache_ = cache_.end();
					}
				}
//				bool less(const CharacterEncodeIterator& other) const {
//					return base_ < other.base_
//						|| (base_ == other.base_ && positionInCache_ - cache_.begin() < other.positionInCache_ - other.cache_.begin());
//				}
			private:
				BaseIterator base_;
				typedef std::array<CodeUnit, 4 / sizeof(CodeUnit) + 1> CacheType;
				mutable CacheType cache_;
				mutable typename CacheType::iterator positionInCache_;
			};

			/**
			 * Writes code point sequence into UTF-x character sequence.
			 * @tparam BaseIterator The base output iterator represents UTF-x character sequence
			 * @see CharacterEncodeIterator, CharacterDecodeIterator
			 */
			template<typename BaseIterator>
			class CharacterOutputIterator : public boost::iterators::iterator_facade<
				CharacterOutputIterator<BaseIterator>, void,
				std::output_iterator_tag, CodePoint&, void
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
				// boost.iterator_facade
				friend class boost::iterators::iterator_core_access;
				CharacterOutputIterator& dereference() const {return *this;}
				void increment() {}
			private:
				BaseIterator base_;
			};

			/// Makes and returns @c CharacterDecodeIterator from the base iterators.
			template<typename BaseIterator>
			inline CharacterDecodeIterator<BaseIterator> makeCharacterDecodeIterator(BaseIterator first, BaseIterator last) {
				return CharacterDecodeIterator<BaseIterator>(first, last);
			}

			/// Makes and returns @c CharacterDecodeIterator from the base iterators and the start position.
			template<typename BaseIterator>
			inline CharacterDecodeIterator<BaseIterator> makeCharacterDecodeIterator(BaseIterator first, BaseIterator last, BaseIterator start) {
				return CharacterDecodeIterator<BaseIterator>(first, last, start);
			}

			/// Makes and returns @c CharacterEncodeIterator from the base iterator.
			template<typename CodeUnit, typename BaseIterator>
			inline CharacterEncodeIterator<BaseIterator, CodeUnit> makeCharacterEncodeIterator(BaseIterator start) {
				return CharacterEncodeIterator<BaseIterator, CodeUnit>(start);
			}

			/// Converts UTF-8 into UTF-16
			template<std::size_t length>
			inline String decode(const char(&source)[length]) {
				return String(
					makeCharacterDecodeIterator(source, source + length - 1),
					makeCharacterDecodeIterator(source, source + length - 1, source + length - 1));
			}

		}
	}
}

#endif // !ASCENSION_UTF_ITERATOR_HPP
