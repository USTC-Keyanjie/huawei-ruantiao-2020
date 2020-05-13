// submit:
// 1. close #define TEST.
// 2. open //#define MMAP
// 3. open //#define NEON

#define TEST
#define MMAP // 使用mmap函数
#define NEON // 打开NEON特性的算子

// #define _WIN32

#include <bits/stdc++.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef MMAP
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#endif

#ifdef NEON
#include <arm_neon.h>
#include <stddef.h>
#endif

#define NUM_THREADS 18 // 线程数

#define MAX_NUM_EDGES 2000005 // 最大可接受边数 200w+5 确定够用
#define MAX_NUM_IDS 2000005   // 最大可接受id数 200w+5 确定够用

#define MAX_OUTPUT_FILE_SIZE 1540000000 // 输出文件的最大大小 2000w*7*11B 确定够用

#define MAX_NUM_THREE_PREDS 10000000 // 反向三级跳表的长度 应该够用

#define MAX_INT 2147483648 // 2^31

// #define NUM_LEN3_RESULT 1200000  // 长度为3结果的总id数
// #define NUM_LEN4_RESULT 1600000  // 长度为4结果的总id数
// #define NUM_LEN5_RESULT 4000000  // 长度为5结果的总id数
// #define NUM_LEN6_RESULT 9000000  // 长度为6结果的总id数
// #define NUM_LEN7_RESULT 13000000 // 长度为7结果的总id数

// 应该够用
#define NUM_LEN3_RESULT 15000000 // 长度为3结果的总id数
#define NUM_LEN4_RESULT 15000000 // 长度为4结果的总id数
#define NUM_LEN5_RESULT 20000000 // 长度为5结果的总id数
#define NUM_LEN6_RESULT 25000000 // 长度为6结果的总id数
#define NUM_LEN7_RESULT 25000000 // 长度为7结果的总id数

using namespace std;

typedef unsigned long long ull;
typedef unsigned int ui;

// 总id数，总边数
ui id_num = 0, edge_num = 0;

#if NUM_THREADS == 1
// 单线程任务分配比例
float seg_ratio[] = {0, 1};

#elif NUM_THREADS == 4
// 4线程任务分配比例
float seg_ratio[] = {0, 0.068, 0.148, 0.284, 1};

#elif NUM_THREADS == 18
// 18线程任务分配比例
float seg_ratio[] = {
    0,
    0.0113, 0.0227, 0.034, 0.0453, 0.057, 0.068,
    0.084, 0.1, 0.116, 0.132, 0.148,
    0.182, 0.216, 0.25, 0.284,
    0.523, 0.761, 1};
#endif

struct Node
{
    ui u_id;
    ui v_id;
    ui money;
};

// 反向三级跳表元素结构体，可以通过 u -> k1 -> k2 -> 起点
// u -> k1的转账金额记录在first_money里
// k1 -> 起点的转账金额记录在last_money里
struct Three_pred
{
    ui u;
    ui k1;
    ui k2;
    ui first_money;
    ui last_money;

    Three_pred() : u(), k1(), k2(), first_money(), last_money() {}
    Three_pred(
        ui u,
        ui k1,
        ui k2,
        ui first_money,
        ui last_money) : u(u), k1(k1), k2(k2), first_money(first_money), last_money(last_money) {}
};

// 输入信息，从u_ids到v_ids
ui u_ids[MAX_NUM_EDGES];
ui v_ids[MAX_NUM_EDGES];
ui money[MAX_NUM_EDGES];

ui ids[MAX_NUM_EDGES * 2];

Node g_succ[MAX_NUM_EDGES];     // 邻接表
ui begin_pos_succ[MAX_NUM_IDS]; // 邻接表每个节点的起始index
Node g_pred[MAX_NUM_EDGES];     // 逆邻接表
ui begin_pos_pred[MAX_NUM_IDS]; // 逆邻接表每个节点的起始index

char idsComma[MAX_NUM_IDS * 11]; // id + ','
ui idsChar_len[MAX_NUM_IDS];     // 每个id的字符串长度

// 每个线程专属区域
struct ThreadMemory
{
    ui path[4];                // 已经走过的节点信息
    ui m_path[4];              // 已经走过点的金额信息
    bool visited[MAX_NUM_IDS]; // 访问标记数组

    Three_pred three_uj[MAX_NUM_THREE_PREDS]; // 反向三级跳表
    ui three_uj_len;                          // 三级跳表长度
    ui reachable[MAX_NUM_IDS];                // 若此点可达起点，则记录其在三级跳表中的index + 1，否则记录0
    ui currentUs[MAX_NUM_THREE_PREDS];        // 可以通过三次跳跃回起点的点
    ui currentUs_len;                         // 当前可以通过三次跳跃回起点的点数量

