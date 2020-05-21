// submit:
// 1. close #define TEST.
// 2. open //#define MMAP
// 3. open //#define NEON

#define TEST
// #define MMAP // 使用mmap函数
// #define NEON // 打开NEON特性的算子，开了反而会慢

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

#define MAX_NUM_EDGES 2500005 // 最大可接受边数 250w+5 确定够用
#define MAX_NUM_IDS 2500005   // 最大可接受id数 250w+5 确定够用

typedef unsigned long long ull;
typedef unsigned int ui;

// 总id数，总边数
ui id_num = 0, edge_num = 0;

struct Node
{
    ui dst_id;
    ull weight;

    Node() {}

    Node(ui next_id, ull weight) : dst_id(next_id), weight(weight) {}

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
ui u_ids[MAX_NUM_EDGES];
ui v_ids[MAX_NUM_EDGES];
ui ids[MAX_NUM_EDGES];
ui succ_begin_pos[MAX_NUM_IDS]; // 对于邻接表每个节点的起始index
ui out_degree[MAX_NUM_IDS];     // 每个节点的出度
ui in_degree[MAX_NUM_IDS];      // 每个节点的入度
Node g_succ[MAX_NUM_EDGES];
__gnu_pbds::gp_hash_table<ui, ui> id_map; // map really id to regular id(0,1,2,...)

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

// TODO: 待优化
void add_float_neno1(int *dst, int *src1, int *src2, int count)
{
    int i;
    for (i = 0; i < count; i += 4)
    {
        int32x4_t in1, in2, out;
        in1 = vld1q_s32(src1);
        src1 += 4;
        in2 = vld1q_s32(src2);
        src2 += 4;
        out = vaddq_s32(in1, in2);
        vst1q_s32(dst, out);
        dst += 4;
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
Time_recorder timerA, timerB;
#endif

void input_fstream(char *testFile)
{
#ifdef TEST
    clock_t input_time = clock();
#endif

    FILE *file = fopen(testFile, "r");
    ui u, v;
    ull weight;
    while (fscanf(file, "%d,%d,%llu\n", &u, &v, &weight) != EOF)
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
    register ui index = 0, ui id_index = 0;
    register byte sign = 0;
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
    memcpy(input_u_ids, u_ids, sizeof(input_u_ids));
    sort(u_ids, u_ids + edge_num);
    u_num = unique(u_ids, u_ids + edge_num) - u_ids;
}

void sort_v_ids_and_unique()
{
    memcpy(input_v_ids, v_ids, sizeof(input_v_ids));
    sort(v_ids, v_ids + edge_num);
    v_num = unique(v_ids, v_ids + edge_num) - v_ids;
}

// 19630345 118ms
void merge_uv_ids()
{
    register int i = 0, j = 0, k = 0;

    while (i < u_num && j < v_num)
    {
        if (u_ids[i] < v_ids[j])
        {
            ids[k++] = u_ids[i++];
        }
        else if (u_ids[i] > v_ids[j])
        {
            ids[k++] = v_ids[j++];
        }
        else
        {
            ids[k++] = u_ids[i++];
            j++;
        }
    }
    while (i < u_num)
    {
        ids[k++] = u_ids[i++];
    }

    while (j < v_num)
    {
        ids[k++] = v_ids[j++];
    }
}

void pre_process()
{
    register ui index = 0;
    thread u_thread(sort_u_ids_and_unique);
    thread v_thread(sort_v_ids_and_unique);
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
        succ_begin_pos[u_hash_id] = ++index;

        // 出度和入度
        out_degree[u_hash_id]++;
        in_degree[v_hash_id]++;
    }

    ui succ_iterator = 0, succ_index = 0, cur_id = 0;
    while (cur_id < id_num)
    {
        if (out_degree[cur_id] && in_degree[cur_id])
        {
            succ_iterator = succ_begin_pos[cur_id];
            succ_begin_pos[cur_id] = succ_index;
            while (succ_iterator != 0)
            {
                g_succ[succ_index++] = Node(input_v_ids[succ_iterator - 1], input_weights[succ_iterator - 1]);
                succ_iterator = u_next[succ_iterator - 1];
            }
            sort(g_succ + succ_begin_pos[cur_id], g_succ + succ_begin_pos[cur_id] + out_degree[cur_id]);
            g_succ[succ_index++] = {UINT32_MAX, UINT32_MAX};
        }
        ++cur_id;
    }
}

struct PathNode
{
    ui id;
    ui next_bro;
    ui next_child;
};

// 每个线程专属区域
struct ThreadMemory
{
    ull distance[MAX_NUM_IDS];
    bool visited[MAX_NUM_IDS];
    ui begin_pos[MAX_NUM_IDS];
    PathNode path[MAX_NUM_IDS];
    float score[MAX_NUM_IDS];
} thread_memory[NUM_THREADS];

mutex id_lock;
ui next_id;

void dijkstra_simple(ui s, ui tid)
{
    auto &distance = thread_memory[tid].distance;
    auto &visited = thread_memory[tid].visited;
    auto &begin_pos = thread_memory[tid].begin_pos;
    auto &path = thread_memory[tid].path;
    auto &score = thread_memory[tid].score;

    // 初始化
    memset(distance, UINT64_MAX, id_num);
    for (ui i = succ_begin_pos[s]; i < succ_begin_pos[s] + out_degree[s]; ++i)
    {
        distance[g_succ[i].dst_id] = g_succ[i].weight;
    }
    memset(visited, false, id_num);
    visited[s] = true;
    memset(begin_pos, 0, id_num);

    ull min_distance = UINT64_MAX;
    ui selected_id;
    for (ui i = 0; i < id_num; ++i)
    {
        // 找到离s点最近的顶点
        for (ui j = 0; j < id_num; ++j)
        {
            if (visited[j] == false && distance[j] < min_distance)
            {
                min_distance = distance[j];
                selected_id = j;
            }
        }
        visited[selected_id] = true;
        for (ui j = succ_begin_pos[selected_id]; j < succ_begin_pos[selected_id] + out_degree[selected_id]; ++j)
        {
            if (distance[selected_id] + g_succ[j].weight < distance[g_succ[j].dst_id])
            {
                distance[g_succ[j].dst_id] = distance[selected_id] + g_succ[j].weight;
            }
        }
        
    }
}

void thread_process(ui tid)
{
#ifdef TEST
    Time_recorder timer;
    timer.setTime();
#endif

    ui cur_id;
    while (true)
    {
        id_lock.lock();
        if (next_id >= id_num)
        {
            id_lock.unlock();
            break;
        }
        else
        {
            while (out_degree[next_id] == 0)
            {
                next_id++;
            }
            cur_id = next_id++;
            id_lock.unlock();
            dijkstra_simple(cur_id, tid);
        }
    }

#ifdef TEST
    timer.logTime("[Thread " + to_string(tid) + "].");
#endif
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

    thread threads[NUM_THREADS];
    ui i = 0;
    while (i < NUM_THREADS)
    {
        threads[i] = thread(thread_process, i++);
    }

    for (auto &thread : threads)
    {
        thread.join();
    }

    return 0;
}