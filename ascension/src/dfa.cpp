/**
 * @file dfa.cpp
 * @author exeal
 * @date 2007
 */

#include "dfa.hpp"
#include "unicode.hpp"
#include <memory>	// std.auto_ptr
#include <bitset>
#include <vector>
#include <map>
#include <stack>
using namespace ascension;
using namespace ascension::regex::dfa;
using namespace ascension::unicode;
using namespace std;

namespace {
	const ASCENSION_DFA_PATTERN_MAXIMUM_LENGTH = 256;

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
}


namespace {
	class CharacterSet {
	public:
		CharacterSet() throw() : first_(0), last_(0) {}
		explicit CharacterSet(CodePoint c) throw() : first_(c), last_(c + 1) {}
		CharacterSet(CodePoint first, CodePoint last) : first_(first), last_(last) {if(first >= last) throw invalid_argument("");}
		void add(CodePoint c) throw() {return add(c, c + 1);}
		void add(CodePoint first, CodePoint last) {
			CharacterSet* p = this;
			while(p->next_.get() != 0)
				p = p->next_.get();
			p->next_.reset(new CharacterSet(first, last));
		}
		bool includes(CodePoint c) const throw() {return (c >= first_ && c < last_) || (next_.get() != 0) && next_->includes(c);}
	private:
		const CodePoint first_, last_;	// first_ <= c < last_
		auto_ptr<CharacterSet> next_;
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

	class Scanner : public ScannerBase {
	public:
		Scanner(const CodePoint* first, const CodePoint* last) : current_(first), last_(last) {}
		CodePoint getCharacter() const throw() {return character_;}
		Token next();
	private:
		const CodePoint* current_;
		const CodePoint* const last_;
		CodePoint character_;
	};

	ScannerBase::Token Scanner::next() {
		if(current_ == last_)
			return END_OF_PATTERN;
		CodePoint c(*current_);
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

	typedef size_t Position;
	typedef bitset<ASCENSION_DFA_PATTERN_MAXIMUM_LENGTH> Positions;
	const Positions EMPTY_POSITIONS;
	typedef vector<Positions*> FollowPositions;

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
	protected:
		Node(auto_ptr<Node> left, auto_ptr<Node> right, bool nullable) throw() : left_(left), right_(right), nullable_(nullable) {}
	private:
		auto_ptr<Node> left_, right_;
		const bool nullable_;	// "nullable" = whether the subtree from this node includes the empty string
	};

	class EmptyNode : public Node {
	public:
		EmptyNode() throw() : Node(auto_ptr<Node>(), auto_ptr<Node>(), true) {}
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
		explicit CharacterSymbol(CodePoint character) throw() : character_(character) {}
		bool matches(CodePoint c) const throw() {return c == character_;}
	private:
		const CodePoint character_;
	};

	class CharacterSetSymbol : virtual public AbstractSymbol {
	public:
		CharacterSetSymbol(auto_ptr<CharacterSet> charset, bool complemental) throw() : charset_(charset), complemental_(complemental) {}
		bool matches(CodePoint c) const throw() {
			const bool inclusive = charset_->includes(c);
			return complemental_ ? !inclusive : inclusive;
		}
	private:
		const auto_ptr<CharacterSet> charset_;
		const bool complemental_;
	};

	class LeafNode : public Node {
	public:
		// constructor
		LeafNode(Position position, auto_ptr<AbstractSymbol> symbol) throw() :
			Node(auto_ptr<Node>(0), auto_ptr<Node>(0), false), symbol_(symbol) {assert(symbol_.get() != 0); pos_.set(position);}
		// "symbol(i)" = the symbol at a position i
		bool matchesSymbol(CodePoint symbol) const throw() {return symbol_->matches(symbol);}
		// Node
		const Positions& getFirstPositions() const throw() {return EMPTY_POSITIONS;}
		void getFollowPositions(FollowPositions&) const throw() {}
		const Positions& getLastPositions() const throw() {return EMPTY_POSITIONS;}
	private:
		Positions pos_;
		auto_ptr<AbstractSymbol> symbol_;
	};

