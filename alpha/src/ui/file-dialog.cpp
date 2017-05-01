
#include "application.hpp"
#include "ambient.hpp"
#include "../resource/messages.h"
#include "editor-window.hpp"
#include <manah/win32/ui/standard-controls.hpp>
#include <commdlg.h>	// GetOpenFileNameW, 
#include <shlwapi.h>	// PathFindFileNameW
#include <dlgs.h>

using namespace alpha;
using namespace manah;
using namespace std;
namespace k = ascension::kernel;
namespace e = ascension::encoding;
namespace py = boost::python;

namespace {
	/// Hook procedure for @c GetOpenFileNameW and @c GetSaveFileNameW.
	UINT_PTR CALLBACK openFileNameHookProc(HWND window, UINT message, WPARAM wp, LPARAM lp) {
		switch(message) {
		case WM_COMMAND:
			// changed "Encoding" => change "Newlines" list whether the encoding is Unicode
			if(LOWORD(wp) == IDC_COMBO_ENCODING && HIWORD(wp) == CBN_SELCHANGE) {
				win32::ui::ComboBox newlineCombobox(win32::borrowed(::GetDlgItem(window, IDC_COMBO_NEWLINE)));
				if(!newlineCombobox.isWindow())
					break;
				win32::ui::ComboBox encodingCombobox(win32::borrowed(::GetDlgItem(window, IDC_COMBO_ENCODING)));
				const wstring keepNLF = Alpha::instance().loadMessage(MSG_DIALOG__KEEP_NEWLINE);
				const auto_ptr<e::Encoder> encoder(e::Encoder::forID(encodingCombobox.getItemData(encodingCombobox.getCurSel())));
				const e::MIBenum encoding = (encoder.get() != 0) ? encoder->properties().mibEnum() : e::MIB_UNKNOWN;
				const int newline = (newlineCombobox.getCount() != 0) ? newlineCombobox.getCurSel() : 0;

				// TODO:
				if(/*encoding == minority::UTF_5 ||*/ encoding == e::standard::UTF_7
						|| encoding == e::fundamental::UTF_8
						|| encoding == e::fundamental::UTF_16LE || encoding == e::fundamental::UTF_16BE || encoding == e::fundamental::UTF_16
						|| encoding == e::standard::UTF_32 || encoding == e::standard::UTF_32LE || encoding == e::standard::UTF_32BE) {
					if(newlineCombobox.getCount() != 7) {
						newlineCombobox.resetContent();
						newlineCombobox.setItemData(newlineCombobox.addString(keepNLF.c_str()), k::NLF_RAW_VALUE);
						newlineCombobox.setItemData(newlineCombobox.addString(localizedString("CR+LF (Windows)")), k::NLF_CR_LF);
						newlineCombobox.setItemData(newlineCombobox.addString(localizedString("LF (Unix)")), k::NLF_LINE_FEED);
						newlineCombobox.setItemData(newlineCombobox.addString(localizedString("CR (Macintosh)")), k::NLF_CARRIAGE_RETURN);
						newlineCombobox.setItemData(newlineCombobox.addString(localizedString("NEL (EBCDIC)")), k::NLF_NEXT_LINE);
						newlineCombobox.setItemData(newlineCombobox.addString(localizedString("LS (U+2028)")), k::NLF_LINE_SEPARATOR);
						newlineCombobox.setItemData(newlineCombobox.addString(localizedString("PS (U+2029)")), k::NLF_PARAGRAPH_SEPARATOR);
						newlineCombobox.setCurSel(newline);
					}
				} else {
					if(newlineCombobox.getCount() != 4) {
						newlineCombobox.resetContent();
						newlineCombobox.setItemData(newlineCombobox.addString(keepNLF.c_str()), k::NLF_RAW_VALUE);
						newlineCombobox.setItemData(newlineCombobox.addString(localizedString("CR+LF (Windows)")), k::NLF_CR_LF);
						newlineCombobox.setItemData(newlineCombobox.addString(localizedString("LF (Unix)")), k::NLF_LINE_FEED);
						newlineCombobox.setItemData(newlineCombobox.addString(localizedString("CR (Macintosh)")), k::NLF_CARRIAGE_RETURN);
						newlineCombobox.setCurSel((newline < 4) ? newline : 0);
					}
				}
			}
			break;
		case WM_INITDIALOG: {
			OPENFILENAMEW& ofn = *reinterpret_cast<OPENFILENAMEW*>(lp);
			HWND dialog = ::GetParent(window);
			win32::ui::ComboBox encodingCombobox(win32::borrowed(::GetDlgItem(window, IDC_COMBO_ENCODING)));
			win32::ui::Static encodingLabel(win32::borrowed(::GetDlgItem(window, IDC_STATIC_1)));
			win32::ui::ComboBox newlineCombobox(win32::borrowed(::GetDlgItem(window, IDC_COMBO_NEWLINE)));
			win32::ui::Static newlineLabel(win32::borrowed(::GetDlgItem(window, IDC_STATIC_2)));
			HFONT guiFont = reinterpret_cast<HFONT>(::SendMessageW(dialog, WM_GETFONT, 0, 0L));

			// ダイアログテンプレートのコントロールの位置合わせなど
			POINT pt;
			RECT rect;
			::GetWindowRect(window, &rect);
			pt.x = rect.left;
			pt.y = rect.top;

			// labels
			::GetWindowRect(::GetDlgItem(dialog, stc2), &rect);
			long x = rect.left;
			encodingLabel.getRect(rect);
			encodingLabel.setPosition(0, x - pt.x, rect.top - pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
			encodingLabel.setFont(guiFont);
			if(newlineLabel.isWindow()) {
				newlineLabel.getRect(rect);
				newlineLabel.setPosition(0, x - pt.x, rect.top - pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
				newlineLabel.setFont(guiFont);
			}

			// comboboxes
			::GetWindowRect(::GetDlgItem(dialog, cmb1), &rect);
			x = rect.left;
			encodingCombobox.getRect(rect);
			encodingCombobox.setPosition(0, x - pt.x, rect.top - pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
			encodingCombobox.setFont(guiFont);
			if(newlineCombobox.isWindow()) {
				newlineCombobox.getRect(rect);
				newlineCombobox.setPosition(0, x - pt.x, rect.top - pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
				newlineCombobox.setFont(guiFont);
			}

			const pair<string, k::Newline>& format = *reinterpret_cast<pair<string, k::Newline>*>(ofn.lCustData);
			const auto_ptr<e::Encoder> asciiEncoder(e::Encoder::forMIB(e::fundamental::US_ASCII));
			assert(asciiEncoder.get() != 0);

			vector<pair<size_t, const e::IEncodingProperties*> > encodings;
			e::Encoder::availableEncodings(back_inserter(encodings));
			for(vector<pair<size_t, const e::IEncodingProperties*> >::const_iterator encoding(encodings.begin()), e(encodings.end()); encoding != e; ++encoding) {
				const wstring name(asciiEncoder->toUnicode(encoding->second->displayName(locale::classic())));
				if(!name.empty()) {
					const int item = encodingCombobox.addString(name.c_str());
					if(item >= 0) {
						encodingCombobox.setItemData(item, static_cast<DWORD>(encoding->first));
						const string internalName(encoding->second->name());
						if(e::compareEncodingNames(internalName.begin(), internalName.end(), format.first.begin(), format.first.end()) == 0)
							encodingCombobox.setCurSel(item);
					}
				}
			}

			if(!newlineCombobox.isWindow()) {
				vector<string> detectors;
				e::EncodingDetector::availableNames(back_inserter(detectors));
				for(vector<string>::const_iterator detector(detectors.begin()), e(detectors.end()); detector != e; ++detector) {
					const wstring name(asciiEncoder->toUnicode(*detector));
					if(!name.empty()) {
						const int item = encodingCombobox.addString(name.c_str());
						encodingCombobox.setItemData(item, 0xffffffffu);
						if(e::compareEncodingNames(name.begin(), name.end(), format.first.begin(), format.first.end()) == 0)
							encodingCombobox.setCurSel(item);
					}
				}
			}

			if(encodingCombobox.getCurSel() == CB_ERR)
				encodingCombobox.setCurSel(0);

			if(newlineCombobox.isWindow()) {
				switch(format.second) {
				case k::NLF_RAW_VALUE:				newlineCombobox.setCurSel(0);	break;
				case k::NLF_CR_LF:					newlineCombobox.setCurSel(1);	break;
				case k::NLF_LINE_FEED:				newlineCombobox.setCurSel(2);	break;
				case k::NLF_CARRIAGE_RETURN:		newlineCombobox.setCurSel(3);	break;
				case k::NLF_NEXT_LINE:				newlineCombobox.setCurSel(4);	break;
				case k::NLF_LINE_SEPARATOR:			newlineCombobox.setCurSel(5);	break;
				case k::NLF_PARAGRAPH_SEPARATOR:	newlineCombobox.setCurSel(6);	break;
				}
				::SendMessageW(window, WM_COMMAND, MAKEWPARAM(IDC_COMBO_ENCODING, CBN_SELCHANGE), 0);
			}
		}
			break;
		case WM_NOTIFY: {
			OFNOTIFYW& ofn = *reinterpret_cast<OFNOTIFYW*>(lp);
			if(ofn.hdr.code == CDN_FILEOK) {	// "Open" or "Save"
				win32::ui::ComboBox encodingCombobox(win32::borrowed(::GetDlgItem(window, IDC_COMBO_ENCODING)));
				win32::ui::ComboBox newlineCombobox(win32::borrowed(::GetDlgItem(window, IDC_COMBO_NEWLINE)));
				win32::ui::Button readOnlyCheckbox(win32::borrowed(::GetDlgItem(::GetParent(window), chx1)));
				pair<string, k::Newline>& format = *reinterpret_cast<pair<string, k::Newline>*>(ofn.lpOFN->lCustData);

				format.first.erase();
				const int encodingCurSel = encodingCombobox.getCurSel();
				if(encodingCurSel != CB_ERR) {
					const DWORD id = encodingCombobox.getItemData(encodingCurSel);
					if(id != 0xffffffffu)
						format.first = e::Encoder::forID(id)->properties().name();
				}
				if(format.first.empty()) {
					const wstring encodingName(encodingCombobox.getText());
					format.first = e::Encoder::forMIB(e::fundamental::US_ASCII)->fromUnicode(encodingName);
				}
				if(!e::Encoder::supports(format.first) && e::EncodingDetector::forName(format.first) == 0) {
					// reject for invalid encoding name
					Alpha::instance().messageBox(MSG_IO__UNSUPPORTED_ENCODING, MB_OK | MB_ICONEXCLAMATION);
					ascension::win32::setWindowLong(window, DWLP_MSGRESULT, true);
					return true;
				}
				if(newlineCombobox.isWindow()) {
					switch(newlineCombobox.getCurSel()) {
					case 0:	format.second = k::NLF_RAW_VALUE;			break;
					case 1:	format.second = k::NLF_CR_LF;				break;
					case 2:	format.second = k::NLF_LINE_FEED;			break;
					case 3:	format.second = k::NLF_CARRIAGE_RETURN;		break;
					case 4:	format.second = k::NLF_NEXT_LINE;			break;
					case 5:	format.second = k::NLF_LINE_SEPARATOR;		break;
					case 6:	format.second = k::NLF_PARAGRAPH_SEPARATOR;	break;
					}
				}
				if(readOnlyCheckbox.isWindow()) {
					// 複数ファイルの場合、チェックボックスの状態が無視される
					// (意図的かもしれない)
					if(readOnlyCheckbox.getCheck() == BST_CHECKED)
						ofn.lpOFN->Flags |= OFN_READONLY;
					else
						ofn.lpOFN->Flags &= ~OFN_READONLY;
				}
			}
		}
			break;
		}

		return 0l;
	}

	py::list openFileDialog(const basic_string<WCHAR>& initialDirectory, py::tuple filters) {
		// convert the filter string
		basic_stringbuf<WCHAR> filterString;
		for(py::ssize_t i = 0, c = py::len(filters); i < c; ++i) {
			const py::tuple filter(filters[i]); 
			const basic_string<WCHAR> caption = py::extract<basic_string<WCHAR> >(filter[0]);
			const basic_string<WCHAR> pattern = py::extract<basic_string<WCHAR> >(filter[1]);
			filterString.sputn(caption.c_str(), static_cast<streamsize>(caption.length() + 1));
			filterString.sputn(pattern.c_str(), static_cast<streamsize>(pattern.length() + 1));
		}
		filterString.sputc(L'\0');

		AutoBuffer<WCHAR> activeBufferDirectory;
		if(initialDirectory.empty() && EditorWindows::instance().selectedBuffer().textFile().isBoundToFile()) {
			// use the directory of the active buffer
			activeBufferDirectory.reset(new WCHAR[MAX_PATH]);
			wcscpy(activeBufferDirectory.get(), EditorWindows::instance().selectedBuffer().textFile().fileName().c_str());
			*::PathFindFileNameW(activeBufferDirectory.get()) = 0;
			if(activeBufferDirectory[0] == 0)
				activeBufferDirectory.reset();
		}

		// setup OSVERSIONINFOW structure
		win32::AutoZero<OSVERSIONINFOW> osVersion;
		osVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
		::GetVersionExW(&osVersion);
		WCHAR fileName[MAX_PATH] = L"";
		pair<string, k::Newline> format(make_pair(e::Encoder::defaultInstance().properties().name(), k::NLF_RAW_VALUE));
		win32::AutoZeroSize<OPENFILENAMEW> newOfn;
		win32::AutoZeroSize<OPENFILENAME_NT4W> oldOfn;
		OPENFILENAMEW& ofn = (osVersion.dwMajorVersion > 4) ? newOfn : *reinterpret_cast<OPENFILENAMEW*>(&oldOfn);
		ofn.hwndOwner = Alpha::instance().getMainWindow().get();
		ofn.hInstance = ::GetModuleHandle(0);
		ofn.lpstrFilter = filterString.str().c_str();
		ofn.lpstrFile = fileName;
		ofn.lpstrInitialDir = !initialDirectory.empty() ? initialDirectory.c_str() : activeBufferDirectory.get();
//		ofn.nFilterIndex = app.readIntegerProfile(L"File", L"activeFilter", 0);
		ofn.nMaxFile = MAX_PATH;
		ofn.Flags = OFN_ALLOWMULTISELECT | OFN_ENABLEHOOK | OFN_ENABLESIZING | OFN_ENABLETEMPLATE
			| OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST/* | OFN_SHOWHELP*/;
		ofn.lCustData = reinterpret_cast<LPARAM>(&format);
		ofn.lpfnHook = openFileNameHookProc;
		ofn.lpTemplateName = MAKEINTRESOURCEW(IDD_DLG_OPENFILE);

		// show dialog box
		if(!toBoolean(::GetOpenFileNameW(&ofn))) {
			const DWORD e = ::CommDlgExtendedError();
			if(e == 0)
				return py::list();	// the user canceled or closed the dialog box
			::PyErr_Format(PyExc_WindowsError, "GetOpenFileNameW failed and CommDlgExtendedError returned %u.", e);
			py::throw_error_already_set();
		}
//		app.writeIntegerProfile(L"File", L"activeFilter", ofn.nFilterIndex);	// save the used filter

		// make the result list
		const basic_string<WCHAR> directory(ofn.lpstrFile);
		py::list result;
		if(directory.length() > ofn.nFileOffset) {	// the number of files to open is 1
			// ofn.lpstrFile is full name
			py::dict temp;
			temp["filename"] = directory;
			temp["encoding"] = format.first;
			temp["read_only"] = toBoolean(ofn.Flags & OFN_READONLY);
			result.append(temp);
		} else {	// open multiple files
			for(WCHAR* fileNames = ofn.lpstrFile + ofn.nFileOffset; *fileNames != 0; ) {
				const size_t len = wcslen(fileNames);
				py::dict temp;
				temp["filename"] = directory + L"\\" + basic_string<WCHAR>(fileNames, len);
				temp["encoding"] = format.first;
				temp["read_only"] = toBoolean(ofn.Flags & OFN_READONLY);
				result.append(temp);
				fileNames += len + 1;
			}
		}
		return result;
	}
	py::object saveFileDialog(const basic_string<WCHAR>& fileName, const string& encoding, k::Newline newline) {
		alpha::Alpha& app = alpha::Alpha::instance();
		win32::AutoZero<OSVERSIONINFOW> osVersion;
		const wstring filterSource(app.loadMessage(MSG_DIALOG__SAVE_FILE_FILTER));
		AutoBuffer<WCHAR> filter(new WCHAR[filterSource.length() + 6]);

		osVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
		::GetVersionExW(&osVersion);
		filterSource.copy(filter.get(), filterSource.length());
		wcsncpy(filter.get() + filterSource.length(), L"\0*.*\0\0", 6);
		AutoBuffer<WCHAR> pathName(new WCHAR[max<size_t>(fileName.length() + 1, MAX_PATH)]);
		wcscpy(pathName.get(), fileName.c_str());

		pair<string, k::Newline> format(make_pair(encoding, newline));
		win32::AutoZeroSize<OPENFILENAMEW> newOfn;
		win32::AutoZeroSize<OPENFILENAME_NT4W> oldOfn;
		OPENFILENAMEW& ofn = (osVersion.dwMajorVersion > 4) ? newOfn : *reinterpret_cast<OPENFILENAMEW*>(&oldOfn);
		ofn.hwndOwner = app.getMainWindow().get();
		ofn.hInstance = ::GetModuleHandle(0);
		ofn.lpstrFilter = filter.get();
		ofn.lpstrFile = pathName.get();
		ofn.nMaxFile = max(static_cast<DWORD>(fileName.length() + 1), static_cast<DWORD>(MAX_PATH));
		ofn.Flags = OFN_ENABLEHOOK | OFN_ENABLESIZING | OFN_ENABLETEMPLATE
			| OFN_EXPLORER | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT /* | OFN_SHOWHELP*/;
		ofn.lCustData = reinterpret_cast<LPARAM>(&format);
		ofn.lpfnHook = openFileNameHookProc;
		ofn.lpTemplateName = MAKEINTRESOURCEW(IDD_DLG_SAVEFILE);

		// show dialog box
		if(!toBoolean(::GetSaveFileNameW(&ofn))) {
			const DWORD e = ::CommDlgExtendedError();
			if(e == 0)
				return py::object();	// the user canceled or closed the dialog box
			::PyErr_Format(PyExc_WindowsError, "GetSaveFileNameW failed and CommDlgExtendedError returned %u.", e);
			py::throw_error_already_set();
		}

		py::dict result;
		result["filename"] = basic_string<WCHAR>(pathName.get());
		result["encoding"] = format.first;
		result["newline"] = format.second;
		return py::object(result);
	}
}

ALPHA_EXPOSE_PROLOGUE(alpha::ambient::Interpreter::LOWEST_INSTALLATION_ORDER)
	py::scope temp(alpha::ambient::Interpreter::instance().module("ui"));

	py::def("open_file_dialog", &openFileDialog, (py::arg("initial_directory") = basic_string<WCHAR>(), py::arg("filters") = py::tuple()));
	py::def("save_file_dialog", &saveFileDialog);
ALPHA_EXPOSE_EPILOGUE()