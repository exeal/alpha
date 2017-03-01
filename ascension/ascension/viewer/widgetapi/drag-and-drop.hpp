/**
 * @file drag-and-drop.hpp
 * @author exeal
 * @date 2011-10-12 created
 * @date 2014
 */

#ifndef ASCENSION_DRAG_AND_DROP_HPP
#define ASCENSION_DRAG_AND_DROP_HPP
#include <ascension/corelib/combination.hpp>
#include <ascension/corelib/interprocess-data.hpp>
#include <ascension/viewer/widgetapi/widget.hpp>
#include <ascension/viewer/widgetapi/event/located-user-input.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#	include <gdkmm/dragcontext.h>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#	include <QDrag>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#	include <NSDragging.h>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#	include <ShlObj.h>	// IDragSourceHelper
#endif
#include <boost/optional.hpp>

namespace ascension {
	namespace viewer {
		namespace widgetapi {
			enum DropAction {
				COPY_ACTION,			///< "Copy" action.
				MOVE_ACTION,			///< "Move" action.
				LINK_ACTION,			///< "Link" action.
				GTK_PRIVATE_ACTION,		///< For @c Gdk#ACTION_PRIVATE of gtkmm.
				GTK_ASK_ACTION,			///< For @c Gdk#ACTION_ASK of gtkmm.
				QT_ACTION_MASK,			///< For @c Qt#ActionMask
				QT_TARGET_MOVE_ACTION,	///< For @c Qt#TargetMoveAction of Qt.
				OSX_GENERIC_ACTION,		///< For @c NSDragOperationGeneric of Quartz.
				OSX_PRIVATE_ACTION,		///< For @c NSDragOperationPrivate of Quartz.
				OSX_DELETE_ACTION,		///< For @c NSDragOperationDelete of Quartz.
				OSX_EVERY_ACTION,		///< For @c NSDragOperationEvery of Quartz.
				WIN32_SCROLL_ACTION,	///< For @c DROPEFFECT_SCROLL of Win32.
				NUMBER_OF_DROP_ACTIONS
			};

			typedef Combination<DropAction, NUMBER_OF_DROP_ACTIONS> DropActions;

#if ASCENSION_SUPPORTS_WINDOW_SYSTEM(GTK)
			DropActions _fromNative(Gdk::DragAction native, const DropActions* = nullptr) {
				DropActions actions;
				if((native & Gdk::ACTION_COPY) != 0)
					actions.set(COPY_ACTION);
				if((native & Gdk::ACTION_MOVE) != 0)
					actions.set(MOVE_ACTION);
				if((native & Gdk::ACTION_LINK) != 0)
					actions.set(LINK_ACTION);
				if((native & Gdk::ACTION_PRIVATE) != 0)
					actions.set(GTK_PRIVATE_ACTION);
				if((native & Gdk::ACTION_ASK) != 0)
					actions.set(GTK_ASK_ACTION);
				return actions;
			}
			Gdk::DragAction _toNative(const DropActions& actions, const Gdk::DragAction* = nullptr) {
				Gdk::DragAction native = Gdk::ACTION_DEFAULT;
				if(action.test(COPY_ACTION))
					native |= Gdk::ACTION_COPY;
				if(action.test(MOVE_ACTION))
					native |= Gdk::ACTION_MOVE;
				if(action.test(LINK_ACTION))
					native |= Gdk::ACTION_LINK;
				if(action.test(GTK_PRIVATE_ACTION))
					native |= Gdk::ACTION_PRIVATE;
				if(action.test(GTK_ASK_ACTION))
					native |= Gdk::ACTION_ASK;
				return native;
			}
#endif
#if ASCENSION_SUPPORTS_WINDOW_SYSTEM(QT)
			DropActions _fromNative(const Qt::DropActions& native, const DropActions* = nullptr) {
				DropActions actions;
				if(native.testFlag(Qt::CopyAction))
					actions.set(COPY_ACTION);
				if(native.testFlag(Qt::MoveAction))
					actions.set(MOVE_ACTION);
				if(native.testFlag(Qt::LinkAction))
					actions.set(LINK_ACTION);
				if(native.testFlag(Qt::ActionMask))
					actions.set(QT_ACTION_MASK);
				if(native.testFlag(Qt::TargetMoveAction))
					actions.set(QT_TARGET_MOVE_ACTION);
				return actions;
			}
			Qt::DropActions _toNative(const DropActions& actions, const Qt::DropActions* = nullptr) {
				Qt::DropActions native(Qt::IgnoreAction);
				if(actions.test(COPY_ACTION))
					native.setFlag(Qt::CopyAction);
				if(actions.test(MOVE_ACTION))
					native.setFlag(Qt::MoveAction);
				if(actions.test(LINK_ACTION))
					native.setFlag(Qt::LinkAction);
				if(actions.test(QT_ACTION_MASK))
					native.setFlag(Qt::ActionMask);
				if(actions.test(QT_TARGET_MOVE_ACTION))
					native.setFlag(Qt::TargetMoveAction);
				return native;
			}
#endif
#if ASCENSION_SUPPORTS_WINDOW_SYSTEM(QUARTZ)
			DropActions _fromNative(NSDragOperation native, const DropActions* = nullptr) {
				DropActions actions;
				if((native & NSDragOperationCopy) != 0)
					actions.set(COPY_ACTION);
				if((native & NSDragOperationMove) != 0)
					actions.set(MOVE_ACTION);
				if((native & NSDragOperationLink) != 0)
					actions.set(LINK_ACTION);
				if((native & NSDragOperationGeneric) != 0)
					actions.set(OSX_GENERIC_ACTION);
				if((native & NSDragOperationPrivate) != 0)
					actions.set(OSX_PRIVATE_ACTION);
				if((native & NSDragOperationDelete) != 0)
					actions.set(OSX_DELETE_ACTION);
				if((native & NSDragOperationEvery) != 0)
					actions.set(OSX_EVERY_ACTION);
				return actions;
			}
#endif
#if ASCENSION_SUPPORTS_WINDOW_SYSTEM(WIN32)
			DropActions _fromNative(DWORD native, const DropActions* = nullptr) {
				DropActions actions;
				if((native & DROPEFFECT_COPY) != 0)
					actions.set(COPY_ACTION);
				if((native & DROPEFFECT_MOVE) != 0)
					actions.set(MOVE_ACTION);
				if((native & DROPEFFECT_LINK) != 0)
					actions.set(LINK_ACTION);
				if((native & DROPEFFECT_SCROLL) != 0)
					actions.set(WIN32_SCROLL_ACTION);
				return actions;
			}
			DWORD _toNative(const DropActions& actions, const DWORD* = nullptr) {
				DWORD native = DROPEFFECT_NONE;
				if(actions.test(COPY_ACTION))
					native |= DROPEFFECT_COPY;
				if(actions.test(MOVE_ACTION))
					native |= DROPEFFECT_MOVE;
				if(actions.test(LINK_ACTION))
					native |= DROPEFFECT_LINK;
				if(actions.test(WIN32_SCROLL_ACTION))
					native |= DROPEFFECT_SCROLL;
				return native;
			}
#endif

