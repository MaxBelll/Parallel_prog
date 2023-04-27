#include <cstdlib>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <map>
#include <vector>
#include <string>
#include <chrono>

using namespace std;
#define err_exit(code, str) { cerr << str << ": " << strerror(code) \
<< endl; exit(EXIT_FAILURE); }
map <int,int> map_res;
string res = "";
typedef void* (*func)(void* arg);
pthread_mutex_t mutex;
struct Param
{
    int* array_ptr;
    int count_elements;
};
void* mapf(void* arg)
{
    Param* params = (Param*)arg;
    for(int i = 0; i < params->count_elements; ++i)
        map_res[params->array_ptr[i] ]+=1; 
    return (NULL);     
}
void* reducef(void* arg)
{
    int err;
    err = pthread_mutex_lock(&mutex);
    if(err != 0)
    	err_exit(err, "Cannot lock mutex");
    
    Param* params = (Param*)arg;
    for(int i = 0; i < params->count_elements; ++i)
       res+= to_string(map_res[params->array_ptr[i]]*params->array_ptr[i]) + ' ';  

    err = pthread_mutex_unlock(&mutex);
    if(err != 0)
        err_exit(err, "Cannot unlock mutex");
    return (NULL);
}
void mapreduce(Param* array,func mapfunc,func reducefunc, int count_threads)
{
	int err;
    Param* params = new Param[count_threads]; //создание структур для передачи
    pthread_t* threads = new pthread_t[count_threads]; //создание потоков

    for(int i = 0; i < count_threads; i++){
        params[i].count_elements = array->count_elements / count_threads;
        params[i].array_ptr = &array->array_ptr[array->count_elements / count_threads * i];
        if(i == count_threads - 1)//отдаем последнему потоку остаток от деления(элементы)
            params[i].count_elements += array->count_elements % count_threads;

        // Создание потока
        err = pthread_create(&threads[i], NULL, mapfunc, (void*)&params[i]);
        if(err != 0){
        cout << "Cannot create a thread: " << strerror(err) << endl;
        exit(-1);
        }
    }  
    // Ожидание завершения всех созданных потоков
    for(int i = 0; i < count_threads; ++i)
        pthread_join(threads[i], NULL);

    vector<int> keys;
    for (auto it = map_res.begin(); it != map_res.end(); it++)
        keys.push_back(it->first);
	//cout << '\n' << keys.size() << '\n' << map_res.size() << '\n';
	
    for(int i = 0; i < count_threads; i++)
    {
        params[i].count_elements = map_res.size()/count_threads;
        params[i].array_ptr = &keys[map_res.size()*i/count_threads];
        if(i == count_threads - 1)
            params[i].count_elements += map_res.size() % count_threads;

        // Создание потока
        err = pthread_create(&threads[i], NULL, reducefunc, (void*)&params[i]);
        if(err != 0){
        cout << "Cannot create a thread: " << strerror(err) << endl;
        exit(-1);
        }

    }  
    // Ожидание завершения всех созданных потоков
    for(int i = 0; i < count_threads; ++i)
        pthread_join(threads[i], NULL);

    //cout << "\nРезультат: " << res << endl;
    delete[] params;
    delete[] array;
    delete[] threads;
}

int main(int argc, char *argv[])
{
    if(argc != 3) {
        cout << "Wrong number of arguments" << endl;
        exit(-1);
    } 
	int err;
    int count_threads = atoi(argv[1]);
    int array_size = atoi(argv[2]);
    if(array_size < count_threads)
        count_threads = array_size;

    srand(time(NULL));
	//cout << "Исходный массив: ";
    int* array = new int[array_size];
    for(int i = 0; i < array_size; i++){
        array[i] = rand() % 10 + 1;
    //    cout  << array[i] << ' ';
    }
    Param* p=new Param;
    p->array_ptr=array;
    p->count_elements=array_size;
    auto begin = chrono::steady_clock::now();
    mapreduce(p,&mapf,&reducef,count_threads);
    auto elapsed_time = chrono::duration_cast<chrono::microseconds>(chrono::steady_clock::now() - begin);
	cout << "Time: " << elapsed_time.count() << '\n';
    pthread_mutex_destroy(&mutex);
    return 0;
}
