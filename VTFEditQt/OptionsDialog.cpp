#include "OptionsDialog.h"

#include "QtUtil.h"
#include "WindowsVtfFormats.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStandardItemModel>
#include <QTabWidget>
#include <QVBoxLayout>

#include <algorithm>

namespace {

static void addMipmapFilters(QComboBox *combo) {
    combo->addItem("Default", static_cast<int>(MIPMAP_FILTER_DEFAULT));
    combo->addItem("Box", static_cast<int>(MIPMAP_FILTER_BOX));
    combo->addItem("Triangle", static_cast<int>(MIPMAP_FILTER_TRIANGLE));
    combo->addItem("Cubic", static_cast<int>(MIPMAP_FILTER_CUBIC));
    combo->addItem("Catrom", static_cast<int>(MIPMAP_FILTER_CATROM));
    combo->addItem("Mitchell", static_cast<int>(MIPMAP_FILTER_MITCHELL));
    combo->addItem("Point", static_cast<int>(MIPMAP_FILTER_POINT));
    combo->addItem("Gaussian", static_cast<int>(MIPMAP_FILTER_GAUSSIAN));
    combo->addItem("Quadratic", static_cast<int>(MIPMAP_FILTER_QUADRATIC));
    combo->addItem("Sinc", static_cast<int>(MIPMAP_FILTER_SINC));
    combo->addItem("Bessel", static_cast<int>(MIPMAP_FILTER_BESSEL));
    combo->addItem("Hanning", static_cast<int>(MIPMAP_FILTER_HANNING));
    combo->addItem("Hamming", static_cast<int>(MIPMAP_FILTER_HAMMING));
    combo->addItem("Blackman", static_cast<int>(MIPMAP_FILTER_BLACKMAN));
    combo->addItem("Kaiser", static_cast<int>(MIPMAP_FILTER_KAISER));
}

static void addResizeMethods(QComboBox *combo) {
    combo->addItem("Nearest power of two", static_cast<int>(RESIZE_NEAREST_POWER2));
    combo->addItem("Biggest power of two", static_cast<int>(RESIZE_BIGGEST_POWER2));
    combo->addItem("Smallest power of two", static_cast<int>(RESIZE_SMALLEST_POWER2));
    combo->addItem("Nearest multiple of 4", static_cast<int>(RESIZE_NEAREST_MULTIPLE4));
}

static void populateFormatCombo(QComboBox *combo, VTFImageFormat preferred) {
    combo->clear();
    for(const auto &item : kWindowsVtfFormats) {
        combo->addItem(QString::fromUtf8(item.label), static_cast<int>(item.format));
    }

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

static int comboIndexForData(QComboBox *combo, int value, int fallbackIndex = 0) {
    if(!combo) return fallbackIndex;
    const int idx = combo->findData(value);
    return idx >= 0 ? idx : fallbackIndex;
}

} // namespace

OptionsDialog::OptionsDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Options");
    setModal(true);
    resize(760, 640);

    auto *root = new QVBoxLayout(this);

    tabs_ = new QTabWidget(this);
    root->addWidget(tabs_, 1);

