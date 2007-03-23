/**
 * @file temporary-macro.cpp
 * @author exeal
 * @date 2003-2007
 */

#include "stdafx.h"
#include "temporary-macro.hpp"
#include "command.hpp"
#include "../manah/win32/file.hpp"
#include "../manah/win32/ui/dialog.hpp"
#include "../manah/win32/ui/standard-controls.hpp"
#include "../manah/com/unknown-impl.hpp"
#include <shlwapi.h>	// PathXxxx

using namespace alpha;
using namespace alpha::command;
using namespace std;
using namespace manah::win32::io;

namespace {
	using namespace manah::win32::ui;

	// 一時マクロを記録した XML ファイルを読み込む
	class TemporaryMacroFileReader : virtual public MSXML2::ISAXContentHandler, virtual public MSXML2::ISAXErrorHandler {
	public:
		// コンストラクタ
		explicit TemporaryMacroFileReader(TemporaryMacro& macro) throw() : macro_(macro), textInputTag_(0) {}
		~TemporaryMacroFileReader() throw() {delete textInputTag_;}
		// IUnknown
		IMPLEMENT_UNKNOWN_NO_REF_COUNT()
		STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject) {
			VERIFY_POINTER(ppvObject);
			if(riid == IID_IUnknown || riid == __uuidof(ISAXContentHandler))
				*ppvObject = static_cast<ISAXContentHandler*>(this);
			else if(riid == __uuidof(ISAXErrorHandler))
				*ppvObject = static_cast<ISAXErrorHandler*>(this);
			else
				return (*ppvObject = 0), E_NOINTERFACE;
			reinterpret_cast<IUnknown*>(*ppvObject)->AddRef();
			return S_OK;
		}
		// ISAXContentHandler
		STDMETHODIMP putDocumentLocator(MSXML2::ISAXLocator* pLocator) {return S_OK;}
		STDMETHODIMP startDocument() {macro_.startDefinition(); return S_OK;}
		STDMETHODIMP endDocument() {macro_.endDefinition(); return S_OK;}
		STDMETHODIMP startPrefixMapping(unsigned short* pwchPrefix, int cchPrefix, unsigned short* pwchUri, int cchUri) {return S_OK;}
		STDMETHODIMP endPrefixMapping(unsigned short* pwchPrefix, int cchPrefix) {return S_OK;}
		STDMETHODIMP startElement(unsigned short* pwchNamespaceUri, int cchNamespaceUri,
				unsigned short* pwchLocalName, int cchLocalName,
				unsigned short* pwchQName, int cchQName, MSXML2::ISAXAttributes* pAttributes) {
			SerializableCommand* command =
				BuiltInCommand::parseXMLInput(reinterpret_cast<wchar_t*>(pwchQName), cchQName, *pAttributes);
			if(command == 0)
				command = CharacterInputCommand::parseXMLInput(reinterpret_cast<wchar_t*>(pwchQName), cchQName, *pAttributes);
			if(command != 0) {
				macro_.pushCommand(*command);
				delete command;
			} else if(TextInputCommand* p =
					TextInputCommand::parseXMLInput(reinterpret_cast<wchar_t*>(pwchQName), cchQName, *pAttributes)) {
				if(textInputTag_ == 0) {
					textInputTag_ = new TextInputTag;
					textInputTag_->command = p;
				}
			} else if(cchLocalName == 12 && wcsncmp(reinterpret_cast<wchar_t*>(pwchLocalName), L"query-prompt", cchLocalName) == 0)
				macro_.insertUserQuery();
			return S_OK;
		}
		STDMETHODIMP endElement(unsigned short* pwchNamespaceUri, int cchNamespaceUri,
				unsigned short* pwchLocalName, int cchLocalName, unsigned short* pwchQName, int cchQName) {
			if(textInputTag_ != 0 && cchQName == 11
					&& wcsncmp(reinterpret_cast<wchar_t*>(pwchQName), L"text-input", cchQName) == 0) {
				textInputTag_->command->setText(textInputTag_->text.str());
				macro_.pushCommand(*textInputTag_->command);
				delete textInputTag_->command;
				delete textInputTag_;
				textInputTag_ = 0;
			}
			return S_OK;
		}
		STDMETHODIMP characters(unsigned short* pwchChars, int cchChars) {
			if(textInputTag_ != 0)
				textInputTag_->text.write(reinterpret_cast<wchar_t*>(pwchChars), cchChars);
			return S_OK;
		}
		STDMETHODIMP ignorableWhitespace(unsigned short* pwchChars, int cchChars) {return S_OK;}
		STDMETHODIMP processingInstruction (unsigned short* pwchTarget, int cchTarget, unsigned short* pwchData, int cchData) {return S_OK;}
		STDMETHODIMP skippedEntity(unsigned short* pwchName, int cchName) {return S_OK;}
		// ISAXErrorHandler
		STDMETHODIMP error(MSXML2::ISAXLocator* pLocator, unsigned short* pwchErrorMessage, HRESULT hrErrorCode) {
			macro_.cancelDefinition();

			int line, column;
			pLocator->getLineNumber(&line);
			pLocator->getColumnNumber(&column);
			Alpha::getInstance().messageBox(MSG_ERROR__FAILED_TO_LOAD_TEMP_MACRO, MB_ICONEXCLAMATION | MB_OK,
				MARGS % macro_.getFileName() % line % column % reinterpret_cast<wchar_t*>(pwchErrorMessage));
			return S_OK;
		}
		STDMETHODIMP fatalError(MSXML2::ISAXLocator* pLocator, unsigned short* pwchErrorMessage, HRESULT hrErrorCode) {
			return error(pLocator, pwchErrorMessage, hrErrorCode);
		}
		STDMETHODIMP ignorableWarning(MSXML2::ISAXLocator* pLocator, unsigned short* pwchErrorMessage, HRESULT hrErrorCode) {return S_OK;}

