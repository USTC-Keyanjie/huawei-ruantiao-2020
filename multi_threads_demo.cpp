#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <unistd.h>

using namespace std;

#define NUM_THREADS     5

// 这里定义子线程函数, 如果处理不同分段的数据不一样,那就写多个函数
// 函数的参数传入的时候需要是 void * 类型, 一般就是数据的指针转成  无类型指针
void *wait(void *t)
{  
   // 先把指针类型恢复, 然后取值
   int tid = *((int*)t);
   
   sleep(1000);

   cout << "tid:" << tid << endl;
   cout << "tid address:" << t << endl;
   // pthread_exit(NULL) 为退出该线程
   pthread_exit(NULL);
}

int main ( ){
   int rc;
   int i;
   // 创建子线程的标识符 就是线程 的id,放在数组中
   pthread_t threads[NUM_THREADS];
   // 线程的属性
   pthread_attr_t attr;
   void *status;

   // 用来存放i的值,传参
   // 可以改成你需要的数据结构
   int indexes[NUM_THREADS];

   // 初始化并设置线程为可连接的（joinable）
   // 这样操作后, 主main 需要等待子线程完成后才进行后一步
   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

   // 创建NUM_THREADS个数的线程
   for( i=0; i < NUM_THREADS; i++ ){
      cout << "main() : creating thread, " << i << endl;
      // 把
      indexes[i] = i;
      rc = pthread_create(&threads[i], NULL, wait, (void *)&indexes[i] );
      if (rc){
         cout << "Error:unable to create thread," << rc << endl;
         exit(-1);
      }
   }

   // 删除属性，并等待其他线程
   pthread_attr_destroy(&attr);
   for( i=0; i < NUM_THREADS; i++ ){
      rc = pthread_join(threads[i], &status);
      if (rc){
         cout << "Error:unable to join," << rc << endl;
         exit(-1);
      }
      cout << "Main: completed thread id :" << i ;
      cout << "  exiting with status :" << status << endl;
   }

   cout << "Main: program exiting." << endl;
   pthread_exit(NULL);
}
