#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    // Cast the parameter to the thread_data structure
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    if (!thread_func_args) {
        ERROR_LOG("thread_func_args is NULL\n");
        return NULL;
    }

    // Wait for the specified time before attempting to lock the mutex
    usleep(thread_func_args->wait_to_obtain_ms * 1000);

    // Attempt to lock the mutex
    if (pthread_mutex_lock(thread_func_args->mutex) != 0) {
        ERROR_LOG("Failed to lock mutex\n");
        return NULL;
    }
    DEBUG_LOG("Mutex locked\n");

    // Wait for the specified time before releasing the mutex
    usleep(thread_func_args->wait_to_release_ms * 1000);

    // Unlock the mutex
    if (pthread_mutex_unlock(thread_func_args->mutex) != 0) {
        ERROR_LOG("Failed to unlock mutex\n");
        return NULL;
    }
    DEBUG_LOG("Mutex unlocked\n");

    // Set the thread completion status to true
    thread_func_args->thread_complete_success = true;

    return thread_param;
}

bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex, int wait_to_obtain_ms, int wait_to_release_ms)
{
    // Allocate memory for the thread_data structure
    struct thread_data* thread_func_args = malloc(sizeof(struct thread_data));
    if (!thread_func_args) {
        ERROR_LOG("Failed to allocate memory for thread_data\n");
        return false;
    }

    // Initialize the thread_data structure
    thread_func_args->mutex = mutex;
    thread_func_args->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_func_args->wait_to_release_ms = wait_to_release_ms;
    thread_func_args->thread_complete_success = false;

    // Create the thread
    if (pthread_create(thread, NULL, threadfunc, thread_func_args) != 0) {
        ERROR_LOG("Failed to create thread\n");
        free(thread_func_args);
        return false;
    }

    DEBUG_LOG("Thread created successfully\n");
    return true;
}

