#include "rule_extractor.h"

RuleExtractor::RuleExtractor(string &line_tree,string &line_str,string &line_align,map<string,double> *lex_s2t,map<string,double> *lex_t2s,RuleCounter *counter)
{
	tspair = new TreeStrPair(line_tree,line_str,line_align,lex_s2t,lex_t2s,counter);
}

void RuleExtractor::extract_rules()
{
	extract_GHKM_rules(tspair->root);
	extract_SPMT_rules();
	extract_compose_rules(tspair->root);
	tspair->dump_all_rules(tspair->root);
}

/**************************************************************************************
 1. 函数功能: 抽取当前子树包含的最小规则
 2. 入口参数: 当前子树的根节点
 3. 出口参数: 无
 4. 算法简介: 先序遍历当前子树，抽取每个每个节点的最小规则
************************************************************************************* */
void RuleExtractor::extract_GHKM_rules(SyntaxNode* node)
{
	if (node->type == 1) 													// 当前节点为边界节点
	{
		Rule rule;
		rule.type = 1;
		rule.src_tree_frag.push_back(node);
		rule.src_node_status.push_back(-1);
		rule.src_node_span.push_back(node->tgt_span);
		rule.tgt_word_status.resize(tspair->tgt_sen_len,-1);
		rule.variable_num = 0;
		for (const auto child : node->children) 							// 对当前节点的每个孩子节点进行扩展，直到遇到边界节点或单词节点为止
		{
			find_frontier_frag(child,rule);
		}
		cal_tgt_word_num(rule);
		if (rule.tgt_word_num <= MAX_RHS_WORD_NUM && rule.src_tree_frag.size() <= MAX_LHS_NODE_NUM)
		{
			node->rules.push_back(rule);
		}
		if (!node->rules.empty())
		{
			attach_unaligned_words(node);
		}
	}
	for (const auto child : node->children)
	{
		extract_GHKM_rules(child);
	}
}

/**************************************************************************************
 1. 函数功能: 寻找当前节点的最小规则片段，并更新目标端单词状态
 2. 入口参数: 当前子树的根节点
 3. 出口参数: 源端规则片段，目标端单词状态，变量个数
 4. 算法简介: 如果当前节点为单词节点或者边界节点，则停止扩展，否则对子树进行先序遍历
************************************************************************************* */
void RuleExtractor::find_frontier_frag(SyntaxNode* node,Rule &rule)
{
	if (node->type == 0) 																//单词节点
	{
		rule.src_tree_frag.push_back(node);
		rule.src_node_status.push_back(-3);
		rule.src_node_span.push_back(node->tgt_span);
	}
	else if (node->type == 1)										                    //边界节点
	{
		rule.src_tree_frag.push_back(node);
		rule.src_node_status.push_back(rule.variable_num);
		rule.src_node_span.push_back(node->tgt_span);
		for (int tgt_idx=node->tgt_span.first;tgt_idx<=node->tgt_span.second;tgt_idx++)
		{
			rule.tgt_word_status.at(tgt_idx) = rule.variable_num;						//根据当前节点的tgt_span更新目标端单词的状态
		}
		rule.variable_num++;
	}
	else if (node->type == 2)              											    //非边界节点
	{
		rule.src_tree_frag.push_back(node);
		rule.src_node_status.push_back(-2);
		rule.src_node_span.push_back(make_pair(-1,-1));
		for (const auto child : node->children)
		{
			find_frontier_frag(child,rule);
		}
	}
}

void RuleExtractor::cal_tgt_word_num(Rule &rule)
{
	for (int i=rule.src_node_span.front().first;i<rule.src_node_span.front().second;i++)
	{
		if (rule.tgt_word_status.at(i) == -1)
		{
			rule.tgt_word_num++;
		}
	}
}

