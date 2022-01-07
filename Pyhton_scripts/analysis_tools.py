# -*- coding: utf-8 -*-
"""
Created on Wed Jan  5 17:35:20 2022

@author: Paolo
"""
import pandas as pd
import os
import matplotlib.pyplot as plt
import argparse
import numpy as np
import seaborn as sns
import itertools as itr
from scipy import interpolate

global columns, experimental_variables;
plt.rcParams['font.size'] = 11




experimental_variables = ["input_entries_density",
                  "input_blocks_density",
                  "rows",
                  "cols",
                  "B_cols",
                  "input_block_size",
                  "algo_block_size",
                  "epsilon",
                  "similarity_func",
                  "scramble",
                  "hierarchic_merge",
                  "merge_limit",
                  "reorder_algorithm"
            ]
    


columns = {'exp_name' : "experiment name",
               'input_type': "type of input", 
               'input_source': "input matrix", 
               'rows' : "K", 
               'cols': "M", 
               'B_cols': "N",
           'B_density': "density of B", 
           'total_nonzeros': "total nonzeros", 
           'input_blocks_density': "fraction of nonzero blocks",
           'input_entries_density': "density inside nonzero blocks", 
           'input_block_size': "block size", 
           'reorder_algorithm': "reorder algorithm",
           'algo_block_size': "block size", 
           'epsilon' : "tau", 
           'similarity_func': "similarity function", 
           'hierarchic_merge': "hierarchic merge",
           'merge_limit': "merge limit", 
           'scramble': "scramble", 
           'Warmup': "warmup experiments", 
           'Repetitions': "number of repetitions", 
           'Algorithm': "algo sel",
           'VBS_avg_nzblock_height': "avg. block height in the reordered matrix",
           'VBS_avg_nzblock_height_error': "error of the avg block height",
           'VBS_total_nonzeros': "total nonzero area in the reordered matrix",
           'VBS_total_nonzeros_error': "total nonzero area error", 
           'VBS_block_rows': "block rows",
           'VBS_block_rows_error': "block rows error", 
           'VBS_nz_blocks': "number of nonzero blocks", 
           'VBS_nz_blocks_error': "error on the number of nonzero blocks",
           'avg_skipped': "avg number of skipped comparisons", 
           'skipped_std': "error of skipped", 
           'avg_comparisons': "avg number of row comparisons", 
           'comparisons_std': "error in row comparisons",
           'VBSmm_algo_mean(ms)': "time to complete a block multiplication", 
           'VBSmm_algo_std': "error in block multiplication", 
           'cusparse_spmm_mean(ms)': "time to complete the cusparse multiplication",
           'cusparse_spmm_std': "error in cusparse multiplication"
           }

def import_results(input_csv):
    
    
    print("\n \n importing from", input_csv)
    results_df = pd.read_csv(input_csv);
    
    
    
    numerics = [
               'rows', 
               'cols', 
               'B_cols',
           'B_density',
           'total_nonzeros', 
           'input_blocks_density',
           'input_entries_density', 
           'input_block_size', 
           'algo_block_size', 
           'epsilon', 
           'Warmup', 
           'Repetitions',
           'merge_limit',
           'Algorithm',
           'VBS_avg_nzblock_height', 
           'VBS_avg_nzblock_height_error',
           'VBS_total_nonzeros', 
           'VBS_total_nonzeros_error', 
           'VBS_block_rows',
           'VBS_block_rows_error', 
           'avg_skipped', 
           'skipped_std', 
           'avg_comparisons', 
           'comparisons_std',
           'VBS_nz_blocks', 
           'VBS_nz_blocks_error',
           'VBSmm_algo_mean(ms)',
           'VBSmm_algo_std',
           'cusparse_spmm_mean(ms)',
           'cusparse_spmm_std']
    
    results_df[numerics] = results_df[numerics].apply(pd.to_numeric)
    
    results_df["input_density"] = results_df.apply(lambda x: x['total_nonzeros']/(x["rows"]*x["cols"]), axis=1)
    
    results_df["output_in_block_density"] = results_df.apply(lambda x: x['total_nonzeros']/x["VBS_total_nonzeros"], axis=1)
    
    results_df["relative_density"] = results_df.apply(lambda x: x['output_in_block_density']/x["input_entries_density"], axis=1)
    
    results_df["relative_block_size"] = results_df.apply(lambda x: x['VBS_avg_nzblock_height']/x["input_block_size"], axis=1)

    results_df["true_relative_density"] = results_df.apply(lambda x: x['output_in_block_density']/x["input_density"], axis=1)
        
    results_df["sp_vs_cu"] = results_df.apply(lambda x: x['cusparse_spmm_mean(ms)']/x["VBSmm_algo_mean(ms)"], axis=1)
    
    for var in experimental_variables:
        print(var, results_df[var].unique());
        
    return results_df;


