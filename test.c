#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <time.h>

// Declarations of your functions (adjust types if your signatures differ)
extern void* my_malloc(size_t size);
extern void  my_free(void* ptr);
extern void init();

#define ALIGNMENT 8
#define UNUSED(x) (void)(x)

// Helper to check if a pointer is correctly aligned
static int is_aligned(void* ptr, size_t alignment) {
    return ((uintptr_t)ptr % alignment) == 0;
}

// ============================================================================
// TEST CASES
// ============================================================================

// Test 1: Basic Allocation and Freeing
void test_basic_alloc_free() {
    printf("[+] Running Test 1: Basic Alloc/Free...\n");
    
    int* ptr = (int*)my_malloc(sizeof(int));
    assert(ptr != NULL && "Allocation failed for a simple integer");
    assert(is_aligned(ptr, ALIGNMENT) && "Pointer is not 8-byte aligned");
    
    *ptr = 42; // Ensure we can write to it
    assert(*ptr == 42);
    
    my_free(ptr);
    printf("    Passed.\n");
}

// Test 2: Edge Cases (Zero size, massive size)
void test_edge_cases() {
    printf("[+] Running Test 2: Edge Cases...\n");
    
    // Behavior for malloc(0) is implementation-defined, but it should either 
    // return NULL or a valid unique pointer that can be safely passed to free().
    void* zero_ptr = my_malloc(0);
    my_free(zero_ptr); 

    // Test a massive allocation that should gracefully fail (return NULL), not crash
    void* huge_ptr = my_malloc((size_t)-1); // Max size_t
    assert(huge_ptr == NULL && "Allocator should fail gracefully on integer overflow/massive requests");
    
    // Freeing a NULL pointer should be a safe no-op
    my_free(NULL);
    
    printf("    Passed.\n");
}

// Test 3: Alignment Verification
void test_alignment() {
    printf("[+] Running Test 3: Alignment Enforcements...\n");
    
    size_t sizes[] = {1, 3, 5, 7, 8, 9, 15, 16, 17, 1023, 1024};
    void* ptrs[11];
    
    for (int i = 0; i < 11; i++) {
        ptrs[i] = my_malloc(sizes[i]);
        assert(ptrs[i] != NULL);
        assert(is_aligned(ptrs[i], ALIGNMENT) && "Allocation did not meet alignment requirements");
        
        // Write data to ensure it's valid memory
        memset(ptrs[i], 0xAA, sizes[i]);
    }
    
    for (int i = 0; i < 11; i++) {
        my_free(ptrs[i]);
    }
    printf("    Passed.\n");
}

// Test 4: Coalescing (Merging adjacent free blocks)
void test_coalescing() {
    printf("[+] Running Test 4: Block Coalescing...\n");
    
    // Allocate 3 contiguous blocks
    char* p1 = (char*)my_malloc(128);
    char* p2 = (char*)my_malloc(128);
    char* p3 = (char*)my_malloc(128);
    
    assert(p1 && p2 && p3);
    
    // Free the middle and chunks around it
    my_free(p2);
    my_free(p1);
    my_free(p3);
    
    // If coalescing works, we should be able to allocate a single chunk 
    // roughly the size of all three combined without triggering a new sbrk/heap expansion.
    char* big_p = (char*)my_malloc(300);
    assert(big_p != NULL && "Coalescing failed: unable to reuse merged adjacent blocks");
    
    my_free(big_p);
    printf("    Passed.\n");
}

// Test 5: High-Stress Random Allocation & Freeing (De-fragmentation check)
void test_stress_and_fragmentation() {
    printf("[+] Running Test 5: High-Stress Random Allocation...\n");
    
    #define NUM_POINTERS 200
    void* ptrs[NUM_POINTERS] = {NULL};
    size_t sizes[NUM_POINTERS];
    
    srand((unsigned int)time(NULL));
    
    // Phase 1: Fill up the allocator with various sizes
    for (int i = 0; i < NUM_POINTERS; i++) {
        sizes[i] = (rand() % 512) + 1; // Sizes from 1 to 512 bytes
        ptrs[i] = my_malloc(sizes[i]);
        assert(ptrs[i] != NULL && "Stress test ran out of memory prematurely");
        memset(ptrs[i], 0xFF, sizes[i]); // Check writeability
    }
    
    // Phase 2: Randomly free half of the blocks to create a fragmented heap
    for (int i = 0; i < NUM_POINTERS; i += 2) {
        my_free(ptrs[i]);
        ptrs[i] = NULL;
    }
    
    // Phase 3: Try re-allocating new sizes into the fragmented holes
    for (int i = 0; i < NUM_POINTERS; i += 2) {
        sizes[i] = (rand() % 128) + 1; // Smaller sizes to fit in holes
        ptrs[i] = my_malloc(sizes[i]);
        assert(ptrs[i] != NULL && "Failed to reallocate into fragmented space");
        memset(ptrs[i], 0xEE, sizes[i]);
    }
    
    // Phase 4: Clean up everything
    for (int i = 0; i < NUM_POINTERS; i++) {
        if (ptrs[i] != NULL) {
            my_free(ptrs[i]);
        }
    }
    printf("    Passed.\n");
}

