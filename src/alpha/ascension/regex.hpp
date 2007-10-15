/**
 * @file regex.hpp
 * Defines wrappers of Boost.Regex library or std#tr1#regex.
 * @author exeal
 * @date 2006-2007
 */

#ifndef ASCENSION_NO_REGEX
#ifndef ASCENSION_REGEX_HPP
#define ASCENSION_REGEX_HPP
#include "unicode.hpp"	// unicode.UTF16To32Iterator
#include "unicode-property.hpp"
#include "internal.hpp"	// internal.Int2Type
#include <memory>
#include <map>
#include <bitset>
#include <boost/regex.hpp>

namespace ascension {

	/**
	 * Classes for matching character sequences against patterns specified by regular expressions.
	 *
	 * An instance of the @c Pattern class represents a regular expression that is specified in
	 * string form in a syntax similar to that used by Perl.
	 *
	 * Instances of the @c Matcher class are used to match character sequences against a given
	 * pattern. Input is provided to matchers via the C++ standard bidirectional iterator in order
	 * to support matching against characters from a wide variety of input sources.
	 *
	 * Classes provided by this namespace have interfaces familiar to Java/ICU regex users.
	 * @a CodePointIterator template parameters of class @c MatchResult and @c Matcher must
	 * represent a UTF-32 code point sequence.
	 *
	 * This uses Boost.Regex to implement internally.
	 */
	namespace regex {

		/**
		 * The result of a match operation.
		 * <p>This interface contains query methods used to determine the results of a match
		 * against a regular expression. The match boundaries, groups and group boundaries can be
		 * seen but not modified through a @c MatchResult.</p>
		 * <p>Almost all methods throw @c IllegalStateException if no match has yet been attempted,
		 * or if the previous match operation failed.</p>
		 * @see Matcher
		 */
		template<typename CodePointIterator> class MatchResult {
		public:
			/// Destructor.
			virtual ~MatchResult() {}
			/// Returns the position after the last character matched.
			virtual const CodePointIterator& end() const = 0;
			/// Returns the position after the last character of the subsequence captured by
			/// @a group during this match.
			virtual const CodePointIterator& end(int group) const = 0;
			/// Returns the input subsequence matched by the previous match.
			virtual String group() const = 0;
			/// Returns the input subsequence captured by @a group during the previous match
			/// operation.
			virtual String group(int group) const = 0;
			/// Returns the number of the capturing groups this match result's pattern.
			virtual std::size_t groupCount() const = 0;
			/// Returns the start position of the match.
			virtual const CodePointIterator& start() const = 0;
			/// Returns the start position of the subsequence captured by @a group during this
			/// match.
			virtual const CodePointIterator& start(int group = 0) const = 0;
		private:
			ASCENSION_STATIC_ASSERT(unicode::CodeUnitSizeOf<CodePointIterator>::result == 4);
		};

		namespace internal {
			/// @internal
			template<typename CodePointIterator>
			class MatchResultImpl : public MatchResult<CodePointIterator> {
			public:
				MatchResultImpl() {}
				explicit MatchResultImpl(const boost::match_results<CodePointIterator>& src) : impl_(src) {}
				const CodePointIterator& end() const {return end(0);}
				const CodePointIterator& end(int group) const {return get(group).second;}
				String group() const {return group(0);}
				String group(int group) const;
				std::size_t groupCount() const {return impl_.size();}
				const CodePointIterator& start() const {return start(0);}
				const CodePointIterator& start(int group) const {return get(group).first;}
			protected:
				boost::match_results<CodePointIterator>& impl() throw() {return impl_;}
				const boost::match_results<CodePointIterator>& impl() const throw() {return impl_;}
			private:
				const boost::sub_match<CodePointIterator>& get(int group) const {
					const boost::sub_match<CodePointIterator>& s = impl_[group];
					if(group != 0 && !impl_[0].matched) throw IllegalStateException("the previous was not performed or failed.");
					if(group != 0 && !s.matched) throw IndexOutOfBoundsException("the specified sub match group is not exist.");
					return s;
				}
				boost::match_results<CodePointIterator> impl_;
			};

