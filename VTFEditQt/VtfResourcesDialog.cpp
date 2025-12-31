#include "VtfResourcesDialog.h"

#include <VTFLib.h>

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

#include <cstring>

namespace {

QString resourceTypeName(vlUInt type) {
    switch(type) {
        case VTF_RSRC_CRC: return "CRC";
        case VTF_RSRC_TEXTURE_LOD_SETTINGS: return "Texture LOD Settings";
        case VTF_RSRC_TEXTURE_SETTINGS_EX: return "Texture Settings (Ex)";
        case VTF_RSRC_KEY_VALUE_DATA: return "Key-Value Data";
        case VTF_RSRC_SHEET: return "Sheet";
        case VTF_LEGACY_RSRC_IMAGE: return "Image Data (legacy)";
        case VTF_LEGACY_RSRC_LOW_RES_IMAGE: return "Thumbnail (legacy)";
        default: break;
    }

    const char a = static_cast<char>(type & 0xFF);
    const char b = static_cast<char>((type >> 8) & 0xFF);
    const char c = static_cast<char>((type >> 16) & 0xFF);
    const QString chars = QString("%1%2%3").arg(QChar(a)).arg(QChar(b)).arg(QChar(c));
    return QString("0x%1 (%2)").arg(static_cast<qulonglong>(type), 8, 16, QChar('0')).toUpper().arg(chars);
}

bool parseUint32(QString text, vlUInt *out) {
    if(!out) return false;
    text = text.trimmed();
    if(text.isEmpty()) return false;

    int base = 10;
    if(text.startsWith("0x", Qt::CaseInsensitive)) {
        base = 16;
        text = text.mid(2);
    } else {
        for(const QChar ch : text) {
            if((ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f')) {
                base = 16;
                break;
            }
        }
    }

    bool ok = false;
    const unsigned int v = text.toUInt(&ok, base);
    if(!ok) return false;
    *out = static_cast<vlUInt>(v);
    return true;
}

vlUInt readResourceU32(vlUInt type) {
    vlUInt size = 0;
    void *p = vlImageGetResourceData(type, &size);
    if(!p || size != sizeof(vlUInt)) return 0;
    vlUInt v = 0;
    std::memcpy(&v, p, sizeof(v));
    return v;
}

QString nodeValueToString(VTFLib::Nodes::CVMTNode *node) {
    if(!node) return {};
    switch(node->GetType()) {
        case NODE_TYPE_STRING: {
            auto *s = dynamic_cast<VTFLib::Nodes::CVMTStringNode *>(node);
            if(!s || !s->GetValue()) return {};
            return QString::fromUtf8(s->GetValue());
        }
        case NODE_TYPE_INTEGER: {
            auto *i = dynamic_cast<VTFLib::Nodes::CVMTIntegerNode *>(node);
            if(!i) return {};
            return QString::number(i->GetValue());
        }
        case NODE_TYPE_SINGLE: {
            auto *f = dynamic_cast<VTFLib::Nodes::CVMTSingleNode *>(node);
            if(!f) return {};
            return QString::number(static_cast<double>(f->GetValue()), 'g', 9);
        }
        default:
            return {};
    }
}

struct InformationFields {
    QString rootName;
    QString author;
    QString contact;
    QString version;
    QString modification;
    QString description;
    QString comments;
};

QString quoted(QString s) {
    s.replace('\\', "\\\\");
    s.replace('"', "\\\"");
    s.replace('\r', "");
    s.replace('\n', "\\n");
    return QString("\"%1\"").arg(s);
}

QString buildInformationKvdText(const InformationFields &f) {
    QStringList lines;
    lines << quoted("Information");
    lines << "{";
    auto add = [&](const QString &k, const QString &v) {
        const QString t = v.trimmed();
        if(t.isEmpty()) return;
        lines << QString("\t%1 %2").arg(quoted(k), quoted(t));
    };
    add("Author", f.author);
    add("Contact", f.contact);
    add("Version", f.version);
    add("Modification", f.modification);
    add("Description", f.description);
    add("Comments", f.comments);
    lines << "}";
    lines << "";
    return lines.join('\n');
}

bool tryParseInformationFromKvdText(const QString &text, InformationFields *out, QString *error) {
    if(out) *out = {};
    VTFLib::CVMTFile vmt;
    const QByteArray bytes = text.toUtf8();
    if(!vmt.Load(bytes.constData(), static_cast<vlUInt>(bytes.size()))) {
        if(error) *error = QString::fromUtf8(vlGetLastError());
        return false;
    }
    auto *root = vmt.GetRoot();
    if(!root) {
        if(error) *error = "No root node.";
        return false;
    }

    InformationFields f;
    if(root->GetName()) f.rootName = QString::fromUtf8(root->GetName());
    f.author = nodeValueToString(root->GetNode("Author"));
    f.contact = nodeValueToString(root->GetNode("Contact"));
    f.version = nodeValueToString(root->GetNode("Version"));
    f.modification = nodeValueToString(root->GetNode("Modification"));
    f.description = nodeValueToString(root->GetNode("Description"));
    f.comments = nodeValueToString(root->GetNode("Comments"));
    if(out) *out = f;
    return true;
}

} // namespace

VtfResourcesDialog::VtfResourcesDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("VTF Resources");
    setModal(true);

