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
 * @param column
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
 * @param caseSensitive set false to enable caseless match
 */
LiteralTransitionRule::LiteralTransitionRule(ContentType contentType, ContentType destination, const String& pattern,
		bool caseSensitive /* = true */) : TransitionRule(contentType, destination), pattern_(pattern), caseSensitive_(caseSensitive) {
}

/// @see TransitionRule#matches
length_t LiteralTransitionRule::matches(const String& line, length_t column) const {
	if(pattern_.empty() && column == line.length())	// matches EOL
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
	const Position eof(getDocument()->getEndPosition(false));
	if(start == eof) {
		changedRegion.first = changedRegion.second = Position::INVALID_POSITION;
		return;
	}
	assert(start < eof);
	Position p(start);
	const String* line = &getDocument()->getLine(start.line);
	const TransitionRules::const_iterator lastRule(rules_.end());
	size_t currentPartition = findClosestPartitionIndex(start);
	assert(currentPartition < partitions_.getSize());
	ContentType contentType = partitions_[currentPartition]->contentType;
	bool transitionOccured = false;
	changedRegion.first = Position::INVALID_POSITION;
	for(DocumentCharacterIterator i(*getDocument(), start); i.tell() != eof;) {
		const Position s(i.tell());
		// 順番に規則のマッチを試みる
		ContentType destination = UNDETERMINED_CONTENT_TYPE;
		for(TransitionRules::const_iterator rule(rules_.begin()); rule != lastRule; ++rule) {
			if((*rule)->getContentType() == contentType) {
				if(const length_t c = (*rule)->matches(*line, i.tell().column)) {
					if(i.tell().column > line->length())
						i.seek(Position(i.tell().line + 1, 0));
					else
						i.seek(Position(i.tell().line, i.tell().column + c));
					destination = (*rule)->getDestination();
					transitionOccured = true;
					break;
				}
			}
		}
		if(destination == UNDETERMINED_CONTENT_TYPE) {	// 1 つもマッチしなかった
			// 次の古いパーティションに到達した -> 貫通
			if(currentPartition + 1 < partitions_.getSize() && s == partitions_[currentPartition + 1]->start) {
				if(changedRegion.first == Position::INVALID_POSITION)
					changedRegion.first = s;
				partitions_.erase(currentPartition + 1);
				transitionOccured = true;
			}
			// 移動
			if((++i).tell().column == 0)
				line = &getDocument()->getLine(i.tell().line);
			if(!transitionOccured && i.tell() > minimalLast)
				break;
		} else {
			const Position& partitionStart = (destination > contentType) ? s : i.tell();
			// 次の古いパーティションと同じ位置でマッチした?
			if(currentPartition + 1 < partitions_.getSize() && partitionStart == partitions_[currentPartition + 1]->start) {
				// 同じパーティションだった -> 終了
				if(destination == partitions_[currentPartition + 1]->contentType && p > minimalLast) {
					changedRegion.second = s;
					return;
				}
				partitions_[++currentPartition]->contentType = contentType = destination;
			} else {	// 次の古いパーティションの前でマッチした
				partitions_.insert(++currentPartition, new Partition(destination, partitionStart, i.tell()));
				contentType = destination;
			}
			if(changedRegion.first == Position::INVALID_POSITION)
				changedRegion.first = partitionStart;
		}
	}
	changedRegion.second = (changedRegion.first != Position::INVALID_POSITION) ? eof : Position::INVALID_POSITION;
}

/// @see text#DocumentPartitioner#documentAboutToBeChanged
void LexicalPartitioner::documentAboutToBeChanged() throw() {
	eofBeforeDocumentChange_ = getDocument()->getEndPosition(false);
}

