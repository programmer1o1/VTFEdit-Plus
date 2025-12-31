#include "BatchConvertDialog.h"

#include "CreateVtfDialog.h"
#include "QtUtil.h"
#include "WindowsVtfFormats.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStandardItemModel>

#include <algorithm>

namespace {

static void populateFormatCombo(QComboBox *combo, VTFImageFormat preferred) {
    if(!combo) return;
    combo->clear();
    for(const auto &item : kWindowsVtfFormats) {
        combo->addItem(QString::fromUtf8(item.label), static_cast<int>(item.format));
    }

    // Disable unavailable entries.
    if(auto *model = qobject_cast<QStandardItemModel *>(combo->model())) {
        for(int i = 0; i < combo->count(); ++i) {
            const auto fmt = static_cast<VTFImageFormat>(combo->itemData(i).toInt());
            if(!vtflibCanEncode(fmt)) {
                if(auto *it = model->item(i)) {
                    it->setEnabled(false);
                    if(fmt == IMAGE_FORMAT_DXT1 || fmt == IMAGE_FORMAT_DXT3 || fmt == IMAGE_FORMAT_DXT5 ||
                       fmt == IMAGE_FORMAT_DXT1_ONEBITALPHA) {
                        it->setToolTip("DXT encoding requires a compressor (e.g. Compressonator).");
                    } else {
                        it->setToolTip("This image format is not supported for creation by this build of VTFLib.");
                    }
                }
            }
        }
    }

    const int idx = combo->findData(static_cast<int>(preferred));
    combo->setCurrentIndex(idx >= 0 ? idx : 0);
}

QVariantMap createOptionsToMap(const SVTFCreateOptions &o) {
    QVariantMap m;
    m["versionMajor"] = static_cast<quint32>(o.uiVersion[0]);
    m["versionMinor"] = static_cast<quint32>(o.uiVersion[1]);
    m["imageFormat"] = static_cast<int>(o.ImageFormat);
    m["flags"] = static_cast<qulonglong>(o.uiFlags);
    m["mipmaps"] = static_cast<bool>(o.bMipmaps != 0);
    m["mipmapFilter"] = static_cast<int>(o.MipmapFilter);
    m["thumbnail"] = static_cast<bool>(o.bThumbnail != 0);
    m["reflectivity"] = static_cast<bool>(o.bReflectivity != 0);
    m["resize"] = static_cast<bool>(o.bResize != 0);
    m["resizeMethod"] = static_cast<int>(o.ResizeMethod);
    m["resizeFilter"] = static_cast<int>(o.ResizeFilter);
    m["resizeWidth"] = static_cast<quint32>(o.uiResizeWidth);
    m["resizeHeight"] = static_cast<quint32>(o.uiResizeHeight);
    m["resizeClamp"] = static_cast<bool>(o.bResizeClamp != 0);
    m["resizeClampWidth"] = static_cast<quint32>(o.uiResizeClampWidth);
    m["resizeClampHeight"] = static_cast<quint32>(o.uiResizeClampHeight);
    m["gamma"] = static_cast<bool>(o.bGammaCorrection != 0);
    m["gammaValue"] = static_cast<double>(o.sGammaCorrection);
    m["sphereMap"] = static_cast<bool>(o.bSphereMap != 0);
    m["srgb"] = static_cast<bool>(o.bSRGB != 0);
    m["alphaThreshold"] = static_cast<quint32>(o.nAlphaThreshold);
    return m;
}

SVTFCreateOptions mapToCreateOptions(const QVariantMap &m) {
    SVTFCreateOptions o{};
    vlImageCreateDefaultCreateStructure(&o);
    o.uiVersion[0] = m.value("versionMajor", static_cast<quint32>(VTF_MAJOR_VERSION)).toUInt();
    o.uiVersion[1] = m.value("versionMinor", static_cast<quint32>(5)).toUInt();
    o.ImageFormat = static_cast<VTFImageFormat>(m.value("imageFormat", static_cast<int>(IMAGE_FORMAT_DXT5)).toInt());
    o.uiFlags = static_cast<vlUInt>(m.value("flags", static_cast<qulonglong>(0)).toULongLong());
    o.bMipmaps = m.value("mipmaps", true).toBool() ? vlTrue : vlFalse;
    o.MipmapFilter = static_cast<VTFMipmapFilter>(m.value("mipmapFilter", static_cast<int>(MIPMAP_FILTER_DEFAULT)).toInt());
    o.bThumbnail = m.value("thumbnail", true).toBool() ? vlTrue : vlFalse;
    o.bReflectivity = m.value("reflectivity", true).toBool() ? vlTrue : vlFalse;
    o.bResize = m.value("resize", false).toBool() ? vlTrue : vlFalse;
    o.ResizeMethod = static_cast<VTFResizeMethod>(m.value("resizeMethod", static_cast<int>(RESIZE_NEAREST_POWER2)).toInt());
    o.ResizeFilter = static_cast<VTFMipmapFilter>(m.value("resizeFilter", static_cast<int>(MIPMAP_FILTER_DEFAULT)).toInt());
    o.uiResizeWidth = m.value("resizeWidth", static_cast<quint32>(0)).toUInt();
    o.uiResizeHeight = m.value("resizeHeight", static_cast<quint32>(0)).toUInt();
    o.bResizeClamp = m.value("resizeClamp", false).toBool() ? vlTrue : vlFalse;
    o.uiResizeClampWidth = m.value("resizeClampWidth", static_cast<quint32>(0)).toUInt();
    o.uiResizeClampHeight = m.value("resizeClampHeight", static_cast<quint32>(0)).toUInt();
    o.bGammaCorrection = m.value("gamma", false).toBool() ? vlTrue : vlFalse;
    o.sGammaCorrection = static_cast<vlSingle>(m.value("gammaValue", 2.2).toDouble());
    o.bSphereMap = m.value("sphereMap", false).toBool() ? vlTrue : vlFalse;
    o.bSRGB = m.value("srgb", false).toBool() ? vlTrue : vlFalse;
    o.nAlphaThreshold = m.value("alphaThreshold", static_cast<quint32>(0)).toUInt();
    return o;
}

} // namespace

