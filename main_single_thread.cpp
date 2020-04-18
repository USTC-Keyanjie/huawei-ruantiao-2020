// submit:
// 1. delete #define TEST.
// 2. open //#define MMAP
// 3. change NUM_LEN7_RESULT to 3000000

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

#define MAX_NUM_EDGES 280000
#define MAX_NUM_IDS 262144
#define MAX_OUT_DEGREE 51

#define NUM_LEN3_RESULT 500000
#define NUM_LEN4_RESULT 500000
#define NUM_LEN5_RESULT 1000000
#define NUM_LEN6_RESULT 2000000
#define NUM_LEN7_RESULT 3200000

#define MAX_INT 2147483647

using namespace std;

int id_num = 0, edge_num = 0, res_count = 0;

int u_ids[MAX_NUM_EDGES];
int v_ids[MAX_NUM_EDGES];
int ids[MAX_NUM_EDGES * 2];

int g_succ[MAX_NUM_IDS][MAX_OUT_DEGREE];
int out_degree[MAX_NUM_IDS];

int path[7];
bool visited[MAX_NUM_IDS];

vector<unordered_map<int, vector<int>>> two_uj;
bool reachable[MAX_NUM_IDS];
vector<int> currentJs;

int res3[NUM_LEN3_RESULT * 3];
int res4[NUM_LEN4_RESULT * 4];
int res5[NUM_LEN5_RESULT * 5];
int res6[NUM_LEN6_RESULT * 6];
int res7[NUM_LEN7_RESULT * 7];
int num_res[5] = {0};
int *results[] = {res3, res4, res5, res6, res7};

void input_fstream(const string &testFile)
{
#ifdef TEST
    clock_t input_time = clock();
#endif

    FILE *file = fopen(testFile.c_str(), "r");
    int money;
    while (fscanf(file, "%d,%d,%d\n", u_ids + edge_num, v_ids + edge_num, &money) != EOF)
    {
        ids[(edge_num << 1)] = u_ids[edge_num];
        ids[(edge_num << 1) + 1] = v_ids[edge_num];
        edge_num++;
    }
    fclose(file);
#ifdef TEST
    cout << "fscanf input time " << (double)(clock() - input_time) / CLOCKS_PER_SEC << "s" << endl;
#endif
}

#ifdef MMAP

void input_mmap(string &testFile)
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
    char *buf;
    buf = (char *)malloc(length);
    strcpy(buf, p_map);
    char *token = strtok(buf, ",\n");
    while (token != NULL)
    {
        ids[(edge_num << 1)] = atoi(token);
        u_ids[edge_num] = ids[(edge_num << 1)];
        token = strtok(NULL, ",\n");
        ids[(edge_num << 1) + 1] = atoi(token);
        v_ids[edge_num++] = ids[(edge_num << 1) + 1];
        token = strtok(NULL, ",\n");
        token = strtok(NULL, ",\n");
    }
    close(fd);
    munmap(p_map, length);
#ifdef TEST
    cout << "mmap input time " << (double)(clock() - input_time) / CLOCKS_PER_SEC << "s" << endl;
#endif
}

#endif

