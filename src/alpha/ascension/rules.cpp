/**
 * @file rules.cpp
 * @author exeal
 * @date 2004-2006 (was Lexer.cpp)
 * @date 2006-2007
 */

#include "stdafx.h"
#include "rules.hpp"
using namespace ascension;
using namespace ascension::rules;
using namespace ascension::text;
using namespace ascension::unicode;
using namespace std;
using namespace manah;
using rules::internal::HashTable;


// internal::HashTable //////////////////////////////////////////////////////

struct HashTable::Entry {
	String data;
	Entry* next;
	explicit Entry(const String& str) throw() : data(str) {}
	~Entry() throw() {delete next;}
};

/**
 * Constructor.
 * @param first the start of the strings
 * @param last the end of the strings
 * @param caseSensitive set true to enable case sensitive match
 */
HashTable::HashTable(const String* first, const String* last, bool caseSensitive)
		: size_(last - first), maxLength_(0), caseSensitive_(caseSensitive) {
	entries_ = new Entry*[size_];
	fill<Entry**, Entry*>(entries_, entries_ + size_, 0);
	while(first < last) {
		Entry* const newEntry = new Entry(caseSensitive_ ? *first : CaseFolder::fold(first->data(), first->data() + first->length()));
		const size_t h = getHashCode(newEntry->data);
		if(first->length() > maxLength_)
			maxLength_ = first->length();
		newEntry->next = (entries_[h % size_] != 0) ? entries_[h % size_] : 0;
		entries_[h % size_] = newEntry;
		++first;
	}
}

/// Destructor.
HashTable::~HashTable() {
	for(size_t i = 0; i < size_; ++i)
		delete entries_[i];
	delete[] entries_;
}

/**
 * Searches the specified string.
 * @param first the start of the string
 * @param last the end of the string
 * @return true if the specified string is found
 */
bool HashTable::find(const Char* first, const Char* last) const {
	if(static_cast<size_t>(last - first) > maxLength_)	// TODO: this code can't trap UCS4 and full case foldings
		return false;

	const String folded = caseSensitive_ ? first : CaseFolder::fold(first, last);
	const size_t h = getHashCode(folded);
	Entry* entry = entries_[h % size_];
	bool found = false;

	while(entry != 0) {
		if(entry->data.length() == folded.length() && wmemcmp(entry->data.data(), folded.data(), folded.length()) == 0) {
			found = true;
			break;
		}
		entry = entry->next;
	}
	return found;
}

/**
 * Returns the hash value of the specified string.
 * @param s the string to retrieve a hash value
 * @return the hash value
 */
inline ulong HashTable::getHashCode(const String& s) {
	ulong h = 0;
	for(length_t i = 0; i < s.length(); ++i) {
		h *= 2;
		h += s[i];
	}
	return h;
}


// URIDetector //////////////////////////////////////////////////////////////

/**
 * 文字列がメールアドレスかを調べる
 * @param first, last 調べる文字列
 * @param asIRI UCS 文字を認めるか (未実装)
 * @return メールアドレスの終端
 */
const Char* URIDetector::eatMailAddress(const Char* first, const Char* last, bool) {
	// このメソッドは "/[\w\d][\w\d\.\-_]*@[\w\d\-_]+(\.[\w\d\-_]+)+/" のようなパターンマッチを行う
#define IS_ALNUM(ch)					\
	(((ch) >= L'A' && (ch) <= L'Z')		\
	|| ((ch) >= L'a' && (ch) <= L'z')	\
	|| ((ch) >= L'0' && (ch) <= L'9'))
#define IS_ALNUMBAR(ch)	\
	(IS_ALNUM(ch) || ch == L'-' || ch == L'_')

	if(last - first < 5)
		return first;

	// 1文字目
	if(!IS_ALNUM(*first))
		return first;

	// 2文字目から '@'
	const Char* const originalFirst = first++;
	for(; first < last - 3; ++first) {
		if(!IS_ALNUMBAR(*first) && *first != L'.')
			break;
	}
	if(*first != L'@' || last - first == 3)
		return originalFirst;

	// '@' の後ろ
	const Char* const atMark = first;
	bool dotAppeared = false;
	for(first = atMark + 1; first < last; ++first) {
		if(IS_ALNUMBAR(*first))
			continue;
		else if(*first == L'.') {
			if(first[-1] == L'.')
				return originalFirst;
			dotAppeared = true;
		} else
			break;
	}
	return (dotAppeared && (first - atMark > 2)) ? first : originalFirst;
}