BatchConvertDialog::BatchConvertDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Batch Convert Folder");
    setModal(true);

    vlImageCreateDefaultCreateStructure(&createOptions_);
    createOptions_.uiVersion[0] = VTF_MAJOR_VERSION;
    createOptions_.uiVersion[1] = 5;
    // Prefer DXT5 when available; otherwise fall back to RGBA8888 so batch convert works out of the box.
    createOptions_.ImageFormat = vtflibCanEncode(IMAGE_FORMAT_DXT5) ? IMAGE_FORMAT_DXT5 : IMAGE_FORMAT_RGBA8888;
    createOptions_.bThumbnail = vtflibCanEncode(IMAGE_FORMAT_DXT1) ? vlTrue : vlFalse;
    {
        QSettings s;
        createOptions_.uiVersion[1] = static_cast<vlUInt>(std::clamp(s.value("options/create/versionMinor", 5).toInt(), 0, 5));
        createOptions_.ImageFormat = static_cast<VTFImageFormat>(s.value("options/create/normalFormat", static_cast<int>(createOptions_.ImageFormat)).toInt());
        createOptions_.bMipmaps = s.value("options/create/mipmaps", true).toBool() ? vlTrue : vlFalse;
        createOptions_.MipmapFilter = static_cast<VTFMipmapFilter>(s.value("options/create/mipmapFilter", static_cast<int>(MIPMAP_FILTER_DEFAULT)).toInt());
        createOptions_.bThumbnail = s.value("options/create/thumbnail", createOptions_.bThumbnail != 0).toBool() ? vlTrue : vlFalse;
        createOptions_.bReflectivity = s.value("options/create/reflectivity", true).toBool() ? vlTrue : vlFalse;
        createOptions_.bGammaCorrection = s.value("options/create/gammaEnabled", false).toBool() ? vlTrue : vlFalse;
        createOptions_.sGammaCorrection = static_cast<vlSingle>(s.value("options/create/gammaValue", 2.2).toDouble());
        createOptions_.bSphereMap = s.value("options/create/sphereMap", false).toBool() ? vlTrue : vlFalse;
        createOptions_.bSRGB = s.value("options/create/srgb", false).toBool() ? vlTrue : vlFalse;
        createOptions_.nAlphaThreshold = static_cast<vlUInt>(std::clamp(s.value("options/create/alphaThreshold", 0).toInt(), 0, 255));

        const bool resizeEnabled = s.value("options/create/resizeEnabled", false).toBool();
        if(resizeEnabled) {
            createOptions_.bResize = vlTrue;
            createOptions_.ResizeMethod = static_cast<VTFResizeMethod>(s.value("options/create/resizeMethod", static_cast<int>(RESIZE_NEAREST_POWER2)).toInt());
            createOptions_.ResizeFilter = static_cast<VTFMipmapFilter>(s.value("options/create/resizeFilter", static_cast<int>(createOptions_.MipmapFilter)).toInt());
        } else {
            createOptions_.bResize = vlFalse;
        }

        createOptions_.bResizeClamp = (resizeEnabled && s.value("options/create/resizeClamp", false).toBool()) ? vlTrue : vlFalse;
        createOptions_.uiResizeClampWidth = static_cast<vlUInt>(std::max(1, s.value("options/create/resizeClampWidth", 1024).toInt()));
        createOptions_.uiResizeClampHeight = static_cast<vlUInt>(std::max(1, s.value("options/create/resizeClampHeight", 1024).toInt()));
    }

    auto *layout = new QFormLayout(this);

    auto *presetRow = new QWidget(this);
    auto *presetLayout = new QHBoxLayout(presetRow);
    presetLayout->setContentsMargins(0, 0, 0, 0);
    presetCombo_ = new QComboBox(this);
    savePresetButton_ = new QPushButton("Save As…", this);
    deletePresetButton_ = new QPushButton("Delete", this);
    connect(savePresetButton_, &QPushButton::clicked, this, &BatchConvertDialog::savePresetAs);
    connect(deletePresetButton_, &QPushButton::clicked, this, &BatchConvertDialog::deletePreset);
    connect(presetCombo_, &QComboBox::currentIndexChanged, this, &BatchConvertDialog::presetChanged);
    presetLayout->addWidget(presetCombo_, 1);
    presetLayout->addWidget(savePresetButton_);
    presetLayout->addWidget(deletePresetButton_);
    layout->addRow("Preset:", presetRow);

    auto *inputRow = new QWidget(this);
    auto *inputLayout = new QHBoxLayout(inputRow);
    inputLayout->setContentsMargins(0, 0, 0, 0);
    inputDir_ = new QLineEdit(this);
    auto *browseIn = new QPushButton("Browse…", this);
    connect(browseIn, &QPushButton::clicked, this, &BatchConvertDialog::browseInput);
    inputLayout->addWidget(inputDir_);
    inputLayout->addWidget(browseIn);
    layout->addRow("Input folder:", inputRow);

    auto *outputRow = new QWidget(this);
    auto *outputLayout = new QHBoxLayout(outputRow);
    outputLayout->setContentsMargins(0, 0, 0, 0);
    outputDir_ = new QLineEdit(this);
    auto *browseOut = new QPushButton("Browse…", this);
    connect(browseOut, &QPushButton::clicked, this, &BatchConvertDialog::browseOutput);
    outputLayout->addWidget(outputDir_);
    outputLayout->addWidget(browseOut);
    layout->addRow("Output folder:", outputRow);

    outputUnderMaterials_ = new QCheckBox("Write under output/materials/", this);
    outputUnderMaterials_->setChecked(true);
    layout->addRow("", outputUnderMaterials_);

    auto *materialsRow = new QWidget(this);
    auto *materialsLayout = new QHBoxLayout(materialsRow);
    materialsLayout->setContentsMargins(0, 0, 0, 0);
    materialsRoot_ = new QLineEdit(this);
    materialsRoot_->setPlaceholderText("Optional: path to your materials/ folder (for $basetexture)");
    auto *browseMaterials = new QPushButton("Browse…", this);
    connect(browseMaterials, &QPushButton::clicked, this, &BatchConvertDialog::browseMaterialsRoot);
    materialsLayout->addWidget(materialsRoot_);
    materialsLayout->addWidget(browseMaterials);
    layout->addRow("Materials root:", materialsRow);

    recursive_ = new QCheckBox("Recurse into subfolders", this);
    recursive_->setChecked(true);
    layout->addRow("", recursive_);

    overwrite_ = new QCheckBox("Overwrite existing files", this);
    overwrite_->setChecked(false);
    layout->addRow("", overwrite_);

    createOptionsButton_ = new QPushButton("Create options…", this);
    connect(createOptionsButton_, &QPushButton::clicked, this, &BatchConvertDialog::chooseCreateOptions);
    layout->addRow("VTF:", createOptionsButton_);

    useAlphaFormat_ = new QCheckBox("Use alternate format when the input has transparency", this);
    useAlphaFormat_->setChecked(false);
    connect(useAlphaFormat_, &QCheckBox::toggled, this, &BatchConvertDialog::updateUi);
    layout->addRow("", useAlphaFormat_);

    alphaFormat_ = new QComboBox(this);
    populateFormatCombo(alphaFormat_, IMAGE_FORMAT_DXT5);
    connect(alphaFormat_, &QComboBox::currentIndexChanged, this, &BatchConvertDialog::updateUi);
    layout->addRow("Alpha format:", alphaFormat_);

    createLodResource_ = new QCheckBox("Create LOD clamp resource (VTF v7.3+)", this);
    createLodResource_->setChecked(false);
    connect(createLodResource_, &QCheckBox::toggled, this, &BatchConvertDialog::updateUi);
    layout->addRow("", createLodResource_);

    auto *lodRow = new QWidget(this);
    auto *lodLayout = new QHBoxLayout(lodRow);
    lodLayout->setContentsMargins(0, 0, 0, 0);
    lodU_ = new QSpinBox(this);
    lodV_ = new QSpinBox(this);
    lodU_->setRange(0, 255);
    lodV_->setRange(0, 255);
    lodU_->setValue(0);
    lodV_->setValue(0);
    lodLayout->addWidget(new QLabel("U:", this));
    lodLayout->addWidget(lodU_);
    lodLayout->addSpacing(12);
    lodLayout->addWidget(new QLabel("V:", this));
    lodLayout->addWidget(lodV_);
    lodLayout->addStretch(1);
    layout->addRow("LOD clamp:", lodRow);

    createInformationResource_ = new QCheckBox("Create Information resource (writes KVD)", this);
    createInformationResource_->setChecked(false);
    connect(createInformationResource_, &QCheckBox::toggled, this, &BatchConvertDialog::updateUi);
    layout->addRow("", createInformationResource_);

    infoAuthor_ = new QLineEdit(this);
    infoContact_ = new QLineEdit(this);
    infoVersion_ = new QLineEdit(this);
    infoModification_ = new QLineEdit(this);
    infoDescription_ = new QPlainTextEdit(this);
    infoComments_ = new QPlainTextEdit(this);
    infoDescription_->setMaximumHeight(70);
    infoComments_->setMaximumHeight(70);
    layout->addRow("Author:", infoAuthor_);
    layout->addRow("Contact:", infoContact_);
    layout->addRow("Version:", infoVersion_);
    layout->addRow("Modification:", infoModification_);
    layout->addRow("Description:", infoDescription_);
    layout->addRow("Comments:", infoComments_);

    createVmts_ = new QCheckBox("Create .vmt next to .vtf", this);
    createVmts_->setChecked(false);
    connect(createVmts_, &QCheckBox::toggled, this, &BatchConvertDialog::updateUi);
    layout->addRow("", createVmts_);

    vmtShader_ = new QLineEdit(this);
    vmtShader_->setText("VertexLitGeneric");
    layout->addRow("VMT shader:", vmtShader_);

    vmtBaseTexturePrefix_ = new QLineEdit(this);
    vmtBaseTexturePrefix_->setPlaceholderText("e.g. materials/custom");
    layout->addRow("BaseTexture prefix:", vmtBaseTexturePrefix_);

    buttons_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons_, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addRow(buttons_);

    presetCombo_->blockSignals(true);
    presetCombo_->addItem("Last used", "__last");
    {
        QSettings s;
        s.beginGroup("batchPresets");
        const auto names = s.childGroups();
        for(const auto &name : names) {
            presetCombo_->addItem(name, name);
        }
        s.endGroup();
    }
    presetCombo_->setCurrentIndex(0);
    presetCombo_->blockSignals(false);

    loadLast();
    updateUi();
}