			/**
			 * Unicode property enabled @c regex_traits for @c boost#basic_regex template class.
			 * This traits class does not implement "additional optional requirements"
			 * (http://www.boost.org/libs/regex/doc/concepts.html#traits).
			 * @note This class is not intended to be subclassed.
			 */
			class RegexTraits {
			private:
				enum {
					// POSIX compatible not in Unicode property
					POSIX_ALNUM = unicode::ucd::SentenceBreak::LAST_VALUE,
					POSIX_BLANK, POSIX_GRAPH, POSIX_PRINT, POSIX_PUNCT, POSIX_WORD, POSIX_XDIGIT,
					// regex specific general category
					GC_ANY, GC_ASSIGNED, GC_ASCII,
					CLASS_END
				};
			public:
				// original interface
				RegexTraits() : collator_(&std::use_facet<std::collate<char_type> >(locale_)) {}
				static bool	unixLineMode, usesExtendedProperties;
				// minimal requirements for traits
				typedef CodePoint char_type;
				typedef std::size_t size_type;
				typedef std::basic_string<char_type> string_type;
				typedef std::locale locale_type;
				typedef std::bitset<CLASS_END> char_class_type;
				static size_type length(const char_type* p) {size_type i = 0; while(p[i] != 0) ++i; return i;}
				char_type translate(char_type c) const;
				char_type translate_nocase(char_type c) const {return unicode::CaseFolder::fold(translate(c));}
				string_type transform(const char_type* p1, const char_type* p2) const {return collator_->transform(p1, p2);}
				string_type transform_primary(const char_type* p1, const char_type* p2) const {return transform(p1, p2);}
				char_class_type lookup_classname(const char_type* p1, const char_type* p2) const;
				string_type lookup_collatename(const char_type* p1, const char_type* p2) const {return transform(p1, p2);}
				bool isctype(char_type c, const char_class_type& f) const;
				int value(char_type c, int radix) const;
				locale_type imbue(locale_type l) {locale_type temp = locale_; collator_ = &std::use_facet<std::collate<char_type> >(locale_ = l); return temp;}
				locale_type getloc() const {return locale_;}
				std::string error_string(boost::regex_constants::error_type) const {return "Unknown error";}
			private:
				static std::size_t findPropertyValue(const String& expression);
			private:
				locale_type locale_;
				const std::collate<char_type>* collator_;
				static std::map<const Char*, int, unicode::ucd::PropertyNameComparer<Char> > names_;
				static void buildNames();
			};
		} // namespace internal

		class Pattern;

		template<typename CodePointIterator>
		class Matcher : virtual public regex::internal::MatchResultImpl<CodePointIterator> {
		public:
			// attributes
			bool						hasAnchoringBounds() const throw();
			bool						hasTransparentBounds() const throw();
			const Pattern&				pattern() const throw();
			Matcher&					region(CodePointIterator start, CodePointIterator end);
			const CodePointIterator&	regionEnd() const throw();
			const CodePointIterator&	regionStart() const throw();
			Matcher&					useAnchoringBounds(bool b) throw();
			Matcher&					usePattern(const Pattern& newPattern);
			Matcher&					useTransparentBounds(bool b) throw();
			// search
			bool	find();
			bool	find(CodePointIterator start);
			bool	lookingAt();
			bool	matches();
			// replacement
			template<typename OutputIterator>
			Matcher&		appendReplacement(OutputIterator out, const String& replacement);
			template<typename OutputIterator>
			OutputIterator	appendTail(OutputIterator out) const;
			String			replaceAll(const String& replacement);
			String			replaceFirst(const String& replacement);
			// in-place replacement
			Matcher&	endInplaceReplacement(CodePointIterator first, CodePointIterator last,
							CodePointIterator regionFirst, CodePointIterator regionLast, CodePointIterator next);
			String		replaceInplace(const String& replacement);
			// explicit reset
			Matcher&	reset();
			Matcher&	reset(CodePointIterator first, CodePointIterator last);
			// result
			std::auto_ptr<MatchResult<CodePointIterator> >	toMatchResult() const;
		private:
			Matcher(const Pattern& pattern, CodePointIterator first, CodePointIterator last);
			template<typename OI> void appendReplacement(OI out, const String& replacement, const ascension::internal::Int2Type<2>&);
			template<typename OI> void appendReplacement(OI out, const String& replacement, const ascension::internal::Int2Type<4>&);
			template<typename OI> OI appendTail(OI out, const ascension::internal::Int2Type<2>&) const;
			template<typename OI> OI appendTail(OI out, const ascension::internal::Int2Type<4>&) const;
			bool acceptResult() {const bool b(impl()[0].matched); matchedZeroWidth_ = b && impl().length() == 0; if(b) current_ = end(); return b;}
			void checkInplaceReplacement() const {if(replaced_) throw IllegalStateException("the matcher entered to in-place replacement.");}
			void checkPreviousMatch() const {if(!impl()[0].matched) throw IllegalStateException("the previous was not performed or failed.");}
			boost::match_flag_type getNativeFlags(const CodePointIterator& first, const CodePointIterator& last, bool continuous) const throw();
			const Pattern* pattern_;
			CodePointIterator current_;
			std::pair<CodePointIterator, CodePointIterator> input_, region_;
			CodePointIterator appendingPosition_;
			bool replaced_;	// true between replaceInplace() and endInplaceReplacement()
			bool matchedZeroWidth_;	// true if the previous performance matched a zero width subsequence
			bool usesAnchoringBounds_, usesTransparentBounds_;
			friend class Pattern;
		};

