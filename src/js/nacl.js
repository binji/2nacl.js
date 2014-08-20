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

'use strict';

var builtin = require('./builtin');

function assert(cond, msg) {
  if (!cond) {
    if (typeof msg === 'function') {
      msg = msg();
    }
    throw new Error('Assertion failed' + (msg ? (': ' + msg) : '.'));
  }
}

// EmbedElement //////////////////////////////////////////////////////////////
function EmbedElement(nmf, mimeType) {
  this.element = document.createElement('embed');
  this.element.setAttribute('width', '0');
  this.element.setAttribute('height', '0');
  this.element.setAttribute('src', this.nmf);
  this.element.setAttribute('type', this.mimeType);
}

EmbedElement.prototype.addEventListener_ = function(message, callback) {
  this.element.addEventListener(message, callback, false);
};

EmbedElement.prototype.addLoadListener = function(callback) {
  this.addEventListener_('load', callback);
};

EmbedElement.prototype.addMessageListener = function(callback) {
  this.addEventListener_('message', callback);
};

EmbedElement.prototype.addErrorListener = function(callback) {
  this.addEventListener_('error', callback);
};

EmbedElement.prototype.addCrashListener = function(callback) {
  this.addEventListener_('crash', callback);
};

EmbedElement.prototype.appendToBody = function() {
  document.body.appendChild(this.element);
};

EmbedElement.prototype.postMessage = function(msg) {
  this.element.postMessage(msg);
};

Object.defineProperty(EmbedElement.prototype, 'lastError', {
  get: function() { return this.element.lastError; },
  enumerable: true
});

Object.defineProperty(EmbedElement.prototype, 'exitStatus', {
  get: function() { return this.element.exitStatus; },
  enumerable: true
});


// Module ////////////////////////////////////////////////////////////////////
/*
 * opts:
 *   name: Name of the module used for error reporting.
 *   embedElementConstructor: Constructor to call when creating the embed
 *      element. If undefined, this will default to using EmbedElement defined
 *      above.
 *   log: Logging function. If undefined, this will use console.log.
 */
function Module(nmf, mimeType, opts) {
  if (!(this instanceof Module)) {
    return new Module(nmf, mimeType, opts);
  }

  // Allow overriding for testing.
  var embedConstructor = opts.embedElementConstructor || EmbedElement;
  this.log_ = opts.log || console.log.bind(console);

  // private
  this.loaded_ = false;
  this.nextCallbackId_ = 1;
  this.idCallbackMap_ = [];
  this.queuedMessages_ = [];
  this.commands_ = [];
  this.typeBuilder_ = new TypeBuilder();
  this.functionBuilder_ = new FunctionBuilder();
  this.handles_ = new HandleList();

  // public
  this.name = opts.name;
  this.element = null;
  this.types = this.typeBuilder_.getNameHash();
  this.functions = this.functionBuilder_.getNameHash();

  this.initBuiltins_();
  this.createEmbed_(nmf, mimeType, embedConstructor);
}

Module.prototype.createEmbed_ = function(nmf, mimeType, embedConstructor) {
  this.element = new embedConstructor(nmf, mimeType);
  this.element.addLoadListener(this.onLoad_.bind(this));
  this.element.addMessageListener(this.onMessage_.bind(this));
  this.element.addErrorListener(this.onError_.bind(this));
  this.element.addCrashListener(this.onCrash_.bind(this));
  this.element.appendToBody();
};

Module.prototype.postMessage_ = function(msg, callback) {
  var id = this.nextCallbackId_++;
  this.idCallbackMap_[id] = callback;

  msg.id = id;
  if (this.loaded_) {
    this.element.postMessage(msg);
  } else {
    this.queuedMessages_.push(msg);
  }
  return id;
};

Module.prototype.makeContext = function() {
  return new Context(this);
};

Module.prototype.onLoad_ = function(event) {
  this.log_('module loaded.');
  this.loaded_ = true;
  this.postQueuedMessages_();
};

Module.prototype.onMessage_ = function(event) {
  var msg = event.data;
  if (typeof(msg) !== 'object') {
    msg = this.name_ + ': unexpected value from module: ' + JSON.stringify(msg);
    throw new Error(msg);
  }

  if (msg.msg) {
    this.log_(msg.msg);
    return;
  }

  var id = msg.id;
  if (id !== 0) {
    var callback = this.idCallbackMap_[id];
    callback(msg);
    delete this.idCallbackMap_[id];
  }
};

