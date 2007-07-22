/**
 * @file dfa.cpp
 * @author exeal
 * @date 2007
 */

#include "stdafx.h"
#include "common.hpp"
#include <memory>	// std.auto_ptr
#include <set>
#include <vector>
#include <algorithm>	// std.set_union
using namespace ascension;

const CodePoint
	ASCENSION_DFA_DOT			= L'.',
	ASCENSION_DFA_SQUARE_OPEN	= L'[',
	ASCENSION_DFA_SQUARE_CLOSE	= L']',
	ASCENSION_DFA_SQUARE_CARET	= L'^',
	ASCENSION_DFA_SQUARE_MINUS	= L'-',
	ASCENSION_DFA_PAREN_OPEN	= L'(',
	ASCENSION_DFA_PAREN_CLOSE	= L')',
	ASCENSION_DFA_PIPE			= L'|',
	ASCENSION_DFA_ASTERISK		= L'*',
	ASCENSION_DFA_PLUS			= L'+',
	ASCENSION_DFA_QUESTION		= L'?',
	ASCENSION_DFA_BRACE_OPEN	= L'{',
	ASCENSION_DFA_BRACE_CLOSE	= L'}',
	ASCENSION_DFA_COMMA			= L',',
	ASCENSION_DFA_BACKSOLIDUS	= L'\\';

class PatternSyntaxException : public std::invalid_argument {
public:
	PatternSyntaxException() : std::invalid_argument("") {}
};

class CharacterSet {
public:
	explicit CharacterSet(CodePoint c) throw() : first_(c), last_(c) {}
	CharacterSet(CodePoint first, CodePoint last) : first_(first), last_(last) {if(first >= last) throw std::invalid_argument("");}
	void add(CodePoint c) throw() {return add(c, c);}
	void add(CodePoint first, CodePoint last) {
		CharacterSet* p = this;
		while(p->next_.get() != 0)
			p = p->next_.get();
		p->next_.reset(new CharacterSet(first, last));
	}
	bool includes(CodePoint c) const throw() {return (c >= first_ && c <= last_) || (next_.get() != 0) && next_->includes(c);}
private:
	const CodePoint first_, last_;	// inclusive
	std::auto_ptr<CharacterSet> next_;
};

struct ScannerBase {
	enum Token {
		LITERAL,		///< A literal character (UCS-4).
		META,			///< A meta character.
		DOT,			///< .
		SQUARE_OPEN,	///< [
		SQUARE_CLOSE,	///< ]
		CARET,			///< ^
		MINUS,			///< -
		PAREN_OPEN,		///< (
		PAREN_CLOSE,	///< )
		PIPE,			///< |
		ASTERISK,		///< *
		PLUS,			///< +
		QUESTION,		///< ?
		BRACE_OPEN,		///< {
		BRACE_CLOSE,	///< }
		COMMA,			///< ,
		END_OF_PATTERN	///< The end of the pattern string.
	};
};

template<typename InputIterator> class Scanner : public ScannerBase {
public:
	Scanner(InputIterator first, InputIterator last) : current_(first), last_(last) {}
	CodePoint getCharacter() const throw() {return character_;}
	Token next();
private:
	InputIterator current_, last_;
	CodePoint character_;
};

template<typename InputIterator> ScannerBase::Token Scanner<InputIterator>::next() {
	if(current_ == last_)
		return END_OF_PATTERN;
	std::iterator_traits<InputIterator>::value_type c(*current_);
	++current_;
	switch(c) {
	case ASCENSION_DFA_DOT:				return DOT;
	case ASCENSION_DFA_SQUARE_OPEN:		return SQUARE_OPEN;
	case ASCENSION_DFA_SQUARE_CLOSE:	return SQUARE_CLOSE;
	case ASCENSION_DFA_SQUARE_CARET:	return CARET;
	case ASCENSION_DFA_SQUARE_MINUS:	return MINUS;
	case ASCENSION_DFA_PAREN_OPEN:		return PAREN_OPEN;
	case ASCENSION_DFA_PAREN_CLOSE:		return PAREN_CLOSE;
	case ASCENSION_DFA_PIPE:			return PIPE;
	case ASCENSION_DFA_ASTERISK:		return ASTERISK;
	case ASCENSION_DFA_PLUS:			return PLUS;
	case ASCENSION_DFA_QUESTION:		return QUESTION;
	case ASCENSION_DFA_BRACE_OPEN:		return BRACE_OPEN;
	case ASCENSION_DFA_BRACE_CLOSE:		return BRACE_CLOSE;
	case ASCENSION_DFA_COMMA:			return COMMA;
	case ASCENSION_DFA_BACKSOLIDUS:	// an escape sequence?
		if(current_ == last_)
			return END_OF_PATTERN;
		c = *current_;
		++current_;
		switch(c) {
		case 'n':
			character_ = LINE_SEPARATOR;
			return LITERAL;
		case 't':
			character_ = '\t';
			return LITERAL;
		}
	default:
		character_ = c;
		return LITERAL;
	}
}

