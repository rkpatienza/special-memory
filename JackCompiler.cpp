#include "JackCompiler.h"
#include <fstream>
#include <string>
#include <filesystem>
#include <algorithm>
#include <iostream>

using namespace std;

/*JACK ANALYZER FUNCTIONS*/
JackAnalyzer::JackAnalyzer(string input, string output) {
	T = new JackTokenizer(input);
	string name = input.substr(0, input.length() - 5);
	C = new CompilationEngine(T, name);
}

JackAnalyzer::~JackAnalyzer() {
	delete T;
	delete C;
}

/*JACK TOKENIZER FUNCTIONS*/
JackAnalyzer::JackTokenizer::JackTokenizer(string filename) {
	jackFile.open(filename);
	curTokn = t = "";
}

bool JackAnalyzer::JackTokenizer::hasMoreTokens() {	// just checks for eof?
	char c = jackFile.peek();
	return !(c == EOF);
}

void JackAnalyzer::JackTokenizer::advance() {
	char c;
	// check if more tokens
	if (!hasMoreTokens())
		return;

	// clear curTokn
	curTokn.clear();
	// get first character
	jackFile.get(c);
	while (c == ' ' || c == '\t' || c == '\n')	// ignore whitespace, tab, newline
		jackFile.get(c);
	curTokn.push_back(c);

	if (isdigit(c)) {/* tokenize int_const */
		t = "int_const";
		while (isdigit(jackFile.peek())) {
			jackFile.get(c);
			curTokn.push_back(c);
		}
	}
	else if (isalpha(c) || c == '_') {		/* tokenize keyword or identifier */
		while (isalpha(jackFile.peek()) || jackFile.peek() == '_' || isdigit(jackFile.peek())) {
			jackFile.get(c);
			curTokn.push_back(c);
		}
		t = keyOrIdent();
	}
	else if (c == '\"') {	/* tokenize string constant */
		t = "string_constant";
		while (jackFile.peek() != '\"') {
			jackFile.get(c);
			curTokn.push_back(c);
		}
		jackFile.get(c);	// consume ending double quote
		curTokn = curTokn.substr(1, curTokn.length() - 1);
	}
	/* watching out for comments */
	else if (c == '/' && jackFile.peek() == '/') {
		curTokn.clear();
		while (jackFile.peek() != '\n')	// go to before end of line char
			jackFile.get(c);
		this->advance();
	}
	else if (c == '/' && jackFile.peek() == '*') {
		curTokn.clear();
		jackFile.get(c);	// consume *
		if (jackFile.peek() == '*')
			jackFile.get(c);	// consume extra * for API comment
		jackFile.get(c);	// go to next character
		while (c != '*') {	// go before closing forward slash
			jackFile.get(c);
			if (jackFile.peek() == '*') {
				jackFile.get(c);
				if (jackFile.peek() != '/')
					jackFile.get(c);
			}
		}
		jackFile.get(c);	// consume closing slash
		this->advance();
	}
	else if (curTokn.find_first_of("{}()[].,;+-*/&|<>=~") != string::npos)	// symbol
		t = "symbol";
}

string JackAnalyzer::JackTokenizer::keyOrIdent() {
	if (curTokn == "class" || curTokn == "constructor" || curTokn == "function" ||
		curTokn == "method" || curTokn == "field" || curTokn == "static" || curTokn == "var" ||
		curTokn == "int" || curTokn == "char" || curTokn == "boolean" || curTokn == "void" || curTokn == "true" ||
		curTokn == "false" || curTokn == "null" || curTokn == "this" || curTokn == "let" || curTokn == "do" ||
		curTokn == "if" || curTokn == "else" || curTokn == "while" || curTokn == "return")
		return "keyword";
	else
		return "identifier";
}

string JackAnalyzer::JackTokenizer::tokenType() {
	return t;
}

std::string JackAnalyzer::JackTokenizer::keyWord() {
	return curTokn;
}

char JackAnalyzer::JackTokenizer::symbol() {
	return curTokn[0];
}

std::string JackAnalyzer::JackTokenizer::identifier() {
	return curTokn;
}

int JackAnalyzer::JackTokenizer::intVal() {
	return stoi(curTokn);
}

std::string JackAnalyzer::JackTokenizer::stringVal() {
	return curTokn;
}

