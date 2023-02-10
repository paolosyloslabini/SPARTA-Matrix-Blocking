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

void BlockingEngine::CollectBlockingInfo(const CSR& cmat)
{
  //will fill the three variables:
  VBR_nzcount = 0; //total nonzeros in the VBR
  VBR_nzblocks_count = 0; //total nonzero blocks in the VBR
  VBR_average_height = 0; //average height of nonzero blocks

  vector<intT> row_partition = get_partition(grouping_result);
  vector<intT> row_permutation = get_permutation(grouping_result);

  intT block_cols = std::ceil(((float) cmat.cols)/col_block_size);
  intT block_rows = row_partition.size() - 1;

  intT total_blocks_height = 0;

  //copy data block_row by block_row
  for(intT ib = 0; ib < block_rows; ib++)
  {
      vector<bool> nonzero_flags(block_cols, false);
      intT row_block_size = row_partition[ib+1] - row_partition[ib];

      //flag nonzero blocks
      for (intT i_reordered = row_partition[ib]; i_reordered < row_partition[ib+1]; i_reordered++)
      {
          intT i = row_permutation[i_reordered];
          for (intT nz = 0; nz < cmat.nzcount[i]; nz++)
          {
              intT j = cmat.ja[i][nz];
              nonzero_flags[j/col_block_size] = true;
          }
      }

      for (intT jb = 0; jb < nonzero_flags.size(); jb++)
      {
          intT tmp_block_col_size = col_block_size;
          if (nonzero_flags[jb] == 1)
          {
              VBR_nzcount += tmp_block_col_size*row_block_size;
              VBR_nzblocks_count++;
              total_blocks_height += row_block_size;
          }
      }

      if (cmat.cols%col_block_size != 0 && nonzero_flags[nonzero_flags.size() - 1] == 1)
      {
          VBR_nzcount -= row_block_size*(col_block_size - cmat.cols%col_block_size); //accounts for the last (possibly shorter) block
      }
  }

  VBR_average_height = ((float) total_blocks_height)/VBR_nzblocks_count;
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
            //comparator = HammingDistanceGroup;
            break;
        case 1: 
            comparator = JaccardDistanceGroup;
            break;
        case 2: 
            comparator = HammingDistanceGroupOPENMP;
            break;        
        case 3: 
            comparator = JaccardDistanceGroupOPENMP;
            break;
    }
}


float HammingDistanceGroupOPENMP(vector<intT> row_A, intT group_size_A, intT* row_B, intT size_B, intT group_size_B, intT block_size)
{
  bool count_zeros = 0;
  intT size_A = row_A.size();
  if (size_A == 0 && size_B == 0) return 0;
  
  if (size_A == 0 or size_B == 0) return max(size_A*group_size_A,size_B*group_size_B); //approximation, to be checked.

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

  intT block_pos_A = -1;
  intT A_block_size = 0;
  //calculates block_size of A; 
  //this amounts to calculate for each element elem/block_size, then counting the unique results.
  for (auto it = row_A.begin(); it != row_A.end(); it++)
  {
    //is this a in a new column-block?
    if (*it/block_size != block_pos_A) 
    { 

        //if yes, update size and current column-block idx.
        block_pos_A = *it/block_size;
        A_block_size++;
    }
  }

  intT diffBA = 0;
  intT intersectBA = 0;
  intT pos_B = -1;

  //evaluates |B intersec A| and |B minus A| in the column-blocked domain
  //So, for each element check elem/block_size
  //  then, only one time for each unique result,
  //  check wheter the same block is occupied in A

  for (int j = 0; j < size_B; j++)
  {
    
    if(row_B[j]/block_size != pos_B)
    {
      //since the value is unique, this can now run independentently

      pos_B = row_B[j]/block_size;
      auto ptr_A = std::lower_bound(row_A.begin(), row_A.end(), pos_B * block_size); //check the smallest element larger than the start of the block
      if (*ptr_A/block_size > pos_B) //if it does not belong to same block, increase difference (can only be larger)
      {
        diffBA++;
      }
      else //otherwise, count it as intersection
      {
        intersectBA++;
      }
    }
  }

  //hamming distance will be diffBA + sizeA - intersectBA. we add some modifiers
  return diffBA*add_to_count_B + (A_block_size - intersectBA)*add_to_count_A;
}

