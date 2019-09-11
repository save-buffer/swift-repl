#include <algorithm>
#include <string>
#include <vector>

#include <swift/AST/ASTContext.h>
#include <swift/AST/ASTWalker.h>
#include <swift/AST/Expr.h>
#include <swift/AST/ParameterList.h>
#include <swift/AST/Pattern.h>
#include <swift/AST/Stmt.h>

#include "TransformAST.h"

void AddImportNodes(swift::SourceFile &src_file,
                    const std::vector<swift::ImportDecl *> &import_decls)
{
    swift::ASTContext &ast_ctx = src_file.getASTContext();
    src_file.Decls.insert(src_file.Decls.begin(),
                          import_decls.begin(), import_decls.end());
}

void CombineTopLevelDeclsAndMoveToBack(swift::SourceFile &src_file)
{
    if(src_file.Decls.empty())
        return;

    std::sort(src_file.Decls.begin(), src_file.Decls.end(),
              [](const swift::Decl *a, const swift::Decl *b)
              {
                  int is_a_top = llvm::isa<swift::TopLevelCodeDecl>(a);
                  int is_b_top = llvm::isa<swift::TopLevelCodeDecl>(b);
                  return is_a_top < is_b_top;
              });
    // NOTE(sasha): TLD = Top Level Declaration
    auto tld_iter = std::find_if(src_file.Decls.begin(), src_file.Decls.end(),
                                 [](const swift::Decl *d)
                                 {
                                     return llvm::isa<swift::TopLevelCodeDecl>(d);
                                 });

    if(tld_iter == src_file.Decls.end())
        return;

    std::vector<swift::ASTNode> body;
    std::for_each(tld_iter, src_file.Decls.end(),
                  [&](swift::Decl *d)
                  {
                      assert(llvm::isa<swift::TopLevelCodeDecl>(d));
                      auto *tld = llvm::dyn_cast<swift::TopLevelCodeDecl>(d);
                      body.insert(body.end(),
                                  tld->getBody()->getElements().begin(),
                                  tld->getBody()->getElements().end());
                      // TODO(sasha): We could potentially delete the
                      //              BraceStmt and TopLevelCodeDecl nodes,
                      //              but maybe not worth figuring out.
                  });
    src_file.Decls.erase(tld_iter, src_file.Decls.end());

    swift::ASTContext &ast_ctx = src_file.getASTContext();
    swift::BraceStmt *body_stmt = swift::BraceStmt::create(
        ast_ctx, swift::SourceLoc(), body, swift::SourceLoc(), true);
    swift::TopLevelCodeDecl *new_tld = new (ast_ctx) swift::TopLevelCodeDecl(
        &src_file, body_stmt);
    src_file.Decls.push_back(new_tld);
}

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
        if(!last_expr)
            return;

        swift::Type return_type = last_expr->getType();

        // Don't create result variable of type void
        if(swift::CanType(return_type) == swift::CanType(ast_ctx.TheEmptyTupleType))
            return;

        auto return_var_name = src_file.getFilename() + "_res";
        swift::VarDecl *return_var = new (ast_ctx) swift::VarDecl(
            false, // IsStatic
            swift::VarDecl::Introducer::Var,
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

        // Insert print statement:
        llvm::SmallVector<swift::ValueDecl *, 0> lookup_result;
        ast_ctx.getStdlibModule()->lookupMember(
            lookup_result,
            ast_ctx.getStdlibModule(),
            swift::DeclName(ast_ctx.getIdentifier("print")),
            swift::Identifier());

        swift::ValueDecl *print_decl = lookup_result.front();
        lookup_result.clear();

        swift::Type string_type;
        for(auto *file : ast_ctx.getStdlibModule()->getFiles())
            file->lookupValue(ast_ctx.getIdentifier("String"), swift::NLKind::UnqualifiedLookup, lookup_result);

        if(auto *type_decl = llvm::dyn_cast<swift::NominalTypeDecl>(lookup_result.front()))
            string_type = type_decl->getDeclaredType();
        assert(string_type);

        swift::Identifier separator_id = ast_ctx.getIdentifier("separator");
        swift::Identifier terminator_id = ast_ctx.getIdentifier("terminator");

        swift::Type print_type = swift::FunctionType::get(
            {
                swift::AnyFunctionType::Param(ast_ctx.TheAnyType, swift::Identifier(), swift::ParameterTypeFlags().withVariadic(true)),
                swift::AnyFunctionType::Param(string_type, separator_id),
                swift::AnyFunctionType::Param(string_type, terminator_id),
            },
            ast_ctx.TheEmptyTupleType);

        swift::DeclRefExpr *print_ref = new (ast_ctx) swift::DeclRefExpr(
            swift::ConcreteDeclRef(print_decl),
            swift::DeclNameLoc(), true, swift::AccessSemantics::Ordinary, print_type);

        swift::DeclRefExpr *res_var_ref = new (ast_ctx) swift::DeclRefExpr(
            swift::ConcreteDeclRef(return_var),
            swift::DeclNameLoc(), true, swift::AccessSemantics::Ordinary, return_type);

        swift::ErasureExpr *erasure = swift::ErasureExpr::create(
            ast_ctx, res_var_ref, ast_ctx.TheAnyType, {});

        swift::ArrayExpr *array = swift::ArrayExpr::create(
            ast_ctx, swift::SourceLoc(), { erasure }, {}, swift::SourceLoc(), swift::ArraySliceType::get(ast_ctx.TheAnyType));

        swift::VarargExpansionExpr *vararg = new (ast_ctx) swift::VarargExpansionExpr(
            array, false, array->getType());

        swift::DefaultArgumentExpr *default_separator = new (ast_ctx) swift::DefaultArgumentExpr(
            swift::ConcreteDeclRef(print_decl), 1, swift::SourceLoc(), string_type);

        swift::DefaultArgumentExpr *default_terminator = new (ast_ctx) swift::DefaultArgumentExpr(
            swift::ConcreteDeclRef(print_decl), 2, swift::SourceLoc(), string_type);

        swift::Type tuple_type = swift::TupleType::get(
            {
                swift::TupleTypeElt(array->getType(), swift::Identifier(), swift::ParameterTypeFlags().withVariadic(true)),
                swift::TupleTypeElt(string_type, separator_id),
                swift::TupleTypeElt(string_type, terminator_id),
            },
            ast_ctx);

        swift::TupleExpr *tuple = swift::TupleExpr::create(
            ast_ctx, swift::SourceLoc(),
            { vararg, default_separator, default_terminator },
            { swift::Identifier(), separator_id, terminator_id },
            { swift::SourceLoc(), swift::SourceLoc(), swift::SourceLoc() },
            swift::SourceLoc(), false, true,
            tuple_type);

        swift::CallExpr *call = swift::CallExpr::create(
            ast_ctx, print_ref, tuple,
            { swift::Identifier() },
            { swift::SourceLoc(), swift::SourceLoc(), swift::SourceLoc() },
            false, true, ast_ctx.TheEmptyTupleType);
        call->setThrows(false);

        // Append the call to print to the body
        std::vector<swift::ASTNode> new_body;
        for(auto statement : last_top_level_code_decl->getBody()->getElements())
            new_body.push_back(statement);
        new_body.push_back(call);

        // Update the body
        last_top_level_code_decl->setBody(swift::BraceStmt::create(
                                              ast_ctx, swift::SourceLoc(),
                                              new_body, swift::SourceLoc(), true)
            );
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
