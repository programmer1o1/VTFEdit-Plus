#pragma once

#include <VTFLib.h>

#include <QString>

#include <vector>

class QImage;

QImage ensureRgba8888(const QImage &image);
std::vector<vlByte> qimageToContiguousRgba(const QImage &rgba8888);

// Returns true if VTFLib can *encode* to the requested format (not just decode).
// For most formats this is true, but for DXTn it depends on whether a compressor is available.
bool vtflibCanEncode(VTFImageFormat format);

bool writeTextFileUtf8(const QString &path, const QString &text, QString *error);
QString loadTextFileUtf8(const QString &path, QString *error);

bool createVtfFromRgbaAndSave(const QImage &rgba, const SVTFCreateOptions &opts, const QString &outPath, QString *error);
