/**
 * @file unicode.hpp
 * @author exeal
 * @date 2005-2007
 * @see ascension#unicode, break-iterator.cpp, collator.cpp, identifier-syntax.cpp, normalizer.cpp
 */

#ifndef ASCENSION_UNICODE_HPP
#define ASCENSION_UNICODE_HPP
#include "internal.hpp"
#include "../../manah/object.hpp"	// manah.Noncopyable
#include "../../manah/memory.hpp"	// manah.AutoBuffer
#include <cassert>
#include <stdexcept>
#include <iterator>
#include <set>
#include <bitset>
#include <map>
#include <locale>

#if ASCENSION_UNICODE_VERSION > 0x0500
#error These class definitions and implementations are based on old version of Unicode.
#endif
/// Tracking revision number of UAX #14 ("Line Breaking Properties")
#define ASCENSION_UAX14_REVISION_NUMBER	19	// 2006-05-23
/// Tracking revision number of UAX #29 ("Text Boundary")
#define ASCENSION_UAX29_REVISION_NUMBER	11	// 2006-10-12
#define CASE_FOLDING_EXPANSION_MAX_CHARS 3

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
	namespace unicode {

		/**
		 * @c surrogates namespace collects low level procedures handle UTF-16 surrogate pair.
		 * @see UTF16To32Iterator, UTF32To16Iterator
		 */
		namespace surrogates {
			/**
			 * Returns if the specified code unit is high (leading)-surrogate.
			 * @param cp the code point
			 * @return true if @a cp is high-surrogate
			 */
			inline bool isHighSurrogate(CodePoint cp) throw() {return (cp & 0xFFFFFC00U) == 0xD800U;}
			/**
			 * Returns if the specified code unit is low (trailing)-surrogate.
			 * @param cp the code point
			 * @return true if @a cp is low-surrogate
			 */
			inline bool isLowSurrogate(CodePoint cp) throw() {return (cp & 0xFFFFFC00U) == 0xDC00U;}
			/**
			 * Returns if the specified code unit is surrogate.
			 * @param cp the code point
			 * @return true if @a cp is surrogate
			 */
			inline bool isSurrogate(CodePoint cp) throw() {return (cp & 0xFFFFF800U) == 0xD800U;}
			/**
			 * Returns high (leading)-surrogate for the specified code point.
			 * @note If @a cp is in BMP, the behavior is undefined.
			 * @param cp the code point
			 * @return the high-surrogate code unit for @a cp
			 */
			inline Char getHighSurrogate(CodePoint cp) throw() {return static_cast<Char>((cp >> 10) & 0xFFFF) + 0xD7C0U;}
			/**
			 * Returns low (trailing)-surrogate for the specified code point.
			 * @note If @a cp is in BMP, the behavior is undefined.
			 * @param cp the code point
			 * @return the low-surrogate code unit for @a cp
			 */
			inline Char getLowSurrogate(CodePoint cp) throw() {return static_cast<Char>(cp & 0x03FFU) | 0xDC00U;}
			/**
			 * Converts the specified surrogate pair to a corresponding code point.
			 * @param high the high-surrogate
			 * @param low the low-surrogate
			 * @return the code point or the value of @a high if the pair is not valid
			 */
			inline CodePoint decode(Char high, Char low) throw() {
				return (isHighSurrogate(high) && isLowSurrogate(low)) ? 0x10000U + (high - 0xD800U) * 0x0400U + low - 0xDC00U : high;}
			/**
			 * Converts the first surrogate pair in the given character sequence to the corresponding code point.
			 * @param first the start of the UTF-16 character sequence
			 * @param last the end of the UTF-16 character sequence
			 * @return the code point
			 */
			template<typename CharacterSequence>
			inline CodePoint decodeFirst(CharacterSequence first, CharacterSequence last) throw() {
				assert(first < last); return (last - first > 1) ? decode(first[0], first[1]) : first[0];}
			/**
			 * Converts the last surrogate pair in the given character sequence to the corresponding code point.
			 * @param first the start of the UTF-16 character sequence
			 * @param last the end of the UTF-16 character sequence
			 * @return the code point
			 */
			template<typename CharacterSequence>
			inline CodePoint decodeLast(CharacterSequence first, CharacterSequence last) throw() {assert(first < last);
				return (last - first > 1 && isHighSurrogate(last[-2]) && isLowSurrogate(last[-1])) ? decode(last[-2], last[-1]) : last[-1];}
			/**
			 * Converts the specified code point to a corresponding surrogate pair.
			 * @param cp the code point
			 * @param[out] dest the surrogate pair
			 * @retval 0 @a cp is a surrogate. in this case, @a dest[0] will be @a cp
			 * @retval 1 @a cp is in BMP
			 * @retval 2 @a cp is out of BMP
			 * @throw std#invalid_argument @a cp can't be expressed by UTF-16
			 */
			template<typename OutputIterator>
			inline length_t encode(CodePoint cp, OutputIterator dest) {
				if(cp < 0x00010000U) {
					*dest = static_cast<Char>(cp & 0xFFFFU);
					return !isSurrogate(cp) ? 1 : 0;
				} else if(cp <= 0x0010FFFFU) {
					*dest = getHighSurrogate(cp);
					*++dest = getLowSurrogate(cp);
					return 2;
				}
				throw std::invalid_argument("the specified code point is not valid.");
			}
			/**
			 * Searches the next high-surrogate in the string.
			 * @param start the start position to search
			 * @param last the end of the string
			 * @return the next high-surrogate
			 */
			template<typename CharacterSequence>
			inline CharacterSequence next(CharacterSequence start, CharacterSequence last) throw() {
				assert(start < last); return start + ((isHighSurrogate(start[0]) && (last - start > 1) && isLowSurrogate(start[1])) ? 2 : 1);}
			/**
			 * Searches the previous high-surrogate in the string.
			 * @param first the start of the string
			 * @param start the start position to search
			 * @return the previous high-surrogate
			 */
			template<typename CharacterSequence>
			inline CharacterSequence previous(CharacterSequence first, CharacterSequence start) throw() {
				assert(first < start); return start - ((isLowSurrogate(start[-1]) && (start - first > 1) && isHighSurrogate(start[-2])) ? 2 : 1);}
			/**
			 * Searches an isolated surrogate character in the specified UTF-16 string.
			 * About UTF-32 strings, use <code>std#find_if(,, std::ptr_fun(isSurrogate))</code> instead.
			 * @a CharacterSequence must represent random-accessible 16-bit character sequence.
			 * @param first the start of the string
			 * @param last the end of the string
			 * @return the isolated surrogate or @a last if not found
			 */
			template<typename CharacterSequence>
			inline CharacterSequence searchIsolatedSurrogate(CharacterSequence first, CharacterSequence last) throw() {
				assert(first < last);
				while(first < last) {
					if(isLowSurrogate(*first)) break;
					else if(isHighSurrogate(*first)) {
						if(last - first > 1 && isLowSurrogate(first[1])) ++first;
						else break;
					}
					++first;
				}
				return first;
			}
		} // namespace surrogates

		/// Returns true if the specified code point is in Unicode codespace (0..10FFFF).
		inline bool isValidCodePoint(CodePoint cp) throw() {return cp <= 0x10FFFFU;}

		/// Returns true if the specified code point is Unicode scalar value.
		inline bool isScalarValue(CodePoint cp) throw() {return isValidCodePoint(cp) && !surrogates::isSurrogate(cp);}

		class CharacterIterator : public std::iterator<std::bidirectional_iterator_tag, CodePoint> {
		public:
			enum {hasBoundary = true};
			/// Indicates the iterator is the last.
			static const CodePoint DONE = 0xFFFFFFFFUL;

		public:
			// standard C++ iterator interface
			virtual ~CharacterIterator() throw();
			CharacterIterator&	operator=(const CharacterIterator& rhs);
			CodePoint			operator*() const throw();
			CharacterIterator&	operator++();
			CharacterIterator&	operator--();
			bool				operator==(const CharacterIterator& rhs) const;
			bool				operator!=(const CharacterIterator& rhs) const;
			bool				operator<(const CharacterIterator& rhs) const;
			bool				operator<=(const CharacterIterator& rhs) const;
			bool				operator>(const CharacterIterator& rhs) const;
			bool				operator>=(const CharacterIterator& rhs) const;
			// attributes
			std::ptrdiff_t	getOffset() const throw();
			bool			isCloneOf(const CharacterIterator& other) const throw();
			// movement
			CharacterIterator&	first();
			CharacterIterator&	last();

			// public pure virtual methods the concrete class should implement
			/// Creates a copy of the iterator. This must be implemented by copy-constructor of @c CharacterIterator.
			virtual std::auto_ptr<CharacterIterator> clone() const = 0;
			/// Returns true if the iterator is not last.
			virtual bool hasNext() const = 0;
			/// Returns true if the iterator is not first.
			virtual bool hasPrevious() const = 0;

			// protected pure virtual methods the concrete class should implement
		protected:
			/// Assigns the other iterator.
			virtual void assign(const CharacterIterator& rhs) = 0;
			/// Returns the current code point value.
			virtual CodePoint current() const throw() = 0;
			/// Moves to the start of the character sequence.
			virtual void doFirst() = 0;
			/// Moves to the end of the character sequence.
			virtual void doLast() = 0;
			/// Returns true if the iterator equals @a rhs.
			virtual bool equals(const CharacterIterator& rhs) const = 0;
			/// Returns true if the iterator is less than @a rhs.
			virtual bool less(const CharacterIterator& rhs) const = 0;
			/// Moves to the previous code unit.
			virtual void next() = 0;
			/// Moves to the next code unit.
			virtual void previous() = 0;

		protected:
			CharacterIterator() throw();
			CharacterIterator(const CharacterIterator& rhs) throw();
		private:
			void verifyRHS(const CharacterIterator& rhs) const {
				if(!isCloneOf(rhs)) throw std::invalid_argument("the right is not a clone of this.");}
			std::ptrdiff_t offset_;
			const CharacterIterator* original_;
		};

		/// Implementation of @c CharacterIterator for C string or @c String.
		class StringCharacterIterator : public CharacterIterator {
		public:
			StringCharacterIterator() throw();
			StringCharacterIterator(const Char* first, const Char* last);
			StringCharacterIterator(const Char* first, const Char* last, const Char* start);
			StringCharacterIterator(const String& s);
			StringCharacterIterator(const String& s, String::const_iterator start);
			StringCharacterIterator(const StringCharacterIterator& rhs) throw();
			const Char*	getFirst() const throw();
			const Char*	getLast() const throw();
			bool		hasNext() const;
			bool		hasPrevious() const;
			const Char*	tell() const throw();
		private:
			void 								assign(const CharacterIterator& rhs);
			std::auto_ptr<CharacterIterator>	clone() const;
			CodePoint							current() const throw();
			void 								doFirst();
			void 								doLast();
			bool 								equals(const CharacterIterator& rhs) const;
			bool 								less(const CharacterIterator& rhs) const;
			void 								next();
			void 								previous();
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
		 * @param BaseIterator the base bidirectional iterator presents UTF-16 character sequence
		 * @param ConcreteIterator set to @c UTF16To32Iterator template class
		 * @see UTF16To32Iterator, UTF16To32IteratorUnsafe, UTF32To16Iterator, ToUTF32Sequence
		 */
		template<typename BaseIterator, typename ConcreteIterator>
		class UTF16To32IteratorBase : public BidirectionalIteratorFacade<ConcreteIterator,
				CodePoint, typename std::iterator_traits<BaseIterator>::difference_type, const CodePoint*, const CodePoint> {
		protected:
			/// Default constructor.
			UTF16To32IteratorBase() {}
			/// Copy-constructor.
			UTF16To32IteratorBase(const UTF16To32IteratorBase& rhs) : p_(rhs.p_) {}
			/// Constructor takes a position to start iteration.
			UTF16To32IteratorBase(BaseIterator start) : p_(start) {}
		public:
			/// Assignment operator.
			ConcreteIterator& operator=(const UTF16To32IteratorBase& rhs) {p_ = rhs.p_; return getConcrete();}
			/// Implements decrement operators.
			void decrement() {if(!getConcrete().hasPrevious()) throw std::logic_error("The iterator is first.");
				--p_; if(getConcrete().hasPrevious() && surrogates::isLowSurrogate(*p_)) --p_;}
			/// Implements dereference operator.
			CodePoint dereference() const {
				if(!getConcrete().hasNext()) throw std::logic_error("The iterator is last.");
				if(!surrogates::isHighSurrogate(*p_)) return *p_;
				++const_cast<UTF16To32IteratorBase*>(this)->p_;
				const CodePoint next = getConcrete().hasNext() ? *p_ : INVALID_CODE_POINT;
				--const_cast<UTF16To32IteratorBase*>(this)->p_;
				return (next != INVALID_CODE_POINT) ? surrogates::decode(*p_, static_cast<Char>(next & 0xFFFF)) : *p_;}
			/// Implements equality operator.
			bool equals(const ConcreteIterator& rhs) const {return p_ == rhs.p_;}
			/// Implements increment opearators.
			void increment() {if(!getConcrete().hasNext()) throw std::logic_error("The iterator is last.");
				++p_; if(getConcrete().hasNext() && surrogates::isLowSurrogate(*p_)) ++p_;}
			/// Implements relational operators.
			bool isLessThan(const ConcreteIterator& rhs) const {return p_ < rhs.p_;}
			/// Returns the current position.
			BaseIterator tell() const {return p_;}
		private:
			ConcreteIterator& getConcrete() throw() {return *static_cast<ConcreteIterator*>(this);}
			const ConcreteIterator& getConcrete() const throw() {return *static_cast<const ConcreteIterator*>(this);}
			BaseIterator p_;
		};

		/// Concrete class derived from @c UTF16To32IteratorBase does not check boundary at all.
		template<typename BaseIterator = const Char*>
		class UTF16To32IteratorUnsafe : public UTF16To32IteratorBase<BaseIterator, UTF16To32IteratorUnsafe<BaseIterator> > {
		public:
			/// Default constructor.
			UTF16To32IteratorUnsafe() {}
			/// Constructor takes a position to start iteration. The ownership of the target text
			/// will not be transferred to this.
			UTF16To32IteratorUnsafe(BaseIterator i) : UTF16To32IteratorBase<BaseIterator, UTF16To32IteratorUnsafe<BaseIterator> >(i) {}
			/// Returns true.
			bool hasNext() const throw() {return true;}
			/// Returns true.
			bool hasPrevious() const throw() {return true;}
		};

		/// Concrete class derived from @c UTF16To32IteratorBase.
		template<typename BaseIterator = const Char*>
		class UTF16To32Iterator : public UTF16To32IteratorBase<BaseIterator, UTF16To32Iterator<BaseIterator> > {
		private:
			typedef UTF16To32IteratorBase<BaseIterator, UTF16To32Iterator<BaseIterator> > Base;
		public:
			/// Default constructor.
			UTF16To32Iterator() {}
			/// Constructor takes a position to start iteration. The ownership of the target text
			/// will not be transferred to this.
			UTF16To32Iterator(BaseIterator first, BaseIterator last) : Base(first), first_(first), last_(last) {}
			/// Constructor takes a position to start iteration. The ownership of the target text
			/// will not be transferred to this.
			UTF16To32Iterator(BaseIterator first, BaseIterator last, BaseIterator start) : Base(start), first_(first), last_(last) {}
			/// Constructor takes a C++ standard container.
			template<typename Container>
			UTF16To32Iterator(const Container& c) : Base(c.begin()), first_(c.begin()), last_(c.end()) {}
			/// Constructor takes a C++ standard container.
			template<typename Container>
			UTF16To32Iterator(const Container& c, BaseIterator start) : Base(start), first_(c.begin()), last_(c.end()) {}
			/// Copy constructor.
			UTF16To32Iterator(const UTF16To32Iterator& rhs) : Base(rhs), first_(rhs.first_), last_(rhs.last_) {}
			/// Assignment operator.
			UTF16To32Iterator& operator=(const UTF16To32Iterator& rhs) {Base::operator=(rhs); first_ = rhs.first_; last_ = rhs.last_; return *this;}
			/// Returns true if the iterator is not at the last.
			bool hasNext() const {return Base::tell() != last_;}
			/// Returns true if the iterator is not at the first.
			bool hasPrevious() const {return Base::tell() != first_;}
		private:
			BaseIterator first_, last_;
		};

		/// Returns the size of a code unit of the specified code unit sequence in bytes.
		template<typename CodeUnitSequence> struct CodeUnitSizeOf {
			enum {result = sizeof(typename std::iterator_traits<CodeUnitSequence>::value_type)};
		};

		/// Converts the code unit sequence into UTF-32. This does not accept UTF-8.
		template<typename CodeUnitSequence, template<class> class AdaptionIterator = UTF16To32Iterator>
		struct ToUTF32Sequence {
			typedef ascension::internal::Select<
				CodeUnitSizeOf<CodeUnitSequence>::result == 4,
				CodeUnitSequence, AdaptionIterator<CodeUnitSequence> > Result;
		};

		/**
		 * Bidirectional iterator scans UTF-32 character sequence as UTF-16.
		 *
		 * UTF-32 sequence scanned by this is given by the template parameter @a BaseIterator.
		 *
		 * This supports four relation operators general bidirectional iterators don't have.
		 * These are available if @a BaseIterator have these facilities.
		 * @param BaseIterator the base bidirectional iterator presents UTF-32 character sequence
		 * @see UTF16To32Iterator
		 */
		template<class BaseIterator = const CodePoint*>
		class UTF32To16Iterator : public BidirectionalIteratorFacade<UTF32To16Iterator<BaseIterator>,
				Char, typename std::iterator_traits<BaseIterator>::difference_type, const Char*, const Char> {
		public:
			/// Default constructor.
			UTF32To16Iterator() {}
			/// Constructor takes a position to start iteration.
			/// The ownership of the target text will not be transferred to this.
			UTF32To16Iterator(BaseIterator start) : p_(start), high_(true) {}
			/// Assignment operator.
			UTF32To16Iterator& operator=(const UTF32To16Iterator& rhs) {p_ = rhs.p_; high_ = rhs.high_;}
		  	/// Implements decrement operators.
			void decrement() {if(!high_) high_ = true; else {--p_; high_ = *p_ < 0x10000U;}}
			/// Implements dereference operator.
			Char dereference() const {if(*p_ < 0x10000U) return static_cast<Char>(*p_ & 0xFFFFU);
				else {Char text[2]; surrogates::encode(*p_, text); return text[high_ ? 0 : 1];}}
			/// Implements equality operator.
			bool equals(const UTF32To16Iterator& rhs) const {return p_ == rhs.p_ && high_ == rhs.high_;}
			/// Implements increment operators.
			void increment() {if(!high_) {high_ = true; ++p_;} else if(*p_ < 0x10000U) ++p_; else high_ = false;}
			/// Implements relational operatora.
			bool isLessThan(const UTF32To16Iterator<BaseIterator>& rhs) const {return p_ < rhs.p_ || (p_ == rhs.p_ && high_ && !rhs.high_);}
			/// Returns the current position.
			BaseIterator tell() const {return p_;}
			enum {hasBoundary = false};
		private:
			BaseIterator p_;
			bool high_;
		};

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
		class Normalizer : public BidirectionalIteratorFacade<Normalizer, const CodePoint> {
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
			Normalizer(const Normalizer& rhs);
			~Normalizer() throw();
			// operator
			Normalizer&	operator=(const Normalizer& rhs);
			// attributes
			std::ptrdiff_t	getOffset() const throw();
			bool			hasNext() const throw();
			bool			hasPrevious() const throw();
			// class operations
			static int		compare(const String& s1, const String& s2, CaseSensitivity caseSensitivity);
			static Form		formForName(const Char* name);
			static String	normalize(CodePoint c, Form form);
			static String	normalize(const CharacterIterator& text, Form form);
			// BidirectionalIteratorFacade
			reference	dereference() const throw();
			void		increment();
			void		decrement();
			bool		equals(const Normalizer& rhs) const throw();
		private:
			void	nextClosure(Direction direction, bool initialize);
		private:
			Form form_;
			std::auto_ptr<CharacterIterator> current_;
			std::basic_string<CodePoint> normalizedBuffer_;
			std::size_t indexInBuffer_;
			std::ptrdiff_t nextOffset_;
		};
#endif /* !ASCENSION_NO_UNICODE_NORMALIZATION */

		class IdentifierSyntax {
		public:
			/// Types of character classification used by @c IdentifierSyntax.
			enum CharacterClassification {
				ASCII,				///< Uses only 7-bit ASCII characters.
				LEGACY_POSIX,		///< Classifies using @c unicode#legacyctype namespace functions.
				UNICODE_DEFAULT,	///< Conforms to the default identifier syntax of UAX #31.
				UNICODE_ALTERNATIVE	///< Conforms to the alternative identifier syntax of UAX #31.
			};
			// constructors
			IdentifierSyntax() throw();
			explicit IdentifierSyntax(CharacterClassification type, bool ignoreCase = false
#ifndef ASCENSION_NO_UNICODE_NORMALIZATION
				, Decomposition equivalenceType = NO_DECOMPOSITION
#endif /* !ASCENSION_NO_UNICODE_NORMALIZATION */
			) throw();
			IdentifierSyntax(const IdentifierSyntax& rhs) throw();
			IdentifierSyntax&	operator=(const IdentifierSyntax& rhs) throw();
			// singleton
			static const IdentifierSyntax&	getDefaultInstance() throw();
			// classification for character
			bool	isIdentifierStartCharacter(CodePoint cp) const throw();
			bool	isIdentifierContinueCharacter(CodePoint cp) const throw();
			bool	isWhiteSpace(CodePoint cp, bool includeTab) const throw();
			// classification for sequence
			template<typename CharacterSequence>
			CharacterSequence	eatIdentifier(CharacterSequence first, CharacterSequence last) const throw();
			template<typename CharacterSequence>
			CharacterSequence	eatWhiteSpaces(CharacterSequence, CharacterSequence last, bool includeTab) const throw();
			// attributes
			void	overrideIdentifierStartCharacters(const String& adding, const String& subtracting);
			void	overrideIdentifierStartCharacters(const std::set<CodePoint>& adding, const std::set<CodePoint>& subtracting);
			void	overrideIdentifierNonStartCharacters(const String& adding, const String& subtracting);
			void	overrideIdentifierNonStartCharacters(const std::set<CodePoint>& adding, const std::set<CodePoint>& subtracting);
		private:
			CharacterClassification type_;
			bool caseSensitive_;
#ifndef ASCENSION_NO_UNICODE_NORMALIZATION
			Decomposition equivalenceType_;
#endif /* !ASCENSION_NO_UNICODE_NORMALIZATION */
			std::basic_string<CodePoint> addedIDStartCharacters_, addedIDNonStartCharacters_;
			std::basic_string<CodePoint> subtractedIDStartCharacters_, subtractedIDNonStartCharacters_;
		};

		/**
		 * An abstract base class for concrete break iterator classes. Break iterators are used to
		 * find and enumerate the location of boundaries in text. These iterators are based on
		 * <a href="http://www.unicode.org/reports/tr29/">UAX #29: Text Boudaries</a>. Clients can
		 * use each concrete iterator class or abstract @c BreakIterator for their polymorphism.
		 *
		 * This class does not have an interface for standard C++ iterator.
		 */
		class BreakIterator : private manah::Unassignable {
		public:
			/// Destructor.
			virtual ~BreakIterator() throw() {}
			/// Returns the locale.
			const std::locale& getLocale() const throw() {return locale_;}
			/// Returns true if @a at addresses a boundary.
			virtual bool isBoundary(const CharacterIterator& at) const = 0;
			/// Moves to the next boundary.
			virtual void next(std::ptrdiff_t amount) = 0;
		protected:
			BreakIterator(const std::locale& lc) throw() : locale_(lc) {}
		private:
			const std::locale& locale_;
		};

		/// @internal
		namespace internal {
			/**
			 * Provides standard C++ iterator interface and facilities for the concrete iterator class.
			 * @param ConcreteIterator the concrete iterator
			 */
			template<class ConcreteIterator> class BreakIteratorFacade : public std::iterator<std::random_access_iterator_tag, Char> {
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
				bool operator==(const ConcreteIterator& rhs) const {return getConcrete().tell() == rhs.tell();}
				bool operator!=(const ConcreteIterator& rhs) const {return getConcrete().tell() != rhs.tell();}
				bool operator<(const ConcreteIterator& rhs) const {return getConcrete().tell() < rhs.tell();}
				bool operator<=(const ConcreteIterator& rhs) const {return getConcrete().tell() <= rhs.tell();}
				bool operator>(const ConcreteIterator& rhs) const {return getConcrete().tell() > rhs.tell();}
				bool operator>=(const ConcreteIterator& rhs) const {return getConcrete().tell() >= rhs.tell();}
			private:
				ConcreteIterator& getConcrete() {return *static_cast<ConcreteIterator*>(this);}
			};
		} // namespace internal

		/// Base class of @c GraphemeBreakIterator.
		class AbstractGraphemeBreakIterator : public BreakIterator {
		public:
			bool	isBoundary(const CharacterIterator& at) const;
			void	next(std::ptrdiff_t amount);
		protected:
			AbstractGraphemeBreakIterator(const std::locale& lc) throw();
			virtual CharacterIterator& getCharacterIterator() throw() = 0;
			virtual const CharacterIterator& getCharacterIterator() const throw() = 0;
		private:
			void	doNext(std::ptrdiff_t amount);
			void	doPrevious(std::ptrdiff_t amount);
		};

		/// @c GraphemeBreakIterator locates grapheme cluster (character) boundaries in text.
		template<class BaseIterator> class GraphemeBreakIterator :
			public AbstractGraphemeBreakIterator, public internal::BreakIteratorFacade<GraphemeBreakIterator <BaseIterator> > {
		public:
			/**
			 * Constructor.
			 * @param base the base iterator
			 * @param lc the locale
			 */
			GraphemeBreakIterator(BaseIterator base,
				const std::locale& lc = std::locale::classic()) : AbstractGraphemeBreakIterator(lc), p_(base) {}
			/// Returns the base iterator.
			BaseIterator& base() throw() {return p_;}
			/// Returns the base iterator.
			const BaseIterator& base() const throw() {return p_;}
		private:
			CharacterIterator& getCharacterIterator() {return static_cast<CharacterIterator&>(p_);}
			const CharacterIterator& getCharacterIterator() const {return static_cast<const CharacterIterator&>(p_);}
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
			Component	getComponent() const throw();
			bool		isBoundary(const CharacterIterator& at) const;
			void		next(std::ptrdiff_t amount);
			void		setComponent(Component component) throw();
		protected:
			AbstractWordBreakIterator(Component component, const IdentifierSyntax& syntax, const std::locale& lc) throw();
			virtual CharacterIterator& getCharacterIterator() throw() = 0;
			virtual const CharacterIterator& getCharacterIterator() const throw() = 0;
		private:
			void	doNext(std::ptrdiff_t amount);
			void	doPrevious(std::ptrdiff_t amount);
			Component component_;
			const IdentifierSyntax& syntax_;
		};

		/// @c WordBreakIterator locates word boundaries in text.
		template<class BaseIterator> class WordBreakIterator :
			public AbstractWordBreakIterator, public internal::BreakIteratorFacade<WordBreakIterator<BaseIterator> > {
		public:
			/**
			 * Constructor.
			 * @param base the base iterator
			 * @param component the component of word to search
			 * @param syntax the identifier syntax for detecting identifier characters
			 * @param lc the locale
			 */
			WordBreakIterator(BaseIterator base, Component component, const IdentifierSyntax& syntax,
				const std::locale& lc = std::locale::classic()) : AbstractWordBreakIterator(component, syntax, lc), p_(base) {}
			/// Returns the base iterator.
			BaseIterator& base() throw() {return p_;}
			/// Returns the base iterator.
			const BaseIterator& base() const throw() {return p_;}
		private:
			CharacterIterator& getCharacterIterator() {return static_cast<CharacterIterator&>(p_);}
			const CharacterIterator& getCharacterIterator() const {return static_cast<const CharacterIterator&>(p_);}
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
			Component	getComponent() const throw();
			bool		isBoundary(const CharacterIterator& at) const;
			void		next(std::ptrdiff_t amount);
			void		setComponent(Component component) throw();
		protected:
			AbstractSentenceBreakIterator(Component component, const IdentifierSyntax& syntax, const std::locale& lc) throw();
			virtual CharacterIterator& getCharacterIterator() throw() = 0;
			virtual const CharacterIterator& getCharacterIterator() const throw() = 0;
		private:
			void	doNext(std::ptrdiff_t amount);
			void	doPrevious(std::ptrdiff_t amount);
			Component component_;
			const IdentifierSyntax& syntax_;
		};

		/// @c SentenceBreakIterator locates sentence boundaries in text.
		template<class BaseIterator> class SentenceBreakIterator :
			public AbstractSentenceBreakIterator, public internal::BreakIteratorFacade<SentenceBreakIterator<BaseIterator> > {
		public:
			/**
			 * Constructor.
			 * @param base the base iterator
			 * @param component the component of sentence to search
			 * @param syntax the identifier syntax to detect alphabets
			 * @param lc the locale
			 */
			SentenceBreakIterator(BaseIterator base, Component component, const IdentifierSyntax& syntax,
				const std::locale& lc = std::locale::classic()) : AbstractSentenceBreakIterator(component, syntax, lc), p_(base) {}
			/// Returns the base iterator.
			BaseIterator& base() throw() {return p_;}
			/// Returns the base iterator.
			const BaseIterator& base() const throw() {return p_;}
		private:
			CharacterIterator& getCharacterIterator() {return static_cast<CharacterIterator&>(p_);}
			const CharacterIterator& getCharacterIterator() const {return static_cast<const CharacterIterator&>(p_);}
			BaseIterator p_;
		};

		/// Base class of @c LineBreakIterator.
		class AbstractLineBreakIterator : public BreakIterator {
		public:
			bool	isBoundary(const CharacterIterator& at) const;
			void	next(std::ptrdiff_t amount);
		protected:
			AbstractLineBreakIterator(const std::locale& lc) throw();
			virtual CharacterIterator& getCharacterIterator() throw() = 0;
			virtual const CharacterIterator& getCharacterIterator() const throw() = 0;
		};

		/// @c LineBreakIterator locates line break opportunities in text.
		template<class BaseIterator> class LineBreakIterator :
			public AbstractLineBreakIterator, public internal::BreakIteratorFacade<LineBreakIterator<BaseIterator> > {
		public:
			/**
			 * Constructor.
			 * @param base the base iterator
			 * @param lc the locale
			 */
			LineBreakIterator(BaseIterator base, const std::locale& lc = std::locale::classic()) : AbstractLineBreakIterator(lc), p_(base) {}
			/// Returns the base iterator.
			BaseIterator& base() throw() {return p_;}
			/// Returns the base iterator.
			const BaseIterator& base() const throw() {return p_;}
		private:
			CharacterIterator& getCharacterIterator() {return static_cast<CharacterIterator&>(p_);}
			const CharacterIterator& getCharacterIterator() const {return static_cast<const CharacterIterator&>(p_);}
			BaseIterator p_;
		};

		/**
		 * @c CaseFolder folds cases of characters and strings. This behavior is based on Default
		 * Case Algorithm of Unicode, and locale-independent and context-insensitive.
		 * @see Collator, Normalizer, searcher#LiteralPattern
		 */
		class CaseFolder : public manah::Noncopyable {
		public:
			static const length_t	MAXIMUM_EXPANSION_CHARACTERS;
			static int			compare(const CharacterIterator& s1, const CharacterIterator& s2, bool excludeTurkishI = false);
			static int			compare(const String& s1, const String& s2, bool excludeTurkishI = false);
			static CodePoint	fold(CodePoint c, bool excludeTurkishI = false) throw();
			template<typename CharacterSequence>
			static String		fold(CharacterSequence first, CharacterSequence last, bool excludeTurkishI = false);
			static String		fold(const String& text, bool excludeTurkishI = false);
		private:
			static CodePoint	foldCommon(CodePoint c) throw();
			static std::size_t	foldFull(CodePoint c, bool excludeTurkishI, CodePoint* dest) throw();
			static CodePoint	foldTurkishI(CodePoint c) throw();
			static const Char COMMON_CASED[], COMMON_FOLDED[], SIMPLE_CASED[], SIMPLE_FOLDED[], FULL_CASED[];
			static const Char* FULL_FOLDED[];
			static const std::size_t NUMBER_OF_COMMON_CASED, NUMBER_OF_SIMPLE_CASED, NUMBER_OF_FULL_CASED;
		};

		class CollationKey : public manah::FastArenaObject<CollationKey> {
		public:
			CollationKey() throw() : length_(0) {}
			CollationKey(manah::AutoBuffer<const uchar> keyValues, std::size_t length) : keyValues_(keyValues), length_(length) {}
			CollationKey(const CollationKey& rhs);
			CollationKey&	operator=(const CollationKey& rhs);
			bool			operator==(const CollationKey& rhs) const throw();
			bool			operator!=(const CollationKey& rhs) const throw();
			bool			operator<(const CollationKey& rhs) const throw();
			bool			operator<=(const CollationKey& rhs) const throw();
			bool			operator>(const CollationKey& rhs) const throw();
			bool			operator>=(const CollationKey& rhs) const throw();
		private:
			const manah::AutoBuffer<const uchar> keyValues_;
			const std::size_t length_;
		};

		class CollationElementIterator : public BidirectionalIteratorFacade<CollationElementIterator, const int> {
		public:
			static const int NULL_ORDER;
			// BidirectionalIteratorFacade
			value_type dereference() const {return current();}
			void increment() {next();}
			void decrement() {previous();}
			bool equals(const CollationElementIterator& rhs) const {return position() == rhs.position();}
			bool isLessThan(const CollationElementIterator &rhs) const {return position() < rhs.position();}
		protected:
			CollationElementIterator() throw() {}
			virtual int			current() const = 0;
			virtual void		next() = 0;
			virtual void		previous() = 0;
			virtual std::size_t	position() const = 0;
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
			virtual ~Collator() throw();
			// attributes
			Decomposition	getDecomposition() const throw();
			Strength		getStrength() const throw();
			void			setDecomposition(Decomposition newDecomposition);
			void			setStrength(Strength newStrength);
			// operations
			int												compare(const String& s1, const String& s2) const;
			virtual int										compare(const CharacterIterator& s1, const CharacterIterator& s2) const = 0;
			std::auto_ptr<CollationElementIterator>			createCollationElementIterator(const String& source) const;
			virtual std::auto_ptr<CollationElementIterator>	createCollationElementIterator(const CharacterIterator& source) const = 0;
			virtual std::auto_ptr<CollationKey>				getCollationKey(const String& s) const = 0;
		protected:
			Collator() throw() : strength_(IDENTICAL), decomposition_(NO_DECOMPOSITION) {}
		private:
			Strength strength_;
			Decomposition decomposition_;
		};

		/// @c NullCollator performs binary comparison.
		class NullCollator : public Collator {
		public:
			NullCollator() throw();
			int										compare(const CharacterIterator& s1, const CharacterIterator& s2) const;
			std::auto_ptr<CollationElementIterator>	createCollationElementIterator(const CharacterIterator& source) const;
			std::auto_ptr<CollationKey>				getCollationKey(const String& s) const;
		private:
			class ElementIterator : public CollationElementIterator {
			public:
				explicit ElementIterator(std::auto_ptr<CharacterIterator> source) throw() : i_(source) {}
				~ElementIterator() throw() {}
				int current() const {return i_->hasNext() ? **i_ : NULL_ORDER;}
				void next() {++*i_;}
				void previous() {--*i_;}
				std::size_t position() const {return i_->getOffset();}
			private:
				std::auto_ptr<CharacterIterator> i_;
			};
		};

		/**
		 * @c DefaultCollator uses DUCET (Default Unicode Collation Element Table) to collate
		 * characters and strings.
		 */
		class DefaultCollator : public Collator {};


// inline implementations ///////////////////////////////////////////////////

/// Constructor.
inline CharacterIterator::CharacterIterator() throw() : offset_(0) {original_ = this;}

/// Copy-constructor.
inline CharacterIterator::CharacterIterator(const CharacterIterator& rhs) throw() : offset_(rhs.offset_), original_(rhs.original_) {}

/// Destructor.
inline CharacterIterator::~CharacterIterator() throw() {}

/// Assignment operator.
inline CharacterIterator& CharacterIterator::operator=(const CharacterIterator& rhs) {
	assign(rhs); original_ = rhs.original_; offset_ = rhs.offset_; return *this;}

/// Dereference operator returns the code point of the pointed character.
inline CodePoint CharacterIterator::operator*() const throw() {return current();}

/// Pre-fix increment operator.
inline CharacterIterator& CharacterIterator::operator++() {next(); ++offset_; return *this;}

/// Pre-fix decrement operator.
inline CharacterIterator& CharacterIterator::operator--() {previous(); --offset_; return *this;}

/// Equality operator.
inline bool CharacterIterator::operator==(const CharacterIterator& rhs) const {verifyRHS(rhs); return equals(rhs);}

/// Inequality operator.
inline bool CharacterIterator::operator!=(const CharacterIterator& rhs) const {verifyRHS(rhs); return !equals(rhs);}

/// Relational operator.
inline bool CharacterIterator::operator<(const CharacterIterator& rhs) const {verifyRHS(rhs); return less(rhs);}

/// Relational operator.
inline bool CharacterIterator::operator<=(const CharacterIterator& rhs) const {verifyRHS(rhs); return less(rhs) || equals(rhs);}

/// Relational operator.
inline bool CharacterIterator::operator>(const CharacterIterator& rhs) const {verifyRHS(rhs); return !less(rhs) && !equals(rhs);}

/// Relational operator.
inline bool CharacterIterator::operator>=(const CharacterIterator& rhs) const {verifyRHS(rhs); return !less(rhs);}

/// Moves to the start of the character sequence.
inline CharacterIterator& CharacterIterator::first() {doFirst(); offset_ = 0; return *this;}

/// Returns the position in the character sequence.
inline std::ptrdiff_t CharacterIterator::getOffset() const throw() {return offset_;}

/// Returns true if the iterator is a clone of @a other.
inline bool CharacterIterator::isCloneOf(const CharacterIterator& other) const throw() {return original_ == other.original_;}

/// Moves to the end of the character sequence.
inline CharacterIterator& CharacterIterator::last() {doLast(); offset_ = 0; return *this;}

/// Returns the first position.
inline const Char* StringCharacterIterator::getFirst() const throw() {return first_;}

/// Returns the last position.
inline const Char* StringCharacterIterator::getLast() const throw() {return last_;}

/// @see CharacterIterator#hasNext
inline bool StringCharacterIterator::hasNext() const {return current_ != last_;}

/// @see CharacterIterator#hasPrevious
inline bool StringCharacterIterator::hasPrevious() const {return current_ != first_;}

/// Returns the current position.
inline const Char* StringCharacterIterator::tell() const throw() {return current_;}

inline void Normalizer::decrement() {
	if(!hasPrevious())
		throw std::out_of_range("the iterator is the first");
	else if(indexInBuffer_ == 0)
		nextClosure(BACKWARD, false);
	else
		--indexInBuffer_;
}

// Returns the current character in the normalized text.
inline Normalizer::reference Normalizer::dereference() const throw() {return normalizedBuffer_[indexInBuffer_];}

// Returns true if both iterators address the same character in the normalized text.
inline bool Normalizer::equals(const Normalizer& rhs) const throw() {
	return current_->isCloneOf(*rhs.current_) && current_->getOffset() == rhs.current_->getOffset() && indexInBuffer_ == rhs.indexInBuffer_;}

/// Returns the current position in the input text that is being normalized.
inline std::ptrdiff_t Normalizer::getOffset() const throw() {return current_->getOffset();}

inline void Normalizer::increment() {
	if(!hasNext())
		throw std::out_of_range("the iterator is the last.");
	else if(++indexInBuffer_ == normalizedBuffer_.length())
		nextClosure(FORWARD, false);
}

/// Returns false if the iterator addresses the end of the normalized text.
inline bool Normalizer::hasNext() const throw() {return current_->hasNext();}

/// Returns false if the iterator addresses the start of the normalized text.
inline bool Normalizer::hasPrevious() const throw() {return current_->hasPrevious() || indexInBuffer_ != 0;}

/// Returns the word component to search.
inline AbstractWordBreakIterator::Component AbstractWordBreakIterator::getComponent() const throw() {return component_;}

/// Sets the word component to search.
inline void AbstractWordBreakIterator::setComponent(Component component) throw() {component_ = component;}

/// Returns the sentence component to search.
inline AbstractSentenceBreakIterator::Component AbstractSentenceBreakIterator::getComponent() const throw() {return component_;}

/// Sets the sentence component to search.
inline void AbstractSentenceBreakIterator::setComponent(Component component) throw() {component_ = component;}

/**
 * Checks whether the specified character sequence starts with an identifier.
 * The type @a CharacterSequence the bidirectional iterator expresses a UTF-16 character sequence.
 * @param first the start of the character sequence
 * @param last the end of the character sequence
 * @return the end of the detected identifier or @a first if an identifier not found
 */
template<typename CharacterSequence>
inline CharacterSequence IdentifierSyntax::eatIdentifier(CharacterSequence first, CharacterSequence last) const throw() {
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
 * The type @a CharacterSequence is the bidirectional iterator expresses a UTF-16 character sequence.
 * @param first the start of the character sequence
 * @param last the end of the character sequence
 * @param includeTab set true to treat a horizontal tab as a white space
 * @return the end of the detected identifier or @a first if an identifier not found
 */
template<typename CharacterSequence>
inline CharacterSequence IdentifierSyntax::eatWhiteSpaces(CharacterSequence first, CharacterSequence last, bool includeTab) const throw() {
	ASCENSION_STATIC_ASSERT(CodeUnitSizeOf<CharacterSequence>::result == 2);
	UTF16To32Iterator<CharacterSequence> i(first, last);
	while(i.hasNext() && isWhiteSpace(*i, includeTab))
		++i;
	return i.tell();
}

/**
 * Compares the two character sequences case-insensitively.
 * @param s1 the character sequence
 * @param s2 the the other
 * @param excludeTurkishI set true to perform "Turkish I mapping"
 * @retval &lt;0 the first character sequence is less than the second
 * @retval 0 the both sequences are same
 * @retval &gt;0 the first character sequence is greater than the second
 */
inline int CaseFolder::compare(const String& s1, const String& s2, bool excludeTurkishI /* = false */) {
	return compare(StringCharacterIterator(s1), StringCharacterIterator(s2), excludeTurkishI);}

/**
 * Folds the case of the specified character. This method performs "simple case folding."
 * @param c the code point of the character to fold
 * @param excludeTurkishI set true to perform "Turkish I mapping"
 * @return the case-folded character
 */
inline CodePoint CaseFolder::fold(CodePoint c, bool excludeTurkishI /* = false */) throw() {
	CodePoint result;
	// Turkish I
	if(excludeTurkishI && c != (result = foldTurkishI(c)))
		return result;
	// common mapping
	if(c != (result = foldCommon(c)))
		return result;
	// simple mapping
	const Char* const p = std::lower_bound(SIMPLE_CASED, SIMPLE_CASED + NUMBER_OF_SIMPLE_CASED, static_cast<Char>(c & 0xFFFF));
	return (*p == c) ? SIMPLE_FOLDED[p - SIMPLE_CASED] : c;
}

/**
 * Folds case of the specified character sequence. This method performs "full case folding."
 * @a CharacterSequence must represents a UTF-16 character sequence.
 * @param first the start of the character sequence
 * @param last the end of the character sequence
 * @param excludeTurkishI set true to perform "Turkish I mapping"
 * @return the folded string
 */
template<typename CharacterSequence>
inline String CaseFolder::fold(CharacterSequence first, CharacterSequence last, bool excludeTurkishI /* = false */) {
	ASCENSION_STATIC_ASSERT(CodeUnitSizeOf<CharacterSequence>::result == 2);
	using namespace std;
	StringBuffer s(ios_base::out);
	CodePoint c, f;
	Char buffer[2];
	for(UTF16To32Iterator<CharacterSequence> i(first, last); i.hasNext(); ++i) {
		c = *i;
		if(!excludeTurkishI || c == (f = foldTurkishI(*i)))
			f = foldCommon(c);
		if(f != c || c >= 0x010000U) {
			if(surrogates::encode(f, buffer) < 2)	s.sputc(buffer[0]);
			else									s.sputn(buffer, 2);
		} else {
			const Char* const p = lower_bound(FULL_CASED, FULL_CASED + NUMBER_OF_FULL_CASED, static_cast<Char>(c & 0xFFFFU));
			if(*p == c)	s.sputn(FULL_FOLDED[p - FULL_CASED], static_cast<streamsize>(wcslen(FULL_FOLDED[p - FULL_CASED])));
			else		s.sputc(static_cast<Char>(c & 0xFFFFU));
		}
	}
	return s.str();
}

/**
 * Folds case of the specified character sequence. This method performs "full case folding."
 * @param text the character sequence
 * @param excludeTurkishI set true to perform "Turkish I mapping"
 * @return the folded string
 */
inline String CaseFolder::fold(const String& text, bool excludeTurkishI /* = false */) {
	return fold(text.data(), text.data() + text.length(), excludeTurkishI);}

inline CodePoint CaseFolder::foldCommon(CodePoint c) throw() {
	if(c < 0x010000U) {	// BMP
		const Char* const p = std::lower_bound(COMMON_CASED, COMMON_CASED + NUMBER_OF_COMMON_CASED, static_cast<Char>(c & 0xFFFFU));
		return (*p == c) ? COMMON_FOLDED[p - COMMON_CASED] : c;
	} else if(c >= 0x010400U && c < 0x010428U)	// Only Deseret is cased in out of BMP (Unicode 5.0).
		c += 0x000028U;
	return c;
}

inline CodePoint CaseFolder::foldTurkishI(CodePoint c) throw() {if(c == 0x0049U) c = 0x0131U; else if(c == 0x0130U) c = 0x0069U; return c;}

}} // namespace ascension.unicode

#undef CASE_FOLDING_EXPANSION_MAX_CHARS

#endif /* !ASCENSION_UNICODE_HPP */