/**
 * @brief URL 文字列を調べる
 *
 * 現時点では以下の文字列を URL の開始とみなす:
 * <ul>
 *   <li>file://</li><li>ftp://</li><li>gopher://</li><li>http://</li><li>https://</li>
 *   <li>mailto://</li><li>news://</li><li>nntp://</li><li>telnet://</li><li>wais://</li>
 * </ul>
 *
 * @param first, last 調べる文字列
 * @param asIRI UCS 文字を認めるか (未実装)
 * @return URL の終端
 */
const Char* URIDetector::eatURL(const Char* first, const Char* last, bool) {
#define STARTS_WITH(prefix, len)	\
	(len < last - first && wmemcmp(first, prefix, len) == 0 && (urlLength = len - 1))

	static const bool urlChars[] = {	// URI 構成文字か
		false,	false,	false,	false,	false,	false,	false,	false,	// 0x00
		false,	false,	false,	false,	false,	false,	false,	false,
		false,	false,	false,	false,	false,	false,	false,	false,	// 0x10
		false,	false,	false,	false,	false,	false,	false,	false,
		false,	true,	false,	true,	true,	true,	true,	false,	// 0x20
		false,	false,	false,	true,	true,	true,	true,	true,
		true,	true,	true,	true,	true,	true,	true,	true,	// 0x30
		true,	true,	true,	true,	false,	true,	false,	true,
		true,	true,	true,	true,	true,	true,	true,	true,	// 0x40
		true,	true,	true,	true,	true,	true,	true,	true,
		true,	true,	true,	true,	true,	true,	true,	true,	// 0x50
		true,	true,	true,	false,	true,	false,	false,	true,
		false,	true,	true,	true,	true,	true,	true,	true,	// 0x60
		true,	true,	true,	true,	true,	true,	true,	true,
		true,	true,	true,	true,	true,	true,	true,	true,	// 0x70
		true,	true,	true,	false,	false,	false,	true,	false
	};

	if(!urlChars[first[0] & 0x00FF] || last - first < 6)
		return first;
	length_t urlLength;
	if(STARTS_WITH(L"file://", 7)
			|| STARTS_WITH(L"ftp://", 6)
			|| STARTS_WITH(L"gopher://", 9)
			|| STARTS_WITH(L"http://", 7)
			|| STARTS_WITH(L"https://", 8)
			|| STARTS_WITH(L"mailto://", 9)
			|| STARTS_WITH(L"news://", 7)
			|| STARTS_WITH(L"nntp://", 7)
			|| STARTS_WITH(L"telnet://", 9)
			|| STARTS_WITH(L"wais://", 7)) {
		for(++urlLength; urlLength < static_cast<String::size_type>(last - first); ++urlLength) {
			if(first[urlLength] > 0x007F || !urlChars[first[urlLength] & 0x00FF])
				return first + urlLength;
		}
		return last;
	}
	return first;

#undef STARTS_WITH
}


// Token ////////////////////////////////////////////////////////////////////

const Token::ID Token::NULL_ID = 0;
const Token::ID Token::UNCALCULATED = -1;
const Token Token::END_OF_SCOPE;


// Rule /////////////////////////////////////////////////////////////////////

/**
 * Protected constructor.
 * @param tokenID the identifier of the token which will be returned by the rule. can be @c Token#NULL_ID
 * @param caseSensitive set false to enable caseless match
 */
Rule::Rule(Token::ID tokenID, bool caseSensitive) throw() : id_(tokenID), caseSensitive_(caseSensitive) {
}


// RegionRule ///////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param id the identifier of the token which will be returned by the rule
 * @param startSequence the pattern's start sequence
 * @param endSequence the pattern's end sequence. if empty, token will end at end of line
 * @param escapeCharacter the character which a character will be ignored
 * @param caseSensitive set false to enable caseless match
 * @throw std#invalid_argument @a startSequence is empty
 */
