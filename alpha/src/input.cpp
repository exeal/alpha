/**
 * @file input.cpp
 * @author exeal
 * @date 2003-2007 (was keyboard-map.cpp)
 * @date 2009, 2014
 */

#include "input.hpp"
#include "application.hpp"	// StatusBar
#include "function-pointer.hpp"
//#include "resource/messages.h"
#include <ascension/viewer/widgetapi/event/user-input.hpp>
#include <bitset>
#include <boost/python/stl_iterator.hpp>
#include <glibmm/i18n.h>
#include <gtkmm/accelgroup.h>


namespace {
	template<typename ValueType>
	inline boost::python::stl_input_iterator<ValueType> makeStlInputIterator(boost::python::object o) {
		if(PyObject* temp = ::PyObject_GetIter(o.ptr()))
			boost::python::xdecref(temp);
		else
			boost::python::throw_error_already_set();
		return boost::python::stl_input_iterator<ValueType>(o);
	}
}

namespace alpha {
	namespace ui {
		// KeyStroke //////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * Creates a @c KeyStroke with the given key and modifiers.
		 * @param naturalKey
		 * @param modifierKeys
		 */
		KeyStroke::KeyStroke(NaturalKey naturalKey, ModifierKey modifierKeys /* = 0 */) : naturalKey_(naturalKey), modifierKeys_(modifierKeys) {
		}

		/**
		 * Equality operator returns @c true if equals to the given @c KeyStroke.
		 * @param other The @c KeyStroke object to test
		 * @return The result
		 */
		bool KeyStroke::operator==(const KeyStroke& other) const BOOST_NOEXCEPT {
			return naturalKey() == other.naturalKey() && modifierKeys() == other.modifierKeys();
		}
/*
namespace {
	inline wstring modifierString(UINT vkey, const wchar_t* defaultString) {
		if(const int sc = ::MapVirtualKeyW(vkey, MAPVK_VK_TO_VSC) << 16) {
			WCHAR s[256];
			if(::GetKeyNameTextW(sc | (1 << 25), s, MANAH_COUNTOF(s)) != 0)
				return s;
		}
		return defaultString;
	}
}

wstring KeyStroke::format(py::object keys) {
	if(py::extract<const KeyStroke&>(keys).check())
		return static_cast<const KeyStroke&>(py::extract<const KeyStroke&>(keys)).format();
	return format(makeStlInputIterator<const KeyStroke>(keys), py::stl_input_iterator<const KeyStroke>());
}
*/
		/// Returns the modifier keys.
		KeyStroke::ModifierKey KeyStroke::modifierKeys() const BOOST_NOEXCEPT {
			return modifierKeys_;
		}

		/// Returns the natural key code.
		KeyStroke::NaturalKey KeyStroke::naturalKey() const BOOST_NOEXCEPT {
			return naturalKey_;
		}

		/// Returns a human-readable text represents this key stroke.
		PlatformString KeyStroke::text() const BOOST_NOEXCEPT {
#if 1
			return Gtk::AccelGroup::get_label(naturalKey(), static_cast<Gdk::ModifierType>(modifierKeys()));
#else
			wstring result;
			result.reserve(256);

			if((modifierKeys() & CONTROL_KEY) != 0)
				result.append(modifierString(VK_CONTROL, L"Ctrl")).append(L"+");
			if((modifierKeys() & SHIFT_KEY) != 0)
				result.append(modifierString(VK_SHIFT, L"Shift")).append(L"+");
			if((modifierKeys() & ALTERNATIVE_KEY) != 0)
				result.append(modifierString(VK_MENU, L"Alt")).append(L"+");

			if(UINT sc = ::MapVirtualKeyW(naturalKey(), MAPVK_VK_TO_VSC) << 16) {
				switch(naturalKey()) {
				case VK_INSERT:	case VK_DELETE:	case VK_HOME:	case VK_END:
				case VK_NEXT:	case VK_PRIOR:	case VK_LEFT:	case VK_RIGHT:
				case VK_UP:		case VK_DOWN:
					sc |= (1 << 24);
				}
				wchar_t buffer[256];
				if(::GetKeyNameTextW(sc | (1 << 25), buffer, MANAH_COUNTOF(buffer)) != 0) {
					result += buffer;
					return result;
				}
			}
			return result += L"(unknown)";
#endif
		}


