# 华为软件挑战赛2020

## TO-DO List

 - [x] 调试多线程分段比例，三段的增长比例应该大致服从 1:5:12
 - [x] 调试结果输出数组大小，尝试将每个线程的结果数组开到现在的一半大小
 - [x] 去除代码中不需要的赋值操作，尽最大可能减少内存复制
 - [ ] 引入预读机制，进行优化
 - [ ] 使用neon_memcpy进行优化，dfs到一个环就直接生成对应的字符串，然后加入到results数组中

## 优化日志

### 2020-4-16

1.88 -> 1.09

1. 优秀的许同学加入了mmap输入、mmap输出以及分块mmap输出
2. 使用数组类型代替大量的vector容器，得到巨大加速

### 2020-4-17

1.09 -> 0.89

1. 优秀的许同学优化了ids数组
2. 加入了快慢指针去重
3. 舍弃id_hash，使用二分查找得到really id 与 regular id的映射关系
4. g_succ有向图的后继数据结构，引入哨兵机制，加速一点点
5. 使用memcpy代替逐个复制数据，加速一点点

### 2020-4-18

0.89 -> 0.76 rank#30

1. 在海神帮助下，加入了多线程代码
2. 将dfs的递归代码改为迭代代码，加速效果一般般

### 2020-4-19

0.76 -> 0.5840

1. 将idsComma和idsLF数组合并
2. 使用数组方式代替vector方式
3. 弃用to_string方法，手动实现int类型数据to_string
4. 使用register关键字修饰常用变量
5. 尝试弃用idChar数组，而选择在输出数据动态生成每个id号的字符串格式，延时0.2s。
    原因在于结果数组有300w条，平均每条6个数字，大约共2000w个数字，也就是要转2000w次类型
    而使用idChar数组预处理，只需要预处理所有的节点，约运算28w次不到，效率大大提升

0.5840 -> 0.5466 rank#27

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

0.5057 -> 0.4198

1. 把数组缩小了一下，加快还挺多~

0.4198 -> 0.4218

1. 数组缩小到原来的1/5竟然还负优化了，这服务器怕不是太烫了吧。不想写了，出门跑步去。

### 2020-4-25

0.4198 -> 0.4411

1. 越优化越慢，拿个手环就这么难吗？

0.4198 -> 0.3149

1. 去重id，排序，二分查找这三个操作可以用计数排序算法一步到位！

0.3149 -> 0.3080

1. 还是非递归的算法效率高一点，现在写非递归超级熟练~
2. 优化到这个成绩压力已经小很多了，明天再加油~

### 2020-4-26

0.3080 -> 0.2281

1. 使用neon的特性，16字节同步复制的memcpy速度可以快很多很多，再优化一点点应该可以进复赛了