	private:
		TemporaryMacro& macro_;
		struct TextInputTag {
			TextInputCommand* command;
			ascension::OutputStringStream text;
		} * textInputTag_;
	};

	// 一時マクロのディレクトリのファイル名をリストボックスかコンボボックスに詰め込む
	void fillTemporaryMacroList(HWND control, bool listBox) {
		::WIN32_FIND_DATAW wfd;
		wchar_t path[MAX_PATH];

		::GetModuleFileNameW(0, path, MAX_PATH);
		wcscpy(::PathFindFileNameW(path), L"tmp-macros\\*.xml");
		HANDLE find = ::FindFirstFileW(path, &wfd);

		if(find != INVALID_HANDLE_VALUE) {
			wchar_t name[MAX_PATH];
			do {
				if(wfd.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY) {
					wcscpy(name, ::PathFindFileNameW(wfd.cFileName));
					*::PathFindExtensionW(name) = 0;
					::SendMessage(control, listBox ? LB_ADDSTRING : CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name));
				}
			} while(toBoolean(::FindNextFile(find, &wfd)));
		}
		::FindClose(find);
	}

	// [一時マクロの読み込み] ダイアログ
	class LoadTemporaryMacroDlg : public FixedIDDialog<IDD_DLG_LOADTEMPMACRO> {
	public:
		const wchar_t* getFileName() const {return fileName_;}
	protected:
		bool onCommand(WORD id, WORD notifyCode, HWND control) {
			if(id == IDC_LIST_MACROS && notifyCode == LBN_DBLCLK && sendItemMessage(IDC_LIST_MACROS, LB_GETCURSEL, 0, 0L) != LB_ERR)
				sendMessage(WM_COMMAND, MAKEWPARAM(IDOK, 0), 0);
			return Dialog::onCommand(id, notifyCode, control);
		}
		void onInitDialog(HWND, bool&) {
			ListBox macros;
			macros.attach(getItem(IDC_LIST_MACROS));
			fillTemporaryMacroList(macros.getHandle(), true);
			if(macros.getCount() == 0) {
				::EnableWindow(getItem(IDOK), false);
				::EnableWindow(getItem(IDC_BTN_EXECUTE), false);
			} else
				macros.setCurSel(0);
		}
		void onOK(bool&) {
			ListBox macros(getItem(IDC_LIST_MACROS));
			const int i = macros.getCurSel();
			if(i != LB_ERR) {
				::GetModuleFileName(0, fileName_, MAX_PATH);
				wchar_t* p = ::PathFindFileNameW(fileName_);
				wcscpy(p, L"tmp-macros\\");
				macros.getText(i, p + 11);
				wcscat(p + 11, L".xml");
			}
		}
	private:
		wchar_t	fileName_[MAX_PATH];
	};

	// [一時マクロの保存] ダイアログ
	class SaveTemporaryMacroDlg : public FixedIDDialog<IDD_DLG_SAVETEMPMACRO> {
	public:
		const wchar_t* getFileName() const {return fileName_;}
	protected:
		bool onCommand(WORD id, WORD notifyCode, HWND control) {
			if(id == IDC_COMBO_MACROS) {
				if(notifyCode == CBN_SELCHANGE)
					::EnableWindow(getItem(IDOK), sendItemMessage(IDC_COMBO_MACROS, CB_GETCURSEL, 0, 0L) != CB_ERR);
				else if(notifyCode == CBN_EDITCHANGE)
					::EnableWindow(getItem(IDOK), ::GetWindowTextLength(getItem(IDC_COMBO_MACROS)) != 0);
				else if(notifyCode == CBN_DBLCLK && sendItemMessage(IDC_COMBO_MACROS, CB_GETCURSEL, 0, 0L) != CB_ERR)
					sendMessage(WM_COMMAND, MAKEWPARAM(IDOK, 0), 0);
			}
			return Dialog::onCommand(id, notifyCode, control);
		}
		void onInitDialog(HWND, bool&) {
			fillTemporaryMacroList(getItem(IDC_COMBO_MACROS), false);
		}
		void onOK(bool&) {
			::GetModuleFileName(0, fileName_, MAX_PATH);
			wchar_t* p = ::PathFindFileNameW(fileName_);
			wcscpy(p, L"tmp-macros\\");
			getItemText(IDC_COMBO_MACROS, p + 11, MAX_PATH);
			wcscat(p + 11, L".xml");
		}
	private:
		wchar_t	fileName_[MAX_PATH];
	};
} // namespace @0

