// submit:
// 1. delete #define TEST.
// 2. open //#define MMAP
// 3. open //#define NEON

// #define TEST
#define MMAP // 使用mmap函数
#define NEON // 打开NEON特性的算子

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

#define MAX_NUM_EDGES 280000
#define MAX_NUM_IDS 200005
#define MAX_OUT_DEGREE 51
#define MAX_IN_DEGREE 51

#define MAX_NUM_THREE_PREDS 10000

#define MAX_OUTPUT_FILE_SIZE 134217728 //submit: 134217728 test: 520000000

#define MAX_INT 2147483647

#define NUM_THREADS 18

using namespace std;

unsigned int id_num = 0, edge_num = 0;

// float seg_ratio[] = {0, 1};
// float seg_ratio[] = {0, 0.068, 0.148, 0.284, 1};
unsigned int seg_id[32];

struct Three_pred
{
    unsigned int u;
    unsigned int k1;
    unsigned int k2;

    Three_pred() : u(), k1(), k2() {}
    Three_pred(unsigned int u, unsigned int k1, unsigned int k2) : u(u), k1(k1), k2(k2) {}
};

unsigned int u_ids[MAX_NUM_EDGES];
unsigned int v_ids[MAX_NUM_EDGES];

unsigned int id_count[MAX_NUM_IDS];
unsigned int ids[MAX_NUM_IDS];

unsigned int g_succ[MAX_NUM_IDS][MAX_OUT_DEGREE];
unsigned int out_degree[MAX_NUM_IDS];
unsigned int g_pred[MAX_NUM_IDS][MAX_IN_DEGREE];
unsigned int in_degree[MAX_NUM_IDS];

char idsComma[MAX_NUM_IDS * 8]; // id + ','

unsigned int idsChar_len[MAX_NUM_IDS];

#define NUM_LEN3_RESULT 1200000
#define NUM_LEN4_RESULT 1600000
#define NUM_LEN5_RESULT 4000000
#define NUM_LEN6_RESULT 9000000
#define NUM_LEN7_RESULT 13000000

struct Result
{
    unsigned int path[4];
    bool visited[MAX_NUM_IDS];

    Three_pred three_uj[MAX_NUM_THREE_PREDS];
    unsigned int three_uj_len;
    unsigned int reachable[MAX_NUM_IDS];
    unsigned int currentJs[MAX_NUM_THREE_PREDS];
    unsigned int currentJs_len;

    unsigned int res3[NUM_LEN3_RESULT];
    unsigned int res4[NUM_LEN4_RESULT];
    unsigned int res5[NUM_LEN5_RESULT];
    unsigned int res6[NUM_LEN6_RESULT];
    unsigned int res7[NUM_LEN7_RESULT];
    unsigned int res_count[5] = {0};
} results[NUM_THREADS];

char str_res[MAX_OUTPUT_FILE_SIZE];

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
    register unsigned int i;
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

