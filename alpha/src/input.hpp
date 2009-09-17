/**
 * @file input.hpp
 * @author exeal
 * @date 2003-2007 (was keyboard-map.hpp)
 * @date 2009
 */

#ifndef ALPHA_INPUT_HPP
#define ALPHA_INPUT_HPP

//#include "resource.h"
#include <manah/win32/windows.hpp>
//#include <sstream>
#include "ambient.hpp"


namespace alpha {
	namespace ui {

		class KeyCombination {
		public:
			/// A virtual key code.
			typedef manah::byte VirtualKey;
			/// Modifier keys.
			enum ModifierKey {
				NO_MODIFIER = 0x00,		///< No modifier key.
				CONTROL_KEY = 0x01,		///< Ctrl key.
				SHIFT_KEY = 0x02,		///< Shift key.
				ALTERNATIVE_KEY = 0x04	///< Alt key.
			};
		public:
			explicit KeyCombination(VirtualKey keyCode, ModifierKey modifiers = NO_MODIFIER);
			VirtualKey keyCode() const /*throw()*/;
			ModifierKey modifiers() const /*throw()*/;
		private:
			const VirtualKey vkey_;
			const ModifierKey modifiers_ : 8;
		};

		/// Maps user inputs and commands.
		class InputMap {
		public:
			InputMap();
			~InputMap() /*throw()*/;
			boost::python::dict allDefinitions() const;
			boost::python::object boundCommands() const;
			boost::python::object command(boost::python::object input) const;
			void define(const boost::python::object input, boost::python::object command, bool force = true);
			boost::python::object definedInputStrokes() const;
			boost::python::object inputStrokesForCommand(boost::python::object command) const;
			bool isLocallyDefined(const boost::python::object input) const;
			void reset();
			boost::python::object resolveParent() const;
			void setResolveParent(boost::python::object parent);
//			void substituteBoundCommand();
			void undefine(boost::python::object input);
		private:
			boost::python::object** access(boost::python::object input) const;
		private:
			class FirstKeyTable;
			FirstKeyTable* firstKeyTable_;
			boost::python::object resolveParent_;
		};

#if 0
		/**
		 * Returns the name of the virtual key.
		 * @param key the virtual key code
		 * @return the key name or an empty string if @a key is invalid
		 */
		inline std::wstring KeyboardMap::keyName(VirtualKey key) {
			UINT c = ::MapVirtualKeyW(key, 0) << 16;
			switch(key) {
			case VK_INSERT:	case VK_DELETE:	case VK_HOME:	case VK_END:
			case VK_NEXT:	case VK_PRIOR:	case VK_LEFT:	case VK_RIGHT:
			case VK_UP:		case VK_DOWN:
				c |= (1 << 24);
			}
			wchar_t buffer[256];
			return (::GetKeyNameTextW(c | (1 << 25), buffer, countof(buffer)) != 0) ? buffer : L"";
		}
#endif

	}
} // namespace alpha.ui

#endif // !ALPHA_INPUT_HPP