/// Constructor.
TemporaryMacro::TemporaryMacro() throw() : state_(NEUTRAL), errorHandlingPolicy_(IGNORE_AND_CONTINUE),
		definingIcon_(static_cast<HICON>(Alpha::getInstance().loadImage(IDR_ICON_TEMPMACRODEFINING, IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR))),
		pausingIcon_(static_cast<HICON>(Alpha::getInstance().loadImage(IDR_ICON_TEMPMACROPAUSING, IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR))) {
}

/**
 * 記録されているマクロを実行し、その末尾に追加記録を開始
 * @throw std::logic_error 実行中、記録中であればスロー
 */
void TemporaryMacro::appendDefinition() {
	execute();
	assert(definingDefinition_.commands.empty());

	SerializableCommand* command;
	for(CommandList::const_iterator it = definition_.commands.begin(); it != definition_.commands.end(); ++it) {
		(*it)->copy(command);
		definingDefinition_.commands.push_back(command);
	}
	definingDefinition_.queryPoints = definition_.queryPoints;
	startDefinition();
}

/**
 * 状態を変える
 * @param newState 新しい状態
 */
void TemporaryMacro::changeState(State newState) throw() {
	state_ = newState;
	Alpha& app = Alpha::getInstance();

	// prohibit mouse
	if(state_ == NEUTRAL || state_ == DEFINING) {
		using ascension::presentation::Presentation;
		BufferList& buffers = app.getBufferList();
		for(size_t i = 0; i < buffers.getCount(); ++i) {
			Presentation& p = buffers.getAt(i).getPresentation();
			for(Presentation::TextViewerIterator it = p.getFirstTextViewer(); it != p.getLastTextViewer(); ++it)
				(*it)->enableMouseInput(!isDefining());
		}
	}

	// update the status bar
	StatusBar& statusBar = app.getStatusBar();
	switch(getState()) {
	case TemporaryMacro::DEFINING:
		statusBar.setText(2, app.loadString(MSG_STATUS__TEMP_MACRO_DEFINING).c_str());
		statusBar.setTipText(2, app.loadString(MSG_STATUS__TEMP_MACRO_DEFINING).c_str());
		statusBar.setIcon(2, definingIcon_.getHandle());
		break;
	case TemporaryMacro::PAUSING:
		statusBar.setText(2, app.loadString(MSG_STATUS__TEMP_MACRO_PAUSING).c_str());
		statusBar.setTipText(2, app.loadString(MSG_STATUS__TEMP_MACRO_PAUSING).c_str());
		statusBar.setIcon(2, pausingIcon_.getHandle());
		break;
	default:
		statusBar.setText(2, L"");
		statusBar.setTipText(2, L"");
		statusBar.setIcon(2, 0);
		break;
	}
}


