// VulkanTest.cpp : Defines the entry point for the application.
//



#include "VulkanTest.h"




#include "spdlog/spdlog.h"
#include <fstream>
#include "st_app.h"








int main()
{
    st::StApp app{};

    try {
        app.run();
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
