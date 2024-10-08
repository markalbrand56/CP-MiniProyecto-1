#include <stdio.h>
#include <omp.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>


#define GRID_SIZE 80        // Size of the grid

#define PLANTS 2000         // Number of plants
#define HERBIVORES 1500     // Number of herbivores
#define CARNIVORES 500      // Number of carnivores

#define HERBIVORE_OLD 30    // Age at which herbivores die
#define CARNIVORE_OLD 50    // Age at which carnivores die

#define MAX_TICKS 10000     // Number of iterations
#define DEBUG_TICK 500      // Number of iterations before printing the state of the grid
#define STARVATION 10       // Number of iterations before herbivores and carnivores die of starvation

// Colors for the grid
#define COLOR_PLANT "\x1b[32m"
#define COLOR_HERBIVORE "\x1b[34m"
#define COLOR_CARNIVORE "\x1b[31m"
#define COLOR_EMPTY "\x1b[37m"
#define COLOR_RESET "\x1b[0m"

// Cell types
typedef enum {
    PLANT,
    HERBIVORE,
    CARNIVORE,
    EMPTY
} CellType;

// Cell structure
typedef struct {
    int energy;
    int age;
    int starve;
    bool acted;
    CellType type;
} Cell;


// Ecosystem structure
typedef struct {
    Cell grid[GRID_SIZE][GRID_SIZE];
    omp_lock_t locks[GRID_SIZE][GRID_SIZE];

} EcoSystem;

// Function to calculate the probability of death
double death_probability(int age, double inflection_point, double steepness) {
    return 1.0 / (1.0 + exp(-(age - inflection_point) / steepness));
}

// Function to reset the acted flag
void reset_acted(EcoSystem *ecoSystem){
    for(int i = 0; i < GRID_SIZE; i++) {
        for(int j = 0; j < GRID_SIZE; j++) {
            omp_set_lock(&ecoSystem->locks[i][j]);
            ecoSystem->grid[i][j].acted = false;
            omp_unset_lock(&ecoSystem->locks[i][j]);
        }
    }
}

// Function to update the plant
void update_plant(EcoSystem *ecoSystem, int reproduction_chance, int i, int j) {
    if (ecoSystem->grid[i][j].acted) {
        return;
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

        return;
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

    // Cell is empty and the reproduction chance is greater that reproduction probability
    if (ecoSystem->grid[x][y].type == EMPTY && (rand() % 100) < reproduction_chance) {
        omp_set_lock(&ecoSystem->locks[x][y]);
        ecoSystem->grid[x][y] = (Cell){1, 0, 0, true, PLANT};  // New plant is born
        omp_unset_lock(&ecoSystem->locks[x][y]);

    }
}

// Function to update the herbivore
void update_herbivore(EcoSystem *ecoSystem, int i, int j) {

        if (ecoSystem->grid[i][j].acted) {
            return;
        }

        omp_set_lock(&ecoSystem->locks[i][j]);
        ecoSystem->grid[i][j].acted = true;
        omp_unset_lock(&ecoSystem->locks[i][j]);

        // Death by starvation
        if (ecoSystem->grid[i][j].starve > STARVATION) {
            omp_set_lock(&ecoSystem->locks[i][j]);
//            printf("Herbivore died by starvation\n");
            ecoSystem->grid[i][j] = (Cell){0, 0, 0, false, EMPTY};  // The herbivore dies
            omp_unset_lock(&ecoSystem->locks[i][j]);

            return;
        }

        ecoSystem -> grid[i][j].age += 1;

        // Death by age
        double death_by_age = death_probability(ecoSystem->grid[i][j].age, HERBIVORE_OLD, 2);
        double r = (double) rand() / RAND_MAX;
        if (r < death_by_age) {
            omp_set_lock(&ecoSystem->locks[i][j]);
//            printf("Herbivore died by age\n");
            ecoSystem->grid[i][j] = (Cell){0, 0, 0, false, EMPTY}; // The herbivore dies
            omp_unset_lock(&ecoSystem->locks[i][j]);

            return;
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

            if (rand() % 100 < 45) {
                ecoSystem -> grid[i][j].starve += 1;
                return;
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
                omp_set_lock(&ecoSystem->locks[x][y]);
                omp_set_lock(&ecoSystem->locks[i][j]);
                ecoSystem->grid[x][y] = ecoSystem->grid[i][j];
                ecoSystem->grid[i][j] = (Cell){0, 0, 0, true, EMPTY}; // The herbivore moves to the empty cell
                omp_unset_lock(&ecoSystem->locks[x][y]);
                omp_unset_lock(&ecoSystem->locks[i][j]);

            }
        }
}

// Function to update the carnivore
void update_carnivore(EcoSystem *ecoSystem, int i, int j){
        if (ecoSystem->grid[i][j].acted) {
            return;
        }

        ecoSystem->grid[i][j].acted = true;

        // Death by starvation
        if (ecoSystem->grid[i][j].starve > STARVATION + 3) {
            omp_set_lock(&ecoSystem->locks[i][j]);
            ecoSystem->grid[i][j] = (Cell){0, 0, 0, false, EMPTY};  // The herbivore dies
            omp_unset_lock(&ecoSystem->locks[i][j]);

            return;
        }

        omp_set_lock(&ecoSystem->locks[i][j]);
        ecoSystem -> grid[i][j].age += 1;
        omp_unset_lock(&ecoSystem->locks[i][j]);

    // Death by age
        double death_by_age = death_probability(ecoSystem->grid[i][j].age, CARNIVORE_OLD, 2);
        if (rand() % 100 < death_by_age * 100) {

            omp_set_lock(&ecoSystem->locks[i][j]);
            ecoSystem->grid[i][j] = (Cell){0, 0, 0, false, EMPTY}; // The herbivore dies
            omp_unset_lock(&ecoSystem->locks[i][j]);

            return;
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

        //printf("Carnivore ate herbivore\n");
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

            } else {
                // Carnivore moves to the empty cell
                omp_set_lock(&ecoSystem->locks[x][y]);
                omp_set_lock(&ecoSystem->locks[i][j]);
                ecoSystem -> grid[x][y] = ecoSystem -> grid[i][j];
                ecoSystem -> grid[i][j] = (Cell){0, 0, 0, false, EMPTY}; // The carnivore moves to the empty cell
                omp_unset_lock(&ecoSystem->locks[x][y]);
                omp_unset_lock(&ecoSystem->locks[i][j]);
            }
        }
}