BatchConvertDialog::~BatchConvertDialog() = default;

QString BatchConvertDialog::inputDir() const {
    return inputDir_->text().trimmed();
}

QString BatchConvertDialog::outputDir() const {
    return outputDir_->text().trimmed();
}

bool BatchConvertDialog::outputUnderMaterials() const {
    return outputUnderMaterials_->isChecked();
}

QString BatchConvertDialog::materialsRoot() const {
    return materialsRoot_->text().trimmed();
}

bool BatchConvertDialog::recursive() const {
    return recursive_->isChecked();
}

bool BatchConvertDialog::overwrite() const {
    return overwrite_->isChecked();
}

bool BatchConvertDialog::createVmts() const {
    return createVmts_->isChecked();
}

QString BatchConvertDialog::vmtShader() const {
    return vmtShader_->text().trimmed();
}

QString BatchConvertDialog::vmtBaseTexturePrefix() const {
    return vmtBaseTexturePrefix_->text().trimmed();
}

SVTFCreateOptions BatchConvertDialog::createOptions() const {
    return createOptions_;
}

void BatchConvertDialog::accept() {
    saveLast();
    QDialog::accept();
}

void BatchConvertDialog::browseInput() {
    const QString dir = QFileDialog::getExistingDirectory(this, "Select input folder");
    if(dir.isEmpty()) return;
    inputDir_->setText(dir);
}

