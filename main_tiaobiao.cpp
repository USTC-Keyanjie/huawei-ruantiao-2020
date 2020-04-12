#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <fstream>
#include <ctime>
using namespace std;

int id_num = 0;
int edge_num = 0;
set<int> ids_set;
vector<int> ids;  // regular-id(0, 1, 2, ..., n-1) to really-id
unordered_map<int, int> id_hash;  // really-id to regular-id(0, 1, 2, ..., n-1)
// unordered_map<int, unordered_set<int> > one_jump;
vector<vector<int> > one_dj;
vector<unordered_map<int, vector<int> > > two_uj; // two_uj[v][u]是一个vector，表示从u到v，中间要走一个节点，这个节点存在vector里
vector<vector<unordered_map<int, vector<int> > > > mul_dj;  // mul_jump[n][u][v]是一个vector，表示从u跳n次可以到v，第n-1次的节点在vector里
vector<vector<int> > results;

bool record_time = false;

void save_result(vector<int> path)
{
    reverse(path.begin(), path.end());
    results.push_back(path);
}

void search_result(int num_jump, int start, int pred2, int pred1)
{
    vector<int> path;
    vector<vector<int> > stack;

    int pre_id, cur_jump = num_jump;
    path.push_back(ids[pred1]);
    path.push_back(ids[pred2]);
    stack.push_back(mul_dj[cur_jump][start][pred2]);

    while (!stack.empty())
    {
        vector<vector<int> >::reverse_iterator preds = stack.rbegin();

        if (!preds->empty())
        {
            pre_id = *(preds->begin());
            preds->erase(preds->begin());

            if (find(path.begin(), path.end(), ids[pre_id]) == path.end())
            {
                path.push_back(ids[pre_id]);
                if (pre_id == start)
                {
                    // 深搜正常出口
                    save_result(path);
                    // stack.pop_back();
                    path.pop_back();
                    //cur_jump++;
                }
                else
                {
                    stack.push_back(mul_dj[--cur_jump][start][pre_id]);
                }
            }
        }
        else
        {
            // 这一层深搜结束
            stack.pop_back();
            path.pop_back();
            cur_jump++;
        }
    }
}


void write_result(string &resultFile)
{
    clock_t start_time = clock();
    ofstream outfile;

    outfile.open(resultFile, ios::out);

    if (!outfile.is_open())
        std::cout << "Open file failure\n";
    outfile << results.size() << endl;

    for (vector<int> circuit : results)
    {
        outfile << circuit[0];
        for (int i = 1; i < circuit.size(); i++)
            outfile << "," << circuit[i];
        outfile << endl;
    }


    outfile.close();
    if (record_time) cout << "输出 " << (double)(clock() - start_time) / CLOCKS_PER_SEC << "s" << endl;

}

bool cmp(vector<int> a, vector<int> b)
{
    return a.size() < b.size() || (a.size() == b.size() && a < b);
}