		// KeyMap /////////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * Creates and returns a new @c KeyMap object.
		 * @param name The name of this @c KeyMap
		 */
		KeyMap::KeyMap(const PlatformString& name /* = PlatformString() */) : name_(name), lockedCount_(0) {
		}

		/// @internal Raises Python's @c PermissionError if locked.
		inline void KeyMap::checkLock(const char* message) const {
			if(isLocked()) {
				::PyErr_SetString(PyExc_PermissionError, message);
				boost::python::throw_error_already_set();
			}
		}

		/**
		 * Binds the specified key stroke to the specified definition.
		 * @param key The key stroke
		 * @param definition The definition which is either a command (Python callable object) or a keymap. If this is
		 *                   @c boost#python#object(), @a key is set to undefined
		 * @throw boost#python#error_already_error(PermissionError) This @c KeyMap is locked
		 */
		void KeyMap::define(const KeyStroke& key, boost::python::object definition) {
			checkLock("KeyMap is locked");
			if(definition == boost::python::object())
				return undefine(key);
			if(!boost::python::extract<KeyMap&>(definition).check() && ::PyCallable_Check(definition.ptr()) == 0) {	// type mismatch
				::PyErr_BadArgument();
				boost::python::throw_error_already_set();
			} else
				table_[key] = definition;	// may throw
		}

		/**
		 * Binds the specified key stroke(s) to the specified definition.
		 * @param key The key stroke(s). This must be either @c KeyStroke or sequence of @c KeyStroke
		 * @param definition The definition which is either a command (Python callable object) or a keymap. If this is
		 *                   @c boost#python#object(), @a key is set to undefined
		 * @throw boost#python#error_already_set(TypeError) @a key had inappropriate type
		 * @throw boost#python#error_already_error(PermissionError) This @c KeyMap is locked
		 */
		void KeyMap::define(boost::python::object key, boost::python::object definition) {
			const auto k(lookupKeyMapAndKeyStroke(key));
			return k.first.define(k.second, definition);
		}

		/**
		 * Returns @c true if this @c KeyMap is locked.
		 * @see #lock, #unlock
		 */
		bool KeyMap::isLocked() const BOOST_NOEXCEPT {
			return lockedCount_ != 0;
		}

		/**
		 * Increments the lock count.
		 * @throw std#overflow_error
		 * @see #isLocked, #unlock
		 */
		void KeyMap::lock() {
			if(lockedCount_ == std::numeric_limits<decltype(lockedCount_)>::max())
				throw std::overflow_error("KeyMap.lock");
			++lockedCount_;
		}

		/// @see AbstractKeyMap#lookupKey(alpha#ui#KeyStroke)
		boost::python::object KeyMap::lookupKey(const KeyStroke& key) const BOOST_NOEXCEPT {
			try {
				const auto found(table_.find(key));
				if(found != std::end(table_))
					return found->second;
			} catch(...) {
			}
			return boost::python::object();
		}

		/// @see AbstractKeyMap#lookupKey(boost#python#object)
		boost::python::object KeyMap::lookupKey(boost::python::object key) const {
			const auto k(lookupKeyMapAndKeyStroke(key));
			return k.first.lookupKey(k.second);
		}

