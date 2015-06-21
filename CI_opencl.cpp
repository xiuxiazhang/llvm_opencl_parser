#include <iostream>

#include "llvm/Support/Host.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"

#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Parse/Parser.h"
#include "clang/Parse/ParseAST.h"

/******************************************************************************
 *
 *****************************************************************************/
class MyASTConsumer : public clang::ASTConsumer
{
public:
    MyASTConsumer() : clang::ASTConsumer() { }
    virtual ~MyASTConsumer() { }

    virtual bool HandleTopLevelDecl( clang::DeclGroupRef d)
    {
        clang::DeclGroupRef::iterator it;
        for( it = d.begin(); it != d.end(); it++)
        {
            clang::FunctionDecl *fd=llvm::dyn_cast<clang::FunctionDecl>(*it);
            if(!fd)
            {
                continue;
            }
              if( fd->isFunctionOrMethod()) {

                std::string res=clang::QualType::getAsString(fd->getResultType().split());
                std::size_t found = res.find("__attribute__((address_space(8)))"); /* find __kernel qualifer */
                if (found != std::string::npos) {
                    std::cout << fd->getNameInfo().getAsString() << "\n";
                    std::cout << fd->getNumParams()<<"\n"; /*number paramters */
                    clang::ParmVarDecl* pd;
                    for(int i = 0; i < fd->getNumParams(); i++) {
                        pd=fd->getParamDecl(i);
                        std::cout << clang::QualType::getAsString(pd->getType().split()) <<"\n"; /* parameter type */
                    }
                }
            }
        }
        return true;
    }
};

/******************************************************************************
 *
 *****************************************************************************/
int main(int argc, char **argv)
{
    using clang::CompilerInstance;
    using clang::TargetOptions;
    using clang::TargetInfo;
    using clang::FileEntry;
    using clang::Token;
    using clang::ASTContext;
    using clang::ASTConsumer;
    using clang::Parser;
    using clang::DiagnosticOptions;
    using clang::TextDiagnosticPrinter;
    using clang::CompilerInvocation;
    using clang::LangOptions;

    CompilerInstance ci;
    DiagnosticOptions diagnosticOptions;
    ci.createDiagnostics();

    CompilerInvocation *Invocation = new CompilerInvocation;
    ci.setInvocation(Invocation);
    LangOptions langOpts;
    langOpts.RTTI = 1;
    langOpts.Bool = 1;
    langOpts.OpenCL= 1;
    Invocation->setLangDefaults(langOpts,
                              clang::IK_OpenCL,
                              clang::LangStandard::lang_opencl);

    llvm::IntrusiveRefCntPtr<TargetOptions> pto( new TargetOptions());
    pto->Triple = llvm::sys::getDefaultTargetTriple();
    TargetInfo *pti = TargetInfo::CreateTargetInfo(ci.getDiagnostics(), pto.getPtr());
    ci.setTarget(pti);

    ci.createFileManager();
    ci.createSourceManager(ci.getFileManager());
    ci.createPreprocessor();
    ci.getPreprocessorOpts().UsePredefines = false;
    MyASTConsumer *astConsumer = new MyASTConsumer();
    ci.setASTConsumer(astConsumer);

    ci.createASTContext();

	const FileEntry *pFile = ci.getFileManager().getFile(argv[1]);
    ci.getSourceManager().createMainFileID(pFile);
    ci.getDiagnosticClient().BeginSourceFile(ci.getLangOpts(),
                                           &ci.getPreprocessor());
    clang::ParseAST(ci.getPreprocessor(), astConsumer, ci.getASTContext());
    ci.getDiagnosticClient().EndSourceFile();

    return 0;
}
