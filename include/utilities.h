#include <vector>
#include <algorithm> //std::copy
#include <iostream>

#include "matrices.h"

std::vector<intT> merge_rows(std::vector<intT> A, intT*B, intT sizeB);

std::vector<intT> get_partition(const std::vector<intT> &grouping);

std::vector<intT> get_permutation(const std::vector<intT> &grouping);


template <class MyType>
void print_vec(std::vector<MyType> vec)
{
    for (intT i = 0; i < vec.size(); i++)
    {
        std::cout << vec[i] << " ";
    }
    std::cout << std::endl;
}

//permutes an array of n elements (original) according to a permutation (perm);
template <class myType>
void permute(myType* arr, std::vector<intT> &perm) {
    //TODO efficient version with cycles
    myType* temp_arr = new myType[perm.size()];
    std::copy(arr, arr + perm.size(), temp_arr);   //auto copying

    for (intT i = 0 ; i < perm.size(); i++)
    {
        arr[i] = temp_arr[perm[i]];
    }

    delete[] temp_arr;
}