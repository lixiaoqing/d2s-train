#include "stdafx.h"
#include "myutils.h"

typedef pair<int,int> Span;			//由起始位置和span长度表示（span长度为实际长度减1）

struct Rule
{
    Span src_span;
    Span tgt_span;
};

struct QuadRules                    //record the four group of rules which use current point as a corner in the alignment matrix
{
    vector<Rule> leftbottom_rules;
    vector<Rule> lefttop_rules;
    vector<Rule> rightbottom_rules;
    vector<Rule> righttop_rules;
};

class SampleExtractor
{
    public:
        SampleExtractor(string &line_src, string &line_tgt, string &line_al);
        void extract(vector<string> &samples);

    private:
		void load_alignment(string &align_line);
		void cal_proj_span();
		Span merge_span(Span span1,Span span2);
        Rule get_min_rule(vector<Rule> &rules);
        Rule get_max_rule(vector<Rule> &rules);
        void fill_alignpoint2quadrules();
        vector<Span> expand_tgt_span(Span tgt_span,Span bound);
        vector<string> get_boundary_words(Rule &rule);

    private:
		vector<vector<Span> > src_span_to_tgt_span;						//记录每个源端span投射到目标端的span，span用起始位置和跨度长度来表示
		vector<vector<Span> > tgt_span_to_src_span;						//记录每个目标端span投射到源的span
		vector<string> src_words;
		vector<string> tgt_words;
		int src_sen_len;
		int tgt_sen_len;
        vector<vector<QuadRules> > alignpoint2quadrules;
};
