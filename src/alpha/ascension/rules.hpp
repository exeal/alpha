/**
 * @file rules.hpp
 * @author exeal
 * @date 2004-2006 (was Lexer.h)
 * @date 2006-2007
 */

#ifndef ASCENSION_RULES_HPP
#define ASCENSION_RULES_HPP
#include "document.hpp"
#include "regex.hpp"
#include <set>
#include <list>

namespace ascension {

	/// Provides a framework for rule based text scanning and document partitioning.
	namespace rules {

		namespace internal {
			class TokenListReconstructor {
			public:
				void	reconstruct(const text::Region& scope);
			private:
			};
			class HashTable {
			public:
				HashTable(const String* first, const String* last, bool caseSensitive) throw();
				~HashTable() throw();
				bool find(const Char* first, const Char* last) const;
				static ulong getHashCode(const String& s) throw();
			private:
				struct Entry;
				Entry** entries_;
				const std::size_t size_;
				std::size_t maxLength_;	// 最長キーワード長さ
				const bool caseSensitive_;
			};
		} // namespace internal

		/**
		 * @c URIDetector detects and searches URI.
		 * Although this should conform to the syntaxes of RFC 3986 and RFC 3987, currently not.
		 */
		class URIDetector {
		public:
			static const Char*	eatMailAddress(const Char* first, const Char* last, bool asIRI);
			static const Char*	eatURL(const Char* first, const Char* last, bool asIRI);
			static void			setSchemes(const std::set<String>& schemes);
		};

		/**
		 * 
		 * @see token#TokenScanner, partition#PartitionScanner
		 */
		class BadScannerStateException : public std::logic_error {
		public:
			BadScannerStateException() : std::logic_error("The scanner can't accept the requested operation in this state.") {}
		};

		struct Token : public manah::FastArenaObject<Token> {
			typedef ushort ID;
			static const ID NULL_ID, UNCALCULATED;
			static const Token END_OF_SCOPE;
			ID id;
			text::Region region;
		};

		struct TokenList {
			std::size_t count;
			Token* array;
		};

		class TokenScanner;

		/**
		 * Base class for concrete rule classes.
		 * @see RegionRule, WordRule, RegexRule
		 */
		class Rule {
		public:
			/// Returns the identifier of the token.
			Token::ID getTokenID() const throw() {return id_;}
			/// Returns true if the match is case sensitive.
			bool isCaseSensitive() const throw() {return caseSensitive_;}
			/**
			 */
			virtual std::auto_ptr<Token> parse(const TokenScanner& scanner, const Char* first, const Char* last) const throw() = 0;
		protected:
			explicit Rule(Token::ID tokenID, bool caseSensitive) throw();
		private:
			const Token::ID id_;
			const bool caseSensitive_;
		};

		/***/
		class RegionRule : public Rule {
		public:
			RegionRule(Token::ID id, const String& startSequence,
				const String& endSequence, Char escapeCharacter = NONCHARACTER, bool caseSensitive = true);
			std::auto_ptr<Token>	parse(const TokenScanner& scanner, const Char* first, const Char* last) const throw();
		private:
			const String startSequence_, endSequence_;
			const Char escapeCharacter_;
		};

		/***/
		class WordRule : protected Rule {
		public:
			WordRule(Token::ID id, const String* first, const String* last, bool caseSensitive = true);
			std::auto_ptr<Token>	parse(const TokenScanner& scanner, const Char* first, const Char* last) const throw();
		private:
			internal::HashTable words_;
		};

#ifndef ASCENSION_NO_REGEX
		/***/
		class RegexRule : public Rule {
		public:
			RegexRule(Token::ID id, const String& pattern, bool caseSensitive = true);
			std::auto_ptr<Token>	parse(const TokenScanner& scanner, const Char* first, const Char* last) const throw();
		private:
			const regex::Pattern pattern_;
		};
#endif /* !ASCENSION_NO_REGEX */