/**************************************************************************************
 1. 函数功能: 将目标端未对齐的单词向左（右）依附到最小规则上，形成新的规则
 2. 入口参数: 无
 3. 出口参数: 无
 4. 算法简介: 见注释
************************************************************************************* */
void RuleExtractor::attach_unaligned_words(SyntaxNode* node)
{
	for (int tgt_idx=0;tgt_idx<tspair->tgt_sen_len;tgt_idx++)									//遍历目标语言句子的所有单词
	{
		if (tspair->tgt_idx_to_src_idx.at(tgt_idx).size() > 0)       							//跳过有对齐的单词
			continue;
		int i;
		for (i=tgt_idx-1;i>=0;i--)                         			    						//向左扫描，直到遇到有对齐的单词
		{
			if (tspair->tgt_idx_to_src_idx.at(i).size() > 0)
				break;
		}
		if (i>=0) 											 									//左边有单词有对齐
		{
			for (int j=0;j<node->rules.front().src_tree_frag.size();j++)	    				//遍历最小规则的每个节点，检查能否被依附
			{
				if (node->rules.front().src_tree_frag.at(j)->tgt_span.second == i				//被检查节点的右边界等于i
					&& node->rules.front().src_tree_frag.at(j)->type == 1)						//被检查节点为边界节点
				{
					Rule rule = node->rules.front();
					rule.type = 2;
					if (rule.src_node_span.at(0).second < tgt_idx)
					{
						rule.src_node_span.at(0).second = tgt_idx;								//更新规则根节点的目标端span
					}
					rule.src_node_span.at(j).second = tgt_idx;									//更新变量节点的在目标端的控制范围
					int variable_idx = node->rules.front().src_node_status.at(j);
					if (variable_idx >= 0)
					{
						for (int k=rule.src_node_span.at(j).first;k<=rule.src_node_span.at(j).second;k++)
						{
							rule.tgt_word_status.at(k) = variable_idx;
						}
					}
					cal_tgt_word_num(rule);
					if (rule.tgt_word_num <= MAX_RHS_WORD_NUM && rule.src_tree_frag.size() <= MAX_LHS_NODE_NUM)
					{
						node->rules.push_back(rule);
					}
				}
			}
		}

		for (i=tgt_idx+1;i<tspair->tgt_sen_len;i++)                        		   				//向右扫描，直到遇到有对齐的单词
		{
			if (tspair->tgt_idx_to_src_idx.at(i).size() > 0)
				break;
		}
		if (i < tspair->tgt_sen_len) 															//右边有单词有对齐
		{
			for (int j=0;j<node->rules.front().src_tree_frag.size();j++)	    				//遍历最小规则的每个节点，检查能否被依附
			{
				if (node->rules.front().src_tree_frag.at(j)->tgt_span.first == i				//被检查节点的左边界等于i
					&& node->rules.front().src_tree_frag.at(j)->type == 1)						//被检查节点为边界节点
				{
					Rule rule = node->rules.front();
					rule.type = 2;
					if (rule.src_node_span.at(0).first > tgt_idx)
					{
						rule.src_node_span.at(0).first = tgt_idx;								//更新规则根节点的目标端span
					}
					rule.src_node_span.at(j).first = tgt_idx;									//更新变量节点的在目标端的控制范围
					int variable_idx = node->rules.front().src_node_status.at(j);
					if (variable_idx >= 0)
					{
						for (int k=rule.src_node_span.at(j).first;k<=rule.src_node_span.at(j).second;k++)
						{
							rule.tgt_word_status.at(k) = variable_idx;
						}
					}
					cal_tgt_word_num(rule);
					if (rule.tgt_word_num <= MAX_RHS_WORD_NUM && rule.src_tree_frag.size() <= MAX_LHS_NODE_NUM)
					{
						node->rules.push_back(rule);
					}
				}
			}
		}
	}
}

