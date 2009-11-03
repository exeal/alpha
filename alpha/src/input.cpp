/**
 * @file input.cpp
 * @author exeal
 * @date 2003-2007 (was keyboard-map.cpp)
 * @date 2009
 */

#include "input.hpp"
#include "application.hpp"	// StatusBar
//#include "resource/messages.h"
#include <bitset>
#include <boost/python/stl_iterator.hpp>
using namespace alpha::ui;
using namespace alpha::ambient;
using namespace std;
namespace py = boost::python;


namespace {
	template<typename ValueType>
	inline py::stl_input_iterator<ValueType> makeStlInputIterator(const py::object& o) {
		if(PyObject* temp = ::PyObject_GetIter(o.ptr()))
			Py_XDECREF(temp);
		else
			py::throw_error_already_set();
		return py::stl_input_iterator<ValueType>(o);
	}
}


// KeyStroke ////////////////////////////////////////////////////////////////

KeyStroke::KeyStroke(VirtualKey naturalKey) : naturalKey_(naturalKey), modifierKeys_(NO_MODIFIER) {
}

KeyStroke::KeyStroke(ModifierKey modifierKeys, VirtualKey naturalKey) : naturalKey_(naturalKey), modifierKeys_(modifierKeys) {
}

namespace {
	KeyStroke::VirtualKey toVirtualKey(py::object o) {
		if(py::extract<KeyStroke::VirtualKey>(o).check())
			return py::extract<KeyStroke::VirtualKey>(o);
		WCHAR c = 0xffffu;
		if(py::extract<py::str>(o).check()) {	// o is a str
			if(py::len(o) == 1) {
				const py::str s = py::extract<py::str>(o);
				c = ::PyString_AsString(s.ptr())[0];
			}
		} else if(PyUnicode_Check(o.ptr()) != 0) {	// o is a unicode
			if(::PyUnicode_GetSize(o.ptr()) == 1) {
				wchar_t wc;
				if(::PyUnicode_AsWideChar(reinterpret_cast<PyUnicodeObject*>(o.ptr()), &wc, 1) != -1)
					c = wc;
			}
		}
		if(c != 0xffffu) {
			const KeyStroke::VirtualKey k = LOBYTE(::VkKeyScanW(c));
			if(k != 0xffu)
				return k;
		}
		::PyErr_BadArgument();
		py::throw_error_already_set();
	}
}

KeyStroke::KeyStroke(py::object naturalKey) : naturalKey_(toVirtualKey(naturalKey)), modifierKeys_(NO_MODIFIER) {
}

KeyStroke::KeyStroke(py::object a1, py::object a2) {
	if(py::extract<ModifierKey>(a1).check()) {
		modifierKeys_ = py::extract<ModifierKey>(a1);
		naturalKey_ = toVirtualKey(a2);
	} else if(py::extract<ModifierKey>(a2).check()) {
		naturalKey_ = toVirtualKey(a1);
		modifierKeys_ = py::extract<ModifierKey>(a2);
	} else {
		::PyErr_BadArgument();
		py::throw_error_already_set();
	}
}

bool KeyStroke::operator==(const KeyStroke& other) const /*throw()*/ {
	return naturalKey() == other.naturalKey() && modifierKeys() == other.modifierKeys();
}

bool KeyStroke::operator<(const KeyStroke& other) const /*throw()*/ {
	return naturalKey() < other.naturalKey()
		|| (naturalKey() == other.naturalKey() && modifierKeys() < other.modifierKeys());
}

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

/// @see InputTrigger#format
wstring KeyStroke::format() const {
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
}

wstring KeyStroke::format(py::object keys) {
	if(py::extract<const KeyStroke&>(keys).check())
		return static_cast<const KeyStroke&>(py::extract<const KeyStroke&>(keys)).format();
	return format(makeStlInputIterator<const KeyStroke>(keys), py::stl_input_iterator<const KeyStroke>());
}

KeyStroke::ModifierKey KeyStroke::modifierKeys() const /*throw()*/ {
	return modifierKeys_;
}