		/// Unchecked exception thrown to indicate a syntax error in a regular-expression pattern.
		class PatternSyntaxException : public std::invalid_argument {
		public:
			/// Error types (corresponds to @c boost#regex_constants#error_type).
			enum Code {
				NOT_ERROR,						///< Not an error.
				INVALID_COLLATION_CHARACTER,	///< An invalid collating element was specified in a [[.name.]] block.
				INVALID_CHARACTER_CLASS_NAME,	///< An invalid character class name was specified in a [[:name:]] block.
				TRAILING_BACKSLASH,				///< An invalid or trailing escape was encountered.
				INVALID_BACK_REFERENCE,			///< A back-reference to a non-existant marked sub-expression was encountered.
				UNMATCHED_BARCKET,				///< An invalid character set [...] was encounted.
				UNMATCHED_PAREN,				///< Mismatched '(' abd ')'.
				UNMATCHED_BRACE,				///< Mismatched '{' and '}'.
				INVALID_CONTENT_OF_BRACES,		///< Invalid contents of a {...} block.
				INVALID_RANGE_END,				///< A character range was invalid, for example [d-a].
				MEMORY_EXHAUSTED,				///< Out of memory.
				INVALID_REPEATITION,			///< An attempt to repeat something that can not be repeated - for example a*+.
				TOO_COMPLEX_REGULAR_EXPRESSION,	///< The expression became too complex to handle.
				STACK_OVERFLOW,					///< Out of program stack space.
				UNKNOWN_ERROR					///< Other unspecified errors.
			};
			/// Constructor.
			PatternSyntaxException(const boost::regex_error& src, const String& pattern);
			/// Retrieves the error code.
			Code getCode() const;
			/// Retrieves the description of the error.
			std::string getDescription() const;
			/// Retrieves the error index.
			std::ptrdiff_t getIndex() const {return impl_.position();}
			/// Retrieves the erroneous regular-expression pattern.
			String getPattern() const throw() {return pattern_;}
		private:
			const boost::regex_error impl_;
			const String pattern_;
		};

