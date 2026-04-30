// VulkanTest.cpp : Defines the entry point for the application.
//



#include "VulkanTest.h"




#include "spdlog/spdlog.h"
#include <fstream>
#include "st_app.h"








int main(int argc, const char* argv[])
{

    if (argc < 2)
    {
        spdlog::info("Usage: {} <filename>",argv[0]);
        return EXIT_FAILURE;
    }
    fs::path fileName(argv[1]);
    st::StApp app{fileName};
    try {
        app.run();
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
