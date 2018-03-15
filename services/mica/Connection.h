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

#include<stdlib.h> //exit(0);
#include<arpa/inet.h>
#include<sys/socket.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>
#include "cityhash.h"
#include "zipf.h"
#include "memcpy.h"
#include "utils.h"

#define BUFLEN 1024

#define ETHER_TYPE  0x0800


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
    UDPClient(const char *host, int port) {
        int i;

        if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
            printf("socket error");
        }
        struct sockaddr_in addr, srcaddr;

        static int instance_id = 0; 
        // dest port
        memset((char *) &si_other, 0, sizeof(si_other));
        si_other.sin_family = AF_INET;
        si_other.sin_port = htons(port);

        dest_port = port;
        dest_host = const_cast<char *>(host);

        // bind
        memset(&srcaddr, 0, sizeof(srcaddr));
        srcaddr.sin_family = AF_INET;
        srcaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        port_base = 6655;
        send_port = port_base + instance_id;
        instance_id ++;

        srcaddr.sin_port = htons(send_port);
        //LOG(INFO) << "sent port set to " << send_port << "\n";
        instance_id += 1;


        if (bind(s, (struct sockaddr *) &srcaddr, sizeof(srcaddr)) < 0) {
            perror("bind");
            exit(1);
        }


        if ((rs = socket(AF_PACKET, SOCK_RAW, htons(ETHER_TYPE))) == -1) {
            perror("listener: socket");
            exit(1);
        }

        BindToInterface(rs, "p1p1", ETHER_TYPE);

    }

    ~UDPClient() {}
    
    void BindToInterface(int raw, char *device, int protocol) {
        struct sockaddr_ll sll;
        struct ifreq ifr;
        bzero(&sll, sizeof(sll));
        bzero(&ifr, sizeof(ifr));
        strncpy((char *) ifr.ifr_name, device, IFNAMSIZ - 1);
        //copy device name to ifr
        if ((ioctl(raw, SIOCGIFINDEX, &ifr)) == -1) {
            perror("Unable to find interface index");
            exit(-1);
        }
        sll.sll_family = AF_PACKET;
        sll.sll_ifindex = ifr.ifr_ifindex;
        sll.sll_protocol = htons(protocol);
        if ((bind(raw, (struct sockaddr *) &sll, sizeof(sll))) == -1) {
            perror("bind: ");
            exit(-1);
        }
    }

    void send(const std::string &key_s, const std::string &value_s, int type){

        RequestBatchHeader requestBatchHeader = RequestBatchHeader();
        requestBatchHeader.magic = 0x78; //req
        requestBatchHeader.num_requests = 1;
        requestBatchHeader.reserved0 = 0;

        const char *key = key_s.data();
        size_t key_length = key_s.size();
        const char *value = value_s.data();
        size_t value_length = value_s.size();


        RequestHeader requestHeader = RequestHeader();
        if (type == 0){
          requestHeader.operation = static_cast<uint8_t>(Operation::kSet);
        } else if (type == 1){
          requestHeader.operation = static_cast<uint8_t>(Operation::kGet);
        }
        else{
          requestHeader.operation = static_cast<uint8_t>(Operation::kDelete);
        }

        requestHeader.kv_length_vec = static_cast<uint32_t>((key_length << 24) | value_length);
        requestHeader.key_hash = CityHash64(key, key_length);
        requestHeader.reserved0 = 0;
        requestHeader.reserved1 = 0;
        requestHeader.opaque = 0;

        requestHeader.result = static_cast<uint8_t>(Result::kNotProcessed);
        //std::cout << "Hash :" << requestHeader.key_hash << "\n";
        //std::cout << "Opera : "<< requestHeader.operation <<"\n";
        std::vector<char> buffer(
                sizeof(RequestBatchHeader) + sizeof(RequestHeader) + ::mica::util::roundup<8>(key_length) +
                ::mica::util::roundup<8>(value_length));
        
        std::vector<char> recv_buffer(1024);
        auto p = buffer.data();

        std::memcpy(p, &requestBatchHeader, sizeof(RequestBatchHeader));
        p += sizeof(RequestBatchHeader);
        std::memcpy(p, &requestHeader, sizeof(RequestHeader));
        p += sizeof(RequestHeader);

        ::mica::util::memcpy<8, char, char>(p, key, ::mica::util::roundup<8>(key_length));
        p += ::mica::util::roundup<8>(key_length);
        //std::cout << ::mica::util::roundup<8>(key_length) << "\n ";
        ::mica::util::memcpy<8, char, char>(p, value, ::mica::util::roundup<8>(value_length));
        p += ::mica::util::roundup<8>(value_length);


        memset((char *) &si_other, 0, sizeof(si_other));
        si_other.sin_family = AF_INET;
        si_other.sin_port = htons(dest_port);
        if (inet_aton(dest_host, &si_other.sin_addr) == 0) {
            fprintf(stderr, "inet_aton() failed\n");
            exit(1);
        }


        if (sendto(s, buffer.data(), buffer.size(), 0, (struct sockaddr *) &si_other, slen) == -1) {
            printf("sendto() fail");
        }
        //LOG(INFO) << "sent packet type: " << type << " key:" << key_s << " value: " << value_s << "\n" ;
        //printf("sent packet\n");
        bool flag = true;
        while (flag) {
            int numbytes = static_cast<int>(recvfrom(rs, recv_buffer.data(), recv_buffer.size(), 0, (struct sockaddr *) &si_other,
                                                     (socklen_t *) (&slen)));
            struct udphdr *udph = (struct udphdr *) (recv_buffer.data() + sizeof(struct iphdr) +
                                                     sizeof(struct ether_header));
            //LOG(INFO) << "get packet " << htons(udph->dest) << " .\n";
            if (htons(udph->dest) == send_port) {
                flag = false;
            }
        }

        // for (int i = 0; i < buffer.size(); ++i) {
        //     printf(" %02x ", (unsigned char) buffer[i]);
        // }



    }



    void set(const std::string &key, const std::string &value) {
        send(key,value,0);
        
    }

    void get(const std::string &key){
        send(key,"",1);
    }
    void del(const std::string &key){
        send(key,"",2);
    }