// ============================================================================
// MAIN EXECUTION
// ============================================================================
/*
int main() {
    printf("===========================================\n");
    printf("STARTING MY_MALLOC / MY_FREE TEST SUITE\n");
    printf("===========================================\n");
    init();
    test_basic_alloc_free();
    test_edge_cases();
    test_alignment();
    test_coalescing();
    test_stress_and_fragmentation();
    
    printf("===========================================\n");
    printf("SUCCESS: All tests passed flawlessly!\n");
    printf("===========================================\n");
    return 0;
}
*/

#define SMALL_COUNT 100000  // 100k allocations
#define SMALL_SIZE  32      // 32 bytes each

#define LARGE_COUNT 100     // 100 allocations
#define LARGE_SIZE  1048576 // 1 Megabyte each

// Helper to get time in seconds
double get_time_seconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

void run_test_small_standard() {
    void** ptrs = malloc(sizeof(void*) * SMALL_COUNT);
    
    double start = get_time_seconds();
    for(int i = 0; i < SMALL_COUNT; i++) {
        ptrs[i] = malloc(SMALL_SIZE);
    }
    for(int i = 0; i < SMALL_COUNT; i++) {
        free(ptrs[i]);
    }
    double end = get_time_seconds();
    
    printf("Standard malloc/free (Small): %f seconds\n", end - start);
    free(ptrs);
}

void run_test_small_custom() {
    // Using standard malloc just to hold the array of pointers for the test
    void** ptrs = malloc(sizeof(void*) * SMALL_COUNT);
    
    double start = get_time_seconds();
    for(int i = 0; i < SMALL_COUNT; i++) {
        ptrs[i] = my_malloc(SMALL_SIZE);
        if (ptrs[i] == NULL) {
            printf("Custom allocator ran out of memory during small test!\n");
            break;
        }
    }
    for(int i = 0; i < SMALL_COUNT; i++) {
        if (ptrs[i] != NULL) my_free(ptrs[i]);
    }
    double end = get_time_seconds();
    
    printf("Custom my_malloc/free (Small): %f seconds\n", end - start);
    free(ptrs);
}

void run_test_large_standard() {
    void** ptrs = malloc(sizeof(void*) * LARGE_COUNT);
    
    double start = get_time_seconds();
    for(int i = 0; i < LARGE_COUNT; i++) {
        ptrs[i] = malloc(LARGE_SIZE);
    }
    for(int i = 0; i < LARGE_COUNT; i++) {
        free(ptrs[i]);
    }
    double end = get_time_seconds();
    
    printf("Standard malloc/free (Large): %f seconds\n", end - start);
    free(ptrs);
}

void run_test_large_custom() {
    void** ptrs = malloc(sizeof(void*) * LARGE_COUNT);
    
    double start = get_time_seconds();
    for(int i = 0; i < LARGE_COUNT; i++) {
        ptrs[i] = my_malloc(LARGE_SIZE);
        if (ptrs[i] == NULL) {
            printf("Custom allocator ran out of memory during large test!\n");
            break;
        }
    }
    for(int i = 0; i < LARGE_COUNT; i++) {
        if (ptrs[i] != NULL) my_free(ptrs[i]);
    }
    double end = get_time_seconds();
    
    printf("Custom my_malloc/free (Large): %f seconds\n", end - start);
    free(ptrs);
}

int main() {
    printf("=== Starting Memory Allocator Benchmark ===\n\n");
    
    printf("--- Test 1: %d allocations of %d bytes ---\n", SMALL_COUNT, SMALL_SIZE);
    init();
    run_test_small_standard();
    run_test_small_custom();
    
    printf("\n--- Test 2: %d allocations of %d bytes (1MB) ---\n", LARGE_COUNT, LARGE_SIZE);
    run_test_large_standard();
    run_test_large_custom();
    
    return 0;
}
