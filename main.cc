#include <vector>

#include "clang/Config/config.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "llvm/Support/Process.h"
#include "llvm/Support/TargetSelect.h"

#include "clang/Driver/Driver.h"

#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/Tooling.h"


using namespace clang::ast_matchers;
using namespace clang::driver;
using namespace clang::tooling;
using namespace clang;
using namespace llvm;

class LiteralArgCommenter : public MatchFinder::MatchCallback {
public:
	LiteralArgCommenter(Rewriter &YVR18Rewriter) : YVR18Rewriter(YVR18Rewriter) {}
	// This callback will be executed whenever the Matcher in YVR18ASTConsumer
	// matches.
	virtual void run(const MatchFinder::MatchResult &Result) {
		// ASTContext allows us to find the source location.
		ASTContext *Context = Result.Context;
		// Record the callees parameters. We can access the callee via the
		// .bind("callee") from the ASTMatcher. We will match these with
		// the callers arguments later.
		std::vector<ParmVarDecl*> Params;
		const FunctionDecl *CD = Result.Nodes.getNodeAs<clang::FunctionDecl>("callee");
		for (FunctionDecl::param_const_iterator PI = CD->param_begin(),
			 PE = CD->param_end();
			 PI != PE; ++PI) {
			Params.push_back(*PI);
		}
		const CallExpr *E = Result.Nodes.getNodeAs<clang::CallExpr>("functions");
		size_t Count = 0;
		if (E && CD && !Params.empty()) {
			auto I = E->arg_begin();
			if (isa<CXXOperatorCallExpr>(E))
				// The first parameter is the object itself, skip over it.
				++I;
			// For each argument match it with the callee parameter
			// If it is an integer or boolean literal then insert a comment
			// into the edit buffer.
			for (auto End = E->arg_end(); I != End; ++I, ++Count) {
				ParmVarDecl *PD = Params[Count];
				FullSourceLoc ParmLocation = Context->getFullLoc(PD->getBeginLoc());
				const Expr *AE = (*I)->IgnoreParenCasts();
				if (auto *IntArg = dyn_cast<IntegerLiteral>(AE)) {
					FullSourceLoc ArgLoc = Context->getFullLoc(IntArg->getBeginLoc());
					if (ParmLocation.isValid() && !PD->getDeclName().isEmpty() /*&& EditedLocations.insert(ArgLoc).second*/) {
						// Will insert our text immediately before the Argument
						YVR18Rewriter.InsertText(ArgLoc, (Twine(" /* ") + PD->getDeclName().getAsString() + " */ ").str());
					}
				}
				// Boolean case is almost identical, use CXXBoolLiteralExpr
			}
		}
	}

	Rewriter& YVR18Rewriter;
	std::set<FullSourceLoc> EditedLocations;
};

class YVR18ASTConsumer : public ASTConsumer {
public:
	YVR18ASTConsumer(Rewriter &R) : LAC(R) {
		// We use almost the same syntax as the ASTMatcher prototyped in
		// clang-query. The changes are the .bind(string) additions so that we
		// can access these once the match has occurred.
		StatementMatcher CallSiteMatcher =
			callExpr(
			allOf(callee(functionDecl(unless(isVariadic())).bind("callee")),
			unless(cxxMemberCallExpr(
			on(hasType(substTemplateTypeParmType())))),
			anyOf(hasAnyArgument(ignoringParenCasts(cxxBoolLiteral())),
			hasAnyArgument(ignoringParenCasts(integerLiteral())))))
			.bind("functions");
		// LAC is our callback that will run when the ASTMatcher finds the pattern above.
		Matcher.addMatcher(CallSiteMatcher, &LAC);
	}
	// Implement the call back so that we can run our Matcher on the source file.
	void HandleTranslationUnit(ASTContext &Context) override {
		Matcher.matchAST(Context);
	}
private:
	MatchFinder Matcher;
	LiteralArgCommenter LAC;
};

// We implement the ASTFrontEndAction interface to run our matcher on a
// source file.
class YVR18FrontendAction : public ASTFrontendAction {
public:
	// Output the edit buffer for this translation unit
	void EndSourceFileAction() override {
		YVR18Rewriter.getEditBuffer(YVR18Rewriter.getSourceMgr().getMainFileID()).write(llvm::outs());
	}
	// Returns our ASTConsumer implementation per translation unit.
	std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) override {
		YVR18Rewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
		return llvm::make_unique<YVR18ASTConsumer>(YVR18Rewriter);
	}
private:
	Rewriter YVR18Rewriter;
};

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory MyToolCategory("pky-codegen options");
// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
// A help message for this specific tool can be added afterwards.
static cl::extrahelp MoreHelp("\nMore help text...\n");

int main(int argc, const char **argv) {
	CommonOptionsParser OptionsParser(argc, argv, MyToolCategory);
	ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());
	// YVR18FrontEndAction is our implementation of ASTFrontEndAction
	return Tool.run(newFrontendActionFactory<YVR18FrontendAction>().get());
}