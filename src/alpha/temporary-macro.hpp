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
		 * @brief �ꎞ�}�N�� (�L�[�{�[�h�}�N��) �̊Ǘ�
		 *
		 * @c TemporaryMacro �͈ꎞ�}�N���̋L�^�A���s�A�ۑ��A�ǂݍ��݂��T�|�[�g����B
		 * @c TemporaryMacro#startDefinition ���ŋL�^���ꂽ�}�N���� XML �`���ŉi���L���\�ł���A
		 * �w�肵���f�B���N�g���� XML �t�@�C���Ƃ��ĕۑ������B
		 * ���� XML �X�L�[�}�� temporary-macro.xsd �ł���
		 */
		class TemporaryMacro : public manah::Noncopyable {
		public:
			/// ���
			enum State {
				NEUTRAL,		///< ��������
				EXECUTING,		///< ���s��
				DEFINING,		///< �L�^��
				QUERYING_USER,	///< ���s�ʒu��~�� (���[�U�̉����҂�)
				PAUSING			///< �L�^�ꎞ��~��
			};

			/// ���s���ɃG���[�����������ꍇ�̏���
			enum ErrorHandlingPolicy {
				IGNORE_AND_CONTINUE,	///< �������đ��s
				QUERY_USER,				///< ���[�U�ɖ₢���킹��
				ABORT					///< ���~
			};

			// �R���X�g���N�^
			TemporaryMacro() throw();
			// ����
			ErrorHandlingPolicy				getErrorHandlingPolicy() const throw();
			const std::basic_string<WCHAR>&	getFileName() const throw();
			State							getState() const throw();
			bool							isDefining() const throw();
			bool							isEmpty() const throw();
			bool							isExecuting() const throw();
			void							setErrorHandlingPolicy(ErrorHandlingPolicy policy) throw();
			// ����
			void	appendDefinition();
			void	cancelDefinition();
			void	endDefinition();
			void	execute(ulong repeatCount = 1);
			void	insertUserQuery();
			void	pauseDefinition();
			void	pushCommand(SerializableCommand& command);
			void	restartDefinition();
			void	startDefinition();
			// �i���L��
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
			State state_;						// �ꎞ�}�N���̏��
			Definition definingDefinition_;		// �L�^���̒�`
			Definition definition_;				// �L�^������`
			ErrorHandlingPolicy errorHandlingPolicy_;
			std::basic_string<WCHAR> fileName_;	// �֘A�t�����Ă���t�@�C����
			manah::win32::Handle<HICON, ::DestroyIcon> definingIcon_, pausingIcon_;
		};


		/**
		 * �L�^�𒆎~����
		 * @throw std#logic_error �L�^���łȂ���΃X���[
		 */
		inline void TemporaryMacro::cancelDefinition() {
			if(!isDefining())
				throw std::logic_error("Temporary macro is not in recording.");
			clearCommandList(true);
			changeState(NEUTRAL);
		}

		/**
		 * �L�^���I������
		 * @throw std::logic_error �L�^���łȂ���΃X���[
		 */
		inline void TemporaryMacro::endDefinition() {
			if(definingDefinition_.commands.empty()) {	// �����L�^���Ă��Ȃ���Β��~
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

		/// �G���[�����|���V�[��Ԃ�
		inline TemporaryMacro::ErrorHandlingPolicy TemporaryMacro::getErrorHandlingPolicy() const throw() {return errorHandlingPolicy_;}

		/// �L�^���e�Ɋ֘A�t�����Ă���t�@�C������Ԃ��B�t�@�C���������ꍇ�͋󕶎���
		inline const std::basic_string<WCHAR>& TemporaryMacro::getFileName() const throw() {return fileName_;}

		/// ��Ԃ�Ԃ�
		inline TemporaryMacro::State TemporaryMacro::getState() const throw() {return state_;}

		/// �L�^���ł��邩��Ԃ�
		inline bool TemporaryMacro::isDefining() const throw() {return state_ == DEFINING || state_ == PAUSING;}

		/// �L�^���e���󂩂�Ԃ�
		inline bool TemporaryMacro::isEmpty() const throw() {return definition_.commands.empty();}

		/// ���s���ł��邩��Ԃ�
		inline bool TemporaryMacro::isExecuting() const throw() {return state_ == EXECUTING || state_ == QUERYING_USER;}

		/**
		 * �L�^�̈ꎞ���f
		 * @throw std::logic_error �L�^���łȂ���΃X���[
		 */
		inline void TemporaryMacro::pauseDefinition() {
			if(state_ != DEFINING)
				throw std::logic_error("Temporary macro is not defining.");
			changeState(PAUSING);
		}

		/**
		 * �L�^�ꎞ���f��Ԃ��畜�A����
		 * @throw std::logic_error �L�^�ꎞ��~�A���͑҂���ԂłȂ���΃X���[
		 */
		inline void TemporaryMacro::restartDefinition() {
			if(state_ != PAUSING)
				throw std::logic_error("Temporary macro is not pausing definition.");
			changeState(DEFINING);
		}

		/**
		 * �G���[�����|���V�[�̐ݒ�
		 * @param policy �V�����|���V�[
		 */
		inline void TemporaryMacro::setErrorHandlingPolicy(ErrorHandlingPolicy policy) throw() {errorHandlingPolicy_ = policy;}

		/**
		 * �L�^���J�n
		 * @throw std::logic_error ���s���A�L�^���ł���΃X���[
		 */
		inline void TemporaryMacro::startDefinition() {
			if(isDefining() || isExecuting())
				throw std::logic_error("Recorder is not ready to start recording.");
			changeState(DEFINING);
		}

	} // namespace command
} // namespace alpha

#endif /* !ALPHA_TEMPORARY_MACRO_HPP */
