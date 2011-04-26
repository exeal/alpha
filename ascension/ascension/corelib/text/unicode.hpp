/**
 * @file unicode.hpp
 * @author exeal
 * @date 2005-2011
 * @see ascension#text
 * @see character-iterator.hpp
 * @see break-iterator.cpp, collator.cpp, identifier-syntax.cpp, normalizer.cpp
 */

#ifndef ASCENSION_UNICODE_HPP
#define ASCENSION_UNICODE_HPP

#include <ascension/config.hpp>	// ASCENSION_NO_UNICODE_NORMALIZATION
#include <ascension/corelib/character-iterator.hpp>
#include <ascension/corelib/memory.hpp>		// AutoBuffer
#include <ascension/corelib/text/unicode-utf.hpp>
#include <ascension/corelib/ustring.hpp>	// ustrlen
#include <iterator>
#include <locale>
#include <set>
#include <stdexcept>

#if ASCENSION_UNICODE_VERSION > 0x0510
#	error These class definitions and implementations are based on old version of Unicode.
#endif

namespace ascension {

	/**
	 * Provides stuffs implement some of the Unicode standard. This includes:
	 * - @c Normalizer class implements <a href="http://www.unicode.org/reports/tr15/">UAX #15:
	 *   Unicode Normalization Forms</a>.
	 * - @c BreakIterator class implements <a href="http://www.unicode.org/reports/tr14/">UAX #14:
	 *   Line Breaking Properties</a> and <a href="http://www.unicode.org/reports/tr29/">UAX #29:
	 *   Text Boundary</a>.
	 * - @c IdentifierSyntax class implements <a href="http://www.unicode.org/reports/tr31/">UAX
	 *   #31: Identifier and Pattern Syntax</a>.
	 * - @c Collator class implements <a href="http://www.unicode.org/reports/tr10/">UTS #10:
	 * Unicode Collation Algorithm</a>.
	 * - @c surrogates namespace provides functions to handle UTF-16 surrogate pairs.
	 * - Unicode properties.
	 * @see ASCENSION_UNICODE_VERSION
	 */
	namespace text {

		/// Returns @c true if the specified code point is in Unicode codespace (0..10FFFF).
		inline bool isValidCodePoint(CodePoint c) /*throw()*/ {return c <= 0x10fffful;}

		/// Returns @c true if the specified code point is Unicode scalar value.
		inline bool isScalarValue(CodePoint c) /*throw()*/ {
			return isValidCodePoint(c) && !surrogates::isSurrogate(c);}

		/// Case sensitivities for caseless-match.
		enum CaseSensitivity {
			CASE_SENSITIVE,							///< Case-sensitive.
			CASE_INSENSITIVE,						///< Case-insensitive.
			CASE_INSENSITIVE_EXCLUDING_TURKISH_I	///< Case-insensitive and excludes Turkish I.
		};

		/// Types of decomposition mapping.
		enum Decomposition {
			NO_DECOMPOSITION,			///< No decomposition.
			CANONICAL_DECOMPOSITION,	///< Canonical decomposition mapping.
			FULL_DECOMPOSITION			///< Canonical and compatibility mapping.
		};

#ifndef ASCENSION_NO_UNICODE_NORMALIZATION
		class Normalizer : public detail::IteratorAdapter<Normalizer,
			std::iterator<std::bidirectional_iterator_tag, CodePoint> > {
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
			~Normalizer() /*throw()*/;
			// operator
			Normalizer&	operator=(const Normalizer& other);

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
			/// Returns the current character in the normalized text.
			const CodePoint& current() const /*throw()*/ {return normalizedBuffer_[indexInBuffer_];}
			/// Returns true if both iterators address the same character in the normalized text.
			bool equals(const Normalizer& other) const /*throw()*/ {
				return /*current_->isCloneOf(*other.current_)
					&&*/ current_->offset() == other.current_->offset()
					&& indexInBuffer_ == other.indexInBuffer_;
			}
			/// Moves to the next normalized character.
			Normalizer& next() {
				if(!hasNext())
					throw std::out_of_range("the iterator is the last.");
				else if(++indexInBuffer_ == normalizedBuffer_.length())
					nextClosure(Direction::FORWARD, false);
				return *this;
			}
			/// Moves to the previous normalized character.
			Normalizer& previous() {
				if(!hasPrevious())
					throw std::out_of_range("the iterator is the first");
				else if(indexInBuffer_ == 0)
					nextClosure(Direction::BACKWARD, false);
				else
					--indexInBuffer_;
				return *this;
			}
		private:
			void nextClosure(Direction direction, bool initialize);
		private:
			Form form_;
			std::auto_ptr<CharacterIterator> current_;
			std::basic_string<CodePoint> normalizedBuffer_;
			std::size_t indexInBuffer_;
			std::ptrdiff_t nextOffset_;
		};
#endif // !ASCENSION_NO_UNICODE_NORMALIZATION

