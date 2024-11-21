#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <time.h>
#include <sys/wait.h>

#define MAX_NUM 1000

// Structura pentru a stoca numărul și semaforul
typedef struct {
    int number;
    sem_t sem;
} shared_data;

int main() {
    int md;
    shared_data *data;
    pid_t pid;
    long pg_size;
    char *name = "my_shared_mem";

    // Crearea obiectului de memorie partajată
    md = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (md == -1) {
        perror("shm_open failed");
        exit(1);
    }

    pg_size = sysconf(_SC_PAGE_SIZE);
    if (ftruncate(md, pg_size) == -1) {
        perror("ftruncate failed");
        exit(1);
    }

    // Mapează memoria partajată în spațiul de memorie al procesului
    data = (shared_data *) mmap(NULL, pg_size, PROT_READ | PROT_WRITE, MAP_SHARED, md, 0);
    if (data == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }

    // Inițializarea semaforului
    if (sem_init(&data->sem, 1, 1) == -1) {
        perror("sem_init failed");
        exit(1);
    }

    data->number = 1; // Pornim numărătoarea de la 1

    // Crearea unui proces copil
    pid = fork();
    if (pid == 0) {
        // Procesul copil
        while (data->number <= MAX_NUM) {
            sem_wait(&data->sem); // Blochează semaforul

            // "Aruncă banul" (random 0 sau 1)
            if (rand() % 2 == 1 && data->number <= MAX_NUM) {
                printf("Process 1 writes %d\n", data->number);
                data->number++;
            }

            sem_post(&data->sem); // Eliberează semaforul
            usleep(10); // Întârziere pentru a simula un interval de timp
        }
        exit(0);
    } else if (pid > 0) {
        // Procesul părinte
        while (data->number <= MAX_NUM) {
            sem_wait(&data->sem); // Blochează semaforul

            // "Aruncă banul" (random 0 sau 1)
            if (rand() % 2 == 1 && data->number <= MAX_NUM) {
                printf("Process 2 writes %d\n", data->number);
                data->number++;
            }

            sem_post(&data->sem); // Eliberează semaforul
            usleep(10); // Întârziere pentru a simula un interval de timp
        }

        // Așteaptă procesul copil
        wait(NULL);
    } else {
        perror("fork failed");
        exit(1);
    }

    // Curățarea: închide și șterge obiectul de memorie partajată
    sem_destroy(&data->sem);
    munmap(data, pg_size);
    shm_unlink(name);

    return 0;
}

