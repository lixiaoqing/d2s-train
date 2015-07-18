#include "rule_counter.h"

void RuleCounter::update(Rule &rule)
{
	string rule_str = rule.rule_str;
    double frac_count = rule.frac_count;
	double lex_weight_t2s = rule.lex_weight_backward*frac_count;
	double lex_weight_s2t = rule.lex_weight_forward*frac_count;
	vector<string> vs = Split(rule_str," ||| ");
	string rule_src = vs[0];
	string rule_tgt = vs[1];
	string tgt_nt_idx_to_src_nt_idx = vs[2];
    auto it1 = rule2count_and_accumulate_lex_weight.find(rule_str);
    if (it1 != rule2count_and_accumulate_lex_weight.end())
    {
        it1->second.count += frac_count;
        it1->second.acc_lex_weight_t2s += lex_weight_t2s;
        it1->second.acc_lex_weight_s2t += lex_weight_s2t;
    }
    else
    {
        rule2count_and_accumulate_lex_weight[rule_str] = {frac_count,lex_weight_t2s,lex_weight_s2t};
    }

    auto it2 = rule_src2count.find(rule_src);
    if (it2 != rule_src2count.end())
    {
        it2->second += frac_count;
    }
    else
    {
        rule_src2count[rule_src] = frac_count;
    }

    auto it3 = rule_tgt2count.find(rule_tgt);
    if (it3 != rule_tgt2count.end())
    {
        it3->second += frac_count;
    }
    else
    {
        rule_tgt2count[rule_tgt] = frac_count;
    }
}

void RuleCounter::dump_rules()
{
    unordered_map<string,int> type2id = {{"lll",0},{"gll",1},{"lgl",2},{"llg",3},{"lgg",4},{"glg",5},{"ggl",6},{"ggg",7}};
	unordered_map <string,int> src_vocab;
	unordered_map <string,int> tgt_vocab;
	vector<string> src_vocab_vec;
	vector<string> tgt_vocab_vec;
	int src_word_id = 0;
	int tgt_word_id = 0;
	ofstream fout("prob.bin",ios::binary);
	if (!fout.is_open())
	{
		cerr<<"fails to open rule-table to write\n";
		return;
	}
    int num = 0;
    for (auto &kvp : rule2count_and_accumulate_lex_weight)
    {
        num++;
        if (num%1000000 == 0)
            cout<<num/1000000<<"M\n";
        string rule = kvp.first;
        vector<string> vs = Split(rule," ||| ");
        string rule_src = vs[0];
        string rule_tgt = vs[1];
		vector<string> nt_align = Split(vs[2]);
		vector<int> tgt_nt_idx_to_src_nt_idx;
		for (auto &e : nt_align)
		{
			tgt_nt_idx_to_src_nt_idx.push_back(stoi(e));
		}
		vector <string> src_word_vec = Split(rule_src);
		vector <int> src_id_vec;
		for (auto &src_word : src_word_vec)
		{
			auto it = src_vocab.find(src_word);
			if (it != src_vocab.end())
			{
				src_id_vec.push_back(it->second);
			}
			else
			{
				src_id_vec.push_back(src_word_id);
				src_vocab.insert(make_pair(src_word,src_word_id));
				src_vocab_vec.push_back(src_word);
				src_word_id++;
			}
		}
		vector <string> tgt_word_vec = Split(rule_tgt);
		vector <int> tgt_id_vec;
		for (auto &tgt_word : tgt_word_vec)
		{
            if (tgt_word.find("[x]") != string::npos)
            {
                tgt_word = "[x]";
            }
			auto it = tgt_vocab.find(tgt_word);
			if (it != tgt_vocab.end())
			{
				tgt_id_vec.push_back(it->second);
			}
			else
			{
				tgt_id_vec.push_back(tgt_word_id);
				tgt_vocab.insert(make_pair(tgt_word,tgt_word_id));
				tgt_vocab_vec.push_back(tgt_word);
				tgt_word_id++;
			}
		}

        double rule_count = kvp.second.count;
        double lex_weight_t2s = d2log(kvp.second.acc_lex_weight_t2s/rule_count);
        double lex_weight_s2t = d2log(kvp.second.acc_lex_weight_s2t/rule_count);
        double trans_prob_t2s = d2log(rule_count/rule_tgt2count[rule_tgt]);
        double trans_prob_s2t = d2log(rule_count/rule_src2count[rule_src]);
        vector<double> prob_vec = {trans_prob_t2s,trans_prob_s2t,lex_weight_t2s,lex_weight_s2t};
		int src_rule_len = src_id_vec.size();
		int tgt_rule_len = tgt_id_vec.size();
		fout.write((char*)&src_rule_len,sizeof(int));
		fout.write((char*)&src_id_vec[0],sizeof(int)*src_rule_len);
		fout.write((char*)&tgt_rule_len,sizeof(int));
		fout.write((char*)&tgt_id_vec[0],sizeof(int)*tgt_rule_len);
		fout.write((char*)&tgt_nt_idx_to_src_nt_idx[0],sizeof(int)*tgt_nt_idx_to_src_nt_idx.size());
		fout.write((char*)&prob_vec[0],sizeof(double)*prob_vec.size());
        cout<<rule<<" ||| "<<trans_prob_t2s<<" "<<trans_prob_s2t<<" "<<lex_weight_t2s<<" "<<lex_weight_s2t<<endl;
    }
    cout<<num<<endl;
    fout.close();

	ofstream f_src_vocab("vocab.ch");
	if (!f_src_vocab.is_open())
	{
		cout<<"fail open ch vocab file to write!\n";
		return;
	}
	for(size_t i=0;i<src_vocab_vec.size();i++)
	{
		f_src_vocab<<src_vocab_vec.at(i)+" "+to_string(i)+"\n";
	}
	f_src_vocab.close();

	ofstream f_tgt_vocab("vocab.en");
	if (!f_tgt_vocab.is_open())
	{
		cout<<"fail open en vocab file to write!\n";
		return;
	}
	for(size_t i=0;i<tgt_vocab_vec.size();i++)
	{
		f_tgt_vocab<<tgt_vocab_vec.at(i)+" "+to_string(i)+"\n";
	}
	f_tgt_vocab.close();
}
