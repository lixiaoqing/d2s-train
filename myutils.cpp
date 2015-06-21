#include "myutils.h"

vector<string> Split(const string &s)
{
	vector <string> vs;
	stringstream ss(s);
	string e;
	while(ss >> e)
		vs.push_back(e);
	return vs;
}

vector<string> Split(const string &s, const string &sep)
{
	vector <string> vs;
	int cur = 0,next;
	next = s.find(sep);
	while(next != string::npos)
	{
		if(s.substr(cur,next-cur) !="")
			vs.push_back(s.substr(cur,next-cur));
		cur = next+sep.size();
		next = s.find(sep,cur);
	}
	vs.push_back(s.substr(cur));
	return vs;
}

void TrimLine(string &line)
{
	line.erase(0,line.find_first_not_of(" \t\r\n"));
	line.erase(line.find_last_not_of(" \t\r\n")+1);
}

void print_vector(vector<int> &v)
{
	for (auto e : v)
		cout<<e<<" ";
	cout<<endl;
}

void load_data_into_blocks(vector<vector<string> > &data_blocks, ifstream &fin,int block_size)
{
	vector<string> block;
	int num = 0;
	string line;
	while(getline(fin,line))
	{
		block.push_back(line);
		num++;
		if (num%block_size == 0)
		{
			data_blocks.push_back(block);
			block.clear();
		}
	}
	if (block.size() > 0)
	{
		data_blocks.push_back(block);
	}
}

double d2log(double prob)
{
    double log_prob = 0.0;
    if( abs(prob) <= numeric_limits<double>::epsilon() )
    {
        log_prob = -99;
    }
    else
    {
        log_prob = log10(prob);
    }
    return log_prob;
}
