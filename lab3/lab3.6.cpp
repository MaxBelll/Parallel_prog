#include <omp.h>
#include <iostream>
using namespace std;
int main()
{
    int sum = 0;
    const int a_size = 100;
    int a[a_size], id, size;
    for (int i = 0; i < 100; i++)
        a[i] = i;
	// Применение опции reduction
	#pragma omp parallel private(id, size)
    { 
    	// Начало параллельной области
        id = omp_get_thread_num();
        size = omp_get_num_threads();
        // Разделяем работу между потоками
        int integer_part = a_size / size;
        int remainder = a_size % size;
        int a_local_size = integer_part +
                           ((id < remainder) ? 1 : 0);
        int start = integer_part * id +
                    ((id < remainder) ? id : remainder);
        int end = start + a_local_size;
        int part_sum = 0;
        // Каждый поток суммирует элементы
        // своей части массива
        for (int i = start; i < end; i++)
            part_sum += a[i];

		#pragma omp critical
        {
            sum += part_sum;
            // Каждый поток выводит свою частичную сумму
            cout << "Thread " << id << ", partial sum = " << part_sum << endl;
        }
    }    
    cout << "Final sum = " << sum << endl;
    return 0;
}