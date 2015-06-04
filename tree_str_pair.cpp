#include "tree_str_pair.h"

TreeStrPair::TreeStrPair(string &line_tree,string &line_str,string &line_align)
{
	vector<string> word_hpos_vec = Split(line_tree);
	src_sen_len = word_hpos_vec.size();
	src_nodes.resize(src_sen_len);
	src_idx_to_tgt_idx.resize(src_sen_len);
	src_span_to_tgt_span.resize(src_sen_len);
	src_span_to_alignment_agreement_flag.resize(src_sen_len);
	for (int beg=0;beg<src_sen_len;beg++)
	{
		src_span_to_tgt_span.at(beg).resize(src_sen_len-beg,make_pair(-1,-1));
		src_span_to_alignment_agreement_flag.at(beg).resize(src_sen_len-beg,false);
	}
	build_tree_from_str(word_hpos_vec);
	tgt_words = Split(line_str);
	tgt_sen_len = tgt_words.size();
	tgt_idx_to_src_idx.resize(tgt_sen_len);
	tgt_span_to_src_span.resize(tgt_sen_len);
	for (int beg=0;beg<tgt_sen_len;beg++)
	{
		tgt_span_to_src_span.at(beg).resize(tgt_sen_len-beg,make_pair(-1,-1));
	}
	load_alignment(line_align);
	cal_proj_span();
	check_alignment_agreement();
	cal_span_for_each_node(root_pos);
}

void TreeStrPair::load_alignment(const string &line_align)
{
	vector<string> alignments = Split(line_align);
	for (auto align : alignments)
	{
		vector<string> idx_pair = Split(align,"-");
		int src_idx = stoi(idx_pair.at(0));
		int tgt_idx = stoi(idx_pair.at(1));
		src_span_to_tgt_span[src_idx][0] = merge_span(src_span_to_tgt_span[src_idx][0],make_pair(tgt_idx,0));
		tgt_span_to_src_span[tgt_idx][0] = merge_span(tgt_span_to_src_span[tgt_idx][0],make_pair(src_idx,0));
		src_idx_to_tgt_idx[src_idx].push_back(tgt_idx);
		tgt_idx_to_src_idx[tgt_idx].push_back(src_idx);
	}
}

/**************************************************************************************
 1. 函数功能: 计算每个源端span投射到目标端的span，以及每个目标端span投射到源端的span
 2. 入口参数: 无
 3. 出口参数: 无
 4. 算法简介: 采用动态规划算法自底向上，自左向右地计算每个span的proj_span，计算公式为
 			  proj_span[beg][len] = proj_span[beg][len-1] + proj_span[beg+len][0]
************************************************************************************* */
void TreeStrPair::cal_proj_span()
{
	for (int span_len=1;span_len<src_sen_len;span_len++)
	{
		for (int beg=0;beg<src_sen_len-span_len;beg++)
		{
			src_span_to_tgt_span[beg][span_len] = merge_span(src_span_to_tgt_span[beg][span_len-1],src_span_to_tgt_span[beg+span_len][0]);
		}
	}
	for (int span_len=1;span_len<tgt_sen_len;span_len++)
	{
		for (int beg=0;beg<tgt_sen_len-span_len;beg++)
		{
			tgt_span_to_src_span[beg][span_len] = merge_span(tgt_span_to_src_span[beg][span_len-1],tgt_span_to_src_span[beg+span_len][0]);
		}
	}
}
/**************************************************************************************
 1. 函数功能: 将两个span合并为一个span
 2. 入口参数: 被合并的两个span
 3. 出口参数: 合并后的span
 4. 算法简介: 见注释
************************************************************************************* */
Span TreeStrPair::merge_span(Span span1,Span span2)
{
	if (span2.first == -1)
		return span1;
	if (span1.first == -1)
		return span2;
	Span span;
	span.first = min(span1.first,span2.first);												// 合并后span的左边界
	span.second = max(span1.first+span1.second,span2.first+span2.second) - span.first;		// 合并后span的长度，即右边界减去左边界
	return span;
}