	class ConcatenationNode : public Node {
	public:
		ConcatenationNode(auto_ptr<Node> left, auto_ptr<Node> right) : Node(left, right, left->isNullable() && right->isNullable()) {}
		const Positions& getFirstPositions() const throw() {
			if(firstpos_.get() == 0) {
				firstpos_.reset(new Positions(getLeft()->getFirstPositions()));
				if(getLeft()->isNullable())
					*firstpos_ |= getRight()->getFirstPositions();
			}
			return *firstpos_;
		}
		void getFollowPositions(FollowPositions& followpos) const throw() {
			const Positions& leftLastPositions = getLeft()->getLastPositions();
			const Positions& rightFirstPositions = getRight()->getFirstPositions();
			assert(leftLastPositions.size() <= followpos.size() && rightFirstPositions.size() <= followpos.size());
			for(Position p(0), e(leftLastPositions.size()); p < e; ++p) {
				if(leftLastPositions.test(p)) {
					if(followpos[p] == 0)
						followpos[p] = new Positions;
					*followpos[p] |= rightFirstPositions;
				}
			}
		}
		const Positions& getLastPositions() const throw() {
			if(lastpos_.get() == 0) {
				lastpos_.reset(new Positions(getRight()->getLastPositions()));
				if(getRight()->isNullable())
					*lastpos_ |= getLeft()->getLastPositions();
			}
			return *lastpos_;
		}
	private:
		mutable auto_ptr<Positions> firstpos_, lastpos_;
	};

	class SelectionNode : public Node {
	public:
		SelectionNode(auto_ptr<Node> left, auto_ptr<Node> right) : Node(left, right, left->isNullable() || right->isNullable()) {}
		const Positions& getFirstPositions() const throw() {
			if(firstpos_.get() == 0) {
				firstpos_.reset(new Positions(getLeft()->getFirstPositions()));
				*firstpos_ |= getRight()->getFirstPositions();
			}
			return *firstpos_;
		}
		void getFollowPositions(FollowPositions&) const throw() {}
		const Positions& getLastPositions() const throw() {
			if(lastpos_.get() == 0) {
				lastpos_.reset(new Positions(getLeft()->getFirstPositions()));
				*lastpos_ |= getRight()->getFirstPositions();
			}
			return *lastpos_;
		}
	private:
		mutable auto_ptr<Positions> firstpos_, lastpos_;
	};

	class ClosureNode : public Node {
	public:
		static const ulong INFINITY_OCCURENCES = 0xFFFFFFFFUL;
		ClosureNode(auto_ptr<Node> base, ulong minOccurences, ulong maxOccurences) :
				Node(base, auto_ptr<Node>(0), minOccurences == 0), minOccurences_(minOccurences), maxOccurences_(maxOccurences) {
			if(minOccurences > maxOccurences) throw PatternSyntaxException();}
		const Positions& getFirstPositions() const throw() {return getLeft()->getFirstPositions();}
		void getFollowPositions(FollowPositions& followpos) const throw() {
			const Positions& leftLastPositions = getLeft()->getLastPositions();
			const Positions& leftFirstPositions = getLeft()->getFirstPositions();
			assert(leftLastPositions.size() <= followpos.size() && leftFirstPositions.size() <= followpos.size());
			for(Position p(0), e(leftLastPositions.size()); p != e; ++p) {
				if(followpos[p] == 0)
					followpos[p] = new Positions;
				*followpos[p] |= leftFirstPositions;
			}
		}
		const Positions& getLastPositions() const throw() {return getLeft()->getLastPositions();}
	private:
		const ulong minOccurences_, maxOccurences_;
	};

