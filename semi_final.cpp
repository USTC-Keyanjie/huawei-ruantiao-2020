// submit:
// 1. close #define TEST.
// 2. open //#define MMAP
// 3. open //#define NEON

#define TEST
#define MMAP // 使用mmap函数
#define NEON // 打开NEON特性的算子

#include <bits/stdc++.h>
#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/hash_policy.hpp>
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

#define MAX_NUM_THREE_PREDS 1000000 // 反向三级跳表的长度 应该够用

#define MAX_INT 2147483648 // 2^31

// #define NUM_LEN3_RESULT 120000  // 长度为3结果的总id数
// #define NUM_LEN4_RESULT 160000  // 长度为4结果的总id数
// #define NUM_LEN5_RESULT 400000  // 长度为5结果的总id数
// #define NUM_LEN6_RESULT 900000  // 长度为6结果的总id数
// #define NUM_LEN7_RESULT 1300000 // 长度为7结果的总id数

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
// edge_num从1开始，因为g数组的第0位是哨兵
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
    ui dst_id;
    ui money;

    Node() {}

    Node(ui next_id, ui money) : dst_id(next_id), money(money) {}

    // 依据next_id进行排序
    bool operator<(const Node &nextNode) const
    {
        return dst_id < nextNode.dst_id;
    }
};

// 反向三级跳表元素结构体，可以通过 u -> k1 -> k2 -> 起点
// u -> k1的转账金额记录在first_money里
// k1 -> 起点的转账金额记录在last_money里
struct Three_pred
{
    ui k1;
    ui k2;
    ui first_money;
    ui last_money;

    Three_pred() : k1(), k2(), first_money(), last_money() {}
    Three_pred(
        ui k1,
        ui k2,
        ui first_money,
        ui last_money) : k1(k1), k2(k2), first_money(first_money), last_money(last_money) {}

    bool operator<(const Three_pred &nextNode) const
    {
        return k1 < nextNode.k1 || (k1 == nextNode.k1 && k2 < nextNode.k2);
    }
};

struct Temp_Three_pred
{
    ui k1;
    ui k2;
    ui first_money;
    ui last_money;
    ui next;

    Temp_Three_pred() : k1(), k2(), first_money(), last_money(), next() {}
    Temp_Three_pred(
        ui k1,
        ui k2,
        ui first_money,
        ui last_money,
        ui next) : k1(k1), k2(k2), first_money(first_money), last_money(last_money), next(next) {}

    bool operator<(const Temp_Three_pred &nextNode) const
    {
        return k1 < nextNode.k1 || (k1 == nextNode.k1 && k2 < nextNode.k2);
    }
};

ui input_data[MAX_NUM_EDGES][5];
ui ids[MAX_NUM_EDGES * 2];
ui succ_info[MAX_NUM_IDS][2]; // 对于邻接表每个节点，第一维表示起始index，第二维表示出度
ui pred_info[MAX_NUM_IDS][2]; // 对于逆邻接表每个节点，第一维表示起始index，第二维表示出度
Node g_pred[MAX_NUM_EDGES * 2];
Node g_succ[MAX_NUM_EDGES * 2];
__gnu_pbds::gp_hash_table<ui, ui> id_map; // map really id to regular id(0,1,2,...)

char idsComma[MAX_NUM_IDS * 16]; // id + ','
ui idsChar_len[MAX_NUM_IDS];     // 每个id的字符串长度

// 每个线程专属区域
struct ThreadMemory
{
    ui path[4];                // 已经走过的节点信息
    ui m_path[4];              // 已经走过点的金额信息
    bool visited[MAX_NUM_IDS]; // 访问标记数组

    Temp_Three_pred temp_three_uj[MAX_NUM_THREE_PREDS]; // 临时反向三级跳表
    Three_pred three_uj[MAX_NUM_THREE_PREDS];           // 最终反向三级跳表
    ui three_uj_len;                                    // 三级跳表长度
    ui begin_end_pos[MAX_NUM_IDS][2];                   // 若此点可达起点，则起点记录其在三级跳表中的index + 1，否则记录0，终点pos取不到，范围是[起点pos-1， 终点pos)
    ui currentUs[MAX_NUM_IDS];                          // 可以通过三次跳跃回起点的点
    ui currentUs_len;                                   // 当前可以通过三次跳跃回起点的点数量

