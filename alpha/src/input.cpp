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


// KeyCombination ///////////////////////////////////////////////////////////

KeyCombination::KeyCombination(VirtualKey key, ModifierKey modifiers /* = NO_MODIFIER */) : vkey_(key), modifiers_(modifiers) {
}

KeyCombination::VirtualKey KeyCombination::keyCode() const /*throw()*/ {
	return vkey_;
}

KeyCombination::ModifierKey KeyCombination::modifiers() const /*throw()*/ {
	return modifiers_;
}


// InputMap.FirstKeyTable ///////////////////////////////////////////////////

class InputMap::FirstKeyTable {
public:
	FirstKeyTable();
	~FirstKeyTable();
	void assign(const KeyCombination& keys, py::object command, bool force);
	void* get(const KeyCombination& keys) const {return access(keys, false);}
private:
	void** access(const KeyCombination& keys, bool force) const;
private:
	void*** planes_[KeyCombination::ALTERNATIVE_KEY << 2];
};

void** InputMap::FirstKeyTable::access(const KeyCombination& keys, bool force) const {
	void*** const& plane = planes_[keys.modifiers()];
	if(plane == 0) {
		if(!force)
			return 0;
		memset(const_cast<void***&>(plane) = new void**[2], 0, sizeof(void**) * 2);
	}
	void** const& half = plane[(keys.keyCode() & 0x80) >> 7];
	if(half == 0) {
		if(!force)
			return 0;
		memset(const_cast<void**&>(half) = new void*[0x80], 0, sizeof(void*) * 0x80);
	}
	return &half[keys.keyCode() & 0x7f];
}

void InputMap::FirstKeyTable::assign(const KeyCombination& keys, py::object command, bool force) {
	void** slot = access(keys, true);
	if(*slot == 0) {
		auto_ptr<py::object> p(new py::object());
		*p = command;
		*slot = p.release();
	} else if(force)
		*static_cast<py::object*>(*slot) = command;
}


// InputMap /////////////////////////////////////////////////////////////////

InputMap::InputMap() : firstKeyTable_(new FirstKeyTable) {
}

InputMap::~InputMap() {
	delete firstKeyTable_;
}

py::object** InputMap::access(py::object input) const {
//	return static_cast<py::object*>(firstKeyTable_->get(py::extract<const KeyCombination&>(input)));
	return 0;
}

py::dict InputMap::allDefinitions() const {
	return py::dict();
}

py::object InputMap::boundCommands() const {
	return py::object(py::handle<>(::PyFrozenSet_New(0)));
}

py::object InputMap::command(py::object input) const {
	return *static_cast<py::object*>(firstKeyTable_->get(py::extract<const KeyCombination&>(input)));	
}

void InputMap::define(const py::object input, py::object command, bool force /* = true */) {
	firstKeyTable_->assign(py::extract<const KeyCombination&>(input), command, force);
}

py::object InputMap::definedInputStrokes() const {
	return py::object(py::handle<>(::PyFrozenSet_New(0)));
}

py::object InputMap::inputStrokesForCommand(py::object command) const {
	return py::object(py::handle<>(::PyFrozenSet_New(0)));
}

bool InputMap::isLocallyDefined(py::object input) const {
	return access(input) != 0;
}

void InputMap::reset() {
}

py::object InputMap::resolveParent() const {
	return resolveParent_;
}

void InputMap::setResolveParent(py::object parent) {
	if(!py::extract<const InputMap&>(parent).check()) {
		::PyErr_BadArgument();
		py::throw_error_already_set();
	}
	// TODO: detect cyclic relation.
	resolveParent_ = parent;
}

void InputMap::undefine(py::object input) {
}


ALPHA_EXPOSE_PROLOGUE(Interpreter::LOWEST_INSTALLATION_ORDER)
	Interpreter& interpreter = Interpreter::instance();
	py::scope temp(interpreter.toplevelPackage());

	py::enum_<KeyCombination::VirtualKey>("VirtualKey");
	py::enum_<KeyCombination::ModifierKey>("ModifierKey")
		.value("no_modifier", KeyCombination::NO_MODIFIER)
		.value("control_key", KeyCombination::CONTROL_KEY)
		.value("shift_key", KeyCombination::SHIFT_KEY)
		.value("alternative_key", KeyCombination::ALTERNATIVE_KEY);

	py::class_<KeyCombination>("KeyCombination", py::init<KeyCombination::VirtualKey, KeyCombination::ModifierKey>())
		.add_property("key_code", &KeyCombination::keyCode)
		.add_property("modifiers", &KeyCombination::modifiers);
	py::class_<InputMap>("InputMap")
		.add_property("resolve_parent", &InputMap::resolveParent, &InputMap::setResolveParent)
		.def("all_definitions", &InputMap::allDefinitions)
		.def("bound_commands", &InputMap::boundCommands)
		.def("command", &InputMap::command)
		.def("define", &InputMap::define, (py::arg("input"), py::arg("command"), py::arg("force") = true))
		.def("defined_input_strokes", &InputMap::definedInputStrokes)
		.def("is_locally_defined", &InputMap::isLocallyDefined)
		.def("reset", &InputMap::reset)
		.def("undefine", &InputMap::undefine);
ALPHA_EXPOSE_EPILOGUE()
