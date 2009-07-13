/**
 * @file message-box.cpp Exposes @c MessageBox class.
 * @author exeal
 * @date 2009
 */

#include "ambient.hpp"
#include "application.hpp"
using namespace alpha;
using namespace std;
namespace py = boost::python;

namespace {
	class MessageBox {
	public:
		enum Buttons {};
		enum DefaultButton {};
		enum Icon {};
		enum Options {};
		enum Result {};

		static Result show(const wstring& message, const wstring& caption,
				Buttons buttons, Icon icon, DefaultButton defaultButton, Options options) {
			const int answer = Alpha::instance().getMainWindow().messageBox(
				message.c_str(), caption.c_str(), buttons | icon | defaultButton | options);
			if(answer == 0)
				ambient::Interpreter::instance().raiseLastWin32Error();
			return static_cast<Result>(answer);
		}
	};
}

ALPHA_EXPOSE_PROLOGUE(20)
	py::scope module(ambient::Interpreter::instance().module("ui"));

	py::class_<MessageBox> klass("MessageBox", py::no_init);

	{
		py::scope klassScope(klass);
		py::enum_<MessageBox::Buttons>("Buttons")
			.value("abort_try_ignore", static_cast<MessageBox::Buttons>(MB_ABORTRETRYIGNORE))
			.value("cancel_try_continue", static_cast<MessageBox::Buttons>(MB_CANCELTRYCONTINUE))
			.value("ok", static_cast<MessageBox::Buttons>(MB_OK))
			.value("ok_cancel", static_cast<MessageBox::Buttons>(MB_OKCANCEL))
			.value("retry_cancel", static_cast<MessageBox::Buttons>(MB_RETRYCANCEL))
			.value("yes_no", static_cast<MessageBox::Buttons>(MB_YESNO))
			.value("yes_no_cancel", static_cast<MessageBox::Buttons>(MB_YESNOCANCEL));
		py::enum_<MessageBox::DefaultButton>("DefaultButton")
			.value("button1", static_cast<MessageBox::DefaultButton>(MB_DEFBUTTON1))
			.value("button2", static_cast<MessageBox::DefaultButton>(MB_DEFBUTTON2))
			.value("button3", static_cast<MessageBox::DefaultButton>(MB_DEFBUTTON3))
			.value("button4", static_cast<MessageBox::DefaultButton>(MB_DEFBUTTON4));
		py::enum_<MessageBox::Icon>("Icon")
			.value("none", static_cast<MessageBox::Icon>(0))
			.value("exclamation", static_cast<MessageBox::Icon>(MB_ICONEXCLAMATION))
			.value("asterisk", static_cast<MessageBox::Icon>(MB_ICONASTERISK))
			.value("error", static_cast<MessageBox::Icon>(MB_ICONERROR))
			.value("hand", static_cast<MessageBox::Icon>(MB_ICONHAND))
			.value("information", static_cast<MessageBox::Icon>(MB_ICONINFORMATION))
			.value("question", static_cast<MessageBox::Icon>(MB_ICONQUESTION))
			.value("stop", static_cast<MessageBox::Icon>(MB_ICONSTOP))
			.value("warning", static_cast<MessageBox::Icon>(MB_ICONWARNING));
		py::enum_<MessageBox::Options>("Options")
			.value("none", static_cast<MessageBox::Options>(0))
			.value("default_desktop_only", static_cast<MessageBox::Options>(MB_DEFAULT_DESKTOP_ONLY))
			.value("right_align", static_cast<MessageBox::Options>(MB_RIGHT))
			.value("rtl_reading", static_cast<MessageBox::Options>(MB_RTLREADING))
			.value("service_notification", static_cast<MessageBox::Options>(MB_SERVICE_NOTIFICATION));
		py::enum_<MessageBox::Result>("Result")
			.value("abort", static_cast<MessageBox::Result>(IDABORT))
			.value("cancel", static_cast<MessageBox::Result>(IDCANCEL))
			.value("continue", static_cast<MessageBox::Result>(IDCONTINUE))
			.value("ignore", static_cast<MessageBox::Result>(IDIGNORE))
			.value("no", static_cast<MessageBox::Result>(IDNO))
			.value("ok", static_cast<MessageBox::Result>(IDOK))
			.value("retry", static_cast<MessageBox::Result>(IDRETRY))
			.value("try_again", static_cast<MessageBox::Result>(IDTRYAGAIN))
			.value("yes", static_cast<MessageBox::Result>(IDYES));
	}

	klass.def("show", &MessageBox::show, (py::arg("message"),
		py::arg("caption") = wstring(L"Alpha"),
		py::arg("buttons") = static_cast<MessageBox::Buttons>(MB_OK),
		py::arg("icon") = static_cast<MessageBox::Icon>(0),
		py::arg("default_button") = static_cast<MessageBox::DefaultButton>(MB_DEFBUTTON1),
		py::arg("options") = static_cast<MessageBox::Options>(0)))
		.staticmethod("show");

ALPHA_EXPOSE_EPILOGUE()