Module.prototype.onError_ = function(event) {
  var msg = this.name_ + ': error loading NaCl module: ' +
            this.element.lastError;
  throw new Error(msg);
};

Module.prototype.onCrash_ = function(event) {
  var msg = this.name_ + ': NaCl module crashed: ' + this.element.exitStatus;
  throw new Error(msg);
};

Module.prototype.postQueuedMessages_ = function() {
  var that = this;
  this.queuedMessages_.forEach(function(msg) {
    that.element.postMessage(msg);
  });

  this.queuedMessages_ = null;
};

Module.prototype.callFunction_ = function(context, func, args) {
  assert(func instanceof CFunction, 'expected func to be CFunction');

  var funcType = this.findOverload_(func.name, args, func.overloads);
  assert(funcType !== null);

  var retHandle = null;
  var retHandleId = 0;
  if (funcType.data.retType !== this.types.void) {
    retHandle = this.handles_.makeHandle(context, funcType.data.retType);
    retHandleId = retHandle.id;
  }

  this.pushCommand_(func.name, funcType.id, args, retHandleId);

  return retHandle;
};

Module.prototype.pushCommand_ = function(cmd, type, args, ret) {
  var message = {
    cmd: cmd,
    type: type,
    args: [],
    argIsHandle: [],
    ret: ret,
  };
  for (var i = 0; i < args.length; ++i) {
    var arg = args[i];
    var value;
    if (arg instanceof Handle) {
      value = arg.id;
    } else {
      value = arg;
    }
    message.args.push(value);
    message.argIsHandle.push(arg instanceof Handle);
  }
  this.commands_.push(message);
};


Module.prototype.commit = function() {
  var that = this;
  assert(arguments.length >= 1, 'expected callback.');

  var callback = arguments[arguments.length - 1];
  assert(callback instanceof Function, 'callback is not Function.');

  var args = Array.prototype.slice.call(arguments, 0, -1);

  var serializeHandle = function(handle) {
    assert(handle instanceof Handle, 'handle is not a Handle.');
    return handle.id;
  };

  var handles = args.map(serializeHandle);
  var msg = {
    commands: this.commands_,
    handles: handles,
  };

  // Remove committed commands.
  this.commands_ = [];

  this.postMessage_(msg, function(result) {
    function idToHandle(value, ix) {
      if (typeof(value) === 'number' && !args[ix].type.isPrimitive()) {
        return that.handles_.getHandle(value);
      }
      return value;
    }

    // convert back from Ids to Handles; keep the primitive values the same.
    var handles = Array.prototype.map.call(result.values, idToHandle);

    // node.js-style callback. First param is error, second param is result.
    callback(null, handles);
  });
};

/*
// Removed because this functionality can be reproduced by .spread(...)

Module.prototype.commitPromise = function() {
  var that = this;
  var args = Array.prototype.slice.call(arguments);
  return new promise.PromisePlus(function(resolve, reject, resolveMany) {
    args.push(function() {
      resolveMany.apply(null, arguments);
    });
    that.commit.apply(that, args);
  });
};
*/

Module.prototype.initBuiltins_ = function() {
  builtin.init(this);
};

Module.prototype.makePointerType = function(id, name, baseType) {
  return this.typeBuilder_.makePointerType.apply(this.typeBuilder_, arguments);
};

Module.prototype.makeStructType = function(id, size, name, fields) {
  return this.typeBuilder_.makeStructType.apply(this.typeBuilder_, arguments);
};

Module.prototype.makeAliasType = function(name, alias) {
  return this.typeBuilder_.makeAliasType.apply(this.typeBuilder_, arguments);
};

Module.prototype.makeFunctionType = function(id, retType) {
  return this.typeBuilder_.makeFunctionType.apply(this.typeBuilder_, arguments);
};

Module.prototype.makeFunction = function(name, overloads) {
  return this.functionBuilder_.makeFunction.apply(this.functionBuilder_,
                                                  arguments);
};

