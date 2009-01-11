/**
 * @file startup-handler.hpp
 * @author exeal
 * @date 2007
 */

#include "core.hpp"
#include "../msxml4.tlh"
#include <string>

namespace alpha {
	namespace ankh {
		class StartupHandler : virtual public MSXML2::ISAXContentHandler, virtual public MSXML2::ISAXErrorHandler {
		public:
			// constructor
			StartupHandler(ScriptSystem& central, const WCHAR* fileName);
			// IUnknown
			IMPLEMENT_UNKNOWN_NO_REF_COUNT()
			STDMETHODIMP QueryInterface(REFIID iid, void** object) {
				VERIFY_POINTER(object);
				if(iid == IID_IUnknown || iid == __uuidof(ISAXContentHandler))
					*object = static_cast<ISAXContentHandler*>(this);
				else if(iid == __uuidof(ISAXErrorHandler))
					*object = static_cast<ISAXErrorHandler*>(this);
				else
					return (*object = 0), E_NOINTERFACE;
				reinterpret_cast<IUnknown*>(*object)->AddRef();
				return S_OK;
			}
			// ISAXContentHandler
			STDMETHODIMP	putDocumentLocator(MSXML2::ISAXLocator* locator);
			STDMETHODIMP	startDocument();
			STDMETHODIMP	endDocument();
			STDMETHODIMP	startPrefixMapping(unsigned short* prefix, int prefixLength, unsigned short* uri, int uriLength);
			STDMETHODIMP	endPrefixMapping(unsigned short* prefix, int length);
			STDMETHODIMP	startElement(unsigned short* namespaceUri, int namespaceUriLength,
								unsigned short* localName, int localNameLength, unsigned short* qName, int qNameLength, MSXML2::ISAXAttributes* attributes);
			STDMETHODIMP	endElement(unsigned short* namespaceUri, int namespaceUriLength,
								unsigned short* localName, int localNameLength, unsigned short* qName, int qNameLength);
			STDMETHODIMP	characters(unsigned short* chars, int length);
			STDMETHODIMP	ignorableWhitespace(unsigned short* chars, int length);
			STDMETHODIMP	processingInstruction (unsigned short* target, int targetLength, unsigned short* data, int dataLength);
			STDMETHODIMP	skippedEntity(unsigned short* name, int length);
			// ISAXErrorHandler
			STDMETHODIMP	error(MSXML2::ISAXLocator* locator, unsigned short* errorMessage, HRESULT errorCode);
			STDMETHODIMP	fatalError(MSXML2::ISAXLocator* locator, unsigned short* errorMessage, HRESULT errorCode);
			STDMETHODIMP	ignorableWarning(MSXML2::ISAXLocator* locator, unsigned short* errorMessage, HRESULT errorCode);
		private:
			void	handleInclude(const std::wstring& fileName);
			void	handleScript(const ::OLECHAR* sourceFile, std::size_t length);
			void	handleVariable(const std::wstring& name, const std::wstring& value, const std::wstring& type, bool constant);
		private:
			ScriptSystem& central_;
		};
	}
}