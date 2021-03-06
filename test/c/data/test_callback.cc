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

#include <gtest/gtest.h>
#include <ppapi/c/pp_var.h>
#include <pthread.h>
#include "bool.h"
#include "error.h"
#include "fake_interfaces.h"
#include "handle.h"
#include "interfaces.h"
#include "json.h"
#include "queue.h"
#include "run.h"
#include "var.h"

class ThreadedTest : public ::testing::Test {
 public:
  static const int kQueueSize = 256;

  virtual void SetUp() {
    js_to_c_queue_ = nb_queue_create(kQueueSize);
    c_to_js_queue_ = nb_queue_create(kQueueSize);
    pthread_create(&thread_, NULL, &ThreadFuncThunk, this);
    fake_interface_set_post_message_callback(&OnPostMessageThunk, this);
  }

  virtual void TearDown() {
    nb_queue_destroy(js_to_c_queue_);
    nb_queue_destroy(c_to_js_queue_);
    EXPECT_EQ(NB_TRUE, fake_interface_check_no_references());
  }

  static void* ThreadFuncThunk(void* thiz) {
    return static_cast<ThreadedTest*>(thiz)->ThreadFunc();
  }

  static void OnPostMessageThunk(struct PP_Var message, void* thiz) {
    static_cast<ThreadedTest*>(thiz)->OnPostMessage(message);
  }

  void EnqueueCMessage(const char* json) {
    struct PP_Var message = json_to_var(json);
    ASSERT_EQ(PP_VARTYPE_DICTIONARY, message.type);
    ASSERT_EQ(1, nb_queue_enqueue(js_to_c_queue_, message));
    nb_var_release(message);
  }

  void EnqueueQuitMessage() {
    ASSERT_EQ(1, nb_queue_enqueue(js_to_c_queue_, PP_MakeUndefined()));
  }

  struct PP_Var DequeueJsMessage() {
    return nb_queue_dequeue(c_to_js_queue_);
  }

  void* ThreadFunc() {
    while (1) {
      struct PP_Var request = nb_queue_dequeue(js_to_c_queue_);
      struct PP_Var response = PP_MakeUndefined();

      /* undefined is the sentinel to exit the thread loop */
      if (request.type == PP_VARTYPE_UNDEFINED) {
        nb_var_release(request);
        break;
      }

      nb_request_run(js_to_c_queue_, request, &response);
      g_nb_ppb_messaging->PostMessage(g_nb_pp_instance, response);
      nb_var_release(response);
      nb_var_release(request);
    }

    return NULL;
  }

  void OnPostMessage(struct PP_Var message) {
    ASSERT_EQ(1, nb_queue_enqueue(c_to_js_queue_, message));
  }

 protected:
  pthread_t thread_;
  NB_Queue* js_to_c_queue_;
  NB_Queue* c_to_js_queue_;
};

TEST_F(ThreadedTest, Basic) {
  const char* request_json =
      "{\"id\": 1,"
      " \"set\": {\"1\": [\"function\", 2]}}";
  EnqueueCMessage(request_json);
  struct PP_Var response_var = DequeueJsMessage();

  char* response_json = var_to_json_flat(response_var);
  ASSERT_STREQ("{\"id\":1,\"values\":[]}\n", response_json);
  free(response_json);
  nb_var_release(response_var);

  NB_FuncId func_id;
  ASSERT_EQ(NB_TRUE, nb_handle_get_func_id(1, &func_id));
  ASSERT_EQ(2, func_id);
  nb_handle_destroy(1);

  EnqueueQuitMessage();
}

TEST_F(ThreadedTest, Callback) {
  const char* request_json =
      "{\"id\": 1,"
      " \"set\": {\"1\": [\"function\", 2]},"
      /* call_with_10_and_add_1(f) */
      " \"commands\": [{\"id\": 0, \"args\": [1], \"ret\": 2}],"
      " \"get\": [2],"
      " \"destroy\": [1, 2]}";
  EnqueueCMessage(request_json);

  struct PP_Var cb_response_var = DequeueJsMessage();
  char* cb_module_json = var_to_json_flat(cb_response_var);
  ASSERT_STREQ("{\"cbId\":1,\"id\":2,\"values\":[10]}\n", cb_module_json);
  free(cb_module_json);
  nb_var_release(cb_response_var);

  // let's assume the JS function doubles the integer given.
  EnqueueCMessage("{\"id\":2,\"cbId\":1,\"values\":[20]}");

  struct PP_Var response_var = DequeueJsMessage();
  char* response_json = var_to_json_flat(response_var);
  ASSERT_STREQ("{\"id\":1,\"values\":[21]}\n", response_json);
  free(response_json);
  nb_var_release(response_var);

  EnqueueQuitMessage();
}

TEST_F(ThreadedTest, Int64) {
  const char* request_json =
      "{\"id\": 1,"
      " \"set\": {\"1\": [\"function\", 2]},"
      /* sum_calls_of_10_and_20(f) */
      " \"commands\": [{\"id\": 1, \"args\": [1], \"ret\": 2}],"
      " \"get\": [2],"
      " \"destroy\": [1, 2]}";
  EnqueueCMessage(request_json);

  struct PP_Var cb_response_var;
  char* cb_module_json;

  // Should be called with f(10) first.
  cb_response_var = DequeueJsMessage();
  cb_module_json = var_to_json_flat(cb_response_var);
  ASSERT_STREQ("{\"cbId\":1,\"id\":2,\"values\":[[\"long\",10,0]]}\n",
               cb_module_json);
  free(cb_module_json);
  nb_var_release(cb_response_var);

  // let's assume the JS function returns 1 << x.
  EnqueueCMessage("{\"id\":2,\"cbId\":1,\"values\":[[\"long\",1024,0]]}");

  // Should be called with f(20) next.
  cb_response_var = DequeueJsMessage();
  cb_module_json = var_to_json_flat(cb_response_var);
  ASSERT_STREQ("{\"cbId\":2,\"id\":2,\"values\":[[\"long\",20,0]]}\n",
               cb_module_json);
  free(cb_module_json);
  nb_var_release(cb_response_var);

  EnqueueCMessage("{\"id\":2,\"cbId\":2,\"values\":[[\"long\",1048576,0]]}");

  // sum_calls_of_10_and_20 will return the sum of the two intermediary results.
  struct PP_Var response_var = DequeueJsMessage();
  char* response_json = var_to_json_flat(response_var);
  ASSERT_STREQ("{\"id\":1,\"values\":[[\"long\",1049600,0]]}\n", response_json);
  free(response_json);
  nb_var_release(response_var);

  EnqueueQuitMessage();
}
