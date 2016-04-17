/**
 * @file keyboard-modifier.hpp
 * Defines @c KeyboardModifiers class.
 * @author exeal
 * @date 2010-11-10 Created as user-input.hpp.
 * @date 2016-04-17 Separated from user-input.hpp.
 */

#ifndef ASCENSION_KEYBOARD_MODIFIER_HPP
#define ASCENSION_KEYBOARD_MODIFIER_HPP
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
				class KeyboardModifiers : public std::bitset<NUMBER_OF_KEYBOARD_MODIFIERS> {
				public:
					typedef
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
						Gdk::ModifierType
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
						// TODO: Specify native type.
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
						Qt::KeyboardModifiers
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
						WORD
#endif
						Native;	///< Native type.

					/// Default constructor creates an empty @c KeyboardModifiers object.
					KeyboardModifiers() BOOST_NOEXCEPT {
					}
					/**
					 * Creates a @c KeyboardModifiers which has only the specified singular value.
					 * @param modifier The modifier bit
					 * @throw std#out_of_range @a modifier is invalid
					 */
					KeyboardModifiers(KeyboardModifier modifier) {
						set(modifier);
					}
					/**
					 * Creates a @c KeyboardModifiers object with the specified @c KeyboardModifier bits.
					 * @tparam Sequence The fixed-size sequence of @c KeyboardModifier
					 * @param sequence The sequence of @c KeyboardModifier bits
					 * @throw std#out_of_range @a sequence has invalid value
					 */
					template<typename Sequence>
					explicit KeyboardModifiers(const Sequence& sequence) {
						_set(sequence, std::integral_constant<std::size_t, std::tuple_size<Sequence>::value - 1>());
					}
					/**
					 * Creates a @c KeyboardModifiers object from the specified platform-native value.
					 * @param source The native value
					 */
					static KeyboardModifiers fromNative(const Native& source) BOOST_NOEXCEPT;
					/// Returns the platform-native value.
					Native native() const BOOST_NOEXCEPT;

				private:
					template<typename Sequence, std::size_t N>
					void _set(const Sequence& sequence, std::integral_constant<std::size_t, N>, typename std::enable_if<N != 0>::type* = nullptr) {
						set(std::get<N>(sequence));
						return _set(sequence, std::integral_constant<std::size_t, N - 1>());
					}
					template<typename Sequence>
					void _set(const Sequence& sequence, std::integral_constant<std::size_t, 0>) {
						set(std::get<0>(sequence));
					}
				};

				inline KeyboardModifiers KeyboardModifiers::fromNative(const Native& source) BOOST_NOEXCEPT {
					KeyboardModifiers value;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
					value.set(SHIFT_DOWN, (source & Gdk::SHIFT_MASK) != 0);
					value.set(CONTROL_DOWN, (source & Gdk::CONTROL_MASK) != 0);
					value.set(ALT_DOWN, (source & Gdk::MOD1_MASK) != 0);
					value.set(META_DOWN, (source & Gdk::META_MASK) != 0);
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#	error Not implemented.
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
					value.set(SHIFT_DOWN, source.testFlag(Qt::ShiftModifier));
					value.set(CONTROL_DOWN, source.testFlag(Qt::ControlModifier));
					value.set(ALT_DOWN, source.testFlag(Qt::AltModifier));
					value.set(META_DOWN, source.testFlag(Qt::MetaModifier));
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
					value.set(SHIFT_DOWN, (source & MOD_SHIFT) != 0);
					value.set(CONTROL_DOWN, (source & MOD_CONTROL) != 0);
					value.set(ALT_DOWN, (source & MOD_ALT) != 0);
					value.set(META_DOWN, (source & MOD_WIN) != 0);
#endif
					return value;
				}

				inline KeyboardModifiers::Native KeyboardModifiers::native() const BOOST_NOEXCEPT {
					Native value = static_cast<Native>(0);
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
					value |= test(SHIFT_DOWN) ? Gdk::SHIFT_MASK : Native(0);
					value |= test(CONTROL_DOWN) ? Gdk::CONTROL_MASK : Native(0);
					value |= test(ALT_DOWN) ? Gdk::MOD1_MASK : Native(0);
					value |= test(META_DOWN) ? Gdk::META_MASK : Native(0);
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#	error Not implemented.
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
					value |= test(SHIFT_DOWN) ? Qt::ShiftModifier : Qt::NoModifier;
					value |= test(CONTROL_DOWN) ? Qt::ControlModifier : Qt::NoModifier;
					value |= test(ALT_DOWN) ? Qt::AltModifier : Qt::NoModifier;
					value |= test(META_DOWN) ? Qt::MetaModifier : Qt::NoModifier;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
					value |= test(SHIFT_DOWN) ? MOD_SHIFT : 0;
					value |= test(CONTROL_DOWN) ? MOD_CONTROL : 0;
					value |= test(ALT_DOWN) ? MOD_ALT : 0;
					value |= test(META_DOWN) ? MOD_WIN : 0;
#endif
					return value;
				}
			}
		}
	}
}

#endif // !ASCENSION_KEYBOARD_MODIFIER_HPP
