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

var assert = require('chai').assert,
    nacl = require('nacl-sdk'),
    path = require('path'),
    testNexe;

function startswith(s, prefix) {
  return s.lastIndexOf(prefix, 0) === 0;
}

function runTest(args, callback) {
  nacl.run(testNexe, args, callback);
}

describe('C Tests', function() {
  this.enableTimeouts(false);

  it('should build successfully', function(done) {
    var outdir = path.resolve(__dirname, '../../out/test/c/test'),
        srcdir = path.resolve(__dirname, '../../src/c'),
        infiles = [
          path.join(srcdir, 'handle.c'),
          path.join(srcdir, 'interfaces.c'),
          path.join(srcdir, 'message.c'),
          path.join(srcdir, 'var.c'),
          path.join(srcdir, 'type.c'),
          path.join(__dirname, 'fake_interfaces.c'),
          path.join(__dirname, 'json.cc'),
          path.join(__dirname, 'main.cc'),
          path.join(__dirname, 'test_handle.cc'),
          path.join(__dirname, 'test_message.cc'),
          path.join(__dirname, 'test_json.cc'),
        ],
        opts = {
          toolchain: 'newlib',
          config: 'Debug',
          arch: 'x86_64',
          outdir: outdir,

          compile: {
            args: ['-Wall', '-pthread'],
            includeDirs: [
              path.resolve(__dirname),
              path.resolve(__dirname, '../../src/c')
            ],
            libs: [
              'jsoncpp', 'ppapi_simple', 'gtest', 'nacl_io', 'ppapi_cpp', 'ppapi'
            ]
          },
        };

    nacl.build(infiles, 'test', opts, function(error, nexe) {
      if (error) {
        return done(error);
      }

      testNexe = nexe;
      parseTests(done);
    });
  });
});

function parseTests(callback) {
  runTest(['--gtest_list_tests'], function(error, stdout) {
    var suites = [],
        suite = null;

    if (error) {
      assert.ok(false, 'Error running to get test list.\n' + error);
    }

    function addSuite() {
      if (suite) {
        suites.push(suite);
      }
    }

    // Parse the stdout for a list of tests.
    stdout.split('\n').forEach(function(line) {
      if (startswith(line, '  ')) {
        suite.cases.push(line.trim());
      } else {
        // Add previous suite, if any.
        addSuite();

        // Create a new suite.
        var suiteName = line.trim().slice(0, -1);

        if (suiteName) {
          suite = {name: suiteName, cases: []};
        } else {
          suite = null;
        }
      }
    });

    addSuite();

    describe('C Tests', function() {
      suites.forEach(function(suite) {
        describe(suite.name, function() {
          suite.cases.forEach(function(testCase) {
            it(testCase, function(done) {
              this.slow(200);
              runTest(['--gtest_filter='+suite.name+'.'+testCase], done);
            });
          });
        });
      });
    });

    callback();
  });
}