void input_fstream(char *testFile)
{
#ifdef TEST
    clock_t input_time = clock();
#endif

    FILE *file = fopen(testFile, "r");
    unsigned int money;
    while (fscanf(file, "%d,%d,%d\n", u_ids + edge_num, v_ids + edge_num, &money) != EOF)
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
    register unsigned int temp = 0;

    for (char *p = buf; *p && p - buf < length; ++p)
    {
        if (*p != ',')
        {
            // atoi
            temp = (temp << 1) + (temp << 3) + *p - '0';
        }
        else
        {
            ++sign;
            id_count[temp] = 1;
            switch (sign)
            {
            case 1: //读取 id1
                u_ids[edge_num] = temp;
                temp = 0;
                break;
            case 2: //读取 id2
                v_ids[edge_num++] = temp;
                temp = 0;
                while (*p != '\n') // 过滤'\n'
                    ++p;
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

unsigned int res_count_digits10_length(unsigned int num)
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

unsigned int res_count_uint2ascii(unsigned int value, char *dst)
{
    static const char digits[] =
        "0001020304050607080910111213141516171819"
        "2021222324252627282930313233343536373839"
        "4041424344454647484950515253545556575859"
        "6061626364656667686970717273747576777879"
        "8081828384858687888990919293949596979899";

    const unsigned int length = res_count_digits10_length(value);
    unsigned int next = length - 1;

    while (value >= 100)
    {
        const unsigned int u = (value % 100) << 1;
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
        unsigned int u = value << 1;
        dst[next - 1] = digits[u];
        dst[next] = digits[u + 1];
    }
    return length;
}

unsigned int digits10_length(unsigned int num)
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

unsigned int uint2ascii(unsigned int value, char *dst)
{
    static const char digits[] =
        "0001020304050607080910111213141516171819"
        "2021222324252627282930313233343536373839"
        "4041424344454647484950515253545556575859"
        "6061626364656667686970717273747576777879"
        "8081828384858687888990919293949596979899";

    const unsigned int length = digits10_length(value);
    dst[length] = ',';
    unsigned int next = length - 1;

    while (value >= 100)
    {
        const unsigned int u = (value % 100) << 1;
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
        unsigned int u = value << 1;
        dst[next - 1] = digits[u];
        dst[next] = digits[u + 1];
    }
    return length;
}

void save_fwrite(char *resultFile)
{
    register unsigned int tid;
    unsigned int all_res_count = 0;

    for (tid = 0; tid < NUM_THREADS; ++tid)
    {
        all_res_count += results[tid].res_count[0] + results[tid].res_count[1] + results[tid].res_count[2] + results[tid].res_count[3] + results[tid].res_count[4];
    }

#ifdef TEST
    clock_t write_time = clock();
    printf("Total Loops %d\n", all_res_count);
#endif

    FILE *fp = fopen(resultFile, "w");

    register unsigned int str_len = res_count_uint2ascii(all_res_count, str_res);
    str_res[str_len++] = '\n';

    register unsigned int thread_offset, line_offset, line, result_id;

    // len 3
    for (tid = 0, thread_offset = 0; tid < NUM_THREADS; ++tid, thread_offset += NUM_LEN3_RESULT)
    {
        for (line = 0, line_offset = 0; line < results[tid].res_count[0]; ++line, line_offset += 3)
        {
            // result_id = results[0][thread_offset + line_offset];
            result_id = results[tid].res3[line_offset];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            // result_id = results[0][thread_offset + line_offset + 1];
            result_id = results[tid].res3[line_offset + 1];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            // result_id = results[0][thread_offset + line_offset + 2];
            result_id = results[tid].res3[line_offset + 2];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3), idsChar_len[result_id]);
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
            memcpy_128(str_res + str_len, idsComma + (result_id << 3));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = results[tid].res4[line_offset + 1];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = results[tid].res4[line_offset + 2];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = results[tid].res4[line_offset + 3];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3), idsChar_len[result_id]);
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
            memcpy_128(str_res + str_len, idsComma + (result_id << 3));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = results[tid].res5[line_offset + 1];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = results[tid].res5[line_offset + 2];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = results[tid].res5[line_offset + 3];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = results[tid].res5[line_offset + 4];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3), idsChar_len[result_id]);
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
            memcpy_128(str_res + str_len, idsComma + (result_id << 3));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = results[tid].res6[line_offset + 1];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = results[tid].res6[line_offset + 2];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = results[tid].res6[line_offset + 3];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = results[tid].res6[line_offset + 4];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = results[tid].res6[line_offset + 5];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3), idsChar_len[result_id]);
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
            memcpy_128(str_res + str_len, idsComma + (result_id << 3));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = results[tid].res7[line_offset + 1];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = results[tid].res7[line_offset + 2];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = results[tid].res7[line_offset + 3];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = results[tid].res7[line_offset + 4];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = results[tid].res7[line_offset + 5];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3), idsChar_len[result_id]);
#endif
            str_len += idsChar_len[result_id];

            result_id = results[tid].res7[line_offset + 6];
#ifdef NEON
            memcpy_128(str_res + str_len, idsComma + (result_id << 3));
