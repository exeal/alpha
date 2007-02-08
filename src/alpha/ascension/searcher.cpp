/**
 * @file search.cpp
 * @author exeal
 * @date 2004-2006 (was TextSearcher.cpp)
 * @date 2006
 *
 * Implementation of text search objects.
 *
 * Ascensio implements regular expression search with Boost.Regex (http://www.boost.org/libs/regex/) and 
 * implements Japanese direct search with C/Migemo (http://www.kaoriya.net/#CMigemo).
 * If you don't have these libraries, define the symbol @c ASCENSION_NO_* in config.hpp.
 *
 * <h3>Regular expression search (Boost.Regex)</h3>
 *
 * Although we use Boost.Regex to implement regular expression search, it is subject to change.
 * However, the header file (searcher.hpp) does not refer this library and the clients don't know that Ascension uses Boost.Regex.
 * All exceptions thrown by Boost.Regex do not propagate to the clients.
 *
 * <h3>Japanese direct search (C/Migemo)</h3>
 *
 * Japanese direct search is available only when the regular expression is available.
 *
 * <h3>To compile without the regular expression search features</h3>
 *
 * To disable the regular expression search, define @c ASCENSION_NO_REGEX.
 * To disable Japanese direct search, define @c ASCENSION_NO_MIGEMO.
 * Call @c TextSearcher#isRegexAvailable or @c TextSearcher#isMigemoAvailable to check if these features are availble at runtime.
 * These methods always same values in a process.
 */

/*
	実装メモ : 大文字小文字を区別しないパターンマッチとケースフォールディング (2006.12.03)
	--
	大文字小文字を区別しない文字列の比較を行うには、本来は両方の文字列をケースフォールドした上で
	バイナリ比較を行うのが正しい手順であるが、単純 (CaseFolding.txt における 'S') なケースフォールディングでは
	文字幅が変わらないため、以下のように文字同士を比較するタイミングで変換するコーディングもありうる:

		extern Char c1, c2;
		toCasefolded(c1) == toCasefolded(c2)

	このようにすると unicode::StringFolder を使わずにすむので、処理が高速化でき、空間も節約できる。
	しかし、このようなアプローチはあまり使わないほうが良いと思う。
	そのため、高機能な TextSearcher クラスではこのアプローチは使用せず、他の軽量なパターンマッチングメカニズム
	(例えば、正規表現を使った字句解析器) などに限って使用することにした。

	大文字小文字を区別しない正規表現であれば、regex::RegexTraits クラスが実装している。
	ただし文字幅の変わるケースフォールディングはサポートしていない。
	これは Boost の正規表現エンジンが常に 1 文字単位の比較を行うことに起因している。
 */

#include "stdafx.h"
#include "searcher.hpp"
#include "break-iterator.hpp"
#include "viewer.hpp"
#ifndef ASCENSION_NO_REGEX
#include "regex.hpp"
#endif /* !ASCENSION_NO_REGEX */

using namespace ascension;
using namespace ascension::rules;
using namespace ascension::searcher;
using namespace ascension::text;
using namespace ascension::unicode;
using namespace std;


// private helpers //////////////////////////////////////////////////////////

namespace {
	/// Implementation of UTF-16 BM search.
	class BMSearcher {
	public:
		/**
		 * Constructor.
		 * @param direction the direction to search
		 * @param first the start of the search pattern
		 * @param last the end of the search pattern. must be greater than @a first
		 * @param caseFolding the case folding type for case-insensitive search. can't be @c unicode#CASEFOLDING_UNICODE_FULL
		 */
		BMSearcher(Direction direction, const Char* first, const Char* last, CaseFoldingType caseFolding) throw() {
			compile(direction, first, last, caseFolding);
		}
		/**
		 * Compiles the pattern.
		 * @param direction the direction to search
		 * @param first the start of the search pattern
		 * @param last the end of the search pattern. must be greater than @a first
		 * @param caseFolding the case folding type for case-insensitive search. can't be @c unicode#CASEFOLDING_UNICODE_FULL
		 */
		void compile(Direction direction, const Char* first, const Char* last, CaseFoldingType caseFolding) throw() {
			assert(first != 0 && last != 0 && first < last && caseFolding != CASEFOLDING_UNICODE_FULL);
			direction_ = direction;
			caseFolding_ = caseFolding;
			patternFirst_ = first;
			patternLast_ = last;
			fill(lastOccurences_, endof(lastOccurences_), last - first);
			if(direction == FORWARD) {
				for(const Char* p = first; p < last; ++p)
					lastOccurences_[fold(*p)] = last - p - 1;
			} else {
				for(const Char* p = last - 1; ; --p) {
					lastOccurences_[fold(*p)] = p - first;
					if(p == first)
						break;
				}
			}
		}
		/// Returns the direction to search.
		Direction getDirection() const throw() {return direction_;}
		/// Returns the case folding type.
		CaseFoldingType getCaseFoldingType() const throw() {return caseFolding_;}
		/**
		 * Searches in the specified target string.
		 * @param first the start of the target string
		 * @param last the end of the target string. must be greater than @a first
		 * @return the pointer to matched position. @c null if the pattern not found
		 */
		template<class Iterator>
		Iterator search(Iterator first, Iterator last) const {
			if(direction_ == FORWARD) {
				Iterator target(first);
				advance(target, (patternLast_ - patternFirst_) - 1);
				for(const Char* pattern; target < last; advance(target, max<length_t>(lastOccurences_[fold(*target)], patternLast_ - pattern))) {
					for(pattern = patternLast_ - 1; fold(*target) == fold(*pattern); --target, --pattern) {
						if(pattern == patternFirst_)
							return target;
					}
				}
			} else {
				iterator_traits<Iterator>::difference_type skipLength;
				Iterator target(last);
				advance(target, patternFirst_ - patternLast_);
				for(const Char* pattern; ; advance(target, -skipLength)) {
					for(pattern = patternFirst_; fold(*target) == fold(*pattern); ++target, ++pattern) {
						if(pattern == patternLast_ - 1) {
							advance(target, patternFirst_ - patternLast_ + 1);
							return target;
						}
					}
					skipLength = max(lastOccurences_[fold(*target)], pattern - patternFirst_ + 1);
					if(skipLength > distance(first, target))
						break;
				}
			}
			return 0;
		}
	private:
		Char fold(Char ch) const throw() {	// TODO: handle as UCS-4.
			switch(caseFolding_) {
			case CASEFOLDING_ASCII:				return CaseFolder::foldASCII(ch);
			case CASEFOLDING_UNICODE_SIMPLE:	return CaseFolder::foldSimple(ch);
			default:							return ch;
			}
		}
	private:
		Direction direction_;
		CaseFoldingType caseFolding_;
		const Char* patternFirst_;
		const Char* patternLast_;
		ptrdiff_t lastOccurences_[65536];
	};
} // namespace @0