/**************************************************************************************
 1. 函数功能: 抽取目标语言每一个span对应的句法短语规则
 2. 入口参数: 无
 3. 出口参数: 无
 4. 算法简介: 遍历目标语言每一个span
 			  1) 检查是否存在满足词对齐约束的源端短语
			  2) 寻找能覆盖源端短语的最低句法节点
			  3) 先序遍历句法子树，找出规则源端
************************************************************************************* */
void RuleExtractor::extract_SPMT_rules()
{
	for (int len=0;len<tspair->tgt_sen_len && len<MAX_SPMT_PHRASE_LEN;len++)	  //遍历目标端所有的短语
	{
		for (int beg=0;beg+len<tspair->tgt_sen_len;beg++)
		{
			pair<int,int> tgt_span = make_pair(beg,beg+len);
			pair<int,int> src_span = cal_src_span_for_tgt_span(tgt_span); 	      //计算目标端短语对应的源端span
			if (src_span.first == -1)										      //目标短语内的单词全都对空
				continue;
			bool flag = check_alignment_for_src_span(src_span,tgt_span);	      //检查源端span中的单词是否对到了目标端span的外面
			if (flag == false)
				continue;
			//寻找能覆盖源端span的最低句法节点
			SyntaxNode *node = tspair->word_nodes.at(src_span.first)->father;     //从源端span最左端单词的词性节点往上找
			while (node->src_span.second < src_span.second) 					  //当前节点无法覆盖源端span右边界（左边界肯定能被覆盖）
			{
				node = node->father;
			}
			if (node->type != 1)                								  //找到的根节点不是边界节点
				continue;
			//先序遍历以当前节点为根节点的子树，找出规则源端
			Rule rule;
			rule.type = 3;
			rule.src_tree_frag.push_back(node);
			rule.src_node_status.push_back(-1);
			rule.src_node_span.push_back(tgt_span);
			rule.tgt_word_status.resize(tspair->tgt_sen_len,-1);
			rule.variable_num = 0;
			for (const auto child : node->children) 							 // 对当前节点的每个孩子节点进行扩展，直到遇到源端span以外的边界节点或单词节点
			{
				flag = find_syntax_phrase_frag(child,rule,src_span);
				if (flag == false)
					break;
			}
			if (flag == false)
				continue;
			int lbound = min(tgt_span.first,node->tgt_span.first);               // 当前规则的左右边界
			int rbound = max(tgt_span.second,node->tgt_span.second);
			rule.src_node_span.at(0) = make_pair(lbound,rbound);
			for (int tgt_idx=lbound;tgt_idx<=rbound;tgt_idx++)
			{
				if (rule.tgt_word_status.at(tgt_idx) == -1 && (tgt_idx<tgt_span.first || tgt_idx>tgt_span.second))
				{
					rule.tgt_word_status.at(tgt_idx) = -2;						 //跳过目标端span以外的未对齐的词
				}
			}
			cal_tgt_word_num(rule);
			if (rule.tgt_word_num <= MAX_RHS_WORD_NUM && rule.src_tree_frag.size() <= MAX_LHS_NODE_NUM)
			{
				node->rules.push_back(rule);
			}
		}
	}
}

/**************************************************************************************
 1. 函数功能: 寻找当前节点的句法短语规则片段，并更新目标端单词状态
 2. 入口参数: 当前子树的根节点
 3. 出口参数: 成功标识，源端规则片段，目标端单词状态，变量个数
 4. 算法简介: 如果当前节点为超过源端span的边界节点，则停止扩展，否则对子树进行先序遍历
 			  如果超过源端span的节点为非边界节点，则返回错误
************************************************************************************* */
bool RuleExtractor::find_syntax_phrase_frag(SyntaxNode* node,Rule &rule,pair<int,int> src_span)
{
	if (node->type == 0) 																//单词节点
	{
		rule.src_tree_frag.push_back(node);
		rule.src_node_status.push_back(-3);
		rule.src_node_span.push_back(node->tgt_span);
	}
	else if (node->type == 1 && (node->src_span.first > src_span.second || node->src_span.second < src_span.first))   //超过了源端span的边界节点
	{
		rule.src_tree_frag.push_back(node);
		rule.src_node_status.push_back(rule.variable_num);
		rule.src_node_span.push_back(node->tgt_span);
		for (int tgt_idx=node->tgt_span.first;tgt_idx<=node->tgt_span.second;tgt_idx++)
		{
			rule.tgt_word_status.at(tgt_idx) = rule.variable_num;						//根据当前节点的tgt_span更新目标端单词的状态
		}
		rule.variable_num++;
	}
	else 																				//非边界节点或源端span内的边界节点
	{
		if (node->type == 2 && (node->src_span.first > src_span.second || node->src_span.second < src_span.first))  //超过了源端span的非边界节点
			return false;       //TODO 或许应该继续扩展
		rule.src_tree_frag.push_back(node);
		rule.src_node_status.push_back(-2);
		rule.src_node_span.push_back(node->tgt_span);
		for (const auto child : node->children)
		{
			bool flag = find_syntax_phrase_frag(child,rule,src_span);
			if (flag == false)
				return false;
		}
	}
	return true;
}

