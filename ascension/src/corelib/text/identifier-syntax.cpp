/**
 * @file identifier-syntax.cpp
 * @author exeal
 * @date 2007-2011
 */

#include <ascension/config.hpp>	// ASCENSION_DEFAULT_CHARACTER_CLASSIFICATION
#include <ascension/corelib/basic-exceptions.hpp>	// ASCENSION_ASSERT_NOT_REACHED
#include <ascension/corelib/text/identifier-syntax.hpp>
#include <ascension/corelib/text/character-property.hpp>
#include <ascension/corelib/text/utf-iterator.hpp>	// utf.CharacterDecodeIterator
#include <vector>
using namespace ascension;
using namespace ascension::text;
using namespace ascension::text::ucd;
using namespace std;


// IdentifierSyntax ///////////////////////////////////////////////////////////////////////////////

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
 * This class provides methods to add/remove characters to/from default code point categories for
 * identifier syntax. For example, the following code defines '_' as the identifier non-start character:
 *
 * @code
 * IdentifierSyntax is;
 * is.overrideIdentifierNonStartCharacters(L"_", L"");  // as if String is std.wstring...
 * @endcode
 *
 * @see kernel#ContentTypeInformationProvider#identifierSyntax
 */

namespace {
	template<typename InputIterator>
	void implementOverrides(InputIterator addingFirst, InputIterator addingLast,
			InputIterator subtractingFirst, InputIterator subtractingLast,
			basic_string<CodePoint>& added, basic_string<CodePoint>& subtracted) {
		basic_stringbuf<CodePoint> adding, subtracting;
		while(addingFirst != addingLast)
			adding.sputc(*addingFirst++);
		while(subtractingFirst != subtractingLast)
			subtracting.sputc(*subtractingFirst++);
		basic_string<CodePoint> a(adding.str()), s(subtracting.str());
		sort(a.begin(), a.end());
		sort(s.begin(), s.end());
		vector<CodePoint> v;
		set_intersection(a.begin(), a.end(), s.begin(), s.end(), back_inserter(v));
		if(!v.empty())
			throw invalid_argument("same character was found the both sets");
		added = a;
		subtracted = s;
	}
} // namespace @0

/**
 * Default constructor. The character classification type is initialized to
 * @c ASCENSION_DEFAULT_CHARACTER_DETECTION_TYPE.
 */
IdentifierSyntax::IdentifierSyntax() /*throw()*/ : type_(ASCENSION_DEFAULT_CHARACTER_CLASSIFICATION), caseSensitive_(true)
#ifndef ASCENSION_NO_UNICODE_NORMALIZATION
		, equivalenceType_(NO_DECOMPOSITION)
#endif // !ASCENSION_NO_UNICODE_NORMALIZATION
{
}

/// Copy-constructor.
IdentifierSyntax::IdentifierSyntax(const IdentifierSyntax& rhs) /*throw()*/ : type_(rhs.type_), caseSensitive_(rhs.caseSensitive_),
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
) /*throw()*/ : type_(type), caseSensitive_(ignoreCase)
#ifndef ASCENSION_NO_UNICODE_NORMALIZATION
		, equivalenceType_(equivalenceType)
#endif // !ASCENSION_NO_UNICODE_NORMALIZATION
{
}

/**
 * Returns the default @c IdentifierSyntax singleton instance. This instance has
 * @c UNICODE_DEFAULT character classification type.
 */
const IdentifierSyntax& IdentifierSyntax::defaultInstance() /*throw()*/ {
	static const IdentifierSyntax instance(UNICODE_DEFAULT);
	return instance;
}

/**
 * Returns @c true if the specified character is ID_Continue.
 * @param c The code point of the character
 * @return true if @a c is identifier continue character
 */
