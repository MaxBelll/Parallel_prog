#include <cstdlib>
#include <iostream>
#include <cmath>
#include <cstring>
#include <pthread.h>

using namespace std;

typedef double (*func_ptr_type)(double);

struct arrg{
    func_ptr_type func_ptr;
    double* array_ptr;
    int count_elements;
};

void *thread_job(void *arg){
    arrg* params = (arrg*)arg;
    for(int i = 0; i < params->count_elements; ++i)
        params->array_ptr[i] = params->func_ptr(params->array_ptr[i]);
    return (NULL);
}

func_ptr_type get_func(int func){
    switch(func){
    case 0:
        return &sqrt;
    case 1:
        return &cos;
    default:
        return &sin;
    }
}
int main(int argc, char* argv[])
{
    if(argc != 4){
        cout << "Wrong number of arguments\n";
        exit(-1);
    }

    int number_of_threads = atoi(argv[1]);
    int array_size = atoi(argv[2]);
    int operation=atoi(argv[3]);

    if(array_size <= 0 || number_of_threads <= 0 || operation<0 || operation>2){
        cout << "Wrong state of some arguments\n";
        exit(-1);
    }

    if(array_size < number_of_threads)
        number_of_threads = array_size;

    // Указатель на функцию
    func_ptr_type func_ptr = get_func(operation);
    double* array = new double[array_size];
    cout << "Initial array:\n";
    for(int i = 0; i < array_size; i++){
        array[i] = (rand() % 20);
        cout  << array[i] << " ";
    }
    
    // Массив аргументов, которые будут переданы в функции потоков
    arrg* args = new arrg[number_of_threads];
    // Массив потоков
    pthread_t* threads = new pthread_t[number_of_threads];

    int count_elements_for_thread = array_size / number_of_threads;

    int err;

    for(int i = 0; i < number_of_threads; i++){
        args[i].func_ptr = func_ptr;
        args[i].count_elements = count_elements_for_thread;
        args[i].array_ptr = &array[count_elements_for_thread * i];

        if(i == number_of_threads - 1)
            args[i].count_elements += array_size % number_of_threads;

        err = pthread_create(&threads[i], NULL, thread_job, (void*)&args[i]);
        if(err != 0){
        cout << "Cannot create a thread: " << strerror(err) << endl;
        exit(-1);
        }
     }  

    // Ждем завершения всех созданных потоков
    for(int i = 0; i < number_of_threads; ++i)
        pthread_join(threads[i], NULL);
    cout << "\nModified array:\n";
    for(int i = 0; i < array_size; i++)
        cout  << array[i] << " ";
    cout << endl;
    
    delete[] args;
    delete[] array;
    delete[] threads;
}