		/// @internal
		std::pair<KeyMap&, const KeyStroke&> KeyMap::lookupKeyMapAndKeyStroke(boost::python::object key) const {
			const boost::python::extract<const KeyStroke&> singleStroke(key);
			if(singleStroke.check())
				return std::make_pair(std::ref(const_cast<KeyMap&>(*this)), std::cref(singleStroke));

			const boost::python::ssize_t numberOfStrokes = boost::python::len(key);
			if(::PySequence_Check(key.ptr()) == 0 || numberOfStrokes < 1) {
				::PyErr_BadArgument();
				boost::python::throw_error_already_set();
			}

			boost::python::object v;
			for(boost::python::ssize_t i = 0; ; ++i) {
				// these may throw Python's TypeError
				const KeyMap& keyMap = boost::python::extract<const KeyMap&>(v);
				const KeyStroke& keyStroke = boost::python::extract<const KeyStroke&>(key[i]);

				if(i == numberOfStrokes - 1)
					return std::make_pair(std::ref(const_cast<KeyMap&>(keyMap)), std::cref(keyStroke));
				v = keyMap.lookupKey(keyStroke);
			}

			ASCENSION_ASSERT_NOT_REACHED();
		}

		/// Returns the name of this @c KeyMap.
		const PlatformString& KeyMap::name() const BOOST_NOEXCEPT {
			return name_;
		}

		/**
		 * Undefines the specified key stroke.
		 * @param key The key stroke
		 * @throw boost#python#error_already_error(PermissionError) This @c KeyMap is locked
		 */
		void KeyMap::undefine(const KeyStroke& key) {
			checkLock("KeyMap is locked");
			try {
				const auto found(table_.find(key));
				if(found != std::end(table_))
					table_.erase(found);
			} catch(...) {
			}
		}

		/**
		 * Undefines the specified key stroke(s).
		 * @param key The key stroke(s). This must be either @c KeyStroke or sequence of @c KeyStroke
		 * @throw boost#python#error_already_set(TypeError) @a key had inappropriate type
		 * @throw boost#python#error_already_error(PermissionError) This @c KeyMap is locked
		 */
		void KeyMap::undefine(boost::python::object key) {
			const auto k(lookupKeyMapAndKeyStroke(key));
			return k.first.undefine(k.second);
		}

		/**
		 * Decrements the lock count.
		 * @throw std#underflow_error
		 * @see #isLocked, #lock
		 */
		void KeyMap::unlock() {
			if(lockedCount_ == std::numeric_limits<decltype(lockedCount_)>::min())
				throw std::underflow_error("KeyMap.unlock");
			--lockedCount_;
		}


		// InputManager ///////////////////////////////////////////////////////////////////////////////////////////////

		InputManager::InputManager() : mappingSchemeLocker_(nullptr), modalMappingSchemeLocker_(nullptr) {
		}

		/// Cancels (ends) the incomplete (pending) key strokes.
		void InputManager::cancelIncompleteKeyStrokes() {
			if(!pendingKeyStrokes_.empty()) {
				pendingKeyStrokes_.clear();
				mappingSchemeLocker_.reset();
				modalMappingSchemeLocker_.reset();
			}
		}

		/**
		 * Handles button press event.
		 * @param event The event object
		 */
		bool InputManager::input(const GdkEventButton& event) {
			// TODO: Not implemented.
			return false;
		}

