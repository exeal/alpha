/**
 * @file input-sequence-checker.hpp
 * @author exeal
 * @date 2006-2011 was session.hpp
 * @date 2011-05-06 separated from session.hpp
 */

#ifndef ASCENSION_INPUT_SEQUENCE_CHECKER_HPP
#define ASCENSION_INPUT_SEQUENCE_CHECKER_HPP

#include <ascension/corelib/string-piece.hpp>
#include <forward_list>
#include <locale>
#include <memory>


namespace ascension {
	namespace texteditor {
		/**
		 * Base class for input sequence checkers.
		 * @see isc
		 */
		class InputSequenceChecker {
		public:
			/// Destructor.
			virtual ~InputSequenceChecker() BOOST_NOEXCEPT {}
			/**
			 * Checks the sequence.
			 * @param lc The locale of the active input
			 * @param preceding The string preceding to the input
			 * @param c The code point of the character to be input
			 * @return true if the input is acceptable
			 */
			virtual bool check(const std::locale& lc, const StringPiece& preceding, CodePoint c) const = 0;
		};

		/**
		 * Collection of input sequence checkers.
		 * @see InputSequenceChecker, Session#getInputSequenceCheckers
		 */
		class InputSequenceCheckers : private boost::noncopyable {
		public:
			void add(std::unique_ptr<InputSequenceChecker> checker);
			bool check(const StringPiece& preceding, CodePoint c) const;
			void clear();
			bool isEmpty() const BOOST_NOEXCEPT;
			void imbue(const std::locale& lc) BOOST_NOEXCEPT;
			const std::locale& locale() const BOOST_NOEXCEPT;
		private:
			std::forward_list<std::unique_ptr<InputSequenceChecker>> strategies_;
			std::locale locale_;
		};

		/// Standard input sequence checkers.
		namespace isc {
			/// I.S.C. for Ainu.
			class AinuInputSequenceChecker : public InputSequenceChecker {
			public:
				bool check(const std::locale& lc, const StringPiece& preceding, CodePoint c) const;
			};

			/// I.S.C. for Thai.
			class ThaiInputSequenceChecker : public InputSequenceChecker {
			public:
				enum Mode {PASS_THROUGH, BASIC_MODE, STRICT_MODE};
				ThaiInputSequenceChecker(Mode mode = BASIC_MODE) BOOST_NOEXCEPT : mode_(mode) {}
				bool check(const std::locale& lc, const StringPiece& preceding, CodePoint c) const;
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
				static CharacterClass getCharacterClass(CodePoint cp) BOOST_NOEXCEPT {
					if(cp < 0x0020u || cp == 0x007fu)
						return CTRL;
					else if(cp >= 0x0e00u && cp < 0x0e60u)
						return charClasses_[cp - 0x0e00u];
					else if(cp >= 0x0e60u && cp < 0x0e80u)
						return CTRL;
					else
						return NON;
				}
				static bool doCheck(CharacterClass lead, CharacterClass follow, bool strict) BOOST_NOEXCEPT {
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
				bool check(const std::locale& lc, const StringPiece& preceding, CodePoint c) const;
			};
		} // namespace isc
	} // namespace texteditor
} // namespace ascension

#endif // !ASCENSION_INPUT_SEQUENCE_CHECKER_HPP
