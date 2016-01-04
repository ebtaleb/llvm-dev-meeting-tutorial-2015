#define DEBUG_TYPE "xor"
#include "llvm/Support/Debug.h"

#include "llvm/ADT/Statistic.h"
STATISTIC(XORCount, "The # of substituted instructions");

#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;

namespace {

    class XOR : public BasicBlockPass {

        public:
            static char ID;
            XOR() : BasicBlockPass(ID) {}

            bool doInitialization(Module &M) override {
                return false;
            }

            bool runOnBasicBlock(BasicBlock &BB) override {

                bool modified = false;

                for (auto IIT = BB.begin(), IE = BB.end(); IIT != IE; ++IIT) {
                    Instruction &Inst = *IIT;

                    if (isValidCandidateInstruction(Inst)) {
                        for (size_t i = 0; i < Inst.getNumOperands(); ++i) {
                            if (isValidCandidateOperand(Inst.getOperand(i))) {

                                Value *NewValue = replaceZero(Inst, i);
                                DEBUG(dbgs() << *NewValue << "\n");

                                // Testing the new value for the null pointer triggers an error
                                // "Terminator found in the middle of a basic block!" : why?
                                if (NewValue != nullptr) {
                                    ReplaceInstWithValue(BB.getInstList(), IIT, NewValue);
                                }

                                // Consecutive zero assignments causes some instructions to be skipped.
                                // The instruction iterator is decremented to avoid this.
                                //
                                // Concerning the return instruction, without this,
                                // the pass would just perform the transformation forever.
                                // TODO: find a way to put the xor result into a virtual register
                                --IIT;


                                modified = true;
                                XORCount = XORCount + 1;
                            }
                        }
                    }
                }

                return modified;
            }

        private:

            // We either target store instructions in the form : store i32 0, i32* %var, function calls
            // and return instructions.
            bool isValidCandidateInstruction(Instruction &Inst) {
                if (isa<StoreInst>(&Inst)) {
                    return true;
                } else if (isa<ReturnInst>(&Inst)) {
                    return true;
                } else if (isa<CallInst>(&Inst)) {
                    return true;
                } else {
                    return false;
                }
            }

            // Check if the value stored in a operand is zero
            bool isValidCandidateOperand(Value *V) {
                if (ConstantInt* CI = dyn_cast<ConstantInt>(V)) {
                    if (CI->isZero()) {
                        return true;
                    }
                }

                return false;
            }

            Value *xorValue(Instruction &Inst, Value *var_ptr) {
                IRBuilder<> Builder(&Inst);

                Value *var_load = Builder.CreateLoad(var_ptr);
                Value *NewValue = Builder.CreateStore(
                        Builder.CreateXor(var_load,
                            var_load), var_ptr);

                return NewValue;
            }

            Value *replaceZero(Instruction &Inst, int operand_num) {

                IRBuilder<> Builder(&Inst);
                unsigned Opcode = Inst.getOpcode();
                // We insert instructions in the following form :
                //
                // %0 = load i32, i32* %var
                // %1 = xor i32 %0, %0
                // store i32 %1, i32* %var

                if (Opcode == Instruction::Store) {

                    Value *var_ptr = Inst.getOperand(1);
                    return xorValue(Inst, var_ptr);
                }

                if (Opcode == Instruction::Ret) {

                    Value *new_var = Builder.CreateAlloca(Inst.getOperand(operand_num)->getType());
                    xorValue(Inst, new_var);
                    return Builder.CreateRet(Builder.CreateLoad(new_var));
                }

                if (Opcode == Instruction::Call) {
                    Value *new_var = Builder.CreateAlloca(Inst.getOperand(operand_num)->getType());
                    xorValue(Inst, new_var);

                    if (CallInst* CI = dyn_cast<CallInst>(&Inst)) {

                        Function *fun = CI->getCalledFunction();
                        if (fun)
                            DEBUG(dbgs() << fun->getName() << "\n");

                        CI->setArgOperand(operand_num, Builder.CreateLoad(new_var));
                    }
                }

                return nullptr;
            }
    };
}

char XOR::ID = 0;
static RegisterPass<XOR>
X(  "xor",  // the option name -> -xor
    "0 Constant to XOR Substitution", // option description
    true, // true as we don't modify the CFG
    false // true if we're writing an analysis
 );

// clang pass registration (optional)
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

static void
registerClangPass(const PassManagerBuilder &, legacy::PassManagerBase &PM) {
    PM.add(new XOR());
}

// Note the registration point, clang offers several insertion points where you
// can insert your pass.
static RegisterStandardPasses RegisterClangPass
(PassManagerBuilder::EP_EarlyAsPossible, registerClangPass);
