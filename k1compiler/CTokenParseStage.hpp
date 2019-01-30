#pragma once

#include "CWordScanStage.hpp"
#include <unordered_map>

namespace k1
{
	class CTokenParseOut : public IPipelineData
	{
	public:
		class CNode
		{
		public:
			enum class E_NODE_TYPE
			{
				UNDEFINED,
				PROG,
				PROC,
				MAIN,

				EXITLOOP,
				LOOP,
				CASE,

				PRINT,
				INPUT,
				ASSIGN,
				INTOP,
				STROP,
				CALL,

				//PROMPT,
				CONSTINT,
				CONSTSTR,
				LOCALVAR,
				RESULTVAR,
				PARAM,
				
				OPERATOR,
				EXPRESSION,
				WHEN,
				OTHERWISE,
			};

			struct SNodeInfo
			{
				E_NODE_TYPE type;
				string value;
			};

		public:
			CNode *createChild(E_NODE_TYPE type = E_NODE_TYPE::UNDEFINED, const string &val = K1T(""));
			CNode *createSib(E_NODE_TYPE type = E_NODE_TYPE::UNDEFINED, const string &val = K1T(""));
			
			const CNode *getParent() const;
			CNode *getParent();
			
			const std::vector<CNode*> &getChilds() const;
			std::vector<CNode*> &getChilds();

			const CNode* getChild(std::vector<CNode*>::size_type) const;
			CNode* getChild(std::vector<CNode*>::size_type);

			SNodeInfo mData;
#ifdef _DEBUG
			void dump(int l=0) const;
#endif
		private:
			friend CTokenParseOut;

			CNode(CNode *parent);
			~CNode();

			CNode *mParent;
			std::vector<CNode*> mNodes;
		};

		enum class E_SYMBOL_TYPE
		{
			INTEGER,
			STRING,
		};

		enum class E_SYMBOL_ACCESS
		{
			RD,
			WR,
			RW,
		};

		struct SSymbol
		{
			SSymbol(const string&, const string&, E_SYMBOL_TYPE, E_SYMBOL_ACCESS);
			string name;
			string proc;
			E_SYMBOL_TYPE type;
			E_SYMBOL_ACCESS access;
		};

	public:
		CTokenParseOut();
		~CTokenParseOut();

		virtual std::uint8_t getType() const;

		const SSymbol &getSymbol(const string &s) const;
		SSymbol &getSymbol(const string &s);

		inline bool hasDeclared(const string &s) const;
		inline bool hasParam(const string &s) const;
		inline bool hasSymbol(const string &s) const;
		inline bool isValidSymbol(const string& s, E_SYMBOL_ACCESS access) const;

		CNode *mRoot;

		std::unordered_multimap<string, SSymbol> mParams;
		std::vector<SSymbol> mOrderedParams;
		std::unordered_multimap<string, SSymbol> mDeclarations;
		std::vector<SSymbol> mOrderedDeclarations;;
	};



	class CTokenParseStage : public IPipelineStage
	{
	public:
		CTokenParseStage(IPipelineStage *prev);
		~CTokenParseStage();

		virtual void execute();

	protected:
		typedef CWordScanOut::E_TOKEN_TYPE TokenType;
		typedef CTokenParseOut::CNode::E_NODE_TYPE NodeType;
		typedef CTokenParseOut::E_SYMBOL_TYPE SymType;
		typedef CTokenParseOut::E_SYMBOL_ACCESS SymAccess;

		void tok_program();
		void tok_main();
		void tok_procedure();
		inline string tok_procName();
		void tok_parameter();
		void tok_pVar();
		void tok_declaration();
		void tok_var();
		void tok_controlflow();
		void tok_statement();

		void tok_printStmt();
		void tok_inputStmt();
		void tok_assignStmt();

		inline string tok_localVar();

		void tok_integerOp();
		inline string tok_integerOperator();
		void tok_stringOp();
		inline string tok_stringOperator();
		inline string tok_resultVar();

		void tok_callStmt();
		void tok_actualParameter();
		void tok_loop();
		void tok_case();
		void tok_when();
		void tok_otherwise();

		void tok_expression();
		inline string tok_logOperator();

		void tok_operand();

	private:
		inline bool curHasVal(const string &s) const;

		string mCurNamespace;

		CTokenParseOut::CNode *mCurNode;
		CTokenParseOut *mParseOut;

		std::vector<CWordScanOut::STokenInfo>::const_iterator mCurToken;
	};
}