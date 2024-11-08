#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <ctype.h> 
#include <mpi.h> 
 
#define HASH_SIZE 10000 
#define WORD_SIZE 50 
#define CHUNK_SIZE 1024  // Tamaño del fragmento que cada proceso leerá del archivo 
 
// Estructura para cada entrada en la hash table 
typedef struct Entry { 
    char word[WORD_SIZE]; 
    int count; 
    struct Entry *next; 
} Entry; 
 
// Hash table 
Entry *hashTable[HASH_SIZE]; 
 
// Función hash 
unsigned int hash(const char *word) { 
    unsigned int hashValue = 0; 
    while (*word) { 
        hashValue = (hashValue * 31 + *word) % HASH_SIZE; 
        word++; 
    } 
    return hashValue; 
} 
 
// Inserta o incrementa una palabra en la hash table 
void insertWord(const char *word) { 
    unsigned int index = hash(word); 
    Entry *entry = hashTable[index]; 
 
    // Busca si la palabra ya está en la lista 
    while (entry != NULL) { 
        if (strcmp(entry->word, word) == 0) { 
            entry->count++; 
            return; 
        } 
        entry = entry->next; 
    } 
 
    // Si la palabra no está, crear una nueva entrada 
    entry = (Entry *)malloc(sizeof(Entry)); 
    strcpy(entry->word, word); 
    entry->count = 1; 
    entry->next = hashTable[index]; 
    hashTable[index] = entry; 
} 
 
// Función para limpiar la hash table 
void clearHashTable() { 
    for (int i = 0; i < HASH_SIZE; i++) { 
        Entry *entry = hashTable[i]; 
        while (entry != NULL) { 
            Entry *temp = entry; 
            entry = entry->next; 
            free(temp); 
        } 
        hashTable[i] = NULL; 
    } 
} 
 
// Procesa una porción de texto y cuenta las palabras 
void processChunk(char *chunk) { 
    char *word = strtok(chunk, " \n\t.,;:!?\"'()"); 
    while (word != NULL) { 
        for (int i = 0; word[i]; i++) { 
            word[i] = tolower(word[i]);  // Convertir a minúsculas 
        } 
        insertWord(word); 
        word = strtok(NULL, " \n\t.,;:!?\"'()"); 
    } 
} 
 
// Encuentra la palabra más frecuente localmente en cada proceso 
void findMostFrequentWordLocal(char *mostFrequentWord, int *highestCount) { 
    *highestCount = 0; 
    for (int i = 0; i < HASH_SIZE; i++) { 
        Entry *entry = hashTable[i]; 
        while (entry != NULL) { 
            if (entry->count > *highestCount) { 
                *highestCount = entry->count; 
                strcpy(mostFrequentWord, entry->word); 
            } 
            entry = entry->next; 
        } 
    } 
} 
 
int main(int argc, char **argv) { 
    int rank, size; 
    MPI_Init(&argc, &argv); 
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); 
    MPI_Comm_size(MPI_COMM_WORLD, &size); 
 
    if (argc < 2) { 
        if (rank == 0) { 
            printf("Uso: mpirun -np <n> %s <archivo_texto>\n", argv[0]); 
        } 
        MPI_Finalize(); 
        return 0; 
    } 
 
    const char *filename = argv[1]; 
    FILE *file; 
    long fileSize; 
 
    if (rank == 0) { 
        file = fopen(filename, "r"); 
        if (!file) { 
            printf("Error al abrir el archivo.\n"); 
            MPI_Abort(MPI_COMM_WORLD, 1); 
        } 
        fseek(file, 0, SEEK_END); 
        fileSize = ftell(file); 
        fseek(file, 0, SEEK_SET); 
    } 
 
    // Compartir el tamaño del archivo con todos los procesos 
    MPI_Bcast(&fileSize, 1, MPI_LONG, 0, MPI_COMM_WORLD); 
 
    // Cada proceso procesa un fragmento 
    long chunkSize = fileSize / size; 
    char *chunk = (char *)malloc(chunkSize + 1); 
    memset(chunk, 0, chunkSize + 1); 
 
    if (rank == 0) { 
        for (int i = 1; i < size; i++) { 
            fseek(file, i * chunkSize, SEEK_SET); 
            fread(chunk, 1, chunkSize, file); 
            MPI_Send(chunk, chunkSize, MPI_CHAR, i, 0, MPI_COMM_WORLD); 
        } 
        fseek(file, 0, SEEK_SET); 
        fread(chunk, 1, chunkSize, file); 
        fclose(file); 
    } else { 
        MPI_Recv(chunk, chunkSize, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); 
    } 
 
    // Procesar el fragmento en cada proceso 
    processChunk(chunk); 
    free(chunk); 
 
    // Encontrar la palabra
más frecuente localmente 
    char localMostFrequentWord[WORD_SIZE]; 
    int localHighestCount; 
    findMostFrequentWordLocal(localMostFrequentWord, &localHighestCount); 
 
    // Reduce los resultados para encontrar la palabra más frecuente global 
    char globalMostFrequentWord[WORD_SIZE]; 
    int globalHighestCount; 
 
    MPI_Reduce(&localHighestCount, &globalHighestCount, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD); 
 
    // Solo el proceso 0 realiza el informe 
    if (rank == 0) { 
        printf("La palabra más repetida es '%s' con %d apariciones.\n", globalMostFrequentWord, globalHighestCount); 
    } 
 
    clearHashTable(); 
    MPI_Finalize(); 
    return 0; 
}