    ui res3[NUM_LEN3_RESULT]; // 长度为3的结果
    ui res4[NUM_LEN4_RESULT]; // 长度为4的结果
    ui res5[NUM_LEN5_RESULT]; // 长度为5的结果
    ui res6[NUM_LEN6_RESULT]; // 长度为6的结果
    ui res7[NUM_LEN7_RESULT]; // 长度为7的结果
    ui res_count[5];          // 各个长度结果数量记录
} local_array[NUM_THREADS];

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
#ifdef _WIN32
#include <sysinfoapi.h>
#else
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

void input_fstream(char *testFile)
{
#ifdef TEST
    clock_t input_time = clock();
#endif

    FILE *file = fopen(testFile, "r");
    ui u, v, m;
    while (fscanf(file, "%d,%d,%d\n", &u, &v, &m) != EOF)
    {
        input_data[edge_num][0] = u;
        input_data[edge_num][1] = v;
        input_data[edge_num++][2] = m;
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

    register int sign = 0;
    register ui temp = 0;

    for (char *p = buf; *p && p - buf < length; ++p)
    {
        switch (*p)
        {
        case ',':
            switch (sign & 1)
            {
            case 0: //读取 id1
                input_data[edge_num][0] = temp;
                break;
            case 1: //读取 id2
                input_data[edge_num][1] = temp;
                break;
            default:
                break;
            }
            temp = 0;
            sign ^= 1;
            break;
        case '\n':
            input_data[edge_num][2] = temp;
            edge_num++;
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
        all_res_count += local_array[tid].res_count[0] + local_array[tid].res_count[1] + local_array[tid].res_count[2] + local_array[tid].res_count[3] + local_array[tid].res_count[4];
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
        for (line = 0, line_offset = 0; line < local_array[tid].res_count[0]; ++line, line_offset += 3)
        {
            // result_id = local_array[0][thread_offset + line_offset];
            result_id = local_array[tid].res3[line_offset];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 4));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 4), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            // result_id = local_array[0][thread_offset + line_offset + 1];
            result_id = local_array[tid].res3[line_offset + 1];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 4));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 4), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            // result_id = local_array[0][thread_offset + line_offset + 2];
            result_id = local_array[tid].res3[line_offset + 2];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 4));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 4), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];
            str_res[str_len - 1] = '\n';
        }
    }

    // len 4
    for (tid = 0, thread_offset = 0; tid < NUM_THREADS; ++tid, thread_offset += NUM_LEN4_RESULT)
    {
        for (line = 0, line_offset = 0; line < local_array[tid].res_count[1]; ++line, line_offset += 4)
        {
            result_id = local_array[tid].res4[line_offset];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 4));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 4), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = local_array[tid].res4[line_offset + 1];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 4));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 4), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = local_array[tid].res4[line_offset + 2];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 4));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 4), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = local_array[tid].res4[line_offset + 3];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 4));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 4), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];
            str_res[str_len - 1] = '\n';
        }
    }

    // len 5
    for (tid = 0, thread_offset = 0; tid < NUM_THREADS; ++tid, thread_offset += NUM_LEN5_RESULT)
    {
        for (line = 0, line_offset = 0; line < local_array[tid].res_count[2]; ++line, line_offset += 5)
        {
            result_id = local_array[tid].res5[line_offset];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 4));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 4), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = local_array[tid].res5[line_offset + 1];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 4));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 4), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = local_array[tid].res5[line_offset + 2];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 4));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 4), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = local_array[tid].res5[line_offset + 3];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 4));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 4), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = local_array[tid].res5[line_offset + 4];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 4));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 4), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];
            str_res[str_len - 1] = '\n';
        }
    }

    // len 6
    for (tid = 0, thread_offset = 0; tid < NUM_THREADS; ++tid, thread_offset += NUM_LEN6_RESULT)
    {
        for (line = 0, line_offset = 0; line < local_array[tid].res_count[3]; ++line, line_offset += 6)
        {
            result_id = local_array[tid].res6[line_offset];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 4));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 4), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = local_array[tid].res6[line_offset + 1];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 4));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 4), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = local_array[tid].res6[line_offset + 2];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 4));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 4), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = local_array[tid].res6[line_offset + 3];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 4));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 4), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = local_array[tid].res6[line_offset + 4];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 4));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 4), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = local_array[tid].res6[line_offset + 5];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 4));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 4), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];
            str_res[str_len - 1] = '\n';
        }
    }

    // len 7
    for (tid = 0, thread_offset = 0; tid < NUM_THREADS; ++tid, thread_offset += NUM_LEN7_RESULT)
    {
        for (line = 0, line_offset = 0; line < local_array[tid].res_count[4]; ++line, line_offset += 7)
        {
            result_id = local_array[tid].res7[line_offset];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 4));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 4), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = local_array[tid].res7[line_offset + 1];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 4));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 4), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = local_array[tid].res7[line_offset + 2];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 4));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 4), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = local_array[tid].res7[line_offset + 3];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 4));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 4), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = local_array[tid].res7[line_offset + 4];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 4));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 4), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = local_array[tid].res7[line_offset + 5];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 4));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 4), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = local_array[tid].res7[line_offset + 6];
