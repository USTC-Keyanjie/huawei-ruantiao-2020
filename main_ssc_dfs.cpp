#include <iostream>
#include <fstream> 
#include <stdlib.h>
#include <ctime>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <queue>
#include <stack>
#include <algorithm>

#define MAX_NUM_RECORD 280000 //最大的转账条目
#define MAX_NUM_RESULT 3000000 //最大结果数
using namespace std;

typedef unordered_map<int, vector<int> > Graph;
typedef vector<vector<int> > Result;

string root = "test_data/";

string input_file;
string out_file;

Graph g_succ;
Graph g_pred;

unordered_set<int> nodes;

Result circuit_results_3;
Result circuit_results_4;
Result circuit_results_5;
Result circuit_results_6;
Result circuit_results_7;

clock_t start_time;
clock_t end_time;

int count_res = 0;
bool record_time = true;

void add_result(vector<int> result, Result &circuit_results)
{
	reverse(result.begin(), result.end());

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

	circuit_results.push_back(result);

}

void input_data()
{
	start_time = clock();
	unsigned int s_id, d_id, money;

	fstream infile;
	infile.open(input_file, ios::in);
	if (!infile.is_open())
		std::cerr << "Open file failure\n";
	char ch;
	while (!infile.eof())
	{
		infile >> s_id >> ch >> d_id >> ch >> money;
		g_succ[s_id].push_back(d_id);
		g_pred[d_id].push_back(s_id);
		nodes.insert(s_id);
		nodes.insert(d_id);
	}
	infile.close();
	end_time = clock();
	if (record_time) cout << "输入 " << (double)(end_time - start_time) / CLOCKS_PER_SEC << "s" << endl;
}

void write_result()
{
	start_time = clock();
	ofstream outfile;

	outfile.open(out_file, ios::out | ios::app);

	if (!outfile.is_open())
		std::cout << "Open file failure\n";
	outfile << circuit_results_3.size() + circuit_results_4.size() + circuit_results_5.size() + circuit_results_6.size() + circuit_results_7.size() << endl;

	for (vector<int> circuit : circuit_results_3)
	{
		outfile << circuit[0];
		for (int i = 1; i < circuit.size(); i++)
			outfile << "," << circuit[i];
		outfile << endl;
	}

	for (vector<int> circuit : circuit_results_4)
	{
		outfile << circuit[0];
		for (int i = 1; i < circuit.size(); i++)
			outfile << "," << circuit[i];
		outfile << endl;
	}

	for (vector<int> circuit : circuit_results_5)
	{
		outfile << circuit[0];
		for (int i = 1; i < circuit.size(); i++)
			outfile << "," << circuit[i];
		outfile << endl;
	}

	for (vector<int> circuit : circuit_results_6)
	{
		outfile << circuit[0];
		for (int i = 1; i < circuit.size(); i++)
			outfile << "," << circuit[i];
		outfile << endl;
	}

	for (vector<int> circuit : circuit_results_7)
	{
		outfile << circuit[0];
		for (int i = 1; i < circuit.size(); i++)
			outfile << "," << circuit[i];
		outfile << endl;
	}

	outfile.close();
	end_time = clock();
	if (record_time) cout << "输出 " << (double)(end_time - start_time) / CLOCKS_PER_SEC << "s" << endl;
}

vector<unordered_set<int> > strongly_connected_components(unordered_set<int> &nodes)
{
	start_time = clock();

	unordered_map<int, int> preorder;
	unordered_map<int, int> lowlink;
	unordered_set<int> scc_found;
	stack<int> scc_stack;
	vector<unordered_set<int> > scc_results;

	int i = 0; // Preorder counter
	for (int source : nodes)
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
						scc.insert(v);
						while (!scc_stack.empty() && preorder[scc_stack.top()] > preorder[v])
						{
							int k = scc_stack.top();
							scc_stack.pop();
							scc.insert(k);
						}
						if (scc.size() > 2) scc_results.push_back(scc);
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
	end_time = clock();
	if (record_time) cout << "分割 " << (double)(end_time - start_time) / CLOCKS_PER_SEC << "s" << endl;
	return scc_results;
}

bool cmp(vector<int> a, vector<int> b)
{
	return a < b;
}

vector<int> intersection(unordered_map<int, int> &cur_nodes_map, vector<int> &succ_raw)
{
	vector<int> succ_new;
	for (int s : succ_raw)
	{
		if (cur_nodes_map[s])
		{
			succ_new.push_back(s);
		}
	}
	return succ_new;
}