/// 記録済み、定義中のコマンドリストをクリア
void TemporaryMacro::clearCommandList(bool definingCommands) {
	CommandList& commands = definingCommands ? definingDefinition_.commands : definition_.commands;
	for(CommandList::iterator it = commands.begin(); it != commands.end(); ++it)
		delete *it;
	commands.clear();
	(definingCommands ? definingDefinition_.queryPoints : definition_.queryPoints).clear();
}

/**
 * 実行
 * @param repeatCount 繰り返し回数
 * @throw std::logic_error 実行中、記録中であればスロー
 * @throw std::invalid_argument 繰り返し回数が 0 のときスロー
 */
void TemporaryMacro::execute(ulong repeatCount /* = 1 */) {
	if(repeatCount == 0)
		throw invalid_argument("Invalid repeat count.");
	else if(isDefining() || isDefining())
		throw logic_error("Player is not ready to run macro.");

	changeState(EXECUTING);
	for(ulong i = 0; i < repeatCount; ++i) {
		CommandList::iterator			command = definition_.commands.begin();
		QueryPointList::const_iterator	queryPoint = definition_.queryPoints.begin();
		for(size_t i = 0; command != definition_.commands.end(); ++i, ++command) {
			// クエリ
			if(queryPoint != definition_.queryPoints.end() && *queryPoint == i) {
				++queryPoint;
				const int answer =
					Alpha::getInstance().messageBox(MSG_OTHER__TEMPORARY_MACRO_QUERY, MB_YESNOCANCEL | MB_ICONQUESTION);
				if(answer == IDNO)	// [いいえ] -> 残りをスキップして次のループに
					break;
				else if(answer == IDCANCEL) {	// [キャンセル] -> 中止
					changeState(NEUTRAL);
					return;
				}
			}
			if(!(*command)->execute()) {
				if(errorHandlingPolicy_ == QUERY_USER);
				else if(errorHandlingPolicy_ == ABORT)	// 中止
					break;
			}
		}
	}
	changeState(NEUTRAL);
}

/**
 * 記録中の一時マクロに入力待ち状態を挿入する
 * @throw std::logic_error 記録中でなければスロー
 */
void TemporaryMacro::insertUserQuery() {
	if(state_ != DEFINING)
		throw logic_error("Temporary macro is not in defining.");
	if(definingDefinition_.queryPoints.empty()
			|| definingDefinition_.queryPoints.back() != definingDefinition_.commands.size())
		definingDefinition_.queryPoints.push_back(definingDefinition_.commands.size());
}

/**
 * 一時マクロをファイルから読み込む
 * @param fileName ファイル名
 * @return 成否
 * @throw std::logic_error 実行中、記録中であればスロー
 */