    // General tab
    {
        auto *tab = new QWidget(tabs_);
        auto *layout = new QFormLayout(tab);

        versionMinor_ = new QSpinBox(tab);
        versionMinor_->setRange(0, 5);
        versionMinor_->setValue(5);
        layout->addRow("VTF minor version:", versionMinor_);

        textureType_ = new QComboBox(tab);
        textureType_->addItem("Animated Texture", 0);
        textureType_->addItem("Environment Map", 1);
        textureType_->addItem("Volume Texture", 2);
        textureType_->setCurrentIndex(0);
        layout->addRow("Texture type:", textureType_);

        normalFormat_ = new QComboBox(tab);
        populateFormatCombo(normalFormat_, vtflibCanEncode(IMAGE_FORMAT_DXT5) ? IMAGE_FORMAT_DXT5 : IMAGE_FORMAT_RGBA8888);
        layout->addRow("Format:", normalFormat_);

        useAlphaFormat_ = new QCheckBox("Use alpha format when input has transparency", tab);
        useAlphaFormat_->setChecked(false);
        connect(useAlphaFormat_, &QCheckBox::toggled, this, &OptionsDialog::updateEnabledStates);
        layout->addRow("", useAlphaFormat_);

        alphaFormat_ = new QComboBox(tab);
        populateFormatCombo(alphaFormat_, IMAGE_FORMAT_DXT5);
        layout->addRow("Alpha format:", alphaFormat_);

        mipmaps_ = new QCheckBox("Generate mipmaps", tab);
        mipmaps_->setChecked(true);
        connect(mipmaps_, &QCheckBox::toggled, this, &OptionsDialog::updateEnabledStates);
        layout->addRow("", mipmaps_);

        mipmapFilter_ = new QComboBox(tab);
        addMipmapFilters(mipmapFilter_);
        layout->addRow("Mipmap filter:", mipmapFilter_);

        thumbnail_ = new QCheckBox("Generate thumbnail (requires DXT1)", tab);
        thumbnail_->setChecked(vtflibCanEncode(IMAGE_FORMAT_DXT1));
        if(!vtflibCanEncode(IMAGE_FORMAT_DXT1)) {
            thumbnail_->setEnabled(false);
            thumbnail_->setToolTip("Thumbnail generation requires DXT1 encoding (Compressonator).");
        }
        layout->addRow("", thumbnail_);

        reflectivity_ = new QCheckBox("Compute reflectivity", tab);
        reflectivity_->setChecked(true);
        layout->addRow("", reflectivity_);

        sphereMap_ = new QCheckBox("Generate sphere map (cubemap)", tab);
        sphereMap_->setChecked(false);
        layout->addRow("", sphereMap_);

        srgb_ = new QCheckBox("sRGB (treat input as sRGB)", tab);
        srgb_->setChecked(false);
        layout->addRow("", srgb_);

        tabs_->addTab(tab, "General");
    }

    // Advanced tab
    {
        auto *tab = new QWidget(tabs_);
        auto *layout = new QFormLayout(tab);

        gammaCorrection_ = new QCheckBox("Gamma correction", tab);
        gammaCorrection_->setChecked(false);
        connect(gammaCorrection_, &QCheckBox::toggled, this, &OptionsDialog::updateEnabledStates);
        layout->addRow("", gammaCorrection_);

        gammaValue_ = new QDoubleSpinBox(tab);
        gammaValue_->setRange(0.1, 5.0);
        gammaValue_->setSingleStep(0.05);
        gammaValue_->setDecimals(2);
        gammaValue_->setValue(2.2);
        layout->addRow("Gamma:", gammaValue_);

        resizeEnabled_ = new QCheckBox("Resize input images", tab);
        resizeEnabled_->setChecked(false);
        connect(resizeEnabled_, &QCheckBox::toggled, this, &OptionsDialog::updateEnabledStates);
        layout->addRow("", resizeEnabled_);

        resizeMethod_ = new QComboBox(tab);
        addResizeMethods(resizeMethod_);
        layout->addRow("Resize method:", resizeMethod_);

        resizeFilter_ = new QComboBox(tab);
        addMipmapFilters(resizeFilter_);
        layout->addRow("Resize filter:", resizeFilter_);

        resizeClamp_ = new QCheckBox("Clamp resize maximum dimensions", tab);
        resizeClamp_->setChecked(false);
        connect(resizeClamp_, &QCheckBox::toggled, this, &OptionsDialog::updateEnabledStates);
        layout->addRow("", resizeClamp_);

        resizeClampW_ = new QSpinBox(tab);
        resizeClampW_->setRange(1, 16384);
        resizeClampW_->setValue(1024);
        layout->addRow("Max width:", resizeClampW_);

        resizeClampH_ = new QSpinBox(tab);
        resizeClampH_->setRange(1, 16384);
        resizeClampH_->setValue(1024);
        layout->addRow("Max height:", resizeClampH_);

        alphaThreshold_ = new QSpinBox(tab);
        alphaThreshold_->setRange(0, 255);
        alphaThreshold_->setValue(0);
        alphaThreshold_->setToolTip("Any pixel alpha below this value will be set to 0 during creation.");
        layout->addRow("Alpha threshold:", alphaThreshold_);

        auto *lumHint = new QLabel("Luminance weights affect mipmap generation and some image processing in VTFLib.", tab);
        lumHint->setWordWrap(true);
        lumHint->setStyleSheet("color: palette(mid);");
        layout->addRow("", lumHint);

        lumR_ = new QDoubleSpinBox(tab);
        lumG_ = new QDoubleSpinBox(tab);
        lumB_ = new QDoubleSpinBox(tab);
        for(auto *s : {lumR_, lumG_, lumB_}) {
            s->setRange(0.0, 1.0);
            s->setDecimals(3);
            s->setSingleStep(0.001);
        }
        lumR_->setValue(0.299);
        lumG_->setValue(0.587);
        lumB_->setValue(0.114);
        layout->addRow("Luminance R:", lumR_);
        layout->addRow("Luminance G:", lumG_);
        layout->addRow("Luminance B:", lumB_);

        tabs_->addTab(tab, "Advanced");
    }