def build_query(fixed):
    #build a query from the fixed dictionary
    q = ""
    for k, val in fixed.items():
        if val != "any":
            q += k + " == " + str(val) + " and ";
    q = q[0:-5];
    return q;   


def generate_exp_iterator(results_df, ignore = [], fixed = {}):
    value_lists = []
    for var in experimental_variables:
        if (var in fixed.keys()):
            value_lists.append([fixed[var],])
        elif (var in ignore):
            value_lists.append(["any",])
        else:
            value_lists.append(results_df[var].unique())
            
    return itr.product(*value_lists);


def make_title(variables_dict, to_print = ["rows","cols","algo_block_size"]):
    q = ""
    for k in to_print:
            q += columns[k] + "=" + str(variables_dict[k]) + ", ";
    return q[0:-2];

def add_to_query(var, val):
    return " and " + var + "==" + str(val);

def make_savename(name, variables_dict, ignore = ["input_block_size",]):
    q = name
    for k, val in variables_dict.items():
        if val != "any" and k not in ignore: 
            q += "_" + k[0:2] + str(val);
    return q + ".jpg";

def check_directory(path):
    # Check whether the specified path exists or not
    isExist = os.path.exists(path)

    if not isExist:
  
      os.makedirs(path)
      
def real_blocking_curve(results_df, variables_dict, variable = "input_entries_density",  save_folder = "../images/reorder_curves/", name =  "blocking_curve_input_entries"):
    
    plt.rcParams['font.size'] = 10


    marker = itr.cycle(('s','^', 'X',  'o', '*')) 
    colors = itr.cycle(('0','0.2','0.4','0.5','0.6'))
    
    fig, ax = plt.subplots(1,1, figsize = (6,6))
    plt.subplots_adjust(left = 0.1, top = 0.95, bottom = 0.15, right = 0.85)

    gen_q = build_query(variables_dict)
    for val in results_df[variable].unique():
        #q = gen_q + add_to_query(variable, val)
        q = gen_q
        this_df = results_df.query(q).sort_values("VBS_avg_nzblock_height", ascending = True);
        this_df = this_df[this_df[variable] == val]
        yp = this_df["true_relative_density"]
        xp = this_df["VBS_avg_nzblock_height"]
        ax.plot(xp,yp, marker = next(marker), color = next(colors), label = val, linewidth=1.5, markersize = 10, fillstyle = "none")
    
    ax.axvline(variables_dict["algo_block_size"], linestyle = "--", alpha = 0.5, label = "Column block size", color = "red")


    ax.set_xlim(0,2*variables_dict["algo_block_size"])
    ax.legend(title = "Original in-block density")
    plt.ylabel("Relative in-block density after reordering") 
    plt.xlabel("Average block height after reordering");    
    plt.title(make_title(variables_dict, to_print = ["rows","cols", "input_blocks_density"]))
    
    
    
    ax.set_aspect(1./ax.get_data_ratio())

    savename = make_savename(name,variables_dict)
    check_directory(save_folder);
    plt.savefig(save_folder + savename, format = 'jpg', dpi=300, bbox_inches = "tight")
    plt.show()
    plt.close()
      
      
