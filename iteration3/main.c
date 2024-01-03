#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>

// Global variables
double global_sqrt_sum = 0;
pthread_mutex_t mutex;

// Thread function
void *calculate_sqrt_sum1(void *arg) {
    long long int *range = (long long int *)arg;
    double local_sqrt_sum = 0;
	// Method 1: No synchronization
    for (long long int i = range[0]; i <= range[1]; i++) {
        double sqrt_val = sqrt(i);
        global_sqrt_sum += sqrt_val;
    }
    
    free(arg);
    return NULL;
}
void *calculate_sqrt_sum2(void *arg) {
    long long int *range = (long long int *)arg;
    double local_sqrt_sum = 0;

    for (long long int i = range[0]; i <= range[1]; i++) {
    // Method 2: Mutex for entire sum
        double sqrt_val = sqrt(i);
        pthread_mutex_lock(&mutex);  
        global_sqrt_sum += sqrt_val;
        pthread_mutex_unlock(&mutex);
    }


    free(arg);
    return NULL;
}
void *calculate_sqrt_sum3(void *arg) {
    long long int *range = (long long int *)arg;
    double local_sqrt_sum = 0;
	// Method 3: Mutex for adding local sum
    for (long long int i = range[0]; i <= range[1]; i++) {
        double sqrt_val = sqrt(i);
        local_sqrt_sum += sqrt_val;
    }

    
    pthread_mutex_lock(&mutex);         // Method 3: Mutex for adding local sum
    global_sqrt_sum += local_sqrt_sum;
    pthread_mutex_unlock(&mutex);
    

    free(arg);
    return NULL;
}
int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Usage: %s <start> <end> <threads> <method>\n", argv[0]);
        return 1;
    }

    long long int a = atoll(argv[1]);
    long long int b = atoll(argv[2]);
    int c = atoi(argv[3]);
    int d = atoi(argv[4]);

    pthread_t threads[c];
    pthread_mutex_init(&mutex, NULL);

    long long int range = (b - a + 1) / c;
    for (int i = 0; i < c; i++) {
        long long int *arg = malloc(2 * sizeof(long long int));
        arg[0] = a + i * range;                    // Start of range
        arg[1] = (i == c - 1) ? b : arg[0] + range - 1; // End of range                            

        
        if (d == 1 && pthread_create(&threads[i], NULL, calculate_sqrt_sum1, arg) != 0) {
            perror("Failed to create thread");
            return 1;
        }
        if (d == 2 && pthread_create(&threads[i], NULL, calculate_sqrt_sum2, arg) != 0) {
            perror("Failed to create thread");
            return 1;
        }
        if (d == 3 && pthread_create(&threads[i], NULL, calculate_sqrt_sum3, arg) != 0) {
            perror("Failed to create thread");
            return 1;
        }
    }

    for (int i = 0; i < c; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&mutex);
    printf("Sum: %e\n", global_sqrt_sum);
    return 0;
}
