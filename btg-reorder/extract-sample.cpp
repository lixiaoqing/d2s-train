#include "extract-sample.h"

SampleExtractor::SampleExtractor(string &line_src, string &line_tgt, string &line_al)
{
	vector<string> wt_hidx_vec = Split(line_src);
	src_sen_len = wt_hidx_vec.size();
	src_nodes.resize(src_sen_len);
	src_words.resize(src_sen_len);
	build_tree_from_str(wt_hidx_vec);
    cal_subtree_span_for_each_node(root_idx);

    span2head.resize(src_sen_len);
    for (int beg=0;beg<src_sen_len;beg++)
    {
        span2head.at(beg).resize(src_sen_len-beg,-2);
    }
    for (int beg=0;beg<src_sen_len;beg++)
    {
        fill_span2head_with_node(beg);
    }

    tgt_words = Split(line_tgt);
	tgt_sen_len = tgt_words.size();

	src_span_to_tgt_span.resize(src_sen_len);
    alignpoint2quadrules.resize(src_sen_len+1,vector<QuadRules>(tgt_sen_len+1));
	for (int beg=0;beg<src_sen_len;beg++)
	{
		src_span_to_tgt_span.at(beg).resize(src_sen_len-beg,make_pair(-1,-1));
	}
	tgt_span_to_src_span.resize(tgt_sen_len);
	for (int beg=0;beg<tgt_sen_len;beg++)
	{
		tgt_span_to_src_span.at(beg).resize(tgt_sen_len-beg,make_pair(-1,-1));
	}
    
    load_alignment(line_al);
	cal_proj_span();
}

/**************************************************************************************
 1. 函数功能: 将字符串解析成句法树
 2. 入口参数: 一句话的依存句法分析结果，每个单元包括（词，词性，中心词位置）三个元素
 3. 出口参数: 无
 4. 算法简介: 见注释
************************************************************************************* */
void SampleExtractor::build_tree_from_str(const vector<string> &wt_hidx_vec)
{
	for (int i=0;i<src_sen_len;i++)
	{
		const string &wt_hidx = wt_hidx_vec.at(i);
		int sep = wt_hidx.rfind('_');
		string wt = wt_hidx.substr(0,sep);
		int hidx = stoi(wt_hidx.substr(sep+1));
		sep = wt.rfind('_');
		string word = wt.substr(0,sep);
		string tag = wt.substr(sep+1);
		src_nodes.at(i).word = word;
		src_words.at(i) = word;
		src_nodes.at(i).tag = tag;
		src_nodes.at(i).idx = i;
		if (hidx == -1)                                 //整个句子的中心词
		{
			root_idx = i;
		}
		else                                            //更新父节点的子节点位置向量，子节点位置保持了从左到右的顺序
		{
			src_nodes.at(hidx).children.push_back(i);
		}
	}
}

void SampleExtractor::cal_subtree_span_for_each_node(int sub_root_idx)
{
	auto &node = src_nodes.at(sub_root_idx);
	if (node.children.empty() )                                           // 叶节点
	{
		node.src_span = make_pair(node.idx,0);
		return;
	}
	for (int child_idx : node.children)
	{
		cal_subtree_span_for_each_node(child_idx);
	}
	auto &first_child = src_nodes.at(node.children.front());
	auto &last_child = src_nodes.at(node.children.back());
    //首先合并第一个和最后一个孩子的源端span，然后与当前节点的源端span合并
	node.src_span = merge_span(merge_span(first_child.src_span,last_child.src_span),make_pair(node.idx,0));
}