KeyStroke::VirtualKey KeyStroke::naturalKey() const /*throw()*/ {
	return naturalKey_;
}


// InputMappingScheme.FirstKeyTable /////////////////////////////////////////////

namespace {
	class KeyTableElement {
	public:
		static const py::object SEQUENCE_NO_MATCH, SEQUENCE_PARTIAL_MATCH;
	public:
		virtual ~KeyTableElement() /*throw()*/ {}
		virtual const py::object& command(vector<const KeyStroke>::const_iterator first, vector<const KeyStroke>::const_iterator last) const = 0;
		virtual bool define(vector<const KeyStroke>::const_iterator first, vector<const KeyStroke>::const_iterator last, py::object command) = 0;
		virtual bool find(py::object command, vector<const KeyStroke>&) const = 0;
		void undefine(vector<const KeyStroke>::const_iterator first, vector<const KeyStroke>::const_iterator last) {
			define(first, last, py::object());
		}
	};
	const py::object KeyTableElement::SEQUENCE_NO_MATCH, KeyTableElement::SEQUENCE_PARTIAL_MATCH;

	class CommandHolder : public KeyTableElement {
	public:
		explicit CommandHolder(py::object command);
		const py::object& command(vector<const KeyStroke>::const_iterator first, vector<const KeyStroke>::const_iterator last) const;
		bool define(vector<const KeyStroke>::const_iterator first, vector<const KeyStroke>::const_iterator last, py::object command);
		bool find(py::object command, vector<const KeyStroke>&) const;
	private:
		py::object command_;
	};

	class SparseKeymap : public KeyTableElement {
	public:
		SparseKeymap(vector<const KeyStroke>::const_iterator first, vector<const KeyStroke>::const_iterator last, py::object command);
		~SparseKeymap();
		const py::object& command(vector<const KeyStroke>::const_iterator first, vector<const KeyStroke>::const_iterator last) const;
		bool define(vector<const KeyStroke>::const_iterator first, vector<const KeyStroke>::const_iterator last, py::object command);
		bool find(py::object command, vector<const KeyStroke>& sequence) const;
	private:
		map<const KeyStroke, KeyTableElement*> map_;
	};
}

class InputMappingScheme::VectorKeymap : public KeyTableElement {
public:
	VectorKeymap();
	~VectorKeymap();
	const py::object& command(vector<const KeyStroke>::const_iterator first, vector<const KeyStroke>::const_iterator last) const;
	bool define(vector<const KeyStroke>::const_iterator first, vector<const KeyStroke>::const_iterator last, py::object command);
	bool find(py::object command, vector<const KeyStroke>& sequence) const;
private:
	KeyTableElement** access(const KeyStroke& keys, bool force) const;
private:
	KeyTableElement*** planes_[KeyStroke::ALTERNATIVE_KEY << 2];
};

CommandHolder::CommandHolder(py::object command) : command_(command) {
}

const py::object& CommandHolder::command(vector<const KeyStroke>::const_iterator first, vector<const KeyStroke>::const_iterator last) const {
	return (first == last) ? command_ : SEQUENCE_NO_MATCH;
}

bool CommandHolder::define(vector<const KeyStroke>::const_iterator first, vector<const KeyStroke>::const_iterator last, py::object command) {
	if(first < last)
		return false;
	assert(first == last);
	return (command_ = command), true;
}

bool CommandHolder::find(py::object command, vector<const KeyStroke>&) const {
	return command == command_;
}

SparseKeymap::SparseKeymap(vector<const KeyStroke>::const_iterator first, vector<const KeyStroke>::const_iterator last, py::object command) {
	assert(first < last);
	pair<const KeyStroke, KeyTableElement*> firstChild(make_pair(*(first++), static_cast<KeyTableElement*>(0)));
	if(first == last)
		firstChild.second = new CommandHolder(command);
	else
		firstChild.second = new SparseKeymap(first, last, command);
	map_.insert(firstChild);
}

SparseKeymap::~SparseKeymap() {
	for(map<const KeyStroke, KeyTableElement*>::const_iterator i(map_.begin()), e(map_.end()); i != e; ++i)
		delete i->second;
}

