/**
 * @file command.cpp
 * @author exeal
 * @date 2004-2007
 */

#include "stdafx.h"
#include "resource/messages.h"
#include "command.hpp"
#include "application.hpp"
#include "mru-manager.hpp"
#include "print.hpp"
#include "about-dialog.hpp"
#include "bookmark-dialog.hpp"
#include "execute-command-dialog.hpp"
#include "search-dialog.hpp"
#include "goto-line-dialog.hpp"
#include "ascension/text-editor.hpp"
#include <shlwapi.h>	// PathXxxx, StrXxxx

using namespace alpha;
using namespace alpha::command;
using namespace ascension;
using namespace ascension::texteditor::commands;
using namespace manah::win32::ui;
using namespace std;


#define CHECK_REBAR_BAND_VISIBILITY(index)			\
	MANAH_AUTO_STRUCT_SIZE(::REBARBANDINFOW, rbbi);	\
	rbbi.fMask = RBBIM_STYLE;						\
	app.rebar_.getBandInfo(index, rbbi);			\
	const bool visible = !toBoolean(rbbi.fStyle & RBBS_HIDDEN);

namespace {	// アイコンビットマップを編集する連中
	HBITMAP createFilteredBitmap(HDC dc, const ::BITMAPINFO& bi,
			const ::BITMAPINFO* mask, const COLORREF* maskColor, ::RGBQUAD(*filterFunction)(const ::RGBQUAD&)) throw() {
		assert(bi.bmiHeader.biBitCount == 32 || bi.bmiHeader.biBitCount == 24);
		assert(mask == 0 || maskColor == 0);
		assert(mask == 0 || mask->bmiHeader.biBitCount == 1);

		const long cx = bi.bmiHeader.biWidth, cy = bi.bmiHeader.biHeight;
		assert(mask == 0 || (cx == mask->bmiHeader.biWidth && cy == mask->bmiHeader.biHeight));

		uchar maskEntry;
		if(mask != 0)
			maskEntry = (mask->bmiColors[0].rgbRed == 0 && mask->bmiColors[0].rgbGreen == 0 && mask->bmiColors[0].rgbBlue == 0) ? 0 : 1;

		const uchar* srcPixels = reinterpret_cast<const uchar*>(bi.bmiColors);
		uchar* destPixels;
		HBITMAP bitmap = ::CreateDIBSection(dc, &bi, DIB_RGB_COLORS, reinterpret_cast<void**>(&destPixels), 0, 0);

		ptrdiff_t offset = 0;								// unit is byte
		ptrdiff_t maskOffset = sizeof(::RGBQUAD) * 2 * 8;	// unit is bit
		for(long y = 0; y < cy; ++y, maskOffset += sizeof(::LONG) * 8 - maskOffset % (sizeof(::LONG) * 8)) {
			for(long x = 0; x < cx; ++x, offset += bi.bmiHeader.biBitCount / 8, ++maskOffset) {
				bool transparent = false;
				if(mask != 0)
					transparent = ((reinterpret_cast<const uchar*>(mask->bmiColors)[maskOffset / 8] >> (7 - maskOffset % 8)) & 0x01) != maskEntry;

				if(bi.bmiHeader.biBitCount == 32) {	// 32 ビット: BITMAPINFO::bmiColors は RGBQUAD[]
					const ::RGBQUAD& src = bi.bmiColors[offset / sizeof(::RGBQUAD)];
					if(maskColor != 0)
						transparent = (*maskColor & 0xFF) == src.rgbRed
							&& ((*maskColor >> 8) & 0xFF) == src.rgbGreen && ((*maskColor >> 16) & 0xFF) == src.rgbBlue;
					*reinterpret_cast<::RGBQUAD*>(&destPixels[offset]) = transparent ? src : (*filterFunction)(src);
				} else {	// 24 ビット: BITMAPINFO::bmiColors は 24 ビットが 1 ピクセルの色情報配列
					if(maskColor != 0)
						transparent = ((*maskColor >> 16) & 0xFF) == srcPixels[offset]
							&& ((*maskColor >> 8) & 0xFF) == srcPixels[offset + 1] && (*maskColor & 0xFF) == srcPixels[offset + 2];
					if(transparent)
						memcpy(destPixels + offset, srcPixels + offset, 3);
					else {
						const ::RGBQUAD src = {srcPixels[offset + 0],
												srcPixels[offset + 1], srcPixels[offset + 2], 0x00};
						const ::RGBQUAD dest = (*filterFunction)(src);
						destPixels[offset + 0] = dest.rgbBlue;
						destPixels[offset + 1] = dest.rgbGreen;
						destPixels[offset + 2] = dest.rgbRed;
					}
				}
			}
		}
		return bitmap;
	}
	pair<HBITMAP, COLORREF> createFilteredBitmap(HDC dc, HBITMAP srcBitmap, ::RGBQUAD(*filterFunction)(const ::RGBQUAD&)) throw() {
		::BITMAP bitmap;
		::GetObject(srcBitmap, sizeof(BITMAP), &bitmap);
		HDC compDC = ::CreateCompatibleDC(0);
		HBITMAP destBitmap = ::CreateCompatibleBitmap(dc, bitmap.bmHeight, bitmap.bmWidth);
		HBITMAP oldBitmap = static_cast<HBITMAP>(::SelectObject(compDC, srcBitmap));
		COLORREF* pixels = new COLORREF[bitmap.bmHeight * bitmap.bmWidth];
		::RGBQUAD color = {0, 0, 0, 0};

		assert(bitmap.bmBitsPixel < 32);
		for(int y = 0; y < bitmap.bmHeight; ++y)
			for(int x = 0; x < bitmap.bmWidth; ++x)
				pixels[y * bitmap.bmWidth + x] = ::GetPixel(compDC, x, y);
		::SelectObject(compDC, destBitmap);
		const COLORREF maskColor = pixels[0];
		for(int y = 0; y < bitmap.bmHeight; ++y) {
			for(int x = 0; x < bitmap.bmWidth; ++x) {
				if(pixels[y * bitmap.bmWidth + x] != pixels[0]) {
					color.rgbRed = GetRValue(pixels[y * bitmap.bmWidth + x]);
					color.rgbGreen = GetGValue(pixels[y * bitmap.bmWidth + x]);
					color.rgbBlue = GetBValue(pixels[y * bitmap.bmWidth + x]);
					color = (*filterFunction)(color);
					::SetPixel(compDC, x, y, RGB(color.rgbRed, color.rgbGreen, color.rgbBlue));
				} else
					::SetPixel(compDC, x, y, pixels[0]);
			}
		}
		::SelectObject(compDC, oldBitmap);
		::DeleteDC(compDC);
		delete[] pixels;
		return make_pair(destBitmap, maskColor);
	}
	inline BYTE applyGamma(double src, double gamma) throw() {
		return static_cast<BYTE>(pow(src / 255.0, gamma) * 255.0);
	}
	inline ::RGBQUAD grayscaleFilter(const ::RGBQUAD& src) throw() {
		static const double redFact = 0.299, greenFact = 0.587, blueFact = 0.114;
		static const double gamma = 0.5;
		const double gray = src.rgbRed * redFact + src.rgbGreen * greenFact + src.rgbBlue * blueFact;
		const BYTE value = applyGamma(gray, gamma);
		::RGBQUAD dest = {value, value, value, src.rgbReserved};
		return dest;
	}
	inline ::RGBQUAD saturationFilter(const ::RGBQUAD& src) throw() {
		static const double gamma = 1.4;
		::RGBQUAD dest = {applyGamma(src.rgbBlue, gamma),
			applyGamma(src.rgbGreen, gamma), applyGamma(src.rgbRed, gamma), src.rgbReserved};
		return dest;
	}
	inline ::RGBQUAD sepiaFilter(const ::RGBQUAD& src) throw() {
		::RGBQUAD dest = grayscaleFilter(src);
		dest.rgbRed = (dest.rgbRed + 10 > 0xFF) ? 0xFF : (dest.rgbRed + 10);
		dest.rgbGreen = (dest.rgbGreen > 10) ? (dest.rgbGreen - 10) : 0;
		dest.rgbBlue = (dest.rgbBlue > 20) ? (dest.rgbBlue - 20) : 0;
		return dest;
	}
} // namespace @0