typedef std::set<std::size_t> Positions;
const Positions EMPTY_POSITIONS;
void unionPositions(const Positions& left, const Positions& right, Positions& destination) {
	std::set_union(left.begin(), left.end(), right.begin(), right.end(), destination.begin());
}
typedef std::vector<Positions*> FollowPositions;

class Node {
public:
	virtual ~Node() throw() {}
	Node* getLeft() const throw() {return left_.get();}
	Node* getRight() const throw() {return right_.get();}
	bool isNullable() const throw() {return nullable_;}

	// "firstpos" = set of positions that can match the first symbol of an instance of the subtree from this node
	virtual const Positions& getFirstPositions() const throw() = 0;
	// "followpos(i)" = positions that may follow immediately after i
	virtual void getFollowPositions(FollowPositions& followpos) const throw() = 0;
	// "lastpos" = set of positions that can match the last symbol of the subtree from this node
	virtual const Positions& getLastPositions() const throw() = 0;
	// "symbol(i)" = the symbol at a position i
	virtual bool matchesSymbol(CodePoint symbol) const throw() = 0;
protected:
	Node(std::auto_ptr<Node> left, std::auto_ptr<Node> right, bool nullable) throw() : left_(left), right_(right), nullable_(nullable) {}
private:
	std::auto_ptr<Node> left_, right_;
	const bool nullable_;	// "nullable" = whether the subtree from this node includes the empty string
};

class EmptyNode : public Node {
public:
	EmptyNode() throw() : Node(std::auto_ptr<Node>(), std::auto_ptr<Node>(), true) {}
	const Positions& getFirstPositions() const throw() {return EMPTY_POSITIONS;}
	void getFollowPositions(FollowPositions&) const throw() {}
	const Positions& getLastPositions() const throw() {return EMPTY_POSITIONS;}
};

class AbstractSymbol {
public:
	virtual ~AbstractSymbol() throw() {}
	virtual bool matches(CodePoint c) const throw() = 0;
};

class CharacterSymbol : virtual public AbstractSymbol {
public:
	bool matches(CodePoint c) const throw() {return c == character_;}
private:
	const CodePoint character_;
};

class CharacterSetSymbol : virtual public AbstractSymbol {
public:
	CharacterSetSymbol(std::auto_ptr<CharacterSet> charset, bool complemental) throw() : charset_(charset), complemental_(complemental) {}
	bool matches(CodePoint c) const throw() {
		const bool inclusive = charset_->includes(c);
		return complemental_ ? !inclusive : inclusive;
	}
private:
	const std::auto_ptr<CharacterSet> charset_;
	const bool complemental_;
};

class LeafNode : public Node {
public:
	LeafNode(std::size_t position, std::auto_ptr<AbstractSymbol> symbol) throw() :
		Node(std::auto_ptr<Node>(0), std::auto_ptr<Node>(0), false), pos_(&position, &position + 1), symbol_(symbol) {assert(symbol_.get() != 0);}
	const Positions& getFirstPositions() const throw() {return EMPTY_POSITIONS;}
	void getFollowPositions(FollowPositions&) const throw() {}
	const Positions& getLastPositions() const throw() {return EMPTY_POSITIONS;}
	bool matchesSymbol(CodePoint symbol) const throw() {return symbol_->matches(symbol);}
private:
	const Positions pos_;
	std::auto_ptr<AbstractSymbol> symbol_;
};

