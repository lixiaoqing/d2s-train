#include "stdafx.h"
#include "myutils.h"
#include "tree_str_pair.h"

int main(int argc, char* argv[])
{
	//ifstream ft(argv[1]);
	//ifstream fs(argv[2]);
	//ifstream fa(argv[3]);
	ifstream ft("toy.ch.dep");
	ifstream fs("toy.en");
	ifstream fa("toy.align");
	string line_tree,line_str,line_align;
	while(getline(ft,line_tree))
	{
		getline(fs,line_str);
		getline(fa,line_align);
		TreeStrPair *tspair = new TreeStrPair(line_tree,line_str,line_align);
		tspair->extract_rules(tspair->root_idx);
		tspair->dump_rules(tspair->root_idx);
	}
}
