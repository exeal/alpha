/**
 * @file input-sequence-checker.cpp
 * @author exeal
 * @date 2006-2011 was session.cpp
 * @date 2011-05-06 separated from session.cpp
 */

#include <ascension/text-editor/input-sequence-checker.hpp>
#include <ascension/corelib/basic-exceptions.hpp>
#include <algorithm>
using namespace ascension;
using namespace ascension::texteditor;
using namespace ascension::texteditor::isc;
using namespace std;


// InputSequenceCheckers //////////////////////////////////////////////////////////////////////////

/// Destructor.
InputSequenceCheckers::~InputSequenceCheckers() {
	for(list<InputSequenceChecker*>::iterator i = strategies_.begin(); i != strategies_.end(); ++i)
		delete *i;
}

/**
 * Registers the sequence checker.
 * @param checker The sequence checker to be registered.
 * @throw std#invalid_argument @a checker is already registered
 */
void InputSequenceCheckers::add(auto_ptr<InputSequenceChecker> checker) {
	if(find(strategies_.begin(), strategies_.end(), checker.get()) != strategies_.end())
		throw invalid_argument("Specified checker is already registered.");
	strategies_.push_back(checker.release());
}

/**
 * Checks the sequence.
 * @param preceding The string preceding the character to be input
 * @param c The code point of the character to be input
 * @return true if the input is acceptable
 * @throw NullPointerException @a receding is @c null
 */
bool InputSequenceCheckers::check(const StringPiece& preceding, CodePoint c) const {
	if(preceding.beginning() == 0 || preceding.end() == 0)
		throw NullPointerException("preceding");
	for(list<InputSequenceChecker*>::const_iterator i = strategies_.begin(); i != strategies_.end(); ++i) {
		if(!(*i)->check(keyboardLayout_, preceding, c))
			return false;
	}
	return true;
}

/// Removes all registered checkers.
void InputSequenceCheckers::clear() {
	for(list<InputSequenceChecker*>::iterator i = strategies_.begin(); i != strategies_.end(); ++i)
		delete *i;
	strategies_.clear();
}

/// Returns if no checker is registerd.
bool InputSequenceCheckers::isEmpty() const /*throw()*/ {
	return strategies_.empty();
}

/**
 * Activates the specified keyboard layout.
 * @param keyboardLayout The keyboard layout
 */
void InputSequenceCheckers::setKeyboardLayout(HKL keyboardLayout) /*throw()*/ {
	keyboardLayout_ = keyboardLayout;
}


// isc.AinuInputSequenceChecker ///////////////////////////////////////////////////////////////////

/// @see InputSequenceChecker#check
bool AinuInputSequenceChecker::check(HKL, const StringPiece& preceding, CodePoint c) const {
	// only check a pair consists of combining semi-voiced sound mark is valid
	return c != 0x309au || (preceding.beginning() < preceding.end() && (
		preceding.end()[-1] == L'\x30bb'		// se (セ)
		|| preceding.end()[-1] == L'\x30c4'		// tu (ツ)
		|| preceding.end()[-1] == L'\x30c8'		// to (ト)
		|| preceding.end()[-1] == L'\x31f7'));	// small fu (小さいフ)
}


// isc.ThaiInputSequenceChecker ///////////////////////////////////////////////////////////////////

const ThaiInputSequenceChecker::CharacterClass ThaiInputSequenceChecker::charClasses_[] = {
	CTRL, CONS, CONS, CONS, CONS, CONS, CONS, CONS,	// U+0E00
	CONS, CONS, CONS, CONS, CONS, CONS, CONS, CONS,
	CONS, CONS, CONS, CONS, CONS, CONS, CONS, CONS,	// U+0E10
	CONS, CONS, CONS, CONS, CONS, CONS, CONS, CONS,
	CONS, CONS, CONS, CONS, FV3,  CONS, FV3,  CONS,	// U+0E20
	CONS, CONS, CONS, CONS, CONS, CONS, CONS, NON,
	FV1,  AV2,  FV1,  FV1,  AV1,  AV3,  AV2,  AV3,	// U+0E30
	BV1,  BV2,  BD,   CTRL, CTRL, CTRL, CTRL, NON,
	LV,   LV,   LV,   LV,   LV,   FV2,  NON,  AD2,	// U+0E40
	TONE, TONE, TONE, TONE, AD1,  AD1,  AD3,  NON,
	NON,  NON,  NON,  NON,  NON,  NON,  NON,  NON,	// U+0E50
	NON,  NON,  NON,  NON,  CTRL, CTRL, CTRL, CTRL
};

