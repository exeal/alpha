/**
 * @file searcher.hpp
 * @author exeal
 * @date 2004-2006 (was TextSearcher.h)
 * @date 2006-2007
 */

#ifndef ASCENSION_SEARCHER_HPP
#define ASCENSION_SEARCHER_HPP
#include "unicode-utils.hpp"
#include "document.hpp"
#include <stdexcept>	// std::logic_error


namespace ascension {

	namespace viewers {class Caret;}

	namespace searcher {

		/**
		 * @c MatchTarget represents the text to be matched.
		 * @see IPatternMatcher, TextSearcher
		 */
		struct MatchTarget {
			const Char* first;			///< the start of the portion to be matched
			const Char* last;			///< the end of the portion to be matched
			const Char* entireFirst;	///< the start of the whole target
			const Char* entireLast;		///< the end of the whole target
			/// Default constructor.
			MatchTarget() throw() {}
			/// Constructor.
			MatchTarget(const Char* ef, const Char* el, const Char* f, const Char* l) throw()
				: first(f), last(l), entireFirst(ef), entireLast(el) {assert(isNormalized());}
			/// Constructor.
			MatchTarget(const Char* f, const Char* l) throw() : first(f), last(l), entireFirst(f), entireLast(l) {assert(isNormalized());}
			/// Returns true if the members are normalized.
			bool isNormalized() const throw() {
				return first <= last && entireFirst <= entireLast && first >= entireFirst && last <= entireLast;}
		};

		/// Matched range of the target string.
		struct MatchedRange {
			const Char* first;	///< the start of the range
			const Char* last;	///< the end of the range
		};

		namespace internal {
			class IPatternMatcher {
			public:
				/// Destructor.
				virtual ~IPatternMatcher() throw() {}
				/// Returns the pattern string.
				virtual const String& getPattern() const throw() = 0;
				/**
				 * Performs pattern match.
				 * @param target the target to match
				 * @param ctypes the character detector for boundary definition or @c null if not needed
				 * @return true if the match was succeeded
				 */
				virtual	bool matches(const MatchTarget& target, const unicode::IdentifierSyntax* ctypes) const = 0;
				/**
				 * Searches the pattern.
				 * @param target the target to search
				 * @param direction the direction to search
				 * @param[out] result the matched range
				 * @param[in] ctypes the character detector for boundary definition or @c null if not needed
				 * @return true if the pattern was found
				 */
				virtual bool search(const MatchTarget& target,
					Direction direction, MatchedRange& result, const unicode::IdentifierSyntax* ctypes) const = 0;
				/**
				 *
				 * @param target the target to match and replace
				 * @param[in,out] replacement the replacement format. this parameter receives replaced string
				 * @param[in] ctypes the character detector for boundary definition or @c null if not needed
				 * @return true if the replacement was succeeded
				 */
				virtual bool replace(const MatchTarget& target, String& replacement, const unicode::IdentifierSyntax* ctypes) const = 0;
			};
		} // namespace internal

/*		/// 日本語曖昧検索フラグ (同一視する文字種、表記など。MS Word のパクリ)。
		/// 幾つかのフラグは正規表現検索時には使用できない
		typedef ushort	JapaneseFuzzySearchFlag;
		const JapaneseFuzzySearchFlag
			JFSF_KANATYPE					= 0x0001,	///< 平仮名/片仮名
			JFSF_YOUON_SOKUON				= 0x0002,	///< 拗音/促音
			JFSF_MINUS_PROLONGEDMARK_DASH	= 0x0004,	///< マイナス/長音/ダッシュ
			JFSF_ITERATIONMARK				= 0x0008,	///< 繰り返し記号
			JFSF_UNUNIFIEDKANJI				= 0x0010,	///< 漢字表記のゆれ
			JFSF_LEGACY_MODERN_KANAFIGURE	= 0x0020,	///< 仮名の新字体/旧字体
			JFSF_PROLONGEDMARK_VOWEL		= 0x0040,	///< 長音と母音
			JFSF_DI_JI_DU_ZU				= 0x0080,	///< ヂ/ジ、ヅ/ズ
			JFSF_BA_VA_HA_FA				= 0x0100,	///< バ/ヴァ、ハ/ファ
			JFSF_TSI_THI_TI_DHI_JI			= 0x0200,	///< ツィ/ティ/チ、ディ/ジ
			JFSF_HYU_FYU_BYU_VYU			= 0x0400,	///< ヒュ/フュ、ビュ/ヴュ
			JFSF_SE_SYE_ZE_JE				= 0x0800,	///< セ/シェ、ゼ/ジェ
			JFSF_A_YA_FOLLOWING_I_E			= 0x1000,	///< イ段、エ段に続くア/ヤ
			JFSF_KI_KU_FOLLOWEDBY_S			= 0x2000;	///< サ行の前のキ/ク
*/