bool IdentifierSyntax::isIdentifierContinueCharacter(CodePoint c) const /*throw()*/ {
	if(binary_search(addedIDNonStartCharacters_.begin(), addedIDNonStartCharacters_.end(), c)
			|| binary_search(addedIDStartCharacters_.begin(), addedIDStartCharacters_.end(), c))
		return true;
	else if(binary_search(subtractedIDStartCharacters_.begin(), subtractedIDStartCharacters_.end(), c)
			|| binary_search(subtractedIDNonStartCharacters_.begin(), subtractedIDNonStartCharacters_.end(), c))
		return false;
	switch(type_) {
	case ASCII:
		return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
	case LEGACY_POSIX:
		return legacyctype::isword(c);
	case UNICODE_DEFAULT:
		return BinaryProperty::is<BinaryProperty::ID_CONTINUE>(c);
	case UNICODE_ALTERNATIVE:
		return !BinaryProperty::is<BinaryProperty::PATTERN_SYNTAX>(c)
			&& !BinaryProperty::is<BinaryProperty::PATTERN_WHITE_SPACE>(c);
	}
	ASCENSION_ASSERT_NOT_REACHED();
}

/**
 * Returns @c true if the specified character is an ID_Start.
 * @param c The code point of the character
 * @return true if @a c is an identifier start character
 */
bool IdentifierSyntax::isIdentifierStartCharacter(CodePoint c) const /*throw()*/ {
	if(binary_search(addedIDStartCharacters_.begin(), addedIDStartCharacters_.end(), c))
		return true;
	else if(binary_search(subtractedIDStartCharacters_.begin(), subtractedIDStartCharacters_.end(), c))
		return false;
	switch(type_) {
	case ASCII:
		return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
	case LEGACY_POSIX:
		return legacyctype::isalpha(c);
	case UNICODE_DEFAULT:
		return BinaryProperty::is<BinaryProperty::ID_START>(c);
	case UNICODE_ALTERNATIVE:
		return !BinaryProperty::is<BinaryProperty::PATTERN_SYNTAX>(c)
			&& !BinaryProperty::is<BinaryProperty::PATTERN_WHITE_SPACE>(c);
	}
	ASCENSION_ASSERT_NOT_REACHED();
}

/**
 * Returns @c true if the specified character is a white space.
 * @param c The code point of the character
 * @param includeTab Set @c true to treat a horizontal tab as a white space
 * @return true if @a c is a white space
 */
bool IdentifierSyntax::isWhiteSpace(CodePoint c, bool includeTab) const /*throw()*/ {
	if(includeTab && c == 0x0009u)
		return true;
	switch(type_) {
	case ASCII:
		return c == 0x0020u;
	case LEGACY_POSIX:
		return legacyctype::isspace(c);
	case UNICODE_DEFAULT:
	case UNICODE_ALTERNATIVE:
		return BinaryProperty::is<BinaryProperty::PATTERN_WHITE_SPACE>(c);
	}
	ASCENSION_ASSERT_NOT_REACHED();
}

/**
 * Overrides default identifier start character set.
 * @param adding The set of characters to add to the default ID_Start characters
 * @param subtracting The set of characters to subtract from the default ID_Start characters
 * @throw InvalidScalarValueException An isolated surrogate is found, or same character was found
 *                                    at both @a adding and @a subtracting
 */
void IdentifierSyntax::overrideIdentifierStartCharacters(const String& adding, const String& subtracting) {
	String::const_iterator isolatedSurrogate(surrogates::searchIsolatedSurrogate(adding.begin(), adding.end()));
	if(isolatedSurrogate != adding.end())
		throw InvalidScalarValueException(*isolatedSurrogate);
	isolatedSurrogate = surrogates::searchIsolatedSurrogate(subtracting.begin(), subtracting.end());
	if(isolatedSurrogate != subtracting.end())
		throw InvalidScalarValueException(*isolatedSurrogate);
	typedef utf::CharacterDecodeIterator<String::const_iterator> I;
	implementOverrides(I(adding.begin(), adding.end()), I(adding.begin(), adding.end(), adding.end()),
		I(subtracting.begin(), subtracting.end()), I(subtracting.begin(), subtracting.end(), subtracting.end()),
		addedIDStartCharacters_, subtractedIDStartCharacters_);
}

