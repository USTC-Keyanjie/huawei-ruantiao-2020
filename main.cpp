// submit: 1. delete #define TEST. 2. open //#include <bits/stdc++.h>

//#define TEST
#include <bits/stdc++.h>

#ifdef TEST
#include <algorithm>
#include <ctime>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#endif

using namespace std;

void input_fstream(string &testFile, int &edge_num, set<int> &ids_set, int *u_ids, int *v_ids)
{
#ifdef TEST
    clock_t input_time = clock();
#endif

    FILE *file = fopen(testFile.c_str(), "r");

    while (fscanf(file, "%d,", u_ids + edge_num) != EOF)
    {
        fscanf(file, "%d,", v_ids + edge_num);
        ids_set.insert(u_ids[edge_num]);
        ids_set.insert(v_ids[edge_num++]);
        while (fgetc(file) != '\n')
            ;
    }
#ifdef TEST
    cout << "Input time " << (double)(clock() - input_time) / CLOCKS_PER_SEC << "s" << endl;
#endif
}

void save_fwrite(const string &resultFile, int &ansCnt, vector<string> &idsComma, vector<string> &idsLF, vector<vector<vector<int>>> &results)
{
#ifdef TEST
    clock_t write_time = clock();
    printf("Total Loops %d\n", ansCnt);
#endif

    FILE *fp = fopen(resultFile.c_str(), "w");
    char buf[1024];
    int idx = sprintf(buf, "%d\n", ansCnt);
    buf[idx] = '\0';
    fwrite(buf, idx, sizeof(char), fp);
    for (int i = 0; i < 5; ++i)
    {
        for (vector<int> &single_result : results[i])
        {
            for (int j = 0; j < i + 2; j++)
            {
                string res = idsComma[single_result[j]];
                fwrite(res.c_str(), res.size(), sizeof(char), fp);
            }
            string res = idsLF[single_result[i + 2]];
            fwrite(res.c_str(), res.size(), sizeof(char), fp);
        }
    }
    fclose(fp);

#ifdef TEST
    cout << "Output time " << (double)(clock() - write_time) / CLOCKS_PER_SEC << "s" << endl;
#endif
}

void dfs(int start,
         int cur_id,
         int depth,
         int &ansCnt,
         vector<vector<int>> &one_dj,
         vector<bool> &visited,
         vector<int> &path,
         vector<vector<vector<int>>> &results,
         vector<unordered_map<int, vector<int>>> &two_uj,
         vector<bool> &reachable,
         vector<int> &currentJs)
{
    visited[cur_id] = true;
    path.push_back(cur_id);

    vector<int>::iterator it = lower_bound(one_dj[cur_id].begin(), one_dj[cur_id].end(), start);
    for (; it != one_dj[cur_id].end(); it++)
    {
        if (!visited[*it])
        {
            if (reachable[*it])
            {
                // find circuits
                for (int &k : two_uj[start][*it])
                {
                    if (!visited[k])
                    {
                        vector<int> res = path;
                        res.push_back(*it);
                        res.push_back(k);
                        results[depth].push_back(res);
                        ++ansCnt;
                    }
                }
            }

            if (depth < 4)
                dfs(start, *it, depth + 1, ansCnt, one_dj, visited, path, results, two_uj, reachable, currentJs);
        }
    }

    visited[cur_id] = false;
    path.pop_back();
}

int main()
{
    int id_num = 0;
    int edge_num = 0;
    int ansCnt = 0;

    // input
    string testFile = "/data/test_data.txt";
    string resultFile = "/projects/student/result.txt";

    // store edge info (u-v)
    int *u_ids = (int *)malloc(sizeof(int) * 280000);
    int *v_ids = (int *)malloc(sizeof(int) * 280000);

    set<int> ids_set;
    vector<int> ids;                 // regular-id(0, 1, 2, ..., n-1) to really-id
    unordered_map<int, int> id_hash; // really-id to regular-id(0, 1, 2, ..., n-1)

#ifdef TEST
    string dataset = "3512444";
    testFile = "test_data/" + dataset + "/test_data.txt";
    resultFile = "test_data/" + dataset + "/result.txt";
    clock_t start_time = clock();

#endif

    input_fstream(testFile, edge_num, ids_set, u_ids, v_ids);

    ids = vector<int>(ids_set.begin(), ids_set.end());
    id_num = ids.size();

    // define data structure
    vector<string> idsComma(id_num); //0...n to sorted id
    vector<string> idsLF(id_num);    //0...n to sorted id

    vector<int> path;
    vector<vector<int>> one_dj(id_num);
    vector<bool> visited(id_num);
    vector<vector<vector<int>>> results(5);

    vector<unordered_map<int, vector<int>>> two_uj(id_num);

    vector<bool> reachable(id_num);
    vector<int> currentJs(id_num);

    for (int i = 0; i < ids.size(); i++)
    {
        id_hash[ids[i]] = i;
    }

    for (int i = 0; i < edge_num; i++)
    {
        int u = id_hash[u_ids[i]], v = id_hash[v_ids[i]];
        one_dj[u].push_back(v);
    }

    for (int i = 0; i < id_num; i++)
    {
        sort(one_dj[i].begin(), one_dj[i].end());
        idsComma[i] = to_string(ids[i]) + ",";
        idsLF[i] = to_string(ids[i]) + "\n";
    }

#ifdef TEST
    clock_t two_uj_time = clock();
#endif

    // 2 up steps jump table
    for (int u = 0; u < id_num; u++)
    {
        // one step succ
        vector<int> &succ1 = one_dj[u];
        for (int &k : succ1)
        {
            // two steps succ
            vector<int> &succ2 = one_dj[k];
            for (int &v : succ2)
            {
                if (k > v && u > v)
                    two_uj[v][u].push_back(k);
            }
        }
    }

#ifdef TEST
    cout << "Construct Jump Table " << (double)(clock() - two_uj_time) / CLOCKS_PER_SEC << "s" << endl;
    clock_t search_time = clock();
#endif

    for (int start = 0; start < id_num; ++start)
    {

#ifdef TEST
        if (start % 100 == 0)
        {
            cout << start << "/" << id_num << " ~ " << ansCnt << endl;
        }
#endif
        for (unordered_map<int, vector<int>>::iterator js = two_uj[start].begin(); js != two_uj[start].end(); ++js)
        {
            int j = js->first;
            reachable[j] = true;
            currentJs.push_back(j);
        }

        dfs(start, start, 0, ansCnt, one_dj, visited, path, results, two_uj, reachable, currentJs);

        for (int &j : currentJs)
        {
            reachable[j] = false;
        }
        currentJs.clear();
    }

#ifdef TEST
    cout << "DFS " << (double)(clock() - search_time) / CLOCKS_PER_SEC << "s" << endl;
#endif

    save_fwrite(resultFile, ansCnt, idsComma, idsLF, results);

#ifdef TEST
    cout << "Total time " << (double)(clock() - start_time) / CLOCKS_PER_SEC << "s" << endl;
#endif

    return 0;
}