    ui res3[NUM_LEN3_RESULT]; // 长度为3的结果
    ui res4[NUM_LEN4_RESULT]; // 长度为4的结果
    ui res5[NUM_LEN5_RESULT]; // 长度为5的结果
    ui res6[NUM_LEN6_RESULT]; // 长度为6的结果
    ui res7[NUM_LEN7_RESULT]; // 长度为7的结果
    ui res_count[5];          // 各个长度结果数量记录
} thread_memory[NUM_THREADS];

char str_res[MAX_OUTPUT_FILE_SIZE]; // 最终答案字符串

#ifdef NEON
// 16个字节以内的复制
inline void memcpy_16(void *dest, void *src)
{
    vst1q_u64((unsigned long *)dest, vld1q_u64((unsigned long *)src));
}

// 大长度字节的复制
inline void memcpy_128(void *dest, void *src, size_t count)
{
    register ui i;
    unsigned long *s = (unsigned long *)src;
    unsigned long *d = (unsigned long *)dest;
    for (i = 0; i <= (count >> 6); ++i, d += 8, s += 8)
    {
        vst1q_u64(&d[0], vld1q_u64(&s[0]));
        vst1q_u64(&d[2], vld1q_u64(&s[2]));
        vst1q_u64(&d[4], vld1q_u64(&s[4]));
        vst1q_u64(&d[6], vld1q_u64(&s[6]));
    }
}
#endif

///Timer
#ifdef TEST
#ifdef _WIN32 // windows
#include <sysinfoapi.h>
#else // unix
#include <sys/time.h>
#endif

struct UniversalTimer
{
    vector<pair<string, int>> logPairs;

#ifdef _WIN32
    unsigned int startTime;
    unsigned int endTime;
#else
    struct timeval startTime;
    struct timeval endTime;
#endif

    void setTime()
    {
#ifdef _WIN32
        startTime = GetTickCount();
#else
        gettimeofday(&startTime, NULL);
#endif
    }

    int getElapsedTimeMS()
    {
#ifdef _WIN32
        endTime = GetTickCount();
        return int(endTime - startTime);
#else
        gettimeofday(&endTime, NULL);
        return int(1000 * (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec) / 1000);
#endif
    }

    void logTimeImpl(string tag, int elapsedTimeMS)
    {
        printf("Time consumed for %-20s is %d ms.\n", tag.c_str(), elapsedTimeMS);
    }

    void logTime(string tag)
    {
        logTimeImpl(tag, getElapsedTimeMS());
    }

    void resetTimeWithTag(string tag)
    {
        logPairs.emplace_back(make_pair(tag, getElapsedTimeMS()));
        setTime();
    }

    void printLogs()
    {
        for (auto x : logPairs)
            logTimeImpl(x.first, x.second);
    }
};
#endif

// 不使用计数排序方案
void input_fstream(char *testFile)
{
#ifdef TEST
    clock_t input_time = clock();
#endif

    FILE *file = fopen(testFile, "r");
    while (fscanf(file, "%d,%d,%d\n", u_ids + edge_num, v_ids + edge_num, money + edge_num) != EOF)
    {
        ids[(edge_num << 1)] = u_ids[edge_num];
        ids[(edge_num << 1) + 1] = v_ids[edge_num];
        ++edge_num;
    }
    fclose(file);
#ifdef TEST
    cout << "fscanf input time " << (double)(clock() - input_time) / CLOCKS_PER_SEC << "s" << endl;
#endif
}

#ifdef MMAP

void input_mmap(char *testFile)
{
#ifdef TEST
    clock_t input_time = clock();
#endif
    int fd = open(testFile, O_RDONLY);
    //get the size of the document
    long length = lseek(fd, 0, SEEK_END);

    //mmap
    char *buf = (char *)mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, 0);

    register int id_pos = 0;
    register ui temp = 0;

    for (char *p = buf; *p && p - buf < length; ++p)
    {
        switch (*p)
        {
        case ',':
            switch (id_pos & 1)
            {
            case 0: //读取 id1
                u_ids[edge_num] = temp;
                ids[id_pos++] = temp;
                temp = 0;
                break;
            case 1: //读取 id2
                v_ids[edge_num] = temp;
                ids[id_pos++] = temp;
                temp = 0;
                break;
            default:
                break;
            }
            break;
        case '\n':
            money[edge_num++] = temp;
            temp = 0;
            break;
        case '\r':
            break;
        default:
            // atoi
            temp = (temp << 1) + (temp << 3) + *p - '0';
            break;
        }
    }

    close(fd);
    munmap(buf, length);
