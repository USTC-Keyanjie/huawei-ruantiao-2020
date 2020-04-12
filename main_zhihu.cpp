#include <iostream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <fstream>
#include <ctime>
#include <string>
#include <queue>
using namespace std;

#define TEST

struct Path {
	//ID最小的第一个输出；
	//总体按照循环转账路径长度升序排序；
	//同一级别的路径长度下循环转账账号ID序列，按照字典序（ID转为无符号整数后）升序排序
	int length;
	vector<int> path;

	Path(int length, const vector<int> &path) : length(length), path(path) {}

	bool operator<(const Path&rhs)const {
		if (length != rhs.length) return length < rhs.length;
		for (int i = 0; i < length; i++) {
			if (path[i] != rhs.path[i])
				return path[i] < rhs.path[i];
		}
	}
};

#define DEPTH_HIGH_LIMIT 7
#define DEPTH_LOW_LIMIT 3
class Solution {
public:
	//maxN=560000
	//maxE=280000 ~avgN=28000
	//vector<int> *G;
	vector<vector<int>> G;
	unordered_map<int, int> idHash; //sorted id to 0...n
	vector<string> idsComma; //0...n to sorted id
	vector<string> idsLF; //0...n to sorted id
	vector<int> inputs; //u-v pairs
	vector<int> inDegrees;
	vector<int> outDegrees;
	vector<bool> vis;
	vector<Path> ans[8];
	vector<int> reachable;

	//P2inv[end][mid][k]表示结点mid到达结点end，中间经过结点k的路径详情
	//estimate size 28000*[100,200]*[1,5]
	vector<unordered_map<int, vector<int>>> P2inv;
	int nodeCnt;
	int ansCnt;

	//TODO:mmap
	void parseInput(string &testFile) {
		FILE* file = fopen(testFile.c_str(), "r");
		int u, v, c;
		int cnt = 0;
		while (fscanf(file, "%d,%d,%d", &u, &v, &c) != EOF) {
			inputs.push_back(u);
			inputs.push_back(v);
			++cnt;
		}
#ifdef TEST
		printf("%d Records in Total\n", cnt);
#endif
	}

	void constructGraph() {
		auto tmp = inputs;
		sort(tmp.begin(), tmp.end());
		tmp.erase(unique(tmp.begin(), tmp.end()), tmp.end());
		nodeCnt = tmp.size();
		idsComma.reserve(nodeCnt);
		idsLF.reserve(nodeCnt);
		nodeCnt = 0;
		for (int &x : tmp) {
			idsComma.push_back(to_string(x) + ',');
			idsLF.push_back(to_string(x) + '\n');
			idHash[x] = nodeCnt++;
		}
#ifdef TEST
		printf("%d Nodes in Total\n", nodeCnt);
#endif
		int sz = inputs.size();
		//G=new vector<int>[nodeCnt];
		G = vector<vector<int>>(nodeCnt);
		inDegrees = vector<int>(nodeCnt, 0);
		outDegrees = vector<int>(nodeCnt, 0);
		for (int i = 0; i < sz; i += 2) {
			int u = idHash[inputs[i]], v = idHash[inputs[i + 1]];
			G[u].push_back(v);
			++inDegrees[v];
			++outDegrees[u];
		}
	}

	void topoSort(vector<int> &degs, bool doSoring) {
		queue<int> que;
		for (int i = 0; i < nodeCnt; i++) {
			if (0 == degs[i])
				que.push(i);
		}
		while (!que.empty()) {
			int u = que.front();
			que.pop();
			for (int &v : G[u]) {
				if (0 == --degs[v])
					que.push(v);
			}
		}
		int cnt = 0;

		for (int i = 0; i < nodeCnt; i++) {
			if (degs[i] == 0) {
				G[i].clear();
				cnt++;
			}
			else if (doSoring) {
				sort(G[i].begin(), G[i].end());
			}
		}
#ifdef TEST
		printf("%d Nodes eliminated in TopoSort\n", cnt);
#endif
	}

	void constructP2() {
		clock_t two_uj_time = clock();
		P2inv = vector<unordered_map<int, vector<int>>>(nodeCnt, unordered_map<int, vector<int>>());
		for (int i = 0; i < nodeCnt; i++) {
			auto &gi = G[i];
			for (int &k : gi) {
				auto &gk = G[k];
				for (int &j : gk) {
					if (j != i) {
						P2inv[j][i].push_back(k);
					}
				}
			}

		}
		for (int i = 0; i < nodeCnt; i++) {
			for (auto &x : P2inv[i]) {
				if (x.second.size() > 1) {
					sort(x.second.begin(), x.second.end());
				}
			}
		}

		cout << "跳表： " << (double)(clock() - two_uj_time) / CLOCKS_PER_SEC << "s" << endl;
	}

	void dfs(int head, int cur, int depth, vector<int> &path) {
		vis[cur] = true;
		path.push_back(cur);
		auto &gCur = G[cur];
		auto it = lower_bound(gCur.begin(), gCur.end(), head);
		//handle [3,6]
		if (it != gCur.end() && *it == head && depth >= DEPTH_LOW_LIMIT && depth < DEPTH_HIGH_LIMIT) {
			ans[depth].emplace_back(Path(depth, path));
			++ansCnt;
		}

		if (depth < DEPTH_HIGH_LIMIT - 1) {
			for (; it != gCur.end(); ++it) {
				if (!vis[*it]) {
					dfs(head, *it, depth + 1, path);
				}
			}
		}
		else if (reachable[cur] > -1 && depth == DEPTH_HIGH_LIMIT - 1) { //handle [7]
			auto ks = P2inv[head][cur];
			int sz = ks.size();
			for (int idx = reachable[cur]; idx < sz; ++idx) {
				int k = ks[idx];
				if (vis[k]) continue;
				auto tmp = path;
				tmp.push_back(k);
				ans[depth + 1].emplace_back(Path(depth + 1, tmp));
				++ansCnt;
			}
		}
		vis[cur] = false;
		path.pop_back();
	}

