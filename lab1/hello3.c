#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>

int main() {
	void* handle;
	void (*hello_from_dyn_runtime_lib)(void);
	handle = dlopen("libhello_runtime.so", RTLD_LAZY);
	if (!handle) {
		fprintf(stderr, "%s\n", dlerror());
         	exit(EXIT_FAILURE);
    	}
    	dlerror();

	hello_from_dyn_runtime_lib = (void (*)(void)) dlsym(handle, "hello_from_dyn_runtime_lib");
	if (dlerror() != NULL) {
        	fprintf(stderr, "%s\n", dlerror());
        	exit(EXIT_FAILURE);
    	}	

	hello_from_dyn_runtime_lib();
	dlclose(handle);
	return 0;
}
