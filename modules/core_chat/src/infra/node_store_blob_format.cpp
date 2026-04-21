/**
 * @file node_store_blob_format.cpp
 * @brief Shared node-store blob format helpers
 */

#include "chat/infra/node_store_blob_format.h"

#include "chat/infra/node_store_core.h"

namespace chat
{
namespace contacts
{

size_t nodeBlobEntrySizeForVersion(uint8_t version)
{
    if (version == NodeStoreCore::kPersistVersion)
    {
        return NodeStoreCore::kSerializedEntrySizeV8;
    }
    if (version == (NodeStoreCore::kPersistVersion - 1))
    {
        return NodeStoreCore::kSerializedEntrySize;
    }
    if (version == (NodeStoreCore::kPersistVersion - 2))
    {
        return NodeStoreCore::kLegacySerializedEntrySize;
    }
    return 0;
}

bool isValidNodeBlobSize(size_t len)
{
    return len != 0 && (len % NodeStoreCore::kSerializedEntrySizeV8) == 0;
}

size_t nodeBlobEntryCount(size_t len)
{
    return isValidNodeBlobSize(len) ? (len / NodeStoreCore::kSerializedEntrySizeV8) : 0;
}

size_t nodeBlobByteSize(size_t count)
{
    return count * NodeStoreCore::kSerializedEntrySizeV8;
}

NodeStoreSdHeader makeNodeStoreSdHeader(const uint8_t* data, size_t len)
{
    NodeStoreSdHeader header{};
    header.ver = NodeStoreCore::kPersistVersion;
    header.reserved[0] = 0;
    header.reserved[1] = 0;
    header.reserved[2] = 0;
    header.count = static_cast<uint32_t>(nodeBlobEntryCount(len));
    header.crc = NodeStoreCore::computeBlobCrc(data, len);
    return header;
}

NodeBlobValidation validateNodeBlobMetadata(size_t len, uint8_t version,
                                            bool has_crc, uint32_t stored_crc,
                                            const uint8_t* data)
{
    if (len == 0)
    {
        return has_crc ? NodeBlobValidation::StaleMetadata : NodeBlobValidation::Empty;
    }
    if (version == 0)
    {
        return NodeBlobValidation::VersionMismatch;
    }
    const size_t entry_size = nodeBlobEntrySizeForVersion(version);
    if (entry_size == 0)
    {
        return NodeBlobValidation::VersionMismatch;
    }
    if ((len % entry_size) != 0)
    {
        return NodeBlobValidation::InvalidLength;
    }
    if ((len / entry_size) > NodeStoreCore::kMaxNodes)
    {
        return NodeBlobValidation::TooManyEntries;
    }
    if (!has_crc)
    {
        return NodeBlobValidation::MissingCrc;
    }
    if (!data)
    {
        return NodeBlobValidation::InvalidLength;
    }
    const uint32_t calc_crc = NodeStoreCore::computeBlobCrc(data, len);
    return (calc_crc == stored_crc) ? NodeBlobValidation::Ok : NodeBlobValidation::CrcMismatch;
}

NodeBlobValidation validateNodeStoreSdHeader(const NodeStoreSdHeader& header)
{
    if (nodeBlobEntrySizeForVersion(header.ver) == 0)
    {
        return NodeBlobValidation::VersionMismatch;
    }
    if (header.count > NodeStoreCore::kMaxNodes)
    {
        return NodeBlobValidation::TooManyEntries;
    }
    return NodeBlobValidation::Ok;
}

NodeBlobValidation validateNodeStoreSdBlob(const NodeStoreSdHeader& header,
                                           const uint8_t* data, size_t len)
{
    const NodeBlobValidation header_status = validateNodeStoreSdHeader(header);
    if (header_status != NodeBlobValidation::Ok)
    {
        return header_status;
    }
    const size_t entry_size = nodeBlobEntrySizeForVersion(header.ver);
    if (entry_size == 0)
    {
        return NodeBlobValidation::VersionMismatch;
    }
    const size_t expected_bytes = header.count * entry_size;
    if (expected_bytes != len)
    {
        return NodeBlobValidation::InvalidLength;
    }
    const uint32_t calc_crc = NodeStoreCore::computeBlobCrc(data, len);
    return (calc_crc == header.crc) ? NodeBlobValidation::Ok : NodeBlobValidation::CrcMismatch;
}

} // namespace contacts
} // namespace chat
