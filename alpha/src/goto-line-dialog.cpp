/**
 * @file goto-line-dialog.cpp
 * Exposes @c goto_line_dialog function to Python.
 * @author exeal
 * @date 2003-2009
 */

#include "application.hpp"
#include "editor-window.hpp"
#include "resource.h"
#include "../resource/messages.h"
#include <manah/win32/ui/dialog.hpp>

using namespace alpha;
using namespace ascension;
using namespace std;
namespace py = boost::python;

namespace {
	/// "Go To Line" dialog box.
	class GotoLineDialog : public manah::win32::ui::FixedIDDialog<IDD_DLG_GOTOLINE> {
	private:
		void onInitDialog(HWND, bool&);
		void onOK(bool& continueDialog);
	private:
		manah::win32::ui::UpDownCtrl lineNumberSpin_;
		MANAH_BEGIN_CONTROL_BINDING()
			MANAH_BIND_CONTROL(IDC_SPIN_LINENUMBER, lineNumberSpin_)
		MANAH_END_CONTROL_BINDING()
	};
}


/// @see Dialog#onInitDialog
void GotoLineDialog::onInitDialog(HWND, bool&) {
	Alpha& app = Alpha::instance();
	const Buffer& buffer = EditorWindows::instance().selectedBuffer();
	const EditorView& viewer = EditorWindows::instance().activePane().visibleView();
	const length_t lineOffset = viewer.verticalRulerConfiguration().lineNumbers.startValue;
	const wstring s = app.loadMessage(MSG_DIALOG__LINE_NUMBER_RANGE, MARGS
						% static_cast<ulong>(kernel::line(buffer.accessibleRegion().first) + lineOffset)
						% static_cast<ulong>(kernel::line(buffer.accessibleRegion().second) + lineOffset));

	setItemText(IDC_STATIC_1, s.c_str());
	lineNumberSpin_.setRange(
		static_cast<int>(kernel::line(buffer.accessibleRegion().first) + lineOffset),
		static_cast<int>(kernel::line(buffer.accessibleRegion().second) + lineOffset));
	lineNumberSpin_.setPosition(static_cast<int>(viewer.caret().line() + lineOffset));
	lineNumberSpin_.invalidateRect(0);

	checkButton(IDC_CHK_SAVESELECTION,
		app.readIntegerProfile(L"Search", L"GotoLineDialog.extendSelection", 0) == 1 ? BST_CHECKED : BST_UNCHECKED);
}

/// @see Dialog#onOK()
void GotoLineDialog::onOK(bool& continueDialog) {
	Alpha& app = Alpha::instance();
/*	// can't perform when temporary macro is defining
	if(app.commandManager().temporaryMacro().state() == command::TemporaryMacro::DEFINING) {
		app.messageBox(MSG_ERROR__PROHIBITED_FOR_MACRO_DEFINING, MB_ICONEXCLAMATION);
		continueDialog = true;
		return;
	}
*/
	EditorView& viewer = EditorWindows::instance().activePane().visibleView();
	length_t line = lineNumberSpin_.getPosition();
	line -= viewer.verticalRulerConfiguration().lineNumbers.startValue;

	// jump
	if(isButtonChecked(IDC_CHK_SAVESELECTION) == BST_CHECKED) {
		viewer.caret().extendSelection(kernel::Position(line, 0));
		app.writeIntegerProfile(L"Search", L"GotoLineDialog.extendSelection", 1);
	} else {
		viewer.caret().moveTo(kernel::Position(line, 0));
		app.writeIntegerProfile(L"Search", L"GotoLineDialog.extendSelection", 0);
	}
}

namespace {
	void gotoLineDialog() {
		static GotoLineDialog dialog;
		dialog.doModeless(Alpha::instance().getMainWindow(), !dialog.isWindow() || !dialog.isVisible());
	}
}

ALPHA_EXPOSE_PROLOGUE(ambient::Interpreter::LOWEST_INSTALLATION_ORDER)
	py::scope temp(ambient::Interpreter::instance().module("ui"));
	py::def("goto_line_dialog", &gotoLineDialog);
ALPHA_EXPOSE_EPILOGUE()
