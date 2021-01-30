//
// Created by weideng(邓伟) on 2020/12/24.
//

#include <vector>
#include <string>
using namespace std;

namespace Tools {

    extern "C" typedef struct C {
        int a;
        int b;
        char c[20];
    } C;

    std::vector<std::string> srs_string_splits(std::string str, std::string chars) {
        vector<string> arr;

        if (chars.empty()) {
            arr.push_back(str);
            return arr;
        }

        int pre = -1;  // 上一次匹配的chars的下标

        for (int i = 0; i < str.size(); ++i) {
            if (chars.find(str[i]) == std::string::npos) {
                continue;
            }

            if (i - pre <= 1) {
                // 前面是空的 skip
            } else {
                arr.push_back(str.substr(pre + 1, i - pre - 1));
            }

            pre = i;
        }

        printf("pre:%d, string:%s, size:%d\r\n", pre, str.c_str(), str.size());

        // 最后一组
        if (pre < (int) str.size() - 1) {
            arr.push_back(str.substr(pre + 1));
        }
        return arr;
    }

    struct C ca, cb, cc, cd;
    static struct C* const arr[] = {
            &ca,
            &cb,
            &cc,
            &cd
    };

    const C *av_muxer_iterate(void **opaque)
    {
        static const uintptr_t size = sizeof(arr)/sizeof(arr[0]) - 1;
        uintptr_t i = (uintptr_t)*opaque;
        const C *f = NULL;

        printf("i=%d, size=%d\r\n", i, size);
        if (i < size) {
            f = arr[i];
        }

        if (f)
            *opaque = (void*)(i + 1);
        return f;
    }

    void test_ptr() {
        void* p = NULL;

        while(av_muxer_iterate(&p)) {
            printf("p:%d\r\n", p);
        }

        p = NULL;

        printf("void* uintptr_t:%d\r\n", p);
        p = (void*) ((uintptr_t)p + 1);
        printf("void* uintptr_t:%d\r\n", p);

    }

    void test() {

#if 0
        auto k = srs_string_splits("xxx", " \t");

        for (auto p : k) {
            printf("%s\r\n", p.c_str());
        }

        auto kk = srs_string_splits("xxx ddd  cc\t aaa", " \t");

        for (auto p : kk) {
            printf("%s\r\n", p.c_str());
        }
#endif
        test_ptr();
    }


}