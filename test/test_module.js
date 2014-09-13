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

var assert = require('assert'),
    module = require('../src/js/module'),
    type = require('../src/js/type'),
    NaClEmbed = require('./nacl_embed_for_testing'),
    Embed = require('../src/js/embed'),
    emptyMessage;

emptyMessage = {
  getHandles: [],
  setHandles: {},
  commands: []
};

describe('Module', function() {
  it('should start with an empty message', function() {
    var m = module.Module();
    assert.deepEqual(m.$getMessage(), emptyMessage);
  });

  it('should work for a simple function type', function() {
    var addType = type.Function(type.int, [type.int, type.int]),
        m = module.Module();

    m.$defineFunction('add', [module.Function(1, addType)]);

    m.add(3, 4);

    assert.deepEqual(m.$getMessage(), {
      getHandles: [],
      setHandles: {
        1: 3,
        2: 4
      },
      commands: [
        {id: 1, args: [1, 2], ret: 3}
      ]
    });
  });

  it('should work for an overloaded function', function() {
    var addIntType = type.Function(type.int, [type.int, type.int]),
        addFloatType = type.Function(type.float, [type.float, type.float]),
        m = module.Module();

    m.$defineFunction('add', [
        module.Function(1, addIntType),
        module.Function(2, addFloatType)
    ]);

    m.add(3, 4);
    m.add(3.5, 4);

    assert.deepEqual(m.$getMessage(), {
      getHandles: [],
      setHandles: {
        1: 3,
        2: 4,
        4: 3.5,
        5: 4
      },
      commands: [
        {id: 1, args: [1, 2], ret: 3},
        {id: 2, args: [4, 5], ret: 6}
      ]
    });
  });

  it('should allow creation of handles', function() {
    var m = module.Module(),
        h1,
        h2;

    h1 = m.$handle(4);
    h2 = m.$handle(4, type.float);

    assert.strictEqual(h1.type, type.schar);
    assert.strictEqual(h2.type, type.float);

    assert.deepEqual(m.$getMessage(), {
      getHandles: [],
      setHandles: {
        1: 4,
        2: 4
      },
      commands: []
    });
  });

  it('should allow use of explicitly-created handles', function() {
    var addType = type.Function(type.int, [type.int, type.int]),
        m = module.Module(),
        h;

    m.$defineFunction('add', [module.Function(1, addType)]);
    h = m.$handle(4);
    m.add(h, h);

    assert.deepEqual(m.$getMessage(), {
      getHandles: [],
      setHandles: { 1: 4 },
      commands: [
        {id: 1, args: [1, 1], ret: 2}
      ]
    });
  });

  it('should allow handle pipelining', function() {
    var voidp = type.Pointer(type.void),
        intp = type.Pointer(type.int),
        mallocType = type.Function(voidp, [type.uint]),
        getIntType = type.Function(type.int, [intp]),
        m = module.Module();

    m.$defineFunction('malloc', [module.Function(1, mallocType)]);
    m.$defineFunction('get', [module.Function(2, getIntType)]);

    m.get(m.malloc(4).cast(intp));

    assert.deepEqual(m.$getMessage(), {
      getHandles: [],
      setHandles: { 1: 4 },
      commands: [
        {id: 1, args: [1], ret: 2},
        {id: 2, args: [2], ret: 3}
      ]
    });
  });

  it('should allow creation of another context', function() {
    var m = module.Module(),
        c = m.$createContext(),
        h;

    m.$context = c;
    h = m.$handle(1000);

    assert.strictEqual(h.id, 1);
    assert.strictEqual(h.context, c);
    assert.strictEqual(c.handles.length, 1);
    assert.strictEqual(c.handles[0].id, 1);
  });

  describe('$commit', function() {
    it('should call callback and extract value from handle', function(done) {
      var ne = NaClEmbed(),
          e = Embed(ne),
          m = module.Module(e),
          addType = type.Function(type.int, [type.int, type.int]),
          h;

      m.$defineFunction('add', [module.Function(1, addType)]);

      ne.load();
      ne.setPostMessageCallback(function(msg) {
        assert.deepEqual(msg, {
          id: 1,
          getHandles: [3],
          setHandles: {1: 3, 2: 4},
          commands: [ {id: 1, args: [1, 2], ret: 3} ]
        });

        ne.message({id: 1, values: [7]});
      });

      h = m.add(3, 4);
      m.$commit([h], function(hVal) {
        assert.strictEqual(hVal, 7);
        done();
      });

      assert.deepEqual(m.$getMessage(), emptyMessage);
    });

    it('should unwrap multiple handles', function(done) {
      var ne = NaClEmbed(),
          e = Embed(ne),
          m = module.Module(e),
          addType = type.Function(type.int, [type.int, type.int]),
          h1,
          h2;

      m.$defineFunction('add', [module.Function(1, addType)]);

      ne.load();
      ne.setPostMessageCallback(function(msg) {
        assert.deepEqual(msg.getHandles, [3, 5]);
        ne.message({id: 1, values: [7, 12]});
      });

      h1 = m.add(3, 4);
      h2 = m.add(h1, 5);
      m.$commit([h1, h2], function(h1Val, h2Val) {
        assert.strictEqual(h1Val, 7);
        assert.strictEqual(h2Val, 12);
        done();
      });
    });

    it('should pass current context to commit callback', function(done) {
      var ne = NaClEmbed(),
          e = Embed(ne),
          m = module.Module(e),
          c = m.$createContext(),
          oldC;

      ne.load();
      ne.setPostMessageCallback(function(msg) {
        ne.message({id: 1});
      });

      oldC = m.$context;
      m.$context = c;

      m.$commit([], function() {
        assert.strictEqual(m.$context, c);
        done();
      });

      // Change context back to default. The commit callback should still have
      // c.
      m.$context = oldC;
    });
  });
});