void save_fwrite(const string &resultFile, vector<string> &idsComma, vector<string> &idsLF)
{

#ifdef TEST
    clock_t write_time = clock();
    printf("Total Loops %d\n", res_count);
#endif

    FILE *fp = fopen(resultFile.c_str(), "w");
    char *str_res = (char *)malloc(512 * 1024 * 1024); // 512M space
    int str_len = sprintf(str_res, "%d\n", res_count);

    for (int res_len = 3; res_len <= 7; ++res_len)
    {
        for (int line = 0; line < num_res[res_len - 3]; ++line)
        {
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

    for (int res_len = 3; res_len <= 7; ++res_len)
    {
        for (int line = 0; line < num_res[res_len - 3]; ++line)
        {
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
    cout << "mmap output time " << (double)(clock() - write_time) / CLOCKS_PER_SEC << "s" << endl;
#endif
}
#endif

// recursion version
void dfs_rec(int start_id, int cur_id, int depth)
{
    int begin = 0;
    while (start_id >= g_succ[cur_id][begin])
        ++begin;

    if (begin == out_degree[cur_id])
        return;

    visited[cur_id] = true;
    path[depth] = cur_id;

    for (; begin < out_degree[cur_id]; ++begin)
    {
        int next_id = g_succ[cur_id][begin];
        if (!visited[next_id])
        {
            if (reachable[next_id])
            {
                for (int &k : two_uj[start_id][next_id])
                {
                    if (!visited[k])
                    {
                        path[depth + 1] = next_id;
                        path[depth + 2] = k;

                        memcpy(results[depth] + num_res[depth] * (depth + 3), path, 4 * depth + 12);
                        ++num_res[depth];
                        ++res_count;
                    }
                }
            }

            if (depth < 4)
                dfs_rec(start_id, next_id, depth + 1);
        }
    }

    visited[cur_id] = false;
}

// iteration version
void dfs_ite(int start_id, int cur_id, int depth)
{
    visited[cur_id] = true;
    path[depth] = cur_id;

    int begin_pos[5] = {0};
    while (start_id >= g_succ[cur_id][begin_pos[0]])
        ++begin_pos[0];

    if (begin_pos[0] == out_degree[cur_id])
        return;

    int next_id;
    int *stack[5];
    stack[0] = g_succ[cur_id];

    while (depth >= 0)
    {
        // no valid succ
        if (begin_pos[depth] == out_degree[cur_id])
        {
            visited[cur_id] = false;
            depth--;
            cur_id = depth >= 0 ? path[depth] : 0;
        }
        else
        {
            next_id = stack[depth][begin_pos[depth]++];
            if (!visited[next_id])
            {

                // find a circuit
                if (reachable[next_id])
                {
                    for (int &k : two_uj[start_id][next_id])
                    {
                        if (!visited[k])
                        {
                            path[depth + 1] = next_id;
                            path[depth + 2] = k;

                            memcpy(results[depth] + num_res[depth] * (depth + 3), path, 4 * depth + 12);
                            ++num_res[depth];
                            ++res_count;
                        }
                    }
                }

                if (depth < 4)
                {
                    stack[++depth] = g_succ[next_id];
                    cur_id = next_id;
                    visited[cur_id] = true;
                    path[depth] = cur_id;
                    begin_pos[depth] = 0;
                    while (start_id >= stack[depth][begin_pos[depth]])
                        ++begin_pos[depth];
                }
            }
        }
    }
}

int binary_search(int target)
{
    int left = 0;
    int right = id_num - 1;
    while (left <= right)
    {
        int mid = (right + left) >> 1;
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
    int slow = 0, fast = 1;
    int double_edge_num = edge_num << 1;
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

    sort(ids, ids + (edge_num << 1));
    duplicate_removal();

    for (int i = 0; i < edge_num; ++i)
    {
        int u = binary_search(u_ids[i]);
        g_succ[u][out_degree[u]++] = binary_search(v_ids[i]);
    }

    for (int i = 0; i < id_num; ++i)
    {
        sort(g_succ[i], g_succ[i] + out_degree[i]);
        g_succ[i][out_degree[i]] = MAX_INT;
        idsComma[i] = to_string(ids[i]) + ",";
        idsLF[i] = to_string(ids[i]) + "\n";
    }

#ifdef TEST
    clock_t two_uj_time = clock();
#endif

    two_uj.resize(id_num);
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
    for (int start_id = 0; start_id < id_num; ++start_id)
    {
#ifdef TEST
        // if (start_id % 100 == 0)
        // {
        //     cout << start_id << "/" << id_num << " ~ " << res_count << " ~ " << (double)(clock() - search_time) / CLOCKS_PER_SEC << " ~ " << (double)(start_id) / id_num << endl;
        // }
#endif

        for (unordered_map<int, vector<int>>::iterator js = two_uj[start_id].begin(); js != two_uj[start_id].end(); ++js)
        {
            int j = js->first;
            reachable[j] = true;
            currentJs.push_back(j);
        }

        // dfs_rec(start_id, start_id, 0);
        dfs_ite(start_id, start_id, 0);

        for (int &j : currentJs)
        {
            reachable[j] = false;
        }
        currentJs.clear();
    }

#ifdef TEST
    cout << "DFS " << (double)(clock() - search_time) / CLOCKS_PER_SEC << "s" << endl;
#endif

#ifdef MMAP
    save_mmap(resultFile, idsComma, idsLF);
#else
    save_fwrite(resultFile, idsComma, idsLF);
#endif

#ifdef TEST
    cout << "Total time " << (double)(clock() - start_time) / CLOCKS_PER_SEC << "s" << endl;
#endif
    return 0;
}