void dfs(int start_id, unordered_set<int> &cur_nodes)
{
	start_time = clock();
	vector<int> path;
	vector<vector<int> > stack;
	unordered_map<int, bool> visited;
	unordered_map<int, int> cur_nodes_map;

    

	for (int node : cur_nodes)
	{
		cur_nodes_map[node] = 1;
	}


	stack.push_back(g_succ[start_id]);
	path.push_back(start_id);
	int cur_id = start_id, next_id;

	while (!stack.empty())
	{
		vector<vector<int> >::reverse_iterator nbrs = stack.rbegin();

		// 还有邻节点可以搜索
		if (!nbrs->empty())
		{
			next_id = *(nbrs->rbegin());
			nbrs->pop_back();

			// 当前节点已经在环中
			if (visited[next_id]) continue;

			// 形成环了
			if (path.size() > 2 && next_id == start_id)
			{
				// 添加结果
				switch (path.size())
				{
				case 3:
					add_result(path, circuit_results_3);
					break;
				case 4:
					add_result(path, circuit_results_4);
					break;
				case 5:
					add_result(path, circuit_results_5);
					break;
				case 6:
					add_result(path, circuit_results_6);
					break;
				case 7:
					add_result(path, circuit_results_7);
					break;
				default:
					break;
				}
				count_res++;
			}
			else
			{
				if (path.size() < 7)
				{
					visited[next_id] = true;
					path.push_back(next_id);
					vector<int> succ = intersection(cur_nodes_map, g_succ[next_id]);
					// TODO: 这里push_back的应该是g_succ[next_id]与cur_nodes的交集
					stack.push_back(succ);
					cur_id = next_id;
				}
			}
		}
		// 没有邻节点可以搜索
		else
		{
			visited[cur_id] = false;
			path.pop_back();
			stack.pop_back();
			cur_id = cur_id == start_id ? start_id : *path.rbegin();
		}
	}
	end_time = clock();
	if (record_time) cout << "搜索 " << (double)(end_time - start_time) / CLOCKS_PER_SEC << "s " << count_res << endl;
}

void remove_node(int id, unordered_set<int> &cur_nodes)
{
	cur_nodes.erase(id);

	// 删除所有前驱的后继
	vector<int> preds = g_pred[id];
	for (int pred : preds) 
		g_succ[pred].erase(find(g_succ[pred].begin(), g_succ[pred].end(), id));

	// 删除此节点的前驱
	g_pred.erase(id);

	// 删除所有后继的前驱
	vector<int> succs = g_succ[id];
	for (int succ : succs) 
		g_pred[succ].erase(find(g_pred[succ].begin(), g_pred[succ].end(), id));

	// 删除此节点的后继
	g_succ.erase(id);
}

int main()
{
    string dataset = "std";

    input_file = root + dataset + "/test_data.txt";
    out_file = root + dataset + "/result.txt";

	// 输入数据
	input_data();

	// 强连通分量分割
	vector<unordered_set<int> > sccs = strongly_connected_components(nodes);

	int count = 0;
	for (unordered_set<int> scc : sccs)
	{
		count += scc.size();
	}

	cout << nodes.size() - count << endl;
	

	while (!sccs.empty())
	{
		unordered_set<int> cur_nodes = *sccs.rbegin();

		// 寻找出度最大的点
		int max_out_degree = 0, start_id = 0;
		for (int node : cur_nodes)
		{
			if (g_succ[node].size() > max_out_degree)
			{
				max_out_degree = g_succ[node].size();
				start_id = node;
			}
		}



		// dfs搜索环路
		dfs(start_id, cur_nodes);

		// 强连通分量分割
		remove_node(start_id, cur_nodes);

		sccs.pop_back();
		vector<unordered_set<int> > sccs_new = strongly_connected_components(cur_nodes);
		sccs.insert(sccs.end(), sccs_new.begin(), sccs_new.end());
	}

	// 排序结果
	start_time = clock();
	sort(circuit_results_3.begin(), circuit_results_3.end(), cmp);
	sort(circuit_results_4.begin(), circuit_results_4.end(), cmp);
	sort(circuit_results_5.begin(), circuit_results_5.end(), cmp);
	sort(circuit_results_6.begin(), circuit_results_6.end(), cmp);
	sort(circuit_results_7.begin(), circuit_results_7.end(), cmp);
	end_time = clock();
	if (record_time) cout << "排序 " << (double)(end_time - start_time) / CLOCKS_PER_SEC << "s" << endl;

	// 输出结果
	write_result();


	cout << count_res << endl;
	return 0;
}