#else
            memcpy(str_res + str_len, idsComma + (result_id << 3), idsChar_len[result_id]);
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

// iteration version
void pre_dfs_ite(register unsigned int start_id, register unsigned int tid)
{
    register unsigned int begin_pos[3] = {0};
    register unsigned int cur_id = start_id, next_id;
    register int depth = 0;
    register unsigned int *stack[3];
    stack[0] = g_pred[cur_id];

    while (start_id > g_pred[cur_id][begin_pos[depth]])
        ++begin_pos[depth];

    while (depth >= 0)
    {
        // no valid succ
        if (begin_pos[depth] == in_degree[cur_id])
        {
            results[tid].visited[cur_id] = false;
            cur_id = --depth > 0 ? results[tid].path[depth - 1] : start_id;
        }
        else
        {
            next_id = stack[depth][begin_pos[depth]++];
            if (!results[tid].visited[next_id])
            {
                results[tid].path[depth] = next_id;
                if (depth == 2)
                {
                    if (results[tid].path[2] != results[tid].path[0])
                        results[tid].three_uj[results[tid].three_uj_len++] = {results[tid].path[2], results[tid].path[1], results[tid].path[0]};
                }
                else
                {
                    stack[++depth] = g_pred[next_id];
                    cur_id = next_id;
                    results[tid].visited[cur_id] = true;
                    results[tid].path[depth] = cur_id;
                    begin_pos[depth] = 0;
                    if (depth < 2)
                    {
                        while (start_id >= stack[depth][begin_pos[depth]])
                            ++begin_pos[depth];
                    }
                    else
                    {
                        while (start_id > stack[depth][begin_pos[depth]])
                            ++begin_pos[depth];
                    }
                }
            }
        }
    }
}