private:
    int s, rs;
    struct sockaddr_in si_other;
    int slen = sizeof(si_other);
    int port_base;
    int send_port;
    int dest_port;
    char *dest_host;
};



template <>
class Connection<MemcachedService> {
 public:
  explicit Connection<MemcachedService>(folly::EventBase& event_base) {
    //std::string host = nsLookUp(FLAGS_hostname);
    //ConnectionOptions opts(host, FLAGS_port, mc_ascii_protocol);

    client_ = std::make_unique<UDPClient>( "192.168.23.2",  2333);
    //LOG(INFO) << "enter sendRequest\n";

    auto loopController = std::make_unique<EventBaseLoopController>();
    loopController->attachEventBase(event_base);
    fm_ = std::make_unique<FiberManager>(std::move(loopController));
  }

  bool isReady() const { return true; }

  folly::Future<MemcachedService::Reply>
  sendRequest(std::unique_ptr<typename MemcachedService::Request> request) {
    //LOG(INFO) << "enter sendRequest\n";
    folly::MoveWrapper<folly::Promise<MemcachedService::Reply> > p;
    auto f = p->getFuture();

    if (request->which() == MemcachedRequest::GET) {
      auto key = request->key();
      fm_->addTask([this, key, p] () mutable {
        client_->get(key);
        p->setValue(MemcachedService::Reply());
      });
    } else if (request->which() == MemcachedRequest::SET) {
      auto key = request->key();
      auto value = request->value();
      fm_->addTask([this, key, value , p] () mutable {
        client_->set(key,value);
        p->setValue(MemcachedService::Reply());
      });
    } else {
      auto key = request->key();
      fm_->addTask([this, key, p] () mutable {
        client_->del(key);
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
