#include "matrices.h"
#include "blocking.h"
#include "utilities.h"

#include <fstream>
#include <iostream>

using namespace std;

int main(int argc, char* argv[])
{
    CLineReader cli(argc, argv);
    if (cli.verbose_ > 0) cli.print();
    CSR cmat(cli);
    if (cli.verbose_ > 0) cmat.print(cli.verbose_);
    BlockingEngine bEngine(cli);

    //evaluate the grouping
    if (cli.verbose_ > 0) cout << "evaluating reordering" <<endl;
    bEngine.GetGrouping(cmat);

    cout << "GROUPING: "; 
    print_vec(bEngine.grouping_result);
    
    bEngine.print();

    //CREATE VBR FROM GROUPING
    if (cli.verbose_ > 0) cout << "create VBR from grouping;" << endl;
    //create a VBR matrix from grouping (without reordering the original csr)
    VBR vbmat;
    vbmat.fill_from_CSR_inplace(cmat, bEngine.grouping_result, cli.col_block_size_);
    if (cli.verbose_ > 1) vbmat.print();

    //GET BLOCK PROPERTIES FROM GROUPING WITHOUT CREATING VBR EXPLICITLY
    if (cli.verbose_ > 0) cout << "nzcount_VBR: " << nzcount_VBR;

    //REORDER CSR, THEN CREATE VBR FROM GROUPING
    if (cli.verbose_ > 0) cout << "reordering the CSR" << endl;
    //create a VBR matrix from the row_partition (reordering the original csr)
    VBR vbmat2; 
    cmat.reorder(bEngine.grouping_result);
    if (cli.verbose_ > 1) cmat.print(cli.verbose_);

    if (cli.verbose_ > 0) cout << "Create VBR from reordered CSR" << endl;
    vbmat2.fill_from_CSR(cmat, get_partition(bEngine.grouping_result), cli.col_block_size_);
    if (cli.verbose_ > 1) vbmat2.print();
    
    if (cli.verbose_ > 1) 
    {
        cout << "GROUPING,";
        print_vec(bEngine.grouping_result);
    }

    cout << "TEST COMPLETED" << endl;
}