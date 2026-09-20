//
// Created by brant_contactci on 9/27/2024.
//

#pragma once

enum OtaStatus {
    ota_error,          //general unspecified error
    ota_pass,           //success
    ota_checksumFail,   //checksum mismatch
    ota_versionFail,    //supplied version older than current version
    ota_noSpaceFail     //no space on device for OTA files
};