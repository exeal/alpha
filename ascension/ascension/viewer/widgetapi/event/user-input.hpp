/**
 * @file user-input.hpp
 * Defines the classes represent user input events.
 * @author exeal
 * @date 2010-11-10 created
 */

#ifndef ASCENSION_USER_INPUT_HPP
#define ASCENSION_USER_INPUT_HPP

#include <ascension/viewer/widgetapi/event/event.hpp>
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
					 * Indicates the state of modifier keys.
					 * @note Corresponds to @c GdkModifierType in GDK.
					 * @note Cooresponds to @c Qt#Modifier and @c Qt#KeyboardModifier in Qt.
					 */
					typedef std::uint32_t KeyboardModifier;

					/// @var SHIFT_DOWN The Shift key is down.

					/// @var CONTROL_DOWN The Ctrl (Control) key is down.

					/// @var ALT_DOWN The Alt key is down.

					/// @var ALT_GRAPH_DOWN The AltGraph key is down.

					/// @var COMMAND_DOWN The Command key is down. Only for Mac OS X.

					static const KeyboardModifier
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
						SHIFT_DOWN = Gdk::SHIFT_MASK,
						CONTROL_DOWN = Gdk::CONTROL_MASK,
						ALT_DOWN = Gdk::MOD1_MASK,
						META_DOWN = Gdk::META_MASK
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
						SHIFT_DOWN = Qt::ShiftModifier,
						CONTROL_DOWN = Qt::ControlModifier,
						ALT_DOWN = Qt::AltModifier,
						META_DOWN = Qt::MetaModifier
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
						SHIFT_DOWN = MOD_SHIFT,
						CONTROL_DOWN = MOD_CONTROL,
						ALT_DOWN = MOD_ALT,
						META_DOWN = MOD_WIN
#endif
						;

				public:
					/**
					 * Returns @c true if the user input is the specified keyboard modifier down.
					 * @param modifier The modifiers to test
					 * @return @c true if @a input has @a mask
					 */
					bool hasModifier(KeyboardModifier mask) const BOOST_NOEXCEPT {
						return (modifiers() & mask) != 0;
					}	
					/**
					 * Returns @c true if the user input has other than the specified keyboard modifiers.
					 * @param mask The modifiers to test
					 * @return @c true if @a input has modifiers other than @a mask
					 */
					bool hasModifierOtherThan(KeyboardModifier mask) const BOOST_NOEXCEPT {
						return (modifiers() & ~mask) != 0;
					}
					/// Returns the keyboard modifier flags.
					KeyboardModifier modifiers() const BOOST_NOEXCEPT {
						return modifiers_;
					}
					/// Returns the time stamp.
					const std::time_t& timeStamp() const BOOST_NOEXCEPT {
						return timeStamp_;
					}

				protected:
					/**
					 * Protected constructor.
					 * @param modifiers The keyboard modifier flags
					 */
					explicit UserInput(KeyboardModifier modifiers) : modifiers_(modifiers) {
						std::time(&timeStamp_);
					}

				private:
					const KeyboardModifier modifiers_;
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
#	define GET_X_LPARAM(l) LOWORD(l)
#	define GET_Y_LPARAM(l) HIWORD(l)
#endif
			return boost::geometry::make<Point>(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
		}
		inline viewer::widgetapi::UserInput::ModifierKey makeModifiers() BOOST_NOEXCEPT {
			viewer::widgetapi::UserInput::ModifierKey modifiers = 0;
			if(::GetKeyState(VK_SHIFT) < 0)
				modifiers |= viewer::widgetapi::UserInput::SHIFT_DOWN;
			if(::GetKeyState(VK_CONTROL) < 0)
				modifiers |= viewer::widgetapi::UserInput::CONTROL_DOWN;
			if(::GetKeyState(VK_MENU) < 0)
				modifiers |= viewer::widgetapi::UserInput::ALT_DOWN;
			return modifiers;
		}
		inline viewer::widgetapi::UserInput::ModifierKey makeModifiers(WPARAM wp) BOOST_NOEXCEPT {
			viewer::widgetapi::UserInput::ModifierKey modifiers = 0;
			if((wp & MK_CONTROL) != 0)
				modifiers = viewer::widgetapi::UserInput::CONTROL_DOWN;
			if((wp & MK_SHIFT) != 0)
				modifiers = viewer::widgetapi::UserInput::SHIFT_DOWN;
			return modifiers;
		}
		inline viewer::widgetapi::KeyInput makeKeyInput(WPARAM wp, LPARAM lp) {
			return viewer::widgetapi::KeyInput(wp, makeModifiers(), LOWORD(lp), HIWORD(lp));
		}
		inline viewer::widgetapi::MouseButtonInput makeMouseButtonInput(viewer::widgetapi::UserInput::MouseButton button, WPARAM wp, LPARAM lp) {
			return viewer::widgetapi::MouseButtonInput(makeMouseLocation<graphics::Point>(lp), button, makeModifiers(wp));
		}
	}
#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
}

#endif // !ASCENSION_USER_INPUT_HPP