#ifdef TEST
    cout << "mmap input time " << (double)(clock() - input_time) / CLOCKS_PER_SEC << "s" << endl;
#endif
}

#endif

// 预留10位数，这总该够了吧
ui digits10_length(ui num)
{
    if (num < 1e9)
    {
        if (num < 1e8)
        {
            if (num < 1e7)
            {
                if (num < 1e6)
                {
                    if (num < 1e5)
                    {
                        if (num < 1e4)
                        {
                            if (num < 1000)
                            {
                                if (num < 100)
                                {
                                    if (num < 10)
                                    {
                                        return 1;
                                    }
                                    return 2;
                                }
                                return 3;
                            }
                            return 4;
                        }
                        return 5;
                    }
                    return 6;
                }
                return 7;
            }
            return 8;
        }
        return 9;
    }
    return 10;
}

// ui转字符串
ui uint2ascii(ui value, char *dst)
{
    static const char digits[] =
        "0001020304050607080910111213141516171819"
        "2021222324252627282930313233343536373839"
        "4041424344454647484950515253545556575859"
        "6061626364656667686970717273747576777879"
        "8081828384858687888990919293949596979899";

    const ui length = digits10_length(value);
    dst[length] = ',';
    ui next = length - 1;

    while (value >= 100)
    {
        const ui u = (value % 100) << 1;
        value /= 100;
        dst[next - 1] = digits[u];
        dst[next] = digits[u + 1];
        next -= 2;
    }

    if (value < 10)
    {
        dst[next] = '0' + value;
    }
    else
    {
        ui u = value << 1;
        dst[next - 1] = digits[u];
        dst[next] = digits[u + 1];
    }
    return length;
}

// fwrite方式写
void save_fwrite(char *resultFile)
{
    register ui tid;
    ui all_res_count = 0;

    for (tid = 0; tid < NUM_THREADS; ++tid)
    {
        all_res_count += thread_memory[tid].res_count[0] + thread_memory[tid].res_count[1] + thread_memory[tid].res_count[2] + thread_memory[tid].res_count[3] + thread_memory[tid].res_count[4];
    }

#ifdef TEST
    clock_t write_time = clock();
    printf("Total Loops %d\n", all_res_count);
#endif

    FILE *fp = fopen(resultFile, "w");

    register ui str_len = uint2ascii(all_res_count, str_res);
    str_res[str_len++] = '\n';

    register ui thread_offset, line_offset, line, result_id;

    // len 3
    for (tid = 0, thread_offset = 0; tid < NUM_THREADS; ++tid, thread_offset += NUM_LEN3_RESULT)
    {
        for (line = 0, line_offset = 0; line < thread_memory[tid].res_count[0]; ++line, line_offset += 3)
        {
            // result_id = thread_memory[0][thread_offset + line_offset];
            result_id = thread_memory[tid].res3[line_offset];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            // result_id = thread_memory[0][thread_offset + line_offset + 1];
            result_id = thread_memory[tid].res3[line_offset + 1];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            // result_id = thread_memory[0][thread_offset + line_offset + 2];
            result_id = thread_memory[tid].res3[line_offset + 2];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];
            str_res[str_len - 1] = '\n';
        }
    }

    // len 4
    for (tid = 0, thread_offset = 0; tid < NUM_THREADS; ++tid, thread_offset += NUM_LEN4_RESULT)
    {
        for (line = 0, line_offset = 0; line < thread_memory[tid].res_count[1]; ++line, line_offset += 4)
        {
            result_id = thread_memory[tid].res4[line_offset];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = thread_memory[tid].res4[line_offset + 1];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = thread_memory[tid].res4[line_offset + 2];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = thread_memory[tid].res4[line_offset + 3];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];
            str_res[str_len - 1] = '\n';
        }
    }

    // len 5
    for (tid = 0, thread_offset = 0; tid < NUM_THREADS; ++tid, thread_offset += NUM_LEN5_RESULT)
    {
        for (line = 0, line_offset = 0; line < thread_memory[tid].res_count[2]; ++line, line_offset += 5)
        {
            result_id = thread_memory[tid].res5[line_offset];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = thread_memory[tid].res5[line_offset + 1];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = thread_memory[tid].res5[line_offset + 2];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = thread_memory[tid].res5[line_offset + 3];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = thread_memory[tid].res5[line_offset + 4];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];
            str_res[str_len - 1] = '\n';
        }
    }

    // len 6
    for (tid = 0, thread_offset = 0; tid < NUM_THREADS; ++tid, thread_offset += NUM_LEN6_RESULT)
    {
        for (line = 0, line_offset = 0; line < thread_memory[tid].res_count[3]; ++line, line_offset += 6)
        {
            result_id = thread_memory[tid].res6[line_offset];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = thread_memory[tid].res6[line_offset + 1];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = thread_memory[tid].res6[line_offset + 2];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = thread_memory[tid].res6[line_offset + 3];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = thread_memory[tid].res6[line_offset + 4];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = thread_memory[tid].res6[line_offset + 5];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];
            str_res[str_len - 1] = '\n';
        }
    }

    // len 7
    for (tid = 0, thread_offset = 0; tid < NUM_THREADS; ++tid, thread_offset += NUM_LEN7_RESULT)
    {
        for (line = 0, line_offset = 0; line < thread_memory[tid].res_count[4]; ++line, line_offset += 7)
        {
            result_id = thread_memory[tid].res7[line_offset];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = thread_memory[tid].res7[line_offset + 1];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = thread_memory[tid].res7[line_offset + 2];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = thread_memory[tid].res7[line_offset + 3];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = thread_memory[tid].res7[line_offset + 4];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = thread_memory[tid].res7[line_offset + 5];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = thread_memory[tid].res7[line_offset + 6];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];
            str_res[str_len - 1] = '\n';
        }
    }

    str_res[str_len] = '\0';
    fwrite(str_res, sizeof(char), str_len, fp);
    fclose(fp);

