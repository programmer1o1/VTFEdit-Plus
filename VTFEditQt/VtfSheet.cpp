#include "VtfSheet.h"

#include <QtEndian>

#include <cstring>

namespace {

bool fail(QString *error, const QString &msg) {
    if(error) *error = msg;
    return false;
}

bool readU32(const QByteArray &data, int &off, std::uint32_t &out) {
    if(off < 0 || off + 4 > data.size()) return false;
    std::uint32_t tmp = 0;
    std::memcpy(&tmp, data.constData() + off, 4);
    out = static_cast<std::uint32_t>(qFromLittleEndian<quint32>(static_cast<quint32>(tmp)));
    off += 4;
    return true;
}

bool readU8(const QByteArray &data, int &off, std::uint8_t &out) {
    if(off < 0 || off + 1 > data.size()) return false;
    out = static_cast<std::uint8_t>(data.at(off));
    off += 1;
    return true;
}

bool readF32(const QByteArray &data, int &off, float &out) {
    std::uint32_t bits = 0;
    if(!readU32(data, off, bits)) return false;
    static_assert(sizeof(float) == sizeof(std::uint32_t), "float must be 32-bit.");
    std::memcpy(&out, &bits, sizeof(float));
    return true;
}

bool readTexCoord(const QByteArray &data, int &off, VtfSheet::TexCoord &out) {
    return readF32(data, off, out.left) &&
           readF32(data, off, out.top) &&
           readF32(data, off, out.right) &&
           readF32(data, off, out.bottom);
}

void appendU32(QByteArray &out, std::uint32_t v) {
    const quint32 le = qToLittleEndian<quint32>(static_cast<quint32>(v));
    out.append(reinterpret_cast<const char *>(&le), 4);
}

void appendU8(QByteArray &out, std::uint8_t v) {
    out.append(reinterpret_cast<const char *>(&v), 1);
}

void appendF32(QByteArray &out, float f) {
    std::uint32_t bits = 0;
    static_assert(sizeof(float) == sizeof(std::uint32_t), "float must be 32-bit.");
    std::memcpy(&bits, &f, sizeof(float));
    appendU32(out, bits);
}

void appendTexCoord(QByteArray &out, const VtfSheet::TexCoord &tc) {
    appendF32(out, tc.left);
    appendF32(out, tc.top);
    appendF32(out, tc.right);
    appendF32(out, tc.bottom);
}

} // namespace

VtfSheet::TexCoord VtfSheet::defaultFullFrame() {
    return TexCoord{0.0f, 0.0f, 1.0f, 1.0f};
}

bool VtfSheet::parse(const QByteArray &data, VtfSheet *out, QString *error) {
    if(!out) return fail(error, "Internal error: null output pointer.");
    out->version = 0;
    out->sequences.clear();

    int off = 0;
    std::uint32_t version = 0;
    std::uint32_t seqCount = 0;
    if(!readU32(data, off, version) || !readU32(data, off, seqCount)) {
        return fail(error, "Sheet data is truncated (missing header).");
    }

    if(version > 1) {
        return fail(error, QString("Unknown sheet resource version %1.").arg(version));
    }
    if(seqCount > 64) {
        return fail(error, QString("Too many sequences (%1). Max is 64.").arg(seqCount));
    }

    out->version = version;

    for(std::uint32_t i = 0; i < seqCount; ++i) {
        std::uint32_t seqNum = 0;
        if(!readU32(data, off, seqNum)) return fail(error, "Sheet data is truncated (sequence header).");
        if(seqNum >= 64) return fail(error, QString("Invalid sequence number %1 (must be 0-63).").arg(seqNum));
        if(out->sequences.contains(seqNum)) return fail(error, QString("Duplicate sequence number %1.").arg(seqNum));

        // struct: <Ixxx?If
        if(off + 3 > data.size()) return fail(error, "Sheet data is truncated (sequence header padding).");
        off += 3;

        std::uint8_t clampByte = 0;
        if(!readU8(data, off, clampByte)) return fail(error, "Sheet data is truncated (sequence clamp).");

        std::uint32_t frameCount = 0;
        float totalTime = 0.0f;
        if(!readU32(data, off, frameCount) || !readF32(data, off, totalTime)) {
            return fail(error, "Sheet data is truncated (sequence header).");
        }

        Sequence seq{};
        seq.seqNum = seqNum;
        seq.clamp = clampByte != 0;
        seq.totalTime = totalTime;
        seq.frames.reserve(static_cast<int>(frameCount));

        for(std::uint32_t f = 0; f < frameCount; ++f) {
            Frame frame{};
            if(!readF32(data, off, frame.duration)) return fail(error, "Sheet data is truncated (frame duration).");
            if(version == 0) {
                TexCoord tc{};
                if(!readTexCoord(data, off, tc)) return fail(error, "Sheet data is truncated (frame texcoord).");
                frame.a = tc;
                frame.b = tc;
                frame.c = tc;
                frame.d = tc;
            } else {
                if(!readTexCoord(data, off, frame.a) ||
                   !readTexCoord(data, off, frame.b) ||
                   !readTexCoord(data, off, frame.c) ||
                   !readTexCoord(data, off, frame.d)) {
                    return fail(error, "Sheet data is truncated (frame texcoords).");
                }
            }
            seq.frames.push_back(frame);
        }

        out->sequences.insert(seqNum, seq);
    }

    return true;
}

QByteArray VtfSheet::serialize(std::uint32_t outVersion, QString *error) const {
    if(outVersion > 1) {
        if(error) *error = QString("Unknown sheet resource version %1.").arg(outVersion);
        return {};
    }
    if(sequences.size() > 64) {
        if(error) *error = QString("Too many sequences (%1). Max is 64.").arg(sequences.size());
        return {};
    }

    QByteArray out;
    out.reserve(256);

    appendU32(out, outVersion);
    appendU32(out, static_cast<std::uint32_t>(sequences.size()));

    for(const auto &seq : sequences) {
        if(seq.seqNum >= 64) {
            if(error) *error = QString("Invalid sequence number %1 (must be 0-63).").arg(seq.seqNum);
            return {};
        }

        appendU32(out, seq.seqNum);
        out.append("\0\0\0", 3);
        appendU8(out, static_cast<std::uint8_t>(seq.clamp ? 1 : 0));
        appendU32(out, static_cast<std::uint32_t>(seq.frames.size()));
        appendF32(out, seq.totalTime);

        for(const auto &frame : seq.frames) {
            appendF32(out, frame.duration);
            appendTexCoord(out, frame.a);
            if(outVersion == 1) {
                appendTexCoord(out, frame.b);
                appendTexCoord(out, frame.c);
                appendTexCoord(out, frame.d);
            }
        }
    }

    return out;
}

