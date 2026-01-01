#include "WadFile.h"

#include <QByteArray>
#include <QFile>
#include <QtEndian>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace {

template <typename T>
bool readPodAt(const QByteArray &data, qsizetype offset, T *out) {
    if(!out) return false;
    if(offset < 0) return false;
    if(offset + static_cast<qsizetype>(sizeof(T)) > data.size()) return false;
    std::memcpy(out, data.constData() + offset, sizeof(T));
    return true;
}

template <typename T>
T readLe(const void *p) {
    static_assert(std::is_trivially_copyable_v<T>);
    if constexpr(sizeof(T) == 2) {
        const quint16 u = qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(p));
        return static_cast<T>(u);
    } else if constexpr(sizeof(T) == 4) {
        const quint32 u = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(p));
        return static_cast<T>(u);
    } else {
        T v{};
        std::memcpy(&v, p, sizeof(T));
        return v;
    }
}

#pragma pack(push, 1)
struct WadHeader {
    char magic[4];
    int32_t lumpCount;
    int32_t dirOffset;
};

struct WadDirEntry {
    int32_t filePos;
    int32_t diskSize;
    int32_t size;
    uint8_t type;
    uint8_t compression;
    uint16_t pad;
    char name[16];
};

struct MipTexHeader {
    char name[16];
    uint32_t width;
    uint32_t height;
    uint32_t offsets[4];
};
#pragma pack(pop)

QString textureNameFrom16(const char name[16]) {
    size_t n = 0;
    for(; n < 16; ++n) {
        if(name[n] == '\0') break;
    }
    return QString::fromLatin1(name, static_cast<int>(n));
}

bool isWadMagic(const WadHeader &h) {
    return (std::memcmp(h.magic, "WAD2", 4) == 0) || (std::memcmp(h.magic, "WAD3", 4) == 0);
}

bool decodeMipTexToRgba8888(const QByteArray &data, const WadDirEntry &entry, WadTexture *out, QString *error) {
    if(!out) return false;

    const qsizetype base = static_cast<qsizetype>(entry.filePos);
    if(base < 0 || base >= data.size()) {
        if(error) *error = "Invalid lump offset.";
        return false;
    }

    MipTexHeader mip{};
    if(!readPodAt(data, base, &mip)) {
        if(error) *error = "Failed to read mip texture header.";
        return false;
    }

    const QString name = textureNameFrom16(mip.name);
    const uint32_t w = readLe<uint32_t>(&mip.width);
    const uint32_t h = readLe<uint32_t>(&mip.height);
    if(w == 0 || h == 0 || w > 16384 || h > 16384) {
        if(error) *error = "Invalid texture dimensions.";
        return false;
    }

    uint32_t offs[4] = {readLe<uint32_t>(&mip.offsets[0]),
                        readLe<uint32_t>(&mip.offsets[1]),
                        readLe<uint32_t>(&mip.offsets[2]),
                        readLe<uint32_t>(&mip.offsets[3])};
    for(int i = 0; i < 4; ++i) {
        if(offs[i] == 0) {
            if(error) *error = "Invalid mip offset.";
            return false;
        }
    }

    const uint64_t size0 = static_cast<uint64_t>(w) * static_cast<uint64_t>(h);
    const uint64_t size3 = std::max<uint64_t>(1, (w >> 3)) * std::max<uint64_t>(1, (h >> 3));

    const qsizetype pix0Off = base + static_cast<qsizetype>(offs[0]);
    if(pix0Off < 0 || pix0Off + static_cast<qsizetype>(size0) > data.size()) {
        if(error) *error = "Invalid mip0 pixel data offset.";
        return false;
    }

    const qsizetype palSizeOff = base + static_cast<qsizetype>(offs[3]) + static_cast<qsizetype>(size3);
    if(palSizeOff < 0 || palSizeOff + 2 > data.size()) {
        if(error) *error = "Invalid palette offset.";
        return false;
    }

    const uint16_t palCount = readLe<uint16_t>(data.constData() + palSizeOff);
    const qsizetype palOff = palSizeOff + 2;
    if(palCount == 0 || palCount > 256) {
        if(error) *error = "Unsupported palette size.";
        return false;
    }
    if(palOff < 0 || palOff + static_cast<qsizetype>(palCount) * 3 > data.size()) {
        if(error) *error = "Palette data out of bounds.";
        return false;
    }

    const auto *pixels = reinterpret_cast<const uint8_t *>(data.constData() + pix0Off);
    const auto *pal = reinterpret_cast<const uint8_t *>(data.constData() + palOff);

    QImage img(static_cast<int>(w), static_cast<int>(h), QImage::Format_RGBA8888);
    if(img.isNull()) {
        if(error) *error = "Failed to allocate image.";
        return false;
    }

    const bool useIndex255Transparency = name.startsWith('{');
    bool hasTransparency = false;

    for(uint32_t y = 0; y < h; ++y) {
        uint8_t *dst = img.scanLine(static_cast<int>(y));
        for(uint32_t x = 0; x < w; ++x) {
            const uint8_t idx = pixels[static_cast<size_t>(y) * static_cast<size_t>(w) + static_cast<size_t>(x)];
            const uint8_t c = static_cast<uint8_t>(std::min<uint16_t>(idx, static_cast<uint16_t>(palCount - 1)));
            const uint8_t r = pal[static_cast<size_t>(c) * 3 + 0];
            const uint8_t g = pal[static_cast<size_t>(c) * 3 + 1];
            const uint8_t b = pal[static_cast<size_t>(c) * 3 + 2];
            uint8_t a = 255;
            if(useIndex255Transparency && idx == 255) {
                a = 0;
                hasTransparency = true;
            }
            dst[x * 4 + 0] = r;
            dst[x * 4 + 1] = g;
            dst[x * 4 + 2] = b;
            dst[x * 4 + 3] = a;
        }
    }

    out->name = name;
    out->rgba8888 = img;
    out->hasTransparency = hasTransparency;
    return true;
}

} // namespace