// iteration version
void dfs_ite(register unsigned int start_id, register unsigned int tid)
{
    results[tid].visited[start_id] = true;
    results[tid].path[0] = start_id;

    register unsigned int begin_pos[4] = {0};
    while (start_id >= g_succ[start_id][begin_pos[0]])
        ++begin_pos[0];

    register unsigned int cur_id = start_id, next_id;
    register int depth = 0;
    register unsigned int *stack[4];
    stack[0] = g_succ[cur_id];

    // length 3 result
    if (results[tid].reachable[cur_id])
    {
        for (unsigned int index = results[tid].reachable[cur_id] - 1; results[tid].three_uj[index].u == cur_id; ++index)
        {
#ifdef NEON
            memcpy_128(
                results[tid].res3 + (results[tid].res_count[0] << 1) + results[tid].res_count[0],
                &results[tid].three_uj[index].u);
#else
            memcpy(
                results[tid].res3 + (results[tid].res_count[0] << 1) + results[tid].res_count[0],
                &results[tid].three_uj[index].u,
                12);
#endif
            ++results[tid].res_count[0];
        }
    }

    while (depth >= 0)
    {
        // no valid succ
        if (begin_pos[depth] == out_degree[cur_id])
        {
            results[tid].visited[cur_id] = false;
            if (--depth >= 0)
                cur_id = results[tid].path[depth];
        }
        else
        {
            next_id = stack[depth][begin_pos[depth]++];
            if (!results[tid].visited[next_id])
            {
                // find a circuit
                if (results[tid].reachable[next_id])
                {
                    switch (depth)
                    {
                    case 0:
                        for (unsigned int index = results[tid].reachable[next_id] - 1; results[tid].three_uj[index].u == next_id; ++index)
                        {
                            if (!results[tid].visited[results[tid].three_uj[index].k1] && !results[tid].visited[results[tid].three_uj[index].k2])
                            {
#ifdef NEON
                                memcpy_128(results[tid].res4 + (results[tid].res_count[1]++ << 2), results[tid].path);
                                memcpy_128(results[tid].res4 + (results[tid].res_count[1] << 2) - 3, &results[tid].three_uj[index].u);
#else
                                memcpy(results[tid].res4 + (results[tid].res_count[1]++ << 2), results[tid].path, 4);
                                memcpy(results[tid].res4 + (results[tid].res_count[1] << 2) - 3, &results[tid].three_uj[index].u, 12);
#endif
                            }
                        }
                        break;
                    case 1:
                        for (unsigned int index = results[tid].reachable[next_id] - 1; results[tid].three_uj[index].u == next_id; ++index)
                        {
                            if (!results[tid].visited[results[tid].three_uj[index].k1] && !results[tid].visited[results[tid].three_uj[index].k2])
                            {
#ifdef NEON
                                memcpy_128(results[tid].res5 + (results[tid].res_count[2] << 2) + results[tid].res_count[2]++, results[tid].path);
                                memcpy_128(results[tid].res5 + (results[tid].res_count[2] << 2) + results[tid].res_count[2] - 3, &results[tid].three_uj[index].u);
#else
                                memcpy(results[tid].res5 + (results[tid].res_count[2] << 2) + results[tid].res_count[2]++, results[tid].path, 8);
                                memcpy(results[tid].res5 + (results[tid].res_count[2] << 2) + results[tid].res_count[2] - 3, &results[tid].three_uj[index].u, 12);
#endif
                            }
                        }

                        break;
                    case 2:
                        for (unsigned int index = results[tid].reachable[next_id] - 1; results[tid].three_uj[index].u == next_id; ++index)
                        {
                            if (!results[tid].visited[results[tid].three_uj[index].k1] && !results[tid].visited[results[tid].three_uj[index].k2])
                            {
#ifdef NEON
                                memcpy_128(results[tid].res6 + (results[tid].res_count[3] << 2) + (results[tid].res_count[3]++ << 1), results[tid].path);
                                memcpy_128(results[tid].res6 + (results[tid].res_count[3] << 2) + (results[tid].res_count[3] << 1) - 3, &results[tid].three_uj[index].u);
#else
                                memcpy(results[tid].res6 + (results[tid].res_count[3] << 2) + (results[tid].res_count[3]++ << 1), results[tid].path, 12);
                                memcpy(results[tid].res6 + (results[tid].res_count[3] << 2) + (results[tid].res_count[3] << 1) - 3, &results[tid].three_uj[index].u, 12);
#endif
                            }
                        }
                        break;
                    case 3:
                        for (unsigned int index = results[tid].reachable[next_id] - 1; results[tid].three_uj[index].u == next_id; ++index)
                        {
                            if (!results[tid].visited[results[tid].three_uj[index].k1] && !results[tid].visited[results[tid].three_uj[index].k2])
                            {
#ifdef NEON
                                memcpy_128(results[tid].res7 + (results[tid].res_count[4] << 2) + (results[tid].res_count[4] << 1) + results[tid].res_count[4]++, results[tid].path);
                                memcpy_128(results[tid].res7 + (results[tid].res_count[4] << 2) + (results[tid].res_count[4] << 1) + results[tid].res_count[4] - 3, &results[tid].three_uj[index].u);
#else
                                memcpy(results[tid].res7 + (results[tid].res_count[4] << 2) + (results[tid].res_count[4] << 1) + results[tid].res_count[4]++, results[tid].path, 16);
                                memcpy(results[tid].res7 + (results[tid].res_count[4] << 2) + (results[tid].res_count[4] << 1) + results[tid].res_count[4] - 3, &results[tid].three_uj[index].u, 12);
#endif
                            }
                        }
                        break;
                    default:
                        break;
                    }
                }

                if (depth < 3)
                {
                    stack[++depth] = g_succ[next_id];
                    cur_id = next_id;
                    results[tid].visited[cur_id] = true;
                    results[tid].path[depth] = cur_id;
                    begin_pos[depth] = 0;
                    while (start_id >= stack[depth][begin_pos[depth]])
                        ++begin_pos[depth];
                }
            }
        }
    }
}

bool three_uj_cmp(Three_pred &a, Three_pred &b)
{
    return a.u != b.u ? a.u < b.u : (a.k1 != b.k1 ? a.k1 < b.k1 : a.k2 < b.k2);
}

