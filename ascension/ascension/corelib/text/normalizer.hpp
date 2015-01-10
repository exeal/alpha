/**
 * @file normalizer.hpp
 * @author exeal
 * @date 2005-2011 was unicode.hpp
 * @date 2011-04-26 separated from unicode.hpp
 * @date 2012-2015
 */

#ifndef ASCENSION_NORMALIZER_HPP
#define ASCENSION_NORMALIZER_HPP

#include <ascension/config.hpp>	// ASCENSION_NO_UNICODE_NORMALIZATION
#ifndef ASCENSION_NO_UNICODE_NORMALIZATION
#include <ascension/direction.hpp>
#include <ascension/corelib/text/character-iterator.hpp>	// CharacterIterator
#include <ascension/corelib/text/character.hpp>
#include <memory>		// std.unique_ptr
#include <stdexcept>	// std.out_of_range
#include <string>
#include <boost/iterator/iterator_facade.hpp>

#if ASCENSION_UNICODE_VERSION > 0x0510
#	error These class definitions and implementations are based on old version of Unicode.
#endif

namespace ascension {
	namespace text {
		class Normalizer : public boost::iterators::iterator_facade<
			Normalizer, const CodePoint, boost::iterators::bidirectional_traversal_tag> {
		public:
			/// Normalization forms.
			enum Form {
				FORM_C,		///< Normalization Form C.
				FORM_D,		///< Normalization Form D.
				FORM_KC,	///< Normalization Form KC.
				FORM_KD		///< Normalization Form KD.
			};

		public:
			Normalizer();
			template<typename CharacterIterator>
			Normalizer(const CharacterIterator& text, Form form);
			Normalizer(const Normalizer& other);
			Normalizer(Normalizer&& other) BOOST_NOEXCEPT;
			Normalizer&	operator=(const Normalizer& other);
			Normalizer& operator=(Normalizer&& other) BOOST_NOEXCEPT;

			/// @name Methods not in Standard Iterator Interface
			/// @{
			/** Returns @c false if the iterator addresses the end of the normalized text. */
			bool hasNext() const BOOST_NOEXCEPT {return characterIterator_.hasNext();}
			/// Returns @c false if the iterator addresses the start of the normalized text.
			bool hasPrevious() const BOOST_NOEXCEPT {return characterIterator_.hasPrevious() || indexInBuffer_ != 0;}
			/// Returns the current position in the input text that is being normalized.
			std::ptrdiff_t offset() const BOOST_NOEXCEPT {return characterIterator_.offset();}
			/// @}

		private:
			friend class boost::iterators::iterator_core_access;
			void nextClosure(Direction direction, bool initialize);
			// boost.iterator_facade
			/// Moves to the previous normalized character.
			void decrement() {
				if(!hasPrevious())
					throw NoSuchElementException("The iterator is the first");
				else if(indexInBuffer_ == 0)
					nextClosure(Direction::BACKWARD, false);
				else
					--indexInBuffer_;
			}
			/// Returns the current character in the normalized text.
			const CodePoint& dereference() const BOOST_NOEXCEPT {
				return normalizedBuffer_[indexInBuffer_];
			}
			/// Returns true if both iterators address the same character in the normalized text.
			bool equal(const Normalizer& other) const BOOST_NOEXCEPT {
				return /*characterIterator_.isCloneOf(*other.characterIterator_)
					&&*/ characterIterator_.offset() == other.characterIterator_.offset()
					&& indexInBuffer_ == other.indexInBuffer_;
			}
			/// Moves to the next normalized character.
			void increment() {
				if(!hasNext())
					throw NoSuchElementException("The iterator is the last.");
				else if(++indexInBuffer_ == normalizedBuffer_.length())
					nextClosure(Direction::FORWARD, false);
			}
		private:
			Form form_;
			detail::CharacterIterator characterIterator_;
			std::basic_string<CodePoint> normalizedBuffer_;
			std::size_t indexInBuffer_;
			std::ptrdiff_t nextOffset_;
		};

		/// @defgroup normalization_free_functions Free Functions Related-to Unicode Normalization
		/// @{
		int compareForCanonicalEquivalence(const String& s1, const String& s2, CaseSensitivity caseSensitivity);
		Normalizer::Form formForName(const Char* name);
		String normalize(CodePoint c, Normalizer::Form form);
		template<typename CharacterIterator> String normalize(const CharacterIterator& text, Normalizer::Form form);
		/// @}

		/**
		 * Constructor.
		 * @tparam CharacterIterator The type of @a text. Should satisfy @c detail#CharacterIteratorConcepts concept
		 * @param text The text to be normalized
		 * @param form The normalization form
		 */
		template<typename CharacterIterator>
		inline Normalizer::Normalizer(const CharacterIterator& text, Form form) : form_(form), characterIterator_(text) {
			nextClosure(Direction::FORWARD, true);
		}

		/**
		 * Normalizes the specified text according to the normalization form.
		 * @tparam CharacterIterator The type of @a text. Should satisfy @c detail#CharacterIteratorConcepts concept
		 * @param text The text to normalize
		 * @param form The normalization form
		 */
		template<typename CharacterIterator>
		inline String text::normalize(const CharacterIterator& text, Normalizer::Form form) {
			// TODO: There is more efficient implementation.
			Normalizer n(text, form);
			std::basic_stringbuf<Char> buffer(std::ios_base::out);
			CodePoint c;
			Char surrogates[2];
			for(Normalizer n(text, form); n.hasNext(); ++n) {
				c = *n;
				if(c < 0x010000ul)
					buffer.sputc(static_cast<Char>(c & 0xffffu));
				else {
					assert(isScalarValue(c));
					Char* temp = surrogates;
					utf::encode(c, temp);
					buffer.sputn(surrogates, 2);
				}
			}
			return buffer.str();
		}
	}
} // namespace ascension.text

#endif // !ASCENSION_NO_UNICODE_NORMALIZATION
#endif // !ASCENSION_UNICODE_HPP