/*COMPILATION ENGINE FUNCTIONS*/
/*
1. read token from tokenizer
2. advance tokenizer
3. output parsing of token to xml file
*/
// integrate symbol tables into compilation engine
// then, use compilation engine to send commands to VMWriter to produce final vm code

JackAnalyzer::CompilationEngine::CompilationEngine(JackTokenizer* T, string output) {
	this->T = T;
	argCount = 0;
	outFile.open(output + ".xml");
	vm = new VMWriter(output + ".vm");
	while (T->tokenType() != "keyword" && T->keyWord() != "class")
		T->advance();
	outFile << "<class>\n";
	CompileClass();
	outFile << "</class>\n";
}

void JackAnalyzer::CompilationEngine::CompileClass() {
//	outFile << '<' + T->tokenType() + "> " + T->keyWord() + " </" + T->tokenType() + ">\n";
	T->advance();
//	outFile << '<' + T->tokenType() + "> " + T->identifier() + " </" + T->tokenType() + ">\n";
	className = T->identifier();
	T->advance();
//	outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
	// classVarDec call
	T->advance();
	while (T->tokenType() == "keyword" && (T->keyWord() == "static" ||T->keyWord()== "field"))
		CompileClassVarDec();
	// subroutineDec call
	while (T->tokenType() == "keyword" && 
		(T->keyWord() == "constructor" ||T->keyWord()== "function" || T->keyWord() == "method"))
		CompileSubroutine();
//	outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
}

void JackAnalyzer::CompilationEngine::CompileClassVarDec() {
	string name, type;
	JackAnalyzer::KIND kind;

	if (T->keyWord() == "static")
		kind = JackAnalyzer::STATIC;
	else
		kind = JackAnalyzer::FIELD;
//	outFile << '<' + T->tokenType() + "> " + T->keyWord() + " </" + T->tokenType() + ">\n";
	T->advance();

	type = T->keyWord();
	writeType();

	name = T->identifier();
	table.Define(name, type, kind);		// add to symbol table

//	outFile << '<' + T->tokenType() + "> " + T->identifier() + " </" + T->tokenType() + ">\n";
	T->advance();
	while (T->tokenType() == "symbol" && T->symbol() == ',') {
//		outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
		T->advance();
		name = T->identifier();
		table.Define(name, type, kind);		// add to symbol table
//		outFile << '<' + T->tokenType() + "> " + T->identifier() + " </" + T->tokenType() + ">\n";
		T->advance();
	}
//	outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
	T->advance();
}

void JackAnalyzer::CompilationEngine::CompileSubroutine() {
	string functionName;
	table.startSubroutine();	// clear subroutine table
	// add this 0 to symbol table if function is a method
	if(T->keyWord() == "method")
		table.Define("this", className, JackAnalyzer::ARG);

//	outFile << '<' + T->tokenType() + "> " + T->keyWord() + " </" + T->tokenType() + ">\n";
	T->advance();
	writeType();
//	outFile << '<' + T->tokenType() + "> " + T->identifier() + " </" + T->tokenType() + ">\n";
	functionName = className + '.' + T->identifier();
	T->advance();
//	outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
	T->advance();
	
	// parameter list
	if (T->tokenType() == "keyword")
		compileParameterList();
//	outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
	T->advance();

	//subroutine body
//	outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
	T->advance();
	// varDec*
	while (T->tokenType() == "keyword" && T->keyWord() == "var")
		compileVarDec();
	// write vmFunction call
	vm->writeFunction(functionName, table.VarCount(VAR));
	// compile statements
	compileStatements();
//	outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
	T->advance();
}

void JackAnalyzer::CompilationEngine::compileParameterList() {
	JackAnalyzer::KIND k = ARG;
	string name, type;
	type = T->keyWord();
	writeType();
	name = T->identifier();
	table.Define(name, type, k);
//	outFile << '<' + T->tokenType() + "> " + T->identifier() + " </" + T->tokenType() + ">\n";
	T->advance();
	while (T->tokenType() == "symbol" && T->symbol() == ',') {
//		outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
		T->advance();
		type = T->keyWord();
		writeType();
		name = T->identifier();
		table.Define(name, type, k);
//		outFile << '<' + T->tokenType() + "> " + T->identifier() + " </" + T->tokenType() + ">\n";
		T->advance();
	}
}

