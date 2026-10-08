#include <gtest/gtest.h>

#include "fake_channel.h"

using testsupport::FakeChannel;

// Wire format: a PacketHeader (two fixed32 fields: length, opcode) followed by
// `length` bytes of the protobuf message.

TEST(Channel, HeaderIsTenBytes) {
    FakeChannel channel;
    EXPECT_EQ(channel.HeaderSize, 10u);
}

TEST(Channel, SendDelimitedWritesHeaderThenBody) {
    FakeChannel channel;
    SessionInitializationRequestMessage message;
    message.set_ishapticsession(true);
    message.set_wantshapticwriteaccess(false);

    channel.send_delimited(OpCode::opSessionInitializationRequestMessage, message);

    PacketHeader header = channel.sent_header();
    EXPECT_EQ(header.opcode(), static_cast<uint32_t>(OpCode::opSessionInitializationRequestMessage));
    EXPECT_EQ(header.length(), message.ByteSizeLong());
    EXPECT_EQ(channel.sent.size(), channel.HeaderSize + message.ByteSizeLong());

    SessionInitializationRequestMessage decoded;
    ASSERT_TRUE(decoded.ParseFromString(channel.sent_body()));
    EXPECT_TRUE(decoded.ishapticsession());
    EXPECT_FALSE(decoded.wantshapticwriteaccess());
}

TEST(Channel, GetDeviceListSendsRequestAndParsesResponse) {
    FakeChannel channel;
    DeviceListResponseMessage response;
    auto *left = response.add_devices();
    left->set_productline("Maestro");
    left->set_serialnumber("L-1");
    left->set_isright(false);
    left->set_isconnected(true);
    auto *right = response.add_devices();
    right->set_productline("Maestro");
    right->set_serialnumber("R-1");
    right->set_isright(true);
    right->set_isconnected(false);
    channel.queue_response(OpCode::opDeviceListResponseMessage, response.SerializeAsString());

    DeviceListResponseMessage result = channel.get_device_list();

    EXPECT_EQ(channel.sent_header().opcode(), static_cast<uint32_t>(OpCode::opDeviceListRequestMessage));
    EXPECT_EQ(channel.sent_header().length(), 0u);
    ASSERT_EQ(result.devices_size(), 2);
    EXPECT_EQ(result.devices(0).serialnumber(), "L-1");
    EXPECT_TRUE(result.devices(0).isconnected());
    EXPECT_EQ(result.devices(1).serialnumber(), "R-1");
    EXPECT_TRUE(result.devices(1).isright());
    EXPECT_TRUE(channel.inbound.empty()) << "the whole response should have been consumed";
}

TEST(Channel, GetClientListSendsRequestAndParsesResponse) {
    FakeChannel channel;
    ClientListResponseMessage response;
    auto *client = response.add_clients();
    client->set_processid(1234);
    client->set_processname("Game.exe");
    channel.queue_response(OpCode::opClientListResponseMessage, response.SerializeAsString());

    ClientListResponseMessage result = channel.get_client_list();

    EXPECT_EQ(channel.sent_header().opcode(), static_cast<uint32_t>(OpCode::opClientListRequestMessage));
    ASSERT_EQ(result.clients_size(), 1);
    EXPECT_EQ(result.clients(0).processid(), 1234u);
    EXPECT_EQ(result.clients(0).processname(), "Game.exe");
}

TEST(Channel, InitializeSessionSendsRequestedAccessAndReturnsTheResponse) {
    FakeChannel channel;
    SessionInitializationResponseMessage response;
    response.set_version("2.1.0");
    response.set_isinteractive(true);
    response.set_ishapticaccessgranted(true);
    response.set_hapticwritesharedmemoryname("Local\\write");
    channel.queue_response(OpCode::opSessionInitializationResponseMessage, response.SerializeAsString());

    SessionInitializationResponseMessage result = channel.initialize_session(true, true);

    SessionInitializationRequestMessage request;
    ASSERT_TRUE(request.ParseFromString(channel.sent_body()));
    EXPECT_TRUE(request.ishapticsession());
    EXPECT_TRUE(request.wantshapticwriteaccess());
    EXPECT_EQ(result.version(), "2.1.0");
    EXPECT_TRUE(result.isinteractive());
    EXPECT_TRUE(result.ishapticaccessgranted());
    EXPECT_EQ(result.hapticwritesharedmemoryname(), "Local\\write");
}

// Documents current behaviour (see SECURITY.md): the body length comes from
// the peer's header and is read as-is - the header first, then exactly that
// many bytes. Nothing bounds it.
TEST(Channel, ReceiveDelimitedReadsTheLengthTheHeaderDeclares) {
    FakeChannel channel;
    channel.queue_response(OpCode::opDeviceListResponseMessage, std::string(1000, '\0'));

    std::string body = channel.receive_delimited();

    ASSERT_EQ(channel.receiveSizes.size(), 2u);
    EXPECT_EQ(channel.receiveSizes[0], channel.HeaderSize);
    EXPECT_EQ(channel.receiveSizes[1], 1000u);
    EXPECT_EQ(body.size(), 1000u);
}

// Known defect, pinned (see SECURITY.md): ParseFromString's result is ignored.
// A truncated response is not rejected - protobuf's partial parse leaves a
// phantom, empty device behind, and the library returns it as if the service
// had sent it. When the library starts rejecting malformed responses, this
// test fails on purpose - update it.
TEST(Channel, KnownDefect_MalformedResponseIsSilentlyAccepted) {
    const std::string malformed("\x0a\xff", 2);   // field 1 (devices), declared 255 bytes long, none present
    DeviceListResponseMessage probe;
    ASSERT_FALSE(probe.ParseFromString(malformed)) << "test input must really be malformed";

    FakeChannel channel;
    channel.queue_response(OpCode::opDeviceListResponseMessage, malformed);

    DeviceListResponseMessage result;
    EXPECT_NO_THROW(result = channel.get_device_list());
    ASSERT_EQ(result.devices_size(), 1) << "the partial parse's phantom device";
    EXPECT_EQ(result.devices(0).serialnumber(), "");
    EXPECT_FALSE(result.devices(0).isconnected());
}
