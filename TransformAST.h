#ifndef TRANSFORM_AST_H
#define TRANSFORM_AST_H

#include <swift/AST/Type.h>
#include <swift/AST/Module.h>

void TransformFinalExpressionAndAddGlobal(swift::SourceFile &src_file);
void WrapInFunction(swift::SourceFile &src_file);
void MakeDeclarationsPublic(swift::SourceFile &src_file);

#endif
