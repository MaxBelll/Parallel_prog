#include <cstdlib>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <pthread.h>

using namespace std;

#define err_exit(code, str) { cerr << str << ": " << strerror(code) \
 << endl; exit(EXIT_FAILURE); }

pthread_mutex_t mutex;
int store = 0;
bool ready = false;

void *producer(void *arg)
{
	int err;
	while(true) 
 	{
		err = pthread_mutex_trylock(&mutex);
		if (err == 0)
		{
		    ready = false;
		    while(!ready) {} 
		    cout << "producer: added task\n";
		    store++;
		    err = pthread_mutex_unlock(&mutex);
		    if(err != 0)
		        err_exit(err, "Cannot unlock mutex"); 
		}    
		ready = true;
    }
	return (NULL);

}
void *consumer(void *arg)
{
	int err;
	while(true) 
 	{
		err = pthread_mutex_trylock(&mutex);
		if (err == 0)
		{
		    ready = false;
		    while(!ready) {} 
		    cout << "consumer: processing " << store <<" task\n";
		    sleep(1);
		    err = pthread_mutex_unlock(&mutex);
		    if(err != 0)
		        err_exit(err, "Cannot unlock mutex"); 
		}    
		ready = true;
    }
	return (NULL);
}

int main()
{
    int err;
    pthread_t thread1, thread2; // Идентификаторы потоков
    
    err = pthread_mutex_init(&mutex, NULL);
    if(err != 0)
        err_exit(err, "Cannot initialize mutex");
    // Создаём потоки
    err = pthread_create(&thread1, NULL, producer, NULL);
    if(err != 0)
        err_exit(err, "Cannot create thread 1");
    sleep(1);
    err = pthread_create(&thread2, NULL, consumer, NULL);
    if(err != 0)
        err_exit(err, "Cannot create thread 2");

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_mutex_destroy(&mutex);
} 