void JackAnalyzer::CompilationEngine::compileVarDec() {
	JackAnalyzer::KIND k = VAR;
	string name, type;
//	outFile << '<' + T->tokenType() + "> " + T->keyWord() + " </" + T->tokenType() + ">\n";
	T->advance();
	type = T->keyWord();
	writeType();
	name = T->identifier();
	table.Define(name, type, k);

//	outFile << '<' + T->tokenType() + "> " + T->identifier() + " </" + T->tokenType() + ">\n";
	T->advance();
	while (T->tokenType() == "symbol" && T->symbol() == ',') {
//		outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
		T->advance();
//		outFile << '<' + T->tokenType() + "> " + T->identifier() + " </" + T->tokenType() + ">\n";
		name = T->identifier();
		table.Define(name, type, k);
		T->advance();
	}
//	outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
	T->advance();
}

void JackAnalyzer::CompilationEngine::compileStatements() {
	// check if there are any statements
	while (T->tokenType() == "keyword") {
		if (T->keyWord() == "let")
			compileLet();
		else if (T->keyWord() == "if")
			compileIf();
		else if (T->keyWord() == "while")
			compileWhile();
		else if (T->keyWord() == "do")
			compileDo();
		else if (T->keyWord() == "return")
			compileReturn();
	}
}

void JackAnalyzer::CompilationEngine::compileLet() {
	// for symbol table
	string type, segment;
	int index;
//	outFile << '<' + T->tokenType() + "> " + T->keyWord() + " </" + T->tokenType() + ">\n";
	T->advance();
	// get details about symbol from symbol table?
	if (table.kindOf(T->identifier()) != NONE) {
		if (table.kindOf(T->identifier()) == STATIC)
			segment = "STATIC";
		else if (table.kindOf(T->identifier()) == FIELD)
			segment = "THIS";
		else if (table.kindOf(T->identifier()) == VAR)
			segment = "LOCAL";
		else
			segment = "ARG";
		type = table.TypeOf(T->identifier());
		index = table.IndexOf(T->identifier());
	}

//	outFile << '<' + T->tokenType() + "> " + T->identifier() + " </" + T->tokenType() + ">\n";
	T->advance();
	// if variable is an array
	if (T->tokenType() == "symbol" && T->symbol() == '[') {
//		outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
		T->advance();
		compileExpression();
//		outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
		T->advance();
	}
//	outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
	T->advance();
	compileExpression();
	cout << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
//	outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
	T->advance();
}

void JackAnalyzer::CompilationEngine::compileIf() {
//	outFile << '<' + T->tokenType() + "> " + T->keyWord() + " </" + T->tokenType() + ">\n";
	T->advance();
//	outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
	T->advance();
	compileExpression();
//	outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
	T->advance();
//	outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
	T->advance();
	compileStatements();
//	outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
	T->advance();
	if (T->tokenType() == "keyword" && T->keyWord() == "else") {
//		outFile << '<' + T->tokenType() + "> " + T->keyWord() + " </" + T->tokenType() + ">\n";
		T->advance();
//		outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
		T->advance();
		compileStatements();
//		outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
		T->advance();
	}
}

void JackAnalyzer::CompilationEngine::compileWhile() {
//	outFile << '<' + T->tokenType() + "> " + T->keyWord() + " </" + T->tokenType() + ">\n";
	T->advance();
//	outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
	T->advance();
	compileExpression();
//	outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
	T->advance();
//	outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
	T->advance();
	compileStatements();
//	outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
	T->advance();
}

void JackAnalyzer::CompilationEngine::compileDo() {
//	outFile << '<' + T->tokenType() + "> " + T->keyWord() + " </" + T->tokenType() + ">\n";
	T->advance();
	compileSubroutineCall();
	cout << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
	// outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
	T->advance();
}

void JackAnalyzer::CompilationEngine::compileReturn() {
//	outFile << '<' + T->tokenType() + "> " + T->keyWord() + " </" + T->tokenType() + ">\n";
	T->advance();
	if (T->tokenType() == "symbol" && T->symbol() == ';') {
//		outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
		T->advance();
	}
	else {
		compileExpression();
//		outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
		T->advance();
	}
	vm->writeReturn();
}