/**************************************************************************************
 1. 函数功能: 检查每个源端span是否满足词对齐一致性
 2. 入口参数: 无
 3. 出口参数: 无
 4. 算法简介: 根据预先计算好的源端span和目标端span的映射表，检查源端span映射到目标端
 			  再映射回来的span是否越过了原来源端span的边界
************************************************************************************* */
void TreeStrPair::check_alignment_agreement()
{
	for (int beg=0;beg<src_sen_len;beg++)
	{
		for (int span_len=0;span_len<src_sen_len-beg;span_len++)
		{
			Span tgt_span = src_span_to_tgt_span[beg][span_len];
			if (tgt_span.first == -1)
			{
				src_span_to_alignment_agreement_flag[beg][span_len] = false;	  //如果目标端span为空，认为不满足对齐一致性
			}
			else
			{
				Span proj_src_span = tgt_span_to_src_span[tgt_span.first][tgt_span.second];
				//如果目标端span投射回来的源端span不超原来的源端span，则满足对齐一致性
				if (proj_src_span.first >= beg && proj_src_span.first+proj_src_span.second <= beg+span_len)
				{
					src_span_to_alignment_agreement_flag[beg][span_len] = true;
				}
			}
		}
	}
}

/**************************************************************************************
 1. 函数功能: 将字符串解析成句法树
 2. 入口参数: 一句话的句法分析结果，Berkeley Parser格式
 3. 出口参数: 无
 4. 算法简介: 见注释
************************************************************************************* */
void TreeStrPair::build_tree_from_str(const vector<string> &word_hpos_vec)
{
	for (int i=0;i<src_sen_len;i++)
	{
		const string &word_hpos = word_hpos_vec.at(i);
		int sep = word_hpos.rfind('_');
		string word = word_hpos.substr(0,sep);
		int hpos = stoi(word_hpos.substr(sep+1));
		src_nodes.at(i).label = word;
		src_nodes.at(i).father = hpos;
		if (hpos == -1)
		{
			root_pos = i;
			src_nodes.at(i).father = i;
		}
		else
		{
			src_nodes.at(hpos).children.push_back(i);
		}
	}
}

/**************************************************************************************
 1. 函数功能: 检查以当前子树中的每个节点是否为边界节点
 2. 入口参数: 当前子树的根节点
 3. 出口参数: 无
 4. 算法简介: 1) 后序遍历当前子树
 			  2) 根据子节点的src_span和tgt_span计算当前节点的src_span和tgt_span
			  3) 检查tgt_span中的每个词是否都对齐到src_span中，从而确定当前节点是否为
			     边界节点
************************************************************************************* */
void TreeStrPair::cal_span_for_each_node(int sub_root_pos)
{
	auto &node = src_nodes.at(sub_root_pos);
	if (node.children.empty() )                                           // 叶节点
	{
		node.src_span = make_pair(sub_root_pos,0);
		node.tgt_span = src_span_to_tgt_span[sub_root_pos][0];
		if (src_span_to_alignment_agreement_flag[sub_root_pos][0] == true)
		{
			node.lex_align_consistent = true;
			node.substree_align_consistent = true;
		}
		cout<<node.label<<' '<<src_nodes.at(node.father).label<<' '<<node.src_span.first<<' '<<node.src_span.second<<' ';
		cout<<node.tgt_span.first<<' '<<node.tgt_span.second<<'\n';
		return;
	}
	for (int child_pos : node.children)
	{
		cal_span_for_each_node(child_pos);
	}
	auto &first_child = src_nodes.at(node.children.front());
	auto &last_child = src_nodes.at(node.children.back());
	node.src_span = merge_span(make_pair(first_child.src_span.first,last_child.src_span.first+last_child.src_span.second
								-first_child.src_span.first),make_pair(sub_root_pos,0));
	node.tgt_span = src_span_to_tgt_span[node.src_span.first][node.src_span.second];
	if (src_span_to_alignment_agreement_flag[node.src_span.first][node.src_span.second] == true)
	{
		node.lex_align_consistent = true;
		node.substree_align_consistent = true;
	}
		cout<<node.label<<' '<<src_nodes.at(node.father).label<<' '<<node.src_span.first<<' '<<node.src_span.second<<' ';
		cout<<node.tgt_span.first<<' '<<node.tgt_span.second<<'\n';
}

int main()
{
	string line_tree = "来自_2 法国_0 的_4 宇航_4 员_-1";
	string line_str = "astronauts coming from France";
	string line_align = "3-0 4-0 0-1 0-2 1-3";
	TreeStrPair *tspair = new TreeStrPair(line_tree,line_str,line_align);
}