RegionRule::RegionRule(Token::ID id, const String& startSequence, const String& endSequence,
		Char escapeCharacter /* = NONCHARACTER */, bool caseSensitive /* = true */)
		: Rule(id, caseSensitive), startSequence_(startSequence), endSequence_(endSequence), escapeCharacter_(escapeCharacter) {
	if(startSequence.empty())
		throw invalid_argument("the start sequence is empty.");
}

/// @see Rule#parse
auto_ptr<Token> RegionRule::parse(const TokenScanner& scanner, const Char* first, const Char* last) const throw() {
	// 開始シーケンスのマッチ
	if(first[0] != startSequence_[0]
			|| static_cast<size_t>(last - first) < startSequence_.length() + endSequence_.length()
			|| (startSequence_.length() > 1 && wmemcmp(first + 1, startSequence_.data() + 1, startSequence_.length() - 1) != 0))
		return auto_ptr<Token>(0);
	const Char* end = last;
	if(!endSequence_.empty()) {
		// 終了シーケンスの検索
		for(const Char* p = first + startSequence_.length(); p <= last - endSequence_.length(); ++p) {
			if(escapeCharacter_ != NONCHARACTER && *p == escapeCharacter_)
				++p;
			else if(*p == endSequence_[0] && wmemcmp(p + 1, endSequence_.data() + 1, endSequence_.length() - 1) == 0) {
				end = p + endSequence_.length();
				break;
			}
		}
	}
	auto_ptr<Token> result(new Token);
	result->id = getTokenID();
	result->region.first.line = result->region.second.line = scanner.getPosition().line;
	result->region.first.column = scanner.getPosition().line;
	result->region.second.column = result->region.first.column + (end - first);
	return result;
}


// WordRule /////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param id the identifier of the token which will be returned by the rule
 * @param first the start of the words
 * @param last the end of the words
 * @param caseSensitive set false to enable caseless match
 * @throw std#invalid_argument @a first and/or @a last are @c null
 */
WordRule::WordRule(Token::ID id, const String* first, const String* last,
		bool caseSensitive /* = true */) : Rule(id, caseSensitive), words_(first, last, caseSensitive) {
}

/// @see SingleLineRule#parse
auto_ptr<Token> WordRule::parse(const TokenScanner& scanner, const Char* first, const Char* last) const {
	if(!words_.find(first, last))
		return auto_ptr<Token>(0);
	auto_ptr<Token> result(new Token);
	result->id = getTokenID();
	result->region.first.line = result->region.second.line = scanner.getPosition().line;
	result->region.first.column = scanner.getPosition().column;
	result->region.second.column = result->region.first.column + (last - first);
	return result;
}


#ifndef ASCENSION_NO_REGEX

// RegexRule ////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param id the identifier of the token which will be returned by the rule
 * @param pattern the pattern string
 * @param caseSensitive set false to enable caseless match
 * @throw boost#regex_error the specified pattern is invalid
 */
RegexRule::RegexRule(Token::ID id, const String& pattern, bool caseSensitive /* = true */)
		: Rule(id, caseSensitive), pattern_(pattern.data(), pattern.data() + pattern.length()) {
}

/// @see SingleLineRule#parse
auto_ptr<Token> RegexRule::parse(const TokenScanner& scanner, const Char* first, const Char* last) const {
	auto_ptr<regex::MatchResult<const Char*> > r(pattern_.search(first, last,
		regex::Pattern::MATCH_AT_ONLY_TARGET_FIRST | (scanner.getPosition().column != 0 ? regex::Pattern::TARGET_FIRST_IS_NOT_BOL : 0)));
	if(r.get() == 0)
		return auto_ptr<Token>(0);
	auto_ptr<Token> result(new Token);
	result->id = getTokenID();
	result->region.first.line = result->region.second.line = scanner.getPosition().line;
	result->region.first.column = scanner.getPosition().column;
	result->region.second.column = result->region.first.column + (r->getEnd() - r->getStart());
	return result;
}

#endif /* !ASCENSION_NO_REGEX */


// TokenScanner /////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param identifierSyntax
 */
TokenScanner::TokenScanner(const IdentifierSyntax& identifierSyntax) throw() : idSyntax_(identifierSyntax), current_() {
}

