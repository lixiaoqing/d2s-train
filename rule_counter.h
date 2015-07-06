#ifndef RULE_COUNTER_H
#define RULE_COUNTER_H
#include "stdafx.h"
#include "myutils.h"

struct Rule
{
	string rule_str;
	double lex_weight_backward;
	double lex_weight_forward;
    double frac_count;
};

struct CountAndLexWeight
{
    double count;
    double acc_lex_weight_t2s;
    double acc_lex_weight_s2t;
};

class RuleCounter
{
    public:
        void update(Rule &rule);
        void dump_rules();

    private:
        unordered_map<string,CountAndLexWeight> rule2count_and_accumulate_lex_weight;
        unordered_map<string,double> rule_src2count;
        unordered_map<string,double> rule_tgt2count;
};

#endif
