#include "../include/daedalus.h"
#include "../include/wyvern/ProgramSlice.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/NativeFormatting.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/ValueMapper.h"
#include <exception>
#include <memory>

using namespace llvm;
#include <llvm/Pass.h>

#define DEBUG_TYPE "Daedalus"

// These Instructions types will not be a slice criterion.
bool canSliceInstrType(Instruction &I) {
    if (isa<BranchInst>(I)) return false; // Branch Instruction have badref
    if (isa<ReturnInst>(I)) return false;
    if (isa<AllocaInst>(I)) return false; // No needed
    if (isa<ICmpInst>(I)) return false;
    if (isa<LoadInst>(I)) return false;
    if (isa<StoreInst>(I)) return false;
    return true;
}

enum instState { UNVISITED, VISITED, DELETED };

// Can be removed, if its on slice instruction set AND all
// uses can be removed,
bool tryRemoveInstruction(Instruction *I, std::set<Instruction *> &s,
                          std::map<Instruction *, instState> &instMap,
                          Instruction *ini) {
    StringRef Iname = I->getName();
    dbgs() << Iname << '\n';
    dbgs() << *I << '\n';
    if (!I || instMap[I] == DELETED) return true;
    if (instMap[I] == VISITED) {
        // I->replaceAllUsesWith(UndefValue::get(I->getType()));
        return false;
    }
    instMap[I] = VISITED;
    if (s.find(I) == s.end() || I->isTerminator()) return false;

    std::set<User *> allUsers;
    for(auto U: I->users()) allUsers.insert(U);

    for (auto U : allUsers) {
	dbgs() << "NEW\n";
        // if (U.getParent() == nullptr) continue;
	if(U == nullptr){
	    dbgs() << "NULL\n";
	    if (I->users().empty() || allUsers.empty()) break;
	    dbgs() << allUsers.size() <<"\n";
	    continue;
	};
        if (Instruction *u = dyn_cast<Instruction>(U)) {
	    dbgs() << "INST\n";
	    /// If cant remove one of its users, then cant remove it as well.
            if (!tryRemoveInstruction(u, s, instMap, ini)) {
                return false;
            }
        }
	allUsers.erase(U);
        if (I->users().empty()) break;
    }
    if (I != ini) {
        s.erase(I);
        I->replaceAllUsesWith(UndefValue::get(I->getType()));
        I->eraseFromParent();
        instMap[I] = DELETED;
    }
    return true;
}

bool canProgramSlice(Instruction *I) {
    /// PHINode MUST to be at top of basic blocks. If our criterion
    /// is a phi-node it will be replace by an callsite, then all the phi-nodes
    /// under it, MUST to be moved above our criterion. But if at least one
    /// of these phi-nodes is an user of our criterion, then it will not
    /// dominate all the uses. In this case, we cant replace it.
    if (PHINode *phi = dyn_cast<PHINode>(I)) {
        for (User *use : phi->users()) {
            if (Instruction *Iuse = dyn_cast<Instruction>(use))
                if (isa<PHINode>(Iuse) && Iuse->getParent() == I->getParent()) {
                    dbgs() << "Criterion is a phi-node which at least one of "
                              "it's users are a Phi-Node and are in the same "
                              "basic block!\n";
                    return false;
                }
        }
    }
    return true;
}

bool meetCriterion(Function *F, Instruction *I) {
    for (auto &BB : *F) {
        Instruction *terminator = BB.getTerminator();
        if (!terminator) {
            dbgs() << "not tem terminal\n";
            return false;
        }
        if (terminator->getNumOperands() == 0) return false; // void inst
        if (Instruction *inst = dyn_cast<ReturnInst>(terminator))
            for (auto &it : inst->operands())
                if (I == it) return true;
    }
    return false;
}

struct iSlice {
    Instruction *I;
    Function *F;
    SmallVector<Value *> args;
    std::set<Instruction *> constOriginalInst;
    bool wasRemoved;
};

namespace Daedalus {

PreservedAnalyses DaedalusPass::run(Module &M, ModuleAnalysisManager &MAM) {
    std::set<Function *> FtoMap;
    std::vector<iSlice> allSlices;
    for (Function &F : M.getFunctionList()) FtoMap.insert(&F);

    std::unique_ptr<Module> module =
        std::make_unique<Module>("New_" + M.getName().str(), M.getContext());

    for (Function *F : FtoMap) {

        PostDominatorTree PDT;
        PDT.recalculate(*F);

        std::set<Instruction *> S;
        for (Instruction &I : instructions(F)) S.insert(&I);

        /// To replace all uses of I with the correpondent call
        for (Instruction *I : S) {
            if (!canSliceInstrType(*I)) continue;
            if (!canProgramSlice(I)) continue;
            if (!meetCriterion(F, I)) continue;
            dbgs() << "I: \t" << *I << '\n';

            ProgramSlice ps = ProgramSlice(*I, *F, PDT);
            if (!ps.canOutline()) continue;

            Function *G = ps.outline();
            SmallVector<Value *> funcArgs = ps.getOrigFunctionArgs();

            // Get the original instruction, before clone on slice, to check if
            // it can be removed
            std::map<Instruction *, Instruction *> constOriginalInst =
                ps.getInstructionInSlice();

            std::set<Instruction *> originInstructionSet;
            for (auto &e : constOriginalInst)
                originInstructionSet.insert(e.first);

            iSlice slice = {I, G, funcArgs, originInstructionSet, false};
            allSlices.push_back(slice);
        }
    }
    // TODO: Try to merge, if cant merge, delete the functions.
    // > let on allSlices, only the slice that is worth to merge.
    // ...
    //
    //
    //

    dbgs() << "Removing inst\n";
    // If it is worth to merge, then substitute the original instruction with
    // the corresponding function call, and removed unsed instructions from
    // original function.

    for (auto IS : allSlices) {
        auto [I, F, args, origInst, wasRemoved] = IS;
        if (wasRemoved) continue;

        CallInst *callInst =
            CallInst::Create(F, args, I->getName(), I->getParent());

        Instruction *moveTo = I;
        if (I && isa<PHINode>(I)) moveTo = I->getParent()->getFirstNonPHI();
        callInst->moveBefore(moveTo);

        // Check if can remove inst, and removed it.
        std::map<Instruction *, instState>
            mutInstMap;                      // If instruction I was removed;
        std::map<Instruction *, bool> visit; // If instruction I was visited;
        std::set<Instruction *>
            mutSet; // a mutable set of instruction on slice.

        for (auto inst : origInst) {
            mutInstMap[inst] = UNVISITED;
            mutSet.insert(inst);
        }
        dbgs() << "Instruction to remove:\n";
        for (auto IS : mutSet) {
            dbgs() << *IS << '\n';
        }
        dbgs() << "END\n";
        mutInstMap[I] = VISITED;
        for (auto [J, isRemoved] : mutInstMap) {
            if (!isRemoved) tryRemoveInstruction(J, mutSet, mutInstMap, I);
            if (mutInstMap.empty()) break;
        }

        I->replaceAllUsesWith(callInst);
        I->eraseFromParent();
        origInst.erase(I);
    }
    dbgs() << "ENDFILE\n";
    for (Function &F : M.getFunctionList()) {
        dbgs() << F << '\n';
    }

    module->print(dbgs(), nullptr);

    return PreservedAnalyses::none();
}
} // namespace Daedalus
