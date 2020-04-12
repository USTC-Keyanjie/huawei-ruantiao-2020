// submit: 1. delete #define TEST. 2. open //#include <bits/stdc++.h>
#define TEST

#ifdef TEST
#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <ctime>
#include <string>
#endif
//#include <bits/stdc++.h>
using namespace std;


bool reverse_adj(int a, int b)
{
	return a > b;
}

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
	cout << "write " << (double)(clock() - write_time) / CLOCKS_PER_SEC << "s" << endl;
#endif

}


vector<vector<int> > get_paths(int start, int cur_id, vector<unordered_map<int, vector<int> > > &two_uj)
{
	vector<int> path;
	vector<vector<int> > paths;

	path.push_back(cur_id);

	// get path num of two_uj[start][next].size()
	for (int &k : two_uj[start][cur_id])
	{
		path.push_back(k);
		paths.push_back(path);
		path.pop_back();
	}

	sort(paths.begin(), paths.end());
	return paths;
}

void dfs_3(int start, int &ansCnt, vector<vector<int> > &one_dj, vector<vector<vector<int> > > &results, vector<unordered_map<int, vector<int> > > &two_uj)
{
	vector<int> left_path;
	left_path.push_back(start);

	for (vector<int>::reverse_iterator it = one_dj[start].rbegin(); it != one_dj[start].rend(); ++it)
	{
		vector<vector<int> > paths = get_paths(start, *it, two_uj);
		// find a circuit
		if (paths.size())
		{
			for (vector<int> &right_path : paths)
			{
				vector<int> path;
				path.insert(path.end(), left_path.begin(), left_path.end());
				path.insert(path.end(), right_path.begin(), right_path.end());
				results[0].push_back(path);
			}
			ansCnt += paths.size();
		}
	}
}


vector<vector<int> > get_paths_visited(int start, int cur_id, vector<bool> &visited, vector<unordered_map<int, unordered_map<int, vector<int> > > > &three_uj)
{
	vector<int> path;
	vector<vector<int> > paths;

	path.push_back(cur_id);

	for (unordered_map<int, vector<int>>::iterator k = three_uj[start][cur_id].begin(); k != three_uj[start][cur_id].end(); ++k)
	{
		if (!visited[k->first])
		{
			path.push_back(k->first);
			for (int &l : k->second)
			{
				if (!visited[l])
				{
					path.push_back(l);
					paths.push_back(path);
					path.pop_back();
				}
			}
			path.pop_back();
		}
	}

	sort(paths.begin(), paths.end());
	return paths;
}

// handle [4, 7] circuit length
void dfs_4567(int start, int id_num, int &ansCnt, vector<vector<int> > &one_dj, vector<vector<vector<int> > > &results, vector<unordered_map<int, unordered_map<int, vector<int> > > > &three_uj)
{
	vector<int> left_path;
	vector<bool> visited(id_num, false);
	vector<vector<int> > stack;

	int cur_id = start, next_id;
	visited[start] = true;
	left_path.push_back(start);
	stack.push_back(one_dj[start]);


	while (!stack.empty())
	{
		vector<vector<int> >::reverse_iterator nbrs = stack.rbegin();

		// has other nbrs
		if (!nbrs->empty())
		{
			next_id = *(nbrs->rbegin());
			nbrs->pop_back();

			// cur_id has been in path
			if (visited[next_id] || next_id <= start) continue;

			vector<vector<int> > paths = get_paths_visited(start, next_id, visited, three_uj);

			// find a circuit
			if (paths.size())
			{
				for (vector<int> &right_path : paths)
				{
					vector<int> path;
					path.insert(path.end(), left_path.begin(), left_path.end());
					path.insert(path.end(), right_path.begin(), right_path.end());
					results[path.size() - 3].push_back(path);
				}
				ansCnt += paths.size();
			}

			if (left_path.size() < 4)
			{
				cur_id = next_id;
				visited[cur_id] = true;
				left_path.push_back(cur_id);
				stack.push_back(one_dj[cur_id]);
			}
		}
		// no left nbr
		else
		{
			visited[cur_id] = false;
			left_path.pop_back();
			stack.pop_back();
			if (left_path.size())
				cur_id = *left_path.rbegin();
		}
	}
}



int main()
{
	int id_num = 0;
	int edge_num = 0;
	int ansCnt = 0;

	// input
	string testFile = "/data/test_data.txt";
	string resultFile = "/projects/student/result.txt";

	set<int> ids_set;
	vector<int> ids;  // regular-id(0, 1, 2, ..., n-1) to really-id
	unordered_map<int, int> id_hash;  // really-id to regular-id(0, 1, 2, ..., n-1)

#ifdef TEST
	string dataset = "3512444";
	testFile = "test_data/" + dataset + "/test_data.txt";
	resultFile = "test_data/" + dataset + "/result.txt";
	clock_t start_time = clock();
#endif

	FILE* file = fopen(testFile.c_str(), "r");

	int* u_ids = (int*)malloc(sizeof(int) * 280000);
	int* v_ids = (int*)malloc(sizeof(int) * 280000);

	while (fscanf(file, "%d,", u_ids + edge_num) != EOF) {
		fscanf(file, "%d,", v_ids + edge_num);
		ids_set.insert(u_ids[edge_num]);
		ids_set.insert(v_ids[edge_num++]);
		while (fgetc(file) != '\n');
	}

	ids = vector<int>(ids_set.begin(), ids_set.end());
	id_num = ids.size();

	// define data structure
	vector<string> idsComma(id_num); //0...n to sorted id
	vector<string> idsLF(id_num); //0...n to sorted id

	vector<vector<int> > one_dj(id_num);
	vector<vector<vector<int> > > results(5);

	vector<unordered_map<int, vector<int> > > two_uj(id_num);
	vector<unordered_map<int, unordered_map<int, vector<int> > > > three_uj(id_num);

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
		sort(one_dj[i].begin(), one_dj[i].end(), reverse_adj);
		idsComma[i] = to_string(ids[i]) + ",";
		idsLF[i] = to_string(ids[i]) + "\n";
	}

#ifdef TEST
	clock_t three_uj_time = clock();
#endif

	// 3 up steps jump table
	for (int u = 0; u < id_num; u++)
	{
		// one step succ
		vector<int> &succ1 = one_dj[u];
		for (int &k : succ1)
		{
			// two steps succ
			vector<int> &succ2 = one_dj[k];
			for (int &l : succ2)
			{
				if (k > l && u > l)
					two_uj[l][u].push_back(k);

				// three steps succ
				vector<int> &succ3 = one_dj[l];
				for (int &v : succ3)
				{
					if (l > v && k > v && u > v && u != l)
						three_uj[v][u][k].push_back(l);
				}
			}
		}
	}

#ifdef TEST
	cout << "construct jump table " << (double)(clock() - three_uj_time) / CLOCKS_PER_SEC << "s" << endl;
	clock_t search_time = clock();
#endif

	
	for (int start = 0; start < id_num; ++start)
	{

#ifdef TEST
		if (start % 100 == 0) {
			cout << start << "/" << id_num << " ~ " << ansCnt << endl;
		}
#endif

		dfs_3(start, ansCnt, one_dj, results, two_uj);
		dfs_4567(start, id_num, ansCnt, one_dj, results, three_uj);
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