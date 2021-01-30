//
// Created by weideng(邓伟) on 2021/1/12.
//
#include <iostream>
#include <string>
#include <vector>
#include <fstream>

using namespace std;

std::vector<std::string> srs_string_split(std::string str, std::string flag)
{
    vector<string> arr;

    if (flag.empty()) {
        arr.push_back(str);
        return arr;
    }

    size_t pos;
    string s = str;

    while ((pos = s.find(flag)) != string::npos) {
        if (pos != 0) {
            arr.push_back(s.substr(0, pos));
        }
        s = s.substr(pos + flag.length());
    }

    if (!s.empty()) {
        arr.push_back(s);
    }

    return arr;
}

bool srs_string_starts_with(string str, string flag)
{
    return str.find(flag) == 0;
}

string srs_string_trim_end(string str, string trim_chars)
{
    std::string ret = str;

    for (int i = 0; i < (int)trim_chars.length(); i++) {
        char ch = trim_chars.at(i);

        while (!ret.empty() && ret.at(ret.length() - 1) == ch) {
            ret.erase(ret.end() - 1);

            // ok, matched, should reset the search
            i = 0;
        }
    }

    return ret;
}

string srs_string_trim_start(string str, string trim_chars)
{
    std::string ret = str;

    for (int i = 0; i < (int)trim_chars.length(); i++) {
        char ch = trim_chars.at(i);

        while (!ret.empty() && ret.at(0) == ch) {
            ret.erase(ret.begin());

            // ok, matched, should reset the search
            i = 0;
        }
    }

    return ret;
}

namespace Tmp {

#define ERROR_SUCCESS 0L

    enum SrsM3u8TagType {
        M3U8_TAG,     // # 开头
        M3U8_TS_TAG,  // #EXTINF: 9
        M3U8_TS_URL,  // after #EXTINF: 9 的ts url
        M3U8_TS_SEQ_URL, // after #EXT-X-MAP: ts seq
        M3U8_OTHER_VALUE,  // 非#开头
        M3U8_ERROR,
    };

    class SrsM3u8Line {
    public:
        SrsM3u8Line(SrsM3u8TagType type, std::string& value);

    public:
        bool is_m3u8_tag();
        bool is_ts_url();
        bool is_ts_seq();
        bool is_other_value();
        bool is_error();

    public:
        SrsM3u8TagType type;
        std::string value;
    };

    class SrsSimpleHlsParser {
    public:
        static int parse_m3u8(const std::string& m3u8, std::vector<SrsM3u8Line>& cols);
    };

    // 重写M3U8的内容，ts/子m3u8
    enum SrsAppM3u8RewriteContent {
        REWRITE_TS = (0x1) << 0,      // 重写TS
        // REWRITE_SUB_M3U8 = (Ox1) << 1, // 重写子m3u8,暂时不实现
    };

// 重写的规则，重写域名，重写参数
    enum SrsAppM3u8RewriteRule {
        REWRITE_HOST  = (0x1) << 0,  // 重写域名
        // REWRITE_PARAM = (0x1) << 2, // 重写参数, 暂时不实现
    };

// 重写对象
#define IS_REWRITE_TS(n)  (((n) & REWRITE_TS) > 0)

// 重写规则
#define IS_REWRITE_HOST(n) (((n) & REWRITE_HOST) > 0)

    struct SrsAppM3u8RewriteContext {
        int rewrite_content; // REWRITE_TS | REWRITE_SUB_M3U8, 重写对象
        int rewrite_rule;    // REWRITE_HOST， 重写规则

        std::string rewrite_host;   // 重写后的域名
        // std::string rewrite_params; // 重写参数
        std::string schema;         // schema http https
    };

    class ISrsAppM3u8Rewrite {
    public:
        virtual ~ISrsAppM3u8Rewrite() = default;
        virtual int rewrite(const std::string& m3u8_host,
                            const std::string& m3u8_path,
                            const std::string& m3u8,
                            std::string& res) = 0;

    public:
        static ISrsAppM3u8Rewrite* create_write(SrsAppM3u8RewriteContext* context);
    };

// 重写 uri
    class SrsAppM3u8UriRewrite : public ISrsAppM3u8Rewrite {
    public:
        SrsAppM3u8UriRewrite(const SrsAppM3u8RewriteContext& context);

