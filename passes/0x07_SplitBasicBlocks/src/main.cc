#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdlib>
#include <ctime>

//TODO

using namespace llvm;

namespace {
    struct SplitBasicBlocks : public PassInfoMixin<SplitBasicBlocks> {
        PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
            //TODO

            auto &CTX = M.getContext();
            IntegerType *int32Ty = IntegerType::getInt32Ty(CTX);

            auto containsPHI = [](const BasicBlock *BB) {
                for (const Instruction &I : *BB) {
                    if (isa<PHINode>(&I)) return true;
                }
                return false;
            };

            for (Function &F : M) {
                if (F.isDeclaration()) continue;

                F.addFnAttr(Attribute::NoInline);

                std::vector<BasicBlock *> worklist;
                for (BasicBlock &BB : F) {
                    if (BB.size() >= 3 && !containsPHI(&BB)) {
                        worklist.push_back(&BB);                                                                    // Save to modify later
                    }
                }

                if (worklist.empty()) continue;

                errs() << formatv("Targeting {0,3} blocks in function {1,-20}", worklist.size(), F.getName());

                for (BasicBlock *BB : worklist) {
                    if ((rand() % 100) >= SPLIT_CHANCE_PERCENT) {
                        continue;
                    }

                    unsigned splitIdx = 1 + (rand() % (BB->size() - 2));                                            // Get index to split BB
                    auto splitIt = std::next(BB->begin(), splitIdx);

                    //TODO

                    BasicBlock *dummyBlock = BasicBlock::Create(CTX, BB->getName() + ".dummy", &F, successor);
                    IRBuilder<>(dummyBlock).CreateBr(successor);

                    //TODO

                    IRBuilder<> builder(oldTerminator);
                    //TODO
                    oldTerminator->eraseFromParent();

                    errs() << formatv("[REPLACED]: Block was slpitted\t");
                }

                errs() << "[Done]\n";
            }
            return PreservedAnalyses::none();
        }
    };
}

// Plugin registration remains the same.
extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return {
        .APIVersion = LLVM_PLUGIN_API_VERSION,
        .PluginName = "SplitBasicBlocks",
        .PluginVersion = "v0.1",
        .RegisterPassBuilderCallbacks = [](PassBuilder &PB) {
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level) {
                    MPM.addPass(SplitBasicBlocks());
                });
        }
    };
}
