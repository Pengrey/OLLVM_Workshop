#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/raw_ostream.h"

#include <vector>
#include <map>

using namespace llvm;

namespace {

    struct ControlFlowFlattening : public PassInfoMixin<ControlFlowFlattening> {
        PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
            for (Function &F : M) {
                if (F.isDeclaration() || F.size() < 3) {
                    continue;
                }

                //TODO

                auto &CTX = F.getContext();
                IntegerType *int32Ty = IntegerType::getInt32Ty(CTX);

                BasicBlock *entryBlock = &F.getEntryBlock();

                if (entryBlock->getTerminator()->getNumSuccessors() != 1) {                                 
                    entryBlock->splitBasicBlock(entryBlock->getTerminator(), "entry.split");
                }

                std::vector<BasicBlock *> originalBlocks;                                                                       // Collect original basic blocks
                for (BasicBlock &BB : F) {
                    if (&BB != entryBlock) {
                        originalBlocks.push_back(&BB);
                    }
                }

                if (originalBlocks.empty()) {
                    continue;
                }

                BasicBlock *dispatcherBlock = BasicBlock::Create(CTX, "dispatcher", &F);
                BasicBlock *defaultBlock = BasicBlock::Create(CTX, "defaultCase", &F);

                new UnreachableInst(CTX, defaultBlock);                                                                         // Default case leads to unreachable

                dispatcherBlock->moveAfter(entryBlock);
                defaultBlock->moveAfter(dispatcherBlock);

                IRBuilder<> allocaBuilder(&entryBlock->front());                                                                // State variable for the dispatcher
                //TODO

                Instruction *entryTerm = entryBlock->getTerminator();
                BasicBlock *firstBlock = entryTerm->getSuccessor(0);

                std::map<BasicBlock *, int> blockToIdMap;                                                                       // Map blocks to unique IDs
                int currentId = 1;
                for (BasicBlock *BB : originalBlocks) {
                    blockToIdMap[BB] = currentId++;
                }

                IRBuilder<> termBuilder(entryTerm);                                                                             // Initialize state variable and branch to dispatcher
                termBuilder.CreateStore(ConstantInt::get(int32Ty, blockToIdMap[firstBlock]), stateVar);
                termBuilder.CreateBr(dispatcherBlock);
                entryTerm->eraseFromParent();

                IRBuilder<> dispatcherBuilder(dispatcherBlock);                                                                 // Build the dispatcher switch
                Value *loadedState = dispatcherBuilder.CreateLoad(int32Ty, stateVar, "loadedState");
                SwitchInst *dispatchSwitch = dispatcherBuilder.CreateSwitch(loadedState, defaultBlock, originalBlocks.size());

                BasicBlock* lastBlock = dispatcherBlock;                                                                        // Rewire original blocks
                for (auto const& [block, id] : blockToIdMap) {
                    dispatchSwitch->addCase(ConstantInt::get(int32Ty, id), block);
                    block->moveAfter(lastBlock);
                }

                //TODO

                std::vector<AllocaInst*> AllocasToMove;                                                                         // Move allocas to entry block
                for (BasicBlock &BB : F) {
                    if (&BB == entryBlock) continue;
                    for (Instruction &I : BB) {
                        if (AllocaInst *AI = dyn_cast<AllocaInst>(&I)) {
                            AllocasToMove.push_back(AI);
                        }
                    }
                }

                BasicBlock::iterator InsertPt = entryBlock->getFirstNonPHIOrDbgOrAlloca();
                for (AllocaInst *AI : AllocasToMove) {
                    AI->moveBefore(InsertPt);
                }

                errs() << "[Done]\n";
            }
            return PreservedAnalyses::none();
        }
    };

}

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return {
        .APIVersion = LLVM_PLUGIN_API_VERSION,
        .PluginName = "ControlFlowFlattening",
        .PluginVersion = "v0.1",
        .RegisterPassBuilderCallbacks = [](PassBuilder &PB) {
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level) {
                    MPM.addPass(ControlFlowFlattening());
                });
        }
    };
}
