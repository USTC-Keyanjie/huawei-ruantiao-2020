// submit:
// 1. close #define TEST.
// 2. close CS should be better
// 3. open //#define MMAP
// 4. open //#define NEON

// #define TEST
// #define CS   // 使用计数排序 STL模式下暂不支持
// #define MMAP // 使用mmap函数
// #define NEON // 打开NEON特性的算子 STL模式下不能开

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

#define MAX_NUM_EDGES 2000000 // 最大可接受边数 确定
#define MAX_NUM_IDS 2000000   // 最大可接受id数 90%确定
#define MAX_OUT_DEGREE 500    // 最大可接受出度 不确定
#define MAX_IN_DEGREE 500     // 最大可接受入度 不确定

#define MAX_NUM_THREE_PREDS 100000 // 反向三级跳表的长度 不确定

#define MAX_OUTPUT_FILE_SIZE 1540000000 // 输出文件的最大大小 2000w*7*11B 确定

#define MAX_INT 2147483647 // 2^31 - 1

#define NUM_THREADS 18 // 线程数

// #define NUM_LEN3_RESULT 1200000  // 长度为3结果的总id数
// #define NUM_LEN4_RESULT 1600000  // 长度为4结果的总id数
// #define NUM_LEN5_RESULT 4000000  // 长度为5结果的总id数
// #define NUM_LEN6_RESULT 9000000  // 长度为6结果的总id数
// #define NUM_LEN7_RESULT 13000000 // 长度为7结果的总id数

// 均不确信
#define NUM_LEN3_RESULT 1500000  // 长度为3结果的总id数
#define NUM_LEN4_RESULT 2000000  // 长度为4结果的总id数
#define NUM_LEN5_RESULT 5000000  // 长度为5结果的总id数
#define NUM_LEN6_RESULT 12000000 // 长度为6结果的总id数
#define NUM_LEN7_RESULT 21000000 // 长度为7结果的总id数

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
    ui id;
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

// 输入信息，从u_ids到v_ids，全部保留
ui u_ids[MAX_NUM_EDGES];
ui v_ids[MAX_NUM_EDGES];
ui money[MAX_NUM_EDGES];

// 保留
ui ids[MAX_NUM_EDGES * 2];

// 图信息换vector
// Node g_succ[MAX_NUM_IDS][MAX_OUT_DEGREE]; // 邻接表
// ui out_degree[MAX_NUM_IDS];     // 每个节点的出度
// Node g_pred[MAX_NUM_IDS][MAX_IN_DEGREE];  // 逆邻接表
// ui in_degree[MAX_NUM_IDS];      // 每个节点的入度

vector<vector<Node>> g_succ(MAX_NUM_IDS); // 邻接表
vector<vector<Node>> g_pred(MAX_NUM_IDS); // 逆邻接表

// 保留
char idsComma[MAX_NUM_IDS * 11]; // id + ','
ui idsChar_len[MAX_NUM_IDS];     // 每个id的字符串长度

// 每个线程专属区域
struct Result
{
    // 保留
    ui path[4];                // 已经走过的节点信息
    ui m_path[4];              // 已经走过点的金额信息
    bool visited[MAX_NUM_IDS]; // 访问标记数组

    // 换 unordered_map
    // Three_pred three_uj[MAX_NUM_THREE_PREDS];    // 反向三级跳表
    // ui three_uj_len;                   // 三级跳表长度
    // ui reachable[MAX_NUM_IDS];         // 若此点可达起点，则记录其在三级跳表中的index + 1，否则记录0
    // ui currentUs[MAX_NUM_THREE_PREDS]; // 可以通过三次跳跃回起点的点
    // ui currentUs_len;                  // 当前可以通过三次跳跃回起点的点数量

    map<ui, map<ull, vector<ull>>> three_uj;

    // ui res3[NUM_LEN3_RESULT]; // 长度为3的结果
    // ui res4[NUM_LEN4_RESULT]; // 长度为4的结果
    // ui res5[NUM_LEN5_RESULT]; // 长度为5的结果
    // ui res6[NUM_LEN6_RESULT]; // 长度为6的结果
    // ui res7[NUM_LEN7_RESULT]; // 长度为7的结果
    ui res_count[5] = {0}; // 各个长度结果数量记录

