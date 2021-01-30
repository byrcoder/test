//
// Created by weideng(邓伟) on 2020/12/4.
//

#include <iostream>
#include <chrono>
#include <thread>
#include <functional>

using namespace std;

namespace Functional {

    class Foo {
    public:
        int call(int i, int j) {
            cout << "call:" << i << "\t" << j << endl;
            return i+j;
        }
    };

    int call_g(int i, int j) {
        cout << "call_g:" << i << "\t" << j << endl;
        return i+j;
    }

    void print(int k) {
        cout << "print:" << k << endl;
    }

    void run() {
        cout << "Functional running\t" << endl;
        function< void(int) > ff(print);
        ff(1);

        const Foo f;
        using namespace std::placeholders;
        function<int(int, int)> ffc = std::bind(&Foo::call, f, 1, 1);
        cout << ffc(2, 2) << endl;

        ffc = std::bind(call_g,  _1, _2);
        cout << ffc(2, 2) << endl;
    }
}