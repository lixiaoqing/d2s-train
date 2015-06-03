#include "rule_counter.h"

void RuleCounter::update(string &rule_src,string &rule_tgt,double lex_weight_t2s,double lex_weight_s2t)
{
    string rule = rule_src+" ||| "+rule_tgt;
    auto it1 = rule2count_and_accumulate_lex_weight.find(rule);
    if (it1 != rule2count_and_accumulate_lex_weight.end())
    {
        it1->second.count += 1;
        it1->second.acc_lex_weight_t2s += lex_weight_t2s;
        it1->second.acc_lex_weight_s2t += lex_weight_s2t;
    }
    else
    {
        rule2count_and_accumulate_lex_weight[rule] = {1,lex_weight_t2s,lex_weight_s2t};
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

    auto it3 = rule_tgt2count.find(rule_tgt);
    if (it3 != rule_tgt2count.end())
    {
        it3->second += 1;
    }
    else
    {
        rule_tgt2count[rule_tgt] = 1;
    }

    string root = rule_src.substr(0,rule_src.find(" "));
    auto it4 = root2count.find(root);
    if (it4 != root2count.end())
    {
        it4->second += 1;
    }
    else
    {
        root2count[root] = 1;
    }
}

void RuleCounter::dump_rules()
{
    for (auto &kvp : rule2count_and_accumulate_lex_weight)
    {
        string rule = kvp.first;
        vector<string> vs = Split(rule," ||| ");
        string &rule_src = vs[0];
        string &rule_tgt = vs[1];
        string root = rule_src.substr(0,rule_src.find(" "));
        double rule_count = (double)kvp.second.count;
        double lex_weight_t2s = kvp.second.acc_lex_weight_t2s/rule_count;
        double lex_weight_s2t = kvp.second.acc_lex_weight_s2t/rule_count;
        double trans_prob_t2s = rule_count/rule_src2count[rule_src];
        double trans_prob_s2t = rule_count/rule_tgt2count[rule_tgt];
        double root2rule_prob = rule_count/root2count[root];
        cout<<rule<<" ||| "<<root2rule_prob<<" "<<trans_prob_t2s<<" "<<trans_prob_s2t<<" "<<lex_weight_t2s<<" "<<lex_weight_s2t<<endl;
    }
}
