/**
 * @file session.hpp
 * @author exeal
 * @date 2006-2013
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

	namespace kernel {
		class Document;
	}

	namespace searcher {
		class IncrementalSearcher;
		class TextSearcher;
	}

	namespace viewers {
		class TextViewer;
	}

	namespace texteditor {
		class InputSequenceCheckers;
		class KillRing;

		/**
		 * @note This class is not intended to be subclassed.
		 */
		class Session {
			ASCENSION_NONCOPYABLE_TAG(Session);
		public:
			/// @name Document Collection
			/// @{
			void addDocument(kernel::Document& document);
			const std::vector<kernel::Document*> documents() const BOOST_NOEXCEPT;
			void removeDocument(kernel::Document& document);
			/// @}

			/// @name Other Attributes
			/// @{
			searcher::IncrementalSearcher& incrementalSearcher() BOOST_NOEXCEPT;
			const searcher::IncrementalSearcher& incrementalSearcher() const BOOST_NOEXCEPT;
			std::shared_ptr<InputSequenceCheckers> inputSequenceCheckers() BOOST_NOEXCEPT;
			std::shared_ptr<const InputSequenceCheckers> inputSequenceCheckers() const BOOST_NOEXCEPT;
			KillRing& killRing() BOOST_NOEXCEPT;
			const KillRing& killRing() const BOOST_NOEXCEPT;
#ifndef ASCENSION_NO_MIGEMO
			const kernel::fileio::PathString& migemoDictionaryPathName() BOOST_NOEXCEPT;
			const kernel::fileio::PathString& migemoLibraryPathName() BOOST_NOEXCEPT;
#endif // !ASCENSION_NO_MIGEMO
			searcher::TextSearcher& textSearcher() BOOST_NOEXCEPT;
			const searcher::TextSearcher& textSearcher() const BOOST_NOEXCEPT;
			void setInputSequenceCheckers(std::unique_ptr<InputSequenceCheckers> isc) BOOST_NOEXCEPT;
#ifndef ASCENSION_NO_MIGEMO
			void setMigemoDictionaryPathName(const kernel::fileio::PathString& pathName);
			void setMigemoLibraryPathName(const kernel::fileio::PathString& pathName);
#endif // !ASCENSION_NO_MIGEMO
			/// @}

		private:
			std::vector<kernel::Document*> documents_;
			KillRing killRing_;
			std::unique_ptr<searcher::IncrementalSearcher> isearch_;
			std::unique_ptr<searcher::TextSearcher> textSearcher_;
			std::shared_ptr<InputSequenceCheckers> inputSequenceCheckers_;
#ifndef ASCENSION_NO_MIGEMO
			kernel::fileio::PathString migemoDictionaryPathName_, migemoLibraryPathName_;
#endif // !ASCENSION_NO_MIGEMO
		};

		bool abortIncrementalSearch(viewers::TextViewer& viewer) BOOST_NOEXCEPT;
		bool endIncrementalSearch(viewers::TextViewer& viewer) BOOST_NOEXCEPT;


		/// Returns the input sequence checkers.
		inline std::shared_ptr<InputSequenceCheckers> Session::inputSequenceCheckers() BOOST_NOEXCEPT {
			return inputSequenceCheckers_;
		}

		/// Returns the input sequence checkers.
		inline std::shared_ptr<const InputSequenceCheckers> Session::inputSequenceCheckers() const BOOST_NOEXCEPT {
			return inputSequenceCheckers_;
		}
	} // namespace texteditor
} // namespace ascension

#endif // !ASCENSION_SESSION_HPP
