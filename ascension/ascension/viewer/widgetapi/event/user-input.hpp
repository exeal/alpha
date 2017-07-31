/**
 * @file user-input.hpp
 * Defines the classes represent user input events.
 * @author exeal
 * @date 2010-11-10 created
 */

#ifndef ASCENSION_USER_INPUT_HPP
#define ASCENSION_USER_INPUT_HPP
#include <ascension/viewer/widgetapi/event/event.hpp>
#include <ascension/viewer/widgetapi/event/keyboard-modifier.hpp>
#include <ctime>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#	include <gdkmm/types.h>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#	error not implemented.
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#	include <QInputEvent>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#	include <ascension/graphics/geometry/point.hpp>
#	include <ascension/win32/windows.hpp>
#endif

namespace ascension {
	namespace viewer {
		namespace widgetapi {
			namespace event {
				/// Abstract class represents a user input.
				class UserInput : public Event {
				public:
					/**
					 * Returns @c true if the user input has all of the specified keyboard modifiers.
					 * @tparam Mask @c KeyboardModifiers or the fixed-size sequence of @c KeyboardModifier
					 * @param mask The keyboard modifiers to test
					 * @return @c true if @a input has @a mask
					 */
					template<typename Mask>
					bool hasAllOfModifiers(const Mask& mask) const {
						const KeyboardModifiers temp(mask);
						return (modifiers() & temp) == temp;
					}
					/**
					 * Returns @c true if the user input has any of the specified keyboard modifiers.
					 * @tparam Mask @c KeyboardModifiers or the fixed-size sequence of @c KeyboardModifier
					 * @param mask The keyboard modifiers to test
					 * @return @c true if the input has @a mask
					 */
					template<typename Mask>
					bool hasAnyOfModifiers(const Mask& mask) const {
						const KeyboardModifiers temp(mask);
						return (modifiers() & temp).any();
					}
					/**
					 * Returns @c true if the user input has the specified keyboard modifier.
					 * @param modifier The modifier to test
					 * @return @c true if the input has @a modifiers
					 * @throw std::out_of_range @a modifier is invalid
					 */
					bool hasModifier(KeyboardModifier modifier) const {
						return modifiers().test(static_cast<std::size_t>(modifier));
					}
					/**
					 * Returns @c true if the user input has other than the specified keyboard modifier.
					 * @param modifier The modifier to test
					 * @return @c true if the input has modifiers other than @a modifier
					 */
					bool hasModifierOtherThan(KeyboardModifier modifier) const BOOST_NOEXCEPT {
						auto temp(modifiers());
						return temp.reset(modifier).any();
					}
					/**
					 * Returns @c true if the user input has other than the specified keyboard modifiers.
					 * @tparam Mask @c KeyboardModifiers or the fixed-size sequence of @c KeyboardModifier
					 * @param mask The keyboard modifiers to test
					 * @return @c true if the input has modifiers other than @a mask
					 */
					template<typename Mask>
					bool hasModifierOtherThan(const Mask& mask) const BOOST_NOEXCEPT {
						const KeyboardModifiers temp(mask);
						return (modifiers() & ~temp).any();
					}
					/// Returns the keyboard modifier flags.
					KeyboardModifiers modifiers() const BOOST_NOEXCEPT {
						return modifiers_;
					}
					/// Returns the time stamp.
					const std::time_t& timeStamp() const BOOST_NOEXCEPT {
						return timeStamp_;
					}

				protected:
					/**
					 * Creates a @c UserInput object with the specified keyboard modifiers and the current time.
					 * @param modifiers The keyboard modifier flags
					 */
					explicit UserInput(const KeyboardModifiers& modifiers = KeyboardModifiers()) : modifiers_(modifiers) {
						std::time(&timeStamp_);
					}

				private:
					const KeyboardModifiers modifiers_;
					std::time_t timeStamp_;
				};
			}
		}
	}

#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
	namespace win32 {
		template<typename Point>
		inline Point makeMouseLocation(LPARAM lp) {
#ifndef GET_X_LPARAM
	// <windowsx.h> defines the followings
#	define GET_X_LPARAM(l) static_cast<int>(static_cast<short>(LOWORD(l)))
#	define GET_Y_LPARAM(l) static_cast<int>(static_cast<short>(HIWORD(l)))
#endif
			return boost::geometry::make<Point>(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
		}
	}
#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
}

#endif // !ASCENSION_USER_INPUT_HPP