		/**
		 * Handles key press/release event.
		 * @param event The event object
		 */
		bool InputManager::input(const GdkEventKey& event) {
			if(event.type != Gdk::KEY_PRESS || event.is_modifier != 0)
				return false;

			const KeyStroke key(static_cast<KeyStroke::NaturalKey>(event.keyval), static_cast<KeyStroke::ModifierKey>(event.state));
			boost::python::object definition;
			if(pendingKeyStrokes_.empty())	// first stroke
				definition = lookupKey(key);
			else {
				pendingKeyStrokes_.push_back(key);
				boost::python::tuple keyStrokes(pendingKeyStrokes_);
				definition = lookupKey(keyStrokes);
			}

			if(::PyCallable_Check(definition.ptr()) != 0) {
				cancelIncompleteKeyStrokes();
				bool typed = false;
#ifndef ALPHA_NO_AMBIENT
				if(inputTypedCharacterCommand_.is_none())
					inputTypedCharacterCommand_ = ambient::Interpreter::instance().module("intrinsics").attr("input_typed_character");
				if(definition == inputTypedCharacterCommand_)
					typed = true;
				if(!typed)
					ambient::Interpreter::instance().executeCommand(definition);
#endif // !ALPHA_NO_AMBIENT
				return !typed;
			} else {
				// make human-readable text string for the incomplete (or undefined) key stroke(s)
				PlatformString incompleteKeyStrokes;
				assert(pendingKeyStrokes_.size() != 1);
				if(pendingKeyStrokes_.empty())
					incompleteKeyStrokes = key.text();
				else {
					for(std::size_t i = 0, c = pendingKeyStrokes_.size(); ; ++i) {
						incompleteKeyStrokes += pendingKeyStrokes_[i].text();
						if(i + 1 < c)
							incompleteKeyStrokes += ' ';
						else
							break;
					}
				}

				if(!definition.is_none()) {
					// begin multiple key stroke(s)
					assert(boost::python::extract<const KeyMap&>(definition).check());
					if(mappingScheme_.get() != nullptr)
						mappingSchemeLocker_.reset(new boost::lock_guard<KeyMapMutex>(KeyMapMutex(mappingScheme_.get())));
					if(modalMappingScheme_.get() != nullptr)
						modalMappingSchemeLocker_.reset(new boost::lock_guard<KeyMapMutex>(KeyMapMutex(modalMappingScheme_.get())));
					Application::instance()->window().statusBar().push(incompleteKeyStrokes);
				} else {	// undefined key stroke(s)
					cancelIncompleteKeyStrokes();
					::gdk_beep();	// TODO: Use Gdk.Window.beep() or Gdk.Display.beep() instead.
					Application::instance()->window().statusBar().push(Glib::ustring::compose(_("%1 is undefined"), incompleteKeyStrokes));
				}
				return true;
			}
		}

		/**
		 * Handles touch event.
		 * @param event The event object
		 */
		bool InputManager::input(const GdkEventTouch& event) {
			// TODO: Not implemented.
			return false;
		}

#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32) && 0
/***/
bool InputManager::input(const MSG& message) {
	if((message.message == WM_KEYDOWN || message.message == WM_SYSKEYDOWN) && message.wParam != VK_PROCESSKEY) {
		// this interrupts WM_CHARs
		const KeyStroke k(
			static_cast<KeyStroke::ModifierKey>(
				((::GetKeyState(VK_CONTROL) < 0) ? KeyStroke::CONTROL_KEY : 0)
				| ((::GetKeyState(VK_SHIFT) < 0) ? KeyStroke::SHIFT_KEY : 0)
				| ((message.message == WM_SYSKEYDOWN || ::GetKeyState(VK_MENU) < 0) ? KeyStroke::ALTERNATIVE_KEY : 0)),
			static_cast<KeyStroke::VirtualKey>(message.wParam));
		switch(k.naturalKey()) {
		case VK_SHIFT:
		case VK_CONTROL:
		case VK_MENU:
			return false;
		}
		bool partialMatch;
		pendingKeySequence_.push_back(k);
		py::object command(mappingScheme_.second->command(pendingKeySequence_, &partialMatch));
		if(command != py::object()) {
			pendingKeySequence_.clear();
			bool typed = false;
			if(inputTypedCharacterCommand_ == py::object())
				inputTypedCharacterCommand_ = Interpreter::instance().module("intrinsics").attr("input_typed_character");
			if(command == inputTypedCharacterCommand_)
				typed = true;
			if(!typed)
				ambient::Interpreter::instance().executeCommand(command);
			alpha::Alpha::instance().statusBar().setText(L"");
			return !typed;
		} else if(partialMatch) {
			const wstring s(KeyStroke::format(pendingKeySequence_.begin(), pendingKeySequence_.end()));
			alpha::Alpha::instance().statusBar().setText(s.c_str());
			return true;
		}
		const bool activateMenu =
			pendingKeySequence_.size() == 1 && (k.modifierKeys() & KeyStroke::ALTERNATIVE_KEY) != 0;
		pendingKeySequence_.clear();
		::MessageBeep(MB_OK);
		alpha::Alpha::instance().statusBar().setText(L"");
		return !activateMenu;
	} else if(message.message == WM_SYSCHAR) {
/*		// interrupt menu activation if key sequence is defined
		if(!pendingKeySequence_.empty())
			return true;
		const KeyStroke k(
			static_cast<KeyStroke::ModifierKey>(
				((::GetKeyState(VK_CONTROL) < 0) ? KeyStroke::CONTROL_KEY : 0)
				| ((::GetKeyState(VK_SHIFT) < 0) ? KeyStroke::SHIFT_KEY : 0)
				| KeyStroke::ALTERNATIVE_KEY),
			LOBYTE(::VkKeyScanW(static_cast<WCHAR>(message.wParam & 0xffffu))));
		bool partialMatch;
		py::object command(mappingScheme_.second->command(pendingKeySequence_, &partialMatch));
		if(command != py::object() || partialMatch)
			return true;
*/	}
	return false;
}
#endif