		/**
		 * @brief テキスト検索を行う
		 *
		 * エディタはこのクラスのインスタンスを保持しており、あらゆる検索操作にこのオブジェクトを使用する。
		 * クライアントはエディタが保持しているオブジェクトを @c Session#getTextSearcher
		 * メソッドで得ることができる他、自分でインスタンスを作成することもできる
		 */
		class TextSearcher : public manah::Noncopyable {
			// 型
		public:
			/// types of search
			enum Type {
				LITERAL,			///< literal search
#ifndef ASCENSION_NO_REGEX
				REGULAR_EXPRESSION,	///< regular expression search
#endif /* !ASCENSION_NO_REGEX */
				WILD_CARD,			///< wild card search (not implemented)
#ifndef ASCENSION_NO_MIGEMO
				MIGEMO				///< Migemo
#endif /* !ASCENSION_NO_MIGEMO */
			};

			/// Search options.
			struct Options {
				bool wholeWord;						///< set true to whole-word match
				Type type;							///< type of search
				unicode::FoldingOptions foldings;	///< options for foldings
				/// Constructor.
				Options() throw() : wholeWord(false), type(LITERAL) {}
				/// Equality operator.
				bool operator==(const Options& rhs) const throw() {
					return wholeWord == rhs.wholeWord
						&& type == rhs.type
						&& foldings.caseFolding == rhs.foldings.caseFolding
						&& foldings.others == rhs.foldings.others
					;
				}
				/// Unequality operator.
				bool operator!=(const Options& rhs) const throw() {return !(*this == rhs);}
			};

		public:
			// constructors.
			TextSearcher();
			virtual ~TextSearcher();
			// attributes
			const Options&	getOptions() const throw();
			const String&	getPattern() const throw();
			const String&	getReplacement() const throw();
			bool			isChangedSinceLastSearch() const throw();
			bool			isMigemoAvailable() const throw();
			bool			isMultilinePattern() const throw();
			static bool		isRegexAvailable() throw();
			void			setOptions(const Options& options);
			void			setPattern(const String& pattern);
			void			setReplacement(const String& replacemnt) throw();
			// operations
			bool	match(const MatchTarget& target, const unicode::IdentifierSyntax& ctypes) const;
			bool	replace(const MatchTarget& target, String& replaced, const unicode::IdentifierSyntax& ctypes) const;
			bool	search(const MatchTarget& target, Direction direction,
						MatchedRange& result, const unicode::IdentifierSyntax& ctypes) const;
		private:
			void	clearPatternCache();
			void	compilePattern() const throw();
		private:
			std::auto_ptr<internal::IPatternMatcher> matcher_;
			String patternString_;	// 検索文字列
			String replacement_;	// 置換文字列
			Options options_;		// 検索オプション
			bool changedFromLast_;
			bool multilinePattern_;
		};

		/// A @c DocumentSearcher searches text in a document.
		class DocumentSearcher {
		public:
			// constructor
			DocumentSearcher(const text::Document& document, const TextSearcher& searcher, const unicode::IdentifierSyntax& ctypes) throw();
			// operations
			bool	replace(const text::Region& target, String& result) const;
			bool	search(const text::Region& target, Direction direction, text::Region& result) const;
		private:
			/// 複数行マッチのために検索対象テキストを複製する
			class TargetDuplicate : public manah::Noncopyable {
			public:
				TargetDuplicate(const text::Document& document, length_t startLine, length_t numberOfLines);
				~TargetDuplicate() throw();
				const Char*	getBuffer() const throw();
				length_t	getLength() const throw();
				length_t	getNumberOfLines() const throw();
				void		getResult(const MatchedRange& range, text::Region& result) const;
			private:
				Char* buffer_;
				Char** lineHeads_;
				const length_t startLine_;
				const length_t numberOfLines_;
				length_t length_;
			};
			const text::Document& document_;
			const TextSearcher& searcher_;
			const unicode::IdentifierSyntax& ctypes_;
		};

		/// A listener observes the state of the incremental searcher.
		class IIncrementalSearcherListener {
		protected:
			/// Used by @c IIncrementalSearcherListener#incrementalSearchPatternChanged.
			enum Result {
				FOUND,			///< Pattern is found (or pattern is empty).
				NOT_FOUND,		///< Pattern is not found.
				COMPLEX_REGEX,	///< The regular expression is too complex.
				BAD_REGEX		///< The regular expression is invalid.
			};
		private:
			/// The search is aborted. Also @c #incrementalSearchCompleted will be called after this.
			virtual void incrementalSearchAborted() = 0;
			/// The search is completed.
			virtual void incrementalSearchCompleted() = 0;
			/**
			 * The search pattern is changed.
			 * @param result the result on new pattern.
			 */
			virtual void incrementalSearchPatternChanged(Result result) = 0;
			/// The search is started. Also @c #incrementalSearchPatternChanged will be called after this.
			virtual void incrementalSearchStarted() = 0;
			friend class IncrementalSearcher;
		};


		/// @c IncrementalSearcher performs incremental search on a viewer.
		class IncrementalSearcher {
		public:
			/**
			 * Incremental search is not running.
			 * @see IncrementalSearcher
			 */
			class NotRunningException : public std::logic_error {
			public:
				/// Constructor.
				NotRunningException() : std::logic_error("Incremental search is not running") {}
			};

