        
        //the Johnson's circuit finding algorithm
#include<iostream>
#include<list>
#include<vector>
#include<map>
#include<fstream>
#include<unordered_map>
#include<unordered_set>
#include<ctime>
#include<algorithm>
using namespace std;

typedef vector<int> VecInt;
typedef list<int> NodeList;
// typedef unordered_set<int> uSet;
typedef unordered_map<int, bool> MapBool;
typedef unordered_map<int, int> uMap;
typedef vector<vector<int>> Result;
typedef map<int, vector<int>> Graph; 
typedef unordered_map<int, NodeList> MapNodeList;

inline bool cmp(pair<int, int>& a, pair<int, int>& b)
{
    return a.second <= b.second;
}

inline bool my_compare(const VecInt& a, const VecInt& b)
{
    return a.size() < b.size() || (a.size() == b.size() && a <= b);
}

// 并查集 find 和 union
inline int uf_find(int x, uMap& id_root)
{   //find 的 非递归形式
    int y = x;
    while(x != id_root[x])
        x = id_root[x];
    
    while(y != x)
    {
        int temp = id_root[y];
        id_root[y] = x;
        y = temp;
    }
    return x;
}

inline void to_union(int p, int q, uMap& id_root, uMap& rank)
{
    int x = uf_find(p, id_root);
    int y = uf_find(q, id_root);

    if(x == y)
        return;
    if(rank[x] < rank[y])
        id_root[x] = y;
    else
    {
        id_root[y] = x;
        if(rank[x] == rank[y])
            rank[x]++;
    }
}

struct Edge
{
    int x;
    int y;
};

void create_graph(Graph& graph, vector<Edge>& edge, uMap& id_root, uMap& rank,const string& datafile)
{
    fstream infile;
    infile.open(datafile, ios::in);
    if(!infile.is_open())
        std::cerr<< "Open file failure\n";
    int id1, id2, money;
    char ch;
    while (!infile.eof())
    {
        infile >> id1 >> ch >> id2 >> ch >> money;
        graph[id1].push_back(id2);

        id_root[id1] = id1;
        id_root[id2] = id2;
        rank[id1] = 0;
        rank[id2] = 0;
        Edge e;
        e.x = id1;
        e.y = id2;
        edge.push_back(e);
    }
    infile.close();
}

void unblock(int U, MapBool& Blocked_map, MapNodeList& B_map)
{
    Blocked_map[U] = false;

    while(!B_map[U].empty())
    {
        int W = B_map[U].front();
        B_map[U].pop_front();

        if(Blocked_map[W])
            unblock(W, Blocked_map, B_map);
    }
}

bool circuit(int v, int S, Graph& graph, VecInt& Stack, MapBool& Blocked_map, MapNodeList& B_map, Result& res)
{
    bool F = false;
    Stack.push_back(v);
    Blocked_map[v] = true;

    for(auto e : graph[v])
    {   
        if(e == S)
        {
            F = true;
            if(Stack.size() >= 3 && Stack.size() <= 7)
            {
                res.push_back(Stack);
            }
        }
        else if(e > S && !Blocked_map[e])
        {   
            // F = circuit(e, S, graph, Stack, Blocked_map, B_map, res);
            if(circuit(e, S, graph, Stack, Blocked_map, B_map, res))
                F = true;
        }
    }
    if(F)
    {   
        unblock(v, Blocked_map, B_map);
    }
    else
    {
        for(auto e : graph[v])
        {
            auto IT = find(B_map[e].begin(), B_map[e].end(), v);
            if(IT == B_map[e].end())
            {
                B_map[e].push_back(v);
            }
        }
    }
    Stack.pop_back();
    return F;
}

void find_all_curcuits(Graph& graph, VecInt& Stack, MapBool& Blocked_map, MapNodeList& B_map, Result& res)
{

    auto graph_end_it = graph.end();
    for(auto graph_it = graph.begin(); graph_it != graph_end_it; graph_it++)
    {   
        for(auto it = graph_it; it != graph.end(); ++it)
        {
            Blocked_map[it->first] = false;
            B_map[it->first].clear();
        }
        circuit(graph_it->first, graph_it->first, graph, Stack, Blocked_map, B_map, res);
    }
}

void write_result(string& resultfile, Result& result)
{
    ofstream outfile;
    
    outfile.open(resultfile, ios::out|ios::app);

    if(!outfile.is_open())
        std::cout<< "Open file failure\n";
    
    outfile << result.size() <<'\n';
    for(int i = 0; i < result.size(); ++i)
    {
        for(auto elem : result[i])
        {
            if(elem != result[i].back())
                outfile << elem <<',';
            else
                outfile << elem <<'\n';
        }
    }
    outfile.close();
}

int main()
{
    clock_t start_time = clock();
    string datafile = "test_data2.txt";
    string resultfile = "test_result2.txt";
    Graph graph;
    VecInt Stack;
    MapBool Blocked_map;
    MapNodeList B_map;
    Result res;
    uMap id_root;
    uMap rank;
    vector<Edge> edge;

    //使用邻接表 保存图 map<int, vector<int>>
    create_graph(graph, edge, id_root, rank, datafile);

    for(auto e : edge)
    {
        to_union(e.x, e.y, id_root, rank);
    }
    
    for(auto e : edge)
    {
        int id1_root = uf_find(e.x, id_root);
    }

    Graph g;

    for(auto e : id_root)
    {
        g[e.second].push_back(e.first);
    }

    for(auto sub_graph_set : g)
    {
        if(sub_graph_set.second.size() < 3)
            continue;
        Graph sub_graph;
        
        for(auto e : sub_graph_set.second)
        {   
            sub_graph[e] = graph[e];
        }
        Stack.clear();
        Blocked_map.clear();
        B_map.clear();
        
        // Johnson算法花费的时间最多
        find_all_curcuits(sub_graph, Stack, Blocked_map, B_map, res);
    }
    
    // find_all_curcuits(graph, Stack, Blocked_map, B_map, res);
 
    //按照比赛要求排序
    sort(res.begin(), res.end(), my_compare);

    //将结果写入文件
    write_result(resultfile, res);

    cout << "total time :"<<double(clock() - start_time)<<"ms"<<"\n";
}