#ifdef NEON
            memcpy_16(str_res + str_len, idsComma + (result_id << 4));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 4), idsChar_len[result_id]);
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
void pre_dfs_ite(register ui start_id, register ThreadMemory *local)
{
    register ui cur_id = start_id, next_id;
    register int depth = 0;
    // 递归栈
    register Node *stack[3];

    // 起点没有前驱
    if (!pred_info[start_id][1])
        return;

    stack[0] = g_pred + pred_info[start_id][0];

    // 寻找遍历起始位置
    while (start_id > stack[0]->dst_id)
        ++stack[0];

    while (depth >= 0)
    {
        next_id = stack[depth]->dst_id;
        // 无前驱
        if (next_id & 0x80000000)
        {
            // 回溯
            local->visited[cur_id] = false;
            cur_id = --depth > 0 ? local->path[depth - 1] : start_id;
        }
        // 有前驱
        else
        {
            local->m_path[depth] = (stack[depth]++)->money;

            if (!local->visited[next_id] && (!depth || is_money_valid(local->m_path[depth], local->m_path[depth - 1])))
            {
                local->path[depth] = next_id;
                // 深度为2，装载反向三级跳表
                if (depth == 2)
                {
                    if (next_id != local->path[0])
                    {
                        // ui &next_three_node = local->begin_end_pos[next_id][0];
                        local->temp_three_uj[local->three_uj_len] = Temp_Three_pred(
                            local->path[1],
                            local->path[0],
                            local->m_path[2],
                            local->m_path[0],
                            local->begin_end_pos[next_id][0]);

                        if (!local->begin_end_pos[next_id][0])
                            local->currentUs[local->currentUs_len++] = next_id;
                        local->begin_end_pos[next_id][0] = ++(local->three_uj_len);
                    }
                }
                // 深度不足2，且next_id还有前驱，则继续搜索
                else if (pred_info[next_id][1])
                {
                    // 进入下一层dfs
                    stack[++depth] = g_pred + pred_info[next_id][0];
                    cur_id = next_id;
                    local->visited[cur_id] = true;
                    local->path[depth] = cur_id;

                    // 寻找起始点
                    // 深度为2时，允许下一个节点等于起始点，这是为了兼容长度为3的环
                    if (depth < 2)
                    {
                        while (start_id >= stack[depth]->dst_id)
                            ++stack[depth];
                    }
                    else
                    {
                        while (start_id > stack[depth]->dst_id)
                            ++stack[depth];
                    }
                }
            }
        }
    }
}

void sort_three_uj(register ThreadMemory *local)
{
    ui three_uj_iterator = 0;
    ui three_uj_index = 0;
    Temp_Three_pred *temp;
    for (ui index = 0; index < local->currentUs_len; ++index)
    {
        ui U = local->currentUs[index];
        three_uj_iterator = local->begin_end_pos[U][0];
        // begin_pos加一存储
        local->begin_end_pos[U][0] = three_uj_index + 1;
        while (three_uj_iterator != 0)
        {
            temp = &local->temp_three_uj[three_uj_iterator - 1];
            local->three_uj[three_uj_index++] = Three_pred(temp->k1, temp->k2, temp->first_money, temp->last_money);
            three_uj_iterator = temp->next;
        };
        sort(local->three_uj + local->begin_end_pos[U][0] - 1, local->three_uj + three_uj_index);
        local->begin_end_pos[U][1] = three_uj_index;
    }
}

