// submit:
// 1. delete #define TEST.
// 2. open //#define MMAP
// 3. change NUM_LEN7_RESULT to 3000000

#define TEST
#define MMAP
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

#define MAX_NUM_EDGES 280000
#define MAX_NUM_IDS 262144
#define MAX_OUT_DEGREE 51

#define NUM_LEN3_RESULT 500000
#define NUM_LEN4_RESULT 500000
#define NUM_LEN5_RESULT 1000000
#define NUM_LEN6_RESULT 2000000
#define NUM_LEN7_RESULT 3200000

#define MAX_INT 2147483647

#define NUM_THREADS 4

using namespace std;

unsigned int id_num = 0, edge_num = 0, res_count = 0;

float seg_ratio[] = {0, 0.068, 0.148, 0.284, 1};

unsigned int u_ids[MAX_NUM_EDGES];
unsigned int v_ids[MAX_NUM_EDGES];
unsigned int ids[MAX_NUM_EDGES * 2];

unsigned int g_succ[MAX_NUM_IDS][MAX_OUT_DEGREE];
unsigned int out_degree[MAX_NUM_IDS];

unsigned int path[NUM_THREADS][7];
bool visited[NUM_THREADS][MAX_NUM_IDS];

vector<unordered_map<unsigned int, vector<unsigned int>>> two_uj;
bool reachable[NUM_THREADS][MAX_NUM_IDS];
unsigned int currentJs[NUM_THREADS][500];
unsigned int num_currentJs[NUM_THREADS];

unsigned int res3[NUM_THREADS * NUM_LEN3_RESULT * 3];
unsigned int res4[NUM_THREADS * NUM_LEN4_RESULT * 4];
unsigned int res5[NUM_THREADS * NUM_LEN5_RESULT * 5];
unsigned int res6[NUM_THREADS * NUM_LEN6_RESULT * 6];
unsigned int res7[NUM_THREADS * NUM_LEN7_RESULT * 7];

unsigned int num_res[NUM_THREADS][5];
unsigned int *results[] = {res3, res4, res5, res6, res7};

