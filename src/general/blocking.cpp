#include "blocking.h"
#include "matrices.h"
#include "utilities.h"
#include "input.h"
#include <math.h> //ceil, floor
#include <vector>
#include <iostream>
#include <chrono>

using namespace std::chrono;

using namespace std;


vector<intT> IterativeBlockingPatternMN(const CSR& cmat, float tau, distFuncGroup distanceFunction,intT block_size, bool use_size, bool use_pattern, int structured_m, int structured_n, intT &comparison_counter, intT &merge_counter, float &timer)
{
    vector<intT> grouping(cmat.rows, -1); //flag each rows as ungrouped (-1)

    auto start = high_resolution_clock::now();

    //main loop. Takes an ungrouped row (i) as a seed for a new block.
    for (intT i = 0; i < cmat.rows; i++)
    {
        if (grouping[i] == -1)
        {
            vector<intT> pattern;
            intT current_group_size = 1;
            grouping[i] = i; // the group is numbered the same as the seed row.
            pattern.insert(pattern.end(), &cmat.ja[i][0], &cmat.ja[i][cmat.nzcount[i]]); //Initialize the pattern with the seed entries.
            
            int structured_sparsity_row_counter = 1;
            vector<intT> structured_sparsity_pattern(pattern);
            vector<intT> structured_sparsity_column_counter(pattern.size(),1);
            bool structured_sparsity_check = true;

            //inner loop, compare each subsequent row with the current pattern
            for (intT j = i + 1; j < cmat.rows; j++)
            {
                if (grouping[j] == -1)
                {
                    comparison_counter++;

                    float dist = distanceFunction(pattern, current_group_size, cmat.ja[j], cmat.nzcount[j], 1, block_size);
                    if (dist < tau)
                    {
                      if (structured_sparsity_row_counter%structured_n == 0) //restart m:n sparsity block
                      {
                        structured_sparsity_row_counter = 0;
                        structured_sparsity_pattern.clear();
                        structured_sparsity_column_counter.clear();
                        structured_sparsity_check = true;
                      }
                      else
                      {
                        structured_sparsity_check = check_structured_sparsity(structured_sparsity_pattern, structured_sparsity_column_counter, cmat.ja[j], cmat.nzcount[j], structured_m);
                      }

                      if (structured_sparsity_check)
                      {
                        merge_counter++;
                        grouping[j] = i;
                        if (use_pattern)
                            pattern = merge_rows(pattern, cmat.ja[j], cmat.nzcount[j]); //update the pattern through union with the new row
                        if (use_size)
                            current_group_size++;

                        //update structural sparsity vectors
                        update_structured_sparsity(structured_sparsity_pattern, structured_sparsity_column_counter, cmat.ja[j], cmat.nzcount[j]);
                        structured_sparsity_row_counter++;
                      }
                    }
                }
            }
        }
    }

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    timer = duration.count();

    return grouping;
}

vector<intT> IterativeBlockingPattern(const CSR& cmat, float tau, distFuncGroup distanceFunction,intT block_size, bool use_size, bool use_pattern, intT &comparison_counter, intT &merge_counter, float &timer)
{
    vector<intT> grouping(cmat.rows, -1); //flag each rows as ungrouped (-1)

    auto start = high_resolution_clock::now();

    //main loop. Takes an ungrouped row (i) as a seed for a new block.
    for (intT i = 0; i < cmat.rows; i++)
    {
        if (grouping[i] == -1)
        {
            vector<intT> pattern;
            intT current_group_size = 1;
            grouping[i] = i; // the group is numbered the same as the seed row.
            pattern.insert(pattern.end(), &cmat.ja[i][0], &cmat.ja[i][cmat.nzcount[i]]); //Initialize the pattern with the seed entries.
            //TODO check performance of std::copy instead


            //inner loop, compare each subsequent row with the current pattern
            for (intT j = i + 1; j < cmat.rows; j++)
            {
                if (grouping[j] == -1)
                {
                    comparison_counter++;

                    float dist = distanceFunction(pattern, current_group_size, cmat.ja[j], cmat.nzcount[j], 1, block_size);
                    if (dist < tau)
                    {
                        merge_counter++;
                        grouping[j] = i;
                        if (use_pattern)
                            pattern = merge_rows(pattern, cmat.ja[j], cmat.nzcount[j]); //update the pattern through union with the new row
                        if (use_size)
                            current_group_size++;
                    }
                }
            }
        }
    }

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    timer = duration.count();

    return grouping;
}