class ConcatenationNode : public Node {
public:
	ConcatenationNode(std::auto_ptr<Node> left, std::auto_ptr<Node> right) : Node(left, right, left->isNullable() && right->isNullable()) {}
	const Positions& getFirstPositions() const throw() {
		if(firstpos_.get() == 0) {
			if(getLeft()->isNullable()) {
				firstpos_.reset(new Positions);
				unionPositions(getLeft()->getFirstPositions(), getRight()->getFirstPositions(), *firstpos_);
			} else
				firstpos_.reset(new Positions(getLeft()->getFirstPositions()));
		}
	}
	void getFollowPositions(FollowPositions& followpos) const throw() {
		for(Positions::const_iterator i(getLeft()->getLastPositions().begin()), e1(getLeft()->getLastPositions().end()); i != e1; ++i) {
			assert(*i < followpos.size());
			if(followpos[*i] == 0)
				followpos[*i] = new Positions;
			for(Positions::const_iterator j(getRight()->getFirstPositions().begin()), e2(getRight()->getFirstPositions().end()); j != e2; ++j)
				followpos[*i]->insert(*j);
		}
	}
	const Positions& getLastPositions() const throw() {
		if(lastpos_.get() == 0) {
			if(getRight()->isNullable()) {
				lastpos_.reset(new Positions);
				unionPositions(getLeft()->getLastPositions(), getRight()->getLastPositions(), *firstpos_);
			} else
				lastpos_.reset(new Positions(getRight()->getLastPositions()));
		}
		return *lastpos_;
	}
private:
	mutable std::auto_ptr<Positions> firstpos_, lastpos_;
};

class SelectionNode : public Node {
public:
	SelectionNode(std::auto_ptr<Node> left, std::auto_ptr<Node> right) : Node(left, right, left->isNullable() || right->isNullable()) {}
	const Positions& getFirstPositions() const throw() {
		if(firstpos_.get() == 0) {
			firstpos_.reset(new Positions);
			unionPositions(getLeft()->getFirstPositions(), getRight()->getFirstPositions(), *firstpos_);
		}
		return *firstpos_;
	}
	void getFollowPositions(std::vector<Positions>&) const throw() {}
	const Positions& getLastPositions() const throw() {
		if(lastpos_.get() == 0) {
			lastpos_.reset(new Positions);
			unionPositions(getLeft()->getFirstPositions(), getRight()->getFirstPositions(), *lastpos_);
		}
		return *lastpos_;
	}
private:
	mutable std::auto_ptr<Positions> firstpos_, lastpos_;
};

class ClosureNode : public Node {
public:
	static const ulong INFINITY_OCCURENCES = 0xFFFFFFFFUL;
	ClosureNode(std::auto_ptr<Node> base, ulong minOccurences, ulong maxOccurences) :
			Node(base, std::auto_ptr<Node>(0), minOccurences == 0), minOccurences_(minOccurences), maxOccurences_(maxOccurences) {
		if(minOccurences > maxOccurences) throw PatternSyntaxException();}
	const Positions& getFirstPositions() const throw() {return getLeft()->getFirstPositions();}
	void getFollowPositions(FollowPositions& followpos) const throw() {
		for(Positions::const_iterator i(getLeft()->getLastPositions().begin()), e1(getLeft()->getLastPositions().end()); i != e1; ++i) {
			assert(*i < followpos.size());
			if(followpos[*i] == 0)
				followpos[*i] = new Positions;
			for(Positions::const_iterator j(getLeft()->getFirstPositions().begin()), e2(getLeft()->getFirstPositions().end()); j != e2; ++j)
				followpos[*i]->insert(*j);
		}
	}
	const Positions& getLastPositions() const throw() {return getLeft()->getLastPositions();}
private:
	const ulong minOccurences_, maxOccurences_;
};