// Function to initialize the ecosystem
void init_ecosystem(EcoSystem *ecoSystem) {

    // initialize the grid with empty cells
    #pragma parallel for schedule(dynamic)
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

    #pragma barrier

    // Add plants, herbivores and carnivores to the grid randomly
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
    // open file 'iter.log' for writing
    FILE *file = fopen("iter.log", "w");

    if (file == NULL) {
        printf("Error opening file!\n");
        exit(1);
    }


    // Initialize the ecosystem
    EcoSystem ecoSystem;
    init_ecosystem(&ecoSystem);
    omp_set_dynamic(1);

    int i;
    for(i = 0; i < MAX_TICKS; i++) {
        reset_acted(&ecoSystem);

        int count_plants = 0;
        int count_herbivores = 0;
        int count_carnivores = 0;

        // Update the cells in parallel
        #pragma omp parallel for schedule(dynamic)
        for (int t = 0; t < GRID_SIZE; t++) {
            for (int k = 0; k < GRID_SIZE; k++) {
                switch (ecoSystem.grid[t][k].type) {
                    case EMPTY:
                        break;
                    case PLANT:
                        count_plants++;
                        update_plant(&ecoSystem, 50, t, k);
                        break;
                    case HERBIVORE:
                        count_herbivores++;
                        update_herbivore(&ecoSystem, t, k);
                        break;
                    case CARNIVORE:
                        count_carnivores++;
                        update_carnivore(&ecoSystem, t, k);
                        break;
                }
            }
        }

        // Write the ecosystem state to the file
        #pragma omp critical
        fprintf(file, "Tick %d: Plants: %d, Herbivores: %d, Carnivores: %d\n", i, count_plants, count_herbivores, count_carnivores);

        if (i % DEBUG_TICK == 0) {
            printf("Tick %d: Plants: %d, Herbivores: %d, Carnivores: %d\n", i, count_plants, count_herbivores, count_carnivores);

            // Print the state of the grid
            printf("State at Tick %d\n", i);
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
        }

        if (count_herbivores == 0 || count_carnivores == 0) {
            printf("Early stop\n");
            break;
        }
    }

    // Print the final state of the ecosystem
    if (i % 1000 != 0) {  // Ensure final state is printed if it was not at a multiple of 500
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
    }

    // Close the file
    fclose(file);

    return 0;
}
