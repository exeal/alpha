/**
 * @file identifier-syntax.cpp
 * @author exeal
 * @date 2007-2014
 */

#include <ascension/config.hpp>	// ASCENSION_DEFAULT_CHARACTER_CLASSIFICATION
#include <ascension/corelib/basic-exceptions.hpp>	// ASCENSION_ASSERT_NOT_REACHED
#include <ascension/corelib/text/identifier-syntax.hpp>
#include <ascension/corelib/text/character-property.hpp>
#include <ascension/corelib/text/utf-iterator.hpp>	// utf.CharacterDecodeIterator
#include <boost/foreach.hpp>
#include <boost/range/algorithm/binary_search.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/set_algorithm.hpp>
#include <boost/range/algorithm/sort.hpp>
#include <functional>	// std.function
#include <vector>


namespace ascension {
	namespace text {
		// IdentifierSyntax ///////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * @class ascension::text::IdentifierSyntax identifier-syntax.hpp
		 * An @c IdentifierSyntax classifies characters and detects identifiers based on
		 * <a href="http://www.unicode.org/reports/tr31/">UAX #31: Identifier and Pattern Syntax</a>.
		 *
		 * This class conforms to Unicode 5.0 UAX #31 revision 7 Level 1.
		 *
		 * <dl>
		 *   <dt>R1 Default Identifiers</dt>
		 *   <dd>Supported if the character classification is @c UNICODE_DEFAULT.</dd>
		 *   <dt>R2 Alternative Identifiers</dt>
		 *   <dd>Supported if the character classification is @c UNICODE_ALTERNATIVE.</dd>
		 *   <dt>R3 Pattern_White_Space and Pattern_Syntax Characters</dt>
		 *   <dd>@c #isWhiteSpace and @c eatWhiteSpaces methods interpret white space characters according
		 *     to the character classification:
		 *     - Only SPACE (U+0020), when @c ASCII is set.
		 *     - Characters that legacyctype#isspace returns true, when @c LEGACY_POSIX is set.
		 *     - Characters have binary property Pattern_White_Space, when either @c UNICODE_DEFAULT or
		 *       @c UNICODE_ALTERNATIVE is set.
		 *     Pattern_Syntax property is not recognized by this class.</dd>
		 *   <dt>R4 Normalized Identifiers</dt>
		 *   <dd>Supported if the normalization type is not @c Normalizer#DONT_NORMALIZE.</dd>
		 *   <dt>R5 Case-Insensitive Identifiers</dt>
		 *   <dd>Supported if the case folding type is not @c CASEFOLDING_NONE.</dd>
		 * </dl>
		 *
		 * <h3>Definitions relative to the Unicode identifier syntax</h3>
		 *
		 * This class provides methods to add/remove characters to/from default code point categories for identifier
		 * syntax. For example, the following code defines '_' as the identifier non-start character:
		 *
		 * @code
		 * IdentifierSyntax is;
		 * is.overrideIdentifierNonStartCharacters(L"_", L"");  // as if String is std.wstring...
		 * @endcode
		 *
		 * @see kernel#ContentTypeInformationProvider#identifierSyntax
		 */

		namespace {
			template<typename SinglePassReadableRange>
			void implementOverrides(
					const SinglePassReadableRange& adding, const SinglePassReadableRange& subtracting,
					std::basic_string<CodePoint>& added, std::basic_string<CodePoint>& subtracted) {
				std::basic_stringbuf<CodePoint> addingBuffer, subtractingBuffer;
				BOOST_FOREACH(const auto& a, adding)
					addingBuffer.sputc(a);
				BOOST_FOREACH(const auto& s, subtracting)
					subtractingBuffer.sputc(s);
				std::basic_string<CodePoint> a(addingBuffer.str()), s(subtractingBuffer.str());
				boost::sort(a);
				boost::sort(s);
				std::vector<CodePoint> v;
				boost::set_intersection(a, s, back_inserter(v));
				if(!boost::empty(v))
					throw std::invalid_argument("same character was found the both sets");
				added = a;
				subtracted = s;
			}
		} // namespace @0

		/**
		 * Default constructor. The character classification type is initialized to
		 * @c ASCENSION_DEFAULT_CHARACTER_DETECTION_TYPE.
		 */
		IdentifierSyntax::IdentifierSyntax() BOOST_NOEXCEPT : type_(ASCENSION_DEFAULT_CHARACTER_CLASSIFICATION), caseSensitive_(true)
#ifndef ASCENSION_NO_UNICODE_NORMALIZATION
				, equivalenceType_(NO_DECOMPOSITION)
#endif // !ASCENSION_NO_UNICODE_NORMALIZATION
		{
		}

