#include "stdafx.h"
#include "vocab.h"

struct TgtRule
{
	bool operator<(const TgtRule &rhs) const{return score<rhs.score;};
	int word_num;                               // 规则目标端的终结符（单词）数
	vector<int> wids;                           // 规则目标端的符号（包括终结符和非终结符）id序列
	double score;                               // 规则打分, 即翻译概率与词汇权重的加权
	vector<double> probs;                       // 翻译概率和词汇权重
	int nt_num;									// 非终结符个数
	vector<int> tgt_nt_idx_to_src_nt_idx;		// 记录每个目标端非终结符对应于源端的第几个非终结符
};

struct RuleTrieNode 
{
	vector<TgtRule> tgt_rules;                  // 一个规则源端对应的所有目标端
	map <int, RuleTrieNode*> id2subtrie_map;    // 当前规则节点到下个规则节点的转换表
};

class RuleTable
{
	public:
		RuleTable(const size_t size_limit,const string &rule_table_file,Vocab *svocab,Vocab *tvocab)
		{
            src_vocab = svocab;
            tgt_vocab = tvocab;
			RULE_NUM_LIMIT=size_limit;
			root=new RuleTrieNode;
			load_rule_table(rule_table_file);
		};

	private:
		void load_rule_table(const string &rule_table_file);

	private:
		int RULE_NUM_LIMIT;                      // 每个规则源端最多加载的目标端个数 
		RuleTrieNode *root;                      // 规则Trie树根节点
        Vocab *src_vocab;
        Vocab *tgt_vocab;
};