    public:
        bool rewrite_ts();
        int  rewrite_uri(const std::string& url, const std::string& host,
                         const std::string& path, std::string& res);
        int  rewrite_seq(const std::string& seq_url, const std::string& host,
                         const std::string& path, std::string& res);

        int rewrite(const std::string& m3u8_host,
                    const std::string& m3u8_path,
                    const std::string& m3u8,
                    std::string& res) override;


    private:
        SrsAppM3u8RewriteContext context;
    };


    SrsM3u8Line::SrsM3u8Line(SrsM3u8TagType type, std::string& value) {
        this->type  = type;
        this->value = value;
    }

    bool SrsM3u8Line::is_m3u8_tag() {
        return type == M3U8_TAG;
    }

    bool SrsM3u8Line::is_ts_url() {
        return type == M3U8_TS_URL;
    }

    bool SrsM3u8Line::is_ts_seq() {
        return type == M3U8_TS_SEQ_URL;
    }

    bool SrsM3u8Line::is_other_value() {
        return type == M3U8_OTHER_VALUE;
    }

    bool SrsM3u8Line::is_error() {
        return type == M3U8_ERROR;
    }

    int SrsSimpleHlsParser::parse_m3u8(const std::string& m3u8, std::vector<SrsM3u8Line>& cols) {
        int ret = 0;
        auto lines = srs_string_split(m3u8, "\n");

        SrsM3u8TagType state = M3U8_ERROR;  // 这里主要标示前一个tag类型
        for (auto line : lines) {

            line = srs_string_trim_start(line, "\r\n\t ");
            line = srs_string_trim_end(line, "\r\n\t ");

            if (line.empty()) {
                // 空内容不改变state状态
                cols.push_back(SrsM3u8Line(M3U8_OTHER_VALUE, line));
                continue;
            }

            SrsM3u8TagType tag = M3U8_ERROR;
            if (line[0] == '#') {
                if (srs_string_starts_with(line, "#EXTINF:")) {
                    tag = M3U8_TS_TAG;
                }
                else if(srs_string_starts_with(line, "#EXT-X-MAP:")) {
                    tag = M3U8_TS_SEQ_URL;
                }
                else {
                    tag = M3U8_TAG;
                }
            }
            else {
                if (state == M3U8_TS_TAG) {
                    tag = M3U8_TS_URL;
                }
                else {
                    tag = M3U8_OTHER_VALUE;
                }
            }

            cols.push_back(SrsM3u8Line(tag, line));
            state = tag;
        }

        return ret;
    }

    static int parse_url(const std::string& url, const std::string& host, const std::string& path,
                         std::string& res_host, std::string& res_path)
    {
        if (url[0] == '/') {  // 绝对路径
            res_host = host;
            res_path = url;
        }
        else if (srs_string_starts_with(url, "http://")
                 || srs_string_starts_with(url, "https://")) { // 带有域名的例子

            size_t n = url.find("://"); // success
            string tmp = url.substr(n+3); // host

            n = tmp.find("/");
            if (n == std::string::npos) { // 只有域名
                res_host = tmp;
                res_path = "/";
            }
            else {
                res_host = tmp.substr(0, n);
                res_path = tmp.substr(n);
            }
        }
        else {

            res_host = host;
            auto n   = path.rfind("/");

            if (n == std::string::npos) { // 理论上不该存在
                res_path = "/" + url;
            }
            else {
                res_path = path.substr(0, n+1); // 包含 /结尾
                res_path += url;
            }
        }

        return 0;
    }

