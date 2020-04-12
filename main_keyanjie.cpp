#include <iostream>
#include <unordered_map>
#include <queue>
#include <stack>
#include <vector>
#include <unordered_set>
#include <ctime>
#include <algorithm>
using namespace std;

unordered_map<int, vector<int> > adj_m;
unordered_map<int, vector<int> > pre;
unordered_map<int, int> visited;  // 0表示没有访问过，1表示访问过
vector<vector<int> > results;

void add_result_dfs(
	int cur_level,
	int stop_level,
	int pre_id,
	vector<int> &result_temp,
	vector<unordered_map<int, vector<int> > > &seven_level)
{
	if (cur_level == stop_level)
	{
		// temp_res要逆序
		vector<int> result(result_temp.begin(), result_temp.end());
		//reverse(result.begin(), result.end());

		// 最小id放最前面
		int min_id = result[0], min_id_index = 0;
		for (int i = 1; i < result.size(); i++)
		{
			if (result[i] < min_id)
			{
				min_id = result[i];
				min_id_index = i;
			}
		}

		reverse(result.begin(), result.begin() + min_id_index + 1);
		reverse(result.begin() + min_id_index + 1, result.end());

		results.push_back(result);
		return;
	}


	for (int i = 0; i < seven_level[cur_level][pre_id].size(); i++)
	{
		result_temp.push_back(seven_level[cur_level][pre_id][i]);
		add_result_dfs(cur_level - 1, stop_level, seven_level[cur_level][pre_id][i], result_temp, seven_level);
		result_temp.pop_back();
	}


}

void add_result(
	int cur_level,
	int stop_level,
	int start_id,
	int pre_id,
	vector<unordered_map<int, vector<int> > > &seven_level)
{
	// 使用dfs添加result
	vector<int> result_temp;
	result_temp.push_back(pre_id);

	cur_level--;
	for (int i = 0; i < seven_level[cur_level][pre_id].size(); i++)
	{
		result_temp.push_back(seven_level[cur_level][pre_id][i]);
		add_result_dfs(cur_level - 1, stop_level, seven_level[cur_level][pre_id][i], result_temp, seven_level);
		result_temp.pop_back();
	}

}

/*
  param:
  start_id 本次最先考虑的id
  cur_level 当前层数
  begin_id 上一层传递过来的id
*/
void bfs_add(
	int start_id,
	int cur_level,
	int pre_id,
	unordered_map<int, int> &delete_edge_record,
	unordered_map<int, int> &id_record,
	vector<unordered_map<int, vector<int> > > &seven_level)
{
	for (int i = 0; i < adj_m[pre_id].size(); i++)
	{
		int end_id = adj_m[pre_id][i];
		// 遇到自身了, visited要置0
		if (end_id == start_id && cur_level > 2)
		{
			visited[start_id] = 0;
			add_result(cur_level, 0, start_id, pre_id, seven_level);
			delete_edge_record[pre_id] = start_id;
		}
		// 遇到曾经碰到过的节点
		else if (id_record[end_id] != 0 && cur_level - id_record[end_id] > 2)
		{
			visited[end_id] = 0;
			add_result(cur_level, id_record[end_id], end_id, pre_id, seven_level);
			delete_edge_record[pre_id] = end_id;
		}
		else if (visited[end_id] == 1)
		{
			continue;
		}
		else
		{
			seven_level[cur_level][end_id].push_back(pre_id);
			id_record[end_id] = cur_level;
		}
		
	}
}

void delete_edge(unordered_map<int, int> &delete_edge_record)
{

	for (auto it = delete_edge_record.begin(); it != delete_edge_record.end(); it++)
	{
		int begin_id = it->first;
		int end_id = it->second;
		int j;
		for (j = 0; j < adj_m[begin_id].size(); j++)
		{
			if (adj_m[begin_id][j] == end_id)
			{
				break;
			}
		}
		adj_m[begin_id].erase(adj_m[begin_id].begin() + j);
	}
}

bool cmp(vector<int> a, vector<int> b)
{
	if (a.size() != b.size())
	{
		return a.size() < b.size();
	}
	else
	{
		for (int i = 0; i < a.size(); i++)
		{
			if (a[i] != b[i])
				return a[i] < b[i];
		}
	}
	return true;
}

int main()
{
	clock_t start = clock();

	vector<unordered_map<int, vector<int> > > data(8);
	data[0][1] = vector<int>(1);

	freopen("test_data2.txt", "r", stdin);
	int s_id, d_id, money;


	while (scanf("%d,%d,%d", &s_id, &d_id, &money) != EOF)
	{
		pre[d_id].push_back(s_id);
		adj_m[s_id].push_back(d_id);
		visited[s_id] = 0;
		visited[d_id] = 0;
		//printf("%d,%d,%d\n", s_id, d_id, money);
	}

	int index_sign = 0;
	// 逐节点遍历
	for (auto it = visited.begin(); it != visited.end(); it++)
	{
		cout << index_sign << endl;
		index_sign++;
		if (it->second == 0)
		{
			int cur_id = it->first;
			it->second = 1;
			vector<unordered_map<int, vector<int> > > seven_level(8);
			unordered_map<int, int> delete_edge_record;
			unordered_map<int, int> id_record;  // 记录已经访问过的id，key<int>表示id号，value<int>表示在第几层访问过

			seven_level[0][cur_id] = vector<int>(0);

			for (int i = 1; i <= 7; i++)
			{
				for (auto pre_level_iter = seven_level[i - 1].begin(); pre_level_iter != seven_level[i - 1].end(); pre_level_iter++)
				{
					bfs_add(cur_id, i, pre_level_iter->first, delete_edge_record, id_record, seven_level);
				}
			}
			delete_edge(delete_edge_record);
		}
	}

	sort(results.begin(), results.end(), cmp);
	clock_t end = clock();
	cout << "花费了" << (double)(end - start) << "ms" << endl;

	cout << results.size() << endl;
	/*
	for (int i = 0; i < results.size(); i++)
	{
		cout << results[i][0];
		for (int j = 1; j < results[i].size(); j++)
		{
			cout << "," << results[i][j];
		}
		cout << endl;
	}
	*/
	
	
	return 0;
}
