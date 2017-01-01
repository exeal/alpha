/**
 * @file keyboard-modifier.hpp
 * Defines @c KeyboardModifiers class.
 * @author exeal
 * @date 2010-11-10 Created as user-input.hpp.
 * @date 2016-04-17 Separated from user-input.hpp.
 */

#ifndef ASCENSION_KEYBOARD_MODIFIER_HPP
#define ASCENSION_KEYBOARD_MODIFIER_HPP
#include <ascension/corelib/combination.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#	include <gdkmm/types.h>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#	error not implemented.
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#	include <QInputEvent>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#	include <ascension/win32/windows.hpp>
#endif
#include <boost/config.hpp>	// BOOST_NOEXCEPT
#include <bitset>

namespace ascension {
	namespace viewer {
		namespace widgetapi {
			namespace event {
				/// Modifier key values which indicate positions of the bit in @c KeyboardModifiers class.
				enum KeyboardModifier {
					SHIFT_DOWN,		///< The Shift key is down.
					CONTROL_DOWN,	///< The Ctrl (Control) key is down.
					ALT_DOWN,		///< The Alt key is down.
					META_DOWN,		///< The Meta key is down.
//					ALT_GRAPH_DOWN,	///< The AltGraph key is down.
//					COMMAND_DOWN,	///< The Command key is down. Only for Mac OS X.
					NUMBER_OF_KEYBOARD_MODIFIERS
				};

				/// Indicates the state of modifier keys.
				typedef Combination<KeyboardModifier, NUMBER_OF_KEYBOARD_MODIFIERS> KeyboardModifiers;

#if ASCENSION_SUPPORTS_WINDOW_SYSTEM(GTK)
				template<typename Model>
				inline Model fromNative(const Gdk::ModifierType& source, typename std::enable_if<std::is_same<Model, KeyboardModifiers>::value>::type* = nullptr) BOOST_NOEXCEPT {
					KeyboardModifiers value;
					value.set(SHIFT_DOWN, (source & Gdk::SHIFT_MASK) != 0);
					value.set(CONTROL_DOWN, (source & Gdk::CONTROL_MASK) != 0);
					value.set(ALT_DOWN, (source & Gdk::MOD1_MASK) != 0);
					value.set(META_DOWN, (source & Gdk::META_MASK) != 0);
				}

				inline Gdk::ModifierType toNative(const KeyboardModifiers& from, const Gdk::ModifierType* = nullptr) BOOST_NOEXCEPT {
					Gdk::ModifierType value = static_cast<Gdk::ModifierType>(0);
					value |= from.test(SHIFT_DOWN) ? Gdk::SHIFT_MASK : static_cast<Gdk::ModifierType>(0);
					value |= from.test(CONTROL_DOWN) ? Gdk::CONTROL_MASK : static_cast<Gdk::ModifierType>(0);
					value |= from.test(ALT_DOWN) ? Gdk::MOD1_MASK : static_cast<Gdk::ModifierType>(0);
					value |= from.test(META_DOWN) ? Gdk::META_MASK : static_cast<Gdk::ModifierType>(0);
					return value;
				}
#endif
#if ASCENSION_SUPPORTS_WINDOW_SYSTEM(QUARTZ)
#	error Not implemented.
#endif
#if ASCENSION_SUPPORTS_WINDOW_SYSTEM(QT)
				template<typename Model>
				inline Model fromNative(const Qt::KeyboardModifiers& source, typename std::enable_if<std::is_same<Model, KeyboardModifiers>::value>::type* = nullptr) BOOST_NOEXCEPT {
					KeyboardModifiers value;
					value.set(SHIFT_DOWN, source.testFlag(Qt::ShiftModifier));
					value.set(CONTROL_DOWN, source.testFlag(Qt::ControlModifier));
					value.set(ALT_DOWN, source.testFlag(Qt::AltModifier));
					value.set(META_DOWN, source.testFlag(Qt::MetaModifier));
				}

				inline Qt::KeyboardModifiers toNative(const KeyboardModifiers& from, const Qt::KeyboardModifiers* = nullptr) BOOST_NOEXCEPT {
					Qt::KeyboardModifiers value = Qt::NoModifier;
					value |= from.test(SHIFT_DOWN) ? Qt::ShiftModifier : Qt::NoModifier;
					value |= from.test(CONTROL_DOWN) ? Qt::ControlModifier : Qt::NoModifier;
					value |= from.test(ALT_DOWN) ? Qt::AltModifier : Qt::NoModifier;
					value |= from.test(META_DOWN) ? Qt::MetaModifier : Qt::NoModifier;
					return value;
				}
#endif
#if ASCENSION_SUPPORTS_WINDOW_SYSTEM(WIN32)
				template<typename Model>
				inline Model fromNative(WORD source, typename std::enable_if<std::is_same<Model, KeyboardModifiers>::value>::type* = nullptr) BOOST_NOEXCEPT {
					KeyboardModifiers value;
					value.set(SHIFT_DOWN, (source & MK_SHIFT) != 0);
					value.set(CONTROL_DOWN, (source & MK_CONTROL) != 0);
					value.set(META_DOWN, (source & MK_ALT) != 0);	// for D&D
					return value;
				}

				inline WORD toNative(const KeyboardModifiers& from, const WORD* = nullptr) BOOST_NOEXCEPT {
					WORD value = 0;
					value |= from.test(SHIFT_DOWN) ? MK_SHIFT : 0;
					value |= from.test(CONTROL_DOWN) ? MK_CONTROL : 0;
					value |= from.test(META_DOWN) ? MK_ALT : 0;	// for D&D
					return value;
				}
#endif
			}
		}
	}
}

#endif // !ASCENSION_KEYBOARD_MODIFIER_HPP