		class Pattern {
		public:
			enum {
				UNIX_LINES = 0x01,			///< Enables Unix lines mode (not implemented).
				CASE_INSENSITIVE = 0x02,	///< Enables case-insensitive matching.
				COMMENTS = 0x04,			///< Permits whitespace and comments in pattern.
				MULTILINE = 0x08,			///< Enables multiline mode.
				LITERAL = 0x10,				///< Enables literal parsing of the pattern.
				DOTALL = 0x20,				///< Enables dotall mode.
				UNICODE_CASE = 0x40,		///< Enables Unicode-aware case folding (not implemented).
				CANON_EQ = 0x80				///< Enables canonical equivalence (not implemented).
			};
		public:
			virtual ~Pattern() throw();
			// attributes
			int		flags() const throw();
			String	pattern() const;
			// compilation
			static std::auto_ptr<const Pattern>			compile(const String& regex, int flags = 0);
			template<typename CodePointIterator>
			std::auto_ptr<Matcher<CodePointIterator> >	matcher(CodePointIterator first, CodePointIterator last) const;
			// tools
			static bool	matches(const String& regex, const String& input);
			template<typename CodePointIterator>
			static bool	matches(const String& regex, CodePointIterator first, CodePointIterator last);
//			template<typename CodePointIterator, typename OutputIterator>
//			std::size_t	split(CodePointIteratorfirst, CodePointIteratorlast, OutputIterator out, std::size_t limit = -1);
		protected:
			Pattern(const Char* first, const Char* last, boost::regex_constants::syntax_option_type nativeSyntax);
		private:
			Pattern(const String& regex, int flags);
			boost::basic_regex<CodePoint, regex::internal::RegexTraits> impl_;
			const int flags_;
			template<typename CodePointIterator> friend class Matcher;
		};

#ifndef ASCENSION_NO_MIGEMO
		/// Builds regular expression pattern for Migemo use.
		class MigemoPattern : public Pattern {
		public:
			static std::auto_ptr<MigemoPattern>	compile(const Char* first, const Char* last, bool ignoreCase);
			static void							initialize(const char* runtimePathName, const char* dictionaryPathName);
			static bool							isMigemoInstalled() throw();
		private:
			MigemoPattern(const Char* first, const Char* last, bool ignoreCase);
			static void	install();
			static manah::AutoBuffer<char> runtimePathName_, dictionaryPathName_;
		};
#endif /* !ASCENSION_NO_MIGEMO */


		// internal.RegexTraits /////////////////////////////////////////////

		inline internal::RegexTraits::char_type internal::RegexTraits::translate(char_type c) const {
			if(unixLineMode) return (c == LINE_FEED) ? LINE_SEPARATOR : c;
			return (c < 0x10000 && std::binary_search(LINE_BREAK_CHARACTERS,
				endof(LINE_BREAK_CHARACTERS), static_cast<Char>(c & 0xFFFF))) ? LINE_SEPARATOR : c;
		}

		inline int internal::RegexTraits::value(char_type c, int radix) const {
			switch(radix) {
			case 8:
				return (c >= '0' && c <= '7') ? static_cast<int>(c - '0') : -1;
			case 10:
				return (c >= '0' && c <= '9') ? static_cast<int>(c - '0') : -1;
			case 16:
				if(c >= '0' && c <= '9')		return c - '0';
				else if(c >= 'A' && c <= 'F')	return c - 'A' + 10;
				else if(c >= 'a' && c <= 'f')	return c - 'a' + 10;
			default:
				return -1;
			}
		}

		// internal.MatchResultImpl /////////////////////////////////////////

		template<typename CPI> inline String regex::internal::MatchResultImpl<CPI>::group(int group) const {
			const std::basic_string<CodePoint> s(get(group).str());
			return String(unicode::UTF32To16Iterator<>(s.data()), unicode::UTF32To16Iterator<>(s.data() + s.length()));}


		// Matcher //////////////////////////////////////////////////////////

		/// Private constructor.
		template<typename CPI> inline Matcher<CPI>::Matcher(const Pattern& pattern, CPI first, CPI last) :
			pattern_(&pattern), current_(first), input_(first, last), region_(first, last), appendingPosition_(input_.first),
			replaced_(false), matchedZeroWidth_(false), usesAnchoringBounds_(true), usesTransparentBounds_(false) {}

		template<typename CPI> template<typename OI>
		inline void Matcher<CPI>::appendReplacement(OI out, const String& replacement, const ascension::internal::Int2Type<2>&) {
			typedef boost::match_results<CPI>::string_type String32;
			if(appendingPosition_ != input_.second) std::copy(appendingPosition_, impl()[0].first, out);
			const String32 replaced(impl().format(String32(
				unicode::UTF16To32Iterator<String::const_iterator>(replacement.begin(), replacement.end()),
				unicode::UTF16To32Iterator<String::const_iterator>(replacement.begin(), replacement.end(), replacement.end()))));
			std::copy(unicode::UTF32To16Iterator<String32::const_iterator>(replaced.begin()),
				unicode::UTF32To16Iterator<String32::const_iterator>(replaced.end()), out);}

