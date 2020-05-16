#include "JackCompiler.h"

using namespace std;

int main() {
	string filename;
	// seven test
	// filename = "D:\\nand2tetris\\nand2tetris\\projects\\11\\Seven\\Main.jack";
	
	// ConvertToBin test
	filename = "D:\\nand2tetris\\nand2tetris\\projects\\11\\ConvertToBin\\Main.jack";

	JackAnalyzer J(filename, "out.xml");

	return 0;
}