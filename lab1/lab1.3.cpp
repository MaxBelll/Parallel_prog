#include <cstdlib>
#include <iostream>
#include <cstring>
#include <pthread.h>
using namespace std;

void *thread_job(void *arg)
{
int *p = (int*)arg;
printf("Thread %d is running..\n",*p);
return (NULL);
}
int main()
{
int n = 0;
cout << "enter the number of threads: ";
cin >> n;

pthread_t thread[n];
int numbers[n];
int err;
for ( int i=0; i<n; i++){
numbers[i]  = i;
err = pthread_create(&thread[i], NULL, thread_job, (void*) &numbers[i]);
if(err != 0) {
cout << "Cannot create a thread: " << strerror(err) << endl;
exit(-1);
}
}
pthread_exit(NULL);
}