def blocking_curve(results_df, variables_dict, variable = "input_entries_density",  save_folder = "../images/reorder_curves/", name =  "blocking_curve_input_entries"):
    
    plt.rcParams['font.size'] = 10


    marker = itr.cycle(('s','^', 'X',  'o', '*')) 
    colors = itr.cycle(('0','0.2','0.4','0.5','0.6'))
    
    fig, ax = plt.subplots(1,1, figsize = (6,6))
    plt.subplots_adjust(left = 0.1, top = 0.95, bottom = 0.15, right = 0.85)

    gen_q = build_query(variables_dict)
    for val in results_df[variable].unique():
        #q = gen_q + add_to_query(variable, val)
        q = gen_q
        this_df = results_df.query(q).sort_values("VBS_avg_nzblock_height", ascending = True);
        this_df = this_df[this_df[variable] == val]
        yp = this_df["relative_density"]
        xp = this_df["VBS_avg_nzblock_height"]
        ax.plot(xp,yp, marker = next(marker), color = next(colors), label = val, linewidth=1.5, markersize = 10, fillstyle = "none")
    
    ax.axhline(1, linestyle = "--", alpha = 0.5, color = "red")
    ax.axvline(variables_dict["algo_block_size"], linestyle = "--", alpha = 0.5, label = "Original blocking", color = "red")

    ax.set_xlim(0,2*variables_dict["algo_block_size"])
    ax.legend(title = "Original in-block density")
    plt.ylabel("Relative in-block density after reordering") 
    plt.xlabel("Average block height after reordering");    
    plt.title(make_title(variables_dict, to_print = ["rows","cols", "input_blocks_density"]))
    
    
    
    ax.set_aspect(1./ax.get_data_ratio())

    savename = make_savename(name,variables_dict)
    check_directory(save_folder);
    plt.savefig(save_folder + savename, format = 'jpg', dpi=300, bbox_inches = "tight")
    plt.show()
    plt.close()


def compare_blocking_curve(df_dict, variable_dict_dict, save_folder = "../images/reorder_curves/", name =  "blocking_curve_input_entries"):
    


    marker = itr.cycle(('s','^', 'X',  'o', '*')) 
    colors = itr.cycle(('0','0.2','0.4','0.5','0.6'))
    
    fig, ax = plt.subplots(1,1, figsize = (6,6))
    plt.subplots_adjust(left = 0.1, top = 0.95, bottom = 0.15, right = 0.85)
    
    
    queries = {}
    for df_name, df in df_dict.items():
        queries[df_name] = build_query(variable_dict_dict[df_name])        
    
    for df_name, df in df_dict.items():
        this_df = df.query(queries[df_name]).sort_values("relative_block_size", ascending = True);
        yp = this_df["relative_density"]
        xp = this_df["relative_block_size"]
        print(xp,yp)
        ax.scatter(xp,yp, marker = next(marker), label = df_name);
    
    
    ax.axhline(1, linestyle = "--", alpha = 0.5, color = "red")
    ax.axvline(1, linestyle = "--", alpha = 0.5, label = "Original blocking", color = "red")
    
    ax.set_xlim(0,2)
    ax.legend(title = "Original in-block density")
    plt.ylabel("Relative in-block density after reordering") 
    plt.xlabel("Relative block height after reordering");    
    #plt.title(make_title(list(variable_dict_dict.values())[0], to_print = ["rows","cols", "input_blocks_density"]))
    
    
    
    ax.set_aspect(1./ax.get_data_ratio())

    savename = make_savename(name,list(variable_dict_dict.values())[0])
    check_directory(save_folder);
    plt.savefig(save_folder + savename, format = 'jpg', dpi=300, bbox_inches = "tight")
    plt.show()
    plt.close()



def bar_plot(results_df, variables_dict, save_folder = "../images/performance_real/", name = "real_barplots"):
        
    graphs = results_df["input_source"].unique();
    measures = ['cusparse_spmm_mean(ms)','VBSmm_algo_mean(ms)']
    
    plt.style.use('grayscale')    
    fig, ax = plt.subplots(1, figsize = (len(graphs), 4), sharex=True);


    colormap = plt.cm.Greys
    ax.tick_params(axis='x', colors='grey')
    ax.grid(b=True, which='major', color='#999999', linestyle='-', alpha=0.3, linewidth=0.6)
    ax.grid(b=True, which='minor', color='#999999', linestyle='-', alpha=0.2, linewidth=0.4)
    ax.set_prop_cycle(color=[colormap(i) for i in np.linspace(0, 1,len(measures))])

    space = 0.2;
    width = (1. - space)/len(measures);

    q = build_query(variables_dict)
    results_df.query(q).plot.bar(x = "input_source",  y = measures, width = width, edgecolor = "black", ax = ax);
     
    plt.ylabel("multiplication time (ms)");
    plt.xlabel("input graph");


    plt.title(make_title(variables_dict,to_print = ["algo_block_size"]))
    
    savename = make_savename(name,variables_dict)
    check_directory(save_folder);
    plt.savefig(save_folder + savename, format = 'jpg', dpi=300, bbox_inches = "tight")
    plt.show()




