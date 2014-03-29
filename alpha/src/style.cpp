/**
 * @file style.cpp
 * @author exeal
 * @date 2010
 */

#include "ambient.hpp"
#include <ascension/presentation.hpp>

using namespace alpha::ambient;
namespace py = boost::python;
namespace pr = ascension::presentation;

ALPHA_EXPOSE_PROLOGUE(Interpreter::LOWEST_INSTALLATION_ORDER)
	py::scope temp(Interpreter::instance().module("presentation"));

	py::enum_<pr::UnderlineStyle>("UnderLine")
		.value("none", pr::NO_UNDERLINE)
		.value("solid", pr::SOLID_UNDERLINE)
		.value("dashed", pr::DASHED_UNDERLINE)
		.value("dotted", pr::DOTTED_UNDERLINE);
	py::enum_<pr::BorderStyle>("BorderLine")
		.value("none", pr::NO_BORDER)
		.value("solid", pr::SOLID_BORDER)
		.value("dashed", pr::DASHED_BORDER)
		.value("dotted", pr::DOTTED_BORDER);

	py::class_<pr::Color>("Color", py::init<>())
		.def(py::init<ascension::byte, ascension::byte, ascension::byte>())
		.add_property("blue", &pr::Color::blue)
		.add_property("green", &pr::Color::green)
		.add_property("red", &pr::Color::red);
	py::class_<pr::Colors>("Colors", py::init<>())
		.def(py::init<const pr::Color&, const pr::Color&>())
		.def_readwrite("background", &pr::Colors::background)
		.def_readwrite("foreground", &pr::Colors::foreground);
	py::class_<pr::TextStyle>("TextStyle",
		py::init<const pr::Colors, bool, bool, bool, pr::UnderlineStyle, pr::Color, pr::BorderStyle, pr::Color>((
			py::arg("color") = pr::Colors(), py::arg("bold") = false, py::arg("italic") = false, py::arg("strikeout") = false,
			py::arg("underline_style") = pr::NO_UNDERLINE, py::arg("underline_color") = pr::Color(),
			py::arg("border_style") = pr::NO_BORDER, py::arg("border_color") = pr::Color())))
		.def_readwrite("color", &pr::TextStyle::color)
		.def_readwrite("bold", &pr::TextStyle::bold)
		.def_readwrite("italic", &pr::TextStyle::italic)
		.def_readwrite("strikeout", &pr::TextStyle::strikeout)
		.def_readwrite("underline_style", &pr::TextStyle::underlineStyle)
		.def_readwrite("underline_color", &pr::TextStyle::underlineColor)
		.def_readwrite("border_style", &pr::TextStyle::borderStyle)
		.def_readwrite("border_color", &pr::TextStyle::borderColor);
ALPHA_EXPOSE_EPILOGUE()
