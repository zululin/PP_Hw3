#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#define MAX_SIZE 2000
#define INF 500000

int V;
int dist[MAX_SIZE][MAX_SIZE];
int job[MAX_SIZE*MAX_SIZE/2];

int min(int a, int b) {
    return (a < b) ? a : b;
}

int max(int a, int b) {
    return (a > b) ? a : b;
}

void* computeDist (void* arg) {
    int i, j, k, start, size, id, t;
    int* data = (int*) arg;

    start = data[0];
    size = data[1];
    k = data[2];

    for (id = start; id < start+size; id++) {
        i = job[id] / V;
        j = job[id] % V;

        dist[i][j] = min(dist[i][j], dist[min(i, k)][max(i, k)]+dist[min(k, j)][max(k, j)]);
    }

    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    int i, j, k, t, rc, numThread, E, R, smallChunk, bigChunk, weight, jobNum;
    char* input;
    char* output;
    char buffer[1024];
    FILE* fp;


    //===== init argument =====//
    input = argv[1];
    output = argv[2];
    numThread = atoi(argv[3]);
    int arg[numThread][4];
    pthread_t threads[numThread];


    //===== read input file =====//
    fp = fopen(input, "r");
    if (!fp) {
        printf("File error\n");
        exit(1);
    }

    //===== construct graph =====//
    k = 0;
    jobNum = 0;
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if (k == 0) {
            k++;
            V = atoi(strtok(buffer, " "));
            E = atoi(strtok(NULL, " "));

            //===== init distance =====//
            for (i = 0; i < V; i++) {
                for (j = 0; j < V; j++) {
                    if (i == j)
                        dist[i][j] = 0;

                    // only consider half of graph because of undirected graph and construct job array
                    if (j > i) {
                        dist[i][j] = INF;
                        job[jobNum] = V * i + j;
                        jobNum++;
                    }
                }
            }
        }
        else {
            i = atoi(strtok(buffer, " "));
            j = atoi(strtok(NULL, " "));
            weight = atoi(strtok(NULL," "));

            dist[i][j] = weight;
            dist[j][i] = weight;
        }
    }
    fclose(fp);

    //===== Compute parameter =====//
    R = jobNum % numThread;
    smallChunk = (int)floor((1.0*jobNum) / numThread);
    bigChunk = (int)ceil((1.0*jobNum) / numThread);

    //===== Compute all pair shortest path =====//
    for (k = 0; k < V; k++) {
        for (t = 0; t < numThread; t++) {
            //===== Compute arguments to every thread =====//
            if (t <= R) {
                arg[t][0] = t * bigChunk;

                if (t < R)
                    arg[t][1] = bigChunk;
                else
                    arg[t][1] = smallChunk;
            }
            else {
                arg[t][0] = R * bigChunk + (t - R) * smallChunk;
                arg[t][1] = smallChunk;
            }
            arg[t][2] = k;

            rc = pthread_create(&threads[t], NULL, computeDist, (void*)&arg[t]);

            if (rc) {
                printf("ERROR; return code from pthread_create() is %d\n", rc);
                exit(-1);
            }
        }

        for (t = 0; t < numThread; t++) {
            pthread_join(threads[t], NULL);
        }
    }

    //===== write output file =====//
    fp = fopen(output, "w");
    if (!fp) {
        printf("File error...\n");
        exit(1);
    }

    for (i = 0; i < V; i++) {
        for (j = 0; j < V; j++) {
            fprintf(fp, "%d ", dist[min(i, j)][max(i, j)]);
        }
        fprintf(fp, "\n");
    }
    fclose(fp);

    pthread_exit(NULL);
}