/// Destructor.
TokenScanner::~TokenScanner() throw() {
	for(list<const Rule*>::iterator i = rules_.begin(); i != rules_.end(); ++i)
		delete *i;
	for(list<const WordRule*>::iterator i = wordRules_.begin(); i != wordRules_.end(); ++i)
		delete *i;
}

/**
 * Adds the new rule to the scanner.
 * @param rule the rule to be added
 * @throw std#invalid_argument @a rule is @c null or already registered
 * @throw BadScannerStateException the scanner is running
 */
void TokenScanner::addRule(auto_ptr<const Rule> rule) {
	if(rule.get() == 0)
		throw invalid_argument("the rule is null.");
	else if(!isDone())
		throw BadScannerStateException();
	else if(find(rules_.begin(), rules_.end(), rule.get()) != rules_.end())
		throw invalid_argument("the rule is already registered.");
	rules_.push_back(rule.release());
}

/**
 * Adds the new word rule to the scanner.
 * @param rule the rule to be added
 * @throw std#invalid_argument @a rule is @c null or already registered
 * @throw BadScannerStateException the scanner is running
 */
void TokenScanner::addRule(auto_ptr<const WordRule> rule) {
	if(rule.get() == 0)
		throw invalid_argument("the rule is null.");
	else if(!isDone())
		throw BadScannerStateException();
	else if(find(wordRules_.begin(), wordRules_.end(), rule.get()) != wordRules_.end())
		throw invalid_argument("the rule is already registered.");
	wordRules_.push_back(rule.release());
}

/// Returns the current scanning position.
const Position& TokenScanner::getPosition() const {
	return current_.tell().tell();
}

/// Returns true if the scanning is done.
bool TokenScanner::isDone() const throw() {
	return current_.isLast();
}

/**
 * Moves to the next token and returns it.
 * @return the found token or @c null if the scan is done
 */
auto_ptr<Token> TokenScanner::nextToken() throw() {
	auto_ptr<Token> result;
	const String* line = &current_.tell().getLine();
	for(; !current_.isLast(); ++current_) {
		if(*current_ == LINE_SEPARATOR)
			line = &(++current_).tell().getLine();
		const Char* const p = line->data() + current_.tell().tell().column;
		const Char* const last = line->data() + line->length();
		for(list<const Rule*>::const_iterator i = rules_.begin(); i != rules_.end(); ++i) {
			result = (*i)->parse(*this, p, last);
			if(result.get() != 0) {
				current_.tell().seek(result->region.getBottom());
				return result;
			}
		}
		if(!wordRules_.empty()) {
			const Char* const wordEnd = idSyntax_.eatIdentifier(p, last);
			if(wordEnd > p) {
				for(list<const WordRule*>::const_iterator i = wordRules_.begin(); i != wordRules_.end(); ++i) {
					result = (*i)->parse(*this, p, wordEnd);
					if(result.get() != 0) {
						current_.tell().seek(result->region.getBottom());
						return result;
					}
				}
			}
		}
	}
	return result;
}

/**
 * Starts the scan with the specified range.
 * @param document the document
 * @param region the region to be scanned
 * @throw text#BadRegionException @a region intersects outside of the document
 */
void TokenScanner::parse(const Document& document, const Region& region) {
	current_ = UTF16To32Iterator<DocumentCharacterIterator,
		utf16boundary::BASE_KNOWS_BOUNDARIES>(DocumentCharacterIterator(document, region.getTop()));
}


// TransitionRule ///////////////////////////////////////////////////////////

/**
 * Protected constructor.
 * @param contentType the content type of the transition source
 * @param destination the content type of the transition destination
 */
TransitionRule::TransitionRule(ContentType contentType, ContentType destination) throw() : contentType_(contentType), destination_(destination) {
}

/// Destructor.
TransitionRule::~TransitionRule() throw() {
}

/**
 * @fn TransitionRule::matches
 * Returns true if the rule matches the specified text.
 * @param line the target line text
 * @param column the column number at which match starts
 * @return the length of the matched pattern. if and only if the match failed, returns 0.
 * if matched zero width text, returns 1
 */


// LiteralTransitionRule ////////////////////////////////////////////////////

