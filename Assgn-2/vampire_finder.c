#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <time.h>

// Mutex for synchronization
HANDLE mutex;

// File handle for the output file
HANDLE outFile;

// Comparison function for qsort
int compareChars(const void *a, const void *b) {
    return (*(char *)a - *(char *)b);
}

// Function to get the required fangs of the vampire number
struct Fangs {
    char x[10];
    char y[10];
};

int next_permutation(char *begin, char *end) {
    // Implementation of a simple next_permutation
    char *i = end - 1;
    while (i > begin && *(i - 1) >= *i)
        --i;

    if (i == begin)
        return 0;

    char *j = end - 1;
    while (*j <= *(i - 1))
        --j;

    // Swap i-1 and j
    char temp = *(i - 1);
    *(i - 1) = *j;
    *j = temp;

    // Reverse the suffix
    j = end - 1;
    while (i < j) {
        temp = *i;
        *i = *j;
        *j = temp;
        ++i;
        --j;
    }

    return 1;
}

// Structure to represent a range of numbers assigned to a thread
typedef struct {
    int start;
    int end;
    int threadID;
    int vampireCount;
} Range;

// Function to check whether the given number is vampire or not
int isVampire(int m_int, int *totalVampireCount, int threadID) {
    // converting the vampire number to string
    char n_str[10];
    sprintf(n_str, "%d", m_int);

    // if no of digits in the number is odd then return false
    if (strlen(n_str) % 2 == 1) {
        return 0;
    }

    // getting the fangs of the number
    struct Fangs fangs = {"", ""};
    char num_vec[10];
    strcpy(num_vec, n_str);
    int n = strlen(n_str);
    qsort(num_vec, n, sizeof(char), compareChars);

    do {
        char x_str[10] = "", y_str[10] = "";
        for (int i = 0; i < n / 2; i++) {
            strncat(x_str, &num_vec[i], 1);
        }
        for (int i = n / 2; i < n; i++) {
            strncat(y_str, &num_vec[i], 1);
        }

        // if numbers have trailing zeroes then skip
        if (x_str[strlen(x_str) - 1] == '0' && y_str[strlen(y_str) - 1] == '0') {
            continue;
        }

        // if x * y is equal to the vampire number then return true
        if (atoi(x_str) * atoi(y_str) == m_int) {
            // Log the vampire number and thread ID
            WaitForSingleObject(mutex, INFINITE);

            outFile = CreateFile("OutFile.txt", FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (outFile != INVALID_HANDLE_VALUE) {
                char buffer[100];
                sprintf(buffer, "%d: Found by Thread %d\n", m_int, threadID);
                DWORD bytesWritten;
                WriteFile(outFile, buffer, strlen(buffer), &bytesWritten, NULL);
                CloseHandle(outFile);
            }

            ReleaseMutex(mutex);

            // Increment the total vampire count
            InterlockedIncrement(totalVampireCount);

            return 1;
        }
    } while (next_permutation(num_vec, num_vec + n));

    return 0;
}

// Function for the thread to check vampire numbers in a range
DWORD WINAPI ThreadFunction(LPVOID arg) {
    Range *range = (Range *)arg;

    for (int num = range->start; num <= range->end; ++num) {
        isVampire(num, &(range->vampireCount), range->threadID);
    }

    // Exit the thread
    return 0;
}

// Function to partition numbers from 1 to N among M threads
void PartitionNumbers(int N, int M, Range *ranges) {
    int rangeSize = N / (2 * M);
    int remainder = N % (2 * M);

    int currentStart = 1;
    int currentEnd = rangeSize;

    for (int i = 0; i < M; ++i) {
        ranges[i].start = currentStart;
        ranges[i].end = currentEnd;
        ranges[i].threadID = i + 1;
        ranges[i].vampireCount = 0;

        currentStart = currentEnd + 1;

        if (i == M - 1) {
            // Adjust the end value for the last thread to include the remainder
            ranges[i].end += remainder;
        } else {
            currentEnd = currentStart + rangeSize - 1;
        }
    }
}

int main() {
    clock_t tic = clock();

    // Read input values N and M from the input file
    FILE *inputFile = fopen("input.txt", "r");
    if (inputFile == NULL) {
        fprintf(stderr, "Error opening input file.\n");
        exit(EXIT_FAILURE);
    }

    int N, M;
    fscanf(inputFile, "%d %d", &N, &M);
    fclose(inputFile);


    // Create M threads
    HANDLE threads[M];

    // Initialize mutex
    mutex = CreateMutex(NULL, FALSE, NULL);
    if (mutex == NULL) {
        fprintf(stderr, "Mutex creation failed.\n");
        exit(EXIT_FAILURE);
    }

    // Divide the numbers from 1 to N among threads
    Range ranges[M];
    PartitionNumbers(N, M, ranges);

    // Launch threads
    for (int i = 0; i < M; ++i) {
        threads[i] = CreateThread(NULL, 0, ThreadFunction, (LPVOID)&ranges[i], 0, NULL);
        if (threads[i] == NULL) {
            fprintf(stderr, "Thread creation failed for Thread %d.\n", i + 1);
            exit(EXIT_FAILURE);
        }
        // printf("Thread %d created successfully.\n", i + 1);
    }

    // Wait for all threads to finish
    WaitForMultipleObjects(M, threads, TRUE, INFINITE);

    // Print the total count of vampire numbers to the console
    int totalVampireCount = 0;
    for (int i = 0; i < M; ++i) {
        totalVampireCount += ranges[i].vampireCount;
    }
    printf("Total Vampire numbers: %d\n", totalVampireCount);

    // Cleanup
    CloseHandle(mutex);

    clock_t toc = clock();

    printf("Elapsed: %f seconds\n", (double)(toc - tic) / CLOCKS_PER_SEC);
    return 0;
}