Module.prototype.getPointerType = function(type) {
  return this.typeBuilder_.getPointerType(type);
};

Module.prototype.destroyHandles = function(handles) {
  this.handles_.runFinalizers(handles);
  this.pushCommand_('*destroyHandles', 0, handles, 0);
  this.handles_.destroyHandles(handles);
};

Module.prototype.findOverload_ = function(funcName, args, funcTypeList) {
  var errorMessages = [],
      i;
  for (i = 0; i < funcTypeList.length; ++i) {
    try {
      if (this.overloadMatches_(funcTypeList[i], args)) {
        return funcTypeList[i];
      }
    } catch(e) {
      errorMessages.push(e.message);
    }
  }

  // Display helpful warning.
  var msg;
  msg = 'No overload found for call "' + funcName + '(';
  msg += Array.prototype.join.call(args, ', ');
  msg += ')".\n';
  msg += 'Possibilities:\n';
  for (i = 0; i < funcTypeList.length; ++i) {
    msg += funcTypeList[i].toString() + '\n';
    msg += '  ' + errorMessages[i] + '\n';
  }
  this.log_(msg);

  return null;
};

Module.prototype.overloadMatches_ = function(funcType, args) {
  if (funcType.argTypes.length !== args.length) {
    throw new Error('Mismatched argument length: ' +
        funcType.argTypes.length + ' != ' + args.length);
  }

  for (var i = 0; i < funcType.argTypes.length; ++i) {
    var arg = args[i];
    var funcArgType = funcType.argTypes[i];
    var argType;
    if (arg instanceof Handle) {
      argType = arg.type;
    } else if (typeof(arg) === 'number') {
      if (Math.floor(arg) === arg) {
        // Int.
        argType = this.getIntType_(arg);
      } else {
        // Float.
        argType = this.types.float64;
      }
    } else if (arg === null) {
      argType = this.types.void$;
    } else if (typeof(arg) === 'string') {
      argType = this.types.String;
    } else if (arg instanceof ArrayBuffer) {
      argType = this.types.ArrayBuffer;
    } else if (arg instanceof Array) {
      argType = this.types.Array;
    } else if (arg instanceof Object) {
      argType = this.types.Dictionary;
    } else if (arg === undefined) {
      throw new Error('Arg #' + i + ' is undefined.');
    } else {
      // TODO(binji): handle other pepper types.
      // What kind of type is this?
      throw new Error('Unexpected type of arg #' + i + ' "' + arg + '": ' +
                      typeof(arg));
    }

    if (!this.canCoerceArgument_(arg, argType, funcArgType)) {
      return false;
    }
  }
  return true;
};

Module.prototype.getIntType_ = function(value) {
  if (value >= -128 && value <= 127) {
    return this.types.int8;
  } else if (value >= -32768 && value <= 32767) {
    return this.types.int16;
  } else if (value >= -2147483648 && value <= 2147483647) {
    return this.types.int32;
  // TODO(binji): JavaScript numbers only have 53-bits of precision, so
  // this is not correct. We need a 64-bit int type.
  } else if (value >= -9223372036854775808 &&
             value <=  9223372036854775807) {
    return this.types.int64;
  } else {
    assert(value > 0, 'expected uint64. ' + value + ' <= 0.');
    return this.types.uint64;
  }
};

Module.prototype.canCoerceArgument_ = function(fromValue, fromType, toType) {
  if (fromType.data.equals(toType.data)) {
    return true;
  }

  // Implicit cast from pointer to pointer.
  if (fromType.isPointer() && toType.isPointer() &&
      this.canCoercePointer_(fromValue, fromType, toType)) {
    return true;
  }

  // Implicit cast from primitive to primitive.
  if (fromType.isPrimitive() && toType.isPrimitive() &&
      this.canCoercePrimitive_(fromValue, fromType, toType)) {
    return true;
  }

  // Implicit cast from String to char*.
  if (fromType.isPepper() && fromType.getJsPrototype() === String &&
      toType.isPointer() && toType.data.equals(this.types.char$.data)) {
    return true;
  }

  var errorMsg = 'Can\'t coerce ' + fromType.name + ' to ' + toType.name;
  if (fromValue) {
    errorMsg += ' (with fromValue = ' + fromValue + ')';
  }

  throw new Error(errorMsg);
};