// concrete pattern matchers ////////////////////////////////////////////////

namespace {
	inline bool checkWordBoundary(const MatchTarget& target,
			const Char* matchedFirst, const Char* matchedLast, const CharacterDetector& ctypes) {
		CStringCharacterIterator i(target.entireFirst, target.entireLast, matchedFirst);
		WordBreakIterator<CStringCharacterIterator> ii(i, AbstractWordBreakIterator::START_OF_SEGMENT, ctypes);
		if(ii.isBoundary(i)) {
			CStringCharacterIterator j(target.entireFirst, target.entireLast, matchedFirst);
			WordBreakIterator<CStringCharacterIterator> jj(j, AbstractWordBreakIterator::END_OF_SEGMENT, ctypes);
			if(jj.isBoundary(j))
				return true;
		}
		return false;
	}

	class LiteralMatcher : virtual public ascension::searcher::internal::IPatternMatcher {
	public:
		LiteralMatcher(const Char* first, const Char* last, CaseFoldingType caseFolding)
			: pattern_(first, last), engine_(new BMSearcher(FORWARD, first, last, caseFolding)) {}
		const String& getPattern() const throw() {return pattern_;}
		bool matches(const MatchTarget& target, const CharacterDetector* ctypes) const {
			const length_t len = target.last - target.first;
			if(len != pattern_.length())
				return false;
			switch(engine_->getCaseFoldingType()) {
			case CASEFOLDING_ASCII:
				if(!CaseFolder::compare<CASEFOLDING_ASCII>(target.first, pattern_.data(), pattern_.length()))
					return false;
				break;
			case CASEFOLDING_UNICODE_SIMPLE:
				if(!CaseFolder::compare<CASEFOLDING_UNICODE_SIMPLE>(target.first, pattern_.data(), pattern_.length()))
					return false;
				break;
			default:
				if(!CaseFolder::compare<CASEFOLDING_NONE>(target.first, pattern_.data(), pattern_.length()))
					return false;
				break;
			}
			return ctypes == 0 || checkWordBoundary(target, target.first, target.last, *ctypes);
		}
		bool replace(const MatchTarget& target, String& replacement, const CharacterDetector* ctypes) const {return matches(target, ctypes);}
		bool search(const MatchTarget& target, Direction direction, MatchedRange& result, const CharacterDetector* ctypes) const {
			if(direction != engine_->getDirection())
				const_cast<LiteralMatcher*>(this)->engine_.reset(
					new BMSearcher(direction, pattern_.data(), pattern_.data() + pattern_.length(), engine_->getCaseFoldingType()));
			const Char* p = target.first;
			while(p < target.last && (result.first = engine_->search(p, target.last))) {
				result.last = result.first + pattern_.length();
				if(ctypes == 0 || checkWordBoundary(target, result.first, result.last, *ctypes))
					return true;
				p = surrogates::next(p, target.last);
			}
			return false;
		}
	private:
		const String pattern_;
		std::auto_ptr<BMSearcher> engine_;
	};

#ifndef ASCENSION_NO_REGEX
	auto_ptr<regex::MatchResult<const Char*> > searchRegexPattern(
			const regex::Pattern& pattern, const MatchTarget& target, Direction direction, const CharacterDetector* ctypes) {
		auto_ptr<regex::MatchResult<const Char*> > result;
		regex::Pattern::MatchOptions flags = regex::Pattern::NORMAL;

		if(direction == FORWARD) {	// 前方検索
			const Char* p = target.first;	// 検索開始位置
			if(target.first != target.entireFirst)
				flags |= regex::Pattern::TARGET_FIRST_IS_NOT_BOL;
			if(target.last != target.entireLast)
				flags |= regex::Pattern::TARGET_LAST_IS_NOT_EOL;
			do {
				result = pattern.search(p, target.entireLast, flags);
				if(result.get() == 0
						|| ctypes == 0 || checkWordBoundary(target, result->getStart(), result->getEnd(), *ctypes))	// 単語単位で探す場合
					break;
				p = result->getEnd();	// 次の検索開始位置へ
				flags |= regex::Pattern::TARGET_FIRST_IS_NOT_BOL;
			} while(p < target.last);
		} else {	// 後方検索
			// マッチ対象先頭でのみマッチするようにする
			flags |= regex::Pattern::MATCH_AT_ONLY_TARGET_FIRST;
			// 見つかるまで行頭にマッチ対象を拡大し続ける
			for(const Char* p = surrogates::previous(target.entireFirst, target.last);
					p >= target.first; p = surrogates::previous(target.entireFirst, p)) {
				if(p == target.entireFirst)
					flags |= regex::Pattern::TARGET_FIRST_IS_NOT_BOL;
				result = pattern.search(p, target.entireLast, flags);
				if(result.get() == 0) {
					if(p <= target.first)
						break;
					continue;
				}
				if(ctypes == 0 || checkWordBoundary(target, result->getStart(), result->getEnd(), *ctypes))	// 単語単位で探す場合
					break;
			}
		}
		return result;
	}

