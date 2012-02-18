/**
 * @file normalizer.hpp
 * @author exeal
 * @date 2005-2011 was unicode.hpp
 * @date 2011-04-26 separated from unicode.hpp
 * @date 2012
 */

#ifndef ASCENSION_NORMALIZER_HPP
#define ASCENSION_NORMALIZER_HPP

#include <ascension/config.hpp>	// ASCENSION_NO_UNICODE_NORMALIZATION
#ifndef ASCENSION_NO_UNICODE_NORMALIZATION
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

		class Normalizer : public boost::iterator_facade<
			Normalizer, CodePoint, std::bidirectional_iterator_tag> {
		public:
			/// Normalization forms.
			enum Form {
				FORM_C,		///< Normalization Form C.
				FORM_D,		///< Normalization Form D.
				FORM_KC,	///< Normalization Form KC.
				FORM_KD		///< Normalization Form KD.
			};
			// constructors
			Normalizer();
			Normalizer(const CharacterIterator& text, Form form);
			Normalizer(const Normalizer& other);
			Normalizer(Normalizer&& other) /*noexcept*/;
			// operator
			Normalizer&	operator=(const Normalizer& other);
			Normalizer& operator=(Normalizer&& other) /*noexcept*/;

			// attributes
			/// Returns @c false if the iterator addresses the end of the normalized text.
			bool hasNext() const /*throw()*/ {return current_->hasNext();}
			/// Returns @c false if the iterator addresses the start of the normalized text.
			bool hasPrevious() const /*throw()*/ {return current_->hasPrevious() || indexInBuffer_ != 0;}
			/// Returns the current position in the input text that is being normalized.
			std::ptrdiff_t offset() const /*throw()*/ {return current_->offset();}

			// class operations
			static int compare(const String& s1, const String& s2, CaseSensitivity caseSensitivity);
			static Form formForName(const Char* name);
			static String normalize(CodePoint c, Form form);
			static String normalize(const CharacterIterator& text, Form form);
			// methods
			/// Moves to the previous normalized character.
			Normalizer& decrement() {
				if(!hasPrevious())
					throw std::out_of_range("the iterator is the first");
				else if(indexInBuffer_ == 0)
					nextClosure(Direction::BACKWARD, false);
				else
					--indexInBuffer_;
				return *this;
			}
			/// Returns the current character in the normalized text.
			const CodePoint& dereference() const /*throw()*/ {return normalizedBuffer_[indexInBuffer_];}
			/// Returns true if both iterators address the same character in the normalized text.
			bool equal(const Normalizer& other) const /*throw()*/ {
				return /*current_->isCloneOf(*other.current_)
					&&*/ current_->offset() == other.current_->offset()
					&& indexInBuffer_ == other.indexInBuffer_;
			}
			/// Moves to the next normalized character.
			Normalizer& increment() {
				if(!hasNext())
					throw std::out_of_range("the iterator is the last.");
				else if(++indexInBuffer_ == normalizedBuffer_.length())
					nextClosure(Direction::FORWARD, false);
				return *this;
			}
		private:
			void nextClosure(Direction direction, bool initialize);
		private:
			Form form_;
			std::unique_ptr<CharacterIterator> current_;
			std::basic_string<CodePoint> normalizedBuffer_;
			std::size_t indexInBuffer_;
			std::ptrdiff_t nextOffset_;
		};

	}
} // namespace ascension.text

#endif // !ASCENSION_NO_UNICODE_NORMALIZATION
#endif // !ASCENSION_UNICODE_HPP