#ifdef TEST
    cout << "fwrite output time " << (double)(clock() - write_time) / CLOCKS_PER_SEC << "s" << endl;
#endif
}

inline bool is_money_valid(ui x, ui y)
{
    return x > y ? (y & 0x40000000 ? true : x - y <= (y << 2)) : y - x <= (x << 1);
}

// iteration version
// 反向三级dfs的迭代版本
void pre_dfs_ite(register ui start_id, register ThreadMemory *cur_memory)
{
    register ui cur_id = start_id, next_id;
    register int depth = 0;
    // 递归栈
    register Node *stack[3];

    // 起点没有前驱
    if (!begin_pos_pred[start_id])
        return;

    stack[0] = g_pred + begin_pos_pred[start_id] - 1;

    // 寻找遍历起始位置
    while (stack[0]->v_id == start_id && start_id > stack[0]->u_id)
        ++stack[0];

    while (depth >= 0)
    {
        // 无前驱
        if (stack[depth]->v_id != cur_id)
        {
            // 回溯
            cur_memory->visited[cur_id] = false;
            cur_id = --depth > 0 ? cur_memory->path[depth - 1] : start_id;
        }
        // 有前驱
        else
        {
            next_id = stack[depth]->u_id;
            cur_memory->m_path[depth] = (stack[depth]++)->money;

            if (!cur_memory->visited[next_id] && (!depth || is_money_valid(cur_memory->m_path[depth], cur_memory->m_path[depth - 1])))
            {
                cur_memory->path[depth] = next_id;
                // 深度为2，装载反向三级跳表
                if (depth == 2)
                {
                    if (cur_memory->path[2] != cur_memory->path[0])
                        cur_memory->three_uj[cur_memory->three_uj_len++] = {cur_memory->path[2], cur_memory->path[1], cur_memory->path[0], cur_memory->m_path[2], cur_memory->m_path[0]};
                }
                // 深度不足2，且next_id还有前驱，则继续搜索
                else if (begin_pos_pred[next_id])
                {
                    // 进入下一层dfs
                    stack[++depth] = g_pred + begin_pos_pred[next_id] - 1;
                    cur_id = next_id;
                    cur_memory->visited[cur_id] = true;
                    cur_memory->path[depth] = cur_id;

                    // 寻找起始点
                    // 深度为2时，允许下一个节点等于起始点，这是为了兼容长度为3的环
                    if (depth < 2)
                    {
                        while (stack[depth]->v_id == cur_id && start_id >= stack[depth]->u_id)
                            ++stack[depth];
                    }
                    else
                    {
                        while (stack[depth]->v_id == cur_id && start_id > stack[depth]->u_id)
                            ++stack[depth];
                    }
                }
            }
        }
    }
}

