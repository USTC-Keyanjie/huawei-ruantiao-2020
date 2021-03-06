# 华为软件挑战赛2020

上合赛区 好想去冰岛看极光呀

我和我的队友[许老师](https://github.com/NicoleXCT)，[海神](https://github.com/wanghaijie2017)一起努力，拿下华为软件挑战赛2020初赛第8，复赛A榜第9，B榜第3，决赛榜第16。

# 华为软件挑战赛2020 初赛

## TO-DO List

 - [x] 调试多线程分段比例，三段的增长比例应该大致服从 1:5:12
 - [x] 调试结果输出数组大小，尝试将每个线程的结果数组开到现在的一半大小
 - [x] 去除代码中不需要的赋值操作，尽最大可能减少内存复制
 - [x] 引入预读机制，进行优化
 - [x] 使用neon_memcpy进行优化
 - [x] dfs到一个环就直接生成对应的字符串，然后加入到results数组中

## 优化日志

### 2020-4-16

**1.88 -> 1.09**

1. 优秀的许同学加入了mmap输入、mmap输出以及分块mmap输出
2. 使用数组类型代替大量的vector容器，得到巨大加速

### 2020-4-17

**1.09 -> 0.89**

1. 优秀的许同学优化了ids数组
2. 加入了快慢指针去重
3. 舍弃id_hash，使用二分查找得到really id 与 regular id的映射关系
4. g_succ有向图的后继数据结构，引入哨兵机制，加速一点点
5. 使用memcpy代替逐个复制数据，加速一点点

### 2020-4-18

**0.89 -> 0.76** rank #30

1. 在海神帮助下，加入了多线程代码
2. 将dfs的递归代码改为迭代代码，加速效果一般般

### 2020-4-19

**0.76 -> 0.5840**

1. 将idsComma和idsLF数组合并
2. 使用数组方式代替vector方式
3. 弃用to_string方法，手动实现int类型数据to_string
4. 使用register关键字修饰常用变量
5. 尝试弃用idChar数组，而选择在输出数据动态生成每个id号的字符串格式，延时0.2s。
    原因在于结果数组有300w条，平均每条6个数字，大约共2000w个数字，也就是要转2000w次类型
    而使用idChar数组预处理，只需要预处理所有的节点，约运算28w次不到，效率大大提升

**0.5840 -> 0.5466** rank #27

1. 使用fwrite代替mmap进行输出，速度更快，输入还是使用mmap
2. {0, 0.068, 0.148, 0.284, 1} 改为 {0, 0.08, 0.21, 0.3622, 1}，提交后0.5466 -> 0.5518，我真是调参鬼才

### 2020-4-21

0.5466 -> 0.5402

1. 调整了结果数组的大小，加速了一点点。

### 2020-4-24

0.5402 -> 0.5057

1. 不在预先生成二级跳表，而是在每次dfs之前都做一次三层反向dfs，可以理解为动态生成了跳表
2. 使用数组实现所有的跳表，至此所有的数据结构都是数组，告别STL从我做起！
3. 心情不太好，不是很想写代码，一写就写了几个低级错误心情更不好了，晚上再加油吧~

**0.5057 -> 0.4198**

1. 把数组缩小了一下，加快还挺多~

0.4198 -> 0.4218

1. 数组缩小到原来的1/5竟然还负优化了，这服务器怕不是太烫了吧。不想写了，出门跑步去。

### 2020-4-25

0.4198 -> 0.4411

1. 越优化越慢，拿个手环就这么难吗？

**0.4198 -> 0.3149**

1. 去重id，排序，二分查找这三个操作可以用计数排序算法一步到位！

0.3149 -> 0.3080

1. 还是非递归的算法效率高一点，现在写非递归超级熟练~
2. 优化到这个成绩压力已经小很多了，明天再加油~

### 2020-4-26

**0.3080 -> 0.2281**

1. 使用neon的特性，16字节同步复制的memcpy速度可以快很多很多，再优化一点点应该可以进复赛了

### 2020-4-27

0.2281 -> 0.2178

1. 使用循环展开优化代码，性能是有提升，不过代码看起来怪怪的。有加速就行，管他呢管他呢~

**0.2178 -> 0.2037** rank # 23

1. 使用18线程并行处理（18线程，就尼玛离谱...）
2. 优化了数据结构，所有与线程有关的数据结构都以结构体形式存储
3. 暂停维护dfs生成字符串结果的方案，dfs生成数字结果方案继续改进

**0.2037 -> 0.1839** rank #17

1. 不再给每个线程均分任务量，因为靠前的id比靠后的id搜索任务重的多。延续4线程的分配经验，设计18线程的任务分配比例。

### 2020-5-4

1. 玩完回来啦，把初赛代码写上注释，就此停止维护了。正式开启复赛进程，冲冲冲！

# 华为软件挑战赛2020 复赛

## TO-DO List

 - [x] 增加金额信息有关数据结构
 - [x] 输入部分增加对金额数据的读取
 - [x] 按照题目要求，dfs和反向dfs里增加对于金额信息的约束
 - [x] 反向三级跳表的排序改进
 - [x] 没把握的数组都使用STL代替

## 优化（瞎整）日志

### 2020-5-6

Runtime Error
 
1. 今天线上吃了6个RE，可是华为测试机没问题的，很奇怪，开局有点不顺呀~

### 2020-5-7

16% THE RESULT IS INCORRECT

1. 支持计数排序和非计数排序两种方案
2. 开了一个很夸张的数组，总算不再是RE了，可是变成了16% THE RESULT IS INCORRECT，我觉得还是数组没开够的原因。
3. 采访了两位大佬，很一致的选择了STL，还是得有个百分百AC的方案，明天请队友一起写STL吧

### 2020-5-8

0% THE RESULT IS INCORRECT

1. 现在有STL版本代码，保证不会出现数组越界问题
2. 结果还是不对，要么是算法正确性还有小漏洞，要么是代码里写的有问题，继续排查~
3. 我导又来找我写文档了，随便水点字给他

### 2020-5-11

6.8473

1. 第一次听到前向星这个数据结构，学习了一波发现还挺适合当前的环境，用上后效果还不错
2. 明明离前四还很远，可是真的不知道还能怎么优化了- -

### 2020-5-13

1. 本来以为我提出了一种天才的数据结构，结果耗时是原来的三倍，我是个智障吧

### 2020-5-14

1. 使用负载均衡策略平衡每个线程的任务
2. 优化三级跳表排序 线上未通过

### 2020-5-15

3.4860

1. 现已加入负载均衡和动态内存
2. 许多地方使用引用代替多次寻址
3. mmap优化，拓扑排序完成，现已加入豪华午餐
4. 加入NEON的memcpy
5. id去重优化，只对一半的id去重
6. 严格限制register关键字的使用

### 2020-5-16
1. 3.8267 5月16日复赛正式提交版本

# 华为软件挑战赛2020 决赛

## TO-DO List

 - [x] 数组实现二叉堆
 - [x] 学习TopK算法

## 优化（瞎整）日志

### 2020-5-23

838.9116

1. 第一次Baseline

### 2020-5-24

1. 尝试使用斐波那契堆代替优先队列，速度大大变慢了

### 2020-5-25

1. 优化Brande算法的除法公式，增加ceoff中间变量减少计算
2. 改内联函数与添加register关键字
3. pq.push改为pq.emplace，线下430s

### 2020-5-26

1184

1. dijkstra时创建前驱图，大大节省计算

### 2020-5-27

1. 不使用memset初始化dis和delta数组，而是在遍历过程中初始化。具体操作为正向dijkstra搜索时初始化delta，反向计算中心性时初始化dis
2. 一起访问的数组合并为结构体，降低cache miss
3. dijkstra的前驱图放弃链式前向星，使用连续前向星
4. 添加拓扑排序优化，级联删除入度为0出度为1的点

### 2020-5-28

1. 内卷优化，dis由ull类型改为ui类型
2. 使用全新的快速堆，push与pop的时间复杂度降为O(1)
3. 进一步优化快速堆，将双端队列形式改为栈，减少clear的memset初始化消耗
4. dijkstra算法内部if判断顺序变更，有很大加速

### 2020-5-29

1. 进一步内卷优化，使用unsigned short存储权重和距离
2. 优化g_succ数组，使用4个字节同时表示id和权重，22个bit表示id，10个bit表示权重
3. dijkstra的前驱图使用 MAX_NUM_IDS * 15 的二维数组来表示，并且使用vector回落，防止二维数组存不下
4. 使用STL的优先队列回落快速堆
5. 增加tarjan优化方式，线上效果比topo优化效果好

### 2020-5-30

1. dijkstra的前驱图使用 MAX_NUM_IDS的一维数组来表示，如果存不下，使用数组前向星来回落