		template<typename CPI> template<typename OI>
		inline void Matcher<CPI>::appendReplacement(OI out, const String& replacement, const ascension::internal::Int2Type<4>&) {
			if(appendingPosition_ != input_.second) std::copy(appendingPosition_, impl()[0].first, out);
			const String& replaced(impl().format(replacement)); std::copy(replaced.begin(), replaced.end(), out);}

		/// Implements a non-terminal append-and-replace step.
		template<typename CPI> template<typename OI>
		inline Matcher<CPI>& Matcher<CPI>::appendReplacement(OI out, const String& replacement) {
			checkInplaceReplacement(); checkPreviousMatch();
			appendReplacement(out, replacement, ascension::internal::Int2Type<unicode::CodeUnitSizeOf<OI>::result>());
			appendingPosition_ = impl()[0].second; return *this;}
		
		template<typename CPI> template<typename OI> inline OI Matcher<CPI>::appendTail(OI out,
			const ascension::internal::Int2Type<2>&) const {return std::copy(unicode::UTF32To16Iterator<CPI>(appendingPosition_), unicode::UTF32To16Iterator<CPI>(input_.second), out);}

		template<typename CPI> template<typename OI> inline OI Matcher<CPI>::appendTail(OI out,
			const ascension::internal::Int2Type<4>&) const {return std::copy(appendingPosition_, input_.second, out);}

		/// Implements a terminal append-and-replace step.
		template<typename CPI> template<typename OI> inline OI Matcher<CPI>::appendTail(OI out) const {
			checkInplaceReplacement(); return appendTail(out, ascension::internal::Int2Type<unicode::CodeUnitSizeOf<OI>::result>());}

		/// Ends the active in-place replacement context.
		template<typename CPI> inline Matcher<CPI>& Matcher<CPI>::endInplaceReplacement(CPI first, CPI last, CPI regionFirst, CPI regionLast, CPI next) {
			if(!replaced_) throw IllegalStateException("the matcher is not entered in in-place replacement context.");
			const bool matchedZW = matchedZeroWidth_; reset(first, last);
			region_.first = regionFirst; region_.second = regionLast; current_ = next; matchedZeroWidth_ = matchedZW; return *this;}

		/// Attempts to find the next subsequence of the input sequence that matches the pattern.
		/// This method starts at the beginning of the matcher's region, or, if a previous
		/// invocation of the method was successful and the matcher has not since been reset, at the
		/// first character not matched by the previous match.
		template<typename CPI> inline bool Matcher<CPI>::find() {checkInplaceReplacement(); boost::regex_search(
			current_, region_.second, impl(), pattern_->impl_, getNativeFlags(current_, region_.second, true)); return acceptResult();}

		/// Resets the matcher and then attempts to find the next subsequence of the input sequence
		/// that matches the pattern, starting at the specified position.
		template<typename CPI> inline bool Matcher<CPI>::find(CPI start) {reset(); boost::regex_search(
			start, input_.second, impl(), pattern_.impl_, getNativeFlags(start, input_.second, true)); return acceptResult();}

		template<typename CPI> inline boost::match_flag_type
		Matcher<CPI>::getNativeFlags(const CPI& first, const CPI& last, bool continuous) const throw() {
			boost::match_flag_type f(boost::regex_constants::match_default);
			if((pattern_->flags() & Pattern::DOTALL) == 0) f |= boost::regex_constants::match_not_dot_newline;
			if((pattern_->flags() & Pattern::MULTILINE) == 0) f |= boost::regex_constants::match_single_line;
			if(continuous && matchedZeroWidth_) f |= boost::regex_constants::match_not_initial_null;
			if(!usesAnchoringBounds_) {
				if(first != input_.first) f |= boost::regex_constants::match_not_bob | boost::regex_constants::match_not_bol;
				if(last != input_.second) f |= boost::regex_constants::match_not_eol | boost::regex_constants::match_not_eol;
			}
			if(usesTransparentBounds_ && first != input_.first) f |= boost::regex_constants::match_prev_avail;
			return f;
		}