const py::object& SparseKeymap::command(vector<const KeyStroke>::const_iterator first, vector<const KeyStroke>::const_iterator last) const {
	if(first == last)
		return SEQUENCE_PARTIAL_MATCH;
	map<const KeyStroke, KeyTableElement*>::const_iterator i(map_.find(*first));
	return (i != map_.end()) ? i->second->command(++first, last) : SEQUENCE_NO_MATCH;
}

bool SparseKeymap::define(vector<const KeyStroke>::const_iterator first, vector<const KeyStroke>::const_iterator last, py::object command) {
	assert(first < last);
	map<const KeyStroke, KeyTableElement*>::const_iterator i(map_.find(*first));
	if(i == map_.end()) {	// new
		if(command != py::object()) {
			if(first == last - 1)
				map_.insert(make_pair(*first, new CommandHolder(command)));
			else
				map_.insert(make_pair(*first, new SparseKeymap(first + 1, last, command)));
		}
	} else if(command == py::object()) {	// undefine
		if(first + 1 < last)
			return false;
		delete i->second;
		map_.erase(i);
	} else {	// overwrite
		assert(i->second != 0);
		return i->second->define(++first, last, command);
	}
	return true;
}

bool SparseKeymap::find(py::object command, vector<const KeyStroke>& sequence) const {
	for(map<const KeyStroke, KeyTableElement*>::const_iterator i(map_.begin()), e(map_.end()); i != e; ++i) {
		if(i->second->find(command, sequence))
			return sequence.push_back(i->first), true;
	}
	return false;
}

InputMappingScheme::VectorKeymap::VectorKeymap() {
	memset(planes_, 0, sizeof(KeyTableElement***) * MANAH_COUNTOF(planes_));
}

InputMappingScheme::VectorKeymap::~VectorKeymap() {
	for(KeyTableElement**** plane = planes_; plane < MANAH_ENDOF(planes_); ++plane) {
		if(*plane != 0) {
			for(size_t i = 0; i < 2; ++i) {
				if(KeyTableElement** half = (*plane)[i]) {
					for(size_t j = 0; j < 0x80; ++j)
						delete half[j];
					delete[] half;
				}
			}
			delete[] *plane;
		}
	}
}

KeyTableElement** InputMappingScheme::VectorKeymap::access(const KeyStroke& keys, bool force) const {
	KeyTableElement*** const& plane = planes_[keys.modifierKeys()];
	if(plane == 0) {
		if(!force)
			return 0;
		memset(const_cast<KeyTableElement***&>(plane) = new KeyTableElement**[2], 0, sizeof(KeyTableElement**) * 2);
	}
	KeyTableElement** const& half = plane[(keys.naturalKey() & 0x80) >> 7];
	if(half == 0) {
		if(!force)
			return 0;
		memset(const_cast<KeyTableElement**&>(half) = new KeyTableElement*[0x80], 0, sizeof(KeyTableElement*) * 0x80);
	}
	return &half[keys.naturalKey() & 0x7f];
}

const py::object& InputMappingScheme::VectorKeymap::command(
		vector<const KeyStroke>::const_iterator first, vector<const KeyStroke>::const_iterator last) const {
	assert(first < last);
	const KeyTableElement* const * const slot = access(*first, false);
	return (slot != 0 && *slot != 0) ? (*slot)->command(++first, last) : SEQUENCE_NO_MATCH;
}

bool InputMappingScheme::VectorKeymap::define(
		vector<const KeyStroke>::const_iterator first, vector<const KeyStroke>::const_iterator last, py::object command) {
	if(first == last)
		return false;
	KeyTableElement** slot = access(*first, true);
	if(*slot == 0) {	// new
		if(first + 1 == last)
			*slot = new CommandHolder(command);
		else
			*slot = new SparseKeymap(++first, last, command);
	} else if(command == py::object()) {	// undefine
		if(first + 1 < last)
			return false;
		delete *slot;
		*slot = 0;
	} else	// overwrite
		return (*slot)->define(++first, last, command);
	return true;
}