pair<int,int> RuleExtractor::cal_src_span_for_tgt_span(pair<int,int> tgt_span)
{
	int src_lbound = -1;
	int src_rbound = -1;
	for (int i=tgt_span.first;i<=tgt_span.second;i++)
	{
		if (tspair->tgt_idx_to_src_span.at(i).first == -1)
			continue;
		if (src_lbound == -1 || src_lbound > tspair->tgt_idx_to_src_span.at(i).first)
		{
			src_lbound = tspair->tgt_idx_to_src_span.at(i).first;
		}
		if (src_rbound == -1 || src_rbound < tspair->tgt_idx_to_src_span.at(i).second)
		{
			src_rbound = tspair->tgt_idx_to_src_span.at(i).second;
		}
	}
	return make_pair(src_lbound,src_rbound);
}

bool RuleExtractor::check_alignment_for_src_span(pair<int,int> src_span,pair<int,int> tgt_span)
{
	for (int j=src_span.first;j<=src_span.second;j++)
	{
		for (int tgt_idx : tspair->src_idx_to_tgt_idx.at(j))
		{
			if (tgt_idx < tgt_span.first || tgt_idx > tgt_span.second)
				return false;
		}
	}
	return true;
}

/**************************************************************************************
 1. 函数功能: 抽取当前子树包含的组合规则
 2. 入口参数: 当前子树的根节点
 3. 出口参数: 无
 4. 算法简介: 先序遍历当前子树，抽取每个每个节点的组合规则
************************************************************************************* */
void RuleExtractor::extract_compose_rules(SyntaxNode* node)
{
	if (node->type == 1)
	{
		vector<Rule>* rules_to_be_composed = &node->rules;
		vector<Rule>* composed_rules = new vector<Rule>;
		vector<vector<Rule>* > rules_to_be_deleted = {composed_rules};
		for (int compose_num=1;compose_num<MAX_RULE_SIZE;compose_num++)	 //一个组合规则最多由MAX_RULE_SIZE个最小规则(或者最多一个SPMT规则)组合而成
		{
			for (auto &rule : *rules_to_be_composed)
			{
				expand_rule(rule,composed_rules);						 //对待扩展规则进行扩展
			}
			if (composed_rules->empty())								 //待扩展规则不包含变量节点，无法继续扩展
				break;
			node->rules.insert(node->rules.end(),composed_rules->begin(),composed_rules->end());
			rules_to_be_composed = composed_rules;						 //将新生成的规则作为下一次的待扩展规则
			composed_rules = new vector<Rule>;
			rules_to_be_deleted.push_back(composed_rules);
		}
		for (auto p_rules : rules_to_be_deleted)
		{
			delete p_rules;												 //删除所有扩张的规则，否则会内存泄露
		}
	}
	for (auto child : node->children)
	{
		extract_compose_rules(child);
	}
}

/**************************************************************************************
 1. 函数功能: 对当前规则进行扩展，将生成的规则放入composed_rules中
 2. 入口参数: 当前规则的引用
 3. 出口参数: 存放新生成规则的composed_rules
 4. 算法简介: 对当前规则的每一个变量节点，使用该节点的最小规则对其进行替换
************************************************************************************* */
void RuleExtractor::expand_rule(Rule &rule,vector<Rule>* composed_rules)
{
	int variable_idx = -1;													//表示当前节点是规则中第几个变量节点
	for (int node_idx=0;node_idx<rule.src_tree_frag.size();node_idx++)		//遍历规则源端的每个节点
	{
		if (rule.src_node_status.at(node_idx) < 0)							//跳过非变量节点，只对变量节点进行扩展生成新规则
			continue;
		variable_idx++;
		for (auto &sub_rule : rule.src_tree_frag.at(node_idx)->rules)		//遍历该变量节点的所有规则
		{
			if (sub_rule.type > 2)											//跳过SPMT规则和组合规则，只使用最小规则来替换当前变量节点
				continue;
			/*
			if (sub_rule.type == 4)											//跳过组合规则，用最小规则和SPMT规则来替换当前变量节点
				continue;
			*/
			if (sub_rule.src_node_span.front() != rule.src_node_span.at(node_idx))
				continue;													//如果最小规则的目标端span与变量节点的不同，则跳过
			generate_new_rule(rule,node_idx,variable_idx,sub_rule,composed_rules);
		}
	}
}