void SampleExtractor::fill_span2head_with_node(int node_idx)
{
	SyntaxNode &node = src_nodes.at(node_idx);
    int children_size = node.children.size();
    for (int children_num=1;children_num<=children_size;children_num++)
    {
        for (int first_child_idx=0;first_child_idx<=children_size-children_num;first_child_idx++)
        {
            auto &first_child = src_nodes.at(node.children.at(first_child_idx));
            auto &last_child = src_nodes.at(node.children.at(first_child_idx+children_num-1));
            //fixed结构，规则源端必须连续
            if (first_child.src_span.first-1 <= node.idx && last_child.src_span.first+last_child.src_span.second+1 >= node.idx)
            {
                //首先合并第一个和最后一个孩子的源端span，然后与当前节点的源端span合并
                Span fixed_span = merge_span(merge_span(first_child.src_span,last_child.src_span),make_pair(node.idx,0));
                span2head.at(fixed_span.first).at(fixed_span.second) = node.idx;                    //记录当前span对应的中心词在句子中的位置
            }
            if (children_num > 1)
            {
                //floating结构，规则源端必须连续
                if (first_child.idx > node.idx || last_child.idx < node.idx)
                {
                    Span floating_span = merge_span(first_child.src_span,last_child.src_span);
                    span2head.at(floating_span.first).at(floating_span.second) = -1;                //将floating结构的中心词位置记为-1
                }
            }
        }
    }
}

/**************************************************************************************
 1. 函数功能: 加载词对齐
 2. 入口参数: 一句话的词对齐结果
 3. 出口参数: 无
 4. 算法简介: 根据每一对对齐的单词，更新每个源端单词对应的目标端span，以及每个目标端
 			  单词对应的源端span
************************************************************************************* */
void SampleExtractor::load_alignment(string &line_align)
{
	vector<string> alignments = Split(line_align);
	for (auto align : alignments)
	{
		vector<string> idx_pair = Split(align,"-");
		int src_idx = stoi(idx_pair.at(0));
		int tgt_idx = stoi(idx_pair.at(1));
		if (src_idx >= src_sen_len || tgt_idx >= tgt_sen_len)
        {
            cerr<<"bad alignment\n";
            exit(0);
        }
		src_span_to_tgt_span[src_idx][0] = merge_span(src_span_to_tgt_span[src_idx][0],make_pair(tgt_idx,0));
		tgt_span_to_src_span[tgt_idx][0] = merge_span(tgt_span_to_src_span[tgt_idx][0],make_pair(src_idx,0));
	}
}

/**************************************************************************************
 1. 函数功能: 计算每个源端span投射到目标端的span，以及每个目标端span投射到源端的span
 2. 入口参数: 无
 3. 出口参数: 无
 4. 算法简介: 采用动态规划算法自底向上，自左向右地计算每个span的proj_span，计算公式为
 			  proj_span[beg][len] = proj_span[beg][len-1] + proj_span[beg+len][0]
************************************************************************************* */
void SampleExtractor::cal_proj_span()
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
 *  1. 函数功能: 将目标端span向两端扩展，直到遇到对齐的词
 *  2. 入口参数: 待扩展的目标端span
 *  3. 出口参数: 所有可用的目标端span
 *  4. 算法简介: 见注释
 ************************************************************************************** */
vector<Span> SampleExtractor::expand_tgt_span(Span tgt_span,Span bound)
{
	vector<Span> expanded_spans;
	for (int tgt_beg=tgt_span.first;tgt_beg>=max(bound.first,tgt_span.first-1);tgt_beg--)       							//向左扩展
	{
		if (tgt_beg<tgt_span.first && tgt_span_to_src_span[tgt_beg][0].first != -1)   										//遇到对齐的词
			break;
		int tgt_span_end = tgt_span.first+tgt_span.second;
		for (int tgt_len=tgt_span_end-tgt_beg;tgt_beg+tgt_len<=min(tgt_span_end+1,bound.first+bound.second);tgt_len++)      //向右扩展
		{
			if (tgt_beg+tgt_len>tgt_span_end && tgt_span_to_src_span[tgt_beg+tgt_len][0].first != -1)             			//遇到对齐的词
				break;
			expanded_spans.push_back(make_pair(tgt_beg,tgt_len));
		}
	}
	return expanded_spans;
}

