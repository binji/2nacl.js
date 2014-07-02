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


#include "commands.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bool.h"
#include "builtin_funcs.h"
#include "error.h"
#include "interfaces.h"
#include "type.h"
#include "var.h"

static void Handle_destroyHandles(Command* command);

static void Handle_get(Command* command);
static void Handle_set(Command* command);
static void Handle_add(Command* command);
static void Handle_sub(Command* command);
static void Handle_lt(Command* command);
static void Handle_lte(Command* command);
static void Handle_gt(Command* command);
static void Handle_gte(Command* command);
static void Handle_eq(Command* command);
static void Handle_ne(Command* command);
static void Handle_free(Command* command);
static void Handle_malloc(Command* command);
static void Handle_memset(Command* command);
static void Handle_memcpy(Command* command);
static void Handle_strlen(Command* command);
static void Handle_puts(Command* command);
static void Handle_varAddRef(Command* command);
static void Handle_varRelease(Command* command);
static void Handle_varFromUtf8(Command* command);
static void Handle_varToUtf8(Command* command);
static void Handle_arrayCreate(Command* command);
static void Handle_arrayGet(Command* command);
static void Handle_arraySet(Command* command);
static void Handle_arrayGetLength(Command* command);
static void Handle_arraySetLength(Command* command);
static void Handle_arrayBufferCreate(Command* command);
static void Handle_arrayBufferByteLength(Command* command);
static void Handle_arrayBufferMap(Command* command);
static void Handle_arrayBufferUnmap(Command* command);
static void Handle_dictCreate(Command* command);
static void Handle_dictGet(Command* command);
static void Handle_dictSet(Command* command);
static void Handle_dictDelete(Command* command);
static void Handle_dictHasKey(Command* command);

typedef void (*HandleFunc)(Command*);
typedef struct {
  const char* name;
  HandleFunc func;
} NameFunc;

// TODO(binji): hashmap
static NameFunc g_FuncMap[] = {
  {"*destroyHandles", Handle_destroyHandles},
  {"get", Handle_get},
  {"set", Handle_set},
  {"add", Handle_add},
  {"sub", Handle_sub},
  {"lt", Handle_lt},
  {"lte", Handle_lte},
  {"gt", Handle_gt},
  {"gte", Handle_gte},
  {"eq", Handle_eq},
  {"ne", Handle_ne},
  {"free", Handle_free},
  {"malloc", Handle_malloc},
  {"memset", Handle_memset},
  {"memcpy", Handle_memcpy},
  {"strlen", Handle_strlen},
  {"puts", Handle_puts},
  {"varAddRef", Handle_varAddRef},
  {"varRelease", Handle_varRelease},
  {"varFromUtf8", Handle_varFromUtf8},
  {"varToUtf8", Handle_varToUtf8},
  {"arrayCreate", Handle_arrayCreate},
  {"arrayGet", Handle_arrayGet},
  {"arraySet", Handle_arraySet},
  {"arrayGetLength", Handle_arrayGetLength},
  {"arraySetLength", Handle_arraySetLength},
  {"arrayBufferCreate", Handle_arrayBufferCreate},
  {"arrayBufferByteLength", Handle_arrayBufferByteLength},
  {"arrayBufferMap", Handle_arrayBufferMap},
  {"arrayBufferUnmap", Handle_arrayBufferUnmap},
  {"dictCreate", Handle_dictCreate},
  {"dictGet", Handle_dictGet},
  {"dictSet", Handle_dictSet},
  {"dictDelete", Handle_dictDelete},
  {"dictHasKey", Handle_dictHasKey},
  {NULL, NULL},
};

bool HandleBuiltinCommand(Command* command) {
  NameFunc* name_func = &g_FuncMap[0];
  for (; name_func->name; name_func++) {
    if (strcmp(name_func->name, command->command) == 0) {
      name_func->func(command);
      return TRUE;
    }
  }
  return FALSE;
}

void Handle_destroyHandles(Command* command) {
  int32_t index;
  int32_t handle_count = GetCommandArgCount(command);
  // TODO(binji): use malloc if handle_count is large?
  Handle* handles = alloca(handle_count * sizeof(Handle));
  for (index = 0; index < handle_count; ++index) {
    Arg* arg;
    if (!GetCommandArg(command, index, &arg)) {
      CMD_VERROR("Can't get arg %d", index);
      return;
    }

    if (!arg->is_handle) {
      CMD_VERROR("Expected arg %d to be a Handle", index);
      return;
    }

    int32_t arg_handle_int;
    if (!GetVarInt32(&arg->var, &arg_handle_int)) {
      CMD_VERROR("Expected handle arg %d to be int32_t", index);
      return;
    }

    Handle handle = arg_handle_int;
    handles[index] = handle;
  }

  DestroyHandles(handles, handle_count);
  printf("destroyHandles()\n");
}