		/**
		 * @c TokenScanner scans a range of document and returns the tokens it finds.
		 *
		 * This scanner is programable with a sequence of rules. The rules must be registered
		 * before start of scanning. Otherwise @c RunningScannerException will be thrown.
		 *
		 * The tokens this scanner returns are only single-line. Multi-line tokens are not
		 * supported by this class.
		 *
		 * To start scanning, call @c #parse method with a target region. And then call
		 * @c #nextToken method repeatedly to get tokens. When reached the end of the scanning
		 * region, the scanning is end.
		 */
		class TokenScanner : public manah::Noncopyable {
		public:
			// constructors
			explicit TokenScanner(const unicode::IdentifierSyntax& identifierSyntax) throw();
			// attributes
			void					addRule(std::auto_ptr<const Rule> rule);
			void					addRule(std::auto_ptr<const WordRule> rule);
			const text::Position&	getPosition() const throw();
			bool					isDone() const throw();
			// operations
			std::auto_ptr<Token>	nextToken() throw();
			void					parse(const text::Document& document, const text::Region& region);
		private:
			const unicode::IdentifierSyntax& idSyntax_;
			std::list<const Rule*> rules_;
			std::list<const WordRule*> wordRules_;
			unicode::UTF16To32Iterator<text::DocumentCharacterIterator, unicode::utf16boundary::BASE_KNOWS_BOUNDARIES> current_;
		};

		/**
		 * A rule for detecting patterns which begin new partition in document.
		 * @note This class is not dervable.
		 * @see LexicalPartitioner
		 */
		class TransitionRule {
		public:
			TransitionRule(text::ContentType contentType, text::ContentType destination, const String& pattern, bool caseSensitive = true);
			text::ContentType	getContentType() const throw();
			text::ContentType	getDestination() const throw();
			length_t			matches(const String& line, length_t column) const;
		private:
			const text::ContentType contentType_, destination_;
#ifndef ASCENSION_NO_REGEX
			const regex::Pattern pattern_;
#else
			const String pattern_;
			const bool caseSensitive_;
#endif /* !ASCENSION_NO_REGEX */
		};

		/**
		 * @c LexicalPartitioner makes document partitions by using the specified lexical rules.
		 * @note This class is not derivable.
		 * @see text#Document, PartitionScanner
		 */
		class LexicalPartitioner : public text::DocumentPartitioner {
		public:
			// constructor
			LexicalPartitioner() throw();
			~LexicalPartitioner() throw();
			// attribute
			template<class InputIterator>
			void	setRules(InputIterator first, InputIterator last);
		private:
			void		clearRules() throw();
			void		computePartitioning(const text::Position& start, const text::Position& minimalLast, text::Region& changedRegion);
			void		dump() const;
			std::size_t	findClosestPartitionIndex(const text::Position& at) const throw();
			// DocumentPartitioner
			void	documentAboutToBeChanged() throw();
			void	documentChanged(const text::DocumentChange& change) throw();
			void	doGetPartition(const text::Position& at, text::DocumentPartition& partition) const throw();
			void	doInstall() throw();
		private:
			struct Partition {
				text::ContentType contentType;
				text::Position start, introducerEnd;
				Partition(text::ContentType type, const text::Position& p,
					const text::Position& introEnd) throw() : contentType(type), start(p), introducerEnd(introEnd) {}
			};
			const text::Position& getPartitionStart(size_t partition) const throw() {return partitions_[partition]->start;}
			manah::GapBuffer<Partition*, manah::GapBuffer_DeletePointer<Partition*> > partitions_;
			typedef std::list<const TransitionRule*> TransitionRules;
			TransitionRules rules_;
			text::Position eofBeforeDocumentChange_;	// ドキュメント変更前の終端位置 (documentAboutToBeChanged で記憶)
		};


		template<class InputIterator> inline void LexicalPartitioner::setRules(InputIterator first, InputIterator last) {
			if(getDocument() != 0)
				throw std::logic_error("The partitioner is already connected to document.");
			clearRules();
			std::copy(first, last, std::back_inserter(rules_));
		}

}} // namespace ascension::rules

#endif /* !ASCENSION_RULES_HPP */
