/**
 * @file exception
 * @date 2009 exeal
 */

#ifndef MANAH_COM_EXCEPTION_HPP
#define MANAH_COM_EXCEPTION_HPP
#include "common.hpp"

namespace manah {
	namespace com {
		template<const IID* iid, typename Next = void>
		class IIDList {
		public:
			static bool find(REFIID riid) {
				return doFind<Next>(riid);
			}
		private:
			template<typename N> static bool doFind(REFIID riid) {
				return (riid == *iid) ? true : N::find(riid);
			}
			template<> static bool doFind<void>(REFIID riid) {
				return toBoolean(::InlineIsEqualGUID(riid, *iid));
			}
		};

		/**
		 * Implements @c ISupportErrorInfo interface.
		 * @param InterfaceIDs the list of interface identifiers to implement
		 * @param Base the base type inherits @c ISupportErrorInfo
		 */
		template<typename InterfaceIDs, typename Base>
		class ISupportErrorInfoImpl : public Base {
		public:
			STDMETHODIMP InterfaceSupportsErrorInfo(REFIID riid) {
				return InterfaceIDs::find(riid) ? S_OK : S_FALSE;
			}
		};

		/// A wrapper for treatment an @c IErrorInfo as a C++ exception (from Essential COM (Don Box)).
		class ComException {
		public:
			/**
			 * Constructor.
			 * @param scode the SCODE
			 * @param riid the IID
			 * @param source the class threw this exception
			 * @param description the description of the exception. if @c null, retrieved by @a scode
			 * @param helpFile the help file. can be @c null
			 * @param helpContext the number of the help topic. can be @c null
			 */
			ComException(HRESULT scode, const IID& iid, const OLECHAR* source,
					const OLECHAR* description = 0, const OLECHAR* helpFile = 0, DWORD helpContext = 0) : hr_(scode) {
//				assert(FAILED(scode));
				ComPtr<ICreateErrorInfo> cei;
				HRESULT hr = ::CreateErrorInfo(cei.initialize());
				if(hr == E_OUTOFMEMORY)
					throw std::bad_alloc("CreateErrorInfo returned E_OUTOFMEMORY.");
				if(E_OUTOFMEMORY == (hr = cei->SetGUID(iid)))
					throw std::bad_alloc("ICreateErrorInfo.SetGUID returned E_OUTOFMEMORY.");
				if(source != 0 && E_OUTOFMEMORY == (hr = cei->SetSource(const_cast<OLECHAR*>(source))))
					throw std::bad_alloc("ICreateErrorInfo.SetSource returned E_OUTOFMEMORY.");
				if(E_OUTOFMEMORY == (hr = cei->SetDescription(
						const_cast<OLECHAR*>((description != 0) ? description : descriptionOfSCode(scode).c_str()))))
					throw std::bad_alloc("ICreateErrorInfo.SetDescription returned E_OUTOFMEMORY.");
				if(helpFile != 0 && E_OUTOFMEMORY == (hr = cei->SetHelpFile(const_cast<OLECHAR*>(helpFile))))
					throw std::bad_alloc("ICreateErrorInfo.SetHelpFile returned E_OUTOFMEMORY.");
				if(helpContext != 0 && E_OUTOFMEMORY == (hr = cei->SetHelpContext(helpContext)))
					throw std::bad_alloc("ICreateErrorInfo.SetHelpContext returned E_OUTOFMEMORY.");
				cei->QueryInterface(IID_IErrorInfo, reinterpret_cast<void**>(errorInfo_.initialize()));
			}
			/// Returns an @c IErrorInfo interface pointer.
			ComPtr<IErrorInfo> errorInfo() const /*throw()*/ {return errorInfo_;}
			/// Throws the exception object as a logical thread exception.
			void raise() {::SetErrorInfo(0, errorInfo_.get());}
			/// Returns the @c HRESULT value of the error.
			HRESULT scode() const /*throw()*/ {return hr_;}
			/**
			 * Returns an error message corresponding to the given @c HRESULT.
			 * @param[in] hr the HRESULT value
			 * @param[out] description the error message
			 * @param[in] language the language identifier
			 */
			static std::basic_string<OLECHAR> descriptionOfSCode(HRESULT hr, DWORD language = LANG_USER_DEFAULT) {
				void* buffer = 0;
				if(::FormatMessageW(
						FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
						0, hr, language, reinterpret_cast<wchar_t*>(&buffer), 0, 0) == 0)
					return std::basic_string<OLECHAR>();
				std::basic_string<OLECHAR> temp(::SysAllocString(static_cast<OLECHAR*>(buffer)));
				::LocalFree(buffer);
				return temp;
			}

		private:
			HRESULT hr_;
			ComPtr<IErrorInfo> errorInfo_;
		};
	}
}

#endif // !MANAH_COM_EXCEPTION_HPP