    // Resources tab
    {
        auto *tab = new QWidget(tabs_);
        auto *layout = new QFormLayout(tab);

        auto *hint = new QLabel("Resources are supported by VTF v7.3+ (minor >= 3).", tab);
        hint->setWordWrap(true);
        hint->setStyleSheet("color: palette(mid);");
        layout->addRow("", hint);

        createLodResource_ = new QCheckBox("Create LOD clamp resource", tab);
        createLodResource_->setChecked(false);
        connect(createLodResource_, &QCheckBox::toggled, this, &OptionsDialog::updateEnabledStates);
        layout->addRow("", createLodResource_);

        lodU_ = new QSpinBox(tab);
        lodV_ = new QSpinBox(tab);
        lodU_->setRange(0, 255);
        lodV_->setRange(0, 255);
        lodU_->setValue(0);
        lodV_->setValue(0);
        layout->addRow("LOD clamp U:", lodU_);
        layout->addRow("LOD clamp V:", lodV_);

        createInformationResource_ = new QCheckBox("Create Information resource (writes KVD)", tab);
        createInformationResource_->setChecked(false);
        connect(createInformationResource_, &QCheckBox::toggled, this, &OptionsDialog::updateEnabledStates);
        layout->addRow("", createInformationResource_);

        infoAuthor_ = new QLineEdit(tab);
        infoContact_ = new QLineEdit(tab);
        infoVersion_ = new QLineEdit(tab);
        infoModification_ = new QLineEdit(tab);
        infoDescription_ = new QPlainTextEdit(tab);
        infoComments_ = new QPlainTextEdit(tab);
        infoDescription_->setMaximumHeight(80);
        infoComments_->setMaximumHeight(80);
        layout->addRow("Author:", infoAuthor_);
        layout->addRow("Contact:", infoContact_);
        layout->addRow("Version:", infoVersion_);
        layout->addRow("Modification:", infoModification_);
        layout->addRow("Description:", infoDescription_);
        layout->addRow("Comments:", infoComments_);

        tabs_->addTab(tab, "Resources");
    }

    buttons_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    auto *reset = buttons_->addButton("Reset", QDialogButtonBox::ResetRole);
    connect(reset, &QPushButton::clicked, this, &OptionsDialog::resetToDefaults);
    connect(buttons_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons_, &QDialogButtonBox::accepted, this, [this] {
        saveToSettings();
        applyVtflibGlobals();
        accept();
    });
    root->addWidget(buttons_);

    loadFromSettings();
    updateEnabledStates();
}

OptionsDialog::~OptionsDialog() = default;

void OptionsDialog::resetToDefaults() {
    versionMinor_->setValue(5);
    textureType_->setCurrentIndex(0);
    populateFormatCombo(normalFormat_, vtflibCanEncode(IMAGE_FORMAT_DXT5) ? IMAGE_FORMAT_DXT5 : IMAGE_FORMAT_RGBA8888);
    useAlphaFormat_->setChecked(false);
    populateFormatCombo(alphaFormat_, IMAGE_FORMAT_DXT5);

    mipmaps_->setChecked(true);
    mipmapFilter_->setCurrentIndex(comboIndexForData(mipmapFilter_, static_cast<int>(MIPMAP_FILTER_DEFAULT), 0));
    if(thumbnail_->isEnabled()) thumbnail_->setChecked(vtflibCanEncode(IMAGE_FORMAT_DXT1));
    reflectivity_->setChecked(true);
    sphereMap_->setChecked(false);
    srgb_->setChecked(false);

    gammaCorrection_->setChecked(false);
    gammaValue_->setValue(2.2);

    resizeEnabled_->setChecked(false);
    resizeMethod_->setCurrentIndex(0);
    resizeFilter_->setCurrentIndex(comboIndexForData(resizeFilter_, static_cast<int>(MIPMAP_FILTER_DEFAULT), 0));
    resizeClamp_->setChecked(false);
    resizeClampW_->setValue(1024);
    resizeClampH_->setValue(1024);

    alphaThreshold_->setValue(0);
    lumR_->setValue(0.299);
    lumG_->setValue(0.587);
    lumB_->setValue(0.114);

    createLodResource_->setChecked(false);
    lodU_->setValue(0);
    lodV_->setValue(0);
    createInformationResource_->setChecked(false);
    infoAuthor_->clear();
    infoContact_->clear();
    infoVersion_->clear();
    infoModification_->clear();
    infoDescription_->setPlainText({});
    infoComments_->setPlainText({});

    updateEnabledStates();
}

