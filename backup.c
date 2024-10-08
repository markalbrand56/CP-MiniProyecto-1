#include <stdio.h>
#include <omp.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>


#define GRID_SIZE 30

#define PLANTS 100
#define HERBIVORES 100
#define CARNIVORES 50

#define HERBIVORE_OLD 50
#define CARNIVORE_OLD 50

#define MAX_TICKS 1500
#define STARVATION 10

#define COLOR_PLANT "\x1b[32m"
#define COLOR_HERBIVORE "\x1b[34m"
#define COLOR_CARNIVORE "\x1b[31m"
#define COLOR_EMPTY "\x1b[37m"
#define COLOR_RESET "\x1b[0m"


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
    omp_lock_t locks[GRID_SIZE][GRID_SIZE];

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

                omp_set_lock(&ecoSystem->locks[i][j]);
                ecoSystem->grid[i][j].acted = true;
                omp_unset_lock(&ecoSystem->locks[i][j]);


                // Death by overpopulation
                int neighbors = 0;
                if (i + 1 < GRID_SIZE && ecoSystem->grid[i + 1][j].type == PLANT) neighbors++;
                if (i - 1 >= 0 && ecoSystem->grid[i - 1][j].type == PLANT) neighbors++;
                if (j + 1 < GRID_SIZE && ecoSystem->grid[i][j + 1].type == PLANT) neighbors++;
                if (j - 1 >= 0 && ecoSystem->grid[i][j - 1].type == PLANT) neighbors++;

                if (neighbors > 3) {
                    omp_set_lock(&ecoSystem->locks[i][j]);
                    ecoSystem->grid[i][j] = (Cell){0, 0, 0, false, EMPTY};  // The plant dies
                    omp_unset_lock(&ecoSystem->locks[i][j]);

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
                    omp_set_lock(&ecoSystem->locks[x][y]);
                    ecoSystem->grid[x][y] = (Cell){1, 0, 0, true, PLANT};  // New plant is born
                    omp_unset_lock(&ecoSystem->locks[x][y]);

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

                omp_set_lock(&ecoSystem->locks[i][j]);
                ecoSystem->grid[i][j].acted = true;
                omp_unset_lock(&ecoSystem->locks[i][j]);

                // Death by starvation
                if (ecoSystem->grid[i][j].starve > STARVATION) {
                    omp_set_lock(&ecoSystem->locks[i][j]);
                    printf("Herbivore died by starvation\n");
                    ecoSystem->grid[i][j] = (Cell){0, 0, 0, false, EMPTY};  // The herbivore dies
                    omp_unset_lock(&ecoSystem->locks[i][j]);

                    continue;
                }

                ecoSystem -> grid[i][j].age += 1;

                // Death by age
                double death_by_age = death_probability(ecoSystem->grid[i][j].age, HERBIVORE_OLD, 2);
                double r = (double) rand() / RAND_MAX;
                if (r < death_by_age) {
                    omp_set_lock(&ecoSystem->locks[i][j]);
                    printf("Herbivore died by age\n");
                    ecoSystem->grid[i][j] = (Cell){0, 0, 0, false, EMPTY}; // The herbivore dies
                    omp_unset_lock(&ecoSystem->locks[i][j]);
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
                    omp_set_lock(&ecoSystem->locks[x][y]);
                    omp_set_lock(&ecoSystem->locks[i][j]);

                    int e = ecoSystem -> grid[x][y].energy;  // Energy of the plant

                    ecoSystem -> grid[x][y] = (Cell){ecoSystem -> grid[i][j].energy + e, ecoSystem -> grid[i][j].age, 0, true, HERBIVORE};
                    ecoSystem -> grid[i][j] = (Cell){0, 0, 0, false, EMPTY}; // The herbivore moves to the plant cell

                    omp_unset_lock(&ecoSystem->locks[x][y]);
                    omp_unset_lock(&ecoSystem->locks[i][j]);

                } else if (ecoSystem -> grid[x][y].type == EMPTY){
                    omp_set_lock(&ecoSystem->locks[x][y]);
                    omp_set_lock(&ecoSystem->locks[i][j]);

                    ecoSystem -> grid[i][j].starve += 1;

                    if (ecoSystem -> grid[i][j].energy > 2) {  // Reproduction

                        ecoSystem -> grid[x][y] = (Cell){1, 0, 0, false, HERBIVORE}; // New herbivore is born
                        ecoSystem -> grid[i][j].energy -= 1;


                    } else {  // Move to the empty cell
                        ecoSystem -> grid[x][y] = ecoSystem -> grid[i][j];
                        ecoSystem -> grid[i][j] = (Cell){0, 0, 0, false, EMPTY}; // The herbivore moves to the empty cell

                    }

                    omp_unset_lock(&ecoSystem->locks[i][j]);
                    omp_unset_lock(&ecoSystem->locks[x][y]);

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
//                        printf("Herbivore moved to avoid carnivore\n");
                        omp_set_lock(&ecoSystem->locks[x][y]);
                        omp_set_lock(&ecoSystem->locks[i][j]);
                        ecoSystem->grid[x][y] = ecoSystem->grid[i][j];
                        ecoSystem->grid[i][j] = (Cell){0, 0, 0, true, EMPTY}; // The herbivore moves to the empty cell
                        omp_unset_lock(&ecoSystem->locks[x][y]);
                        omp_unset_lock(&ecoSystem->locks[i][j]);

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
                    omp_set_lock(&ecoSystem->locks[i][j]);
                    ecoSystem->grid[i][j] = (Cell){0, 0, 0, false, EMPTY};  // The herbivore dies
                    omp_unset_lock(&ecoSystem->locks[i][j]);

                    continue;
                }

                ecoSystem -> grid[i][j].age += 1;

                // Death by age
                double death_by_age = death_probability(ecoSystem->grid[i][j].age, CARNIVORE_OLD, 2);
                if (rand() % 100 < death_by_age * 100) {

                    omp_set_lock(&ecoSystem->locks[i][j]);
                    ecoSystem->grid[i][j] = (Cell){0, 0, 0, false, EMPTY}; // The herbivore dies
                    omp_unset_lock(&ecoSystem->locks[i][j]);

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
                    omp_set_lock(&ecoSystem->locks[x][y]);
                    omp_set_lock(&ecoSystem->locks[i][j]);

                    ecoSystem -> grid[x][y] = (Cell){ecoSystem -> grid[i][j].energy + e, ecoSystem -> grid[i][j].age, 0, true, CARNIVORE};
                    ecoSystem -> grid[i][j] = (Cell){0, 0, 0, false, EMPTY}; // The carnivore moves to the herbivore cell

                    omp_unset_lock(&ecoSystem->locks[x][y]);
                    omp_unset_lock(&ecoSystem->locks[i][j]);

                    printf("Carnivore ate herbivore\n");
                } else if (ecoSystem -> grid[x][y].type == EMPTY){
                    ecoSystem -> grid[i][j].starve += 1;

                    // Reproduction
                    if (ecoSystem -> grid[i][j].energy > 3) {
                        omp_set_lock(&ecoSystem->locks[x][y]);
                        omp_set_lock(&ecoSystem->locks[i][j]);

                        ecoSystem->grid[x][y] = (Cell){2, 0, 0, false, CARNIVORE}; // New carnivore is born
                        ecoSystem -> grid[i][j].energy -= 2;

                        omp_unset_lock(&ecoSystem->locks[x][y]);
                        omp_unset_lock(&ecoSystem->locks[i][j]);

//                        printf("Carnivore reproduced,  energy %d\n", ecoSystem -> grid[i][j].energy);
                    } else {
                        // Carnivore moves to the empty cell
                        omp_set_lock(&ecoSystem->locks[x][y]);
                        omp_set_lock(&ecoSystem->locks[i][j]);
                        ecoSystem -> grid[x][y] = ecoSystem -> grid[i][j];
                        ecoSystem -> grid[i][j] = (Cell){0, 0, 0, false, EMPTY}; // The carnivore moves to the empty cell
                        omp_unset_lock(&ecoSystem->locks[x][y]);
                        omp_unset_lock(&ecoSystem->locks[i][j]);
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
            ecoSystem->grid[i][j].acted = false;
            omp_init_lock(&ecoSystem->locks[i][j]);
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
    omp_set_num_threads(4);
    EcoSystem ecoSystem;
    init_ecosystem(&ecoSystem);

    int i;
    for(i = 0; i < MAX_TICKS; i++) {
#pragma barrier
        reset_acted(&ecoSystem);

        int count_plants = 0;
        int count_herbivores = 0;
        int count_carnivores = 0;



        for (int t = 0; t < GRID_SIZE; t++) {
            for (int k = 0; k < GRID_SIZE; k++) {
                switch (ecoSystem.grid[t][k].type) {
                    case EMPTY:
                        break;
                    case PLANT:
                        count_plants++;
                        break;
                    case HERBIVORE:
                        count_herbivores++;
                        break;
                    case CARNIVORE:
                        count_carnivores++;
                        break;
                }
            }
        }
        printf("%d Plants: %d, Herbivores: %d, Carnivores: %d\n", i, count_plants, count_herbivores, count_carnivores);
        printf("\n");

        if (count_herbivores == 0 || count_carnivores == 0) {
            printf("Early stop\n");
            break;
        }


#pragma omp parallel sections
        {
#pragma omp section
            update_plant(&ecoSystem, 50);

#pragma omp section
            update_herbivore(&ecoSystem);

#pragma omp section
            update_carnivore(&ecoSystem);
        }

    }

    printf("Final state\n");
    for (int t = 0; t < GRID_SIZE; t++) {
        for (int k = 0; k < GRID_SIZE; k++) {
            switch (ecoSystem.grid[t][k].type) {
                case EMPTY:
                    printf(" %sE%s ", COLOR_EMPTY, COLOR_RESET);
                    break;
                case PLANT:
                    printf(" %sP%s ", COLOR_PLANT, COLOR_RESET);
                    break;
                case HERBIVORE:
                    printf(" %sH%s ", COLOR_HERBIVORE, COLOR_RESET);
                    break;
                case CARNIVORE:
                    printf(" %sC%s ", COLOR_CARNIVORE, COLOR_RESET);
                    break;
            }
        }
        printf("\n");
    }
    printf("Tick %d\n", i);




    return 0;
}

