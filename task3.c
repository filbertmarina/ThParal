#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <cuda_runtime.h>

#include <cublas_v2.h>


///////////////////////////////////////////////////
// Фунция main
///////////////////////////////////////////////////
int main(int argc, char**argv) {

//Установка параметров 
int SIZE;
double err_max;
int iter_max;
SIZE = atoi(argv[2]);
err_max=atof(argv[1]);
iter_max = atoi(argv[3]);
	
	
	cublasHandle_t handle;
	cublasStatus_t stat;

	stat = cublasCreate(&handle);
	if (stat != CUBLAS_STATUS_SUCCESS) {
		printf("CUBLAS initialization failed\n");
		return EXIT_FAILURE;
	}
//Запуск clock();
	clock_t start;
	start = clock();

	
	//////////////////////////////////
	//Заполнение массивов mas, mas_old, выделение памяти на cpu
	//////////////////////////////////
	double* mas = (double*)calloc(SIZE * SIZE, sizeof(double));
	double* mas_old = (double*)calloc(SIZE * SIZE, sizeof(double));

	mas[0] = 10;
	mas[SIZE - 1] = 20;
	mas[(SIZE) * (SIZE - 1)] = 20;
	mas[(SIZE) * (SIZE)-1] = 30;

	for (int i = 1; i < SIZE - 1; i++)
		mas[i] = mas[i - 1] + (mas[SIZE - 1] - mas[0]) / SIZE;

	for (int i = 1; i < SIZE - 1; i++) {
		mas[SIZE * (SIZE - 1) + i] = mas[SIZE * (SIZE - 1) + i - 1] + (mas[(SIZE) * (SIZE)-1] - mas[(SIZE) * (SIZE - 1)]) / SIZE;
		mas[(SIZE) * (i)] = mas[(i - 1) * (SIZE)] + (mas[(SIZE) * (SIZE - 1)] - mas[0]) / SIZE;
		mas[(SIZE) * (i)+(SIZE - 1)] = mas[(SIZE) * (i - 1) + (SIZE - 1)] + (mas[(SIZE) * (SIZE)-1] - mas[SIZE - 1]) / SIZE;
	}

	for (int i = 0; i < SIZE * SIZE; i++){
	mas_old[i] = mas[i];
}
	
	
	
	int iter = 0;
	double err = 1.0;

	
	///////////////////////////////////////////////////////////////////////
	//Основной цикл программы: проверка максимальных значений, нахождение вектора ошибок, нахождение максимальной ошибки,
	// перезапись mas в mas_old
	///////////////////////////////////////////////////////////////////////
#pragma acc enter data copyin(mas[0:SIZE*SIZE], mas_old[0:SIZE*SIZE]) 
	{
			double a = -1;
				int index = 0;


		while ((err > err_max) && iter < iter_max) {

			

#pragma acc data present(mas, mas_old)
#pragma acc parallel loop independent collapse(2) async
			for (int i = 1; i < SIZE - 1; i++)
				for (int j = 1; j < SIZE - 1; j++)
					mas[i * SIZE + j] =0.25* (mas_old[i * SIZE + j - 1] + mas_old[(i - 1) * SIZE + j] + mas_old[(i + 1) * SIZE + j] + mas_old[i * SIZE + j + 1]);





			if (iter % 100 == 0)
			{

#pragma acc wait
#pragma acc data present(mas, mas_old)
#pragma acc host_data use_device(mas,mas_old)
				{

	//(на gpu)нахождение вектора разницы массивов mas и mas_old, запись в mas_old
				stat = cublasDaxpy(handle, SIZE * SIZE, &a, (const double*)mas, 1, mas_old, 1);
					if (stat != CUBLAS_STATUS_SUCCESS) {
						printf("CUBLAS initialization failed1\n");
						return EXIT_FAILURE;
					}


	//(на gpu)нахождение индекса максимального значения в mas_old
					stat =cublasIdamax(handle, SIZE*SIZE, mas_old, 1, &index);
					if (stat != CUBLAS_STATUS_SUCCESS) {
						printf("CUBLAS initialization failed2\n");
						return EXIT_FAILURE;
					}



				}

//обновляем найденную ошибку на cpu
#pragma acc update host(mas_old[index - 1]) 
				err = fabs(mas_old[index - 1]);


//перезапись массивов
#pragma acc host_data use_device(mas_old, mas)
				stat = cublasDcopy(handle, SIZE * SIZE, mas, 1, mas_old, 1);
			}

			
			double* m = mas;
			mas = mas_old;
			mas_old = m;


iter += 1;

//вывод времени работы и ошибки 
			if (iter % 100 == 0 && iter != 1) {
				double t = (double)(clock() - start) / CLOCKS_PER_SEC;
				printf(" time: %lf\n", t);
				printf("%d  %lf", iter, err);
				printf("\n");
			}
		}
	}
	
	cublasDestroy(handle);

//using free to free cpu memory
	free(mas_old);
	free(mas);

//вывод общего времени работы программы, функции clock()
	double t = (double)(clock() - start) / CLOCKS_PER_SEC;
	printf(" time: %lf\n", t);
	return EXIT_SUCCESS;

}




