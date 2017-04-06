/**
 * @file application.hpp
 * @author exeal
 * @date 2003-2009, 2014-2015
 * @date 2017-02-20 Separated from application.cpp.
 */

#include "application.hpp"
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#include "ambient.hpp"
#include "buffer-list.hpp"
#include "input.hpp"
//#include "ui.hpp"
#include "editor-pane.hpp"
#include "editor-panes.hpp"
#include "editor-view.hpp"
#include "function-pointer.hpp"
//#include "search.hpp"
#include "ui/main-window.hpp"
#include <ascension/graphics/font/font.hpp>
//#include <ascension/graphics/native-conversion.hpp>
#include <ascension/viewer/text-area.hpp>
#include <boost/property_tree/ptree.hpp>
#include <CommDlg.h>	// ChooseFontW
#include <Dlgs.h>

namespace alpha {
	std::shared_ptr<Application> Application::instance_;

	Application::Application(std::unique_ptr<ui::MainWindow> window) : win32::WindowApplication<ui::MainWindow>(std::move(window)) {
		if(instance_ != nullptr)
			throw ascension::IllegalStateException("");
		instance_ = std::shared_ptr<Application>(this, boost::null_deleter());
	}

	Application::~Application() BOOST_NOEXCEPT {
		assert(instance_ != nullptr);
		instance_.reset();
	}

	void Application::changeFont() {
#if 0
		EditorView& activeView = EditorPanes::instance().activePane().selectedView();
		LOGFONTW font;
		auto cf(ascension::win32::makeZeroSize<CHOOSEFONTW>());

		::GetObjectW(editorFont_, sizeof(decltype(font)), &font);
		cf.hwndOwner = mainWindow().handle().get();
		cf.lpLogFont = &font;
		cf.lpfnHook = chooseFontHookProc;
		cf.Flags = CF_APPLY | CF_ENABLEHOOK | CF_INITTOLOGFONTSTRUCT | CF_NOVERTFONTS | CF_SCREENFONTS;
		cf.hInstance = handle().get();

		if(ascension::win32::boole(::ChooseFontW(&cf))) {
			font.lfItalic = false;
			font.lfWeight = FW_REGULAR;
			setFont(font);
		}
#endif
	}

	/// 全てのエディタと一部のコントロールに新しいフォントを設定
	void Application::setFont(const ascension::graphics::font::FontDescription& font) {
#if 0
		LOGFONTW lf = font;

		lf.lfWeight = FW_NORMAL;
		editorFont_ = ::CreateFontIndirectW(&lf);

		// update the all presentations
		const int ydpi = win32::gdi::ScreenDC().getDeviceCaps(LOGPIXELSY); 
		BufferList& buffers = BufferList::instance();
		for(size_t i = 0; i < buffers.numberOfBuffers(); ++i) {
			ascension::presentation::Presentation& p = buffers.at(i).presentation();
			std::shared_ptr<const ascension::presentation::RunStyle> defaultStyle(p.defaultTextRunStyle());
			std::unique_ptr<ascension::presentation::RunStyle> newDefaultStyle(
				(defaultStyle.get() != 0) ? new ascension::presentation::RunStyle(*defaultStyle) : new ascension::presentation::RunStyle);
			newDefaultStyle->fontFamily = lf.lfFaceName;
			newDefaultStyle->fontProperties.weight = static_cast<ascension::presentation::FontProperties::Weight>(lf.lfWeight);
			newDefaultStyle->fontProperties.style = (lf.lfItalic != 0) ?
				ascension::presentation::FontProperties::ITALIC : presentation::FontProperties::NORMAL_STYLE;
			newDefaultStyle->fontProperties.size = lf.lfHeight * 96.0 / ydpi;
			newDefaultStyle->fontSizeAdjust = 0.0;
			p.setDefaultTextRunStyle(std::shared_ptr<const ascension::presentation::RunStyle>(newDefaultStyle.release()));
		}

		// 一部のコントロールにも設定
		if(settings().get("view.applyMainFontToSomeControls", true)) {
//			if(bookmarkDialog_.get() != 0 && bookmarkDialog_->isWindow())
//				bookmarkDialog_->sendItemMessage(IDC_LIST_BOOKMARKS, WM_SETFONT, reinterpret_cast<WPARAM>(editorFont_), true);
//			if(searchDialog_.get() != 0 && searchDialog_->isWindow()) {
//				searchDialog_->sendItemMessage(IDC_COMBO_FINDWHAT, WM_SETFONT, reinterpret_cast<WPARAM>(editorFont_), true);
//				searchDialog_->sendItemMessage(IDC_COMBO_REPLACEWITH, WM_SETFONT, reinterpret_cast<WPARAM>(editorFont_), true);
//			}
		}

		// INI ファイルに保存
		settings().put("view.font.default", lf);

		// 等幅 <-> 可変幅で表記を変える必要がある
		mainWindow().statusBar().adjustPaneWidths();
#endif
	}
}

#endif