    auto *root = new QVBoxLayout(this);

    auto *hint = new QLabel("Resources are supported by VTF v7.3+ (CRC, LOD clamp, settings, keyvalues, etc.).", this);
    hint->setWordWrap(true);
    root->addWidget(hint);

    auto *crcGroup = new QGroupBox("CRC", this);
    auto *crcLayout = new QFormLayout(crcGroup);
    crcEnabled_ = new QCheckBox("Enable CRC resource", crcGroup);
    crcHex_ = new QLineEdit(crcGroup);
    crcHex_->setPlaceholderText("0x00000000");
    crcLayout->addRow(crcEnabled_);
    crcLayout->addRow("Value:", crcHex_);
    root->addWidget(crcGroup);

    auto *lodGroup = new QGroupBox("Texture LOD Settings", this);
    auto *lodLayout = new QFormLayout(lodGroup);
    lodEnabled_ = new QCheckBox("Enable LOD clamp resource", lodGroup);
    lodU_ = new QSpinBox(lodGroup);
    lodV_ = new QSpinBox(lodGroup);
    lodU_->setRange(0, 255);
    lodV_->setRange(0, 255);
    lodLayout->addRow(lodEnabled_);
    lodLayout->addRow("Clamp U:", lodU_);
    lodLayout->addRow("Clamp V:", lodV_);
    root->addWidget(lodGroup);

    auto *tsoGroup = new QGroupBox("Texture Settings (Ex)", this);
    auto *tsoLayout = new QFormLayout(tsoGroup);
    tsoEnabled_ = new QCheckBox("Enable settings resource", tsoGroup);
    tso0_ = new QSpinBox(tsoGroup);
    tso1_ = new QSpinBox(tsoGroup);
    tso2_ = new QSpinBox(tsoGroup);
    tso3_ = new QSpinBox(tsoGroup);
    for(auto *s : {tso0_, tso1_, tso2_, tso3_}) s->setRange(0, 255);
    tsoLayout->addRow(tsoEnabled_);
    tsoLayout->addRow("Byte 0:", tso0_);
    tsoLayout->addRow("Byte 1:", tso1_);
    tsoLayout->addRow("Byte 2:", tso2_);
    tsoLayout->addRow("Byte 3:", tso3_);
    root->addWidget(tsoGroup);

    auto *kvdGroup = new QGroupBox("Key-Value Data", this);
    auto *kvdLayout = new QVBoxLayout(kvdGroup);
    kvdEnabled_ = new QCheckBox("Enable key-value resource", kvdGroup);
    kvdLayout->addWidget(kvdEnabled_);

