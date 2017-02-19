/**
 * @file new-file-format-dialog.cpp
 * @author exeal
 * @date 2003-2009
 */

#include "new-file-format-dialog.hpp"
#include "application.hpp"
#include "../resource/messages.h"
#include <manah/win32/ui/standard-controls.hpp>
using alpha::ui::NewFileFormatDialog;
using manah::win32::ui::ComboBox;
using namespace ascension::encoding;
using namespace ascension::kernel;
using namespace std;


/**
 * Constructor.
 * @param encoding the encoding initially selected
 * @param newline the newline initially selected
 */
NewFileFormatDialog::NewFileFormatDialog(const string& encoding, ascension::kernel::Newline newline) throw() : encoding_(encoding), newline_(newline) {
}

/// @see Dialog#onCommand
bool NewFileFormatDialog::onCommand(WORD id, WORD notifyCode, HWND control) {
	if(id != IDC_COMBO_ENCODING || notifyCode != CBN_SELCHANGE)
		return Dialog::onCommand(id, notifyCode, control);
	const int item = encodingCombobox_.getCurSel();
	if(item == CB_ERR)
		return Dialog::onCommand(id, notifyCode, control);

	const auto_ptr<Encoder> encoder(Encoder::forID(encodingCombobox_.getItemData(item)));
	const MIBenum mib = (encoder.get() != 0) ? encoder->properties().mibEnum() : MIB_UNKNOWN;

	if(mib == standard::UTF_7 || mib == fundamental::UTF_8
			|| mib == fundamental::UTF_16LE || mib == fundamental::UTF_16BE || mib == fundamental::UTF_16
			|| mib == standard::UTF_32 || mib == standard::UTF_32LE || mib == standard::UTF_32BE
			|| encoder->properties().name() == "UTF-5") {
		if(newlineCombobox_.getCount() != 6) {
			const int org = (newlineCombobox_.getCount() != 0) ? newlineCombobox_.getCurSel() : 0;
			newlineCombobox_.resetContent();
			newlineCombobox_.setItemData(newlineCombobox_.addString(localizedString("CR+LF (Windows)")), NLF_CR_LF);
			newlineCombobox_.setItemData(newlineCombobox_.addString(localizedString("LF (Unix)")), NLF_LINE_FEED);
			newlineCombobox_.setItemData(newlineCombobox_.addString(localizedString("CR (Macintosh)")), NLF_CARRIAGE_RETURN);
			newlineCombobox_.setItemData(newlineCombobox_.addString(localizedString("NEL (EBCDIC)")), NLF_NEXT_LINE);
			newlineCombobox_.setItemData(newlineCombobox_.addString(localizedString("LS (U+2028)")), NLF_LINE_SEPARATOR);
			newlineCombobox_.setItemData(newlineCombobox_.addString(localizedString("PS (U+2029)")), NLF_PARAGRAPH_SEPARATOR);
			newlineCombobox_.setCurSel(org);
		}
	} else {
		if(newlineCombobox_.getCount() != 3) {
			const int org = (newlineCombobox_.getCount() != 0) ? newlineCombobox_.getCurSel() : 0;
			newlineCombobox_.resetContent();
			newlineCombobox_.setItemData(newlineCombobox_.addString(localizedString("CR+LF (Windows)")), NLF_CR_LF);
			newlineCombobox_.setItemData(newlineCombobox_.addString(localizedString("LF (Unix)")), NLF_LINE_FEED);
			newlineCombobox_.setItemData(newlineCombobox_.addString(localizedString("CR (Macintosh)")), NLF_CARRIAGE_RETURN);
			newlineCombobox_.setCurSel((org < newlineCombobox_.getCount()) ? org : 0);
		}
	}
	return true;
}

/// @see Dialog#onInitDialog
void NewFileFormatDialog::onInitDialog(HWND focusWindow, bool&) {
	// "Encoding"
	vector<pair<size_t, const IEncodingProperties*> > encodings;
	Encoder::availableEncodings(back_inserter(encodings));
	const auto_ptr<Encoder> asciiEncoder(Encoder::forMIB(fundamental::US_ASCII));
	for(vector<pair<size_t, const IEncodingProperties*> >::const_iterator encoding(encodings.begin()), e(encodings.end()); encoding != e; ++encoding) {
		const wstring name(asciiEncoder->toUnicode(encoding->second->displayName(locale::classic())));
		if(!name.empty()) {
			const int item = encodingCombobox_.addString(name.c_str());
			if(item >= 0) {
				encodingCombobox_.setItemData(item, static_cast<DWORD>(encoding->first));
				const string internalName(encoding->second->name());
				if(compareEncodingNames(internalName.begin(), internalName.end(), encoding_.begin(), encoding_.end()) == 0)
					encodingCombobox_.setCurSel(item);
			}
		}
	}
	if(encodingCombobox_.getCurSel() == CB_ERR)
		encodingCombobox_.setCurSel(0);

	// "Newline"
	onCommand(IDC_COMBO_ENCODING, CBN_SELCHANGE, 0);
	for(int i = 0; i < 6; ++i) {
		if(newline_ == static_cast<ascension::kernel::Newline>(newlineCombobox_.getItemData(i))) {
			newlineCombobox_.setCurSel(i);
			break;
		}
	}

//	// [ï∂èëÉ^ÉCÉv]
//	for(list<wstring>::const_iterator it = documentTypes_.begin(); it != documentTypes_.end(); ++it)
//		documentTypeCombobox_.addString(it->c_str());
//	documentTypeCombobox_.setCurSel(static_cast<int>(documentType_));
}

/// @see Dialog#onOK
void NewFileFormatDialog::onOK(bool&) {
	encoding_ = Encoder::forID(encodingCombobox_.getItemData(encodingCombobox_.getCurSel()))->properties().name();
	newline_ = static_cast<ascension::kernel::Newline>(newlineCombobox_.getItemData(newlineCombobox_.getCurSel()));
//	documentType_ = documentTypeCombobox_.getCurSel();
}