			boost::optional<DropAction> resolveDefaultDropAction(
				const DropActions& possibleActions,
				const event::MouseButtons& buttons, const event::KeyboardModifiers& modifiers);

			class DragContext {
			public:
				explicit DragContext(Widget::reference source) BOOST_NOEXCEPT : source_(source) {}
				DropAction defaultAction() const;
				DropAction execute(const DropActions& supportedActions
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
					, int mouseButton, GdkEvent* event
#endif
				);
				void setData(const InterprocessData& data);
				void setImage(const graphics::Image& image, const boost::geometry::model::d2::point_xy<std::uint32_t>& hotspot);
				DropActions supportedActions() const;
			private:
				Widget::reference source_;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				Glib::RefPtr<Gdk::DragContext> context_;
				std::shared_ptr<const InterprocessData> data_;
				Glib::RefPtr<Gdk::Pixbuf> icon_;
				int iconHotspotX_, iconHotspotY_;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				QDrag impl_;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				win32::com::SmartPointer<IDataObject> mimeData_;
				win32::com::SmartPointer<IDragSourceHelper> imageProvider_;
#else
				ASCENSION_CANT_DETECT_PLATFORM();
#endif
			};

			/**
			 * An event when a drag and drop action leaves the target.
			 * @see DragEnterInput, DragMoveInput, DropInput, DropTarget#dragLeft
			 */
			class DragLeaveInput : public event::Event {};

			/// The base class of @c DropInput and @c DragMoveInput classes.
			class DragInputBase : public event::LocatedUserInput {
			public:
				/**
				 * Sets the drop action to be the proposed action.
				 * @see #proposedAction, #setDropAction, Event#consume
				 */
				void acceptProposedAction() {setDropAction(proposedAction());}
				/**
				 * Returns the action to be performed on the data by the target.
				 * @see #setDropAction
				 */
				boost::optional<DropAction> dropAction() const BOOST_NOEXCEPT {return action_;}
				/**
				 * Returns the possible drop actions.
				 * @see #dropAction
				 */
				const DropActions& possibleActions() const BOOST_NOEXCEPT {return possibleActions_;}
				/**
				 * Returns the proposed drop action.
				 * @see #dropAction
				 */
				boost::optional<DropAction> proposedAction() const BOOST_NOEXCEPT {return defaultAction_;}
				/**
				 * Sets the specified @a action to be performed on the data by the target.
				 * @see #dropAction
				 */
				void setDropAction(boost::optional<DropAction> action) {action_ = action;}

			protected:
				/**
				 * Constructs a @c DropInputBase object.
				 * @param userInput The base input information
				 * @param possibleActions The possible drop actions
				 */
				DragInputBase(const LocatedUserInput& userInput, const DropActions& possibleActions) :
					event::LocatedUserInput(userInput), possibleActions_(possibleActions),
					defaultAction_(resolveDefaultDropAction(possibleActions, userInput.buttons(), userInput.modifiers())) {}