bool InputMappingScheme::VectorKeymap::find(py::object command, vector<const KeyStroke>& sequence) const {
	for(manah::byte m = 0; m < MANAH_COUNTOF(planes_); ++m) {
		if(KeyTableElement*** const plane = planes_[m]) {
			for(KeyStroke::VirtualKey h = 0; h < 2; ++h) {
				if(KeyTableElement** const half = plane[h]) {
					for(KeyStroke::VirtualKey k = 0; k < 0x80; ++k) {
						if(const KeyTableElement* slot = half[k]) {
							if(slot->find(command, sequence))
								return sequence.push_back(KeyStroke(static_cast<KeyStroke::ModifierKey>(m), k | (h << 7))), true;
						}
					}
				}
			}
		}
	}
	return false;
}


// InputMappingScheme ///////////////////////////////////////////////////////

InputMappingScheme::InputMappingScheme(const wstring& name) : name_(name), keymap_(new VectorKeymap) {
}

InputMappingScheme::~InputMappingScheme() {
	delete keymap_;
}

py::object** InputMappingScheme::access(py::object input) const {
//	return static_cast<py::object*>(firstKeyTable_->get(py::extract<const KeyCombination&>(input)));
	return 0;
}

py::dict InputMappingScheme::allDefinitions() const {
	return py::dict();
}

py::object InputMappingScheme::boundCommands() const {
	return py::object(py::handle<>(::PyFrozenSet_New(0)));
}

namespace {
	void toKeySequence(py::object input, vector<const KeyStroke>& v) {
		if(py::extract<const KeyStroke&>(input).check()) {
			const KeyStroke& keys = py::extract<const KeyStroke&>(input);
			v.assign(&keys, &keys + 1);
		} else
			v.assign(makeStlInputIterator<const KeyStroke>(input), py::stl_input_iterator<const KeyStroke>());
	}
}

py::object InputMappingScheme::command(const vector<const KeyStroke>& keySequence, bool* partialMatch) const {
	const py::object& result = keymap_->command(keySequence.begin(), keySequence.end());
	if(partialMatch)
		*partialMatch = &result == &KeyTableElement::SEQUENCE_PARTIAL_MATCH;
	return result;
}

py::object InputMappingScheme::command(py::object input) const {
	vector<const KeyStroke> v;
	toKeySequence(input, v);
	return command(v, 0);
}

void InputMappingScheme::define(const py::object input, py::object command, bool force /* = true */) {
	// TODO: consider force paramter.
	vector<const KeyStroke> v;
	toKeySequence(input, v);
	keymap_->define(v.begin(), v.end(), command);
}

py::object InputMappingScheme::definedInputSequences() const {
	return py::object(py::handle<>(::PyFrozenSet_New(0)));
}

py::object InputMappingScheme::inputSequencesForCommand(py::object command) const {
	vector<const KeyStroke> keySequence;
	if(!keymap_->find(command, keySequence))
		return py::object(py::handle<>(::PyFrozenSet_New(0)));

	py::list l;
	for(vector<const KeyStroke>::const_reverse_iterator i(keySequence.rbegin()), e(keySequence.rend()); i != e; ++i)
		l.append(py::object(*i));
	py::list l2;
	l2.append(l);
	return /*py::object(py::handle<>(::PyFrozenSet_New(*/l2/*.ptr())))*/;
}

bool InputMappingScheme::isLocallyDefined(py::object input) const {
	return access(input) != 0;
}

const wstring& InputMappingScheme::name() const /*throw()*/ {
	return name_;
}

void InputMappingScheme::reset() {
}

void InputMappingScheme::undefine(py::object input) {
}


// InputManager /////////////////////////////////////////////////////////////

InputManager::InputManager() :
		mappingScheme_(make_pair(py::object(), static_cast<InputMappingScheme*>(0))),
		modalMappingScheme_(make_pair(py::object(), static_cast<InputMappingScheme*>(0))) {
}

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

/// Returns the singleton instance.
InputManager& InputManager::instance() {
	static InputManager singleton;
	return singleton;
}

