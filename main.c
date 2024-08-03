#include <stdio.h>
#include <omp.h>
#include <stdlib.h>
#include <math.h>

#define GRID_SIZE 20

#define PLANTS 75
#define HERBIVORES 30
#define CARNIVORES 15

#define HERBIVORE_OLD 20
#define CARNIVORE_OLD 50

#define MAX_TICKS 100


typedef enum {
    PLANT,
    HERBIVORE,
    CARNIVORE,
    EMPTY
} CellType;

typedef struct {
    int energy;
    int age;
    int starve;
    CellType type;
} Cell;



typedef struct {
    Cell grid[GRID_SIZE][GRID_SIZE];
} EcoSystem;

double death_probability(int age, double inflection_point, double steepness) {
    return 1.0 / (1.0 + exp(-(age - inflection_point) / steepness));
}


void update_plant(EcoSystem *ecoSystem, int reproduction_chance) {

    for(int i = 0; i < GRID_SIZE; i++) {
        for(int j = 0; j < GRID_SIZE; j++) {
            if(ecoSystem->grid[i][j].type == PLANT) {

//                // Death by age
//                double death_by_age = death_probability(ecoSystem->grid[i][j].age, 50, 10);
//                if (rand() % 100 < death_by_age * 100) {
//                    ecoSystem->grid[i][j].type = EMPTY; // The plant dies
//                    continue;
//                }

                // Death by overpopulation
                int neighbors = 0;
                if (i + 1 < GRID_SIZE && ecoSystem->grid[i + 1][j].type == PLANT) neighbors++;
                if (i - 1 >= 0 && ecoSystem->grid[i - 1][j].type == PLANT) neighbors++;
                if (j + 1 < GRID_SIZE && ecoSystem->grid[i][j + 1].type == PLANT) neighbors++;
                if (j - 1 >= 0 && ecoSystem->grid[i][j - 1].type == PLANT) neighbors++;
                if (neighbors > 3) {
                    printf("Plant died by overpopulation\n");
                    ecoSystem->grid[i][j].type = EMPTY; // The plant dies
                    ecoSystem->grid[i][j].energy = 0;
                    ecoSystem->grid[i][j].age = 0;
                    ecoSystem->grid[i][j].starve = 0;
                    continue;
                }

                // Reproduction
                int direction = rand() % 4;
                int x = i, y = j;

                switch(direction) {
                    case 0:  // right
                        if (i + 1 < GRID_SIZE) x = i + 1;
                        break;
                    case 1: // left
                        if (i - 1 >= 0) x = i - 1;
                        break;
                    case 2: // up
                        if (j + 1 < GRID_SIZE) y = j + 1;
                        break;
                    case 3: //
                        if (j - 1 >= 0) y = j - 1;
                        break;
                    default:
                        break;
                }

                // Cell is empy and the reproduction chance is greater that reproduction probability
                if (ecoSystem->grid[x][y].type == EMPTY && (rand() % 100) < reproduction_chance) {
                    ecoSystem->grid[x][y].type = PLANT;
                    ecoSystem->grid[x][y].energy = 1;
                    ecoSystem->grid[x][y].age = 0;
                    ecoSystem->grid[x][y].starve = 0;
                }

            }
        }
    }
}



