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
string dataset = "2";
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

#define MAX_NUM_EDGES 2500005 // 最大可接受边数 250w+5 确定够用
#define MAX_NUM_IDS 2500005   // 最大可接受id数 250w+5 确定够用

#define TopN 100 // 只输出前TopN个结果

typedef unsigned long long ull;
typedef unsigned int ui;

// 总id数，总边数
ui id_num = 0, edge_num = 0;

struct Node
{
    ui dst_id;
    ull weight;

    Node() {}

    Node(ui cur_id, ull weight) : dst_id(cur_id), weight(weight) {}

    // 依据next_id进行排序
    bool operator<(const Node &nextNode) const
    {
        return dst_id < nextNode.dst_id;
    }
};

ui input_u_ids[MAX_NUM_EDGES];
ui input_v_ids[MAX_NUM_EDGES];
ull input_weights[MAX_NUM_EDGES];
ui u_next[MAX_NUM_EDGES];
ui v_next[MAX_NUM_EDGES];
ui u_ids[MAX_NUM_EDGES];
ui v_ids[MAX_NUM_EDGES];
ui ids[MAX_NUM_EDGES];
Node g_succ[MAX_NUM_EDGES];
Node g_pred[MAX_NUM_EDGES];
ui out_degree[MAX_NUM_IDS];       // 每个节点的出度
ui in_degree[MAX_NUM_IDS];        // 每个节点的入度
ui succ_begin_pos[MAX_NUM_IDS];   // 对于邻接表每个节点的起始index
ui pred_begin_pos[MAX_NUM_IDS];   // 对于逆邻接表每个节点的起始index
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
    ull weight;
    while (fscanf(file, "%u,%u,%llu\n", &u, &v, &weight) != EOF)
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
    ui *v_id_ptr = input_u_ids;
    ull *w_ptr = input_weights;
    ull num = 0;
    ui index = 0, ui id_index = 0;
    byte sign = 0;
    char cur_char;
    char *p = buf;
    ui input_num = 0;
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
                if (sign == 0)
                    u_id_ptr[edge_num] = num;
                else
                    v_id_ptr[edge_num] = num;
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
        succ_iterator = succ_begin_pos[cur_id];
        succ_begin_pos[cur_id] = succ_index;
        while (succ_iterator != 0)
        {
            g_succ[succ_index++] = Node(input_v_ids[succ_iterator - 1], input_weights[succ_iterator - 1]);
            succ_iterator = u_next[succ_iterator - 1];
        }
        ++cur_id;
    }
}

void build_g_pred()
{
    ui pred_iterator = 0, pred_index = 0, cur_id = 0;
    while (cur_id < id_num)
    {
        pred_iterator = pred_begin_pos[cur_id];
        pred_begin_pos[cur_id] = pred_index;
        while (pred_iterator != 0)
        {
            g_pred[pred_index++] = Node(input_u_ids[pred_iterator - 1], input_weights[pred_iterator - 1]);
            pred_iterator = v_next[pred_iterator - 1];
        }
        ++cur_id;
    }
}

void pre_process()
{
    ui index = 0;
    thread u_thread = thread(sort_u_ids_and_unique);
    thread v_thread = thread(sort_v_ids_and_unique);
    u_thread.join();
    v_thread.join();
    merge_uv_ids();

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
        succ_begin_pos[u_hash_id] = index + 1;
        v_next[index] = pred_begin_pos[v_hash_id];
        pred_begin_pos[v_hash_id] = ++index;

        // 出度和入度
        out_degree[u_hash_id]++;
        in_degree[v_hash_id]++;
    }

    thread thread_g_succ = thread(build_g_succ);
    thread thread_g_pred = thread(build_g_pred);
    thread_g_succ.join();
    thread_g_pred.join();
}

struct Pq_elem
{
    ui id;
    ull dis;

    Pq_elem() {}

    Pq_elem(ui id, ull dis) : id(id), dis(dis) {}

    bool operator<(const Pq_elem &nextNode) const
    {
        return dis > nextNode.dis;
    }
};

