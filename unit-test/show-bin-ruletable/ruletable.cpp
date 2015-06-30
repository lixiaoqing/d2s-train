#include "ruletable.h"

void RuleTable::load_rule_table(const string &rule_table_file)
{
	ifstream fin(rule_table_file.c_str(),ios::binary);
	if (!fin.is_open())
	{
		cerr<<"cannot open rule table file!\n";
		return;
	}
	int src_rule_len=0;
	while(fin.read((char*)&src_rule_len,sizeof(int)))
	{
		vector<int> src_wids;
		src_wids.resize(src_rule_len);
		fin.read((char*)&src_wids[0],sizeof(int)*src_rule_len);
        for (auto wid :src_wids)
            cout<<src_vocab->get_word(wid)<<' ';
        cout<<"||| ";

		int tgt_rule_len=0;
		fin.read((char*)&tgt_rule_len,sizeof(int));
		TgtRule tgt_rule;
		tgt_rule.wids.resize(tgt_rule_len);
		fin.read((char*)&(tgt_rule.wids[0]),sizeof(int)*tgt_rule_len);
        for (auto wid :tgt_rule.wids)
            cout<<tgt_vocab->get_word(wid)<<' ';
        cout<<"||| ";

		int nt_num;
		fin.read((char*)&nt_num,sizeof(int));
        cout<<nt_num<<' ';
		tgt_rule.nt_num = nt_num;
		tgt_rule.word_num = tgt_rule_len-nt_num;
		if (nt_num > 0)
		{
			tgt_rule.tgt_nt_idx_to_src_nt_idx.resize(nt_num);
			fin.read((char*)&(tgt_rule.tgt_nt_idx_to_src_nt_idx[0]),sizeof(int)*nt_num);
            for (int src_nt_idx : tgt_rule.tgt_nt_idx_to_src_nt_idx)
            {
                cout<<src_nt_idx<<' ';
            }
		}
        cout<<"||| ";

		tgt_rule.probs.resize(PROB_NUM);
		fin.read((char*)&(tgt_rule.probs[0]),sizeof(double)*PROB_NUM);
        for (auto p : tgt_rule.probs)
            cout<<p<<' ';
        cout<<endl;

        int rule_type_id;
		fin.read((char*)&rule_type_id,sizeof(int));
	}
	fin.close();
	cout<<"load rule table file "<<rule_table_file<<" over\n";
}

int main( int argc, char *argv[])
{
	Vocab *src_vocab = new Vocab("vocab.ch");
	Vocab *tgt_vocab = new Vocab("vocab.en");
	RuleTable *ruletable = new RuleTable(100,"prob.bin",src_vocab,tgt_vocab);
	return 0;
}
