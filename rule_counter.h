#ifndef RULE_COUNTER_H
#define RULE_COUNTER_H
#include "stdafx.h"
#include "myutils.h"

struct Rule
{
	string rule_str;
	double lex_weight_forward;
	double lex_weight_backward;
};

struct CountAndLexWeight
{
    int count;
    double acc_lex_weight_t2s;
    double acc_lex_weight_s2t;
};

class RuleCounter
{
    public:
        void update(Rule &rule);
        void dump_rules();

    private:
        map<string,CountAndLexWeight> rule2count_and_accumulate_lex_weight;
        map<string,int> rule_src2count;
        map<string,int> rule_tgt2count;
};

#endif