py::object InputManager::mappingScheme() const {
	return mappingScheme_.first;
}

py::object InputManager::modalMappingScheme() const {
	return modalMappingScheme_.first;
}

void InputManager::setMappingScheme(py::object scheme) {
	if(scheme == modalMappingScheme()) {
		::PyErr_BadArgument();
		py::throw_error_already_set();
	}
	mappingScheme_.first = scheme;
	mappingScheme_.second = py::extract<InputMappingScheme*>(scheme);
}

void InputManager::setModalMappingScheme(py::object scheme) {
	if(scheme == mappingScheme()) {
		::PyErr_BadArgument();
		py::throw_error_already_set();
	}
	modalMappingScheme_.first = scheme;
	modalMappingScheme_.second = py::extract<InputMappingScheme*>(scheme);
}


namespace {
	void inputTypedCharacter(py::object ed, py::ssize_t n) {::PyErr_SetString(PyExc_NotImplementedError, "This command is not callable.");}
	py::object mappingScheme() {return InputManager::instance().mappingScheme();}
	py::object modalMappingScheme() {return InputManager::instance().modalMappingScheme();}
	void setAsMappingScheme(py::object s) {InputManager::instance().setMappingScheme(s);}
	void setAsModalMappingScheme(py::object s) {InputManager::instance().setModalMappingScheme(s);}
}

