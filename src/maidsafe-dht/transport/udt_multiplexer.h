/* Copyright (c) 2010 maidsafe.net limited
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
    * Neither the name of the maidsafe.net limited nor the names of its
    contributors may be used to endorse or promote products derived from this
    software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef MAIDSAFE_DHT_TRANSPORT_UDT_MULTIPLEXER_H_
#define MAIDSAFE_DHT_TRANSPORT_UDT_MULTIPLEXER_H_

#include <array>  // NOLINT
#include <vector>

#include "boost/asio/io_service.hpp"
#include "boost/asio/ip/udp.hpp"
#include "maidsafe-dht/transport/transport.h"
#include "maidsafe-dht/transport/udt_dispatch_op.h"
#include "maidsafe-dht/transport/udt_dispatcher.h"
#include "maidsafe-dht/transport/udt_packet.h"

namespace maidsafe {

namespace transport {

class UdtMultiplexer {
 public:
  explicit UdtMultiplexer(boost::asio::io_service &asio_service);
  ~UdtMultiplexer();

  // Open the multiplexer as a client for the specified protocol.
  TransportCondition Open(const boost::asio::ip::udp &protocol);

  // Open the multiplexer as a server on the specified endpoint.
  TransportCondition Open(const boost::asio::ip::udp::endpoint &endpoint);

  // Whether the multiplexer is open.
  bool IsOpen() const;

  // Close the multiplexer.
  void Close();

  // Asynchronously receive a single packet and dispatch it.
  template <typename DispatchHandler>
  void AsyncDispatch(DispatchHandler handler) {
    UdtDispatchOp<DispatchHandler> op(handler,
                                      boost::asio::buffer(receive_buffer_),
                                      &sender_endpoint_, &dispatcher_);
    socket_.async_receive_from(boost::asio::buffer(receive_buffer_),
                               sender_endpoint_, 0, op);
  }

 private:
  friend class UdtAcceptor;
  friend class UdtSocket;

  // Disallow copying and assignment.
  UdtMultiplexer(const UdtMultiplexer&);
  UdtMultiplexer &operator=(const UdtMultiplexer&);

  // Called by the acceptor or socket objects to send a packet. Returns true if
  // the data was sent successfully, false otherwise.
  template <typename Packet>
  TransportCondition SendTo(const Packet &packet,
              const boost::asio::ip::udp::endpoint &endpoint) {
    std::array<unsigned char, UdtPacket::kMaxSize> data;
    auto buffer = boost::asio::buffer(&data[0], UdtPacket::kMaxSize);
    if (size_t length = packet.Encode(buffer)) {
      boost::system::error_code ec;
      socket_.send_to(boost::asio::buffer(buffer, length), endpoint, 0, ec);
      return ec ? kSendFailure : kSuccess;
    }
    return kSendFailure;
  }

  // The UDP socket used for all UDT protocol communication.
  boost::asio::ip::udp::socket socket_;

  // Data members used to receive information about incoming packets.
  std::vector<unsigned char> receive_buffer_;
  boost::asio::ip::udp::endpoint sender_endpoint_;

  // Dispatcher keeps track of the active sockets and the acceptor.
  UdtDispatcher dispatcher_;
};

}  // namespace transport

}  // namespace maidsafe

#endif  // MAIDSAFE_DHT_TRANSPORT_UDT_MULTIPLEXER_H_