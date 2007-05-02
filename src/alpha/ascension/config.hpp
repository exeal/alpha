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


// about ascension.text /////////////////////////////////////////////////////

/**
 * @def ASCENSION_DEFAULT_LINE_BREAK
 * Default line break. This must be one of @c ascension#text#LineBreak enumeration.
 */
#define ASCENSION_DEFAULT_LINE_BREAK ascension::text::LB_CRLF


// about ascension.unicode //////////////////////////////////////////////////

/**
 * @def ASCENSION_DEFAULT_CHARACTER_CLASSIFICATION
 * Default character set for @c ascension#unicode#IdentifierSyntax. This value is used by default
 * character classification (such as the partitioner which does not have scanners). This must be
 * one of @c ascension#unicode#CharacterDetector#CharacterClassification enumeration.
 */
#define ASCENSION_DEFAULT_CHARACTER_CLASSIFICATION ascension::unicode::IdentifierSyntax::UNICODE_DEFAULT

/**
 * @def ASCENSION_NO_UNICODE_NORMALIZATION
 * Define the symbol if you do not use Unicode normalization features. If defined,
 * @c ascension#unicode#Normalizer class and @c ascension#unicode#CanonicalCombiningClass class
 * will be not available.
 * @see ascension#unicode#Normalizer, ascension#unicode#CanonicalCombiningClass,
 * @see ASCENSION_NO_UNICODE_COMPATIBILITY_MAPPING
 */
// #define ASCENSION_NO_UNICODE_NORMALIZATION

/**
 * @def ASCENSION_NO_UNICODE_COMPATIBILITY_MAPPING
 * Define the symbol if you do not use Unicode compatibility mappings. If defined, normalization
 * features for compatibility mapping and compatibility equivalents will be not available.
 * @see ASCENSION_NO_UNICODE_NORMALIZATION, ascension#unicode#Normalizer
 */
#define ASCENSION_NO_UNICODE_COMPATIBILITY_MAPPING

/**
 * @def ASCENSION_NO_UNICODE_COLLATION
 * Define the symbol if you do not use Unicode collation facilities.
 * @see ascension#unicode#Collator, collator.hpp
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
 * @def ASCENSION_NO_EXTENDED_ENCODINGS
 * Define the symbol if you do not use extended encodings (UTF-8, UTF-16LE, UTF-16BE, and Windows
 * code pages are enabled even if defined).
 * @see ascension#encodings
 */
// #define ASCENSION_NO_EXTENDED_ENCODINGS


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


// about ascension.viewers //////////////////////////////////////////////////

/**
 * @def ASCENSION_DEFAULT_TEXT_ORIENTATION
 * Default text orientation for rendering. This must be one of @c ascension#viewers#Orientation
 * enumeration.
 */
#define ASCENSION_DEFAULT_TEXT_ORIENTATION ascension::viewers::LEFT_TO_RIGHT

/**
 * @def ASCENSION_DEFAULT_TEXT_ALIGNMENT
 * Default text alignment for rendering. This must be one of @c ascension#viewers#Alignment enumeration.
 */
#define ASCENSION_DEFAULT_TEXT_ALIGNMENT ascension::viewers::ALIGN_LEFT

/**
 * @def ASCENSION_LINE_LAYOUT_CACHE_SIZE
 * Default size of cache buffer of @c ascension#viewers#LineLayoutBuffer. This value is used by
 * @c ascension#viewers#LineLayoutBuffer class.
 */
#define ASCENSION_DEFAULT_LINE_LAYOUT_CACHE_SIZE 256

/**
 * @def ASCENSION_NO_DOUBLE_BUFFERING
 * Define the symbol if you do not use double-buffering for non-flicker drawing.
 */
// #define ASCENSION_NO_DOUBLE_BUFFERING

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
