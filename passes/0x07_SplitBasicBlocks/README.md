# Split Basic Blocks Pass
This is a simple implementation of a LLVM pass that randomly splits basic blocks in the LLVM IR code. The pass iterates over all the functions and their basic blocks, and for each eligible block, it may choose to split it.

The implementation first iterates over all functions in the module. For each function, it finds eligible basic blocks (those with at least 3 instructions and no PHI nodes) and saves them in a worklist to be modified later.

> Note: We save the blocks to a worklist to avoid modifying the function's basic block list while iterating over it. This prevents iterator invalidation, which could lead to crashes or undefined behavior.

```cpp
<SNIP>
    struct SplitBasicBlock : public PassInfoMixin<SplitBasicBlock> {
        PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
            // Seed the pass's RNG. This makes the obfuscation different on every compilation.
            srand(time(NULL));
            <SNIP>
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
```

After populating the worklist, the pass iterates over the selected blocks. For each block, it has a 50% chance of applying the transformation. If a block is chosen, it is split into two parts: the original block (`BB`) and a `successor` block. A third `dummyBlock` is also created.

The original terminator of `BB` is then replaced with a conditional branch. The key to this obfuscation is that the condition for this branch is a **constant** (`true` or `false`) chosen randomly by the pass at compile-time.

```cpp
                errs() << formatv("Targeting {0,3} blocks in function {1,-20}", worklist.size(), F.getName());

                for (BasicBlock *BB : worklist) {
                    if ((rand() % 100) >= SPLIT_CHANCE_PERCENT) {
                        continue;
                    }

                    unsigned splitIdx = 1 + (rand() % (BB->size() - 2));
                    auto splitIt = std::next(BB->begin(), splitIdx);

                    // 1. Split the block.
                    BasicBlock *successor = BB->splitBasicBlock(splitIt, BB->getName() + ".split");
                    Instruction *oldTerminator = BB->getTerminator();

                    // 2. Create a dummy block that will be one of the branch targets.
                    BasicBlock *dummyBlock = BasicBlock::Create(CTX, BB->getName() + ".dummy", &F, successor);
                    IRBuilder<>(dummyBlock).CreateBr(successor);
                    
                    // 3. The pass's RNG decides which way the branch will always go.
                    bool condition = (rand() % 2 == 0);
                    Value* fixedCond = condition ? ConstantInt::getTrue(CTX) : ConstantInt::getFalse(CTX);

                    // 4. Create a conditional branch that uses this fixed value.
                    IRBuilder<> builder(oldTerminator);
                    builder.CreateCondBr(fixedCond, successor, dummyBlock);
                    oldTerminator->eraseFromParent();
                }

                errs() << "[Done]\n";
            }
<SNIP>
```

> The resulting control flow will always be the same at runtime (e.g., `BB -> dummyBlock -> successor` or `BB -> successor`), so the programs functionality is preserved.