		class CollationKey : public FastArenaObject<CollationKey> {
		public:
			CollationKey() /*throw()*/ : length_(0) {}
			CollationKey(AutoBuffer<const uint8_t> keyValues,
				std::size_t length) : keyValues_(keyValues), length_(length) {}
			CollationKey(const CollationKey& other);
			CollationKey&operator=(const CollationKey& other);
			bool operator==(const CollationKey& other) const /*throw()*/;
			bool operator!=(const CollationKey& other) const /*throw()*/;
			bool operator<(const CollationKey& other) const /*throw()*/;
			bool operator<=(const CollationKey& other) const /*throw()*/;
			bool operator>(const CollationKey& other) const /*throw()*/;
			bool operator>=(const CollationKey& other) const /*throw()*/;
		private:
			const AutoBuffer<const uint8_t> keyValues_;
			const std::size_t length_;
		};

		class CollationElementIterator {
		public:
			static const int NULL_ORDER;
		public:
			virtual ~CollationElementIterator() /*throw()*/;
			bool equals(const CollationElementIterator& other) const {
				return position() == other.position();
			}
			bool less(const CollationElementIterator &other) const {
				return position() < other.position();
			}
		public:
			virtual int current() const = 0;
			virtual void next() = 0;
			virtual void previous() = 0;
			virtual std::size_t position() const = 0;
		protected:
			CollationElementIterator() /*throw()*/;
		};

		class Collator {
		public:
			enum Strength {
				PRIMARY = 0,
				SECONDARY = 1,
				TERTIARY = 2,
				QUATERNARY = 3,
				IDENTICAL = 15
			};
			// constructor
			virtual ~Collator() /*throw()*/;
			// attributes
			Decomposition decomposition() const /*throw()*/;
			void setDecomposition(Decomposition newDecomposition);
			void setStrength(Strength newStrength);
			Strength strength() const /*throw()*/;
			// operations
			virtual std::auto_ptr<CollationKey> collationKey(const String& s) const = 0;
			int compare(const String& s1, const String& s2) const;
			virtual int compare(const CharacterIterator& s1, const CharacterIterator& s2) const = 0;
			std::auto_ptr<CollationElementIterator>
				createCollationElementIterator(const String& source) const;
			virtual std::auto_ptr<CollationElementIterator>
				createCollationElementIterator(const CharacterIterator& source) const = 0;
		protected:
			Collator() /*throw()*/ : strength_(IDENTICAL), decomposition_(NO_DECOMPOSITION) {}
		private:
			Strength strength_;
			Decomposition decomposition_;
		};

		/// @c NullCollator performs binary comparison.
		class NullCollator : public Collator {
		public:
			NullCollator() /*throw()*/;
			std::auto_ptr<CollationKey> collationKey(const String& s) const;
			int compare(const CharacterIterator& s1, const CharacterIterator& s2) const;
			std::auto_ptr<CollationElementIterator>
				createCollationElementIterator(const CharacterIterator& source) const;
		private:
			class ElementIterator : public CollationElementIterator {
			public:
				explicit ElementIterator(std::auto_ptr<CharacterIterator> source) /*throw()*/ : i_(source) {}
				~ElementIterator() /*throw()*/ {}
				int current() const {return i_->hasNext() ? i_->current() : NULL_ORDER;}
				void next() {i_->next();}
				void previous() {i_->previous();}
				std::size_t position() const {return i_->offset();}
			private:
				std::auto_ptr<CharacterIterator> i_;
			};
		};

		/**
		 * @c DefaultCollator uses DUCET (Default Unicode Collation Element Table) to collate
		 * characters and strings.
		 */
		class DefaultCollator : public Collator {};

	}
} // namespace ascension.text

#endif // !ASCENSION_UNICODE_HPP