	class RegexMatcher : virtual public ascension::searcher::internal::IPatternMatcher {
	public:
		RegexMatcher(const Char* first, const Char* last, bool ignoreCase)
			: pattern_(first, last, ignoreCase ? CASEFOLDING_UNICODE_SIMPLE : CASEFOLDING_NONE) {}
		const String& getPattern() const throw() {static String p; p.assign(pattern_.getPatternString()); return p;}
		bool matches(const MatchTarget& target, const CharacterDetector* ctypes) const {
			result_ = pattern_.matches(target.first, target.last,
				(target.first == target.entireFirst ? 0 : regex::Pattern::TARGET_FIRST_IS_NOT_BOL)
				|| (target.last == target.entireLast ? 0 : regex::Pattern::TARGET_LAST_IS_NOT_EOL));
			if(result_.get() != 0 && (ctypes == 0 || checkWordBoundary(target, result_->getStart(), result_->getEnd(), *ctypes)))
				return true;
			result_.reset(0);
			return false;
		}
		bool replace(const MatchTarget& target, String& replacement, const CharacterDetector* ctypes) const {
			if(!matches(target, ctypes))
				return false;
			assert(result_.get() != 0);
			replacement.assign(result_->replace(replacement.data(), replacement.data() + replacement.length()));
			return true;
		}
		bool search(const MatchTarget& target, Direction direction, MatchedRange& result, const CharacterDetector* ctypes) const {
			result_ = searchRegexPattern(pattern_, target, direction, ctypes);
			if(result_.get() == 0)
				return false;
			result.first = result_->getStart();
			result.last = result_->getEnd();
			return true;
		}
	private:
		regex::Pattern pattern_;
		mutable auto_ptr<regex::MatchResult<const Char*> > result_;
	};
#endif /* !ASCENSION_NO_REGEX */

#ifndef ASCENSION_NO_MIGEMO
	class MigemoMatcher : virtual public ascension::searcher::internal::IPatternMatcher {
	public:
		MigemoMatcher(const Char* first, const Char* last, bool ignoreCase)
			: pattern_(regex::MigemoPattern::create(first, last, ignoreCase)) {if(pattern_.get() == 0) throw runtime_error("Migemo is not installed.");}
		const String& getPattern() const throw() {static String p; p.assign(pattern_->getPatternString()); return p;} 
		bool matches(const MatchTarget& target, const CharacterDetector* ctypes) const {
			auto_ptr<regex::MatchResult<const Char*> > result(pattern_->matches(target.first, target.last,
				(target.first == target.entireFirst ? 0 : regex::Pattern::TARGET_FIRST_IS_NOT_BOL)
				|| (target.last == target.entireLast ? 0 : regex::Pattern::TARGET_LAST_IS_NOT_EOL)));
			return result.get() != 0 && (ctypes == 0 || checkWordBoundary(target, result->getStart(), result->getEnd(), *ctypes));
		}
		bool replace(const MatchTarget& target, String& replacement, const CharacterDetector* ctypes) const {return matches(target, ctypes);}
		bool search(const MatchTarget& target, Direction direction, MatchedRange& result, const CharacterDetector* ctypes) const {
			auto_ptr<regex::MatchResult<const Char*> > r(searchRegexPattern(*pattern_, target, direction, ctypes));
			if(r.get() == 0)
				return false;
			result.first = r->getStart();
			result.last = r->getEnd();
			return true;
		}
	private:
		std::auto_ptr<regex::MigemoPattern> pattern_;
	};
#endif /* !ASCENSION_NO_MIGEMO */

} // namespace @0


// private helpers //////////////////////////////////////////////////////////

namespace {
	/// 検索対象文字列走査イテレータ
	class FoldedSearchTargetIterator : public iterator<bidirectional_iterator_tag, Char> {
	public:
	private:
		const Document& document_;
		length_t line_, pos_;
		String foldedContent_;
		vector<length_t> shortendPositions_;
	};
} // namespace @0

namespace {	// フォルダの定義
	const CodePoint	SKIPPED = 0xFFFFFFFF;
	template<int> class CharFolder {
		public:	static CodePoint fold(CodePoint cp);
	};

	const Char smallFormsFoldingTable[] = {	// for Folder<FO_SMALL_FORMS>
		0x002C, 0x3001, 0x002E, 0x003B, 0x003A, 0x003F, 0x0021, 0x2014,
		0x0028, 0x0029, 0x007B, 0x007D, 0x3014, 0x3015, 0x0023, 0x0026,
		0x002A, 0x002B, 0x002D, 0x003C, 0x003E, 0x003D, 0x005C, 0x0024,
		0x0025, 0x0040,
	};