void SampleExtractor::extract(vector<string> &samples)
{
    fill_alignpoint2quadrules();
    for (int src_idx=0;src_idx<src_sen_len;src_idx++)
    {
        for (int tgt_idx=0;tgt_idx<tgt_sen_len;tgt_idx++)
        {
            auto &leftbottom_rules = alignpoint2quadrules[src_idx][tgt_idx].leftbottom_rules;
            auto &righttop_rules = alignpoint2quadrules[src_idx][tgt_idx].righttop_rules;
            if (!leftbottom_rules.empty() && !righttop_rules.empty())
            {
                Rule min_rule_leftbottom = get_min_rule(leftbottom_rules);
                Rule min_rule_righttop = get_min_rule(righttop_rules);
                vector<string> boundary_words_leftbottom = get_boundary_words(min_rule_leftbottom);
                vector<string> boundary_words_righttop = get_boundary_words(min_rule_righttop);
                string sample = "straight c11="+boundary_words_leftbottom.at(0)+" c21="+boundary_words_righttop.at(0)
                                       +" e11="+boundary_words_leftbottom.at(1)+" e21="+boundary_words_righttop.at(1)
                                       +" c12="+boundary_words_leftbottom.at(2)+" c22="+boundary_words_righttop.at(2)
                                       +" e12="+boundary_words_leftbottom.at(3)+" e22="+boundary_words_righttop.at(3);
                /*
                string sample = "straight |||";
                for (int i=min_rule_leftbottom.src_span.first;i<=min_rule_leftbottom.src_span.first+min_rule_leftbottom.src_span.second;i++)
                {
                    sample += " "+src_words.at(i);
                }
                sample += " |||";
                for (int i=min_rule_leftbottom.tgt_span.first;i<=min_rule_leftbottom.tgt_span.first+min_rule_leftbottom.tgt_span.second;i++)
                {
                    sample += " "+tgt_words.at(i);
                }
                sample += " |||";
                for (int i=min_rule_righttop.src_span.first;i<=min_rule_righttop.src_span.first+min_rule_righttop.src_span.second;i++)
                {
                    sample += " "+src_words.at(i);
                }
                sample += " |||";
                for (int i=min_rule_righttop.tgt_span.first;i<=min_rule_righttop.tgt_span.first+min_rule_righttop.tgt_span.second;i++)
                {
                    sample += " "+tgt_words.at(i);
                }
                */
                samples.push_back(sample);
            }
            auto &lefttop_rules = alignpoint2quadrules[src_idx][tgt_idx].lefttop_rules;
            auto &rightbottom_rules = alignpoint2quadrules[src_idx][tgt_idx].rightbottom_rules;
            if (!lefttop_rules.empty() && !rightbottom_rules.empty())
            {
                Rule max_rule_lefttop = get_max_rule(lefttop_rules);
                Rule max_rule_rightbottom = get_max_rule(rightbottom_rules);
                vector<string> boundary_words_lefttop = get_boundary_words(max_rule_lefttop);
                vector<string> boundary_words_rightbottom = get_boundary_words(max_rule_rightbottom);
                string sample = "inverted c11="+boundary_words_lefttop.at(0)+" c21="+boundary_words_rightbottom.at(0)
                                       +" e11="+boundary_words_lefttop.at(1)+" e21="+boundary_words_rightbottom.at(1)
                                       +" c12="+boundary_words_lefttop.at(2)+" c22="+boundary_words_rightbottom.at(2)
                                       +" e12="+boundary_words_lefttop.at(3)+" e22="+boundary_words_rightbottom.at(3);
                /*
                string sample = "inverted |||";
                for (int i=max_rule_lefttop.src_span.first;i<=max_rule_lefttop.src_span.first+max_rule_lefttop.src_span.second;i++)
                {
                    sample += " "+src_words.at(i);
                }
                sample += " |||";
                for (int i=max_rule_lefttop.tgt_span.first;i<=max_rule_lefttop.tgt_span.first+max_rule_lefttop.tgt_span.second;i++)
                {
                    sample += " "+tgt_words.at(i);
                }
                sample += " |||";
                for (int i=max_rule_rightbottom.src_span.first;i<=max_rule_rightbottom.src_span.first+max_rule_rightbottom.src_span.second;i++)
                {
                    sample += " "+src_words.at(i);
                }
                sample += " |||";
                for (int i=max_rule_rightbottom.tgt_span.first;i<=max_rule_rightbottom.tgt_span.first+max_rule_rightbottom.tgt_span.second;i++)
                {
                    sample += " "+tgt_words.at(i);
                }
                */
                samples.push_back(sample);
            }
        }
    }
}