Module.prototype.canCoercePointer_ = function(fromValue, fromType, toType) {
  assert(fromType.isPointer(), 'expected pointer, not ' + fromType);
  assert(toType.isPointer(), 'expected pointer, not ' + toType);

  if (fromValue === null) {
    return true;
  }

  // Unwrap the pointers and compare the base types. This will allow us to
  // implicitly cast from int32* to long*, for example.
  try {
    if (this.canCoerceArgument_(undefined, fromType.baseType,
                                toType.baseType)) {
      return true;
    }
  } catch(e) {
    // It's OK if this fails.
  }

  // Coercing a pointer to void* is always valid.
  if (toType === this.types.void$) {
    return true;
  }

  throw new Error('Can only coerce to void*, not ' + toType + '.');
};

Module.prototype.canCoercePrimitive_ = function(fromValue, fromType, toType) {
  assert(fromType.isPrimitive(), 'expected primitive, not ' + fromType);
  assert(toType.isPrimitive(), 'expected primitive, not ' + toType);

  if (fromType.isInt === toType.isInt) {
    if (fromType.isInt) {
      // Both ints.
      if (fromType.sizeof() > toType.sizeof()) {
        throw new Error('Argument type is too large: ' + fromType + ' > ' +
                        toType + '.');
      } else if (fromType.sizeof() === toType.sizeof()) {
        if (fromType.isSigned === toType.isSigned) {
          return true;
        }

        if (fromType.isSigned) {
          // Cast from signed to unsigned.
          if (typeof fromValue === 'number') {
            return fromValue >= 0;
          } else {
            // We don't know what the value is, assume that this is OK.
            return true;
          }
        } else {
          // Cast from unsigned to signed.
          // TODO(binji): fix this check.
          return true;
        }

        throw new Error('Signed/unsigned mismatch: ' + fromType + ', ' +
                        toType + '.');
      }
    } else {
      // Both floats.
      if (fromType.sizeof() > toType.sizeof()) {
        throw new Error('Argument type is too large: ' + fromType + ' > ' +
                        toType + '.');
      }
    }
  } else {
    // One int, one float.
    if (fromType.isInt) {
      // From int to float.
      if ((toType === this.types.float32 && fromType.sizeof() >= 4) ||
          (toType === this.types.float64 && fromType.sizeof() === 8)) {
        throw new Error('Argument type is too large: ' + fromType + ' > ' +
                        toType + '.');
      }
    } else {
      // From float to int.
      throw new Error('Implicit cast from float to int: ' + fromType + ' => ' +
                      toType + '.');
    }
  }

  return true;
};


//// TypeBuilder /////////////////////////////////////////////////////////////
function TypeBuilder() {
  this.idHash = {};
  this.nameHash = {};
}

TypeBuilder.prototype.getNameHash = function() {
  return this.nameHash;
};

TypeBuilder.prototype.findTypeData = function(typeData) {
  for (var id in this.idHash) {
    var type = this.idHash[id];
    if (type.data.equals(typeData)) {
      return type;
    }
  }
  return null;
};

TypeBuilder.prototype.registerType_ = function(id, name, cStr, typeData) {
  assert(id !== 0, 'id === 0');
  assert(!(id in this.idHash), 'id ' + id + ' already exists');
  assert(!(name in this.nameHash), 'name ' + name + ' already exists');

  var otherType = this.findTypeData(typeData);
  assert(otherType === null,
         function() {
           return 'type ' + name + ' already exists. id = ' + otherType.id +
           ' name = ' + otherType.name; });
  var newType = new Type(id, name, cStr, typeData, null);
  this.idHash[id] = newType;
  this.nameHash[name] = newType;
  return newType;
};

TypeBuilder.prototype.makeAliasType = function(name, alias, cStr) {
  var otherType = this.nameHash[alias.name];
  var newType = new Type(otherType.id, name, cStr, otherType.data, otherType);
  assert(!(name in this.nameHash), 'name ' + name + ' already exists');
  this.nameHash[name] = newType;
  return newType;
};

TypeBuilder.prototype.makePepperType = function(id, name, type) {
  var typeData = new PepperTypeData(name, type);
  return this.registerType_(id, name, null, typeData);
};