	template<> inline CodePoint CharFolder<foldingoptions::CANONICAL_DUPLICATES>::fold(CodePoint cp) {
		switch(cp) {
		case 0x0374:	return 0x02B9;	// Greek Numeral Sign -> Modifier Letter Prime
		case 0x037E:	return 0x003B;	// Greek Question Mark -> Semicolon
		case 0x0387:	return 0x00B7;	// Greek Ano Teleia -> Middle Dot
		case 0x1FBE:	return 0x03B9;	// Greek Prosgegrammeni -> Greek Small Letter Iota
		case 0x1FEF:	return 0x0060;	// Greek Varia -> Grave Accent
		case 0x1FFD:	return 0x00B4;	// Greek Oxia -> Acute Accent
		case 0x2000:	return 0x2002;	// En Quad -> En Space
		case 0x2001:	return 0x2003;	// Em Quad -> Em Space
		case 0x2126:	return 0x03A9;	// Ohm Sign -> Greek Capital Letter Omega
		case 0x212A:	return 0x004B;	// Kelvin Sign -> Latin Capital Letter K
		case 0x212B:	return 0x00C5;	// Angstrom Sign -> Latin Capital Letter A With Ring Above
		case 0x2329:	return 0x3008;	// Left-Pointing Angle Bracket -> Left Angle Bracket
		case 0x232A:	return 0x3009;	// Right-Pointing Angle Bracket -> Right Angle Bracket
		default:		return cp;
		}
	}
	template<> inline CodePoint CharFolder<foldingoptions::DASHES>::fold(CodePoint cp) {
		return GeneralCategory::of(cp) == GeneralCategory::PUNCTUATION_DASH ? 0x002D : cp;
	}
	template<> inline CodePoint CharFolder<foldingoptions::GREEK_LETTERFORMS>::fold(CodePoint cp) {
		switch(cp) {
		case 0x03D0:	return 0x03B2;	// Greek Beta Symbol -> Greek Small Letter Beta
		case 0x03D1:	return 0x03B8;	// Greek Theta Symbol -> Greek Small Letter Theta
		case 0x03D2:	return 0x03A5;	// Greek Upsilon With Hook Symbol -> Greek Capital Letter Upsilon
		case 0x03D5:	return 0x03C6;	// Greek Phi Symbol -> Greek Small Letter Phi
		case 0x03D6:	return 0x03C0;	// Greek Pi Symbol -> Greek Small Letter Pi
		case 0x03F0:	return 0x03BA;	// Greek Kappa Symbol -> Greek Small Letter Kappa
		case 0x03F1:	return 0x03C1;	// Greek Rho Symbol -> Greek Small Letter Rho
		case 0x03F2:	return 0x03C2;	// Greek Lunate Sigma Symbol -> Greek Small Letter Final Sigma
		case 0x03F4:	return 0x0398;	// Greek Capital Theta Symbol -> Greek Capital Letter Theta
		case 0x03F5:	return 0x03B5;	// Greek Lunate Epsilon Symbol -> Greek Small Letter Epsilon
		default:		return cp;
		}
	}
	template<> inline CodePoint CharFolder<foldingoptions::HEBREW_ALTERNATES>::fold(CodePoint cp) {
		if(cp >= 0xFB20 && cp <= 0xFB28) {
			static const wchar_t hebrewAlternatives[] = {
				0x05E2, 0x05D0, 0x05D3, 0x05D4, 0x05DB, 0x05DC, 0x05DD, 0x05E8, 0x05EA
			};
			return hebrewAlternatives[cp - 0xFB20];
		}
		return cp;
	}
	template<> inline CodePoint CharFolder<foldingoptions::OVERLINE>::fold(CodePoint cp) {
		return (cp >= 0xFE49 && cp <= 0xFE4C) ? 0x203E : cp;
	}
//	template<> inline CodePoint CharFolder<foldingoptions::NATIVE_DIGIT>::fold(CodePoint cp) {
//		return CharacterFolder::foldDigit(cp);
//	}
	template<> inline CodePoint CharFolder<foldingoptions::NOBREAK>::fold(CodePoint cp) {
		switch(cp) {
		case 0x00A0:	return 0x0020;	// No-Break Space -> Space
		case 0x0F0C:	return 0x0F0D;	// Tibetan Mark Delimiter Tsheg Bstar -> Tibetan Mark Shad
		case 0x2007:	return 0x0020;	// Figure Space -> Space
		case 0x2011:	return 0x2010;	// Non-Breaking Hyphen -> Hyphen
		case 0x202F:	return 0x0020;	// Narrow No-Break Space -> Space
		default:		return cp;
		}
	}
	template<> inline CodePoint CharFolder<foldingoptions::SPACE>::fold(CodePoint cp) {
		return (GeneralCategory::of(cp) == GeneralCategory::SEPARATOR_SPACE) ? 0x0020 : cp;
	}
	template<> inline CodePoint CharFolder<foldingoptions::SMALL_FORMS>::fold(CodePoint cp) {
		return (cp >= 0xFE50 && cp <= 0xFE6B) ? smallFormsFoldingTable[cp - 0xFE50] : cp;
	}
	template<> inline CodePoint CharFolder<foldingoptions::UNDERLINE>::fold(CodePoint cp) {
		return (cp == 0x2017 || (cp >= 0xFE4D && cp <= 0xFE4F)) ? 0x005E : cp;
	}

