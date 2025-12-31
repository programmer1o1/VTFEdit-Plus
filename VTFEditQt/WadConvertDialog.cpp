#include "WadConvertDialog.h"

#include "CreateVtfDialog.h"
#include "QtUtil.h"
#include "WadFile.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QCoreApplication>

#include <algorithm>
#include <cstring>

namespace {

QString quoted(const QString &s) {
    QString out = s;
    out.replace('"', "\\\"");
    return QString("\"%1\"").arg(out);
}

QString basicVmtText(const QString &shader, const QString &baseTexture) {
    const QString shaderName = shader.trimmed().isEmpty() ? "VertexLitGeneric" : shader.trimmed();
    QStringList lines;
    lines << quoted(shaderName);
    lines << "{";
    lines << QString("\t%1 %2").arg(quoted("$basetexture"), quoted(baseTexture));
    lines << "}";
    lines << "";
    return lines.join('\n');
}

SVTFCreateOptions createOptionsFromSettings() {
    SVTFCreateOptions o{};
    vlImageCreateDefaultCreateStructure(&o);
    o.uiVersion[0] = VTF_MAJOR_VERSION;
    o.uiVersion[1] = 5;

    QSettings s;
    o.uiVersion[1] = static_cast<vlUInt>(std::clamp(s.value("options/create/versionMinor", 5).toInt(), 0, 5));
    o.ImageFormat = static_cast<VTFImageFormat>(s.value("options/create/normalFormat", static_cast<int>(IMAGE_FORMAT_DXT5)).toInt());
    o.bMipmaps = s.value("options/create/mipmaps", true).toBool() ? vlTrue : vlFalse;
    o.MipmapFilter = static_cast<VTFMipmapFilter>(s.value("options/create/mipmapFilter", static_cast<int>(MIPMAP_FILTER_DEFAULT)).toInt());
    o.bThumbnail = s.value("options/create/thumbnail", true).toBool() ? vlTrue : vlFalse;
    o.bReflectivity = s.value("options/create/reflectivity", true).toBool() ? vlTrue : vlFalse;
    o.bGammaCorrection = s.value("options/create/gammaEnabled", false).toBool() ? vlTrue : vlFalse;
    o.sGammaCorrection = static_cast<vlSingle>(s.value("options/create/gammaValue", 2.2).toDouble());
    o.bSphereMap = s.value("options/create/sphereMap", false).toBool() ? vlTrue : vlFalse;
    o.bSRGB = s.value("options/create/srgb", false).toBool() ? vlTrue : vlFalse;
    o.nAlphaThreshold = static_cast<vlUInt>(std::clamp(s.value("options/create/alphaThreshold", 0).toInt(), 0, 255));

    const bool resizeEnabled = s.value("options/create/resizeEnabled", false).toBool();
    if(resizeEnabled) {
        o.bResize = vlTrue;
        o.ResizeMethod = static_cast<VTFResizeMethod>(s.value("options/create/resizeMethod", static_cast<int>(RESIZE_NEAREST_POWER2)).toInt());
        o.ResizeFilter = static_cast<VTFMipmapFilter>(s.value("options/create/resizeFilter", static_cast<int>(o.MipmapFilter)).toInt());
        o.bResizeClamp = s.value("options/create/resizeClamp", false).toBool() ? vlTrue : vlFalse;
        o.uiResizeClampWidth = static_cast<vlUInt>(std::max(1, s.value("options/create/resizeClampWidth", 1024).toInt()));
        o.uiResizeClampHeight = static_cast<vlUInt>(std::max(1, s.value("options/create/resizeClampHeight", 1024).toInt()));
    } else {
        o.bResize = vlFalse;
        o.bResizeClamp = vlFalse;
    }

    // Keep "sRGB-ness" consistent: bSRGB influences processing, and TEXTUREFLAGS_SRGB is the header flag.
    if(o.bSRGB) {
        o.uiFlags |= TEXTUREFLAGS_SRGB;
    } else {
        o.uiFlags &= ~static_cast<vlUInt>(TEXTUREFLAGS_SRGB);
    }

    return o;
}

} // namespace