TypeBuilder.prototype.makePointerType = function(id, name, baseType) {
  var typeData = new PointerTypeData(baseType);
  return this.registerType_(id, name, null, typeData);
};

TypeBuilder.prototype.makePrimitiveType = function(id, name, size, isSigned,
                                                   isInt) {
  var typeData = new PrimitiveTypeData(name, size, isSigned, isInt);
  return this.registerType_(id, name, null, typeData);
};

TypeBuilder.prototype.makeStructType = function(id, name, size, fields) {
  var typeData = new StructTypeData(name, size, fields);
  return this.registerType_(id, name, null, typeData);
};

TypeBuilder.prototype.makeVoidType = function(id, name) {
  var typeData = new VoidTypeData();
  return this.registerType_(id, name, null, typeData);
};

TypeBuilder.prototype.makeFunctionType = function(id, retType) {
  var args = Array.prototype.slice.call(arguments, 1);
  var constructor = Function.bind.apply(FunctionTypeData, [null].concat(args));
  var typeData = new constructor();
  var name = '__FunctionType' + id;
  return this.registerType_(id, name, null, typeData);
};

TypeBuilder.prototype.getPointerType = function(baseType) {
  var newTypeData = new PointerTypeData(baseType);
  var otherType = this.findTypeData(newTypeData);
  if (otherType !== null) {
    return otherType;
  }

  // Don't blow up yet... it is not an error to create a type that doesn't
  // have an id. We can't send it to the NaCl module, but we can use it for
  // type-checking.
  return new Type(0, '', null, newTypeData);
};


//// Context /////////////////////////////////////////////////////////////////
function Context(module) {
  this.$module_ = module;
  this.$handles_ = [];
  // TODO(binji): nice way to clean up all these handles.
  // It would also be nice to clean up malloc'd memory, release PP_Vars, etc.

  for (var name in module.functions) {
    if (module.functions.hasOwnProperty(name)) {
      this[name] = module.functions[name];
    }
  }
}

Context.prototype.$registerHandle = function(handle) {
  assert(handle instanceof Handle, 'handle is not a Handle.');
  this.$handles_.push(handle);
};

Context.prototype.$callFunction = function(func, args) {
  return this.$module_.callFunction_(this, func, args);
};

Context.prototype.$destroyHandles = function() {
  this.$module_.destroyHandles(this.$handles_);
  this.$handles_ = [];
};

Context.prototype.$setField = function(structField, struct_p, value) {
  assert(structField instanceof StructField);
  assert(struct_p instanceof Handle);

  var dst = this.add(struct_p, structField.offset);
  var pointerType = this.$module_.getPointerType(structField.type);
  if (structField.type.isPointer()) {
    return this.set(dst.cast(this.$module_.types.void$$),
                    value.cast(this.$module_.types.void$));
  } else {
    return this.set(dst.cast(pointerType), value);
  }
};

Context.prototype.$getField = function(structField, struct_p) {
  assert(structField instanceof StructField);
  assert(struct_p instanceof Handle);

  var ptr = this.add(struct_p, structField.offset);
  var pointerType = this.$module_.getPointerType(structField.type);
  if (structField.type.isPointer()) {
    return this.get(ptr.cast(this.$module_.types.void$$)).
        cast(structField.type);
  } else {
    return this.get(ptr.cast(pointerType));
  }
};

Context.prototype.$malloc = function(size) {
  var handle = this.malloc(size);
  handle.setFinalizer(this.free.bind(this, handle));
  return handle;
};

Context.prototype.$mallocType = function(type) {
  var pointerType = this.$module_.getPointerType(type);
  var size = type.sizeof();
  var handle = this.malloc(size).cast(pointerType);

  handle.setFinalizer(this.free.bind(this, handle));
  return handle;
};


//// HandleList //////////////////////////////////////////////////////////////
function HandleList() {
  this.nextId_ = 1;
  this.handleIdHash_ = {};
}

HandleList.prototype.makeHandle = function(context, type, id) {
  return new Handle(this, context, type, id);
};

HandleList.prototype.getHandle = function(id) {
  return this.handleIdHash_[id];
};

HandleList.prototype.registerHandle = function(handle) {
  var id = this.nextId_++;
  this.handleIdHash_[id] = handle;
  return id;
};

