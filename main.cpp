#include "stdafx.h"
#include "myutils.h"
#include "tree_str_pair.h"

void load_lex_trans_table(unordered_map<string,double> &lex_trans_table,string lex_trans_file)
{
	ifstream fin(lex_trans_file.c_str());
    string line;
    while(getline(fin,line))
    {
        vector<string> vs = Split(line);
        lex_trans_table[vs[0]+" "+vs[1]] = stod(vs[2]);
    }
};

int main(int argc, char* argv[])
{
	ifstream ft(argv[1]);
	ifstream fs(argv[2]);
	ifstream fa(argv[3]);
	vector<vector<string> > line_tree_vecs;
	vector<vector<string> > line_str_vecs;
	vector<vector<string> > line_align_vecs;
	int block_size = 1000;
	load_data_into_blocks(line_tree_vecs,ft,block_size);
	load_data_into_blocks(line_str_vecs,fs,block_size);
	load_data_into_blocks(line_align_vecs,fa,block_size);
	unordered_map<string,double> lex_s2t;
	unordered_map<string,double> lex_t2s;
    load_lex_trans_table(lex_s2t,argv[4]);
    load_lex_trans_table(lex_t2s,argv[5]);
    RuleCounter rule_counter;

	int processed_num = 0;
	int block_num = line_tree_vecs.size();
	for (size_t i=0;i<block_num;i++)
	{
		vector<vector<Rule> > rule_collectors;
		block_size = line_tree_vecs.at(i).size();
		rule_collectors.resize(block_size);
#pragma omp parallel for num_threads(16)
		for (size_t j=0;j<block_size;j++)
		{
			TreeStrPair tspair = TreeStrPair(line_tree_vecs.at(i).at(j),line_str_vecs.at(i).at(j),line_align_vecs.at(i).at(j),&lex_s2t,&lex_t2s);
            if (tspair.valid_flag == false)
                continue;
			tspair.extract_rules(tspair.root_idx);
			tspair.dump_rules(tspair.root_idx,rule_collectors.at(j));
		}
		processed_num += block_size;
		cerr<<processed_num<<endl;
		for (auto &rules : rule_collectors)
		{
			for (auto &rule : rules)
			{
				rule_counter.update(rule);
			}
		}
	}
    rule_counter.dump_rules();
}
