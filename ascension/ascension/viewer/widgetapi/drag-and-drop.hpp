/**
 * @file drag-and-drop.hpp
 * @author exeal
 * @date 2011-10-12 created
 */

#ifndef ASCENSION_DRAG_AND_DROP_HPP
#define ASCENSION_DRAG_AND_DROP_HPP
#include <ascension/viewer/widgetapi/user-input.hpp>
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
#	include <gtkmm/selectiondata.h>
#elif defined(ASCENSION_WINDOW_SYSTEM_QT)
#	include <QMimeData>
#elif defined(ASCENSION_WINDOW_SYSTEM_QUARTZ)
#	include <NSPasteboard.h>
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
#	include <ObjIdl.h>
#endif

namespace ascension {
	namespace viewers {
		namespace widgetapi {

			typedef std::uint16_t DropAction;
			const DropAction DROP_ACTION_IGNORE = 0;
			const DropAction DROP_ACTION_COPY = 1 << 0;
			const DropAction DROP_ACTION_MOVE = 1 << 1;
			const DropAction DROP_ACTION_LINK = 1 << 2;
#ifdef ASCENSION_WINDOW_SYSTEM_WIN32
			const DropAction DROP_ACTION_WIN32_SCROLL = 1 << 3;
#endif
			DropAction resolveDefaultDropAction(DropAction possibleActions, UserInput::ModifierKey modifierKeys);

			class DragLeaveInput : public Event {};

#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
			typedef Gtk::SelectionData NativeMimeData;
#elif defined(ASCENSION_WINDOW_SYSTEM_QT)
			typedef QMimeData NativeMimeData;
#elif defined(ASCENSION_WINDOW_SYSTEM_QUARTZ)
			typedef NSPasteboard NativeMimeData;
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
			typedef IDataObject NativeMimeData;
#endif
			class DropInput : public MouseButtonInput {
			public:
				DropInput(const MouseButtonInput& mouse, DropAction possibleActions, const NativeMimeData& data) :
					MouseButtonInput(mouse), possibleActions_(possibleActions),
					defaultAction_(resolveDefaultDropAction(possibleActions, mouse.modifiers())), mimeData_(data) {}
				void acceptProposedAction();
				DropAction dropAction() const /*noexcept*/ {return action_;}
				const NativeMimeData& mimeData() const /*noexcept*/ {return mimeData_;}
				DropAction possibleActions() const /*noexcept*/ {return possibleActions_;}
				DropAction proposedAction() const /*noexcept*/ {return defaultAction_;}
				void setDropAction(DropAction action);
			private:
				const DropAction possibleActions_;
				const DropAction defaultAction_;
				DropAction action_;
				const NativeMimeData& mimeData_;
			};

			class DragMoveInput : public DropInput {
			public:
				DragMoveInput(const MouseButtonInput& mouse, DropAction possibleActions,
					const NativeMimeData& data) : DropInput(mouse, possibleActions, data) {}
				friend class Widget;
			};

			class DragEnterInput : public DragMoveInput {
			public:
				DragEnterInput(const MouseButtonInput& mouse, DropAction possibleActions,
					const NativeMimeData& data) : DragMoveInput(mouse, possibleActions, data) {}
				friend class Widget;
			};

			/**
			 * @see Widget
			 */
			class DropTarget {
			public:
				/// Destructor.
				virtual ~DropTarget() /*throw()*/ {}
				virtual void dragEntered(DragEnterInput& input) = 0;
				virtual void dragLeft(DragLeaveInput& input) = 0;
				virtual void dragMoved(DragMoveInput& input) = 0;
				virtual void dropped(DropInput& input) = 0;
			};

		}
	}
}

#endif // !ASCENSION_DRAG_AND_DROP_HPP