			/**
			 * Client tried to undo the unundoable incremental search command.
			 * @see IncrementalSearcher#undo
			 */
			class EmptyUndoBufferException : public std::logic_error {
			public:
				/// Constructor.
				EmptyUndoBufferException() : std::logic_error("Undo buffer of incremental search is empty and not undoable.") {}
			};

		public:
			// constructor
			IncrementalSearcher() throw();
			// attributes
			bool				canUndo() const throw();
			Direction			getDirection() const;
			const String&		getPattern() const throw();
			TextSearcher::Type	getType() const;
			bool				isRunning() const throw();
			// operations
			void	abort();
			bool	addCharacter(Char ch);
			bool	addCharacter(CodePoint cp);
			bool	addString(const Char* first, const Char* last);
			bool	addString(const String& text);
			void	end();
			bool	jumpToNextMatch(Direction direction);
			void	reset();
			void	start(viewers::Caret& caret, TextSearcher& searcher,
						TextSearcher::Type type, Direction direction, IIncrementalSearcherListener* listener = 0);
			bool	undo();
		private:
			bool	update();
			
			// データメンバ
		private:
			enum Operation {TYPE, JUMP};
			struct Status {
				text::Region range;		// マッチ位置
				Direction direction;	// そのときの検索方向
			};
			viewers::Caret* caret_;
			TextSearcher* searcher_;
			TextSearcher::Type type_;					// 検索タイプ
			IIncrementalSearcherListener* listener_;
			std::stack<Operation> operationHistory_;	// 操作履歴
			std::stack<Status> statusHistory_;			// 状態履歴
			Status* firstStatus_;						// statusHistory_ の一番下へのポインタ
			String pattern_;							// 検索式
			String lastPattern_;						// 前回検索終了時の検索式
			bool matched_;								// 最後の update 呼び出しで一致テキストが見つかったか
		};


// inline ///////////////////////////////////////////////////////////////////

	/// Returns the replacement string.
	inline const String& TextSearcher::getReplacement() const throw() {return replacement_;}
	/// Sets the new replacement string.
	inline void TextSearcher::setReplacement(const String& replacement) throw() {replacement_.assign(replacement);}
	/// Returns the target buffer.
	inline const Char* DocumentSearcher::TargetDuplicate::getBuffer() const throw() {return buffer_;}
	/// Returns the length of the target.
	inline length_t DocumentSearcher::TargetDuplicate::getLength() const throw() {return length_;}
	/// Returns the number of lines of the target.
	inline length_t DocumentSearcher::TargetDuplicate::getNumberOfLines() const throw() {return numberOfLines_;}
	/// Constructor.
	inline IncrementalSearcher::IncrementalSearcher() throw() {}
	/// Returns if the previous command is undoable.
	inline bool IncrementalSearcher::canUndo() const throw() {return !operationHistory_.empty();}
	/**
	 * Returns the direction of the search.
	 * @return the direction
	 * @throw IncrementalSearcher#NotRunningException the searcher is not running
	 */
	inline Direction IncrementalSearcher::getDirection() const {
		if(!isRunning())
			throw NotRunningException();
		return statusHistory_.top().direction;
	}
	/// Returns the current search pattern.
	inline const String& IncrementalSearcher::getPattern() const throw() {return pattern_;}
	/**
	 * Returns the current search type.
	 * @throw IncrementalSearcher#NotRunningException the searcher is not running
	 */
	inline TextSearcher::Type IncrementalSearcher::getType() const {
		if(!isRunning())
			throw NotRunningException();
		return type_;
	}
	/// 現在検索実行中かを返す
	inline bool IncrementalSearcher::isRunning() const throw() {return !statusHistory_.empty();}
	/// 現在設定されている検索条件を返す
	inline const TextSearcher::Options& TextSearcher::getOptions() const throw() {return options_;}
	/// Returns the search pattern text.
	inline const String& TextSearcher::getPattern() const throw() {return patternString_;}
	/// 最後に検索してから検索条件やオプションが変更されているかを返す
	inline bool TextSearcher::isChangedSinceLastSearch() const throw() {return changedFromLast_;}
	/// 検索文字列が複数行にマッチするパターンかを返す
	inline bool TextSearcher::isMultilinePattern() const throw() {return multilinePattern_;}
	/// Returns true if the search using regular expression is available.
	inline bool TextSearcher::isRegexAvailable() throw() {
#ifdef ASCENSION_NO_REGEX
		return false;
#else
		return true;
#endif /* ASCENSION_NO_REGEX */
	}
	/**
	 * Appends the specified text to the end of the current pattern.
	 * @param text the string to be appended
	 * @return true if the pattern is found
	 * @throw IncrementalSearcher#NotRunningException the searcher is not running
	 * @throw std#invalid_argument the string is empty
	 * @throw ... any exceptions specified by Boost.Regex will be thrown if the regular expression error occured
	 */
	inline bool IncrementalSearcher::addString(const String& text) {return addString(text.data(), text.data() + text.length());}

}} // namespace ascension::searcher

#endif /* !ASCENSION_SEARCHER_HPP */