		/// Queries the anchoring of region bounds for the matcher.
		/// @return true the matcher uses <em>anchoring</em> bounds. false otherwise
		template<typename CPI> inline bool Matcher<CPI>::hasAnchoringBounds() const throw() {return usesAnchoringBounds_;}

		/// Queries the transparency of the region bounds for the matcher.
		/// @retval true the matcher uses <em>transparent</em> bounds
		/// @retval false the matcher uses <em>opaque</em> bounds
		template<typename CPI> inline bool Matcher<CPI>::hasTransparentBounds() const throw() {return usesTransparentBounds_;}

		/// Attempts to match the input sequence, starting at the beginning of the region, against
		/// the pattern. Like the @c matches method, this method always starts at the beginning of
		/// the region; unlike that method, it does not require that the entire region be matched.
		template<typename CPI> inline bool Matcher<CPI>::lookingAt() {
			boost::regex_search(region_.first, region_.second, impl(), pattern_->impl_, getNativeFlags(
				region_.first, region_.second, false) | boost::regex_constants::match_continuous); return acceptResult();}

		/// Attempts to match the entire region against the pattern.
		template<typename CPI> inline bool Matcher<CPI>::matches() {
			boost::regex_match(region_.first, region_.second, impl(), pattern_->impl_,
				getNativeFlags(region_.first, region_.second, false)); return acceptResult();}

		/// Returns the pattern interpreted by the matcher.
		template<typename CPI> inline const Pattern& Matcher<CPI>::pattern() const throw() {return *pattern_;}

		/// Sets the limits of the matcher's region. Invoking this method resets the matcher.
		template<typename CPI> inline Matcher<CPI>& Matcher<CPI>::region(
			CPI start, CPI end) {reset(); current_ = region_.first = start; region_.second = end; return *this;}

		/// Reports the end of the matcher's region.
		template<typename CPI> inline const CPI& Matcher<CPI>::regionEnd() const throw() {return region_.second;}

		/// Reports the beginning of the matcher's region.
		template<typename CPI> inline const CPI& Matcher<CPI>::regionStart() const throw() {return region_.first;}

		/// ...
		template<typename CPI> inline String Matcher<CPI>::replaceInplace(const String& replacement) {
			if(!impl()[0].matched) throw IllegalStateException("the previous was failed or not performed.");
			else if(replaced_) throw IllegalStateException("this matcher already entered in in-place replacement.");
			const std::basic_string<CodePoint> temp(impl().format(std::basic_string<CodePoint>(
				unicode::UTF16To32Iterator<String::const_iterator>(replacement.begin(), replacement.end()),
				unicode::UTF16To32Iterator<String::const_iterator>(replacement.begin(), replacement.end(), replacement.end()))));
			replaced_ = true;
			return String(unicode::UTF32To16Iterator<>(temp.data()), unicode::UTF32To16Iterator<>(temp.data() + temp.length()));}

		/// Replaces every subsequence of the input sequence that matches the pattern with the given
		/// replacement string. This method first resets the matcher.
		template<typename CPI> inline String Matcher<CPI>::replaceAll(const String& replacement) {reset(); OutputStringStream s;
			std::ostream_iterator<Char, Char> os(s); while(find()) appendReplacement(os, replacement); appendTail(os); return s.str();}

		/// Replaces the first subsequence of the input sequence that matches the pattern with the
		/// given replacement string. This method first resets the matcher.
		template<typename CPI> inline String Matcher<CPI>::replaceFirst(const String& replacement) {reset(); OutputStringStream s;
			std::ostream_iterator<Char, Char> os(s); if(find()) appendReplacement(os, replacement); appendTail(os); return s.str();}

