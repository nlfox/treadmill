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

#include <folly/MoveWrapper.h>
#include <folly/fibers/EventBaseLoopController.h>
#include <folly/fibers/FiberManager.h>
#include <folly/futures/Future.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/IOBuf.h>
#include <mcrouter/lib/network/AsyncMcClient.h>
#include <mcrouter/lib/network/gen/Memcache.h>

#include "treadmill/Connection.h"
#include "treadmill/StatisticsManager.h"
#include "treadmill/Util.h"
#include "treadmill/services/memcached/MemcachedService.h"
#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>

DECLARE_string(hostname);
DECLARE_int32(port);


using boost::asio::ip::udp;
using facebook::memcache::AsyncMcClient;
using facebook::memcache::ConnectionOptions;
using facebook::memcache::McDeleteRequest;
using facebook::memcache::McGetRequest;
using facebook::memcache::McSetRequest;
using facebook::memcache::McOperation;
using folly::fibers::EventBaseLoopController;
using folly::fibers::FiberManager;

namespace facebook {
namespace windtunnel {
namespace treadmill {

struct udp_header
{
    uint16_t request_id;
    uint16_t seq_num;
    uint16_t datagram_num;
    uint16_t reserved;
};


class UDPClient {
public:
    UDPClient(
            boost::asio::io_service &io_service,
            const std::string &host,
            const std::string &port
    ) : io_service_(io_service), socket_(io_service, udp::endpoint(udp::v4(), 0)) {
        udp::resolver resolver(io_service_);
        udp::resolver::query query(udp::v4(), host, port);
        udp::resolver::iterator iter = resolver.resolve(query);
        endpoint_ = *iter;
    }

    ~UDPClient() {
        socket_.close();
    }

    void send(const std::string &msg) {
        udp_header header = {
            request_id,
            0,
            1,
            0
          };
        std::string buf;
        char* header_ptr = reinterpret_cast<char*>(&header);
        buf += header_ptr[1];
	buf += header_ptr[0];
	buf += header_ptr[3];
	buf += header_ptr[2];
	buf += header_ptr[5];
	buf += header_ptr[4];
	buf += header_ptr[7];
	buf += header_ptr[6];
        buf.append(msg);
        //LOG(INFO) << "Message:" << buf << " \n";
	socket_.send_to(boost::asio::buffer(buf, buf.size()), endpoint_); 
        size_t len = socket_.receive_from(
                boost::asio::buffer(recv_buf), endpoint_);
        request_id++;
        //LOG(INFO) << "Received: " <<recv_buf.data() <<"\n";
    }

private:
    boost::asio::io_service &io_service_;
    udp::socket socket_;
    uint16_t request_id = 0 ;
    boost::array<char, 1024> recv_buf;
    udp::endpoint endpoint_;
};



template <>
class Connection<MemcachedService> {
 public:
  explicit Connection<MemcachedService>(folly::EventBase& event_base) {
    std::string host = nsLookUp(FLAGS_hostname);
    ConnectionOptions opts(host, FLAGS_port, mc_ascii_protocol);

    
    io_service_ = std::make_unique<boost::asio::io_service>();
    client_ = std::make_unique<UDPClient>(*io_service_, host,  std::to_string(FLAGS_port));

    auto loopController = std::make_unique<EventBaseLoopController>();
    loopController->attachEventBase(event_base);
    fm_ = std::make_unique<FiberManager>(std::move(loopController));
  }

  bool isReady() const { return true; }

  folly::Future<MemcachedService::Reply>
  sendRequest(std::unique_ptr<typename MemcachedService::Request> request) {
    LOG(INFO) << "enter sendRequest\n";
    folly::MoveWrapper<folly::Promise<MemcachedService::Reply> > p;
    auto f = p->getFuture();

    if (request->which() == MemcachedRequest::GET) {
      auto key = request->key();
      fm_->addTask([this, key, p] () mutable {
        client_->send("get " + key +"\r\n");
        p->setValue(MemcachedService::Reply());
      });
    } else if (request->which() == MemcachedRequest::SET) {
      auto key = request->key();
      auto value = request->value();
      fm_->addTask([this, key, value , p] () mutable {
        client_->send("set " + key + " 0 900 " + std::to_string(value.length()) + "\r\n"+ value+"\r\n");
        p->setValue(MemcachedService::Reply());
      });
    } else {
      auto key = request->key();
      fm_->addTask([this, key, p] () mutable {
        client_->send("delete " + key +"\r\n");
        p->setValue(MemcachedService::Reply());
      });
    }
    return f;
  }

 private:
  std::unique_ptr<UDPClient> client_;
  std::unique_ptr<boost::asio::io_service> io_service_;
  std::unique_ptr<FiberManager> fm_;
};

} // namespace treadmill
} // namespace windtunnel
} // namespace facebook
