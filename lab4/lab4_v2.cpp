#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <mpi.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <iostream>
using namespace std;
#define alpha 1E+5 // Параметр уравнения
#define EPS 1E-8   // Порог сходимости
typedef struct Point {
    int x;
    int y;
    int z;
} Point;
typedef struct DPoint {
    double x;
    double y;
    double z;
} DPoint;
typedef struct Result {
    double result;
    int iters;
} Result;
#define clocktimeDifference(start, stop) \
    1.0 * (stop.tv_sec - start.tv_sec) + \
        1.0 * (stop.tv_nsec - start.tv_nsec) / 1E+9
#define max(x, y) \
    ((x) > (y) ? (x) : (y))
#define ROOT_RANK 0
double phi(double x,double y,double z){
    return ((x) * (x) + (y) * (y) + (z) * (z));
}
double ro(double x,double y,double z){
    return (6 - alpha * phi(x, y, z));
}
// Проверка краевых элементов
static bool isBoundary(Point N, int i, int j, int k){
    return i == 0 || i == N.x - 1 ||
           j == 0 || j == N.y - 1 ||
           k == 0 || k == N.z - 1;
}
// Составление краевых условий 1-го рода
static void setBoundary(double *grid_data, Point N, DPoint h, DPoint p0){
    for (int i = 0; i < N.x; i++)
        for (int j = 0; j < N.y; j++)
            for (int k = 0; k < N.z; k++){
                double x = 0;
                double y = 0;
                double z = 0;
                if (isBoundary(N, i, j, k)){
                    x = p0.x + i * h.x;
                    y = p0.y + j * h.y;
                    z = p0.z + k * h.z;
                }
                grid_data[N.y * N.z * i + N.z * j + k] = phi(x, y, z);
            }
}
// Формула итерационного процесса Якоби
static double jacobi(double *grid_data, Point N, DPoint h, DPoint p0, int i, int j, int k) {
    double hx2 = h.x * h.x, hy2 = h.y * h.y, hz2 = h.z * h.z;
    int ii = N.y * N.z, jj = N.z;
    double phix = (grid_data[ii * (i - 1) + jj * j + k] +
                   grid_data[ii * (i + 1) + jj * j + k]) /
                  hx2;
    double phiy = (grid_data[ii * i + jj * (j - 1) + k] +
                   grid_data[ii * i + jj * (j + 1) + k]) /
                  hy2;
    double phiz = (grid_data[ii * i + jj * j + k - 1] +
                   grid_data[ii * i + jj * j + k + 1]) /
                  hz2;
    double x = p0.x + i * h.x;
    double y = p0.y + j * h.y;
    double z = p0.z + k * h.z;
    return (phix + phiy + phiz - ro(x, y, z)) /
           (2 / hx2 + 2 / hy2 + 2 / hz2 + alpha);
}
// Последовательное решение уравнения
static void sequentialSolution(Point D, Point N, DPoint p0, Result *result) {
    DPoint h = (DPoint){D.x / (N.x - 1.), D.y / (N.y - 1.), D.z / (N.z - 1.)};
    // Создание сетки
    double *grid_data = new double[N.x * N.y * N.z];
    // Инициализация сетки
    setBoundary(grid_data, N, h, p0);
    // Сетка последующих итераций
    double *new_data = new double[N.x * N.y * N.z];
    memcpy(new_data, grid_data, sizeof(double) * N.x * N.y * N.z);
    double jacobi_result = 0;
    int iters = 0;
    do{
        jacobi_result = 0;
        // Вычисление функции в узлах сетки
        for (int i = 1; i < N.x - 1; i++)
            for (int j = 1; j < N.y - 1; j++)
                for (int k = 1; k < N.z - 1; k++){
                    new_data[N.y * N.z * i + N.z * j + k] = jacobi(grid_data, N, h, p0, i, j, k);
                    jacobi_result = max(jacobi_result, fabs(grid_data[N.y * N.z * i + N.z * j + k] -
                                                            new_data[N.y * N.z * i + N.z * j + k]));
                }
        memcpy(grid_data, new_data, sizeof(double) * N.x * N.y * N.z);
        iters++;
        // Проверка порога сходимости и максимального кол-ва итераций
    } while (EPS < jacobi_result);
    delete grid_data;
    delete new_data;
    *result = (Result){jacobi_result, iters};
}
// Отправка граничных элементов между процессами
static void sendBorders(int rank, int size,
                        MPI_Request *down_send, MPI_Request *down_recv,
                        MPI_Request *up_send, MPI_Request *up_recv,
                        double *data, int local_size, int borders_offset, int block_size)
{
    // Отправка и приём верхних границ
    if (rank < size - 1){
        MPI_Isend(data + (local_size + borders_offset - block_size), block_size,
                  MPI_DOUBLE, rank + 1, 0, MPI_COMM_WORLD, down_send);
        MPI_Irecv(data + (local_size + borders_offset), block_size,
                  MPI_DOUBLE, rank + 1, 0, MPI_COMM_WORLD, down_recv);
    }
    // Отправка и приём нижних границ
    if (0 < rank){
        MPI_Isend(data + borders_offset, block_size,
                  MPI_DOUBLE, rank - 1, 0, MPI_COMM_WORLD, up_send);
        MPI_Irecv(data, block_size, MPI_DOUBLE, rank - 1, 0,
                  MPI_COMM_WORLD, up_recv);
    }
}
// Приём граничных элементов
static void recvBorders(int rank, int size,
                        MPI_Request *down_send, MPI_Request *down_recv,
                        MPI_Request *up_send, MPI_Request *up_recv)
{
    // Ожидание верхних границ
    if (rank < size - 1){
        MPI_Wait(down_send, MPI_STATUSES_IGNORE);
        MPI_Wait(down_recv, MPI_STATUSES_IGNORE);
    }
    // Ожидание нижних границ
    if (0 < rank){
        MPI_Wait(up_send, MPI_STATUSES_IGNORE);
        MPI_Wait(up_recv, MPI_STATUSES_IGNORE);
    }
}
// Обработка блока данных по оси D.x
static double processBlock(double *grid_data, double *new_data, Point N, DPoint h, DPoint p0,
                           int x_beg, int x_end, double current_result)
{
    double result = current_result;
    // Вычисление функции в узлах блока
    for (int i = x_beg; i < x_end; i++)
        for (int j = 1; j < N.y - 1; j++)
            for (int k = 1; k < N.z - 1; k++){
                new_data[N.y * N.z * i + N.z * j + k] = jacobi(grid_data, N, h, p0, i, j, k);
                result = max(result, fabs(grid_data[N.y * N.z * i + N.z * j + k] -
                                          new_data[N.y * N.z * i + N.z * j + k]));
            }
    return result;
}
// Обработка поля данных (блоков, не доходящих до границ)
static double processField(int rank, int size, double *data, double *new_data,
                           int local_size, int borders, int block_size,
                           Point N, DPoint h, DPoint p0, double current_result)
{
    double result = current_result;
    // Обработка блока 0-го процесса
    if (rank == 0){
        int x_beg = 1;
        int x_end = (local_size + borders) / block_size - 2;
        result = processBlock(data, new_data, N, h, p0, x_beg, x_end, result);
    }
    // Обработка блока последнего процесса
    if (rank == size - 1){
        int x_beg = 2;
        int x_end = (local_size + borders) / block_size - 1;
        result = processBlock(data, new_data, N, h, p0, x_beg, x_end, result);
    }
    // Обработка центральных блоков
    if (0 < rank && rank < size - 1){
        int x_beg = 2;
        int x_end = (local_size + borders) / block_size - 2;
        result = processBlock(data, new_data, N, h, p0, x_beg, x_end, result);
    }
    return result;
}
// Обработка границ
static double processBorders(int rank, int size, double *data, double *new_data,
                             int local_size, int borders, int block_size,
                             Point N, DPoint h, DPoint p0, double current_result)
{
    double result = current_result;
    int x_bottom = (local_size + borders) / block_size - 2;
    int x_upper = 1;
    // Обработка нижней границы
    if (rank < size - 1)
        result = processBlock(data, new_data, N, h, p0, x_bottom, x_bottom + 1, result);
    // Обработка верхней границы
    if (0 < rank)
        result = processBlock(data, new_data, N, h, p0, x_upper, x_upper + 1, result);
    return result;
}
// Параллельное решение уравнения
static void parallelSolution(Point D, Point N, DPoint p0, int rank, int size, Result *result){
    Result root_result;
    double *grid_data = NULL;
    int block_size = N.y * N.z;
    DPoint h = (DPoint){1.0 * D.x / (N.x - 1),
                        1.0 * D.y / (N.y - 1),
                        1.0 * D.z / (N.z - 1)};
    // Инициализация поля данных (сетки)
    if (rank == ROOT_RANK){
        grid_data = new double[N.x * block_size];
        setBoundary(grid_data, N, h, p0);
    }
    // Массив размеров локальных данных
    int *local_sizes = new int[size];
    // Массив размеров сдвигов локальных данных
    int *offsets = new int[size];
    int chunk_size = N.x / size;
    int remainder = N.x % size;
    // Определение размеров локальных данных
    int shift = 0;
    for (int i = 0; i != size; ++i){
        local_sizes[i] = chunk_size + (i < remainder ? 1 : 0);
        offsets[i] = shift; // Сдвиг строки локальных данных
        shift += local_sizes[i];
        local_sizes[i] *= block_size; // Получение размера локального блока
        offsets[i] *= block_size;     // Получение сдвига блока данных
    }
    // Определение границ и сдвигов границ
    int borders = 2 * block_size;    // Кол-во границ
    int borders_offset = block_size; // Сдвиг границ
    if (rank == 0 || rank == size - 1){
        borders = block_size;
        if (rank == 0)
            borders_offset = 0;
    }
    // Выделение памяти под блок данных и границы
    double *local_data = new double[local_sizes[rank] + borders];
    // Разбиение поля данных на блоки разного размера и передача другим процессам
    MPI_Scatterv(grid_data, local_sizes, offsets, MPI_DOUBLE, local_data + borders_offset,
                 local_sizes[rank], MPI_DOUBLE, ROOT_RANK, MPI_COMM_WORLD);
    // Сетка последующих итераций
    double *new_data = new double[local_sizes[rank] + borders];
    memcpy(new_data, local_data, sizeof(double) * (local_sizes[rank] + borders));
    double jacobi_result = 0;
    int iters = 0;
    do{
        jacobi_result = 0;
        MPI_Request down_send;
        MPI_Request down_recv;
        MPI_Request up_send;
        MPI_Request up_recv;
        // Посылаем границы другим процессам
        sendBorders(rank, size, &down_send, &down_recv, &up_send, &up_recv,
                    local_data, local_sizes[rank], borders_offset, block_size);
        // Обрабатываем поле данных (без границ)
        jacobi_result = processField(rank, size, local_data, new_data,
                                     local_sizes[rank], borders, block_size,
                                     N, h, p0, jacobi_result);
        // Принимаем границы
        recvBorders(rank, size, &down_send, &down_recv, &up_send, &up_recv);
        // Обрабатываем границы
        processBorders(rank, size, local_data, new_data,
                       local_sizes[rank], borders, block_size,
                       N, h, p0, jacobi_result);
        // Обмениваемся результатми между процессами и находи max
        MPI_Allreduce(MPI_IN_PLACE, &jacobi_result, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
        // Обновление сетки для следующей итерации
        double *tmp = local_data;
        local_data = new_data;
        new_data = tmp;
        iters++;
        // Проверка порога сходимости и максимального кол-ва итераций
    } while (EPS < jacobi_result);
    delete new_data;
    delete local_sizes;
    delete offsets;
    if (rank == 0){
        delete grid_data;
        *result = (Result){jacobi_result, iters};
    }
}
void solver(Point D, Point N, DPoint p0, Result *result) {
    int rank = ROOT_RANK;
    int size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (size == 1 || N.x / size < 3)
        sequentialSolution(D, N, p0, result);
    else
        parallelSolution(D, N, p0, rank, size, result);
}
int main(int argc, char *argv[]) {
    int rank = 0;
    int size = 1;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (argc != 2){
        if (rank == ROOT_RANK)
            cerr << "Wrong number of arguments!\nEnter:\n<N.x> <N.y> <N.z>\n";
        MPI_Finalize();
        exit(EXIT_FAILURE);
    }
    Point D = (Point){2, 2, 2};
    Point N = (Point){atoi(argv[1]), atoi(argv[1]), atoi(argv[1])};
    DPoint p0 = (DPoint){-1, -1, -1};
    MPI_Barrier(MPI_COMM_WORLD);
    struct timespec start, stop;
    clock_gettime(CLOCK_MONOTONIC, &start);
    Result res;
    solver(D, N, p0, &res);
    MPI_Barrier(MPI_COMM_WORLD);
    clock_gettime(CLOCK_MONOTONIC, &stop);
    if (rank == ROOT_RANK){
        cout << "Result:" << res.result << "\nIters:" << res.iters << endl
             << "Elapsed time:" << clocktimeDifference(start, stop) << endl;
    }
    MPI_Finalize();
    return 0;
}