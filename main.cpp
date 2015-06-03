#include "stdafx.h"
#include "myutils.h"
#include "rule_extractor.h"
#include "rule_counter.h"

void load_lex_trans_table(map<string,double> &lex_trans_table,string lex_trans_file)
{
	ifstream fin(lex_trans_file.c_str());
    string line;
    while(getline(fin,line))
    {
        vector<string> vs = Split(line);
        lex_trans_table[vs[0]+" "+vs[1]] = stod(vs[2]);
    }
};

int main(int argc, char* argv[])
{
	ifstream ft(argv[1]);
	ifstream fs(argv[2]);
	ifstream fa(argv[3]);
	map<string,double> lex_s2t;
	map<string,double> lex_t2s;
    load_lex_trans_table(lex_s2t,argv[4]);
    load_lex_trans_table(lex_t2s,argv[5]);
    RuleCounter rule_counter;
	string line_tree,line_str,line_align;
	while(getline(ft,line_tree))
	{
		getline(fs,line_str);
		getline(fa,line_align);
		//cerr<<line_str<<endl;
		RuleExtractor rule_extractor(line_tree,line_str,line_align,&lex_s2t,&lex_t2s,&rule_counter);
		rule_extractor.extract_rules();
	}
    rule_counter.dump_rules();
}
