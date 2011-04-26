/**
 * @file identifier-syntax.hpp
 * @author exeal
 * @date 2005-2011 was unicode.hpp
 * @date 2011-04-26 separated from unicode.hpp
 */

#ifndef ASCENSION_IDENTIFIER_SYNTAX_HPP
#define ASCENSION_IDENTIFIER_SYNTAX_HPP

#include <ascension/config.hpp>	// ASCENSION_NO_UNICODE_NORMALIZATION
#include <ascension/corelib/text/unicode.hpp>
#include <ascension/corelib/text/unicode-utf.hpp>	// UTF16To32Iterator
#include <set>

#if ASCENSION_UNICODE_VERSION > 0x0510
#	error These class definitions and implementations are based on old version of Unicode.
#endif

namespace ascension {

	namespace text {

		class IdentifierSyntax {
		public:
			/// Types of character classification used by @c IdentifierSyntax.
			enum CharacterClassification {
				/// Uses only 7-bit ASCII characters.
				ASCII,
				/// Classifies using @c text#ucd#legacyctype namespace functions.
				LEGACY_POSIX,
				/// Conforms to the default identifier syntax of UAX #31.
				UNICODE_DEFAULT,
				/// Conforms to the alternative identifier syntax of UAX #31.
				UNICODE_ALTERNATIVE
			};
			// constructors
			IdentifierSyntax() /*throw()*/;
			explicit IdentifierSyntax(CharacterClassification type, bool ignoreCase = false
#ifndef ASCENSION_NO_UNICODE_NORMALIZATION
				, Decomposition equivalenceType = NO_DECOMPOSITION
#endif // !ASCENSION_NO_UNICODE_NORMALIZATION
			) /*throw()*/;
			IdentifierSyntax(const IdentifierSyntax& other) /*throw()*/;
			IdentifierSyntax& operator=(const IdentifierSyntax& other) /*throw()*/;
			// singleton
			static const IdentifierSyntax& defaultInstance() /*throw()*/;
			// classification for character
			bool isIdentifierStartCharacter(CodePoint c) const /*throw()*/;
			bool isIdentifierContinueCharacter(CodePoint c) const /*throw()*/;
			bool isWhiteSpace(CodePoint c, bool includeTab) const /*throw()*/;

			// classification for sequence
			/**
			 * Checks whether the specified character sequence starts with an identifier.
			 * The type @a CharacterSequence the bidirectional iterator expresses a UTF-16
			 * character sequence. This method is exception-neutral (does not throw if
			 * @a CharacterSequence does not).
			 * @tparam CharacterSequence UTF-16 character sequence
			 * @param first The start of the character sequence
			 * @param last The end of the character sequence
			 * @return The end of the detected identifier or @a first if an identifier not found
			 */
			template<typename CharacterSequence>
			inline CharacterSequence eatIdentifier(
					CharacterSequence first, CharacterSequence last) const {
				ASCENSION_STATIC_ASSERT(CodeUnitSizeOf<CharacterSequence>::value == 2);
				UTF16To32Iterator<CharacterSequence> i(first, last);
				if(!i.hasNext() || !isIdentifierStartCharacter(*i))
					return first;
				while(i.hasNext() && isIdentifierContinueCharacter(*i))
					++i;
				return i.tell();
			}
			/**
			 * Checks whether the specified character sequence starts with white space characters.
			 * The type @a CharacterSequence is the bidirectional iterator expresses a UTF-16
			 * character sequence. This method is exception-neutral (does not throw if
			 * @a CharacterSequence does not).
			 * @tparam CharacterSequence UTF-16 character sequence
			 * @param first The start of the character sequence
			 * @param last The end of the character sequence
			 * @param includeTab Set @c true to treat a horizontal tab as a white space
			 * @return The end of the detected identifier or @a first if an identifier not found
			 */
			template<typename CharacterSequence>
			inline CharacterSequence eatWhiteSpaces(
					CharacterSequence first, CharacterSequence last, bool includeTab) const {
				ASCENSION_STATIC_ASSERT(CodeUnitSizeOf<CharacterSequence>::value == 2);
				UTF16To32Iterator<CharacterSequence> i(first, last);
				while(i.hasNext() && isWhiteSpace(*i, includeTab))
					++i;
				return i.tell();
			}

			// attributes
			void overrideIdentifierStartCharacters(
				const String& adding, const String& subtracting);
			void overrideIdentifierStartCharacters(
				const std::set<CodePoint>& adding, const std::set<CodePoint>& subtracting);
			void overrideIdentifierNonStartCharacters(
				const String& adding, const String& subtracting);
			void overrideIdentifierNonStartCharacters(
				const std::set<CodePoint>& adding, const std::set<CodePoint>& subtracting);
		private:
			CharacterClassification type_;
			bool caseSensitive_;
#ifndef ASCENSION_NO_UNICODE_NORMALIZATION
			Decomposition equivalenceType_;
#endif // !ASCENSION_NO_UNICODE_NORMALIZATION
			std::basic_string<CodePoint>
				addedIDStartCharacters_, addedIDNonStartCharacters_,
				subtractedIDStartCharacters_, subtractedIDNonStartCharacters_;
		};

	}
} // namespace ascension.text

#endif // !ASCENSION_IDENTIFIER_SYNTAX_HPP
