
#ifndef AST_HDR
#define AST_HDR

/*
 * $Log: ast.h,v $
 * Revision 1.11  1996/03/20 17:02:42  mjrg
 * Added multiple arguments to calls.
 * Instrument pvm_send instead of pvm_recv to get tags.
 *
 * Revision 1.10  1995/08/24 15:03:45  hollings
 * AIX/SP-2 port (including option for split instruction/data heaps)
 * Tracing of rexec (correctly spawns a paradynd if needed)
 * Added rtinst function to read getrusage stats (can now be used in metrics)
 * Critical Path
 * Improved Error reporting in MDL sematic checks
 * Fixed MDL Function call statement
 * Fixed bugs in TK usage (strings passed where UID expected)
 *
 * Revision 1.9  1995/05/18  10:29:58  markc
 * Added new opcode DataAddr
 *
 * Revision 1.8  1995/03/10  19:29:13  hollings
 * Added code to include base tramp cost in first mini-tramp.
 *
 * Revision 1.7  1995/02/16  08:52:55  markc
 * Corrected error in comments -- I put a "star slash" in the comment.
 *
 * Revision 1.6  1995/02/16  08:32:50  markc
 * Changed igen interfaces to use strings/vectors rather than charigen-arrays
 * Changed igen interfaces to use bool, not Boolean.
 * Cleaned up symbol table parsing - favor properly labeled symbol table objects
 * Updated binary search for modules
 * Moved machine dependnent ptrace code to architecture specific files.
 * Moved machine dependent code out of class process.
 * Removed almost all compiler warnings.
 * Use "posix" like library to remove compiler warnings
 *
 * Revision 1.5  1994/11/02  11:00:57  markc
 * Replaced string-handles.
 *
 * Revision 1.4  1994/09/22  01:44:51  markc
 * Made first arg to AstNode constructor const
 * strdup'd callee in AstNode::AstNode, this is temporary
 * Made first arg to createPrimitiveCall const
 *
 */

//
// Define a AST class for use in generating primitive and pred calls
//
//

#include <stdio.h>
#include <strstream.h>
#include "paradynd/src/inst.h"

class dataReqNode;

typedef enum { sequenceNode, opCodeNode, operandNode, callNode } nodeType;

typedef enum { Constant, ConstantPtr, DataValue, DataPtr, Param, DataAddr } operandType;

// a register.
typedef int reg;

class registerSlot {
 public:
    int number;         // what register is it
    bool inUse;      	// free or in use.
    bool needsSaving;	// been used since last rest
    bool mustRestore;   // need to restore it before we are done.		
    bool startsLive;	// starts life as a live register.
};

class registerSpace {
    public:
	registerSpace(int dCount, int *deads, int lCount, int *lives);
	reg allocateRegister(char *insn, unsigned &base);
	void freeRegister(int reg);
	void resetSpace();
	bool isFreeRegister(reg reg_number);
	int getRegisterCount() { return numRegisters; }
	registerSlot *getRegSlot(int i) { return (&registers[i]); }
	bool readOnlyRegister(reg reg_number);
    private:
	int numRegisters;
	int highWaterRegister;
	registerSlot *registers;
};

class AstNode;

class AstNode {
    public:
	AstNode(const string func, AstNode *l, AstNode *r) {
	    loperand = l;
	    roperand = r;
	    if (func == "setCounter") {
		type = opCodeNode;
		op = storeOp;
            } else if (func == "addCounter") {
		type = opCodeNode;
		roperand = new AstNode(plusOp, l, r);
		op = storeOp;
	    } else if (func == "subCounter") {
		type = opCodeNode;
		roperand = new AstNode(minusOp, l, r);
		op = storeOp;
	    } else {
		type = callNode;
		callee = func;
		if (l)
		  operands += l;
		if (r)
		  operands += r;
	    }
	};

        AstNode(const string func, vector<AstNode *> ast_args) {
            if (func == "setCounter" || func == "addCounter" 
           	|| func == "subCounter") {
                loperand = ast_args[0];
                roperand = ast_args[1];
            } else {
                operands = ast_args;
            }
            if (func == "setCounter") {
                type = opCodeNode;
                op = storeOp;
            } else if (func == "addCounter") {
                type = opCodeNode;
                roperand = new AstNode(plusOp, loperand, roperand);
                op = storeOp;
            } else if (func == "subCounter") {
                type = opCodeNode;
                roperand = new AstNode(minusOp, loperand, roperand);
                op = storeOp;
            } else {
                type = callNode;
                callee = func;
            }
        };
	AstNode(operandType ot, void *arg) {
	    type = operandNode;
	    oType = ot;
	    if ((oType == DataPtr) || (oType == DataValue)) {
		dValue = (dataReqNode *) arg;
	    } else {
		oValue = (void *) arg;
	    }

	    loperand = NULL;
	    roperand = NULL;
	};
	AstNode(AstNode *l, AstNode *r) {
	    type = sequenceNode;
	    loperand = l;
	    roperand = r;
	};
	AstNode(opCode ot, AstNode *l, AstNode *r) {
	    type = opCodeNode;
	    op = ot;
	    loperand = l;
	    roperand = r;
	};
	int generateTramp(process *proc, char *i, unsigned &base,
			  int baseTrampCost);
	reg generateCode(process *proc, registerSpace *rs, char *i, 
			 unsigned &base);
	int cost();	// return the # of instruction times in the ast.
	void print();
    private:
	nodeType type;
	opCode op;		// for opCode ndoes
	string callee;		// for call ndoes
	operandType oType;	// for operand nodes
	void *oValue;		// for operand nodes
	dataReqNode *dValue;	// for operand nodes
	AstNode *loperand;
	AstNode *roperand;
	vector<AstNode *> operands;
	int firstInsn;
	int lastInsn;
};

AstNode *createPrimitiveCall(const string func, dataReqNode*, int param2);
AstNode *createIf(AstNode *expression, AstNode *action);
AstNode *createCall(const string func, dataReqNode *, AstNode *arg);

#endif