    // STL
    vector<ui> res3;
    vector<ui> res4;
    vector<ui> res5;
    vector<ui> res6;
    vector<ui> res7;

} results[NUM_THREADS];

char str_res[MAX_OUTPUT_FILE_SIZE]; // 最终答案字符串

#ifdef NEON
// 16个字节以内的复制
void *memcpy_128(void *dest, void *src)
{
    unsigned long *s = (unsigned long *)src;
    unsigned long *d = (unsigned long *)dest;
    vst1q_u64(&d[0], vld1q_u64(&s[0]));
    return dest;
}

// 大长度字节的复制
void *memcpy_128(void *dest, void *src, size_t count)
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
    return dest;
}
#endif

#ifdef CS
// fstream方式输入
void input_fstream(char *testFile)
{
#ifdef TEST
    clock_t input_time = clock();
#endif

    FILE *file = fopen(testFile, "r");
    // ui money;
    while (fscanf(file, "%d,%d,%d\n", u_ids + edge_num, v_ids + edge_num, money + edge_num) != EOF)
    {
        id_count[u_ids[edge_num]] = 1;
        id_count[v_ids[edge_num]] = 1;
        ++edge_num;
    }
    fclose(file);
#ifdef TEST
    cout << "fscanf input time " << (double)(clock() - input_time) / CLOCKS_PER_SEC << "s" << endl;
#endif
}

#ifdef MMAP
// MMAP方式输入
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
    register int sign = 0;
    register ui temp = 0;

    for (char *p = buf; *p && p - buf < length; ++p)
    {
        if (*p != ',' && *p != '\n')
        {
            // 位运算符方式将字符串转int
            temp = (temp << 1) + (temp << 3) + *p - '0';
        }
        else
        {
            ++sign;
            switch (sign)
            {
            case 1: //读取 id1
                u_ids[edge_num] = temp;
                id_count[temp] = 1;
                temp = 0;
                break;
            case 2: //读取 id2
                v_ids[edge_num] = temp;
                id_count[temp] = 1;
                temp = 0;
                break;
            case 3: // 读取money
                money[edge_num++] = temp;
                temp = 0;
                sign = 0; // 开始读取下一行
                break;
            default:
                break;
            }
        }
    }

    close(fd);
    munmap(buf, length);
#ifdef TEST
    cout << "mmap input time " << (double)(clock() - input_time) / CLOCKS_PER_SEC << "s" << endl;
#endif
}
#endif

#else
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
    register int sign = 0;
    register ui temp = 0;

    for (char *p = buf; *p && p - buf < length; ++p)
    {
        if (*p != ',' && *p != '\n')
        {
            // atoi
            temp = (temp << 1) + (temp << 3) + *p - '0';
        }
        else
        {
            ++sign;
            switch (sign)
            {
            case 1: //读取 id1
                u_ids[edge_num] = temp;
                ids[id_pos++] = temp;
                temp = 0;
                break;
            case 2: //读取 id2
                v_ids[edge_num] = temp;
                ids[id_pos++] = temp;
                temp = 0;
                break;
            case 3: // 读取 money
                money[edge_num++] = temp;
                temp = 0;
                sign = 0; // 开始读取下一行
                break;
            default:
                break;
            }
        }
    }

    close(fd);
    munmap(buf, length);
#ifdef TEST
    cout << "mmap input time " << (double)(clock() - input_time) / CLOCKS_PER_SEC << "s" << endl;
#endif
}

#endif

#endif

// 计算总结果数长度，不超过2000w，那么最多8位数
ui res_count_digits10_length(ui num)
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

// 将结果数量转为字符串
ui res_count_uint2ascii(ui value, char *dst)
{
    static const char digits[] =
        "0001020304050607080910111213141516171819"
        "2021222324252627282930313233343536373839"
        "4041424344454647484950515253545556575859"
        "6061626364656667686970717273747576777879"
        "8081828384858687888990919293949596979899";

    const ui length = res_count_digits10_length(value);
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

// 先预留10位数，这总该够了吧
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
        all_res_count += results[tid].res_count[0] + results[tid].res_count[1] + results[tid].res_count[2] + results[tid].res_count[3] + results[tid].res_count[4];
    }