void update_herbivore(EcoSystem *ecoSystem){
    for(int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {

            if (ecoSystem->grid[i][j].type == HERBIVORE ) {

                // Death by starvation
                if (ecoSystem->grid[i][j].starve >= 3) {
                    printf("Herbivore died by starvation\n");
                    ecoSystem->grid[i][j].type = EMPTY; // The herbivore dies
                    ecoSystem->grid[i][j].energy = 0;
                    ecoSystem->grid[i][j].age = 0;
                    ecoSystem->grid[i][j].starve = 0;
                    continue;
                }

                ecoSystem -> grid[i][j].age += 1;

                // Death by age
                double death_by_age = death_probability(ecoSystem->grid[i][j].age, HERBIVORE_OLD, 2);
                double r = (double) rand() / RAND_MAX;
                if (r < death_by_age) {
                    printf("Herbivore died by age\n");

                    ecoSystem->grid[i][j].type = EMPTY; // The herbivore dies
                    ecoSystem->grid[i][j].energy = 0;
                    ecoSystem->grid[i][j].age = 0;
                    ecoSystem->grid[i][j].starve = 0;

                    continue;
                }

                // Movement
                int direction = rand() % 4;
                int x = i, y = j;

                switch(direction) {
                    case 0:  // right
                        if (i + 1 < GRID_SIZE) x = i + 1;
                        break;
                    case 1: // left
                        if (i - 1 >= 0) x = i - 1;
                        break;
                    case 2: // up
                        if (j + 1 < GRID_SIZE) y = j + 1;
                        break;
                    case 3: //
                        if (j - 1 >= 0) y = j - 1;
                        break;
                    default:
                        break;
                }

                if ( ecoSystem -> grid[x][y].type == PLANT){
                    // Finds a plant and eats it
                    printf("Herbivore ate plant\n");
                    ecoSystem -> grid[x][y] = ecoSystem -> grid[i][j];
                    ecoSystem -> grid[x][y].energy += 1;
                    ecoSystem -> grid[x][y].starve = 0;

                    ecoSystem -> grid[i][j].type = EMPTY;
                    ecoSystem -> grid[i][j].starve = 0;
                    ecoSystem -> grid[i][j].energy = 0;

                } else {
                    ecoSystem -> grid[i][j].starve += 1;
                }

                if (ecoSystem -> grid[x][y].type == EMPTY){
                    // Reproduction elegible
                    if (ecoSystem -> grid[i][j].energy > 3) {
                        // Each herbivore needs to have eaten 5 plants to reproduce
                        ecoSystem -> grid[x][y].type = HERBIVORE;
                        ecoSystem -> grid[x][y].energy = 2;
                        ecoSystem -> grid[x][y].starve = 0;
                        ecoSystem -> grid[x][y].age = 0;

                        printf("Herbivore reproduced\n");
                    } else {
                        // Herbivore moves to the empty cell
                        ecoSystem -> grid[x][y] = ecoSystem -> grid[i][j];

                        ecoSystem -> grid[i][j].type = EMPTY;
                        ecoSystem -> grid[i][j].energy = 0;
                        ecoSystem -> grid[i][j].starve = 0;
                        ecoSystem -> grid[i][j].age = 0;
                    }

                } else if (ecoSystem -> grid[x][y].type == CARNIVORE){
                    // Move if there is a predator
                    switch (direction) {
                        case 0:  // Carnivore is to the right, move to the left
                            if (i - 1 >= 0) x = i - 1;
                            break;
                        case 1: // Carnivore is to the left, move to the right
                            if (i + 1 < GRID_SIZE) x = i + 1;
                            break;
                        case 2: // Carnivore is up, move down
                            if (j - 1 >= 0) y = j - 1;
                            break;
                        case 3: // Carnivore is down, move up
                            if (j + 1 < GRID_SIZE) y = j + 1;
                            break;
                        default:
                            break;
                    }

                    if (ecoSystem -> grid[x][y].type == EMPTY) {
                        ecoSystem->grid[x][y] = ecoSystem->grid[i][j];
                        ecoSystem->grid[i][j].type = EMPTY;
                        ecoSystem->grid[i][j].energy = 0;
                        ecoSystem->grid[i][j].starve = 0;
                        ecoSystem->grid[i][j].age = 0;
                    }
                }

            }

        }
    }
}

void update_carnivore(EcoSystem *ecoSystem){
    for(int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {

            //check if it has space and energy to reproduce
            if (ecoSystem->grid[i][j].type == CARNIVORE ) {

                // Death by starvation
                if (ecoSystem->grid[i][j].starve >= 5) {
                    printf("Carnivore died by starvation\n");

                    ecoSystem->grid[i][j].type = EMPTY; // The carnivore dies
                    ecoSystem->grid[i][j].energy = 0;
                    ecoSystem->grid[i][j].age = 0;
                    ecoSystem->grid[i][j].starve = 0;

                    continue;
                }

                ecoSystem -> grid[i][j].age += 1;

                // Death by age
                double death_by_age = death_probability(ecoSystem->grid[i][j].age, CARNIVORE_OLD, 2);
                if (rand() % 100 < death_by_age * 100) {

                    ecoSystem->grid[i][j].type = EMPTY; // The herbivore dies
                    ecoSystem->grid[i][j].energy = 0;
                    ecoSystem->grid[i][j].age = 0;
                    ecoSystem->grid[i][j].starve = 0;

                    continue;
                }

                int direction = rand() % 4;
                int x = i, y = j;

                switch(direction) {
                    case 0:  // right
                        if (i + 1 < GRID_SIZE) x = i + 1;
                        break;
                    case 1: // left
                        if (i - 1 >= 0) x = i - 1;
                        break;
                    case 2: // up
                        if (j + 1 < GRID_SIZE) y = j + 1;
                        break;
                    case 3: //
                        if (j - 1 >= 0) y = j - 1;
                        break;
                    default:
                        break;
                }

                if(ecoSystem->grid[x][y].type == HERBIVORE){
                   // Carnivore eats herbivore
                    ecoSystem -> grid[x][y] = ecoSystem -> grid[i][j];
                    ecoSystem -> grid[x][y].energy += 2;
                    ecoSystem -> grid[x][y].starve = 0;

                    ecoSystem -> grid[i][j].type = EMPTY;
                    ecoSystem -> grid[i][j].starve = 0;
                    ecoSystem -> grid[i][j].energy = 0;
                    ecoSystem -> grid[i][j].age = 0;

                    printf("Carnivore ate herbivore\n");
                } else if (ecoSystem -> grid[x][y].type == EMPTY){
                    // Reproduction
                    if (ecoSystem -> grid[i][j].energy > 4) { // Each carnivore needs to have eaten 7 herbivores to reproduce

                        ecoSystem->grid[x][y].type = CARNIVORE;
                        ecoSystem->grid[x][y].energy = 3;
                        ecoSystem->grid[x][y].starve = 0;
                        ecoSystem->grid[x][y].age = 0;

                        printf("Carnivore reproduced\n");
                    }
                } else {
                    // if there is no herbivore the carnivore will starve
                    ecoSystem -> grid[i][j].starve += 1;
                }

            }

        }
    }

}


