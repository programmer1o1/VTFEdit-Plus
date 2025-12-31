#include "QtUtil.h"

#include <QFile>
#include <QImage>

#include <array>
#include <cstdint>
#include <cstring>

QImage ensureRgba8888(const QImage &image) {
    if(image.isNull()) return {};
    if(image.format() == QImage::Format_RGBA8888) return image;
    return image.convertToFormat(QImage::Format_RGBA8888);
}

std::vector<vlByte> qimageToContiguousRgba(const QImage &rgba8888) {
    const int w = rgba8888.width();
    const int h = rgba8888.height();
    std::vector<vlByte> out(static_cast<size_t>(w) * static_cast<size_t>(h) * 4);
    for(int y = 0; y < h; ++y) {
        const uchar *row = rgba8888.constScanLine(y);
        std::memcpy(out.data() + static_cast<size_t>(y) * static_cast<size_t>(w) * 4, row, static_cast<size_t>(w) * 4);
    }
    return out;
}

bool vtflibCanEncode(VTFImageFormat format) {
    // Cache results (VTFLib's capability won't change during runtime).
    static std::array<int8_t, IMAGE_FORMAT_COUNT> cache{}; // 0 = unknown, 1 = can encode, -1 = can't encode

    if(format < 0 || format >= IMAGE_FORMAT_COUNT) return false;

    int8_t &v = cache[static_cast<size_t>(format)];
    if(v != 0) return v > 0;

    // Check VTFLib's own support list first.
    SVTFImageFormatInfo info{};
    if(vlImageGetImageFormatInfoEx(format, &info) == 0 || info.bIsSupported == 0) {
        v = -1;
        return false;
    }

    // Probe conversion from RGBA8888. For block compression, 4x4 is valid.
    constexpr int w = 4;
    constexpr int h = 4;
    std::array<vlByte, w * h * 4> rgba{};
    const vlUInt outSize = vlImageComputeImageSize(w, h, 1, 1, format);
    if(outSize == 0) {
        v = -1;
        return false;
    }
    std::vector<vlByte> out(static_cast<size_t>(outSize));
    const bool ok = vlImageConvertFromRGBA8888(rgba.data(), out.data(), w, h, format) != 0;
    v = ok ? 1 : -1;
    return ok;
}

bool writeTextFileUtf8(const QString &path, const QString &text, QString *error) {
    QFile file(path);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if(error) *error = file.errorString();
        return false;
    }
    const QByteArray bytes = text.toUtf8();
    if(file.write(bytes) != bytes.size()) {
        if(error) *error = file.errorString();
        return false;
    }
    return true;
}

QString loadTextFileUtf8(const QString &path, QString *error) {
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if(error) *error = file.errorString();
        return {};
    }
    const QByteArray bytes = file.readAll();
    return QString::fromUtf8(bytes);
}

bool createVtfFromRgbaAndSave(const QImage &rgba, const SVTFCreateOptions &opts, const QString &outPath, QString *error) {
    if(rgba.isNull()) {
        if(error) *error = "Input image is empty.";
        return false;
    }
    QImage rgba8888 = rgba;
    if(rgba8888.format() != QImage::Format_RGBA8888) rgba8888 = rgba8888.convertToFormat(QImage::Format_RGBA8888);

    vlUInt id = 0;
    if(!vlCreateImage(&id) || !vlBindImage(id)) {
        if(error) *error = QString::fromUtf8(vlGetLastError());
        if(id) vlDeleteImage(id);
        return false;
    }

    SVTFCreateOptions createOpts = opts;
    std::vector<vlByte> rgbaBytes = qimageToContiguousRgba(rgba8888);
    if(!vlImageCreateSingle(static_cast<vlUInt>(rgba8888.width()), static_cast<vlUInt>(rgba8888.height()), rgbaBytes.data(), &createOpts)) {
        if(error) *error = QString::fromUtf8(vlGetLastError());
        vlDeleteImage(id);
        return false;
    }

    const QByteArray encoded = QFile::encodeName(outPath);
    if(!vlImageSave(encoded.constData())) {
        if(error) *error = QString::fromUtf8(vlGetLastError());
        vlDeleteImage(id);
        return false;
    }

    vlDeleteImage(id);
    return true;
}
