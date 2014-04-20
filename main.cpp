#include <cstdlib>

#include <iostream>
#include <exception>


int main()
{
	try
	{
	}
	catch (std::exception const& err)
	{
		std::cerr << u8"Exception: " << err.what() << std::endl;
		return EXIT_FAILURE;
	}
	catch (...)
	{
		std::cerr << u8"Unknown exception." << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
