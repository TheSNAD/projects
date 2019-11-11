#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc, char * argv[]) {
	int n = 10;
	
    int mat[n][n], **res;
    
    for (int i = 0; i < n; ++i){
        for (int j = 0; j < n; ++j){
            mat[i][j] = i * n + j;
        }
    }
	
		int size, id;
		double time;
		MPI_Init(&argc, &argv);
		MPI_Comm_size(MPI_COMM_WORLD, &size);
		MPI_Comm_rank(MPI_COMM_WORLD, &id);
		
		if (size < 2){
			printf("Очень мало процессов\n");
			MPI_Finalize();
			exit(1);
		}
		
		if (id == 0){
			res = malloc(n * sizeof(res[0]));
			for (int i = 0; i < n; ++i){
				res[i] = malloc(n * sizeof(int));
			}
			time = MPI_Wtime();
		}

		MPI_Barrier(MPI_COMM_WORLD);
		
		if (id != 0){
			for (int i = (id - 1); i < n; i += (size - 1)){
				
				printf("%d: ", i);
		        int loc_arr[n]; 
				for (int j = 0; j < n; ++j)
					loc_arr[j] = mat[j][i];
				
				MPI_Gather(loc_arr, n, MPI_INT, res[i], n, MPI_INT, 0, MPI_COMM_WORLD);
			}
		}
		MPI_Barrier(MPI_COMM_WORLD);
		if (id == 0){
			printf("Исходная матрица: \n");
			for (int i = 0; i < n; ++i){
				for (int j = 0; j < n; ++j){
					printf("%d ", i * n + j);
				}
				printf("\n");
			}
			printf("Транспонированная матрица: \n");
			for (int i = 0; i < n; ++i){
				for (int j = 0; j < n; ++j){
					printf("%d ", res[i][j]);
				}
				printf("\n");
			}
			printf("Количество процессов: %d\n", size);
			printf("Время: %lf\n", MPI_Wtime() - time);
			for (int i = 0; i < n; ++i){
				free(res[i]);
			}
			free(res);
		}	
	return 0;
}