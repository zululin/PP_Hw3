#include <mpi.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_SIZE 900
#define INF 500000

struct Element {
    int id;
    int weight;
    struct Element* next;
};

struct Element graph[MAX_SIZE];
int dist[MAX_SIZE][MAX_SIZE];
int final[MAX_SIZE][MAX_SIZE];
int outgoing[MAX_SIZE];
int msgIn[MAX_SIZE];
int msgOut[MAX_SIZE][MAX_SIZE];


int min(int a, int b) {
    if (a < b)
        return a;
    else
        return b;
}

void addEdge(struct Element* nodeA, struct Element* nodeB, int weight) {
    struct Element* newA = (struct Element*)malloc(sizeof(struct Element));
    struct Element* newB = (struct Element*)malloc(sizeof(struct Element));

    newA->id = nodeA->id;
    newA->weight = weight;
    newA->next = nodeB->next;

    newB->id = nodeB->id;
    newB->weight = weight;
    newB->next = nodeA->next;

    nodeA->next = newB;
    nodeB->next = newA;
}

void traverseGraph(int V) {
    int id;
    struct Element* tmp;

    for (id = 0; id < V; id++) {
        tmp = graph[id].next;

        while (tmp != NULL) {
            printf("%d(%d) ", tmp->id, tmp->weight);
            tmp = tmp -> next;
        }
        printf("\n");
    }
}

void freeGraph(int V) {
    int id;
    struct Element* tmp;
    struct Element* tmp2;

    for (id = 0; id < V; id++) {
        tmp = graph[id].next;

        while (tmp != NULL) {
            tmp2 = tmp;
            tmp = tmp -> next;
            free(tmp2);
        }
    }
}

int main(int argc, char* argv[]) {
    int i, j, k, t, V, E, weight, size, myRank, new;
    int start, step = 0, flag = 0, total = 1;
    int msg[MAX_SIZE][2];
    int result[2];
    char* input;
    char* output;
    char buffer[1024];
    struct Element* tmp;
    FILE* fp;
	MPI_Status status;
	MPI_Request request[MAX_SIZE];

    //===== init argument =====//
    input = argv[1];
    output = argv[2];

    //===== read input file =====//
    fp = fopen(input, "r");
    if (!fp) {
        printf("File error\n");
        exit(1);
    }

    //===== construct graph =====//
    k = 0;
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if (k == 0) {
            k++;
            V = atoi(strtok(buffer, " "));
            E = atoi(strtok(NULL, " "));

            //===== init graph =====//
            for (i = 0; i < V; i++) {
                graph[i].id = i;
                graph[i].weight = 0;
                graph[i].next = NULL;

                //===== init distance =====//
                for (j = 0; j < V; j++) {
                    if (i == j)
                        dist[i][j] = 0;
                    else
                        dist[i][j] = INF;
                }
            }
        }
        else {
            i = atoi(strtok(buffer, " "));
            j = atoi(strtok(NULL, " "));
            weight = atoi(strtok(NULL," "));

            outgoing[i]++;
            outgoing[j]++;
            addEdge(&graph[i], &graph[j], weight);
        }
    }
    fclose(fp);

    // Start MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    start = myRank;

    //===== phase 1 - init job to every neighbors =====//
    tmp = graph[start].next;
    while (tmp != NULL) {
        i = tmp->id;
        weight = tmp->weight;

        dist[start][i] = weight;
        msg[i][0] = start;
        msg[i][1] = weight;

        MPI_Isend(&msg[i], 2, MPI_INT, i, 0, MPI_COMM_WORLD, &request[i]);

        tmp = tmp->next;
    }

    while (total > 0) {
        flag = 0;

        //===== phase 2 - send any source to neighbors' shortest path via self =====//
        if (step > 0) {
            tmp = graph[myRank].next;
            while (tmp != NULL) {
                i = tmp->id;

                // important!! release previous round's isend request
                MPI_Wait(&request[i], &status);
                MPI_Isend(&msgOut[i], V, MPI_INT, i, 0, MPI_COMM_WORLD, &request[i]);

                tmp = tmp->next;
            }
        }

        // recv every neoghbors' msg
        for (j = 0; j < outgoing[myRank]; j++) {
            //===== phase 1 - recv init job =====//
            if (step == 0) {
                MPI_Recv(&result, 2, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);

                start = result[0];
                new = result[1];

                // update shortest path
                if (new < dist[start][myRank]) {
                    dist[start][myRank] = new;
                    flag++;
                }
            }
            else {
                //===== phase 2 - recv any source to self's shortest path through neighbors =====//
                MPI_Recv(&msgIn, V, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);

                for (k = 0; k < V; k++) {
                    start = k;
                    new = msgIn[k];

                    if (new < dist[start][myRank]) {
                        dist[start][myRank] = new;
                        flag++;
                    }
                }
            }
        }

        // compute every source to neighbors' shortest path via self
        for (start = 0; start < V; start++) {
            tmp = graph[myRank].next;
            while (tmp != NULL) {
                i = tmp->id;

                msgOut[i][start] = dist[start][myRank]+dist[myRank][i];
                tmp = tmp->next;
            }
        }
        step++;

        // check whether any update occur
        MPI_Allreduce(&flag, &total, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    }

    //===== collect answer =====//
    for (i = 0; i < V; i++) {
        for (j = 0; j < V; j++) {
            final[i][j] = 0;

            if (j != myRank)
                dist[i][j] = 0;
        }
    }
    MPI_Reduce(&dist, &final, MAX_SIZE*MAX_SIZE, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);


    if (myRank == 0) {
        //===== write output file =====//
        fp = fopen(output, "w");
        if (!fp) {
            printf("File error...\n");
            exit(1);
        }

        for (i = 0; i < V; i++) {
            for (j = 0; j < V; j++) {
                fprintf(fp, "%d ", final[i][j]);
            }
            fprintf(fp, "\n");
        }
        fclose(fp);

    }

    MPI_Finalize();
    freeGraph(V);
}