def performance_heatmap(results_df,variables_dict, save_folder = "../images/performance_landscape/performance_heatmap/", name = "reorder_and_multiply_heatmap_"):
    
    
    if variables_dict["reorder_algorithm"] == "saad": 
            name = "reorder_heatmap_saad";
            
    heatmap_array = []
    for input_blocks_density in results_df["input_blocks_density"].unique():
        
        row = []
        for input_entries_density in results_df["input_entries_density"].unique():
            
            q = build_query(variables_dict)
            q += add_to_query("input_entries_density", input_entries_density);
            q += add_to_query("input_blocks_density", input_blocks_density);
            
            interp_df = results_df.query(q).sort_values("VBS_avg_nzblock_height");
            interp_df.drop_duplicates("VBS_avg_nzblock_height", inplace = True)
            interp_df.sort_values("VBS_avg_nzblock_height", ascending = True, inplace = True)
            xp = interp_df.query(q)["VBS_avg_nzblock_height"];
            yp = interp_df.query(q)["sp_vs_cu"]
            
            val = np.max(yp);
#            val = np.interp(64,xp,yp)
            row.append(val)
        heatmap_array.append(row)
    
    heat_df = pd.DataFrame(heatmap_array, columns=results_df["input_entries_density"].unique())
    heat_df.set_index(results_df["input_blocks_density"].unique(), inplace = True)
    heat_df.sort_index(level=0, ascending=False, inplace=True)
    
    cmap = sns.diverging_palette(0,255,sep=1, as_cmap=True)
    
    plt.gca()
    ax = sns.heatmap(heat_df, linewidths=.5, annot=True, cbar_kws={'label': 'speed-up vs cusparse'}, cmap = cmap, center = 1, vmin = 0, vmax = 5)
    bottom, top = ax.get_ylim()
    ax.set_ylim(bottom + 0.5, top - 0.5)
    plt.xlabel("Density inside nonzero blocks") 
    plt.ylabel("Fraction of nonzero blocks");
    
    plt.title(make_title(variables_dict, to_print = ["rows","cols","B_cols","input_block_size"]))
    savename = make_savename(name,variables_dict)
    
    check_directory(save_folder);
    plt.savefig(save_folder + savename, format = 'jpg', dpi=300, bbox_inches = "tight")
    plt.show()
    plt.close()


def compare_heatmap(original_df, compare_df, original_variables_dict, compare_variables_dict, save_folder = "../images/reorder_landscape/reorder_heatmap/", name = "reorder_heatmap_comparison"):
            
    
    
    
    
    def find_interpolated_value(df):
        interp_df = df.sort_values("VBS_avg_nzblock_height", ascending = True)
        
        xp = np.array(interp_df["VBS_avg_nzblock_height"])
        #adds the extreme value of one-block matrix
        first_val = 1;
        last_val = interp_df["rows"].values[0]
        xp = np.append(first_val, xp)
        xp = np.append(xp, last_val)

        
        yp = np.array(interp_df["relative_density"])
        #adds the extreme values of one-block matrix and all-blocks matrix
        first_val = 1
        last_val = interp_df["input_density"].values[0]
        yp = np.append(first_val, yp)
        yp = np.append(yp, last_val)

        
        b_size = compare_variables_dict["input_block_size"]
        val = np.interp(b_size,xp,yp)
            
        if val > 1: 
            val = 1
        return val        
            
    heatmap_array = []
    unique_cols = []
    unique_rows = []
    for input_blocks_density in compare_df["input_blocks_density"].unique():
        if input_blocks_density in original_df["input_blocks_density"].unique():
        
            unique_rows.append(input_blocks_density)
            row = []
            for input_entries_density in compare_df["input_entries_density"].unique():
                if input_entries_density in original_df["input_blocks_density"].unique():
    
                    if input_entries_density not in unique_cols:
                        unique_cols.append(input_entries_density)

                    q_c = build_query(compare_variables_dict)
                    q_c += add_to_query("input_entries_density", input_entries_density);
                    q_c += add_to_query("input_blocks_density", input_blocks_density);
                    
                    q_o = build_query(original_variables_dict)
                    q_o += add_to_query("input_entries_density", input_entries_density);
                    q_o += add_to_query("input_blocks_density", input_blocks_density);
                    
                    
                    #interp_df.drop_duplicates("VBS_avg_nzblock_height", inplace = True)
                    interp_compare_df = compare_df.query(q_c).sort_values("VBS_avg_nzblock_height", ascending = True)
                    interp_original_df = original_df.query(q_o).sort_values("VBS_avg_nzblock_height", ascending = True)
                    
                    val_compare = find_interpolated_value(interp_compare_df);
                    val_original = find_interpolated_value(interp_original_df);
                    
                    val = val_compare/val_original
                    row.append(val)
            
            heatmap_array.append(row)
    
    print(unique_cols, compare_df["input_entries_density"].unique())
    print(unique_rows, compare_df["input_blocks_density"].unique())
    heat_df = pd.DataFrame(heatmap_array, columns=unique_cols)
    heat_df.set_index(compare_df["input_blocks_density"].unique(), inplace = True)
    heat_df.sort_index(level=0, ascending=False, inplace=True)
    
    cmap = sns.diverging_palette(0,255,sep=1, as_cmap=True)
    
    plt.gca()
    ax = sns.heatmap(heat_df, linewidths=.5, annot=True, cbar_kws={'label': 'relative rho after reordering'}, cmap = cmap, center = 0, vmin = 0, vmax = 1)
    bottom, top = ax.get_ylim()
    ax.set_ylim(bottom + 0.5, top - 0.5)
    plt.xlabel("Density inside nonzero blocks") 
    plt.ylabel("Fraction of nonzero blocks");
    
    #plt.title(make_title(compare_variables_dict))
    savename = make_savename(name,compare_variables_dict)
    
    check_directory(save_folder);
    #plt.savefig(save_folder + savename, format = 'jpg', dpi=300, bbox_inches = "tight")
    plt.show()
    plt.close()