	//search from 0...n
	//由于要求id最小的在前，因此搜索的全过程中不考虑比起点id更小的节点
	void solve() {
		ansCnt = 0;
		constructP2();
		vis = vector<bool>(nodeCnt, false);
		vector<int> path;
		reachable = vector<int>(nodeCnt, -1);
		vector<int> currentJs(nodeCnt);

		clock_t start_time = clock();

		for (int i = 0; i < nodeCnt; ++i) {
#ifdef TEST
			if (i % 100 == 0) {
				cout << i << "/" << nodeCnt << " ~ " << ansCnt << endl;
			}
#endif
			if (!G[i].empty()) {
				//可以通过大于head的id返回的
				for (auto &js : P2inv[i]) {
					int j = js.first;
					if (j > i) {
						auto &val = js.second;
						int sz = val.size();
						int lb = lower_bound(val.begin(), val.end(), i) - val.begin();
						if (lb < val.size()) reachable[j] = lb;
						currentJs.push_back(j);
					}
				}
				dfs(i, i, 1, path);
				for (int &x : currentJs)
					reachable[x] = -1;
				currentJs.clear();
			}
		}
		cout << "搜索： " << (double)(clock() - start_time) / CLOCKS_PER_SEC << "s" << endl;

#ifdef TEST
		for (int i = DEPTH_LOW_LIMIT; i <= DEPTH_HIGH_LIMIT; i++) {
			printf("Loop Size %d: %d/%d ~ %.3lf\n", i, ans[i].size(), ansCnt, ans[i].size()*1.0 / ansCnt);
		}
#endif
	}

	void save(const string &outputFile) {
		auto t = clock();
		ofstream out;
		out.open(outputFile);
		//      open(outputFile, O_RDWR | O_CREAT,0666);
#ifdef TEST
		printf("Total Loops %d\n", ansCnt);
#endif
		out << ansCnt << "\n";
		for (int i = DEPTH_LOW_LIMIT; i <= DEPTH_HIGH_LIMIT; i++) {
			//sort(ans[i].begin(),ans[i].end());
			for (auto &x : ans[i]) {
				auto path = x.path;
				int sz = path.size();
				for (int j = 0; j < sz - 1; j++)
					out << idsComma[path[j]];
				out << idsLF[path[sz - 1]];
			}
		}
		cout << clock() - t << endl;
	}

	void save_fputs(const string &outputFile) {
		auto t = clock();
		FILE *fp = fopen(outputFile.c_str(), "w");
		char buf[1024];

#ifdef TEST
		printf("Total Loops %d\n", ansCnt);
#endif
		int idx = sprintf(buf, "%d\n", ansCnt);
		buf[idx] = '\0';
		fputs(buf, fp);
		for (int i = DEPTH_LOW_LIMIT; i <= DEPTH_HIGH_LIMIT; i++) {
			//sort(ans[i].begin(),ans[i].end());
			for (auto &x : ans[i]) {
				auto path = x.path;
				int sz = path.size();
				idx = 0;
				for (int j = 0; j < sz - 1; j++) {
					idx += sprintf(buf + idx, "%s", idsComma[path[j]].c_str());
				}
				idx += sprintf(buf + idx, "%s", idsLF[path[sz - 1]].c_str());
				buf[idx] = '\0';
				fputs(buf, fp);
			}
		}
		fclose(fp);
		cout << clock() - t << endl;
	}

	void save_fwrite(const string &outputFile) {
		clock_t write_time = clock();
		auto t = clock();
		FILE *fp = fopen(outputFile.c_str(), "wb");
		char buf[1024];

#ifdef TEST
		printf("Total Loops %d\n", ansCnt);
#endif
		int idx = sprintf(buf, "%d\n", ansCnt);
		buf[idx] = '\0';
		fwrite(buf, idx, sizeof(char), fp);
		for (int i = DEPTH_LOW_LIMIT; i <= DEPTH_HIGH_LIMIT; i++) {
			//sort(ans[i].begin(),ans[i].end());
			for (auto &x : ans[i]) {
				auto path = x.path;
				int sz = path.size();
				//idx=0;
				for (int j = 0; j < sz - 1; j++) {
					auto res = idsComma[path[j]];
					fwrite(res.c_str(), res.size(), sizeof(char), fp);
				}
				auto res = idsLF[path[sz - 1]];
				fwrite(res.c_str(), res.size(), sizeof(char), fp);
			}
		}
		fclose(fp);
		cout << clock() - t << endl;
		cout << "写入： " << (double)(clock() - write_time) / CLOCKS_PER_SEC << "s" << endl;
	}
};

int main()
{
	string testFile = "test_data/2861665/test_data.txt";
	string outputFile = "test_data/2861665/result.txt";
#ifdef TEST
	string answerFile = "test_data/2861665/answer.txt";
#endif
	auto t = clock();
	//    for(int i=0;i<100;i++){
	Solution solution;
	solution.parseInput(testFile);
	solution.constructGraph();
	solution.topoSort(solution.inDegrees, false);
	solution.topoSort(solution.outDegrees, true);
	solution.solve();
	solution.save_fwrite(outputFile);

	cout << clock() - t << endl;
	//    }

	return 0;
}