void BatchConvertDialog::browseOutput() {
    const QString dir = QFileDialog::getExistingDirectory(this, "Select output folder");
    if(dir.isEmpty()) return;
    outputDir_->setText(dir);
}

void BatchConvertDialog::browseMaterialsRoot() {
    const QString dir = QFileDialog::getExistingDirectory(this, "Select materials folder");
    if(dir.isEmpty()) return;
    materialsRoot_->setText(dir);
}

void BatchConvertDialog::chooseCreateOptions() {
    CreateVtfDialog dlg(this);
    if(dlg.exec() != QDialog::Accepted) return;
    createOptions_ = dlg.options();
}

void BatchConvertDialog::savePresetAs() {
    const QString name = QInputDialog::getText(this, "Save preset", "Preset name:");
    const QString trimmed = name.trimmed();
    if(trimmed.isEmpty()) return;

    QSettings s;
    s.beginGroup("batchPresets");
    if(s.childGroups().contains(trimmed)) {
        const auto r = QMessageBox::question(this, "Overwrite preset", "Preset already exists. Overwrite?",
                                            QMessageBox::Yes | QMessageBox::No);
        if(r != QMessageBox::Yes) {
            s.endGroup();
            return;
        }
        s.remove(trimmed);
    }
    storePreset(trimmed);
    s.endGroup();

    const int existingIndex = presetCombo_->findData(trimmed);
    if(existingIndex == -1) {
        presetCombo_->addItem(trimmed, trimmed);
        presetCombo_->setCurrentIndex(presetCombo_->count() - 1);
    } else {
        presetCombo_->setCurrentIndex(existingIndex);
    }
}

