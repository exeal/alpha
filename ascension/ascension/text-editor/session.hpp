/**
 * @file session.hpp
 * @author exeal
 * @date 2006-2011
 */

#ifndef ASCENSION_SESSION_HPP
#define ASCENSION_SESSION_HPP

#include <ascension/config.hpp>				// ASCENSION_NO_MIGEMO
#include <ascension/corelib/text/character.hpp>
#include <ascension/text-editor/kill-ring.hpp>
#ifndef ASCENSION_NO_MIGEMO
#	include <ascension/kernel/fileio.hpp>	// fileio.PathCharacter
#endif // !ASCENSION_NO_MIGEMO
#include <memory>
#include <vector>

namespace ascension {

	namespace kernel {class Document;}

	namespace searcher {
		class IncrementalSearcher;
		class TextSearcher;
	}

	namespace viewers {class TextViewer;}

	namespace texteditor {

		class InputSequenceCheckers;
		class KillRing;

		/**
		 * @note This class is not derivable.
		 */
		class Session {
			ASCENSION_NONCOPYABLE_TAG(Session);
		public:
			// constructor
			Session() /*throw()*/;
			~Session() /*throw()*/;
			// attributes
			const std::vector<kernel::Document*> documents() const /*throw()*/;
			searcher::IncrementalSearcher& incrementalSearcher() /*throw()*/;
			const searcher::IncrementalSearcher& incrementalSearcher() const /*throw()*/;
			InputSequenceCheckers* inputSequenceCheckers() /*throw()*/;
			const InputSequenceCheckers* inputSequenceCheckers() const /*throw()*/;
			KillRing& killRing() /*throw()*/;
			const KillRing& killRing() const /*throw()*/;
#ifndef ASCENSION_NO_MIGEMO
			const kernel::fileio::PathString& migemoPathName(bool runtime) /*throw()*/;
#endif // !ASCENSION_NO_MIGEMO
			searcher::TextSearcher& textSearcher() /*throw()*/;
			const searcher::TextSearcher& textSearcher() const /*throw()*/;
			void setInputSequenceCheckers(std::unique_ptr<InputSequenceCheckers> isc) /*throw()*/;
#ifndef ASCENSION_NO_MIGEMO
			void setMigemoPathName(const kernel::fileio::PathString& pathName, bool runtime);
#endif // !ASCENSION_NO_MIGEMO
			// operations
			void addDocument(kernel::Document& document);
			void removeDocument(kernel::Document& document);

		private:
			std::vector<kernel::Document*> documents_;
			KillRing killRing_;
			std::unique_ptr<searcher::IncrementalSearcher> isearch_;
			std::unique_ptr<searcher::TextSearcher> textSearcher_;
			std::unique_ptr<InputSequenceCheckers> inputSequenceCheckers_;
#ifndef ASCENSION_NO_MIGEMO
			kernel::fileio::PathString migemoRuntimePathName_, migemoDictionaryPathName_;
#endif // !ASCENSION_NO_MIGEMO
		};

		bool abortIncrementalSearch(viewers::TextViewer& viewer) /*throw()*/;
		bool endIncrementalSearch(viewers::TextViewer& viewer) /*throw()*/;


		/// Returns the input sequence checkers.
		inline InputSequenceCheckers* Session::inputSequenceCheckers() /*throw()*/ {
			return inputSequenceCheckers_.get();
		}

		/// Returns the input sequence checkers.
		inline const InputSequenceCheckers* Session::inputSequenceCheckers() const /*throw()*/ {
			return inputSequenceCheckers_.get();
		}

	} // namespace texteditor
} // namespace ascension

#endif // !ASCENSION_SESSION_HPP
