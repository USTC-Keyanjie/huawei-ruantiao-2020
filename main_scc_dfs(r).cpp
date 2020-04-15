// submit: 1. delete #define TEST. 2. open //#include <bits/stdc++.h>

#define TEST
//#include <bits/stdc++.h>

#ifdef TEST
#include <algorithm>
#include <ctime>
#include <iostream>
#include <map>
#include <set>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// #include <unistd.h>
// #include <sys/stat.h> 
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <fcntl.h>
// #include <linux/fb.h>
// #include <sys/mman.h>
// #include <sys/ioctl.h>
#endif

using namespace std;

vector<int> succ_begin;


void input_fstream(string &testFile, int &edge_num, set<int> &ids_set, int* u_ids, int* v_ids)
{
#ifdef TEST
	clock_t input_time = clock();
#endif

	FILE* file = fopen(testFile.c_str(), "r");

	while (fscanf(file, "%d,", u_ids + edge_num) != EOF) {
		fscanf(file, "%d,", v_ids + edge_num);
		ids_set.insert(u_ids[edge_num]);
		ids_set.insert(v_ids[edge_num++]);
		while (fgetc(file) != '\n');
	}
#ifdef TEST
	cout << "Input time " << (double)(clock() - input_time) / CLOCKS_PER_SEC << "s" << endl;
#endif
}

/*
void input_fstream(string &testFile, int &edge_num, set<int> &ids_set, int* u_ids, int* v_ids)
{
#ifdef TEST
	clock_t input_time = clock();
#endif
	int fd;
	char *p_map;
	fd = open(testFile.c_str(),O_RDWR);
	if(fd < 0) {
		printf("open fail\n");
		exit(1);
	}
	//get the size of the document
	int length = lseek(fd,0,SEEK_END);
  
	//mmap
	p_map = (char *)mmap(NULL, length, PROT_READ, MAP_PRIVATE,fd, 0);
	if(p_map == MAP_FAILED) {
		printf("mmap fail\n");
		close(fd);
		munmap(p_map, length);
	}
	char *buf;
	buf = (char *)malloc(length);
	strcpy(buf,p_map);
    char* token = strtok(buf, ",\n");
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
*/