void BatchConvertDialog::deletePreset() {
    const QString key = presetCombo_->currentData().toString();
    if(key.isEmpty() || key == "__last") return;
    const auto r = QMessageBox::question(this, "Delete preset", QString("Delete preset '%1'?").arg(key),
                                        QMessageBox::Yes | QMessageBox::No);
    if(r != QMessageBox::Yes) return;

    QSettings s;
    s.beginGroup("batchPresets");
    s.remove(key);
    s.endGroup();

    presetCombo_->removeItem(presetCombo_->currentIndex());
    presetCombo_->setCurrentIndex(0);
}

void BatchConvertDialog::presetChanged(int) {
    const QString key = presetCombo_->currentData().toString();
    if(key == "__last") {
        loadLast();
        return;
    }
    loadPreset(key);
}

void BatchConvertDialog::updateUi() {
    const bool enabled = createVmts_->isChecked();
    vmtShader_->setEnabled(enabled);
    vmtBaseTexturePrefix_->setEnabled(enabled);
    materialsRoot_->setEnabled(enabled);

    alphaFormat_->setEnabled(useAlphaFormat_->isChecked());

    const bool lod = createLodResource_->isChecked();
    lodU_->setEnabled(lod);
    lodV_->setEnabled(lod);

    const bool info = createInformationResource_->isChecked();
    for(auto *w : {infoAuthor_, infoContact_, infoVersion_, infoModification_}) w->setEnabled(info);
    infoDescription_->setEnabled(info);
    infoComments_->setEnabled(info);
}