    /*
     * @url: 1. 相对路径 a/b.ts 2. 绝对路径 /a/b.ts 3.http:// https://tmp_host/a/b.ts
     * @host: 上下文域名
     * @path: 上下文path
     */
    int parse_url2(const std::string& url, const std::string& host, const std::string& path,
                         std::string& res_host, std::string& res_path)
    {
        if (url[0] == '/') {  // 绝对路径
            res_host = host;
            res_path = url;
        }
        else if (srs_string_starts_with(url, "http://")
                 || srs_string_starts_with(url, "https://")) { // 带有域名的例子

            size_t n = url.find("://"); // success
            string tmp = url.substr(n+3); // host

            n = tmp.find("/");
            if (n == std::string::npos) { // 只有域名
                res_host = tmp;
                res_path = "/";
            }
            else {
                res_host = tmp.substr(0, n);
                res_path = tmp.substr(n);
            }
        }
        else {

            res_host = host;
            auto n   = path.rfind("/");

            if (n == std::string::npos) { // 理论上不该存在
                res_path = "/" + url;
            }
            else {
                res_path = path.substr(0, n+1); // 包含 /结尾
                res_path += url;
            }
        }

        return ERROR_SUCCESS;
    }

    ISrsAppM3u8Rewrite * ISrsAppM3u8Rewrite::create_write(SrsAppM3u8RewriteContext *context) {
        switch (context->rewrite_content) {
            case REWRITE_HOST:
            default:
                return new SrsAppM3u8UriRewrite(*context);
        }
    }

    SrsAppM3u8UriRewrite::SrsAppM3u8UriRewrite(const SrsAppM3u8RewriteContext &context) {
        this->context = context;
    }

    bool SrsAppM3u8UriRewrite::rewrite_ts() {
        return IS_REWRITE_TS(context.rewrite_content & REWRITE_TS);
    }

    int SrsAppM3u8UriRewrite::rewrite_uri(const std::string& url, const std::string& host,
                                          const std::string& path, std::string& res) {
        int ret = ERROR_SUCCESS;

        if (url.empty()) {
            res = url;
            return ret;
        }

        std::string tmp_host;
        std::string tmp_path;

        ret = parse_url(url, host, path, tmp_host, tmp_path);

        if (ret != ERROR_SUCCESS) {
            return ret;
        }

        // 重写域名
        if (IS_REWRITE_HOST(context.rewrite_rule)) {
            res = context.schema + "://" + context.rewrite_host + tmp_path;
        }
        else {
            res = url;
        }

        return ret;

    }

    int SrsAppM3u8UriRewrite::rewrite_seq(const std::string& seq_url, const std::string& host,
                                          const std::string& path, std::string& res) {
        int ret = ERROR_SUCCESS;

        std::string tmp_url = seq_url;
        std::string tmp_res = "#EXT-X-MAP:";

        int npos ;
        if ((npos = tmp_url.find("#EXT-X-MAP:")) == std::string::npos) { // 容灾用
            res = seq_url;
            return ret;
        }

        tmp_url = tmp_url.substr(npos+1);

        std::vector<std::string> splits = srs_string_split(tmp_url, ",");

        for (int i = 0; i < splits.size(); ++i) {

            if ((npos = splits[i].find("URI=") )== std::string::npos) {
                if (i == 0) {
                    tmp_res += splits[i];
                }
                else {
                    tmp_res += ","  + splits[i];
                }
                continue;
            }

            std::string seq_url = splits[i].substr(npos+4);

            seq_url = srs_string_trim_start(seq_url, "\"' ");
            seq_url = srs_string_trim_end(seq_url, "\"' ");

            printf("get seq_url:%s\n", seq_url.c_str());

            std::string rewrite_seq_url;
            ret = rewrite_uri(seq_url, host, path, rewrite_seq_url);

            if (ret != ERROR_SUCCESS) {
                printf("rewrite seq_url:%s, ret:%d failed\n", seq_url.c_str(), ret);
                return ret;
            }

            printf("get seq_url:%s, rewrite_url:%s\n", seq_url.c_str(), rewrite_seq_url.c_str());

            if (i == 0) {
                tmp_res += "URI=\"" + rewrite_seq_url + "\"";
            }
            else {
                tmp_res += ",URI=\"" + rewrite_seq_url + "\"";
            }
        }

        res = tmp_res;

        return ret;

    }