	class Parser {
	public:
		Parser(const CodePoint* first, const CodePoint* last) : matchesBOL_(*first == '^'), matchesEOL_(*--last == '$'), scanner_(matchesBOL_ ? ++first : first, matchesEOL_ ? --last : last) {}
		const vector<LeafNode*>& getLeafs() const throw() {return leafs_;}
		Node& getRoot() const throw() {return *root_;}
		const vector<const AbstractSymbol*>& getSymbols() const throw() {return symbols_;}
	private:
		void eatToken() {next_ = scanner_.next();}
		auto_ptr<Node> processClass();
		auto_ptr<Node> processExpression();
		auto_ptr<Node> processFactor();
		auto_ptr<Node> processPrimary();
		auto_ptr<Node> processTerm();
	private:
		const bool matchesBOL_, matchesEOL_;
		Scanner scanner_;
		auto_ptr<Node> root_;
		ScannerBase::Token next_;
		vector<const AbstractSymbol*> symbols_;
		vector<LeafNode*> leafs_;
	};
}

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

auto_ptr<Node> Parser::processClass() {
	const bool complemental = next_ == ScannerBase::CARET;
	if(complemental)
		eatToken();
	auto_ptr<CharacterSet> charset;
	while(next_ == ScannerBase::LITERAL) {
		CodePoint c = scanner_.getCharacter();
		eatToken();
		if(next_ == ScannerBase::MINUS) {
			eatToken();
			if(next_ != ScannerBase::LITERAL)
				throw PatternSyntaxException();
			if(charset.get() == 0)
				charset.reset(new CharacterSet(min(c, scanner_.getCharacter()), max(c, scanner_.getCharacter())));
			else
				charset->add(min(c, scanner_.getCharacter()), max(c, scanner_.getCharacter()));
			eatToken();
		} else {
			if(charset.get() == 0)
				charset.reset(new CharacterSet(c));
			else
				charset->add(c);
		}
	}
	auto_ptr<AbstractSymbol> newSymbol(new CharacterSetSymbol(charset, complemental));
	symbols_.push_back(newSymbol.get());
	auto_ptr<LeafNode> newNode(new LeafNode(leafs_.size(), newSymbol));
	leafs_.push_back(newNode.get());
	return newNode;
}

auto_ptr<Node> Parser::processExpression() {
	auto_ptr<Node> node(processTerm());
	if(next_ == ScannerBase::PIPE) {
		eatToken();
		return auto_ptr<Node>(new SelectionNode(node, processExpression()));
	}
	return node;
}

auto_ptr<Node> Parser::processFactor() {
	auto_ptr<Node> node(processPrimary()), closure;
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

auto_ptr<Node> Parser::processPrimary() {
	if(next_== ScannerBase::LITERAL) {
		const CodePoint c = scanner_.getCharacter();
		auto_ptr<AbstractSymbol> newSymbol(new CharacterSymbol(c));
		bool unique = true;
		for(vector<const AbstractSymbol*>::const_iterator i(symbols_.begin()), e(symbols_.end()); i != e; ++i) {
			if((*i)->matches(c)) {
				unique = false;
				break;
			}
		}
		if(unique)
			symbols_.push_back(newSymbol.get());
		auto_ptr<LeafNode> newNode(new LeafNode(leafs_.size(), newSymbol));
		leafs_.push_back(newNode.get());
		eatToken();
		return newNode;
	} else if(next_ == ScannerBase::DOT) {
		auto_ptr<AbstractSymbol> newSymbol(new CharacterSetSymbol(auto_ptr<CharacterSet>(new CharacterSet()), true));
		symbols_.push_back(newSymbol.get());
		auto_ptr<LeafNode> newNode(new LeafNode(leafs_.size(), newSymbol));
		leafs_.push_back(newNode.get());
		eatToken();
		return newNode;
	} else if(next_ == ScannerBase::SQUARE_OPEN) {
		eatToken();
		auto_ptr<Node> newNode(processClass());
		if(next_ != ScannerBase::SQUARE_CLOSE)
			throw PatternSyntaxException();
		eatToken();
		return newNode;
	} else if(next_ == ScannerBase::PAREN_OPEN) {
		eatToken();
		auto_ptr<Node> newNode(processExpression());
		if(next_ != ScannerBase::PAREN_CLOSE)
			throw PatternSyntaxException();
		eatToken();
		return newNode;
	}
	throw PatternSyntaxException();
}

auto_ptr<Node> Parser::processTerm() {
	if(next_ == Scanner::END_OF_PATTERN)
		return auto_ptr<Node>(new EmptyNode());
	auto_ptr<Node> node(processFactor());
	switch(next_) {
	case ScannerBase::LITERAL:
	case ScannerBase::DOT:
	case ScannerBase::SQUARE_OPEN:
	case ScannerBase::PAREN_OPEN:
		return auto_ptr<Node>(new ConcatenationNode(node, processTerm()));
	default:
		return node;
	}
}

class Pattern::DFA {
public:
	DFA(const CodePoint* pattern, size_t length);
private:
	struct State {
		Positions positions;
		bool accepting;
		map<CodePoint, State*> move;
		State() : accepting(false) {}
		~State() throw() {for(map<CodePoint, State*>::iterator i(move.begin()), e(move.end()); i != e; ++i) delete i->second;}
	};
	vector<State*> identifiedStates_;	// "Istates" = the set of sets of positions that have been identified
};

Pattern::DFA::DFA(const CodePoint* pattern, size_t length) {
	// convert regular expression to DFA

	// 1. construct the syntax tree for the regular expression
	//      including a # at the end
	//    thereby also symbol(i)
	Parser parser(pattern, pattern + length);

	// 2. construct firstpos(i) and followpos(i)
	//      (lastpos and nullable are needed when computing these)
	FollowPositions followpos(parser.getLeafs().size(), static_cast<Positions*>(0));
	parser.getRoot().getFollowPositions(followpos);

	// 3. do the subset construction algorithm
	State* p = new State;
	p->positions = parser.getRoot().getFirstPositions();
	p->accepting = p->positions.test(parser.getLeafs().size() - 1);
	identifiedStates_.push_back(p);
	stack<State*> unmarkedStates;	// "Ustates" = the 'to do' set of sets of positions
	unmarkedStates.push(p);
	while(!unmarkedStates.empty()) {
		p = unmarkedStates.top();
		unmarkedStates.pop();
//		for(vector<AbstractSymbol*>::const_iterator s(parser.getSymbols().begin()), es(parser.getSymbols().end()); s != es; ++s) {
		for(CodePoint s = 0; s < 0x110000; ++s) {
			auto_ptr<State> q(new State);
			for(Position pos(0), ep(p->positions.size()); pos != ep; ++pos) {
				if(parser.getSymbols()[pos]->matches(s))
					q->positions |= *followpos[pos];
			}
			q->accepting = q->positions.test(parser.getLeafs().size() - 1);
			if(q->positions.any()) {
				vector<State*>::const_iterator i(identifiedStates_.begin());
				const vector<State*>::const_iterator e(identifiedStates_.end());
				for(; i != e; ++i) {
					if((*i)->positions == q->positions)
						break;
				}
				if(i == e) {	// "Q not in Istates"
					identifiedStates_.push_back(q.get());
					unmarkedStates.push(q.get());
				} else
					p = *i;
			}
			p->move[s] = q.release();
		}
	}
}


// Pattern //////////////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param first the start of the pattern string
 * @param last the end of the pattern string
 * @param options the syntax options
 * @throw PatternSyntaxException
 */
Pattern::Pattern(const Char* first, const Char* last, const SyntaxOptions& options /* = NORMAL */) : impl_(0) {
	if(first >= last)
		throw invalid_argument("");
	manah::AutoBuffer<CodePoint> temp(new CodePoint[last - first]);
	copy(UTF16To32Iterator<>(first, last), UTF16To32Iterator<>(first, last, last), temp.get());
	impl_ = new DFA(temp.get(), last - first);
}

/**
 * Constructor.
 * @param pattern the pattern string
 * @param options the syntax options
 * @throw PatternSyntaxException
 */
Pattern::Pattern(const String& pattern, const SyntaxOptions& options /* = NORMAL */) : impl_(0) {
	if(pattern.empty())
		throw invalid_argument("");
	manah::AutoBuffer<CodePoint> temp(new CodePoint[pattern.length()]);
	copy(StringCharacterIterator(pattern), StringCharacterIterator(pattern, pattern.end()), temp.get());
	impl_ = new DFA(temp.get(), pattern.length());
}

/// Destructor.
Pattern::~Pattern() throw() {
	delete impl_;
}
