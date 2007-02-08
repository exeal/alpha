/**
 *	@file config.hpp
 *	@brief Configure Ascension build settings using this file.
 *	@author exeal
 *	@date 2005-2007
 */

#ifndef ASCENSION_CONFIG_HPP
#define ASCENSION_CONFIG_HPP

/// Default line break
#define ASCENSION_DEFAULT_LINE_BREAK ascension::text::LB_CRLF

/// Default character set for @c CharacterDetector.
/// This value is used by default character detection (such as the partitioner which does not have scanners).
#define ASCENSION_DEFAULT_CHARACTER_DETECTION_TYPE ascension::unicode::CharacterDetector::UCD

/// Define the symbol if you do not use extended encodings (UTF-8, UTF-16LE, UTF-16BE, and Windows code pages are enabled even if defined).
// #define ASCENSION_NO_EXTENDED_ENCODINGS

/// Default text orientation
#define ASCENSION_DEFAULT_TEXT_ORIENTATION ascension::viewers::LEFT_TO_RIGHT

/// Default text alignment
#define ASCENSION_DEFAULT_TEXT_ALIGNMENT ascension::viewers::ALIGN_LEFT

/// Length of array @c TextRenderer#layouts_
#define ASCENSION_TEXT_RENDERER_CACHE_LINES 128

/// Length of array @c VisualPoint#layouts_
#define ASCENSION_VISUAL_POINT_CACHE_LINES 8

/// Length of array @c TextRenderer#sparseLayoutsCache_ (Currently, this value is not used)
#define ASCENSION_TEXT_RENDERER_SPARSE_CACHE_LINES 32

/// Define the symbol if you do not use Unicode character foldings (may reduce code size).
//#define ASCENSION_NO_UNICODE_FOLDING

/// Define the symbol if you do not use regular expressions.
// #define ASCENSION_NO_REGEX

/// Define the symbol if you do not use C/Migemo.
// #define ASCENSION_NO_MIGEMO

/// Define the symbol if you do not use Microsoft Active Input Method Manager (Global IME).
/// You can also disable AIM at runtime.
/// @see Viewer#enableActiveInputMethod, Viewer#isActiveInputMethodEnabled
// #define ASCENSION_NO_ACTIVE_INPUT_METHOD_MANAGER

/// Define the symbol if you do not use Text Services Framework (However, TSF is not supported now).
#define ASCENSION_NO_TEXT_SERVICES_FRAMEWORK

/// Define the symbol if you do not use Microsoft Active Accessibility 2.0.
// #define ASCENSION_NO_ACTIVE_ACCESSIBILITY

/// Define the symbol if you do not use Microsoft Text Object Model (However, TOM is not supported now).
#define ASCENSION_NO_TEXT_OBJECT_MODEL

/// Define the symbol if you do not use double-buffering for drawing.
// #define ASCENSION_NO_DOUBLE_BUFFERING

/// Clipboard format for rectangle text
#define ASCENSION_RECTANGLE_TEXT_CLIP_FORMAT L"MSDEVColumnSelect"

#ifdef ASCENSION_NO_REGEX
#ifndef ASCENSION_NO_MIGEMO
#error "conflicted configuration: Migemo support requires the regular expression engine"
#endif /* !ASCENSION_NO_MIGEMO */
#endif /* ASCENSION_NO_REGEX */

#endif /* !ASCENSION_CONFIG_HPP */
