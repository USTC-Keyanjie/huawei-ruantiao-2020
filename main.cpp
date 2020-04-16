// submit:
// 1. delete #define TEST.

//#define TEST
#include <bits/stdc++.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#define MAX_NUM_EDGES 280000 // 可接受的最大边数
#define MAX_NUM_IDS 280000   // 可接受的最大结点数
#define MAX_OUT_DEGREE 51    // 可接受的最大出度

#define NUM_LEN3_RESULT 500000
#define NUM_LEN4_RESULT 500000
#define NUM_LEN5_RESULT 1000000
#define NUM_LEN6_RESULT 2000000
#define NUM_LEN7_RESULT 3200000 // 提交时改成3000000

#define MAX_INT 2147483647

using namespace std;

int id_num = 0, edge_num = 0, res_count = 0;

// store edge info (u-v)
int u_ids[MAX_NUM_EDGES];
int v_ids[MAX_NUM_EDGES];

int g_succ[MAX_NUM_IDS][MAX_OUT_DEGREE]; // 图的邻接表
int out_degree[MAX_NUM_IDS];             // 每个结点的出度，也可用作邻接表的范围限定

// DFS部分
int path[7];
bool visited[MAX_NUM_IDS];

// 反向二级跳表部分
vector<unordered_map<int, vector<int>>> two_uj;
bool reachable[MAX_NUM_IDS];
vector<int> currentJs;

// results
// e.g.
// for path length 6 [9, 5, 6, 2, 0, 3]
// for (int i = 0; i < 6; ++i)
// {
//     results[6 - 3][num_res6 * 6 + i] = path[i];
// }
// num_res6++;
int res3[NUM_LEN3_RESULT * 3];
int res4[NUM_LEN4_RESULT * 4];
int res5[NUM_LEN5_RESULT * 5];
int res6[NUM_LEN6_RESULT * 6];
int res7[NUM_LEN7_RESULT * 7];
int num_res[5] = {0};
int *results[] = {res3, res4, res5, res6, res7};

void input_data(string &testFile, set<int> &ids_set)
{
#ifdef TEST
    clock_t input_time = clock();
#endif
    int fd;

    fd = open(testFile.c_str(), O_RDONLY);
    //get the size of the document
    int length = lseek(fd, 0, SEEK_END);

    //mmap
    char *p_map = (char *)mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, 0);
    if (p_map == MAP_FAILED)
    {
        printf("mmap fail\n");
        close(fd);
        munmap(p_map, length);
    }
    char *buf;
    buf = (char *)malloc(length);
    strcpy(buf, p_map);
    char *token = strtok(buf, ",\n");
    while (token != NULL)
    {
        u_ids[edge_num] = atoi(token);
        ids_set.insert(u_ids[edge_num]);
        token = strtok(NULL, ",\n");
        v_ids[edge_num] = atoi(token);
        ids_set.insert(v_ids[edge_num++]);
        token = strtok(NULL, ",\n");
        token = strtok(NULL, ",\n");
    }
    close(fd);
    munmap(p_map, length);
#ifdef TEST
    cout << "Input time " << (double)(clock() - input_time) / CLOCKS_PER_SEC << "s" << endl;
#endif
}

