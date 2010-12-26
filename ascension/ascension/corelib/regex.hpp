/**
 * @file regex.hpp
 * Defines wrappers of Boost.Regex library or std#tr1#regex.
 * @author exeal
 * @date 2006-2010
 */

#include <ascension/config.hpp>	// ASCENSION_NO_REGEX, ASCENSION_NO_MIGEMO

#ifndef ASCENSION_NO_REGEX
#ifndef ASCENSION_REGEX_HPP
#define ASCENSION_REGEX_HPP

#include <ascension/corelib/unicode-property.hpp>
#include <ascension/corelib/unicode-utf.hpp>	// text.UTF16To32Iterator
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
		 * @tparam CodePointIterator an iterator represents a position in the input character
		 *                           sequence (should be UTF-32)
		 * @see Matcher
		 */
		template<typename CodePointIterator> class MatchResult {
		public:
			/// Destructor.
			virtual ~MatchResult() /*throw()*/ {}
			/**
			 * Returns the position after the last character matched.
			 * @return The position after the last character matched
			 */
			virtual const CodePointIterator& end() const = 0;
			/**
			 * Returns the position after the last character of the subsequence captured by the
			 * given group during this match.
			 * @param group The index of a capturing group in this matcher's pattern
			 * @return The position after the last character captured by the group
			 */
			virtual const CodePointIterator& end(int group) const = 0;
			/**
			 * Returns the input subsequence matched by the previous match.
			 * @return The (possibly empty) subsequence matched by the previous match, in string
			 *         form
			 */
			virtual String group() const = 0;
			/**
			 * Returns the input subsequence captured by the given group during the previous match
			 * operation.
			 * @param group The index of a capturing group in this matcher's pattern
			 * @return The (possibly empty) subsequence captured by the group during the previous
			 *         match
			 */
			virtual String group(int group) const = 0;
			/**
			 * Returns the number of the capturing groups this match result's pattern.
			 * @return The number of capturing groups in this matcher's pattern
			 */
			virtual std::size_t groupCount() const = 0;
			/**
			 * Returns the start position of the match.
			 * @return The position of the first character matched
			 */
			virtual const CodePointIterator& start() const = 0;
			/**
			 * Returns the start position of the subsequence captured by the given group during
			 * this match.
			 * @param group The index of a capturing group in this matcher's pattern
			 * @return The position of the first character captured by the group
			 */
			virtual const CodePointIterator& start(int group = 0) const = 0;
		private:
			ASCENSION_STATIC_ASSERT(text::CodeUnitSizeOf<CodePointIterator>::result == 4);
		};
	}

	namespace detail {
		/// @internal
		template<typename CodePointIterator>
		class MatchResultImpl : public regex::MatchResult<CodePointIterator> {
		public:
			MatchResultImpl() {}
			explicit MatchResultImpl(
				const boost::match_results<CodePointIterator>& src) : impl_(src) {}
			const CodePointIterator& end() const {return end(0);}
			const CodePointIterator& end(int group) const {return get(group).second;}
			String group() const {return group(0);}
			String group(int group) const {
				const std::basic_string<CodePoint> s(get(group).str());
				return String(text::UTF32To16Iterator<>(
					s.data()), text::UTF32To16Iterator<>(s.data() + s.length()));
			}
			std::size_t groupCount() const {return impl_.size();}
			const CodePointIterator& start() const {return start(0);}
			const CodePointIterator& start(int group) const {return get(group).first;}
		protected:
			boost::match_results<CodePointIterator>& impl() /*throw()*/ {return impl_;}
			const boost::match_results<CodePointIterator>& impl() const /*throw()*/ {return impl_;}
		private:
			const boost::sub_match<CodePointIterator>& get(int group) const {
				const boost::sub_match<CodePointIterator>& s = impl_[group];
				if(group != 0 && !impl_[0].matched)
					throw IllegalStateException("the previous was not performed or failed.");
				if(group != 0 && !s.matched)
					throw IndexOutOfBoundsException("the specified sub match group is not exist.");
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
				POSIX_ALNUM = text::ucd::SentenceBreak::LAST_VALUE,
				POSIX_BLANK, POSIX_GRAPH, POSIX_PRINT, POSIX_PUNCT, POSIX_WORD, POSIX_XDIGIT,
				// regex specific general category
				GC_ANY, GC_ASSIGNED, GC_ASCII,
				CLASS_END
			};
		public:
			// original interface
			RegexTraits() : collator_(&std::use_facet<std::collate<char_type> >(locale_)) {}
			static bool unixLineMode, usesExtendedProperties;
			// minimal requirements for traits
			typedef CodePoint char_type;
			typedef std::size_t size_type;
			typedef std::basic_string<char_type> string_type;
			typedef std::locale locale_type;
			typedef std::bitset<CLASS_END> char_class_type;
			static size_type length(const char_type* p) {size_type i = 0; while(p[i] != 0) ++i; return i;}
			char_type translate(char_type c) const {
				if(unixLineMode) return (c == LINE_FEED) ? LINE_SEPARATOR : c;
				return (c < 0x10000ul && std::binary_search(NEWLINE_CHARACTERS,
					ASCENSION_ENDOF(NEWLINE_CHARACTERS), static_cast<Char>(c & 0xffffu))) ? LINE_SEPARATOR : c;
			}
			char_type translate_nocase(char_type c) const {return text::CaseFolder::fold(translate(c));}
			string_type transform(const char_type* p1, const char_type* p2) const {return collator_->transform(p1, p2);}
			string_type transform_primary(const char_type* p1, const char_type* p2) const {return transform(p1, p2);}
			char_class_type lookup_classname(const char_type* p1, const char_type* p2) const;
			string_type lookup_collatename(const char_type* p1, const char_type* p2) const {return transform(p1, p2);}
			bool isctype(char_type c, const char_class_type& f) const;
			int value(char_type c, int radix) const {
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
			locale_type imbue(locale_type l) {
				locale_type temp = locale_;
				collator_ = &std::use_facet<std::collate<char_type> >(locale_ = l);
				return temp;
			}
			locale_type getloc() const {return locale_;}
			std::string error_string(boost::regex_constants::error_type) const {return "Unknown error";}
		private:
			static std::size_t findPropertyValue(const String& expression);
		private:
			locale_type locale_;
			const std::collate<char_type>* collator_;
			static std::map<const Char*, int, text::ucd::PropertyNameComparer<Char> > names_;
			static void buildNames();
		};
	} // namespace internal

	namespace regex {

		class Pattern;

		// the documentation is regex.cpp
		template<typename CodePointIterator>
		class Matcher : public detail::MatchResultImpl<CodePointIterator> {
		private:
			typedef typename detail::MatchResultImpl<CodePointIterator> Base;
		public:
			/**
			 * Returns the pattern that is interpreted by this matcher.
			 * @return The pattern for which this matcher was created
			 */
			const Pattern& pattern() const /*throw()*/ {return *pattern_;}
			/**
			 * Changes the @c Pattern that this @c Matcher uses to find matches with.
			 * This method causes this matcher to lose information about the groups of the last
			 * match that occurred. The matcher's position in the input is maintained and its last
			 * append position is unaffected.
			 * @param newPattern The new pattern used by this matcher
			 * @return This matcher
			 */
			Matcher& usePattern(const Pattern& newPattern) {pattern_ = &pattern; Base::impl().reset(); return *this;}

			/**
			 * @brief Sets the limits of this matcher's region. The region is the part of the input
			 * sequence that will be searched to find a match. Invoking this method resets the
			 * matcher, and then sets the region to start at the index specified by the @a start
			 * parameter and end at the index specified by the @a end parameter.
			 * @par Depending on the transparency and anchoring being used (see
			 * @c useTransparentBounds and @c useAnchoringBounds), certain constructs such as
			 * anchors may behave differently at or around the boundaries of the region.
			 * @param start The index to start searching at (inclusive)
			 * @param end The index to end searching at (exclusive)
			 * @return This matcher
			 * @throw std#out_of_range If @a start is greater than the length of the input
			 *                         sequence, if @a end is greater than the length of the input
			 *                         sequence, or if @a start is greater than @a end
			 */
			Matcher& region(CodePointIterator start, CodePointIterator end) {
				reset();
				current_ = region_.first = start;
				region_.second = end;
				return *this;
			}
			/**
			 * Reports the end index (exclusive) of this matcher's region. The searches this
			 * matcher conducts are limited to finding matches within @c regionStart (inclusive)
			 * and @c regionEnd (exclusive).
			 * @return The ending point of this matcher's region
			 */
			const CodePointIterator& regionEnd() const /*throw()*/ {return region_.second;}
			/**
			 * Reports the start index of this matcher's region. The searches this matcher conducts
			 * are limited to finding matches within @c regionStart (inclusive) and @c regionEnd
			 * (exclusive).
			 * @return The starting point of this matcher's region
			 */
			const CodePointIterator& regionStart() const /*throw()*/ {return region_.first;}

			/**
			 * Queries the anchoring of region bounds for this matcher.
			 * @par This method returns @c true if this matcher uses <em>anchoring</em> bounds,
			 * @c false otherwise.
			 * @par See @c useAnchoringBounds for a description of anchoring bounds.
			 * @par By default, a matcher uses anchoring region boundaries.
			 * @return true if this matcher is using anchoring bounds, false otherwise
			 * @see useAnchoringBounds
			 */
			bool hasAnchoringBounds() const /*throw()*/ {return usesAnchoringBounds_;}
			/**
			 * Queries the transparency of region bounds for this matcher.
			 * @par This method returns @c true if this matcher uses <em>transparent</em> bounds,
			 * @c false if it uses <em>opaque</em> bounds.
			 * @par See @c useTransparentBounds for a description of transparent and opaque bounds.
			 * @par By default, a matcher uses opaque region boundaries.
			 * @return true if this matcher is using transparent bounds, false otherwise
			 * @see useTransparentBounds
			 */
			bool hasTransparentBounds() const /*throw()*/ {return usesTransparentBounds_;}
			/**
			 * Sets the anchoring of region bounds for this matcher.
			 * @par Invoking this method with an argument of @c true will set this matcher to use
			 * <em>anchoring</em> bounds. If the boolean argument is @c false, then
			 * <em>non-anchoring</em> bounds will be used.
			 * @par Using anchoring bounds, the boundaries of this matcher's region match anchors
			 * such as ^ and $.
			 * @par Without anchoring bounds, the boundaries of this matcher's region will not
			 * match anchors such as ^ and $.
			 * @par By default, a matcher uses anchoring region boundaries.
			 * @param b A boolean indicating whether or not to use anchoring bounds
			 * @return This matcher
			 * @see hasAnchoringBounds
			 */
			Matcher& useAnchoringBounds(bool b) /*throw()*/ {usesAnchoringBounds_ = b; return *this;}
			/**
			 * Sets the transparency of region bounds for this matcher.
			 * @par Invoking this method with an argument of @c true will set this matcher to use
			 * <em>transparent</em> bounds. If the boolean argument is @c false, then
			 * <em>opaque</em> bounds will be used.
			 * @par Using transparent bounds, the boundaries of this matcher's region are
			 * transparent to lookahead, lookbehind, and boundary matching constructs. Those
			 * constructs can see beyond the boundaries of the region to see if a match is
			 * appropriate.
			 * @par Using opaque bounds, the boundaries of this matcher's region are opaque to
			 * lookahead, lookbehind, and boundary matching constructs that may try to see beyond
			 * them. Those constructs cannot look past the boundaries so they will fail to match
			 * anything outside of the region.
			 * @par By default, a matcher uses opaque bounds.
			 * @param b A boolean indicating whether to use opaque or transparent regions
			 * @return This matcher
			 * @see hasTransparentBounds
			 */
			Matcher& useTransparentBounds(bool b) /*throw()*/ {usesTransparentBounds_ = b; return *this;}

			/**
			 * Attempts to find the next subsequence of the input sequence that matches the
			 * pattern.
			 * <p>This method starts at the beginning of the matcher's region, or, if a previous
			 * invocation of the method was successful and the matcher has not since been reset, at
			 * the first character not matched by the previous match.</p>
			 * <p>If the match succeeds then more information can be obtained via the @c start,
			 * @c end, and @c group methods.</p>
			 * @return true if, and only if, a subsequence of the input sequence matches this
			 *         matcher's pattern
			 */
			bool find() {
				checkInplaceReplacement();
				boost::regex_search(current_, region_.second, Base::impl(),
					pattern_->impl_, nativeFlags(current_, region_.second, true));
				return acceptResult();
			}
			/**
			 * Resets this matcher and then attempts to find the next subsequence of the input
			 * sequence that matches the pattern, starting at the specified index.
			 * If the match succeeds then more information can be obtained via the @c start,
			 * @c end, and @c group methods, and subsequent invocations of the @c find() method
			 * will start at the first character not matched by this match.
			 * @param start 
			 * @return true if, and only if, a subsequence of the input sequence starting at the
			 *         given index matches this matcher's pattern
			 * @throw std::out_of_range 
			 */
			bool find(CodePointIterator start) {
				reset();
				boost::regex_search(start, input_.second, Base::impl(),
					pattern_->impl_, nativeFlags(start, input_.second, true));
				return acceptResult();
			}
			/**
			 * Attempts to match the input sequence, starting at the beginning of the region,
			 * against the pattern.
			 * <p>Like the @c matches method, this method always starts at the beginning of the
			 * region; unlike that method, it does not require that the entire region be matched.
			 * </p>
			 * <p>If the match succeeds then more information can be obtained via the @c start,
			 * @c end, and @c group methods.
			 * @return true if, and only if, a prefix of the input sequence matches this matcher's
			 *         pattern
			 */
			bool lookingAt() {
				boost::regex_search(region_.first, region_.second, Base::impl(),
					pattern_->impl_, nativeFlags(region_.first, region_.second, false)
						| boost::regex_constants::match_continuous);
				return acceptResult();}
			/**
			 * Attempts to match the entire region against the pattern.
			 * If the match succeeded then more information can be obtained via the @c start,
			 * @c end, and @c group methods.
			 * @return true if, and only if, the entire region sequence matches this matcher's
			 *         pattern
			 */
			bool matches() {
				boost::regex_match(region_.first, region_.second, Base::impl(),
					pattern_->impl_, nativeFlags(region_.first, region_.second, false));
				return acceptResult();
			}

			// documentation is regex.cpp
			template<typename OutputIterator>
			Matcher& appendReplacement(OutputIterator out, const String& replacement) {
				checkInplaceReplacement(); checkPreviousMatch();
				appendReplacement(out, replacement, ascension::internal::Int2Type<text::CodeUnitSizeOf<OI>::result>());
				appendingPosition_ = Base::impl()[0].second;
				return *this;
			}
			/**
			 * Implements a terminal append-and-replace step.
			 * This method reads characters from the input sequence, starting at the append
			 * position, and appends them to the given string buffer. It is intended to be invoked
			 * after one or more invocations of the @c appendReplacement method in order to copy
			 * the remainder of the input sequence.
			 * @tparam OutputIterator The type of @a out
			 * @param out The output character sequence
			 * @return The output iterator
			 */
			template<typename OutputIterator>
			OutputIterator appendTail(OutputIterator out) const {
				checkInplaceReplacement();
				return appendTail(out, internal::Int2Type<text::CodeUnitSizeOf<OutputIterator>::result>());
			}
			// documentation is regex.cpp
			String replaceAll(const String& replacement) {
				reset();
				std::basic_ostringstream<Char> s;
				std::ostream_iterator<Char, Char> os(s);
				while(find())
					appendReplacement(os, replacement);
				appendTail(os);
				return s.str();
			}
			// documentation is regex.cpp
			String replaceFirst(const String& replacement) {
				reset();
				std::basic_ostringstream<Char> s;
				std::ostream_iterator<Char, Char> os(s);
				if(find())
					appendReplacement(os, replacement);
				appendTail(os);
				return s.str();
			}

			/// Ends the active in-place replacement context.
			Matcher& endInplaceReplacement(CodePointIterator first, CodePointIterator last,
					CodePointIterator regionFirst, CodePointIterator regionLast, CodePointIterator next) {
				if(!replaced_)
					throw IllegalStateException("the matcher is not entered in in-place replacement context.");
				const bool matchedZW = matchedZeroWidth_;
				reset(first, last);
				region_.first = regionFirst;
				region_.second = regionLast;
				current_ = next;
				matchedZeroWidth_ = matchedZW;
				return *this;
			}
			/// ...
			String replaceInplace(const String& replacement) {
				if(!Base::impl()[0].matched)
					throw IllegalStateException("the previous was failed or not performed.");
				else if(replaced_)
					throw IllegalStateException("this matcher already entered in in-place replacement.");
				const std::basic_string<CodePoint> temp(Base::impl().format(std::basic_string<CodePoint>(
					text::UTF16To32Iterator<String::const_iterator>(
						replacement.begin(), replacement.end()),
					text::UTF16To32Iterator<String::const_iterator>(
						replacement.begin(), replacement.end(), replacement.end()))));
				replaced_ = true;
				return String(text::UTF32To16Iterator<>(
					temp.data()), text::UTF32To16Iterator<>(temp.data() + temp.length()));
			}

			/**
			 * Resets the matcher.
			 * Resetting a matcher discards all of its explicit state information and sets its
			 * append position to the beginning of the input. The matcher's region is set to the
			 * default region, which is its entire character sequence. The anchoring and
			 * transparency of this matcher's region boundaries are unaffected.
			 * @return This matcher
			 */
			Matcher& reset() {
				Base::impl() = boost::match_results<CodePointIterator>();
				region_ = input_;
				current_ = appendingPosition_ = input_.first;
				replaced_ = false;
				return *this;
			}
			/**
			 * Resets this matcher with a new input sequence.
			 * Resetting a matcher discards all of its explicit state information and sets its
			 * append position to the beginning of the input. The matcher's region is set to the
			 * default region, which is its entire character sequence. The anchoring and
			 * transparency of this matcher's region boundaries are unaffected.
			 * @param first The beginning of the new input character sequence
			 * @param last The end of the new input character sequence
			 * @return This matcher
			 */
			Matcher& reset(CodePointIterator first, CodePointIterator last) {
				input_.first = first;
				input_.second = last;
				return reset();
			}

			/**
			 * Returns the match state of this matcher as a @c MatchResult.
			 * The result is unaffected by subsequent operations performed upon the matcher.
			 * @return A @c MatchResult with the state of this matcher
			 */
			std::auto_ptr<MatchResult<CodePointIterator> > toMatchResult() const {
				return std::auto_ptr<MatchResult<CPI> >(new internal::MatchResultImpl<CPI>(Base::impl()));
			}
		private:
			Matcher(const Pattern& pattern, CodePointIterator first, CodePointIterator last) :
				pattern_(&pattern), current_(first), input_(first, last), region_(first, last),
				appendingPosition_(input_.first), replaced_(false), matchedZeroWidth_(false),
				usesAnchoringBounds_(true), usesTransparentBounds_(false) {}
			template<typename OutputIterator> void appendReplacement(OutputIterator out,
					const String& replacement, const detail::Int2Type<2>&) {
				typedef typename boost::match_results<CodePointIterator>::string_type String32;
				if(appendingPosition_ != input_.second)
					std::copy(appendingPosition_, Base::impl()[0].first, out);
				const String32 replaced(Base::impl().format(String32(
					text::UTF16To32Iterator<String::const_iterator>(replacement.begin(), replacement.end()),
					text::UTF16To32Iterator<String::const_iterator>(replacement.begin(), replacement.end(), replacement.end()))));
				std::copy(text::UTF32To16Iterator<typename String32::const_iterator>(replaced.begin()),
					text::UTF32To16Iterator<typename String32::const_iterator>(replaced.end()), out);
			}
			template<typename OutputIterator> void appendReplacement(OutputIterator out,
					const String& replacement, const detail::Int2Type<4>&) {
				if(appendingPosition_ != input_.second)
					std::copy(appendingPosition_, Base::impl()[0].first, out);
				const String& replaced(Base::impl().format(replacement));
				std::copy(replaced.begin(), replaced.end(), out);
			}
			template<typename OutputIterator>
			OutputIterator appendTail(OutputIterator out, const detail::Int2Type<2>&) const {
				return std::copy(
					text::UTF32To16Iterator<CodePointIterator>(appendingPosition_),
					text::UTF32To16Iterator<CodePointIterator>(input_.second), out);
			}
			template<typename OutputIterator>
			OutputIterator appendTail(OutputIterator out, const detail::Int2Type<4>&) const {
				return std::copy(appendingPosition_, input_.second, out);
			}
			bool acceptResult() {const bool b(Base::impl()[0].matched); matchedZeroWidth_ = b && Base::impl().length() == 0; if(b) current_ = Base::end(); return b;}
			void checkInplaceReplacement() const {if(replaced_) throw IllegalStateException("the matcher entered to in-place replacement.");}
			void checkPreviousMatch() const {if(!Base::impl()[0].matched) throw IllegalStateException("the previous was not performed or failed.");}
			boost::match_flag_type nativeFlags(const CodePointIterator& first,
					const CodePointIterator& last, bool continuous) const /*throw()*/ {
				boost::match_flag_type f(boost::regex_constants::match_default);
				if((pattern_->flags() & Pattern::DOTALL) == 0)
					f |= boost::regex_constants::match_not_dot_newline;
				if((pattern_->flags() & Pattern::MULTILINE) == 0)
					f |= boost::regex_constants::match_single_line;
				if(continuous && matchedZeroWidth_)
					f |= boost::regex_constants::match_not_initial_null;
				if(!usesAnchoringBounds_) {
					if(first != input_.first)
						f |= boost::regex_constants::match_not_bob | boost::regex_constants::match_not_bol;
					if(last != input_.second)
						f |= boost::regex_constants::match_not_eol | boost::regex_constants::match_not_eol;
				}
				if(usesTransparentBounds_ && first != input_.first)
					f |= boost::regex_constants::match_prev_avail;
				return f;
			}
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
			ASCENSION_UNASSIGNABLE_TAG(PatternSyntaxException);
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
			/// Destructor.
			~PatternSyntaxException() throw() {}
			/// Retrieves the error code.
			Code getCode() const;
			/// Retrieves the description of the error.
			std::string getDescription() const;
			/// Retrieves the error index.
			std::ptrdiff_t getIndex() const {return impl_.position();}
			/// Retrieves the erroneous regular-expression pattern.
			String getPattern() const /*throw()*/ {return pattern_;}
		private:
			const boost::regex_error impl_;
			const String pattern_;
		};

		// the documentation is regex.cpp
		class Pattern {
			ASCENSION_UNASSIGNABLE_TAG(Pattern);
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
			virtual ~Pattern() /*throw()*/;

			/**
			 * Returns this pattern's match flags.
			 * @return The match flags specified when this pattern was compiled
			 */
			int flags() const /*throw()*/ {return flags_;}
			/**
			 * Returns the regular expression from which this pattern was compiled.
			 * @return The source of this pattern
			 */
			String pattern() const {
				const std::basic_string<CodePoint> s(impl_.str());
				return String(text::UTF32To16Iterator<>(s.data()), text::UTF32To16Iterator<>(s.data() + s.length()));
			}

			/**
			 * Compiles the given regular expression into a pattern with the given flags.
			 * @param regex The expression to be compiled
			 * @param flags Match flags
			 * @throw UnknownValueException Bit values other than those corresponding to the
			 *                              defined match flags are set in @a flags
			 * @throw PatternSyntaxException The expression's syntax is invalid
			 */
			static std::auto_ptr<const Pattern> compile(const String& regex, int flags = 0) {
				return std::auto_ptr<const Pattern>(new Pattern(regex, flags));
			}
			/**
			 * Creates a matcher that will match the given input against this pattern.
			 * @tparam CodePointIterator A type of the input character sequence
			 * @param first The beginning of the character sequence to be matched
			 * @param last The end of the character sequence to be matched
			 * @return A new matcher for this pattern
			 */
			template<typename CodePointIterator>
			std::auto_ptr<Matcher<CodePointIterator> > matcher(CodePointIterator first, CodePointIterator last) const {
				return std::auto_ptr<Matcher<CodePointIterator> >(new Matcher<CodePointIterator>(*this, first, last));
			}

			/**
			 * Compiles the given regular expression and attempts to match the given input against it.
			 * An invocation of this convenience method of the form
			 * @code
			 * Pattern.matches(regex, input);
			 * @endcode
			 * behaves in exactly the same way as the expression
			 * @code
			 * Pattern.compile(regex).matcher(input).matches()
			 * @endcode
			 * If a pattern is to be used multiple times, compiling it once and reusing it will be
			 * more efficient than invoking this method each time.
			 * @param regex the expression to be compiled
			 * @param input the character sequence to be matched
			 * @throw PatternSyntaxException the expression's syntax is invalid
			 */
			static bool matches(const String& regex, const String& input) {
				return matches(regex,
					text::StringCharacterIterator(input), text::StringCharacterIterator(input, input.end()));
			}
			/**
			 * Compiles the given regular expression and attempts to match the given input against it.
			 * @param regex the expression to be compiled
			 * @param first the character sequence to be matched
			 * @param last the end of @a first
			 * @throw PatternSyntaxException the expression's syntax is invalid
			 */
			template<typename CodePointIterator>
			static bool matches(const String& regex, CodePointIterator first, CodePointIterator last) {
				return compile(regex)->matcher(first, last)->matches();
			}
//			template<typename CodePointIterator, typename OutputIterator>
//			std::size_t split(CodePointIteratorfirst, CodePointIteratorlast, OutputIterator out, std::size_t limit = -1);
		protected:
			Pattern(const Char* first, const Char* last, boost::regex_constants::syntax_option_type nativeSyntax);
		private:
			Pattern(const String& regex, int flags);
			boost::basic_regex<CodePoint, detail::RegexTraits> impl_;
			const int flags_;
			template<typename CodePointIterator> friend class Matcher;
		};

#ifndef ASCENSION_NO_MIGEMO
		/// Builds regular expression pattern for Migemo use.
		class MigemoPattern : public Pattern {
			ASCENSION_UNASSIGNABLE_TAG(MigemoPattern);
		public:
			static std::auto_ptr<MigemoPattern> compile(const Char* first, const Char* last, bool caseSensitive);
			static void initialize(const char* runtimePathName, const char* dictionaryPathName);
			static bool isMigemoInstalled() /*throw()*/;
		private:
			MigemoPattern(const Char* first, const Char* last, bool caseSensitive);
			static void install();
			static AutoBuffer<char> runtimePathName_, dictionaryPathName_;
		};
#endif // !ASCENSION_NO_MIGEMO

	}
}	// namespace ascension.regex

#endif // !ASCENSION_REGEX_HPP
#endif // !ASCENSION_NO_REGEX
