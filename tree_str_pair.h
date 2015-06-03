#ifndef TREE_STR_PAIR_H
#define TREE_STR_PAIR_H
#include "stdafx.h"
#include "myutils.h"
#include "rule_counter.h"

typedef pair<int,int> Span;			//由起始位置和span长度表示（span长度为实际长度减1）
// 源端句法树节点
struct SyntaxNode
{
	string label;                                   // 该节点的句法标签或者词
	int father;
	vector<int> children;
	Span src_span;                         // 该节点对应的源端span,用首位两个单词的位置表示
	Span tgt_span;                         // 该节点对应的目标端span
	set<string> str_rules;							// 该节点所有规则的字符串形式
	bool lex_align_consistent;						// 该节点单词是否满足对齐一致性
	bool substree_align_consistent;					// 该节点对应的子树是否满足对齐一致性
	
	SyntaxNode ()
	{
		father   = -1;
		src_span = make_pair(-1,-1);
		tgt_span = make_pair(-1,-1);
		lex_align_consistent = false;
		lex_align_consistent = true;
	}
};

class TreeStrPair
{
	public:
		TreeStrPair(string &line_tree,string &line_str,string &line_align);

	private:
		void build_tree_from_str(const vector<string> &word_hpos_vec);
		void load_alignment(const string &align_line);
		void cal_proj_span();
		Span merge_span(Span span1,Span span2);
		void check_alignment_agreement();
		void cal_span_for_each_node(int sub_root_pos);

	public:
		int root_pos;
		vector<SyntaxNode> src_nodes;
		vector<vector<Span> > src_span_to_tgt_span;						//记录每个源端span投射到目标端的span
		vector<vector<Span> > tgt_span_to_src_span;						//记录每个目标端span投射到源端的span
		vector<vector<int> > src_idx_to_tgt_idx;						//记录每个源端位置对应的目标端位置
		vector<vector<int> > tgt_idx_to_src_idx;						//记录每个目标端位置对应的源端位置
		vector<vector<bool> > src_span_to_alignment_agreement_flag;		//记录每个源端span是否满足对齐一致性
		vector<string> tgt_words;
		int src_sen_len;
		int tgt_sen_len;
};

#endif
