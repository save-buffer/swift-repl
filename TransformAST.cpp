#include <string>

#include <swift/AST/ASTContext.h>
#include <swift/AST/ASTWalker.h>
#include <swift/AST/Expr.h>
#include <swift/AST/ParameterList.h>
#include <swift/AST/Pattern.h>
#include <swift/AST/Stmt.h>

#include "TransformAST.h"

void TransformFinalExpressionAndAddGlobal(swift::SourceFile &src_file)
{
    if(src_file.Decls.empty())
        return;

    swift::Decl *last_decl = src_file.Decls.back();
    assert(last_decl);
    if(auto last_top_level_code_decl = llvm::dyn_cast<swift::TopLevelCodeDecl>(last_decl))
    {
        swift::ASTContext &ast_ctx = src_file.getASTContext();
        llvm::MutableArrayRef<swift::ASTNode>::iterator back_iterator =
            last_top_level_code_decl->getBody()->getElements().end() - 1;
        swift::Expr *last_expr = (*back_iterator).dyn_cast<swift::Expr *>();
        swift::Type return_type = last_expr->getType();

        auto return_var_name = src_file.getFilename() + "_res";
        swift::VarDecl *return_var = new (ast_ctx) swift::VarDecl(
            false, // IsStatic
            swift::VarDecl::Specifier::Var,
            false, // IsCaptureList
            swift::SourceLoc(),
            ast_ctx.getIdentifier(return_var_name.str()),
            &src_file);

        return_var->setType(return_type);
        return_var->setInterfaceType(return_type);
        src_file.Decls.insert(src_file.Decls.begin(), return_var);

        swift::NamedPattern *var_reference =
            new (ast_ctx) swift::NamedPattern(return_var, /* Implicit */ true); 
        var_reference->setType(return_type);

        swift::PatternBindingDecl *assignment =
            swift::PatternBindingDecl::createImplicit(
                ast_ctx, swift::StaticSpellingKind::None, var_reference,
                last_expr, last_top_level_code_decl);

        *back_iterator = assignment;
    }
}

void WrapInFunction(swift::SourceFile &src_file)
{
    if(src_file.Decls.empty())
        return;

    swift::Decl *last_decl = src_file.Decls.back();
    assert(last_decl);
    if(auto last_top_level_code_decl = llvm::dyn_cast<swift::TopLevelCodeDecl>(last_decl))
    {
        swift::ASTContext &ast_ctx = src_file.getASTContext();

        swift::ParameterList *empty_params_list =
            swift::ParameterList::createEmpty(ast_ctx);

        llvm::StringRef fn_name_str = src_file.getFilename();
        swift::Identifier fn_name_id = ast_ctx.getIdentifier(fn_name_str);
        swift::DeclName fn_name(ast_ctx, fn_name_id, empty_params_list);

        swift::BraceStmt *old_top_level_body = last_top_level_code_decl->getBody();
        swift::SourceLoc fn_start = old_top_level_body->getLBraceLoc();
        swift::SourceLoc fn_end = old_top_level_body->getRBraceLoc();

        swift::FuncDecl *new_func = swift::FuncDecl::create(
            ast_ctx, // AST Context
            swift::SourceLoc(), swift::StaticSpellingKind::None, // StaticLoc, StaticSpelling
            fn_start, fn_name, fn_start, // FuncLoc, Name, NameLoc
            false, swift::SourceLoc(),   // Throws, ThrowsLoc
            nullptr, // GenericParams
            empty_params_list, // BodyParams
            swift::TypeLoc(), // ReturnType should be void
            last_top_level_code_decl->getParent()); // Parent
        new_func->setInterfaceType(swift::FunctionType::get({}, ast_ctx.TheEmptyTupleType, {}));

        swift::BraceStmt *fn_body = swift::BraceStmt::create(
            ast_ctx, swift::SourceLoc(), old_top_level_body->getElements(),
            swift::SourceLoc(), true);

        new_func->setBody(fn_body);
        src_file.Decls.back() = new_func;
    }
}

void MakeDeclarationsPublic(swift::SourceFile &src_file)
{
    class Walker : public swift::ASTWalker
    {
        bool CanBeMadePublic(swift::Decl *decl)
        {
            assert(decl);
            if(llvm::isa<swift::StructDecl>(decl->getDeclContext()))
            {
                if(auto var = llvm::dyn_cast<swift::VarDecl>(decl))
                    if(var->getOriginalWrappedProperty())
                        return false;
            }
            else if(auto accessor = llvm::dyn_cast<swift::AccessorDecl>(decl))
                return CanBeMadePublic(accessor->getStorage());
            
            return true;
        }
        
        bool walkToDeclPre(swift::Decl *decl) override
        {
            if(!CanBeMadePublic(decl))
                return true;

            if(auto *value_decl = llvm::dyn_cast<swift::ValueDecl>(decl))
            {
                auto access = swift::AccessLevel::Public;

                if(llvm::isa<swift::ClassDecl>(value_decl) ||
                   value_decl->isPotentiallyOverridable())
                {
                    if(!value_decl->isFinal())
                        access = swift::AccessLevel::Open;
                }
                
                value_decl->overwriteAccess(access);
                if(auto *storage_decl = llvm::dyn_cast<swift::AbstractStorageDecl>(decl))
                    storage_decl->overwriteSetterAccess(access);
            }
            return true;
        }
    };
    
    Walker w;

    for(swift::Decl *decl : src_file.Decls)
        decl->walk(w);
}