HandleList.prototype.runFinalizers = function(handles) {
  for (var id in handles) {
    if (!handles.hasOwnProperty(id)) {
      continue;
    }

    // TODO(binji): I think this will need to be reverse-order of the call to
    // setFinalizer; otherwise it seems likely that handles that reference
    // other handles will be destroyed out-of-order.
    handles[id].finalize();
  }
};

HandleList.prototype.destroyHandles = function(handles) {
  for (var id in handles) {
    if (!handles.hasOwnProperty(id)) {
      continue;
    }

    var handle = handles[id];
    delete this.handleIdHash_[handle.id];
  }
};


//// Handle //////////////////////////////////////////////////////////////////
function Handle(handleList, context, type, id) {
  this.handleList_ = handleList;
  this.type = type;
  if (id !== undefined) {
    this.id = id;
  } else {
    this.id = handleList.registerHandle(this);
    context.$registerHandle(this);
  }

  this.context_ = context;
  this.clonedFrom_ = null;
  this.finalizer_ = null;
}

Handle.prototype.getRoot_ = function() {
  var handle = this;
  while (handle.clonedFrom_ !== null) {
    handle = handle.clonedFrom_;
    assert(handle instanceof Handle);
  }

  return handle;
};

Handle.prototype.setFinalizer = function(finalizer) {
  var root = this.getRoot_();
  assert(root.finalizer_ === null);
  root.finalizer_ = finalizer;
};

Handle.prototype.toString = function() {
  return '[Handle ' + this.id + ' ' + this.type.toString() + ']';
};

Handle.prototype.cast = function(newType) {
  // TODO(binji): check validity of cast
  var handle = new Handle(this.handleList_, this.context_, newType, this.id);
  handle.clonedFrom_ = this;
  return handle;
};

Handle.prototype.finalize = function() {
  if (!this.finalizer_) {
    return;
  }

  this.finalizer_();
};


//// Type ////////////////////////////////////////////////////////////////////
function Type(id, name, cStr, typeData, aliasOf) {
  this.id = id;
  this.name = name;
  this.cStr = cStr;
  this.data = typeData;
  this.aliasOf = aliasOf || null;

  var that = this;

  // Delegate other properties to TypeData.
  for (var propName in typeData) {
    if (propName in this) {
      continue;
    }

    var prop = typeData[propName];
    this.delegateToTypeData_(propName, prop);
  }
}

Type.prototype.delegateToTypeData_ = function(propName, prop) {
  if (typeof prop === 'function') {
    this[propName] = prop.bind(this.data);
  } else {
    Object.defineProperty(this, propName, {
      get: function() { return prop; }
    });
  }
};

Type.prototype.toString = function() {
  return this.cStr || this.data.toString();
};

Type.prototype.equals = function(otherType) {
  if (!(otherType instanceof Type)) {
    throw new Error('Attempting to compare Type with non-Type.');
  }
  return this.data.equals(otherType.data);
};

//// TypeData ////////////////////////////////////////////////////////////////
function TypeData() {}

TypeData.KIND_VOID = 0;
TypeData.KIND_POINTER = 1;
TypeData.KIND_PRIMITIVE = 2;
TypeData.KIND_PEPPER = 3;
TypeData.KIND_STRUCT = 4;
TypeData.KIND_FUNCTION = 5;

TypeData.prototype.isVoid = function() {
  return this.kind === TypeData.KIND_VOID;
};
TypeData.prototype.isPointer = function() {
  return this.kind === TypeData.KIND_POINTER;
};
TypeData.prototype.isPrimitive = function() {
  return this.kind === TypeData.KIND_PRIMITIVE;
};
TypeData.prototype.isPepper = function() {
  return this.kind === TypeData.KIND_PEPPER;
};
TypeData.prototype.isStruct = function() {
  return this.kind === TypeData.KIND_STRUCT;
};
TypeData.prototype.isFunction = function() {
  return this.kind === TypeData.KIND_FUNCTION;
};
TypeData.prototype.getJsPrototype = function() { return null; };

TypeData.prototype.equals = function(other) { return false; };

