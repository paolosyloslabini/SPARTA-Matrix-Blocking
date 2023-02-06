#include <vector>
#include "utilities.h"
#include <numeric> //std::iota
#include <fstream>
using namespace std;


vector<intT> get_permutation(const vector<intT> &grouping)
{
    auto comp = [&](int i, int j)
    {
        return grouping[i] < grouping[j];
    };

    vector<intT> v(grouping.size());
    iota(v.begin(), v.end(), 0);
    sort (v.begin(), v.end(), comp);

    return v;
}

vector<intT> get_partition(const vector<intT> &grouping)
{
    vector<intT> reordered_groups(grouping);
    vector<intT> partition;

    sort(reordered_groups.begin(), reordered_groups.end());
    
    intT current_group = -1;
    intT current_size = 0;
    intT i = 0;
    while (i < reordered_groups.size())
    {
        if (current_group != reordered_groups[i])
        {
            current_group = reordered_groups[i];
            partition.push_back(i);
        }
        i++;
    }
    partition.push_back(reordered_groups.size());
    return partition;
}


bool check_structured_sparsity(vector<intT>& structured_sparsity_pattern, vector<intT>& structured_sparsity_column_counter, intT* row, intT row_len, int m)
{
    //check that Row does not break the m:n structured sparsity when added to structured_sparsity_pattern;
    intT i = 0;
    intT j = 0;
    while (i < structured_sparsity_pattern.size() && j < row_len)
    {
        if (structured_sparsity_pattern[i] < row[j])
            i++;
        else if (structured_sparsity_pattern[i] > row[j])
            j++;
        else
        {
            if (structured_sparsity_column_counter[i] >= m) //column is already full
                return false;
            i++;
            j++;
        }
    }

    return true;
}

void update_structured_sparsity(vector<intT>& structured_sparsity_pattern, vector<intT>& structured_sparsity_column_counter, intT* row, intT row_len)
{
    //A,B sparse rows (compressed indices format)
    vector<intT> new_pattern;
    vector<intT> new_counter;

    intT i = 0;
    intT j = 0;

    while (i < structured_sparsity_pattern.size() && j < row_len)
    {
        if (structured_sparsity_pattern[i] < row[j])
        {
            new_pattern.push_back(structured_sparsity_pattern[i]);
            new_counter.push_back(structured_sparsity_column_counter[i]);
            i++;
        }
        else if (structured_sparsity_pattern[i] > row[j])
        {
            new_pattern.push_back(row[j]);
            new_counter.push_back(1);
            j++;
        }
        else
        {
            new_pattern.push_back(structured_sparsity_pattern[i]);
            new_counter.push_back(structured_sparsity_column_counter[i] + 1);
            i++;
            j++;
        }
    }
    
    while (i < structured_sparsity_pattern.size())
    {
        new_pattern.push_back(structured_sparsity_pattern[i]);
        new_counter.push_back(structured_sparsity_column_counter[i]);
        i++;
    }
    
    while (j < row_len)
    {
        new_pattern.push_back(row[j]);
        new_counter.push_back(1);
        j++;
    }

    structured_sparsity_pattern.clear();
    structured_sparsity_column_counter.clear();
    copy(new_pattern.begin(), new_pattern.end(), std::back_inserter(structured_sparsity_pattern));
    copy(new_counter.begin(), new_counter.end(), std::back_inserter(structured_sparsity_column_counter));
}


vector<intT> merge_rows(vector<intT> A, intT*B, intT size_B)
{
    //A,B sparse rows (compressed indices format)
    vector<intT> result;
    intT size_A = A.size();

    result.reserve(A.size() + size_B);
    std::merge(A.begin(), A.end(),
            B, B + size_B,
           std::back_inserter(result));

    return result;
}


void save_blocking_data(ostream &outfile, CLineReader &cLine, BlockingEngine &bEngine, CSR &cmat, bool save_blocking)
{
    string header;
    string values;
    auto add_to_output = [&](string name, string value)
    {
        header += name + ","; 
        values += value + ",";
    };
    add_to_output("matrix", cLine.filename_);
    add_to_output("rows", to_string(cmat.rows));
    add_to_output("cols", to_string(cmat.cols));
    add_to_output("nonzeros", to_string(cmat.nztot()));
    add_to_output("blocking_algo", to_string(cLine.blocking_algo_));
    add_to_output("tau", to_string(cLine.tau_));
    add_to_output("row_block_size", to_string(cLine.row_block_size_));
    add_to_output("col_block_size", to_string(cLine.col_block_size_));
    add_to_output("use_pattern", to_string(cLine.sim_use_pattern_));
    add_to_output("sim_use_groups", to_string(cLine.sim_use_groups_));
    add_to_output("sim_measure", to_string(cLine.sim_measure_));
    add_to_output("reorder", to_string(cLine.reorder_));
    add_to_output("exp_name", cLine.exp_name_);
    add_to_output("time_to_block", to_string(bEngine.timer));
    add_to_output("merge_counter", to_string(bEngine.merge_counter));
    add_to_output("comparison_counter", to_string(bEngine.comparison_counter));

    outfile << header << endl;
    outfile << values << endl;

    vector<intT> nzcount_VBR = cmat.get_VBR_nzcount(bEngine.grouping_result,cLine.col_block_size_);

    outfile << "NZCOUNT,";
    print_vec(nzcount_VBR, outfile, ",");

    if (save_blocking)
    {
        outfile << "GROUPING,";
        print_vec(bEngine.grouping_result, outfile, ",");
    }
}