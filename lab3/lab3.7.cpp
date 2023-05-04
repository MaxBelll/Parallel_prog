#include <cstdlib> // для system
#include <iostream>
#include <string>
#include <chrono>
#include <omp.h>


#define N 10000

const int NUM_THREADS = 1;

using std::cout;
using std::cin;
using std::endl;
double c1,c2,h,tmp;
int n=N,k;
double a[N][N];
double b[N],y[N],x[N];

double xbeg,xend;
double f(double x)
{
	x=1;
	return x;
}
double p(double x)
{
	x=-2*x/(x*x+1);
	return x;
}
double q(double x)
{
	x=2/(x*x+1);
	return x;
}
void gauss()
{
	//Прямой ход
	for(k=0;k<=n-2;k++)
	{
		#pragma omp parallel for private(tmp) num_threads(NUM_THREADS) 
		for(int i=k+1;i<=n-1;i++)
		{
			tmp = a[i][k];
			b[i] = b[i] - b[k] * tmp / a[k][k];
			#pragma omp parallel for num_threads(NUM_THREADS)
			for(int j=0;j<=n-1;j++)
				a[i][j] = a[i][j] - a[k][j] * tmp / a[k][k];
		}
	}

	//Обратный ход
	y[n-1] = b[n-1] / a[n-1][n-1];
	#pragma omp parallel for num_threads(NUM_THREADS)
	for(int i=n-2;i>=0;i=i-1)
	{
		y[i] = b[i];
		#pragma omp parallel for num_threads(NUM_THREADS)
		for(int j=i+1;j<=n-1;j++)

		y[i] = y[i] - a[i][j] * y[j];
		y[i] = y[i] / a[i][i];
	}

}
int main()
{

	setlocale(0, "");
	cout << "Введите количество точек на интервале (<=" << N << ") n=" ;
	cin >> n;
	cout << "x-координату начала интервала=" ;
	cin >> xbeg;
	cout << "x-координату конца интервала=" ;
	cin >> xend;
	auto start = std::chrono::steady_clock::now();
	h=(xend-xbeg)/(n-1);
	#pragma omp parallel for num_threads(NUM_THREADS)
	for ( int i = 0; i < n; i++)
	{
		b[i]=0;
		y[i]=0;
		x[i]=xbeg+i*h;
	}

	#pragma omp parallel for num_threads(NUM_THREADS)
	//Инициализация двумерного динамического массива
	for (int i = 0; i < N; i++)
	{
		#pragma omp parallel for num_threads(NUM_THREADS)
		for (int j = 0; j < i+1; j++)
		{
			a[i][j]=0;
		}
	}

	//Граничные условия
	c1=1.5;
	c2=-1;

	// Правые части системы
	for (int i = 0; i < n-2; i++)
		b[i]=f(x[i]);

	//Граничные условия
	b[n-2]=c1;
	b[n-1]=c2;

	// Коэффициенты системы
	for(int i=0;i<=n-3;i++)
	{
		a[i][i]=1/(h*h);
		a[i][i+1]=-(2/(h*h)+2*p(x[i])/h-q(x[i]));
		a[i][i+2]=1/(h*h)+2*p(x[i])/h;
	}

	//Граничный условия
	a[n-2][0]=1;
	a[n-1][n-1]=1;
	gauss();
	//for(int i=0;i<n;i++)
	//	cout << "x("<< i <<")= "<< x[i] << " "<<"y("<< i<<")="<<y[i]<< endl;

	//Проверка решения на граничные условия
	cout << "Проверка решения на граничные условия"<< endl;
	cout <<y[0] <<" "<< y[n-1] << endl;

	auto end = std::chrono::steady_clock::now();
	cout<<"TIME: "<<std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()<<endl;
}