#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <limits.h>
#include <errno.h>

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




// Function to check if the given string is a valid integer
int is_valid_integer(char *str) {
    char *endptr;
    errno = 0;
    long val = strtol(str, &endptr, 10);

    // Check for errors: e.g., the string does not represent an integer
    // or the integer is too small or too large
    if (endptr == str || *endptr != '\0' || errno == ERANGE || val < INT_MIN || val > INT_MAX) {
        return 0; // Not valid
    }
    return 1; // Valid
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Usage: %s <start> <end> <threads> <method>\n", argv[0]);
        return 1;
    }

    // Validate inputs and convert to integers
    if (!is_valid_integer(argv[1]) || !is_valid_integer(argv[2]) ||
        !is_valid_integer(argv[3]) || !is_valid_integer(argv[4])) {
        printf("Error: All inputs must be integers.\n");
        return 1;
    }

    // Declaring the user inputs
    long long int a = atoll(argv[1]);
    long long int b = atoll(argv[2]);
    int c = atoi(argv[3]);
    int d = atoi(argv[4]);

    // Check for valid range and method
    if (a < 0 || b < 0 || a > b || c <= 0 || d < 1 || d > 3) {
        printf("Error: Invalid range or method number.\n");
        return 1;
    }

    pthread_t threads[c];
    pthread_mutex_init(&mutex, NULL);

    // Creating threads and dividing the ranges
    long long int range = (b - a + 1) / c;
    for (int i = 0; i < c; i++) {
        long long int *arg = malloc(2 * sizeof(long long int));
        arg[0] = a + i * range;                    // Start of range
        arg[1] = (i == c - 1) ? b : arg[0] + range - 1; // End of range                            

        // Error checks and the creation of threads
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

    // waiting the threads
    for (int i = 0; i < c; i++) {
        if(pthread_join(threads[i], NULL) != 0) {
            perror("Failed to wait thread");
            return 1;
        }
    }

    pthread_mutex_destroy(&mutex);
    printf("Sum: %e\n", global_sqrt_sum);
    return 0;
}