int main()
{
    clock_t start_time = clock();

    string dataset = "77409";

    // 输入
    string testFile = "test_data/" + dataset + "/test_data.txt";
    string resultFile = "test_data/" + dataset + "/result.txt";
    FILE* file = fopen(testFile.c_str(), "r");

    int u, v, c;
    int* u_ids = (int*)malloc(sizeof(int) * 280000);
    int* v_ids = (int*)malloc(sizeof(int) * 280000);
    // TODO: 1. 这两个数据的保存 2. idhash的处理
    /*
    while (fscanf(file, "%d,%d,%d", &u, &v, &c) != EOF) {
        one_jump[u].insert(v);
        ids_set.insert(u);
        ids_set.insert(v);
    }
    */
    while (fscanf(file, "%d,", u_ids + edge_num) != EOF) {
        fscanf(file, "%d,", v_ids + edge_num);
        ids_set.insert(u_ids[edge_num]);
        ids_set.insert(v_ids[edge_num++]);
        while (fgetc(file) != '\n');
    }

    ids = vector<int>(ids_set.begin(), ids_set.end());

    for (int i = 0; i < ids.size(); i++)
    {
        id_hash[ids[i]] = i;
    }

    id_num = ids.size();
    one_dj.resize(id_num);
    two_uj.resize(id_num);
    mul_dj.resize(7, vector<unordered_map<int, vector<int> > >(id_num));

    for (int i = 0; i < edge_num; i++)
    {
        int u = id_hash[u_ids[i]], v = id_hash[v_ids[i]];
        one_dj[u].push_back(v);
    }

    // 可以在插入时就有序
    for (int i = 0; i < id_num; i++)
    {
        sort(one_dj[i].begin(), one_dj[i].end());
    }

    // 维护上2跳表
    for (int i = 0; i < id_num; i++)
    {
        // 一层后继
        vector<int> &succ1 = one_dj[i];
        for (int &k : succ1)
        {
            // 二层后继
            vector<int> &succ2 = one_dj[k];
            for (int &j : succ2)
            {
                int v = ids[j];
                int u = ids[i];
                if (k > j && i > j)
                {

                    two_uj[j][i].push_back(k);
                }
            }
        }
    }
    

    // 维护下1跳表
    for (int start = 0; start < id_num; ++start)
    {
        // cout << 1 << " " << start << endl;
        // 遍历下1跳表
        for (int end : one_dj[start])
        {
            // 路径必须递增
            if (start < end)
            {
                mul_dj[1][start][end].push_back(start);
            }
        }
    }

    /* 不再需要单独维护下2跳表
    // 维护下2跳表
    for (int start = 0; start < n; ++start)
    {
        // cout << 2 << " " << start << endl;
        // 遍历下1跳表
        for (auto it = mul_dj[1][start].begin(); it != mul_dj[1][start].end(); ++it)
        {
            int next = it->first;
            for (int end : one_dj[next])
            {
                // 路径必须大于起始点，下2跳时不考虑回环，也就不考虑start == end 的情况
                if (start < end)
                    mul_dj[2][start][end].push_back(next);
            }
        }
    }
    */

    // 维护下2跳到下5跳
    for (int num_jump = 2; num_jump <= 5; ++num_jump)
    {
        for (int start = 0; start < id_num; ++start)
        {
            // cout << num_jump << " " << start << endl;
            // 遍历下n-1跳表
            for (auto it = mul_dj[num_jump - 1][start].begin(); it != mul_dj[num_jump - 1][start].end(); ++it)
            {
                int next = it->first;


                // 得到two_uj[start][next].size()条路径
                for (int &pred1 : two_uj[start][next])
                {
                    //路径读入
                    search_result(num_jump - 1, start, next, pred1);
                }

                // 遍历one_dj[next]，得到end，start < end就放进来
                for (int end : one_dj[next])
                {
                    // 路径必须递增
                    if (start < end)
                        mul_dj[num_jump][start][end].push_back(next);
                }
                
            }
        }
    }

    // 单独处理7环
    for (int start = 0; start < id_num; ++start)
    {
        // cout << 7 << " " << start << endl;
        // 遍历下n-1跳表
        for (auto it = mul_dj[5][start].begin(); it != mul_dj[5][start].end(); ++it)
        {
            int next = it->first;

            // 判断这个next是否可能通过two_uj到达start
            if (two_uj[start][next].size())
            {
                // 可以，那么得到two_uj[start][next].size()条路径
                for (int &pred1 : two_uj[start][next])
                {
                    //路径读入
                    search_result(5, start, next, pred1);
                }
            }
        }
    }

    sort(results.begin(), results.end(), cmp);

    cout << results.size() << endl;
    /*
    for (vector<int> res: results)
    {
        for (int r : res)
         {
             cout << r << " ";
         }
         cout << endl;
    }
    */


    write_result(resultFile);
    clock_t end_time = clock();
    cout << (double)(end_time - start_time) / CLOCKS_PER_SEC << "s" << endl;
    return 0;
}