/**
 * Constructor.
 * @param contentType the content type of the transition source
 * @param destination the content type of the transition destination
 * @param pattern the pattern string to introduce the transition.
 * if empty string is specified, the transition will be occured at the end of line
 * @param escapeCharacter the character which a character will be ignored. if @c NONCHARACTER is
 * specified, the escape character will be not set. this is always case-sensitive
 * @param caseSensitive set false to enable caseless match
 */
LiteralTransitionRule::LiteralTransitionRule(ContentType contentType, ContentType destination,
		const String& pattern, Char escapeCharacter /* = NONCHARACTER */, bool caseSensitive /* = true */) :
		TransitionRule(contentType, destination), pattern_(pattern), escapeCharacter_(escapeCharacter), caseSensitive_(caseSensitive) {
}

/// @see TransitionRule#matches
length_t LiteralTransitionRule::matches(const String& line, length_t column) const {
	if(escapeCharacter_ != NONCHARACTER && column > 0 && line[column - 1] == escapeCharacter_)
		return 0;
	else if(pattern_.empty() && column == line.length())	// matches EOL
		return 1;
	else if(line.length() - column < pattern_.length())
		return 0;
	else if(caseSensitive_)
		return (wmemcmp(pattern_.data(), line.data() + column, pattern_.length()) == 0) ? pattern_.length() : 0;
	return (CaseFolder::compare(StringCharacterIterator(pattern_),
		StringCharacterIterator(line, line.begin() + column)) == 0) ? pattern_.length() : 0;
}


#ifndef ASCENSION_NO_REGEX

// RegexTransitionRule //////////////////////////////////////////////////////

/**
 * Constructor.
 * @param contentType the content type of the transition source
 * @param destination the content type of the transition destination
 * @param pattern the pattern string to introduce the transition. can't be empty
 * @param caseSensitive set false to enable caseless match
 * @throw boost#regex_error @a pattern is invalid
 */
RegexTransitionRule::RegexTransitionRule(ContentType contentType, ContentType destination, const String& pattern,
		bool caseSensitive /* = true */) : TransitionRule(contentType, destination), 
		pattern_(pattern.data(), pattern.data() + pattern.length(), caseSensitive ? regex::Pattern::NORMAL : regex::Pattern::CASE_INSENSITIVE) {
}

/// @see TransitionRule#matches
length_t RegexTransitionRule::matches(const String& line, length_t column) const {
	try {
		using namespace regex;
		Pattern::MatchOptions flags = Pattern::MATCH_AT_ONLY_TARGET_FIRST;
		if(column != 0)
			flags |= Pattern::TARGET_FIRST_IS_NOT_BOL;
		auto_ptr<MatchResult<const Char*> > result(pattern_.search(line.data() + column, line.data() + line.length(), flags));
		return (result.get() != 0) ? max(result->getEnd() - result->getStart(), 1) : 0;
	} catch(runtime_error&) {
		return 0;
	}
}

#endif /* !ASCENSION_NO_REGEX */


// LexicalPartitioner ///////////////////////////////////////////////////////

/// Constructor.
LexicalPartitioner::LexicalPartitioner() throw() {
}

/// Destructor.
LexicalPartitioner::~LexicalPartitioner() throw() {
	clearRules();
}

/// Deletes all the transition rules.
void LexicalPartitioner::clearRules() throw() {
	for(list<const TransitionRule*>::const_iterator i(rules_.begin()); i != rules_.end(); ++i)
		delete *i;
	rules_.clear();
}

/**
 * Computes and constructs the partitions on the specified region.
 * @param start the start of the region to compute
 * @param minimalLast the partitioner must scan to this position at least
 * @param[out] changedRegion the region whose content type was changed
 */
void LexicalPartitioner::computePartitioning(const Position& start, const Position& minimalLast, Region& changedRegion) {
	// TODO: see LexicalPartitioner.documentChanged.
}

/// @see text#DocumentPartitioner#documentAboutToBeChanged
void LexicalPartitioner::documentAboutToBeChanged() throw() {
}

