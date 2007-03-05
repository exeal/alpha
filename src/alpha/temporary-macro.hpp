/**
 * @file temporary-macro.hpp
 * @author exeal
 * @date 2003-2007
 */

#ifndef ALPHA_TEMPORARY_MACRO_HPP
#define ALPHA_TEMPORARY_MACRO_HPP

#include "../manah/win32/windows.hpp"
#include <list>
#include <algorithm>
#include <functional>


namespace alpha {
	namespace command {
		class CommandManager;
		class SerializableCommand;

		/**
		 * @brief 一時マクロ (キーボードマクロ) の管理
		 *
		 * @c TemporaryMacro は一時マクロの記録、実行、保存、読み込みをサポートする。
		 * @c TemporaryMacro#startDefinition 等で記録されたマクロは XML 形式で永続記憶可能であり、
		 * 指定したディレクトリに XML ファイルとして保存される。
		 * その XML スキーマは temporary-macro.xsd である
		 */
		class TemporaryMacro : public manah::Noncopyable {
		public:
			/// 状態
			enum State {
				NEUTRAL,		///< 何も無し
				EXECUTING,		///< 実行中
				DEFINING,		///< 記録中
				QUERYING_USER,	///< 実行位置停止中 (ユーザの応答待ち)
				PAUSING			///< 記録一時停止中
			};

			/// 実行中にエラーが発生した場合の処理
			enum ErrorHandlingPolicy {
				IGNORE_AND_CONTINUE,	///< 無視して続行
				QUERY_USER,				///< ユーザに問い合わせる
				ABORT					///< 中止
			};

			// コンストラクタ
			TemporaryMacro() throw();
			// 属性
			ErrorHandlingPolicy				getErrorHandlingPolicy() const throw();
			const std::basic_string<WCHAR>&	getFileName() const throw();
			State							getState() const throw();
			bool							isDefining() const throw();
			bool							isEmpty() const throw();
			bool							isExecuting() const throw();
			void							setErrorHandlingPolicy(ErrorHandlingPolicy policy) throw();
			// 操作
			void	appendDefinition();
			void	cancelDefinition();
			void	endDefinition();
			void	execute(ulong repeatCount = 1);
			void	insertUserQuery();
			void	pauseDefinition();
			void	pushCommand(SerializableCommand& command);
			void	restartDefinition();
			void	startDefinition();
			// 永続記憶
			bool	load(const std::basic_string<WCHAR>& fileName);
			bool	save(const std::basic_string<WCHAR>& fileName);
			void	showLoadDialog();
			void	showSaveDialog();

		private:
			void	changeState(State newState) throw();
			void	clearCommandList(bool definingCommands);
			typedef std::list<SerializableCommand*> CommandList;
			typedef std::list<std::size_t> QueryPointList;
			struct Definition {
				CommandList commands;
				QueryPointList queryPoints;
			};
			State state_;						// 一時マクロの状態
			Definition definingDefinition_;		// 記録中の定義
			Definition definition_;				// 記録した定義
			ErrorHandlingPolicy errorHandlingPolicy_;
			std::basic_string<WCHAR> fileName_;	// 関連付けられているファイル名
			manah::win32::Handle<HICON, ::DestroyIcon> definingIcon_, pausingIcon_;
		};


		/**
		 * 記録を中止する
		 * @throw std#logic_error 記録中でなければスロー
		 */
		inline void TemporaryMacro::cancelDefinition() {
			if(!isDefining())
				throw std::logic_error("Temporary macro is not in recording.");
			clearCommandList(true);
			changeState(NEUTRAL);
		}

		/**
		 * 記録を終了する
		 * @throw std::logic_error 記録中でなければスロー
		 */
		inline void TemporaryMacro::endDefinition() {
			if(definingDefinition_.commands.empty()) {	// 何も記録していなければ中止
				cancelDefinition();
				return;
			}
			if(!isDefining())
				throw std::logic_error("Temporary macro is not defining.");
			definition_ = definingDefinition_;
			definingDefinition_.commands.clear();
			definingDefinition_.queryPoints.clear();
			changeState(NEUTRAL);
			fileName_.erase();;
		}

		/// エラー処理ポリシーを返す
		inline TemporaryMacro::ErrorHandlingPolicy TemporaryMacro::getErrorHandlingPolicy() const throw() {return errorHandlingPolicy_;}

		/// 記録内容に関連付けられているファイル名を返す。ファイルが無い場合は空文字列
		inline const std::basic_string<WCHAR>& TemporaryMacro::getFileName() const throw() {return fileName_;}

		/// 状態を返す
		inline TemporaryMacro::State TemporaryMacro::getState() const throw() {return state_;}

		/// 記録中であるかを返す
		inline bool TemporaryMacro::isDefining() const throw() {return state_ == DEFINING || state_ == PAUSING;}

		/// 記録内容が空かを返す
		inline bool TemporaryMacro::isEmpty() const throw() {return definition_.commands.empty();}

		/// 実行中であるかを返す
		inline bool TemporaryMacro::isExecuting() const throw() {return state_ == EXECUTING || state_ == QUERYING_USER;}

		/**
		 * 記録の一時中断
		 * @throw std::logic_error 記録中でなければスロー
		 */
		inline void TemporaryMacro::pauseDefinition() {
			if(state_ != DEFINING)
				throw std::logic_error("Temporary macro is not defining.");
			changeState(PAUSING);
		}

		/**
		 * 記録一時中断状態から復帰する
		 * @throw std::logic_error 記録一時停止、入力待ち状態でなければスロー
		 */
		inline void TemporaryMacro::restartDefinition() {
			if(state_ != PAUSING)
				throw std::logic_error("Temporary macro is not pausing definition.");
			changeState(DEFINING);
		}

		/**
		 * エラー処理ポリシーの設定
		 * @param policy 新しいポリシー
		 */
		inline void TemporaryMacro::setErrorHandlingPolicy(ErrorHandlingPolicy policy) throw() {errorHandlingPolicy_ = policy;}

		/**
		 * 記録を開始
		 * @throw std::logic_error 実行中、記録中であればスロー
		 */
		inline void TemporaryMacro::startDefinition() {
			if(isDefining() || isExecuting())
				throw std::logic_error("Recorder is not ready to start recording.");
			changeState(DEFINING);
		}

	} // namespace command
} // namespace alpha

#endif /* !ALPHA_TEMPORARY_MACRO_HPP */
