# Copyright 2014 Ben Smith. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

SDK_DIR = out/nacl_sdk
NACL_SDK_ROOT = $(SDK_DIR)/pepper_canary
NINJA = out/ninja/ninja
NINJA_DIR = $(dir $(NINJA))
STAMP_DIR = out/stamps
PORTS_STAMP = $(STAMP_DIR)/ports


.PHONY: all
all: ninja sdk ports examples

$(STAMP_DIR):
	mkdir -p $(STAMP_DIR)


# ninja
$(NINJA_DIR):
	mkdir -p $(NINJA_DIR)

$(NINJA): | $(NINJA_DIR)
	python third_party/ninja/bootstrap.py
	cp third_party/ninja/ninja $(NINJA)

.PHONY: ninja
ninja: $(NINJA)


# sdk
out/nacl_sdk.zip:
	cd out && \
	wget http://storage.googleapis.com/nativeclient-mirror/nacl/nacl_sdk/nacl_sdk.zip

$(SDK_DIR): out/nacl_sdk.zip
	cd out && \
	unzip nacl_sdk.zip

$(NACL_SDK_ROOT): | $(SDK_DIR)
	$(SDK_DIR)/naclsdk update pepper_canary --force

.PHONY: sdk
sdk: $(NACL_SDK_ROOT)


# ports
PORTS = zlib libgit2

$(PORTS_STAMP): $(NACL_SDK_ROOT) | $(STAMP_DIR)
	NACL_SDK_ROOT=$(abspath $(NACL_SDK_ROOT)) $(MAKE) -C third_party/naclports $(PORTS) NACL_ARCH=pnacl FORCE=1
	touch $@

.PHONY: ports
ports: $(PORTS_STAMP)

EXAMPLE_DEPS = $(NINJA) $(NACL_SDK_ROOT) $(PORTS_STAMP)

# examples
.PHONY: example-git example-zip example-zlib
git: $(EXAMPLE_DEPS)
	@$(NINJA) git

zip: $(EXAMPLE_DEPS)
	@$(NINJA) zip

zlib: $(EXAMPLE_DEPS)
	@$(NINJA) zlib

.PHONY: run-git run-zip run-zlib
run-git: $(EXAMPLE_DEPS)
	@$(NINJA) run-git

run-zip: $(EXAMPLE_DEPS)
	@$(NINJA) run-zip

run-zlib: $(EXAMPLE_DEPS)
	@$(NINJA) run-zlib
