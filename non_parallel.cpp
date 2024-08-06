#include <stdio.h>
#include <omp.h>
#include <stdlib.h>
#include <math.h>

#define GRID_SIZE 30

#define PLANTS 100
#define HERBIVORES 50
#define CARNIVORES 25

#define HERBIVORE_OLD 50
#define CARNIVORE_OLD 50

#define MAX_TICKS 1000
#define STARVATION 10


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
    bool acted;
    CellType type;
} Cell;



typedef struct {
    Cell grid[GRID_SIZE][GRID_SIZE];
} EcoSystem;

double death_probability(int age, double inflection_point, double steepness) {
    return 1.0 / (1.0 + exp(-(age - inflection_point) / steepness));
}

void reset_acted(EcoSystem *ecoSystem) {
    for(int i = 0; i < GRID_SIZE; i++) {
        for(int j = 0; j < GRID_SIZE; j++) {
            ecoSystem->grid[i][j].acted = false;
        }
    }
}


void update_plant(EcoSystem *ecoSystem, int reproduction_chance) {
    for(int i = 0; i < GRID_SIZE; i++) {
        for(int j = 0; j < GRID_SIZE; j++) {
            if(ecoSystem->grid[i][j].type == PLANT) {

                if (ecoSystem->grid[i][j].acted) {
                    continue;
                }

                ecoSystem->grid[i][j].acted = true;


                // Death by overpopulation
                int neighbors = 0;
                if (i + 1 < GRID_SIZE && ecoSystem->grid[i + 1][j].type == PLANT) neighbors++;
                if (i - 1 >= 0 && ecoSystem->grid[i - 1][j].type == PLANT) neighbors++;
                if (j + 1 < GRID_SIZE && ecoSystem->grid[i][j + 1].type == PLANT) neighbors++;
                if (j - 1 >= 0 && ecoSystem->grid[i][j - 1].type == PLANT) neighbors++;

                if (neighbors > 3) {
                    ecoSystem->grid[i][j] = (Cell){0, 0, 0, false, EMPTY};  // The plant dies
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
                    ecoSystem->grid[x][y] = (Cell){1, 0, 0, true, PLANT};  // New plant is born
                }

            }
        }
    }
}




