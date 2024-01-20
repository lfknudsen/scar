#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

int main(int argc, char* argv[])
{
	string filename = "program_text.txt";
	if (argc > 1)
	{
		vector<string> arguments;
		arguments.assign(argv + 1, argv + argc);
		filename = arguments[0];
	}

	ifstream program_file (filename);
	if (!program_file.is_open())
	{
		cout << "Could not open file.";
	}

	vector<string> reserved = { ";", "=", "+", "-" };
	vector<string, int> vtable;
	vector<string> lines;
	string line;
	while (getline(program_file, line))
	{
		lines.push_back(line);
	}
	cout << lines[0];
	program_file.close();
	
	return 0;
}
