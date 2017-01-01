/**
 * @file drag-and-drop.hpp
 * @author exeal
 * @date 2011-10-12 created
 * @date 2014
 */

#ifndef ASCENSION_DRAG_AND_DROP_HPP
#define ASCENSION_DRAG_AND_DROP_HPP
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
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			typedef Gdk::DragAction DropAction;
			const DropAction DROP_ACTION_IGNORE = Gdk::ACTION_DEFAULT;
			const DropAction DROP_ACTION_COPY = Gdk::ACTION_COPY;
			const DropAction DROP_ACTION_MOVE = Gdk::ACTION_MOVE;
			const DropAction DROP_ACTION_LINK = Gdk::ACTION_LINK;
			const DropAction DROP_ACTION_GTK_PRIVATE = Gdk::ACTION_PRIVATE;
			const DropAction DROP_ACTION_GTK_ASK = Gdk::ACTION_ASK;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
			typedef ??? DropAction;
			const DropAction DROP_ACTION_IGNORE = Qt::IgnoreAction;
			const DropAction DROP_ACTION_COPY = Qt::CopyAction;
			const DropAction DROP_ACTION_MOVE = Qt::MoveAction;
			const DropAction DROP_ACTION_LINK = Qt::LinkAction;
			const DropAction DROP_ACTION_QT_MASK = Qt::ActionMask;
			const DropAction DROP_ACTION_QT_TARGET_MOVE = Qt::TargetMoveAction;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
			typedef ??? DropAction;
			const DropAction DROP_ACTION_IGNORE = NSDragOperationNone;
			const DropAction DROP_ACTION_COPY = NSDragOperationCopy;
			const DropAction DROP_ACTION_MOVE = NSDragOperationMove;
			const DropAction DROP_ACTION_LINK = NSDragOperationLink;
			const DropAction DROP_ACTION_OSX_GENERIC = NSDragOperationGeneric;
			const DropAction DROP_ACTION_OSX_PRIVATE = NSDragOperationPrivate;
			const DropAction DROP_ACTION_OSX_DELETE = NSDragOperationDelete;
			const DropAction DROP_ACTION_OSX_EVERY = NSDragOperationEvery;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			typedef DWORD DropAction;
			const DropAction DROP_ACTION_IGNORE = DROPEFFECT_NONE;
			const DropAction DROP_ACTION_COPY = DROPEFFECT_COPY;
			const DropAction DROP_ACTION_MOVE = DROPEFFECT_MOVE;
			const DropAction DROP_ACTION_LINK = DROPEFFECT_LINK;
			const DropAction DROP_ACTION_WIN32_SCROLL = DROPEFFECT_SCROLL;
#else
			ASCENSION_CANT_DETECT_PLATFORM();
#endif
			DropAction resolveDefaultDropAction(DropAction possibleActions,
				const event::MouseButtons& buttons, const event::KeyboardModifiers& modifiers);

			class DragContext {
			public:
				explicit DragContext(Widget::reference source) BOOST_NOEXCEPT : source_(source) {}
				DropAction defaultAction() const;
				DropAction execute(DropAction supportedActions
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
					, int mouseButton, GdkEvent* event
#endif
				);
				void setData(std::shared_ptr<const InterprocessData> data);
				void setImage(const graphics::Image& image, const boost::geometry::model::d2::point_xy<std::uint32_t>& hotspot);
				DropAction supportedActions() const;
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
				DropAction dropAction() const BOOST_NOEXCEPT {return action_;}
				/**
				 * Returns the possible drop actions.
				 * @see #dropAction
				 */
				DropAction possibleActions() const BOOST_NOEXCEPT {return possibleActions_;}
				/**
				 * Returns the proposed drop action.
				 * @see #dropAction
				 */
				DropAction proposedAction() const BOOST_NOEXCEPT {return defaultAction_;}
				/**
				 * Sets the specified @a action to be performed on the data by the target.
				 * @see #dropAction
				 */
				void setDropAction(DropAction action) {action_ = action;}

			protected:
				/**
				 * Constructs a @c DropInputBase object.
				 * @param userInput The base input information
				 * @param possibleActions The possible drop actions
				 */
				DragInputBase(const LocatedUserInput& userInput, DropAction possibleActions) :
					event::LocatedUserInput(userInput), possibleActions_(possibleActions),
					defaultAction_(resolveDefaultDropAction(possibleActions, userInput.buttons(), userInput.modifiers())) {}

			private:
				const DropAction possibleActions_, defaultAction_;
				DropAction action_;
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
				DropInput(const LocatedUserInput& userInput, DropAction possibleActions, std::shared_ptr<const InterprocessData> data) : DragInputBase(userInput, possibleActions), data_(data) {
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
				DragMoveInput(const LocatedUserInput& userInput, DropAction possibleActions,
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
				DragEnterInput(const LocatedUserInput& userInput, DropAction possibleActions,
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
