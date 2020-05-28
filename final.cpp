// submit:
// 1. close #define TEST.
// 2. open //#define MMAP
// 3. open //#define NEON

#define TEST
// #define MMAP // 使用mmap函数
// #define NEON // 打开NEON特性的算子，开了反而会慢

// #include <bits/stdc++.h>
#include <algorithm>
#include <fcntl.h>
#include <iostream>
#include <mutex>
#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <vector>

#ifdef TEST
#include <unordered_map>
#else
#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/hash_policy.hpp>
#endif

using namespace std;

#ifdef TEST
// 0
// 1
// 2
// 3
string dataset = "1";
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

#define NUM_THREADS 8 // 线程数

#define MAX_NUM_EDGES 2500005 // 最大可接受边数 250w+5
#define MAX_NUM_IDS 2500005   // 最大可接受id数 250w+5

// #define MAX_NUM_EDGES 255 // 最大可接受边数 250w+5
// #define MAX_NUM_IDS 255   // 最大可接受id数 250w+5

#define TopK 100 // 只输出前TopK个结果

#define STOP_FLAG 0x80000000

typedef unsigned long long ull;
typedef unsigned int ui;

// 总id数，总边数
ui id_num = 0, edge_num = 0;

bool is_sparse;
ui input_u_ids[MAX_NUM_EDGES];
ui input_v_ids[MAX_NUM_EDGES];
ui input_weights[MAX_NUM_EDGES];
ui u_next[MAX_NUM_EDGES];
ui v_next[MAX_NUM_EDGES];
ui u_ids[MAX_NUM_EDGES];
ui v_ids[MAX_NUM_EDGES];
ui ids[MAX_NUM_EDGES];
ui g_succ[MAX_NUM_EDGES][2];
ui out_degree[MAX_NUM_IDS];     // 每个节点的出度
ui in_degree[MAX_NUM_IDS];      // 每个节点的入度
ui out_degree2[MAX_NUM_IDS];    // id重排序后每个节点的出度
ui in_degree2[MAX_NUM_IDS];     // id重排序后每个节点的入度
ui succ_begin_pos[MAX_NUM_IDS]; // 对于邻接表每个节点的起始index
ui pred_begin_pos[MAX_NUM_IDS];
ui topo_stack[MAX_NUM_IDS];        // 拓扑排序的栈
ui topo_pred_info[MAX_NUM_IDS][2]; // 存储前驱点信息  第一维：id 第二维：下一个兄弟index
ui topo_pred_num[MAX_NUM_IDS];     // 前驱数量
bool delete_recorder[MAX_NUM_IDS];
double global_score[MAX_NUM_IDS]; // 存储答案的数组

#ifdef TEST
unordered_map<ui, ui> id_map;
#else
__gnu_pbds::gp_hash_table<ui, ui> id_map; // map really id to regular id(0,1,2,...)
#endif

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
    ui i;
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

