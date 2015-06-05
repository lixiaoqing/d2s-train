#ifndef TREE_STR_PAIR_H
#define TREE_STR_PAIR_H
#include "stdafx.h"
#include "myutils.h"

typedef pair<int,int> Span;			//由起始位置和span长度表示（span长度为实际长度减1）
// 源端句法树节点
struct SyntaxNode
{
	string word;                                    // 该节点的词
	string tag;                                     // 该节点的词性
	int idx;										// 该节点在句子中的位置
	int father;										// 该节点的父节点在句子中的位置
	vector<int> children;							// 该节点的孩子节点在句子中的位置
	Span src_span;                         			// 该节点对应的源端span,用起始位置和跨度长度表示
	Span tgt_span;                         			// 该节点对应的目标端span
	vector<string> rules;							// 该节点所有规则的字符串形式
	bool lex_align_consistent;						// 该节点单词是否满足对齐一致性
	bool substree_align_consistent;					// 该节点对应的子树是否满足对齐一致性
	
	SyntaxNode ()
	{
		father = -1;
		idx = -1;
		src_span = make_pair(-1,-1);
		tgt_span = make_pair(-1,-1);
		lex_align_consistent = false;
		lex_align_consistent = false;
	}
};

struct RuleSrcUnit
{
	int type;
	string word;
	string tag;
	Span tgt_span;
	double lex_weight;
};

class TreeStrPair
{
	public:
		TreeStrPair(string &line_tree,string &line_str,string &line_align);
		void extract_rules(int sub_root_idx);
		void dump_rules(int sub_root_idx);

	private:
		void build_tree_from_str(const vector<string> &word_hidx_vec);
		void load_alignment(const string &align_line);
		void cal_proj_span();
		Span merge_span(Span span1,Span span2);
		void check_alignment_agreement();
		void cal_span_for_each_node(int sub_root_idx);
		void extract_head_rule(SyntaxNode &node);
		void extract_head_mod_rule(SyntaxNode &node);
		vector<Span> expand_tgt_span(Span tgt_span,Span bound);
		void generalize_head_mod_rule(SyntaxNode &node,vector<RuleSrcUnit> &rule_src,Span expanded_tgt_span,string &config);
		bool is_config_valid(vector<RuleSrcUnit> &rule_src,string &config);

	public:
		int root_idx;
		vector<SyntaxNode> src_nodes;
		vector<vector<Span> > src_span_to_tgt_span;						//记录每个源端span投射到目标端的span
		vector<vector<Span> > tgt_span_to_src_span;						//记录每个目标端span投射到源端的span
		vector<vector<int> > src_idx_to_tgt_idx;						//记录每个源端位置对应的目标端位置
		vector<vector<int> > tgt_idx_to_src_idx;						//记录每个目标端位置对应的源端位置
		vector<vector<bool> > src_span_to_alignment_agreement_flag;		//记录每个源端span是否满足对齐一致性
		vector<string> tgt_words;
		int src_sen_len;
		int tgt_sen_len;
		set<string> open_tags;
};

#endif
