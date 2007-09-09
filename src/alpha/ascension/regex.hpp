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
	 * Defines stuffs related to regular expression. Classes provided by this namespace have
	 * interfaces familiar to Java/ICU regex users. @a CodePointIterator template parameters of
	 * class @c MatchResult and @c Matcher must represent a UTF-32 code point sequence.
	 */
	namespace regex {

		/**
		 * A result of a match operation.
		 * @see Matcher
		 */
		template<typename CodePointIterator> class MatchResult {
		public:
			/// Destructor.
			virtual ~MatchResult() {}
			/// Returns the end of the matched subtring.
			virtual const CodePointIterator& end() const = 0;
			/// Returns the end of the matched subtring.
			virtual const CodePointIterator& end(int group) const = 0;
			///
			virtual String group() const = 0;
			///
			virtual String group(int group) const = 0;
			/// Returns the number of the marked regex groups.
			virtual std::size_t groupCount() const = 0;
			/**
			 * Replaces the matched range by the specified replacement pattern.
			 * @param replacement the replacement string
			 * @return the replaced string
			 */
			virtual String replace(const String& replacement) const = 0;
			///
			virtual const CodePointIterator& start() const = 0;
			///
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
				String replace(const String& replacement) const;
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
					POSIX_ALNUM = unicode::ucd::SentenceBreak::LAST_VALUE, POSIX_BLANK, POSIX_GRAPH, POSIX_PRINT, POSIX_PUNCT, POSIX_WORD, POSIX_XDIGIT,
					// regex specific general category
					GC_ANY, GC_ASSIGNED, GC_ASCII,
					CLASS_END
				};
			public:
				// original interface
				RegexTraits() : collator_(&std::use_facet<std::collate<char_type> >(locale_)) {}
				static bool	enablesExtendedProperties;
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
			void checkPreviousMatch() {if(!impl()[0].matched) throw IllegalStateException("the previous was not performed or failed.");}
			boost::match_flag_type getNativeFlags(const CodePointIterator& first, const CodePointIterator& last, bool continuous) const throw();
			const Pattern* pattern_;
			CodePointIterator current_;
			std::pair<CodePointIterator, CodePointIterator> input_, region_;
			CodePointIterator appendingPosition_;
			bool usesAnchoringBounds_, usesTransparentBounds_;
			friend class Pattern;
		};

		class PatternSyntaxException : public std::invalid_argument {
		public:
			enum {
			};
			/// Constructor.
			explicit PatternSyntaxException(const boost::regex_error& src) : std::invalid_argument(""), impl_(src) {}
			/// Returns the error code.
			int code() const {return impl_.code();}
			/// Returns the position in the expression where parsing stopped.
			std::ptrdiff_t position() const {return impl_.position();}
		private:
			const boost::regex_error impl_;
		};

		class Pattern {
		public:
			enum {
				CANON_EQ = 0x01,			///< Enables canonical equivalents (not implemented).
				CASE_INSENSITIVE = 0x02,	///< Enables caseless matches.
				COMMENTS = 0x04,			///< Enables white spaces and comments in pattern string.
				DOTALL = 0x08,				///< Indicates that <kbd>/./</kbd> matches any characters include EOLs.
				LITERAL = 0x10,				///< Enables literal matches.
				MULTILINE = 0x20,			///< Enables multiline mode.
				UNICODE_CASE = 0x40,		///< Enables Unicode-comformant caseless matches (not implemented).
				UNIX_LINES = 0x80			///< Enables Unix line mode (not implemented).
			};
		public:
			virtual ~Pattern() throw();
			// attributes
			int		flags() const throw();
			String	pattern() const;
			// compilation
			static std::auto_ptr<const Pattern>				compile(const String& regex, int flags = 0);
			template<typename CodePointIterator>
			std::auto_ptr<Matcher<CodePointIterator> >	matcher(CodePointIterator first, CodePointIterator last) const;
			// tools
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

		template<typename CPI> inline String regex::internal::MatchResultImpl<CPI>::replace(const String& replacement) const {
			if(!impl_[0].matched) throw IllegalStateException("the previous was failed or not performed.");
			const std::basic_string<CodePoint> temp(impl_.format(std::basic_string<CodePoint>(
				unicode::UTF16To32Iterator<String::const_iterator>(replacement.begin(), replacement.end()),
				unicode::UTF16To32Iterator<String::const_iterator>(replacement.begin(), replacement.end(), replacement.end()))));
			return String(unicode::UTF32To16Iterator<>(temp.data()), unicode::UTF32To16Iterator<>(temp.data() + temp.length()));}

		// Matcher //////////////////////////////////////////////////////////

		/// Private constructor.
		template<typename CPI> inline Matcher<CPI>::Matcher(const Pattern& pattern, CPI first, CPI last) :
			pattern_(&pattern), current_(first), input_(first, last), region_(first, last),
			appendingPosition_(input_.first), usesAnchoringBounds_(true), usesTransparentBounds_(false) {}

		template<typename CPI> template<typename OI>
		inline void Matcher<CPI>::appendReplacement(OI out, const String& replacement, const ascension::internal::Int2Type<2>&) {
			if(appendingPosition_ != input_.second) std::copy(appendingPosition_, impl()[0].first, out);
			const String& replaced(replace(replacement)); std::copy(replaced.begin(), replaced.end(), out);}

		template<typename CPI> template<typename OI>
		inline void Matcher<CPI>::appendReplacement(OI out, const String& replacement, const ascension::internal::Int2Type<4>&) {
			if(appendingPosition_ != input_.second) std::copy(appendingPosition_, impl()[0].first, out);
			const String& replaced(impl().format(replacement)); std::copy(replaced.begin(), replaced.end(), out);}

		template<typename CPI> template<typename OI>
		inline Matcher<CPI>& Matcher<CPI>::appendReplacement(OI out, const String& replacement) {
			checkPreviousMatch();
			appendReplacement(out, replacement, ascension::internal::Int2Type<unicode::CodeUnitSizeOf<OI>::result>());
			appendingPosition_ = impl()[0].second; return *this;}
		
		template<typename CPI> template<typename OI> inline OI Matcher<CPI>::appendTail(OI out,
			const ascension::internal::Int2Type<2>&) const {return std::copy(unicode::UTF32To16Iterator<CPI>(appendingPosition_), unicode::UTF32To16Iterator<CPI>(input_.second), out);}

		template<typename CPI> template<typename OI> inline OI Matcher<CPI>::appendTail(OI out,
			const ascension::internal::Int2Type<4>&) const {return std::copy(appendingPosition_, input_.second, out);}

		/**
		 * @param out
		 */
		template<typename CPI> template<typename OI> inline OI Matcher<CPI>::appendTail(OI out) const {
			return appendTail(out, unicode::CodeUnitSizeOf<OI>::result());}

		/// Searches the next subsequence matches the pattern in the input sequence.
		template<typename CPI> inline bool Matcher<CPI>::find() {if(boost::regex_search(current_, region_.second, impl(),
			pattern_->impl_, getNativeFlags(current_, region_.second, true))) current_ = impl()[0].second; return impl()[0].matched;}

		/// Resets the engine and searches the next subsequence matches the pattern in the input sequence from the given position.
		template<typename CPI> inline bool Matcher<CPI>::find(CPI start) {
			reset(); if(boost::regex_search(start, input_.second, impl(), pattern_.impl_,
				getNativeFlags(start, input_.second, true))) current_ = impl()[0].second; return impl()[0].matched;}

		template<typename CPI> inline boost::match_flag_type
		Matcher<CPI>::getNativeFlags(const CPI& first, const CPI& last, bool continuous) const throw() {
			boost::match_flag_type f(boost::regex_constants::match_default);
			if((pattern_->flags() & Pattern::DOTALL) == 0) f |= boost::regex_constants::match_not_dot_newline;
			if((pattern_->flags() & Pattern::MULTILINE) == 0) f |= boost::regex_constants::match_single_line;
			if(continuous && impl()[0].matched && impl().length() == 0) f |= boost::regex_constants::match_not_initial_null;
			if(!usesAnchoringBounds_) {
				if(first != input_.first) f |= boost::regex_constants::match_not_bob | boost::regex_constants::match_not_bol;
				if(last != input_.second) f |= boost::regex_constants::match_not_eol | boost::regex_constants::match_not_eol;
			}
			if(usesTransparentBounds_ && first != input_.first) f |= boost::regex_constants::match_prev_avail;
			return f;
		}

		/// Returns true if the engine uses anchoring bounds.
		template<typename CPI> inline bool Matcher<CPI>::hasAnchoringBounds() const throw() {return usesAnchoringBounds_;}

		/// Returns true if the engine uses transparent bounds.
		template<typename CPI> inline bool Matcher<CPI>::hasTransparentBounds() const throw() {return usesTransparentBounds_;}

		/// Begins the match from the beginning of the region.
		template<typename CPI> inline bool Matcher<CPI>::lookingAt() {return boost::regex_search(current_, region_.second,
			impl(), pattern_->impl_, getNativeFlags(current_, region_.second, false) | boost::regex_constants::match_continuous);}

		/// Returns true if the pattern matches the entire region.
		template<typename CPI> inline bool Matcher<CPI>::matches() {return boost::regex_match(
			region_.first, region_.second, impl(), pattern_->impl_, getNativeFlags(region_.first, region_.second, false));}

		/// Returns the pattern interpreted by the regex engine.
		template<typename CPI> inline const Pattern& Matcher<CPI>::pattern() const throw() {return *pattern_;}

		/// Sets the region of the regex engine.
		template<typename CPI> inline Matcher<CPI>& Matcher<CPI>::region(
			CPI start, CPI end) {reset(); current_ = region_.first = start; region_.second = end; return *this;}

		/// Returns the end of the region of the regex engine.
		template<typename CPI> inline const CPI& Matcher<CPI>::regionEnd() const throw() {return region_.second;}

		/// Returns the beginning of the regex engine.
		template<typename CPI> inline const CPI& Matcher<CPI>::regionStart() const throw() {return region_.first;}

		/// Replaces the subsequences in the input sequence match the pattern with the given string.
		template<typename CPI> inline String Matcher<CPI>::replaceAll(const String& replacement) {reset(); OutputStringStream s;
			std::ostream_iterator<Char> os(s); while(find()) appendReplacement(os, replacement); appendTail(os); return s.str();}

		/// Replaces the first subsequence in the input sequence matches the pattern with the given string.
		template<typename CPI> inline String Matcher<CPI>::replaceFirst(const String& replacement) {reset(); OutputStringStream s;
			std::ostream_iterator<Char> os(s); if(find()) appendReplacement(os, replacement); appendTail(os); return s.str();}

		/// Resets the regex engine.
		template<typename CPI> inline Matcher<CPI>& Matcher<CPI>::reset() {
			impl() = boost::match_results<CPI>(); region_ = input_; appendingPosition_ = input_.first; return *this;}

		/// Resets the regex engine with the new input sequence.
		template<typename CPI> inline Matcher<CPI>& Matcher<CPI>::reset(CPI first, CPI last) {input_.first = first; input_.second; return reset();}

		/// Returns a match result of the regex engine as @c MatchResult.
		template<typename CPI> inline std::auto_ptr<MatchResult<CPI> > Matcher<CPI>::toMatchResult() const {
			return std::auto_ptr<MatchResult<CPI> >(new internal::MatchResultImpl<CPI>(impl()));}

		/// Sets the anchors of the region boundaries of the regex engine.
		template<typename CPI> inline Matcher<CPI>& Matcher<CPI>::useAnchoringBounds(bool b) throw() {usesAnchoringBounds_ = b; return *this;}

		/// Changes the pattern to match.
		template<typename CPI> inline Matcher<CPI>& Matcher<CPI>::usePattern(
			const Pattern& newPattern) {pattern_ = &pattern; impl().reset(); return *this;}

		/// Sets the transparencies of the region boundaries of the regex engine.
		template<typename CPI> inline Matcher<CPI>& Matcher<CPI>::useTransparentBounds(bool b) throw() {usesTransparentBounds_ = b; return *this;}

		// Pattern //////////////////////////////////////////////////////////

		/// Compiles the given regex into the pattern with the flags.
		inline std::auto_ptr<const Pattern> Pattern::compile(
			const String& regex, int flags /* = 0 */) {return std::auto_ptr<const Pattern>(new Pattern(regex, flags));}

		/// Returns the match flags of the pattern.
		inline int Pattern::flags() const throw() {return flags_;}

		/// Creates the regular expression engine with the given input sequence.
		template<typename CPI> inline std::auto_ptr<Matcher<CPI> > Pattern::matcher(CPI first, CPI last) const {
			return std::auto_ptr<Matcher<CPI> >(new Matcher<CPI>(*this, first, last));}

		/// Returns true if the given regular expression matches the input sequence.
		template<typename CPI> inline bool Pattern::matches(const String& regex, CPI first, CPI last) {return compile(regex).matcher(first, last).matches();}

		/// Returns the compiled regular expression.
		inline String Pattern::pattern() const {const std::basic_string<CodePoint> s(impl_.str());
			return String(unicode::UTF32To16Iterator<>(s.data()), unicode::UTF32To16Iterator<>(s.data() + s.length()));}

}}	// namespace ascension.regex

#endif /* !ASCENSION_REGEX_HPP */
#endif /* !ASCENSION_NO_REGEX */