		/// Returns the singleton instance.
		InputManager& InputManager::instance() {
			static InputManager singleton;
			return singleton;
		}

		template<typename Key>
		inline boost::python::object InputManager::internalLookupKey(const Key& key) const {
			boost::python::object definition;
			if(const std::shared_ptr<KeyMap> keyMap = mappingScheme())
				definition = keyMap->lookupKey(key);
			if(definition.is_none()) {
				if(const std::shared_ptr<KeyMap> keyMap = modalMappingScheme())
					definition = keyMap->lookupKey(key);
			}
			return definition;
		}

		/// @see AbstractKeyMap#lookupKey(alpha#ui#KeyStroke)
		boost::python::object InputManager::lookupKey(const KeyStroke& key) const BOOST_NOEXCEPT {
			return internalLookupKey(key);
		}

		/// @see AbstractKeyMap#lookupKey(boost#python#object)
		boost::python::object InputManager::lookupKey(boost::python::object key) const {
			return internalLookupKey(key);
		}

		/**
		 * Returns the global mapping scheme.
		 * @see #modalMappingScheme, #setMappingScheme
		 */
		std::shared_ptr<KeyMap> InputManager::mappingScheme() const BOOST_NOEXCEPT {
			return mappingScheme_;
		}

		/**
		 * Returns the modal mapping scheme.
		 * @see #mappingScheme, #setModalMappingScheme
		 */
		std::shared_ptr<KeyMap> InputManager::modalMappingScheme() const BOOST_NOEXCEPT {
			return modalMappingScheme_;
		}

		/**
		 * Sets the global mapping scheme.
		 * @param scheme The mapping scheme to set. Can be @c null
		 * @throw boost#python#error_already_set(PermissionError) @c InputManager is receiving some input
		 * @throw boost#python#error_already_set(ValueError) @a scheme is same as @c #modalMappingScheme()
		 * @see #mappingScheme, #setModalMappingScheme
		 */
		void InputManager::setMappingScheme(std::shared_ptr<KeyMap> scheme) {
			if(!pendingKeyStrokes_.empty())
				::PyErr_SetString(PyExc_PermissionError, "InputManager.setMappingScheme");
			else if(scheme == modalMappingScheme())
				::PyErr_SetString(PyExc_ValueError, "InputManager.setMappingScheme");
			if(::PyErr_Occurred() != nullptr)
				boost::python::throw_error_already_set();
			mappingScheme_ = scheme;
		}