    int SrsAppM3u8UriRewrite::rewrite(const std::string &m3u8_host, const std::string &m3u8_path,
                                      const std::string &m3u8, std::string &res) {
        int ret = ERROR_SUCCESS;
        std::vector<SrsM3u8Line> lines;

        ret = SrsSimpleHlsParser::parse_m3u8(m3u8, lines);

        if (ret != ERROR_SUCCESS) {
            int len = m3u8.size() > 100 ? 100 : m3u8.size();
            printf("parse m3u8 failed ret:%d, %s\n", ret, m3u8.substr(0, len).c_str());
            return ret;
        }

        for (auto line : lines) {
            std::string tag = line.value;

            if (line.is_ts_url() && rewrite_ts()) {
                ret = rewrite_uri(line.value, m3u8_host, m3u8_path, tag);

                if (ret != ERROR_SUCCESS) {
                    printf("rewrite ts url failed url:%s, path:%s, host:%s\n",
                              line.value.c_str(), m3u8_path.c_str(), m3u8_host.c_str());
                    return ret;
                }
#if 0
                srs_trace("rewrite url:%s -> %s.", line.value.c_str(), tag.c_str());
#endif
            }
            else if(line.is_ts_seq() && rewrite_ts()) {
                ret = rewrite_seq(line.value, m3u8_host, m3u8_path, tag);
                if (ret != ERROR_SUCCESS) {
                    printf("rewrite ts seq url failed url:%s, path:%s, host:%s\n",
                              line.value.c_str(), m3u8_path.c_str(), m3u8_host.c_str());
                    return ret;
                }
            }

            res.append(tag).append("\n");
        }

#if 1
        printf("raw_m3u8:%s.\r\n rewrite_m3u8:%s\n", m3u8.c_str(), res.c_str());
#endif

        return ret;
    }

    void test_time();

    void test() {

        std::vector<SrsM3u8Line> res_m3u8;

        ifstream in;

        in.open("1.m3u8", ios::in );

        ofstream out;
        out.open("2.m3u8", ios::out);

        std::string m3u8((std::istreambuf_iterator<char>(in)),
                        std::istreambuf_iterator<char>());

        cout << m3u8 << ".raw_m3u8" << endl;

        cout << endl;
        SrsSimpleHlsParser::parse_m3u8(m3u8, res_m3u8);

        SrsAppM3u8RewriteContext context;
        context.rewrite_host = "www.baidu.com";
        context.rewrite_rule = REWRITE_HOST;
        context.rewrite_content = REWRITE_TS;
        context.schema = "http";

        SrsAppM3u8UriRewrite rewrite(context);

        std::string re_m3u8;
        rewrite.rewrite("www.aliyun.com", "/12345.m3u8", m3u8, re_m3u8);

        cout << "rewrite begin" << endl;

        cout << re_m3u8 << endl;

        cout << "rewrite end" << endl;


        int i = 0;
        for (auto v : res_m3u8) {
            cout << i++ << ". " << v.type << " ->" << v.value << endl;
            out << v.value << endl;
        }

        for(auto v : res_m3u8) {
            if (v.is_ts_url()) {
                std::string res_host, res_path;
                parse_url(v.value, "www.origin.com", "/1/2.m3u8", res_host, res_path);
                cout << v.value << "->" << res_host << "|"<< res_path << endl;
                parse_url(v.value, "www.origin.com", "/1/2.m3u8?x=y", res_host, res_path);
            }
        }

        cout << "m3u8 over" << endl;

        test_time();
    }

    struct SrsTimeSpentAttr
    {
        int ms_50;
        int ms_100;
        int ms_200;
        int ms_500;
        int ms_800;
        int ms_1000;
        int ms_2000;
        int ms_3000;
        int ms_5000;
        int ms_10000;
        int ms_large;
    };

    class SrsTimeSpentMonitor
    {
    public:
        SrsTimeSpentMonitor(const SrsTimeSpentAttr& attr);
        ~SrsTimeSpentMonitor();

    public:
        virtual int64_t get_spend_time();
        bool attr_api(int64_t spend, int level, int attrid);
        void set_report(bool report);

        void do_report();

    protected:
        const SrsTimeSpentAttr& attr;
        int64_t start_time;
        bool report;
    };

    class SrsUserTimeSpendMonitor : public SrsTimeSpentMonitor
    {
    public:
        SrsUserTimeSpendMonitor(const SrsTimeSpentAttr& attr, int64_t spend_time);
        ~SrsUserTimeSpendMonitor();