void input_fstream(const string &testFile)
{
#ifdef TEST
    clock_t input_time = clock();
#endif

    FILE *file = fopen(testFile.c_str(), "r");
    unsigned int money;
    while (fscanf(file, "%d,%d,%d\n", u_ids + edge_num, v_ids + edge_num, &money) != EOF)
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

void input_mmap(const string &testFile)
{
#ifdef TEST
    clock_t input_time = clock();
#endif
    int fd = open(testFile.c_str(), O_RDONLY);
    //get the size of the document
    long length = lseek(fd, 0, SEEK_END);

    //mmap
    char *buf = (char *)mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, 0);

    register int id_pos = -1;
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
            switch (sign)
            {
            case 1: //读取 id1
                u_ids[edge_num] = temp;
                ids[++id_pos] = temp;
                temp = 0;
                break;
            case 2: //读取 id2
                v_ids[edge_num++] = temp;
                ids[++id_pos] = temp;
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

unsigned int digits10_length(unsigned int v)
{
    if (v < 10)
        return 1;
    if (v < 100)
        return 2;
    if (v < 1000)
        return 3;
    if (v < 1e12) // 10^12
    {
        if (v < 1e8) // 10^8
        {
            if (v < 1e6)
            {
                if (v < 1e4)
                {
                    return 4;
                }

                return 5 + (v >= 1e5);
            }

            return 7 + (v >= 1e7);
        }
        if (v < 1e10)
        {
            return 9 + (v >= 1e9);
        }
        return 11 + (v >= 1e11);
    }
    return 12;
}

unsigned int uint2ascii(unsigned int value, char *dst, unsigned int offset)
{
    static const char digits[] =
        "0001020304050607080910111213141516171819"
        "2021222324252627282930313233343536373839"
        "4041424344454647484950515253545556575859"
        "6061626364656667686970717273747576777879"
        "8081828384858687888990919293949596979899";

    const unsigned int length = digits10_length(value);
    unsigned int next = length - 1;

    while (value >= 100)
    {
        const unsigned int i = (value % 100) * 2;
        value /= 100;
        dst[offset + next - 1] = digits[i];
        dst[offset + next] = digits[i + 1];
        next -= 2;
    }

    if (value < 10)
    {
        dst[offset + next] = '0' + value;
    }
    else
    {
        unsigned int i = value * 2;
        dst[offset + next - 1] = digits[i];
        dst[offset + next] = digits[i + 1];
    }
    return length;
}

void save_fwrite(const string &resultFile, vector<string> &idsComma, vector<string> &idsLF)
{

    for (int thread_id = 0; thread_id < NUM_THREADS; ++thread_id)
    {
        for (int len = 0; len < 5; len++)
        {
            res_count += num_res[thread_id][len];
        }
    }

#ifdef TEST
    clock_t write_time = clock();
    printf("Total Loops %d\n", res_count);
#endif

    FILE *fp = fopen(resultFile.c_str(), "w");
    char *str_res = (char *)malloc(512 * 1024 * 1024); // 512M space

    register unsigned int str_len = 0;

    str_len = sprintf(str_res, "%d\n", res_count);

    // str_len += uint2ascii(res_count, str_res, str_len);
    // str_res[str_len++] = '\n';

    register unsigned int thread_offset, line_offset;

    for (unsigned int res_len = 3; res_len <= 7; ++res_len)
    {
        for (int tid = 0; tid < NUM_THREADS; ++tid)
        {
            switch (res_len)
            {
            case 3:
                thread_offset = NUM_LEN3_RESULT * 3;
                break;
            case 4:
                thread_offset = NUM_LEN4_RESULT * 4;
                break;
            case 5:
                thread_offset = NUM_LEN5_RESULT * 5;
                break;
            case 6:
                thread_offset = NUM_LEN6_RESULT * 6;
                break;
            case 7:
                thread_offset = NUM_LEN7_RESULT * 7;
                break;
            default:
                break;
            }

            for (unsigned int line = 0; line < num_res[tid][res_len - 3]; ++line)
            {
                line_offset = line * res_len;
                for (unsigned int id = 0; id < res_len - 1; ++id)
                {
                    const char *res = idsComma[results[res_len - 3][tid * thread_offset + line_offset + id]].c_str();
                    memcpy(str_res + str_len, res, strlen(res));
                    str_len += strlen(res);
                    // str_len += uint2ascii(ids[results[res_len - 3][tid * thread_offset + line_offset + id]], str_res, str_len);
                    // str_res[str_len++] = ',';
                }
                const char *res = idsLF[results[res_len - 3][tid * thread_offset + line_offset + res_len - 1]].c_str();
                memcpy(str_res + str_len, res, strlen(res));
                str_len += strlen(res);

                // str_len += uint2ascii(ids[results[res_len - 3][tid * thread_offset + line_offset + res_len - 1]], str_res, str_len);
                // str_res[str_len++] = '\n';
            }
        }
    }

    str_res[str_len] = '\0';
    fwrite(str_res, sizeof(char), str_len, fp);
    fclose(fp);

#ifdef TEST
    cout << "fwrite output time " << (double)(clock() - write_time) / CLOCKS_PER_SEC << "s" << endl;
#endif
}

#ifdef MMAP
// memcpy version
void save_mmap(const string &resultFile, vector<string> &idsComma, vector<string> &idsLF)
{
    for (int thread_id = 0; thread_id < NUM_THREADS; ++thread_id)
    {
        for (int len = 0; len < 5; len++)
        {
            res_count += num_res[thread_id][len];
        }
    }
#ifdef TEST
    clock_t write_time = clock();
    printf("Total Loops %d\n", res_count);
#endif
    //copy the results into str_res and get the length of results
    char *str_res = (char *)malloc(512 * 1024 * 1024); // 512M space

    char buf[1024];
    int idx = sprintf(buf, "%d\n", res_count);
    buf[idx] = '\0';
    memcpy(str_res, buf, idx);
    int str_len = idx;
    
    // register unsigned int str_len = 0;
    // str_len += uint2ascii(res_count, str_res, str_len);
    // str_res[str_len++] = '\n';

    register unsigned int thread_offset, line_offset;

    for (int res_len = 3; res_len <= 7; ++res_len)
    {
        for (int tid = 0; tid < NUM_THREADS; ++tid)
        {
            switch (res_len)
            {
            case 3:
                thread_offset = NUM_LEN3_RESULT * 3;
                break;
            case 4:
                thread_offset = NUM_LEN4_RESULT * 4;
                break;
            case 5:
                thread_offset = NUM_LEN5_RESULT * 5;
                break;
            case 6:
                thread_offset = NUM_LEN6_RESULT * 6;
                break;
            case 7:
                thread_offset = NUM_LEN7_RESULT * 7;
                break;
            default:
                break;
            }
            for (int line = 0; line < num_res[tid][res_len - 3]; ++line)
            {
                line_offset = line * res_len;
                for (int id = 0; id < res_len - 1; ++id)
                {
                    const char *res = idsComma[results[res_len - 3][tid * thread_offset + line_offset + id]].c_str();
                    memcpy(str_res + str_len, res, strlen(res));
                    str_len += strlen(res);

                    // str_len += uint2ascii(ids[results[res_len - 3][tid * thread_offset + line_offset + id]], str_res, str_len);
                    // str_res[str_len++] = ',';
                }
                const char *res = idsLF[results[res_len - 3][tid * thread_offset + line_offset + res_len - 1]].c_str();
                memcpy(str_res + str_len, res, strlen(res));
                str_len += strlen(res);

                // str_len += uint2ascii(ids[results[res_len - 3][tid * thread_offset + line_offset + res_len - 1]], str_res, str_len);
                // str_res[str_len++] = '\n';
            }
        }
    }

    //create and open the resultFile
    int fd = open(resultFile.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);

    //allocate space
    int f_ret = ftruncate(fd, str_len);
    char *const address = (char *)mmap(NULL, str_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    memcpy(address, str_res, str_len);
    int mun_ret = munmap(address, str_len);
    close(fd);

#ifdef TEST
    cout << "mmap output time " << (double)(clock() - write_time) / CLOCKS_PER_SEC << "s" << endl;
#endif
}
#endif

// recursion version
void dfs_rec(unsigned int start_id, unsigned int cur_id, int depth, int tid)
{
    unsigned int begin = 0;
    while (start_id >= g_succ[cur_id][begin])
        ++begin;

    if (begin == out_degree[cur_id])
        return;

    visited[tid][cur_id] = true;
    path[tid][depth] = cur_id;

    for (; begin < out_degree[cur_id]; ++begin)
    {
        unsigned int next_id = g_succ[cur_id][begin];
        if (!visited[tid][next_id])
        {
            if (reachable[tid][next_id])
            {
                for (unsigned int &k : two_uj[start_id][next_id])
                {
                    if (!visited[tid][k])
                    {
                        path[tid][depth + 1] = next_id;
                        path[tid][depth + 2] = k;

                        unsigned int thread_offset = 0;
                        switch (depth)
                        {
                        case 0:
                            thread_offset = NUM_LEN3_RESULT * 3;
                            break;
                        case 1:
                            thread_offset = NUM_LEN4_RESULT * 4;
                            break;
                        case 2:
                            thread_offset = NUM_LEN5_RESULT * 5;
                            break;
                        case 3:
                            thread_offset = NUM_LEN6_RESULT * 6;
                            break;
                        case 4:
                            thread_offset = NUM_LEN7_RESULT * 7;
                            break;
                        default:
                            break;
                        }

                        memcpy(results[depth] + tid * thread_offset + num_res[tid][depth] * (depth + 3), path[tid], 4 * depth + 12);
                        ++num_res[tid][depth];
                    }
                }
            }

            if (depth < 4)
                dfs_rec(start_id, next_id, depth + 1, tid);
        }
    }

    visited[tid][cur_id] = false;
}

// iteration version
void dfs_ite(unsigned int start_id, unsigned int cur_id, int depth, int tid)
{
    visited[tid][cur_id] = true;
    path[tid][depth] = cur_id;

    unsigned int begin_pos[5] = {0};
    while (start_id >= g_succ[cur_id][begin_pos[0]])
        ++begin_pos[0];

    if (begin_pos[0] == out_degree[cur_id])
        return;

    unsigned int next_id;
    unsigned int *stack[5];
    stack[0] = g_succ[cur_id];

    while (depth >= 0)
    {
        // no valid succ
        if (begin_pos[depth] == out_degree[cur_id])
        {
            visited[tid][cur_id] = false;
            depth--;
            cur_id = depth >= 0 ? path[tid][depth] : 0;
        }
        else
        {
            next_id = stack[depth][begin_pos[depth]++];
            if (!visited[tid][next_id])
            {

                // find a circuit
                if (reachable[tid][next_id])
                {
                    for (unsigned int &k : two_uj[start_id][next_id])
                    {
                        if (!visited[tid][k])
                        {
                            path[tid][depth + 1] = next_id;
                            path[tid][depth + 2] = k;

                            unsigned int thread_offset = 0;
                            switch (depth)
                            {
                            case 0:
                                thread_offset = NUM_LEN3_RESULT * 3;
                                break;
                            case 1:
                                thread_offset = NUM_LEN4_RESULT * 4;
                                break;
                            case 2:
                                thread_offset = NUM_LEN5_RESULT * 5;
                                break;
                            case 3:
                                thread_offset = NUM_LEN6_RESULT * 6;
                                break;
                            case 4:
                                thread_offset = NUM_LEN7_RESULT * 7;
                                break;
                            default:
                                break;
                            }

                            memcpy(results[depth] + tid * thread_offset + num_res[tid][depth] * (depth + 3), path[tid], 4 * depth + 12);
                            ++num_res[tid][depth];
                        }
                    }
                }

                if (depth < 4)
                {
                    stack[++depth] = g_succ[next_id];
                    cur_id = next_id;
                    visited[tid][cur_id] = true;
                    path[tid][depth] = cur_id;
                    begin_pos[depth] = 0;
                    while (start_id >= stack[depth][begin_pos[depth]])
                        ++begin_pos[depth];
                }
            }
        }
    }
}

unsigned int binary_search(unsigned int target)
{
    register unsigned int left = 0;
    register unsigned int right = id_num - 1;
    register unsigned int mid;

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
    unsigned int slow = 0, fast = 1;
    unsigned int double_edge_num = edge_num << 1;
    while (fast < double_edge_num)
    {
        if (ids[fast] != ids[slow])
            ids[++slow] = ids[fast];
        fast++;
    }
    id_num = slow + 1;
}

// 这里定义子线程函数, 如果处理不同分段的数据不一样,那就写多个函数
// 函数的参数传入的时候需要是 void * 类型, 一般就是数据的指针转成  无类型指针
void *thread_process(void *t)
{

#ifdef TEST
    clock_t search_time = clock();
#endif

    // 先把指针类型恢复, 然后取值
    unsigned int tid = *((unsigned int *)t);

    unsigned int end_id = (unsigned int)(seg_ratio[tid + 1] * id_num);
    for (unsigned int start_id = (unsigned int)(seg_ratio[tid] * id_num); start_id < end_id; ++start_id)
    {
#ifdef TEST
        // if (start_id % 100 == 0)
        // {
        //     cout << start_id << "/" << id_num << " ~ " << (double)(clock() - search_time) / CLOCKS_PER_SEC << " ~ " << (double)(start_id) / id_num << endl;
        // }
#endif

        for (unordered_map<unsigned int, vector<unsigned int>>::iterator js = two_uj[start_id].begin(); js != two_uj[start_id].end(); ++js)
        {
            unsigned int sign = js->first;
            reachable[tid][sign] = true;
            currentJs[tid][num_currentJs[tid]++] = sign;
        }

        // dfs_rec(start_id, start_id, 0, tid);
        dfs_ite(start_id, start_id, 0, tid);

        for (unsigned int thread_id = 0; thread_id < num_currentJs[tid]; ++thread_id)
        {
            reachable[tid][currentJs[tid][thread_id]] = false;
        }

        num_currentJs[tid] = 0;
    }

#ifdef TEST
    cout << "tid " << tid << " DFS " << (double)(clock() - search_time) / CLOCKS_PER_SEC << "s" << endl;
#endif
    // pthread_exit(NULL) 为退出该线程
    pthread_exit(NULL);
}

int main()
{
    string testFile = "/data/test_data.txt";
    string resultFile = "/projects/student/result.txt";
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
    string dataset = "3512444";
    testFile = "test_data/" + dataset + "/test_data.txt";
    resultFile = "test_data/" + dataset + "/result.txt";
    clock_t start_time = clock();
#endif

    vector<string> idsComma(MAX_NUM_IDS); // id + ','
    vector<string> idsLF(MAX_NUM_IDS);    // id + '\n'

#ifdef MMAP
    input_mmap(testFile);
#else
    input_fstream(testFile);
#endif

#ifdef TEST
    clock_t sort_time = clock();
#endif
    sort(ids, ids + (edge_num << 1));
#ifdef TEST
    cout << "sort " << (double)(clock() - sort_time) / CLOCKS_PER_SEC << endl;
#endif

    duplicate_removal();

    for (unsigned int thread_id = 0; thread_id < edge_num; ++thread_id)
    {
        unsigned int u = binary_search(u_ids[thread_id]);
        g_succ[u][out_degree[u]++] = binary_search(v_ids[thread_id]);
    }

    for (unsigned int thread_id = 0; thread_id < id_num; ++thread_id)
    {
        sort(g_succ[thread_id], g_succ[thread_id] + out_degree[thread_id]);
        g_succ[thread_id][out_degree[thread_id]] = MAX_INT;
        idsComma[thread_id] = to_string(ids[thread_id]) + ",";
        idsLF[thread_id] = to_string(ids[thread_id]) + "\n";
    }

#ifdef TEST
    clock_t two_uj_time = clock();
#endif
    register unsigned int u;
    register unsigned int k;
    register unsigned int v;

    two_uj.resize(id_num);
    for (u = 0; u < id_num; u++)
    {
        unsigned int *succ1 = g_succ[u];
        // one step succ
        for (unsigned int it1 = 0; it1 < out_degree[u]; ++it1)
        {
            k = succ1[it1];
            // two steps succ
            unsigned int *succ2 = g_succ[k];
            for (unsigned int it2 = 0; it2 < out_degree[k]; ++it2)
            {
                v = succ2[it2];
                if (k > v && u > v)
                    two_uj[v][u].push_back(k);
            }
        }
    }

#ifdef TEST
    cout << "Construct Jump Table " << (double)(clock() - two_uj_time) / CLOCKS_PER_SEC << "s" << endl;
#endif

    int thread_id;
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
    for (thread_id = 0; thread_id < NUM_THREADS; thread_id++)
    {
#ifdef TEST
        cout << "main() : creating thread, " << thread_id << endl;
#endif
        indexes[thread_id] = thread_id;
        pthread_create(&threads[thread_id], NULL, thread_process, (void *)&indexes[thread_id]);
    }

    // 删除属性，并等待其他线程
    pthread_attr_destroy(&attr);
    for (thread_id = 0; thread_id < NUM_THREADS; ++thread_id)
    {
        pthread_join(threads[thread_id], &status);
#ifdef TEST
        cout << "Main: completed thread id :" << thread_id;
        cout << "  exiting with status :" << status << endl;
#endif
    }

#ifdef TEST
    cout << "Main: program exiting." << endl;
#endif

#ifdef MMAP
    save_mmap(resultFile, idsComma, idsLF);
#else
    save_fwrite(resultFile, idsComma, idsLF);
#endif

#ifdef TEST
    cout << "Total time " << (double)(clock() - start_time) / CLOCKS_PER_SEC << "s" << endl;
#endif
    pthread_exit(NULL);
    return 0;
}