ALPHA_EXPOSE_PROLOGUE(Interpreter::LOWEST_INSTALLATION_ORDER)
	Interpreter& interpreter = Interpreter::instance();
	py::scope scope(interpreter.module("bindings"));

	py::enum_<KeyStroke::VirtualKey>("NaturalKey")
		.value("left_mouse_button", VK_LBUTTON)
		.value("right_mouse_button", VK_RBUTTON)
		.value("cancel", VK_CANCEL)
		.value("middle_mouse_button", VK_MBUTTON)
		.value("x1_mouse_button", VK_XBUTTON1)
		.value("x2_mouse_button", VK_XBUTTON2)
		.value("back_space", VK_BACK).value("bs", VK_BACK)
		.value("tab", VK_TAB)
		.value("clear", VK_CLEAR)
		.value("enter", VK_RETURN)
		.value("shift", VK_SHIFT)
		.value("control", VK_CONTROL)
		.value("menu", VK_MENU).value("alt", VK_MENU)
		.value("pause", VK_PAUSE)
		.value("caps_lock", VK_CAPITAL).value("caps_lk", VK_CAPITAL)
		.value("kana", VK_KANA).value("kana", VK_HANGUL)	// 0x15
		.value("junja", VK_JUNJA)
		.value("final", VK_FINAL)
		.value("hanja", VK_HANJA).value("kanji", VK_KANJI)	// 0x19
		.value("escape", VK_ESCAPE)
		.value("convert", VK_CONVERT)
		.value("nonconvert", VK_NONCONVERT)
		.value("accept", VK_ACCEPT)
		.value("modechange", VK_MODECHANGE)
		.value("space", VK_SPACE)
		.value("page_up", VK_PRIOR)
		.value("page_down", VK_NEXT)
		.value("end", VK_END)
		.value("home", VK_HOME)
		.value("left", VK_LEFT)
		.value("up", VK_UP)
		.value("right", VK_RIGHT)
		.value("down", VK_DOWN)
		.value("select", VK_SELECT)
		.value("print", VK_PRINT)
		.value("execute", VK_EXECUTE)
		.value("print_screen", VK_SNAPSHOT).value("prt_sc", VK_SNAPSHOT)
		.value("insert", VK_INSERT).value("ins", VK_INSERT)
		.value("delete", VK_DELETE).value("del", VK_DELETE)
		.value("help", VK_HELP)
		.value("zero", 0x30)
		.value("one", 0x31)
		.value("two", 0x32)
		.value("three", 0x33)
		.value("four", 0x34)
		.value("five", 0x35)
		.value("six", 0x36)
		.value("seven", 0x37)
		.value("eight", 0x38)
		.value("nine", 0x39)
		.value("left_windows", VK_LWIN)
		.value("right_windows", VK_RWIN)
		.value("context_menu", VK_APPS).value("applications", VK_APPS)
		.value("sleep", VK_SLEEP)
		.value("numpad0", VK_NUMPAD0)
		.value("numpad1", VK_NUMPAD1)
		.value("numpad2", VK_NUMPAD2)
		.value("numpad3", VK_NUMPAD3)
		.value("numpad4", VK_NUMPAD4)
		.value("numpad5", VK_NUMPAD5)
		.value("numpad6", VK_NUMPAD6)
		.value("numpad7", VK_NUMPAD7)
		.value("numpad8", VK_NUMPAD8)
		.value("numpad9", VK_NUMPAD9)
		.value("multiply", VK_MULTIPLY)
		.value("add", VK_ADD)
		.value("plus", VK_ADD)
		.value("separator", VK_SEPARATOR)
		.value("minus", VK_SUBTRACT)
		.value("decimal", VK_DECIMAL)
		.value("divide", VK_DIVIDE)
		.value("f1", VK_F1)
		.value("f2", VK_F2)
		.value("f3", VK_F3)
		.value("f4", VK_F4)
		.value("f5", VK_F5)
		.value("f6", VK_F6)
		.value("f7", VK_F7)
		.value("f8", VK_F8)
		.value("f9", VK_F9)
		.value("f10", VK_F10)
		.value("f11", VK_F11)
		.value("f12", VK_F12)
		.value("f13", VK_F13)
		.value("f14", VK_F14)
		.value("f15", VK_F15)
		.value("f16", VK_F16)
		.value("f17", VK_F17)
		.value("f18", VK_F18)
		.value("f19", VK_F19)
		.value("f20", VK_F20)
		.value("f21", VK_F21)
		.value("f22", VK_F22)
		.value("f23", VK_F23)
		.value("f24", VK_F24)
		.value("number_lock", VK_NUMLOCK).value("num_lock", VK_NUMLOCK).value("nm_lk", VK_NUMLOCK)
		.value("scroll_lock", VK_SCROLL).value("scr_lk", VK_SCROLL)
		.value("left_shift", VK_LSHIFT)
		.value("right_shift", VK_RSHIFT)
		.value("left_control", VK_LCONTROL)
		.value("right_control", VK_RCONTROL)
		.value("left_menu", VK_LMENU).value("left_alt", VK_LMENU)
		.value("right_menu", VK_RMENU).value("right_alt", VK_RMENU)
		.value("browser_back", VK_BROWSER_BACK)
		.value("browser_forward", VK_BROWSER_FORWARD)
		.value("browser_refresh", VK_BROWSER_REFRESH)
		.value("browser_stop", VK_BROWSER_STOP)
		.value("browser_search", VK_BROWSER_SEARCH)
		.value("browser_favorites", VK_BROWSER_FAVORITES)
		.value("browser_home", VK_BROWSER_HOME)
		.value("volume_mute", VK_VOLUME_MUTE)
		.value("volume_down", VK_VOLUME_DOWN)
		.value("volume_up", VK_VOLUME_UP)
		.value("media_next_track", VK_MEDIA_NEXT_TRACK)
		.value("media_prev_track", VK_MEDIA_PREV_TRACK)
		.value("media_stop", VK_MEDIA_STOP)
		.value("media_play_pause", VK_MEDIA_PLAY_PAUSE)
		.value("launch_mail", VK_LAUNCH_MAIL).value("start_mail", VK_LAUNCH_MAIL)
		.value("launch_media_select", VK_LAUNCH_MEDIA_SELECT).value("media_select", VK_LAUNCH_MEDIA_SELECT)
		.value("launch_application_1", VK_LAUNCH_APP1).value("start_application_1", VK_LAUNCH_APP1)
		.value("launch_application_2", VK_LAUNCH_APP2).value("start_application_2", VK_LAUNCH_APP2)
		.value("oem_1", VK_OEM_1)
		.value("oem_plus", VK_OEM_PLUS)
		.value("oem_comma", VK_OEM_COMMA)
		.value("oem_minus", VK_OEM_MINUS)
		.value("oem_period", VK_OEM_PERIOD)
		.value("oem_2", VK_OEM_2)
		.value("oem_3", VK_OEM_3)
		.value("oem_4", VK_OEM_4)
		.value("oem_5", VK_OEM_5)
		.value("oem_6", VK_OEM_6)
		.value("oem_7", VK_OEM_7)
		.value("oem_8", VK_OEM_8)
		.value("oem_ax", VK_OEM_AX)
		.value("oem_102", VK_OEM_102)
		.value("ico_help", VK_ICO_HELP)
		.value("ico_00", VK_ICO_00)
		.value("process", VK_PROCESSKEY)
		.value("ico_clear", VK_ICO_CLEAR)