// 这里定义子线程函数, 如果处理不同分段的数据不一样,那就写多个函数
// 函数的参数传入的时候需要是 void * 类型, 一般就是数据的指针转成  无类型指针
void *thread_process(void *t)
{

#ifdef TEST
    clock_t search_time = clock();
#endif

    // 先把指针类型恢复, 然后取值
    register unsigned int tid = *((unsigned int *)t);

    // unsigned int end_id = (unsigned int)(seg_ratio[tid + 1] * id_num);
    // for (unsigned int start_id = (unsigned int)(seg_ratio[tid] * id_num); start_id < end_id; ++start_id)
    for (unsigned int start_id = seg_id[tid]; start_id < seg_id[tid + 1]; ++start_id)
    {
#ifdef TEST
        // if (start_id % 100 == 0)
        // {
        //     printf("%d/%d ~ %.2lfs ~ %2.lf%%\n", start_id, id_num, (double)(clock() - search_time) / CLOCKS_PER_SEC, (double)(start_id) / id_num * 100);
        // }
#endif
        pre_dfs_ite(start_id, tid);
        // pre_dfs_rec(start_id, start_id, 0, tid);
        // 有直达的点才继续搜下去
        if (results[tid].three_uj_len)
        {
            sort(results[tid].three_uj, results[tid].three_uj + results[tid].three_uj_len, three_uj_cmp);
            results[tid].three_uj[results[tid].three_uj_len] = {MAX_INT, MAX_INT, MAX_INT};
            for (unsigned int index = 0; index < results[tid].three_uj_len; ++index)
            {
                if (!results[tid].reachable[results[tid].three_uj[index].u])
                {
                    results[tid].reachable[results[tid].three_uj[index].u] = index + 1;
                    results[tid].currentJs[results[tid].currentJs_len++] = results[tid].three_uj[index].u;
                }
            }

            dfs_ite(start_id, tid);
            // dfs_rec(start_id, start_id, 0, tid);

            for (unsigned int j = 0; j < results[tid].currentJs_len; ++j)
            {
                results[tid].reachable[results[tid].currentJs[j]] = 0;
            }
            results[tid].three_uj_len = 0;
            results[tid].currentJs_len = 0;
        }
    }

#ifdef TEST
    cout << "tid " << tid << " DFS " << (double)(clock() - search_time) / CLOCKS_PER_SEC << "s" << endl;
#endif
    // pthread_exit(NULL) 为退出该线程
    pthread_exit(NULL);
}

