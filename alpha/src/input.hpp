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
#include <vector>


namespace alpha {
	namespace ui {
/*
		class InputTrigger {
		public:
			virtual bool equals(const InputTrigger& other) const throw() = 0;
			virtual std::wstring format() const throw() = 0;
		};
		std::wostream& operator<<(std::wostream& out, const InputTrigger& v) {return out << v.format();}
*/
		class KeyStroke /*: public InputTrigger*/ {
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
			explicit KeyStroke(VirtualKey naturalKey);
			KeyStroke(ModifierKey modifierKeys, VirtualKey naturalKey);
			KeyStroke(boost::python::object naturalKey);
			KeyStroke(boost::python::object a1, boost::python::object a2);
			bool operator==(const KeyStroke& other) const /*throw()*/;
			bool operator<(const KeyStroke& other) const /*throw()*/;
			template<typename InputIterator>
			static std::wstring format(InputIterator first, InputIterator last);
			static std::wstring format(boost::python::object keys);
			ModifierKey modifierKeys() const /*throw()*/;
			VirtualKey naturalKey() const /*throw()*/;
		private:
			// InputTrigger
			std::wstring format() const /*throw()*/;
		private:
			/*const*/ VirtualKey naturalKey_;
			/*const*/ ModifierKey modifierKeys_ : 8;
		};
/*
		class InputTriggerSequence {
		public:
			virtual std::wostream& operator<<(std::wostream& out) = 0;
			bool endsWith(const InputTriggerSequence& triggerSequence, bool equals) const;
			bool isEmpty() const throw();
			virtual boost::python::tuple prefixes() const = 0;
			bool startsWith(const InputTriggerSequence& triggerSequence, bool equals) const;
			boost::python::tuple triggers() const;
		protected:
			explicit InputTriggerSequence(const InputTrigger* first, const InputTrigger* last);
		private:
		};
		std::wostream& operator<<(std::wostream& out, const InputTriggerSequence& v);

		class KeySequence : public InputTriggerSequence {
		public:
			boost::python::tuple keyStrokes() const throw();
		private:
			boost::python::tuple keyStrokes_;
		};
*/
		/// Maps user inputs and commands.
		class InputMappingScheme {
			MANAH_NONCOPYABLE_TAG(InputMappingScheme);
		public:
			explicit InputMappingScheme(const std::wstring& name);
			~InputMappingScheme() /*throw()*/;
			boost::python::dict allDefinitions() const;
			boost::python::object boundCommands() const;
			boost::python::object command(const std::vector<const KeyStroke>& keySequence, bool* partialMatch) const;
			boost::python::object command(boost::python::object input) const;
			void define(const boost::python::object input, boost::python::object command, bool force = true);
			boost::python::object definedInputSequences() const;
			boost::python::object inputSequencesForCommand(boost::python::object command) const;
			bool isLocallyDefined(const boost::python::object input) const;
			const std::wstring& name() const /*throw()*/;
			void reset();
//			boost::python::object resolveParent() const;
//			void setResolveParent(boost::python::object parent);
//			void substituteBoundCommand();
			void undefine(boost::python::object input);
		private:
			boost::python::object** access(boost::python::object input) const;
		private:
			const std::wstring name_;
			class VectorKeymap;
			VectorKeymap* keymap_;
//			boost::python::object resolveParent_;
		};

		class InputManager {
			MANAH_NONCOPYABLE_TAG(InputManager);
		public:
			InputManager();
			bool input(const MSG& message);
			static InputManager& instance();
			boost::python::object mappingScheme() const;
			boost::python::object modalMappingScheme() const;
			void setMappingScheme(boost::python::object scheme);
			void setModalMappingScheme(boost::python::object scheme);
		private:
			std::pair<boost::python::object, const InputMappingScheme*> mappingScheme_, modalMappingScheme_;
			std::vector<const KeyStroke> pendingKeySequence_;
			boost::python::object inputTypedCharacterCommand_;
		};


		template<typename InputIterator> inline std::wstring KeyStroke::format(InputIterator first, InputIterator last) {
			std::wstring s;
			while(true) {
				s += first->format();
				if(++first == last)
					break;
				s += L" ";
			}
			return s;
		}

	}
} // namespace alpha.ui

#endif // !ALPHA_INPUT_HPP