// memcpy version
void save_results(const string &resultFile, vector<string> &idsComma, vector<string> &idsLF)
{
#ifdef TEST
    clock_t write_time = clock();
    printf("Total Loops %d\n", res_count);
#endif
    //copy the results into str_res and get the length of results
    char *str_res;
    str_res = (char *)malloc(512 * 1024 * 1024); // 512M space

    char buf[1024];
    int idx = sprintf(buf, "%d\n", res_count);
    buf[idx] = '\0';
    memcpy(str_res, buf, idx);

    int str_len = idx;

    // 结果长度
    for (int res_len = 3; res_len <= 7; ++res_len)
    {
        // 每行结果
        for (int line = 0; line < num_res[res_len - 3]; ++line)
        {
            // 环中每个id
            for (int id = 0; id < res_len - 1; ++id)
            {
                const char *res = idsComma[results[res_len - 3][line * res_len + id]].c_str();
                memcpy(str_res + str_len, res, strlen(res));
                str_len += strlen(res);
            }
            const char *res = idsLF[results[res_len - 3][line * res_len + res_len - 1]].c_str();
            memcpy(str_res + str_len, res, strlen(res));
            str_len += strlen(res);
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
    cout << "Output time " << (double)(clock() - write_time) / CLOCKS_PER_SEC << "s" << endl;
#endif
}

void dfs(int start_id, int cur_id, int depth)
{
    // 只搜索比start大的点，由于哨兵机制，不需要判断begin会越界，因为最后一个数是INT_MAX，start_id一定小于它
    int begin = 0;
    while (start_id >= g_succ[cur_id][begin])
        ++begin;

    if (begin == out_degree[cur_id])
        return;

    visited[cur_id] = true;
    path[depth] = cur_id;

    // 搜索所有大于start的后继
    for (; begin < out_degree[cur_id]; ++begin)
    {
        int next_id = g_succ[cur_id][begin];
        // 后继被访问过
        if (!visited[next_id])
        {
            // 后继可直接到达起点，就找到一个环
            if (reachable[next_id])
            {
                // find circuits
                for (int &k : two_uj[start_id][next_id])
                {
                    if (!visited[k])
                    {
                        path[depth + 1] = next_id;
                        path[depth + 2] = k;

                        // int length = depth + 3;
                        // memcpy(results[depth] + num_res[depth] * length, path, sizeof(int) * (depth + 3));
                        memcpy(results[depth] + num_res[depth] * (depth + 3), path, 4 * depth + 12);
                        ++num_res[depth];
                        ++res_count;
                    }
                }
            }

            if (depth < 4)
                dfs(start_id, next_id, depth + 1);
        }
    }

    visited[cur_id] = false;
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

    set<int> ids_set;
    vector<int> ids;                 // regular-id(0, 1, 2, ..., n-1) to really-id
    unordered_map<int, int> id_hash; // really-id to regular-id(0, 1, 2, ..., n-1)

    vector<string> idsComma(MAX_NUM_IDS); // id + ','
    vector<string> idsLF(MAX_NUM_IDS);    // id + '\n'

    // input_data
    input_data(testFile, ids_set);

    ids = vector<int>(ids_set.begin(), ids_set.end());
    id_num = ids.size();

    two_uj.resize(id_num);

    for (int i = 0; i < id_num; ++i)
    {
        id_hash[ids[i]] = i;
    }

    for (int i = 0; i < edge_num; ++i)
    {
        int u = id_hash[u_ids[i]], v = id_hash[v_ids[i]];
        g_succ[u][out_degree[u]++] = v;
    }

    for (int i = 0; i < id_num; ++i)
    {
        sort(g_succ[i], g_succ[i] + out_degree[i]);
        // 哨兵机制，dfs刚开始确定begin时，只需要判断start和其后继的值即可，不需要判断越界
        // 存在产生bug的可能性：刚好有结点出度为50，那么这里赋值时会造成数组越界
        g_succ[i][out_degree[i]] = MAX_INT;
        // 这两个数组真的有必要吗？可以输出时动态生成 to_string(ids[i]) + "," 这样的数据吗？
        idsComma[i] = to_string(ids[i]) + ",";
        idsLF[i] = to_string(ids[i]) + "\n";
    }

#ifdef TEST
    clock_t two_uj_time = clock();
#endif

    // 构建反向二级跳表
    for (int u = 0; u < id_num; u++)
    {
        int *succ1 = g_succ[u];
        // one step succ
        for (int it1 = 0; it1 < out_degree[u]; ++it1)
        {
            int k = succ1[it1];
            // two steps succ
            int *succ2 = g_succ[k];
            for (int it2 = 0; it2 < out_degree[k]; ++it2)
            {
                int v = succ2[it2];
                if (k > v && u > v)
                    two_uj[v][u].push_back(k);
            }
        }
    }

#ifdef TEST
    cout << "Construct Jump Table " << (double)(clock() - two_uj_time) / CLOCKS_PER_SEC << "s" << endl;
    clock_t search_time = clock();
#endif
    // DFS搜索
    for (int start_id = 0; start_id < id_num; ++start_id)
    {
#ifdef TEST
        // if (start_id % 100 == 0)
        // {
        //     cout << start_id << "/" << id_num << " ~ " << res_count << endl;
        // }
#endif

        for (unordered_map<int, vector<int>>::iterator js = two_uj[start_id].begin(); js != two_uj[start_id].end(); ++js)
        {
            int j = js->first;
            reachable[j] = true;
            currentJs.push_back(j);
        }

        dfs(start_id, start_id, 0);

        for (int &j : currentJs)
        {
            reachable[j] = false;
        }
        currentJs.clear();
    }

#ifdef TEST
    cout << "DFS " << (double)(clock() - search_time) / CLOCKS_PER_SEC << "s" << endl;
#endif

    save_results(resultFile, idsComma, idsLF);

#ifdef TEST
    cout << "Total time " << (double)(clock() - start_time) / CLOCKS_PER_SEC << "s" << endl;
#endif
    return 0;
}