vector<intT> FixedBlocking(const CSR& cmat, intT row_block_size)
{
  vector<intT> grouping(cmat.rows);
  for(intT i = 0; i < cmat.rows; i++)
  {
    grouping[i] = i/row_block_size;
  }
  return grouping;
}


vector<intT> BlockingEngine::GetGrouping(const CSR& cmat)
{
    //run the blocking function and store statistics
    comparison_counter = 0;
    merge_counter = 0;
    timer = 0;
    switch(blocking_algo)
    {
      case iterative_pattern:
        grouping_result = IterativeBlockingPatternMN(cmat, tau, comparator, col_block_size, use_groups, use_pattern, structured_m, structured_n,comparison_counter, merge_counter, timer);
        break;
      case iterative:
        grouping_result = IterativeBlockingPattern(cmat, tau, comparator, col_block_size, use_groups, use_pattern, comparison_counter, merge_counter, timer);
        break;
      case fixed_size:
        grouping_result = FixedBlocking(cmat, row_block_size);
        break;
    }
    return grouping_result;
}

BlockingEngine::BlockingEngine(CLineReader &cline)
{       
  tau = cline.tau_;
  use_groups = cline.sim_use_groups_;
  use_pattern = cline.sim_use_pattern_;
  blocking_algo = cline.blocking_algo_;
  row_block_size = cline.row_block_size_;
  col_block_size = cline.col_block_size_;
  SetComparator(cline.sim_measure_);

}

void BlockingEngine::print()
{
  cout << "BLOCKING ENGINE INFO: " << endl;
  cout << "timer: " << timer << endl;
  cout << "comparison counter: " << comparison_counter << endl;
  cout << "merge counter: " << merge_counter << endl;
  cout << endl;
}

void BlockingEngine::SetComparator(int choice)
{
    switch(choice)
    {
        case 0:
            comparator = HammingDistanceGroup;
            break;
        case 1: 
            comparator = JaccardDistanceGroup;
            break;
    }
}


float HammingDistanceGroup(vector<intT> row_A, intT group_size_A, intT* row_B, intT size_B, intT group_size_B, intT block_size)
{
  bool count_zeros = 0;
  intT size_A = row_A.size();
  if (size_A == 0 and size_B == 0) return 0;

  intT blocksize_A = ceil(size_A/block_size);
  intT blocksize_B = ceil(size_B/block_size);
  
  if (size_A == 0 or size_B == 0) return max(blocksize_A*group_size_A,blocksize_B*group_size_B);

  intT add_to_count_A, add_to_count_B;
  if (count_zeros)
  {
    add_to_count_A = group_size_B;
    add_to_count_B = group_size_A;
  }
  else
  {
    add_to_count_A = group_size_A;
    add_to_count_B = group_size_B;
  }

  auto blockpos_A = [&](intT i){return floor(row_A[i]/block_size);}; 
  auto blockpos_B = [&](intT j){return floor(row_B[j]/block_size);};


  intT i = 0;
  intT j = 0;
  intT count = 0;
  intT pos_A;
  intT pos_B;
  
  while (i < size_A && j < size_B)
  {
    pos_A = blockpos_A(i);
    pos_B = blockpos_B(j);

    if (pos_A < pos_B)
    {
      count += add_to_count_A;
      while(i < size_A && blockpos_A(i) == pos_A) i++;
    }
    else if (pos_A > pos_B)
    {
      count += add_to_count_B;
      while(j < size_B && blockpos_B(j) == pos_B) j++;
    }
    else
    {
      while(i < size_A && blockpos_A(i) == pos_A) i++;
      while(j < size_B && blockpos_B(j) == pos_B) j++;
    }
  }

  while (i < size_A)
  { 
    pos_A = blockpos_A(i);
    count += add_to_count_A;
    while(i < size_A && blockpos_A(i) == pos_A) i++;
  }

  while (j < size_B)
  { 
    pos_B = blockpos_B(j);
    count += add_to_count_B;
    while(j < size_B && blockpos_B(j) == pos_B) j++;
  }

  return count;
}

float JaccardDistanceGroup(vector<intT> row_A, intT group_size_A, intT* row_B, intT size_B, intT group_size_B, intT block_size)
{
  intT size_A = row_A.size();
  if (size_A == 0 && size_B == 0) return 0;
  if (size_A == 0 || size_B == 0) return 1;

  float h = HammingDistanceGroup(row_A, group_size_A, row_B, size_B, group_size_B, block_size);
  return 2*h/(size_A*group_size_A + size_B*group_size_B + h);
}