// iteration version
// 正向dfs的迭代版本
void dfs_ite(register ui start_id, register ThreadMemory *cur_memory)
{
    cur_memory->visited[start_id] = true;
    cur_memory->path[0] = start_id;

    register ui cur_id = start_id, next_id, index;
    register int depth = 0;
    // 递归栈
    register Node *stack[4];

    // 起点没有后继
    if (!begin_pos_pred[start_id])
        return;

    stack[0] = g_succ + begin_pos_succ[start_id] - 1;
    while (stack[0]->u_id == start_id && start_id > stack[0]->v_id)
        ++stack[0];

    // length 3 result
    // 首先查找所有长度为3的结果，因为不需要搜索就可以得到
    if (cur_memory->reachable[cur_id])
    {
        for (ui index = cur_memory->reachable[cur_id] - 1; cur_memory->three_uj[index].u == cur_id; ++index)
        {
            if (is_money_valid(cur_memory->three_uj[index].last_money, cur_memory->three_uj[index].first_money))
            {
#ifdef NEON
                memcpy_16(
                    cur_memory->res3 + (cur_memory->res_count[0] << 1) + cur_memory->res_count[0],
                    &cur_memory->three_uj[index].u);
#else
                memcpy(
                    cur_memory->res3 + (cur_memory->res_count[0] << 1) + cur_memory->res_count[0],
                    &cur_memory->three_uj[index].u,
                    12);
#endif
                ++cur_memory->res_count[0];
            }
        }
    }

    // dfs搜索
    while (depth >= 0)
    {
        // 无后继
        if (stack[depth]->u_id != cur_id)
        {
            // 回溯
            cur_memory->visited[cur_id] = false;
            cur_id = --depth >= 0 ? cur_memory->path[depth] : start_id;
        }
        // 有后继
        else
        {
            next_id = stack[depth]->v_id;
            cur_memory->m_path[depth] = (stack[depth]++)->money;

            if (!cur_memory->visited[next_id] && (!depth || is_money_valid(cur_memory->m_path[depth - 1], cur_memory->m_path[depth])))
            {
                // 找到环
                if (cur_memory->reachable[next_id])
                {
                    switch (depth)
                    {
                    // 长度为4
                    case 0:
                        for (index = cur_memory->reachable[next_id] - 1; cur_memory->three_uj[index].u == next_id; ++index)
                        {
                            if (is_money_valid(cur_memory->m_path[0], cur_memory->three_uj[index].first_money) &&
                                is_money_valid(cur_memory->three_uj[index].last_money, cur_memory->m_path[0]) &&
                                !cur_memory->visited[cur_memory->three_uj[index].k1] && !cur_memory->visited[cur_memory->three_uj[index].k2])
                            {
#ifdef NEON
                                memcpy_16(cur_memory->res4 + (cur_memory->res_count[1]++ << 2), cur_memory->path);
                                memcpy_16(cur_memory->res4 + (cur_memory->res_count[1] << 2) - 3, &cur_memory->three_uj[index].u);
#else
                                // 保存已经走过的点
                                memcpy(cur_memory->res4 + (cur_memory->res_count[1]++ << 2), cur_memory->path, 4);
                                // 保存反向三级跳表
                                memcpy(cur_memory->res4 + (cur_memory->res_count[1] << 2) - 3, &cur_memory->three_uj[index].u, 12);
#endif
                            }
                        }
                        break;
                    // 长度为5
                    case 1:
                        for (index = cur_memory->reachable[next_id] - 1; cur_memory->three_uj[index].u == next_id; ++index)
                        {
                            if (is_money_valid(cur_memory->m_path[1], cur_memory->three_uj[index].first_money) &&
                                is_money_valid(cur_memory->three_uj[index].last_money, cur_memory->m_path[0]) &&
                                !cur_memory->visited[cur_memory->three_uj[index].k1] && !cur_memory->visited[cur_memory->three_uj[index].k2])
                            {
#ifdef NEON
                                memcpy_16(cur_memory->res5 + (cur_memory->res_count[2] << 2) + cur_memory->res_count[2]++, cur_memory->path);
                                memcpy_16(cur_memory->res5 + (cur_memory->res_count[2] << 2) + cur_memory->res_count[2] - 3, &cur_memory->three_uj[index].u);
#else
                                memcpy(cur_memory->res5 + (cur_memory->res_count[2] << 2) + cur_memory->res_count[2]++, cur_memory->path, 8);
                                memcpy(cur_memory->res5 + (cur_memory->res_count[2] << 2) + cur_memory->res_count[2] - 3, &cur_memory->three_uj[index].u, 12);
#endif
                            }
                        }

                        break;
                    // 长度为6
                    case 2:
                        for (index = cur_memory->reachable[next_id] - 1; cur_memory->three_uj[index].u == next_id; ++index)
                        {
                            if (is_money_valid(cur_memory->m_path[2], cur_memory->three_uj[index].first_money) &&
                                is_money_valid(cur_memory->three_uj[index].last_money, cur_memory->m_path[0]) &&
                                !cur_memory->visited[cur_memory->three_uj[index].k1] && !cur_memory->visited[cur_memory->three_uj[index].k2])
                            {
#ifdef NEON
                                memcpy_16(cur_memory->res6 + (cur_memory->res_count[3] << 2) + (cur_memory->res_count[3]++ << 1), cur_memory->path);
                                memcpy_16(cur_memory->res6 + (cur_memory->res_count[3] << 2) + (cur_memory->res_count[3] << 1) - 3, &cur_memory->three_uj[index].u);
#else
                                memcpy(cur_memory->res6 + (cur_memory->res_count[3] << 2) + (cur_memory->res_count[3]++ << 1), cur_memory->path, 12);
                                memcpy(cur_memory->res6 + (cur_memory->res_count[3] << 2) + (cur_memory->res_count[3] << 1) - 3, &cur_memory->three_uj[index].u, 12);
#endif
                            }
                        }
                        break;
                    // 长度为7
                    case 3:
                        for (index = cur_memory->reachable[next_id] - 1; cur_memory->three_uj[index].u == next_id; ++index)
                        {
                            if (is_money_valid(cur_memory->m_path[3], cur_memory->three_uj[index].first_money) &&
                                is_money_valid(cur_memory->three_uj[index].last_money, cur_memory->m_path[0]) &&
                                !cur_memory->visited[cur_memory->three_uj[index].k1] && !cur_memory->visited[cur_memory->three_uj[index].k2])
                            {
#ifdef NEON
                                memcpy_16(cur_memory->res7 + (cur_memory->res_count[4] << 2) + (cur_memory->res_count[4] << 1) + cur_memory->res_count[4]++, cur_memory->path);
                                memcpy_16(cur_memory->res7 + (cur_memory->res_count[4] << 2) + (cur_memory->res_count[4] << 1) + cur_memory->res_count[4] - 3, &cur_memory->three_uj[index].u);
#else
                                memcpy(cur_memory->res7 + (cur_memory->res_count[4] << 2) + (cur_memory->res_count[4] << 1) + cur_memory->res_count[4]++, cur_memory->path, 16);
                                memcpy(cur_memory->res7 + (cur_memory->res_count[4] << 2) + (cur_memory->res_count[4] << 1) + cur_memory->res_count[4] - 3, &cur_memory->three_uj[index].u, 12);
#endif
                            }
                        }
                        break;
                    default:
                        break;
                    }
                }
                // 深度不足3，且next_id还有后继，则继续搜索
                if (depth < 3 && begin_pos_succ[next_id])
                {
                    // 向更深一层dfs
                    stack[++depth] = g_succ + begin_pos_succ[next_id] - 1;
                    cur_id = next_id;
                    cur_memory->visited[cur_id] = true;
                    cur_memory->path[depth] = cur_id;
                    // 寻找起始位置
                    while (stack[depth]->u_id == cur_id && start_id > stack[depth]->v_id)
                        ++stack[depth];
                }
            }
        }
    }
}

