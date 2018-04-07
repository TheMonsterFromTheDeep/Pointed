#include <iostream>
#include "include/Pointed.h"

using namespace Pointed;

/* The purpose of this demo is to demonstrate the main property of Pointed
 * that is not satisfied by std::unique_ptr and friends: it is possible to have
 * a reference to memory that will properly act as nullptr when it would otherwise
 * point to dangling memory--thus it is a reference to owned memory that
 *   a) will not prevent the memory from deallocating
 *   b) will not be dangling if the memory is deallocated
 *
 * To demonstrate this, we create an owned integer pointer inside the test()
 * method, and point out reference at it. After test() exits, the owned memory
 * should be deallocated due to RAII, and the reference should become null.
 */

Ref<int> myIntRef;

void test() {
	auto ownedInt = my<int>(20);
	
	std::cout << "Owned int: " << *ownedInt << "\n";
	
	myIntRef = ownedInt;
	
	std::cout << "Reference: " << *myIntRef << "\n";
}

int main() {
	test();
	
	if(myIntRef) { std::cout << "Can still access reference; value: " << *myIntRef << "\n"; }
	else { std::cout << "Cannot still access reference--it is null.\n"; }
}