    kvdTabs_ = new QTabWidget(kvdGroup);
    {
        auto *rawTab = new QWidget(kvdTabs_);
        auto *rawLayout = new QVBoxLayout(rawTab);
        rawLayout->setContentsMargins(0, 0, 0, 0);
        kvdText_ = new QPlainTextEdit(rawTab);
        kvdText_->setPlaceholderText("\"Key\" \"Value\"\n");
        kvdText_->setMinimumHeight(140);
        rawLayout->addWidget(kvdText_);
        kvdTabs_->addTab(rawTab, "Raw");
    }
    {
        auto *infoTab = new QWidget(kvdTabs_);
        auto *infoLayout = new QVBoxLayout(infoTab);
        infoLayout->setContentsMargins(0, 0, 0, 0);

        auto *hint = new QLabel("Edits the KVD resource using the Windows \"Information Resource\" schema.", infoTab);
        hint->setWordWrap(true);
        infoLayout->addWidget(hint);

        auto *form = new QFormLayout;
        infoAuthor_ = new QLineEdit(infoTab);
        infoContact_ = new QLineEdit(infoTab);
        infoVersion_ = new QLineEdit(infoTab);
        infoModification_ = new QLineEdit(infoTab);
        infoDescription_ = new QPlainTextEdit(infoTab);
        infoComments_ = new QPlainTextEdit(infoTab);
        infoDescription_->setMaximumHeight(90);
        infoComments_->setMaximumHeight(90);
        form->addRow("Author:", infoAuthor_);
        form->addRow("Contact:", infoContact_);
        form->addRow("Version:", infoVersion_);
        form->addRow("Modification:", infoModification_);
        form->addRow("Description:", infoDescription_);
        form->addRow("Comments:", infoComments_);
        infoLayout->addLayout(form);

        infoParseStatus_ = new QPlainTextEdit(infoTab);
        infoParseStatus_->setReadOnly(true);
        infoParseStatus_->setMaximumHeight(70);
        infoParseStatus_->setPlaceholderText("Parse status");
        infoLayout->addWidget(infoParseStatus_);

        kvdTabs_->addTab(infoTab, "Information");
    }

    kvdLayout->addWidget(kvdTabs_);
    root->addWidget(kvdGroup);

    auto *otherGroup = new QGroupBox("Other Resources (read-only)", this);
    auto *otherLayout = new QVBoxLayout(otherGroup);
    otherResources_ = new QPlainTextEdit(otherGroup);
    otherResources_->setReadOnly(true);
    otherResources_->setMinimumHeight(100);
    otherLayout->addWidget(otherResources_);
    root->addWidget(otherGroup);

    buttons_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons_, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons_);

    for(auto *cb : {crcEnabled_, lodEnabled_, tsoEnabled_, kvdEnabled_}) {
        connect(cb, &QCheckBox::toggled, this, &VtfResourcesDialog::updateEnabledStates);
    }

    connect(kvdTabs_, &QTabWidget::currentChanged, this, [this](int idx) {
        if(!kvdEnabled_ || !kvdEnabled_->isChecked()) return;
        if(!kvdTabs_) return;
        // "Information" tab is index 1.
        if(idx == 1) loadInformationFieldsFromKvdText();
    });
    for(auto *le : {infoAuthor_, infoContact_, infoVersion_, infoModification_}) {
        connect(le, &QLineEdit::textChanged, this, &VtfResourcesDialog::updateKvdTextFromInformationFields);
    }
    connect(infoDescription_, &QPlainTextEdit::textChanged, this, &VtfResourcesDialog::updateKvdTextFromInformationFields);
    connect(infoComments_, &QPlainTextEdit::textChanged, this, &VtfResourcesDialog::updateKvdTextFromInformationFields);

    loadFromBoundVtf();
    updateEnabledStates();
}

VtfResourcesDialog::~VtfResourcesDialog() = default;

void VtfResourcesDialog::updateEnabledStates() {
    crcHex_->setEnabled(crcEnabled_->isChecked());
    lodU_->setEnabled(lodEnabled_->isChecked());
    lodV_->setEnabled(lodEnabled_->isChecked());
    tso0_->setEnabled(tsoEnabled_->isChecked());
    tso1_->setEnabled(tsoEnabled_->isChecked());
    tso2_->setEnabled(tsoEnabled_->isChecked());
    tso3_->setEnabled(tsoEnabled_->isChecked());
    kvdTabs_->setEnabled(kvdEnabled_->isChecked());
    kvdText_->setEnabled(kvdEnabled_->isChecked() && kvdTabs_ && kvdTabs_->currentIndex() == 0);
    for(auto *w : {infoAuthor_, infoContact_, infoVersion_, infoModification_}) w->setEnabled(kvdEnabled_->isChecked());
    infoDescription_->setEnabled(kvdEnabled_->isChecked());
    infoComments_->setEnabled(kvdEnabled_->isChecked());
    infoParseStatus_->setEnabled(kvdEnabled_->isChecked());
}

