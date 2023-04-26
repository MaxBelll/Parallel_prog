#include <iostream>
#include <cstring>
#include <pthread.h>
#include <cmath>
#include <chrono>
using namespace std;


void *thread_job(void *arg){
int *n = (int*) arg; 
chrono::steady_clock sc;//замер времени работы потока
auto start = sc.now(); 
int a;
for(int i = 0; i < 1000000; i++){
	a=sqrt(1001); 
	}
auto end = sc.now();
auto time_span = static_cast<chrono::duration<double>>(end - start);
cout << "Thread work time: " << time_span.count() << " seconds\n";
return (NULL);
}

int main(int argc, char *argv[])
{
int count = atoi(argv[1]);
int err;
pthread_t thread;

chrono::steady_clock sc;//замер времени создания
auto start = sc.now(); 
err = pthread_create(&thread, NULL, thread_job, (void *) &count);
	if(err != 0) {
		cout << "Cannot create a thread: " << strerror(err) << endl;
		}
auto end = sc.now();
auto time_span = static_cast<chrono::duration<double>>(end - start);
cout << "Thread create time: " << time_span.count() << " seconds\n"; 

pthread_exit(NULL);
}