#ifdef TEST
    clock_t write_time = clock();
    printf("Total Loops %d\n", all_res_count);
#endif

    FILE *fp = fopen(resultFile, "w");

    register ui str_len = res_count_uint2ascii(all_res_count, str_res);
    str_res[str_len++] = '\n';

    register ui thread_offset, line_offset, line, result_id;

    // len 3
    for (tid = 0, thread_offset = 0; tid < NUM_THREADS; ++tid, thread_offset += NUM_LEN3_RESULT)
    {
        for (line = 0, line_offset = 0; line < results[tid].res_count[0]; ++line, line_offset += 3)
        {
            // result_id = results[0][thread_offset + line_offset];
            result_id = results[tid].res3[line_offset];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            // result_id = results[0][thread_offset + line_offset + 1];
            result_id = results[tid].res3[line_offset + 1];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            // result_id = results[0][thread_offset + line_offset + 2];
            result_id = results[tid].res3[line_offset + 2];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
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
        for (line = 0, line_offset = 0; line < results[tid].res_count[1]; ++line, line_offset += 4)
        {
            result_id = results[tid].res4[line_offset];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = results[tid].res4[line_offset + 1];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = results[tid].res4[line_offset + 2];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = results[tid].res4[line_offset + 3];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
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
        for (line = 0, line_offset = 0; line < results[tid].res_count[2]; ++line, line_offset += 5)
        {
            result_id = results[tid].res5[line_offset];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = results[tid].res5[line_offset + 1];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = results[tid].res5[line_offset + 2];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = results[tid].res5[line_offset + 3];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = results[tid].res5[line_offset + 4];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
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
        for (line = 0, line_offset = 0; line < results[tid].res_count[3]; ++line, line_offset += 6)
        {
            result_id = results[tid].res6[line_offset];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = results[tid].res6[line_offset + 1];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = results[tid].res6[line_offset + 2];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = results[tid].res6[line_offset + 3];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = results[tid].res6[line_offset + 4];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = results[tid].res6[line_offset + 5];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
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
        for (line = 0, line_offset = 0; line < results[tid].res_count[4]; ++line, line_offset += 7)
        {
            result_id = results[tid].res7[line_offset];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = results[tid].res7[line_offset + 1];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = results[tid].res7[line_offset + 2];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = results[tid].res7[line_offset + 3];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = results[tid].res7[line_offset + 4];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = results[tid].res7[line_offset + 5];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id, idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = results[tid].res7[line_offset + 6];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3) + (result_id << 1) + result_id);
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

bool is_money_valid(ui x, ui y)
{
    // if (x > y)
    // {
    //     // 会越界
    //     if (y & 0x40000000)
    //     {
    //         return true;
    //     }
    //     else
    //     {
    //         return x - y <= (y << 2);
    //     }
    // }
    // else
    // {
    //     return y - x <= (x << 1);
    // }

    return x > y ? (y & 0x40000000 ? true : x - y <= (y << 2)) : y - x <= (x << 1);
}

// iteration version
// 反向三级dfs的迭代版本
void pre_dfs_ite(register ui start_id, register ui tid)
{
    vector<Node>::iterator begin_pos[3];
    register ui cur_id = start_id, next_id;
    register int depth = 0;
    begin_pos[0] = g_pred[start_id].begin();

    // 寻找遍历起始位置
    while (start_id > begin_pos[depth]->id)
        ++begin_pos[depth];

    while (depth >= 0)
    {
        // 无前驱
        if (!(~(begin_pos[depth]->id | 0x80000000)))
        {
            results[tid].visited[cur_id] = false;
            cur_id = --depth > 0 ? results[tid].path[depth - 1] : start_id;
        }
        // 有前驱
        else
        {
            next_id = begin_pos[depth]->id;
            results[tid].m_path[depth] = (begin_pos[depth]++)->money;

            if (!results[tid].visited[next_id] && (!depth || is_money_valid(results[tid].m_path[depth], results[tid].m_path[depth - 1])))
            {
                results[tid].path[depth] = next_id;
                // 深度为2，装载反向三级跳表
                if (depth == 2)
                {
                    if (results[tid].path[2] != results[tid].path[0])
                    {
                        // results[tid].three_uj[results[tid].three_uj_len++] = {results[tid].path[2], results[tid].path[1], results[tid].path[0], results[tid].m_path[2], results[tid].m_path[0]};
                        results[tid].three_uj[results[tid].path[2]][((ull)results[tid].path[1] << 32) | (ull)results[tid].m_path[2]].push_back(((ull)results[tid].path[0] << 32) | (ull)results[tid].m_path[0]);
                    }
                }
                // 深度不足2，继续搜索
                else
                {
                    // 进入下一层dfs
                    cur_id = next_id;
                    results[tid].visited[cur_id] = true;
                    results[tid].path[++depth] = cur_id;
                    begin_pos[depth] = g_pred[next_id].begin();

                    // 寻找起始点
                    // 深度为2时，允许下一个节点等于起始点，这是为了兼容长度为3的环
                    if (depth < 2)
                    {
                        while (start_id >= begin_pos[depth]->id)
                            ++begin_pos[depth];
                    }
                    else
                    {
                        while (start_id > begin_pos[depth]->id)
                            ++begin_pos[depth];
                    }
                }
            }
        }
    }
}

// iteration version
// 正向dfs的迭代版本
void dfs_ite(register ui start_id, register ui tid)
{
    results[tid].visited[start_id] = true;
    results[tid].path[0] = start_id;

    vector<Node>::iterator begin_pos[4];
    begin_pos[0] = g_succ[start_id].begin();
    while (start_id >= begin_pos[0]->id)
        ++begin_pos[0];

    register ui cur_id = start_id, next_id;
    register int depth = 0;

    // length 3 result
    // 首先查找所有长度为3的结果，因为不需要搜索就可以得到
    if (results[tid].three_uj.find(cur_id) != results[tid].three_uj.end())
    {
        for (auto it1 = results[tid].three_uj[cur_id].begin(); it1 != results[tid].three_uj[cur_id].end(); ++it1)
        {
            for (auto it2 = it1->second.begin(); it2 != it1->second.end(); ++it2)
            {
                if (is_money_valid(*it2, it1->first))
                {
                    results[tid].res3.push_back(cur_id);
                    results[tid].res3.push_back(it1->first >> 32);
                    results[tid].res3.push_back(*it2 >> 32);
                    ++results[tid].res_count[0];
                }
            }
        }
    }

    // dfs搜索
    while (depth >= 0)
    {
        // 无后继
        if (!(~(begin_pos[depth]->id | 0x80000000)))
        {
            // 回溯
            results[tid].visited[cur_id] = false;
            if (--depth >= 0)
                cur_id = results[tid].path[depth];
        }
        // 有后继
        else
        {
            next_id = begin_pos[depth]->id;
            results[tid].m_path[depth] = (begin_pos[depth]++)->money;

            if (!results[tid].visited[next_id] && (!depth || is_money_valid(results[tid].m_path[depth - 1], results[tid].m_path[depth])))
            {
                // 找到环
                if (results[tid].three_uj.find(next_id) != results[tid].three_uj.end())
                {
                    switch (depth)
                    {
                    // 长度为4
                    case 0:
                        for (auto it1 = results[tid].three_uj[next_id].begin(); it1 != results[tid].three_uj[next_id].end(); ++it1)
                        {
                            for (auto it2 = it1->second.begin(); it2 != it1->second.end(); ++it2)
                            {
                                if (is_money_valid(results[tid].m_path[0], it1->first) && is_money_valid(*it2, results[tid].m_path[0]) &&
                                    !results[tid].visited[it1->first >> 32] && !results[tid].visited[*it2 >> 32])
                                {
                                    results[tid].res4.insert(results[tid].res4.end(), results[tid].path, results[tid].path + 1);
                                    results[tid].res4.push_back(next_id);
                                    results[tid].res4.push_back(it1->first >> 32);
                                    results[tid].res4.push_back(*it2 >> 32);
                                    ++results[tid].res_count[1];
                                }
                            }
                        }
                        break;
                    // 长度为5
                    case 1:
                        for (auto it1 = results[tid].three_uj[next_id].begin(); it1 != results[tid].three_uj[next_id].end(); ++it1)
                        {
                            for (auto it2 = it1->second.begin(); it2 != it1->second.end(); ++it2)
                            {
                                if (is_money_valid(results[tid].m_path[1], it1->first) && is_money_valid(*it2, results[tid].m_path[0]) &&
                                    !results[tid].visited[it1->first >> 32] && !results[tid].visited[*it2 >> 32])
                                {
                                    results[tid].res5.insert(results[tid].res5.end(), results[tid].path, results[tid].path + 2);
                                    results[tid].res5.push_back(next_id);
                                    results[tid].res5.push_back(it1->first >> 32);
                                    results[tid].res5.push_back(*it2 >> 32);
                                    ++results[tid].res_count[2];
                                }
                            }
                        }
                        break;
                    // 长度为6
                    case 2:
                        for (auto it1 = results[tid].three_uj[next_id].begin(); it1 != results[tid].three_uj[next_id].end(); ++it1)
                        {
                            for (auto it2 = it1->second.begin(); it2 != it1->second.end(); ++it2)
                            {
                                if (is_money_valid(results[tid].m_path[2], it1->first) && is_money_valid(*it2, results[tid].m_path[0]) &&
                                    !results[tid].visited[it1->first >> 32] && !results[tid].visited[*it2 >> 32])
                                {
                                    results[tid].res6.insert(results[tid].res6.end(), results[tid].path, results[tid].path + 3);
                                    results[tid].res6.push_back(next_id);
                                    results[tid].res6.push_back(it1->first >> 32);
                                    results[tid].res6.push_back(*it2 >> 32);
                                    ++results[tid].res_count[3];
                                }
                            }
                        }
                        break;
                    // 长度为7
                    case 3:
                        for (auto it1 = results[tid].three_uj[next_id].begin(); it1 != results[tid].three_uj[next_id].end(); ++it1)
                        {
                            for (auto it2 = it1->second.begin(); it2 != it1->second.end(); ++it2)
                            {
                                if (is_money_valid(results[tid].m_path[3], it1->first) && is_money_valid(*it2, results[tid].m_path[0]) &&
                                    !results[tid].visited[it1->first >> 32] && !results[tid].visited[*it2 >> 32])
                                {
                                    results[tid].res7.insert(results[tid].res7.end(), results[tid].path, results[tid].path + 4);
                                    results[tid].res7.push_back(next_id);
                                    results[tid].res7.push_back(it1->first >> 32);
                                    results[tid].res7.push_back(*it2 >> 32);
                                    ++results[tid].res_count[4];
                                }
                            }
                        }
                        break;
                    default:
                        break;
                    }
                }

                if (depth < 3)
                {
                    // 向更深一层dfs
                    cur_id = next_id;
                    results[tid].visited[cur_id] = true;
                    results[tid].path[++depth] = cur_id;
                    begin_pos[depth] = g_succ[next_id].begin();
                    // 寻找起始位置
                    while (start_id >= begin_pos[depth]->id)
                        ++begin_pos[depth];
                }
            }
        }
    }
}

// 反向三级跳表排序
bool cmp_three_uj(Three_pred &a, Three_pred &b)
{
    return a.u != b.u ? a.u < b.u : (a.k1 != b.k1 ? a.k1 < b.k1 : a.k2 < b.k2);
    // return a.k1 < b.k1;
}

// 这里定义子线程函数, 如果处理不同分段的数据不一样,那就写多个函数
// 函数的参数传入的时候需要是 void * 类型, 一般就是数据的指针转成  无类型指针
void *thread_process(void *t)
{

#ifdef TEST
    clock_t search_time = clock();
#endif

    // 先把指针类型恢复, 然后取值
    register ui tid = *((ui *)t);

    ui end_id = (ui)(seg_ratio[tid + 1] * id_num);
    for (ui start_id = (ui)(seg_ratio[tid] * id_num); start_id < end_id; ++start_id)
    {
#ifdef TEST
        if (start_id % 100 == 0)
        {
            printf("%d/%d ~ %.2lfs ~ %2.lf%%\n", start_id, id_num, (double)(clock() - search_time) / CLOCKS_PER_SEC, (double)(start_id) / id_num * 100);
        }
#endif
        results[tid].three_uj.clear();
        pre_dfs_ite(start_id, tid);
        // 有直达的点才继续搜下去
        if (results[tid].three_uj.size())
        {
            dfs_ite(start_id, tid);
        }
    }

#ifdef TEST
    cout << "tid " << tid << " DFS " << (double)(clock() - search_time) / CLOCKS_PER_SEC << "s" << endl;
#endif
    // pthread_exit(NULL) 为退出该线程
    pthread_exit(NULL);
}

bool cmp_node(Node &a, Node &b)
{
    return a.id < b.id;
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
    char testFile[] = "test_data_fs/639096/test_data.txt";
    char resultFile[] = "test_data_fs/639096/result.txt";
    clock_t start_time = clock();
#else
    char testFile[] = "/data/test_data.txt";
    char resultFile[] = "/projects/student/result.txt";
#endif

#ifdef MMAP
    input_mmap(testFile);
#else
    input_fstream(testFile);
#endif

    register ui index;

#ifdef CS
    for (index = 1; index < MAX_NUM_IDS; ++index)
    {
        id_count[index] += id_count[index - 1];
    }
    id_num = id_count[MAX_NUM_IDS - 1];

    // 获取hash_id，生成ids字符串，生成邻接表与逆邻接表
    register ui u_hash_id, v_hash_id;
    for (index = 0; index < edge_num - 3; index += 4)
    {
        u_hash_id = id_count[u_ids[index]] - 1;
        v_hash_id = id_count[v_ids[index]] - 1;
        ids[u_hash_id] = u_ids[index];
        ids[v_hash_id] = v_ids[index];
        // TODO: 可以使用neon_memcpy一起赋值
        g_succ[u_hash_id][out_degree[u_hash_id]].id = v_hash_id;
        g_succ[u_hash_id][out_degree[u_hash_id]++].money = money[index];
        g_pred[v_hash_id][in_degree[v_hash_id]].id = u_hash_id;
        g_pred[v_hash_id][in_degree[v_hash_id]++].money = money[index];

        u_hash_id = id_count[u_ids[index + 1]] - 1;
        v_hash_id = id_count[v_ids[index + 1]] - 1;
        ids[u_hash_id] = u_ids[index + 1];
        ids[v_hash_id] = v_ids[index + 1];
        g_succ[u_hash_id][out_degree[u_hash_id]].id = v_hash_id;
        g_succ[u_hash_id][out_degree[u_hash_id]++].money = money[index + 1];
        g_pred[v_hash_id][in_degree[v_hash_id]].id = u_hash_id;
        g_pred[v_hash_id][in_degree[v_hash_id]++].money = money[index + 1];

        u_hash_id = id_count[u_ids[index + 2]] - 1;
        v_hash_id = id_count[v_ids[index + 2]] - 1;
        ids[u_hash_id] = u_ids[index + 2];
        ids[v_hash_id] = v_ids[index + 2];
        g_succ[u_hash_id][out_degree[u_hash_id]].id = v_hash_id;
        g_succ[u_hash_id][out_degree[u_hash_id]++].money = money[index + 2];
        g_pred[v_hash_id][in_degree[v_hash_id]].id = u_hash_id;
        g_pred[v_hash_id][in_degree[v_hash_id]++].money = money[index + 2];

        u_hash_id = id_count[u_ids[index + 3]] - 1;
        v_hash_id = id_count[v_ids[index + 3]] - 1;
        ids[u_hash_id] = u_ids[index + 3];
        ids[v_hash_id] = v_ids[index + 3];
        g_succ[u_hash_id][out_degree[u_hash_id]].id = v_hash_id;
        g_succ[u_hash_id][out_degree[u_hash_id]++].money = money[index + 3];
        g_pred[v_hash_id][in_degree[v_hash_id]].id = u_hash_id;
        g_pred[v_hash_id][in_degree[v_hash_id]++].money = money[index + 3];
    }

    // 循环展开剩余部分
    for (; index < edge_num; ++index)
    {
        u_hash_id = id_count[u_ids[index]] - 1;
        v_hash_id = id_count[v_ids[index]] - 1;

        ids[u_hash_id] = u_ids[index];
        ids[v_hash_id] = v_ids[index];

        g_succ[u_hash_id][out_degree[u_hash_id]].id = v_hash_id;
        g_succ[u_hash_id][out_degree[u_hash_id]++].money = money[index];
        g_pred[v_hash_id][in_degree[v_hash_id]].id = u_hash_id;
        g_pred[v_hash_id][in_degree[v_hash_id]++].money = money[index];
    }

    for (index = 0; index < id_num - 3; index += 4)
    {
        // 邻接表和逆邻接表排序
        sort(g_succ[index], g_succ[index] + out_degree[index], cmp_node);
        sort(g_pred[index], g_pred[index] + in_degree[index], cmp_node);
        // 设置哨兵
        g_succ[index][out_degree[index]] = {MAX_INT, MAX_INT};
        g_pred[index][in_degree[index]] = {MAX_INT, MAX_INT};
        // 转字符串
        idsChar_len[index] = uint2ascii(ids[index], idsComma + (index << 3) + (index << 1) + index) + 1;

        sort(g_succ[index + 1], g_succ[index + 1] + out_degree[index + 1], cmp_node);
        sort(g_pred[index + 1], g_pred[index + 1] + in_degree[index + 1], cmp_node);
        g_succ[index + 1][out_degree[index + 1]] = {MAX_INT, MAX_INT};
        g_pred[index + 1][in_degree[index + 1]] = {MAX_INT, MAX_INT};
        idsChar_len[index + 1] = uint2ascii(ids[index + 1], idsComma + ((index + 1) << 3) + ((index + 1) << 1) + (index + 1)) + 1;

        sort(g_succ[index + 2], g_succ[index + 2] + out_degree[index + 2], cmp_node);
        sort(g_pred[index + 2], g_pred[index + 2] + in_degree[index + 2], cmp_node);
        g_succ[index + 2][out_degree[index + 2]] = {MAX_INT, MAX_INT};
        g_pred[index + 2][in_degree[index + 2]] = {MAX_INT, MAX_INT};
        idsChar_len[index + 2] = uint2ascii(ids[index + 2], idsComma + ((index + 2) << 3) + ((index + 2) << 1) + (index + 2)) + 1;

        sort(g_succ[index + 3], g_succ[index + 3] + out_degree[index + 3], cmp_node);
        sort(g_pred[index + 3], g_pred[index + 3] + in_degree[index + 3], cmp_node);
        g_succ[index + 3][out_degree[index + 3]] = {MAX_INT, MAX_INT};
        g_pred[index + 3][in_degree[index + 3]] = {MAX_INT, MAX_INT};
        idsChar_len[index + 3] = uint2ascii(ids[index + 3], idsComma + ((index + 3) << 3) + ((index + 3) << 1) + (index + 3)) + 1;
    }

    for (; index < id_num; ++index)
    {
        sort(g_succ[index], g_succ[index] + out_degree[index], cmp_node);
        sort(g_pred[index], g_pred[index] + in_degree[index], cmp_node);
        g_succ[index][out_degree[index]] = {MAX_INT, MAX_INT};
        g_pred[index][in_degree[index]] = {MAX_INT, MAX_INT};
        idsChar_len[index] = uint2ascii(ids[index], idsComma + (index << 3) + (index << 1) + index) + 1;
    }

#else
    sort(ids, ids + (edge_num << 1));
    duplicate_removal();

    register ui u_id, v_id;
    for (index = 0; index < edge_num - 3; index += 4)
    {
        u_id = binary_search(u_ids[index]);
        v_id = binary_search(v_ids[index]);
        // g_succ[u_id][out_degree[u_id]++] = {v_id, money[index]};
        // g_pred[v_id][in_degree[v_id]++] = {u_id, money[index]};
        g_succ[u_id].push_back({v_id, money[index]});
        g_pred[v_id].push_back({u_id, money[index]});

        u_id = binary_search(u_ids[index + 1]);
        v_id = binary_search(v_ids[index + 1]);
        // g_succ[u_id][out_degree[u_id]++] = {v_id, money[index + 1]};
        // g_pred[v_id][in_degree[v_id]++] = {u_id, money[index + 1]};
        g_succ[u_id].push_back({v_id, money[index + 1]});
        g_pred[v_id].push_back({u_id, money[index + 1]});

        u_id = binary_search(u_ids[index + 2]);
        v_id = binary_search(v_ids[index + 2]);
        // g_succ[u_id][out_degree[u_id]++] = {v_id, money[index + 2]};
        // g_pred[v_id][in_degree[v_id]++] = {u_id, money[index + 2]};
        g_succ[u_id].push_back({v_id, money[index + 2]});
        g_pred[v_id].push_back({u_id, money[index + 2]});

        u_id = binary_search(u_ids[index + 3]);
        v_id = binary_search(v_ids[index + 3]);
        // g_succ[u_id][out_degree[u_id]++] = {v_id, money[index + 3]};
        // g_pred[v_id][in_degree[v_id]++] = {u_id, money[index + 3]};
        g_succ[u_id].push_back({v_id, money[index + 3]});
        g_pred[v_id].push_back({u_id, money[index + 3]});
    }

    // 循环展开剩余部分
    for (; index < edge_num; ++index)
    {
        u_id = binary_search(u_ids[index]);
        v_id = binary_search(v_ids[index]);
        g_succ[u_id].push_back({v_id, money[index]});
        g_pred[v_id].push_back({u_id, money[index]});
    }

    for (index = 0; index < id_num - 3; index += 4)
    {
        sort(g_succ[index].begin(), g_succ[index].end(), cmp_node);
        sort(g_pred[index].begin(), g_pred[index].end(), cmp_node);
        g_succ[index].push_back({MAX_INT, MAX_INT});
        g_pred[index].push_back({MAX_INT, MAX_INT});
        idsChar_len[index] = uint2ascii(ids[index], idsComma + (index << 3) + (index << 1) + index) + 1;

        sort(g_succ[index + 1].begin(), g_succ[index + 1].end(), cmp_node);
        sort(g_pred[index + 1].begin(), g_pred[index + 1].end(), cmp_node);
        g_succ[index + 1].push_back({MAX_INT, MAX_INT});
        g_pred[index + 1].push_back({MAX_INT, MAX_INT});
        idsChar_len[index + 1] = uint2ascii(ids[index + 1], idsComma + ((index + 1) << 3) + ((index + 1) << 1) + (index + 1)) + 1;

        sort(g_succ[index + 2].begin(), g_succ[index + 2].end(), cmp_node);
        sort(g_pred[index + 2].begin(), g_pred[index + 2].end(), cmp_node);
        g_succ[index + 2].push_back({MAX_INT, MAX_INT});
        g_pred[index + 2].push_back({MAX_INT, MAX_INT});
        idsChar_len[index + 2] = uint2ascii(ids[index + 2], idsComma + ((index + 2) << 3) + ((index + 2) << 1) + (index + 2)) + 1;

        sort(g_succ[index + 3].begin(), g_succ[index + 3].end(), cmp_node);
        sort(g_pred[index + 3].begin(), g_pred[index + 3].end(), cmp_node);
        g_succ[index + 3].push_back({MAX_INT, MAX_INT});
        g_pred[index + 3].push_back({MAX_INT, MAX_INT});
        idsChar_len[index + 3] = uint2ascii(ids[index + 3], idsComma + ((index + 3) << 3) + ((index + 3) << 1) + (index + 3)) + 1;
    }

    for (; index < id_num; ++index)
    {
        sort(g_succ[index].begin(), g_succ[index].end(), cmp_node);
        sort(g_pred[index].begin(), g_pred[index].end(), cmp_node);
        g_succ[index].push_back({MAX_INT, MAX_INT});
        g_pred[index].push_back({MAX_INT, MAX_INT});
        idsChar_len[index] = uint2ascii(ids[index], idsComma + (index << 3) + (index << 1) + index) + 1;
    }
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
#ifdef TEST
        cout << "main() : creating thread, " << tid << endl;
#endif
        indexes[tid] = tid;
        pthread_create(&threads[tid], NULL, thread_process, (void *)&indexes[tid]);
    }

    // 删除属性，并等待其他线程
    pthread_attr_destroy(&attr);
    for (tid = 0; tid < NUM_THREADS; ++tid)
    {
        pthread_join(threads[tid], &status);
    }

    save_fwrite(resultFile);

#ifdef TEST
    cout << "Total time " << (double)(clock() - start_time) / CLOCKS_PER_SEC << "s" << endl;
#endif
    pthread_exit(NULL);
    return 0;
}
