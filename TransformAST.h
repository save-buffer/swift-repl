#ifndef TRANSFORM_AST_H
#define TRANSFORM_AST_H

#include <swift/AST/Decl.h>
#include <swift/AST/Type.h>
#include <swift/AST/Module.h>

void AddImportNodes(swift::SourceFile &src_file,
                    const std::vector<swift::ImportDecl *> &import_decls);
void CombineTopLevelDeclsAndMoveToBack(swift::SourceFile &src_file);
void TransformFinalExpressionAndAddGlobal(swift::SourceFile &src_file);
void WrapInFunction(swift::SourceFile &src_file);
void MakeDeclarationsPublic(swift::SourceFile &src_file);

#endif