void JackAnalyzer::CompilationEngine::compileExpression() {
	string command;
	compileTerm();
	while (T->tokenType() == "symbol" &&
		(T->symbol() == '+' || T->symbol() == '-' || T->symbol() == '*' || T->symbol() == '/' ||
		T->symbol() == '&' || T->symbol() == '|' || T->symbol() == '<' || T->symbol() == '>' ||
		T->symbol() == '=')) {
		if (T->symbol() == '+')
			command = "add";
		else if (T->symbol() == '-')
			command = "sub";
		else if (T->symbol() == '*')
			command = "call Math.multiply 2";
		else if (T->symbol() == '/')
			command = "call Math.divide 2";
		else if (T->symbol() == '&')
			command = "and";
		else if (T->symbol() == '|')
			command = "or";
		else if (T->symbol() == '<')
			command = "lt";
		else if (T->symbol() == '>')
			command = "gt";
		else if (T->symbol() == '=')
			command = "eq";

//		outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
		T->advance();
		compileTerm();
		vm->writeArithmetic(command);
	}
}

void JackAnalyzer::CompilationEngine::compileTerm() {
	string segment, type;
	int index;
	if (T->tokenType() == "int_const") {
//		outFile << '<' + T->tokenType() + "> " + to_string(T->intVal()) + " </" + T->tokenType() + ">\n";
		vm->writePush("constant", T->intVal());
		T->advance();
	}
	else if (T->tokenType() == "string_const") {
//		outFile << '<' + T->tokenType() + "> " + T->stringVal() + " </" + T->tokenType() + ">\n";
		// push string length
		vm->writePush("constant", T->stringVal().length());
		// call String constructor
		vm->writeCall("String.new", 1);
		// append all characters to new string
		for (int i = 0; i < T->stringVal().length(); i++) {
			vm->writePush("constant", (int)T->stringVal()[i]);
			vm->writeCall("String.appendChar", 1);
		}
		T->advance();
	}
	else if (T->tokenType() == "keyword") {	// true, false, null, this
		if (T->keyWord() == "true") {
			vm->writePush("constant", 1);
			vm->writeArithmetic("neg");
		}
		else if (T->keyWord() == "false" || T->keyWord() == "null")
			vm->writePush("constant", 0);
		else
			vm->writePush("argument", 0); // not sure about dealing w/ the this keyword?
//		outFile << '<' + T->tokenType() + "> " + T->keyWord() + " </" + T->tokenType() + ">\n";
		T->advance();
	}
	else if (T->tokenType() == "identifier") {
		// get details about symbol from symbol table?
		if (table.kindOf(T->identifier()) != NONE) {
			if (table.kindOf(T->identifier()) == STATIC)
				segment = "static";
			else if (table.kindOf(T->identifier()) == FIELD)
				segment = "this";
			else if (table.kindOf(T->identifier()) == VAR)
				segment = "local";
			else
				segment = "argument";
			type = table.TypeOf(T->identifier());
			index = table.IndexOf(T->identifier());
			vm->writePush(segment, index);
		}
//		outFile << '<' + T->tokenType() + "> " + T->identifier() + " </" + T->tokenType() + ">\n";
		T->advance();
		if (T->tokenType() == "symbol" && T->symbol() == '[') {
//			outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
			T->advance();
			compileExpression();
//			outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
			T->advance();
		}
		else if (T->tokenType() == "symbol" && T->symbol() == '(') {
			compileSubroutineCall();
		}
	}
	else if (T->tokenType() == "symbol") {
		if (T->symbol() == '(') {
//			outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
			T->advance();
			compileExpression();
//			outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
			T->advance();
		}
		else if (T->symbol() == '-' || T->symbol() == '~') {
//			outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
			T->advance();
			compileTerm();
		}
	}
}

void JackAnalyzer::CompilationEngine::compileExpressionList() {
	if (T->tokenType() == "symbol" && T->symbol() == ')')
		return;
	compileExpression();
	argCount++;
	T->advance();
	while (T->tokenType() == "symbol" && T->symbol() == ',') {
//		outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
		T->advance();
		compileExpression();
		argCount++;
	}
}

void JackAnalyzer::CompilationEngine::compileSubroutineCall() {
	string subroutineName = T->identifier();
//	outFile << '<' + T->tokenType() + "> " + T->identifier() + " </" + T->tokenType() + ">\n";
	T->advance();
	if (T->symbol() == '(') {
//		outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
		T->advance();
		compileExpressionList();
	}
	else {
		subroutineName += T->symbol();
//		outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
		T->advance();
		subroutineName += T->identifier();
//		outFile << '<' + T->tokenType() + "> " + T->identifier() + " </" + T->tokenType() + ">\n";
		T->advance();
//		outFile << '<' + T->tokenType() + "> " + T->symbol() + " </" + T->tokenType() + ">\n";
		T->advance();
		compileExpressionList();
	}
	vm->writeCall(subroutineName, argCount);
	argCount = 0;
}

