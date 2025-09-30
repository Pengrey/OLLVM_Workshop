#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {
    struct ListInstructions : public PassInfoMixin<ListInstructions> {
        PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
            for (auto& F : M) {
                for (auto& BB : F) {
                    for (auto& I : BB) {
                        errs() << "Instruction:\n" << I << "\n";
                    }
                }
            }
            return PreservedAnalyses::all();
        };
    };
}

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return {
        .APIVersion = LLVM_PLUGIN_API_VERSION,
        .PluginName = "ListInstructions",
        .PluginVersion = "v0.1",
        .RegisterPassBuilderCallbacks = [](PassBuilder &PB) {
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level) {
                    MPM.addPass(ListInstructions());
                });
        }
    };
}
