/**
 * @file input.cpp
 * @author exeal
 * @date 2003-2007 (was keyboard-map.cpp)
 * @date 2009
 */

#include "input.hpp"
//#include "resource/messages.h"
#include <bitset>
using namespace alpha::ui;
using namespace alpha::ambient;
using namespace std;
namespace py = boost::python;


// KeyStroke ////////////////////////////////////////////////////////////////

KeyStroke::KeyStroke(VirtualKey naturalKey) : naturalKey_(naturalKey), modifierKeys_(NO_MODIFIER) {
}

KeyStroke::KeyStroke(ModifierKey modifierKeys, VirtualKey naturalKey) : naturalKey_(naturalKey), modifierKeys_(modifierKeys) {
}

namespace {
	KeyStroke::VirtualKey toVirtualKey(py::object o) {
		if(py::extract<KeyStroke::VirtualKey>(o).check())
			return py::extract<KeyStroke::VirtualKey>(o);
		else if(py::extract<py::str>(o).check()) {	// o is a str
			if(py::len(o) != 1) {
				::PyErr_BadArgument();
				py::throw_error_already_set();
			}
			const py::str s = py::extract<py::str>(o);
			return static_cast<KeyStroke::VirtualKey>(toupper(::PyString_AsString(s.ptr())[0], locale::classic()));
		} else if(PyUnicode_Check(o.ptr()) != 0) {	// o is a unicode
			if(::PyUnicode_GetSize(o.ptr()) == 1) {
				wchar_t c;
				if(::PyUnicode_AsWideChar(reinterpret_cast<PyUnicodeObject*>(o.ptr()), &c, 1) != -1) {
					if(c < 0x0100u)
						return static_cast<KeyStroke::VirtualKey>(toupper(static_cast<char>(c & 0x00ffu), locale::classic()));
					else
						::PyErr_BadArgument();
				}
			}
			py::throw_error_already_set();
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

	if((modifierKeys() | CONTROL_KEY) != 0)
		result.append(modifierString(VK_CONTROL, L"Ctrl")).append(L"+");
	if((modifierKeys() | SHIFT_KEY) != 0)
		result.append(modifierString(VK_SHIFT, L"Shift")).append(L"+");
	if((modifierKeys() | ALTERNATIVE_KEY) != 0)
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
		static const py::object PATH_NOT_FOUND;
	public:
		virtual ~KeyTableElement() /*throw()*/ {}
		virtual const py::object* command(vector<const KeyStroke>::const_iterator first, vector<const KeyStroke>::const_iterator last) const = 0;
		virtual bool define(vector<const KeyStroke>::const_iterator first, vector<const KeyStroke>::const_iterator last, py::object command) = 0;
		virtual bool find(py::object command, vector<const KeyStroke>&) const = 0;
		void undefine(vector<const KeyStroke>::const_iterator first, vector<const KeyStroke>::const_iterator last) {
			define(first, last, py::object());
		}
	};
	const py::object KeyTableElement::PATH_NOT_FOUND;

	class CommandHolder : public KeyTableElement {
	public:
		explicit CommandHolder(py::object command);
		const py::object* command(vector<const KeyStroke>::const_iterator first, vector<const KeyStroke>::const_iterator last) const;
		bool define(vector<const KeyStroke>::const_iterator first, vector<const KeyStroke>::const_iterator last, py::object command);
		bool find(py::object command, vector<const KeyStroke>&) const;
	private:
		py::object command_;
	};

	class SparseKeymap : public KeyTableElement {
	public:
		SparseKeymap(vector<const KeyStroke>::const_iterator first, vector<const KeyStroke>::const_iterator last, py::object command);
		~SparseKeymap();
		const py::object* command(vector<const KeyStroke>::const_iterator first, vector<const KeyStroke>::const_iterator last) const;
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
	const py::object* command(vector<const KeyStroke>::const_iterator first, vector<const KeyStroke>::const_iterator last) const;
	bool define(vector<const KeyStroke>::const_iterator first, vector<const KeyStroke>::const_iterator last, py::object command);
	bool find(py::object command, vector<const KeyStroke>& sequence) const;
private:
	KeyTableElement*** planes_[KeyStroke::ALTERNATIVE_KEY << 2];
};

CommandHolder::CommandHolder(py::object command) : command_(command) {
}

const py::object* CommandHolder::command(vector<const KeyStroke>::const_iterator first, vector<const KeyStroke>::const_iterator last) const {
	return (first == last) ? &command_ : &PATH_NOT_FOUND;
}

bool CommandHolder::define(vector<const KeyStroke>::const_iterator first, vector<const KeyStroke>::const_iterator last, py::object command) {
	if(first != last)
		return false;
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

const py::object* SparseKeymap::command(vector<const KeyStroke>::const_iterator first, vector<const KeyStroke>::const_iterator last) const {
	if(first == last)
		return 0;
	map<const KeyStroke, KeyTableElement*>::const_iterator i(map_.find(*first));
	return (i != map_.end()) ? i->second->command(++first, last) : &PATH_NOT_FOUND;
}

bool SparseKeymap::define(vector<const KeyStroke>::const_iterator first, vector<const KeyStroke>::const_iterator last, py::object command) {
	if(first == last)
		return false;
	map<const KeyStroke, KeyTableElement*>::const_iterator i(map_.find(*first));
	if(i == map_.end()) {	// new
		if(command != py::object()) {
			if(first == last - 1)
				map_.insert(make_pair(*first, new CommandHolder(command)));
			else
				map_.insert(make_pair(*first, new SparseKeymap(first + 1, last, command)));
		}
	} else if(command == py::object()) {	// undefine
		delete i->second;
		map_.erase(i);
	} else {	// overwrite
		if(!i->second->define(++first, last, command)) {
			undefine(first, last);
			define(first, last, command);
		}
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


class InputMappingScheme::FirstKeyTable {
public:
	FirstKeyTable();
	~FirstKeyTable();
	void assign(const KeyStroke& keys, py::object command, bool force);
	void* get(const KeyStroke& keys) const {void** p = access(keys, false); return (p != 0) ? *p : 0;}
private:
	void** access(const KeyStroke& keys, bool force) const;
private:
	void*** planes_[KeyStroke::ALTERNATIVE_KEY << 2];
};

InputMappingScheme::VectorKeymap::FirstKeyTable() {
	memset(planes_, 0, sizeof(KeyTableElement***) * MANAH_COUNTOF(planes_));
}

InputMappingScheme::VectorKeymap::~FirstKeyTable() {
	for(KeyTableElement**** plane = planes_; plane < MANAH_ENDOF(planes_); ++plane) {
		for(size_t i = 0; i < 2; ++i) {
			KeyTableElement** half = (*plane)[i];
			for(size_t j = 0; j < 0x80; ++j)
				delete half[j];
			delete[] half;
		}
		delete[] *plane;
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

void InputMappingScheme::FirstKeyTable::assign(const KeyStroke& keys, py::object command, bool force) {
	void** slot = access(keys, true);
	if(*slot == 0) {
		auto_ptr<py::object> p(new py::object());
		*p = command;
		*slot = p.release();
	} else if(force)
		*static_cast<py::object*>(*slot) = command;
}

const py::object* InputMappingScheme::VectorKeymap::command(
		vector<const KeyStroke>::const_iterator first, vector<const KeyStroke>::const_iterator last) const {
	assert(first < last);
	const KeyTableElement* const * const slot = access(*first, false);
}

bool InputMappingScheme::VectorKeymap::define(
		vector<const KeyStroke>::const_iterator first, vector<const KeyStroke>::const_iterator last, py::object command) {
}

bool InputMappingScheme::VectorKeymap::find(py::object command, vector<const KeyStroke>& sequence) const {
}


// InputMappingScheme ///////////////////////////////////////////////////////

InputMappingScheme::InputMappingScheme(const wstring& name) : name_(name), firstKeyTable_(new FirstKeyTable) {
}

InputMappingScheme::~InputMappingScheme() {
	delete firstKeyTable_;
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

py::object InputMappingScheme::command(py::object input) const {
	py::object* const p = static_cast<py::object*>(firstKeyTable_->get(py::extract<const KeyStroke&>(input)));
	return (p != 0) ? *p : py::object();	
}

void InputMappingScheme::define(const py::object input, py::object command, bool force /* = true */) {
	firstKeyTable_->assign(py::extract<const KeyStroke&>(input), command, force);
}

py::object InputMappingScheme::definedInputSequences() const {
	return py::object(py::handle<>(::PyFrozenSet_New(0)));
}

py::object InputMappingScheme::inputSequencesForCommand(py::object command) const {
	py::list s;
	return py::object(py::handle<>(::PyFrozenSet_New(0)));
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
	if(message.message == WM_KEYDOWN || message.message == WM_SYSKEYDOWN) {
		// this interrupts WM_CHARs
		const KeyStroke k(
			static_cast<KeyStroke::ModifierKey>(
				((::GetKeyState(VK_CONTROL) < 0) ? KeyStroke::CONTROL_KEY : 0)
				| ((::GetKeyState(VK_SHIFT) < 0) ? KeyStroke::SHIFT_KEY : 0)
				| ((message.message == WM_SYSKEYDOWN || ::GetKeyState(VK_MENU) < 0) ? KeyStroke::ALTERNATIVE_KEY : 0)),
			static_cast<KeyStroke::VirtualKey>(message.wParam & 0x00ffu));
		py::object command(mappingScheme_.second->command(py::object(py::ptr(&k))));
		if(command != py::object()) {
			ambient::Interpreter::instance().executeCommand(command);
			return true;
		}
	} else if(message.message == WM_SYSCHAR) {
		// interrupt menu activation if key sequence is defined
		const KeyStroke k(
			static_cast<KeyStroke::ModifierKey>(
				((::GetKeyState(VK_CONTROL) < 0) ? KeyStroke::CONTROL_KEY : 0)
				| ((::GetKeyState(VK_SHIFT) < 0) ? KeyStroke::SHIFT_KEY : 0)
				| KeyStroke::ALTERNATIVE_KEY),
			LOBYTE(::VkKeyScanW(static_cast<WCHAR>(message.wParam & 0xffffu))));
		return mappingScheme_.second->command(py::object(py::ptr(&k))) != py::object();
	}
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
	void setAsMappingScheme(py::object s) {InputManager::instance().setMappingScheme(s);}
	void setAsModalMappingScheme(py::object s) {InputManager::instance().setModalMappingScheme(s);}
}

ALPHA_EXPOSE_PROLOGUE(Interpreter::LOWEST_INSTALLATION_ORDER)
	Interpreter& interpreter = Interpreter::instance();
	py::scope temp(interpreter.module("bindings"));

	py::enum_<KeyStroke::VirtualKey>("NaturalKey");
	py::enum_<KeyStroke::ModifierKey>("ModifierKey")
		.value("no_modifier", KeyStroke::NO_MODIFIER)
		.value("control_key", KeyStroke::CONTROL_KEY)
		.value("shift_key", KeyStroke::SHIFT_KEY)
		.value("alternative_key", KeyStroke::ALTERNATIVE_KEY);

	py::class_<KeyStroke>("KeyStroke", py::init<py::object, py::object>())
		.def(py::init<py::object>())
		.add_property("natural_key", &KeyStroke::naturalKey)
		.add_property("modifier_keys", &KeyStroke::modifierKeys);
	py::class_<InputMappingScheme, boost::noncopyable>("InputMappingScheme", py::init<const std::wstring>())
//		.add_property("resolve_parent", &InputMappingScheme::resolveParent, &InputMappingScheme::setResolveParent)
		.def("all_definitions", &InputMappingScheme::allDefinitions)
		.def("bound_commands", &InputMappingScheme::boundCommands)
		.def("command", &InputMappingScheme::command)
		.def("define", &InputMappingScheme::define, (py::arg("input"), py::arg("command"), py::arg("force") = true))
		.def("defined_input_sequences", &InputMappingScheme::definedInputSequences)
		.def("is_locally_defined", &InputMappingScheme::isLocallyDefined)
		.def("reset", &InputMappingScheme::reset)
		.def("set_as_mapping_scheme", &setAsMappingScheme)
		.def("set_as_modal_mapping_scheme", &setAsModalMappingScheme)
		.def("undefine", &InputMappingScheme::undefine);
ALPHA_EXPOSE_EPILOGUE()