void VtfResourcesDialog::loadFromBoundVtf() {
    crcEnabled_->setChecked(vlImageGetHasResource(VTF_RSRC_CRC) ? true : false);
    if(crcEnabled_->isChecked()) {
        const vlUInt v = readResourceU32(VTF_RSRC_CRC);
        crcHex_->setText(QString("0x%1").arg(static_cast<qulonglong>(v), 8, 16, QChar('0')).toUpper());
    } else {
        crcHex_->setText("0x00000000");
    }

    lodEnabled_->setChecked(vlImageGetHasResource(VTF_RSRC_TEXTURE_LOD_SETTINGS) ? true : false);
    if(lodEnabled_->isChecked()) {
        const vlUInt packed = readResourceU32(VTF_RSRC_TEXTURE_LOD_SETTINGS);
        SVTFTextureLODControlResource lod{};
        std::memcpy(&lod, &packed, sizeof(lod));
        lodU_->setValue(lod.ResolutionClampU);
        lodV_->setValue(lod.ResolutionClampV);
    } else {
        lodU_->setValue(0);
        lodV_->setValue(0);
    }

    tsoEnabled_->setChecked(vlImageGetHasResource(VTF_RSRC_TEXTURE_SETTINGS_EX) ? true : false);
    if(tsoEnabled_->isChecked()) {
        const vlUInt packed = readResourceU32(VTF_RSRC_TEXTURE_SETTINGS_EX);
        SVTFTextureSettingsExResource tso{};
        std::memcpy(&tso, &packed, sizeof(tso));
        tso0_->setValue(tso.Flags0);
        tso1_->setValue(tso.Flags1);
        tso2_->setValue(tso.Flags2);
        tso3_->setValue(tso.Flags3);
    } else {
        tso0_->setValue(0);
        tso1_->setValue(0);
        tso2_->setValue(0);
        tso3_->setValue(0);
    }

    kvdEnabled_->setChecked(vlImageGetHasResource(VTF_RSRC_KEY_VALUE_DATA) ? true : false);
    if(kvdEnabled_->isChecked()) {
        vlUInt size = 0;
        void *p = vlImageGetResourceData(VTF_RSRC_KEY_VALUE_DATA, &size);
        QByteArray bytes;
        if(p && size > 0) bytes = QByteArray(reinterpret_cast<const char *>(p), static_cast<int>(size));
        if(!bytes.isEmpty() && bytes.endsWith('\0')) bytes.chop(1);
        kvdText_->setPlainText(QString::fromUtf8(bytes));
    } else {
        kvdText_->setPlainText({});
    }

    loadInformationFieldsFromKvdText();
    rebuildOtherResourcesList();
}

void VtfResourcesDialog::loadInformationFieldsFromKvdText() {
    if(updatingKvdUi_) return;
    updatingKvdUi_ = true;

    infoParseStatus_->clear();
    const QString raw = kvdText_ ? kvdText_->toPlainText() : QString{};
    if(raw.trimmed().isEmpty()) {
        infoAuthor_->clear();
        infoContact_->clear();
        infoVersion_->clear();
        infoModification_->clear();
        infoDescription_->setPlainText({});
        infoComments_->setPlainText({});
        infoParseStatus_->setPlainText("KVD is empty.");
        updatingKvdUi_ = false;
        return;
    }

    InformationFields f;
    QString err;
    if(!tryParseInformationFromKvdText(raw, &f, &err)) {
        infoParseStatus_->setPlainText(QString("Parse failed: %1").arg(err));
        updatingKvdUi_ = false;
        return;
    }

    infoAuthor_->setText(f.author);
    infoContact_->setText(f.contact);
    infoVersion_->setText(f.version);
    infoModification_->setText(f.modification);
    infoDescription_->setPlainText(f.description);
    infoComments_->setPlainText(f.comments);

    const QString rootName = f.rootName.trimmed();
    if(!rootName.isEmpty() && rootName.compare("Information", Qt::CaseInsensitive) != 0) {
        infoParseStatus_->setPlainText(QString("Note: root is %1 (expected \"Information\"). Editing will overwrite it.").arg(rootName));
    } else {
        infoParseStatus_->setPlainText("OK");
    }

    updatingKvdUi_ = false;
}

void VtfResourcesDialog::updateKvdTextFromInformationFields() {
    if(updatingKvdUi_) return;
    if(!kvdTabs_ || kvdTabs_->currentIndex() != 1) return; // Only auto-sync when editing the Information tab.
    if(!kvdEnabled_ || !kvdEnabled_->isChecked()) return;

    updatingKvdUi_ = true;
    InformationFields f;
    f.author = infoAuthor_->text();
    f.contact = infoContact_->text();
    f.version = infoVersion_->text();
    f.modification = infoModification_->text();
    f.description = infoDescription_->toPlainText();
    f.comments = infoComments_->toPlainText();
    kvdText_->setPlainText(buildInformationKvdText(f));
    infoParseStatus_->setPlainText("OK (generated)");
    updatingKvdUi_ = false;
}

