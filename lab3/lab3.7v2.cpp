#include <omp.h>
#include <iostream>
#include <cmath>

#define n 10000

using namespace std;

const int NUM_THREADS = 8;

double x0=0,xn=1,g1=0,g2=0,dx=(xn-x0)/n;
double array_x[n],array_f[n],a[n][n],b[n],*u1,*u2;

float f(float x)
{
    return 2*sin(x*x)+cos(x*x);
}
double *Jacobi_method(double A[n][n], double b[n], int size)
{
    unsigned int start_time = clock();
    double x[size];
    double x_old[size];
    double s;
    for (int i = 0; i < size; i++)
        x[i] = b[i];
    for (int i = 0; i < size; i++)
    {
        x_old[i] = x[i];

        s = 0;
        #pragma omp parallel for reduction(+ : s) num_threads(NUM_THREADS)
        for (int j = 0; j < size; j++)
            if (i != j)
                s += A[i][j] * x_old[j];
        x[i] = (b[i] - s) / A[i][i];
    }
    unsigned int end_time = clock(); // конечное время
    unsigned int time = end_time - start_time;
    cout << " Время метод Якоби " << time << endl;
    return x;
}
// Метод Гаусса-Зейделя
double *Gauss_Seidel_method(double A[n][n], double b[n], int size)
{
    unsigned int start_time = clock();
    double x[size] ;
    double s = 0;
    for (int i = 0; i < size; i++)
        x[i] = b[i];
    for (int i = 0; i < size; i++)
    {
        s = 0;
        #pragma omp parallel for reduction(+ : s) num_threads(NUM_THREADS)
        for (int j = 0; j < size; j++)
            if (i != j)
                s += A[i][j] * x[j];
        x[i] = (b[i] - s) / A[i][i];
    }
    unsigned int end_time = clock(); // конечное время
    unsigned int time = end_time - start_time;
    cout << " Время метод гаусса " << time << endl;
    return x;
}
int main() {
    //double x0=0,xn=1,g1=0,g2=0,dx=(xn-x0)/n;
    //double array_x[n],array_f[n],a[n][n],b[n],*u1,*u2;
    for(int i=0;i < n;++i)
    {
        for( int j=0;j < n;++j)
        a[i][j]=0;
        b[i]=0;
        array_x[i]=x0;
        array_f[i]=f(x0);
        x0+=dx;
    }
    unsigned int start_time = clock();
    b[0]=g1;
    a[0][0]=1;
    b[n-1]=g2;
    a[n-1][n-1]=1;
    #pragma omp parallel for num_threads(NUM_THREADS)
    for( int i=1;i < n-1;++i)
    {

        a[i][i]=2/dx/dx;
        a[i][i+1]=-1/dx/dx;
        a[i][i-1]=-1/dx/dx;
        b[i]=array_f[i];
    }
    unsigned int end_time = clock(); // конечное время
    unsigned int time = end_time - start_time;
    cout << " Время нахождения A методом конечных разностей" << time << endl;
    u1=Gauss_Seidel_method(a,b,n);
    u2=Jacobi_method(a,b,n);
    return 0;
}