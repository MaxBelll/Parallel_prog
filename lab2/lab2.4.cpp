#include <cstdlib>
#include <iostream>
#include <cstring>
#include <pthread.h>
#include <chrono>
#include <math.h> 

using namespace std;

#define MICROSECONDS_IN_SECOND 1e-6

#define err_exit(code, str) { cerr << str << ": " << strerror(code) \
        << endl; exit(EXIT_FAILURE); }

int current_task = 0; // Указатель на текущее задание
pthread_mutex_t mutex; // Мьютекс
pthread_spinlock_t spinlock;// Спинлок
int number_of_tasks;
int sync_type;//тип синхронизации

void *thread_job(void *arg)
{
    int err;
    int x = 999999;
    
    for(int i = 0; i < number_of_tasks; i++){
        if (sync_type == 0) {
            err = pthread_mutex_lock(&mutex);
            if(err != 0)
                err_exit(err, "Cannot lock mutex");
            }
        else {
            err = pthread_spin_lock(&spinlock);
            if(err != 0)
            err_exit(err, "Cannot lock spinlock");
        }

        if (sync_type == 0) {
            err = pthread_mutex_unlock(&mutex);
            if(err != 0)
                err_exit(err, "Cannot unlock mutex");
        }
        else {
            err = pthread_spin_unlock(&spinlock);
            if(err != 0)
                err_exit(err, "Cannot unlock spinlock");
  		}
		x = sqrt(x);
    	x = x*x;
    	}
    return (NULL);
} 

//argv1 - number of threads, argv2 - number of tasks
int main(int argc, char *argv[])
{
    if(argc != 3) {
        cout << "Wrong number of arguments" << endl;
        exit(-1);
    } 
	int err;
    int number_of_threads = atoi(argv[1]);
    number_of_tasks = atoi(argv[2]);

    pthread_t* threads = new pthread_t[number_of_threads];

    err = pthread_mutex_init(&mutex, NULL);
    if(err != 0)
		err_exit(err, "Cannot initialize mutex");

	err = pthread_spin_init(&spinlock, PTHREAD_PROCESS_PRIVATE);
	if(err != 0)
		err_exit(err, "Cannot initialize spinlock");

	
	//работа с mutex
	sync_type = 0;
    auto begin = chrono::steady_clock::now();
    for (int i = 0; i < number_of_threads; ++i){
        err = pthread_create(&threads[i], NULL, thread_job, NULL);
        if(err != 0)
            err_exit(err, "Cannot create thread");
    }

    for (int i = 0; i < number_of_threads; ++i){
        pthread_join(threads[i], NULL);
    }
    auto end = chrono::steady_clock::now();
    auto time_difference = chrono::duration_cast<chrono::microseconds>(end - begin).count() * MICROSECONDS_IN_SECOND;
    cout << "Time with mutex: " << time_difference  << " seconds" << endl;
  
    //работа сo spinlock
    sync_type = 1;
    begin = chrono::steady_clock::now();
    for (int i = 0; i < number_of_threads; ++i){
        err = pthread_create(&threads[i], NULL, thread_job, NULL);
        if(err != 0)
            err_exit(err, "Cannot create thread");
    }

    for (int i = 0; i < number_of_threads; ++i){
        pthread_join(threads[i], NULL);
    }
    end = chrono::steady_clock::now();
    time_difference = chrono::duration_cast<chrono::microseconds>(end - begin).count()*MICROSECONDS_IN_SECOND;
    cout << "Time with spinlock: " << time_difference  << " seconds" << endl;
    
    pthread_mutex_destroy(&mutex);
	pthread_spin_destroy(&spinlock);
    delete[] threads;
    return 0;
} 

