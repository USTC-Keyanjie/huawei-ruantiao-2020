// submit:
// 1. delete #define TEST.
// 2. open //#define MMAP

#define TEST
// #define MMAP
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

#define MAX_NUM_EDGES 280000 // 280000
#define MAX_NUM_IDS 204800   // 204800
#define ID_COUNT_SIZE 80000000
#define MAX_OUT_DEGREE 51
#define MAX_IN_DEGREE 51

#define MAX_NUM_THREE_PREDS 10000

#define NUM_LEN3_RESULT 1200000
#define NUM_LEN4_RESULT 1600000
#define NUM_LEN5_RESULT 4000000
#define NUM_LEN6_RESULT 9000000
#define NUM_LEN7_RESULT 14000000

#define MAX_OUTPUT_FILE_SIZE 140000000 //submit: 140000000 test: 520000000

#define MAX_INT 2147483647

#define NUM_THREADS 4

using namespace std;

unsigned int id_num = 0, edge_num = 0;

// float seg_ratio[] = {0, 1};
float seg_ratio[] = {0, 0.068, 0.148, 0.284, 1};

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

unsigned int id_count[ID_COUNT_SIZE];
unsigned int ids[MAX_NUM_IDS];

unsigned int g_succ[MAX_NUM_IDS][MAX_OUT_DEGREE];
unsigned int out_degree[MAX_NUM_IDS];
unsigned int g_pred[MAX_NUM_IDS][MAX_IN_DEGREE];
unsigned int in_degree[MAX_NUM_IDS];

unsigned int path[NUM_THREADS][4];
bool visited[NUM_THREADS][MAX_NUM_IDS];

char idsChar[MAX_NUM_IDS * 10]; // chars id
unsigned int idsChar_len[MAX_NUM_IDS];

Three_pred three_uj[NUM_THREADS][MAX_NUM_THREE_PREDS];
unsigned int three_uj_len[NUM_THREADS];
unsigned int reachable[NUM_THREADS][MAX_NUM_IDS];
unsigned int currentJs[NUM_THREADS][MAX_NUM_THREE_PREDS];
unsigned int currentJs_len[NUM_THREADS];

unsigned int res3[NUM_THREADS * NUM_LEN3_RESULT];
unsigned int res4[NUM_THREADS * NUM_LEN4_RESULT];
unsigned int res5[NUM_THREADS * NUM_LEN5_RESULT];
unsigned int res6[NUM_THREADS * NUM_LEN6_RESULT];
unsigned int res7[NUM_THREADS * NUM_LEN7_RESULT];

unsigned int res_count[NUM_THREADS][5];
unsigned int *results[] = {res3, res4, res5, res6, res7};

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

unsigned int digits10_length(unsigned int k2)
{
    if (k2 < 10)
        return 1;
    if (k2 < 100)
        return 2;
    if (k2 < 1000)
        return 3;

    if (k2 < 1e9)
    {
        if (k2 < 1e7)
        {
            if (k2 < 1e5)
            {
                return 4 + (k2 >= 1e4);
            }
            return 6 + (k2 >= 1e6);
        }
        return 8 + (k2 >= 1e8);
    }
    return 10;
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
    unsigned int next = length - 1;

    while (value >= 100)
    {
        const unsigned int u = (value % 100) * 2;
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
        unsigned int u = value * 2;
        dst[next - 1] = digits[u];
        dst[next] = digits[u + 1];
    }
    return length;
}