void OptionsDialog::updateEnabledStates() {
    alphaFormat_->setEnabled(useAlphaFormat_->isChecked());
    mipmapFilter_->setEnabled(mipmaps_->isChecked());

    gammaValue_->setEnabled(gammaCorrection_->isChecked());

    const bool resize = resizeEnabled_->isChecked();
    resizeMethod_->setEnabled(resize);
    resizeFilter_->setEnabled(resize);
    resizeClamp_->setEnabled(resize);

    const bool clamp = resize && resizeClamp_->isChecked();
    resizeClampW_->setEnabled(clamp);
    resizeClampH_->setEnabled(clamp);

    const bool lod = createLodResource_->isChecked();
    lodU_->setEnabled(lod);
    lodV_->setEnabled(lod);

    const bool info = createInformationResource_->isChecked();
    for(auto *w : {infoAuthor_, infoContact_, infoVersion_, infoModification_}) w->setEnabled(info);
    infoDescription_->setEnabled(info);
    infoComments_->setEnabled(info);
}

void OptionsDialog::loadFromSettings() {
    QSettings s;

    versionMinor_->setValue(std::clamp(s.value("options/create/versionMinor", 5).toInt(), 0, 5));
    textureType_->setCurrentIndex(std::clamp(s.value("options/create/textureType", 0).toInt(), 0, 2));

    populateFormatCombo(normalFormat_, static_cast<VTFImageFormat>(s.value("options/create/normalFormat", static_cast<int>(normalFormat_->currentData().toInt())).toInt()));
    const int alphaFmt = s.value("options/create/alphaFormat", static_cast<int>(IMAGE_FORMAT_DXT5)).toInt();
    alphaFormat_->setCurrentIndex(comboIndexForData(alphaFormat_, alphaFmt, 0));

    useAlphaFormat_->setChecked(s.value("options/create/useAlphaFormat", false).toBool());

    mipmaps_->setChecked(s.value("options/create/mipmaps", true).toBool());
    mipmapFilter_->setCurrentIndex(comboIndexForData(mipmapFilter_, s.value("options/create/mipmapFilter", static_cast<int>(MIPMAP_FILTER_DEFAULT)).toInt(), 0));

    if(thumbnail_->isEnabled()) {
        thumbnail_->setChecked(s.value("options/create/thumbnail", thumbnail_->isChecked()).toBool());
    }
    reflectivity_->setChecked(s.value("options/create/reflectivity", true).toBool());
    sphereMap_->setChecked(s.value("options/create/sphereMap", false).toBool());
    srgb_->setChecked(s.value("options/create/srgb", false).toBool());

    gammaCorrection_->setChecked(s.value("options/create/gammaEnabled", false).toBool());
    gammaValue_->setValue(std::clamp(s.value("options/create/gammaValue", 2.2).toDouble(), 0.1, 5.0));

    resizeEnabled_->setChecked(s.value("options/create/resizeEnabled", false).toBool());
    resizeMethod_->setCurrentIndex(comboIndexForData(resizeMethod_, s.value("options/create/resizeMethod", static_cast<int>(RESIZE_NEAREST_POWER2)).toInt(), 0));
    resizeFilter_->setCurrentIndex(comboIndexForData(resizeFilter_, s.value("options/create/resizeFilter", static_cast<int>(MIPMAP_FILTER_DEFAULT)).toInt(), 0));
    resizeClamp_->setChecked(s.value("options/create/resizeClamp", false).toBool());
    resizeClampW_->setValue(std::clamp(s.value("options/create/resizeClampWidth", 1024).toInt(), 1, 16384));
    resizeClampH_->setValue(std::clamp(s.value("options/create/resizeClampHeight", 1024).toInt(), 1, 16384));

    alphaThreshold_->setValue(std::clamp(s.value("options/create/alphaThreshold", 0).toInt(), 0, 255));

    lumR_->setValue(std::clamp(s.value("options/luminance/r", 0.299).toDouble(), 0.0, 1.0));
    lumG_->setValue(std::clamp(s.value("options/luminance/g", 0.587).toDouble(), 0.0, 1.0));
    lumB_->setValue(std::clamp(s.value("options/luminance/b", 0.114).toDouble(), 0.0, 1.0));

    createLodResource_->setChecked(s.value("options/resources/createLod", false).toBool());
    lodU_->setValue(std::clamp(s.value("options/resources/lodClampU", 0).toInt(), 0, 255));
    lodV_->setValue(std::clamp(s.value("options/resources/lodClampV", 0).toInt(), 0, 255));

    createInformationResource_->setChecked(s.value("options/resources/createInformation", false).toBool());
    infoAuthor_->setText(s.value("options/resources/infoAuthor").toString());
    infoContact_->setText(s.value("options/resources/infoContact").toString());
    infoVersion_->setText(s.value("options/resources/infoVersion").toString());
    infoModification_->setText(s.value("options/resources/infoModification").toString());
    infoDescription_->setPlainText(s.value("options/resources/infoDescription").toString());
    infoComments_->setPlainText(s.value("options/resources/infoComments").toString());
}

