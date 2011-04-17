/**
 * @file native-object.hpp
 * @author exeal
 * @date 2011-04-17
 */

#ifndef ASCENSION_NATIVE_OBJECT_HPP
#define ASCENSION_NATIVE_OBJECT_HPP

namespace ascension {

	/**
	 *
	 * @tparam T The type of the object
	 */
	template<typename T>
	class NativeObject {
	public:
		typedef T element_type;	///< The type of the object.
		struct Api {
			static void addReference(element_type object);
			static void release(element_type object);
		};
	public:
		/// Default constructor.
		NativeObject() : object_(0) {}
		/***/
		explicit NativeObject(element_type object, bool addReference = false) : object_(object) {
			if(addReference && object_ != 0)
				Api::addReference(object_);
		}
		/// Copy-constructor.
		NativeObject(const NativeObject& other) : object_(other.get()) {
			if(object_ != 0)
				Api::addReference(object_);
		}
		/// Copy-constructor.
		template<typename U>
		NativeObject(const NativeObject<U>& other) : object_(other.get()) {
			if(object_ != 0)
				Api::addReference(object_);
		}
		/// Destructor.
		~NativeObject() {
			if(object_ != 0)
				Api::release(object_);
		}
		/// Assignment operator.
		NativeObject& operator=(const NativeObject& other) {
			NativeObject(other).swap(*this);
			return *this;
		}
		/// Assignment operator.
		template<typename U>
		NativeObject& operator=(const NativeObject<U>& other) {
			NativeObject(other).swap(*this);
			return *this;
		}
		/// Assignment operator.
		NativeObject& operator=(const element_type other) {
			NativeObject(other).swap(*this);
			return *this;
		}
		/// Resets the object.
		void reset() {NativeObject().swap(*this);}
		/**
		 * Resets with the new object.
		 * @param object The new object
		 */
		void reset(element_type object) {NativeObject(object).swap(*this);}
		/// Returns the object.
		element_type get() const {return object_;}
		/// Swaps 
		void swap(NativeObject& other) {
			element_type temp(object_);
			object_ = other.get();
			other.object_ = temp;
		}
	private:
		element_type object_;
	};

	template<typename T, typename U>
	inline bool operator==(const NativeObject<T>& t, const NativeObject<U>& u) {
		return t.get() == u.get();
	}

	template<typename T, typename U>
	inline bool operator!=(const NativeObject<T>& t, const NativeObject<U>& u) {return !(t == u);}

	template<typename T, typename U>
	inline bool operator==(const NativeObject<T>& t, const U* u) {return t.get() == u;}

	template<typename T, typename U>
	inline bool operator!=(const NativeObject<T>& t, const U* u) {return !(t == u);}

	template<typename T, typename U>
	inline bool operator==(const T* t, const NativeObject<U>& u) {return t == u.get();}

	template<typename T, typename U>
	inline bool operator!=(const T* t, const NativeObject<U>& u) {return !(t == u);}

}

#endif // !ASCENSION_NATIVE_OBJECT_HPP
