/**
 * @file unicode-utils.hpp
 * @author exeal
 * @date 2005-2007
 * @see ascension#unicode, break-iterator.hpp
 */

#ifndef ASCENSION_UNICODE_UTILS_HPP
#define ASCENSION_UNICODE_UTILS_HPP
#include "common.hpp"
#include "../../manah/object.hpp"	// manah::Noncopyable
#include "../../manah/memory.hpp"	// manah::AutoBuffer
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
#define CASE_FOLDING_EXPANSION_MAX_CHARS 3

namespace ascension {

	/**
	 * Provides stuffs implement some of the Unicode standard. This includes:
	 * - @c Normalizer class implements <a href="http://www.unicode.org/reports/tr15/">UAX #15:
	 *   Unicode Normalization Forms</a>
	 * - @c BreakIterator class implements <a href="http://www.unicode.org/reports/tr29/">UAX #29:
	 *   Text Boundary</a>
	 * - @c IdentifierSyntax class implements <a href="http://www.unicode.org/reports/tr31/">UAX
	 *   #31: Identifier and Pattern Syntax</a>
	 * - @c surrogates namespace contains functions to handle UTF-16 surrogate pairs
	 * - Unicode properties
	 * @see ASCENSION_UNICODE_VERSION
	 */
	namespace unicode {

