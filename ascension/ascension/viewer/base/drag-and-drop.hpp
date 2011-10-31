/**
 * @file drag-and-drop.hpp
 * @author exeal
 * @date 2011-10-12 created
 */

#ifndef ASCENSION_DRAG_AND_DROP_HPP
#define ASCENSION_DRAG_AND_DROP_HPP
#include <ascension/viewer/base/user-input.hpp>

namespace ascension {
	namespace viewers {
		namespace base {

			typedef uint16_t DropAction;
			const DropAction DROP_ACTION_IGNORE = 0;
			const DropAction DROP_ACTION_COPY = 1 << 1;
			const DropAction DROP_ACTION_MOVE = 1 << 2;
			const DropAction DROP_ACTION_LINK = 1 << 3;
#ifdef ASCENSION_WINDOW_SYSTEM_WIN32
			const DropAction DROP_ACTION_WIN32_SCROLL = 1 << 4;
#endif
			DropAction resolveDefaultDropAction(DropAction possibleActions, UserInput::ModifierKey modifierKeys);

			class DragLeaveInput : public Event {};

			class DropInput : public MouseButtonInput {
			public:
				void acceptProposedAction();
				DropAction dropAction() const {return action_;}
				DropAction possibleActions() const {return possibleActions_;}
				DropAction proposedAction() const {return defaultAction_;}
				void setDropAction(DropAction action);
			protected:
				DropInput(const base::MouseButtonInput& mouse, DropAction possibleActions) :
					MouseButtonInput(mouse), possibleActions_(possibleActions),
					defaultAction_(resolveDefaultDropAction(possibleActions, mouse.modifiers())) {}
			private:
				const DropAction possibleActions_;
				const DropAction defaultAction_;
				DropAction action_;
				friend class Widget;
			};

			class DragMoveInput : public DropInput {
			protected:
				DragMoveInput(const base::MouseButtonInput& mouse, DropAction possibleActions) : DropInput(mouse, possibleActions) {}
				friend class Widget;
			};

			class DragEnterInput : public DragMoveInput {
			private:
				DragEnterInput(const base::MouseButtonInput& mouse, DropAction possibleActions) : DragMoveInput(mouse, possibleActions) {}
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