template<typename InputIterator> class Parser {
public:
	Parser(InputIterator first, InputIterator last) : matchesBOL_(*first == '^'), matchesEOL_(*--last == '$'), scanner_(matchesBOL_ ? ++first : first, matchesEOL_ ? --last : last) {}
	const std::vector<AbstractSymbol*>& getLeafs() const throw() {return leafs_;}
	Node& getRoot() const throw() {return **root_;}
	const std::vector<AbstractSymbol*>& getSymbols() const throw() {return symbols_;}
private:
	void eatToken() {next_ = scanner_.next();}
	std::auto_ptr<Node> processClass();
	std::auto_ptr<Node> processExpression();
	std::auto_ptr<Node> processFactor();
	std::auto_ptr<Node> processPrimary();
	std::auto_ptr<Node> processTerm();
private:
	const bool matchesBOL_, matchesEOL_;
	Scanner<InputIterator> scanner_;
	std::auto_ptr<Node> root_;
	ScannerBase::Token next_;
	std::vector<const AbstractSymbol*> symbols_;
	std::vector<LeafNode*> leafs_;
};

/*
	Formal description

	TopLevel ::= '^'? Expression '$'?
	Expression ::= Terminal
	               Terminal '|' Expression
	Terminal ::= <<empty>>
	             Factor Terminal
	Factor ::= Primary Quantifier?
	Primary ::= Character
	            '.'
	            '[' Class ']'
				'\p' Property
				'\P' Property
	            '(' Expression ')'
	Quantifier ::= '*'
	               '+'
	               '?'
	               '{' Digits '}'
	               '{' Digits ',' '}'
	               '{' Digits ',' Digits '}'
	Class ::= '^'? Character
	          '^'? Character '-' Character
	Property ::= Character
	             '{' Character+ '}'
	Digits ::= '0'
	           [1-9] [0-9]*
	Character ::= <<any Unicode character>>
*/

template<typename InputIterator> std::auto_ptr<Node> Parser<InputIterator>::processClass() {
	const bool complemental = next_ == ScannerBase::CARET;
	if(complemental)
		eatToken();
	std::auto_ptr<CharacterSet> charset;
	while(next_ == ScannerBase::LITERAL) {
		CodePoint c = scanner_.getCharacter();
		eatToken();
		if(next_ == ScannerBase::MINUS) {
			eatToken();
			if(next_ != ScannerBase::LITERAL)
				throw PatternSyntaxException();
			if(charset.get() == 0)
				charset.reset(new CharacterSet(std::min(c, scanner_.getCharacter()), std::max(c, scanner_.getCharacter())));
			else
				charset->add(std::min(c, scanner_.getCharacter()), std::max(c, scanner_.getCharacter()));
			eatToken();
		} else {
			if(charset.get() == 0)
				charset.reset(new CharacterSet(c));
			else
				charset->add(c);
		}
	}
	std::auto_ptr<Node> newSymbol(new CharacterSetSymbol(charset, complemental));
	symbols_.insert(newSymbol.get());
	std::auto_ptr<LeafNode> newNode(new LeafNode(leafs_.size(), newSymbol));
	leafs_.push_back(newNode.get());
	return newNode;
}

template<typename InputIterator> std::auto_ptr<Node> Parser<InputIterator>::processExpression() {
	std::auto_ptr<Node> node(processTerm());
	if(next_ == ScannerBase::VERTICAL) {
		eatToken();
		return std::auto_ptr<Node>(new SelectionNode(node, processExpression()));
	}
	return node;
}

template<typename InputIterator> std::auto_ptr<Node> Parser<InputIterator>::processFactor() {
	std::auto_ptr<Node> node(processPrimary()), closure;
	switch(next_) {
	case ScannerBase::ASTERISK:
		closure.reset(new ClosureNode(node, 0, ClosureNode::INFINITY_OCCURENCES));
		break;
	case ScannerBase::PLUS:
		closure.reset(new ClosureNode(node, 1, ClosureNode::INFINITY_OCCURENCES));
		break;
	case ScannerBase::QUESTION:
		closure.reset(new ClosureNode(node, 0, 1));
		break;
	}
	if(next_ == ScannerBase::BRACE_OPEN) {
		ulong minOccurences = 0, maxOccurences;
		// TODO: parse quantifier form such as "{m, n}"
	}
	if(closure.get() != 0) {
		eatToken();
		return closure;
	}
	return node;
}

