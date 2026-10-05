#include "./TestFramework.h"
#include <cstring>
#include <exception>
#include <SDL2/SDL_ttf.h>

int main(int argc, char *argv[])
{
    if (TTF_Init() != 0)
    {
        std::cerr << "TTF_Init failed: " << TTF_GetError() << std::endl;
        return 1;
    }
    const char *filter = argc > 1 ? argv[1] : nullptr;
    int run = 0;
    int failedTests = 0;
    for (const auto &testCase : test::AllTests())
    {
        if (filter && !std::strstr(testCase.name, filter))
        {
            continue;
        }
        const int failuresBefore = test::FailureCount();
        try
        {
            testCase.run();
        }
        catch (const std::exception &e)
        {
            ++test::FailureCount();
            std::cerr << "  threw " << e.what() << std::endl;
        }
        ++run;
        if (test::FailureCount() != failuresBefore)
        {
            ++failedTests;
            std::cerr << "FAIL " << testCase.name << std::endl;
        }
        else
        {
            std::cout << "ok   " << testCase.name << std::endl;
        }
    }
    TTF_Quit();
    std::cout << std::endl
              << run - failedTests << "/" << run << " tests passed" << std::endl;
    return failedTests == 0 && run > 0 ? 0 : 1;
}
