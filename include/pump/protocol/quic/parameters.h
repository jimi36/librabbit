/*
 * Copyright (C) 2015-2018 ZhengHaiTao <ming8ren@163.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef pump_protocol_quic_parameters_h
#define pump_protocol_quic_parameters_h

#include "pump/types.h"

namespace pump {
namespace protocol {
namespace quic {

    typedef uint8_t transport_parameter_type;

    // This parameter is the value of the Destination Connection ID field from the first Initial packet sent by the 
    // client. This transport parameter is only sent by a server.
    const static transport_parameter_type PARAM_ORIGINAL_DESTINATION_CONNECTION_ID  = 0x00;

    // The maximum idle timeout is a value in milliseconds that is encoded as an integer. Idle timeout is disabled 
    // when both endpoints omit this transport parameter or specify a value of 0.
    const static transport_parameter_type PARAM_MAX_IDLE_TIMEOUT = 0x01;

    // A stateless reset token is used in verifying a stateless reset. This parameter is a sequence of 16 bytes. 
    // This transport parameter MUST NOT be sent by a client but MAY be sent by a server. A server that does not 
    // send this transport parameter cannot use stateless reset (Section 10.3) for the connection ID negotiated 
    // during the handshake.
    const static transport_parameter_type PARAM_STATELESS_RESET_TOKEN = 0x02;

    // he maximum UDP payload size parameter is an integer value that limits the size of UDP payloads that the 
    // endpoint is willing to receive. UDP datagrams with payloads larger than this limit are not likely to be 
    // processed by the receiver.
    //
    // The default for this parameter is the maximum permitted UDP payload of 65527. Values below 1200 are invalid.
    //
    // This limit does act as an additional constraint on datagram size in the same way as the path MTU, but it is 
    // a property of the endpoint and not the path; see Section 14. It is expected that this is the space an 
    // endpoint dedicates to holding incoming packets.
    const static transport_parameter_type PARAM_MAX_UDP_PAYLOAD_SIZE = 0x03;

    // The initial maximum data parameter is an integer value that contains the initial value for the maximum amount 
    // of data that can be sent on the connection. This is equivalent to sending a MAX_DATA for the connection 
    // immediately after completing the handshake.
    const static transport_parameter_type PARAM_INITIAL_MAX_DATA = 0x04;

    // This parameter is an integer value specifying the initial flow control limit for locally initiated 
    // bidirectional streams. This limit applies to newly created bidirectional streams opened by the endpoint that 
    // sends the transport parameter. In client transport parameters, this applies to streams with an identifier 
    // with the least significant two bits set to 0x00; in server transport parameters, this applies to streams 
    // with the least significant two bits set to 0x01.
    const static transport_parameter_type PARAM_INITIAL_MAX_STREAM_DATA_BIDI_LOCAL  = 0x05;

    // This parameter is an integer value specifying the initial flow control limit for peer-initiated bidirectional 
    // streams. This limit applies to newly created bidirectional streams opened by the endpoint that receives the 
    // transport parameter. In client transport parameters, this applies to streams with an identifier with the 
    // least significant two bits set to 0x01; in server transport parameters, this applies to streams with the least 
    // significant two bits set to 0x00.
    const static transport_parameter_type PARAM_INITIAL_MAX_STREAM_DATA_BIDI_REMOTE = 0x06;

    // This parameter is an integer value specifying the initial flow control limit for unidirectional streams. This 
    // limit applies to newly created unidirectional streams opened by the endpoint that receives the transport 
    // parameter. In client transport parameters, this applies to streams with an identifier with the least significant 
    // two bits set to 0x03; in server transport parameters, this applies to streams with the least significant two bits 
    // set to 0x02.
    const static transport_parameter_type PARAM_INITIAL_MAX_STREAM_DATA_UNI = 0x07;

    // The initial maximum bidirectional streams parameter is an integer value that contains the initial maximum number 
    // of bidirectional streams the endpoint that receives this transport parameter is permitted to initiate. If this 
    // parameter is absent or zero, the peer cannot open bidirectional streams until a MAX_STREAMS frame is sent. Setting 
    // this parameter is equivalent to sending a MAX_STREAMS of the corresponding type with the same value.
    const static transport_parameter_type PARAM_INITIAL_MAX_STREAMS_BIDI = 0x08;

    // The initial maximum unidirectional streams parameter is an integer value that contains the initial maximum number 
    // of unidirectional streams the endpoint that receives this transport parameter is permitted to initiate. If this 
    // parameter is absent or zero, the peer cannot open unidirectional streams until a MAX_STREAMS frame is sent. Setting 
    // this parameter is equivalent to sending a MAX_STREAMS of the corresponding type with the same value.
    const static transport_parameter_type PARAM_INITIAL_MAX_STREAMS_UNI = 0x09;

    // The acknowledgment delay exponent is an integer value indicating an exponent used to decode the ACK Delay field in 
    // the ACK frame. If this value is absent, a default value of 3 is assumed (indicating a multiplier of 8). Values above 
    // 20 are invalid.
    const static transport_parameter_type PARAM_ACK_DELAY_EXPONENT = 0x0a;

    // The maximum acknowledgment delay is an integer value indicating the maximum amount of time in milliseconds by which 
    // the endpoint will delay sending acknowledgments. This value SHOULD include the receiver's expected delays in alarms 
    // firing. For example, if a receiver sets a timer for 5ms and alarms commonly fire up to 1ms late, then it should send 
    // a max_ack_delay of 6ms. If this value is absent, a default of 25 milliseconds is assumed. Values of 214 or greater 
    // are invalid.
    const static transport_parameter_type PARAM_MAX_ACK_DELAY = 0x0b;

    // The disable active migration transport parameter is included if the endpoint does not support active connection 
    // migration (Section 9) on the address being used during the handshake. An endpoint that receives this transport 
    // parameter MUST NOT use a new local address when sending to the address that the peer used during the handshake. This 
    // transport parameter does not prohibit connection migration after a client has acted on a preferred_address transport 
    // parameter. This parameter is a zero-length value.
    const static transport_parameter_type PARAM_DISABLE_ACTIVE_MIGRATION = 0x0c;

    // The server's preferred address is used to effect a change in server address at the end of the handshake. This transport 
    // parameter is only sent by a server. Servers MAY choose to only send a preferred address of one address family by sending 
    // an all-zero address and port (0.0.0.0:0 or [::]:0) for the other family. IP addresses are encoded in network byte order.
    //
    // The preferred_address transport parameter contains an address and port for both IPv4 and IPv6. The four-byte IPv4 Address 
    // field is followed by the associated two-byte IPv4 Port field. This is followed by a 16-byte IPv6 Address field and 
    // two-byte IPv6 Port field. After address and port pairs, a Connection ID Length field describes the length of the following 
    // Connection ID field. Finally, a 16-byte Stateless Reset Token field includes the stateless reset token associated with the 
    // connection ID.
    //
    // The Connection ID field and the Stateless Reset Token field contain an alternative connection ID that has a sequence number 
    // of 1. Having these values sent alongside the preferred address ensures that there will be at least one unused active 
    // connection ID when the client initiates migration to the preferred address.
    //
    // The Connection ID and Stateless Reset Token fields of a preferred address are identical in syntax and semantics to the 
    // corresponding fields of a NEW_CONNECTION_ID frame. A server that chooses a zero-length connection ID MUST NOT provide a 
    // preferred address. Similarly, a server MUST NOT include a zero-length connection ID in this transport parameter. A client 
    // MUST treat a violation of these requirements as a connection error of type TRANSPORT_PARAMETER_ERROR.
    //
    // Preferred Address {
    //   IPv4 Address (32),
    //   IPv4 Port (16),
    //   IPv6 Address (128),
    //   IPv6 Port (16),
    //   Connection ID Length (8),
    //   Connection ID (..),
    //   Stateless Reset Token (128),
    // }
    const static transport_parameter_type PARAM_PREFERRED_ADDRESS = 0x0d;

    // This is an integer value specifying the maximum number of connection IDs from the peer that an endpoint is willing to store. 
    // This value includes the connection ID received during the handshake, that received in the preferred_address transport 
    // parameter, and those received in NEW_CONNECTION_ID frames. The value of the active_connection_id_limit parameter MUST be at 
    // least 2. An endpoint that receives a value less than 2 MUST close the connection with an error of type 
    // TRANSPORT_PARAMETER_ERROR. If this transport parameter is absent, a default of 2 is assumed. If an endpoint issues a 
    // zero-length connection ID, it will never send a NEW_CONNECTION_ID frame and therefore ignores the active_connection_id_limit 
    // value received from its peer.
    const static transport_parameter_type PARAM_ACTIVE_CONNECTION_ID_LIMIT = 0x0e;

    // This is the value that the endpoint included in the Source Connection ID field of the first Initial packet it sends for the 
    // connection.
    const static transport_parameter_type PARAM_INITIAL_SOURCE_CONNECTION_ID = 0x0f;

    // This is the value that the server included in the Source Connection ID field of a Retry packet. This transport parameter is 
    // only sent by a server.
    const static transport_parameter_type PARAM_RETRY_SOURCE_CONNECTION_ID = 0x10;


    /******************************************************************************************************************************/


    // Default ack delay exponent with 3.
    const static int32_t DEF_ACK_DELAY_EXPONENT = 3;

    // Default max ack delay with 25 milliseconds.
    const static int32_t DEF_MAX_ACK_DELAY = 25;

    // Default idle timeout with 30s.
    const static int32_t DEF_IDLE_TIMEOUT = 30000;

    // Default remote idle timeout for accept with 5s;
    const static int32_t DEF_MIN_REMOTE_IDLE_TIMEOUT = 5000;

    // Default idle timeout used before handshake completion with 5s.
    const static int32_t DEF_HANDSHAKE_IDLE_TIMEOUT = 5000;

    // Default timeout for a connection until the crypto handshake succeeds with 10s.
    const static int32_t DEF_HANDSHAKE_TIMEOUT = 10000;

    // Default max time until sending a packet to keep a connection alive with 20s.
    // It should be shorter than the time that NATs clear their mapping.
    const static int32_t DEF_MAX_KEEP_ALIVE_INTERVAL = 20000;

    // Default min active connection id limit with 2.
    const static int32_t MIN_ACTIVE_CID_LIMIT = 2;

    // Default time to keeping closed sessions around in order to retransmit the CONNECTION_CLOSE with 5s.
    // After this time all information about the old connection will be deleted.
    const static int32_t DEF_RETIRED_CID_DELETE_TIMEOUT = 5000;

}
}
}

#endif