/// @see text#DocumentPartitioner#documentChanged
void LexicalPartitioner::documentChanged(const DocumentChange& change) throw() {
	// 削除の場合、削除範囲に完全に含まれていたパーティションを削除する
	if(change.isDeletion()) {
		size_t deletedFirst = findClosestPartitionIndex(change.getRegion().getTop());
		if(change.getRegion().getTop() != partitions_[deletedFirst]->start)
			++deletedFirst;
		size_t deletedLast = findClosestPartitionIndex(change.getRegion().getBottom());
		if(deletedLast == partitions_.getSize() - 1 && change.getRegion().getBottom() == eofBeforeDocumentChange_)
			++deletedLast;
		if(deletedLast > deletedFirst)
			partitions_.erase(deletedFirst, deletedLast - deletedFirst);
	}
	// ドキュメントの変更に合わせて、全てのパーティションを移動する
	size_t numberOfPartitions = partitions_.getSize();
	for(size_t i = 0; i < numberOfPartitions; ++i) {
		partitions_[i]->start = updatePosition(partitions_[i]->start, change, FORWARD);
		partitions_[i]->introducerEnd = updatePosition(partitions_[i]->introducerEnd, change, FORWARD);
	}
	// ドキュメント先頭を含むパーティションが無くなった -> 既定のパーティションを詰める
	if(numberOfPartitions == 0 || partitions_[0]->start != getDocument()->getStartPosition(false)) {
		if(numberOfPartitions == 0 || partitions_[0]->contentType != DEFAULT_CONTENT_TYPE) {
			partitions_.insert(0, new Partition(DEFAULT_CONTENT_TYPE, Position(0, 0), Position(0, 0)));
			++numberOfPartitions;
		} else
			partitions_[0]->start = partitions_[0]->introducerEnd = getDocument()->getStartPosition(false);
	}
	// ドキュメント終端が始点のパーティション -> 削除する
	if(numberOfPartitions > 1 && partitions_[numberOfPartitions - 1]->start == getDocument()->getEndPosition(false))
		partitions_.erase(numberOfPartitions - 1);
	// 影響を受けた部分を再構築
	const Position& changedStart = change.getRegion().getTop();
	const size_t closestPartitionIndex = findClosestPartitionIndex(changedStart);
	Position reparseStart;
	if(closestPartitionIndex == 0 || changedStart >= partitions_[closestPartitionIndex]->introducerEnd)
		reparseStart = max(partitions_[closestPartitionIndex]->introducerEnd, Position(changedStart.line, 0));
	else {
		reparseStart = partitions_[closestPartitionIndex - 1]->introducerEnd;
		const Position m = (changedStart.line == 0 || partitions_[closestPartitionIndex]->start.column > 0) ?
			Position(changedStart.line, 0) : Position(changedStart.line - 1, getDocument()->getLineLength(changedStart.line - 1));
		reparseStart = max(reparseStart, m);
	}
	if(change.isDeletion())
		reparseStart = (--DocumentCharacterIterator(*getDocument(), reparseStart)).tell();
	Region changedRegion;
	computePartitioning(reparseStart, change.isDeletion() ? changedStart : change.getRegion().getBottom(), changedRegion);
	if(!changedRegion.isEmpty())
		notifyDocument(changedRegion);
//	dump();
}

/// @see text#DocumentPartitioner#doGetPartition
void LexicalPartitioner::doGetPartition(const Position& at, DocumentPartition& partition) const throw() {
	const size_t i = findClosestPartitionIndex(at);
	const Partition& p = *partitions_[i];
	partition.contentType = p.contentType;
	partition.region.first = p.start;
	partition.region.second = (i < partitions_.getSize() - 1) ? partitions_[i + 1]->start : getDocument()->getEndPosition(false);
}

/// @see text#DocumentPartitioner#doInstall
void LexicalPartitioner::doInstall() throw() {
	partitions_.clear();
	partitions_.insert(0, new Partition(DEFAULT_CONTENT_TYPE, Position(0, 0), Position(0, 0)));
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

inline size_t LexicalPartitioner::findClosestPartitionIndex(const Position& at) const throw() {
	return ascension::internal::searchBound(
		static_cast<size_t>(0), partitions_.getSize(), at, bind1st(mem_fun(LexicalPartitioner::getPartitionStart), this));
}
