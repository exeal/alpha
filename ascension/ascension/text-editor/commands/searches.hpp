/**
 * @file searches.hpp
 * @author exeal
 * @date 2006-2011 was text-editor.hpp
 * @date 2011-05-06
 * @date 2011-2013, 2015
 * @date 2016-11-05 Separated from command.hpp.
 */

#ifndef ASCENSION_SEARCHES_HPP
#define ASCENSION_SEARCHES_HPP
#include <ascension/kernel/searcher.hpp>
#include <ascension/text-editor/command.hpp>

namespace ascension {
	namespace searcher {
		class InteractiveReplacementCallback;
	}

	namespace texteditor {
		namespace commands {
			/// Searches and bookmarks all matched lines.
			class BookmarkMatchLinesCommand : public Command {
			public:
				explicit BookmarkMatchLinesCommand(viewer::TextViewer& viewer,
					const kernel::Region& region = kernel::Region()) BOOST_NOEXCEPT;
				Index numberOfMarkedLines() const BOOST_NOEXCEPT;
			private:
				bool perform();
				const kernel::Region region_;
				Index numberOfMarkedLines_;
			};

			/**
			 * Searches the next/previous or the previous match and selects matched region. The search performs using
			 * the current search conditions.
			 *
			 * To find the next/previous for the incremental search, use
			 * @c IncrementalSearchCommand instead.
			 */
			class FindNextCommand : public Command {
			public:
				FindNextCommand(viewer::TextViewer& viewer, Direction direction) BOOST_NOEXCEPT;
			private:
				bool perform();
				const Direction direction_;
			};

			/// Begins the incremental search. If the search is already running, jumps to the next/previous match.
			class IncrementalFindCommand : public Command {
			public:
				IncrementalFindCommand(viewer::TextViewer& view, searcher::TextSearcher::Type type,
					Direction direction, searcher::IncrementalSearchCallback* callback = nullptr) BOOST_NOEXCEPT;
			private:
				bool perform();
				searcher::TextSearcher::Type type_;
				const Direction direction_;
				searcher::IncrementalSearchCallback* const callback_;
			};

			/// Moves the caret or extends the selection to the match bracket.
			class MatchBracketCommand : public Command {
			public:
				MatchBracketCommand(viewer::TextViewer& viewer, bool extendSelection = false) BOOST_NOEXCEPT;
			private:
				bool perform();
				const bool extends_;
			};

			/// Replaces all matched texts.
			class ReplaceAllCommand : public Command {
			public:
				ReplaceAllCommand(viewer::TextViewer& viewer, bool onlySelection,
					const String& replacement, searcher::InteractiveReplacementCallback* callback) BOOST_NOEXCEPT;
				std::size_t numberOfLastReplacements() const BOOST_NOEXCEPT;
			private:
				bool perform();
				const bool onlySelection_;
				const String replacement_;
				searcher::InteractiveReplacementCallback* const callback_;
				std::size_t numberOfLastReplacements_;
			};
		}
	}
}

#endif // !ASCENSION_SEARCHES_HPP