void save_fwrite(char *resultFile)
{
    register unsigned int tid;
    unsigned int all_res_count = 0;
    for (int res_len = 0; res_len < 5; ++res_len)
    {
        for (tid = 0; tid < NUM_THREADS; ++tid)
        {
            all_res_count += res_count[tid][res_len];
        }
    }

#ifdef TEST
    clock_t write_time = clock();
    printf("Total Loops %d\n", all_res_count);
#endif

    FILE *fp = fopen(resultFile, "w");
    char *str_res = (char *)malloc(MAX_OUTPUT_FILE_SIZE);

    register unsigned int str_len = uint2ascii(all_res_count, str_res);
    str_res[str_len++] = '\n';

    register unsigned int line_offset, line, id, result_id;
    for (tid = 0; tid < NUM_THREADS; ++tid)
    {
        for (line = 0, line_offset = 0; line < res_count[tid][0]; ++line, line_offset += 3)
        {
            for (id = 0; id < 2; ++id)
            {
                result_id = results[0][tid * NUM_LEN3_RESULT + line_offset + id];
                memcpy(str_res + str_len, idsChar + result_id * 10, idsChar_len[result_id]);
                str_len += idsChar_len[result_id];
                str_res[str_len++] = ',';
            }
            result_id = results[0][tid * NUM_LEN3_RESULT + line_offset + 2];
            memcpy(str_res + str_len, idsChar + result_id * 10, idsChar_len[result_id]);
            str_len += idsChar_len[result_id];
            str_res[str_len++] = '\n';
        }
    }

    for (tid = 0; tid < NUM_THREADS; ++tid)
    {
        for (line = 0, line_offset = 0; line < res_count[tid][1]; ++line, line_offset += 4)
        {
            for (id = 0; id < 3; ++id)
            {
                result_id = results[1][tid * NUM_LEN4_RESULT + line_offset + id];
                memcpy(str_res + str_len, idsChar + result_id * 10, idsChar_len[result_id]);
                str_len += idsChar_len[result_id];
                str_res[str_len++] = ',';
            }
            result_id = results[1][tid * NUM_LEN4_RESULT + line_offset + 3];
            memcpy(str_res + str_len, idsChar + result_id * 10, idsChar_len[result_id]);
            str_len += idsChar_len[result_id];
            str_res[str_len++] = '\n';
        }
    }

    for (tid = 0; tid < NUM_THREADS; ++tid)
    {
        for (line = 0, line_offset = 0; line < res_count[tid][2]; ++line, line_offset += 5)
        {
            for (id = 0; id < 4; ++id)
            {
                result_id = results[2][tid * NUM_LEN5_RESULT + line_offset + id];
                memcpy(str_res + str_len, idsChar + result_id * 10, idsChar_len[result_id]);
                str_len += idsChar_len[result_id];
                str_res[str_len++] = ',';
            }
            result_id = results[2][tid * NUM_LEN5_RESULT + line_offset + 4];
            memcpy(str_res + str_len, idsChar + result_id * 10, idsChar_len[result_id]);
            str_len += idsChar_len[result_id];
            str_res[str_len++] = '\n';
        }
    }

    for (tid = 0; tid < NUM_THREADS; ++tid)
    {
        for (line = 0, line_offset = 0; line < res_count[tid][3]; ++line, line_offset += 6)
        {
            for (id = 0; id < 5; ++id)
            {
                result_id = results[3][tid * NUM_LEN6_RESULT + line_offset + id];
                memcpy(str_res + str_len, idsChar + result_id * 10, idsChar_len[result_id]);
                str_len += idsChar_len[result_id];
                str_res[str_len++] = ',';
            }
            result_id = results[3][tid * NUM_LEN6_RESULT + line_offset + 5];
            memcpy(str_res + str_len, idsChar + result_id * 10, idsChar_len[result_id]);
            str_len += idsChar_len[result_id];
            str_res[str_len++] = '\n';
        }
    }

    for (tid = 0; tid < NUM_THREADS; ++tid)
    {
        for (line = 0, line_offset = 0; line < res_count[tid][4]; ++line, line_offset += 7)
        {
            for (id = 0; id < 6; ++id)
            {
                result_id = results[4][tid * NUM_LEN7_RESULT + line_offset + id];
                memcpy(str_res + str_len, idsChar + result_id * 10, idsChar_len[result_id]);
                str_len += idsChar_len[result_id];
                str_res[str_len++] = ',';
            }
            result_id = results[4][tid * NUM_LEN7_RESULT + line_offset + 6];
            memcpy(str_res + str_len, idsChar + result_id * 10, idsChar_len[result_id]);
            str_len += idsChar_len[result_id];
            str_res[str_len++] = '\n';
        }
    }

    str_res[str_len] = '\0';
    fwrite(str_res, sizeof(char), str_len, fp);
    fclose(fp);

#ifdef TEST
    cout << "fwrite output time " << (double)(clock() - write_time) / CLOCKS_PER_SEC << "s" << endl;
#endif
}

// recursion version
void pre_dfs_rec(unsigned int start_id, unsigned int cur_id, int depth, int tid)
{
    unsigned int begin = 0;
    if (depth < 2)
    {
        while (start_id >= g_pred[cur_id][begin])
            ++begin;
    }
    else
    {
        while (start_id > g_pred[cur_id][begin])
            ++begin;
    }

    for (; begin < in_degree[cur_id]; ++begin)
    {
        unsigned int next_id = g_pred[cur_id][begin];
        if (!visited[tid][next_id])
        {
            path[tid][depth] = next_id;

            if (depth == 2 && path[tid][2] != path[tid][0])
            {
                three_uj[tid][three_uj_len[tid]++] = {path[tid][2], path[tid][1], path[tid][0]};
            }
            else
            {
                visited[tid][next_id] = true;
                pre_dfs_rec(start_id, next_id, depth + 1, tid);
                visited[tid][next_id] = false;
            }
        }
    }
}

