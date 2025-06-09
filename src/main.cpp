#include "ColdWindEngine.h"


int main(int argc, char** argv)
{
	try {
		coldwind::ColdWindEngine app("Hello", 800, 600);
		app.run();
	}
	catch (const std::exception& e) {
		return EXIT_FAILURE;	
	}
	return 0;
}

