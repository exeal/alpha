/**
 * @file content-assist.hpp
 * @author exeal
 * @date 2003-2006 (was CompletionWindow.h)
 * @date 2006-2007
 */

#ifndef ASCENSION_CONTENT_ASSIST_HPP
#define ASCENSION_CONTENT_ASSIST_HPP
#include "document.hpp"
#include "../../manah/win32/ui/standard-controls.hpp"	// manah::windows::ui::ListBox
#include <set>


namespace ascension {

	namespace viewers {
		class SourceViewer;
		class VisualPoint;
	}

	namespace contentassist {

		/// A completion window.
		class CompletionWindow : public manah::windows::ui::ListBox {
		public:
			// constructors
			explicit CompletionWindow(viewers::SourceViewer& viewer);
			virtual ~CompletionWindow();
			// construction
			bool	create();
			// attributes
			text::Region	getContextRegion() const;
			bool			isRunning() const throw();
			void			setFont(const HFONT font);
			// operations
			void	abort();
			void	complete();
			bool	start(const std::set<String>& candidateWords);
			bool	updateListCursel();

		protected:
			virtual LRESULT	dispatchEvent(UINT message, WPARAM wParam, LPARAM lParam);
		private:
			void	updateDefaultFont();

		private:
			viewers::SourceViewer& viewer_;
			HFONT defaultFont_;
			bool running_;
			text::Position contextStart_;		// 補完開始位置の前方の単語先頭
			viewers::VisualPoint* contextEnd_;	// 補完開始位置の後方の単語終端
		};


		/// Returns if the completion is running.
		inline bool CompletionWindow::isRunning() const throw() {return running_;}

}} // namespace ascension::contentassist

#endif /* ASCENSION_CONTENT_ASSIST_HPP */