// 反向三级跳表排序
inline bool cmp_three_uj(Three_pred &a, Three_pred &b)
{
    return a.u != b.u ? a.u < b.u : (a.k1 != b.k1 ? a.k1 < b.k1 : a.k2 < b.k2);
    // return a.k1 < b.k1;
}

// 这里定义子线程函数, 如果处理不同分段的数据不一样,那就写多个函数
// 函数的参数传入的时候需要是 void * 类型, 一般就是数据的指针转成  无类型指针
void *thread_process(void *t)
{

#ifdef TEST
    UniversalTimer timer;
    timer.setTime();
#endif

    // 先把指针类型恢复, 然后取值
    register ui tid = *((ui *)t);
    register ThreadMemory *cur_memory = &thread_memory[tid];

    ui end_id = (ui)(seg_ratio[tid + 1] * id_num);
    for (ui start_id = (ui)(seg_ratio[tid] * id_num); start_id < end_id; ++start_id)
    {
#ifdef TEST
        // if (start_id % 100 == 0)
        //     printf("%d/%d ~ %.2lfs ~ %2.lf%%\n", start_id, id_num, (double)(clock() - search_time) / CLOCKS_PER_SEC, (double)(start_id) / id_num * 100);

#endif
        pre_dfs_ite(start_id, cur_memory);

        // 有直达的点才继续搜下去
        if (cur_memory->three_uj_len)
        {
            // 反向三级跳表排序
            sort(cur_memory->three_uj, cur_memory->three_uj + cur_memory->three_uj_len, cmp_three_uj);
            // stable_sort(cur_memory->three_uj, cur_memory->three_uj + cur_memory->three_uj_len, three_uj_cmp);

            // 设置哨兵
            cur_memory->three_uj[cur_memory->three_uj_len] = {MAX_INT, MAX_INT, MAX_INT, MAX_INT, MAX_INT};
            // 设置reachable和currentUs数组
            for (ui index = 0; index < cur_memory->three_uj_len; ++index)
            {
                if (!cur_memory->reachable[cur_memory->three_uj[index].u])
                {
                    cur_memory->reachable[cur_memory->three_uj[index].u] = index + 1;
                    cur_memory->currentUs[cur_memory->currentUs_len++] = cur_memory->three_uj[index].u;
                }
            }

            dfs_ite(start_id, cur_memory);

            // reachable和currentUs还原
            for (ui j = 0; j < cur_memory->currentUs_len; ++j)
            {
                cur_memory->reachable[cur_memory->currentUs[j]] = 0;
            }
            cur_memory->three_uj_len = 0;
            cur_memory->currentUs_len = 0;
        }
    }

#ifdef TEST
    // TODO 这里要改
    cout << "tid " << tid << " DFS " << (double)(clock() - search_time) / CLOCKS_PER_SEC << "s" << endl;
#endif
    // pthread_exit(NULL) 为退出该线程
    pthread_exit(NULL);
}