void BatchConvertDialog::loadLast() {
    QSettings s;
    s.beginGroup("batchLast");
    inputDir_->setText(s.value("inputDir").toString());
    outputDir_->setText(s.value("outputDir").toString());
    outputUnderMaterials_->setChecked(s.value("outputUnderMaterials", true).toBool());
    materialsRoot_->setText(s.value("materialsRoot").toString());
    recursive_->setChecked(s.value("recursive", true).toBool());
    overwrite_->setChecked(s.value("overwrite", false).toBool());
    createVmts_->setChecked(s.value("createVmts", false).toBool());
    {
        QSettings g;
        const bool useAlpha = s.contains("useAlphaFormat") ? s.value("useAlphaFormat").toBool()
                                                           : g.value("options/create/useAlphaFormat", false).toBool();
        useAlphaFormat_->setChecked(useAlpha);
        const int alphaFmt = s.contains("alphaFormat") ? s.value("alphaFormat").toInt()
                                                       : g.value("options/create/alphaFormat", static_cast<int>(IMAGE_FORMAT_DXT5)).toInt();
        populateFormatCombo(alphaFormat_, static_cast<VTFImageFormat>(alphaFmt));

        const bool lod = s.contains("createLodResource") ? s.value("createLodResource").toBool()
                                                         : g.value("options/resources/createLod", false).toBool();
        createLodResource_->setChecked(lod);
        lodU_->setValue(s.contains("lodClampU") ? s.value("lodClampU").toInt() : g.value("options/resources/lodClampU", 0).toInt());
        lodV_->setValue(s.contains("lodClampV") ? s.value("lodClampV").toInt() : g.value("options/resources/lodClampV", 0).toInt());

        const bool info = s.contains("createInformationResource") ? s.value("createInformationResource").toBool()
                                                                  : g.value("options/resources/createInformation", false).toBool();
        createInformationResource_->setChecked(info);
        infoAuthor_->setText(s.contains("infoAuthor") ? s.value("infoAuthor").toString() : g.value("options/resources/infoAuthor").toString());
        infoContact_->setText(s.contains("infoContact") ? s.value("infoContact").toString() : g.value("options/resources/infoContact").toString());
        infoVersion_->setText(s.contains("infoVersion") ? s.value("infoVersion").toString() : g.value("options/resources/infoVersion").toString());
        infoModification_->setText(s.contains("infoModification") ? s.value("infoModification").toString()
                                                                  : g.value("options/resources/infoModification").toString());
        infoDescription_->setPlainText(s.contains("infoDescription") ? s.value("infoDescription").toString()
                                                                     : g.value("options/resources/infoDescription").toString());
        infoComments_->setPlainText(s.contains("infoComments") ? s.value("infoComments").toString()
                                                               : g.value("options/resources/infoComments").toString());
    }
    vmtShader_->setText(s.value("vmtShader", "VertexLitGeneric").toString());
    vmtBaseTexturePrefix_->setText(s.value("vmtPrefix").toString());
    createOptions_ = mapToCreateOptions(s.value("createOptions").toMap());
    s.endGroup();
    updateUi();
}