// recursion version
void dfs_rec(unsigned int start_id, unsigned int cur_id, int depth, int tid)
{
    if (reachable[tid][cur_id])
    {
        unsigned int thread_offset = 0;
        switch (depth)
        {
        case 0:
            thread_offset = NUM_LEN3_RESULT;
            break;
        case 1:
            thread_offset = NUM_LEN4_RESULT;
            break;
        case 2:
            thread_offset = NUM_LEN5_RESULT;
            break;
        case 3:
            thread_offset = NUM_LEN6_RESULT;
            break;
        case 4:
            thread_offset = NUM_LEN7_RESULT;
            break;
        default:
            break;
        }
        for (unsigned int i = reachable[tid][cur_id] - 1; three_uj[tid][i].u == cur_id; ++i)
        {
            if (!visited[tid][three_uj[tid][i].k1] && !visited[tid][three_uj[tid][i].k2])
            {
                memcpy(results[depth] + tid * thread_offset + res_count[tid][depth]++ * (depth + 3), path[tid], 4 * depth);
                results[depth][tid * thread_offset + res_count[tid][depth] * (depth + 3) - 3] = cur_id;
                results[depth][tid * thread_offset + res_count[tid][depth] * (depth + 3) - 2] = three_uj[tid][i].k1;
                results[depth][tid * thread_offset + res_count[tid][depth] * (depth + 3) - 1] = three_uj[tid][i].k2;
            }
        }
    }

    if (depth < 4)
    {
        unsigned int begin = 0;
        while (start_id >= g_succ[cur_id][begin])
            ++begin;

        visited[tid][cur_id] = true;
        path[tid][depth] = cur_id;

        for (; begin < out_degree[cur_id]; ++begin)
        {
            if (!visited[tid][g_succ[cur_id][begin]])
            {
                dfs_rec(start_id, g_succ[cur_id][begin], depth + 1, tid);
            }
        }
        visited[tid][cur_id] = false;
    }
}

bool three_uj_cmp(Three_pred &a, Three_pred &b)
{
    if (a.u != b.u)
        return a.u < b.u;
    else if (a.k1 != b.k1)
        return a.k1 < b.k1;
    else
        return a.k2 < b.k2;
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
        //     printf("%d/%d ~ %.2lfs ~ %2.lf%%\n", start_id, id_num, (double)(clock() - search_time) / CLOCKS_PER_SEC, (double)(start_id) / id_num * 100);
        // }
#endif

        pre_dfs_rec(start_id, start_id, 0, tid);
        // 有直达的点才继续搜下去
        if (three_uj_len[tid])
        {
            sort(three_uj[tid], three_uj[tid] + three_uj_len[tid], three_uj_cmp);
            three_uj[tid][three_uj_len[tid]] = {MAX_INT, MAX_INT, MAX_INT};
            for (unsigned int i = 0; i < three_uj_len[tid]; ++i)
            {
                if (!reachable[tid][three_uj[tid][i].u])
                {
                    reachable[tid][three_uj[tid][i].u] = i + 1;
                    currentJs[tid][currentJs_len[tid]++] = three_uj[tid][i].u;
                }
            }

            // dfs_ite(start_id, tid);
            dfs_rec(start_id, start_id, 0, tid);

            for (unsigned int j = 0; j < currentJs_len[tid]; ++j)
            {
                reachable[tid][currentJs[tid][j]] = 0;
            }
            three_uj_len[tid] = 0;
            currentJs_len[tid] = 0;
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
    char testFile[] = "test_data/3512444/test_data.txt";
    char resultFile[] = "test_data/3512444/result.txt";
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

    for (unsigned int id = 1; id < MAX_NUM_IDS; ++id)
    {
        id_count[id] += id_count[id - 1];
    }
    id_num = id_count[MAX_NUM_IDS - 1];

    register unsigned int u_hash_id, v_hash_id;
    for (unsigned int id = 0; id < edge_num; ++id)
    {
        u_hash_id = id_count[u_ids[id]] - 1;
        v_hash_id = id_count[v_ids[id]] - 1;

        ids[u_hash_id] = u_ids[id];
        ids[v_hash_id] = v_ids[id];

        g_succ[u_hash_id][out_degree[u_hash_id]++] = v_hash_id;
        g_pred[v_hash_id][in_degree[v_hash_id]++] = u_hash_id;
    }

    for (unsigned int id = 0; id < id_num; ++id)
    {
        sort(g_succ[id], g_succ[id] + out_degree[id]);
        sort(g_pred[id], g_pred[id] + in_degree[id]);
        g_succ[id][out_degree[id]] = MAX_INT;
        g_pred[id][in_degree[id]] = MAX_INT;
        idsChar_len[id] = uint2ascii(ids[id], idsChar + 10 * id);
    }

    int tid;
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