/// @see text#DocumentPartitioner#documentChanged
void LexicalPartitioner::documentChanged(const DocumentChange& change) throw() {
	// TODO: there is more efficient implementation using LexicalPartitioner.computePartitioning.
	const Document& document = *getDocument();
	const Position eof(document.getEndPosition(false));
	Position affectedEnd(change.getRegion().getBottom());	// the end of the region to scan. the scanning will end if reached this

	// delete the partitions start at the deleted region
	DocumentCharacterIterator p(document, Position(change.getRegion().getTop().line, 0));
	assert(!change.getRegion().isEmpty());
	// locate the first partition to delete
	size_t predeletedFirst = findClosestPartition(p.tell());
	if(p.tell() >= partitions_[predeletedFirst]->getTokenEnd())
		++predeletedFirst;	// do not delete this partition
//	else if(predeletedFirst < partitions_.getSize() - 1 && partitions_[predeletedFirst + 1]->tokenStart < affectedEnd)
//		++predeletedFirst;	// delete from the next partition
	// locate the last partition to delete
	const Position& e = change.isDeletion() ? affectedEnd : change.getRegion().getTop();
	size_t predeletedLast = findClosestPartition(e) + 1;	// exclusive
	if(predeletedLast < partitions_.getSize() && partitions_[predeletedLast]->tokenStart < affectedEnd)
		++predeletedLast;
//	else if(partitions_[predeletedLast - 1]->start == affectedEnd)
//		--predeletedLast;
	ContentType affectedEndContentType =
		getTransitionStateAt((predeletedLast < partitions_.getSize()) ? partitions_[predeletedLast]->start : eof);
	if(predeletedLast > predeletedFirst)
		partitions_.erase(predeletedFirst, predeletedLast - predeletedFirst);

	// move the partitions adapting to the document change
	size_t numberOfPartitions = partitions_.getSize();
	for(size_t i = 0; i < numberOfPartitions; ++i) {
		partitions_[i]->start = updatePosition(partitions_[i]->start, change, FORWARD);
		partitions_[i]->tokenStart = updatePosition(partitions_[i]->tokenStart, change, FORWARD);
	}
	if(change.isDeletion())
		affectedEnd = (predeletedFirst < partitions_.getSize()) ? partitions_[predeletedFirst]->start : eof;
	else {
		if(predeletedFirst == partitions_.getSize())
			affectedEnd = eof;
		else if(partitions_[predeletedFirst]->start > affectedEnd)
			affectedEnd = partitions_[predeletedFirst]->start;
	}

	// push a default partition if the partition includes the start of the document
	if(numberOfPartitions == 0 || partitions_[0]->start != document.getStartPosition(false)) {
		if(numberOfPartitions == 0 || partitions_[0]->contentType != DEFAULT_CONTENT_TYPE) {
			partitions_.insert(0, new Partition(DEFAULT_CONTENT_TYPE, Position::ZERO_POSITION, Position::ZERO_POSITION, 0));
			++numberOfPartitions;
		} else {
			partitions_[0]->start = partitions_[0]->tokenStart = document.getStartPosition(false);
			partitions_[0]->tokenLength = 0;
		}
	}

	// delete the partition whose start position is the end of the document
	if(numberOfPartitions > 1 && partitions_[numberOfPartitions - 1]->start == document.getEndPosition(false))
		partitions_.erase(numberOfPartitions - 1);

	// reconstruct partitions in the affected region
	const String* line = &document.getLine(p.tell().line);
	size_t partition = findClosestPartition(p.tell());
	ContentType contentType = partitions_[partition]->contentType, destination;
	while(true) {	// scan and tokenize into partitions...
		// if reached the end of the affected region and content types are same, we are done
		if(p.tell() == eof || (p.tell() == affectedEnd && contentType == affectedEndContentType))
			break;
		const bool eol = p.tell().column == line->length();
		length_t tokenLength = tryTransition(*line, p.tell().column, contentType, destination);
		if(tokenLength != 0) {
			// insert the new partition
			if(eol)
				tokenLength = 0;	// a terminator is zero-length...
			++partition;
			const Position tokenEnd = Position(p.tell().line, p.tell().column + tokenLength);
			if(p.tell() == partitions_[0]->start && p.tell() == Position::ZERO_POSITION) {
				partitions_.erase(0);
				--partition;
			}
			partitions_.insert(partition,
				new Partition(destination, (destination > contentType) ? p.tell() : tokenEnd, p.tell(), tokenLength));
			contentType = destination;
			if(!eol)
				p.seek(tokenEnd);
		}
		// go to the next character if no transition occured
		if(tokenLength == 0 && (++p).tell().column == 0)
			line = &document.getLine(p.tell().line);
		// if over the end of the affect region, erase overriden and update affectedEnd
		if(partition < partitions_.getSize() - 1 && p.tell() > partitions_[partition + 1]->tokenStart) {
			size_t deletedLast = findClosestPartition(p.tell()) + 1;	// exclusive
			if(p.tell() == partitions_[deletedLast - 1]->start)
				--deletedLast;
			else if(deletedLast < partitions_.getSize() && partitions_[deletedLast]->tokenStart < p.tell())
				++deletedLast;
			if(deletedLast > partition + 1) {
				// obtain information at the affected end before delete
				affectedEnd = (deletedLast < partitions_.getSize()) ? partitions_[deletedLast]->start : eof;
				affectedEndContentType = getTransitionStateAt(affectedEnd);
				partitions_.erase(partition + 1, deletedLast - partition - 1);
			}
		}
	}
	notifyDocument(Region(Position(change.getRegion().getTop().line, 0), p.tell()));

#ifdef _DEBUG
	// diagnose partitions
	for(size_t i = 0, e = partitions_.getSize(); i < e - 1; ++i) {
		assert(partitions_[i]->contentType != partitions_[i + 1]->contentType);
		assert(partitions_[i]->start < partitions_[i + 1]->start);
	}
#endif /* _DEBUG */
}