//// VoidTypeData ////////////////////////////////////////////////////////////
function VoidTypeData() { TypeData.call(this); }
VoidTypeData.prototype = Object.create(TypeData.prototype);
VoidTypeData.prototype.constructor = VoidTypeData;
VoidTypeData.prototype.kind = TypeData.KIND_VOID;
VoidTypeData.prototype.sizeof = function() { return 0; };
VoidTypeData.prototype.toString = function() { return 'void'; };

VoidTypeData.prototype.equals = function(other) {
  if (this === other) {
    return true;
  }

  return this.constructor === other.constructor;
};


//// PrimitiveTypeData ///////////////////////////////////////////////////////
function PrimitiveTypeData(name, size, isSigned, isInt) {
  TypeData.call(this);
  this.name = name;
  this.size = size;
  this.isSigned = isSigned;
  this.isInt = isInt;
}

PrimitiveTypeData.TO_STR = {
  char: 'char',
  int8: 'int8_t',
  uint8: 'uint8_t',
  int16: 'int16_t',
  uint16: 'uint16_t',
  int32: 'int32_t',
  uint32: 'uint32_t',
  long: 'long',
  ulong: 'unsigned long',
  int64: 'int64_t',
  uint64: 'uint64_t',
  float32: 'float',
  float64: 'double',
};

PrimitiveTypeData.prototype = Object.create(TypeData.prototype);
PrimitiveTypeData.prototype.constructor = PrimitiveTypeData;
PrimitiveTypeData.prototype.kind = TypeData.KIND_PRIMITIVE;
PrimitiveTypeData.prototype.sizeof = function() { return this.size; };

PrimitiveTypeData.prototype.toString = function() {
  return PrimitiveTypeData.TO_STR[this.name];
};

PrimitiveTypeData.prototype.equals = function(other) {
  if (this === other) {
    return true;
  }

  return this.constructor === other.constructor &&
         this.name === other.name &&
         this.size === other.size &&
         this.isSigned === other.isSigned &&
         this.isInt === other.isInt;
};


//// PointerTypeData /////////////////////////////////////////////////////////
function PointerTypeData(baseType) {
  TypeData.call(this);
  this.baseType = baseType;
}

PointerTypeData.prototype = Object.create(TypeData.prototype);
PointerTypeData.prototype.constructor = PointerTypeData;
PointerTypeData.prototype.kind = TypeData.KIND_POINTER;

PointerTypeData.prototype.sizeof = function() {
  return 4;  // NaCl pointers are always 32-bit.
};

PointerTypeData.prototype.toString = function() {
  return this.baseType.toString() + '*';
};

PointerTypeData.prototype.equals = function(other) {
  if (this === other) {
    return true;
  }

  return this.constructor === other.constructor &&
         this.baseType.data.equals(other.baseType.data);
};


//// StructField /////////////////////////////////////////////////////////////
function StructField(name, type, offset) {
  this.name = name;
  this.type = type;
  this.offset = offset;
  this.fields = {};
  if (type.data instanceof StructTypeData) {
    this.addFields_(this.type.fields);
  }
}

StructField.prototype.addFields_ = function(fields) {
  for (var name in fields) {
    if (!fields.hasOwnProperty(name)) {
      continue;
    }

    var field = fields[name];
    var type = field.type;
    var offset = this.offset + field.offset;
    this.fields[name] = new StructField(name, type, offset);
  }
};

StructField.prototype.equals = function(other) {
  if (this === other) {
    return true;
  }

  return this.constructor === other.constructor &&
         this.name === other.name &&
         this.type.equals(other.type) &&
         this.offset === other.offset;
};


//// StructTypeData //////////////////////////////////////////////////////////
function StructTypeData(name, size, fields) {
  TypeData.call(this);
  this.name = name;
  this.size = size;
  this.fields = {};
  this.addFields_(fields);
}

StructTypeData.prototype = Object.create(TypeData.prototype);
StructTypeData.prototype.constructor = StructTypeData;
StructTypeData.prototype.kind = TypeData.KIND_STRUCT;

StructTypeData.prototype.sizeof = function() { return this.size; };
StructTypeData.prototype.toString = function() {
  return 'struct ' + this.name;
};

StructTypeData.prototype.equals = function(other) {
  if (this === other) {
    return true;
  }

  return this.constructor === other.constructor &&
         this.name === other.name;
};

