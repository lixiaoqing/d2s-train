#include "rule_counter.h"

void RuleCounter::update(string &rule)
{
	vector<string> vs = Split(rule," ||| ");
	string rule_src = vs[0];
	string rule_tgt = vs[1];
	string tgt_nt_idx_to_src_nt_idx = vs[2];
	double lex_weight_t2s = stod(vs[3]);
	double lex_weight_s2t = stod(vs[4]);
	string rule_str = rule_src + " ||| " + rule_tgt + " ||| " + tgt_nt_idx_to_src_nt_idx;
    auto it1 = rule2count_and_accumulate_lex_weight.find(rule_str);
    if (it1 != rule2count_and_accumulate_lex_weight.end())
    {
        it1->second.count += 1;
        it1->second.acc_lex_weight_t2s += lex_weight_t2s;
        it1->second.acc_lex_weight_s2t += lex_weight_s2t;
    }
    else
    {
        rule2count_and_accumulate_lex_weight[rule_str] = {1,lex_weight_t2s,lex_weight_s2t};
    }

    auto it2 = rule_src2count.find(rule_src);
    if (it2 != rule_src2count.end())
    {
        it2->second += 1;
    }
    else
    {
        rule_src2count[rule_src] = 1;
    }

    auto it3 = rule_tgt2count.find(rule_tgt+" ||| "+tgt_nt_idx_to_src_nt_idx);
    if (it3 != rule_tgt2count.end())
    {
        it3->second += 1;
    }
    else
    {
        rule_tgt2count[rule_tgt+" ||| "+tgt_nt_idx_to_src_nt_idx] = 1;
    }
}

void RuleCounter::dump_rules()
{
	ofstream fout("rule-table");
	if (!fout.is_open())
	{
		cerr<<"fails to open rule-table to write\n";
		return;
	}
    for (auto &kvp : rule2count_and_accumulate_lex_weight)
    {
        string rule = kvp.first;
        vector<string> vs = Split(rule," ||| ");
        string &rule_src = vs[0];
        string &rule_tgt = vs[1];
		string tgt_nt_idx_to_src_nt_idx = vs[2];
        double rule_count = (double)kvp.second.count;
        double lex_weight_t2s = kvp.second.acc_lex_weight_t2s/rule_count;
        double lex_weight_s2t = kvp.second.acc_lex_weight_s2t/rule_count;
        double trans_prob_t2s = rule_count/rule_src2count[rule_src];
        double trans_prob_s2t = rule_count/rule_tgt2count[rule_tgt+" ||| "+tgt_nt_idx_to_src_nt_idx];
        fout<<rule<<" ||| "<<trans_prob_t2s<<" "<<trans_prob_s2t<<" "<<lex_weight_t2s<<" "<<lex_weight_s2t<<endl;
    }
}