/// @see text#DocumentPartitioner#doGetPartition
void LexicalPartitioner::doGetPartition(const Position& at, DocumentPartition& partition) const throw() {
	const size_t i = findClosestPartition(at);
	const Partition& p = *partitions_[i];
	partition.contentType = p.contentType;
	partition.region.first = p.start;
	partition.region.second = (i < partitions_.getSize() - 1) ? partitions_[i + 1]->start : getDocument()->getEndPosition(false);
}

/// @see text#DocumentPartitioner#doInstall
void LexicalPartitioner::doInstall() throw() {
	partitions_.clear();
	partitions_.insert(0, new Partition(DEFAULT_CONTENT_TYPE, Position::ZERO_POSITION, Position::ZERO_POSITION, 0));
	Region dummy;
	computePartitioning(getDocument()->getStartPosition(false), getDocument()->getEndPosition(false), dummy);
}

void LexicalPartitioner::dump() const {
#ifdef _DEBUG
	win32::DumpContext dout;
	dout << "LexicalPartitioner dump start:\n";
	for(size_t i = 0; i < partitions_.getSize(); ++i) {
		const Partition& p = *partitions_[i];
		dout << "\t" << p.contentType << " = ("
			<< static_cast<ulong>(p.start.line) << ", " << static_cast<ulong>(p.start.column) << ")\n";
	}
#endif /* _DEBUG */
}

inline size_t LexicalPartitioner::findClosestPartition(const Position& at) const throw() {
	size_t result = ascension::internal::searchBound(
		static_cast<size_t>(0), partitions_.getSize(), at, bind1st(mem_fun(LexicalPartitioner::getPartitionStart), this));
	return (partitions_[result]->tokenStart == at && result > 0 && at.column == getDocument()->getLineLength(at.line)) ? result - 1 : result;
}

inline ContentType LexicalPartitioner::getTransitionStateAt(const Position& at) const throw() {
	if(at == Position::ZERO_POSITION)
		return DEFAULT_CONTENT_TYPE;
	size_t i = findClosestPartition(at);
	if(partitions_[i]->start == at)
		--i;
	return partitions_[i]->contentType;
}

/**
 *
 * @param line the scanning line text
 * @param column the column number at which match starts
 * @param contentType the current content type
 * @param[out] destination the type of the transition destination content
 * @return the length of the pattern matched or 0 if the all rules did not matched
 */
inline length_t LexicalPartitioner::tryTransition(
		const String& line, length_t column, ContentType contentType, ContentType& destination) const throw() {
	for(TransitionRules::const_iterator rule(rules_.begin()), e(rules_.end()); rule != e; ++rule) {
		if((*rule)->getContentType() == contentType) {
			if(const length_t c = (*rule)->matches(line, column)) {
				destination = (*rule)->getDestination();
				return c;
			}
		}
	}
	destination = UNDETERMINED_CONTENT_TYPE;
	return 0;
}