// iteration version
// 正向dfs的迭代版本
void dfs_ite(register ui start_id, register ThreadMemory *local)
{
    local->visited[start_id] = true;
    local->path[0] = start_id;

    register ui cur_id = start_id, next_id, index;
    register int depth = 0;
    // 递归栈
    register Node *stack[4];

    // 起点没有后继
    if (!succ_info[start_id])
        return;

    stack[0] = g_succ + succ_info[start_id][0];
    while (start_id > stack[0]->dst_id)
        ++stack[0];

    // length 3 result
    // 首先查找所有长度为3的结果，因为不需要搜索就可以得到
    if (local->begin_end_pos[cur_id][0])
    {
        for (ui index = local->begin_end_pos[cur_id][0] - 1; index < local->begin_end_pos[cur_id][1]; ++index)
        {
            if (is_money_valid(local->three_uj[index].last_money, local->three_uj[index].first_money))
            {
                *(local->res3 + (local->res_count[0] << 1) + local->res_count[0]) = cur_id;
#ifdef NEON
                memcpy_16(
                    local->res3 + (local->res_count[0] << 1) + local->res_count[0] + 1,
                    &local->three_uj[index].k1);
#else
                memcpy(
                    local->res3 + (local->res_count[0] << 1) + local->res_count[0] + 1,
                    &local->three_uj[index].k1,
                    8);
#endif
                ++local->res_count[0];
            }
        }
    }

    // dfs搜索
    while (depth >= 0)
    {
        next_id = stack[depth]->dst_id;

        // 无后继
        if (next_id & 0x80000000)
        {
            // 回溯
            local->visited[cur_id] = false;
            cur_id = --depth >= 0 ? local->path[depth] : start_id;
        }
        // 有后继
        else
        {
            local->m_path[depth] = (stack[depth]++)->money;

            if (!local->visited[next_id] && (!depth || is_money_valid(local->m_path[depth - 1], local->m_path[depth])))
            {
                // 找到环
                if (local->begin_end_pos[next_id][0])
                {
                    switch (depth)
                    {
                    // 长度为4
                    case 0:
                        for (index = local->begin_end_pos[next_id][0] - 1; index < local->begin_end_pos[next_id][1]; ++index)
                        {
                            if (is_money_valid(local->m_path[0], local->three_uj[index].first_money) &&
                                is_money_valid(local->three_uj[index].last_money, local->m_path[0]) &&
                                !local->visited[local->three_uj[index].k1] && !local->visited[local->three_uj[index].k2])
                            {
#ifdef NEON
                                memcpy_16(local->res4 + (local->res_count[1]++ << 2), local->path);
                                *(local->res4 + (local->res_count[1] << 2) - 3) = next_id;
                                memcpy_16(local->res4 + (local->res_count[1] << 2) - 2, &local->three_uj[index].k1);
#else
                                // 保存已经走过的点
                                memcpy(local->res4 + (local->res_count[1]++ << 2), local->path, 4);
                                *(local->res4 + (local->res_count[1] << 2) - 3) = next_id;
                                // 保存反向三级跳表
                                memcpy(local->res4 + (local->res_count[1] << 2) - 2, &local->three_uj[index].k1, 8);
#endif
                            }
                        }
                        break;
                    // 长度为5
                    case 1:
                        for (index = local->begin_end_pos[next_id][0] - 1; index < local->begin_end_pos[next_id][1]; ++index)
                        {
                            if (is_money_valid(local->m_path[1], local->three_uj[index].first_money) &&
                                is_money_valid(local->three_uj[index].last_money, local->m_path[0]) &&
                                !local->visited[local->three_uj[index].k1] && !local->visited[local->three_uj[index].k2])
                            {
#ifdef NEON
                                memcpy_16(local->res5 + (local->res_count[2] << 2) + local->res_count[2], local->path);
                                ++local->res_count[2];
                                *(local->res5 + (local->res_count[2] << 2) + local->res_count[2] - 3) = next_id;
                                memcpy_16(local->res5 + (local->res_count[2] << 2) + local->res_count[2] - 2, &local->three_uj[index].k1);

#else
                                memcpy(local->res5 + (local->res_count[2] << 2) + local->res_count[2], local->path, 8);
                                ++local->res_count[2];
                                *(local->res5 + (local->res_count[2] << 2) + local->res_count[2] - 3) = next_id;
                                memcpy(local->res5 + (local->res_count[2] << 2) + local->res_count[2] - 2, &local->three_uj[index].k1, 8);
#endif
                            }
                        }

                        break;
                    // 长度为6
                    case 2:
                        for (index = local->begin_end_pos[next_id][0] - 1; index < local->begin_end_pos[next_id][1]; ++index)
                        {
                            if (is_money_valid(local->m_path[2], local->three_uj[index].first_money) &&
                                is_money_valid(local->three_uj[index].last_money, local->m_path[0]) &&
                                !local->visited[local->three_uj[index].k1] && !local->visited[local->three_uj[index].k2])
                            {
#ifdef NEON
                                memcpy_16(local->res6 + (local->res_count[3] << 2) + (local->res_count[3] << 1), local->path);
                                ++local->res_count[3];
                                *(local->res6 + (local->res_count[3] << 2) + (local->res_count[3] << 1) - 3) = next_id;
                                memcpy_16(local->res6 + (local->res_count[3] << 2) + (local->res_count[3] << 1) - 2, &local->three_uj[index].k1);
#else
                                memcpy(local->res6 + (local->res_count[3] << 2) + (local->res_count[3] << 1), local->path, 12);
                                ++local->res_count[3];
                                *(local->res6 + (local->res_count[3] << 2) + (local->res_count[3] << 1) - 3) = next_id;
                                memcpy(local->res6 + (local->res_count[3] << 2) + (local->res_count[3] << 1) - 2, &local->three_uj[index].k1, 8);
#endif
                            }
                        }
                        break;
                    // 长度为7
                    case 3:
                        for (index = local->begin_end_pos[next_id][0] - 1; index < local->begin_end_pos[next_id][1]; ++index)
                        {
                            if (is_money_valid(local->m_path[3], local->three_uj[index].first_money) &&
                                is_money_valid(local->three_uj[index].last_money, local->m_path[0]) &&
                                !local->visited[local->three_uj[index].k1] && !local->visited[local->three_uj[index].k2])
                            {
#ifdef NEON
                                memcpy_16(local->res7 + (local->res_count[4] << 2) + (local->res_count[4] << 1) + local->res_count[4], local->path);
                                ++local->res_count[4];
                                *(local->res7 + (local->res_count[4] << 2) + (local->res_count[4] << 1) + local->res_count[4] - 3) = next_id;
                                memcpy_16(local->res7 + (local->res_count[4] << 2) + (local->res_count[4] << 1) + local->res_count[4] - 2, &local->three_uj[index].k1);
#else
                                memcpy(local->res7 + (local->res_count[4] << 2) + (local->res_count[4] << 1) + local->res_count[4], local->path, 16);
                                ++local->res_count[4];
                                *(local->res7 + (local->res_count[4] << 2) + (local->res_count[4] << 1) + local->res_count[4] - 3) = next_id;
                                memcpy(local->res7 + (local->res_count[4] << 2) + (local->res_count[4] << 1) + local->res_count[4] - 2, &local->three_uj[index].k1, 8);

#endif
                            }
                        }
                        break;
                    default:
                        break;
                    }
                }
                // 深度不足3，且next_id还有后继，则继续搜索
                if (depth < 3 && succ_info[next_id])
                {
                    // 向更深一层dfs
                    stack[++depth] = g_succ + succ_info[next_id][0];
                    cur_id = next_id;
                    local->visited[cur_id] = true;
                    local->path[depth] = cur_id;
                    // 寻找起始位置
                    while (start_id >= stack[depth]->dst_id)
                        ++stack[depth];
                }
            }
        }
    }
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
    register ThreadMemory *local = &local_array[tid];

    ui end_id = (ui)(seg_ratio[tid + 1] * id_num);
    for (ui start_id = (ui)(seg_ratio[tid] * id_num); start_id < end_id; ++start_id)
    {
#ifdef TEST
        // if (start_id % 100 == 0)
        //     printf("%d/%d ~ %.2lfs ~ %2.lf%%\n", start_id, id_num, (double)(clock() - search_time) / CLOCKS_PER_SEC, (double)(start_id) / id_num * 100);
#endif
        pre_dfs_ite(start_id, local);

        // 有直达的点才继续搜下去
        if (local->three_uj_len)
        {
            // 反向三级跳表排序
            sort_three_uj(local);
            dfs_ite(start_id, local);

            // reachable和currentUs还原
            for (ui U = 0; U < local->currentUs_len; ++U)
            {
                local->begin_end_pos[local->currentUs[U]][0] = 0;
                local->begin_end_pos[local->currentUs[U]][1] = 0;
            }
            local->three_uj_len = 0;
            local->currentUs_len = 0;
        }
    }

#ifdef TEST
    // TODO 这里要改
    // cout << "tid " << tid << " DFS " << (double)(clock() - search_time) / CLOCKS_PER_SEC << "s" << endl;
#endif
    // 退出该线程
    pthread_exit(NULL);
}

