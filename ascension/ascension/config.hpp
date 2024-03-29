/**
 * @file config.hpp
 * @brief Configure Ascension build settings using this file.
 * @author exeal
 * @date 2005-2013
 */

#ifndef ASCENSION_CONFIG_HPP
#define ASCENSION_CONFIG_HPP

/**
 * @def ASCENSION_CUSTOM_SHARED_PTR_HPP
 * User-provided header file path.
 * Ascension uses @c std#tr1#shared_ptr class and includes the header file provided by compiler or
 * Boost.SharedPtr. If you don't have the neither, you can specify your own shared_ptr.hpp file.
 */
// #define ASCENSION_CUSTOM_SHARED_PTR_HPP <your-file-name>


// about ascension.kernel ///////////////////////////////////////////////////

/**
 * @def ASCENSION_DEFAULT_NEWLINE
 * Default newline. This must be one of @c ascension#kernel#Newline enumerations except 
 * @c ascension#kernel#Newline#USE_INTRINSIC_VALUE and
*  @c ascension#kernel#Newline#USE_DOCUMENT_INPUT.
 */
#ifndef ASCENSION_DEFAULT_NEWLINE
#	ifdef _WIN32
#		define ASCENSION_DEFAULT_NEWLINE ascension::text::Newline::CARRIAGE_RETURN_FOLLOWED_BY_LINE_FEED
#	else
#		define ASCENSION_DEFAULT_NEWLINE ascension::text::Newline::LINE_FEED
#	endif
#endif	// !ASCENSION_DEFAULT_NEWLINE


// about ascension.text //////////////////////////////////////////////////

/**
 * @def ASCENSION_NO_UNICODE_NORMALIZATION
 * Define the symbol if you do not use Unicode normalization features. If defined,
 * @c ascension#text#Normalizer class and @c ascension#text#CanonicalCombiningClass class
 * will be not available.
 * @see ascension#text#Normalizer, ascension#text#CanonicalCombiningClass,
 * @see ASCENSION_NO_UNICODE_COMPATIBILITY_MAPPING
 */
// #define ASCENSION_NO_UNICODE_NORMALIZATION

/**
 * @def ASCENSION_NO_UNICODE_COMPATIBILITY_MAPPING
 * Define the symbol if you do not use Unicode compatibility mappings. If defined, normalization
 * features for compatibility mapping and compatibility equivalents will be not available.
 * @see ASCENSION_NO_UNICODE_NORMALIZATION, ascension#text#Normalizer
 */
#define ASCENSION_NO_UNICODE_COMPATIBILITY_MAPPING

/**
 * @def ASCENSION_NO_UNICODE_COLLATION
 * Define the symbol if you do not use Unicode collation facilities.
 * @see ascension#text#Collator, collator.hpp
 */
#define ASCENSION_NO_UNICODE_COLLATION

/**
 * @def ASCENSION_NO_UNICODE_FOLDING
 * Define the symbol if you do not use Unicode character foldings (may reduce code size).
 * @note This is not supported currently.
 */
//#define ASCENSION_NO_UNICODE_FOLDING


// about ascension.encodings ////////////////////////////////////////////////

/**
 * @def ASCENSION_NO_STANDARD_ENCODINGS
 * Define the symbol if you do not use standard encodings.
 * @see ascension#encodings#standard
 */
// #define ASCENSION_NO_STANDARD_ENCODINGS

/**
 * @def ASCENSION_NO_PROPRIETARY_ENCODINGS
 * Define the symbol if you do not use proprietary encodings.
 * @see ascension#encodings#proprietary
 */
// #define ASCENSION_NO_PROPRIETARY_ENCODINGS

/**
 * @def ASCENSION_NO_MINORITY_ENCODINGS
 * Define the symbol if you do not use some minority encodings.
 * @see ascension#encodings#minority
 */
// #define ASCENSION_NO_MINORITY_ENCODINGS


// about ascension.regex ////////////////////////////////////////////////////

/**
 * @def ASCENSION_NO_REGEX
 * Define the symbol if you do not use regular expressions.
 * @see ascension#regex, ASCENSION_NO_MIGEMO
 */
// #define ASCENSION_NO_REGEX

/**
 * @def ASCENSION_NO_MIGEMO
 * Define the symbol if you do not use C/Migemo.
 * @see ascension#regex#MigemoPattern, ASCENSION_NO_REGEX
 */
// #define ASCENSION_NO_MIGEMO


// about ascension.layout ///////////////////////////////////////////////////

/**
 * @def ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE
 * Default size of cache buffer of @c ascension#layout#LineLayoutBuffer. This value is used by
 * @c ascension#layout#LineLayoutBuffer class.
 */
#ifndef ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE
#	define ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE 256
#endif	// !ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE

/**
 * @def ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
 * Define the symbol to enable Unicode Variation Selectors Supplement (U+E0100..E01EF) workaround.
 * For more details, see the description of layout.cpp.
 */
#ifndef ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
#	define ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
#endif // !ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND


// about ascension.presentation /////////////////////////////////////////////

/**
 * @def ASCENSION_DEFAULT_TEXT_READING_DIRECTION
 * Default text reading direction for rendering. This must be either @c LEFT_TO_RIGHT or
 * @c RIGHT_TO_LEFT of @c ascension#presentation#ReadingDirection enumeration.
 */
#ifndef ASCENSION_DEFAULT_TEXT_READING_DIRECTION
#	define ASCENSION_DEFAULT_TEXT_READING_DIRECTION ascension::presentation::LEFT_TO_RIGHT
#endif	// !ASCENSION_DEFAULT_TEXT_READING_DIRECTION

/**
 * @def ASCENSION_DEFAULT_TEXT_ANCHOR
 * Default text anchor for rendering. This must be either @c TEXT_ACHOR_START or @c TEXT_ANCHOR_END
 * of @c ascension#presentation#TextAnchor enumeration.
 */
#ifndef ASCENSION_DEFAULT_TEXT_ANCHOR
#	define ASCENSION_DEFAULT_TEXT_ANCHOR ascension::presentation::TEXT_ANCHOR_START
#endif	// !ASCENSION_DEFAULT_TEXT_ANCHOR

/**
 * @def ASCENSION_HYPERLINKS_CACHE_SIZE
 * Size of cache buffer of @c ascension#presentation#Presentation#hyperlinks.
 */
#ifndef ASCENSION_HYPERLINKS_CACHE_SIZE
#	define ASCENSION_HYPERLINKS_CACHE_SIZE 256
#endif	// !ASCENSION_HYPERLINKS_CACHE_SIZE


// about ascension.texteditor ///////////////////////////////////////////////

/**
 * @def ASCENSION_DEFAULT_MAXIMUM_KILLS
 * The default value of the parameter of @c ascension#texteditor#KillRing constructor.
 */
#ifndef ASCENSION_DEFAULT_MAXIMUM_KILLS
#	define ASCENSION_DEFAULT_MAXIMUM_KILLS 30
#endif	// !ASCENSION_DEFAULT_MAXIMUM_KILLS


// about ascension.viewer ///////////////////////////////////////////////////

/**
 * @def ASCENSION_HANDLE_STANDARD_EDIT_CONTROL_MESSAGES
 * Define the symbol if the text editor window should handle window messages related to the
 * standard EDIT control. These messages include: @c WM_CLEAR, @c WM_CUT
 */
#define ASCENSION_HANDLE_STANDARD_EDIT_CONTROL_MESSAGES

/**
 * @def ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER
 * Define the symbol if you do not use Microsoft Active Input Method Manager (Global IME). You can
 * also disable AIM at runtime. This effects only on Win32 platform.
 * @see ascension#viewer#TextViewer#enableActiveInputMethod,
 * ascension#viewer#TextViewer#isActiveInputMethodEnabled
 * @deprecated 0.8
 */
// #define ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER

/**
 * @def ASCENSION_NO_TEXT_SERVICES_FRAMEWORK
 * Define the symbol if you do not use Text Services Framework. This effects only on Win32 platform.
 * @note This is not supported currently.
 */
#define ASCENSION_NO_TEXT_SERVICES_FRAMEWORK

/**
 * @def ASCENSION_NO_ACTIVE_ACCESSIBILITY
 * Define the symbol if you do not use Microsoft Active Accessibility 2.0. This effects only on
 * Win32 platform.
 * @note To define this symbol is strongly discouraged.
 */
// #define ASCENSION_NO_ACTIVE_ACCESSIBILITY

/**
 * @def ASCENSION_NO_TEXT_OBJECT_MODEL
 * Define the symbol if you do not use Microsoft Text Object Model. This effects only on Win32
 * platform.
 * @note This is not supported currently.
 */
#define ASCENSION_NO_TEXT_OBJECT_MODEL

/**
 * @def ASCENSION_RECTANGLE_TEXT_MIME_FORMAT
 * MIME data format for rectangle text.
 * @see ascension#viewer#utils#rectangleTextMimeDataFormat
 */
//#define ASCENSION_RECTANGLE_TEXT_MIME_FORMAT "your-own-string"

#ifdef ASCENSION_NO_REGEX
#	ifndef ASCENSION_NO_MIGEMO
#		error "conflicted configuration: Migemo support requires the regular expression engine"
#	endif // !ASCENSION_NO_MIGEMO
#endif // ASCENSION_NO_REGEX

#endif // !ASCENSION_CONFIG_HPP