// CommandManager ///////////////////////////////////////////////////////////

/// Constructor.
CommandManager::CommandManager() throw() : lastCommandID_(0) {
}

/**
 * Returns the name of the specified command
 * @param id the identifier of the command
 * @return the human-readable name
 */
wstring CommandManager::caption(CommandID id) const {
	wstring s(Alpha::instance().loadMessage(id));
	const size_t eol = s.find_first_of(L"\n\r");
	if(eol != wstring::npos)
		s.resize(eol);
	return s;
}

/**
 * Recreates the image list by load icons from the specified directory.
 * @param directory the directory name
 * @return true if succeeded
 */
bool CommandManager::createImageList(const basic_string<WCHAR>& directory) {
	icons_[ICONSTATE_NORMAL].destroy();
	icons_[ICONSTATE_DISABLED].destroy();
	icons_[ICONSTATE_HOT].destroy();
	iconIndices_.clear();

	icons_[ICONSTATE_NORMAL].create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 0);
	icons_[ICONSTATE_DISABLED].create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 0);
	icons_[ICONSTATE_HOT].create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 0);

	WCHAR path[MAX_PATH];
	::WIN32_FIND_DATAW wfd;
	HANDLE find;

	wcscpy(path, directory.c_str());
	::PathAppendW(path, L"*.*");
	find = ::FindFirstFileW(path, &wfd);

	if(find == INVALID_HANDLE_VALUE)
		return 0;

	do {
		const WCHAR* extension = ::PathFindExtensionW(wfd.cFileName);
		bool imageIsBmp;

		if(wcslen(extension) != 4)
			continue;
		else if(::StrCmpNIW(extension + 1, L"bmp", 3) == 0)
			imageIsBmp = true;
		else if(::StrCmpNIW(extension + 1, L"ico", 3) == 0)
			imageIsBmp = false;
		else
			continue;

		WCHAR fileTitle[MAX_PATH];
		wcscpy(fileTitle, ::PathFindFileNameW(wfd.cFileName));
		::PathFindExtensionW(fileTitle)[0] = 0;

		const CommandID id = static_cast<CommandID>(wcstoul(fileTitle, 0, 10) + CMD_SPECIAL_START);
		if(id < CMD_SPECIAL_START || id > CMD_SPECIAL_END)
			continue;
		wcscpy(fileTitle, directory.c_str());
		::PathAppendW(fileTitle, wfd.cFileName);

		HBITMAP bitmap = 0;
		HICON icon = 0;

		// イメージを読み込む
		Alpha& app = Alpha::instance();
		if(imageIsBmp)
			bitmap = static_cast<HBITMAP>(app.loadImage(fileTitle, IMAGE_BITMAP, 16, 16, LR_CREATEDIBSECTION | LR_LOADFROMFILE));
		else {
			::ICONINFO iconInfo;
			icon = static_cast<HICON>(app.loadImage(fileTitle, IMAGE_ICON, 16, 16, LR_CREATEDIBSECTION | LR_LOADFROMFILE));
			::GetIconInfo(icon, &iconInfo);
			bitmap = iconInfo.hbmColor;
		}

		// フィルタを適用してイメージリストに追加する
		if(bitmap != 0 || icon != 0) {
			HDC dc = ::GetDC(0);
			::BITMAP bmp;
			iconIndices_.insert(make_pair(id, icons_[ICONSTATE_NORMAL].getNumberOfImages()));
			::GetObject(bitmap, sizeof(::BITMAP), &bmp);

			if(bmp.bmBitsPixel >= 24) {
				// 色情報を取得
				::BITMAPINFO* pbi = static_cast<::BITMAPINFO*>(::operator new(
					sizeof(::BITMAPINFOHEADER) + bmp.bmWidth * bmp.bmHeight * bmp.bmBitsPixel / 8));
				memset(&pbi->bmiHeader, 0, sizeof(::BITMAPINFOHEADER));
				pbi->bmiHeader.biSize = sizeof(::BITMAPINFOHEADER);
				::GetDIBits(dc, bitmap, 0, bmp.bmHeight, 0, pbi, DIB_RGB_COLORS);
				pbi->bmiHeader.biCompression = BI_RGB;
				::GetDIBits(dc, bitmap, 0, pbi->bmiHeader.biHeight, pbi->bmiColors, pbi, DIB_RGB_COLORS);

				HBITMAP grayBitmap, hotBitmap;
				if(!imageIsBmp) {	// アイコン
					::ICONINFO iconInfo;
					::GetIconInfo(icon, &iconInfo);
					::BITMAP maskBitmap;
					::GetObject(iconInfo.hbmMask, sizeof(::BITMAP), &maskBitmap);
					assert(maskBitmap.bmBitsPixel == 1);

					::BITMAPINFO* mask = static_cast<::BITMAPINFO*>(::operator new(
						sizeof(::BITMAPINFOHEADER) + sizeof(::RGBQUAD) * 2 + maskBitmap.bmWidth * maskBitmap.bmHeight / 4));
					memset(&mask->bmiHeader, 0, sizeof(::BITMAPINFOHEADER));
					mask->bmiHeader.biSize = sizeof(::BITMAPINFOHEADER);
					::GetDIBits(dc, iconInfo.hbmMask, 0, maskBitmap.bmHeight, 0, mask, DIB_RGB_COLORS);
					mask->bmiHeader.biCompression = BI_RGB;
					::GetDIBits(dc, iconInfo.hbmMask, 0, mask->bmiHeader.biHeight, mask->bmiColors + 2, mask, DIB_RGB_COLORS);

					icons_[ICONSTATE_NORMAL].add(icon);
					grayBitmap = iconInfo.hbmColor = createFilteredBitmap(dc, *pbi, mask, 0, sepiaFilter);
					HICON grayIcon = ::CreateIconIndirect(&iconInfo);
					icons_[ICONSTATE_DISABLED].add(grayIcon);
					hotBitmap = iconInfo.hbmColor = createFilteredBitmap(dc, *pbi, mask, 0, saturationFilter);
					HICON hotIcon = ::CreateIconIndirect(&iconInfo);
					icons_[ICONSTATE_HOT].add(hotIcon);
					::DestroyIcon(grayIcon);
					::DestroyIcon(hotIcon);

					::operator delete(mask);
				} else {	// ビットマップ
					if(bmp.bmBitsPixel == 32) {	// 32 ビット -> アルファチャンネルを使用
						icons_[ICONSTATE_NORMAL].add(bitmap);
						icons_[ICONSTATE_DISABLED].add(grayBitmap = createFilteredBitmap(dc, *pbi, 0, 0, sepiaFilter));
						icons_[ICONSTATE_HOT].add(hotBitmap = createFilteredBitmap(dc, *pbi, 0, 0, saturationFilter));
					} else {	// 24 ビット以下 -> 左上の色が透明色
						const COLORREF maskColor = RGB(pbi->bmiColors[0].rgbRed, pbi->bmiColors[0].rgbGreen, pbi->bmiColors[0].rgbBlue);
						icons_[ICONSTATE_NORMAL].add(bitmap, maskColor);
						icons_[ICONSTATE_DISABLED].add(grayBitmap = createFilteredBitmap(dc, *pbi, 0, &maskColor, sepiaFilter), maskColor);
						icons_[ICONSTATE_HOT].add(hotBitmap = createFilteredBitmap(dc, *pbi, 0, &maskColor, saturationFilter), maskColor);
					}
				}

				::DeleteObject(grayBitmap);
				::DeleteObject(hotBitmap);
				delete[] pbi;
			}
			::ReleaseDC(0, dc);

			if(imageIsBmp)
				::DeleteObject(bitmap);
			else
				::DestroyIcon(icon);
		}
	} while(toBoolean(::FindNextFileW(find, &wfd)));
	::FindClose(find);

	icons_[ICONSTATE_NORMAL].setBkColor(CLR_NONE);
	icons_[ICONSTATE_DISABLED].setBkColor(CLR_NONE);
	icons_[ICONSTATE_HOT].setBkColor(CLR_NONE);

	return true;
}

