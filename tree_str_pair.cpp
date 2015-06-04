#include "tree_str_pair.h"

TreeStrPair::TreeStrPair(string &line_tree,string &line_str,string &line_align)
{
	vector<string> word_hidx_vec = Split(line_tree);
	src_sen_len = word_hidx_vec.size();
	src_nodes.resize(src_sen_len);
	src_idx_to_tgt_idx.resize(src_sen_len);
	src_span_to_tgt_span.resize(src_sen_len);
	src_span_to_alignment_agreement_flag.resize(src_sen_len);
	for (int beg=0;beg<src_sen_len;beg++)
	{
		src_span_to_tgt_span.at(beg).resize(src_sen_len-beg,make_pair(-1,-1));
		src_span_to_alignment_agreement_flag.at(beg).resize(src_sen_len-beg,false);
	}
	build_tree_from_str(word_hidx_vec);
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
	cal_span_for_each_node(root_idx);
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
void TreeStrPair::build_tree_from_str(const vector<string> &word_hidx_vec)
{
	for (int i=0;i<src_sen_len;i++)
	{
		const string &word_hidx = word_hidx_vec.at(i);
		int sep = word_hidx.rfind('_');
		string word = word_hidx.substr(0,sep);
		int hidx = stoi(word_hidx.substr(sep+1));
		src_nodes.at(i).label = word;
		src_nodes.at(i).idx = i;
		src_nodes.at(i).father = hidx;
		if (hidx == -1)
		{
			root_idx = i;
			src_nodes.at(i).father = i;
		}
		else
		{
			src_nodes.at(hidx).children.push_back(i);
		}
	}
}

/**************************************************************************************
 1. 函数功能: 检查当前子树中的每个节点是否为边界节点
 2. 入口参数: 当前子树的根节点
 3. 出口参数: 无
 4. 算法简介: 1) 后序遍历当前子树
 			  2) 根据子节点的src_span和tgt_span计算当前节点的src_span和tgt_span
			  3) 检查tgt_span中的每个词是否都对齐到src_span中，从而确定当前节点是否为
			     边界节点
************************************************************************************* */
void TreeStrPair::cal_span_for_each_node(int sub_root_idx)
{
	auto &node = src_nodes.at(sub_root_idx);
	if (node.children.empty() )                                           // 叶节点
	{
		node.src_span = make_pair(node.idx,0);
		node.tgt_span = src_span_to_tgt_span[node.idx][0];
		if (src_span_to_alignment_agreement_flag[node.idx][0] == true)
		{
			node.lex_align_consistent = true;
			node.substree_align_consistent = true;
		}
		//cout<<node.label<<' '<<src_nodes.at(node.father).label<<' '<<node.src_span.first<<' '<<node.src_span.second<<' ';
		//cout<<node.tgt_span.first<<' '<<node.tgt_span.second<<' '<<node.lex_align_consistent<<' '<<node.substree_align_consistent<<'\n';
		return;
	}
	for (int child_idx : node.children)
	{
		cal_span_for_each_node(child_idx);
	}
	auto &first_child = src_nodes.at(node.children.front());
	auto &last_child = src_nodes.at(node.children.back());
	node.src_span = merge_span(make_pair(first_child.src_span.first,last_child.src_span.first+last_child.src_span.second
								-first_child.src_span.first),make_pair(node.idx,0));
	node.tgt_span = src_span_to_tgt_span[node.src_span.first][node.src_span.second];
	if (src_span_to_alignment_agreement_flag[node.idx][0] == true)
	{
		node.lex_align_consistent = true;
	}
	if (src_span_to_alignment_agreement_flag[node.src_span.first][node.src_span.second] == true)
	{
		node.substree_align_consistent = true;
	}
	//cout<<node.label<<' '<<src_nodes.at(node.father).label<<' '<<node.src_span.first<<' '<<node.src_span.second<<' ';
	//cout<<node.tgt_span.first<<' '<<node.tgt_span.second<<' '<<node.lex_align_consistent<<' '<<node.substree_align_consistent<<'\n';
}

/**************************************************************************************
 1. 函数功能: 抽取当前子树中每个节点对应的规则
 2. 入口参数: 当前子树的根节点
 3. 出口参数: 无
 4. 算法简介: 1) 后序遍历当前子树
 			  2) 根据子节点与根节点的对齐一致性判断是否能抽取规则
************************************************************************************* */
void TreeStrPair::extract_rules(int sub_root_idx)
{
	auto &node = src_nodes.at(sub_root_idx);
	if (node.children.empty() )                                           // 叶节点
	{
		if (node.lex_align_consistent == true)
		{
			extract_head_rule(node);
		}
		return;
	}
	for (int child_idx : node.children)
	{
		extract_rules(child_idx);
	}
	if (node.lex_align_consistent == true)
	{
		extract_head_rule(node);
		bool flag = true;
		for (int child_idx : node.children)
		{
			if (src_nodes.at(child_idx).substree_align_consistent == false)
			{
				flag == false;
				break;
			}
		}
		if (flag == true)
		{
			extract_head_mod_rule(node);
		}
	}
}

void TreeStrPair::extract_head_rule(SyntaxNode &node)
{
	string rule_src = node.label;
	vector<Span> expanded_tgt_spans = expand_tgt_span(src_span_to_tgt_span[node.idx][0],make_pair(0,tgt_sen_len-1));
	for (auto expanded_tgt_span : expanded_tgt_spans)
	{
		string rule_tgt;
		for (int i=expanded_tgt_span.first; i<=expanded_tgt_span.first+expanded_tgt_span.second; i++)
		{
			rule_tgt += tgt_words.at(i) + " ";
		}
		node.rules.push_back(rule_src+" ||| "+rule_tgt);
	}
}

void TreeStrPair::extract_head_mod_rule(SyntaxNode &node)
{
	string rule_src;
	int variable_num = 0;
	vector<int> tgt_replacement_status(tgt_sen_len,-1);
	for (auto child_idx : node.children)
	{
		auto &child = src_nodes.at(child_idx);
		if (child.children.empty())
		{
			rule_src += child.label + " ";
		}
		else
		{
			rule_src += "x"+to_string(variable_num)+":"+child.label+" ";
			for (int i=child.tgt_span.first;i<=child.tgt_span.first+child.tgt_span.second;i++)
			{
				tgt_replacement_status.at(i) = variable_num;
			}
			variable_num++;
		}
	}
	rule_src += node.label;

	vector<Span> expanded_tgt_spans = expand_tgt_span(node.tgt_span,make_pair(0,tgt_sen_len-1));
	for (auto expanded_tgt_span : expanded_tgt_spans)
	{
		string rule_tgt;
		int i = expanded_tgt_span.first;
		int tgt_span_end = expanded_tgt_span.first+expanded_tgt_span.second;
		while(i<=tgt_span_end)
		{
			if (tgt_replacement_status.at(i) == -1)
			{
				rule_tgt += tgt_words.at(i) + " ";
				i++;
			}
			else
			{
				int src_variable_num = tgt_replacement_status.at(i);
				rule_tgt += "x"+to_string(src_variable_num)+" ";
				while(i<=tgt_span_end && tgt_replacement_status.at(i) == src_variable_num)
				{
					i++;
				}
			}
		}
		node.rules.push_back(rule_src+" ||| "+rule_tgt);
	}
}

void TreeStrPair::dump_rules(int sub_root_idx)
{
	auto &node = src_nodes.at(sub_root_idx);
	if (node.children.empty() )                                           // 叶节点
	{
		for (auto rule : node.rules)
		{
			cout<<rule<<endl;
		}
		return;
	}
	for (int child_idx : node.children)
	{
		dump_rules(child_idx);
	}
	for (auto rule : node.rules)
	{
		cout<<rule<<endl;
	}

}
/**************************************************************************************
 *  1. 函数功能: 将目标端span向两端扩展，直到遇到对齐的词
 *  2. 入口参数: 待扩展的目标端span
 *  3. 出口参数: 所有可用的目标端span
 *  4. 算法简介: 见注释
 ************************************************************************************** */
vector<Span> TreeStrPair::expand_tgt_span(Span tgt_span,Span bound)
{
	if (tgt_span.first == -1)
		return {tgt_span};
	vector<Span> expanded_spans;
	for (int tgt_beg=tgt_span.first;tgt_beg>=max(bound.first,tgt_span.first-3);tgt_beg--)       //向左扩展
	{
		if (tgt_beg<tgt_span.first && !tgt_idx_to_src_idx[tgt_beg].empty())           //遇到对齐的词
			break;
		int tgt_span_end = tgt_span.first+tgt_span.second;
		for (int tgt_len=tgt_span_end-tgt_beg;tgt_beg+tgt_len<=min(tgt_span_end+3,bound.first+bound.second);tgt_len++)      //向右扩展
		{
			if (tgt_len>tgt_span_end-tgt_beg && !tgt_idx_to_src_idx[tgt_beg+tgt_len].empty())                     //遇到对齐的词
				break;
			expanded_spans.push_back(make_pair(tgt_beg,tgt_len));
		}
	}
	return expanded_spans;
}
