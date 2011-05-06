/**
 * @file input-sequence-checker.hpp
 * @author exeal
 * @date 2006-2011 was session.hpp
 * @date 2011-05-06 separated from session.hpp
 */

#ifndef ASCENSION_INPUT_SEQUENCE_CHECKER_HPP
#define ASCENSION_INPUT_SEQUENCE_CHECKER_HPP

#include <ascension/config.hpp>
#include <ascension/corelib/string-piece.hpp>
#ifdef ASCENSION_OS_WINDOWS
#	include <ascension/win32/windows.hpp>	// HKL
#endif
#include <list>
#include <memory>


namespace ascension {
	namespace texteditor {

#ifdef ASCENSION_OS_WINDOWS
		typedef HKL NativeKeyboardLayout;
#else
#endif

		/**
		 * Base class for input sequence checkers.
		 * @see isc
		 */
		class InputSequenceChecker {
		public:
			/// Destructor.
			virtual ~InputSequenceChecker() /*throw()*/ {}
			/**
			 * Checks the sequence.
			 * @param keyboardLayout The active keyboard layout
			 * @param preceding The string preceding to the input
			 * @param c The code point of the character to be input
			 * @return true if the input is acceptable
			 */
			virtual bool check(NativeKeyboardLayout keyboardLayout, const StringPiece& preceding, CodePoint c) const = 0;
		};

		/**
		 * Collection of input sequence checkers.
		 * @see InputSequenceChecker, Session#getInputSequenceCheckers
		 */
		class InputSequenceCheckers {
			ASCENSION_NONCOPYABLE_TAG(InputSequenceCheckers);
		public:
			~InputSequenceCheckers();
			void add(std::auto_ptr<InputSequenceChecker> checker);
			bool check(const StringPiece& preceding, CodePoint c) const;
			void clear();
			bool isEmpty() const /*throw()*/;
			void setKeyboardLayout(NativeKeyboardLayout keyboardLayout) /*throw()*/;
		private:
			std::list<InputSequenceChecker*> strategies_;
			NativeKeyboardLayout keyboardLayout_;
		};

		/// Standard input sequence checkers.
		namespace isc {

			/// I.S.C. for Ainu.
			class AinuInputSequenceChecker : public InputSequenceChecker {
			public:
				bool check(HKL keyboardLayout, const StringPiece& preceding, CodePoint c) const;
			};

			/// I.S.C. for Thai.
			class ThaiInputSequenceChecker : public InputSequenceChecker {
				ASCENSION_UNASSIGNABLE_TAG(ThaiInputSequenceChecker);
			public:
				enum Mode {PASS_THROUGH, BASIC_MODE, STRICT_MODE};
				ThaiInputSequenceChecker(Mode mode = BASIC_MODE) /*throw()*/ : mode_(mode) {}
				bool check(HKL keyboardLayout, const StringPiece& preceding, CodePoint c) const;
			private:
				enum CharacterClass {
					CTRL, NON, CONS,	// treat unassigned characters in Thai block as controls
					LV, FV1, FV2, FV3, BV1, BV2,
					BD, TONE, AD1, AD2, AD3,
					AV1, AV2, AV3,
					CHARCLASS_COUNT
				};
				const Mode mode_;
				static const CharacterClass charClasses_[];
				static const char checkMap_[];
				static CharacterClass getCharacterClass(CodePoint cp) /*throw()*/ {
					if(cp < 0x0020u || cp == 0x007fu)		return CTRL;
					else if(cp >= 0x0e00u && cp < 0x0e60u)	return charClasses_[cp - 0x0e00u];
					else if(cp >= 0x0e60u && cp < 0x0e80u)	return CTRL;
					else									return NON;
				}
				static bool doCheck(CharacterClass lead, CharacterClass follow, bool strict) /*throw()*/ {
					const char result = checkMap_[lead * CHARCLASS_COUNT + follow];
					if(result == 'A' || result == 'C' || result == 'X')
						return true;
					else if(result == 'R')
						return false;
					else /* if(result == 'S') */
						return !strict;
				}
			};

			/// I.S.C. for Vietnamese.
			class VietnameseInputSequenceChecker : public InputSequenceChecker {
			public:
				bool check(HKL keyboardLayout, const StringPiece& preceding, CodePoint c) const;
			};
		} // namespace isc

	} // namespace texteditor
} // namespace ascension

#endif // !ASCENSION_INPUT_SEQUENCE_CHECKER_HPP
