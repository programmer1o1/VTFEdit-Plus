#include "VtfFlagsDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace {

struct FlagItem {
    const char *name;
    vlUInt mask;
};

constexpr FlagItem kFlags[] = {
    {"Point sample", TEXTUREFLAGS_POINTSAMPLE},
    {"Trilinear", TEXTUREFLAGS_TRILINEAR},
    {"Clamp S", TEXTUREFLAGS_CLAMPS},
    {"Clamp T", TEXTUREFLAGS_CLAMPT},
    {"Anisotropic", TEXTUREFLAGS_ANISOTROPIC},
    {"Hint DXT5", TEXTUREFLAGS_HINT_DXT5},
    {"sRGB", TEXTUREFLAGS_SRGB},
    {"Normal map", TEXTUREFLAGS_NORMAL},
    {"No mipmaps", TEXTUREFLAGS_NOMIP},
    {"No LOD", TEXTUREFLAGS_NOLOD},
    {"Min mip", TEXTUREFLAGS_MINMIP},
    {"Procedural", TEXTUREFLAGS_PROCEDURAL},
    {"One bit alpha", TEXTUREFLAGS_ONEBITALPHA},
    {"Eight bit alpha", TEXTUREFLAGS_EIGHTBITALPHA},
    {"Envmap", TEXTUREFLAGS_ENVMAP},
    {"Render target", TEXTUREFLAGS_RENDERTARGET},
    {"Depth render target", TEXTUREFLAGS_DEPTHRENDERTARGET},
    {"No debug override", TEXTUREFLAGS_NODEBUGOVERRIDE},
    {"Single copy", TEXTUREFLAGS_SINGLECOPY},
    {"Clamp U", TEXTUREFLAGS_CLAMPU},
    {"Vertex texture", TEXTUREFLAGS_VERTEXTEXTURE},
    {"SSBump", TEXTUREFLAGS_SSBUMP},
    {"No depth buffer", TEXTUREFLAGS_NODEPTHBUFFER},
    {"Border", TEXTUREFLAGS_BORDER},
};

QString hexFlags(vlUInt flags) {
    return QString("0x%1").arg(static_cast<qulonglong>(flags), 8, 16, QChar('0')).toUpper();
}

} // namespace

VtfFlagsDialog::VtfFlagsDialog(vlUInt initialFlags, QWidget *parent)
    : QDialog(parent), initialFlags_(initialFlags) {
    setWindowTitle("Edit VTF Flags");
    setModal(true);
    resize(520, 520);

    auto *root = new QVBoxLayout(this);

    auto *topRow = new QHBoxLayout;
    filter_ = new QLineEdit(this);
    filter_->setPlaceholderText("Filter flags…");
    connect(filter_, &QLineEdit::textChanged, this, &VtfFlagsDialog::filterChanged);
    topRow->addWidget(filter_, 1);

    auto *btnAll = new QPushButton("All", this);
    connect(btnAll, &QPushButton::clicked, this, &VtfFlagsDialog::selectAll);
    topRow->addWidget(btnAll);

    auto *btnNone = new QPushButton("None", this);
    connect(btnNone, &QPushButton::clicked, this, &VtfFlagsDialog::selectNone);
    topRow->addWidget(btnNone);
    root->addLayout(topRow);

    summary_ = new QLabel(this);
    root->addWidget(summary_);

    container_ = new QWidget(this);
    auto *containerLayout = new QVBoxLayout(container_);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(4);

    for(const auto &flag : kFlags) {
        FlagRow row;
        row.mask = flag.mask;
        row.name = QString::fromUtf8(flag.name);
        row.check = new QCheckBox(row.name, container_);
        row.check->setChecked((initialFlags_ & row.mask) != 0);
        connect(row.check, &QCheckBox::toggled, this, &VtfFlagsDialog::updateSummary);
        rows_.push_back(row);
        containerLayout->addWidget(row.check);
    }
    containerLayout->addStretch(1);

    scroll_ = new QScrollArea(this);
    scroll_->setWidgetResizable(true);
    scroll_->setWidget(container_);
    root->addWidget(scroll_, 1);

    buttons_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons_, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons_);

    updateSummary();
}

VtfFlagsDialog::~VtfFlagsDialog() = default;

vlUInt VtfFlagsDialog::flags() const {
    vlUInt out = 0;
    for(const auto &row : rows_) {
        if(row.check->isChecked()) out |= row.mask;
    }
    return out;
}

void VtfFlagsDialog::filterChanged(const QString &) {
    applyFilter();
}

void VtfFlagsDialog::applyFilter() {
    const QString needle = filter_->text().trimmed();
    if(needle.isEmpty()) {
        for(const auto &row : rows_) row.check->setVisible(true);
        return;
    }
    for(const auto &row : rows_) {
        row.check->setVisible(row.name.contains(needle, Qt::CaseInsensitive));
    }
}

void VtfFlagsDialog::updateSummary() {
    const vlUInt current = flags();
    const vlUInt changed = (current ^ initialFlags_);
    summary_->setText(QString("Current: %1    Changed: %2").arg(hexFlags(current), hexFlags(changed)));
}

void VtfFlagsDialog::selectAll() {
    for(const auto &row : rows_) row.check->setChecked(true);
}

void VtfFlagsDialog::selectNone() {
    for(const auto &row : rows_) row.check->setChecked(false);
}