		/**
		 * @c surrogates namespace collects low level procedures handle UTF-16 surrogate pair.
		 * @see UTF16To32Iterator, UTF32To16Iterator, UTF16To32IteratorSafe
		 */
		namespace surrogates {
			/**
			 * Returns if the specified code unit is high (leading)-surrogate.
			 * @param ch the code unit
			 * @return true if @a ch is high-surrogate
			 */
			inline bool isHighSurrogate(Char ch) throw() {return (ch & 0xFC00U) == 0xD800U;}
			/**
			 * Returns if the specified code unit is low (trailing)-surrogate.
			 * @param ch the code unit
			 * @return true if @a ch is low-surrogate
			 */
			inline bool isLowSurrogate(Char ch) throw() {return (ch & 0xFC00U) == 0xDC00U;}
			/**
			 * Returns if the specified code unit is surrogate.
			 * @param ch the code unit
			 * @return true if @a ch is surrogate
			 */
			inline bool isSurrogate(Char ch) throw() {return (ch & 0xF800U) == 0xD800U;}
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
			inline Char getLowSurrogate(CodePoint cp) throw() {return static_cast<Char>(cp & 0x03FF) | 0xDC00U;}
			/**
			 * Converts the specified surrogate pair to a corresponding code point.
			 * @param high the high-surrogate
			 * @param low the low-surrogate
			 * @return the code point or the value of @a high if the pair is not valid
			 */
			inline CodePoint decode(Char high, Char low) throw() {
				return (isHighSurrogate(high) && isLowSurrogate(low)) ? 0x10000 + (high - 0xD800) * 0x400 + low - 0xDC00 : high;}
			/**
			 * Converts a surrogate pair to a corresponding code point.
			 * @param p the pointer to the surrogate pair in a UTF-16 string
			 * @param length the length of @a p. must be greater than zero
			 * @return the code point
			 */
			inline CodePoint decode(const Char* p, std::size_t length) throw() {
				assert(p != 0 && length != 0); return (length > 1) ? decode(p[0], p[1]) : p[0];}
			/**
			 * Converts the specified code point to a corresponding surrogate pair.
			 * @param cp the code point
			 * @param[out] dest the surrogate pair
			 * @retval 1 @a cp is in BMP
			 * @retval 2 @a cp is out of BMP
			 * @throw std#invalid_argument @a cp can't be expressed by UTF-16
			 */
			inline length_t encode(CodePoint cp, Char* dest) {
				assert(dest != 0);
				if(cp < 0x00010000) {
					dest[0] = static_cast<Char>(cp & 0xFFFF);
					return 1;
				} else if(cp <= 0x0010FFFF) {
					dest[0] = getHighSurrogate(cp);
					dest[1] = getLowSurrogate(cp);
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
			inline const Char* next(const Char* start, const Char* last) throw() {
				assert(start != 0 && last != 0 && start < last);
				return start + ((isHighSurrogate(start[0]) && (last - start > 1) && isLowSurrogate(start[1])) ? 2 : 1);
			}
			/**
			 * Searches the previous high-surrogate in the string.
			 * @param first the start of the string
			 * @param start the start position to search
			 * @return the previous high-surrogate
			 */
			inline const Char* previous(const Char* first, const Char* start) throw() {
				assert(first != 0 && start != 0 && first < start);
				return start - ((isLowSurrogate(start[-1]) && (start - first > 1) && isHighSurrogate(start[-2])) ? 2 : 1);
			}
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

		/// 
		class CharacterIterator : public std::iterator<std::random_access_iterator_tag, Char> {
		public:
			static const CodePoint END_OF_BUFFER = 0xFFFFFFFFU;
			virtual ~CharacterIterator() throw() {}
			virtual std::auto_ptr<CharacterIterator> clone() const = 0;
			virtual bool isFirst() const = 0;
			virtual bool isLast() const = 0;
			std::ptrdiff_t getIndex() const throw() {return index_;}
			CodePoint current() const {
				if(isLast()) return END_OF_BUFFER;
				const Char c = dereference();
				if(!surrogates::isHighSurrogate(c)) return c;
				const_cast<CharacterIterator*>(this)->next();
				if(isLast()) {const_cast<CharacterIterator*>(this)->previous(); return c;}
				const Char n = dereference();
				const_cast<CharacterIterator*>(this)->previous();
				return surrogates::decode(c, n);}
			CharacterIterator& next() {if(!isLast()) {
				increment(); ++index_; if(!isLast() && surrogates::isLowSurrogate(dereference())) increment(); ++index_;} return *this;}
			CharacterIterator& previous() {if(!isFirst()) {
				decrement(); --index_; if(!isFirst() && surrogates::isLowSurrogate(dereference())) decrement(); --index_;} return *this;}
			bool operator==(const CharacterIterator& rhs) const throw() {return index_ == rhs.index_;}
			bool operator!=(const CharacterIterator& rhs) const throw() {return index_ != rhs.index_;}
			bool operator<(const CharacterIterator& rhs) const throw() {return index_ < rhs.index_;}
			bool operator<=(const CharacterIterator& rhs) const throw() {return index_ <= rhs.index_;}
			bool operator>(const CharacterIterator& rhs) const throw() {return index_ > rhs.index_;}
			bool operator>=(const CharacterIterator& rhs) const throw() {return index_ >= rhs.index_;}
			std::ptrdiff_t operator-(const CharacterIterator& rhs) const throw() {return index_ - rhs.index_;}
		protected:
			CharacterIterator(std::ptrdiff_t index) throw() : index_(index) {}
			CharacterIterator(const CharacterIterator& rhs) throw() : index_(rhs.index_) {}
			CharacterIterator& operator=(const CharacterIterator& rhs) throw() {index_ = rhs.index_; return *this;}
			virtual Char dereference() const = 0;
			virtual void increment() = 0;
			virtual void decrement() = 0;
		private:
			std::ptrdiff_t index_;
		};

		/// 
		class CStringCharacterIterator : public CharacterIterator {
		public:
			CStringCharacterIterator() throw() : CharacterIterator(0) {}
			CStringCharacterIterator(const Char* start, const Char* first, const Char* last) throw() :
				CharacterIterator(start - first), current_(start), first_(first), last_(last) {}
			CStringCharacterIterator(const CStringCharacterIterator& rhs) throw() :
				CharacterIterator(rhs), current_(rhs.current_), first_(rhs.first_), last_(rhs.last_) {}
			CStringCharacterIterator& operator=(const CStringCharacterIterator& rhs) throw() {
				CharacterIterator::operator=(rhs); current_ = rhs.current_; first_ = rhs.first_; last_ = rhs.last_; return *this;}
			std::auto_ptr<CharacterIterator> clone() const {return std::auto_ptr<CharacterIterator>(new CStringCharacterIterator(*this));}
			void decrement() {--current_;}
			Char dereference() const {return *current_;}
			const Char* getFirst() const {return first_;}
			const Char* getLast() const {return last_;}
			void increment() {++current_;}
			bool isFirst() const {return current_ == first_;}
			bool isLast() const {return current_ == last_;}
			const Char* tell() const throw() {return current_;}
		private:
			const Char* current_;
			const Char* first_;
			const Char* last_;
		};

		/**
		 * Provides constants represent policies on handling ill-formed UTF-16 sequences.
		 * These values are used as the second template parameter of @c UTF16To32Iterator class.
		 */
		namespace utf16boundary {
			enum {
				/// Does not check boundary at all.
				DONT_CHECK,
				/**
				 * The base iterator (the first template parameter of @c UTF16To32Iterator) has
				 * own boundaries. The base iterator must have two method @c isFirst and @c isLast.
				 * take no parameter and return no value.
				 */
				BASE_KNOWS_BOUNDARIES,
				/**
				 * @c UTF16To32Iterator constructor takes additional two iterators represent the
				 * boundaries of the character sequence. The iterator checks whether the current
				 * position is the boundary using these iterators.
				 */
				USE_BOUNDARY_ITERATORS
			};
		}

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
		 * @see UTF16To32Iterator, UTF32To16Iterator
		 */
		template<class BaseIterator, class ConcreteIterator>
		class UTF16To32IteratorBase : public std::iterator<std::bidirectional_iterator_tag,
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
			/// Prefix increment operator.
			ConcreteIterator& operator++() {if(isLast()) throw std::logic_error("The iterator is last.");
				++p_; if(!isLast() && surrogates::isLowSurrogate(*p_)) ++p_; return getConcrete();}
			/// Postfix increment operator.
			const ConcreteIterator operator++(int) {ConcreteIterator temp(getConcrete()); ++*this; return temp;}
			/// Prefix decrement operator.
			ConcreteIterator& operator--() {if(isFirst()) throw std::logic_error("The iterator is first.");
				--p_; if(!isFirst() && surrogates::isLowSurrogate(*p_)) --p_; return getConcrete();}
			/// Postfix decrement operator.
			const ConcreteIterator operator--(int) {ConcreteIterator temp(getConcrete()); --*this; return temp;}
			/// Dereference operator.
			CodePoint operator*() const {
				if(isLast()) throw std::logic_error("The iterator is last.");
				if(!surrogates::isHighSurrogate(*p_)) return *p_;
				BaseIterator next(p_); if(getConcrete().doIsLast(++next)) return *p_; return surrogates::decode(*p_, *next);}
			/// Equality operator.
			bool operator==(const UTF16To32IteratorBase& rhs) const {return p_ == rhs.p_;}
			/// Inequality operator.
			bool operator!=(const UTF16To32IteratorBase& rhs) const {return p_ != rhs.p_;}
			/// Relational operator.
			bool operator<(const UTF16To32IteratorBase& rhs) const {return p_ < rhs.p_;}
			/// Relational operator.
			bool operator<=(const UTF16To32IteratorBase& rhs) const {return p_ <= rhs.p_;}
			/// Relational operator.
			bool operator>(const UTF16To32IteratorBase& rhs) const {return p_ > rhs.p_;}
			/// Relational operator.
			bool operator>=(const UTF16To32IteratorBase& rhs) const {return p_ >= rhs.p_;}
			/// Returns true if the iterator is first. This returns a meaningful value when the policy is not @c DONT_CEHCK.
			bool isFirst() const {return getConcrete().doIsFirst(p_);}
			/// Returns true if the iterator is last. This returns a meaningful value when the policy is not @c DONT_CEHCK.
			bool isLast() const {return getConcrete().doIsLast(p_);}
			/// Returns the current position.
			BaseIterator tell() const {return p_;}
		private:
			ConcreteIterator& getConcrete() throw() {return *static_cast<ConcreteIterator*>(this);}
			const ConcreteIterator& getConcrete() const throw() {return *static_cast<const ConcreteIterator*>(this);}
			BaseIterator p_;
		};

		/**
		 * Concrete class derived from @c UTF16To32IteratorBase.
		 * @param BaseIterator the base bidirectional iterator presents UTF-16 character sequence
		 * @param boundarySafePolicy one of the values defined in @c utf16boundary namespace
		 */
		template<class BaseIterator = const Char*, int boundarySafePolicy = utf16boundary::DONT_CHECK> class UTF16To32Iterator;
		template<class BaseIterator> class UTF16To32Iterator<BaseIterator, utf16boundary::DONT_CHECK>
				: public UTF16To32IteratorBase<BaseIterator, UTF16To32Iterator<BaseIterator, utf16boundary::DONT_CHECK> > {
		public:
			UTF16To32Iterator() {}
			UTF16To32Iterator(BaseIterator i) :
			  UTF16To32IteratorBase<BaseIterator, UTF16To32Iterator<BaseIterator, utf16boundary::DONT_CHECK> >(i) {}
		private:
			bool doIsFirst(const BaseIterator&) const throw() {return false;}
			bool doIsLast(const BaseIterator&) const throw() {return false;}
			friend class UTF16To32IteratorBase<BaseIterator, UTF16To32Iterator<BaseIterator, utf16boundary::DONT_CHECK> >;
		};
		template<class BaseIterator> class UTF16To32Iterator<BaseIterator, utf16boundary::BASE_KNOWS_BOUNDARIES>
				: public UTF16To32IteratorBase<BaseIterator, UTF16To32Iterator<BaseIterator, utf16boundary::BASE_KNOWS_BOUNDARIES> > {
		public:
			UTF16To32Iterator() {}
			UTF16To32Iterator(BaseIterator i) :
			  UTF16To32IteratorBase<BaseIterator, UTF16To32Iterator<BaseIterator, utf16boundary::BASE_KNOWS_BOUNDARIES> >(i) {}
		private:
			bool doIsFirst(const BaseIterator& i) const {return i.isFirst();}
			bool doIsLast(const BaseIterator& i) const {return i.isLast();}
			friend class UTF16To32IteratorBase<BaseIterator, UTF16To32Iterator<BaseIterator, utf16boundary::BASE_KNOWS_BOUNDARIES> >;
		};
		template<class BaseIterator> class UTF16To32Iterator<BaseIterator, utf16boundary::USE_BOUNDARY_ITERATORS>
				: public UTF16To32IteratorBase<BaseIterator, UTF16To32Iterator<BaseIterator, utf16boundary::USE_BOUNDARY_ITERATORS> > {
		private:
			typedef UTF16To32IteratorBase<BaseIterator, UTF16To32Iterator<BaseIterator, utf16boundary::USE_BOUNDARY_ITERATORS> > Base;
		public:
			UTF16To32Iterator() {}
			UTF16To32Iterator(BaseIterator i, BaseIterator first, BaseIterator last) : Base(i), first_(first), last_(last) {}
			UTF16To32Iterator(const UTF16To32Iterator& rhs) : Base(rhs), first_(rhs.first_), last_(rhs.last_) {}
			UTF16To32Iterator& operator=(const UTF16To32Iterator& rhs) {Base::operator=(rhs); first_ = rhs.first_; last_ = rhs.last_; return *this;}
		private:
			bool doIsFirst(const BaseIterator& i) const {return i == first_;}
			bool doIsLast(const BaseIterator& i) const {return i == last_;}
		private:
			BaseIterator first_, last_;
			friend class UTF16To32IteratorBase<BaseIterator, UTF16To32Iterator<BaseIterator, utf16boundary::USE_BOUNDARY_ITERATORS> >;
		};

		/**
		 * Bidirectional iterator scans UTF-32 character sequence as UTF-16.
		 *
		 * Scanned UTF-32 sequence is given by the template parameter @a BaseIterator.
		 *
		 * This supports four relation operators general bidirectional iterators don't have.
		 * These are available if @a BaseIterator have these facilities.
		 * @param BaseIterator the base bidirectional iterator presents UTF-32 character sequence
		 * @see UTF16To32Iterator
		 */
		template<class BaseIterator = const CodePoint*>
		class UTF32To16Iterator : public std::iterator<std::bidirectional_iterator_tag,
				Char, typename std::iterator_traits<BaseIterator>::difference_type, const Char*, const Char> {
		public:
			/// Default constructor.
			UTF32To16Iterator() {}
			/// Constructor takes a position to start iteration.
			UTF32To16Iterator(BaseIterator start) : p_(start), high_(true) {}
			/// Assignment operator.
			UTF32To16Iterator& operator=(const UTF32To16Iterator& rhs) {p_ = rhs.p_; high_ = rhs.high_;}
			/// Prefix increment operator.
			UTF32To16Iterator& operator++() {if(!high_) {high_ = true; ++p_;} else if(*p_ < 0x10000) ++p_; else high_ = false; return *this;}
			/// Postfix increment operator.
			const UTF32To16Iterator operator++(int) {UTF32To16Iterator temp(*this); ++(*this); return temp;}
			/// Prefix decrement operator.
			UTF32To16Iterator&	 operator--() {if(!high_) high_ = true; else {--p_; high_ = *p_ < 0x10000;} return *this;}
			/// Postfix decrement operator.
			const UTF32To16Iterator operator--(int) {UTF32To16Iterator temp(*this); --(*this); return temp;}
			/// Dereference operator.
			Char operator*() const {if(*p_ < 0x10000) return static_cast<Char>(*p_ & 0xFFFF);
				else {Char text[2]; surrogates::encode(*p_, text); return text[high_ ? 0 : 1];}}
			/// Equality operator.
			bool operator==(const UTF32To16Iterator& rhs) const {return p_ == rhs.p_ && high_ == rhs.high_;}
			/// Inequality operator.
			bool operator!=(const UTF32To16Iterator& rhs) const {return p_ != rhs.p_ || high_ != rhs.high_;}
			/// Relational operator.
			bool operator<(const UTF32To16Iterator& rhs) const {return p_ < rhs.p_ || (p_ == rhs.p_ && high_ && !rhs.high_);}
			/// Relational operator.
			bool operator<=(const UTF32To16Iterator& rhs) const {return p_ < rhs.p_ || (p_ == rhs.p_ && (high_ || high_ == rhs.high_));}
			/// Relational operator.
			bool operator>(const UTF32To16Iterator& rhs) const {return p_ > rhs.p_ || (p_ == rhs.p_ && !high_ && rhs.high_);}
			/// Relational operator.
			bool operator>=(const UTF32To16Iterator& rhs) const {return p_ > rhs.p_ || (p_ == rhs.p_ && (rhs.high_ || high_ == rhs.high_));}
			/// Returns the current position.
			BaseIterator tell() const {return p_;}
		private:
			BaseIterator p_;
			bool high_;
		};

		/**
		 * Case folding types.
		 * @note Currently, Ascension supports only locale/language-independent foldings.
		 * @see CaseFolder, CaseFoldings
		 */
		enum CaseFolding {
			CASEFOLDING_NONE			= 0x00,	///< Does not perform case foldings.
			CASEFOLDING_ASCII			= 0x01,	///< Folds only ASCII alphabets.
			CASEFOLDING_UNICODE_SIMPLE	= 0x02,	///< Unicode simple case folding.
			CASEFOLDING_UNICODE_FULL	= 0x03,	///< Unicode full case folding (not implemented).
			CASEFOLDING_TURKISH_I		= 0x04,	///< Performs Turkish mapping.
			CASEFOLDING_TYPE_MASK		= 0x03	///< Mask for obtaining the folding type.
		};

		/// Bit combination of @c CaseFolding.
		typedef manah::Flags<CaseFolding> CaseFoldings;

#ifndef ASCENSION_NO_UNICODE_NORMALIZATION
		class Normalizer : public std::iterator<std::bidirectional_iterator_tag, CodePoint> {
		public:
			/// Normalization forms.
			enum Form {
				FORM_C,		///< Normalization Form C.
				FORM_D,		///< Normalization Form D.
				FORM_KC,	///< Normalization Form KC.
				FORM_KD		///< Normalization Form KD.
			};
			/// Decomposition mapping types.
			enum Type {
				DONT_NORMALIZE,	///< Does not normalize.
				CANONICAL,		///< Canonical normalization.
				COMPATIBILITY	///< Compatibility normalization.
			};
			// constructors
			Normalizer();
			Normalizer(const Char* first, const Char* last, Form form);
			Normalizer(const String& text, Form form);
			Normalizer(const Normalizer& rhs);
			~Normalizer() throw();
			// operators
			Normalizer&			operator=(const Normalizer& rhs);
			CodePoint			operator*() const throw();
			Normalizer&			operator++();
			const Normalizer	operator++(int);
			Normalizer&			operator--();
			const Normalizer	operator--(int);
			bool				operator==(const Normalizer& rhs) const throw();
			bool				operator!=(const Normalizer& rhs) const throw();
			// attributes
			bool		isFirst() const throw();
			bool		isLast() const throw();
			const Char*	tell() const throw();
			// utilities
			static int		compare(const String& s1, const String& s2, Type type, const CaseFoldings& caseFolding);
			static String	normalize(const Char* first, const Char* last, Form form);
		private:
			static std::basic_string<CodePoint>	normalize(UTF16To32Iterator<const Char*, utf16boundary::USE_BOUNDARY_ITERATORS> i, Form form);
			void								normalizeCurrentBlock(Direction direction);
			Form form_;
			UTF16To32Iterator<const Char*, utf16boundary::USE_BOUNDARY_ITERATORS> current_;
			std::basic_string<CodePoint> normalizedBuffer_;
			std::size_t indexInBuffer_;
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
			explicit IdentifierSyntax(CharacterClassification type, const CaseFoldings& caseFolding = CASEFOLDING_NONE
#ifndef ASCENSION_NO_UNICODE_NORMALIZATION
				, Normalizer::Type normalizationType = Normalizer::DONT_NORMALIZE
#endif /* !ASCENSION_NO_UNICODE_NORMALIZATION */
			) throw();
			IdentifierSyntax(const IdentifierSyntax& rhs) throw();
			IdentifierSyntax&	operator=(const IdentifierSyntax& rhs) throw();
			// classification for character
			bool	isIdentifierStartCharacter(CodePoint cp) const throw();
			bool	isIdentifierContinueCharacter(CodePoint cp) const throw();
			bool	isWhiteSpace(CodePoint cp, bool includeTab) const throw();
			// classification for sequence
			template<class CharacterSequence>
			CharacterSequence	eatIdentifier(CharacterSequence first, CharacterSequence last) const throw();
			template<class CharacterSequence>
			CharacterSequence	eatWhiteSpaces(CharacterSequence, CharacterSequence last, bool includeTab) const throw();
			// attributes
			void	overrideIdentifierStartCharacters(const String& adding, const String& subtracting);
			void	overrideIdentifierStartCharacters(const std::set<CodePoint>& adding, const std::set<CodePoint>& subtracting);
			void	overrideIdentifierNonStartCharacters(const String& adding, const String& subtracting);
			void	overrideIdentifierNonStartCharacters(const std::set<CodePoint>& adding, const std::set<CodePoint>& subtracting);
		private:
			CharacterClassification type_;
			CaseFoldings caseFolding_;
#ifndef ASCENSION_NO_UNICODE_NORMALIZATION
			Normalizer::Type normalizationType_;
#endif /* !ASCENSION_NO_UNICODE_NORMALIZATION */
			std::basic_string<CodePoint> addedIDStartCharacters_, addedIDNonStartCharacters_;
			std::basic_string<CodePoint> subtractedIDStartCharacters_, subtractedIDNonStartCharacters_;
		};

		/// @c CaseFolder performs case foldings.
		class CaseFolder : public manah::Noncopyable {
		public:
			template<CaseFolding type>
			static bool						compare(const Char* p1, const Char* p2, length_t length);
			static Char						foldASCII(Char ch) throw();
			static length_t					foldFull(CodePoint cp, CodePoint* dest);
			static manah::AutoBuffer<Char>	foldFull(const Char* first, const Char* last);
			static length_t					foldFull(const Char* first, const Char* last, Char* dest);
			static CodePoint				foldSimple(CodePoint cp);
			static manah::AutoBuffer<Char>	foldSimple(const Char* first, const Char* last);
			static void						foldSimple(const Char* first, const Char* last, Char* dest);
		private:
			static const Char CASED_UCS2[], FOLDED_UCS2[];
			static const CodePoint CASED_UCS4[], FOLDED_UCS4[];
		};

		namespace internal {
			// helpers for Unicode properties implementation
			template<class Code> struct CodeRange {
				Code first, last;
				bool operator<(Code rhs) const {return first < rhs;}
			};
			struct PropertyRange {
				CodePoint first, last;
				ushort property;
				bool operator<(CodePoint rhs) const {return first < rhs;}
			};
			template<class Element> static const Element* findInRange(const Element* first, const Element* last, CodePoint cp) {
				const Element* p = std::lower_bound(first, last, cp);
				if(p == last) return 0;
				else if(p->first == cp) return p;
				else if(p->first > cp && p != first && p[-1].last >= cp) return p - 1;
				else return 0;
			}
		} // namespace internal

		/**
		 * A function object compares Unicode property (value) names based on "Property and Property Value Matching"
		 * (http://www.unicode.org/Public/UNIDATA/UCD.html#Property_and_Property_Value_Matching).
		 * @param p1 one property name
		 * @param p2 the other property name
		 * @return true if p1 &lt; p2
		 */
		template<typename CharType>
		struct PropertyNameComparer {
			bool		operator()(const CharType* p1, const CharType* p2) const;
			static int	compare(const CharType* p1, const CharType* p2);
		};

		const int NOT_PROPERTY = 0;

		/**
		 * General categories.
		 * These values are based on Unicode standard 5.0.0 "4.5 General Category".
		 */
		class GeneralCategory {
		public:
			enum {
				// sub-categories
				LETTER_UPPERCASE = 1,		///< Lu = Letter, uppercase
				LETTER_LOWERCASE,			///< Ll = Letter, lowercase
				LETTER_TITLECASE,			///< Lt = Letter, titlecase
				LETTER_MODIFIER,			///< Lm = Letter, modifier
				LETTER_OTHER,				///< Lo = Letter, other
				MARK_NONSPACING,			///< Mn = Mark, nonspacing
				MARK_SPACING_COMBINING,		///< Mc = Mark, spacing combining
				MARK_ENCLOSING,				///< Me = Mark, enclosing
				NUMBER_DECIMAL_DIGIT,		///< Nd = Number, decimal digit
				NUMBER_LETTER,				///< Nl = Number, letter
				NUMBER_OTHER,				///< No = Number, other
				PUNCTUATION_CONNECTOR,		///< Pc = Punctuation, connector
				PUNCTUATION_DASH,			///< Pd = Punctuation, dash
				PUNCTUATION_OPEN,			///< Ps = Punctuation, open
				PUNCTUATION_CLOSE,			///< Pe = Punctuation, close
				PUNCTUATION_INITIAL_QUOTE,	///< Pi = Punctuation, initial quote
				PUNCTUATION_FINAL_QUOTE,	///< Pf = Punctuation, final quote
				PUNCTUATION_OTHER,			///< Po = Punctuation, other
				SYMBOL_MATH,				///< Sm = Symbol, math
				SYMBOL_CURRENCY,			///< Sc = Symbol, currency
				SYMBOL_MODIFIER,			///< Sk = Symbol, modifier
				SYMBOL_OTHER,				///< So = Symbol, other
				SEPARATOR_SPACE,			///< Zs = Separator, space
				SEPARATOR_LINE,				///< Zl = Separator, line
				SEPARATOR_PARAGRAPH,		///< Zp = Separator, paragraph
				OTHER_CONTROL,				///< Cc = Other, control
				OTHER_FORMAT,				///< Cf = Other, format
				OTHER_SURROGATE,			///< Cs = Other, surrogate
				OTHER_PRIVATE_USE,			///< Co = Other, private use
				OTHER_UNASSIGNED,			///< Cn = Other, not assigned
				// super-categories
				LETTER,			///< L = Letter
				LETTER_CASED,	///< Lc = Letter, cased
				MARK,			///< M = Mark
				NUMBER,			///< N = Number
				PUNCTUATION,	///< P = Punctuation
				SYMBOL,			///< S = Symbol
				SEPARATOR,		///< Z = Separator
				OTHER,			///< C = Other
				COUNT
			};
			static const Char LONG_NAME[], SHORT_NAME[];
			static int	forName(const Char* name);
			template<int superCategory>
			static bool	is(int subCategory);
			static int	of(CodePoint cp) throw();
		private:
			static const internal::PropertyRange ranges_[];
			static const std::size_t count_;
			static std::map<const Char*, int, PropertyNameComparer<Char> > names_;
			static void buildNames();
		};
		
		/// Returns true if the specified character is a letter.
		template<> inline bool GeneralCategory::is<GeneralCategory::LETTER>(int gc) {return gc >= LETTER_UPPERCASE && gc <= LETTER_OTHER;}
		/// Returns true if the specified sub-category is a cased letter.
		template<> inline bool GeneralCategory::is<GeneralCategory::LETTER_CASED>(int gc) {return gc >= LETTER_UPPERCASE && gc <= LETTER_TITLECASE;}
		/// Returns true if the specified sub-category is a mark.
		template<> inline bool GeneralCategory::is<GeneralCategory::MARK>(int gc) {return gc >= MARK_NONSPACING && gc <= MARK_ENCLOSING;}
		/// Returns true if the specified sub-category is a number.
		template<> inline bool GeneralCategory::is<GeneralCategory::NUMBER>(int gc) {return gc >= NUMBER_DECIMAL_DIGIT && gc <= NUMBER_OTHER;}
		/// Returns true if the specified sub-category is a punctuation.
		template<> inline bool GeneralCategory::is<GeneralCategory::PUNCTUATION>(int gc) {return gc >= PUNCTUATION_CONNECTOR && gc <= PUNCTUATION_OTHER;}
		/// Returns true if the specified sub-category is a symbol.
		template<> inline bool GeneralCategory::is<GeneralCategory::SYMBOL>(int gc) {return gc >= SYMBOL_MATH && gc <= SYMBOL_OTHER;}
		/// Returns true if the specified sub-category is a separator.
		template<> inline bool GeneralCategory::is<GeneralCategory::SEPARATOR>(int gc) {return gc >= SEPARATOR_SPACE && gc <= SEPARATOR_PARAGRAPH;}
		/// Returns true if the specified sub-category is an other.
		template<> inline bool GeneralCategory::is<GeneralCategory::OTHER>(int gc) {return gc >= OTHER_CONTROL && gc <= OTHER_UNASSIGNED;}
		
		/**
		 * Code blocks.
		 * These values are based on Blocks.txt obtained from UCD.
		 */
		class CodeBlock {
		public:
			enum {
				NO_BLOCK = GeneralCategory::COUNT,
				BASIC_LATIN, LATIN_1_SUPPLEMENT, LATIN_EXTENDED_A, LATIN_EXTENDED_B, IPA_EXTENSIONS,
				SPACING_MODIFIER_LETTERS, COMBINING_DIACRITICAL_MARKS, GREEK_AND_COPTIC, CYRILLIC,
				CYRILLIC_SUPPLEMENT, ARMENIAN, HEBREW, ARABIC, SYRIAC, ARABIC_SUPPLEMENT, THAANA,
				NKO, DEVANAGARI, BENGALI, GURMUKHI, GUJARATI, ORIYA, TAMIL, TELUGU, KANNADA, MALAYALAM,
				SINHALA, THAI, LAO, TIBETAN, MYANMAR, GEORGIAN, HANGUL_JAMO, ETHIOPIC, ETHIOPIC_SUPPLEMENT,
				CHEROKEE, UNIFIED_CANADIAN_ABORIGINAL_SYLLABICS, OGHAM, RUNIC, TAGALOG, HANUNOO, BUHID,
				TAGBANWA, KHMER, MONGOLIAN, LIMBU, TAI_LE, NEW_TAI_LUE, KHMER_SYMBOLS, BUGINESE,
				BALINESE, PHONETIC_EXTENSIONS, PHONETIC_EXTENSIONS_SUPPLEMENT,
				COMBINING_DIACRITICAL_MARKS_SUPPLEMENT, LATIN_EXTENDED_ADDITIONAL, GREEK_EXTENDED,
				GENERAL_PUNCTUATION, SUPERSCRIPTS_AND_SUBSCRIPTS, CURRENCY_SYMBOLS,
				COMBINING_DIACRITICAL_MARKS_FOR_SYMBOLS, LETTERLIKE_SYMBOLS, NUMBER_FORMS, ARROWS,
				MATHEMATICAL_OPERATORS, MISCELLANEOUS_TECHNICAL, CONTROL_PICTURES,
				OPTICAL_CHARACTER_RECOGNITION, ENCLOSED_ALPHANUMERICS, BOX_DRAWING, BLOCK_ELEMENTS,
				GEOMETRIC_SHAPES, MISCELLANEOUS_SYMBOLS, DINGBATS, MISCELLANEOUS_MATHEMATICAL_SYMBOLS_A,
				SUPPLEMENTAL_ARROWS_A, BRAILLE_PATTERNS, SUPPLEMENTAL_ARROWS_B,
				MISCELLANEOUS_MATHEMATICAL_SYMBOLS_B, SUPPLEMENTAL_MATHEMATICAL_OPERATORS,
				MISCELLANEOUS_SYMBOLS_AND_ARROWS, GLAGOLITIC, LATIN_EXTENDED_C, COPTIC,
				GEORGIAN_SUPPLEMENT, TIFINAGH, ETHIOPIC_EXTENDED, SUPPLEMENTAL_PUNCTUATION,
				CJK_RADICALS_SUPPLEMENT, KANGXI_RADICALS, IDEOGRAPHIC_DESCRIPTION_CHARACTERS,
				CJK_SYMBOLS_AND_PUNCTUATION, HIRAGANA, KATAKANA, BOPOMOFO, HANGUL_COMPATIBILITY_JAMO,
				KANBUN, BOPOMOFO_EXTENDED, CJK_STROKES, KATAKANA_PHONETIC_EXTENSIONS,
				ENCLOSED_CJK_LETTERS_AND_MONTHS, CJK_COMPATIBILITY, CJK_UNIFIED_IDEOGRAPHS_EXTENSION_A,
				YIJING_HEXAGRAM_SYMBOLS, CJK_UNIFIED_IDEOGRAPHS, YI_SYLLABLES, YI_RADICALS,
				MODIFIER_TONE_LETTERS, LATIN_EXTENDED_D, SYLOTI_NAGRI, PHAGS_PA, HANGUL_SYLLABLES,
				HIGH_SURROGATES, HIGH_PRIVATE_USE_SURROGATES, LOW_SURROGATES, PRIVATE_USE_AREA,
				CJK_COMPATIBILITY_IDEOGRAPHS, ALPHABETIC_PRESENTATION_FORMS, ARABIC_PRESENTATION_FORMS_A,
				VARIATION_SELECTORS, VERTICAL_FORMS, COMBINING_HALF_MARKS, CJK_COMPATIBILITY_FORMS,
				SMALL_FORM_VARIANTS, ARABIC_PRESENTATION_FORMS_B, HALFWIDTH_AND_FULLWIDTH_FORMS,
				SPECIALS, LINEAR_B_SYLLABARY, LINEAR_B_IDEOGRAMS, AEGEAN_NUMBERS, ANCIENT_GREEK_NUMBERS,
				OLD_ITALIC, GOTHIC, UGARITIC, OLD_PERSIAN, DESERET, SHAVIAN, OSMANYA, CYPRIOT_SYLLABARY,
				PHOENICIAN, KHAROSHTHI, CUNEIFORM, CUNEIFORM_NUMBERS_AND_PUNCTUATION, BYZANTINE_MUSICAL_SYMBOLS,
				MUSICAL_SYMBOLS, ANCIENT_GREEK_MUSICAL_NOTATION, TAI_XUAN_JING_SYMBOLS,
				COUNTING_ROD_NUMERALS, MATHEMATICAL_ALPHANUMERIC_SYMBOLS, CJK_UNIFIED_IDEOGRAPHS_EXTENSION_B,
				CJK_COMPATIBILITY_IDEOGRAPHS_SUPPLEMENT, TAGS, VARIATION_SELECTORS_SUPPLEMENT,
				SUPPLEMENTARY_PRIVATE_USE_AREA_A, SUPPLEMENTARY_PRIVATE_USE_AREA_B, COUNT
			};
			static const Char LONG_NAME[], SHORT_NAME[];
			static int	forName(const Char* name);
			static int	of(CodePoint cp) throw();
		private:
			static const internal::PropertyRange ranges_[];
			static const std::size_t count_;
			static std::map<const Char*, int, PropertyNameComparer<Char> > names_;
			static void buildNames();
		};

#ifndef ASCENSION_NO_UNICODE_NORMALIZATION
		/**
		 * Canonical combining classes.
		 * These are based on Unicode standard 5.0.0 "4.3 Combining Classes".
		 * @see Normalizer
		 */
		class CanonicalCombiningClass {
		public:
			enum {
				NOT_REORDERED			= 0,	///< Spacing, split, enclosing, reordrant, and Tibetan subjoined (0).
				OVERLAY					= 1,	///< Overlays and interior (1).
				NUKTA					= 7,	///< Nuktas (7).
				KANA_VOICING			= 8,	///< Hiragana/Katakana voicing marks (8).
				VIRAMA					= 9,	///< Viramas (9).
				ATTACHED_BELOW_LEFT		= 200,	///< Below left attached (200).
				ATTACHED_BELOW			= 202,	///< Below attached (202).
				ATTACHED_BELOW_RIGHT	= 204,	///< Below right attached (204). This class does not currently have members.
				ATTACHED_LEFT			= 208,	///< Left attached (208). This class does not currently have members.
				ATTACHED_RIGHT			= 210,	///< Right attached (210). This class does not currently have members.
				ATTACHED_ABOVE_LEFT		= 212,	///< Above left attached (212). This class does not currently have members.
				ATTACHED_ABOVE			= 214,	///< Above attached (214). This class does not currently have members.
				ATTAHCED_ABOVE_RIGHT	= 216,	///< Above right attached (216).
				BELOW_LEFT				= 218,	///< Below left (218).
				BELOW					= 220,	///< Below (220)
				BELOW_RIGHT				= 222,	///< Below right (222).
				LEFT					= 224,	///< Left (224).
				RIGHT					= 226,	///< Right (226).
				ABOVE_LEFT				= 228,	///< Above left (228).
				ABOVE					= 230,	///< Above (230).
				ABOVE_RIGHT				= 232,	///< Above right (232).
				DOUBLE_BELOW			= 233,	///< Double below (233).
				DOUBLE_ABOVE			= 234,	///< Double above (234).
				IOTA_SUBSCRIPT			= 240	///< Below (iota subscript) (240).
			};
			static const Char LONG_NAME[], SHORT_NAME[];
			static int	forName(const Char* name);
			static int	of(CodePoint cp) throw();
		private:
			static const Char SRC_UCS2[];
			static const CodePoint SRC_UCS4[];
			static const uchar DEST_UCS2[], DEST_UCS4[];
			static const std::size_t UCS2_COUNT, UCS4_COUNT;
			static std::map<const Char*, int, PropertyNameComparer<Char> > names_;
			static void buildNames();
		};
#endif /* !ASCENSION_NO_UNICODE_NORMALIZATION */

		/**
		 * Scripts.
		 * These are based on <a href="http://www.unicode.org/reports/tr24/">UAX #24: Script Names</a>
		 * revision 9 and Scripts.txt obtained from UCD.
		 */
		class Script {
		public:
			enum {
				UNKNOWN = CodeBlock::COUNT, COMMON,
				// Unicode 4.0
				LATIN, GREEK, CYRILLIC, ARMENIAN, HEBREW, ARABIC, SYRIAC, THAANA,
				DEVANAGARI, BENGALI, GURMUKHI, GUJARATI, ORIYA, TAMIL, TELUGU, KANNADA,
				MALAYALAM, SINHALA, THAI, LAO, TIBETAN, MYANMAR, GEORGIAN, HANGUL,
				ETHIOPIC, CHEROKEE, CANADIAN_ABORIGINAL, OGHAM, RUNIC, KHMER, MONGOLIAN,
				HIRAGANA, KATAKANA, BOPOMOFO, HAN, YI, OLD_ITALIC, GOTHIC, DESERET,
				INHERITED, TAGALOG, HANUNOO, BUHID, TAGBANWA, LIMBU, TAI_LE,
				LINEAR_B, UGARITIC, SHAVIAN, OSMANYA, CYPRIOT, BRAILLE,
				// Unicode 4.1
				BUGINESE, COPTIC, NEW_TAI_LUE, GLAGOLITIC, TIFINAGH, SYLOTI_NAGRI,
				OLD_PERSIAN, KHAROSHTHI,
				// Unicode 5.0
				BALINESE, CUNEIFORM, PHOENICIAN, PHAGS_PA, NKO,
				// derived
				KATAKANA_OR_HIRAGANA,
				COUNT
			};
			static const Char LONG_NAME[], SHORT_NAME[];
			static int	forName(const Char* name);
			static int	of(CodePoint cp) throw();
		private:
			static const internal::PropertyRange ranges_[];
			static const std::size_t count_;
			static std::map<const Char*, int, PropertyNameComparer<Char> > names_;
			static void buildNames();
		};

		/**
		 * Hangul syllable types.
		 * These values are based on HangulSyllableType.txt obtained from UCD.
		 */
		class HangulSyllableType {
		public:
			enum {
				NOT_APPLICABLE = Script::COUNT,	///< NA = Not_Applicable
				LEADING_JAMO,					///< L = Leading_Jamo
				VOWEL_JAMO,						///< V = Vowel_Jamo
				TRAILING_JAMO,					///< T = Trailing_Jamo
				LV_SYLLABLE,					///< LV = LV_Syllable
				LVT_SYLLABLE,					///< LVT = LVT_Syllable
				COUNT
			};
			static const Char LONG_NAME[], SHORT_NAME[];
			static int	forName(const Char* name);
			static int	of(CodePoint cp) throw();
		private:
			static std::map<const Char*, int, PropertyNameComparer<Char> > names_;
			static void buildNames();
		};
		
		/**
		 * Binary properties.
		 * These values are based on UCD.html and PropList.txt obtained from UCD.
		 * @note Some values are not implemented.
		 */
		class BinaryProperty {
		public:
			enum {
				ALPHABETIC = HangulSyllableType::COUNT, ASCII_HEX_DIGIT, BIDI_CONTROL, BIDI_MIRRORED,
				COMPOSITION_EXCLUSION, DASH, DEFAULT_IGNORABLE_CODE_POINT, DEPRECATED, DIACRITIC,
				EXPANDS_ON_NFC, EXPANDS_ON_NFD, EXPANDS_ON_NFKC, EXPANDS_ON_NFKD, EXTENDER,
				FULL_COMPOSITION_EXCLUSION, GRAPHEME_BASE, GRAPHEME_EXTEND, HEX_DIGIT, HYPHEN,
				ID_CONTINUE, ID_START, IDEOGRAPHIC, IDS_BINARY_OPERATOR, IDS_TRINARY_OPERATOR,
				JOIN_CONTROL, LOGICAL_ORDER_EXCEPTION, LOWERCASE, MATH, NONCHARACTER_CODE_POINT,
				OTHER_ALPHABETIC, OTHER_DEFAULT_IGNORABLE_CODE_POINT, OTHER_GRAPHEME_EXTEND,
				OTHER_ID_CONTINUE, OTHER_ID_START, OTHER_LOWERCASE, OTHER_MATH, OTHER_UPPERCASE,
				PATTERN_SYNTAX, PATTERN_WHITE_SPACE, QUOTATION_MARK, RADICAL, SOFT_DOTTED, STERM,
				TERMINAL_PUNCTUATION, UNIFIED_IDEOGRAPH, UPPERCASE, VARIATION_SELECTOR, WHITE_SPACE,
				XID_CONTINUE, XID_START, COUNT
			};
			static int	forName(const Char* first, const Char* last);
			static bool	is(CodePoint cp, int property);
			template<int property>
			static bool	is(CodePoint cp) throw();
		private:
			static std::map<const Char*, int, PropertyNameComparer<Char> > names_;
			static void buildNames();
#include "code-table/uprops-binary-property-table-definition"
		};

#include "code-table/uprops-implementation"
		// derived core properties (explicit specialization)
		template<> inline bool BinaryProperty::is<BinaryProperty::ALPHABETIC>(CodePoint cp) {
			const int gc = GeneralCategory::of(cp);
			return gc == GeneralCategory::LETTER_UPPERCASE
				|| gc == GeneralCategory::LETTER_LOWERCASE
				|| gc == GeneralCategory::LETTER_TITLECASE
				|| gc == GeneralCategory::LETTER_OTHER
				|| gc == GeneralCategory::NUMBER_LETTER
				|| is<OTHER_ALPHABETIC>(cp);}
		template<> inline bool BinaryProperty::is<BinaryProperty::DEFAULT_IGNORABLE_CODE_POINT>(CodePoint cp) {
			const int gc = GeneralCategory::of(cp);
			return (gc == GeneralCategory::OTHER_FORMAT
				|| gc == GeneralCategory::OTHER_CONTROL
				|| gc == GeneralCategory::OTHER_SURROGATE
				|| is<OTHER_DEFAULT_IGNORABLE_CODE_POINT>(cp)
				|| is<NONCHARACTER_CODE_POINT>(cp))
				&& !is<WHITE_SPACE>(cp)
				&& (cp < 0xFFF9 || cp > 0xFFFB);}
		template<> inline bool BinaryProperty::is<BinaryProperty::LOWERCASE>(CodePoint cp) {
			return GeneralCategory::of(cp) == GeneralCategory::LETTER_LOWERCASE || is<OTHER_LOWERCASE>(cp);}
		template<> inline bool BinaryProperty::is<BinaryProperty::GRAPHEME_EXTEND>(CodePoint cp) {
			const int gc = GeneralCategory::of(cp);
			return gc == GeneralCategory::MARK_ENCLOSING
				|| gc == GeneralCategory::MARK_NONSPACING
				|| is<OTHER_GRAPHEME_EXTEND>(cp);}
		template<> inline bool BinaryProperty::is<BinaryProperty::GRAPHEME_BASE>(CodePoint cp) {
			const int gc = GeneralCategory::of(cp);
			return !GeneralCategory::is<GeneralCategory::OTHER>(gc)
				&& gc != GeneralCategory::SEPARATOR_LINE
				&& gc != GeneralCategory::SEPARATOR_PARAGRAPH
				&& !is<GRAPHEME_EXTEND>(cp);}
		template<> inline bool BinaryProperty::is<BinaryProperty::ID_CONTINUE>(CodePoint cp) {
			const int gc = GeneralCategory::of(cp);
			return GeneralCategory::is<GeneralCategory::LETTER>(gc)
				|| gc == GeneralCategory::MARK_NONSPACING
				|| gc == GeneralCategory::MARK_SPACING_COMBINING
				|| gc == GeneralCategory::NUMBER_DECIMAL_DIGIT
				|| gc == GeneralCategory::NUMBER_LETTER
				|| gc == GeneralCategory::PUNCTUATION_CONNECTOR
				|| is<OTHER_ID_START>(cp)
				|| is<OTHER_ID_CONTINUE>(cp);}
		template<> inline bool BinaryProperty::is<BinaryProperty::ID_START>(CodePoint cp) {
			const int gc = GeneralCategory::of(cp);
			return GeneralCategory::is<GeneralCategory::LETTER>(gc)
				|| gc == GeneralCategory::NUMBER_LETTER
				|| is<OTHER_ID_START>(cp);}
		template<> inline bool BinaryProperty::is<BinaryProperty::MATH>(CodePoint cp) {
			return GeneralCategory::of(cp) == GeneralCategory::SYMBOL_MATH || is<OTHER_MATH>(cp);}
		template<> inline bool BinaryProperty::is<BinaryProperty::UPPERCASE>(CodePoint cp) {
			return GeneralCategory::of(cp) == GeneralCategory::LETTER_UPPERCASE || is<OTHER_UPPERCASE>(cp);}

		/**
		 * Grapheme_Cluster_Break property.
		 * These values are based on UAX #29.
		 */
		class GraphemeClusterBreak {
		public:
			enum {
				CR = BinaryProperty::COUNT, LF, CONTROL, EXTEND, L, V, T, LV, LVT, OTHER, COUNT
			};
			static const Char LONG_NAME[], SHORT_NAME[];
			static int	forName(const Char* name);
			static int	of(CodePoint cp) throw();
		private:
			static std::map<const Char*, int, PropertyNameComparer<Char> > names_;
			static void buildNames();
		};

		/**
		 * Word_Break property.
		 * These values are based on UAX #29.
		 */
		class WordBreak {
		public:
			enum {
				FORMAT = GraphemeClusterBreak::COUNT, KATAKANA, A_LETTER, MID_LETTER, MID_NUM, NUMERIC, EXTEND_NUM_LET, OTHER, COUNT
			};
			static const Char LONG_NAME[], SHORT_NAME[];
			static int	forName(const Char* name);
			static int	of(CodePoint cp,
							const IdentifierSyntax& syntax = IdentifierSyntax(IdentifierSyntax::UNICODE_DEFAULT),
							const std::locale& lc = std::locale::classic()) throw();
		private:
			static std::map<const Char*, int, PropertyNameComparer<Char> > names_;
			static void buildNames();
		};

		/**
		 * Sentence_Break property.
		 * These values are based on UAX #29.
		 */
		class SentenceBreak {
		public:
			enum {
				SEP = WordBreak::COUNT, FORMAT, SP, LOWER, UPPER, O_LETTER, NUMERIC, A_TERM, S_TERM, CLOSE, OTHER, COUNT
			};
			static const Char LONG_NAME[], SHORT_NAME[];
			static int	forName(const Char* name);
			static int	of(CodePoint cp) throw();
		private:
			static std::map<const Char*, int, PropertyNameComparer<Char> > names_;
			static void buildNames();
		};

		/**
		 * Legacy character classification like @c std#ctype (from <a
		 * href="http://www.unicode.org/reports/tr18/">UTS #18: Unicode Regular Expression, Annex
		 * C: Compatibility Property</a>.
		 */
		namespace legacyctype {
			/// Returns true if the character is an alphabet (alpha := \\p{Alphabetic}).
			inline bool isalpha(CodePoint cp) {return BinaryProperty::is<BinaryProperty::ALPHABETIC>(cp);}
			/// Returns true if the character is an alphabet or numeric (alnum := [:alpha:] | [:digit:]).
			inline bool isalnum(CodePoint cp) {return isalpha(cp) || isdigit(cp);}
			/// Returns true if the character is a blank (blank := \\p{Whitespace} - [\\N{LF} \\N{VT} \\N{FF} \\N{CR} \\N{NEL} \\p{gc=Line_Separator} \\p{gc=Paragraph_Separator}]).
			inline bool isblank(CodePoint cp) {
				if(cp == LINE_FEED || cp == L'\v' || cp == L'\f' || cp == CARRIAGE_RETURN || cp == NEXT_LINE)	return false;
				if(BinaryProperty::is<BinaryProperty::WHITE_SPACE>(cp)) {
					const int gc = GeneralCategory::of(cp);
					return gc != GeneralCategory::SEPARATOR_LINE && gc != GeneralCategory::SEPARATOR_PARAGRAPH;
				}
				return false;
			}
			/// Returns true if the character is a control code (cntrl := \\p{gc=Control}).
			inline bool iscntrl(CodePoint cp) {return GeneralCategory::of(cp) == GeneralCategory::OTHER_CONTROL;}
			/// Returns true if the character is a digit (digit := \\p{gc=Decimal_Number}).
			inline bool isdigit(CodePoint cp) {return GeneralCategory::of(cp) == GeneralCategory::NUMBER_DECIMAL_DIGIT;}
			/// Returns true if the character is graphical (graph := [^[:space:]\\p{gc=Control}\\p{Format}\\p{Surrogate}\\p{Unassigned}]).
			inline bool isgraph(CodePoint cp) {
				if(isspace(cp))	return false;
				const int gc = GeneralCategory::of(cp);
				return gc != GeneralCategory::OTHER_CONTROL
					&& gc != GeneralCategory::OTHER_FORMAT
					&& gc != GeneralCategory::OTHER_SURROGATE
					&& gc != GeneralCategory::OTHER_UNASSIGNED;
			}
			/// Returns true if the character is lower (lower := \\p{Lowercase}).
			inline bool islower(CodePoint cp) {return BinaryProperty::is<BinaryProperty::LOWERCASE>(cp);}
			/// Returns true if the character is printable (print := ([:graph] | [:blank:]) - [:cntrl:]).
			inline bool isprint(CodePoint cp) {return (isgraph(cp) || isblank(cp)) && !iscntrl(cp);}
			/// Returns true if the character is a punctuation (punct := \\p{gc=Punctuation}).
			inline bool ispunct(CodePoint cp) {return GeneralCategory::is<GeneralCategory::PUNCTUATION>(GeneralCategory::of(cp));}
			/// Returns true if the character is a white space (space := \\p{Whitespace}).
			inline bool isspace(CodePoint cp) {return BinaryProperty::is<BinaryProperty::WHITE_SPACE>(cp);}
			/// Returns true if the character is capital (upper := \\p{Uppercase}).
			inline bool isupper(CodePoint cp) {return BinaryProperty::is<BinaryProperty::UPPERCASE>(cp);}
			/// Returns true if the character can consist a word (word := [:alpha:]\\p{gc=Mark}[:digit:]\\p{gc=Connector_Punctuation}).
			inline bool isword(CodePoint cp) {
				if(isalpha(cp) || isdigit(cp)) return true;
				const int gc = GeneralCategory::of(cp);
				return GeneralCategory::is<GeneralCategory::MARK>(gc) || gc == GeneralCategory::PUNCTUATION_CONNECTOR;}
			/// Returns true if the character is a hexadecimal (xdigit := \\p{gc=Decimal_Number} | \\p{Hex_Digit}).
			inline bool isxdigit(CodePoint cp) {
				return GeneralCategory::of(cp) == GeneralCategory::NUMBER_DECIMAL_DIGIT || BinaryProperty::is<BinaryProperty::HEX_DIGIT>(cp);}
		} // namespace legacyctype


// inline implementations ///////////////////////////////////////////////////

/// Returns the current character in the normalized text.
inline CodePoint Normalizer::operator*() const throw() {return normalizedBuffer_[indexInBuffer_];}

/// Prefix incremental operator.
inline Normalizer& Normalizer::operator++() {
	if(isLast())
		throw std::out_of_range("the iterator is the last.");
	if(++indexInBuffer_ == normalizedBuffer_.length()) {
		if(!(++current_).isLast())
			normalizeCurrentBlock(FORWARD);
	}
	return *this;
}

/// Postfix increment opearator.
inline const Normalizer Normalizer::operator++(int) {Normalizer temp(*this); ++*this; return temp;}

/// Prefix decrement operator.
inline Normalizer& Normalizer::operator--() {
	if(isFirst())
		throw std::out_of_range("the iterator is the first");
	if(indexInBuffer_ == 0) {
		--current_;
		normalizeCurrentBlock(BACKWARD);
	} else
		--indexInBuffer_;
	return *this;
}

/// Postfix decrement opearator.
inline const Normalizer Normalizer::operator--(int) {Normalizer temp(*this); --*this; return temp;}

/// Equality operator returns true if both iterators address the same character in the normalized text.
inline bool Normalizer::operator==(const Normalizer& rhs) const throw() {
	return current_ == rhs.current_ && indexInBuffer_ == rhs.indexInBuffer_;}

/// Inequality operator.
inline bool Normalizer::operator!=(const Normalizer& rhs) const throw() {return !(*this == rhs);}

/**
 * Compares the two strings according to the specified decomposition mapping.
 * @param s1 the string
 * @param s2 the other string
 * @param type decomposition mapping type
 * @param caseFolding the options for case folding
 * @retval &lt;0 @a s1 is less than @a s2
 * @retval 0 the two strings are canonical or compatibiilty equivalent
 * @retval &gt;0 @a s1 is greater than @a s2
 */
inline int Normalizer::compare(const String& s1, const String& s2, Type type, const CaseFoldings& caseFolding) {
	const String ds1(normalize(s1.data(), s1.data() + s1.length(), (type == CANONICAL) ? FORM_D : FORM_KD));
	const String ds2(normalize(s2.data(), s2.data() + s2.length(), (type == CANONICAL) ? FORM_D : FORM_KD));
	return s1.compare(s2);
}

/// Returns true if the iterator addresses the start of the normalized text.
inline bool Normalizer::isFirst() const throw() {return current_.isFirst() && indexInBuffer_ == 0;}

/// Returns true if the iterator addresses the end of the normalized text.
inline bool Normalizer::isLast() const throw() {return current_.isLast();}

/// Returns the current position in the input text that is being normalized.
inline const Char* Normalizer::tell() const throw() {return current_.tell();}

/**
 * Checks whether the specified character sequence starts with an identifier.
 * The type @a CharacterSequence the bidirectional iterator expresses a UTF-16 character sequence.
 * @param first the start of the character sequence
 * @param last the end of the character sequence
 * @return the end of the detected identifier or @a first if an identifier not found
 */
template<class CharacterSequence>
inline CharacterSequence IdentifierSyntax::eatIdentifier(CharacterSequence first, CharacterSequence last) const throw() {
	UTF16To32Iterator<CharacterSequence, utf16boundary::USE_BOUNDARY_ITERATORS> i(first, first, last);
	if(!isIdentifierStartCharacter(*i))
		return first;
	while(!i.isLast() && isIdentifierContinueCharacter(*i))
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
template<class CharacterSequence>
inline CharacterSequence IdentifierSyntax::eatWhiteSpaces(CharacterSequence first, CharacterSequence last, bool includeTab) const throw() {
	UTF16To32Iterator<CharacterSequence, utf16boundary::USE_BOUNDARY_ITERATORS> i(first, first, last);
	while(!i.isLast() && isWhiteSpace(*i, includeTab))
		++i;
	return i.tell();
}

#define IMPLEMENT_FORNAME																				\
	if(name == 0) throw std::invalid_argument("the name is null.");										\
	else if(names_.empty()) buildNames();																\
	const std::map<const Char*, int, PropertyNameComparer<Char> >::const_iterator i(names_.find(name));	\
	return (i != names_.end()) ? i->second : NOT_PROPERTY;

/**
 * Compares the given two strings.
 * @param p1 one property name
 * @param p2 the other property name
 * @return true if p1 &lt; p2
 */
template<typename CharType>
inline bool PropertyNameComparer<CharType>::operator()(const CharType* p1, const CharType* p2) const {return compare(p1, p2) < 0;}

/**
 * Compares the given two strings.
 * @param p1 one property name
 * @param p2 the other property name
 * @return &lt; 0 if @a p1 &lt; @a p2
 * @return 0 if @a p1 == @a p2
 * @return &gt; 0 if @a p1 &gt; @a p2
 */
template<typename CharType>
inline int PropertyNameComparer<CharType>::compare(const CharType* p1, const CharType* p2) {
	while(*p1 != 0 && *p2 != 0) {
		if(*p1 == '_' || *p1 == '-' || *p1 == ' ') {
			++p1; continue;
		} else if(*p2 == '_' || *p2 == '-' || *p2 == ' ') {
			++p2; continue;
		}
		const int c1 = std::tolower(*p1, std::locale::classic()), c2 = std::tolower(*p2, std::locale::classic());
		if(c1 != c2)	return c1 - c2;
		else			++p1, ++p2;
	}
	return *p1 - *p2;
}

/// Returns the General_Category with the given name.
inline int GeneralCategory::forName(const Char* name) {IMPLEMENT_FORNAME}

/// Returns General_Category value of the specified character.
inline int GeneralCategory::of(CodePoint cp) throw() {
	if(const internal::PropertyRange* p = internal::findInRange(ranges_, ranges_ + count_, cp))
		return p->property;
	return OTHER_UNASSIGNED;
}

/// Returns the Block with the given name.
inline int CodeBlock::forName(const Char* name) {IMPLEMENT_FORNAME}

/// Returns Block value of the specified character.
inline int CodeBlock::of(CodePoint cp) throw() {
	if(const internal::PropertyRange* p = internal::findInRange(ranges_, ranges_ + count_, cp))
		return p->property;
	return NO_BLOCK;
}

#ifndef ASCENSION_NO_UNICODE_NORMALIZATION
/// Returns the Canonical_Combining_Class with the given name.
inline int CanonicalCombiningClass::forName(const Char* name) {IMPLEMENT_FORNAME}

/// Returns the Canonical_Combining_Class of the specified character.
inline int CanonicalCombiningClass::of(CodePoint cp) throw() {
	if(cp < 0x10000) {
		const Char* const p = std::lower_bound(SRC_UCS2, SRC_UCS2 + UCS2_COUNT, static_cast<Char>(cp & 0xFFFFU));
		return (*p == cp) ? DEST_UCS2[p - SRC_UCS2] : NOT_REORDERED;
	} else {
		const CodePoint* const p = std::lower_bound(SRC_UCS4, SRC_UCS4 + UCS4_COUNT, cp);
		return (*p != cp) ? DEST_UCS4[p - SRC_UCS4] : NOT_REORDERED;
	}
}
#endif /* !ASCENSION_NO_UNICODE_NORMALIZATION */

/// Returns the Script with the given name.
inline int Script::forName(const Char* name) {IMPLEMENT_FORNAME}

/// Returns Script value of the specified character.
inline int Script::of(CodePoint cp) throw() {
	if(const internal::PropertyRange* p = internal::findInRange(ranges_, ranges_ + count_, cp))
		return p->property;
	return UNKNOWN;
}

/// Returns the Hangul_Syllable_Type with the given name.
inline int HangulSyllableType::forName(const Char* name) {IMPLEMENT_FORNAME}

/// Returns the Hangul syllable type property value of @a cp.
inline int HangulSyllableType::of(CodePoint cp) throw() {
	if(cp >= 0x1100 && cp <= 0x1159 || cp == 0x115F)
		return LEADING_JAMO;
	else if(cp >= 0x1160 && cp <= 0x11A2)
		return VOWEL_JAMO;
	else if(cp >= 0x11A8 && cp <= 0x11F9)
		return TRAILING_JAMO;
	else if(cp >= 0xAC00 && cp <= 0xD7A3)
		return ((cp - 0xAC00) % 28 == 0) ? LV_SYLLABLE : LVT_SYLLABLE;
	else
		return NOT_APPLICABLE;
}

template<> inline bool CaseFolder::compare<CASEFOLDING_NONE>(const Char* p1, const Char* p2, length_t length) {
	using namespace std;
	return wmemcmp(p1, p2, length) == 0;
}

template<> inline bool CaseFolder::compare<CASEFOLDING_ASCII>(const Char* p1, const Char* p2, length_t length) {
	for(length_t i = 0; i < length; ++i) {
		if(foldASCII(p1[i]) != foldASCII(p2[i]))
			return false;
	}
	return true;
}

template<> inline bool CaseFolder::compare<CASEFOLDING_UNICODE_SIMPLE>(const Char* p1, const Char* p2, length_t length) {
	// this does not support UCS-4
	for(length_t i = 0; i < length; ++i) {
		if(foldSimple(p1[i]) != foldSimple(p2[i]))
			return false;
	}
	return true;
}

/**
 * Performs ASCII case folding.
 * @param ch the source character
 * @return the folded character
 */
inline Char CaseFolder::foldASCII(Char ch) throw() {return (ch >= L'A' && ch <= L'Z') ? ch + L'a' - L'A' : ch;}

/**
 * Performs full case folding.
 * @param first the start of the source text
 * @param last the end of the source text
 * @return the folded string
 * @throw std#invalid_argument any parameter is @c null
 */
inline manah::AutoBuffer<Char> CaseFolder::foldFull(const Char* first, const Char* last) {
	if(first == 0 || last == 0)
		throw std::invalid_argument("any parameter is null.");
	manah::AutoBuffer<Char> result(new Char[static_cast<std::size_t>(last - first) * CASE_FOLDING_EXPANSION_MAX_CHARS]);
	foldFull(first, last, result.get());
	return result;
}

/**
 * Performs full case folding.
 * @param first the start of the source text
 * @param last the end of the source text
 * @param[out] dest the destination buffer. this buffer must have the size of greater than or equals to (@a last - @a first) * 3
 * @throw std#invalid_argument any parameter is @c null
 */
inline length_t CaseFolder::foldFull(const Char* first, const Char* last, Char* dest) {
	if(first == 0 || last == 0 || dest == 0)
		throw std::invalid_argument("any parameter is null.");
	CodePoint folded[CASE_FOLDING_EXPANSION_MAX_CHARS];
	length_t written = 0;
	while(first < last) {
		const CodePoint cp = surrogates::decode(first, last - first);
		const length_t c = foldFull(cp, folded);
		for(length_t j = 0; j < c; ++j)
			written += surrogates::encode(folded[j], dest + written);
		first += (cp < 0x010000) ? 1 : 2;
	}
	return written;
}

/**
 * Performs simple case folding.
 * @param first the start of the source text
 * @param last the end of the source text
 * @return the folded string
 * @throw std#invalid_argument any parameter is @c null
 */
inline manah::AutoBuffer<Char> CaseFolder::foldSimple(const Char* first, const Char* last) {
	if(first == 0 || last == 0)
		throw std::invalid_argument("any parameter is null.");
	manah::AutoBuffer<Char> result(new Char[static_cast<std::size_t>(last - first)]);
	foldSimple(first, last, result.get());
	return result;
}

/**
 * Performs simple case folding.
 * @param first the start of the source text
 * @param last the end of the source text
 * @param[out] dest the destination buffer. this buffer must have the size of greater than or equals to @a last - @a first
 * @throw std#invalid_argument any parameter is @c null
 */
inline void CaseFolder::foldSimple(const Char* first, const Char* last, Char* dest) {
	if(first == 0 || last == 0 || dest == 0)
		throw std::invalid_argument("any parameter is null.");
	while(first < last) {
		const CodePoint cp = surrogates::decode(first, last - first);
		dest += surrogates::encode(foldSimple(cp), dest);
		first += (cp < 0x010000) ? 1 : 2;
	}
}

}} // namespace ascension::unicode

#undef CASE_FOLDING_EXPANSION_MAX_CHARS
#undef IMPLEMENT_FORNAME

#endif /* !ASCENSION_UNICODE_UTILS_HPP */