void JackAnalyzer::CompilationEngine::writeType() {
	string type;	// refers to "int, char, boolean, identifier"
	// determine type
	if (T->tokenType() == "keyword")
		type = T->keyWord();
	else
		type = T->identifier();
//	outFile << '<' + T->tokenType() + "> " + type + " </" + T->tokenType() + ">\n";
	T->advance();
}

/* VMWRITER FUNCTIONS */
JackAnalyzer::CompilationEngine::VMWriter::VMWriter(string vmFilename) {
	outFile.open(vmFilename);
}

void JackAnalyzer::CompilationEngine::VMWriter::writePush(string segment, int index) {
	outFile << "push " << segment << ' ' << index << '\n';
}

void JackAnalyzer::CompilationEngine::VMWriter::writePop(string segment, int index) {
	outFile << "pop " << segment << ' ' << index << '\n';
}

void JackAnalyzer::CompilationEngine::VMWriter::writeArithmetic(string command) {
	// expects command to be one of the 9 ops (e.g. add, sub, neg, not, gt, etc)
	outFile << command << endl;
}

void JackAnalyzer::CompilationEngine::VMWriter::writeLabel(string label) {
	outFile << "label " << label << '\n';
}

void JackAnalyzer::CompilationEngine::VMWriter::writeGoto(string label) {
	outFile << "goto " << label << '\n';
}

void JackAnalyzer::CompilationEngine::VMWriter::writeIf(string label) {
	outFile << "if-goto " << label << '\n';
}

void JackAnalyzer::CompilationEngine::VMWriter::writeCall(string name, int nArgs) {
	outFile << "call " << name << ' ' << nArgs << '\n';
}

void JackAnalyzer::CompilationEngine::VMWriter::writeFunction(string name, int nLocals) {
	outFile << "function " << name << ' ' << nLocals << '\n';
}

void JackAnalyzer::CompilationEngine::VMWriter::writeReturn() {
	outFile << "return\n";
}

void JackAnalyzer::CompilationEngine::VMWriter::close() {
	outFile.close();
}

/* SYMBOL TABLE FUNCTIONS */
JackAnalyzer::SymbolTable::SymbolTable() {
	classScope = new unordered_map<string, JackAnalyzer::details>;
	subScope = new unordered_map<string, JackAnalyzer::details>;
}

JackAnalyzer::SymbolTable::~SymbolTable() {
	delete classScope;
	delete subScope;
}

void JackAnalyzer::SymbolTable::startSubroutine() {
	subScope->clear();
}

void JackAnalyzer::SymbolTable::Define(string name, string type, KIND k) {
	int index;
	details S;
	// generate index
	index = VarCount(k);
	S.index = index;
	S.type = type;
	S.kind = k;
	if(k == JackAnalyzer::STATIC || k == JackAnalyzer::FIELD)
		classScope->insert(pair<string, details>(name, S));
	else
		subScope->insert(pair<string, details>(name, S));
}

int JackAnalyzer::SymbolTable::VarCount(KIND k) {
	int count = 0;
	unordered_map<string, details>::iterator it;
	if (k == JackAnalyzer::STATIC || k == JackAnalyzer::FIELD) {
		it = classScope->begin();
		while (it != classScope->end())
		{
			if (it->second.kind == k)
				count++;
			++it;
		}
	}
	else {
		it = subScope->begin();
		while (it != subScope->end())
		{
			if (it->second.kind == k)
				count++;
			++it;
		}
	}
	return count;
}

JackAnalyzer::KIND JackAnalyzer::SymbolTable::kindOf(string name) {
	if (classScope->find(name) == classScope->end())
		return subScope->find(name)->second.kind;
	else if (subScope->find(name) == subScope->end())
		return NONE;
	else
		return classScope->find(name)->second.kind;
}

string JackAnalyzer::SymbolTable::TypeOf(string name) {
	if (classScope->find(name) == classScope->end())
		return subScope->find(name)->second.type;
	else
		return classScope->find(name)->second.type;
}

int JackAnalyzer::SymbolTable::IndexOf(string name) {
	if (classScope->find(name) == classScope->end())
		return subScope->find(name)->second.index;
	else
		return classScope->find(name)->second.index;
}
