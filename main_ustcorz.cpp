//vs能编译通过，其他编译器没有试过
#include <iostream>
#include <fstream> 
#include <stdlib.h>
#include <time.h>
constexpr auto NUM_ID = 7000;//NUM_ID应该大于最大的ID数max
constexpr auto NUM_CIR = 230;//环的最大数目（考虑大量重复的环）
constexpr auto LEN = 5100;//LEN应该大于用户的总数
using namespace std;

//定义结构体，全局变量
int visited[NUM_ID];
typedef struct{
	unsigned int data[7];//存一个环
	unsigned int length;//环的长度
}CIR;//一个环的信息
CIR circle[NUM_CIR];//元素为环结构体的数组

//类DFS算法，找出所有环
bool DFS(unsigned int G[][9], unsigned int ID, int numcirl) {
	visited[ID] = numcirl;
	circle[numcirl].data[circle[numcirl].length++] = ID;
	if (circle[numcirl].length == 7) {//环的长度要小于8
		for (unsigned int k=1; k <= G[ID][0]; k++) //ID有没有出度是环头
			if (G[ID][k] == circle[numcirl].data[0]) return 1;//找到环
		circle[numcirl].length = 0;
		visited[ID] = -1;
		return 0;//不是环
	}
	for (unsigned int i = 1; i <= G[ID][0]; i++) {
		if (circle[numcirl].length > 2)//环的长度要大于2
			for (unsigned int j = 1; j <= G[ID][0]; j++)
				if (G[ID][j] == circle[numcirl].data[0]) return 1;//找到环
		if (visited[G[ID][i]] == numcirl) continue;
		if (DFS(G, G[ID][i], numcirl)) return 1;
		else {
			visited[ID] = -1;
			return 0;
		}
	}
	circle[numcirl].length = 0;//退出递归时，
	return 0;
}

int main()
{
	long start = clock();  //开始时间
	long finish;//结束时间，用于测试代码运行时间
	long t;//时间间隔
   
	//打开读取数据的文件，存放在df（data of file）数组中
	FILE* fin = NULL;
	unsigned int* df1 = (unsigned int*)malloc(sizeof(unsigned int) * NUM_ID);
	unsigned int* df2 = (unsigned int*)malloc(sizeof(unsigned int) * NUM_ID);
	unsigned int n = 0;
	unsigned int max = 0;//整张图里最大的ID
	if (fopen_s(&fin, "test_data2.txt", "rb") != 0) {
		printf("error open test_data.txt\n");
		return 0;
	}
	if (fin == 0) return 0;

	while (fscanf_s(fin, "%u", df1 + n) != EOF) {
		fseek(fin, 1, 1);
		fscanf_s(fin, "%u", df2 + n);
		if (df1[n] > max) max = df1[n];
		if (df2[n] > max) max = df2[n];
		fseek(fin, 2, 1);
		while (fgetc(fin) != '\n');
		n++;
	}
	fclose(fin);


	//创建有向图(邻接矩阵)
	/*邻接矩阵：
	序号：即用户ID，随机访问减少时间开销
	第0列：该顶点的出度（这个ID账号给多少个人转账）
	第n列（n>=1）：该顶点的出边邻接结点
	*/
	unsigned int G[NUM_ID][9];
	unsigned int ID;
	for (unsigned int i = 0; i <= max; i++) {
		G[i][0] = 0;//所有出度初始化为0
		visited[i] = -1;//随便也初始化访问记录数组
	}
	for (unsigned int i = 0; i < n; i++) {
		ID = df1[i];
		G[ID][G[ID][0]+1] = df2[i];
		G[ID][0]++;//出度加一
	}
	free(df1);
	free(df2);

	//找出所有环
	unsigned int numcirl=0;//环的总数量
	for (unsigned int i = 0; i <= max; i++) {
		circle[numcirl].length = 0;//初始化长度
		if (G[i][0] == 0) continue;
		circle[numcirl].data[circle[numcirl].length++] = i;
		visited[i] = numcirl;//访问记号
		if (DFS(G, G[i][1], numcirl)) numcirl++;//若成环，则环数加一
		else {
			visited[i] = -1;//不成环，删除访问记号
			circle[numcirl].length = 0;
		}
	}

	//环内排序
	unsigned int tmp[7], len, minID, flag, *sort;
	for (unsigned int i = 0; i < numcirl; i++) {
		//用更加简短的变量代替，代码更加简洁
		sort = &circle[i].data[0];
		len = circle[i].length;
		minID = *sort;
		flag = 0;
		for (unsigned int j = 1; j < len; j++)//找最小
			if (*(sort+j) < minID) {
				minID = *(sort+j);
				flag = j;//flag是最小ID在环内的位置
			} 
		if (flag == 0)continue;//若顺序正确则不用调整
		//若顺序不正确，整体位移（flag+j）个单位
		for (unsigned int j = 0; j < len; j++)
			tmp[j] = *(sort + j);
		for (unsigned int j = 0; j < len; j++)
			*(sort + j) = tmp[(flag + j) % len];
	}

	//将环的序号按环的长度收集至sortidx，好处是，不用对环整体移动，只需要将环对应的序号(idx)排好序即可;
	unsigned int* sortidx, head = 0, rear = numcirl - 1;
	unsigned int num_3, num_4, num_5, num_6, num_7;//长度为n(3,4...7)的环的个数
	sortidx = (unsigned int*)malloc(sizeof(unsigned int) * numcirl);
	for (unsigned int j= 0; j < numcirl; j++) {//环间排序，长度为3和7
		if (circle[j].length == 3) sortidx[head++] = j;
		if (circle[j].length == 7) sortidx[rear--] = j;
	}
	num_3 = head;
	num_7 = numcirl - 1 - rear;
	for (unsigned int j = 0; j < numcirl; j++) {//环间排序，长度为4和5
		if (circle[j].length == 4) sortidx[head++] = j;
		if (circle[j].length == 6) sortidx[rear--] = j;
	}
	num_4 = head - num_3;
	num_6 = numcirl - 1 - rear - num_7;
	for (unsigned int j = 0; j < numcirl; j++)  //环间排序，长度为6
		if (circle[j].length == 5) sortidx[head++] = j;
	num_5 = head - num_3 - num_4;
	//for (unsigned int j = 0; j < numcirl; j++) cout << sortidx[j] << endl;
	//cout << num_3 << endl << num_4 << endl << num_5 << endl << num_6 << endl << num_7 << endl;

	//按sortidx中的序号依次将环输出到文件
	FILE* fout = NULL;
	if (fopen_s(&fout, "result1.txt", "wb") != 0) {
		printf("error open result.txt\n");
		return 0;
	}
	if (fout == 0) return 0;
	unsigned int idx;
	fprintf(fout, "%u\n", numcirl);
	for (unsigned int i = 0, j; i < numcirl; i++) {
		idx = sortidx[i];
		for (j = 0; j < circle[idx].length - 1; j++) {
			fprintf(fout, "%u,", circle[idx].data[j]);
		}
		fprintf(fout, "%u\n", circle[idx].data[j]);//每行最后不输出逗号
	}
	                     
	//计算运行时间
	finish = clock();
	cout << "时间: " << finish - start << "ms\n"; 
}
