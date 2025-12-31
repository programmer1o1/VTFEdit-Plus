#pragma once

#include <QByteArray>
#include <QMap>
#include <QString>
#include <QVector>

#include <cstdint>

class VtfSheet final {
public:
    struct TexCoord final {
        float left = 0.0f;
        float top = 0.0f;
        float right = 1.0f;
        float bottom = 1.0f;
    };

    struct Frame final {
        float duration = 0.1f;
        TexCoord a{};
        TexCoord b{};
        TexCoord c{};
        TexCoord d{};
    };

    struct Sequence final {
        std::uint32_t seqNum = 0;
        bool clamp = false;
        float totalTime = 0.0f;
        QVector<Frame> frames{};
    };

    std::uint32_t version = 0;  // v0 or v1
    QMap<std::uint32_t, Sequence> sequences{};

    static bool parse(const QByteArray &data, VtfSheet *out, QString *error);
    QByteArray serialize(std::uint32_t outVersion, QString *error) const;

    static TexCoord defaultFullFrame();
};