void OptionsDialog::saveToSettings() const {
    QSettings s;

    s.setValue("options/create/versionMinor", versionMinor_->value());
    s.setValue("options/create/textureType", textureType_ ? textureType_->currentData().toInt() : 0);
    s.setValue("options/create/normalFormat", normalFormat_->currentData().toInt());
    s.setValue("options/create/useAlphaFormat", useAlphaFormat_->isChecked());
    s.setValue("options/create/alphaFormat", alphaFormat_->currentData().toInt());
    s.setValue("options/create/mipmaps", mipmaps_->isChecked());
    s.setValue("options/create/mipmapFilter", mipmapFilter_->currentData().toInt());
    s.setValue("options/create/thumbnail", thumbnail_->isChecked());
    s.setValue("options/create/reflectivity", reflectivity_->isChecked());
    s.setValue("options/create/sphereMap", sphereMap_->isChecked());
    s.setValue("options/create/srgb", srgb_->isChecked());
    s.setValue("options/create/gammaEnabled", gammaCorrection_->isChecked());
    s.setValue("options/create/gammaValue", gammaValue_->value());
    s.setValue("options/create/resizeEnabled", resizeEnabled_->isChecked());
    s.setValue("options/create/resizeMethod", resizeMethod_->currentData().toInt());
    s.setValue("options/create/resizeFilter", resizeFilter_->currentData().toInt());
    s.setValue("options/create/resizeClamp", resizeClamp_->isChecked());
    s.setValue("options/create/resizeClampWidth", resizeClampW_->value());
    s.setValue("options/create/resizeClampHeight", resizeClampH_->value());
    s.setValue("options/create/alphaThreshold", alphaThreshold_->value());

    s.setValue("options/luminance/r", lumR_->value());
    s.setValue("options/luminance/g", lumG_->value());
    s.setValue("options/luminance/b", lumB_->value());

    s.setValue("options/resources/createLod", createLodResource_->isChecked());
    s.setValue("options/resources/lodClampU", lodU_->value());
    s.setValue("options/resources/lodClampV", lodV_->value());

    s.setValue("options/resources/createInformation", createInformationResource_->isChecked());
    s.setValue("options/resources/infoAuthor", infoAuthor_->text().trimmed());
    s.setValue("options/resources/infoContact", infoContact_->text().trimmed());
    s.setValue("options/resources/infoVersion", infoVersion_->text().trimmed());
    s.setValue("options/resources/infoModification", infoModification_->text().trimmed());
    s.setValue("options/resources/infoDescription", infoDescription_->toPlainText().trimmed());
    s.setValue("options/resources/infoComments", infoComments_->toPlainText().trimmed());
}

void OptionsDialog::applyVtflibGlobals() const {
    vlSetFloat(VTFLIB_LUMINANCE_WEIGHT_R, static_cast<vlSingle>(lumR_->value()));
    vlSetFloat(VTFLIB_LUMINANCE_WEIGHT_G, static_cast<vlSingle>(lumG_->value()));
    vlSetFloat(VTFLIB_LUMINANCE_WEIGHT_B, static_cast<vlSingle>(lumB_->value()));
}
