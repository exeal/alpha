/**
 * @file unicode.hpp
 * @author exeal
 * @date 2005-2010
 * @see ascension#text
 * @see character-iterator.hpp
 * @see break-iterator.cpp, collator.cpp, identifier-syntax.cpp, normalizer.cpp
 */

#ifndef ASCENSION_UNICODE_HPP
#define ASCENSION_UNICODE_HPP

#include <ascension/corelib/character-iterator.hpp>
#include <ascension/corelib/memory.hpp>		// AutoBuffer
#include <ascension/corelib/unicode-utf.hpp>
#include <ascension/corelib/ustring.hpp>	// ustrlen
#include <iterator>
#include <locale>
#include <set>
#include <stdexcept>

#if ASCENSION_UNICODE_VERSION > 0x0510
#error These class definitions and implementations are based on old version of Unicode.
#endif
/// Tracking revision number of UAX #14 ("Line Breaking Properties")
#define ASCENSION_UAX14_REVISION_NUMBER 19	// 2006-05-23
/// Tracking revision number of UAX #29 ("Text Boundary")
#define ASCENSION_UAX29_REVISION_NUMBER 11	// 2006-10-12

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
		class Normalizer : public StandardConstBidirectionalIteratorAdapter<Normalizer, CodePoint> {
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

		class IdentifierSyntax {
		public:
			/// Types of character classification used by @c IdentifierSyntax.
			enum CharacterClassification {
				/// Uses only 7-bit ASCII characters.
				ASCII,
				/// Classifies using @c text#ucd#legacyctype namespace functions.
				LEGACY_POSIX,
				/// Conforms to the default identifier syntax of UAX #31.
				UNICODE_DEFAULT,
				/// Conforms to the alternative identifier syntax of UAX #31.
				UNICODE_ALTERNATIVE
			};
			// constructors
			IdentifierSyntax() /*throw()*/;
			explicit IdentifierSyntax(CharacterClassification type, bool ignoreCase = false
#ifndef ASCENSION_NO_UNICODE_NORMALIZATION
				, Decomposition equivalenceType = NO_DECOMPOSITION
#endif // !ASCENSION_NO_UNICODE_NORMALIZATION
			) /*throw()*/;
			IdentifierSyntax(const IdentifierSyntax& other) /*throw()*/;
			IdentifierSyntax& operator=(const IdentifierSyntax& other) /*throw()*/;
			// singleton
			static const IdentifierSyntax& defaultInstance() /*throw()*/;
			// classification for character
			bool isIdentifierStartCharacter(CodePoint cp) const /*throw()*/;
			bool isIdentifierContinueCharacter(CodePoint cp) const /*throw()*/;
			bool isWhiteSpace(CodePoint cp, bool includeTab) const /*throw()*/;

			// classification for sequence
			/**
			 * Checks whether the specified character sequence starts with an identifier.
			 * The type @a CharacterSequence the bidirectional iterator expresses a UTF-16
			 * character sequence. This method is exception-neutral (does not throw if
			 * @a CharacterSequence does not).
			 * @tparam CharacterSequence
			 * @param first the start of the character sequence
			 * @param last the end of the character sequence
			 * @return the end of the detected identifier or @a first if an identifier not found
			 */
			template<typename CharacterSequence>
			inline CharacterSequence eatIdentifier(
					CharacterSequence first, CharacterSequence last) const {
				ASCENSION_STATIC_ASSERT(CodeUnitSizeOf<CharacterSequence>::result == 2);
				UTF16To32Iterator<CharacterSequence> i(first, last);
				if(!i.hasNext() || !isIdentifierStartCharacter(*i))
					return first;
				while(i.hasNext() && isIdentifierContinueCharacter(*i))
					++i;
				return i.tell();
			}
			/**
			 * Checks whether the specified character sequence starts with white space characters.
			 * The type @a CharacterSequence is the bidirectional iterator expresses a UTF-16
			 * character sequence. This method is exception-neutral (does not throw if
			 * @a CharacterSequence does not).
			 * @tparam CharacterSequence
			 * @param first the start of the character sequence
			 * @param last the end of the character sequence
			 * @param includeTab set true to treat a horizontal tab as a white space
			 * @return the end of the detected identifier or @a first if an identifier not found
			 */
			template<typename CharacterSequence>
			inline CharacterSequence eatWhiteSpaces(
					CharacterSequence first, CharacterSequence last, bool includeTab) const {
				ASCENSION_STATIC_ASSERT(CodeUnitSizeOf<CharacterSequence>::result == 2);
				UTF16To32Iterator<CharacterSequence> i(first, last);
				while(i.hasNext() && isWhiteSpace(*i, includeTab))
					++i;
				return i.tell();
			}

			// attributes
			void overrideIdentifierStartCharacters(
				const String& adding, const String& subtracting);
			void overrideIdentifierStartCharacters(
				const std::set<CodePoint>& adding, const std::set<CodePoint>& subtracting);
			void overrideIdentifierNonStartCharacters(
				const String& adding, const String& subtracting);
			void overrideIdentifierNonStartCharacters(
				const std::set<CodePoint>& adding, const std::set<CodePoint>& subtracting);
		private:
			CharacterClassification type_;
			bool caseSensitive_;
#ifndef ASCENSION_NO_UNICODE_NORMALIZATION
			Decomposition equivalenceType_;
#endif // !ASCENSION_NO_UNICODE_NORMALIZATION
			std::basic_string<CodePoint>
				addedIDStartCharacters_, addedIDNonStartCharacters_,
				subtractedIDStartCharacters_, subtractedIDNonStartCharacters_;
		};

		/**
		 * An abstract base class for concrete break iterator classes. Break iterators are used to
		 * find and enumerate the location of boundaries in text. These iterators are based on
		 * <a href="http://www.unicode.org/reports/tr29/">UAX #29: Text Boudaries</a>. Clients can
		 * use each concrete iterator class or abstract @c BreakIterator for their polymorphism.
		 *
		 * This class does not have an interface for standard C++ iterator.
		 */
		class BreakIterator {
			ASCENSION_UNASSIGNABLE_TAG(BreakIterator);
		public:
			/// Destructor.
			virtual ~BreakIterator() /*throw()*/ {}
			/// Returns the locale.
			const std::locale& locale() const /*throw()*/ {return locale_;}
			/// Returns true if @a at addresses a boundary.
			virtual bool isBoundary(const CharacterIterator& at) const = 0;
			/// Moves to the next boundary.
			virtual void next(std::ptrdiff_t amount) = 0;
		protected:
			BreakIterator(const std::locale& lc) /*throw()*/ : locale_(lc) {}
		private:
			const std::locale& locale_;
		};

		/// @internal
		namespace internal {
			/**
			 * Provides standard C++ iterator interface and facilities for the concrete iterator
			 * class.
			 * @tparam ConcreteIterator the concrete iterator
			 */
			template<class ConcreteIterator>
			class BreakIteratorFacade : public std::iterator<std::random_access_iterator_tag, Char> {
			public:
				reference operator*() const {return *getConcrete().tell();}
				reference operator[](difference_type index) const {return getConcrete().tell()[index];}
				ConcreteIterator& operator++() {getConcrete().next(+1); return getConcrete();}
				const ConcreteIterator operator++(int) {ConcreteIterator temp(getConcrete()); ++*this; return temp;}
				ConcreteIterator& operator--() {getConcrete().next(-1); return getConcrete();}
				const ConcreteIterator operator--(int) {ConcreteIterator temp(getConcrete()); --*this; return temp;}
				ConcreteIterator& operator+=(difference_type offset) {getConcrete().next(+offset); return getConcrete();}
				ConcreteIterator& operator-=(difference_type offset) {getConcrete().next(-offset); return getConcrete();}
				const ConcreteIterator operator+(difference_type offset) {ConcreteIterator temp(*this); return temp += offset;}
				const ConcreteIterator operator-(difference_type offset) {ConcreteIterator temp(*this); return temp -= offset;}
				bool operator==(const ConcreteIterator& other) const {return getConcrete().tell() == other.tell();}
				bool operator!=(const ConcreteIterator& other) const {return getConcrete().tell() != other.tell();}
				bool operator<(const ConcreteIterator& other) const {return getConcrete().tell() < other.tell();}
				bool operator<=(const ConcreteIterator& other) const {return getConcrete().tell() <= other.tell();}
				bool operator>(const ConcreteIterator& other) const {return getConcrete().tell() > other.tell();}
				bool operator>=(const ConcreteIterator& other) const {return getConcrete().tell() >= other.tell();}
			private:
				ConcreteIterator& getConcrete() {return *static_cast<ConcreteIterator*>(this);}
			};
		} // namespace internal

		/// Base class of @c GraphemeBreakIterator.
		class AbstractGraphemeBreakIterator : public BreakIterator {
		public:
			bool isBoundary(const CharacterIterator& at) const;
			void next(std::ptrdiff_t amount);
		protected:
			AbstractGraphemeBreakIterator(const std::locale& lc) /*throw()*/;
			virtual CharacterIterator& characterIterator() /*throw()*/ = 0;
			virtual const CharacterIterator& characterIterator() const /*throw()*/ = 0;
		private:
			void doNext(std::ptrdiff_t amount);
			void doPrevious(std::ptrdiff_t amount);
		};

		/// @c GraphemeBreakIterator locates grapheme cluster (character) boundaries in text.
		template<class BaseIterator>
		class GraphemeBreakIterator : public AbstractGraphemeBreakIterator,
			public internal::BreakIteratorFacade<GraphemeBreakIterator <BaseIterator> > {
			ASCENSION_UNASSIGNABLE_TAG(GraphemeBreakIterator);
		public:
			/**
			 * Constructor.
			 * @param base the base iterator
			 * @param lc the locale
			 */
			GraphemeBreakIterator(
				BaseIterator base, const std::locale& lc = std::locale::classic())
				: AbstractGraphemeBreakIterator(lc), p_(base) {}
			/// Returns the base iterator.
			BaseIterator& base() /*throw()*/ {return p_;}
			/// Returns the base iterator.
			const BaseIterator& base() const /*throw()*/ {return p_;}
		private:
			CharacterIterator& characterIterator() /*throw()*/ {
				return static_cast<CharacterIterator&>(p_);}
			const CharacterIterator& characterIterator() const /*throw()*/ {
				return static_cast<const CharacterIterator&>(p_);}
			BaseIterator p_;
		};

		/// Base class of @c WordBreakIterator.
		class AbstractWordBreakIterator : public BreakIterator {
		public:
			/**
			 * Components of segment to search word boundaries.
			 * These values specify which boundary the iterator scans.
			 * @see WordBreakIterator
			 */
			enum Component {
				/// Breaks at each starts of segments.
				START_OF_SEGMENT			= 0x01,
				/// Breaks at each ends of segments.
				END_OF_SEGMENT				= 0x02,
				/// Breaks at each starts and ends of segments.
				BOUNDARY_OF_SEGMENT			= START_OF_SEGMENT | END_OF_SEGMENT,
				/// Only words consist of alpha-numerics.
				ALPHA_NUMERIC				= 0x04,
				/// Start of word consists of alpha-numerics.
				START_OF_ALPHANUMERICS		= START_OF_SEGMENT | ALPHA_NUMERIC,
				/// End of word consists of alpha-numerics.
				END_OF_ALPHANUMERICS		= END_OF_SEGMENT | ALPHA_NUMERIC,
				/// Start or end of word consists of alpha-numerics.
				BOUNDARY_OF_ALPHANUMERICS	= BOUNDARY_OF_SEGMENT | ALPHA_NUMERIC
			};
			/// Returns the word component to search.
			AbstractWordBreakIterator::Component component() const /*throw()*/ {return component_;}
			bool isBoundary(const CharacterIterator& at) const;
			void next(std::ptrdiff_t amount);
			void setComponent(Component component);
		protected:
			AbstractWordBreakIterator(Component component,
				const IdentifierSyntax& syntax, const std::locale& lc) /*throw()*/;
			virtual CharacterIterator& characterIterator() /*throw()*/ = 0;
			virtual const CharacterIterator& characterIterator() const /*throw()*/ = 0;
		private:
			void doNext(std::ptrdiff_t amount);
			void doPrevious(std::ptrdiff_t amount);
			Component component_;
			const IdentifierSyntax& syntax_;
		};

		/// @c WordBreakIterator locates word boundaries in text.
		template<class BaseIterator>
		class WordBreakIterator : public AbstractWordBreakIterator,
			public internal::BreakIteratorFacade<WordBreakIterator<BaseIterator> > {
			ASCENSION_UNASSIGNABLE_TAG(WordBreakIterator);
		public:
			/**
			 * Constructor.
			 * @param base the base iterator
			 * @param component the component of word to search
			 * @param syntax the identifier syntax for detecting identifier characters
			 * @param lc the locale
			 */
			WordBreakIterator(BaseIterator base, Component component,
				const IdentifierSyntax& syntax, const std::locale& lc = std::locale::classic())
				: AbstractWordBreakIterator(component, syntax, lc), p_(base) {}
			/// Returns the base iterator.
			BaseIterator& base() /*throw()*/ {return p_;}
			/// Returns the base iterator.
			const BaseIterator& base() const /*throw()*/ {return p_;}
		private:
			CharacterIterator& characterIterator() /*throw()*/ {
				return static_cast<CharacterIterator&>(p_);}
			const CharacterIterator& characterIterator() const /*throw()*/ {
				return static_cast<const CharacterIterator&>(p_);}
			BaseIterator p_;
		};

		/// Base class of @c SentenceBreakIterator.
		class AbstractSentenceBreakIterator : public BreakIterator {
		public:
			/**
			 * Components of segment to search word boundaries.
			 * These values specify which boundary the iterator scans.
			 * @see WordBreakIterator
			 */
			enum Component {
				/// Breaks at each starts of segments.
				START_OF_SEGMENT	= 0x01,
				/// Breaks at each ends of segments.
				END_OF_SEGMENT		= 0x02,
				/// Breaks at each starts and ends of segments.
				BOUNDARY_OF_SEGMENT	= START_OF_SEGMENT | END_OF_SEGMENT,
			};
			/// Returns the sentence component to search.
			AbstractSentenceBreakIterator::Component component() const /*throw()*/ {return component_;}
			bool isBoundary(const CharacterIterator& at) const;
			void next(std::ptrdiff_t amount);
			void setComponent(Component component);
		protected:
			AbstractSentenceBreakIterator(Component component,
				const IdentifierSyntax& syntax, const std::locale& lc) /*throw()*/;
			virtual CharacterIterator& characterIterator() /*throw()*/ = 0;
			virtual const CharacterIterator& characterIterator() const /*throw()*/ = 0;
		private:
			void doNext(std::ptrdiff_t amount);
			void doPrevious(std::ptrdiff_t amount);
			Component component_;
			const IdentifierSyntax& syntax_;
		};

		/// @c SentenceBreakIterator locates sentence boundaries in text.
		template<class BaseIterator>
		class SentenceBreakIterator : public AbstractSentenceBreakIterator,
			public internal::BreakIteratorFacade<SentenceBreakIterator<BaseIterator> > {
			ASCENSION_UNASSIGNABLE_TAG(SentenceBreakIterator);
		public:
			/**
			 * Constructor.
			 * @param base the base iterator
			 * @param component the component of sentence to search
			 * @param syntax the identifier syntax to detect alphabets
			 * @param lc the locale
			 */
			SentenceBreakIterator(BaseIterator base, Component component,
				const IdentifierSyntax& syntax, const std::locale& lc = std::locale::classic())
				: AbstractSentenceBreakIterator(component, syntax, lc), p_(base) {}
			/// Returns the base iterator.
			BaseIterator& base() /*throw()*/ {return p_;}
			/// Returns the base iterator.
			const BaseIterator& base() const /*throw()*/ {return p_;}
		private:
			CharacterIterator& characterIterator() /*throw()*/ {
				return static_cast<CharacterIterator&>(p_);}
			const CharacterIterator& characterIterator() const /*throw()*/ {
				return static_cast<const CharacterIterator&>(p_);}
			BaseIterator p_;
		};

		/// Base class of @c LineBreakIterator.
		class AbstractLineBreakIterator : public BreakIterator {
		public:
			bool isBoundary(const CharacterIterator& at) const;
			void next(std::ptrdiff_t amount);
		protected:
			AbstractLineBreakIterator(const std::locale& lc) /*throw()*/;
			virtual CharacterIterator& characterIterator() /*throw()*/ = 0;
			virtual const CharacterIterator& characterIterator() const /*throw()*/ = 0;
		};

		/// @c LineBreakIterator locates line break opportunities in text.
		template<class BaseIterator>
		class LineBreakIterator : public AbstractLineBreakIterator,
			public internal::BreakIteratorFacade<LineBreakIterator<BaseIterator> > {
			ASCENSION_UNASSIGNABLE_TAG(LineBreakIterator);
		public:
			/**
			 * Constructor.
			 * @param base the base iterator
			 * @param lc the locale
			 */
			LineBreakIterator(BaseIterator base,
				const std::locale& lc = std::locale::classic())
				: AbstractLineBreakIterator(lc), p_(base) {}
			/// Returns the base iterator.
			BaseIterator& base() /*throw()*/ {return p_;}
			/// Returns the base iterator.
			const BaseIterator& base() const /*throw()*/ {return p_;}
		private:
			CharacterIterator& characterIterator() /*throw()*/ {
				return static_cast<CharacterIterator&>(p_);}
			const CharacterIterator& characterIterator() const /*throw()*/ {
				return static_cast<const CharacterIterator&>(p_);}
			BaseIterator p_;
		};

		/**
		 * @c CaseFolder folds cases of characters and strings. This behavior is based on Default
		 * Case Algorithm of Unicode, and locale-independent and context-insensitive.
		 * @see Collator, Normalizer, searcher#LiteralPattern
		 */
		class CaseFolder {
			ASCENSION_NONCOPYABLE_TAG(CaseFolder);
		public:
			static const length_t MAXIMUM_EXPANSION_CHARACTERS;
			static int compare(const CharacterIterator& s1,
				const CharacterIterator& s2, bool excludeTurkishI = false);
			/**
			 * Compares the two character sequences case-insensitively.
			 * @param s1 the character sequence
			 * @param s2 the the other
			 * @param excludeTurkishI set true to perform "Turkish I mapping"
			 * @retval &lt;0 the first character sequence is less than the second
			 * @retval 0 the both sequences are same
			 * @retval &gt;0 the first character sequence is greater than the second
			 */
			static int compare(const String& s1, const String& s2, bool excludeTurkishI = false) {
				return compare(StringCharacterIterator(s1),
					StringCharacterIterator(s2), excludeTurkishI);
			}
			static CodePoint fold(CodePoint c, bool excludeTurkishI = false) /*throw()*/;
			template<typename CharacterSequence> static String fold(
				CharacterSequence first, CharacterSequence last, bool excludeTurkishI = false);
			/**
			 * Folds case of the specified character sequence. This method performs "full case folding."
			 * @param text the character sequence
			 * @param excludeTurkishI set true to perform "Turkish I mapping"
			 * @return the folded string
			 */
			static String fold(const String& text, bool excludeTurkishI = false) {
				return fold(text.data(), text.data() + text.length(), excludeTurkishI);
			}
		private:
			static CodePoint foldCommon(CodePoint c) /*throw()*/ {
				const CodePoint* const p = std::lower_bound(
					COMMON_CASED_, COMMON_CASED_ + NUMBER_OF_COMMON_CASED_, c);
				return (*p == c) ? COMMON_FOLDED_[p - COMMON_CASED_] : c;}
			static std::size_t foldFull(CodePoint c, bool excludeTurkishI, CodePoint* dest) /*throw()*/;
			static CodePoint foldTurkishI(CodePoint c) /*throw()*/ {
				if(c == 0x0049u) c = 0x0131u; else if(c == 0x0130u) c = 0x0069u; return c;}
			static const CodePoint COMMON_CASED_[],
				COMMON_FOLDED_[], SIMPLE_CASED_[], SIMPLE_FOLDED_[], FULL_CASED_[];
			static const Char* FULL_FOLDED_[];
			static const std::size_t
				NUMBER_OF_COMMON_CASED_, NUMBER_OF_SIMPLE_CASED_, NUMBER_OF_FULL_CASED_;
		};

		class CollationKey : public FastArenaObject<CollationKey> {
		public:
			CollationKey() /*throw()*/ : length_(0) {}
			CollationKey(AutoBuffer<const uchar> keyValues,
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
			const AutoBuffer<const uchar> keyValues_;
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


		// inline implementations /////////////////////////////////////////////////////////////////

		/**
		 * Folds the case of the specified character. This method performs "simple case folding."
		 * @param c the code point of the character to fold
		 * @param excludeTurkishI set true to perform "Turkish I mapping"
		 * @return the case-folded character
		 */
		inline CodePoint CaseFolder::fold(CodePoint c, bool excludeTurkishI /* = false */) /*throw()*/ {
			CodePoint result;
			// Turkish I
			if(excludeTurkishI && c != (result = foldTurkishI(c)))
				return result;
			// common mapping
			if(c != (result = foldCommon(c)))
				return result;
			// simple mapping
			const CodePoint* const p = std::lower_bound(SIMPLE_CASED_, SIMPLE_CASED_ + NUMBER_OF_SIMPLE_CASED_, c);
			return (*p == c) ? SIMPLE_FOLDED_[p - SIMPLE_CASED_] : c;
		}

		/**
		 * Folds case of the specified character sequence. This method performs "full case folding."
		 * @a CharacterSequence must represents a UTF-16 character sequence.
		 * @tparam CharacterSequence
		 * @param first the start of the character sequence
		 * @param last the end of the character sequence
		 * @param excludeTurkishI set true to perform "Turkish I mapping"
		 * @return the folded string
		 */
		template<typename CharacterSequence>
		inline String CaseFolder::fold(CharacterSequence first,
				CharacterSequence last, bool excludeTurkishI /* = false */) {
			ASCENSION_STATIC_ASSERT(CodeUnitSizeOf<CharacterSequence>::result == 2);
			using namespace std;
			std::basic_stringbuf<Char> s(ios_base::out);
			CodePoint c, f;
			Char buffer[2];
			for(UTF16To32Iterator<CharacterSequence> i(first, last); i.hasNext(); ++i) {
				c = *i;
				if(!excludeTurkishI || c == (f = foldTurkishI(*i)))
					f = foldCommon(c);
				if(f != c || c >= 0x010000u) {
					if(surrogates::encode(f, buffer) < 2)
						s.sputc(buffer[0]);
					else
						s.sputn(buffer, 2);
				} else {
					const CodePoint* const p = lower_bound(
						FULL_CASED_, FULL_CASED_ + NUMBER_OF_FULL_CASED_, c);
					if(*p == c)
						s.sputn(FULL_FOLDED_[p - FULL_CASED_],
							static_cast<std::streamsize>(ustrlen(FULL_FOLDED_[p - FULL_CASED_])));
					else
						s.sputc(static_cast<Char>(c & 0xffffu));
				}
			}
			return s.str();
		}

}} // namespace ascension.text

#endif // !ASCENSION_UNICODE_HPP
