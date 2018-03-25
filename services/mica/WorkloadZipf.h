/*
 *  Copyright (c) 2014, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <vector>

#include "treadmill/services/memcached/MemcachedService.h"
#include "zipf.h"
#include "treadmill/Workload.h"

DECLARE_int64(number_of_keys);

using folly::Future;
using folly::Promise;

namespace facebook {
namespace windtunnel {
namespace treadmill {

template <>
class Workload<MemcachedService> {
 public:
  enum State {
    WARMUP,
    GET
  };

  Workload<MemcachedService>(folly::dynamic config)
    {
      state_ = State::WARMUP;
      index_ = 0;
      zg = new ZipfGen(1024, 0.99, static_cast<uint64_t>(1));
      get_set = 0;
    }

  std::tuple<std::unique_ptr<MemcachedService::Request>,
             Promise<MemcachedService::Reply>,
             Future<MemcachedService::Reply>>
  getNextRequest() {
    if (index_ == FLAGS_number_of_keys) {
      index_ = 0;
    }
    std::string key;
    if (state_ == State::WARMUP) {
      key = std::to_string(index_);
    }
    else {
      key = std::to_string(zg->next());
    }
    std::unique_ptr<MemcachedService::Request> request;
    if (state_ == State::WARMUP) {
      request = std::make_unique<MemcachedRequest>(MemcachedRequest::SET,
                                                     std::move(key));
      std:: string i = std::to_string(index_);
      std::string( 4-i.length(), '0').append(i);
      request->setValue(i);
      
      LOG(INFO) << "set " << i;
      
      if (index_ == FLAGS_number_of_keys - 1) {
        //LOG(INFO) << "WARMUP complete";
        state_ = State::GET;
      }
    } else if (state_ == State::GET) {
      if (get_set == 30){
          request = std::make_unique<MemcachedRequest>(MemcachedRequest::SET,
                                                     std::move(key));
          //LOG(INFO) << "set " << key;
          request->setValue(key);
          get_set = 0;
      } else {
          //LOG(INFO) << "get " << key ;
          request = std::make_unique<MemcachedRequest>(MemcachedRequest::GET, std::move(key));
          get_set++;
      }
    }
    Promise<MemcachedService::Reply> p;
    auto f = p.getFuture();
    ++index_;
    return std::make_tuple(std::move(request), std::move(p), std::move(f));
  }

  folly::dynamic makeConfigOutputs(
                  std::vector<Workload<MemcachedService>*> workloads) {
    return folly::dynamic::object;
  }

 private:

  ZipfGen* zg;
  
  State state_;
  int index_;
  int get_set;
};

}  // namespace treadmill
}  // namespace windtunnel
}  // namespace facebook
