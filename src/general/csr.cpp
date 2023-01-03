#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>    // std::sort
#include <numeric> //std::iota
#include "matrices.h"
#include "utilities.h"

using namespace std;

void CSR::clean()
{
    /*----------------------------------------------------------------------
    | Free up memory allocated for CSR structs.
    |--------------------------------------------------------------------*/

    if (rows + cols <= 1) return;

    if (ma && !job) 
    {
        for(intT i = 0; i < rows; i++)
        {
            if (ma[i]) delete[] ma[i];
        }
    }

    if (ja) 
    {
        for(intT i = 0; i < rows; i++)
        {
            if (ja[i]) delete[] ja[i];
        }
    }
    delete[] nzcount;
}

void CSR::reorder(vector<intT> grouping)
{
    if (grouping.size() != rows)
    {
        cout << "INVALID SIZE" << endl;
    }

    vector<intT> v = get_permutation(grouping);

    permute(ja,v);
    if (job) permute(ma, v);
    permute(nzcount, v);
}


vector<intT> CSR::get_VBR_nzcount(const vector<intT> &grouping, intT block_col_size)
{
    vector<intT> row_partition = get_partition(grouping);
    vector<intT> row_permutation = get_permutation(grouping);
    return get_VBR_nzcount(row_partition, row_permutation, block_col_size);
}

vector<intT> CSR::get_VBR_nzcount(const vector<intT> &row_partition, const vector<intT> &row_permutation, intT block_col_size)
{
    
    intT block_cols = cols/block_col_size;
    intT block_rows = row_partition.size() - 1;


    //stats storage
    vector<intT> nz_block_count(row_partition.size() - 1,0);

    //copy data block_row by block_row
    for(intT ib = 0; ib < block_rows; ib++)
    {
        vector<bool> nonzero_flags(block_cols, false);
        intT row_block_size = row_partition[ib+1] - row_partition[ib];

        //flag nonzero blocks
        for (intT i_reordered = row_partition[ib]; i_reordered < row_partition[ib+1]; i_reordered++)
        {
            intT i = row_permutation[i_reordered];
            for (intT nz = 0; nz < nzcount[i]; nz++)
            {
                intT j = ja[i][nz];
                nonzero_flags[j/block_col_size] = true;
            }
        }

        //fill nzcount
        nz_block_count[ib] = std::count(nonzero_flags.begin(), nonzero_flags.end(), true);
    }
    return nz_block_count;
}


void CSR::read_from_edgelist(ifstream& infile, string delimiter, bool pattern_only)
{

    intT last_node = -1;
    intT current_node;
    string temp;
    vector<vector<intT>> pos_holder;
    vector<vector<DataT>> val_holder;
    intT max_column = 0;
    intT i = -1; 
    DataT val;
    intT total_nonzeros = 0;

    while (infile.peek() == '#' or infile.peek() == '%') infile.ignore(2048, '\n');
    while (getline(infile, temp)) {

        total_nonzeros++;
        int del_pos = temp.find(delimiter);
        int del_size = delimiter.length();

        string first_node_string = temp.substr(0, del_pos); //retrieve the part of the string before the delimiter
        current_node = stoi(first_node_string);
        temp.erase(0, del_pos + del_size);
        
        del_pos = temp.find(delimiter);
        string second_node_string = temp.substr(0, del_pos); //retrieve the part of the string after the delimiter
        intT child = stoi(second_node_string);
        max_column = std::max(max_column, child);
	    
        if (not pattern_only)
        {
            temp.erase(0, del_pos + del_size);
            del_pos = temp.find(delimiter);
            string val_string = temp.substr(0, del_pos); //retrieve the part of the string after the delimiter
            val = stof(val_string);
        }


        //fill with empty lines to reach current row
        if (current_node > i)
        {
            while (i < current_node)
            {
                vector<intT> new_pos_row;
		        pos_holder.push_back(new_pos_row);
                if (not pattern_only)
                {
                    vector<DataT> new_val_row;
                    val_holder.push_back(new_val_row);
                }
                i++;
            }
        }
        else if (current_node < i)
        {
            cout << "CANNOT READ MATRIX. INDICES MUST INCREASE" << endl;
            return;
        }
        pos_holder[i].push_back(child);
    	if (not pattern_only) val_holder[i].push_back(val);
    }


    job = pattern_only? 0 : 1;
    rows = pos_holder.size();
    cols = max_column + 1;
    nzcount = new intT[rows];
    ja = new intT*[rows];
    if (not pattern_only) 
    {
        ma = new DataT*[rows];
    }

    for (intT i = 0; i < pos_holder.size(); i++)
    {
        auto row_pos = pos_holder[i];
        nzcount[i] = row_pos.size();
        ja[i] = new intT[nzcount[i]]; 
        std::copy(row_pos.begin(), row_pos.end(), ja[i]);
	    pos_holder[i].clear();
	    
        if (not pattern_only)
        {
            auto row_val = val_holder[i];
            ma[i] = new DataT[nzcount[i]];
            std::copy(row_val.begin(), row_val.end(), ma[i]);
            val_holder[i].clear();
        }
    }

    pos_holder.clear();
    val_holder.clear();
}

void CSR::print(intT verbose)
{
    if (verbose > 0)
    {
        cout << "PRINTING A CSR MATRIX (arrays only)" << endl;
        cout << "ROWS:" << rows << " COLS:" << cols << " PATTERN:" << job << endl; 
        cout << "JA:" << endl;
        for (intT i = 0; i < rows; i++)
        {
            cout << "-- ";
            for (intT j = 0; j < nzcount[i]; j++)
            {
                cout << ja[i][j] << " ";
            }
            cout << endl;

        }


        cout << "MA:" << endl;
        for (intT i = 0; i < rows; i++)
        {
            cout << "-- ";
            for (intT j = 0; j < nzcount[i]; j++)
            {
                cout << ma[i][j] << " ";
            }
            cout << endl;
        }

        cout << "NZCOUNT:" << endl;
        for (intT i = 0; i < rows; i++)
        {
                cout << nzcount[i] << " ";
        }
        cout << endl;
    }

    cout << "PRINTING A CSR MATRIX" << endl;
    cout << "ROWS:" << rows << " COLS:" << cols << " PATTERN:" << job << endl; 
    //loop through rows
    for (intT i = 0; i < rows; i++)
    {
        intT j = 0;
        for (intT nzs = 0; nzs < nzcount[i]; nzs++) 
        {
            intT nz_column = ja[i][nzs]; //find column (row) index of next nonzero element
            
	        DataT elem;
	        if (job == 1) elem = ma[i][nzs]; //value of that element;
	        else elem = 1;
		
            while (j < nz_column)
            {
                j++;
                cout << 0 << " ";
            }
            cout << elem << " ";
            j++;
        }
        while (j < cols)
        {
            j++;
            cout << 0 << " ";
        }

	cout << endl;
    }

}