WadConvertDialog::WadConvertDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("WAD Convert");
    setModal(true);
    resize(760, 560);

    createOptions_ = createOptionsFromSettings();
    {
        QSettings s;
        useAlphaFormat_ = s.value("options/create/useAlphaFormat", false).toBool();
        alphaFormat_ = static_cast<VTFImageFormat>(s.value("options/create/alphaFormat", static_cast<int>(IMAGE_FORMAT_DXT5)).toInt());
    }

    auto *layout = new QFormLayout(this);

    auto *wadRow = new QWidget(this);
    auto *wadRowLayout = new QHBoxLayout(wadRow);
    wadRowLayout->setContentsMargins(0, 0, 0, 0);
    wadPath_ = new QLineEdit(this);
    auto *browseWadButton = new QPushButton("Browse…", this);
    connect(browseWadButton, &QPushButton::clicked, this, &WadConvertDialog::browseWad);
    wadRowLayout->addWidget(wadPath_, 1);
    wadRowLayout->addWidget(browseWadButton);
    layout->addRow("WAD file:", wadRow);

    auto *outRow = new QWidget(this);
    auto *outRowLayout = new QHBoxLayout(outRow);
    outRowLayout->setContentsMargins(0, 0, 0, 0);
    outputDir_ = new QLineEdit(this);
    auto *browseOutButton = new QPushButton("Browse…", this);
    connect(browseOutButton, &QPushButton::clicked, this, &WadConvertDialog::browseOutput);
    outRowLayout->addWidget(outputDir_, 1);
    outRowLayout->addWidget(browseOutButton);
    layout->addRow("Output folder:", outRow);

    createOptionsButton_ = new QPushButton("Create options…", this);
    connect(createOptionsButton_, &QPushButton::clicked, this, &WadConvertDialog::chooseCreateOptions);
    layout->addRow("VTF:", createOptionsButton_);

    createVmts_ = new QCheckBox("Create .vmt next to .vtf", this);
    createVmts_->setChecked(false);
    layout->addRow("", createVmts_);

    vmtShader_ = new QLineEdit(this);
    vmtShader_->setPlaceholderText("VertexLitGeneric");
    vmtShader_->setText("VertexLitGeneric");
    layout->addRow("VMT shader:", vmtShader_);

    progressLabel_ = new QLabel(this);
    progressLabel_->setText("Ready.");
    progressLabel_->setStyleSheet("color: palette(mid);");
    layout->addRow("Status:", progressLabel_);

    progressBar_ = new QProgressBar(this);
    progressBar_->setRange(0, 1);
    progressBar_->setValue(0);
    layout->addRow("", progressBar_);

    log_ = new QPlainTextEdit(this);
    log_->setReadOnly(true);
    log_->setMaximumBlockCount(2000);
    layout->addRow("Log:", log_);

    buttons_ = new QDialogButtonBox(QDialogButtonBox::Close, this);
    convertButton_ = buttons_->addButton("Convert", QDialogButtonBox::AcceptRole);
    connect(buttons_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(convertButton_, &QPushButton::clicked, this, &WadConvertDialog::convert);
    layout->addRow(buttons_);
}

WadConvertDialog::~WadConvertDialog() = default;

void WadConvertDialog::appendLog(const QString &line) {
    if(!log_) return;
    log_->appendPlainText(line);
}

void WadConvertDialog::setBusy(bool busy) {
    if(wadPath_) wadPath_->setEnabled(!busy);
    if(outputDir_) outputDir_->setEnabled(!busy);
    if(createVmts_) createVmts_->setEnabled(!busy);
    if(vmtShader_) vmtShader_->setEnabled(!busy);
    if(createOptionsButton_) createOptionsButton_->setEnabled(!busy);
    if(convertButton_) convertButton_->setEnabled(!busy);
}