	template<> inline CodePoint CharFolder<foldingoptions::KANA>::fold(CodePoint cp) {
		// 片仮名に統一
		return ((cp >= 0x30A1 && cp <= 0x30F6) || cp == 0x30FD || cp == 0x30FE) ? cp - 0x0060 : cp;
	}
	template<> inline CodePoint CharFolder<foldingoptions::LETTER_FORMS>::fold(CodePoint cp) {
		// based on LetterformFolding.txt obtained from UTR #30
		switch(cp) {
		case 0x017F:	return 0x0073;	// Latin Small Letter Long S -> Latin Small Letter S
		case 0x03D0:	return 0x03B2;	// Greek Beta Symbol -> Greek Small Letter Beta
		case 0x03D1:	return 0x03B8;	// Greek Theta Symbol -> Greek Small Letter Theta
		case 0x03D2:	return 0x03A5;	// Greek Upsilon With Hook Symbol -> Greek Capital Letter Upsilon
		case 0x03D5:	return 0x03C6;	// Greek Phi Symbol -> Greek Small Letter Phi
		case 0x03D6:	return 0x03C0;	// Greek Pi Symbol -> Greek Small Letter Pi
		case 0x03F0:	return 0x03BA;	// Greek Kappa Symbol -> Greek Small Letter Kappa
		case 0x03F1:	return 0x03C1;	// Greek Rho Symbol -> Greek Small Letter Rho
		case 0x03F2:	return 0x03C2;	// Greek Lunate Sigma Symbol -> Greek Small Letter Final Sigma
		case 0x03F4:	return 0x0398;	// Greek Capital Theta Symbol -> Greek Capital Letter Theta
		case 0x03F5:	return 0x03B5;	// Greek Lunate Epsilon Symbol -> Greek Small Letter Epsilon
		case 0x03F9:	return 0x03A3;	// Greek Capital Lunate Sigma Symbol -> Greek Capital Letter Sigma
		case 0x2107:	return 0x0190;	// Euler Constant -> Latin Capital Letter Open E
		case 0x2135:	return 0x05D0;	// Alef Symbol -> Hebrew Letter Alef
		case 0x2136:	return 0x05D1;	// Bet Symbol -> Hebrew Letter Bet
		case 0x2137:	return 0x05D2;	// Gimel Symbol -> Hebrew Letter Gimel
		case 0x2138:	return 0x05D3;	// Dalet Symbol -> Hebrew Letter Dalet
		default:		return cp;
		}
	}
	template<> inline CodePoint CharFolder<foldingoptions::WIDTH>::fold(CodePoint cp) {
		static Char buffer[2];
		if(cp >= 0x10000)
			return cp;
		if(::LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_HALFWIDTH, reinterpret_cast<WCHAR*>(&cp), 1, buffer, 1) != 0)
			return buffer[0];
		return cp;
		//return toHalfWidth(ch);	// 半角カナは対象外
	}

	template<> inline CodePoint CharFolder<foldingoptions::SKIP_CONTROLS>::fold(CodePoint cp) {
		const int gc = GeneralCategory::of(cp);
		return (gc == GeneralCategory::OTHER_CONTROL || gc == GeneralCategory::OTHER_FORMAT) ? SKIPPED : cp;
	}
	template<> inline CodePoint CharFolder<foldingoptions::SKIP_DIACRITICS>::fold(CodePoint cp) {
		return BinaryProperty::is<BinaryProperty::DIACRITIC>(cp) ? SKIPPED : cp;
	}
	template<> inline CodePoint CharFolder<foldingoptions::SKIP_KASHIDA>::fold(CodePoint cp) {
		// does not consider <U+FE71: Arabic Tatweel With Fathatan Above> like IE6...
		return (cp == 0x0640) ? SKIPPED : cp;	// Arabic Tatweel
	}
	template<> inline CodePoint CharFolder<foldingoptions::SKIP_PUNCTUATIONS>::fold(CodePoint cp) {
		return GeneralCategory::is<GeneralCategory::PUNCTUATION>(cp) ? SKIPPED : cp;
	}
	template<> inline CodePoint CharFolder<foldingoptions::SKIP_SYMBOLS>::fold(CodePoint cp) {
		return GeneralCategory::is<GeneralCategory::SYMBOL>(cp) ? SKIPPED : cp;
	}
//	template<> inline CodePoint CharFolder<FoldingOptions::SKIP_VOWELS>::fold(CodePoint cp) {}
	template<> inline CodePoint CharFolder<foldingoptions::SKIP_WHITESPACES>::fold(CodePoint cp) {
		return BinaryProperty::is<BinaryProperty::WHITE_SPACE>(cp) ? SKIPPED : cp;
	}
} // namespace @0


// TextSearcher /////////////////////////////////////////////////////////////

/// Default constructor.
TextSearcher::TextSearcher() : changedFromLast_(false) {
}

/// Destructor.
TextSearcher::~TextSearcher() {
	clearPatternCache();
}

/// Clears the cache of the search pattern.
void TextSearcher::clearPatternCache() {
	matcher_.reset(0);
}

/**
 * Compiles the regular expression pattern.
 * @return succeeded or not
 */
void TextSearcher::compilePattern() const throw() {
	TextSearcher& self = *const_cast<TextSearcher*>(this);
	if(matcher_.get() != 0)
		return;
#ifndef ASCENSION_NO_REGEX
	else if(options_.type == REGULAR_EXPRESSION)
		self.matcher_.reset(new RegexMatcher(patternString_.data(), patternString_.data() + patternString_.length(),
			(options_.foldings.caseFolding != CASEFOLDING_UNICODE_FULL) ? options_.foldings.caseFolding : CASEFOLDING_NONE));
#endif /* !ASCENSION_NO_REGEX */
#ifndef ASCENSION_NO_MIGEMO
	else if(options_.type == MIGEMO)
		self.matcher_.reset(new MigemoMatcher(patternString_.data(), patternString_.data() + patternString_.length(),
			(options_.foldings.caseFolding != CASEFOLDING_UNICODE_FULL) ? options_.foldings.caseFolding : CASEFOLDING_NONE));
#endif /* !ASCENSION_NO_MIGEMO */
	else
		self.matcher_.reset(new LiteralMatcher(patternString_.data(), patternString_.data() + patternString_.length(),
			(options_.foldings.caseFolding != CASEFOLDING_UNICODE_FULL) ? options_.foldings.caseFolding : CASEFOLDING_NONE));
}

/// Returns true if Migemo is available.
bool TextSearcher::isMigemoAvailable() const throw() {
#ifdef ASCENSION_NO_MIGEMO
	return false;
#else
	return regex::MigemoPattern::isMigemoInstalled();
#endif /* ASCENSION_NO_MIGEMO */
}

/**
 * Performs the match.
 * @param target the target to be matched
 * @param ctypes the boundary definition
 * @return true if matched
 * @throw ... any exceptions specified by Boost.Regex will be thrown if the regular expression error occured
 */
bool TextSearcher::match(const MatchTarget& target, const CharacterDetector& ctypes) const {
	if(!target.isNormalized())
		throw invalid_argument("the target is not normalized.");
	return matcher_->matches(target, options_.wholeWord ? &ctypes : 0);
}

bool TextSearcher::replace(const MatchTarget& target, String& replaced, const CharacterDetector& ctypes) const {
	if(!target.isNormalized())
		throw invalid_argument("the target is not normalized.");
	replaced.assign(replacement_);
	return matcher_->replace(target, replaced, options_.wholeWord ? &ctypes : 0);
}

/**
 * Searches the pattern in the specified target string.
 * @param target 検索対象文字列。空文字列だと常に失敗する。後方検索の場合、@c first メンバが検索終了位置、@c last メンバが検索開始位置
 * @param direction the direction to search
 * @param[out] result the matched range
 * @param[in] ctypes the boundary definition
 * @return true if the pattern is found
 * @throw ... any exceptions specified by Boost.Regex will be thrown if the regular expression error occured
 */