void SampleExtractor::fill_alignpoint2quadrules()
{
    //traverse src span
    for (int src_beg=0;src_beg<src_sen_len;src_beg++)
	{
        for (int src_len=0;src_beg+src_len<src_sen_len;src_len++)
		{
            if (src_len > 0 && span2head.at(src_beg).at(src_len) == -2)               //单节点视为well-formed结构
                continue;
            int src_end = src_beg + src_len;
            Span src_span = make_pair(src_beg,src_len);
			Span tgt_span = src_span_to_tgt_span[src_beg][src_len];
			if (tgt_span.first == -1)	  //如果目标端span为空，认为不满足对齐一致性
                continue;
            Span proj_src_span = tgt_span_to_src_span[tgt_span.first][tgt_span.second];
            //如果目标端span投射回来的源端span不超原来的源端span，则满足对齐一致性
            if (proj_src_span.first >= src_beg && proj_src_span.first+proj_src_span.second <= src_beg+src_len)
            {
                vector<Span> tgt_spans = expand_tgt_span(tgt_span,make_pair(0,tgt_sen_len-1));
                for (auto e_tgt_span : tgt_spans)
                {
                    int tgt_beg = e_tgt_span.first;
                    int tgt_end = e_tgt_span.first+e_tgt_span.second;
                    assert(alignpoint2quadrules.size() > src_beg);
                    assert(alignpoint2quadrules.size() > src_end+1);
                    assert(alignpoint2quadrules.at(src_end+1).size() > tgt_beg);
                    assert(alignpoint2quadrules.at(src_end+1).size() > tgt_end+1);
                    Rule rule = {src_span,e_tgt_span};
                    alignpoint2quadrules[src_beg][tgt_beg].righttop_rules.push_back(rule); 
                    alignpoint2quadrules[src_beg][tgt_end+1].rightbottom_rules.push_back(rule); 
                    alignpoint2quadrules[src_end+1][tgt_beg].lefttop_rules.push_back(rule); 
                    alignpoint2quadrules[src_end+1][tgt_end+1].leftbottom_rules.push_back(rule); 
                }
            }
		}
	}
}

Rule SampleExtractor::get_min_rule(vector<Rule> &rules)
{
    Rule min_rule = rules.front();
    for (auto &rule : rules)
    {
        if (rule.src_span.second < min_rule.src_span.second || rule.tgt_span.second < min_rule.tgt_span.second)
        {
            min_rule = rule;
        }
    }
    return min_rule;
}

Rule SampleExtractor::get_max_rule(vector<Rule> &rules)
{
    Rule max_rule = rules.front();
    for (auto &rule : rules)
    {
        if (rule.src_span.second > max_rule.src_span.second || rule.tgt_span.second > max_rule.tgt_span.second)
        {
            max_rule = rule;
        }
    }
    return max_rule;
}

vector<string> SampleExtractor::get_boundary_words(Rule &rule)
{
    vector<string> boundary_words;
    boundary_words.push_back(src_words.at(rule.src_span.first));
    boundary_words.push_back(tgt_words.at(rule.tgt_span.first));
    boundary_words.push_back(src_words.at(rule.src_span.first+rule.src_span.second));
    boundary_words.push_back(tgt_words.at(rule.tgt_span.first+rule.tgt_span.second));
    return boundary_words;
}

/**************************************************************************************
 1. 函数功能: 将两个span合并为一个span
 2. 入口参数: 被合并的两个span
 3. 出口参数: 合并后的span
 4. 算法简介: 见注释
************************************************************************************* */
Span SampleExtractor::merge_span(Span span1,Span span2)
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

int main(int argc, char* argv[])
{
	ifstream fs(argv[1]);
	ifstream ft(argv[2]);
	ifstream fa(argv[3]);

    string line_src,line_tgt,line_al;
    while(getline(fs,line_src) && getline(ft,line_tgt) && getline(fa,line_al))
    {
        SampleExtractor se(line_src,line_tgt,line_al);
        vector<string> samples;
        se.extract(samples);
        for (auto &sample : samples)
        {
            cout<<sample<<endl;
        }
    }
}