		/// Copy-constructor.
		IdentifierSyntax::IdentifierSyntax(const IdentifierSyntax& rhs) BOOST_NOEXCEPT : type_(rhs.type_), caseSensitive_(rhs.caseSensitive_),
#ifndef ASCENSION_NO_UNICODE_NORMALIZATION
				equivalenceType_(rhs.equivalenceType_),
#endif // !ASCENSION_NO_UNICODE_NORMALIZATION
				addedIDStartCharacters_(rhs.addedIDStartCharacters_), addedIDNonStartCharacters_(rhs.addedIDNonStartCharacters_),
				subtractedIDStartCharacters_(rhs.subtractedIDStartCharacters_), subtractedIDNonStartCharacters_(rhs.subtractedIDNonStartCharacters_)
		{
		}

		/**
		 * Constructor.
		 * @param type The classification type
		 * @param ignoreCase Set @c true to perform caseless match
		 * @param equivalenceType The decomposition type for canonical/compatibility equivalents
		 */
		IdentifierSyntax::IdentifierSyntax(CharacterClassification type, bool ignoreCase /* = false */
#ifndef ASCENSION_NO_UNICODE_NORMALIZATION
				, Decomposition equivalenceType /* = NO_DECOMPOSITION */
#endif // !ASCENSION_NO_UNICODE_NORMALIZATION
		) BOOST_NOEXCEPT : type_(type), caseSensitive_(ignoreCase)
#ifndef ASCENSION_NO_UNICODE_NORMALIZATION
				, equivalenceType_(equivalenceType)
#endif // !ASCENSION_NO_UNICODE_NORMALIZATION
		{
		}

		/**
		 * Returns the default @c IdentifierSyntax singleton instance. This instance has @c UNICODE_DEFAULT character
		 * classification type.
		 */
		const IdentifierSyntax& IdentifierSyntax::defaultInstance() BOOST_NOEXCEPT {
			static const IdentifierSyntax instance(UNICODE_DEFAULT);
			return instance;
		}

		/**
		 * Returns @c true if the specified character is ID_Continue.
		 * @param c The code point of the character
		 * @return true if @a c is identifier continue character
		 */
		bool IdentifierSyntax::isIdentifierContinueCharacter(CodePoint c) const BOOST_NOEXCEPT {
			if(boost::binary_search(addedIDNonStartCharacters_, c) || boost::binary_search(addedIDStartCharacters_, c))
				return true;
			else if(boost::binary_search(subtractedIDStartCharacters_, c) || boost::binary_search(subtractedIDNonStartCharacters_, c))
				return false;
			switch(type_) {
				case ASCII:
					return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
				case LEGACY_POSIX:
					return ucd::legacyctype::isword(c);
				case UNICODE_DEFAULT:
					return ucd::BinaryProperty::is<ucd::BinaryProperty::ID_CONTINUE>(c);
				case UNICODE_ALTERNATIVE:
					return !ucd::BinaryProperty::is<ucd::BinaryProperty::PATTERN_SYNTAX>(c)
						&& !ucd::BinaryProperty::is<ucd::BinaryProperty::PATTERN_WHITE_SPACE>(c);
			}
			ASCENSION_ASSERT_NOT_REACHED();
		}

		/**
		 * Returns @c true if the specified character is an ID_Start.
		 * @param c The code point of the character
		 * @return true if @a c is an identifier start character
		 */
		bool IdentifierSyntax::isIdentifierStartCharacter(CodePoint c) const BOOST_NOEXCEPT {
			if(boost::binary_search(addedIDStartCharacters_, c))
				return true;
			else if(boost::binary_search(subtractedIDStartCharacters_, c))
				return false;
			switch(type_) {
				case ASCII:
					return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
				case LEGACY_POSIX:
					return ucd::legacyctype::isalpha(c);
				case UNICODE_DEFAULT:
					return ucd::BinaryProperty::is<ucd::BinaryProperty::ID_START>(c);
				case UNICODE_ALTERNATIVE:
					return !ucd::BinaryProperty::is<ucd::BinaryProperty::PATTERN_SYNTAX>(c)
						&& !ucd::BinaryProperty::is<ucd::BinaryProperty::PATTERN_WHITE_SPACE>(c);
			}
			ASCENSION_ASSERT_NOT_REACHED();
		}

		/**
		 * Returns @c true if the specified character is a white space.
		 * @param c The code point of the character
		 * @param includeTab Set @c true to treat a horizontal tab as a white space
		 * @return true if @a c is a white space
		 */
		bool IdentifierSyntax::isWhiteSpace(CodePoint c, bool includeTab) const BOOST_NOEXCEPT {
			if(includeTab && c == 0x0009u)
				return true;
			switch(type_) {
				case ASCII:
					return c == 0x0020u;
				case LEGACY_POSIX:
					return ucd::legacyctype::isspace(c);
				case UNICODE_DEFAULT:
				case UNICODE_ALTERNATIVE:
					return ucd::BinaryProperty::is<ucd::BinaryProperty::PATTERN_WHITE_SPACE>(c);
			}
			ASCENSION_ASSERT_NOT_REACHED();
		}

