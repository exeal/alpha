/**
 * @file input.hpp
 * @author exeal
 * @date 2003-2007 (was keyboard-map.hpp)
 * @date 2009, 2014
 */

#ifndef ALPHA_INPUT_HPP
#define ALPHA_INPUT_HPP
#include "ambient.hpp"
#include <ascension/viewer/widgetapi/user-input.hpp>
#include <boost/functional/hash.hpp>	// boost.hash_combine, boost.hash_value
#include <boost/operators.hpp>	// boost.equality_comparable
#include <memory>
#include <unordered_map>
#include <vector>

namespace alpha {
	namespace ui {
		class Input {
		public:
//			virtual bool equals(const Input& other) const BOOST_NOEXCEPT = 0;
//			virtual Glib::ustring format() const BOOST_NOEXCEPT = 0;
		};
//		std::wostream& operator<<(std::wostream& out, const Input& v) {return out << v.format();}

		/// Represents a key sequence.
		class KeyStroke : public Input, private boost::equality_comparable<KeyStroke> {
		public:
			typedef ascension::viewers::widgetapi::KeyInput::Code NaturalKey;
			typedef ascension::viewers::widgetapi::UserInput::KeyboardModifier ModifierKey;

		public:
			explicit KeyStroke(NaturalKey naturalKey, ModifierKey modifierKeys = 0);
			explicit KeyStroke(const Glib::ustring& format);
			bool operator==(const KeyStroke& other) const BOOST_NOEXCEPT;
			ModifierKey modifierKeys() const BOOST_NOEXCEPT;
			NaturalKey naturalKey() const BOOST_NOEXCEPT;
			Glib::ustring text() const BOOST_NOEXCEPT;

		private:
			/*const*/ NaturalKey naturalKey_;
			/*const*/ ModifierKey modifierKeys_;
		};
	}
}

namespace std {
	template<> struct hash<alpha::ui::KeyStroke> {
		size_t operator()(const alpha::ui::KeyStroke& key) const {
			size_t n = boost::hash_value(key.naturalKey());
			boost::hash_combine(n, key.modifierKeys());
			return n;
		}
	};
}

namespace alpha {
	namespace ui {
		class AbstractKeyMap {
		public:
			/**
			 * Returns the definition bound to the specified key stroke.
			 * @param key The key stroke
			 * @return The definition or @c None if @a key is bound to nothing (undefined)
			 */
			virtual boost::python::object lookupKey(const KeyStroke& key) const BOOST_NOEXCEPT;
			/**
			 * Returns the definition bound to the specified key stroke(s).
			 * @param key The key stroke(s). This must be either @c KeyStroke or sequence of @c KeyStroke
			 * @return The definition or @c None if @a key is bound to nothing (undefined)
			 * @throw boost#python#error_already_set(TypeError) @a key had inappropriate type
			 */
			virtual boost::python::object lookupKey(boost::python::object key) const;
		};

		class KeyMap : public AbstractKeyMap, public std::enable_shared_from_this<KeyMap> {
		public:
			explicit KeyMap(const Glib::ustring& name = Glib::ustring());

			/// @name Attributes
			/// @{
			boost::python::object lookupKey(const KeyStroke& key) const override BOOST_NOEXCEPT;
			boost::python::object lookupKey(boost::python::object key) const override;
			const Glib::ustring& name() const BOOST_NOEXCEPT;
			/// @}

			/// @name Definitions
			/// @{
			void define(const KeyStroke& key, boost::python::object definition);
			void define(boost::python::object key, boost::python::object definition);
			void undefine(const KeyStroke& key);
			void undefine(boost::python::object key);
			/// @}

			/// @name Access Control
			/// @{
			bool isLocked() const BOOST_NOEXCEPT;
			void lock() BOOST_NOEXCEPT;
			void unlock() BOOST_NOEXCEPT;
			/// @}

		private:
			void checkLock(const char* message) const;
			std::pair<KeyMap&, const KeyStroke&> lookupKeyMapAndKeyStroke(boost::python::object key) const;
		private:
			const Glib::ustring name_;
			std::unordered_map<KeyStroke, boost::python::object> table_;
			std::size_t lockedCount_;
		};
#if 0
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
#endif
		class InputManager : public AbstractKeyMap, private boost::noncopyable {
		public:
			InputManager();
			static InputManager& instance();

			/// @name Key Maps
			/// @{
			boost::python::object lookupKey(const KeyStroke& key) const override BOOST_NOEXCEPT;
			boost::python::object lookupKey(boost::python::object key) const override;
			std::shared_ptr<KeyMap> mappingScheme() const BOOST_NOEXCEPT;
			std::shared_ptr<KeyMap> modalMappingScheme() const BOOST_NOEXCEPT;
			void setMappingScheme(std::shared_ptr<KeyMap> scheme);
			void setModalMappingScheme(std::shared_ptr<KeyMap> scheme);
			/// @}

			/// @name Input Handling
			/// @{
			void cancelIncompleteKeyStrokes();
			bool input(const GdkEventButton& event);	// for button_press_event
			bool input(const GdkEventKey& event);	// for key_press_event
			bool input(const GdkEventTouch& event);	// for touch_event
			/// @}

		private:
			template<typename Key>
			boost::python::object internalLookupKey(const Key& key) const;
		private:
			std::shared_ptr<KeyMap> mappingScheme_, modalMappingScheme_;
			std::vector<KeyStroke> pendingKeyStrokes_;
			boost::python::object inputTypedCharacterCommand_;
			typedef ascension::detail::LockGuard<KeyMap, &KeyMap::lock, &KeyMap::unlock> KeyMapLocker;
			KeyMapLocker mappingSchemeLocker_, modalMappingSchemeLocker_;
		};
	}
} // namespace alpha.ui

#endif // !ALPHA_INPUT_HPP
