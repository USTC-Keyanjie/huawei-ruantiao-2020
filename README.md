# 华为软件挑战赛2020

## 优化日志

2020-4-16

1.88 -> 1.09

1. 优秀的许同学加入了mmap输入、mmap输出以及分块mmap输出
2. 使用数组类型代替大量的vector容器，得到巨大加速

2020-4-17

1.09 -> 0.89

1. 优秀的许同学优化了ids数组
2. 加入了快慢指针去重
3. 舍弃id_hash，使用二分查找得到really id 与 regular id的映射关系
4. g_succ有向图的后继数据结构，引入哨兵机制，加速一点点
5. 使用memcpy代替逐个复制数据，加速一点点

2020-4-18

0.89 -> 0.76 rank#30

1. 在海神帮助下，加入了多线程代码
2. 将dfs的递归代码改为迭代代码，加速效果一般般

2020-4-19

0.76 -> 0.5840

1. 将idsComma和idsLF数组合并
2. 使用数组方式代替vector方式
3. 弃用to_string方法，手动实现int类型数据to_string
4. 使用register关键字修饰常用变量

2020-4-19

0.5840 -> 0.5466 rank#27

1. 使用fwrite代替mmap，速度更快