// 每个线程专属区域
struct ThreadMemory
{
    ull dis[MAX_NUM_IDS];
    // 小根堆
    priority_queue<Pq_elem> pq;
    ui id_stack[MAX_NUM_IDS];  // 出栈的节点会离s越来越近
    ui sigma[MAX_NUM_IDS];     // 起点到当前点最短路径的数量
    double delta[MAX_NUM_IDS]; // sigma_st(index) / sigma_st
    double score[MAX_NUM_IDS]; // 位置中心性
} thread_memory[NUM_THREADS];

void dijkstra_priority_queue(ui s, ui tid)
{
    if (out_degree[s] == 0)
        return;
    auto &dis = thread_memory[tid].dis;
    auto &pq = thread_memory[tid].pq;
    auto &id_stack = thread_memory[tid].id_stack;
    auto &sigma = thread_memory[tid].sigma;
    auto &delta = thread_memory[tid].delta;
    auto &score = thread_memory[tid].score;

    // 初始化
    // memset(dis, UINT64_MAX, id_num);
    fill(dis, dis + id_num, UINT64_MAX);
    dis[s] = 0;
    memset(sigma, 0, id_num * sizeof(double));
    sigma[s] = 1;
    memset(delta, 0, id_num * sizeof(double));
    pq.push(Pq_elem(s, 0));

    int id_stack_index = -1; // id_stack的指针
    ull cur_dis;
    ui cur_id;
    while (!pq.empty())
    {
        // 找到离s点最近的顶点
        cur_dis = pq.top().dis;
        cur_id = pq.top().id;
        pq.pop();

        if (cur_dis > dis[cur_id]) //dis[cur_id]可能经过松弛后变小了，原压入堆中的路径失去价值
            continue;

        id_stack[++id_stack_index] = cur_id;

        // 遍历cur_id的后继
        for (ui j = succ_begin_pos[cur_id]; j < succ_begin_pos[cur_id] + out_degree[cur_id]; ++j)
        {
            if (dis[cur_id] + g_succ[j].weight < dis[g_succ[j].dst_id])
            {
                dis[g_succ[j].dst_id] = dis[cur_id] + g_succ[j].weight;
                sigma[g_succ[j].dst_id] = sigma[cur_id];
                pq.push(Pq_elem(g_succ[j].dst_id, dis[g_succ[j].dst_id]));
            }
            else if (dis[cur_id] + g_succ[j].weight == dis[g_succ[j].dst_id])
            {
                sigma[g_succ[j].dst_id] += sigma[cur_id];
            }
        }
    }

    ui pred_id = 0;
    while (id_stack_index > 0)
    {
        cur_id = id_stack[id_stack_index--];
        for (ui j = pred_begin_pos[cur_id]; j < pred_begin_pos[cur_id] + in_degree[cur_id]; ++j)
        {
            pred_id = g_pred[j].dst_id;
            if (dis[pred_id] + g_pred[j].weight == dis[cur_id])
            {
                delta[pred_id] += (sigma[pred_id] * 1.0 / sigma[cur_id]) * (1 + delta[cur_id]);
            }
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
            s_id = cur_id++;
            id_lock.unlock();
            dijkstra_priority_queue(s_id, tid);
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

void print_rec(FILE *fp, priority_queue<Res_pq_elem> &pq)
{
    if (pq.empty())
        return;

    Res_pq_elem res = pq.top();
    pq.pop();
    print_rec(fp, pq);
    fprintf(fp, "%u,%.3lf\n", res.id, res.score);
}

void save_fwrite(char *resultFile)
{
    memcpy(global_score, thread_memory[0].score, id_num * sizeof(double));
    int index = 1;
    while (index < NUM_THREADS)
    {
        /* code */ // neon 相加
        ui score_index = 0;
        while (score_index < id_num)
        {
            global_score[score_index] += thread_memory[index].score[score_index];
            ++score_index;
        }

        ++index;
    }

    index = 0;
    priority_queue<Res_pq_elem> pq;
    while (index < TopN)
    {
        pq.push(Res_pq_elem(index, global_score[index]));
        ++index;
    }

    while (index < id_num)
    {
        if (global_score[index] > pq.top().score && abs(global_score[index] - pq.top().score) > 0.0001)
        {
            pq.pop();
            pq.push(Res_pq_elem(index, global_score[index]));
        }
        ++index;
    }

    FILE *fp;

    fp = fopen(resultFile, "wb");
    // print_rec(fp, pq);
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

// TODO: 修改变量名
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