		/**
		 * Overrides default identifier start character set.
		 * @param adding The set of characters to add to the default ID_Start characters
		 * @param subtracting The set of characters to subtract from the default ID_Start characters
		 * @throw InvalidScalarValueException An isolated surrogate is found, or same character was found at both @a adding and
		 *                                    @a subtracting
		 */
		void IdentifierSyntax::overrideIdentifierStartCharacters(const StringPiece& adding, const StringPiece& subtracting) {
			auto isolatedSurrogate(surrogates::searchIsolatedSurrogate(adding));
			if(isolatedSurrogate != boost::const_end(adding))
				throw InvalidScalarValueException(*isolatedSurrogate);
			isolatedSurrogate = surrogates::searchIsolatedSurrogate(subtracting);
			if(isolatedSurrogate != boost::const_end(subtracting))
				throw InvalidScalarValueException(*isolatedSurrogate);
			typedef utf::CharacterDecodeIterator<decltype(isolatedSurrogate)> I;
			implementOverrides(
				boost::make_iterator_range(I(std::begin(adding), std::end(adding)), I(std::begin(adding), std::end(adding), std::end(adding))),
				boost::make_iterator_range(I(std::begin(subtracting), std::end(subtracting)), I(std::begin(subtracting), std::end(subtracting), std::end(subtracting))),
				addedIDStartCharacters_, subtractedIDStartCharacters_);
		}

		/**
		 * Overrides default identifier start character set.
		 * @param adding The set of characters to add to the default ID_Start characters
		 * @param subtracting The set of characters to subtract from the default ID_Start characters
		 * @throw InvalidScalarValueException An isolated surrogate is found, or same character was found
		 *                                    at both @a adding and @a subtracting
		 */
		void IdentifierSyntax::overrideIdentifierStartCharacters(const std::set<CodePoint>& adding, const std::set<CodePoint>& subtracting) {
			std::set<CodePoint>::const_iterator isolatedSurrogate(boost::find_if(adding, surrogates::isSurrogate));
			if(isolatedSurrogate != boost::const_end(adding))
				throw InvalidScalarValueException(*isolatedSurrogate);
			isolatedSurrogate = boost::find_if(subtracting, surrogates::isSurrogate);
			if(isolatedSurrogate != boost::const_end(subtracting))
				throw InvalidScalarValueException(*isolatedSurrogate);
			implementOverrides(adding, subtracting, addedIDStartCharacters_, subtractedIDStartCharacters_);
		}

		/**
		 * Overrides standard identifier only continue character set.
		 * @param adding The set of characters to add to standard ID_Continue characters
		 * @param subtracting The set of characters to subtract to standard ID_Continue characters
		 * @throw InvalidScalarValueException An isolated surrogate is found, or same character was found at both
		 *                                    @a adding and @a subtracting
		 */
		void IdentifierSyntax::overrideIdentifierNonStartCharacters(const StringPiece& adding, const StringPiece& subtracting) {
			auto isolatedSurrogate(surrogates::searchIsolatedSurrogate(adding));
			if(isolatedSurrogate != boost::const_end(adding))
				throw InvalidScalarValueException(*isolatedSurrogate);
			isolatedSurrogate = surrogates::searchIsolatedSurrogate(subtracting);
			if(isolatedSurrogate != boost::const_end(subtracting))
				throw InvalidScalarValueException(*isolatedSurrogate);
			typedef utf::CharacterDecodeIterator<decltype(isolatedSurrogate)> I;
			implementOverrides(
				boost::make_iterator_range(I(std::begin(adding), std::end(adding)), I(std::begin(adding), std::end(adding), std::end(adding))),
				boost::make_iterator_range(I(std::begin(subtracting), std::end(subtracting)), I(std::begin(subtracting), std::end(subtracting), std::end(subtracting))),
				addedIDNonStartCharacters_, subtractedIDNonStartCharacters_);
		}

		/**
		 * Overrides standard identifier only continue character set.
		 * @param adding The set of characters to add to standard ID_Continue characters
		 * @param subtracting The set of characters to subtract to standard ID_Continue characters
		 * @throw InvalidScalarValueException An isolated surrogate is found, or same character was found at both
		 *                                    @a adding and @a subtracting
		 */
		void IdentifierSyntax::overrideIdentifierNonStartCharacters(const std::set<CodePoint>& adding, const std::set<CodePoint>& subtracting) {
			std::set<CodePoint>::const_iterator isolatedSurrogate(boost::find_if(adding, surrogates::isSurrogate));
			if(isolatedSurrogate != boost::const_end(adding))
				throw InvalidScalarValueException(*isolatedSurrogate);
			isolatedSurrogate = boost::find_if(subtracting, surrogates::isSurrogate);
			if(isolatedSurrogate != boost::const_end(subtracting))
				throw InvalidScalarValueException(*isolatedSurrogate);
			implementOverrides(adding, subtracting, addedIDNonStartCharacters_, subtractedIDNonStartCharacters_);
		}
	}
}