void Handle_get(Command* command) {
  switch (command->type) {
    case TYPE_FUNC_GET_VOID_P: {
      ARG_VOIDP_CAST(0, void**);
      void* result = (void*)getVoidP(arg0);
      RegisterHandleVoidp(command->ret_handle, result);
      printf("get(%p) => %p (%d)\n", arg0, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GET_CHAR: {
      ARG_CHARP(0)
      char result = (char)getChar(arg0);
      RegisterHandleInt32(command->ret_handle, result);
      printf("get(%p) => %d (%d)\n", arg0, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GET_INT8: {
      ARG_VOIDP_CAST(0, int8_t*);
      int8_t result = (int8_t)getInt8(arg0);
      RegisterHandleInt32(command->ret_handle, result);
      printf("get(%p) => %d (%d)\n", arg0, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GET_UINT8: {
      ARG_VOIDP_CAST(0, uint8_t*);
      uint8_t result = (uint8_t)getUint8(arg0);
      RegisterHandleUint32(command->ret_handle, result);
      printf("get(%p) => %u (%d)\n", arg0, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GET_INT16: {
      ARG_VOIDP_CAST(0, int16_t*);
      int16_t result = (int16_t)getInt16(arg0);
      RegisterHandleInt32(command->ret_handle, result);
      printf("get(%p) => %d (%d)\n", arg0, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GET_UINT16: {
      ARG_VOIDP_CAST(0, uint16_t*);
      uint16_t result = (uint16_t)getUint16(arg0);
      RegisterHandleUint32(command->ret_handle, result);
      printf("get(%p) => %u (%d)\n", arg0, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GET_INT32: {
      ARG_VOIDP_CAST(0, int32_t*);
      int32_t result = (int32_t)getInt32(arg0);
      RegisterHandleInt32(command->ret_handle, result);
      printf("get(%p) => %d (%d)\n", arg0, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GET_UINT32: {
      ARG_VOIDP_CAST(0, uint32_t*);
      uint32_t result = (uint32_t)getUint32(arg0);
      RegisterHandleUint32(command->ret_handle, result);
      printf("get(%p) => %u (%d)\n", arg0, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GET_LONG: {
      ARG_VOIDP_CAST(0, long*);
      long result = (long)getLong(arg0);
      RegisterHandleInt32(command->ret_handle, result);
      printf("get(%p) => %ld (%d)\n", arg0, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GET_ULONG: {
      ARG_VOIDP_CAST(0, unsigned long*);
      unsigned long result = (unsigned long)getUlong(arg0);
      RegisterHandleUint32(command->ret_handle, result);
      printf("get(%p) => %lu (%d)\n", arg0, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GET_INT64: {
      ARG_VOIDP_CAST(0, int64_t*);
      int64_t result = (int64_t)getInt64(arg0);
      RegisterHandleInt32(command->ret_handle, result);
      printf("get(%p) => %lld (%d)\n", arg0, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GET_UINT64: {
      ARG_VOIDP_CAST(0, uint64_t*);
      uint64_t result = (uint64_t)getUint64(arg0);
      RegisterHandleUint32(command->ret_handle, result);
      printf("get(%p) => %llu (%d)\n", arg0, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GET_FLOAT32: {
      ARG_VOIDP_CAST(0, float*);
      float result = (float)getFloat32(arg0);
      RegisterHandleUint32(command->ret_handle, result);
      printf("get(%p) => %g (%d)\n", arg0, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GET_FLOAT64: {
      ARG_VOIDP_CAST(0, double*);
      double result = (double)getFloat64(arg0);
      RegisterHandleUint32(command->ret_handle, result);
      printf("get(%p) => %g (%d)\n", arg0, result, command->ret_handle);
      break;
    }
    default:
      TYPE_FAIL;
      break;
  }
}

void Handle_set(Command* command) {
  switch (command->type) {
    case TYPE_FUNC_SET_VOID_P: {
      ARG_VOIDP_CAST(0, void**);
      ARG_VOIDP(1);
      setVoidP(arg0, arg1);
      printf("set(%p, %p)\n", arg0, arg1);
      break;
    }
    case TYPE_FUNC_SET_CHAR: {
      ARG_CHARP(0)
      ARG_INT(1);
      setChar(arg0, arg1);
      printf("set(%p, %d)\n", arg0, arg1);
      break;
    }
    case TYPE_FUNC_SET_INT8: {
      ARG_VOIDP_CAST(0, int8_t*);
      ARG_INT(1);
      setInt8(arg0, arg1);
      printf("set(%p, %d)\n", arg0, arg1);
      break;
    }
    case TYPE_FUNC_SET_UINT8: {
      ARG_VOIDP_CAST(0, uint8_t*);
      ARG_UINT(1);
      setUint8(arg0, arg1);
      printf("set(%p, %u)\n", arg0, arg1);
      break;
    }
    case TYPE_FUNC_SET_INT16: {
      ARG_VOIDP_CAST(0, int16_t*);
      ARG_INT(1);
      setInt16(arg0, arg1);
      printf("set(%p, %d)\n", arg0, arg1);
      break;
    }
    case TYPE_FUNC_SET_UINT16: {
      ARG_VOIDP_CAST(0, uint16_t*);
      ARG_UINT(1);
      setUint16(arg0, arg1);
      printf("set(%p, %u)\n", arg0, arg1);
      break;
    }
    case TYPE_FUNC_SET_INT32: {
      ARG_VOIDP_CAST(0, int32_t*);
      ARG_INT(1);
      setInt32(arg0, arg1);
      printf("set(%p, %d)\n", arg0, arg1);
      break;
    }
    case TYPE_FUNC_SET_UINT32: {
      ARG_VOIDP_CAST(0, uint32_t*);
      ARG_UINT(1);
      setUint32(arg0, arg1);
      printf("set(%p, %u)\n", arg0, arg1);
      break;
    }
    case TYPE_FUNC_SET_LONG: {
      ARG_VOIDP_CAST(0, long*);
      ARG_INT_CAST(1, long);
      setLong(arg0, arg1);
      printf("set(%p, %ld)\n", arg0, arg1);
      break;
    }
    case TYPE_FUNC_SET_ULONG: {
      ARG_VOIDP_CAST(0, unsigned long*);
      ARG_UINT_CAST(1, unsigned long);
      setUlong(arg0, arg1);
      printf("set(%p, %lu)\n", arg0, arg1);
      break;
    }
    case TYPE_FUNC_SET_INT64: {
      ARG_VOIDP_CAST(0, int64_t*);
      ARG_INT64(1);
      setInt64(arg0, arg1);
      printf("set(%p, %lld)\n", arg0, arg1);
      break;
    }
    case TYPE_FUNC_SET_UINT64: {
      ARG_VOIDP_CAST(0, uint64_t*);
      ARG_UINT64(1);
      setUint64(arg0, arg1);
      printf("set(%p, %llu)\n", arg0, arg1);
      break;
    }
    case TYPE_FUNC_SET_FLOAT32: {
      ARG_VOIDP_CAST(0, float*);
      ARG_FLOAT32(1);
      setFloat32(arg0, arg1);
      printf("set(%p, %g)\n", arg0, arg1);
      break;
    }
    case TYPE_FUNC_SET_FLOAT64: {
      ARG_VOIDP_CAST(0, double*);
      ARG_FLOAT64(1);
      setFloat64(arg0, arg1);
      printf("set(%p, %g)\n", arg0, arg1);
      break;
    }
    default:
      TYPE_FAIL;
      break;
  }
}

void Handle_add(Command* command) {
  switch (command->type) {
    case TYPE_FUNC_ADD_VOID_P: {
      ARG_VOIDP(0);
      ARG_INT(1);
      void* result = (void*)addVoidP(arg0, arg1);
      RegisterHandleVoidp(command->ret_handle, result);
      printf("add(%p, %d) => %p (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_ADD_INT32: {
      ARG_INT(0);
      ARG_INT(1);
      int32_t result = (int32_t)addInt32(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("add(%d, %d) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_ADD_UINT32: {
      ARG_UINT(0);
      ARG_UINT(1);
      uint32_t result = (uint32_t)addUint32(arg0, arg1);
      RegisterHandleUint32(command->ret_handle, result);
      printf("add(%u, %u) => %u (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_ADD_INT64: {
      ARG_INT64(0);
      ARG_INT64(1);
      int64_t result = (int64_t)addInt64(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("add(%lld, %lld) => %lld (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_ADD_UINT64: {
      ARG_UINT64(0);
      ARG_UINT64(1);
      uint64_t result = (uint64_t)addUint64(arg0, arg1);
      RegisterHandleUint32(command->ret_handle, result);
      printf("add(%llu, %llu) => %llu (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_ADD_FLOAT32: {
      ARG_FLOAT32(0);
      ARG_FLOAT32(1);
      float result = (float)addFloat32(arg0, arg1);
      RegisterHandleUint32(command->ret_handle, result);
      printf("add(%g, %g) => %g (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_ADD_FLOAT64: {
      ARG_FLOAT64(0);
      ARG_FLOAT64(1);
      double result = (double)addFloat64(arg0, arg1);
      RegisterHandleUint32(command->ret_handle, result);
      printf("add(%g, %g) => %g (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    default:
      TYPE_FAIL;
      break;
  }
}

void Handle_sub(Command* command) {
  switch (command->type) {
    case TYPE_FUNC_SUB_VOID_P: {
      ARG_VOIDP(0);
      ARG_INT(1);
      void* result = (void*)subVoidP(arg0, arg1);
      RegisterHandleVoidp(command->ret_handle, result);
      printf("sub(%p, %d) => %p (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_SUB_INT32: {
      ARG_INT(0);
      ARG_INT(1);
      int32_t result = (int32_t)subInt32(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("sub(%d, %d) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_SUB_UINT32: {
      ARG_UINT(0);
      ARG_UINT(1);
      uint32_t result = (uint32_t)subUint32(arg0, arg1);
      RegisterHandleUint32(command->ret_handle, result);
      printf("sub(%u, %u) => %u (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_SUB_INT64: {
      ARG_INT64(0);
      ARG_INT64(1);
      int64_t result = (int64_t)subInt64(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("sub(%lld, %lld) => %lld (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_SUB_UINT64: {
      ARG_UINT64(0);
      ARG_UINT64(1);
      uint64_t result = (uint64_t)subUint64(arg0, arg1);
      RegisterHandleUint32(command->ret_handle, result);
      printf("sub(%llu, %llu) => %llu (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_SUB_FLOAT32: {
      ARG_FLOAT32(0);
      ARG_FLOAT32(1);
      float result = (float)subFloat32(arg0, arg1);
      RegisterHandleUint32(command->ret_handle, result);
      printf("sub(%g, %g) => %g (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_SUB_FLOAT64: {
      ARG_FLOAT64(0);
      ARG_FLOAT64(1);
      double result = (double)subFloat64(arg0, arg1);
      RegisterHandleUint32(command->ret_handle, result);
      printf("sub(%g, %g) => %g (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    default:
      TYPE_FAIL;
      break;
  }
}

void Handle_lt(Command* command) {
  switch (command->type) {
    case TYPE_FUNC_LT_VOID_P: {
      ARG_VOIDP(0);
      ARG_VOIDP(1);
      int result = (int)ltVoidP(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("lt(%p, %p) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_LT_CHAR: {
      ARG_INT(0);
      ARG_INT(1);
      int result = (int)ltChar(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("lt(%d, %d) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_LT_INT8: {
      ARG_INT(0);
      ARG_INT(1);
      int result = (int)ltInt8(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("lt(%d, %d) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_LT_UINT8: {
      ARG_UINT(0);
      ARG_UINT(1);
      int result = (int)ltUint8(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("lt(%u, %u) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_LT_INT16: {
      ARG_INT(0);
      ARG_INT(1);
      int result = (int)ltInt16(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("lt(%d, %d) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_LT_UINT16: {
      ARG_UINT(0);
      ARG_UINT(1);
      int result = (int)ltUint16(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("lt(%u, %u) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_LT_INT32: {
      ARG_INT(0);
      ARG_INT(1);
      int32_t result = (int32_t)ltInt32(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("lt(%d, %d) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_LT_UINT32: {
      ARG_UINT(0);
      ARG_UINT(1);
      int result = (int)ltUint32(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("lt(%u, %u) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_LT_LONG: {
      ARG_INT_CAST(0, long);
      ARG_INT_CAST(1, long);
      int result = (int)ltLong(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("lt(%ld, %ld) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_LT_ULONG: {
      ARG_UINT_CAST(0, unsigned long);
      ARG_UINT_CAST(1, unsigned long);
      int result = (int)ltUlong(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("lt(%lu, %lu) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_LT_INT64: {
      ARG_INT64(0);
      ARG_INT64(1);
      int result = (int)ltInt64(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("lt(%lld, %lld) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_LT_UINT64: {
      ARG_UINT64(0);
      ARG_UINT64(1);
      int result = (int)ltUint64(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("lt(%llu, %llu) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_LT_FLOAT32: {
      ARG_FLOAT32(0);
      ARG_FLOAT32(1);
      int result = (int)ltFloat32(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("lt(%g, %g) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_LT_FLOAT64: {
      ARG_FLOAT64(0);
      ARG_FLOAT64(1);
      int result = (int)ltFloat64(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("lt(%g, %g) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    default:
      TYPE_FAIL;
      break;
  }
}

void Handle_lte(Command* command) {
  switch (command->type) {
    case TYPE_FUNC_LTE_VOID_P: {
      ARG_VOIDP(0);
      ARG_VOIDP(1);
      int result = (int)lteVoidP(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("lte(%p, %p) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_LTE_CHAR: {
      ARG_INT(0);
      ARG_INT(1);
      int result = (int)lteChar(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("lte(%d, %d) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_LTE_INT8: {
      ARG_INT(0);
      ARG_INT(1);
      int result = (int)lteInt8(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("lte(%d, %d) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_LTE_UINT8: {
      ARG_UINT(0);
      ARG_UINT(1);
      int result = (int)lteUint8(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("lte(%u, %u) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_LTE_INT16: {
      ARG_INT(0);
      ARG_INT(1);
      int result = (int)lteInt16(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("lte(%d, %d) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_LTE_UINT16: {
      ARG_UINT(0);
      ARG_UINT(1);
      int result = (int)lteUint16(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("lte(%u, %u) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_LTE_INT32: {
      ARG_INT(0);
      ARG_INT(1);
      int32_t result = (int32_t)lteInt32(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("lte(%d, %d) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_LTE_UINT32: {
      ARG_UINT(0);
      ARG_UINT(1);
      int result = (int)lteUint32(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("lte(%u, %u) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_LTE_LONG: {
      ARG_INT_CAST(0, long);
      ARG_INT_CAST(1, long);
      int result = (int)lteLong(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("lte(%ld, %ld) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_LTE_ULONG: {
      ARG_UINT_CAST(0, unsigned long);
      ARG_UINT_CAST(1, unsigned long);
      int result = (int)lteUlong(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("lte(%lu, %lu) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_LTE_INT64: {
      ARG_INT64(0);
      ARG_INT64(1);
      int result = (int)lteInt64(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("lte(%lld, %lld) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_LTE_UINT64: {
      ARG_UINT64(0);
      ARG_UINT64(1);
      int result = (int)lteUint64(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("lte(%llu, %llu) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_LTE_FLOAT32: {
      ARG_FLOAT32(0);
      ARG_FLOAT32(1);
      int result = (int)lteFloat32(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("lte(%g, %g) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_LTE_FLOAT64: {
      ARG_FLOAT64(0);
      ARG_FLOAT64(1);
      int result = (int)lteFloat64(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("lte(%g, %g) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    default:
      TYPE_FAIL;
      break;
  }
}

void Handle_gt(Command* command) {
  switch (command->type) {
    case TYPE_FUNC_GT_VOID_P: {
      ARG_VOIDP(0);
      ARG_VOIDP(1);
      int result = (int)gtVoidP(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("gt(%p, %p) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GT_CHAR: {
      ARG_INT(0);
      ARG_INT(1);
      int result = (int)gtChar(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("gt(%d, %d) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GT_INT8: {
      ARG_INT(0);
      ARG_INT(1);
      int result = (int)gtInt8(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("gt(%d, %d) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GT_UINT8: {
      ARG_UINT(0);
      ARG_UINT(1);
      int result = (int)gtUint8(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("gt(%u, %u) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GT_INT16: {
      ARG_INT(0);
      ARG_INT(1);
      int result = (int)gtInt16(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("gt(%d, %d) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GT_UINT16: {
      ARG_UINT(0);
      ARG_UINT(1);
      int result = (int)gtUint16(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("gt(%u, %u) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GT_INT32: {
      ARG_INT(0);
      ARG_INT(1);
      int32_t result = (int32_t)gtInt32(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("gt(%d, %d) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GT_UINT32: {
      ARG_UINT(0);
      ARG_UINT(1);
      int result = (int)gtUint32(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("gt(%u, %u) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GT_LONG: {
      ARG_INT_CAST(0, long);
      ARG_INT_CAST(1, long);
      int result = (int)gtLong(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("gt(%ld, %ld) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GT_ULONG: {
      ARG_UINT_CAST(0, unsigned long);
      ARG_UINT_CAST(1, unsigned long);
      int result = (int)gtUlong(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("gt(%lu, %lu) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GT_INT64: {
      ARG_INT64(0);
      ARG_INT64(1);
      int result = (int)gtInt64(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("gt(%lld, %lld) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GT_UINT64: {
      ARG_UINT64(0);
      ARG_UINT64(1);
      int result = (int)gtUint64(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("gt(%llu, %llu) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GT_FLOAT32: {
      ARG_FLOAT32(0);
      ARG_FLOAT32(1);
      int result = (int)gtFloat32(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("gt(%g, %g) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GT_FLOAT64: {
      ARG_FLOAT64(0);
      ARG_FLOAT64(1);
      int result = (int)gtFloat64(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("gt(%g, %g) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    default:
      TYPE_FAIL;
      break;
  }
}

void Handle_gte(Command* command) {
  switch (command->type) {
    case TYPE_FUNC_GTE_VOID_P: {
      ARG_VOIDP(0);
      ARG_VOIDP(1);
      int result = (int)gteVoidP(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("gte(%p, %p) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GTE_CHAR: {
      ARG_INT(0);
      ARG_INT(1);
      int result = (int)gteChar(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("gte(%d, %d) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GTE_INT8: {
      ARG_INT(0);
      ARG_INT(1);
      int result = (int)gteInt8(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("gte(%d, %d) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GTE_UINT8: {
      ARG_UINT(0);
      ARG_UINT(1);
      int result = (int)gteUint8(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("gte(%u, %u) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GTE_INT16: {
      ARG_INT(0);
      ARG_INT(1);
      int result = (int)gteInt16(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("gte(%d, %d) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GTE_UINT16: {
      ARG_UINT(0);
      ARG_UINT(1);
      int result = (int)gteUint16(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("gte(%u, %u) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GTE_INT32: {
      ARG_INT(0);
      ARG_INT(1);
      int32_t result = (int32_t)gteInt32(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("gte(%d, %d) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GTE_UINT32: {
      ARG_UINT(0);
      ARG_UINT(1);
      int result = (int)gteUint32(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("gte(%u, %u) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GTE_LONG: {
      ARG_INT_CAST(0, long);
      ARG_INT_CAST(1, long);
      int result = (int)gteLong(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("gte(%ld, %ld) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GTE_ULONG: {
      ARG_UINT_CAST(0, unsigned long);
      ARG_UINT_CAST(1, unsigned long);
      int result = (int)gteUlong(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("gte(%lu, %lu) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GTE_INT64: {
      ARG_INT64(0);
      ARG_INT64(1);
      int result = (int)gteInt64(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("gte(%lld, %lld) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GTE_UINT64: {
      ARG_UINT64(0);
      ARG_UINT64(1);
      int result = (int)gteUint64(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("gte(%llu, %llu) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GTE_FLOAT32: {
      ARG_FLOAT32(0);
      ARG_FLOAT32(1);
      int result = (int)gteFloat32(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("gte(%g, %g) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_GTE_FLOAT64: {
      ARG_FLOAT64(0);
      ARG_FLOAT64(1);
      int result = (int)gteFloat64(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("gte(%g, %g) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    default:
      TYPE_FAIL;
      break;
  }
}

void Handle_eq(Command* command) {
  switch (command->type) {
    case TYPE_FUNC_EQ_VOID_P: {
      ARG_VOIDP(0);
      ARG_VOIDP(1);
      int result = (int)eqVoidP(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("eq(%p, %p) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_EQ_CHAR: {
      ARG_INT(0);
      ARG_INT(1);
      int result = (int)eqChar(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("eq(%d, %d) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_EQ_INT8: {
      ARG_INT(0);
      ARG_INT(1);
      int result = (int)eqInt8(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("eq(%d, %d) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_EQ_UINT8: {
      ARG_UINT(0);
      ARG_UINT(1);
      int result = (int)eqUint8(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("eq(%u, %u) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_EQ_INT16: {
      ARG_INT(0);
      ARG_INT(1);
      int result = (int)eqInt16(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("eq(%d, %d) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_EQ_UINT16: {
      ARG_UINT(0);
      ARG_UINT(1);
      int result = (int)eqUint16(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("eq(%u, %u) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_EQ_INT32: {
      ARG_INT(0);
      ARG_INT(1);
      int32_t result = (int32_t)eqInt32(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("eq(%d, %d) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_EQ_UINT32: {
      ARG_UINT(0);
      ARG_UINT(1);
      int result = (int)eqUint32(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("eq(%u, %u) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_EQ_LONG: {
      ARG_INT_CAST(0, long);
      ARG_INT_CAST(1, long);
      int result = (int)eqLong(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("eq(%ld, %ld) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_EQ_ULONG: {
      ARG_UINT_CAST(0, unsigned long);
      ARG_UINT_CAST(1, unsigned long);
      int result = (int)eqUlong(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("eq(%lu, %lu) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_EQ_INT64: {
      ARG_INT64(0);
      ARG_INT64(1);
      int result = (int)eqInt64(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("eq(%lld, %lld) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_EQ_UINT64: {
      ARG_UINT64(0);
      ARG_UINT64(1);
      int result = (int)eqUint64(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("eq(%llu, %llu) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_EQ_FLOAT32: {
      ARG_FLOAT32(0);
      ARG_FLOAT32(1);
      int result = (int)eqFloat32(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("eq(%g, %g) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_EQ_FLOAT64: {
      ARG_FLOAT64(0);
      ARG_FLOAT64(1);
      int result = (int)eqFloat64(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("eq(%g, %g) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    default:
      TYPE_FAIL;
      break;
  }
}

void Handle_ne(Command* command) {
  switch (command->type) {
    case TYPE_FUNC_NE_VOID_P: {
      ARG_VOIDP(0);
      ARG_VOIDP(1);
      int result = (int)neVoidP(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("ne(%p, %p) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_NE_CHAR: {
      ARG_INT(0);
      ARG_INT(1);
      int result = (int)neChar(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("ne(%d, %d) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_NE_INT8: {
      ARG_INT(0);
      ARG_INT(1);
      int result = (int)neInt8(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("ne(%d, %d) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_NE_UINT8: {
      ARG_UINT(0);
      ARG_UINT(1);
      int result = (int)neUint8(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("ne(%u, %u) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_NE_INT16: {
      ARG_INT(0);
      ARG_INT(1);
      int result = (int)neInt16(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("ne(%d, %d) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_NE_UINT16: {
      ARG_UINT(0);
      ARG_UINT(1);
      int result = (int)neUint16(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("ne(%u, %u) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_NE_INT32: {
      ARG_INT(0);
      ARG_INT(1);
      int32_t result = (int32_t)neInt32(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("ne(%d, %d) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_NE_UINT32: {
      ARG_UINT(0);
      ARG_UINT(1);
      int result = (int)neUint32(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("ne(%u, %u) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_NE_LONG: {
      ARG_INT_CAST(0, long);
      ARG_INT_CAST(1, long);
      int result = (int)neLong(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("ne(%ld, %ld) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_NE_ULONG: {
      ARG_UINT_CAST(0, unsigned long);
      ARG_UINT_CAST(1, unsigned long);
      int result = (int)neUlong(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("ne(%lu, %lu) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_NE_INT64: {
      ARG_INT64(0);
      ARG_INT64(1);
      int result = (int)neInt64(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("ne(%lld, %lld) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_NE_UINT64: {
      ARG_UINT64(0);
      ARG_UINT64(1);
      int result = (int)neUint64(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("ne(%llu, %llu) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_NE_FLOAT32: {
      ARG_FLOAT32(0);
      ARG_FLOAT32(1);
      int result = (int)neFloat32(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("ne(%g, %g) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    case TYPE_FUNC_NE_FLOAT64: {
      ARG_FLOAT64(0);
      ARG_FLOAT64(1);
      int result = (int)neFloat64(arg0, arg1);
      RegisterHandleInt32(command->ret_handle, result);
      printf("ne(%g, %g) => %d (%d)\n", arg0, arg1, result, command->ret_handle);
      break;
    }
    default:
      TYPE_FAIL;
      break;
  }
}

void Handle_free(Command* command) {
  TYPE_CHECK(TYPE_FUNC_FREE);
  ARG_VOIDP(0);
  free(arg0);
  printf("free(%p)\n", arg0);
}

void Handle_malloc(Command* command) {
  TYPE_CHECK(TYPE_FUNC_MALLOC);
  ARG_UINT(0);
  void* result = (void*)malloc(arg0);
  RegisterHandleVoidp(command->ret_handle, result);
  printf("malloc(%u) => %p (%d)\n", arg0, result, command->ret_handle);
}

void Handle_memset(Command* command) {
  TYPE_CHECK(TYPE_FUNC_MEMSET);
  ARG_VOIDP(0);
  ARG_INT(1);
  ARG_UINT(2);
  void* result = (void*)memset(arg0, arg1, arg2);
  RegisterHandleVoidp(command->ret_handle, result);
  printf("memset(%p, %d, %u) => %p (%d)\n", arg0, arg1, arg2, result, command->ret_handle);
}

void Handle_memcpy(Command* command) {
  TYPE_CHECK(TYPE_FUNC_MEMCPY);
  ARG_VOIDP(0);
  ARG_VOIDP(1);
  ARG_UINT(2);
  void* result = (void*)memcpy(arg0, arg1, arg2);
  RegisterHandleVoidp(command->ret_handle, result);
  printf("memcpy(%p, %p, %u) => %p (%d)\n", arg0, arg1, arg2, result, command->ret_handle);
}

void Handle_strlen(Command* command) {
  TYPE_CHECK(TYPE_FUNC_STRLEN);
  ARG_CHARP(0)
  size_t result = (size_t)strlen(arg0);
  RegisterHandleUint32(command->ret_handle, result);
  printf("strlen(%p) => %u (%d)\n", arg0, result, command->ret_handle);
}

void Handle_puts(Command* command) {
  TYPE_CHECK(TYPE_FUNC_PUTS);
  ARG_CHARP(0)
  int result = (int)puts(arg0);
  RegisterHandleInt32(command->ret_handle, result);
  printf("puts(%p) => %d (%d)\n", arg0, result, command->ret_handle);
}

void Handle_varAddRef(Command* command) {
  TYPE_CHECK(TYPE_FUNC_VAR_ADD_REF);
  ARG_VAR(0);
  varAddRef(arg0);
  printf("varAddRef(%s)\n", VarTypeToString(&arg0));
}

void Handle_varRelease(Command* command) {
  TYPE_CHECK(TYPE_FUNC_VAR_RELEASE);
  ARG_VAR(0);
  varRelease(arg0);
  printf("varRelease(%s)\n", VarTypeToString(&arg0));
}

void Handle_varFromUtf8(Command* command) {
  TYPE_CHECK(TYPE_FUNC_VAR_FROM_UTF8);
  ARG_CHARP(0)
  ARG_UINT(1);
  struct PP_Var result = (struct PP_Var)varFromUtf8(arg0, arg1);
  RegisterHandleVar(command->ret_handle, result);
  printf("varFromUtf8(%p, %u) => %s (%d)\n", arg0, arg1, VarTypeToString(&result), command->ret_handle);
}

void Handle_varToUtf8(Command* command) {
  TYPE_CHECK(TYPE_FUNC_VAR_TO_UTF8);
  ARG_VAR(0);
  ARG_VOIDP_CAST(1, uint32_t*);
  char* result = (char*)varToUtf8(arg0, arg1);
  RegisterHandleVoidp(command->ret_handle, result);
  printf("varToUtf8(%s, %p) => %p (%d)\n", VarTypeToString(&arg0), arg1, result, command->ret_handle);
}

void Handle_arrayCreate(Command* command) {
  TYPE_CHECK(TYPE_FUNC_ARRAY_CREATE);
  struct PP_Var result = (struct PP_Var)arrayCreate();
  RegisterHandleVar(command->ret_handle, result);
  printf("arrayCreate() => %s (%d)\n", VarTypeToString(&result), command->ret_handle);
}

void Handle_arrayGet(Command* command) {
  TYPE_CHECK(TYPE_FUNC_ARRAY_GET);
  ARG_VAR(0);
  ARG_UINT(1);
  struct PP_Var result = (struct PP_Var)arrayGet(arg0, arg1);
  RegisterHandleVar(command->ret_handle, result);
  printf("arrayGet(%s, %u) => %s (%d)\n", VarTypeToString(&arg0), arg1, VarTypeToString(&result), command->ret_handle);
}

void Handle_arraySet(Command* command) {
  TYPE_CHECK(TYPE_FUNC_ARRAY_SET);
  ARG_VAR(0);
  ARG_UINT(1);
  ARG_VAR(2);
  int32_t result = (int32_t)arraySet(arg0, arg1, arg2);
  RegisterHandleInt32(command->ret_handle, result);
  printf("arraySet(%s, %u, %s) => %d (%d)\n", VarTypeToString(&arg0), arg1, VarTypeToString(&arg2), result, command->ret_handle);
}

void Handle_arrayGetLength(Command* command) {
  TYPE_CHECK(TYPE_FUNC_ARRAY_GET_LENGTH);
  ARG_VAR(0);
  uint32_t result = (uint32_t)arrayGetLength(arg0);
  RegisterHandleUint32(command->ret_handle, result);
  printf("arrayGetLength(%s) => %u (%d)\n", VarTypeToString(&arg0), result, command->ret_handle);
}

void Handle_arraySetLength(Command* command) {
  TYPE_CHECK(TYPE_FUNC_ARRAY_SET_LENGTH);
  ARG_VAR(0);
  ARG_UINT(1);
  int32_t result = (int32_t)arraySetLength(arg0, arg1);
  RegisterHandleInt32(command->ret_handle, result);
  printf("arraySetLength(%s, %u) => %d (%d)\n", VarTypeToString(&arg0), arg1, result, command->ret_handle);
}

void Handle_arrayBufferCreate(Command* command) {
  TYPE_CHECK(TYPE_FUNC_ARRAY_BUFFER_CREATE);
  ARG_UINT(0);
  struct PP_Var result = (struct PP_Var)arrayBufferCreate(arg0);
  RegisterHandleVar(command->ret_handle, result);
  printf("arrayBufferCreate(%u) => %s (%d)\n", arg0, VarTypeToString(&result), command->ret_handle);
}

void Handle_arrayBufferByteLength(Command* command) {
  TYPE_CHECK(TYPE_FUNC_ARRAY_BUFFER_BYTE_LENGTH);
  ARG_VAR(0);
  ARG_VOIDP_CAST(1, uint32_t*);
  int32_t result = (int32_t)arrayBufferByteLength(arg0, arg1);
  RegisterHandleInt32(command->ret_handle, result);
  printf("arrayBufferByteLength(%s, %p) => %d (%d)\n", VarTypeToString(&arg0), arg1, result, command->ret_handle);
}

void Handle_arrayBufferMap(Command* command) {
  TYPE_CHECK(TYPE_FUNC_ARRAY_BUFFER_MAP);
  ARG_VAR(0);
  void* result = (void*)arrayBufferMap(arg0);
  RegisterHandleVoidp(command->ret_handle, result);
  printf("arrayBufferMap(%s) => %p (%d)\n", VarTypeToString(&arg0), result, command->ret_handle);
}

void Handle_arrayBufferUnmap(Command* command) {
  TYPE_CHECK(TYPE_FUNC_ARRAY_BUFFER_UNMAP);
  ARG_VAR(0);
  arrayBufferUnmap(arg0);
  printf("arrayBufferUnmap(%s)\n", VarTypeToString(&arg0));
}

void Handle_dictCreate(Command* command) {
  TYPE_CHECK(TYPE_FUNC_DICT_CREATE);
  struct PP_Var result = (struct PP_Var)dictCreate();
  RegisterHandleVar(command->ret_handle, result);
  printf("dictCreate() => %s (%d)\n", VarTypeToString(&result), command->ret_handle);
}

void Handle_dictGet(Command* command) {
  TYPE_CHECK(TYPE_FUNC_DICT_GET);
  ARG_VAR(0);
  ARG_VAR(1);
  struct PP_Var result = (struct PP_Var)dictGet(arg0, arg1);
  RegisterHandleVar(command->ret_handle, result);
  printf("dictGet(%s, %s) => %s (%d)\n", VarTypeToString(&arg0), VarTypeToString(&arg1), VarTypeToString(&result), command->ret_handle);
}

void Handle_dictSet(Command* command) {
  TYPE_CHECK(TYPE_FUNC_DICT_SET);
  ARG_VAR(0);
  ARG_VAR(1);
  ARG_VAR(2);
  int32_t result = (int32_t)dictSet(arg0, arg1, arg2);
  RegisterHandleInt32(command->ret_handle, result);
  printf("dictSet(%s, %s, %s) => %d (%d)\n", VarTypeToString(&arg0), VarTypeToString(&arg1), VarTypeToString(&arg2), result, command->ret_handle);
}

void Handle_dictDelete(Command* command) {
  TYPE_CHECK(TYPE_FUNC_DICT_DELETE);
  ARG_VAR(0);
  ARG_VAR(1);
  dictDelete(arg0, arg1);
  printf("dictDelete(%s, %s)\n", VarTypeToString(&arg0), VarTypeToString(&arg1));
}

void Handle_dictHasKey(Command* command) {
  TYPE_CHECK(TYPE_FUNC_DICT_HAS_KEY);
  ARG_VAR(0);
  ARG_VAR(1);
  int32_t result = (int32_t)dictHasKey(arg0, arg1);
  RegisterHandleInt32(command->ret_handle, result);
  printf("dictHasKey(%s, %s) => %d (%d)\n", VarTypeToString(&arg0), VarTypeToString(&arg1), result, command->ret_handle);
}


