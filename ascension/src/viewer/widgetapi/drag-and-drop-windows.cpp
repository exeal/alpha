/**
 * @file drag-and-drop-windows.cpp
 * Implements entities defined in drag-and-drops.hpp for Win32 window system.
 * @author exeal
 * @date 2016-12-25 Created.
 */

#include <ascension/viewer/widgetapi/drag-and-drop.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)

#include <ascension/corelib/native-conversion.hpp>
#include <ascension/graphics/geometry/native-conversions.hpp>
#include <ascension/viewer/widgetapi/event/mouse-button-input.hpp>
#include <ascension/win32/com/com.hpp>
#ifdef _DEBUG
#	include <ascension/log.hpp>
#endif
#include <ShlGuid.h>	// CLSID_DragDropHelper
#include <ShObjIdl.h>	// IDropTargetHelper

namespace ascension {
	namespace viewer {
		namespace widgetapi {
			namespace detail {
				namespace {
					win32::com::SmartPointer<IDropTargetHelper> dropTargetHelper() {
						static boost::optional<win32::com::SmartPointer<IDropTargetHelper>> instance;
						if(instance == boost::none)
							instance = win32::com::SmartPointer<IDropTargetHelper>::create(CLSID_DragDropHelper, IID_IDropTargetHelper, CLSCTX_INPROC_SERVER);
						return boost::get(instance);
					}

					inline DropAction translateDropActions(DWORD effect) BOOST_NOEXCEPT {
						DropAction result = DROP_ACTION_IGNORE;
						if(win32::boole(effect & DROPEFFECT_COPY))
							result |= widgetapi::DROP_ACTION_COPY;
						if(win32::boole(effect & DROPEFFECT_MOVE))
							result |= widgetapi::DROP_ACTION_MOVE;
						if(win32::boole(effect & DROPEFFECT_LINK))
							result |= widgetapi::DROP_ACTION_LINK;
						if(win32::boole(effect & DROPEFFECT_SCROLL))
							result |= widgetapi::DROP_ACTION_WIN32_SCROLL;
						return result;
					}

					inline DWORD translateDropAction(DropAction dropAction) BOOST_NOEXCEPT {
						DWORD effect = DROPEFFECT_NONE;
						if((dropAction & widgetapi::DROP_ACTION_COPY) != 0)
							effect |= DROPEFFECT_COPY;
						if((dropAction & widgetapi::DROP_ACTION_MOVE) != 0)
							effect |= DROPEFFECT_MOVE;
						if((dropAction & widgetapi::DROP_ACTION_LINK) != 0)
							effect |= DROPEFFECT_LINK;
						if((dropAction & widgetapi::DROP_ACTION_WIN32_SCROLL) != 0)
							effect |= DROPEFFECT_SCROLL;
						return effect;
					}
				}

				namespace {
					inline std::shared_ptr<InterprocessData> makeInterprocessData(IDataObject* data) {
						return std::make_shared<InterprocessData>(win32::com::SmartPointer<IDataObject>(data));
					}
				}