void WadConvertDialog::browseWad() {
    QSettings s;
    const QString startDir = s.value("paths/wad_convert/wad").toString();
    const QString path = QFileDialog::getOpenFileName(this, "Open WAD", startDir, "WAD files (*.wad);;All files (*.*)");
    if(path.isEmpty()) return;
    s.setValue("paths/wad_convert/wad", QFileInfo(path).absolutePath());
    wadPath_->setText(path);
}

void WadConvertDialog::browseOutput() {
    QSettings s;
    const QString startDir = s.value("paths/wad_convert/out").toString();
    const QString dir = QFileDialog::getExistingDirectory(this, "Select output folder", startDir);
    if(dir.isEmpty()) return;
    s.setValue("paths/wad_convert/out", dir);
    outputDir_->setText(dir);
}

void WadConvertDialog::chooseCreateOptions() {
    CreateVtfDialog dlg(this);
    if(dlg.exec() != QDialog::Accepted) return;
    createOptions_ = dlg.options();
    useAlphaFormat_ = dlg.useAlphaFormat();
    alphaFormat_ = dlg.alphaFormat();
    appendLog("Updated create options for this conversion run.");
}

void WadConvertDialog::convert() {
    const QString wadPath = wadPath_ ? wadPath_->text().trimmed() : QString();
    const QString outDirPath = outputDir_ ? outputDir_->text().trimmed() : QString();
    if(wadPath.isEmpty()) {
        appendLog("Error: pick a WAD file.");
        return;
    }
    if(outDirPath.isEmpty()) {
        appendLog("Error: pick an output folder.");
        return;
    }

    QList<WadTexture> textures;
    QString err;
    if(!readWadTextures(wadPath, &textures, &err)) {
        appendLog(QString("Failed to read WAD: %1").arg(err));
        return;
    }

    QDir outDir(outDirPath);
    if(!outDir.exists()) {
        if(!outDir.mkpath(".")) {
            appendLog("Failed to create output folder.");
            return;
        }
    }

    setBusy(true);
    progressBar_->setRange(0, textures.size());
    progressBar_->setValue(0);

    int okCount = 0;
    int failCount = 0;

    for(int i = 0; i < textures.size(); ++i) {
        const WadTexture &tex = textures[i];
        const QString baseName = sanitizeWadTextureNameForFile(tex.name);
        const QString outVtf = outDir.filePath(baseName + ".vtf");
        const QString outVmt = outDir.filePath(baseName + ".vmt");

        progressLabel_->setText(QString("Converting %1 (%2/%3)…").arg(baseName).arg(i + 1).arg(textures.size()));
        progressBar_->setValue(i);
        QCoreApplication::processEvents();

        vlUInt id = 0;
        if(!vlCreateImage(&id) || !vlBindImage(id)) {
            appendLog(QString("%1: VTFLib create/bind failed: %2").arg(baseName, QString::fromUtf8(vlGetLastError())));
            if(id) vlDeleteImage(id);
            ++failCount;
            continue;
        }

        SVTFCreateOptions opts = createOptions_;
        if(useAlphaFormat_ && tex.hasTransparency) {
            opts.ImageFormat = alphaFormat_;
        }

        if(!vtflibCanEncode(opts.ImageFormat)) {
            appendLog(QString("%1: Selected format cannot be encoded on this build.").arg(baseName));
            vlDeleteImage(id);
            ++failCount;
            continue;
        }
        if(opts.bThumbnail && !vtflibCanEncode(IMAGE_FORMAT_DXT1)) {
            opts.bThumbnail = vlFalse;
            appendLog(QString("%1: DXT1 unavailable; thumbnail skipped.").arg(baseName));
        }

        std::vector<vlByte> rgba = qimageToContiguousRgba(tex.rgba8888);
        if(!vlImageCreateSingle(static_cast<vlUInt>(tex.rgba8888.width()), static_cast<vlUInt>(tex.rgba8888.height()), rgba.data(), &opts)) {
            appendLog(QString("%1: Create failed: %2").arg(baseName, QString::fromUtf8(vlGetLastError())));
            vlDeleteImage(id);
            ++failCount;
            continue;
        }

        // Apply resource templates from Global Options (LOD clamp + Information), if enabled.
        {
            QSettings settings;
            const bool createLod = settings.value("options/resources/createLod", false).toBool();
            const bool createInfo = settings.value("options/resources/createInformation", false).toBool();
            if(createLod || createInfo) {
                if(opts.uiVersion[0] != VTF_MAJOR_VERSION || opts.uiVersion[1] < 3) {
                    appendLog(QString("%1: Resources skipped (requires VTF v7.3+).").arg(baseName));
                } else {
                    if(createLod) {
                        SVTFTextureLODControlResource lod{};
                        lod.ResolutionClampU =
                            static_cast<vlByte>(std::clamp(settings.value("options/resources/lodClampU", 0).toInt(), 0, 255));
                        lod.ResolutionClampV =
                            static_cast<vlByte>(std::clamp(settings.value("options/resources/lodClampV", 0).toInt(), 0, 255));
                        vlUInt packed = 0;
                        std::memcpy(&packed, &lod, sizeof(lod));
                        if(!vlImageSetResourceData(VTF_RSRC_TEXTURE_LOD_SETTINGS, sizeof(vlUInt), &packed)) {
                            appendLog(QString("%1: LOD resource warning: %2").arg(baseName, QString::fromUtf8(vlGetLastError())));
                        }
                    }

                    if(createInfo) {
                        auto add = [](QStringList *lines, const QString &k, const QString &v) {
                            const QString t = v.trimmed();
                            if(t.isEmpty()) return;
                            lines->push_back(QString("\t\"%1\" \"%2\"").arg(k, t));
                        };

                        QStringList lines;
                        lines << "\"Information\"";
                        lines << "{";
                        add(&lines, "Author", settings.value("options/resources/infoAuthor").toString());
                        add(&lines, "Contact", settings.value("options/resources/infoContact").toString());
                        add(&lines, "Version", settings.value("options/resources/infoVersion").toString());
                        add(&lines, "Modification", settings.value("options/resources/infoModification").toString());
                        add(&lines, "Description", settings.value("options/resources/infoDescription").toString());
                        add(&lines, "Comments", settings.value("options/resources/infoComments").toString());
                        lines << "}";
                        lines << "";

                        QByteArray bytes = lines.join('\n').toUtf8();
                        bytes.push_back('\0');
                        if(!vlImageSetResourceData(VTF_RSRC_KEY_VALUE_DATA, static_cast<vlUInt>(bytes.size()), bytes.data())) {
                            appendLog(QString("%1: Information resource warning: %2").arg(baseName, QString::fromUtf8(vlGetLastError())));
                        }
                    }
                }
            }
        }

        const QByteArray encoded = QFile::encodeName(outVtf);
        if(!vlImageSave(encoded.constData())) {
            appendLog(QString("%1: Save failed: %2").arg(baseName, QString::fromUtf8(vlGetLastError())));
            vlDeleteImage(id);
            ++failCount;
            continue;
        }

        vlDeleteImage(id);

        if(createVmts_ && createVmts_->isChecked()) {
            QString fileErr;
            const QString vmtText = basicVmtText(vmtShader_ ? vmtShader_->text() : QString(), baseName);
            if(!writeTextFileUtf8(outVmt, vmtText, &fileErr)) {
                appendLog(QString("%1: Wrote VTF but failed to write VMT: %2").arg(baseName, fileErr));
            }
        }

        appendLog(QString("%1: OK").arg(baseName));
        ++okCount;
    }

    progressBar_->setValue(textures.size());
    progressLabel_->setText(QString("Done. %1 OK, %2 failed.").arg(okCount).arg(failCount));
    setBusy(false);
}