/**
 * Returns the description of the specified command.
 * @param id the identifier of the command
 * @return the description text
 */
wstring CommandManager::description(CommandID id) const {
	if(id >= CMD_SPECIAL_BUFFERSSTART && id < CMD_SPECIAL_BUFFERSEND) {
		const Buffer& buffer = Alpha::instance().bufferList().at(id - CMD_SPECIAL_BUFFERSSTART);
		return buffer.textFile().isOpen() ? buffer.textFile().location() : Alpha::instance().loadMessage(MSG_BUFFER__UNTITLED);
	}
	const wstring s(Alpha::instance().loadMessage(id));
	const size_t eol = s.find_first_of(L"\n\r");
	return (eol != wstring::npos) ? s.substr(eol + 1) : L"";
}

/**
 * Executes the specified command.
 * @param id th identifier of the command to execute
 * @param userContext set true if an end user executes. in this case, the command may not be executed
 * @return the result of the command
 */
bool CommandManager::executeCommand(CommandID id, bool userContext) {
	if(!isEnabled(id, userContext))
		return false;

	Alpha& app = Alpha::instance();
	EditorView& view = app.bufferList().activeView();
	Buffer& buffer = view.document();

	// 一時マクロ記録中
	if(temporaryMacro_.state() == TemporaryMacro::DEFINING && isRecordable(id))
		temporaryMacro_.pushCommand(BuiltInCommand(id));

	lastCommandID_ = id;

	switch(id) {
	case CMD_FILE_NEW:				app.bufferList().addNew(); return true;
	case CMD_FILE_NEWWITHFORMAT:	app.bufferList().addNewDialog(); return true;
	case CMD_FILE_OPEN:				return app.bufferList().openDialog() == BufferList::OPENRESULT_SUCCEEDED;
	case CMD_FILE_CLOSE:			return app.bufferList().close(app.bufferList().activeIndex(), true);
	case CMD_FILE_CLOSEALL:			return app.bufferList().closeAll(true, false);
	case CMD_FILE_SAVE:				return app.bufferList().save(app.bufferList().activeIndex());
	case CMD_FILE_SAVEAS:
		if(app.bufferList().save(app.bufferList().activeIndex(), false)) {
			// TODO: call mode-application.
//			app.applyDocumentType(app.bufferList().active());
			return true;
		}
		return false;
	case CMD_FILE_SAVEALL:				return app.bufferList().saveAll();
	case CMD_FILE_REOPEN:				return app.bufferList().reopen(app.bufferList().activeIndex(), false) == BufferList::OPENRESULT_SUCCEEDED;
	case CMD_FILE_REOPENWITHCODEPAGE:	return app.bufferList().reopen(app.bufferList().activeIndex(), true) == BufferList::OPENRESULT_SUCCEEDED;
	case CMD_FILE_EXIT:					app.getMainWindow().postMessage(WM_CLOSE); return true;
//	case CMD_FILE_SENDMAIL:				return buffer.sendFile(toBoolean(app.readIntegerProfile(L"File", L"sendMailAsAttachment", 1)));
	case CMD_FILE_CLOSEOTHERS:			return app.bufferList().closeAll(true, true);
	case CMD_FILE_PRINT:				Printing::instance().print(buffer, true); return true;
	case CMD_FILE_PRINTSETUP:			Printing::instance().setupPages(); return true;

	case CMD_EDIT_DELETE:			return DeletionCommand(view, DeletionCommand::NEXT_CHARACTER).execute() == 0;
	case CMD_EDIT_BACKSPACE:		return DeletionCommand(view, DeletionCommand::PREVIOUS_CHARACTER).execute() == 0;
	case CMD_EDIT_DELETETONEXTWORD:	return DeletionCommand(view, DeletionCommand::NEXT_WORD).execute() == 0;
	case CMD_EDIT_DELETETOPREVWORD:	return DeletionCommand(view, DeletionCommand::PREVIOUS_WORD).execute() == 0;
	case CMD_EDIT_DELETELINE:		return DeletionCommand(view, DeletionCommand::WHOLE_LINE).execute() == 0;
	case CMD_EDIT_INSERTPREVLINE:	return NewlineCommand(view, true).execute() == 0;
	case CMD_EDIT_BREAK:			return NewlineCommand(view, false).execute() == 0;
	case CMD_EDIT_UNDO:	return UndoCommand(view, true).execute() == 0;
	case CMD_EDIT_REDO:	return UndoCommand(view, false).execute() == 0;
	case CMD_EDIT_CUT:						return ClipboardCommand(view, ClipboardCommand::CUT, true).execute() == 0;
	case CMD_EDIT_COPY:						return ClipboardCommand(view, ClipboardCommand::COPY, true).execute() == 0;
	case CMD_EDIT_PASTE:					return ClipboardCommand(view, ClipboardCommand::PASTE, false).execute() == 0;
	case CMD_EDIT_PASTEFROMCLIPBOARDRING:	return ClipboardCommand(view, ClipboardCommand::PASTE, true).execute() == 0;
	case CMD_EDIT_INSERTTAB:	return texteditor::commands::CharacterInputCommand(view, static_cast<CodePoint>(L'\t')).execute() == 0;
	case CMD_EDIT_DELETETAB:	return IndentationCommand(view, false, true, 1).execute() == 0;
	case CMD_EDIT_TABIFY:	return TabifyCommand(view, true).execute() == 0;
	case CMD_EDIT_UNTABIFY:	return TabifyCommand(view, false).execute() == 0;
	case CMD_EDIT_CHARTOCODEPOINT:	return CharacterCodePointConversionCommand(view, true).execute() == 0;
	case CMD_EDIT_CODEPOINTTOCHAR:	return CharacterCodePointConversionCommand(view, false).execute() == 0;
	case CMD_EDIT_RECOMPOSE:	return ReconversionCommand(view).execute() == 0;
	case CMD_EDIT_TOGGLEOVERTYPEMODE:
		InputStatusToggleCommand(view, InputStatusToggleCommand::OVERTYPE_MODE).execute();
		return true;
	case CMD_EDIT_OPENCANDIDATEWINDOW:
		if(temporaryMacro_.state() == TemporaryMacro::DEFINING)
			return false;
		return CompletionProposalPopupCommand(view).execute() == 0;
	case CMD_EDIT_HOME:			CaretMovementCommand(view, CaretMovementCommand::BEGINNING_OF_DOCUMENT, false).execute(); return true;
	case CMD_EDIT_END:			CaretMovementCommand(view, CaretMovementCommand::END_OF_DOCUMENT, false).execute(); return true;
	case CMD_EDIT_LINEHOME:		CaretMovementCommand(view, CaretMovementCommand::BEGINNING_OF_VISUAL_LINE, false).execute(); return true;
	case CMD_EDIT_LINEEND:		CaretMovementCommand(view, CaretMovementCommand::END_OF_LINE, false).execute(); return true;
	case CMD_EDIT_FIRSTCHAR:	CaretMovementCommand(view, CaretMovementCommand::FIRST_PRINTABLE_CHARACTER_OF_VISUAL_LINE, false).execute(); return true;
	case CMD_EDIT_LASTCHAR:		CaretMovementCommand(view, CaretMovementCommand::LAST_PRINTABLE_CHARACTER_OF_VISUAL_LINE, false).execute(); return true;
	case CMD_EDIT_FIRSTCHARORLINEHOME:	CaretMovementCommand(view, CaretMovementCommand::CONTEXTUAL_BEGINNING_OF_VISUAL_LINE, false).execute(); return true;
	case CMD_EDIT_LASTCHARORLINEEND:	CaretMovementCommand(view, CaretMovementCommand::CONTEXTUAL_END_OF_VISUAL_LINE, false).execute(); return true;
	case CMD_EDIT_CHARNEXT:		CaretMovementCommand(view, CaretMovementCommand::RIGHT_CHARACTER, false).execute(); return true;
	case CMD_EDIT_CHARPREV:		CaretMovementCommand(view, CaretMovementCommand::LEFT_CHARACTER, false).execute(); return true;
	case CMD_EDIT_WORDENDNEXT:	CaretMovementCommand(view, CaretMovementCommand::RIGHT_WORDEND, false).execute(); return true;
	case CMD_EDIT_WORDENDPREV:	CaretMovementCommand(view, CaretMovementCommand::LEFT_WORDEND, false).execute(); return true;
	case CMD_EDIT_WORDNEXT:		CaretMovementCommand(view, CaretMovementCommand::RIGHT_WORD, false).execute(); return true;
	case CMD_EDIT_WORDPREV:		CaretMovementCommand(view, CaretMovementCommand::LEFT_WORD, false).execute(); return true;
	case CMD_EDIT_LINEDOWN:		CaretMovementCommand(view, CaretMovementCommand::NEXT_VISUAL_LINE, false).execute(); return true;
	case CMD_EDIT_LINEUP:		CaretMovementCommand(view, CaretMovementCommand::PREVIOUS_VISUAL_LINE, false).execute(); return true;
	case CMD_EDIT_PAGEDOWN:		CaretMovementCommand(view, CaretMovementCommand::NEXT_PAGE, false).execute(); return true;
	case CMD_EDIT_PAGEUP:		CaretMovementCommand(view, CaretMovementCommand::PREVIOUS_PAGE, false).execute(); return true;
	case CMD_EDIT_HOMEEXTEND:			CaretMovementCommand(view, CaretMovementCommand::BEGINNING_OF_DOCUMENT, true).execute(); return true;
	case CMD_EDIT_ENDEXTEND:			CaretMovementCommand(view, CaretMovementCommand::END_OF_DOCUMENT, true).execute(); return true;
	case CMD_EDIT_LINEHOMEEXTEND:		CaretMovementCommand(view, CaretMovementCommand::BEGINNING_OF_VISUAL_LINE, true).execute(); return true;
	case CMD_EDIT_LINEENDEXTEND:		CaretMovementCommand(view, CaretMovementCommand::END_OF_VISUAL_LINE, true).execute(); return true;
	case CMD_EDIT_FIRSTCHAREXTEND:		CaretMovementCommand(view, CaretMovementCommand::FIRST_PRINTABLE_CHARACTER_OF_VISUAL_LINE, true).execute(); return true;
	case CMD_EDIT_LASTCHAREXTEND:		CaretMovementCommand(view, CaretMovementCommand::LAST_PRINTABLE_CHARACTER_OF_VISUAL_LINE, true).execute(); return true;
	case CMD_EDIT_FIRSTCHARORLINEHOMEEXTEND:	CaretMovementCommand(view, CaretMovementCommand::CONTEXTUAL_BEGINNING_OF_VISUAL_LINE, true).execute(); return true;
	case CMD_EDIT_LASTCHARORLINEENDEXTEND:		CaretMovementCommand(view, CaretMovementCommand::CONTEXTUAL_END_OF_VISUAL_LINE, true).execute(); return true;
	case CMD_EDIT_CHARNEXTEXTEND:		CaretMovementCommand(view, CaretMovementCommand::RIGHT_CHARACTER, true).execute(); return true;
	case CMD_EDIT_CHARPREVEXTEND:		CaretMovementCommand(view, CaretMovementCommand::LEFT_CHARACTER, true).execute(); return true;
	case CMD_EDIT_WORDENDNEXTEXTEND:	CaretMovementCommand(view, CaretMovementCommand::RIGHT_WORDEND, true).execute(); return true;
	case CMD_EDIT_WORDENDPREVEXTEND:	CaretMovementCommand(view, CaretMovementCommand::LEFT_WORDEND, true).execute(); return true;
	case CMD_EDIT_WORDNEXTEXTEND:		CaretMovementCommand(view, CaretMovementCommand::RIGHT_WORD, true).execute(); return true;
	case CMD_EDIT_WORDPREVEXTEND:		CaretMovementCommand(view, CaretMovementCommand::LEFT_WORD, true).execute(); return true;
	case CMD_EDIT_LINEDOWNEXTEND:		CaretMovementCommand(view, CaretMovementCommand::NEXT_VISUAL_LINE, true).execute(); return true;
	case CMD_EDIT_LINEUPEXTEND:			CaretMovementCommand(view, CaretMovementCommand::PREVIOUS_VISUAL_LINE, true).execute(); return true;
	case CMD_EDIT_PAGEDOWNEXTEND:		CaretMovementCommand(view, CaretMovementCommand::NEXT_PAGE, true).execute(); return true;
	case CMD_EDIT_PAGEUPEXTEND:			CaretMovementCommand(view, CaretMovementCommand::PREVIOUS_PAGE, true).execute(); return true;
	case CMD_EDIT_SELECTALL:			SelectionCreationCommand(view, SelectionCreationCommand::ALL).execute(); return true;
	case CMD_EDIT_SELECTCURRENTWORD:	SelectionCreationCommand(view, SelectionCreationCommand::CURRENT_WORD).execute(); return true;
	case CMD_EDIT_CANCELSELECTION:		CancelCommand(view).execute(); return true;
	case CMD_EDIT_SCROLLHOME:			view.sendMessage(WM_VSCROLL, SB_TOP); return true;
	case CMD_EDIT_SCROLLEND:			view.sendMessage(WM_VSCROLL, SB_BOTTOM); return true;
	case CMD_EDIT_SCROLLLINEDOWN:		view.sendMessage(WM_VSCROLL, SB_LINEDOWN); return true;
	case CMD_EDIT_SCROLLLINEUP:			view.sendMessage(WM_VSCROLL, SB_LINEUP); return true;
	case CMD_EDIT_SCROLLPAGEDOWN:		view.sendMessage(WM_VSCROLL, SB_PAGEDOWN); return true;
	case CMD_EDIT_SCROLLPAGEUP:			view.sendMessage(WM_VSCROLL, SB_PAGEUP); return true;
	case CMD_EDIT_SCROLLCOLUMNNEXT:		view.sendMessage(WM_HSCROLL, SB_RIGHT); return true;
	case CMD_EDIT_SCROLLCOLUMNPREV:		view.sendMessage(WM_HSCROLL, SB_LEFT); return true;
	case CMD_EDIT_ENSURECARETCENTER:	view.caret().recenter(); return true;
	case CMD_EDIT_ENSURECARETVISIBLE:	view.caret().show(); return true;
	case CMD_EDIT_ROWCHARNEXT:			RowSelectionExtensionCommand(view, RowSelectionExtensionCommand::RIGHT_CHARACTER).execute(); return true;
	case CMD_EDIT_ROWCHARPREV:			RowSelectionExtensionCommand(view, RowSelectionExtensionCommand::LEFT_CHARACTER).execute(); return true;
	case CMD_EDIT_ROWLINEDOWN:			RowSelectionExtensionCommand(view, RowSelectionExtensionCommand::NEXT_VISUAL_LINE).execute(); return true;
	case CMD_EDIT_ROWLINEEND:			RowSelectionExtensionCommand(view, RowSelectionExtensionCommand::END_OF_LINE).execute(); return true;
	case CMD_EDIT_ROWLINEHOME:			RowSelectionExtensionCommand(view, RowSelectionExtensionCommand::BEGINNING_OF_VISUAL_LINE).execute(); return true;
	case CMD_EDIT_ROWLINEUP:			RowSelectionExtensionCommand(view, RowSelectionExtensionCommand::PREVIOUS_VISUAL_LINE).execute(); return true;
	case CMD_EDIT_ROWWORDENDNEXT:		RowSelectionExtensionCommand(view, RowSelectionExtensionCommand::RIGHT_WORDEND).execute(); return true;
	case CMD_EDIT_ROWWORDENDPREV:		RowSelectionExtensionCommand(view, RowSelectionExtensionCommand::LEFT_WORDEND).execute(); return true;
	case CMD_EDIT_ROWWORDNEXT:			RowSelectionExtensionCommand(view, RowSelectionExtensionCommand::RIGHT_WORD).execute(); return true;
	case CMD_EDIT_ROWWORDPREV:			RowSelectionExtensionCommand(view, RowSelectionExtensionCommand::LEFT_WORD).execute(); return true;
	case CMD_EDIT_CHARFROMABOVELINE:	return CharacterInputFromNextLineCommand(view, false).execute() == 0;
	case CMD_EDIT_CHARFROMBELOWLINE:	return CharacterInputFromNextLineCommand(view, true).execute() == 0;
	case CMD_EDIT_TRANSPOSELINES:		return TranspositionCommand(view, TranspositionCommand::LINES).execute() == 0;
	case CMD_EDIT_TRANSPOSECHARS:		return TranspositionCommand(view, TranspositionCommand::CHARACTERS).execute() == 0;
	case CMD_EDIT_TRANSPOSEWORDS:		return TranspositionCommand(view, TranspositionCommand::WORDS).execute() == 0;
//	case CMD_EDIT_SHOWABBREVIATIONDLG:	{
//		AbbreviationsDlg().doModal(app.getMainWindow());
//		return true;
//	}
	case CMD_EDIT_NARROWTOSELECTION:
		buffer.narrow(view.caret().selectionRegion());
		return true;
	case CMD_EDIT_WIDEN:
		buffer.widen();
		return true;

	case CMD_SEARCH_FIND:		app.showSearchDialog(); return true;
	case CMD_SEARCH_FINDNEXT:	return app.searchDialog().searchNext(FORWARD);
	case CMD_SEARCH_FINDPREV:	return app.searchDialog().searchNext(BACKWARD);
	case CMD_SEARCH_REPLACEALLINTERACTIVE:	app.searchDialog().replaceAll(true); return true;
	case CMD_SEARCH_REPLACEALL:				app.searchDialog().replaceAll(false); return true;
	case CMD_SEARCH_BOOKMARKALL:	app.searchDialog().bookmarkAll(); return true;
//	case CMD_SEARCH_REVOKEMARK:		view.highlightMatchTexts(false); return true;
	case CMD_SEARCH_GOTOLINE:		ui::GotoLineDialog().doModal(app.getMainWindow()); return true;
	case CMD_SEARCH_TOGGLEBOOKMARK:		BookmarkCommand(view, BookmarkCommand::TOGGLE_CURRENT_LINE).execute(); return true;
	case CMD_SEARCH_NEXTBOOKMARK:		CaretMovementCommand(view, CaretMovementCommand::NEXT_BOOKMARK).execute(); return true;
	case CMD_SEARCH_PREVBOOKMARK:		CaretMovementCommand(view, CaretMovementCommand::PREVIOUS_BOOKMARK).execute(); return true;
	case CMD_SEARCH_CLEARBOOKMARKS:		BookmarkCommand(view, BookmarkCommand::CLEAR_ALL).execute(); return true;
	case CMD_SEARCH_MANAGEBOOKMARKS:
		if(!app.bookmarkDialog_->isWindow()) {
			app.bookmarkDialog_->doModeless(app.getMainWindow());
			app.pushModelessDialog(app.bookmarkDialog_->getHandle());
			if(toBoolean(app.readIntegerProfile(L"View", L"applyMainFontToSomeControls", 1)))
				app.bookmarkDialog_->sendItemMessage(IDC_LIST_BOOKMARKS, WM_SETFONT, reinterpret_cast<WPARAM>(app.editorFont_), true);
		} else
			app.bookmarkDialog_->setActive();
		return true;
	case CMD_SEARCH_GOTOMATCHBRACKET:		return CaretMovementCommand(view, CaretMovementCommand::MATCH_BRACKET, false).execute() == 0;
	case CMD_SEARCH_EXTENDTOMATCHBRACKET:	return CaretMovementCommand(view, CaretMovementCommand::MATCH_BRACKET, true).execute() == 0;
	case CMD_SEARCH_INCREMENTALSEARCH:
		if(temporaryMacro_.state() == TemporaryMacro::DEFINING)
			return false;
		view.beginIncrementalSearch(searcher::LITERAL, FORWARD);
		return true;
	case CMD_SEARCH_INCREMENTALSEARCHR:
		if(temporaryMacro_.state() == TemporaryMacro::DEFINING)
			return false;
		view.beginIncrementalSearch(searcher::LITERAL, BACKWARD);
		return true;
	case CMD_SEARCH_INCREMENTALSEARCHRF:
		if(temporaryMacro_.state() == TemporaryMacro::DEFINING)
			return false;
		view.beginIncrementalSearch(searcher::REGULAR_EXPRESSION, FORWARD);
		return true;
	case CMD_SEARCH_INCREMENTALSEARCHRR:
		if(temporaryMacro_.state() == TemporaryMacro::DEFINING)
			return false;
		view.beginIncrementalSearch(searcher::REGULAR_EXPRESSION, BACKWARD);
		return true;
	case CMD_SEARCH_INCREMENTALSEARCHMF:
		if(temporaryMacro_.state() == TemporaryMacro::DEFINING)
			return false;
		view.beginIncrementalSearch(searcher::MIGEMO, FORWARD);
		return true;
	case CMD_SEARCH_INCREMENTALSEARCHMR:
		if(temporaryMacro_.state() == TemporaryMacro::DEFINING)
			return false;
		view.beginIncrementalSearch(searcher::MIGEMO, BACKWARD);
		return true;

	case CMD_VIEW_TOOLBAR: {
		CHECK_REBAR_BAND_VISIBILITY(0);
		app.rebar_.showBand(0, !visible);
		return true;
	}
	case CMD_VIEW_STATUSBAR:
		app.statusBar_.show(app.statusBar_.isVisible() ? SW_HIDE : SW_SHOW);
		app.onSize(SIZE_RESTORED, -1, -1);
		return true;
	case CMD_VIEW_BUFFERBAR: {
		CHECK_REBAR_BAND_VISIBILITY(1);
		app.rebar_.showBand(1, !visible);
		return true;
	}
	case CMD_VIEW_WRAPNO: {
		presentation::Presentation& p = buffer.presentation();
		for(presentation::Presentation::TextViewerIterator i = p.firstTextViewer(); i != p.lastTextViewer(); ++i) {
			if((*i)->configuration().lineWrap.mode == layout::LineWrapConfiguration::NONE)
				continue;
			viewers::TextViewer::Configuration c = (*i)->configuration();
			c.lineWrap.mode = layout::LineWrapConfiguration::NONE;
			(*i)->setConfiguration(&c, 0);
		}
		return true;
	}
//	case CMD_VIEW_WRAPBYSPECIFIEDWIDTH:	view.setWrapMode(WPM_SPECIFIED); return true;
	case CMD_VIEW_WRAPBYWINDOWWIDTH: {
		presentation::Presentation& p = buffer.presentation();
		for(presentation::Presentation::TextViewerIterator i = p.firstTextViewer(); i != p.lastTextViewer(); ++i) {
			if((*i)->configuration().lineWrap.mode == layout::LineWrapConfiguration::NORMAL)
				continue;
			viewers::TextViewer::Configuration c = (*i)->configuration();
			c.lineWrap.mode = layout::LineWrapConfiguration::NORMAL;
			(*i)->setConfiguration(&c, 0);
		}
		return true;
	}
	case CMD_VIEW_REFRESH:	view.invalidateRect(0); return true;
	case CMD_VIEW_NEXTBUFFER:
		if(app.bufferList().numberOfBuffers() > 1) {
			size_t i = app.bufferList().activeIndex();
			i = (i + 1 != app.bufferList().numberOfBuffers()) ? i + 1 : 0;
			app.bufferList().setActive(i);
		}
		return true;
	case CMD_VIEW_PREVBUFFER:
		if(app.bufferList().numberOfBuffers() > 1) {
			size_t i = app.bufferList().activeIndex();
			i = (i != 0) ? i - 1 : app.bufferList().numberOfBuffers() - 1;
			app.bufferList().setActive(i);
		}
		return true;

	case CMD_MACRO_DEFINE:
		if(!temporaryMacro_.isExecuting()) {
			if(!temporaryMacro_.isDefining())
				temporaryMacro_.startDefinition();
			else
				temporaryMacro_.endDefinition();
			return true;
		}
		return false;
	case CMD_MACRO_EXECUTE:
		if(!temporaryMacro_.isDefining() && !temporaryMacro_.isExecuting()) {
			temporaryMacro_.execute();
			return true;
		}
		return false;
	case CMD_MACRO_APPEND:
		if(!temporaryMacro_.isDefining() && !temporaryMacro_.isExecuting()) {
			temporaryMacro_.appendDefinition();
			return true;
		}
		return false;
	case CMD_MACRO_PAUSERESTART:
		if(temporaryMacro_.state() == TemporaryMacro::DEFINING)
			temporaryMacro_.pauseDefinition();
		else if(temporaryMacro_.state() == TemporaryMacro::PAUSING)
			temporaryMacro_.restartDefinition();
		else
			return false;
		return true;
	case CMD_MACRO_INSERTQUERY:
		if(temporaryMacro_.state() == TemporaryMacro::DEFINING) {
			temporaryMacro_.insertUserQuery();
			return true;
		}
		return false;
	case CMD_MACRO_ABORT:
		if(temporaryMacro_.isDefining()) {
			temporaryMacro_.cancelDefinition();
			return true;
		}
		return false;
	case CMD_MACRO_SAVEAS:
		if(!temporaryMacro_.isDefining() && !temporaryMacro_.isExecuting() && !temporaryMacro_.isEmpty()) {
			temporaryMacro_.showSaveDialog();
			return true;
		}
		return false;
	case CMD_MACRO_LOAD:
		if(!temporaryMacro_.isDefining() && !temporaryMacro_.isExecuting()) {
			temporaryMacro_.showLoadDialog();
			return true;
		}
		return false;

	case CMD_TOOL_COMMONOPTION:		return false;	// not (never) implemented
	case CMD_TOOL_DOCTYPEOPTION:	return false;	// not (never) implemented
	case CMD_TOOL_FONT:				app.changeFont(); return true;
//	case CMD_TOOL_EXECUTE:			app.onToolExecute(); return true;
	case CMD_TOOL_EXECUTECOMMAND:
		ui::ExecuteCommandDlg(
			toBoolean(app.readIntegerProfile(L"View", L"applyMainFontToSomeControls", 1)) ?
				app.editorFont_ : 0).doModal(app.getMainWindow());
		return true;
		
	case CMD_WINDOW_SPLITNS: {
		EditorPane& activePane = app.bufferList().editorWindow().getActivePane();
		app.bufferList().editorWindow().splitNS(activePane, *(new EditorPane(activePane)));
		return true;
	}
	case CMD_WINDOW_SPLITWE: {
		EditorPane& activePane = app.bufferList().editorWindow().getActivePane();
		app.bufferList().editorWindow().splitWE(activePane, *(new EditorPane(activePane)));
		return true;
	}
	case CMD_WINDOW_UNSPLITOTHERS:	app.bufferList().editorWindow().removeInactivePanes();	break;
	case CMD_WINDOW_UNSPLITACTIVE:	app.bufferList().editorWindow().removeActivePane();		break;
	case CMD_WINDOW_NEXTPANE:			app.bufferList().editorWindow().activateNextPane();		break;
	case CMD_WINDOW_PREVPANE:			app.bufferList().editorWindow().activatePreviousPane();	break;
	case CMD_WINDOW_TOPMOSTALWAYS:
		app.getMainWindow().setPosition(
			toBoolean(app.getMainWindow().getExStyle() & WS_EX_TOPMOST) ? HWND_NOTOPMOST : HWND_TOPMOST,
			0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		return true;

	case CMD_HELP_ABOUT:
		ui::AboutDialog().doModal(app.getMainWindow());
		return true;

	default:
		if(id >= CMD_SPECIAL_MRUSTART && id <= CMD_SPECIAL_MRUEND) {	// [最近使ったファイル]
			if(app.bufferList().open(app.mruManager_->at(id - CMD_SPECIAL_MRUSTART)) == BufferList::OPENRESULT_FAILED) {
				app.mruManager_->remove(id - CMD_SPECIAL_MRUSTART);
				return false;
			}
		} /*else if(id >= CMD_FILE_DOCTYPELIST_START && id < CMD_FILE_DOCTYPELIST_END) {	// [新規]
			app.bufferList().addNew();
			// TODO: class mode-application.
			app.applyDocumentType(app.bufferList().active(),
				app.bufferList().getDocumentTypeManager().getAt(id - CMD_FILE_DOCTYPELIST_START).name);
		} else if(id >= CMD_TOOL_DOCTYPELIST_START && id < CMD_TOOL_DOCTYPELIST_END)	// [適用文書タイプ]
			// TODO: class mode-application.
			app.applyDocumentType(app.bufferList().active(),
				app.bufferList().getDocumentTypeManager().getAt(id - CMD_TOOL_DOCTYPELIST_START).name);
		else if(id >= CMD_EDIT_PLUGINLIST_START && id < CMD_EDIT_PLUGINLIST_END) {	// [スクリプト]
			try {
				app.scriptMacroManager_->execute(id - CMD_EDIT_PLUGINLIST_START);
				return true;
			} catch(out_of_range&) {
				// 無視
			} catch(ScriptMacroManager::ScriptOpenFailureException& e) {
				app.messageBox(MSG_SCRIPT__FAILED_TO_OPEN_MACRO_SCRIPT, MB_ICONHAND, MARGS % e.fileName_);
			} catch(ScriptMacroManager::InvalidLanguageException& e) {
				app.messageBox(MSG_SCRIPT__INVALID_LANGUAGE_NAME, MB_ICONEXCLAMATION, MARGS % e.language_);
			}
//			app.getDocumentManager().getActiveDocument()->refreshWindow();
			return false;
		}
*/		lastCommandID_ = 0;	// 取り敢えず無効に...
//		app.bufferBar_.redrawWindow();
		return true;
	}
	return false;
}

/**
 * Returns true if the command is checked.
 * @param id the identifier of the command
 * @return true if checked
 */
bool CommandManager::isChecked(CommandID id) const {
	Alpha& app = Alpha::instance();
	if(id >= CMD_SPECIAL_BUFFERSSTART && id < CMD_SPECIAL_BUFFERSEND)
		return id - CMD_SPECIAL_BUFFERSSTART == app.bufferList().activeIndex();

	switch(id) {
	case CMD_SEARCH_FIND:
		return app.searchDialog_->isVisible();
	case CMD_SEARCH_MANAGEBOOKMARKS:
		return app.bookmarkDialog_->isVisible();

	case CMD_VIEW_TOOLBAR: {
		CHECK_REBAR_BAND_VISIBILITY(0);
		return visible;
	}
	case CMD_VIEW_BUFFERBAR: {
		CHECK_REBAR_BAND_VISIBILITY(1);
		return visible;
	}
	case CMD_VIEW_STATUSBAR:
		return app.statusBar_.isVisible();
	case CMD_VIEW_WRAPNO:
		return app.bufferList().activeView().configuration().lineWrap.mode == layout::LineWrapConfiguration::NONE;
//	case CMD_VIEW_WRAPBYSPECIFIEDWIDTH:
//		return app.bufferList().activeView().getLayoutSetter().getSettings().wrapMode == WPM_SPECIFIED;
	case CMD_VIEW_WRAPBYWINDOWWIDTH:
		return app.bufferList().activeView().configuration().lineWrap.mode != layout::LineWrapConfiguration::NONE;

	case CMD_MACRO_DEFINE:			return temporaryMacro_.isDefining();
	case CMD_MACRO_EXECUTE:			return temporaryMacro_.isExecuting();
	case CMD_MACRO_PAUSERESTART:	return temporaryMacro_.state() == TemporaryMacro::PAUSING;

	case CMD_WINDOW_TOPMOSTALWAYS:
		return toBoolean(app.getMainWindow().getExStyle() & WS_EX_TOPMOST);

	default:
		return false;
	}
}

/**
 * Returns true if the command is executable.
 * @param id the identifier of the command
 * @param userContext set true if an end user tries
 * @return true if enabled
 */
bool CommandManager::isEnabled(CommandID id, bool userContext) const {
	// 一時マクロに記録できない操作は最初から実行しない
	if(temporaryMacro_.isDefining() && !isRecordable(id) && (id < CMD_MACRO_DEFINE || id > CMD_MACRO_LOAD))
		return false;
//	if(app.scriptMacroManager_->isExecuting()
//			&& (userContext || (id >= CMD_MACRO_DEFINE && id < CMD_MACRO_DEFINE + 1000)))
//		return false;

	Alpha& app = Alpha::instance();
	const EditorView& view = app.bufferList().activeView();
	const Buffer& buffer = view.document();
	const bool modified = buffer.isModified();
	const bool readOnly = buffer.isReadOnly();
	const bool hasSelection = !view.caret().isSelectionEmpty();

	switch(id) {
		// ファイル
	case CMD_FILE_SAVE:
		return modified && !readOnly;
	case CMD_FILE_SAVEALL:
		for(size_t i = 0; i < app.bufferList().numberOfBuffers(); ++i) {
			if(app.bufferList().at(i).isModified())
				return true;
		}
		return false;
	case CMD_FILE_REOPEN:
	case CMD_FILE_REOPENWITHCODEPAGE:
		return buffer.textFile().isOpen();
	case CMD_FILE_SENDMAIL:
		return buffer.textFile().isOpen()
			|| !toBoolean(app.readIntegerProfile(L"File", L"sendMailAsAttachment", 1));
	case CMD_FILE_CLOSEOTHERS:
		return app.bufferList().numberOfBuffers() > 1;

		// 編集
	case CMD_EDIT_DELETE:
	case CMD_EDIT_BACKSPACE:
	case CMD_EDIT_DELETETONEXTWORD:
	case CMD_EDIT_DELETETOPREVWORD:
	case CMD_EDIT_DELETELINE:
	case CMD_EDIT_INSERTPREVLINE:
	case CMD_EDIT_BREAK:
		return !readOnly;
	case CMD_EDIT_UNDO:
		return !readOnly && buffer.numberOfUndoableEdits() != 0;
	case CMD_EDIT_REDO:
		return !readOnly && buffer.numberOfRedoableEdits() != 0;
	case CMD_EDIT_CUT:
		return !readOnly && hasSelection;
	case CMD_EDIT_COPY:
		return hasSelection;
	case CMD_EDIT_PASTE:
		return !readOnly && view.caret().canPaste() != 0;
	case CMD_EDIT_PASTEFROMCLIPBOARDRING:
		return !readOnly && app.bufferList().editorSession().clipboardRing().numberOfItems() != 0;
	case CMD_EDIT_INSERTTAB:
	case CMD_EDIT_DELETETAB:
		return !readOnly;
	case CMD_EDIT_TABIFY:
	case CMD_EDIT_UNTABIFY:
		return !readOnly && hasSelection;
	case CMD_EDIT_CHARTOCODEPOINT:
	case CMD_EDIT_CODEPOINTTOCHAR:
	case CMD_EDIT_RECOMPOSE:
		return !readOnly;
	case CMD_EDIT_OPENCANDIDATEWINDOW:
	case CMD_EDIT_TRANSPOSELINES:
	case CMD_EDIT_TRANSPOSECHARS:
	case CMD_EDIT_TRANSPOSEWORDS:
		return !readOnly && !hasSelection;
	case CMD_EDIT_NARROWTOSELECTION:
		return hasSelection;
	case CMD_EDIT_WIDEN:
		return buffer.isNarrowed();

		// 検索
	case CMD_SEARCH_FINDNEXT:
	case CMD_SEARCH_FINDPREV:
		return app.bufferList().editorSession().incrementalSearcher().isRunning()
			|| (app.searchDialog_->isWindow() &&
				::GetWindowTextLengthW(app.searchDialog_->getItem(IDC_COMBO_FINDWHAT)) != 0);
	case CMD_SEARCH_REPLACEALLINTERACTIVE:
	case CMD_SEARCH_REPLACEALL:
		return !readOnly && app.searchDialog_->isWindow() &&
			(::GetWindowTextLengthW(app.searchDialog_->getItem(IDC_COMBO_FINDWHAT)) != 0);
	case CMD_SEARCH_REVOKEMARK:
		return false;
	case CMD_SEARCH_INCREMENTALSEARCHRF:
	case CMD_SEARCH_INCREMENTALSEARCHRR:
	case CMD_SEARCH_INCREMENTALSEARCHMF:
	case CMD_SEARCH_INCREMENTALSEARCHMR:
		return searcher::TextSearcher::isRegexAvailable();
	case CMD_SEARCH_FINDFILES:
	case CMD_SEARCH_SEARCHMULTIPLEFILES:
	case CMD_SEARCH_REPLACEMULTIPLEFILES:
		return false;

		// 表示
	case CMD_VIEW_WRAPNO:
		return true;
	case CMD_VIEW_WRAPBYSPECIFIEDWIDTH:
		return false;
	case CMD_VIEW_WRAPBYWINDOWWIDTH:
		return true;
	case CMD_VIEW_NEXTBUFFER:
	case CMD_VIEW_PREVBUFFER:
		return app.bufferList().numberOfBuffers() > 1;

		// マクロ
	case CMD_MACRO_DEFINE:			return !temporaryMacro_.isExecuting();
	case CMD_MACRO_EXECUTE:
	case CMD_MACRO_APPEND:			return !temporaryMacro_.isDefining() && !temporaryMacro_.isExecuting() && !temporaryMacro_.isEmpty();
	case CMD_MACRO_PAUSERESTART:	return temporaryMacro_.isDefining();
	case CMD_MACRO_INSERTQUERY:		return temporaryMacro_.state() == TemporaryMacro::DEFINING;
	case CMD_MACRO_ABORT:			return temporaryMacro_.isDefining();
	case CMD_MACRO_SAVEAS:			return !temporaryMacro_.isDefining() && !temporaryMacro_.isExecuting() && !temporaryMacro_.isEmpty();
	case CMD_MACRO_LOAD:			return !temporaryMacro_.isDefining() && !temporaryMacro_.isExecuting();
		return false;

		// ツール
	case CMD_TOOL_COMMONOPTION:
	case CMD_TOOL_DOCTYPEOPTION:
		return false;

		// ウィンドウ
	case CMD_WINDOW_UNSPLITACTIVE:
	case CMD_WINDOW_UNSPLITOTHERS:
	case CMD_WINDOW_NEXTPANE:
	case CMD_WINDOW_PREVPANE:
		return app.bufferList().editorWindow().isSplit(app.bufferList().editorWindow().getActivePane());

	default:
		return true;
	}
}

/**
 * Returns true if the command is recordable to the temporary macro.
 * @param id the identifier of the command
 * @return true if recordable
 */
bool CommandManager::isRecordable(CommandID id) const {
	switch(id) {
	case CMD_FILE_SENDMAIL:

	case CMD_EDIT_OPENCANDIDATEWINDOW: case CMD_EDIT_SHOWABBREVIATIONDLG:

	case CMD_SEARCH_GOTOLINE: case CMD_SEARCH_MANAGEBOOKMARKS:
	case CMD_SEARCH_INCREMENTALSEARCH: case CMD_SEARCH_INCREMENTALSEARCHR:
	case CMD_SEARCH_INCREMENTALSEARCHRF: case CMD_SEARCH_INCREMENTALSEARCHRR:
	case CMD_SEARCH_INCREMENTALSEARCHMF: case CMD_SEARCH_INCREMENTALSEARCHMR:

	case CMD_VIEW_TOOLBAR: case CMD_VIEW_STATUSBAR: case CMD_VIEW_REFRESH:
	case CMD_VIEW_BUFFERBAR:

	case CMD_MACRO_DEFINE: case CMD_MACRO_EXECUTE: case CMD_MACRO_APPEND:
	case CMD_MACRO_PAUSERESTART: case CMD_MACRO_INSERTQUERY: case CMD_MACRO_ABORT:
	case CMD_MACRO_SAVEAS: case CMD_MACRO_LOAD:

	case CMD_TOOL_FONT: case CMD_TOOL_EXECUTE: case CMD_TOOL_EXECUTECOMMAND:
	
	case CMD_WINDOW_TOPMOSTALWAYS:

	case CMD_HELP_ABOUT:
		return false;
	default:
		if(id >= CMD_SPECIAL_BUFFERSSTART && id <= CMD_SPECIAL_BUFFERSEND)
			return false;
		return true;
	}
}

/**
 * Returns the text of the command to display as a menu item.
 * @param id the identifier of the command
 */
wstring CommandManager::menuName(CommandID id) const {
	static wchar_t buffer[MAX_PATH * 2];

	// [マクロ]
/*	if(id >= CMD_EDIT_PLUGINLIST_START && id < CMD_EDIT_PLUGINLIST_END) {
		if(scriptMacroManager_->getCount() != 0) {
			wcscpy(buffer, scriptMacroManager_->getName(id - CMD_EDIT_PLUGINLIST_START).c_str());
			wcscat(buffer, L"\t");
			wcscat(buffer, keyboardMap_.getKeyString(id, useShortKeyNames_).c_str());
		} else
			wcscpy(buffer, loadMessage(MSG_ERROR__FAILED_TO_LOAD_SOMETHING).c_str());
		return buffer;
	}
	
	else*/ if(id >= CMD_SPECIAL_MRUSTART && id <= CMD_SPECIAL_MRUEND) {
		swprintf(buffer, L"&%X  %s", id - CMD_SPECIAL_MRUSTART,
			Alpha::instance().mruManager().at(id - CMD_SPECIAL_MRUSTART).c_str());
		return buffer;
	}
	
	else if(id >= CMD_SPECIAL_BUFFERSSTART && id <= CMD_SPECIAL_BUFFERSEND) {
		const Buffer& document = Alpha::instance().bufferList().at(id - CMD_SPECIAL_BUFFERSSTART);
		if(id - CMD_SPECIAL_BUFFERSSTART < 0x10)
			swprintf(buffer, L"&%X  ", id - CMD_SPECIAL_BUFFERSSTART);
		else
			buffer[0] = 0;
		wcscat(buffer, document.name().c_str());
		return buffer;
	}

	wstring s(Alpha::instance().loadMessage(id));
	const size_t eol = s.find_first_of(L"\n\r");
	if(eol != wstring::npos)
		s.resize(eol);
	const wstring keys = Alpha::instance().keyboardMap().keyString(id);
	if(!keys.empty()) {
		s += L"\t";
		s += keys;
	}
	return s;
}

/**
 * Returns the name of the specified command.
 * @param id the identifier of the command
 * @return the human-readable name
 */
wstring CommandManager::name(CommandID id) const {
	wstring s(Alpha::instance().loadMessage(id));
	const size_t eol = s.find_first_of(L"\n\r");
	if(eol != wstring::npos) {
		s.resize(eol);
		// CJK アクセスキー
		if(s[eol - 1] == L')' || (s.length() > 4 && s.compare(eol - 4, 4, L")...") == 0)) {
			const size_t opener = s.rfind(L'(');
			if(opener != wstring::npos)
				s.resize(opener);
		}
	}

	// '&' を取り除く
	const size_t amp = s.find(L'&');
	if(amp != wstring::npos)
		return s.substr(0, amp) + s.substr(amp + 1);
	else
		return s;
}

#undef CHECK_REBAR_BAND_VISIBILITY
