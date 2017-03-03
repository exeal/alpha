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
#include <ascension/graphics/image.hpp>
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
			namespace {
				DropAction firstAction(const DropActions& actions) {
					assert(actions.any());
					for(std::size_t i = 0; i < actions.size(); ++i) {
						if(actions.test(i))
							return static_cast<DropAction>(i);
					}
					ASCENSION_ASSERT_NOT_REACHED();
				}
			}

			boost::optional<DropAction> DragContext::execute(const DropActions& supportedActions, win32::com::SmartPointer<IDropSource> source) {
				DWORD effect;
				const auto hr = ::DoDragDrop(mimeData_.get(), source.get(), toNative<DWORD>(supportedActions), &effect);
				if(FAILED(hr))
					throw makePlatformError();
				const auto action(fromNative<DropActions>(effect));
				return action.any() ? boost::make_optional(firstAction(action)) : boost::none;
			}
			
			void DragContext::setData(InterprocessData&& data) {
				mimeData_ = data.native();
			}

			void DragContext::setImage(const graphics::Image& image, const boost::geometry::model::d2::point_xy<std::uint32_t>& hotspot) {
				if(mimeData_.get() == nullptr)
					return;
				if(imageProvider_.get() == nullptr)
					imageProvider_ = win32::com::SmartPointer<IDragSourceHelper>::create(CLSID_DragDropHelper, IID_IDragSourceHelper, CLSCTX_INPROC_SERVER);
				SHDRAGIMAGE shdi;
				shdi.sizeDragImage.cx = image.width();
				shdi.sizeDragImage.cy = image.height();
				shdi.ptOffset.x = hotspot.x();
				shdi.ptOffset.y = hotspot.y();
				shdi.hbmpDragImage = image.asNative().get();
				imageProvider_->InitializeFromBitmap(&shdi, mimeData_.get());
			}


			namespace detail {
				namespace {
					win32::com::SmartPointer<IDropTargetHelper> dropTargetHelper() {
						static boost::optional<win32::com::SmartPointer<IDropTargetHelper>> instance;
						if(instance == boost::none)
							instance = win32::com::SmartPointer<IDropTargetHelper>::create(CLSID_DragDropHelper, IID_IDropTargetHelper, CLSCTX_INPROC_SERVER);
						return boost::get(instance);
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
						fromNative<DropActions>(*effect), makeInterprocessData(data));
					try {
						target_.dragEntered(input);
					} catch(const std::bad_alloc&) {
						return E_OUTOFMEMORY;
					} catch(...) {
						return E_UNEXPECTED;
					}
					data_ = makeInterprocessData(data);
					if(input.dropAction() == boost::none)
						*effect = DROPEFFECT_NONE;
					else
						*effect = toNative<DWORD>(DropActions(boost::get(input.dropAction())));
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
							fromNative<DropActions>(*effect), data_));
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
							fromNative<DropActions>(*effect), makeInterprocessData(data)));
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

			boost::optional<DropAction> resolveDefaultDropAction(const DropActions& possibleActions, const event::MouseButtons&, const event::KeyboardModifiers& modifiers) {
				if(possibleActions.none())
					return boost::none;
				else if(possibleActions.count() == 1)
					return firstAction(possibleActions);

				boost::optional<DWORD> effect;
				if(modifiers == event::KeyboardModifiers(event::SHIFT_DOWN)
						|| modifiers == event::KeyboardModifiers(std::make_tuple(event::SHIFT_DOWN, event::ALT_DOWN))
						|| modifiers == event::KeyboardModifiers(std::make_tuple(event::CONTROL_DOWN, event::ALT_DOWN))
						|| modifiers == event::KeyboardModifiers(std::make_tuple(event::SHIFT_DOWN, event::CONTROL_DOWN, event::ALT_DOWN)))
					effect = DROPEFFECT_MOVE;
				else if(modifiers == event::KeyboardModifiers(event::CONTROL_DOWN))
					effect = DROPEFFECT_COPY;
				else if(modifiers == event::KeyboardModifiers(event::ALT_DOWN)
						|| modifiers == event::KeyboardModifiers(std::make_tuple(event::SHIFT_DOWN, event::CONTROL_DOWN)))
					effect = DROPEFFECT_LINK;

				if(effect == boost::none) {
					// query the system default setting
					static const WCHAR REGISTRY_VALUE_NAME[] = L"DefaultDropEffect\\*";
					DWORD systemDefaultEffect, type, size;
					if(ERROR_SUCCESS != ::RegQueryValueExW(HKEY_CLASSES_ROOT, REGISTRY_VALUE_NAME, nullptr, &type, reinterpret_cast<LPBYTE>(&systemDefaultEffect), &size) || type != REG_DWORD || size != sizeof(DWORD))
						// TODO: Accept other types and sizes
						systemDefaultEffect = DROPEFFECT_MOVE;
					effect = systemDefaultEffect;
				}

				DropActions action = fromNative<DropActions>(boost::get(effect));
				assert(action.count() == 1);
				action &= possibleActions;
				return action.any() ? firstAction(action) : firstAction(possibleActions);

			}
		}
	}
}

#endif
