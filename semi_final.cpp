// submit:
// 1. close #define TEST.
// 2. open //#define MMAP
// 3. open //#define NEON

#define TEST
#define MMAP // 使用mmap函数
// #define NEON // 打开NEON特性的算子，开了反而会慢
// #define MALLOC

#include <bits/stdc++.h>
#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/hash_policy.hpp>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
using namespace std;

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
string dataset = "18875018";
#endif

#ifdef MMAP
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#endif

#ifdef NEON
#include <arm_neon.h>
#include <stddef.h>
#endif

#define NUM_THREADS 4      // 线程数
#define NUM_BATCHS 128 + 1 // 分块数
#define MIN_BATCH_SIZE 128 // 每一块至少有128个点，不然就按128点分块

#define MAX_NUM_EDGES 2000005 // 最大可接受边数 200w+5 确定够用
#define MAX_NUM_IDS 2000005   // 最大可接受id数 200w+5 确定够用

#define MAX_NUM_THREE_PREDS 1000000 // 反向三级跳表的长度 应该够用

#define MAX_INT 2147483648 // 2^31

#define RESERVE_MEMORY 2684354560          // 预留2.5G内存数组
#define MEMORY_BLOCK_SIZE 80 * 1024 * 1024 // 分片大小为80M

typedef unsigned long long ull;
typedef unsigned int ui;

// 总id数，总边数
ui id_num = 0, edge_num = 0;

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

ui input_data[MAX_NUM_EDGES][3];
ui u_next[MAX_NUM_EDGES];
ui v_next[MAX_NUM_EDGES];
ui ids[MAX_NUM_EDGES * 2];
ui succ_begin_pos[MAX_NUM_IDS]; // 对于邻接表每个节点的起始index
ui pred_begin_pos[MAX_NUM_IDS]; // 对于逆邻接表每个节点的起始index
ui out_degree[MAX_NUM_IDS];     // 每个节点的出度
ui in_degree[MAX_NUM_IDS];      // 每个节点的入度
Node g_pred[MAX_NUM_EDGES * 2];
Node g_succ[MAX_NUM_EDGES * 2];
ui topo_stack[MAX_NUM_IDS];
__gnu_pbds::gp_hash_table<ui, ui> id_map; // map really id to regular id(0,1,2,...)

char idsComma[MAX_NUM_IDS * 16]; // id + ','
ui idsChar_len[MAX_NUM_IDS];     // 每个id的字符串长度

#ifndef MALLOC
char reserve_memory[RESERVE_MEMORY];
char *new_memory_ptr = reserve_memory;
#else
char *new_memory_ptr;
#endif
mutex memory_lock;

// 每个线程专属区域
struct ThreadMemory
{
    // dfs中用的数据
    char char_path[80]; // 已经走过的节点信息
    char *path_pos[5];  // path中每一层结果的起始位置

    // pre_dfs和dfs公用的数据
    ui path[4];                // 已经走过的节点信息
    ui m_path[4];              // 已经走过点的金额信息
    bool visited[MAX_NUM_IDS]; // 访问标记数组

    ui temp_three_uj[MAX_NUM_THREE_PREDS][5]; // 临时反向三级跳表
    Three_pred three_uj[MAX_NUM_THREE_PREDS]; // 最终反向三级跳表
    ui three_uj_len;                          // 三级跳表长度
    ui begin_end_pos[MAX_NUM_IDS][2];         // 若此点可达起点，则起点记录其在三级跳表中的index + 1，否则记录0，终点pos取不到，范围是[起点pos-1， 终点pos)
    ui currentUs[MAX_NUM_IDS];                // 可以通过三次跳跃回起点的点
    ui currentUs_len;                         // 当前可以通过三次跳跃回起点的点数量

    char *next_res_ptr[5]; // 下一个结果的写入指针
    ui res_count[5];       // 各个长度结果数量记录
    ui remaining_size[5];  // 各个长度结果当前分片剩余大小
} thread_memory[NUM_THREADS];

struct Batch
{
    ui cur_block[5]; // 记录当前分片号，范围是[0, 7]
    char *batch_begin_pos[5][8];
    char *batch_end_pos[5][8];
} batchs[NUM_BATCHS];

ui batch_num, batch_size, cur_batch_id;
mutex batch_lock;