// TODO: 待优化
void add_double_neno1(int *dst, int *src1, int *src2, int count)
{
    int i;
    for (i = 0; i < count; i += 4)
    {
        int32x4_t in1, in2, out;
        in1 = vld1q_s64(src1);
        src1 += 8;
        in2 = vld1q_s64(src2);
        src2 += 8;
        out = vaddq_s64(in1, in2);
        vst1q_s64(dst, out);
        dst += 8;
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

struct Time_recorder
{
    vector<pair<string, int>> logs;

#ifdef _WIN32
    unsigned int start_time;
    unsigned int end_time;
#else
    struct timeval start_time;
    struct timeval end_time;
#endif

    void setTime()
    {
#ifdef _WIN32
        start_time = GetTickCount();
#else
        gettimeofday(&start_time, NULL);
#endif
    }

    int getElapsedTimeMS()
    {
#ifdef _WIN32
        end_time = GetTickCount();
        return int(end_time - start_time);
#else
        gettimeofday(&end_time, NULL);
        return int(1000 * (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec) / 1000);
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
        logs.emplace_back(make_pair(tag, getElapsedTimeMS()));
        setTime();
    }

    void printLogs()
    {
        for (auto x : logs)
            logTimeImpl(x.first, x.second);
    }
};
#endif

#ifdef TEST
Time_recorder timer_global, timer_local;
#endif

void input_fstream(char *testFile)
{
#ifdef TEST
    clock_t input_time = clock();
#endif

    FILE *file = fopen(testFile, "r");
    ui u, v;
    ui weight;
    while (fscanf(file, "%u,%u,%u\n", &u, &v, &weight) != EOF)
    {
        if (weight > 0)
        {
            input_u_ids[edge_num] = u;
            input_v_ids[edge_num] = v;
            input_weights[edge_num++] = weight;
        }
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

    ui *u_id_ptr = input_u_ids;
    ui *v_id_ptr = input_v_ids;
    ui *w_ptr = input_weights;
    ull num = 0;
    ui index = 0, sign = 0;
    char cur_char;
    char *p = buf;
    while (index++ < length)
    {
        cur_char = *(p++);
        if (cur_char >= '0')
        {
            // atoi
            num = (num << 1) + (num << 3) + cur_char - '0';
        }
        else if (cur_char != '\r')
        {
            if (cur_char != '\n')
            {
                if (sign)
                    v_id_ptr[edge_num] = num;
                else
                    u_id_ptr[edge_num] = num;
                sign ^= 1;
            }
            else
            {
                w_ptr[edge_num++] = num;
            }
            num = 0;
        }
    }
    close(fd);
    munmap(buf, length);
#ifdef TEST
    cout << "mmap input time " << (double)(clock() - input_time) / CLOCKS_PER_SEC << "s" << endl;
#endif
}
#endif

ui u_num, v_num;
void sort_u_ids_and_unique()
{
    memcpy(u_ids, input_u_ids, sizeof(input_u_ids));
    sort(u_ids, u_ids + edge_num);
    u_num = unique(u_ids, u_ids + edge_num) - u_ids;
}

void sort_v_ids_and_unique()
{
    memcpy(v_ids, input_v_ids, sizeof(input_v_ids));
    sort(v_ids, v_ids + edge_num);
    v_num = unique(v_ids, v_ids + edge_num) - v_ids;
}

void merge_uv_ids()
{
    ui i = 0, j = 0;

    while (i < u_num && j < v_num)
    {
        if (u_ids[i] < v_ids[j])
        {
            ids[id_num++] = u_ids[i++];
        }
        else if (u_ids[i] > v_ids[j])
        {
            ids[id_num++] = v_ids[j++];
        }
        else
        {
            ids[id_num++] = u_ids[i++];
            j++;
        }
    }
    while (i < u_num)
    {
        ids[id_num++] = u_ids[i++];
    }

    while (j < v_num)
    {
        ids[id_num++] = v_ids[j++];
    }
}

void build_g_succ()
{
    ui succ_iterator = 0, succ_index = 0, cur_id = 0;
    while (cur_id < id_num)
    {
        if (delete_recorder[cur_id] == false)
        {
            succ_iterator = succ_begin_pos[cur_id];
            succ_begin_pos[cur_id] = succ_index;
            while (succ_iterator != 0)
            {
                g_succ[succ_index][0] = input_v_ids[succ_iterator - 1];
                g_succ[succ_index++][1] = input_weights[succ_iterator - 1];
                succ_iterator = u_next[succ_iterator - 1];
            }
            // sort(g_succ + succ_begin_pos[cur_id], g_succ + succ_index);
        }
        ++cur_id;
    }
    succ_begin_pos[cur_id] = succ_index;
}

void pre_process()
{
    ui index = 0;
    thread u_thread = thread(sort_u_ids_and_unique);
    thread v_thread = thread(sort_v_ids_and_unique);
    u_thread.join();
    v_thread.join();
    merge_uv_ids();

    if (id_num * 2 < edge_num)
        is_sparse = false;
    else
        is_sparse = true;

    is_sparse = true; // 测试用，强制稀疏图
    // is_sparse = false; // 测试用，强制稠密图

    // 真实id与hashid的映射处理
    while (index < id_num - 3)
    {
        id_map[ids[index]] = index;
        id_map[ids[index + 1]] = index + 1;
        id_map[ids[index + 2]] = index + 2;
        id_map[ids[index + 3]] = index + 3;
        index += 4;
    }

    while (index < id_num)
    {
        id_map[ids[index]] = index;
        ++index;
    }

    ui u_hash_id, v_hash_id;
    index = 0;
    while (index < edge_num)
    {
        // 改为regular id
        u_hash_id = input_u_ids[index] = id_map[input_u_ids[index]];
        v_hash_id = input_v_ids[index] = id_map[input_v_ids[index]];

        // 链表串起来
        u_next[index] = succ_begin_pos[u_hash_id];
        succ_begin_pos[u_hash_id] = ++index;

        out_degree[u_hash_id]++;
        in_degree[v_hash_id]++;
    }

    if (is_sparse)
    {
        // topo sort
        // 去除所有入度为0且出度为1的点
        int topo_index = -1;
        for (index = 0; index < id_num; ++index)
        {
            if (in_degree[index] == 0 && out_degree[index] == 1)
            {
                topo_stack[++topo_index] = index;
                delete_recorder[index] = true;
            }
        }
        ui topo_pred_info_len = 0;
        while (topo_index >= 0)
        {
            u_hash_id = topo_stack[topo_index--];
            index = succ_begin_pos[u_hash_id];
            v_hash_id = input_v_ids[index - 1];

            topo_pred_info[topo_pred_info_len][0] = u_hash_id;
            topo_pred_info[topo_pred_info_len][1] = pred_begin_pos[v_hash_id];
            pred_begin_pos[v_hash_id] = ++topo_pred_info_len;
            topo_pred_num[v_hash_id] += topo_pred_num[u_hash_id] + 1;

            --out_degree[u_hash_id];
            --in_degree[v_hash_id];
            if (in_degree[v_hash_id] == 0 && out_degree[v_hash_id] == 1)
            {
                topo_stack[++topo_index] = v_hash_id;
                delete_recorder[v_hash_id] = true;
            }
        }
    }

    build_g_succ();
}

struct Pq_elem
{
    ui id;
    ui dis;

    inline Pq_elem() {}

    inline Pq_elem(ui id, ui dis) : id(id), dis(dis) {}

    inline bool operator<(const Pq_elem &nextNode) const
    {
        return dis > nextNode.dis;
    }
};

struct Dij_data
{
    ui dis;
    ui local_succ_begin_pos;
    ui sigma; // 起点到当前点最短路径的数量
};

struct BC_data
{
    ui pred_begin_pos;
    double delta; // sigma_st(index) / sigma_st
    double score; // 位置中心性
};

// 每个线程专属区域
struct ThreadMemorySparse
{
    ui dij_data[MAX_NUM_IDS][3]; // 0: dis 1: local_succ_begin_pos 2: sigma
    BC_data bc_data[MAX_NUM_IDS];

    // 小根堆
    priority_queue<Pq_elem> pq;
    ui id_stack[MAX_NUM_IDS]; // 出栈的节点会离s越来越近
    ui pred_info[MAX_NUM_EDGES][2];

} thread_memory_sparse[NUM_THREADS];

void dfs(ui cur_id, ui depth, ui num, ui tid)
{
    ui pred_id, pred_num;
    for (ui i = pred_begin_pos[cur_id]; i != 0; i = topo_pred_info[i - 1][1])
    {
        pred_id = topo_pred_info[i - 1][0];
        pred_num = topo_pred_num[pred_id] + 1;
        thread_memory_sparse[tid].bc_data[cur_id].score += pred_num * (num + depth);
        if (pred_num > 1)
        {
            dfs(pred_id, depth + 1, num, tid);
        }
    }
}

void dijkstra_priority_queue_sparse(ui s, ui tid)
{
    auto &dij_data = thread_memory_sparse[tid].dij_data;
    auto &bc_data = thread_memory_sparse[tid].bc_data;
    auto &pq = thread_memory_sparse[tid].pq;
    auto &id_stack = thread_memory_sparse[tid].id_stack;
    auto &pred_info = thread_memory_sparse[tid].pred_info;

    int id_stack_index = -1; // id_stack的指针
    ui cur_dis, update_dis;
    ui cur_id, next_id, pred_id, cur_id_sigma, multiple, pred_info_len = 0;
    ui cur_pos, end_pos;
    double coeff;

    dij_data[s][0] = 0;
    dij_data[s][2] = 1;

    pq.emplace(Pq_elem(s, 0));

    // 最多循环n次
    while (!pq.empty())
    {
        // 找到离s点最近的顶点
        cur_dis = pq.top().dis;
        cur_id = pq.top().id;
        // O(logn)
        pq.pop();

        if (cur_dis > dij_data[cur_id][0]) // dis可能经过松弛后变小了，原压入堆中的路径失去价值
            continue;

        id_stack[++id_stack_index] = cur_id;
        bc_data[cur_id].delta = 0;
        cur_pos = dij_data[cur_id][1];
        end_pos = cur_pos + out_degree[cur_id];
        // 遍历cur_id的后继 平均循环d次(平均出度)
        while (cur_pos < end_pos)
        {
            cur_id_sigma = dij_data[cur_id][2];
            update_dis = dij_data[cur_id][0] + g_succ[cur_pos][1]; // 11.05 ldr
            next_id = g_succ[cur_pos][0];

            if (update_dis == dij_data[next_id][0])
            {
                dij_data[next_id][2] += cur_id_sigma;
                pred_info[pred_info_len][0] = cur_id;
                pred_info[pred_info_len][1] = bc_data[next_id].pred_begin_pos;
                bc_data[next_id].pred_begin_pos = pred_info_len++; // 7.45 str
            }
            else if (update_dis < dij_data[next_id][0])
            {
                dij_data[next_id][0] = update_dis;
                // O(logn)
                pq.emplace(Pq_elem(next_id, dij_data[next_id][0]));

                dij_data[next_id][2] = cur_id_sigma;
                pred_info[pred_info_len][0] = cur_id;
                pred_info[pred_info_len][1] = bc_data[next_id].pred_begin_pos = STOP_FLAG;
                bc_data[next_id].pred_begin_pos = pred_info_len++; // 7.45 str
            }
            ++cur_pos;
        }
    }

    multiple = topo_pred_num[s] + 1;
    dfs(s, 0, id_stack_index, tid);

    // O(M)
    while (id_stack_index > 0)
    {
        cur_id = id_stack[id_stack_index--];
        cur_pos = bc_data[cur_id].pred_begin_pos;
        bc_data[cur_id].score += bc_data[cur_id].delta * multiple;
        dij_data[cur_id][0] = UINT32_MAX;
        coeff = (1 + bc_data[cur_id].delta) / dij_data[cur_id][2];
        // 遍历cur_id的前驱，且前驱必须在起始点到cur_id的最短路径上 平均循环d'次(平均入度)
        while ((cur_pos & STOP_FLAG) == 0)
        {
            pred_id = pred_info[cur_pos][0];
            cur_pos = pred_info[cur_pos][1];
            bc_data[pred_id].delta += dij_data[pred_id][2] * coeff;
        }
    }
    dij_data[s][0] = UINT32_MAX;
}

struct ThreadMemoryDense
{
    ui dis[MAX_NUM_IDS];
    // 小根堆
    priority_queue<Pq_elem> pq;
    ui id_stack[MAX_NUM_IDS];  // 出栈的节点会离s越来越近
    ui sigma[MAX_NUM_IDS];     // 起点到当前点最短路径的数量
    double delta[MAX_NUM_IDS]; // sigma_st(index) / sigma_st
    double score[MAX_NUM_IDS]; // 位置中心性
    ui local_pred_info[MAX_NUM_EDGES][2];
    ui local_pred_begin_pos[MAX_NUM_IDS];
} thread_memory_dense[NUM_THREADS];

void dijkstra_priority_queue_dense(ui s, ui tid)
{
    auto &dis = thread_memory_dense[tid].dis;
    auto &pq = thread_memory_dense[tid].pq;
    auto &id_stack = thread_memory_dense[tid].id_stack;
    auto &sigma = thread_memory_dense[tid].sigma;
    auto &delta = thread_memory_dense[tid].delta;
    auto &score = thread_memory_dense[tid].score;
    auto &local_pred_info = thread_memory_dense[tid].local_pred_info;
    auto &local_pred_begin_pos = thread_memory_dense[tid].local_pred_begin_pos;

    int id_stack_index = -1; // id_stack的指针
    ui cur_dis, update_dis;
    ui cur_id, next_id, pred_id, pred_info_len = 0;
    ui cur_pos, end_pos;
    double coeff;

    // 初始化 2n
    memset(dis, 0x3f, id_num << 3);
    dis[s] = 0;
    sigma[s] = 1;
    memset(delta, 0, id_num << 3);

    pq.emplace(Pq_elem(s, 0));

    // 最多循环n次
    while (!pq.empty())
    {
        // 找到离s点最近的顶点
        cur_dis = pq.top().dis;
        cur_id = pq.top().id;
        // O(logn)
        pq.pop();

        if (cur_dis > dis[cur_id]) //dis[cur_id]可能经过松弛后变小了，原压入堆中的路径失去价值
            continue;

        id_stack[++id_stack_index] = cur_id;
        cur_pos = succ_begin_pos[cur_id];
        end_pos = succ_begin_pos[cur_id + 1];
        // 遍历cur_id的后继 平均循环d次(平均出度)
        while (cur_pos < end_pos)
        {
            update_dis = dis[cur_id] + g_succ[cur_pos][1];
            next_id = g_succ[cur_pos][0];
            if (update_dis < dis[next_id])
            {
                dis[next_id] = update_dis;
                sigma[next_id] = 0;
                // O(logn)
                pq.emplace(Pq_elem(next_id, dis[next_id]));
                local_pred_begin_pos[next_id] = 0x80000000;
            }
            if (update_dis == dis[next_id])
            {
                sigma[next_id] += sigma[cur_id];
                local_pred_info[pred_info_len][0] = cur_id;
                local_pred_info[pred_info_len][1] = local_pred_begin_pos[next_id];
                local_pred_begin_pos[next_id] = pred_info_len++;
            }
            ++cur_pos;
        }
    }

    // O(M)
    while (id_stack_index > 0)
    {
        cur_id = id_stack[id_stack_index--];
        cur_pos = local_pred_begin_pos[cur_id];
        coeff = (1 + delta[cur_id]) / sigma[cur_id];
        // 遍历cur_id的前驱，且前驱必须在起始点到cur_id的最短路径上 平均循环d'次(平均入度)
        while ((cur_pos & 0x80000000) == 0)
        {
            pred_id = local_pred_info[cur_pos][0];
            cur_pos = local_pred_info[cur_pos][1];
            delta[pred_id] += sigma[pred_id] * coeff;
        }
        score[cur_id] += delta[cur_id];
    }
}

mutex id_lock;
ui cur_id;

void thread_process(ui tid)
{
#ifdef TEST
    Time_recorder timer;
    timer.setTime();
#endif
    auto &dij_data = thread_memory_sparse[tid].dij_data;
    // 初始化
    for (ui i = 0; i < id_num; ++i)
    {
        dij_data[i][0] = UINT32_MAX;
        dij_data[i][1] = succ_begin_pos[i];
    }

    ui s_id;
    while (true)
    {
        id_lock.lock();
        if (cur_id >= id_num)
        {
            id_lock.unlock();
            break;
        }
        else
        {
#ifdef TEST
            if (cur_id % 10000 == 0)
                printf("[%0.1f%%] ~ %u/%u\n", 100.0 * cur_id / id_num, cur_id, id_num);
#endif
            while (delete_recorder[cur_id] && cur_id < id_num)
                cur_id++;

            if (cur_id < id_num)
                s_id = cur_id++;
            else
            {
                id_lock.unlock();
                break;
            }

            id_lock.unlock();
            if (is_sparse)
                dijkstra_priority_queue_sparse(s_id, tid);
            else
                dijkstra_priority_queue_dense(s_id, tid);
        }
    }

#ifdef TEST
    timer.logTime("[Thread " + to_string(tid) + "].");
#endif
}

struct Res_pq_elem
{
    ui id;
    double score;

    Res_pq_elem() {}

    Res_pq_elem(ui id, double score) : id(id), score(score) {}

    bool operator<(const Res_pq_elem &nextNode) const
    {
        if (abs(score - nextNode.score) > 0.0001)
            return score > nextNode.score;
        else
            return id < nextNode.id;
    }
};

void save_fwrite(char *resultFile)
{
    if (is_sparse)
    {
        for (ui thread_index = 0; thread_index < NUM_THREADS; ++thread_index)
        {
            for (ui id_index = 0; id_index < id_num; ++id_index)
            {
                global_score[id_index] += thread_memory_sparse[thread_index].bc_data[id_index].score;
            }
        }
    }
    else
    {
        for (ui thread_index = 0; thread_index < NUM_THREADS; ++thread_index)
        {
            for (ui id_index = 0; id_index < id_num; ++id_index)
            {
                global_score[id_index] += thread_memory_dense[thread_index].score[id_index];
            }
        }
    }

    int index = 0;
    priority_queue<Res_pq_elem> pq;
    while (index < TopK)
    {
        pq.emplace(Res_pq_elem(index, global_score[index]));
        ++index;
    }

    while (index < id_num)
    {
        if (global_score[index] > pq.top().score && abs(global_score[index] - pq.top().score) > 0.0001)
        {
            pq.pop();
            pq.emplace(Res_pq_elem(index, global_score[index]));
        }
        ++index;
    }

    FILE *fp;

    fp = fopen(resultFile, "wb");

    Res_pq_elem res_arr[100];
    index = 99;
    while (index >= 0)
    {
        res_arr[index] = pq.top();
        pq.pop();
        index--;
    }
    index = 0;
    while (index < 100)
    {
        fprintf(fp, "%u,%.3lf\n", ids[res_arr[index].id], res_arr[index].score);
        ++index;
    }

    fclose(fp);
}

int main(int argc, char *argv[])
{
#ifdef TEST
    string data_root = "test_data_js/";
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

#ifdef TEST
    timer_global.setTime();
    timer_local.setTime();
#endif

#ifdef MMAP
    input_mmap(testFile);
#else
    input_fstream(testFile);
#endif
#ifdef TEST
    timer_local.resetTimeWithTag("Read Input File");
#endif

    pre_process();
#ifdef TEST
    timer_local.resetTimeWithTag("Pre Process Data");
#endif

    thread threads[NUM_THREADS];
    ui i = 0;
    while (i < NUM_THREADS)
    {
        threads[i] = thread(thread_process, i);
        ++i;
    }

    for (auto &thread : threads)
    {
        thread.join();
    }

#ifdef TEST
    timer_local.resetTimeWithTag("Solve Results");
#endif

    save_fwrite(resultFile);
#ifdef TEST
    timer_local.resetTimeWithTag("Output");
#endif

#ifdef TEST
    timer_local.printLogs();
    timer_global.logTime("Whole Process");
#endif

    return 0;
}