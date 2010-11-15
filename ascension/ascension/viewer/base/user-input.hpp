/**
 * @file user-input.hpp
 * Defines the classes represent user input events.
 * @author exeal
 * @date 2010-11-10 created
 */

#ifndef ASCENSION_USER_INPUT_HPP
#define ASCENSION_USER_INPUT_HPP

#include <ascension/graphics/geometry.hpp>	// graphics.Point
#include <ctime>

namespace ascension {
	namespace viewers {
		namespace base {

			/// Abstract class represents a user input.
			class UserInput {
			public:
				enum Modifiers {
					/// The Shift key is down.
					SHIFT_DOWN		= 1 << 0,
					/// The Ctrl (Control) key is down.
					CONTROL_DOWN	= 1 << 1,
					/// The Alt key is down.
					ALT_DOWN		= 1 << 2,
					/// The AltGraph key is down.
					ALT_GRAPH_DOWN	= 1 << 3,
					/// The Command key is down. Only for Mac OS X.
					COMMAND_DOWN	= 1 << 4,
					/// The Mouse Button1 (usually left button) is down.
					BUTTON1_DOWN	= 1 << 5,
					/// The Mouse Button2 (usually middle button) is down.
					BUTTON2_DOWN	= 1 << 6,
					/// The Mouse Button3 (usually right button) is down.
					BUTTON3_DOWN	= 1 << 7,
					/// The Mouse Button4 (usually X1 button) is down.
					BUTTON4_DOWN	= 1 << 8,
					/// The Mouse Button5 (usually X2 button) is down.
					BUTTON5_DOWN	= 1 << 9
				};
			public:
				int modifiers() const /*throw()*/ {return modifiers_;}
				const std::time_t& timeStamp() const /*throw()*/ {return timeStamp_;}
			protected:
				/**
				 * Protected constructor.
				 * @param modifiers The modifier flags
				 */
				explicit UserInput(int modifiers) : modifiers_(modifiers) {std::time(&timeStamp_);}
			private:
				const int modifiers_;
				std::time_t timeStamp_;
			};

			/// Abstract class represents a user input located at a specific position in the screen.
			class LocatedUserInput : public UserInput {
			public:
				/// Returns the location.
				const graphics::Point<>& location() const /*throw()*/ {return location_;}
			protected:
				/**
				 * Protected constructor.
				 * @param location The location
				 * @param modifiers The modifier flags
				 */
				LocatedUserInput(const graphics::Point<>& location, int modifiers) : UserInput(modifiers), location_(location) {
				}
			private:
				const graphics::Point<> location_;
			};

			/// A @c MouseInput represents a general mouse event.
			class MouseInput : public LocatedUserInput {
			public:
				MouseInput(const graphics::Point<>& location, int modifiers) : LocatedUserInput(location, modifiers) {
				}
			};

			/// A @c MouseWheelEvent represents a mouse wheel event.
			class MouseWheelInput : public MouseInput {
			public:
				/**
				 * Constructor.
				 * @param location
				 * @param modifiers
				 * @param rotation
				 */
				MouseWheelInput(const graphics::Point<>& location, int modifiers,
					graphics::Dimension<>& rotation) : MouseInput(location, modifiers), rotation_(rotation) {}
				/// Returns the mouse wheel rotation.
				const graphics::Dimension<>& rotation() const /*throw()*/ {return rotation_;}
			private:
				const graphics::Dimension<> rotation_;
			};

			typedef uint8_t KeyboardCode;

			class KeyInput : public UserInput {
			public:
				KeyInput(KeyboardCode code, int modifiers, int repeatCount, int messageFlags)
					: UserInput(modifiers), keyCode_(code), repeatCount_(repeatCount), messageFlags_(messageFlags) {}
			private:
				const KeyboardCode keyCode_;
				const int repeatCount_, messageFlags_;
			};

		}
	}
}


#endif // !ASCENSION_USER_INPUT_HPP