const char ThaiInputSequenceChecker::checkMap_[] =
	"XAAAAAA" "RRRRRRRRRR"	// CTRL
	"XAAASSA" "RRRRRRRRRR"	// NON
	"XAAAASA" "CCCCCCCCCC"	// CONS
	"XSASSSS" "RRRRRRRRRR"	// LV
	"XSASASA" "RRRRRRRRRR"	// FV1
	"XAAAASA" "RRRRRRRRRR"	// FV2
	"XAAASAS" "RRRRRRRRRR"	// FV3
	"XAAAASA" "RRRCCRRRRR"	// BV1
	"XAAASSA" "RRRCRRRRRR"	// BV2
	"XAAASSA" "RRRRRRRRRR"	// BD 
	"XAAAAAA" "RRRRRRRRRR"	// TONE
	"XAAASSA" "RRRRRRRRRR"	// AD1	
	"XAAASSA" "RRRRRRRRRR"	// AD2	
	"XAAASSA" "RRRRRRRRRR"	// AD3	
	"XAAASSA" "RRRCCRRRRR"	// AV1	
	"XAAASSA" "RRRCRRRRRR"	// AV2	
	"XAAASSA" "RRRCRCRRRR";	// AV3

/// @see InputSequenceChecker#check
bool ThaiInputSequenceChecker::check(HKL, const StringPiece& preceding, CodePoint c) const {
	// standardized by WTT 2.0:
	// - http://mozart.inet.co.th/cyberclub/trin/thairef/wtt2/char-class.pdf
	// - http://www.nectec.or.th/it-standards/keyboard_layout/thai-key.htm
	if(mode_ == PASS_THROUGH)
		return true;
	// if there is not a preceding character, as if a control is
	// Sara Am -> Nikhahit + Sara Aa
	return doCheck(
		!preceding.isEmpty() ? getCharacterClass(preceding.end()[-1]) : CTRL,
		getCharacterClass((c != 0x0e33u) ? c : 0x0e4du),
		mode_ == STRICT_MODE);
}


// isc.VietnameseInputSequenceChecker /////////////////////////////////////////////////////////////

/// @see InputSequenceChecker#check
bool VietnameseInputSequenceChecker::check(HKL keyboardLayout, const StringPiece& preceding, CodePoint c) const {
	// The Vietnamese alphabet (quốc ngữ) has 12 vowels, 5 tone marks and other consonants. This
	// code checks if the input is conflicting the pattern <vowel> + <0 or 1 tone mark>. Does not
	// check when the input locale is not Vietnamese, because Vietnamese does not have own script
	// Like Uniscribe, ignored if the vowel is a composite.
	// 
	// Reference:
	// - Vietnamese alphabet (http://en.wikipedia.org/wiki/Vietnamese_alphabet)
	// - Vietnamese Writing System (http://www.cjvlang.com/Writing/writviet.html)
	static const CodePoint VOWELS[24] = {
		'A', 'E', 'I', 'O', 'U', 'Y',
		'a', 'e', 'i', 'o', 'u', 'y',
		0x00c2u, 0x00cau, 0x00d4u, 0x00e2u, 0x00eau, 0x00f4u,	// Â Ê Ô â ê ô
		0x0102u, 0x0103u, 0x01a0u, 0x01a1u, 0x01afu, 0x01b0u	// Ă ă Ơ ơ Ư ư
	};
	static const CodePoint TONE_MARKS[5] = {0x0300u, 0x0301u, 0x0303u, 0x0309u, 0x0323u};

	if(PRIMARYLANGID(LOWORD(keyboardLayout)) != LANG_VIETNAMESE)
		return true;
	else if(!preceding.isEmpty() && binary_search(TONE_MARKS, ASCENSION_ENDOF(TONE_MARKS), c))
		return binary_search(VOWELS, ASCENSION_ENDOF(VOWELS), preceding.end()[-1]);
	return true;
}
