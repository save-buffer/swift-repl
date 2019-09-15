/**
 * Copyright (c) 2019-present, Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
