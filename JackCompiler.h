#pragma once
#include <fstream>
#include <unordered_map>

class JackAnalyzer {	// take in directory as argument
	/*
		1. Create a JackTokenizer from the Xxx.jack input file.
		2. Create an output file called Xxx.xml and prepare it for writing.
		3. Use the CompilationEngine to compile the input JackTokenizer into the output file.
	*/
	enum KIND {
		STATIC,
		FIELD,
		ARG,
		VAR,
		NONE
	};
	struct details {
		std::string type;
		KIND kind;
		int index;
	};
	class JackTokenizer {
		/* Removes all comments and white space from the input stream
		and breaks it into Jack language tokens, as specified by the Jack grammar.
		*/
		std::string curTokn;
		std::string t;
		std::ifstream jackFile;
		std::string keyOrIdent();	// determines if token is a keyword or identifier
	public: 
		JackTokenizer(std::string filename);	// opens input file/stream & gets ready to tokenize it
		bool hasMoreTokens();	// more tokens in the input?
		void advance();	// grabs next token if hasMoreTokens() & determines its type; initially no current token
		std::string tokenType();	// returns type of current token
		// functions that are called depending on tokenType()
		std::string keyWord(); // returns keyword which is the current token; only called when tokenType() = KEYWORD
		char symbol();	// returns character which is current token; called when tokenType() = SYMBOL
		std::string identifier();	// returns identifier which is current token; tokenType = IDENTIFIER
		int intVal();	// returns int val of current token; tokenType() = INT_CONSTANT
		std::string stringVal();	// returns string value of current token; tokenType = STRING_CONSTANT

	};
	class SymbolTable {
		std::unordered_map<std::string, JackAnalyzer::details>* classScope;
		std::unordered_map<std::string, JackAnalyzer::details>* subScope;
	public:
		SymbolTable();	// creates new empty symbol table
		~SymbolTable();
		void startSubroutine();	// starts new subroutine scope (reset subroutine symbol table)
		/*
		*	Defines a new identifier of a given name, type, and kind
		*	Assigns it a running index. STATIC/FIELD have class scope
		*	ARG/VAR have subroutine scope
		*/
		void Define(std::string name, std::string type, KIND k);
		int VarCount(KIND k);	// returns num of variables of the given KIND defined in the current scope
		JackAnalyzer::KIND kindOf(std::string name);	// returns KIND of named identifier in current scope. if identifier is unknown returns NONE
		std::string TypeOf(std::string name);	// returns type of the named identifier in current scope
		int IndexOf(std::string name);	// returns index assigned to the named identifier
	};
	class CompilationEngine {
		class VMWriter {
			std::ofstream outFile;
		public:
			VMWriter(std::string vmFilename);
			void writePush(std::string segment, int index);
			void writePop(std::string segment, int index);
			void writeArithmetic(std::string command);
			void writeLabel(std::string label);
			void writeGoto(std::string label);
			void writeIf(std::string label);
			void writeCall(std::string name, int nArgs);
			void writeFunction(std::string name, int nLocals);
			void writeReturn();
			void close();
		};
	//	Output xml files to a new folder.
		JackTokenizer* T;
		VMWriter* vm;
		std::ofstream outFile;
		SymbolTable table;
		std::string className;
		int argCount;
		// extra utilty
		void writeType();	// deals w/ outputing the write code for type
		void compileSubroutineCall();
	public:
		CompilationEngine(JackTokenizer* T, std::string output);
		void CompileClass();	// compiles a complete class
		void CompileClassVarDec();	// compiles static/field declaration
		void CompileSubroutine();	// compiles a complete method, function, or constructor
		void compileParameterList();	// compiles a (possibly empty) parameter list; not incl. ()
		void compileVarDec();	// compiles a var declaration
		void compileStatements();	// compiles a sequence of statements, not incl. {}
		void compileDo();	// compiles a do statement
		void compileLet();	// compiles a let statement
		void compileWhile();	// compiles a while statement
		void compileReturn();	// compiles a return statement
		void compileIf();	// compiles if statement, possibly w/ else clause
		void compileExpression();	// compiles an expression
		void compileTerm();
		void compileExpressionList();	// compiles (possibly empty) comma-separated list of expressions
	};
	JackTokenizer* T;
	CompilationEngine* C;
public:
	JackAnalyzer(std::string input, std::string output);
	~JackAnalyzer();
};