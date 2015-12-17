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


                    DEBUG(dbgs() << Inst.getOperand(0)->getValueName() << "\n");
                    DEBUG(dbgs() << Inst.getNumOperands() << "\n");
                    //DEBUG(dbgs() << Inst.getOperand(1)->getName() << "\n");
                    //DEBUG(dbgs() << Inst.getOperand(1)->getValueName() << "\n");
                    DEBUG(dbgs() << "................" << "\n");

                    IRBuilder<> Builder(&Inst);

                    StoreInst *SI = dyn_cast<StoreInst>(IIT);

                    if (ConstantInt* CI = dyn_cast<ConstantInt>(Inst.getOperand(0))) {
                        if (CI->getBitWidth() <= 32) {
                            // Check if the value stored is zero
                            if (CI->isZero()) {

                                // We either target store instructions in the form : store i32 0, i32* %var
                                // and return instructions.
                                unsigned Opcode = Inst.getOpcode();
                                // We insert instructions in the following form :
                                //
                                // %0 = load i32, i32* %var, align 4
                                // %1 = xor i32 %0, %0
                                // store i32 %1, i32* %var, align 4

                                if (Opcode == Instruction::Store) {

                                    Value *var_ptr = Inst.getOperand(1);
                                    Value *var_load = Builder.CreateAlignedLoad(var_ptr, SI->getAlignment());
                                    Value *NewValue = Builder.CreateAlignedStore(
                                            Builder.CreateXor(var_load,
                                                var_load), var_ptr, SI->getAlignment());
                                    ReplaceInstWithValue(BB.getInstList(), IIT, NewValue);

                                    // Consecutive zero assignments causes some instructions to be skipped.
                                    // The instruction iterator is decremented to avoid this.
                                    --IIT;
                                }

                                // inconsequent transformation
                                if (Opcode == Instruction::Ret) {
                                    Value *one_1 = ConstantInt::get(CI->getContext(), llvm::APInt(32, 1, true));
                                    Value *one_2 = ConstantInt::get(CI->getContext(), llvm::APInt(32, 1, true));
                                    Value *NewValue = Builder.CreateRet(
                                            Builder.CreateXor(one_1, one_2));
                                    ReplaceInstWithValue(BB.getInstList(), IIT, NewValue);

                                    // Without this, the pass will just perform the transformation forever
                                    // TODO: find a way to put the xor result into a virtual register
                                    IIT--;
                                }

                                modified = true;
                                XORCount = XORCount + 1;
                            }
                        }
                    }
                }

                return modified;
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