void BatchConvertDialog::saveLast() const {
    QSettings s;
    s.beginGroup("batchLast");
    s.setValue("inputDir", inputDir());
    s.setValue("outputDir", outputDir());
    s.setValue("outputUnderMaterials", outputUnderMaterials());
    s.setValue("materialsRoot", materialsRoot());
    s.setValue("recursive", recursive());
    s.setValue("overwrite", overwrite());
    s.setValue("createVmts", createVmts());
    s.setValue("useAlphaFormat", useAlphaFormat());
    s.setValue("alphaFormat", static_cast<int>(alphaFormat()));
    s.setValue("createLodResource", createLodResource());
    s.setValue("lodClampU", lodClampU());
    s.setValue("lodClampV", lodClampV());
    s.setValue("createInformationResource", createInformationResource());
    s.setValue("infoAuthor", informationAuthor());
    s.setValue("infoContact", informationContact());
    s.setValue("infoVersion", informationVersion());
    s.setValue("infoModification", informationModification());
    s.setValue("infoDescription", informationDescription());
    s.setValue("infoComments", informationComments());
    s.setValue("vmtShader", vmtShader());
    s.setValue("vmtPrefix", vmtBaseTexturePrefix());
    s.setValue("createOptions", createOptionsToMap(createOptions_));
    s.endGroup();
}

void BatchConvertDialog::loadPreset(const QString &name) {
    if(name.isEmpty()) return;
    QSettings s;
    s.beginGroup("batchPresets");
    if(!s.childGroups().contains(name)) {
        s.endGroup();
        return;
    }
    s.beginGroup(name);
    inputDir_->setText(s.value("inputDir").toString());
    outputDir_->setText(s.value("outputDir").toString());
    outputUnderMaterials_->setChecked(s.value("outputUnderMaterials", true).toBool());
    materialsRoot_->setText(s.value("materialsRoot").toString());
    recursive_->setChecked(s.value("recursive", true).toBool());
    overwrite_->setChecked(s.value("overwrite", false).toBool());
    createVmts_->setChecked(s.value("createVmts", false).toBool());
    useAlphaFormat_->setChecked(s.value("useAlphaFormat", false).toBool());
    populateFormatCombo(alphaFormat_, static_cast<VTFImageFormat>(s.value("alphaFormat", static_cast<int>(IMAGE_FORMAT_DXT5)).toInt()));
    createLodResource_->setChecked(s.value("createLodResource", false).toBool());
    lodU_->setValue(s.value("lodClampU", 0).toInt());
    lodV_->setValue(s.value("lodClampV", 0).toInt());
    createInformationResource_->setChecked(s.value("createInformationResource", false).toBool());
    infoAuthor_->setText(s.value("infoAuthor").toString());
    infoContact_->setText(s.value("infoContact").toString());
    infoVersion_->setText(s.value("infoVersion").toString());
    infoModification_->setText(s.value("infoModification").toString());
    infoDescription_->setPlainText(s.value("infoDescription").toString());
    infoComments_->setPlainText(s.value("infoComments").toString());
    vmtShader_->setText(s.value("vmtShader", "VertexLitGeneric").toString());
    vmtBaseTexturePrefix_->setText(s.value("vmtPrefix").toString());
    createOptions_ = mapToCreateOptions(s.value("createOptions").toMap());
    s.endGroup();
    s.endGroup();
    updateUi();
}