bool TextSearcher::search(const MatchTarget& target, Direction direction, MatchedRange& result, const CharacterDetector& ctypes) const {
	if(!target.isNormalized())
		throw invalid_argument("the target is not normalized.");
	compilePattern();
	return matcher_->search(target, direction, result, options_.wholeWord ? &ctypes : 0);
}

/// 検索オプションの設定
void TextSearcher::setOptions(const Options& options) {
	if(options != options_) {
		options_ = options;
		changedFromLast_ = true;
		clearPatternCache();
		multilinePattern_ = 
			(options_.type == LITERAL && patternString_.find(L'\n') != String::npos)
			|| (options_.type == REGULAR_EXPRESSION && patternString_.find(L"\\n") != String::npos);
	}
}

/// 検索文字列の設定
void TextSearcher::setPattern(const String& pattern) {
	if(pattern != patternString_) {
		patternString_ = pattern;
		changedFromLast_ = true;
		clearPatternCache();
		multilinePattern_ = 
			(options_.type == LITERAL && patternString_.find(L'\n') != String::npos)
			|| (options_.type == REGULAR_EXPRESSION && patternString_.find(L"\\n") != String::npos);
	}
}


// DocumentSearcher /////////////////////////////////////////////////////////

/**
 * コンストラクタ
 * @param document 対象ドキュメント
 * @param searcher 検索オブジェクト
 * @param ctypes 単語境界の検出に使う文字分類
 */
DocumentSearcher::DocumentSearcher(const Document& document, const TextSearcher& searcher, const CharacterDetector& ctypes)
	: document_(document), searcher_(searcher), ctypes_(ctypes) {}

/**
 * 指定テキストの置換結果を計算する
 * @param target マッチ対象テキスト
 * @param[out] result 置換結果のテキスト
 * @return マッチの成否
 * @throw ... any exceptions specified by Boost.Regex will be thrown if the regular expression error occured
 */
bool DocumentSearcher::replace(const Region& target, String& result) const {
	const bool multiLineSel = target.getTop().line != target.getBottom().line;
	if(searcher_.isMultilinePattern() || !multiLineSel) {
		MatchTarget matchTarget;
		if(!multiLineSel) {	// マッチ対象が1行以内
			const String& line = document_.getLine(target.first.line);
			matchTarget.entireFirst = line.data();
			matchTarget.entireLast = line.data() + line.length();
			matchTarget.first = line.data() + target.getTop().column;
			matchTarget.last = line.data() + target.getBottom().column;
			return searcher_.replace(matchTarget, result, ctypes_);
		} else {	// マッチ対象が複数行
			auto_ptr<TargetDuplicate> duplicatedTarget(
				new TargetDuplicate(document_, target.getTop().line, target.getBottom().line - target.getTop().line + 1));
			matchTarget.entireFirst = duplicatedTarget->getBuffer();
			matchTarget.entireLast = matchTarget.entireFirst + duplicatedTarget->getLength();
			matchTarget.first = matchTarget.entireFirst + target.getTop().column;
			matchTarget.last = matchTarget.entireLast - (document_.getLineLength(target.getBottom().line) - target.getBottom().column);
			return searcher_.replace(matchTarget, result, ctypes_);
		}
	}
	return false;
}

/**
 * テキストのパターン検索を行う
 * @param target 検索範囲
 * @param forward 前方検索の場合 true
 * @param[out] result マッチ範囲
 * @retval true パターンが見つかった
 * @retval false パターンが見つからなかった (@p result の内容は無意味)
 * @throw ... any exceptions specified by Boost.Regex will be thrown if the regular expression error occured
 */
bool DocumentSearcher::search(const Region& target, Direction direction, Region& result) const {
	// 複数行マッチの場合はパターンの行数を数えておく -> 対象文字列はこの行数だけ用意する
	length_t patternLineCount = 1;
	if(searcher_.isMultilinePattern()) {
		const String& pattern = searcher_.getPattern();
		if(searcher_.getOptions().type == TextSearcher::LITERAL)
			patternLineCount = count(pattern.begin(), pattern.end(), L'\n');
		else {
			assert(searcher_.getOptions().type == TextSearcher::REGULAR_EXPRESSION);
			for(String::size_type i = 0; ; ++i, ++patternLineCount) {
				i = pattern.find(L"\\n", i);
				if(i == String::npos)
					break;
			}
		}
		assert(patternLineCount > 1);
	}

	const Position& begin = (direction == FORWARD) ? target.getTop() : target.getBottom();
	const Position& end = (direction == FORWARD) ? target.getBottom() : target.getTop();
	for(length_t i = begin.line, column = begin.column; ; i += (direction == FORWARD) ? 1 : -1) {
		MatchTarget searchTarget;
		auto_ptr<TargetDuplicate> duplicatedTarget;

		// 対象文字列の準備
		patternLineCount = min(ascension::internal::distance(i, end.line) + 1, patternLineCount);
		if(patternLineCount == 1) {	// 単一行マッチ
			const String& line = document_.getLine(i);
			searchTarget.entireFirst = line.data();
			searchTarget.entireLast = line.data() + line.length();
		} else {	// 複数行マッチ -> バッファを別に用意する
			duplicatedTarget.reset(new TargetDuplicate(document_, (direction == FORWARD) ? i : i - patternLineCount + 1, patternLineCount));
			searchTarget.entireFirst = duplicatedTarget->getBuffer();
			searchTarget.entireLast = searchTarget.entireFirst + duplicatedTarget->getLength();
		}
		if(direction == FORWARD) {
			searchTarget.first = searchTarget.entireFirst + ((i == begin.line) ? begin.column : 0);
			searchTarget.last = (i + patternLineCount - 1 < end.line) ?
				searchTarget.entireLast : searchTarget.entireLast - (document_.getLineLength(end.line) - end.column);
		} else {
			searchTarget.first = searchTarget.entireFirst + ((i - patternLineCount + 1 > end.line) ? 0 : end.column);
			searchTarget.last = searchTarget.entireLast - ((i == begin.line) ? document_.getLineLength(i) - begin.column : 0);
		}

		MatchedRange matchedRange;
		if(searcher_.search(searchTarget, direction, matchedRange, ctypes_)) {	// 見つかった
			if(patternLineCount == 1) {
				result.first = Position(i, matchedRange.first - searchTarget.entireFirst);
				result.second = Position(i, matchedRange.last - searchTarget.entireFirst);
			} else
				duplicatedTarget->getResult(matchedRange, result);
			return true;
		}
		if(i == end.line)	// 見つからなかった
			break;
	}
	return false;
}

