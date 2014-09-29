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

#include "test_gen.h"

extern "C" {
int g_foo_called = 0;
}

TEST_F(GeneratorTest, Simple) {
  const char* request_json =
      "{\"id\": 1, \"commands\": [{\"id\": 1, \"args\": []}]}";
  const char* response_json = "{\"id\":1,\"values\":[]}\n";
  RunTest(request_json, response_json);
  EXPECT_EQ(1, g_foo_called);
}