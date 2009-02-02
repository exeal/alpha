/**
 * @file code-pages-dialog.cpp
 * @author exeal
 * @date 2003-2009
 */

#include "alpha.hpp"
#include "code-pages-dialog.hpp"
#include "application.hpp"
#include "../resource/messages.h"
#include <manah/win32/ui/standard-controls.hpp>
using alpha::ui::EncodingsDialog;
using namespace ascension::encoding;
using namespace std;


/**
 * Constructor.
 * @param encoding the encoding initially selected
 * @param forReading set true to enumelate encodings for read files
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
	vector<pair<size_t, const IEncodingProperties*> > encodings;
	Encoder::availableEncodings(back_inserter(encodings));
	const auto_ptr<Encoder> asciiEncoder(Encoder::forMIB(fundamental::US_ASCII));
	assert(asciiEncoder.get() != 0);

	for(vector<pair<size_t, const IEncodingProperties*> >::const_iterator encoding(encodings.begin()), e(encodings.end()); encoding != e; ++encoding) {
		const wstring name(asciiEncoder->toUnicode(encoding->second->displayName(locale::classic())));
		if(!name.empty()) {
			const int item = encodingList_->addString(name.c_str());
			if(item >= 0) {
				encodingList_->setItemData(item, static_cast<DWORD>(encoding->first));
				const string internalName(encoding->second->name());
				if(compareEncodingNames(internalName.begin(), internalName.end(), result_.begin(), result_.end()) == 0)
					encodingList_->setCurSel(item);
			}
		}
	}
	if(forReading_) {
		vector<string> detectors;
		EncodingDetector::availableNames(back_inserter(detectors));
		for(vector<string>::const_iterator detector(detectors.begin()), e(detectors.end()); detector != e; ++detector) {
			const wstring name(asciiEncoder->toUnicode(*detector));
			if(!name.empty()) {
				const int item = encodingList_->addString(name.c_str());
				if(item >= 0) {
					encodingList_->setItemData(item, 0xFFFFFFFFU);
					if(compareEncodingNames(name.begin(), name.end(), result_.begin(), result_.end()) == 0)
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
	if(id != 0xFFFFFFFFU)
		result_ = Encoder::forID(id)->properties().name();
	else if(const int len = encodingList_->getTextLen(item)) {
		manah::AutoBuffer<wchar_t> name(new wchar_t[len + 1]);
		encodingList_->getText(item, name.get());
		result_ = Encoder::forMIB(fundamental::US_ASCII)->fromUnicode(name.get());
	}
}