template<typename InputIterator> std::auto_ptr<Node> Parser<InputIterator>::processPrimary() {
	if(next_== ScannerBase::LITERAL) {
		const CodePoint c = scanner_.getCharacter();
		std::auto_ptr<AbstractSymbol> newSymbol(new CharacterSymbol(c));
		bool unique = true;
		for(std::vector<const AbstractSymbol*>::const_iterator i(symbols_.begin()), e(symbols_.end()); i != e; ++i) {
			if((*i)->matches(c)) {
				unique = false;
				break;
			}
		}
		if(unique)
			symbols_.push_back(newSymbol.get());
		std::auto_ptr<LeafNode> newNode(new LeafNode(leafs_.size(), newSymbol));
		leafs_.push_back(newNode.get());
		eatToken();
		return newNode;
	} else if(next_ == ScannerBase::DOT) {
		std::auto_ptr<AbstractSymbol> newSymbol(new CharacterSetSymbol(std::list<std::pair<CodePoint, CodePoint> >(), true));
		symbols_.push_back(newSymbol.get());
		std::auto_ptr<LeafNode> newNode(new LeafNode(leafs_.size(), newSymbol));
		leafs_.push_back(newNode.get());
		eatToken();
		return newNode;
	} else if(next_ == ScannerBase::SQUARE_OPEN) {
		eatToken();
		std::auto_ptr<Node> newNode(processClass());
		if(next_ != ScannerBase::SQUARE_CLOSE)
			throw PatternSyntaxException();
		eatToken();
		return newNode;
	} else if(next_ == ScannerBase::PAREN_OPEN) {
		eatToken();
		std::auto_ptr<Node> newNode(processExpression());
		if(next_ != ScannerBase::PAREN_CLOSE)
			throw PatternSyntaxException();
		eatToken();
		return newNode;
	}
	throw PatternSyntaxException();
}

template<typename InputIterator> std::auto_ptr<Node> Parser<InputIterator>::processTerm() {
	if(next_ == Scanner::END_OF_PATTERN)
		return std::auto_ptr<Node>(new EmptyNode());
	std::auto_ptr<Node> node(processFactor());
	switch(next_) {
	case ScannerBase::LITERAL:
	case ScannerBase::DOT:
	case ScannerBase::SQUARE_OPEN:
	case ScannerBase::PAREN_OPEN:
		return std::auto_ptr<Node>(new ConcatenationNode(node, processTerm()));
	default:
		return node;
	}
}

template<typename InputIterator> class DFA {
public:
	DFA(InputIterator first, InputIterator last);
private:
	struct State {
		Positions positions;
		bool final;
		State** move;
		State(std::size_t numberOfSymbols) : positions(new State[]) {}
	};
	std::vector<State*> identifiedStates_;	// "Istates" = the set of sets of positions that have been identified
};

template<typename InputIterator> DFA<InputIterator>::DFA(InputIterator first, InputIterator last) {
	// convert regular expression to DFA

	// 1. construct the syntax tree for the regular expression
	//      including a # at the end
	//    thereby also symbol(i)
	Parser<InputIterator> parser(first, last);

	// 2. construct firstpos(i) and followpos(i)
	//      (lastpos and nullable are needed when computing these)
	std::vector<Positions> followpos(parser.getLeafs().size(), static_cast<Positions*>(0));
	parser.getRoot().getFollowPositions(followpos);

	// 3. do the subset construction algorithm
	State* p = new State;
	while(!unmarkedStates.empty()) {
		p = unmarkedState.front();
		unmarkedStates.pop_front();
		for(std::vector<AbstractSymbol*>::const_iterator s(parser.getSymbols().begin()), es(parser.getSymbols().end()); s != es; ++s) {
			std::auto_ptr<State> q(new State);
			for(Positions::const_iterator pos(p->positions.begin(), pe(p->positions.end()); pos != pe; ++pos) {
				if( == s)
					q->positions.insert(followpos[*pos].begin(), followpos[*pos].end());
			}
			q->final = q->positions.find(parser.getLeafs().size() - 1) != q->positions.end();
			if(!q->positions.empty()) {
			}
			p->move[s] = q.release();
		}
	}
}