/// コンストラクタ
DocumentSearcher::TargetDuplicate::TargetDuplicate(const Document& document, length_t startLine, length_t numberOfLines)
		: numberOfLines_(numberOfLines), startLine_(startLine) {
	// バッファの長さを計算する
	length_ = numberOfLines_ - 1;
	for(length_t i = startLine_; i < startLine_ + numberOfLines_; ++i)
		length_ += document.getLineLength(i);
	buffer_ = new Char[length_];

	// バッファを構築する (対象行を '\n' で連結する)
	Char* p = buffer_;
	lineHeads_ = new Char*[numberOfLines_];
	lineHeads_[0] = buffer_;
	for(length_t i = startLine_; ; ++i) {
		const String& s = document.getLine(i);
		wmemcpy(p, s.data(), s.length());
		if(i == startLine_ + numberOfLines_ - 1)
			break;
		p[s.length()] = L'\n';
		p += s.length() + 1;
		lineHeads_[i - startLine_ + 1] = p;	// 行頭の位置を覚えとく
	}
}

/// デストラクタ
DocumentSearcher::TargetDuplicate::~TargetDuplicate() {
	delete[] buffer_;
	delete[] lineHeads_;
}

/// 複製へのポインタをドキュメント位置に変換する
void DocumentSearcher::TargetDuplicate::getResult(const MatchedRange& range, Region& result) const {
	for(result.second.line = startLine_ + numberOfLines_ - 1; ; --result.second.line) {
		const Char* const lineHead = lineHeads_[result.second.line - startLine_];
		if(range.last >= lineHead) {
			result.second.column = range.last - lineHead;
			break;
		}
	}
	for(result.first.line = result.second.line; ; --result.first.line) {
		const Char* const lineHead = lineHeads_[result.first.line - startLine_];
		if(range.first >= lineHead) {
			result.first.column = range.first - lineHead;
			break;
		}
	}
}


// IncrementalSearcher //////////////////////////////////////////////////////

/// Aborts the search.
void IncrementalSearcher::abort() {
	if(isRunning()) {
		if(listener_ != 0)
			listener_->incrementalSearchAborted();
		caret_->select(firstStatus_->range);
		end();
	}
}

/**
 * Appends the specified character to the end of the current search pattern.
 * @param ch the character to be appended
 * @return true if the pattern is found
 * @throw IncrementalSearcher#NotRunningException the searcher is not running
 * @throw ... any exceptions specified by Boost.Regex will be thrown if the regular expression error occured
 */
bool IncrementalSearcher::addCharacter(Char ch) {
	if(!isRunning())
		throw NotRunningException();
	pattern_ += ch;
	operationHistory_.push(TYPE);
	return update();
}


/**
 * Appends the specified character to the end of the current search pattern.
 * @param cp the character to be appended
 * @return true if the pattern is found
 * @throw IncrementalSearcher#NotRunningException the searcher is not running
 * @throw ... any exceptions specified by Boost.Regex will be thrown if the regular expression error occured
 */
bool IncrementalSearcher::addCharacter(CodePoint cp) {
	if(!isRunning())
		throw NotRunningException();
	if(cp < 0x010000)
		return addCharacter(static_cast<Char>(cp));

	Char surrogates[2];
	surrogates::encode(cp, surrogates);
	return addString(surrogates, surrogates + 2);
}

/**
 * Appends the specified string to the end of the search pattern.
 * @param first the start of the string to be appended
 * @param last the end og the string to be appended
 * @return true if the pattern is found
 * @throw IncrementalSearcher#NotRunningException the searcher is not running
 * @throw std#invalid_argument the string is empty
 * @throw ... any exceptions specified by Boost.Regex will be thrown if the regular expression error occured
 */
bool IncrementalSearcher::addString(const Char* first, const Char* last) {
	assert(first != 0 && last != 0 && first <= last);
	if(!isRunning())
		throw NotRunningException();
	else if(first == last)
		throw invalid_argument("Added string is empty.");
	pattern_.append(first, last);
	for(const Char* p = first; p < last; ++p)
		operationHistory_.push(TYPE);
	return update();
}

/// Ends the search.
void IncrementalSearcher::end() {
	while(!statusHistory_.empty())
		statusHistory_.pop();
	if(listener_ != 0)
		listener_->incrementalSearchCompleted();
	caret_ = 0;
	searcher_ = 0;
	listener_ = 0;
	firstStatus_ = 0;
	lastPattern_ = pattern_;
	pattern_.erase();
}

/**
 * Jumps to the next matched position.
 * @param direction the new direction of the search
 * @return true if matched after jump
 * @throw IncrementalSearcher#NotRunningException the searcher is not running
 * @throw ... any exceptions specified by Boost.Regex will be thrown if the regular expression error occured
 */
bool IncrementalSearcher::jumpToNextMatch(Direction direction) {
	if(!isRunning())
		throw NotRunningException();

	if(pattern_.empty()) {
		statusHistory_.top().direction = direction;
		if(!lastPattern_.empty())
			return addString(lastPattern_);	// 前回終了時のパターンで検索
		else {
			if(listener_ != 0)
				listener_->incrementalSearchPatternChanged(IIncrementalSearcherListener::FOUND);
			return true;
		}
	} else if(!matched_
			&& !operationHistory_.empty()
			&& operationHistory_.top() == JUMP
			&& statusHistory_.top().direction == direction)
		return false;	// マッチしていない状態でジャンプしようとした
	else {
		const Status status = {Region(caret_->getTopPoint(), caret_->getBottomPoint()), direction};
		statusHistory_.push(status);
		operationHistory_.push(JUMP);
		return update();
	}
}

