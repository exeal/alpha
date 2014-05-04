/**
 * @file scope-guard.hpp
 * @author exeal
 * @date 2004-2010 Was common.hpp
 * @date 2010-10-21 Separated from common.hpp
 * @date 2010-11-07 Joined with common.hpp
 * @date 2011-2014 Was basic-types.hpp
 * @date 2014-05-04 Separated from basic-types.hpp
 */

#ifndef ASCENSION_SCOPE_GUARD_HPP
#define ASCENSION_SCOPE_GUARD_HPP
#include <memory>
#include <type_traits>
#include <boost/noncopyable.hpp>

namespace ascension {
	namespace detail {
		/// Implements minimal "Scope Guard" idiom.
		class ScopeGuard : private boost::noncopyable {
		public:
			/// Constructor takes a functor called when this @c ScopeGuard object is destructed.
			template<typename Functor>
			explicit ScopeGuard(Functor functor) : exit_(new TypedExitFunctor<Functor>(functor)) {}
			/// Move-constructor.
			ScopeGuard(ScopeGuard&& other) BOOST_NOEXCEPT : exit_(std::move(other.exit_)) {}
			/// Move-assignment operator.
			ScopeGuard& operator=(ScopeGuard&& other) BOOST_NOEXCEPT {
				exit_ = std::move(other.exit_);
				return *this;
			}

		private:
			ScopeGuard() /* = delete */;
			class ExitFunctor {};
			template<typename Functor> class TypedExitFunctor : public ExitFunctor {
			public:
				explicit TypedExitFunctor(Functor functor) : functor_(functor) {}
				~TypedExitFunctor() {functor_();}
			private:
				Functor functor_;
			};
			std::unique_ptr<ExitFunctor> exit_;
		};

		/// Interface of object which implements @c BasicLockable concept.
		struct Locker {
			/// Locks the resource.
			virtual void lock() = 0;
			/// Unlock the resource.
			virtual void unlock() BOOST_NOEXCEPT = 0;
		};

		template<typename Target, typename LockMethod, typename UnlockMethod>
		struct LockerWithClass : public Locker {
			LockerWithClass(Target* target, LockMethod lockMethod, UnlockMethod unlockMethod) : target_(target), lockMethod_(lockMethod), unlockMethod_(unlockMethod) {
			}
			void lock() override {
				if(target_ != nullptr)
					(target_->*lockMethod_)();
			}
			void unlock() override {
				if(target_ != nullptr)
					(target_->*unlockMethod_)();
			}
		private:
			Target* const target_;
			LockMethod lockMethod_;
			UnlockMethod unlockMethod_;
		};

		template<typename Target, typename LockFunction, typename UnlockFunction>
		struct LockerWithFreeFunctions : public Locker {
			LockerWithFreeFunctions(Target* taget, LockFunction lockFunction, UnlockFunction unlockFunction) : target_(taget), lockFunction_(lockFunction), unlockFunction_(unlockFunction) {
			}
			void lock() override {
				if(target_ != nullptr)
					lockFunction_(*target_);
			}
			void unlock() override {
				if(target_ != nullptr)
					unlockFunction_(*target_);
			}
		private:
			Target* const target_;
			LockFunction lockFunction_;
			UnlockFunction unlockFunction_;
		};

		class Mutex {
		public:
			template<typename Target, typename LockFunction, typename UnlockFunction>
			Mutex(Target* target, LockFunction lockFunction, UnlockFunction unlockFunction,
					typename std::enable_if<std::is_member_function_pointer<LockFunction>::value && std::is_member_function_pointer<UnlockFunction>::value>::type* = nullptr)
					: locker_(new LockerWithClass<Target, LockFunction, UnlockFunction>(target, lockFunction, unlockFunction)) {
			}
			template<typename Target, typename LockFunction, typename UnlockFunction>
			Mutex(Target* target, LockFunction lockFunction, UnlockFunction unlockFunction,
					typename std::enable_if<!std::is_member_function_pointer<LockFunction>::value || !std::is_member_function_pointer<UnlockFunction>::value>::type* = nullptr)
					: locker_(new LockerWithFreeFunctions<Target, LockFfunction, UnlockFunction>(target, lockFunction, unlockFunction)) {
			}
			void lock() {
				locker_->lock();
			}
			void unlock() {
				locker_->unlock();
			}
		private:
			std::unique_ptr<Locker> locker_;
		};

		template<typename Lockable, void (Lockable::*lockMethod)(void), void (Lockable::*unlockMethod)(void)>
		class MutexWithClass : public LockerWithClass<Lockable, decltype(lockMethod), decltype(unlockMethod)> {
		public:
			MutexWithClass(Lockable* lockable) : LockerWithClass(lockable, lockMethod, unlockMethod) {}
		};

		template<typename Lockable, void (*lockMethod)(Lockable&), void (*unlockMethod)(Lockable&)>
		class MutexWithFreeFunctions : public LockerWithFreeFunctions<Lockable, decltype(lockMethod), decltype(unlockMethod)> {
		public:
			MutexWithFreeFunctions(Lockable* lockable) : LockerWithFreeFunctions(lockable, lockMethod, unlockMethod) {}
		};
	}
}

#endif // !ASCENSION_SCOPE_GUARD_HPP
