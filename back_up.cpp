    // fscanf输入 还有问题
    /*
    start = clock();
    fin = fopen("test_data1.txt", "r");
  	while (fscanf(fin, "%u", &s_id) != EOF)
	{
        fseek(fin, 1, 1);
        fscanf(fin, "%u", &d_id);
        fseek(fin, 2, 1);
        while (fgetc(fin) != '\n');
        g[s_id].push_back(d_id);
	}
    end = clock();
    cout << "花费了" << (double)(end - start) << "ms" << endl;
    */

// 打印强连通分量
   /*
    for (unordered_set<int> scc : scc_results)
    {
        for (int i : scc)
        {
            cout << i << " ";
        }
        cout << endl;
    }
    cout << endl;
    cout << scc_results.size() << endl;
    */

/* 备份
void search_result(int num_jump, int start, int pred)
{
	vector<int> path;
	vector<vector<int> > stack;

	int pre_id, cur_jump = num_jump - 1;
	path.push_back(pred);
	stack.push_back(mul_jump[cur_jump][start][pred]);

	while (!stack.empty())
	{
		vector<vector<int> >::reverse_iterator preds = stack.rbegin();

		if (!preds->empty())
		{
			pre_id = *(preds->begin());
			preds->erase(preds->begin());

			if (pre_id != start && find(path.begin(), path.end(), pre_id) == path.end())
			{
				path.push_back(pre_id);
				// 深搜正常出口
				if (cur_jump == 2)
				{
					path.push_back(start);
					save_result(path);
					path.pop_back();
				}
				else
				{
					stack.push_back(mul_jump[--cur_jump][start][pre_id]);
				}
			}
			else
			{
				// 出现提前闭合的边，异常出口 或者 提前到终点
				stack.pop_back();
				path.pop_back();
				cur_jump++;
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
*/

/* 非递归形式的dfs，效率不高
void dfs(int start, int id_num, int &ansCnt, vector<vector<int> > &one_dj, vector<vector<vector<int> > > &results, vector<unordered_map<int, vector<int> > > &two_uj)
{
	vector<int> left_path;
	vector<vector<int> > paths;
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

			paths = get_paths(start, next_id, visited, two_uj);

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

			if (left_path.size() < 5)
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
*/