    public:
        int64_t get_spend_time() override;
        void set_spend_time(int64_t spend_time);

    private:
        int64_t spend_time;
    };


    SrsTimeSpentMonitor::SrsTimeSpentMonitor(const SrsTimeSpentAttr &attr) : attr(attr)
    {
        start_time = 0;
        report     = true;
    }

    SrsTimeSpentMonitor::~SrsTimeSpentMonitor()
    {
        do_report();
    }

    int64_t SrsTimeSpentMonitor::get_spend_time()
    {
        return 0;
    }

    bool SrsTimeSpentMonitor::attr_api(int64_t spend, int level, int attrid)
    {
        if(spend <= level && attrid > 0) {
            printf("spend:%lld, level:%d, attrid\n", spend, level, attrid);
            return true;
        }
        return false;
    }

    void SrsTimeSpentMonitor::set_report(bool report)
    {
        this->report = report;
    }

    void SrsTimeSpentMonitor::do_report()
    {
        if(!report) {
            return;
        }
        int64_t spend_time = get_spend_time();

        if(attr_api(spend_time, 50, attr.ms_50)) {
            return;
        }

        if(attr_api(spend_time, 100, attr.ms_100)) {
            return;
        }

        if(attr_api(spend_time, 200, attr.ms_200)) {
            return;
        }

        if(attr_api(spend_time, 500, attr.ms_500)) {
            return;
        }

        if(attr_api(spend_time, 800, attr.ms_800)) {
            return;
        }

        if(attr_api(spend_time, 1000, attr.ms_1000)) {
            return;
        }

        if(attr_api(spend_time, 2000, attr.ms_2000)) {
            return;
        }

        if(attr_api(spend_time, 3000, attr.ms_3000)) {
            return;
        }

        if(attr_api(spend_time, 5000, attr.ms_5000)) {
            return;
        }

        if(attr_api(spend_time, 10000, attr.ms_10000)) {
            return;
        }

        if(attr.ms_large > 0) {
            printf("spend:%lld, level:%d, attrid\n", spend_time, -1, attr.ms_large);
        }
    }

    SrsUserTimeSpendMonitor::SrsUserTimeSpendMonitor(const SrsTimeSpentAttr &attr, int64_t spend_time) :
            SrsTimeSpentMonitor(attr)
    {
        this->spend_time = spend_time;
    }

    SrsUserTimeSpendMonitor::~SrsUserTimeSpendMonitor()
    {
        do_report();
    }

    int64_t SrsUserTimeSpendMonitor::get_spend_time()
    {
        return spend_time;
    }

    void SrsUserTimeSpendMonitor::set_spend_time(int64_t spend_time)
    {
        this->spend_time = spend_time;
    }

    void test_time() {

        static SrsTimeSpentAttr attr =
        {
                .ms_50       = 35440383,
                .ms_100      = 35440384,
                .ms_200      = 35440385,
                .ms_500      = 35440386,
                .ms_800      = 35440387,
                .ms_1000     = 35440388,
                .ms_2000     = 35440389,
                .ms_3000     = 35440390,
                .ms_5000     = 35440391,
                .ms_10000    = 35440392,
                .ms_large    = 35440393,
        };

        {
            SrsUserTimeSpendMonitor monitor(attr, 10);
        }

        {
            SrsUserTimeSpendMonitor monitor(attr, 100);
        }

        {
            SrsUserTimeSpendMonitor monitor(attr, 200);
        }

        {
            SrsUserTimeSpendMonitor monitor(attr, 300);
        }

        {
            SrsUserTimeSpendMonitor monitor(attr, 500);
        }

        {
            SrsUserTimeSpendMonitor monitor(attr, 800);
        }

        {
            SrsUserTimeSpendMonitor monitor(attr, 2500);
        }

        {
            SrsUserTimeSpendMonitor monitor(attr, 3500);
        }

        {
            SrsUserTimeSpendMonitor monitor(attr, 5500);
        }

        {
            SrsUserTimeSpendMonitor monitor(attr, 10500);
        }
    }

}