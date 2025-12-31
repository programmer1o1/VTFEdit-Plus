#pragma once

#include <QImage>
#include <QList>
#include <QString>

struct WadTexture {
    QString name;
    QImage rgba8888; // QImage::Format_RGBA8888
    bool hasTransparency = false;
};

// Reads WAD2/WAD3 and extracts MIP texture lumps as RGBA8888 images.
bool readWadTextures(const QString &wadPath, QList<WadTexture> *outTextures, QString *error);

// Best-effort filename sanitizer for WAD texture names.
QString sanitizeWadTextureNameForFile(const QString &name);

