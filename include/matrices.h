#pragma once


#include <string>
#include <vector>
#include <fstream>

typedef long int intT;
typedef float DataT; //precision for input matrices entries
typedef float DataT_C; //precision for result matrix entries
//types for the two multiplied matrices and the result matrix. available choices are: 
//          INT_8, INT_32
//          FLOAT_32, FLOAT_32

struct CSR
{
    /*--------------------------------------------------------------
    | Compressed sparse row (CSR) matrix format,
    | generally not square
    |==========
    |       cmat  = a CSR struct
    |       rows  = # of columns of the matrix
    |       cols  = # of columns of the matrix
            job   = 0: sparsity pattern only (boolean matrix)
    |               1: data and pattern
    |--------------------------------------------------------------------   */
    intT rows;      /* number of rows                                        */
    intT cols;      /* number of cols                                        */
    intT* nzcount;  /* number of nonzero entry in each row (column)          */
    intT** ja;      /* pointer-to-pointer to store column (row) indices      */
    DataT** ma;   /* pointer-to-pointer to store nonzero entries           */

    int job; // 0 if the matrix is patter only; 1 otherwise

    void clean();
    void read_from_edgelist(std::ifstream& infile, std::string delimiter = " ", bool pattern_only = true);
    void reorder(std::vector<intT> grouping);
    std::vector<intT> get_VBR_nzcount(const std::vector<intT> &grouping, intT block_col_size = 1);
    std::vector<intT> get_VBR_nzcount(const std::vector<intT> &row_partition, const std::vector<intT> &row_permutation, intT block_col_size = 1);

    void print(intT verbose = 0);
    intT nztot()
    {
        intT nztot = 0;
        for (intT i = 0; i < rows; i++)
        {
            nztot += nzcount[i];
        }
        return nztot;
    }

};

struct VBR
{
    intT rows;
    intT cols;
    intT block_rows;  	            /* the block row dimension of the matrix    	        */
    intT block_cols;	            /* the block column dimension of the matrix   	        */
    intT* nzcount;	        /* i-th elements is the number of nonzero blocks in block-row i*/
    intT* jab;              /* block-columns indices of nonzero blocks      */
    intT* row_part;         /*row_part[i] is the index of the first row of the i-th block. Last element is number of rows for convenience  */
    DataT* mab;             /* array containing all entries, block by block	        */
    intT block_col_size;                  
    intT nztot;              /* total number of nonzero elements in mab*/

    void clean();
    intT get_row_partition(intT row);
    int partition_check(const std::vector<intT> &candidate_part);
    void fill_from_CSR(const CSR& cmat,const std::vector<intT> &row_partition, intT block_size);
    void fill_from_CSR_inplace(const CSR& cmat,const std::vector<intT> &grouping, intT block_size);

    DataT* get_block_start(intT row_block_idx);
    void print_row(intT row);
    void print(int verbose = 0);
    DataT* get_mab_position(intT block_row);
    intT* get_jab_position(intT block_row);
    intT nz_blocks();
    intT nz_entries();
    DataT* get_block(intT i, intT j);
};