#include <omp.h>
#include <iostream>
#include <chrono>
using namespace std;

int f(int x)
{
	for (int i = 0; i < 500000000; i++)
         x+= i - i/2;

    return x;
}
int main() 
{
	int a[100], b[100];
	// Инициализация массива b
	for(int i = 0; i<100; i++)
		b[i] = i;
	// Директива OpenMP для распараллеливания цикла
	#pragma omp parallel for
	for(int i = 0; i<100; i++)
	{
		a[i] = b[i];
		b[i] = 2*a[i];
	}

	int result = 0;
	// Далее значения a[i] и b[i] используются, например, так:
	auto begin = chrono::steady_clock::now();
	#pragma omp parallel for reduction(+ : result)
	for(int i = 0; i<100; i++)
		result += (a[i] + f(b[i]));
	cout << "Result = " << result << endl;
	auto elapsed_time = chrono::duration_cast<chrono::microseconds>(chrono::steady_clock::now() - begin);
	cout << "Time = " << elapsed_time.count() << '\n';
	return 0;
}