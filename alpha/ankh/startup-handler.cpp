/**
 * @file startup-handler.cpp
 * @author exeal
 * @date 2007
 */

#include "startup-handler.hpp"
#include "../../manah/com/common.hpp"
using namespace alpha::ankh;
using namespace manah::com;
using namespace std;

//#import <msxml4.dll> raw_interfaces_only

/**
 * Constructor.
 * @param fileName the name of the input file
 */
StartupHandler::StartupHandler(ScriptSystem& central, const WCHAR* fileName) : central_(central) {
	ComPtr<MSXML2::ISAXXMLReader> reader;
	HRESULT hr = reader.createInstance(__uuidof(MSXML2::SAXXMLReader40));
	if(FAILED(hr))
		throw hr;
	hr = reader->putContentHandler(this);
	hr = reader->putErrorHandler(this);
	hr = reader->putFeature(reinterpret_cast<unsigned short*>(L"schema-validation"), VARIANT_TRUE);
	hr = reader->parseURL(reinterpret_cast<unsigned short*>(const_cast<WCHAR*>(fileName)));
}

/// @see ISAXContentHandler#characters
STDMETHODIMP StartupHandler::characters(unsigned short* chars, int length) {return S_OK;}

/// @see ISAXContentHandler#endDocument
STDMETHODIMP StartupHandler::endDocument() {return S_OK;}

/// @see ISAXContentHandler#endElement
STDMETHODIMP StartupHandler::endElement(unsigned short* namespaceUri, int namespaceUriLength,
				unsigned short* localName, int localNameLength, unsigned short* qName, int qNameLength) {return S_OK;}

/// @see ISAXContentHandler#endPrefixMapping
STDMETHODIMP StartupHandler::endPrefixMapping(unsigned short* prefix, int length) {return S_OK;}

/// @see ISAXErrorHandler#error
STDMETHODIMP StartupHandler::error(MSXML2::ISAXLocator* locator, unsigned short* errorMessage, HRESULT errorCode) {return S_OK;}

/// @see ISAXErrorHandler#fatalError
STDMETHODIMP StartupHandler::fatalError(MSXML2::ISAXLocator* locator, unsigned short* errorMessage, HRESULT errorCode) {return S_OK;}

void StartupHandler::handleInclude(const wstring& fileName) {}

void StartupHandler::handleScript(const ::OLECHAR* sourceFile, size_t length) {
	::BSTR fileName = ::SysAllocStringLen(sourceFile, static_cast<unsigned int>(length));
	central_.LoadScript(fileName);
	::SysFreeString(fileName);
}

void StartupHandler::handleVariable(const wstring& name, const wstring& value, const wstring& type, bool constant) {}

/// @see ISAXErrorHandler#ignorableWarning
STDMETHODIMP StartupHandler::ignorableWarning(MSXML2::ISAXLocator* locator, unsigned short* errorMessage, HRESULT errorCode) {return S_OK;}

/// @see ISAXContentHandler#ignorableWhitespace
STDMETHODIMP StartupHandler::ignorableWhitespace(unsigned short* chars, int length) {return S_OK;}

/// @see ISAXContentHandler#putDocumentLocator
STDMETHODIMP StartupHandler::putDocumentLocator(MSXML2::ISAXLocator* locator) {return S_OK;}

/// @see ISAXContentHandler#skippedEntity
STDMETHODIMP StartupHandler::skippedEntity(unsigned short* name, int length) {return S_OK;}

/// @see ISAXContentHandler#startDocument
STDMETHODIMP StartupHandler::startDocument() {return S_OK;}

/// @see ISAXContentHandler#startElement
STDMETHODIMP StartupHandler::startElement(unsigned short* namespaceUri, int namespaceUriLength,
		unsigned short* localName, int localNameLength, unsigned short* qName, int qNameLength, MSXML2::ISAXAttributes* attributes) {
	if(localNameLength == 6 && wmemcmp(reinterpret_cast<wchar_t*>(localName), L"script", 6) == 0) {
		wchar_t* sourceFile;
		int length;
		attributes->getValueFromQName(reinterpret_cast<unsigned short*>(L"src"), 3, reinterpret_cast<unsigned short**>(&sourceFile), &length);
		handleScript(sourceFile, length);
	}
	return S_OK;
}

/// @see ISAXContentHandler#startPrefixMapping
STDMETHODIMP StartupHandler::startPrefixMapping(unsigned short* prefix, int prefixLength, unsigned short* uri, int uriLength) {return S_OK;}

/// @see ISAXContentHandler#processingInstruction
STDMETHODIMP StartupHandler::processingInstruction (unsigned short* target, int targetLength, unsigned short* data, int dataLength) {return S_OK;}