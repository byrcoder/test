#include <iostream>
#include <chrono>
#include <thread>
#include <vector>


using namespace std;

namespace ThreadC11 {
    extern void run();
    extern void run20();
}

namespace Functional {
    extern void run();
}


namespace Tools {
    extern void test();
}

namespace Ffmpeg {
    extern void test(int argc, char *argv[]);
}

namespace Tmp {
    extern void test();
}

namespace Tcp {
    extern void test();
}

int main(int argc, char *argv[]) {

#if 0
    Tools::test();
    std::cout << "Hello, World!" << std::endl;
#endif

#if 0
    Tmp::test();
#endif

#if 0
    Tcp::test();
#endif

#if 1
    Ffmpeg::test(argc, argv);
#endif

#if 0
    ThreadC11::run();
    ThreadC11::run20();
    Functional::run();
#endif
    return 0;
}