StructTypeData.prototype.addFields_ = function(fields) {
  for (var name in fields) {
    if (!fields.hasOwnProperty(name)) {
      continue;
    }

    var field = fields[name];
    this.addField_(name, field.type, field.offset);
  }
};

StructTypeData.prototype.addField_ = function(name, type, offset) {
  assert(type instanceof Type, 'type is not instance of Type.');
  assert(offset >= 0, 'offset ' + offset + ' < 0');
  assert(offset + type.sizeof() <= this.size,
         'offset ' + offset + ' > size');
  assert(!this.fields.hasOwnProperty(name),
         'field ' + name + ' already exists');
  this.fields[name] = new StructField(name, type, offset);
};


//// FunctionTypeData ////////////////////////////////////////////////////////
function FunctionTypeData(retType) {
  TypeData.call(this);
  this.retType = retType;
  this.argTypes = Array.prototype.slice.call(arguments, 1);

  // Validate the types.
  assert(retType instanceof Type, 'return type is not instance of Type');
  this.argTypes.forEach(function(argType, i) {
    assert(argType instanceof Type,
           'argument ' + i + ' is not instance of Type');
  });
}

FunctionTypeData.prototype = Object.create(TypeData.prototype);
FunctionTypeData.prototype.constructor = FunctionTypeData;
FunctionTypeData.prototype.kind = TypeData.KIND_FUNCTION;

FunctionTypeData.prototype.sizeof = function() {
  return 4;
};

FunctionTypeData.prototype.toString = function() {
  var s = '';
  s += this.retType.toString();
  s += ' (*)(';
  s += this.argTypes.map(function(x) { return x.toString(); }).join(', ');
  s += ')';
  return s;
};

FunctionTypeData.prototype.equals = function(other) {
  if (this === other) {
    return true;
  }

  if (this.constructor !== other.constructor ||
      !this.retType.data.equals(other.retType.data)) {
    return false;
  }

  if (this.argTypes.length !== other.argTypes.length) {
    return false;
  }

  for (var i = 0; i < this.argTypes.length; ++i) {
    if (!this.argTypes[i].data.equals(other.argTypes[i].data)) {
      return false;
    }
  }

  return true;
};


//// PepperTypeData //////////////////////////////////////////////////////////
function PepperTypeData(name, jsPrototype) {
  TypeData.call(this);
  this.name = name;
  this.jsPrototype = jsPrototype;
}

PepperTypeData.prototype = Object.create(TypeData.prototype);
PepperTypeData.constructor = PepperTypeData;
PepperTypeData.prototype.kind = TypeData.KIND_PEPPER;

PepperTypeData.prototype.sizeof = function() {
  return 20;  /* sizeof(PP_Var) */
};
PepperTypeData.prototype.toString = function() {
  return this.name;
};
PepperTypeData.prototype.getJsPrototype = function() {
  return this.jsPrototype;
};

PepperTypeData.prototype.equals = function(other) {
  if (this === other) {
    return true;
  }

  return this.constructor === other.constructor &&
         this.jsPrototype === other.jsPrototype;
};


//// FunctionBuilder /////////////////////////////////////////////////////////
function FunctionBuilder() {
  this.nameHash = {};
}

FunctionBuilder.prototype.makeFunction = function(name, overloads) {
  assert(!(name in this.nameHash), 'function with this name already exists.');

  var cfunc = new CFunction(name, overloads);
  var func = function() {
    assert(this instanceof Context);
    return this.$callFunction(cfunc, arguments);
  };

  this.nameHash[name] = func;
  return func;
};

FunctionBuilder.prototype.getNameHash = function() {
  return this.nameHash;
};


//// Functions ///////////////////////////////////////////////////////////////
function CFunction(name, overloads) {
  this.name = name;
  if (overloads instanceof Array) {
    this.overloads = overloads;
  } else {
    this.overloads = [overloads];
  }

  // Quick sanity check.
  this.overloads.forEach(function(funcType) {
    assert(funcType.data instanceof FunctionTypeData,
           'expected FunctionTypeData');
  });
}


module.exports = {
  Module: Module,
  // For Testing.
  TypeBuilder: TypeBuilder
};
