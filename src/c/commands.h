// Copyright 2014 Ben Smith. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef COMMANDS_H_
#define COMMANDS_H_

#include "bool.h"
#include "message.h"


bool HandleBuiltinCommand(Command* command);


#define TYPE_CHECK(expected) \
  VERROR_IF(command->type == expected, \
            "Type mismatch. Expected %s. Got %s.", \
            TypeToString(expected), \
            TypeToString(command->type))

#define TYPE_FAIL \
  VERROR("Type didn't match any types. Got %s.", TypeToString(command->type))

#define ARG_VOIDP(index) \
  void* arg##index; \
  if (!GetArgVoidp(command, index, &arg##index)) return

#define ARG_VOIDP_CAST(index, type) \
  void* arg##index##_voidp; \
  if (!GetArgVoidp(command, index, &arg##index##_voidp)) return; \
  type arg##index = (type)arg##index##_voidp

#define ARG_CHARP(index) \
  char* arg##index; \
  if (!GetArgCharp(command, index, &arg##index)) return;

#define ARG_INT(index) \
  int32_t arg##index; \
  if (!GetArgInt32(command, index, &arg##index)) return

#define ARG_INT_CAST(index, type) \
  int32_t arg##index##_int; \
  if (!GetArgInt32(command, index, &arg##index##_int)) return; \
  type arg##index = (type)arg##index##_int

#define ARG_INT64(index) \
  int64_t arg##index; \
  if (!GetArgInt64(command, index, &arg##index)) return

#define ARG_UINT(index) \
  uint32_t arg##index; \
  if (!GetArgUint32(command, index, &arg##index)) return

#define ARG_UINT_CAST(index, type) \
  uint32_t arg##index##_uint; \
  if (!GetArgUint32(command, index, &arg##index##_uint)) return; \
  type arg##index = (type)arg##index##_uint

#define ARG_UINT64(index) \
  uint64_t arg##index; \
  if (!GetArgUint64(command, index, &arg##index)) return

#define ARG_VAR(index) \
  struct PP_Var arg##index; \
  if (!GetArgVar(command, index, &arg##index)) return

#define ARG_FLOAT32(index) \
  float arg##index; \
  if (!GetArgFloat32(command, index, &arg##index)) return

#define ARG_FLOAT64(index) \
  double arg##index; \
  if (!GetArgFloat64(command, index, &arg##index)) return

#endif  // COMMANDS_H_