/**
 * 検索直後の状態に戻す
 * @throw #NotRunningException the searcher is not running
 */
void IncrementalSearcher::reset() {
	if(statusHistory_.empty())
		throw NotRunningException();

	while(!operationHistory_.empty())
		operationHistory_.pop();
	while(statusHistory_.size() > 1)
		statusHistory_.pop();
	pattern_.erase();
	caret_->select(statusHistory_.top().range);
	if(listener_ != 0)
		listener_->incrementalSearchPatternChanged(IIncrementalSearcherListener::FOUND);
}

/**
 * Starts the search.
 * @param caret the caret
 * @param searcher the text search object
 * @param type the type of the search
 * @param direction the initial search direction
 * @param listener the listener. can be @c null
 */
void IncrementalSearcher::start(viewers::Caret& caret,
		TextSearcher& searcher, TextSearcher::Type type, Direction direction, IIncrementalSearcherListener* listener /* = 0 */) {
	if(isRunning())
		end();

	const Status status = {Region(caret.getTopPoint(), caret.getBottomPoint()), direction};
	caret_ = &caret;
	searcher_ = &searcher;
	type_ = type;
	statusHistory_.push(status);
	firstStatus_ = &statusHistory_.top();
	if(listener_ = listener) {
		listener_->incrementalSearchStarted();
		listener_->incrementalSearchPatternChanged(IIncrementalSearcherListener::FOUND);
	}
}

/**
 * Undoes the last search command.
 * @return true if matched after the undo 
 * @throw #NotRunningException the searcher is not running
 * @throw #EmptyUndoBufferException can't undo
 * @throw ... any exceptions specified by Boost.Regex will be thrown if the regular expression error occured
 */
bool IncrementalSearcher::undo() {
	if(!isRunning())
		throw NotRunningException();
	else if(!canUndo())
		throw EmptyUndoBufferException();

	const Operation lastOperation = operationHistory_.top();
	operationHistory_.pop();
	if(lastOperation == TYPE) {	// 文字の入力を元に戻す -> 検索式の末尾を削る
		if(pattern_.length() > 1
				&& surrogates::isHighSurrogate(pattern_[pattern_.length() - 2])
				&& surrogates::isLowSurrogate(pattern_[pattern_.length() - 1]))
			pattern_.erase(pattern_.length() - 2);
		else
			pattern_.erase(pattern_.length() - 1);
		return update();
	} else if(lastOperation == JUMP) {	// 次のマッチ位置へのジャンプを元に戻す -> 1つ前の状態に戻る
		const Status lastStatus = statusHistory_.top();
		caret_->select(lastStatus.range);
		statusHistory_.pop();
		if(!matched_) {	// ジャンプを元に戻すと必ずマッチした状態になる
			matched_ = true;
			if(listener_ != 0)
				listener_->incrementalSearchPatternChanged(IIncrementalSearcherListener::FOUND);
		}
		return true;
	} else {
		assert(false);
		return false;	// あり得ぬ
	}
}

/**
 * Re-searches using the current state.
 * @return true if the pattern is found
 * @throw ... any exceptions specified by Boost.Regex will be thrown if the regular expression error occured
 */
bool IncrementalSearcher::update() {
	assert(!statusHistory_.empty());

	if(pattern_.empty()) {
		caret_->select(firstStatus_->range);
		matched_ = true;
		if(listener_ != 0)
			listener_->incrementalSearchPatternChanged(IIncrementalSearcherListener::FOUND);
		return true;
	}

	const Document& document = *caret_->getDocument();
	TextSearcher::Options options = searcher_->getOptions();
	const TextSearcher::Type originalType = options.type;
	const CharacterDetector& ctypes = caret_->getDocument()->getContentTypeInformation().getCharacterDetector(caret_->getContentType());
	const DocumentSearcher dsearch(document, *searcher_, ctypes);
	const Status& status = statusHistory_.top();

	options.type = type_;
	searcher_->setOptions(options);
	searcher_->setPattern(pattern_);

	Region matched;
	const Region scope(
		status.direction ? status.range.second : document.getStartPosition(),
		status.direction ? document.getEndPosition() : status.range.first);
	bool found;
#ifndef ASCENSION_NO_REGEX
	try {
#endif /* !ASCENSION_NO_REGEX */
		found = dsearch.search(scope, status.direction, matched);
#ifndef ASCENSION_NO_REGEX
	} catch(boost::regex_error&) {
		if(listener_ != 0)
			listener_->incrementalSearchPatternChanged(IIncrementalSearcherListener::BAD_REGEX);
		// 一時的に変更したオプションを元に戻す
		options.type = originalType;
		searcher_->setOptions(options);
		throw;
	} catch(runtime_error&) {
		if(listener_ != 0)
			listener_->incrementalSearchPatternChanged(IIncrementalSearcherListener::COMPLEX_REGEX);
		// 一時的に変更したオプションを元に戻す
		options.type = originalType;
		searcher_->setOptions(options);
		throw;
	}
#endif /* !ASCENSION_NO_REGEX */

	if(matched_ = found)
		caret_->select(matched);

	// 一時的に変更したオプションを元に戻す
	options.type = originalType;
	searcher_->setOptions(options);

	if(listener_ != 0)
		listener_->incrementalSearchPatternChanged(found ? IIncrementalSearcherListener::FOUND : IIncrementalSearcherListener::NOT_FOUND);
	return found;
}


// private helper ///////////////////////////////////////////////////////////

#ifndef ASCENSION_NO_REGEX
#endif /* !ASCENSION_NO_REGEX */