void VtfResourcesDialog::rebuildOtherResourcesList() {
    QStringList lines;
    const vlUInt count = vlImageGetResourceCount();
    for(vlUInt i = 0; i < count; ++i) {
        const vlUInt type = vlImageGetResourceType(i);
        if(type == VTF_RSRC_CRC || type == VTF_RSRC_TEXTURE_LOD_SETTINGS || type == VTF_RSRC_TEXTURE_SETTINGS_EX || type == VTF_RSRC_KEY_VALUE_DATA) {
            continue;
        }
        if(type == VTF_LEGACY_RSRC_IMAGE || type == VTF_LEGACY_RSRC_LOW_RES_IMAGE) {
            continue;
        }

        vlUInt size = 0;
        (void)vlImageGetResourceData(type, &size);
        lines.push_back(QString("%1  (%2 bytes)").arg(resourceTypeName(type)).arg(size));
    }
    if(lines.isEmpty()) lines.push_back("(None)");
    otherResources_->setPlainText(lines.join('\n'));
}

bool VtfResourcesDialog::applyToBoundVtf(QString *error) {
    auto fail = [&](const QString &msg) {
        if(error) *error = msg;
        return false;
    };

    if(crcEnabled_->isChecked()) {
        vlUInt crc = 0;
        if(!parseUint32(crcHex_->text(), &crc)) return fail("CRC value must be a valid 32-bit integer (decimal or 0x... hex).");
        if(!vlImageSetResourceData(VTF_RSRC_CRC, sizeof(vlUInt), &crc)) return fail(QString::fromUtf8(vlGetLastError()));
    } else if(vlImageGetHasResource(VTF_RSRC_CRC)) {
        if(!vlImageSetResourceData(VTF_RSRC_CRC, 0, nullptr)) return fail(QString::fromUtf8(vlGetLastError()));
    }

    if(lodEnabled_->isChecked()) {
        SVTFTextureLODControlResource lod{};
        lod.ResolutionClampU = static_cast<vlByte>(lodU_->value());
        lod.ResolutionClampV = static_cast<vlByte>(lodV_->value());
        vlUInt packed = 0;
        std::memcpy(&packed, &lod, sizeof(lod));
        if(!vlImageSetResourceData(VTF_RSRC_TEXTURE_LOD_SETTINGS, sizeof(vlUInt), &packed)) return fail(QString::fromUtf8(vlGetLastError()));
    } else if(vlImageGetHasResource(VTF_RSRC_TEXTURE_LOD_SETTINGS)) {
        if(!vlImageSetResourceData(VTF_RSRC_TEXTURE_LOD_SETTINGS, 0, nullptr)) return fail(QString::fromUtf8(vlGetLastError()));
    }

    if(tsoEnabled_->isChecked()) {
        SVTFTextureSettingsExResource tso{};
        tso.Flags0 = static_cast<vlByte>(tso0_->value());
        tso.Flags1 = static_cast<vlByte>(tso1_->value());
        tso.Flags2 = static_cast<vlByte>(tso2_->value());
        tso.Flags3 = static_cast<vlByte>(tso3_->value());
        vlUInt packed = 0;
        std::memcpy(&packed, &tso, sizeof(tso));
        if(!vlImageSetResourceData(VTF_RSRC_TEXTURE_SETTINGS_EX, sizeof(vlUInt), &packed)) return fail(QString::fromUtf8(vlGetLastError()));
    } else if(vlImageGetHasResource(VTF_RSRC_TEXTURE_SETTINGS_EX)) {
        if(!vlImageSetResourceData(VTF_RSRC_TEXTURE_SETTINGS_EX, 0, nullptr)) return fail(QString::fromUtf8(vlGetLastError()));
    }

    if(kvdEnabled_->isChecked()) {
        QByteArray bytes = kvdText_->toPlainText().toUtf8();
        bytes.push_back('\0');
        if(!vlImageSetResourceData(VTF_RSRC_KEY_VALUE_DATA, static_cast<vlUInt>(bytes.size()), bytes.data())) return fail(QString::fromUtf8(vlGetLastError()));
    } else if(vlImageGetHasResource(VTF_RSRC_KEY_VALUE_DATA)) {
        if(!vlImageSetResourceData(VTF_RSRC_KEY_VALUE_DATA, 0, nullptr)) return fail(QString::fromUtf8(vlGetLastError()));
    }

    return true;
}