float JaccardDistanceGroupOPENMP(vector<intT> row_A, intT group_size_A, intT* row_B, intT size_B, intT group_size_B, intT block_size)
{
  bool count_zeros = 0;
  intT size_A = row_A.size();
  if (size_A == 0 && size_B == 0) return 0;
  if (size_A == 0 || size_B == 0) return 1;

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

  intT block_pos_A = -1;
  intT A_block_size = 0;
  //calculates block_size of A; 
  //this amounts to calculate for each element elem/block_size, then counting the unique results.
  for (auto it = row_A.begin(); it != row_A.end(); it++)
  {
    //is this a in a new column-block?
    if (*it/block_size != block_pos_A) 
    { 
        //if yes, update size and current column-block idx.
        block_pos_A = *it/block_size;
        A_block_size++;
    }
  }

  intT diffBA = 0;
  intT intersectBA = 0;
  intT pos_B = -1;

  //evaluates |B intersec A| and |B minus A| in the column-blocked domain
  //So, for each element check elem/block_size
  //  then, only one time for each unique result,
  //  check wheter the same block is occupied in A

  for (int j = 0; j < size_B; j++)
  {
    
    if(row_B[j]/block_size != pos_B)
    {
      //since the value is unique, this can now run independentently

      pos_B = row_B[j]/block_size;
      auto ptr_A = std::lower_bound(row_A.begin(), row_A.end(), pos_B * block_size); //check the smallest element larger than the start of the block
      if (ptr_A != row_A.end() && *ptr_A/block_size == pos_B) //if it does not belong to same block, increase difference (can only be larger)
      {
        intersectBA++;
      }
      else //otherwise, count it as intersection
      {
        diffBA++;
      }
    }
  }
  //hamming distance will be diffBA + sizeA - intersectBA. we add some modifiers

  intT B_block_size = diffBA + intersectBA;
  intT count = diffBA*add_to_count_B + (A_block_size - intersectBA)*add_to_count_A;
  return (2.0*count)/(A_block_size*group_size_A + B_block_size*group_size_B + count);
}


float HammingDistanceGroup(vector<intT> row_A, intT group_size_A, intT* row_B, intT size_B, intT group_size_B, intT block_size)
{
  bool count_zeros = 0;
  intT size_A = row_A.size();
  if (size_A == 0 && size_B == 0) return 0;
  if (size_A == 0 || size_B == 0) return max(size_A*group_size_A,size_B*group_size_B);

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

  intT i = 0;
  intT j = 0;
  intT count = 0;
  intT pos_A;
  intT pos_B;
  
  while (i < size_A && j < size_B)
  {
    pos_A = row_A[i]/block_size;
    pos_B = row_B[j]/block_size;

    if (pos_A < pos_B)
    {
      count += add_to_count_A;
      while(i < size_A && row_A[i]/block_size == pos_A) i++;
    }
    else if (pos_A > pos_B)
    {
      count += add_to_count_B;
      while(j < size_B && row_B[j]/block_size == pos_B) j++;
    }
    else
    {
      while(i < size_A && row_A[i]/block_size == pos_A) i++;
      while(j < size_B && row_B[j]/block_size == pos_B) j++;
    }
  }

  while (i < size_A)
  { 
    pos_A = row_A[i]/block_size;
    count += add_to_count_A;
    while(i < size_A && row_A[i]/block_size == pos_A) i++;
  }

  while (j < size_B)
  { 
    pos_B = row_B[j]/block_size;;
    count += add_to_count_B;
    while(j < size_B && row_B[j]/block_size == pos_B) j++;
  }

  return count;
}

float JaccardDistanceGroup(vector<intT> row_A, intT group_size_A, intT* row_B, intT size_B, intT group_size_B, intT block_size)
{
  intT size_A = row_A.size();
  if (size_A == 0 && size_B == 0) return 0;
  if (size_A == 0 || size_B == 0) return 1;
  
  bool count_zeros = 0;

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

  intT i = 0;
  intT j = 0;
  intT count = 0;
  intT pos_A;
  intT pos_B;
  intT block_size_A = 0;
  intT block_size_B = 0;

  while (i < size_A && j < size_B)
  {
    pos_A = row_A[i]/block_size;
    pos_B = row_B[j]/block_size;

    if (pos_A < pos_B)
    {
      count += add_to_count_A;
      block_size_A++;
      while(i < size_A && row_A[i]/block_size == pos_A) i++;
    }
    else if (pos_A > pos_B)
    {
      count += add_to_count_B;
      block_size_B++;
      while(j < size_B && row_B[j]/block_size == pos_B) j++;
    }
    else
    {
      block_size_A++;
      block_size_B++;
      while(i < size_A && row_A[i]/block_size == pos_A) i++;
      while(j < size_B && row_B[j]/block_size == pos_B) j++;
    }
  }

  while (i < size_A)
  { 
    pos_A = row_A[i]/block_size;
    count += add_to_count_A;
    block_size_A++;
    while(i < size_A && row_A[i]/block_size == pos_A) i++;
  }

  while (j < size_B)
  { 
    pos_B = row_B[j]/block_size;;
    count += add_to_count_B;
    block_size_B++;
    while(j < size_B && row_B[j]/block_size == pos_B) j++;
  }

  return (2.0*count)/(block_size_A*group_size_A + block_size_B*group_size_B + count);
}