void prework_g_succ()
{
    ui u_hash_id;
    for (ui index = 0; index < edge_num; ++index)
    {
        // 改为regular id
        u_hash_id = input_data[index][0] = id_map[input_data[index][0]];
        // 链表串起来
        input_data[index][3] = succ_info[u_hash_id][0];
        succ_info[u_hash_id][0] = index + 1;
        // 出度
        succ_info[u_hash_id][1]++;
    }
}

void prework_g_pred()
{
    ui v_hash_id;
    for (ui index = 0; index < edge_num; ++index)
    {
        // 改为regular id
        v_hash_id = input_data[index][1] = id_map[input_data[index][1]];
        // 链表串起来
        input_data[index][4] = pred_info[v_hash_id][0];
        pred_info[v_hash_id][0] = index + 1;
        // 入度
        pred_info[v_hash_id][1]++;
    }
}

void construct_g_succ()
{
    ui succ_iterator = 0;
    ui succ_index = 0;
    for (ui cur_id = 0; cur_id < id_num; ++cur_id)
    {
        if (succ_info[cur_id][1] && pred_info[cur_id][1])
        {
            succ_iterator = succ_info[cur_id][0];
            succ_info[cur_id][0] = succ_index;
            while (succ_iterator != 0)
            {
                g_succ[succ_index++] = Node(input_data[succ_iterator - 1][1], input_data[succ_iterator - 1][2]);
                succ_iterator = input_data[succ_iterator - 1][3];
            }
            sort(g_succ + succ_info[cur_id][0], g_succ + succ_info[cur_id][0] + succ_info[cur_id][1]);
            g_succ[succ_index++] = Node(MAX_INT, MAX_INT);
        }
    }
}

