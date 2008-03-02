/**
 *	@file config.hpp
 *	@brief Configure Ascension build settings using this file.
 *	@author exeal
 *	@date 2005-2007
 */

#ifndef ASCENSION_CONFIG_HPP
#define ASCENSION_CONFIG_HPP


/**
 * @def ASCENSION_SHARED_POINTER
 * Reference-counted smart pointer template class. Default is @c manah#SharedPointer. You can use
 * other class such as @c boost#shared_ptr.
 */
#define ASCENSION_SHARED_POINTER manah::SharedPointer
//#define ASCENSION_SHARED_POINTER boost::shared_ptr


// about ascension.kernel ///////////////////////////////////////////////////

/**
 * @def ASCENSION_DEFAULT_NEWLINE
 * Default newline. This must be one of @c ascension#kernel#Newline enumerations except 
 * @c ascension#kernel#NLF_RAW_VALUE and @c ascension#kernel#NLF_DOCUMENT_INPUT.
 */
#ifdef _WIN32
#	define ASCENSION_DEFAULT_NEWLINE ascension::kernel::NLF_CR_LF
#else
#	define ASCENSION_DEFAULT_NEWLINE ascension::kernel::NLF_LINE_FEED
#endif


// about ascension.kernel.files /////////////////////////////////////////////

#ifdef _WIN32
#	define ASCENSION_FILE_NAME_CHARACTER_TYPE ::WCHAR
#else
#	define ASCENSION_FILE_NAME_CHARACTER_TYPE char
#endif


// about ascension.text //////////////////////////////////////////////////

/**
 * @def ASCENSION_DEFAULT_CHARACTER_CLASSIFICATION
 * Default character set for @c ascension#text#IdentifierSyntax. This value is used by default
 * character classification (such as the partitioner which does not have scanners). This must be
 * one of @c ascension#text#CharacterDetector#CharacterClassification enumeration.
 */
#define ASCENSION_DEFAULT_CHARACTER_CLASSIFICATION ascension::text::IdentifierSyntax::UNICODE_DEFAULT

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
 * @def ASCENSION_DEFAULT_TEXT_ORIENTATION
 * Default text orientation for rendering. This must be one of @c ascension#layout#Orientation
 * enumeration.
 */
#define ASCENSION_DEFAULT_TEXT_ORIENTATION ascension::layout::LEFT_TO_RIGHT

/**
 * @def ASCENSION_DEFAULT_TEXT_ALIGNMENT
 * Default text alignment for rendering. This must be one of @c ascension#layout#Alignment enumeration.
 */
#define ASCENSION_DEFAULT_TEXT_ALIGNMENT ascension::layout::ALIGN_LEFT

/**
 * @def ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE
 * Default size of cache buffer of @c ascension#layout#LineLayoutBuffer. This value is used by
 * @c ascension#layout#LineLayoutBuffer class.
 */
#define ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE 256

/**
 * @def ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND
 * Define the symbol to enable Unicode Variation Selectors Supplement (U+E0100..E01EF) workaround.
 * For more details, see the description of layout.cpp.
 */
#define ASCENSION_VARIATION_SELECTORS_SUPPLEMENT_WORKAROUND


// about ascension.presentation /////////////////////////////////////////////

/**
 * @def ASCENSION_HYPERLINKS_CACHE_SIZE
 * Size of cache buffer of @c ascension#presentation#Presentation#hyperlinks.
 */
#define ASCENSION_HYPERLINKS_CACHE_SIZE 256


// about ascension.viewers //////////////////////////////////////////////////

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
 * @see ascension#viewers#TextViewer#enableActiveInputMethod,
 * ascension#viewers#TextViewer#isActiveInputMethodEnabled
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
 * @def ASCENSION_RECTANGLE_TEXT_CLIP_FORMAT
 * Clipboard format for rectangle text. This effects only on Win32 platform.
 */
#define ASCENSION_RECTANGLE_TEXT_CLIP_FORMAT L"MSDEVColumnSelect"

#ifdef ASCENSION_NO_REGEX
#ifndef ASCENSION_NO_MIGEMO
#error "conflicted configuration: Migemo support requires the regular expression engine"
#endif /* !ASCENSION_NO_MIGEMO */
#endif /* ASCENSION_NO_REGEX */

#endif /* !ASCENSION_CONFIG_HPP */