inline bool cmp_succ(Node &a, Node &b)
{
    return a.u_id < b.u_id || (a.u_id == b.u_id && a.v_id < b.v_id);
}

inline bool cmp_pred(Node &a, Node &b)
{
    return a.v_id < b.v_id || (a.v_id == b.v_id && a.u_id < b.u_id);
}

ui binary_search(ui target)
{
    register ui left = 0;
    register ui right = id_num - 1;
    register ui mid;

    while (left <= right)
    {
        mid = (right + left) >> 1;
        if (ids[mid] == target)
        {
            return mid;
        }
        else if (target > ids[mid])
        {
            left = mid + 1;
        }
        else if (target < ids[mid])
        {
            right = mid - 1;
        }
    }
    return -1;
}

void duplicate_removal()
{
    register ui slow = 0, fast = 1;
    register ui double_edge_num = edge_num << 1;
    while (fast < double_edge_num)
    {
        if (ids[fast] != ids[slow])
            ids[++slow] = ids[fast];
        fast++;
    }
    id_num = slow + 1;
}

void pre_process()
{
    register ui index;

    sort(ids, ids + (edge_num << 1));
    duplicate_removal();

    register ui u_id, v_id;
    for (index = 0; index < edge_num - 3; index += 4)
    {
        u_id = binary_search(u_ids[index]);
        v_id = binary_search(v_ids[index]);
        g_succ[index] = {u_id, v_id, money[index]};

        u_id = binary_search(u_ids[index + 1]);
        v_id = binary_search(v_ids[index + 1]);
        g_succ[index + 1] = {u_id, v_id, money[index + 1]};

        u_id = binary_search(u_ids[index + 2]);
        v_id = binary_search(v_ids[index + 2]);
        g_succ[index + 2] = {u_id, v_id, money[index + 2]};

        u_id = binary_search(u_ids[index + 3]);
        v_id = binary_search(v_ids[index + 3]);
        g_succ[index + 3] = {u_id, v_id, money[index + 3]};
    }

    // 循环展开剩余部分
    for (; index < edge_num; ++index)
    {
        u_id = binary_search(u_ids[index]);
        v_id = binary_search(v_ids[index]);
        g_succ[index] = {u_id, v_id, money[index]};
    }

// 复制长度为edge_num * 3(每个单元有3个数) * 4(每个数占4个字节)
#ifdef NEON
    memcpy_128(g_pred, g_succ, (edge_num << 3) + (edge_num << 2));
#else
    memcpy(g_pred, g_succ, (edge_num << 3) + (edge_num << 2));
#endif

    sort(g_succ, g_succ + edge_num, cmp_succ);
    for (index = 0; index < edge_num - 3; index += 4)
    {
        if (!begin_pos_succ[g_succ[index].u_id])
            begin_pos_succ[g_succ[index].u_id] = index + 1;

        if (!begin_pos_succ[g_succ[index + 1].u_id])
            begin_pos_succ[g_succ[index + 1].u_id] = index + 2;

        if (!begin_pos_succ[g_succ[index + 2].u_id])
            begin_pos_succ[g_succ[index + 2].u_id] = index + 3;

        if (!begin_pos_succ[g_succ[index + 3].u_id])
            begin_pos_succ[g_succ[index + 3].u_id] = index + 4;
    }

    for (; index < edge_num; ++index)
    {
        if (!begin_pos_succ[g_succ[index].u_id])
            begin_pos_succ[g_succ[index].u_id] = index + 1;
    }

    sort(g_pred, g_pred + edge_num, cmp_pred);
    for (index = 0; index < edge_num - 3; index += 4)
    {
        if (!begin_pos_pred[g_pred[index].v_id])
            begin_pos_pred[g_pred[index].v_id] = index + 1;

        if (!begin_pos_pred[g_pred[index + 1].v_id])
            begin_pos_pred[g_pred[index + 1].v_id] = index + 2;

        if (!begin_pos_pred[g_pred[index + 2].v_id])
            begin_pos_pred[g_pred[index + 2].v_id] = index + 3;

        if (!begin_pos_pred[g_pred[index + 3].v_id])
            begin_pos_pred[g_pred[index + 3].v_id] = index + 4;
    }
    for (; index < edge_num; ++index)
    {
        if (!begin_pos_pred[g_pred[index].v_id])
            begin_pos_pred[g_pred[index].v_id] = index + 1;
    }

    for (index = 0; index < id_num - 3; index += 4)
    {
        idsChar_len[index] = uint2ascii(ids[index], idsComma + (index << 3) + (index << 1) + index) + 1;
        idsChar_len[index + 1] = uint2ascii(ids[index + 1], idsComma + ((index + 1) << 3) + ((index + 1) << 1) + (index + 1)) + 1;
        idsChar_len[index + 2] = uint2ascii(ids[index + 2], idsComma + ((index + 2) << 3) + ((index + 2) << 1) + (index + 2)) + 1;
        idsChar_len[index + 3] = uint2ascii(ids[index + 3], idsComma + ((index + 3) << 3) + ((index + 3) << 1) + (index + 3)) + 1;
    }

    for (; index < id_num; ++index)
    {
        idsChar_len[index] = uint2ascii(ids[index], idsComma + (index << 3) + (index << 1) + index) + 1;
    }
}

