/**
 * @file encodings-dialog.cpp
 * Implements and exposes @c encodings_dialog function to Python.
 * @author exeal
 * @date 2003-2009 (was code-pages-dialog.hpp and code-pages-dialog.cpp)
 * @date 2009
 */

#include "application.hpp"
#include "../resource/messages.h"
#include <manah/win32/ui/dialog.hpp>
#include <manah/win32/ui/standard-controls.hpp>
using namespace alpha;
using namespace std;
namespace e = ascension::encoding;
namespace py = boost::python;

namespace {
	/// "Select Encoding" dialog box.
	class EncodingsDialog : public manah::win32::ui::FixedIDDialog<IDD_DLG_CODEPAGES> {
	public:
		EncodingsDialog(const string& encoding, bool forReading) /*throw()*/;
		const string& resultEncoding() const /*throw()*/;
	private:
		string result_;
		const bool forReading_;
		manah::win32::Borrowed<manah::win32::ui::ListBox> encodingList_;
		MANAH_BEGIN_CONTROL_BINDING()
			MANAH_BIND_CONTROL(IDC_LIST_CODEPAGES, encodingList_)
		MANAH_END_CONTROL_BINDING()
		bool onCommand(WORD id, WORD notifyCode, HWND control);		// WM_COMMAND
		void onInitDialog(HWND focusWindow, bool& focusDefault);	// WM_INITDIALOG
		void onOK(bool& continueDialog);							// IDOK
	};
}


/**
 * Constructor.
 * @param encoding the encoding initially selected
 * @param forReading set @c true to enumelate encodings for read files
 */
EncodingsDialog::EncodingsDialog(const string& encoding, bool forReading) throw() : result_(encoding), forReading_(forReading) {
}

/// @see Dialog#onCommand
bool EncodingsDialog::onCommand(WORD id, WORD notifyCode, HWND control) {
	if(id == IDC_LIST_CODEPAGES && notifyCode == LBN_DBLCLK) {
		postMessage(WM_COMMAND, IDOK);
		return true;
	}
	return Dialog::onCommand(id, notifyCode, control);
}

/// @see Dialog#onInitDialog
void EncodingsDialog::onInitDialog(HWND focusWindow, bool&) {
	vector<pair<size_t, const e::IEncodingProperties*> > encodings;
	e::Encoder::availableEncodings(back_inserter(encodings));
	const auto_ptr<e::Encoder> asciiEncoder(e::Encoder::forMIB(e::fundamental::US_ASCII));
	assert(asciiEncoder.get() != 0);

	for(vector<pair<size_t, const e::IEncodingProperties*> >::const_iterator encoding(encodings.begin()), e(encodings.end()); encoding != e; ++encoding) {
		const wstring name(asciiEncoder->toUnicode(encoding->second->displayName(locale::classic())));
		if(!name.empty()) {
			const int item = encodingList_->addString(name.c_str());
			if(item >= 0) {
				encodingList_->setItemData(item, static_cast<DWORD>(encoding->first));
				const string internalName(encoding->second->name());
				if(e::compareEncodingNames(internalName.begin(), internalName.end(), result_.begin(), result_.end()) == 0)
					encodingList_->setCurSel(item);
			}
		}
	}
	if(forReading_) {
		vector<string> detectors;
		e::EncodingDetector::availableNames(back_inserter(detectors));
		for(vector<string>::const_iterator detector(detectors.begin()), e(detectors.end()); detector != e; ++detector) {
			const wstring name(asciiEncoder->toUnicode(*detector));
			if(!name.empty()) {
				const int item = encodingList_->addString(name.c_str());
				if(item >= 0) {
					encodingList_->setItemData(item, 0xffffffffu);
					if(e::compareEncodingNames(name.begin(), name.end(), result_.begin(), result_.end()) == 0)
						encodingList_->setCurSel(item);
				}
			}
		}
	}

	encodingList_->setCurSel((encodingList_->getCurSel() != LB_ERR) ? encodingList_->getCurSel() : 0);
}

/// @see Dialog#onOK
void EncodingsDialog::onOK(bool&) {
	const int item = encodingList_->getCurSel();
	const DWORD id = encodingList_->getItemData(item);
	if(id != 0xffffffffu)
		result_ = e::Encoder::forID(id)->properties().name();
	else if(const int len = encodingList_->getTextLen(item)) {
		manah::AutoBuffer<wchar_t> name(new wchar_t[len + 1]);
		encodingList_->getText(item, name.get());
		result_ = e::Encoder::forMIB(e::fundamental::US_ASCII)->fromUnicode(name.get());
	}
}

/// Returns the selected encoding.
inline const string& EncodingsDialog::resultEncoding() const {
	return result_;
}

namespace {
	string encodingsDialog(const string& encoding, bool forReading) {
		EncodingsDialog dialog(encoding, forReading);
		dialog.doModal(Alpha::instance().getMainWindow());
		return dialog.resultEncoding();
	}
}

ALPHA_EXPOSE_PROLOGUE(ambient::Interpreter::LOWEST_INSTALLATION_ORDER)
	py::scope temp(ambient::Interpreter::instance().module("ui"));
	py::def("encodings_dialog", &encodingsDialog);
ALPHA_EXPOSE_EPILOGUE()
