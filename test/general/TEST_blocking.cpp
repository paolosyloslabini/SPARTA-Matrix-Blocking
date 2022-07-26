#include "matrices.h"
#include "blocking.h"
#include "similarity.h"
#include <fstream>
#include <iostream>

using namespace std;

int main()
{
    CSR cmat;
    ifstream fin;
    fin.open("data/TEST_matrix_weighted.txt");
    cmat.read_from_edgelist(fin, " ", false);
    cmat.print();

    distFunc distanceFunction = &JaccardDistance;
    float tau = 0.7;
    vector<intT> grouping = IterativeBlockingGeneral(cmat, tau, distanceFunction);

    cout << "MATRIX BLOCKED WITH JACCARD: BLOCKING =" << endl;
    for (intT i = 0; i < cmat.rows; i++)
    {
        cout << grouping[i] << endl;
    }

    cmat.print();


    cmat.reorder(grouping);
    cmat.print();
    cmat.clean();
}