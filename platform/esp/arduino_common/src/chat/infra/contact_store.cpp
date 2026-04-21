/**
 * @file contact_store.cpp
 * @brief Contact nickname storage shell implementation
 */

#include "platform/esp/arduino_common/chat/infra/contact_store.h"
#include "../internal/blob_store_io.h"

#include <SD.h>

namespace chat
{
namespace contacts
{

#ifndef CONTACT_STORE_LOG_ENABLE
#define CONTACT_STORE_LOG_ENABLE 1
#endif

#if CONTACT_STORE_LOG_ENABLE
#define CONTACT_STORE_LOG(...) Serial.printf(__VA_ARGS__)
#else
#define CONTACT_STORE_LOG(...)
#endif

ContactStore::ContactStore()
    : core_(*this)
{
}

void ContactStore::begin()
{
    backend_ = (SD.cardType() != CARD_NONE) ? StorageBackend::Sd : StorageBackend::Flash;
    CONTACT_STORE_LOG("[ContactStore] backend=%s\n",
                      backend_ == StorageBackend::Sd ? "sd" : "flash");
    core_.begin();
}

std::string ContactStore::getNickname(uint32_t node_id) const
{
    return core_.getNickname(node_id);
}

bool ContactStore::setNickname(uint32_t node_id, const char* nickname)
{
    return core_.setNickname(node_id, nickname);
}

bool ContactStore::removeNickname(uint32_t node_id)
{
    return core_.removeNickname(node_id);
}

bool ContactStore::hasNickname(const char* nickname) const
{
    return core_.hasNickname(nickname);
}

std::vector<uint32_t> ContactStore::getAllContactIds() const
{
    return core_.getAllContactIds();
}

size_t ContactStore::getCount() const
{
    return core_.getCount();
}

bool ContactStore::loadBlob(std::vector<uint8_t>& out)
{
    if (backend_ == StorageBackend::Sd)
    {
        if (loadFromSD(out))
        {
            CONTACT_STORE_LOG("[ContactStore] load source=sd path=%s len=%u\n",
                              kSdPath,
                              static_cast<unsigned>(out.size()));
            return true;
        }
    }
    else
    {
        if (loadFromFlash(out))
        {
            CONTACT_STORE_LOG("[ContactStore] load source=flash ns=%s len=%u\n",
                              kPrefNs,
                              static_cast<unsigned>(out.size()));
            return true;
        }
    }
    out.clear();
    CONTACT_STORE_LOG("[ContactStore] load source=none\n");
    return false;
}

bool ContactStore::saveBlob(const uint8_t* data, size_t len)
{
    if (backend_ == StorageBackend::Sd)
    {
        const bool ok = saveToSD(data, len);
        CONTACT_STORE_LOG("[ContactStore] save target=sd path=%s len=%u ok=%u\n",
                          kSdPath,
                          static_cast<unsigned>(len),
                          ok ? 1U : 0U);
        return ok;
    }

    const bool ok = saveToFlash(data, len);
    CONTACT_STORE_LOG("[ContactStore] save target=flash ns=%s len=%u ok=%u\n",
                      kPrefNs,
                      static_cast<unsigned>(len),
                      ok ? 1U : 0U);
    return ok;
}

bool ContactStore::loadFromSD(std::vector<uint8_t>& out) const
{
    return chat::infra::loadRawBlobFromSd(kSdPath, out);
}

bool ContactStore::saveToSD(const uint8_t* data, size_t len) const
{
    return chat::infra::saveRawBlobToSd(kSdPath, data, len);
}

bool ContactStore::loadFromFlash(std::vector<uint8_t>& out) const
{
    return chat::infra::loadRawBlobFromPreferences(kPrefNs, kPrefKey, out);
}

bool ContactStore::saveToFlash(const uint8_t* data, size_t len) const { return chat::infra::saveRawBlobToPreferences(kPrefNs, kPrefKey, data, len); }

} // namespace contacts
} // namespace chat