void init_ecosystem(EcoSystem *ecoSystem) {
    // Se inicializa el ecosistema con celdas vacías
    for(int i = 0; i < GRID_SIZE; i++) {
        for(int j = 0; j < GRID_SIZE; j++) {
            ecoSystem->grid[i][j].type = EMPTY;
            ecoSystem->grid[i][j].energy = 0;
            ecoSystem->grid[i][j].age = 0;
            ecoSystem->grid[i][j].starve = 0;
        }
    }

    // Se generan las plantas, herbívoros y carnívoros de manera aleatoria
    for(int i = 0; i < PLANTS; i++) {
        int x = rand() % GRID_SIZE;
        int y = rand() % GRID_SIZE;

        ecoSystem->grid[x][y].type = PLANT;
        ecoSystem->grid[x][y].energy = 1; // Energía inicial de la planta
        ecoSystem->grid[x][y].age = 0;
        ecoSystem->grid[x][y].starve = 0;
    }
    for(int i = 0; i < HERBIVORES; i++) {
        int x = rand() % GRID_SIZE;
        int y = rand() % GRID_SIZE;

        while (ecoSystem->grid[x][y].type != EMPTY) {
            x = rand() % GRID_SIZE;
            y = rand() % GRID_SIZE;
        }

        ecoSystem->grid[x][y].type = HERBIVORE;
        ecoSystem->grid[x][y].energy = 2; // Energía inicial del herbívoro
        ecoSystem->grid[x][y].age = 0;
        ecoSystem->grid[x][y].starve = 0;
    }
    for(int i = 0; i < CARNIVORES; i++) {
        int x = rand() % GRID_SIZE;
        int y = rand() % GRID_SIZE;

        while (ecoSystem->grid[x][y].type != EMPTY) {
            x = rand() % GRID_SIZE;
            y = rand() % GRID_SIZE;
        }

        ecoSystem->grid[x][y].type = CARNIVORE;
        ecoSystem->grid[x][y].energy = 3; // Energía inicial del carnívoro
        ecoSystem->grid[x][y].age = 0;
        ecoSystem->grid[x][y].starve = 0;
    }
}


int main() {
    // Configura la localización para soportar UTF-8
    EcoSystem ecoSystem;
    init_ecosystem(&ecoSystem);

    for(int i = 0; i < MAX_TICKS; i++) {
        #pragma omp parallel sections
        {
            #pragma omp section
            {
                update_plant(&ecoSystem, 10);
            }

            #pragma omp section
            {
                update_herbivore(&ecoSystem);
            }

            #pragma omp section
            {
                update_carnivore(&ecoSystem);
            }
        }

        #pragma omp parallel
        {
            #pragma omp single
            {
                printf("Tick %d\n", i + 1);

                int count_plants = 0;
                int count_herbivores = 0;
                int count_carnivores = 0;
                for (int t = 0; t < GRID_SIZE; t++) {
                    for (int k = 0; k < GRID_SIZE; k++) {
                        switch (ecoSystem.grid[t][k].type) {
                            case EMPTY:
                                printf(" E ");
                                break;
                            case PLANT:
                                printf(" P ");
                                count_plants++;
                                break;
                            case HERBIVORE:
                                printf(" H ");
                                count_herbivores++;
                                break;
                            case CARNIVORE:
                                printf(" C ");
                                count_carnivores++;
                                break;
                        }
                    }
                    printf("\n");
                }
                printf("Plants: %d, Herbivores: %d, Carnivores: %d\n", count_plants, count_herbivores, count_carnivores);
                printf("\n");
            }
        }
    }

    return 0;
}