void update_herbivore(EcoSystem *ecoSystem){
    for(int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {

            if (ecoSystem->grid[i][j].type == HERBIVORE ) {

                if (ecoSystem->grid[i][j].acted) {
                    continue;
                }

                ecoSystem->grid[i][j].acted = true;

                // Death by starvation
                if (ecoSystem->grid[i][j].starve > STARVATION) {
                    printf("Herbivore died by starvation\n");
                    ecoSystem->grid[i][j] = (Cell){0, 0, 0, false, EMPTY};  // The herbivore dies
                    continue;
                }

                ecoSystem -> grid[i][j].age += 1;

                // Death by age
                double death_by_age = death_probability(ecoSystem->grid[i][j].age, HERBIVORE_OLD, 2);
                double r = (double) rand() / RAND_MAX;
                if (r < death_by_age) {
                    printf("Herbivore died by age\n");

                    ecoSystem->grid[i][j] = (Cell){0, 0, 0, false, EMPTY}; // The herbivore dies

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
                    int e = ecoSystem -> grid[x][y].energy;  // Energy of the plant
                    ecoSystem -> grid[x][y] = (Cell){ecoSystem -> grid[i][j].energy + e, ecoSystem -> grid[i][j].age, 0, true, HERBIVORE};
                    ecoSystem -> grid[i][j] = (Cell){0, 0, 0, false, EMPTY}; // The herbivore moves to the plant cell
                    printf("Herbivore ate plant, energy %d\n", ecoSystem -> grid[x][y].energy);

                } else if (ecoSystem -> grid[x][y].type == EMPTY){
                    ecoSystem -> grid[i][j].starve += 1;


                    if (ecoSystem -> grid[i][j].energy > 2) {
                        ecoSystem -> grid[x][y] = (Cell){1, 0, 0, true, HERBIVORE}; // New herbivore is born
                        ecoSystem -> grid[i][j].energy -= 1;

                        printf("Herbivore reproduced\n");
                    } else {
                        // Herbivore moves to the empty cell
                        ecoSystem -> grid[x][y] = ecoSystem -> grid[i][j];
                        ecoSystem -> grid[i][j] = (Cell){0, 0, 0, false, EMPTY}; // The herbivore moves to the empty cell
                    }

                } else if (ecoSystem -> grid[x][y].type == CARNIVORE){
//                    ecoSystem -> grid[i][j].starve += 1;

                    if (rand() % 100 < 45) {
                        continue;
                    }


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
                        printf("Herbivore moved to avoid carnivore\n");
                        ecoSystem->grid[x][y] = ecoSystem->grid[i][j];
                        ecoSystem->grid[i][j] = (Cell){0, 0, 0, true, EMPTY}; // The herbivore moves to the empty cell
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

                if (ecoSystem->grid[i][j].acted) {
                    continue;
                }

                ecoSystem->grid[i][j].acted = true;

                // Death by starvation
                if (ecoSystem->grid[i][j].starve > STARVATION + 3) {
                    printf("Carnivore died by starvation\n");

                    ecoSystem->grid[i][j] = (Cell){0, 0, 0, false, EMPTY};  // The herbivore dies

                    continue;
                }

                ecoSystem -> grid[i][j].age += 1;

                // Death by age
                double death_by_age = death_probability(ecoSystem->grid[i][j].age, CARNIVORE_OLD, 2);
                if (rand() % 100 < death_by_age * 100) {

                    ecoSystem->grid[i][j] = (Cell){0, 0, 0, false, EMPTY}; // The herbivore dies

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
                    int e = ecoSystem -> grid[x][y].energy;  // Energy of the herbivore
                    ecoSystem -> grid[x][y] = (Cell){ecoSystem -> grid[i][j].energy + e, ecoSystem -> grid[i][j].age, 0, true, CARNIVORE};
                    ecoSystem -> grid[i][j] = (Cell){0, 0, 0, false, EMPTY}; // The carnivore moves to the herbivore cell

                    printf("Carnivore ate herbivore\n");
                } else if (ecoSystem -> grid[x][y].type == EMPTY){
                    ecoSystem -> grid[i][j].starve += 1;

                    // Reproduction
                    if (ecoSystem -> grid[i][j].energy > 3) {
                        ecoSystem->grid[x][y] = (Cell){1, 0, 0, false, CARNIVORE}; // New carnivore is born
                        ecoSystem -> grid[i][j].energy -= 1;

                        printf("Carnivore reproduced,  energy %d\n", ecoSystem -> grid[i][j].energy);
                    } else {
                        // Carnivore moves to the empty cell
                        ecoSystem -> grid[x][y] = ecoSystem -> grid[i][j];
                        ecoSystem -> grid[i][j] = (Cell){0, 0, 0, false, EMPTY}; // The carnivore moves to the empty cell
                    }
                } else {
                    // if there is no herbivore the carnivore will starve
//                    ecoSystem -> grid[i][j].starve += 1;
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

        ecoSystem->grid[x][y] = (Cell){2, 0, 0, false, PLANT};
    }
    for(int i = 0; i < HERBIVORES; i++) {
        int x = rand() % GRID_SIZE;
        int y = rand() % GRID_SIZE;

        while (ecoSystem->grid[x][y].type != EMPTY) {
            x = rand() % GRID_SIZE;
            y = rand() % GRID_SIZE;
        }

        ecoSystem->grid[x][y] = (Cell){1, 0, 0, false, HERBIVORE};
    }
    for(int i = 0; i < CARNIVORES; i++) {
        int x = rand() % GRID_SIZE;
        int y = rand() % GRID_SIZE;

        while (ecoSystem->grid[x][y].type != EMPTY) {
            x = rand() % GRID_SIZE;
            y = rand() % GRID_SIZE;
        }

        ecoSystem->grid[x][y] = (Cell){1, 0, 0, false, CARNIVORE};
    }
}


int main() {
    // Configura la localización para soportar UTF-8
    EcoSystem ecoSystem;
    init_ecosystem(&ecoSystem);

    for (int i = 0; i < MAX_TICKS; i++){
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

        update_plant(&ecoSystem, 30);
        update_herbivore(&ecoSystem);
        update_carnivore(&ecoSystem);
        reset_acted(&ecoSystem);

        if (count_herbivores == 0 || count_carnivores == 0) {
            printf("Early stop\n");
            break;
        }
    }

//    for(int i = 0; i < MAX_TICKS; i++) {
//        printf("Tick %d\n", i + 1);
//
//        reset_acted(&ecoSystem);
//
//        int count_plants = 0;
//        int count_herbivores = 0;
//        int count_carnivores = 0;
//        for (int t = 0; t < GRID_SIZE; t++) {
//            for (int k = 0; k < GRID_SIZE; k++) {
//                switch (ecoSystem.grid[t][k].type) {
//                    case EMPTY:
//                        printf(" E ");
//                        break;
//                    case PLANT:
//                        printf(" P ");
//                        count_plants++;
//                        break;
//                    case HERBIVORE:
//                        printf(" H ");
//                        count_herbivores++;
//                        break;
//                    case CARNIVORE:
//                        printf(" C ");
//                        count_carnivores++;
//                        break;
//                }
//            }
//            printf("\n");
//        }
//        printf("Plants: %d, Herbivores: %d, Carnivores: %d\n", count_plants, count_herbivores, count_carnivores);
//        printf("\n");
//
//        #pragma omp parallel sections
//        {
//            #pragma omp section
//            {
//                update_plant(&ecoSystem, 20);
//            }
//
//            #pragma omp section
//            {
//                update_herbivore(&ecoSystem);
//            }
//
//            #pragma omp section
//            {
//                update_carnivore(&ecoSystem);
//            }
//        }
//    }

    return 0;
}