//		.value("_packet", VK_PACKET)
		.value("attn", VK_ATTN)
		.value("crsel", VK_CRSEL)
		.value("exsel", VK_EXSEL)
		.value("ereof", VK_EREOF)
		.value("play", VK_PLAY)
		.value("zoom", VK_ZOOM)
		.value("pa1", VK_PA1)
		.value("oem_clear", VK_OEM_CLEAR);
	py::enum_<KeyStroke::ModifierKey>("ModifierKey")
		.value("none", KeyStroke::NO_MODIFIER)
		.value("ctrl", KeyStroke::CONTROL_KEY)
		.value("shift", KeyStroke::SHIFT_KEY)
		.value("alt", KeyStroke::ALTERNATIVE_KEY)
		.value("ctrl_shift", static_cast<KeyStroke::ModifierKey>(KeyStroke::CONTROL_KEY | KeyStroke::SHIFT_KEY))
		.value("ctrl_alt", static_cast<KeyStroke::ModifierKey>(KeyStroke::CONTROL_KEY | KeyStroke::ALTERNATIVE_KEY))
		.value("shift_alt", static_cast<KeyStroke::ModifierKey>(KeyStroke::SHIFT_KEY | KeyStroke::ALTERNATIVE_KEY))
		.value("ctrl_shift_alt", static_cast<KeyStroke::ModifierKey>(KeyStroke::CONTROL_KEY | KeyStroke::SHIFT_KEY | KeyStroke::ALTERNATIVE_KEY));

	py::class_<KeyStroke>("KeyStroke", py::init<py::object, py::object>())
		.def(py::init<py::object>())
		.add_property("natural_key", &KeyStroke::naturalKey)
		.add_property("modifier_keys", &KeyStroke::modifierKeys)
		.def<wstring(py::object)>("format", &KeyStroke::format).staticmethod("format");
	py::class_<InputMappingScheme, boost::noncopyable>("InputMappingScheme", py::init<const std::wstring>())
//		.add_property("resolve_parent", &InputMappingScheme::resolveParent, &InputMappingScheme::setResolveParent)
		.def("all_definitions", &InputMappingScheme::allDefinitions)
		.def("bound_commands", &InputMappingScheme::boundCommands)
		.def<py::object (InputMappingScheme::*)(py::object) const>("command", &InputMappingScheme::command)
		.def("define", &InputMappingScheme::define, (py::arg("input"), py::arg("command"), py::arg("force") = true))
		.def("defined_input_sequences", &InputMappingScheme::definedInputSequences)
		.def("get", &mappingScheme).staticmethod("get")
		.def("get_modal", &modalMappingScheme).staticmethod("get_modal")
		.def("is_locally_defined", &InputMappingScheme::isLocallyDefined)
		.def("reset", &InputMappingScheme::reset)
		.def("set_as_mapping_scheme", &setAsMappingScheme)
		.def("set_as_modal_mapping_scheme", &setAsModalMappingScheme)
		.def("undefine", &InputMappingScheme::undefine);

	py::scope scope1(interpreter.module("intrinsics"));
	py::def("input_typed_character", &inputTypedCharacter, (py::arg("ed") = py::object(), py::arg("n") = 1));
ALPHA_EXPOSE_EPILOGUE()