bool TemporaryMacro::load(const basic_string<WCHAR>& fileName) {
	if(isDefining() || isExecuting())
		throw logic_error("Not ready to load.");

	manah::com::ComPtr<MSXML2::ISAXXMLReader> reader;
	TemporaryMacroFileReader handler(*this);
	HRESULT hr = reader.createInstance(__uuidof(MSXML2::SAXXMLReader));
	const basic_string<WCHAR> oldFileName = fileName;

	if(FAILED(hr))
		return false;
	hr = reader->putContentHandler(&handler);
	hr = reader->putErrorHandler(&handler);
	fileName_ = fileName;
	if(FAILED(hr = reader->parseURL(reinterpret_cast<ushort*>(const_cast<wchar_t*>(fileName.c_str()))))) {
		fileName_.assign(oldFileName);
		return false;
	}
	fileName_.assign(fileName);
	return true;
}

/**
 * コマンドを 1 つ記録
 * @param command コマンド
 * @throw std::logic_error 記録中でなければスロー
 */
void TemporaryMacro::pushCommand(SerializableCommand& command) {
	if(state_ != DEFINING)
		throw std::logic_error("Temporary macro is not defining.");
	SerializableCommand* newCommand;
	command.copy(newCommand);
	definingDefinition_.commands.push_back(newCommand);
}

/**
 * 一時マクロをファイルに保存
 * @param fileName ファイル名
 * @return 成否
 * @throw std::logic_error 実行中、記録中であればスロー
 */
bool TemporaryMacro::save(const basic_string<WCHAR>& fileName) {
	if(isDefining() || isExecuting())
		throw logic_error("Not ready to save.");

	wchar_t* directory = new wchar_t[fileName.length() + 1];
	wcscpy(directory, fileName.c_str());
	*::PathFindFileNameW(directory) = 0;
	if(!toBoolean(::PathFileExistsW(directory)))
		::CreateDirectoryW(directory, 0);
	delete[] directory;

	CommandList::iterator			command = definition_.commands.begin();
	QueryPointList::const_iterator	queryPoint = definition_.queryPoints.begin();
	wostringstream					output;
	output << L"<?xml version=\"1.0\" ?>\n<temporary-macro>\n";
	for(size_t i = 0; command != definition_.commands.end(); ++i, ++command) {
		// クエリ
		if(queryPoint != definition_.queryPoints.end() && *queryPoint == i) {
			++queryPoint;
			output << L"\t<query-prompt-command />\n";
		}
		output << L"\t";
		(*command)->getXMLOutput(output);
	}
	output << L"</temporary-macro>\n";

	using namespace ascension::encodings;
	auto_ptr<Encoder> encoder(EncoderFactory::getInstance().createEncoder(CP_UTF8));
	if(encoder.get() == 0)
		return false;

	File<true> file(fileName.c_str(), GENERIC_WRITE, FILE_SHARE_READ, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL);
	if(!file.isOpened())
		return false;

	const wstring xml = output.str();
	const size_t bufferSize = xml.length() * encoder->getMaxNativeCharLength();
	HGLOBAL data = ::GlobalAlloc(GHND, bufferSize);
	uchar* buffer = static_cast<uchar*>(::GlobalLock(data));

//	file.write(UTF8_BOM, countof(UTF8_BOM));
	file.write(buffer, static_cast<DWORD>(encoder->fromUnicode(buffer, bufferSize, xml.data(), xml.length())));
	file.close();
	::GlobalUnlock(data);
	::GlobalFree(data);
	fileName_.assign(fileName);
	return true;
}

/// 一時マクロを読み込むためのダイアログを表示する
void TemporaryMacro::showLoadDialog() {
	LoadTemporaryMacroDlg dlg;
	if(dlg.doModal(Alpha::getInstance().getMainWindow().getHandle()) == IDOK)
		load(dlg.getFileName());
}

/// 一時マクロを保存するためのダイアログを表示する
void TemporaryMacro::showSaveDialog() {
	SaveTemporaryMacroDlg dlg;
	if(dlg.doModal(Alpha::getInstance().getMainWindow().getHandle()) == IDOK)
		save(dlg.getFileName());
}
