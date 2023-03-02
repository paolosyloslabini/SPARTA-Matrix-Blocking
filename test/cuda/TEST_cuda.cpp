#include "cuda_utilities.h"
#include "matrices.h"
#include "blocking.h"
#include "utilities.h"
#include <string.h>
#include <cuda_runtime_api.h>
#include <iostream>
#include <random>
#include <algorithm> //fill, equal


using namespace std;



int main(int argc, char* argv[])
{
    //keep track of time
    float dt;
    vector<double> algo_times;
    float mean_time;
    float std_time;

    bool check_results = true;


    //import CSR
    CLineReader cli(argc, argv);
    if (cli.verbose_ > 0) cli.print();
    CSR cmat_A(cli); //sparse operand
    BlockingEngine bEngine(cli);
    //bEngine.blocking_algo = fixed_size; //set blocking algo to fixed_size
    //bEngine.row_block_size = bEngine.col_block_size;


    VBR vbmat; 
    if (cli.verbose_ > 0) cout << "Blocking the matrix" << endl;
    bEngine.GetGrouping(cmat_A);

    if (cli.verbose_ > 0) cout << "Creating VBR" << endl;
    vbmat.fill_from_CSR_inplace(cmat_A, bEngine.grouping_result, cli.col_block_size_);
    if (cli.verbose_ > 1) vbmat.print();

    intT A_rows = cmat_A.rows;
    intT A_cols = cmat_A.cols;
    intT B_rows = A_cols;
    intT B_cols = 32;
    intT C_cols = B_cols;
    intT C_rows = A_rows;

    //dense operand
    DataT* mat_B = new DataT[B_rows * B_cols];

    random_device rd;

    mt19937 e2(rd());
    uniform_real_distribution<> dist(0, 1);

    for (int n = 0; n < B_rows*B_cols; n++) 
    {
        mat_B[n] = 1.; //dist(e2);
    }
    if (cli.verbose_ > 1) 
    {
        cout << "Matrix B: " << B_rows << " X " << B_cols << endl;
        print_mat(mat_B, B_rows, B_cols, B_cols);
    }


    //******************************************
    //*****SERIAL CSR MULTIPLICATION PHASE******
    //******************************************

    DataT* mat_C_serial = new DataT[C_rows*C_cols]{0};

    if (cli.verbose_ > 0) cout << "CSR Serial Multiplication" << endl;
    cmat_A.multiply(mat_B, B_cols, mat_C_serial);
    if (cli.verbose_ > 1) 
    {
        cout << "Serial CSR: multiplication result" << endl;
        print_mat(mat_C_serial, C_rows, C_cols, C_cols);
    }


    //******************************************
    //****VBR by dense MULTIPLICATION PHASE***
    //******************************************

    DataT_C* mat_C_VBR = new DataT_C[C_rows * C_cols]{ 0 }; //will store result of dense-dense multiplication
    algo_times.clear();
    //run the VBR-dense multiplications

#ifdef PICO_DEBUG
    printf("===================================== Input matrices =====================================\n");
    printf("File %s, line %d: print cmat_A\n", __FILE__, __LINE__);
    cmat_A.print(2);
    pico_print_SpMMM("NULL", 0, 0, 0, NULL, NULL, NULL, "mat_B", B_rows, B_cols, mat_B, "mat_C_VBR", C_rows, C_cols, mat_C_VBR);
    printf("==========================================================================================\n\n\n");
#endif

    //run the VBR-dense multiplications

/*
    std::cout << "cublas start" << std::endl;

    for (int i = -cli.warmup_; i < cli.exp_repetitions_; i++)
    {
        fill(mat_C_VBR, mat_C_VBR + C_cols*C_rows, 0);
        cublas_blockmat_multiplyAB(vbmat, mat_B, B_cols, mat_C_VBR, dt, 8);
        //only saves non-warmup runs
        if (i >= 0) algo_times.push_back(dt);
        bool equality_check = equal(mat_C_VBR, mat_C_VBR + C_cols*C_rows, mat_C_serial);
        cout << "CORRECTNESS CHECK: " << equality_check << endl;
    }

    std::cout << "cublas end" << std::endl; */
    cout << "TIME: " << avg(algo_times) << endl;

    //mean_time = mean(algo_times);
    //std_time = std_dev(algo_times);
    //algo_times.clear();

//     delete[] mat_C_VBR;



    //******************************************
    //****Sparse by dense MULTIPLICATION PHASE***
    //******************************************


    //TODO



    //******************************************
    //****VBR by dense MULTIPLICATION PHASE***
    //******************************************

    std::cout << "==================================== cusparse start ======================================" << std::endl;

    DataT_C* mat_C_VBR2 = new DataT_C[C_rows * C_cols]{ 0 };

    DataT *csrVal;
    int *csrRowPtr, *csrColInd;
    prepare_cusparse_CSR( cmat_A, &csrRowPtr, &csrColInd, &csrVal);

#ifdef PICO_DEBUG
    printf("print CSR generated by \"prepare_cusparse_CSR\":\n");
    pico_print_SpMMM("cmat_A", A_rows, A_cols, cmat_A.nztot(), csrRowPtr, csrColInd, csrVal, "NULL", 0, 0, NULL, "NULL", 0, 0, NULL);
#endif

    cudaDeviceSynchronize();
    std::cout << "------------------------------- prepare_cusparse_CSR done --------------------------------" << std::endl;
    cusparse_gemm_custom2(C_rows, B_rows, (int) cmat_A.nztot(), csrRowPtr, csrColInd, csrVal, mat_B, B_cols, B_cols, mat_C_VBR2, C_cols, 1, 1, dt); // BUG qui

    std::cout << "===================================== cusparse end =======================================" << std::endl;

    int cmp = memcmp(mat_C_VBR, mat_C_VBR2, C_rows * C_cols * sizeof(DataT_C) );
    std::cout << "memcmp of mat_C_VBR and mat_C_VBR2 is " << cmp << std::endl;

    DataT_C* mat_C_VBR3 = new DataT_C[C_rows * C_cols]{ 0 };
    cmp = memcmp(mat_C_VBR3, mat_C_VBR2, C_rows * C_cols * sizeof(DataT_C) );
    std::cout << "memcmp of mat_C_VBR3 and mat_C_VBR2 is " << cmp << std::endl;


    std::cout << "================================ cusparse ellpack start ==================================" << std::endl;

    if (vbmat.block_rows == vbmat.block_cols) {
        BDG_CKP

#ifdef PICO_DEBUG
        vbmat.print();
#endif

        int ell_blocksize, ell_rows, ell_cols, num_blocks;
        intT* columns;
        DataT_C* values;
        prepare_cusparse_BLOCKEDELLPACK(&vbmat, &ell_blocksize, &ell_rows, &ell_cols, &num_blocks, &columns, &values);

        std::cout << "------------------------------- prepare_cusparse_BELL done -------------------------------" << std::endl;

#ifdef PICO_DEBUG
        pico_print_SpMMM("BEL_A", vbmat.rows, vbmat.cols, ell_blocksize, ell_rows, ell_cols, num_blocks, columns, values, "mat_B", B_rows, B_cols, mat_B, "C_VBR", C_rows, C_cols, mat_C_VBR3);
#endif

        cusparse_gemm_custom_ellpack(A_rows, A_cols, ell_blocksize, ell_cols, ell_rows, num_blocks, columns, values, mat_B, B_cols, B_cols, mat_C_VBR3, C_cols, 1, 1, dt);

#ifdef PICO_DEBUG
        vbmat.print();
        pico_print_SpMMM("BEL_A", vbmat.rows, vbmat.cols, ell_blocksize, ell_rows, ell_cols, num_blocks, columns, values, "mat_B", B_rows, B_cols, mat_B, "C_VBR", C_rows, C_cols, mat_C_VBR3);
#endif
    }

    std::cout << "================================= cusparse ellpack end ===================================" << std::endl;

    cmp = memcmp(mat_C_VBR3, mat_C_VBR2, C_rows * C_cols * sizeof(DataT_C) );
    std::cout << "memcmp of mat_C_VBR3 and mat_C_VBR2 is " << cmp << std::endl;

    std::cout << "END" << std::endl;

}