def reorder_heatmap(results_df, variables_dict, save_folder = "../images/reorder_landscape/reorder_heatmap/", name = "reorder_heatmap_"):
    
    
    if variables_dict["reorder_algorithm"] == "saad": 
            name = "reorder_heatmap_saad";
            
    heatmap_array = []
    for input_blocks_density in results_df["input_blocks_density"].unique():
        
        row = []
        for input_entries_density in results_df["input_entries_density"].unique():
            
            q = build_query(variables_dict)
            q += add_to_query("input_entries_density", input_entries_density);
            q += add_to_query("input_blocks_density", input_blocks_density);
            
            interp_df = results_df.query(q).sort_values("VBS_avg_nzblock_height");
            #interp_df.drop_duplicates("VBS_avg_nzblock_height", inplace = True)
            interp_df.sort_values("VBS_avg_nzblock_height", ascending = True, inplace = True)
            xp = interp_df.query(q)["VBS_avg_nzblock_height"];
            yp = interp_df.query(q)["relative_density"]
            
            b_size = variables_dict["algo_block_size"]
            if max(xp) < b_size:
                val = 0
            elif min(xp) > 1.2*b_size:
                print("overmax!",input_entries_density, input_blocks_density )
                plt.figure()
                plt.plot(xp,yp)
                plt.show()
                plt.close()
                val = 1000
            else:
                val = np.interp(b_size,xp,yp)
                
            if val > 1: 
                val = 1
            row.append(val)
        heatmap_array.append(row)
    
    heat_df = pd.DataFrame(heatmap_array, columns=results_df["input_entries_density"].unique())
    heat_df.set_index(results_df["input_blocks_density"].unique(), inplace = True)
    heat_df.sort_index(level=0, ascending=False, inplace=True)
    
    cmap = sns.diverging_palette(0,255,sep=1, as_cmap=True)
    
    plt.gca()
    ax = sns.heatmap(heat_df, linewidths=.5, annot=True, cbar_kws={'label': 'relative rho after reordering'}, cmap = cmap, center = 0, vmin = 0, vmax = 1)
    bottom, top = ax.get_ylim()
    ax.set_ylim(bottom + 0.5, top - 0.5)
    plt.xlabel("Density inside nonzero blocks") 
    plt.ylabel("Fraction of nonzero blocks");
    
    plt.title(make_title(variables_dict))
    savename = make_savename(name,variables_dict)
    
    check_directory(save_folder);
    plt.savefig(save_folder + savename, format = 'jpg', dpi=300, bbox_inches = "tight")
    plt.show()
    plt.close()