#ifdef NEON
// 16个字节以内的复制
inline void memcpy_16(void *dest, void *src)
{
    vst1q_u64((unsigned long *)dest, vld1q_u64((unsigned long *)src));
}

// 128字节复制
inline void memcpy_128(void *dest, void *src)
{
    unsigned long *s = (unsigned long *)src;
    unsigned long *d = (unsigned long *)dest;
    vst1q_u64(&d[0], vld1q_u64(&s[0]));
    vst1q_u64(&d[2], vld1q_u64(&s[2]));
    vst1q_u64(&d[4], vld1q_u64(&s[4]));
    vst1q_u64(&d[6], vld1q_u64(&s[6]));
}

// 大长度字节的复制
inline void memcpy_neon(void *dest, void *src, size_t count)
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

#ifdef TEST
UniversalTimer timerA, timerB;
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
    // long length = lseek(fd, 0, SEEK_END);

    struct stat stat_buf;
    fstat(fd, &stat_buf);
    size_t length = stat_buf.st_size;

    //mmap
    char *buf = (char *)mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, 0);

    ui *input_ptr = input_data[0];
    ui num = 0;
    register ui index = 0;
    char cur_char;
    char *p = buf;

    for (; index < length; ++index, ++p)
    {
        cur_char = *p;
        if (cur_char >= '0')
        {
            // atoi
            num = (num << 1) + (num << 3) + cur_char - '0';
        }
        else if (cur_char != '\r')
        {
            input_ptr[edge_num++] = num;
            num = 0;
        }
    }

    edge_num /= 3;
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
    FILE *fp = fopen(resultFile, "wb");

    register ui tid;
    ui all_res_count = 0;
    for (tid = 0; tid < NUM_THREADS; ++tid)
        all_res_count += thread_memory[tid].res_count[0] + thread_memory[tid].res_count[1] + thread_memory[tid].res_count[2] + thread_memory[tid].res_count[3] + thread_memory[tid].res_count[4];

#ifdef TEST
    printf("Total Loops %d\n", all_res_count);
#endif

    char char_res_count[32];
    ui result_str_len = uint2ascii(all_res_count, char_res_count);
    char_res_count[result_str_len++] = '\n';
    fwrite(char_res_count, sizeof(char), result_str_len, fp);

    register ui depth, bid, b_it;

    for (depth = 0; depth < 5; ++depth)
    {
        for (bid = 0; bid < batch_num; ++bid)
        {
            auto &block = batchs[bid];
            ui cur_block = block.cur_block[depth];
            for (b_it = 0; b_it <= cur_block; ++b_it)
            {
                char *begin = block.batch_begin_pos[depth][b_it];
                char *end = block.batch_end_pos[depth][b_it];
                if (begin < end)
                {
                    fwrite(begin, sizeof(char), end - begin, fp);
                }
            }
        }
    }
    fclose(fp);
}

void save_unistd_write(char *resultFile)
{
    int fd = open(resultFile, O_RDWR | O_CREAT, 0666);

    register ui tid;
    ui all_res_count = 0;
    for (tid = 0; tid < NUM_THREADS; ++tid)
        all_res_count += thread_memory[tid].res_count[0] + thread_memory[tid].res_count[1] + thread_memory[tid].res_count[2] + thread_memory[tid].res_count[3] + thread_memory[tid].res_count[4];

#ifdef TEST
    printf("Total Loops %d\n", all_res_count);
#endif

    char char_res_count[32];
    ui result_str_len = uint2ascii(all_res_count, char_res_count);
    char_res_count[result_str_len++] = '\n';
    write(fd, char_res_count, result_str_len);

    register ui depth, bid, b_it;

    for (depth = 0; depth < 5; ++depth)
    {
        for (bid = 0; bid < batch_num; ++bid)
        {
            ui cur_block = batchs[bid].cur_block[depth];
            for (b_it = 0; b_it <= cur_block; ++b_it)
            {
                char *begin_addr = batchs[bid].batch_begin_pos[depth][b_it];
                char *end_addr = batchs[bid].batch_end_pos[depth][b_it];
                if (begin_addr < end_addr)
                {
                    write(fd, begin_addr, end_addr - begin_addr);
                }
            }
        }
    }
    close(fd);
}