void BatchConvertDialog::storePreset(const QString &name) const {
    if(name.isEmpty()) return;
    QSettings s;
    s.beginGroup("batchPresets");
    s.beginGroup(name);
    s.setValue("inputDir", inputDir());
    s.setValue("outputDir", outputDir());
    s.setValue("outputUnderMaterials", outputUnderMaterials());
    s.setValue("materialsRoot", materialsRoot());
    s.setValue("recursive", recursive());
    s.setValue("overwrite", overwrite());
    s.setValue("createVmts", createVmts());
    s.setValue("useAlphaFormat", useAlphaFormat());
    s.setValue("alphaFormat", static_cast<int>(alphaFormat()));
    s.setValue("createLodResource", createLodResource());
    s.setValue("lodClampU", lodClampU());
    s.setValue("lodClampV", lodClampV());
    s.setValue("createInformationResource", createInformationResource());
    s.setValue("infoAuthor", informationAuthor());
    s.setValue("infoContact", informationContact());
    s.setValue("infoVersion", informationVersion());
    s.setValue("infoModification", informationModification());
    s.setValue("infoDescription", informationDescription());
    s.setValue("infoComments", informationComments());
    s.setValue("vmtShader", vmtShader());
    s.setValue("vmtPrefix", vmtBaseTexturePrefix());
    s.setValue("createOptions", createOptionsToMap(createOptions_));
    s.endGroup();
    s.endGroup();
}

bool BatchConvertDialog::useAlphaFormat() const {
    return useAlphaFormat_ && useAlphaFormat_->isChecked();
}

VTFImageFormat BatchConvertDialog::alphaFormat() const {
    if(!alphaFormat_) return IMAGE_FORMAT_DXT5;
    return static_cast<VTFImageFormat>(alphaFormat_->currentData().toInt());
}

bool BatchConvertDialog::createLodResource() const {
    return createLodResource_ && createLodResource_->isChecked();
}

int BatchConvertDialog::lodClampU() const {
    return lodU_ ? lodU_->value() : 0;
}

int BatchConvertDialog::lodClampV() const {
    return lodV_ ? lodV_->value() : 0;
}

bool BatchConvertDialog::createInformationResource() const {
    return createInformationResource_ && createInformationResource_->isChecked();
}

QString BatchConvertDialog::informationAuthor() const {
    return infoAuthor_ ? infoAuthor_->text().trimmed() : QString{};
}

QString BatchConvertDialog::informationContact() const {
    return infoContact_ ? infoContact_->text().trimmed() : QString{};
}

QString BatchConvertDialog::informationVersion() const {
    return infoVersion_ ? infoVersion_->text().trimmed() : QString{};
}

QString BatchConvertDialog::informationModification() const {
    return infoModification_ ? infoModification_->text().trimmed() : QString{};
}

QString BatchConvertDialog::informationDescription() const {
    return infoDescription_ ? infoDescription_->toPlainText().trimmed() : QString{};
}

QString BatchConvertDialog::informationComments() const {
    return infoComments_ ? infoComments_->toPlainText().trimmed() : QString{};
}