def delta_heatmap(results_df, variables_dict, save_folder = "../images/reorder_landscape/delta_heatmap/", name = "delta_heatmap_"):
    
    if variables_dict["reorder_algorithm"] == "saad": 
            name = name + "saad_";
            
    b_size = variables_dict["algo_block_size"]

    heatmap_array = []
    for input_blocks_density in results_df["input_blocks_density"].unique():
        
        row = []
        for input_entries_density in results_df["input_entries_density"].unique():
            
            q = build_query(variables_dict)
            q += add_to_query("input_entries_density", input_entries_density);
            q += add_to_query("input_blocks_density", input_blocks_density);
            
            interp_df = results_df.query(q).sort_values("relative_density");
            #interp_df.drop_duplicates("VBS_avg_nzblock_height", inplace = True)
            interp_df.sort_values("relative_density", ascending = True, inplace = True)
            yp = interp_df.query(q)["VBS_avg_nzblock_height"];
            xp = interp_df.query(q)["relative_density"]
            
            
            error = 0.1
            if max(xp) < 1 - error:
                val = 0
            elif min(xp) > 1 + error:
                val = 0
            else:
                val = int(np.interp(1,xp,yp))
                
            row.append(val)
        heatmap_array.append(row)
   
    pd.options.display.float_format = '{:.2f}'.format

    heat_df = pd.DataFrame(heatmap_array, columns=results_df["input_entries_density"].unique())
    heat_df.set_index(results_df["input_blocks_density"].unique(), inplace = True)
    heat_df.sort_index(level=0, ascending=False, inplace=True)
    
    cmap = sns.diverging_palette(0,255,sep=1, as_cmap=True)
    
    plt.gca()
    ax = sns.heatmap(heat_df, linewidths=.5, annot=True, cbar_kws={'label': 'Delta after reordering'}, cmap = cmap, center = 0, vmin = 0, vmax = b_size)
    bottom, top = ax.get_ylim()
    ax.set_ylim(bottom + 0.5, top - 0.5)
    plt.xlabel("Density inside nonzero blocks") 
    plt.ylabel("Fraction of nonzero blocks");
    
    plt.title(make_title(variables_dict))
    savename = make_savename(name,variables_dict)
    
    check_directory(save_folder);
    plt.savefig(save_folder + savename, format = 'jpg', dpi=300, bbox_inches = "tight")
    plt.show()
    plt.close()



def epsilon_heatmap(results_df, variables_dict, save_folder = "../images/performance_landscape/epsilon_heatmap", name = "best_epsilon_heatmap_"):
    
    
    if variables_dict["reorder_algorithm"] == "saad": 
            name = "reorder_heatmap_saad";
            
    heatmap_array = []
    for input_blocks_density in results_df["input_blocks_density"].unique():
        
        row = []
        for input_entries_density in results_df["input_entries_density"].unique():
            
            q = build_query(variables_dict)
            q += add_to_query("input_entries_density", input_entries_density);
            q += add_to_query("input_blocks_density", input_blocks_density);
            
            interp_df = results_df.query(q).sort_values("VBS_avg_nzblock_height");
            interp_df.drop_duplicates("VBS_avg_nzblock_height", inplace = True)
            interp_df.sort_values("VBS_avg_nzblock_height", ascending = True, inplace = True)
            epsilons = interp_df.query(q)["epsilon"];

            yp = interp_df.query(q)["sp_vs_cu"]
            
            idx = np.argmax(yp);
            val = epsilons[idx]
            row.append(val)
        heatmap_array.append(row)
    
    heat_df = pd.DataFrame(heatmap_array, columns=results_df["input_entries_density"].unique())
    heat_df.set_index(results_df["input_blocks_density"].unique(), inplace = True)
    heat_df.sort_index(level=0, ascending=False, inplace=True)
    
    cmap = sns.diverging_palette(0,255,sep=1, as_cmap=True)
    
    plt.gca()
    ax = sns.heatmap(heat_df, linewidths=.5, annot=True, cbar_kws={'label': 'best tau for multiplication'}, cmap = cmap, center = 1, vmin = 0, vmax = 1)
    bottom, top = ax.get_ylim()
    ax.set_ylim(bottom + 0.5, top - 0.5)
    plt.xlabel("Density inside nonzero blocks") 
    plt.ylabel("Fraction of nonzero blocks");
    
    plt.title(make_title(variables_dict, to_print = ["rows","cols","B_cols","input_block_size"]))
    savename = make_savename(name,variables_dict)
    plt.savefig(save_folder + savename, format = 'jpg', dpi=300, bbox_inches = "tight")
    plt.show()
    plt.close()