inline bool is_money_valid(ui x, ui y)
{
    return x > y ? (y & 0x40000000 ? true : x - y <= (y << 2)) : y - x <= (x << 1);
}

// iteration version
// 反向三级dfs的迭代版本
void pre_dfs_ite(ui start_id, ThreadMemory *this_thread)
{
    auto &visited = this_thread->visited;
    auto &path = this_thread->path;
    auto &m_path = this_thread->m_path;
    auto &begin_end_pos = this_thread->begin_end_pos;
    auto &temp_three_uj = this_thread->temp_three_uj;
    auto &three_uj_len = this_thread->three_uj_len;
    auto &currentUs_len = this_thread->currentUs_len;
    auto &currentUs = this_thread->currentUs;

    ui cur_id = start_id, next_id;
    register int depth = 0;
    // 递归栈
    Node *stack[3];

    stack[0] = g_pred + pred_begin_pos[start_id];

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
            visited[cur_id] = false;
            if (--depth > 0)
                cur_id = path[depth - 1];
        }
        // 有前驱
        else
        {
            m_path[depth] = (stack[depth]++)->money;

            if (!visited[next_id] && (!depth || is_money_valid(m_path[depth], m_path[depth - 1])))
            {
                path[depth] = next_id;
                // 深度为2，装载反向三级跳表
                if (depth == 2)
                {
                    if (next_id != path[0])
                    {
                        ui &next_three_node = begin_end_pos[next_id][0];
                        temp_three_uj[three_uj_len][0] = path[1];
                        temp_three_uj[three_uj_len][1] = path[0];
                        temp_three_uj[three_uj_len][2] = m_path[2];
                        temp_three_uj[three_uj_len][3] = m_path[0];
                        temp_three_uj[three_uj_len][4] = next_three_node;

                        if (!next_three_node)
                            currentUs[currentUs_len++] = next_id;
                        next_three_node = ++(three_uj_len);
                    }
                }
                // 深度不足2，且next_id还有前驱，则继续搜索
                else if (in_degree[next_id])
                {
                    // 进入下一层dfs
                    stack[++depth] = g_pred + pred_begin_pos[next_id];
                    path[depth] = cur_id = next_id;
                    visited[cur_id] = true;

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

void sort_three_uj(ThreadMemory *this_thread)
{
    auto &currentUs_len = this_thread->currentUs_len;
    auto &currentUs = this_thread->currentUs;
    auto &begin_end_pos = this_thread->begin_end_pos;
    auto &temp_three_uj = this_thread->temp_three_uj;
    auto &three_uj = this_thread->three_uj;

    ui three_uj_iterator = 0;
    ui three_uj_index = 0;
    for (ui index = 0; index < currentUs_len; ++index)
    {
        ui U = currentUs[index];
        three_uj_iterator = begin_end_pos[U][0];
        // begin_pos加一存储
        begin_end_pos[U][0] = three_uj_index + 1;
        while (three_uj_iterator != 0)
        {
            ui *temp = temp_three_uj[three_uj_iterator - 1];
            three_uj[three_uj_index++] = {temp[0], temp[1], temp[2], temp[3]};
            three_uj_iterator = temp[4];
        };
        sort(three_uj + begin_end_pos[U][0] - 1, three_uj + three_uj_index);
        begin_end_pos[U][1] = three_uj_index;
    }
}

inline void generate_result(ui depth, ui next_id, ThreadMemory *this_thread, Batch *this_batch)
{
    // 例：
    // 要保存长度为4的结果，depth = 0，加一操作后depth为1
    // path_pos[0]表示path中第一个元素的终止位置
    // this_thread->next_res[1]表示长度为4的结果保存位置
    auto &next_res_ptr = this_thread->next_res_ptr;
    auto &three_uj = this_thread->three_uj;
    auto &remaining_size = this_thread->remaining_size;
    auto &char_path = this_thread->char_path;
    auto &begin_end_pos = this_thread->begin_end_pos;
    auto &m_path = this_thread->m_path;
    auto &visited = this_thread->visited;
    auto &path_pos = this_thread->path_pos;
    auto &res_count = this_thread->res_count;

    auto &cur_block = this_batch->cur_block;
    auto &batch_begin_pos = this_batch->batch_begin_pos;
    auto &batch_end_pos = this_batch->batch_end_pos;

    for (ui index = begin_end_pos[next_id][0] - 1; index < begin_end_pos[next_id][1]; ++index)
    {
        ui k1 = three_uj[index].k1, k2 = three_uj[index].k2;
        if (!visited[k1] && !visited[k2] &&
            is_money_valid(m_path[depth - 1], three_uj[index].first_money) &&
            is_money_valid(three_uj[index].last_money, m_path[0]))
        {
            // 也许不够保存这条路径了
            if (remaining_size[depth] < 80)
            {
                batch_end_pos[depth][cur_block[depth]++] = next_res_ptr[depth];
                memory_lock.lock();
                next_res_ptr[depth] = new_memory_ptr;
                new_memory_ptr += MEMORY_BLOCK_SIZE;
                memory_lock.unlock();
                remaining_size[depth] = MEMORY_BLOCK_SIZE;
                batch_begin_pos[depth][cur_block[depth]] = next_res_ptr[depth];
            }

            char *begin_pos = next_res_ptr[depth];
            // 保存已经走过的点
            ui path_len = path_pos[depth - 1] - char_path;
#ifdef NEON
            memcpy_128(next_res_ptr[depth], char_path);
#else
            memcpy(next_res_ptr[depth], char_path, path_len);
#endif
            next_res_ptr[depth] += path_len;

// 保存next_id
#ifdef NEON
            memcpy_16(next_res_ptr[depth], idsComma + (next_id << 4));
#else
            memcpy(next_res_ptr[depth], idsComma + (next_id << 4), 16);
#endif
            next_res_ptr[depth] += idsChar_len[next_id];

            // 保存反向三级跳表
#ifdef NEON
            memcpy_16(next_res_ptr[depth], idsComma + (k1 << 4));
#else
            memcpy(next_res_ptr[depth], idsComma + (k1 << 4), 16);
#endif

            next_res_ptr[depth] += idsChar_len[k1];
#ifdef NEON
            memcpy_16(next_res_ptr[depth], idsComma + (k2 << 4));
#else
            memcpy(next_res_ptr[depth], idsComma + (k2 << 4), 16);
#endif
            next_res_ptr[depth] += idsChar_len[k2];
            *(next_res_ptr[depth] - 1) = '\n';

            ++res_count[depth];
            remaining_size[depth] -= next_res_ptr[depth] - begin_pos;
        }
    }
}

// iteration version
// 正向dfs的迭代版本
void dfs_ite(ui start_id, ThreadMemory *this_thread, Batch *this_batch)
{
    auto &begin_end_pos = this_thread->begin_end_pos;
    auto &visited = this_thread->visited;
    auto &path = this_thread->path;
    auto &char_path = this_thread->char_path;
    auto &path_pos = this_thread->path_pos;
    auto &three_uj = this_thread->three_uj;
    auto &next_res_ptr = this_thread->next_res_ptr;
    auto &remaining_size = this_thread->remaining_size;
    auto &res_count = this_thread->res_count;
    auto &m_path = this_thread->m_path;

    auto &batch_begin_pos = this_batch->batch_begin_pos;
    auto &batch_end_pos = this_batch->batch_end_pos;
    auto &cur_block = this_batch->cur_block;

    visited[start_id] = true;
    path[0] = start_id;
#ifdef NEON
    memcpy_16(char_path, idsComma + (start_id << 4));
#else
    memcpy(char_path, idsComma + (start_id << 4), 16);
#endif
    path_pos[0] = char_path + idsChar_len[start_id];

    ui cur_id = start_id, next_id;
    register int depth = 0;
    // 递归栈
    Node *stack[4];

    stack[0] = g_succ + succ_begin_pos[start_id];
    while (start_id > stack[0]->dst_id)
        ++stack[0];

    // length 3 result
    // 首先查找所有长度为3的结果，因为不需要搜索就可以得到
    if (begin_end_pos[cur_id][0])
    {
        auto &save_pos = next_res_ptr[0];
        for (ui index = begin_end_pos[cur_id][0] - 1; index < begin_end_pos[cur_id][1]; ++index)
        {
            if (is_money_valid(three_uj[index].last_money, three_uj[index].first_money))
            {
                // 也许不够保存这条路径了
                if (remaining_size[0] < 80)
                {
                    batch_end_pos[0][cur_block[0]++] = next_res_ptr[0];
                    memory_lock.lock();
                    next_res_ptr[0] = new_memory_ptr;
                    new_memory_ptr += MEMORY_BLOCK_SIZE;
                    memory_lock.unlock();
                    remaining_size[0] = MEMORY_BLOCK_SIZE;
                    batch_begin_pos[0][cur_block[0]] = next_res_ptr[0];
                }

                char *begin_pos = next_res_ptr[depth];
// cur_id
#ifdef NEON
                memcpy_16(save_pos, idsComma + (cur_id << 4));
#else
                memcpy(save_pos, idsComma + (cur_id << 4), 16);
#endif
                save_pos += idsChar_len[cur_id];

                // 保存反向三级跳表
                ui k1 = three_uj[index].k1, k2 = three_uj[index].k2;
#ifdef NEON
                memcpy_16(save_pos, idsComma + (k1 << 4));
#else
                memcpy(save_pos, idsComma + (k1 << 4), 16);
#endif
                save_pos += idsChar_len[k1];
#ifdef NEON
                memcpy_16(save_pos, idsComma + (k2 << 4));
#else
                memcpy(save_pos, idsComma + (k2 << 4), 16);
#endif
                save_pos += idsChar_len[k2];
                *(save_pos - 1) = '\n';

                ++res_count[0];
                remaining_size[0] -= save_pos - begin_pos;
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
            visited[cur_id] = false;
            // dfs中用的数据
            if (--depth >= 0)
                cur_id = path[depth];
        }
        // 有后继
        else
        {
            m_path[depth] = (stack[depth]++)->money;

            if (!visited[next_id] && (!depth || is_money_valid(m_path[depth - 1], m_path[depth])))
            {
                // 找到环
                if (begin_end_pos[next_id][0])
                {
                    generate_result(depth + 1, next_id, this_thread, this_batch);
                }

                // 深度不足3，且next_id还有后继，则继续搜索
                if (depth < 3 && out_degree[next_id])
                {
                    // 向更深一层dfs
                    stack[++depth] = g_succ + succ_begin_pos[next_id];
                    path[depth] = cur_id = next_id;
                    visited[cur_id] = true;
#ifdef NEON
                    memcpy_16(path_pos[depth - 1], idsComma + (cur_id << 4));
#else
                    memcpy(path_pos[depth - 1], idsComma + (cur_id << 4), 16);
#endif
                    path_pos[depth] = path_pos[depth - 1] + idsChar_len[cur_id];
                    // 寻找起始位置
                    while (start_id >= stack[depth]->dst_id)
                        ++stack[depth];
                }
            }
        }
    }
}

void batch_process(ui this_batch_id, ui tid, ThreadMemory *this_thread)
{
    auto &begin_end_pos = this_thread->begin_end_pos;
    auto &currentUs = this_thread->currentUs;
    auto &next_res_ptr = this_thread->next_res_ptr;
    auto &three_uj_len = this_thread->three_uj_len;
    auto &currentUs_len = this_thread->currentUs_len;

    Batch *this_batch = batchs + this_batch_id;
    auto &cur_block = this_batch->cur_block;
    auto &batch_begin_pos = this_batch->batch_begin_pos;
    auto &batch_end_pos = this_batch->batch_end_pos;

    ui end_id = (this_batch_id + 1) * batch_size;
    end_id = end_id < id_num ? end_id : id_num;

    for (ui depth = 0; depth < 5; ++depth)
        batch_begin_pos[depth][0] = next_res_ptr[depth];

    for (ui start_id = this_batch_id * batch_size; start_id < end_id; ++start_id)
    {
        // 起点必须有后继也有前驱才进入dfs
        if (out_degree[start_id] == 0 || in_degree[start_id] == 0)
            continue;

        pre_dfs_ite(start_id, this_thread);

        // 有直达的点才继续搜下去
        if (three_uj_len)
        {
            // 反向三级跳表排序
            sort_three_uj(this_thread);
            dfs_ite(start_id, this_thread, this_batch);

            // reachable和currentUs还原
            for (ui U = 0; U < currentUs_len; ++U)
            {
                begin_end_pos[currentUs[U]][0] = 0;
                begin_end_pos[currentUs[U]][1] = 0;
            }
            three_uj_len = 0;
            currentUs_len = 0;
        }
    }

    for (ui depth = 0; depth < 5; ++depth)
    {
        batch_end_pos[depth][cur_block[depth]] = next_res_ptr[depth];
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
    ui tid = *((ui *)t), this_batch_id;
    ThreadMemory *this_thread = &thread_memory[tid];

    while (true)
    {
        batch_lock.lock();
        if (cur_batch_id >= batch_num)
        {
            batch_lock.unlock();
            break;
        }
        else
        {
            this_batch_id = cur_batch_id++;
            batch_lock.unlock();
            batch_process(this_batch_id, tid, this_thread);
        }
    }
    // 退出该线程
    pthread_exit(NULL);
}

void build_idComma()
{
    register ui index = 0;
    for (; index < id_num - 3; index += 4)
    {
        idsChar_len[index] = uint2ascii(ids[index], idsComma + (index << 4)) + 1;
        idsChar_len[index + 1] = uint2ascii(ids[index + 1], idsComma + (index << 4) + 16) + 1;
        idsChar_len[index + 2] = uint2ascii(ids[index + 2], idsComma + (index << 4) + 32) + 1;
        idsChar_len[index + 3] = uint2ascii(ids[index + 3], idsComma + (index << 4) + 48) + 1;
    }

    for (; index < id_num; ++index)
    {
        idsChar_len[index] = uint2ascii(ids[index], idsComma + (index << 4)) + 1;
    }
}

void prework()
{
    register ui index = 0;
    // 真实id与hashid的映射处理
    for (; index < id_num - 3; index += 4)
    {
        id_map[ids[index]] = index;
        id_map[ids[index + 1]] = index + 1;
        id_map[ids[index + 2]] = index + 2;
        id_map[ids[index + 3]] = index + 3;
    }

    for (; index < id_num; ++index)
    {
        id_map[ids[index]] = index;
    }

    ui u_hash_id, v_hash_id;
    for (index = 0; index < edge_num; ++index)
    {
        if (id_map.find(input_data[index][1]) == id_map.end())
            continue;
        // 改为regular id
        u_hash_id = input_data[index][0] = id_map[input_data[index][0]];
        v_hash_id = input_data[index][1] = id_map[input_data[index][1]];

        // 链表串起来
        u_next[index] = succ_begin_pos[u_hash_id];
        succ_begin_pos[u_hash_id] = index + 1;

        v_next[index] = pred_begin_pos[v_hash_id];
        pred_begin_pos[v_hash_id] = index + 1;
        // 出度和入度
        out_degree[u_hash_id]++;
        in_degree[v_hash_id]++;
    }
}

void topo_sort()
{
    register ui index;
    ui u_hash_id, v_hash_id;

    // 去除所有入度为0的点
    int topo_index = -1;
    for (index = 0; index < id_num; ++index)
    {
        if (!in_degree[index])
            topo_stack[++topo_index] = index;
    }
    while (topo_index >= 0)
    {
        u_hash_id = topo_stack[topo_index--];
        index = succ_begin_pos[u_hash_id];
        while (index)
        {
            v_hash_id = input_data[index - 1][1];
            --out_degree[u_hash_id];
            if (--in_degree[v_hash_id] == 0)
                topo_stack[++topo_index] = v_hash_id;

            input_data[index - 1][0] = MAX_INT; // 标记此边失效
            index = u_next[index - 1];
        }
    }

    // 去除所有出度为0的点
    for (index = 0; index < id_num; ++index)
    {
        if (!out_degree[index])
            topo_stack[++topo_index] = index;
    }
    while (topo_index >= 0)
    {
        v_hash_id = topo_stack[topo_index--];
        index = pred_begin_pos[v_hash_id];
        while (index)
        {
            u_hash_id = input_data[index - 1][0];
            // 走到失效边上
            if (u_hash_id & 0x80000000)
            {
                index = v_next[index - 1];
                continue;
            }

            --in_degree[v_hash_id];
            if (--out_degree[u_hash_id] == 0)
                topo_stack[++topo_index] = u_hash_id;

            input_data[index - 1][1] = MAX_INT; // 标记此边失效
            index = v_next[index - 1];
        }
    }
}

void build_g_succ()
{
    ui succ_iterator = 0;
    ui succ_index = 0;

    for (ui cur_id = 0; cur_id < id_num; ++cur_id)
    {
        if (out_degree[cur_id] && in_degree[cur_id])
        {
            succ_iterator = succ_begin_pos[cur_id];
            succ_begin_pos[cur_id] = succ_index;
            while (succ_iterator != 0)
            {
                if (input_data[succ_iterator - 1][1] & 0x80000000)
                {
                    succ_iterator = u_next[succ_iterator - 1];
                    continue;
                }
                g_succ[succ_index++] = {input_data[succ_iterator - 1][1], input_data[succ_iterator - 1][2]};
                succ_iterator = u_next[succ_iterator - 1];
            }
            sort(g_succ + succ_begin_pos[cur_id], g_succ + succ_begin_pos[cur_id] + out_degree[cur_id]);
            g_succ[succ_index++] = {MAX_INT, MAX_INT};
        }
    }
}

void build_g_pred()
{
    ui pred_iterator = 0;
    ui pred_index = 0;
    for (ui cur_id = 0; cur_id < id_num; ++cur_id)
    {
        if (out_degree[cur_id] && in_degree[cur_id])
        {
            pred_iterator = pred_begin_pos[cur_id];
            pred_begin_pos[cur_id] = pred_index;
            while (pred_iterator != 0)
            {
                if (input_data[pred_iterator - 1][0] & 0x80000000)
                {
                    pred_iterator = v_next[pred_iterator - 1];
                    continue;
                }
                g_pred[pred_index++] = {input_data[pred_iterator - 1][0], input_data[pred_iterator - 1][2]};
                pred_iterator = v_next[pred_iterator - 1];
            }
            sort(g_pred + pred_begin_pos[cur_id], g_pred + pred_begin_pos[cur_id] + in_degree[cur_id]);
            g_pred[pred_index++] = {MAX_INT, MAX_INT};
        }
    }
}

void pre_process()
{
    register ui index;
    for (index = 0; index < edge_num - 3; index += 4)
    {
        ids[id_num++] = input_data[index][0];
        ids[id_num++] = input_data[index + 1][0];
        ids[id_num++] = input_data[index + 2][0];
        ids[id_num++] = input_data[index + 3][0];
    }

    for (; index < edge_num; ++index)
    {
        ids[id_num++] = input_data[index][0];
    }
    sort(ids, ids + id_num);
    id_num = unique(ids, ids + id_num) - ids;

#ifdef TEST
    timerB.resetTimeWithTag("Unique Id");
#endif

    thread thread_idComma = thread(build_idComma);

    prework();

#ifdef TEST
    timerB.resetTimeWithTag("Pre work");
#endif

    topo_sort();

#ifdef TEST
    timerB.resetTimeWithTag("topo_sort");
#endif

    thread thread_g_succ = thread(build_g_succ);
    thread thread_g_pred = thread(build_g_pred);
    thread_g_succ.join();
    thread_g_pred.join();
    thread_idComma.join();
#ifdef TEST
    timerB.resetTimeWithTag("build graph");
#endif
}

int main(int argc, char *argv[])
{
#ifdef TEST
    string data_root = "test_data_fs/";
    string test_data = "/test_data.txt";
    string res_file = "/result.txt";
    string input_file, output_file;

    if (argv[1])
    {
        input_file = data_root + string(argv[1]) + test_data;
        output_file = data_root + string(argv[1]) + res_file;
    }
    else
    {
        input_file = data_root + dataset + test_data;
        output_file = data_root + dataset + res_file;
    }

    char testFile[100];
    char resultFile[100];
    strcpy(testFile, input_file.c_str());
    strcpy(resultFile, output_file.c_str());
#else
    char testFile[] = "/data/test_data.txt";
    char resultFile[] = "/projects/student/result.txt";
#endif

#ifdef MALLOC
    char *reserve_memory = (char *)malloc(RESERVE_MEMORY);
    new_memory_ptr = reserve_memory;
#endif

#ifdef TEST
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

    // 每个block里的id数至少要有128个
    batch_size = id_num >> 7;
    batch_size = batch_size > MIN_BATCH_SIZE ? batch_size : MIN_BATCH_SIZE;
    batch_num = ceil((double)id_num / batch_size);

    ui tid;
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
    // save_unistd_write(resultFile);

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