/**
 * Overrides default identifier start character set.
 * @param adding The set of characters to add to the default ID_Start characters
 * @param subtracting The set of characters to subtract from the default ID_Start characters
 * @throw InvalidScalarValueException An isolated surrogate is found, or same character was found
 *                                    at both @a adding and @a subtracting
 */
void IdentifierSyntax::overrideIdentifierStartCharacters(const set<CodePoint>& adding, const set<CodePoint>& subtracting) {
	set<CodePoint>::const_iterator isolatedSurrogate(find_if(adding.begin(), adding.end(), ptr_fun(surrogates::isSurrogate)));
	if(isolatedSurrogate != adding.end())
		throw InvalidScalarValueException(*isolatedSurrogate);
	isolatedSurrogate = find_if(subtracting.begin(), subtracting.end(), ptr_fun(surrogates::isSurrogate));
	if(isolatedSurrogate != subtracting.end())
		throw InvalidScalarValueException(*isolatedSurrogate);
	implementOverrides(adding.begin(), adding.end(),
		subtracting.begin(), subtracting.end(), addedIDStartCharacters_, subtractedIDStartCharacters_);
}

/**
 * Overrides standard identifier only continue character set.
 * @param adding The set of characters to add to standard ID_Continue characters
 * @param subtracting The set of characters to subtract to standard ID_Continue characters
 * @throw InvalidScalarValueException An isolated surrogate is found, or same character was found
 *                                    at both @a adding and @a subtracting
 */
void IdentifierSyntax::overrideIdentifierNonStartCharacters(const String& adding, const String& subtracting) {
	String::const_iterator isolatedSurrogate(surrogates::searchIsolatedSurrogate(adding.begin(), adding.end()));
	if(isolatedSurrogate != adding.end())
		throw InvalidScalarValueException(*isolatedSurrogate);
	isolatedSurrogate = surrogates::searchIsolatedSurrogate(subtracting.begin(), subtracting.end());
	if(isolatedSurrogate != subtracting.end())
		throw InvalidScalarValueException(*isolatedSurrogate);
	typedef utf::CharacterDecodeIterator<String::const_iterator> I;
	implementOverrides(I(adding.begin(), adding.end()), I(adding.begin(), adding.end(), adding.end()),
		I(subtracting.begin(), subtracting.end()), I(subtracting.begin(), subtracting.end(), subtracting.end()),
		addedIDNonStartCharacters_, subtractedIDNonStartCharacters_);
}

/**
 * Overrides standard identifier only continue character set.
 * @param adding The set of characters to add to standard ID_Continue characters
 * @param subtracting The set of characters to subtract to standard ID_Continue characters
 * @throw InvalidScalarValueException An isolated surrogate is found, or same character was found
 *                                    at both @a adding and @a subtracting
 */
void IdentifierSyntax::overrideIdentifierNonStartCharacters(const set<CodePoint>& adding, const set<CodePoint>& subtracting) {
	set<CodePoint>::const_iterator isolatedSurrogate(find_if(adding.begin(), adding.end(), ptr_fun(surrogates::isSurrogate)));
	if(isolatedSurrogate != adding.end())
		throw InvalidScalarValueException(*isolatedSurrogate);
	isolatedSurrogate = find_if(subtracting.begin(), subtracting.end(), ptr_fun(surrogates::isSurrogate));
	if(isolatedSurrogate != subtracting.end())
		throw InvalidScalarValueException(*isolatedSurrogate);
	implementOverrides(adding.begin(), adding.end(),
		subtracting.begin(), subtracting.end(), addedIDNonStartCharacters_, subtractedIDNonStartCharacters_);
}