			private:
				const DropActions possibleActions_;
				const boost::optional<DropAction> defaultAction_;
				boost::optional<DropAction> action_;
			};

			/**
			 * An event when a drag and drop action is completed.
			 * @see InterprocessdData, DropTarget#dropped
			 */
			class DropInput : public DragInputBase {
			public:
				/**
				 * Creates a @c DropInput object.
				 * @param userInput The base input information
				 * @param possibleActions The possible drop actions
				 * @param data The data that was dropped on the target
				 * @throw NullPointerException @a data is @c null
				 */
				DropInput(const LocatedUserInput& userInput, const DropActions& possibleActions, std::shared_ptr<const InterprocessData> data) : DragInputBase(userInput, possibleActions), data_(data) {
					if(data.get() == nullptr)
						throw NullPointerException("data");
				}
				/// Returns the data that was dropped on the target.
				const InterprocessData& data() const BOOST_NOEXCEPT {return *data_;}

			private:
				const std::shared_ptr<const InterprocessData> data_;
			};

			/**
			 * An event while a drag and drop action is in progress.
			 * @see DragEnterInput, DragLeaveInput, DropInput, DropTarget#dragMoved
			 */
			class DragMoveInput : public DragInputBase {
			public:
				/**
				 * Creates a @c DragMoveInput object.
				 * @param userInput The base input information
				 * @param possibleActions The possible drop actions
				 * @param formats The formats of the data that was moved on the target
				 * @throw NullPointerException @a formats is @c null
				 */
				DragMoveInput(const LocatedUserInput& userInput, const DropActions& possibleActions,
						std::shared_ptr<const InterprocessDataFormats> formats) : DragInputBase(userInput, possibleActions), formats_(formats) {
					if(formats.get() == nullptr)
						throw NullPointerException("formats");
				}
				/**
				 * Notifies that future motions will also be accepted if they remain within the specified rectangle.
				 * @param rectangle The rectangle on the target
				 */
				void accept(boost::optional<const graphics::Rectangle> rectangle) {}	// TODO: Not implemented.
				/**
				 * Notifies that future motions within the specified rectangle are not acceptable.
				 * @param rectangle The rectangle on the target
				 */
				void ignore(boost::optional<const graphics::Rectangle> rectangle) {}	// TODO: Not implemented.
				/// Returns the formats of the data that was moved on the target.
				const InterprocessDataFormats& dataFormats() const BOOST_NOEXCEPT {return *formats_;}

			private:
				const std::shared_ptr<const InterprocessDataFormats> formats_;
			};

			/**
			 * An event when a drag and drop action entered the target.
			 * @see DropTarget#dragEntered
			 */
			class DragEnterInput : public DragMoveInput {
			public:
				/**
				 * Creates a @c DragEnterInput object.
				 * @param userInput The base input information
				 * @param possibleActions The possible drop actions
				 * @param formats The formats of the data that was moved on the target
				 * @throw NullPointerException @a formats is @c null
				 */
				DragEnterInput(const LocatedUserInput& userInput, const DropActions& possibleActions,
					std::shared_ptr<const InterprocessDataFormats> formats) : DragMoveInput(userInput, possibleActions, formats) {}
			};

			/**
			 */
			class DropTarget {
			public:
				/// Destructor.
				virtual ~DropTarget() BOOST_NOEXCEPT {}
				virtual void dragEntered(DragEnterInput& input) = 0;
				virtual void dragLeft(DragLeaveInput& input) = 0;
				virtual void dragMoved(DragMoveInput& input) = 0;
				virtual void dropped(DropInput& input) = 0;
			};

			namespace detail {
				class DragEventAdapter {
				public:
					DragEventAdapter(DropTarget& target, Proxy<Widget>& widget) : target_(target), widget_(widget) {}
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
					void adaptDragLeaveEvent(const Glib::RefPtr<Gdk::DragContext>& context, guint time);
					bool adaptDragMoveEvent(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, guint time);
					bool adaptDropEvent(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, guint time);
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
					void adaptDragEnterEvent(QDragEnterEvent* event);
					void adaptDragLeaveEvent(QDragLeaveEvent* event);
					void adaptDragMoveEvent(QDragMoveEvent* event);
					void adaptDropEvent(QDropEvent* event);
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
					HRESULT adaptDragEnterEvent(IDataObject* data, DWORD keyState, POINTL location, DWORD* effect);
					HRESULT adaptDragLeaveEvent();
					HRESULT adaptDragMoveEvent(DWORD keyState, POINTL location, DWORD* effect);
					HRESULT adaptDropEvent(IDataObject* data, DWORD keyState, POINTL location, DWORD* effect);
#else
					ASCENSION_CANT_DETECT_PLATFORM();
#endif
				private:
					DropTarget& target_;
					Proxy<Widget> widget_;
					std::shared_ptr<const InterprocessData> data_;
				};
			}
		}
	}
}

#endif // !ASCENSION_DRAG_AND_DROP_HPP
