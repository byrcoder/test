//
// Created by weideng(邓伟) on 2020/12/4.
//

#include <iostream>
#include <chrono>
#include <thread>
#include <condition_variable>
//#include <coroutine>


#if CPPCORO
#include <cppcoro/generator.hpp>
#endif


using namespace std;

namespace ThreadC11 {

    void output(int i) {
        cout << "output" << endl;
        this_thread::sleep_for(2us);
        cout << i << ". output end" << endl;
    }

    void run() {
        cout << "Thread running\t" << endl;
        for (int i = 0; i < 4; i++)
        {
            auto t1 = new thread(output, i);
            t1->join();
            cout << "t1" << endl;

            auto la = [] (int i)  { output(i); };
            auto t2 = new thread(la, i);
            t2->join();
            cout << "t2" << endl;

            auto t3 = new thread([](int i) {
                output(i);
            }, i);
            t3->join();
            cout << "t3" << endl;
        }

        // getchar();
    }

    void run20() {

        condition_variable cond;
        std::mutex cv_m;

        auto t = new jthread([&]() {
            this_thread::sleep_for(200ms);
            cout << "before wait" << endl;
            unique_lock<mutex> g_lock(cv_m);
            cond.wait(g_lock);
            cout << "end wait" << endl;
        });

        auto t2 = new jthread([&] {
            cout << "before signal" << endl;
            this_thread::sleep_for(100ms);
            // std::lock_guard<std::mutex> lk(cv_m);
            // unique_lock<mutex> g_lock(cv_m);
            cond.notify_all();

            std::this_thread::sleep_for(200ms);
            {
                std::lock_guard<std::mutex> lk(cv_m);
            }

            cond.notify_all();
            cout << "after signal" << endl;
        });

        auto t3 = new jthread([] (std::stop_token st) {
           while(!st.stop_requested()) {
               this_thread::sleep_for(1s);
           }
        });

        t->join();
        t2->join();

        cout << "thread before stopped" << endl;
        std::this_thread::sleep_for(1s);
        t3->request_stop();
        t3->join();
        cout << "thread after stopped" << endl;

    }
#if CPPCORO
    cppcoro::task<> consume_items(int const n)
    {
        int i = 1;
        for(auto const& s : produce_items())
        {
            print_time();
            std::cout << "consumed " << s << '\n';
            if (++i > n) break;
        }

        co_return;
    }

    cppcoro::generator<std::string> produce_items()
    {
        while (true)
        {
            auto v = rand();
            using namespace std::string_literals;
            auto i = "item "s + std::to_string(v);
            print_time();
            std::cout << "produced " << i << '\n';
            co_yield i;
        }
    }

    void co_run() {
        cppcoro::sync_wait(consume_items(5));
    }
#endif
}