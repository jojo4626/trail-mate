/**
 * @file node_store.cpp
 * @brief Persisted NodeInfo store shell backed by a single selected persistence backend
 */

#include "platform/esp/arduino_common/chat/infra/meshtastic/node_store.h"
#include "../../internal/blob_store_io.h"
#include "chat/infra/node_store_blob_format.h"

#include <SD.h>
#include <cstdio>
#include <esp_err.h>
#include <nvs.h>
#include <string>

namespace chat
{
namespace meshtastic
{

#ifndef NODE_STORE_LOG_ENABLE
#define NODE_STORE_LOG_ENABLE 1
#endif

#if NODE_STORE_LOG_ENABLE
#define NODE_STORE_LOG(...) Serial.printf(__VA_ARGS__)
#else
#define NODE_STORE_LOG(...)
#endif

namespace
{

void logNvsStats(const char* tag, const char* ns)
{
    nvs_stats_t stats{};
    esp_err_t err = nvs_get_stats(nullptr, &stats);
    if (err == ESP_OK)
    {
        NODE_STORE_LOG("[NodeStore] NVS stats(%s): used=%u free=%u total=%u namespaces=%u\n",
                       tag ? tag : "?",
                       static_cast<unsigned>(stats.used_entries),
                       static_cast<unsigned>(stats.free_entries),
                       static_cast<unsigned>(stats.total_entries),
                       static_cast<unsigned>(stats.namespace_count));
    }
    else
    {
        NODE_STORE_LOG("[NodeStore] NVS stats(%s) err=%s\n",
                       tag ? tag : "?",
                       esp_err_to_name(err));
    }

    if (ns && ns[0])
    {
        nvs_handle_t handle;
        err = nvs_open(ns, NVS_READONLY, &handle);
        if (err == ESP_OK)
        {
            NODE_STORE_LOG("[NodeStore] NVS open ns=%s ok\n", ns);
            nvs_close(handle);
        }
        else
        {
            NODE_STORE_LOG("[NodeStore] NVS open ns=%s err=%s\n", ns, esp_err_to_name(err));
        }
    }
}

bool canonicalizeNodePayload(const uint8_t* data, size_t len, uint8_t version, std::vector<uint8_t>& out)
{
    std::vector<contacts::NodeEntry> entries;
    if (!contacts::NodeStoreCore::decodeBlob(entries, data, len, version))
    {
        out.clear();
        return false;
    }

    contacts::NodeStoreCore::encodeBlob(out, entries);
    return true;
}

} // namespace

NodeStore::NodeStore()
    : core_(*this)
{
}

void NodeStore::begin()
{
    backend_ = (SD.cardType() != CARD_NONE) ? StorageBackend::Sd : StorageBackend::Nvs;
    NODE_STORE_LOG("[NodeStore] backend=%s\n",
                   backend_ == StorageBackend::Sd ? "sd" : "nvs");
    core_.begin();
}

void NodeStore::applyUpdate(uint32_t node_id, const contacts::NodeUpdate& update)
{
    core_.applyUpdate(node_id, update);
}

void NodeStore::upsert(uint32_t node_id, const char* short_name, const char* long_name,
                       uint32_t now_secs, float snr, float rssi, uint8_t protocol,
                       uint8_t role, uint8_t hops_away, uint8_t hw_model, uint8_t channel)
{
    NODE_STORE_LOG("[NodeStore] upsert node=%08lX ts=%lu snr=%.1f rssi=%.1f\n",
                   static_cast<unsigned long>(node_id),
                   static_cast<unsigned long>(now_secs),
                   snr,
                   rssi);
    core_.upsert(node_id, short_name, long_name, now_secs, snr, rssi,
                 protocol, role, hops_away, hw_model, channel);
}

void NodeStore::updateProtocol(uint32_t node_id, uint8_t protocol, uint32_t now_secs)
{
    core_.updateProtocol(node_id, protocol, now_secs);
}

void NodeStore::updatePosition(uint32_t node_id, const contacts::NodePosition& position)
{
    core_.updatePosition(node_id, position);
}

bool NodeStore::remove(uint32_t node_id)
{
    const bool removed = core_.remove(node_id);
    if (removed)
    {
        NODE_STORE_LOG("[NodeStore] remove node=%08lX remaining=%u\n",
                       static_cast<unsigned long>(node_id),
                       static_cast<unsigned>(core_.getEntries().size()));
    }
    return removed;
}

const std::vector<contacts::NodeEntry>& NodeStore::getEntries() const
{
    return core_.getEntries();
}

void NodeStore::clear()
{
    core_.clear();
}

bool NodeStore::flush()
{
    return core_.flush();
}

bool NodeStore::loadBlob(std::vector<uint8_t>& out)
{
    out.clear();

    if (backend_ == StorageBackend::Sd)
    {
        if (loadFromSd(out))
        {
            NODE_STORE_LOG("[NodeStore] load source=sd path=%s len=%u\n",
                           kPersistNodesFile,
                           static_cast<unsigned>(out.size()));
            return true;
        }
    }
    else
    {
        if (loadFromNvs(out))
        {
            NODE_STORE_LOG("[NodeStore] load source=nvs ns=%s len=%u\n",
                           kPersistNodesNs,
                           static_cast<unsigned>(out.size()));
            return true;
        }
    }

    NODE_STORE_LOG("[NodeStore] load source=none\n");
    return false;
}

bool NodeStore::saveBlob(const uint8_t* data, size_t len)
{
    if (backend_ == StorageBackend::Sd)
    {
        const bool ok = saveToSd(data, len);
        NODE_STORE_LOG("[NodeStore] save target=sd len=%u count=%u ok=%u\n",
                       static_cast<unsigned>(len),
                       static_cast<unsigned>(contacts::nodeBlobEntryCount(len)),
                       ok ? 1U : 0U);
        return ok;
    }

    const bool ok = saveToNvs(data, len);
    NODE_STORE_LOG("[NodeStore] save target=nvs len=%u count=%u ok=%u\n",
                   static_cast<unsigned>(len),
                   static_cast<unsigned>(contacts::nodeBlobEntryCount(len)),
                   ok ? 1U : 0U);
    return ok;
}

void NodeStore::clearBlob()
{
    if (SD.cardType() != CARD_NONE && SD.exists(kPersistNodesFile))
    {
        SD.remove(kPersistNodesFile);
    }
    clearNvs();
}

bool NodeStore::loadFromNvs(std::vector<uint8_t>& out)
{
    out.clear();

    chat::infra::PreferencesBlobMetadata meta;
    if (!chat::infra::loadRawBlobFromPreferencesWithMetadata(kPersistNodesNs,
                                                             kPersistNodesKey,
                                                             kPersistNodesKeyVer,
                                                             kPersistNodesKeyCrc,
                                                             out,
                                                             &meta))
    {
        NODE_STORE_LOG("[NodeStore] NVS blob missing/unreadable ns=%s\n", kPersistNodesNs);
        logNvsStats("begin", kPersistNodesNs);
        return false;
    }

    NODE_STORE_LOG("[NodeStore] blob len=%u ver=%u crc=%08lX has_crc=%u\n",
                   static_cast<unsigned>(meta.len),
                   static_cast<unsigned>(meta.version),
                   static_cast<unsigned long>(meta.crc),
                   meta.has_crc ? 1U : 0U);

    if (meta.len == 0)
    {
        if (meta.has_crc || meta.has_version)
        {
            NODE_STORE_LOG("[NodeStore] stale meta detected ver=%u has_crc=%u\n",
                           static_cast<unsigned>(meta.version),
                           meta.has_crc ? 1U : 0U);
        }
        NODE_STORE_LOG("[NodeStore] NVS backup empty\n");
        return false;
    }

    if (!meta.has_version)
    {
        NODE_STORE_LOG("[NodeStore] missing version len=%u\n",
                       static_cast<unsigned>(meta.len),
                       static_cast<unsigned>(meta.len));
        out.clear();
        return false;
    }

    const size_t entry_size = contacts::nodeBlobEntrySizeForVersion(meta.version);
    if (entry_size == 0 ||
        meta.len == 0 ||
        (meta.len % entry_size) != 0 ||
        (meta.len / entry_size) > contacts::NodeStoreCore::kMaxNodes)
    {
        NODE_STORE_LOG("[NodeStore] invalid blob shape len=%u ver=%u\n",
                       static_cast<unsigned>(meta.len),
                       static_cast<unsigned>(meta.version));
        out.clear();
        return false;
    }

    if (!meta.has_crc)
    {
        NODE_STORE_LOG("[NodeStore] missing crc\n");
        out.clear();
        return false;
    }

    const uint32_t calc_crc = contacts::NodeStoreCore::computeBlobCrc(out.data(), out.size());
    if (calc_crc != meta.crc)
    {
        NODE_STORE_LOG("[NodeStore] crc mismatch stored=%08lX calc=%08lX\n",
                       static_cast<unsigned long>(meta.crc),
                       static_cast<unsigned long>(calc_crc));
        out.clear();
        return false;
    }

    std::vector<uint8_t> normalized;
    if (!canonicalizeNodePayload(out.data(), out.size(), meta.version, normalized))
    {
        NODE_STORE_LOG("[NodeStore] decode failed ver=%u len=%u\n",
                       static_cast<unsigned>(meta.version),
                       static_cast<unsigned>(out.size()));
        out.clear();
        return false;
    }

    out.swap(normalized);
    NODE_STORE_LOG("[NodeStore] loaded=%u\n",
                   static_cast<unsigned>(out.size() / contacts::NodeStoreCore::kSerializedEntrySizeV8));
    return true;
}

bool NodeStore::loadFromSd(std::vector<uint8_t>& out) const
{
    if (SD.cardType() == CARD_NONE)
    {
        NODE_STORE_LOG("[NodeStore] load SD skipped: card none\n");
        return false;
    }

    File file = SD.open(kPersistNodesFile, FILE_READ);
    if (!file)
    {
        NODE_STORE_LOG("[NodeStore] load SD open failed path=%s\n", kPersistNodesFile);
        return false;
    }

    const size_t file_size = static_cast<size_t>(file.size());
    if (file_size < sizeof(contacts::NodeStoreSdHeader))
    {
        NODE_STORE_LOG("[NodeStore] load SD short file path=%s size=%u\n",
                       kPersistNodesFile,
                       static_cast<unsigned>(file_size));
        file.close();
        return false;
    }

    contacts::NodeStoreSdHeader header{};
    if (file.read(reinterpret_cast<uint8_t*>(&header), sizeof(header)) != sizeof(header))
    {
        NODE_STORE_LOG("[NodeStore] load SD header read failed path=%s\n", kPersistNodesFile);
        file.close();
        return false;
    }

    if (header.count == 0 || header.count > contacts::NodeStoreCore::kMaxNodes)
    {
        NODE_STORE_LOG("[NodeStore] load SD header invalid path=%s ver=%u count=%u\n",
                       kPersistNodesFile,
                       static_cast<unsigned>(header.ver),
                       static_cast<unsigned>(header.count));
        file.close();
        return false;
    }

    const size_t payload_len = file_size - sizeof(header);
    if (payload_len == 0)
    {
        NODE_STORE_LOG("[NodeStore] load SD empty payload path=%s ver=%u count=%u\n",
                       kPersistNodesFile,
                       static_cast<unsigned>(header.ver),
                       static_cast<unsigned>(header.count));
        file.close();
        return false;
    }

    out.resize(payload_len);
    const size_t read_bytes = file.read(out.data(), payload_len);
    file.close();

    if (read_bytes != payload_len)
    {
        NODE_STORE_LOG("[NodeStore] load SD payload read mismatch path=%s want=%u got=%u\n",
                       kPersistNodesFile,
                       static_cast<unsigned>(payload_len),
                       static_cast<unsigned>(read_bytes));
        out.clear();
        return false;
    }

    const size_t entry_size = contacts::nodeBlobEntrySizeForVersion(header.ver);
    if (entry_size == 0)
    {
        NODE_STORE_LOG("[NodeStore] load SD unsupported version path=%s ver=%u\n",
                       kPersistNodesFile,
                       static_cast<unsigned>(header.ver));
        out.clear();
        return false;
    }

    const size_t expected_len = static_cast<size_t>(header.count) * entry_size;
    if (expected_len != out.size())
    {
        NODE_STORE_LOG("[NodeStore] load SD blob size mismatch path=%s len=%u ver=%u count=%u expected=%u\n",
                       kPersistNodesFile,
                       static_cast<unsigned>(out.size()),
                       static_cast<unsigned>(header.ver),
                       static_cast<unsigned>(header.count),
                       static_cast<unsigned>(expected_len));
        out.clear();
        return false;
    }

    const uint32_t calc_crc = contacts::NodeStoreCore::computeBlobCrc(out.data(), out.size());
    if (calc_crc != header.crc)
    {
        NODE_STORE_LOG("[NodeStore] load SD crc mismatch path=%s stored=%08lX calc=%08lX\n",
                       kPersistNodesFile,
                       static_cast<unsigned long>(header.crc),
                       static_cast<unsigned long>(calc_crc));
        out.clear();
        return false;
    }

    std::vector<uint8_t> normalized;
    if (!canonicalizeNodePayload(out.data(), out.size(), header.ver, normalized))
    {
        NODE_STORE_LOG("[NodeStore] load SD decode failed path=%s ver=%u len=%u\n",
                       kPersistNodesFile,
                       static_cast<unsigned>(header.ver),
                       static_cast<unsigned>(out.size()));
        out.clear();
        return false;
    }

    out.swap(normalized);
    NODE_STORE_LOG("[NodeStore] loaded=%u (SD)\n",
                   static_cast<unsigned>(out.size() / contacts::NodeStoreCore::kSerializedEntrySizeV8));
    return true;
}

bool NodeStore::saveToNvs(const uint8_t* data, size_t len) const
{
    chat::infra::PreferencesBlobMetadata meta;
    if (data && len > 0)
    {
        if (!contacts::isValidNodeBlobSize(len) ||
            contacts::nodeBlobEntryCount(len) > contacts::NodeStoreCore::kMaxNodes)
        {
            return false;
        }
        meta.len = len;
        meta.has_version = true;
        meta.version = contacts::NodeStoreCore::kPersistVersion;
        meta.has_crc = true;
        meta.crc = contacts::NodeStoreCore::computeBlobCrc(data, len);
    }

    const bool ok = chat::infra::saveRawBlobToPreferencesWithMetadata(kPersistNodesNs,
                                                                      kPersistNodesKey,
                                                                      kPersistNodesKeyVer,
                                                                      kPersistNodesKeyCrc,
                                                                      data,
                                                                      len,
                                                                      &meta,
                                                                      true);
    if (!ok)
    {
        NODE_STORE_LOG("[NodeStore] save failed ns=%s\n", kPersistNodesNs);
        logNvsStats("save-write", kPersistNodesNs);
    }
    return ok;
}

bool NodeStore::saveToSd(const uint8_t* data, size_t len) const
{
    if (SD.cardType() == CARD_NONE)
    {
        return false;
    }
    if (!contacts::isValidNodeBlobSize(len) ||
        contacts::nodeBlobEntryCount(len) > contacts::NodeStoreCore::kMaxNodes)
    {
        return false;
    }

    const std::string temp_path = std::string(kPersistNodesFile) + ".tmp";
    if (SD.exists(temp_path.c_str()))
    {
        SD.remove(temp_path.c_str());
    }

    File file = SD.open(temp_path.c_str(), FILE_WRITE);
    if (!file)
    {
        return false;
    }

    contacts::NodeStoreSdHeader header = contacts::makeNodeStoreSdHeader(data, len);

    bool ok = (file.write(reinterpret_cast<const uint8_t*>(&header), sizeof(header)) == sizeof(header));
    if (ok && data && len > 0)
    {
        ok = (file.write(data, len) == len);
    }

    file.close();
    if (!ok)
    {
        SD.remove(temp_path.c_str());
        return false;
    }

    if (SD.exists(kPersistNodesFile))
    {
        SD.remove(kPersistNodesFile);
    }

    if (!SD.rename(temp_path.c_str(), kPersistNodesFile))
    {
        SD.remove(temp_path.c_str());
        return false;
    }

    NODE_STORE_LOG("[NodeStore] save SD path=%s len=%u ver=%u count=%u\n",
                   kPersistNodesFile,
                   static_cast<unsigned>(len),
                   static_cast<unsigned>(header.ver),
                   static_cast<unsigned>(header.count));
    return true;
}

void NodeStore::clearNvs() const
{
    chat::infra::clearPreferencesKeys(kPersistNodesNs,
                                      kPersistNodesKey,
                                      kPersistNodesKeyVer,
                                      kPersistNodesKeyCrc);
}

} // namespace meshtastic
} // namespace chat