int main()
{
#ifdef TEST
    // std
    // 41
    // 294
    // 34195
    // 51532
    // 639096
    // 697518
    // 881420
    // 2408026
    // 2541581
    // 19630345
    char testFile[] = "test_data_fs/19630345/test_data.txt";
    char resultFile[] = "test_data_fs/19630345/result.txt";
    clock_t start_time = clock();
#else
    char testFile[] = "/data/test_data.txt";
    char resultFile[] = "/projects/student/result.txt";
#endif

#ifdef TEST
    UniversalTimer timerA, timerB;
    timerA.setTime();
    timerB.setTime();
#endif

#ifdef MMAP
    input_mmap(testFile);
#else
    input_fstream(testFile);
#endif
#ifdef TEST
    timerB.resetTimeWithTag("Read Input File");
#endif

    pre_process();
#ifdef TEST
    timerB.resetTimeWithTag("Pre Process Data");
#endif

    register ui tid;
    // 创建子线程的标识符 就是线程 的id,放在数组中
    pthread_t threads[NUM_THREADS];
    // 线程的属性
    pthread_attr_t attr;
    void *status;

    // 用来存放i的值,传参
    // 可以改成你需要的数据结构
    int indexes[NUM_THREADS];

    // 初始化并设置线程为可连接的（joinable）
    // 这样操作后, 主main 需要等待子线程完成后才进行后一步
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    // 创建NUM_THREADS个数的线程
    for (tid = 0; tid < NUM_THREADS; tid++)
    {
        indexes[tid] = tid;
        pthread_create(&threads[tid], NULL, thread_process, (void *)&indexes[tid]);
    }

    // 删除属性，并等待其他线程
    pthread_attr_destroy(&attr);
    for (tid = 0; tid < NUM_THREADS; ++tid)
    {
        pthread_join(threads[tid], &status);
    }

#ifdef TEST
    timerB.resetTimeWithTag("Solving Results");
#endif
    save_fwrite(resultFile);

#ifdef TEST
    timerB.resetTimeWithTag("Output");
#endif

#ifdef TEST
    timerB.printLogs();
    timerA.logTime("Whole Process");
#endif
    pthread_exit(NULL);
    return 0;
}
