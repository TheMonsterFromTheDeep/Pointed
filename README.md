## Pointed
Pointed is a very simple C++ library to enable usage of a particular kind of memory management: single owner, with any number of possible references, while also preventing dangling memory problems.

Right now, usually `std::unique_ptr<T>` is used for single-owner memory. However, every possible kind of reference to this memory has no real way to detect when the memory is deallocated, opening the program up to dangling reference errors.

Pointed enables this kind of memory management through two new types: `Pointed::Single<T>` and `Pointed::Ref<T>`.

A `Single<T>`, which is essentially a `std::unique_ptr<T>`, owns a pointer to `T`, and thus is subject to move semantics and RAII. After it leaves scope, the memory it owns is deallocated.

A `Ref<T>` is a reference to some `Single<T>` somewhere. After the `Single<T>` it references goes out of scope, the `Ref<T>` will act as though it references a null pointer, and will throw a `Pointed::InvalidRefException` on an attempt to access it. Note that a `Ref<T>` can have a different type than the `Single<T>` it references, as long as it can be `dynamic_cast`ed.

Note that this is not at all thread safe. In the following code:
```C++
/* Ref<int> ref */
if(ref) {
	std::cout << "The value: " << *ref << "\n";
}
```
It is entirely possible for the `Single<T>` that `ref` points to to become null between the null-check (`if(ref)`) and the actual usage of `ref`.

Pointed also includes some methods that ease development of code that uses its smart pointer types. The following code:
```C++
Single<int> owner(new int(20));
Ref<int> ref = owner;
```
is equivalent to the following (safer, and depending on preference, easier to read) code:
```C++
auto owner = my<int>(20); /* Safer than explicitly calling new */
auto ref = refer(owner);
```

Note that all symbols that Pointed defines reside inside the `Pointed` namespace, meaning they either need to be prefixed with `Pointed`, or they can be imported globally with `using namespace Pointed;` (however, this is not recommended for public API header files).

Finally, it should be noted that Pointed does have some additional overhead, which should be relatively obvious. It has to do some bookkeeping in order to be able to invalidate all of the additional `Ref<T>`s--namely, it stores a linked list that must be walked over whenever a `Single<T>` goes out of scope, and which must be modified whenever a `Ref<T>` is created or goes out of scope.