bool readWadTextures(const QString &wadPath, QList<WadTexture> *outTextures, QString *error) {
    if(!outTextures) return false;
    outTextures->clear();

    QFile f(wadPath);
    if(!f.open(QIODevice::ReadOnly)) {
        if(error) *error = f.errorString();
        return false;
    }
    const QByteArray data = f.readAll();
    if(data.size() < static_cast<qsizetype>(sizeof(WadHeader))) {
        if(error) *error = "File is too small to be a WAD.";
        return false;
    }

    WadHeader header{};
    if(!readPodAt(data, 0, &header) || !isWadMagic(header)) {
        if(error) *error = "Not a supported WAD2/WAD3 file.";
        return false;
    }

    const int32_t lumpCount = readLe<int32_t>(&header.lumpCount);
    const int32_t dirOffset = readLe<int32_t>(&header.dirOffset);
    if(lumpCount <= 0 || lumpCount > 100000) {
        if(error) *error = "Invalid lump count.";
        return false;
    }
    if(dirOffset < 0 || static_cast<qsizetype>(dirOffset) >= data.size()) {
        if(error) *error = "Invalid directory offset.";
        return false;
    }
    const qsizetype dirEnd = static_cast<qsizetype>(dirOffset) + static_cast<qsizetype>(lumpCount) * sizeof(WadDirEntry);
    if(dirEnd > data.size()) {
        if(error) *error = "Directory out of bounds.";
        return false;
    }

    for(int32_t i = 0; i < lumpCount; ++i) {
        WadDirEntry entry{};
        const qsizetype off = static_cast<qsizetype>(dirOffset) + static_cast<qsizetype>(i) * sizeof(WadDirEntry);
        if(!readPodAt(data, off, &entry)) continue;

        // WAD3 MIP texture lump type is 0x43 ('C').
        if(entry.type != 0x43) continue;

        WadTexture tex{};
        QString err;
        if(!decodeMipTexToRgba8888(data, entry, &tex, &err)) {
            // Skip invalid lumps; caller can surface a summary if desired.
            continue;
        }
        if(tex.name.isEmpty()) tex.name = textureNameFrom16(entry.name);
        if(tex.name.isEmpty()) tex.name = QString("texture_%1").arg(i);
        outTextures->push_back(tex);
    }

    if(outTextures->isEmpty()) {
        if(error) *error = "No supported texture lumps found in WAD.";
        return false;
    }

    return true;
}

QString sanitizeWadTextureNameForFile(const QString &name) {
    QString out = name;
    out = out.trimmed();
    if(out.isEmpty()) out = "texture";
    for(auto &ch : out) {
        if(ch == '/' || ch == '\\' || ch == ':' || ch == '*' || ch == '?' || ch == '"' || ch == '<' || ch == '>' || ch == '|' ||
           ch.unicode() < 0x20) {
            ch = '_';
        }
    }
    return out;
}