/**************************************************************************************
 1. 函数功能: 根据当前规则和变量节点的规则生成新规则
 2. 入口参数: 当前规则，变量节点的规则，变量节点在所有节点中的位置，变量节点在所有变量
 			  节点中的位置
 3. 出口参数: 存放新生成规则的composed_rules
 4. 算法简介: 见注释
************************************************************************************* */
void RuleExtractor::generate_new_rule(Rule &rule,int node_idx,int variable_idx,Rule &sub_rule,vector<Rule>* composed_rules)
{
	Rule new_rule;
	new_rule.variable_num = rule.variable_num + sub_rule.variable_num - 1;
	new_rule.type = 4;
	new_rule.size = rule.size + 1;
	//生成新规则的源端句法节点序列
	new_rule.src_tree_frag.assign(rule.src_tree_frag.begin(),rule.src_tree_frag.begin()+node_idx);
	new_rule.src_tree_frag.insert(new_rule.src_tree_frag.end(),sub_rule.src_tree_frag.begin(),sub_rule.src_tree_frag.end());
	new_rule.src_tree_frag.insert(new_rule.src_tree_frag.end(),rule.src_tree_frag.begin()+node_idx+1,rule.src_tree_frag.end());
	//生成新规则源端每个节点对应的目标端span
	new_rule.src_node_span.assign(rule.src_node_span.begin(),rule.src_node_span.begin()+node_idx);
	new_rule.src_node_span.insert(new_rule.src_node_span.end(),sub_rule.src_node_span.begin(),sub_rule.src_node_span.end());
	new_rule.src_node_span.insert(new_rule.src_node_span.end(),rule.src_node_span.begin()+node_idx+1,rule.src_node_span.end());
	//生成新规则源端每个节点的状态信息（根节点，内部节点，单词节点或者第i个变量节点）
	new_rule.src_node_status.assign(rule.src_node_status.begin(),rule.src_node_status.begin()+node_idx);
	//生成新规则目标端每个单词的状态信息（没被替换或者被第i个源端变量替换）
	new_rule.tgt_word_status = rule.tgt_word_status;
	for (int i=0;i<sub_rule.src_node_status.size();i++)
	{
		int status = sub_rule.src_node_status.at(i);
		if (status == -1)
		{
			new_rule.src_node_status.push_back(-2);						//最小规则的根节点变成组合规则的内部节点
			for (int j=new_rule.src_node_span.at(node_idx+i).first;j<=new_rule.src_node_span.at(node_idx+i).second;j++)
			{
				new_rule.tgt_word_status.at(j) = -1;					//将该节点控制的目标端的每个单词的状态置为-1
			}
		}
		else if (status >= 0)
		{
			new_rule.src_node_status.push_back(status+variable_idx);	//最小规则的变量节点需更新变量编号
			for (int j=sub_rule.src_node_span.at(i).first;j<=sub_rule.src_node_span.at(i).second;j++)
			{
				new_rule.tgt_word_status.at(j) = status+variable_idx;	//更新该变量控制的目标端的每个单词的状态
			}
		}
		else
		{
			new_rule.src_node_status.push_back(status);					//最小规则的内部节点和单词节点状态保持不变
		}
	}
	for (int i=node_idx+1;i<rule.src_node_status.size();i++)
	{
		int status = rule.src_node_status.at(i);
		if (status >= 0)
		{
			new_rule.src_node_status.push_back(status+sub_rule.variable_num-1);		//后面的的变量节点需更新变量编号
			for (int j=rule.src_node_span.at(i).first;j<=rule.src_node_span.at(i).second;j++)
			{
				new_rule.tgt_word_status.at(j) = status+sub_rule.variable_num-1;	//更新该变量控制的目标端的每个单词的状态
			}
		}
		else
		{
			new_rule.src_node_status.push_back(status);					//内部节点和单词节点状态保持不变
		}
	}
	cal_tgt_word_num(new_rule);
	if (new_rule.tgt_word_num <= MAX_RHS_WORD_NUM && new_rule.src_tree_frag.size() <= MAX_LHS_NODE_NUM)
	{
		composed_rules->push_back(new_rule);
	}
}
