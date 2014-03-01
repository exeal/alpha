/**
 * @file function-pointer.hpp
 */

#ifndef ALPHA_FUNCTION_POINTER_HPP
#define ALPHA_FUNCTION_POINTER_HPP
#include <boost/mpl/size.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/preprocessor/repetition/enum_shifted_binary_params.hpp>
#include <boost/preprocessor/repetition/enum_shifted_params.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>

namespace alpha {
	namespace ambient {
		namespace detail {
			template<typename Lambda>
			struct Signature : Signature<decltype(&Lambda::operator())> {
			};
			template<typename R, typename K>
			struct Signature<R(K::*)() const> {
				typedef boost::mpl::vector<R> Arguments;
			};

#define ALPHA_SPECIALIZE_AMBIENT_SIGNATURE(z, arity, _)										\
	template<typename R, typename K, BOOST_PP_ENUM_SHIFTED_PARAMS(arity, typename A)>		\
	struct Signature<R(K::*)(BOOST_PP_ENUM_SHIFTED_PARAMS(arity, A)) const> {				\
		typedef boost::mpl::vector<R, BOOST_PP_ENUM_SHIFTED_PARAMS(arity, A)> Arguments;	\
	};

			BOOST_PP_REPEAT_FROM_TO(2, 10, ALPHA_SPECIALIZE_AMBIENT_SIGNATURE, _)

#undef ALPHA_SPECIALIZE_AMBIENT_SIGNATURE

			template<typename Lambda, int arity> class Caller;

			template<typename Lambda> struct LambdaHolder {
				template<int N>
				struct Arguments {
					typedef typename boost::mpl::at<typename Signature<Lambda>::Arguments, boost::mpl::int_<N>>::type type;
				};
				typedef typename Arguments<0>::type ResultType;
				LambdaHolder(Lambda* f) {
					this->f = f;
				}
				static Lambda* f;
			};
			template<typename Lambda> Lambda* LambdaHolder<Lambda>::f = nullptr;

#define ALPHA_SPECIALIZE_AMBIENT_CALLER(z, arity, _)															\
	template<typename Lambda> struct Caller<Lambda, arity> : LambdaHolder<Lambda> {								\
		Caller(Lambda* f) : LambdaHolder(f) {}																	\
		static ResultType call(BOOST_PP_ENUM_SHIFTED_BINARY_PARAMS(arity, typename Arguments<, >::type a)) {	\
			return (*f)(BOOST_PP_ENUM_SHIFTED_PARAMS(arity, a));												\
		}																										\
	};

			BOOST_PP_REPEAT_FROM_TO(1, 10, ALPHA_SPECIALIZE_AMBIENT_CALLER, _)
			
#undef ALPHA_SPECIALIZE_AMBIENT_CALLER

			template<typename Lambda>
			class FunctionPointer : public Caller<Lambda, boost::mpl::size<typename Signature<Lambda>::Arguments>::value> {
			public:
				FunctionPointer(Lambda f) : Caller<Lambda, boost::mpl::size<typename Signature<Lambda>::Arguments>::value>(&f) {}
			};
		}

		template<typename Lambda>
		inline auto makeFunctionPointer(Lambda f) -> decltype(&detail::FunctionPointer<Lambda>::call) {
			static Lambda f_(f);
			detail::FunctionPointer<Lambda> fp_(f_);
			return &detail::FunctionPointer<Lambda>::call;
		}
	}
}

#endif // !ALPHA_FUNCTION_POINTER_HPP