		/**
		 * Sets the modal mapping scheme.
		 * @param scheme The mapping scheme to set. Can be @c null
		 * @throw boost#python#error_already_set(PermissionError) @c InputManager is receiving some input
		 * @throw boost#python#error_already_set(ValueError) @a scheme is same as @c #modalMappingScheme()
		 * @see #modalMappingScheme, #setMappingScheme
		 */
		void InputManager::setModalMappingScheme(std::shared_ptr<KeyMap> scheme) {
			if(!pendingKeyStrokes_.empty())
				::PyErr_SetString(PyExc_PermissionError, "InputManager.setModalMappingScheme");
			else if(scheme == mappingScheme())
				::PyErr_SetString(PyExc_ValueError, "InputManager.setModalMappingScheme");
			if(::PyErr_Occurred() != nullptr)
				boost::python::throw_error_already_set();
			modalMappingScheme_ = scheme;
		}

#ifndef ALPHA_NO_AMBIENT
		ALPHA_EXPOSE_PROLOGUE(ambient::Interpreter::LOWEST_INSTALLATION_ORDER)
			ambient::Interpreter& interpreter = ambient::Interpreter::instance();
			boost::python::scope scope(interpreter.module("bindings"));

			boost::python::enum_<KeyStroke::NaturalKey>("NaturalKey");
			// TODO: Define values of NaturalKey.

			boost::python::enum_<KeyStroke::ModifierKey>("ModifierKey")
				.value("none", static_cast<KeyStroke::ModifierKey>(0))
				.value("shift", static_cast<KeyStroke::ModifierKey>(ascension::viewer::widgetapi::event::UserInput::SHIFT_DOWN))
				.value("ctrl", static_cast<KeyStroke::ModifierKey>(ascension::viewer::widgetapi::event::UserInput::CONTROL_DOWN))
				.value("alt", static_cast<KeyStroke::ModifierKey>(ascension::viewer::widgetapi::event::UserInput::ALT_DOWN))
				.value("meta", static_cast<KeyStroke::ModifierKey>(ascension::viewer::widgetapi::event::UserInput::META_DOWN));

			boost::python::class_<KeyStroke>("KeyStroke", boost::python::no_init)
				.def(boost::python::init<KeyStroke::NaturalKey, KeyStroke::ModifierKey>((boost::python::arg("natural_key"), boost::python::arg("modifier_keys") = 0)))
				.add_property("natural_key", &KeyStroke::naturalKey)
				.add_property("modifier_keys", &KeyStroke::modifierKeys)
				.def("text", &KeyStroke::text);

			boost::python::class_<KeyMap, std::shared_ptr<KeyMap>>("KeyMap")
				.def(boost::python::init<>((boost::python::arg("name") = PlatformString())))
				.def_readonly("name", boost::python::make_function(&KeyMap::name, boost::python::return_value_policy<boost::python::copy_const_reference>()))
				.def<void(KeyMap::*)(boost::python::object, boost::python::object)>("define", &KeyMap::define)
				.def<boost::python::object(KeyMap::*)(boost::python::object) const>("lookup_key", &KeyMap::lookupKey)
				.def<void(KeyMap::*)(boost::python::object)>("undefine", &KeyMap::undefine)
				.def("get", ambient::makeFunctionPointer([]() {
					return InputManager::instance().mappingScheme();
				})).staticmethod("get")
				.def("get_modal", ambient::makeFunctionPointer([]() {
					return InputManager::instance().modalMappingScheme();
				})).staticmethod("get_modal")
				.def("set_as_mapping_scheme", ambient::makeFunctionPointer([](std::shared_ptr<KeyMap> self) {
					InputManager::instance().setMappingScheme(self);
				}))
				.def("set_as_modal_mapping_scheme", ambient::makeFunctionPointer([](std::shared_ptr<KeyMap> self) {
					InputManager::instance().setModalMappingScheme(self);
				}));

			boost::python::scope scope1(interpreter.module("intrinsics"));
			boost::python::def("input_typed_character",
				ambient::makeFunctionPointer([](boost::python::object ed, boost::python::ssize_t n) {
					::PyErr_SetString(PyExc_NotImplementedError, "This command is not callable.");
				}),
				(boost::python::arg("ed") = boost::python::object(), boost::python::arg("n") = 1));
		ALPHA_EXPOSE_EPILOGUE()
#endif // !ALPHA_NO_AMBIENT
	}
}