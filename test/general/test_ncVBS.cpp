#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h> //getopt, optarg
#include <ctime>
#include <cmath>
#include <algorithm> //min_element, max_element




#include <vector>
#include <string>


#include "sparse_utilities.h"
#include "nvbs_utilities.h"
#include "comp_mats.h"

int main(int argc, char* argv[]) {

    ncVBS vbmat;
    int mat_rows = 30;
    int mat_cols = 20;
    int block_size = 5;
    float mat_density = 0.5f;
    float row_density = 0.2f;

    random_ncVBS(vbmat, mat_rows, mat_cols, block_size, mat_density, row_density);
    matprint(vbmat);

    DataT* mat = new DataT[mat_rows * mat_cols]{ 0 };
    convert_to_mat(vbmat, mat, 0);
    matprint(mat, mat_rows, mat_cols, mat_cols, 0);



}