int main()
{
#ifdef TEST
    // std
    // 3738
    // 38252
    // 58284
    // 77409
    // E 28W N 30000 A 1004812
    // E 28W N 28500 A 2861665
    // E 28W N 25700 A 2896262
    // E 28W N 25000 A 3512444
    // E 28W N 59989 A 2755223
    char testFile[] = "test_data/1004812/test_data.txt";
    char resultFile[] = "test_data/1004812/result.txt";
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

    register unsigned int index;

    for (index = 1; index < MAX_NUM_IDS; ++index)
    {
        id_count[index] += id_count[index - 1];
    }
    id_num = id_count[MAX_NUM_IDS - 1];

    // 获取hash_id，生成ids字符串，生成邻接表与逆邻接表
    register unsigned int u_hash_id, v_hash_id;
    for (index = 0; index < edge_num - 3; index += 4)
    {
        u_hash_id = id_count[u_ids[index]] - 1;
        v_hash_id = id_count[v_ids[index]] - 1;
        ids[u_hash_id] = u_ids[index];
        ids[v_hash_id] = v_ids[index];
        g_succ[u_hash_id][out_degree[u_hash_id]++] = v_hash_id;
        g_pred[v_hash_id][in_degree[v_hash_id]++] = u_hash_id;

        u_hash_id = id_count[u_ids[index + 1]] - 1;
        v_hash_id = id_count[v_ids[index + 1]] - 1;
        ids[u_hash_id] = u_ids[index + 1];
        ids[v_hash_id] = v_ids[index + 1];
        g_succ[u_hash_id][out_degree[u_hash_id]++] = v_hash_id;
        g_pred[v_hash_id][in_degree[v_hash_id]++] = u_hash_id;

        u_hash_id = id_count[u_ids[index + 2]] - 1;
        v_hash_id = id_count[v_ids[index + 2]] - 1;
        ids[u_hash_id] = u_ids[index + 2];
        ids[v_hash_id] = v_ids[index + 2];
        g_succ[u_hash_id][out_degree[u_hash_id]++] = v_hash_id;
        g_pred[v_hash_id][in_degree[v_hash_id]++] = u_hash_id;

        u_hash_id = id_count[u_ids[index + 3]] - 1;
        v_hash_id = id_count[v_ids[index + 3]] - 1;
        ids[u_hash_id] = u_ids[index + 3];
        ids[v_hash_id] = v_ids[index + 3];
        g_succ[u_hash_id][out_degree[u_hash_id]++] = v_hash_id;
        g_pred[v_hash_id][in_degree[v_hash_id]++] = u_hash_id;
    }

    for (; index < edge_num; ++index)
    {
        u_hash_id = id_count[u_ids[index]] - 1;
        v_hash_id = id_count[v_ids[index]] - 1;

        ids[u_hash_id] = u_ids[index];
        ids[v_hash_id] = v_ids[index];

        g_succ[u_hash_id][out_degree[u_hash_id]++] = v_hash_id;
        g_pred[v_hash_id][in_degree[v_hash_id]++] = u_hash_id;
    }

    for (index = 0; index < id_num - 3; index += 4)
    {
        sort(g_succ[index], g_succ[index] + out_degree[index]);
        sort(g_pred[index], g_pred[index] + in_degree[index]);
        g_succ[index][out_degree[index]] = MAX_INT;
        g_pred[index][in_degree[index]] = MAX_INT;
        idsChar_len[index] = uint2ascii(ids[index], idsComma + (index << 3)) + 1;

        sort(g_succ[index + 1], g_succ[index + 1] + out_degree[index + 1]);
        sort(g_pred[index + 1], g_pred[index + 1] + in_degree[index + 1]);
        g_succ[index + 1][out_degree[index + 1]] = MAX_INT;
        g_pred[index + 1][in_degree[index + 1]] = MAX_INT;
        idsChar_len[index + 1] = uint2ascii(ids[index + 1], idsComma + ((index + 1) << 3)) + 1;

        sort(g_succ[index + 2], g_succ[index + 2] + out_degree[index + 2]);
        sort(g_pred[index + 2], g_pred[index + 2] + in_degree[index + 2]);
        g_succ[index + 2][out_degree[index + 2]] = MAX_INT;
        g_pred[index + 2][in_degree[index + 2]] = MAX_INT;
        idsChar_len[index + 2] = uint2ascii(ids[index + 2], idsComma + ((index + 2) << 3)) + 1;

        sort(g_succ[index + 3], g_succ[index + 3] + out_degree[index + 3]);
        sort(g_pred[index + 3], g_pred[index + 3] + in_degree[index + 3]);
        g_succ[index + 3][out_degree[index + 3]] = MAX_INT;
        g_pred[index + 3][in_degree[index + 3]] = MAX_INT;
        idsChar_len[index + 3] = uint2ascii(ids[index + 3], idsComma + ((index + 3) << 3)) + 1;
    }

    for (; index < id_num; ++index)
    {
        sort(g_succ[index], g_succ[index] + out_degree[index]);
        sort(g_pred[index], g_pred[index] + in_degree[index]);
        g_succ[index][out_degree[index]] = MAX_INT;
        g_pred[index][in_degree[index]] = MAX_INT;
        idsChar_len[index] = uint2ascii(ids[index], idsComma + (index << 3)) + 1;
    }

    register unsigned int tid;
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

    // 均分节点任务
    unsigned int w = (unsigned int)(id_num * 1.0 / NUM_THREADS);
    unsigned int left = w;
    for (index = 1; index < NUM_THREADS; ++index, left += w)
    {
        seg_id[index] = left;
    }
    seg_id[NUM_THREADS] = id_num;

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