void construct_g_pred()
{
    ui pred_iterator = 0;
    ui pred_index = 0;
    for (ui cur_id = 0; cur_id < id_num; ++cur_id)
    {
        if (succ_info[cur_id][1] && pred_info[cur_id][1])
        {
            pred_iterator = pred_info[cur_id][0];
            pred_info[cur_id][0] = pred_index;
            while (pred_iterator != 0)
            {
                g_pred[pred_index++] = Node(input_data[pred_iterator - 1][0], input_data[pred_iterator - 1][2]);
                pred_iterator = input_data[pred_iterator - 1][4];
            }
            sort(g_pred + pred_info[cur_id][0], g_pred + pred_info[cur_id][0] + pred_info[cur_id][1]);
            g_pred[pred_index++] = Node(MAX_INT, MAX_INT);
        }
    }
}

void pre_process()
{
    register ui index;

    // TODO: 可以用memcpy优化
    for (index = 0; index < edge_num; ++index)
    {
        ids[id_num++] = input_data[index][0];
        ids[id_num++] = input_data[index][1];
    }
    sort(ids, ids + id_num);
    id_num = unique(ids, ids + id_num) - ids;

    for (index = 0; index < id_num; ++index)
    {
        id_map[ids[index]] = index;
        idsChar_len[index] = uint2ascii(ids[index], idsComma + (index << 4)) + 1;
    }

    thread pre_thread_g_succ = thread(prework_g_succ);
    thread pre_thread_g_pred = thread(prework_g_pred);
    pre_thread_g_succ.join();
    pre_thread_g_pred.join();

    thread thread_g_succ = thread(construct_g_succ);
    thread thread_g_pred = thread(construct_g_pred);
    thread_g_succ.join();
    thread_g_pred.join();
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
    // 18875018
    // 19630345
    char testFile[] = "test_data_fs/18875018/test_data.txt";
    char resultFile[] = "test_data_fs/18875018/result.txt";
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