		/// Resets the matcher. Resetting a matcher discards all of its explicit state information
		/// and sets its append position to the beginning of the input. The matcher's region is set
		/// to the default region, which is its entire character sequence. The anchoring and
		/// transparency of the matcher's region boundaries are unaffected. 
		template<typename CPI> inline Matcher<CPI>& Matcher<CPI>::reset() {impl() = boost::match_results<CPI>();
			region_ = input_; current_ = appendingPosition_ = input_.first; replaced_ = false; return *this;}

		/// Resets the matcher with a new input sequence.
		template<typename CPI> inline Matcher<CPI>& Matcher<CPI>::reset(
			CPI first, CPI last) {input_.first = first; input_.second; return reset();}

		/// Returns the match state of the matcher as a @c MatchResult.
		/// This result is unaffected by subsequent operations performed upon the matcher.
		template<typename CPI> inline std::auto_ptr<MatchResult<CPI> > Matcher<CPI>::toMatchResult() const {
			return std::auto_ptr<MatchResult<CPI> >(new internal::MatchResultImpl<CPI>(impl()));}

		/// Sets the anchoring of region bounds for the matcher.
		/// @param b true to use <em>anchoring</em> bounds
		template<typename CPI> inline Matcher<CPI>& Matcher<CPI>::useAnchoringBounds(bool b) throw() {
			usesAnchoringBounds_ = b; return *this;}

		/// Changes the pattern the matcher uses to find matches with. This method causes the
		/// matcher to lose information about the groups of the last match that occured. The
		/// matcher's position in the input is maintained and its last append position is unaffected.
		template<typename CPI> inline Matcher<CPI>& Matcher<CPI>::usePattern(
			const Pattern& newPattern) {pattern_ = &pattern; impl().reset(); return *this;}

		/// Sets the transparency of region bounds for the matcher.
		/// @param b true to use <em>transparent</em> bounds
		template<typename CPI> inline Matcher<CPI>& Matcher<CPI>::useTransparentBounds(bool b) throw() {
			usesTransparentBounds_ = b; return *this;}

		// Pattern //////////////////////////////////////////////////////////

		/**
		 * Compiles the given regular expression into a pattern with the given flags.
		 * @param regex the expression to be compiled
		 * @param flags match flags
		 * @throw std#invalid_argument bit values other than those corresponding to the defined match flags are set in @a flags
		 * @throw PatternSyntaxException the expression's syntax is invalid
		 */
		inline std::auto_ptr<const Pattern> Pattern::compile(
			const String& regex, int flags /* = 0 */) {return std::auto_ptr<const Pattern>(new Pattern(regex, flags));}

		/// Returns this pattern's match flags.
		inline int Pattern::flags() const throw() {return flags_;}

		/// Creates a matcher that will match the given input against this pattern.
		template<typename CPI> inline std::auto_ptr<Matcher<CPI> > Pattern::matcher(
			CPI first, CPI last) const {return std::auto_ptr<Matcher<CPI> >(new Matcher<CPI>(*this, first, last));}

		/**
		 * Compiles the given regular expression and attempts to match the given input against it.
		 * @param regex the expression to be compiled
		 * @param first the character sequence to be matched
		 * @param last the end of @a first
		 * @throw PatternSyntaxException the expression's syntax is invalid
		 */
		template<typename CPI> inline bool Pattern::matches(
			const String& regex, CPI first, CPI last) {return compile(regex)->matcher(first, last)->matches();}

		/**
		 * Compiles the given regular expression and attempts to match the given input against it.
		 * @param regex the expression to be compiled
		 * @param input the character sequence to be matched
		 * @throw PatternSyntaxException the expression's syntax is invalid
		 */
		inline bool Pattern::matches(const String& regex, const String& input) {
			return matches(regex, unicode::StringCharacterIterator(input), unicode::StringCharacterIterator(input, input.end()));}

		/// Returns the regular expression from which this pattern was compiled.
		inline String Pattern::pattern() const {const std::basic_string<CodePoint> s(impl_.str());
			return String(unicode::UTF32To16Iterator<>(s.data()), unicode::UTF32To16Iterator<>(s.data() + s.length()));}

}}	// namespace ascension.regex

#endif /* !ASCENSION_REGEX_HPP */
#endif /* !ASCENSION_NO_REGEX */