void save_fwrite(const string &resultFile, int &ansCnt, vector<string> &idsComma, vector<string> &idsLF, vector<vector<vector<int> > > &results) {

#ifdef TEST
	clock_t write_time = clock();
	printf("Total Loops %d\n", ansCnt);
#endif

	FILE *fp = fopen(resultFile.c_str(), "w");
	char buf[1024];
	int idx = sprintf(buf, "%d\n", ansCnt);
	buf[idx] = '\0';
	fwrite(buf, idx, sizeof(char), fp);
	for (int i = 0; i < 5; ++i) {

		for (vector<int> &single_result : results[i]) {

			for (int j = 0; j < i + 2; j++) {
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

/*
void save_fwrite(const string &resultFile, int &ansCnt, vector<string> &idsComma, vector<string> &idsLF, vector<vector<vector<int> > > &results) {

#ifdef TEST
	clock_t write_time = clock();
	printf("Total Loops %d\n", ansCnt);
#endif
    
	int fd=open(resultFile.c_str(), O_RDWR|O_CREAT, 0644);
	if(fd < 0) {
		printf("open fail\n");
		exit(1);
	}
	
    int length = 0;
    char buf[1024];
	int idx = sprintf(buf, "%d\n", ansCnt);
	buf[idx] = '\0';
	length += idx;
    for (int i = 0; i < 5; ++i) {
		for (vector<int> &single_result : results[i]) {
			for (int j = 0; j < i + 2; j++) {
				const char* res = idsComma[single_result[j]].c_str();
				length += strlen(res);
			}
			const char* res = idsLF[single_result[i + 2]].c_str();
			length += strlen(res);
		}
	}
	int f_ret = ftruncate(fd, length);
	if (-1 == f_ret) { 
		printf("ftruncate fail\n");
		exit(1);
	}
	char *const address = (char *)mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if (MAP_FAILED == address) { 
		printf("mmap fail\n");
		exit(1);
	}
	
	int pos = 0;
	memcpy(address, buf, idx);
	pos += idx;
    
    for (int i = 0; i < 5; ++i) {

		for (vector<int> &single_result : results[i]) {

			for (int j = 0; j < i + 2; j++) {
				const char* res = idsComma[single_result[j]].c_str();
				memcpy(address+pos,res,strlen(res));
				pos += strlen(res);
			}
			const char* res = idsLF[single_result[i + 2]].c_str();
			memcpy(address+pos,res,strlen(res));
			pos += strlen(res);
		}
	}
	int mun_ret = munmap(address, length);
	if (-1 == mun_ret) {
		printf("munmap fail\n");
		exit(1);
	}
	close(fd);

#ifdef TEST
	cout << "Output time " << (double)(clock() - write_time) / CLOCKS_PER_SEC << "s" << endl;
#endif

}
*/

void dfs(int start,
         int cur_id,
         int depth,
         int &ansCnt,
         vector<vector<int>> &g_succ,
         vector<bool> &visited,
         vector<int> &path,
         vector<vector<vector<int>>> &results,
         vector<unordered_map<int, vector<int>>> &two_uj,
         vector<bool> &reachable,
         vector<int> &currentJs,
         vector<int> &scc_color)
{
    visited[cur_id] = true;
    path.push_back(cur_id);

	vector<int>::iterator it = lower_bound(g_succ[cur_id].begin(), g_succ[cur_id].end(), start);
	for (; it != g_succ[cur_id].end(); ++it)
    {
        int next_id = *it;
        if (!visited[next_id] && scc_color[cur_id] == scc_color[next_id])
        {
            if (reachable[next_id])
            {
                // find circuits
                for (int &k : two_uj[start][next_id])
                {
                    if (!visited[k])
                    {
                        vector<int> res = path;
                        res.push_back(next_id);
                        res.push_back(k);
                        results[depth].push_back(res);
                        ++ansCnt;
                    }
                }
            }

            if (depth < 4)
                dfs(start, next_id, depth + 1, ansCnt, g_succ, visited, path, results, two_uj, reachable, currentJs, scc_color);
        }
    }

    visited[cur_id] = false;
    path.pop_back();
}

// special for first divide strongly connected components
vector<unordered_set<int>> first_strongly_connected_components(int &cur_scc_color, int id_num, vector<vector<int>> &g_succ, vector<int> &scc_color, int div_scc_threshold)
{
#ifdef TEST
    clock_t scc_time = clock();
#endif

    unordered_map<int, int> preorder;
    unordered_map<int, int> lowlink;
    unordered_set<int> scc_found;
    stack<int> scc_stack;
    vector<unordered_set<int>> scc_results;

    int i = 0; // Preorder counter
    for (int source = 0; source < id_num; ++source)
    {
        if (scc_found.find(source) == scc_found.end())
        {
            stack<int> stack;
            stack.push(source);
            while (!stack.empty())
            {
                int v = stack.top();
                if (preorder[v] == 0)
                {
                    preorder[v] = ++i;
                }
                bool done = true;
                for (int w : g_succ[v])
                {
                    if (preorder[w] == 0)
                    {
                        stack.push(w);
                        done = false;
                        break;
                    }
                }
                if (done)
                {
                    lowlink[v] = preorder[v];
                    for (int w : g_succ[v])
                    {
                        if (scc_found.find(w) == scc_found.end())
                        {
                            lowlink[v] = preorder[w] > preorder[v] ? min(lowlink[v], lowlink[w]) : min(lowlink[v], preorder[w]);
                        }
                    }
                    stack.pop();
                    if (lowlink[v] == preorder[v])
                    {
                        unordered_set<int> scc;
                        scc_color[v] = cur_scc_color;
                        scc.insert(v);
                        while (!scc_stack.empty() && preorder[scc_stack.top()] > preorder[v])
                        {
                            int k = scc_stack.top();
                            scc_stack.pop();
                            scc_color[k] = cur_scc_color;
                            scc.insert(k);
                        }
                        cur_scc_color++;
                        if (scc.size() > div_scc_threshold)
                            scc_results.push_back(scc);
                        scc_found.insert(scc.begin(), scc.end());
                    }
                    else
                    {
                        scc_stack.push(v);
                    }
                }
            }
        }
    }

#ifdef TEST
    cout << "SCC " << (double)(clock() - scc_time) / CLOCKS_PER_SEC << "s" << endl;
#endif

    return scc_results;
}

vector<unordered_set<int>> strongly_connected_components(int &cur_scc_color, unordered_set<int> &nodes, vector<vector<int>> &g_succ, vector<int> &scc_color, int div_scc_threshold, int cur_id)
{
#ifdef TEST
    clock_t scc_time = clock();
#endif

    unordered_map<int, int> preorder;
    unordered_map<int, int> lowlink;
    unordered_set<int> scc_found;
    stack<int> scc_stack;
    vector<unordered_set<int>> scc_results;

    int i = 0; // Preorder counter
    for (int source : nodes)
    {
        if (source >= cur_id && scc_found.find(source) == scc_found.end())
        {
            stack<int> stack;
            stack.push(source);
            while (!stack.empty())
            {
                int v = stack.top();
                if (preorder[v] == 0)
                {
                    preorder[v] = ++i;
                }
                bool done = true;
                for (int w : g_succ[v])
                {
                    if (scc_color[v] == scc_color[w] && preorder[w] == 0)
                    {
                        stack.push(w);
                        done = false;
                        break;
                    }
                }
                if (done)
                {
                    lowlink[v] = preorder[v];
                    for (int w : g_succ[v])
                    {
                        if (scc_color[v] == scc_color[w] && scc_found.find(w) == scc_found.end())
                        {
                            lowlink[v] = preorder[w] > preorder[v] ? min(lowlink[v], lowlink[w]) : min(lowlink[v], preorder[w]);
                        }
                    }
                    stack.pop();
                    if (lowlink[v] == preorder[v])
                    {
                        unordered_set<int> scc;
                        scc_color[v] = cur_scc_color;
                        scc.insert(v);
                        while (!scc_stack.empty() && preorder[scc_stack.top()] > preorder[v])
                        {
                            int k = scc_stack.top();
                            scc_stack.pop();
                            scc_color[k] = cur_scc_color;
                            scc.insert(k);
                        }
                        cur_scc_color++;
                        if (scc.size() > div_scc_threshold)
                            scc_results.push_back(scc);
                        scc_found.insert(scc.begin(), scc.end());
                    }
                    else
                    {
                        scc_stack.push(v);
                    }
                }
            }
        }
    }

#ifdef TEST
    cout << "SCC " << (double)(clock() - scc_time) / CLOCKS_PER_SEC << "s" << endl;
#endif

    return scc_results;
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
    // 3512444
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
    vector<vector<int>> g_succ(id_num);
    succ_begin.resize(id_num);
    vector<bool> visited(id_num);
    vector<vector<vector<int>>> results(5);

    vector<unordered_map<int, vector<int>>> two_uj(id_num);

    vector<bool> reachable(id_num);
    vector<int> currentJs(id_num);

    // scc: strongly_connected_components
    int div_scc_freq = 1000;     // divide scc every div_scc_freq, range in [1, id_num / 2]
    int div_scc_threshold = 2; // divide scc if num of nodes in scc more than div_scc_threshold, range in [2, id_num]
    int cur_scc_color = 1;      // using different number to color different scc
    vector<int> scc_color(id_num, 0);

    for (int i = 0; i < ids.size(); i++)
    {
        id_hash[ids[i]] = i;
    }

    for (int i = 0; i < edge_num; i++)
    {
        int u = id_hash[u_ids[i]], v = id_hash[v_ids[i]];
        g_succ[u].push_back(v);
    }

    for (int i = 0; i < id_num; i++)
    {
        sort(g_succ[i].begin(), g_succ[i].end());
        succ_begin[i] = lower_bound(g_succ[i].begin(), g_succ[i].end(), i) - g_succ[i].begin();
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
        vector<int> &succ1 = g_succ[u];
        for (int &k : succ1)
        {
            // two steps succ
            vector<int> &succ2 = g_succ[k];
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

    vector<unordered_set<int>> sccs = first_strongly_connected_components(cur_scc_color, id_num, g_succ, scc_color, div_scc_threshold);
    vector<unordered_set<int>> temp_sccs, next_sccs;
    for (int start = 0; start < id_num; ++start)
    { 

#ifdef TEST
        if (start % 100 == 0)
        {
            cout << start << "/" << id_num << " ~ " << ansCnt << endl;
        }
#endif
        if ((start + 1) % div_scc_freq == 0)
        {
            int sccs_size = sccs.size();
            for (int i = 0; i < sccs_size; ++i)
            {
                temp_sccs = strongly_connected_components(cur_scc_color, sccs[i], g_succ, scc_color, div_scc_threshold, start);
                next_sccs.insert(next_sccs.end(), temp_sccs.begin(), temp_sccs.end());
                temp_sccs.clear();
            }
            sccs.clear();
            sccs = next_sccs;
            next_sccs.clear();
        }

        for (unordered_map<int, vector<int>>::iterator js = two_uj[start].begin(); js != two_uj[start].end(); ++js)
        {
            int j = js->first;
            reachable[j] = true;
            currentJs.push_back(j);
        }

        dfs(start, start, 0, ansCnt, g_succ, visited, path, results, two_uj, reachable, currentJs, scc_color);

        for (int &j : currentJs)
        {
            reachable[j] = false;
        }
        currentJs.clear();

        scc_color[start] = 0;
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