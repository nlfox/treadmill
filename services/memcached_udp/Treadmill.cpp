/*
 *  Copyright (c) 2014, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "treadmill/services/memcached_udp/Connection.h"
#include "treadmill/services/mica/WorkloadZipf.h"

#include "treadmill/Treadmill.h"

int main(int argc, char* argv[]) {
  // Init treadmill
  facebook::windtunnel::treadmill::init(argc, argv);
  // Start treadmill
  return facebook::windtunnel::treadmill::run<
    facebook::windtunnel::treadmill::MemcachedService>(argc, argv);
}
