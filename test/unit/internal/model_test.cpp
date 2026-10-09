#include <gtest/gtest.h>

#include "cci/client.h"
#include "cci/device.h"

TEST(DeviceDescription, KeepsWhatItWasGiven) {
    contactci::DeviceDescription device("Maestro", "MA-0042", true, false);
    EXPECT_EQ(device.get_product_line(), "Maestro");
    EXPECT_EQ(device.get_serial_number(), "MA-0042");
    EXPECT_TRUE(device.get_is_right());
    EXPECT_FALSE(device.get_is_connected());
}

TEST(ClientDescription, KeepsWhatItWasGiven) {
    contactci::ClientDescription client(4242, "UnityEditor.exe");
    EXPECT_EQ(client.get_process_id(), 4242u);
    EXPECT_EQ(client.get_process_name(), "UnityEditor.exe");
}
