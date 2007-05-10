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

	/// Defines stuffs related to regular expression.
	namespace regex {

		namespace internal {
			/**
			 * Unicode property enabled @c regex_traits for @c boost#basic_regex template class.
			 * This traits class does not implement "additional optional requirements"
			 * (http://www.boost.org/libs/regex/doc/concepts.html#traits).
			 * @note This class is final.
			 */
			class RegexTraits {
			private:
				enum {
					// POSIX compatible not in Unicode property
					POSIX_ALNUM = unicode::BinaryProperty::COUNT, POSIX_BLANK, POSIX_GRAPH, POSIX_PRINT, POSIX_PUNCT, POSIX_WORD, POSIX_XDIGIT,
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
				std::string error_string(boost::regex_constants::error_type n) const {return "Unknown error";}
			private:
				static std::size_t findPropertyValue(const String& expression);
			private:
				locale_type locale_;
				const std::collate<char_type>* collator_;
				static std::map<const Char*, int, unicode::PropertyNameComparer<Char> > names_;
				static void buildNames();
			};
		} // namespace internal

		/**
		 * Represents a match result. @a CharacterSequence template parameter must represent
		 * a UTF-32 code unit sequence.
		 * @see Pattern
		 */
		template<typename CharacterSequence>
		class MatchResult : public manah::Noncopyable {
		public:
			// attributes
			CharacterSequence	getEnd() const;
			String				getGroup(int group) const;
			CharacterSequence	getGroupEnd(int group) const;
			CharacterSequence	getGroupStart(int group) const;
			std::size_t			getNumberOfGroups() const;
			CharacterSequence	getStart() const;
			// operation
			String	replace(const Char* first, const Char* last) const;
		private:
			ASCENSION_STATIC_ASSERT(unicode::CodeUnitSizeOf<CharacterSequence>::result == 4);
			typedef const boost::match_results<CharacterSequence> Impl;
			explicit MatchResult(std::auto_ptr<Impl> impl) throw() : impl_(impl) {}
			std::auto_ptr<Impl> impl_;
			friend class Pattern;
		};

		class Pattern : public manah::Noncopyable {
		public:
			/// Options for expression syntax.
			enum SyntaxOption {
				NORMAL = 0x00,					///< Specifies that only without any other flags.
				CASE_INSENSITIVE = 0x01,		///< Enables case insensitive match.
				EXTENDED_PROPERTIES = 0x02,		///< Enables extended Unicode properties (see the description of @c Pattern).
				CANONICAL_EQUIVALENTS = 0x04	///< Forces normalization of pattern and target (not implemented currently).
			};
			/// Options for pattern match.
			enum MatchOption {
				NONE = 0x0000,							///< Specifies that only without any other flags.
				MULTILINE = 0x0001,						///< Specifies that the target is multiline.
				DOTALL = 0x0002,						///< Specifies that "." matches a NLF character.
				MATCH_AT_ONLY_TARGET_FIRST = 0x0004,	///< Specifies that the pattern must match the start of the target.
				TARGET_FIRST_IS_NOT_BOB = 0x0008,		///< Specifies that the start of the target is not the begin of the buffer.
				TARGET_LAST_IS_NOT_EOB = 0x0010,		///< Specifies that the end of the target is not the end of the buffer.
				TARGET_FIRST_IS_NOT_BOL = 0x0020,		///< Specifies that the start of the target is not the begin of a line.
				TARGET_LAST_IS_NOT_EOL = 0x0040,		///< Specifies that the end of the target is not the end of a line.
				TARGET_FIRST_IS_NOT_BOW = 0x0080,		///< Specifies that the start of the target is not the begin of a word.
				TARGET_LAST_IS_NOT_EOW = 0x0100			///< Specifies that the end of the target is not the end of a word.
			};
			typedef manah::Flags<SyntaxOption> SyntaxOptions;
			typedef manah::Flags<MatchOption> MatchOptions;
			// constructors
			Pattern(const Char* first, const Char* last, const SyntaxOptions& options = NORMAL);
			Pattern(const String& pattern, const SyntaxOptions& options = NORMAL);
			virtual ~Pattern() throw();
			// attributes
			std::locale	getLocale() const throw();
			String		getPatternString() const;
			void		setLocale(const std::locale& lc) throw();
			// operations
			template<typename CharacterSequence> std::auto_ptr<MatchResult<CharacterSequence> >
				matches(CharacterSequence first, CharacterSequence last, const MatchOptions& flags = NONE) const;
			template<typename CharacterSequence> std::auto_ptr<MatchResult<CharacterSequence> >
				search(CharacterSequence first, CharacterSequence last, const MatchOptions& flags = NONE) const;
		protected:
			Pattern(const Char* first, const Char* last,
				const manah::Flags<SyntaxOption>& options, boost::regex_constants::syntax_option_type nativeSyntax);
		private:
			static boost::regex_constants::match_flag_type	translateMatchOptions(const MatchOptions& options) throw();
			boost::basic_regex<CodePoint, internal::RegexTraits> impl_;
			const SyntaxOptions options_;
		};

#ifndef ASCENSION_NO_MIGEMO
		/// Builds regular expression pattern for Migemo use.
		class MigemoPattern : public Pattern {
		public:
			static std::auto_ptr<MigemoPattern>	create(const Char* first, const Char* last, bool ignoreCase);
			static void							initialize(const char* runtimePathName, const char* dictionaryPathName);
			static bool							isMigemoInstalled() throw();
		private:
			MigemoPattern(const Char* first, const Char* last, bool ignoreCase);
			static void	install();
			static manah::AutoBuffer<char> runtimePathName_, dictionaryPathName_;
		};
#endif /* !ASCENSION_NO_MIGEMO */


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

		/// Returns the end of the matched substring.
		template<typename CharacterSequence> inline CharacterSequence MatchResult<CharacterSequence>::getEnd() const {assert((*impl_)[0].matched); return (*impl_)[0].second;}

		/**
		 * Replaces the matched range by the specified replacement pattern.
		 * @param first the start of the replacement string
		 * @param last the end of the replacement string
		 * @return the replaced string
		 */
		template<typename CharacterSequence>
		inline String MatchResult<CharacterSequence>::replace(const Char* first, const Char* last) const {
			const std::basic_string<CodePoint> temp(impl_->format(std::basic_string<CodePoint>(
				unicode::UTF16To32Iterator<>(first, last), unicode::UTF16To32Iterator<>(first, last, last))));
			return String(unicode::UTF32To16Iterator<>(temp.data()), unicode::UTF32To16Iterator<>(temp.data() + temp.length()));}

		/// Returns the string of the matched sub-expression.
		template<typename CharacterSequence>
		inline String MatchResult<CharacterSequence>::getGroup(int group) const {
			if(!(*impl_)[group].matched) throw std::out_of_range("the specified group is not matched.");
			std::basic_string<CodePoint> s(impl_->str());
			return String(unicode::UTF32To16Iterator<>(s.data()), unicode::UTF32To16Iterator<>(s.data() + s.length()));}

		/// Returns the end of the matched sub-expression.
		template<typename CharacterSequence>
		inline CharacterSequence MatchResult<CharacterSequence>::getGroupEnd(int group) const {
			if(!(*impl_)[group].matched) throw std::out_of_range("the specified group is not matched."); return (*impl_)[group].second.tell();}

		/// Returns the start of the matched sub-expression.
		template<typename CharacterSequence>
		inline CharacterSequence MatchResult<CharacterSequence>::getGroupStart(int group) const {
			if(!(*impl_)[group].matched) throw std::out_of_range("the specified group is not matched."); return (*impl_)[group].first.tell();}

		/// Returns the number of the matched sub-expressions.
		template<typename CharacterSequence>
		inline std::size_t MatchResult<CharacterSequence>::getNumberOfGroups() const {return impl_->size();}

		/// Returns the start of the matched substring.
		template<typename CharacterSequence>
		inline CharacterSequence MatchResult<CharacterSequence>::getStart() const {assert((*impl_)[0].matched); return (*impl_)[0].first;}

		/// Returns the active locale.
		inline std::locale Pattern::getLocale() const throw() {return impl_.getloc();}

		/// Returns the pattern string.
		inline String Pattern::getPatternString() const {
			const std::basic_string<CodePoint> s(impl_.str());
			return String(unicode::UTF32To16Iterator<>(s.data()), unicode::UTF32To16Iterator<>(s.data() + s.length()));
		}

		/**
		 * Determines whether the pattern matches the specified string.
		 * @param first the start of the target string
		 * @param last the end of the target string
		 * @param options the match options
		 * @return pointer to the result or @c null if the match is failed
		 * @throw std#runtime_error an error occured in the regular expression match (see @c boost#regex_match)
		 */
		template<typename CharacterSequence> inline std::auto_ptr<MatchResult<CharacterSequence> >
		Pattern::matches(CharacterSequence first, CharacterSequence last, const MatchOptions& options /* = NONE */) const {
			std::auto_ptr<boost::match_results<CharacterSequence> > p(new boost::match_results<CharacterSequence>);
			return std::auto_ptr<MatchResult<CharacterSequence> >(boost::regex_match(
				first, last, *p, impl_, translateMatchOptions(options)) ?new MatchResult<CharacterSequence>(p) : 0);}

		/**
		 * Searches the specified target string for the pattern.
		 * @param first the start of the target string
		 * @param last the end of the target string
		 * @param options the search options
		 * @return pointer to the result or @c null if the search is failed
		 * @throw std#runtime_error an error occured in the regular expression search (see @c boost#regex_search)
		 */
		template<typename CharacterSequence> inline std::auto_ptr<MatchResult<CharacterSequence> >
		Pattern::search(CharacterSequence first, CharacterSequence last, const MatchOptions& options /* = NONE */) const {
			std::auto_ptr<boost::match_results<CharacterSequence> > p(new boost::match_results<CharacterSequence>);
			return std::auto_ptr<MatchResult<CharacterSequence> >(boost::regex_search(first, last,
				*p, impl_, translateMatchOptions(options)) ? new MatchResult<CharacterSequence>(p) : 0);}

		/// Sets the active locale.
		inline void Pattern::setLocale(const std::locale& lc) throw() {impl_.imbue(lc);}

		inline boost::regex_constants::match_flag_type Pattern::translateMatchOptions(const MatchOptions& flags) throw() {
			using namespace boost::regex_constants;
			return static_cast<match_flag_type>(
				(flags.has(MULTILINE) ? 0 : match_single_line)
				| (flags.has(DOTALL) ? 0 : match_not_dot_newline)
				| (flags.has(MATCH_AT_ONLY_TARGET_FIRST) ? match_continuous : 0)
				| (flags.has(TARGET_FIRST_IS_NOT_BOB) ? match_not_bob | match_prev_avail : 0)
				| (flags.has(TARGET_LAST_IS_NOT_EOB) ? match_not_eob : 0)
				| (flags.has(TARGET_FIRST_IS_NOT_BOL) ? match_not_bol | match_prev_avail : 0)
				| (flags.has(TARGET_LAST_IS_NOT_EOL) ? match_not_eol : 0)
				| (flags.has(TARGET_FIRST_IS_NOT_BOW) ? match_not_bow | match_prev_avail : 0)
				| (flags.has(TARGET_LAST_IS_NOT_EOW) ? match_not_eow : 0));
		}

	}
}

#endif /* !ASCENSION_REGEX_HPP */
#endif /* !ASCENSION_NO_REGEX */