				HRESULT DragEventAdapter::adaptDragEnterEvent(IDataObject* data, DWORD keyState, POINTL location, DWORD* effect) {
					if(data == nullptr)
						return E_INVALIDARG;
					ASCENSION_WIN32_VERIFY_COM_POINTER(effect);
					*effect = DROPEFFECT_NONE;

					HRESULT hr;
#ifdef _DEBUG
					{
						win32::com::SmartPointer<IEnumFORMATETC> formats;
						if(SUCCEEDED(hr = data->EnumFormatEtc(DATADIR_GET, formats.initialize()))) {
							FORMATETC format;
							ULONG fetched;
							ASCENSION_LOG_TRIVIAL(debug) << L"DragEnter received a data object exposes the following formats.\n";
							for(formats->Reset(); formats->Next(1, &format, &fetched) == S_OK; ) {
								std::array<WCHAR, 256> name;
								if(::GetClipboardFormatNameW(format.cfFormat, name.data(), name.size() - 1) != 0)
									ASCENSION_LOG_TRIVIAL(debug) << L"\t" << name.data() << L"\n";
								else
									ASCENSION_LOG_TRIVIAL(debug) << L"\t" << L"(unknown format : " << format.cfFormat << L")\n";
								if(format.ptd != nullptr)
									::CoTaskMemFree(format.ptd);
							}
						}
					}
#endif // _DEBUG
					const auto p(fromNative<graphics::Point>(location));
					widgetapi::DragEnterInput input(
						win32::makeLocatedUserInput(keyState, widgetapi::mapFromGlobal(widget_, p)),
						translateDropActions(*effect), makeInterprocessData(data));
					try {
						target_.dragEntered(input);
					} catch(const std::bad_alloc&) {
						return E_OUTOFMEMORY;
					} catch(...) {
						return E_UNEXPECTED;
					}
					data_ = makeInterprocessData(data);
					*effect = translateDropAction(input.dropAction());
					if(auto helper = dropTargetHelper()) {
						auto p2(toNative<POINT>(p));
						helper->DragEnter(widget_.get()->handle().get(), data, &p2, *effect);
					}

					return S_OK;
				}

				HRESULT DragEventAdapter::adaptDragLeaveEvent() {
					data_.reset();
					if(auto helper = dropTargetHelper())
						helper->DragLeave();
					try {
						target_.dragLeft(DragLeaveInput());
					} catch(const std::bad_alloc&) {
						return E_OUTOFMEMORY;
					} catch(...) {
						return E_UNEXPECTED;
					}
					return S_OK;
				}

				HRESULT DragEventAdapter::adaptDragMoveEvent(DWORD keyState, POINTL location, DWORD* effect) {
					ASCENSION_WIN32_VERIFY_COM_POINTER(effect);
					if(data_.get() == nullptr)
						return E_UNEXPECTED;

					const auto p(fromNative<graphics::Point>(location));
					try {
						target_.dragMoved(widgetapi::DragMoveInput(
							win32::makeLocatedUserInput(keyState, widgetapi::mapFromGlobal(widget_, p)),
							translateDropActions(*effect), data_));
					} catch(const std::bad_alloc&) {
						return E_OUTOFMEMORY;
					} catch(...) {
						return E_UNEXPECTED;
					}
					if(auto helper = dropTargetHelper()) {
						auto p2(toNative<POINT>(p));
						helper->DragOver(&p2, *effect);
					}
					return S_OK;
				}

				HRESULT DragEventAdapter::adaptDropEvent(IDataObject* data, DWORD keyState, POINTL location, DWORD* effect) {
					if(data == nullptr)
						return E_INVALIDARG;
					ASCENSION_WIN32_VERIFY_COM_POINTER(effect);
					*effect = DROPEFFECT_NONE;
					data_.reset();
/*
					FORMATETC fe = {::RegisterClipboardFormatA("Rich Text Format"), 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
					STGMEDIUM stg;
					data->GetData(&fe, &stg);
					const char* bytes = static_cast<char*>(::GlobalLock(stg.hGlobal));
					ASCENSION_LOG_TRIVIAL(debug) << bytes;
					::GlobalUnlock(stg.hGlobal);
					::ReleaseStgMedium(&stg);
*/
					HRESULT hr = S_OK;
					const auto p(fromNative<graphics::Point>(location));
					try {
						target_.dropped(widgetapi::DropInput(
							win32::makeLocatedUserInput(keyState, widgetapi::mapFromGlobal(widget_, p)),
							translateDropActions(*effect), makeInterprocessData(data)));
					} catch(const std::bad_alloc&) {
						hr = E_OUTOFMEMORY;
					} catch(...) {
						hr = E_UNEXPECTED;
					}
					if(auto helper = dropTargetHelper()) {
						auto p2(toNative<POINT>(p));
						helper->DragOver(&p2, *effect);
					}
					return hr;
				}
			}
		}
	}
}

#endif
