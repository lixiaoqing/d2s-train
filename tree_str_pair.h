#ifndef TREE_STR_PAIR_H
#define TREE_STR_PAIR_H
#include "stdafx.h"
#include "myutils.h"
#include "rule_counter.h"

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
	unordered_map<string,vector<double> > rules;    // 该节点所有规则的字符串形式
	bool lex_align_consistent;						// 该节点单词是否满足对齐一致性
	bool subtree_align_consistent;					// 该节点对应的子树是否满足对齐一致性
	
	SyntaxNode ()
	{
		father = -1;
		idx = -1;
		src_span = make_pair(-1,-1);
		tgt_span = make_pair(-1,-1);
		lex_align_consistent = false;
		subtree_align_consistent = false;
	}
};

class TreeStrPair
{
	public:
		TreeStrPair(string &line_tree,string &line_str,string &line_align,unordered_map<string,double> *lex_s2t,unordered_map<string,double> *lex_t2s);
		void extract_rules(int sub_root_idx);
		void dump_rules(int sub_root_idx,vector<Rule> &rule_collector);

	private:
		void build_tree_from_str(const vector<string> &word_hidx_vec);
		void load_alignment(const string &align_line);
		void cal_proj_span();
		Span merge_span(Span span1,Span span2);
		void check_alignment_agreement();
		void cal_span_for_each_node(int sub_root_idx);
		void cal_lex_weight();
        bool check_subtree_align_consistent(SyntaxNode &node,int first_child_idx,int children_num);
		void extract_head_rule(SyntaxNode &node);
		void extract_fixed_rule(SyntaxNode &node,int first_child_idx,int children_num);
		void extract_floating_rule(SyntaxNode &node,int first_child_idx,int children_num);
        void generalize_rule(SyntaxNode &node,int first_child_idx,int children_num,Span rule_span,string config,int tgt_span_num,string struct_type);
		vector<Span> expand_tgt_span(Span tgt_span,Span bound);
		vector<vector<int> > get_tgt_replacement_status(vector<vector<Span> > &nt_spans_vec,Span rule_span);
		bool is_nt_span_combination_valid(vector<Span> &partial_combination, Span next_nt_span);

	public:
        bool valid_flag;
		unordered_map<string,double> *plex_s2t;
		unordered_map<string,double> *plex_t2s;
		int root_idx;
		vector<SyntaxNode> src_nodes;
		vector<vector<Span> > src_span_to_tgt_span;						//记录每个源端span投射到目标端的span
		vector<vector<Span> > tgt_span_to_src_span;						//记录每个目标端span投射到源端的span
		vector<vector<int> > src_idx_to_tgt_idx;						//记录每个源端位置对应的目标端位置
		vector<vector<int> > tgt_idx_to_src_idx;						//记录每个目标端位置对应的源端位置
		vector<vector<bool> > src_span_to_alignment_agreement_flag;		//记录每个源端span是否满足对齐一致性
		vector<double> lex_weight_t2s;									//记录每个源端单词的词汇权重
		vector<double> lex_weight_s2t;									//记录每个目标端端单词的词汇权重
		vector<string> tgt_words;
		int src_sen_len;
		int tgt_sen_len;
		set<string> open_tags;
};

#endif
