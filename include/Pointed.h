/* Copyright (c) 2018 TheMonsterFromTheDeep
 * 
 * This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
 * 
 *     1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
 * 
 *     2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
 * 
 *     3. This notice may not be removed or altered from any source distribution.
*/

#ifndef POINTED_H_
#define POINTED_H_

#ifndef __cplusplus
#error "Pointed is a C++ library, and should not be used with a C compiler."
#else

#include <exception>
#include <typeinfo>

namespace Pointed {
	template<class T> class Single;
	template<class T> class Ref;

	class InvalidRefException : public std::exception {
	public:
		inline InvalidRefException() { }
		inline const char *what() const noexcept { return "Attempt to access invalid Ref."; }
	};

	/* Similar to std::unique_ptr<> -- owns a piece of memory, and can transfer it. Can have
	 * any number of Refs pointing at it, and will set them all to null when it deallocates.
	 *
	 * Essentially enables weak_ptr<>-like behavior for unique_ptr<>-like ownership.
	 */
	template<class T>
	class Single {
		T *pointer;
		/* We use a void* pointer so that Refs can be of different (compatible) types (hooray
		 * for polymorphism); this might be unnecessary but in any case it won't be a breaking
		 * change to change it.
		 */
		mutable void *top = nullptr;
	public:
		Single(const Single<T>& other) = delete; /* No copy constructor--this owns memory */

		explicit Single(T *toTakeOwnershipOf) : pointer(toTakeOwnershipOf) { }

		Single(Single<T> &&other) noexcept : pointer(other.pointer), top(other.top) {
			other.pointer = nullptr;
			other.top = nullptr;
		}

		template<class O>
		Single(Single<O> &&other) {
			pointer = dynamic_cast<T*>(other.pointer);
			if (!pointer) {
				throw std::bad_cast();
			}
			top = other.top;
			other.pointer = nullptr;
			other.top = nullptr;
		}

		~Single() {
			delete pointer;
			if (top) ((Ref<T>*)top)->invalidate();
		}

		bool operator==(const Single<T>& other) { return pointer == other.pointer; }
		bool operator==(const Ref<T>& ref) { return pointer == ref.pointer; }

		T *operator->() { return pointer; }
		T &operator*() { return *pointer; }
		T &self() { return operator*(); }
		
		friend class Ref<T>;

		template<class U>
		friend class Single;
	};

	/* This is the primary method of constructing Singles -- similar to make_unique, but
	 * with a shorter name.
	 *
	 * Usage example:
	 * auto owner = my<int>(20);
	 */
	template<class T, class... Args>
	Single<T> my(Args&& ...args) {
		return Single<T>(new T(std::forward<Args>(args)...));
	}

	template<class T>
	class Ref {
		T *pointer = nullptr;
		const mutable void *last = nullptr;
		const mutable void *next = nullptr;

		void nullify() {
			pointer = nullptr;
			last = nullptr;
			next = nullptr;
		}

		void invalidate() {
			nullify();
			if (next) ((Ref<T>*)next)->invalidate();
		}

		void delink() {
			if (next) ((Ref<T>*)next)->last = last;
			if (last) ((Ref<T>*)last)->next = next;
		}
	public:
		Ref() { }

		~Ref() {
			delink();
		}

		Ref(const Ref<T>& other) {
			operator=(other);
		}

		Ref(const Single<T>& single) {
			operator=(single);
		}

		Ref<T>& operator=(const Ref<T>& other);
		Ref<T>& operator=(const Single<T>& single);
		bool operator==(const Ref<T>& other) { return pointer == other.pointer; }
		bool operator==(const Single<T>& single) { return pointer == single.pointer; }

		T* operator->() {
			if (!pointer) throw InvalidRefException();
			return (T*)pointer;
		}

		T& operator*() {
			if (!pointer) throw InvalidRefException();
			return *reinterpret_cast<T*>(pointer);
		}

		T& self() { return operator*(); }

		operator bool() { return pointer != nullptr; }

		friend class Single<T>;

		template<class U>
		friend class Ref;

		template<class To, class From>
		friend Ref<To> RefCast(const Ref<From>& ref);
	};

	/* Returns a null Ref. */
	template<class T>
	Ref<T> NullRef() {
		return Ref<T>();
	}

	/* Essentially does a dynamic_cast from one Ref<> to another. */
	template<class To, class From>
	Ref<To> RefCast(const Ref<From>& ref) {
		To *pointerNew = dynamic_cast<To*>(ref.pointer);

		if (pointerNew) {
			Ref<To> result;

			result.pointer = pointerNew;

			result.next = (void*)&ref;
			if (ref.last) {
				result.last = ref.last;
				((Ref<From>*)result.last)->next = (void*)(&result);
			}
			ref.last = (void*)(&result);

			return result;
		}
		else {
			return NullRef<To>();
		}
	}

	template<class T>
	Ref<T>& Ref<T>::operator=(const Ref<T>& other) {
		delink();
		nullify();

		pointer = other.pointer;

		next = (void*)&other;
		if (other.last) {
			last = other.last;
			((Ref<T>*)last)->next = (void*)this;
		}
		other.last = (void*)this;

		return *this;
	}

	template<class T>
	Ref<T>& Ref<T>::operator=(const Single<T>& single) {
		delink();
		nullify();

		pointer = single.pointer;

		if (single.top) {
			next = (void*)single.top;
			((Ref<T>*)single.top)->last = this;
		}
		single.top = (void*)this;
		
		return *this;
	}
	
	template<class T>
	Ref<T> refer(const Single<T>& single) { return Ref<T>(single); }
	
	template<class T>
	Ref<T> refer(const Ref<T>& ref) { return Ref<T>(ref); }
}

#endif /* C++ */

#endif /* Header guard */