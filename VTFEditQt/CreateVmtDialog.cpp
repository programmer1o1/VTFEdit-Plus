#include "CreateVmtDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QVBoxLayout>

#include <QStringList>

namespace {

QString quoted(const QString &s) {
    QString out = s;
    out.replace('\\', "\\\\");
    out.replace('"', "\\\"");
    return QString("\"%1\"").arg(out);
}

} // namespace

CreateVmtDialog::CreateVmtDialog(QString suggestedBaseTexture, QWidget *parent, bool suggestAlphaTest, bool suggestTranslucent) : QDialog(parent) {
    setWindowTitle("Create VMT");
    setModal(true);
    resize(720, 560);

    auto *root = new QVBoxLayout(this);
    auto *form = new QFormLayout;

    shader_ = new QComboBox(this);
    shader_->addItem("LightmappedGeneric");
    shader_->addItem("VertexLitGeneric");
    shader_->addItem("UnlitGeneric");
    shader_->addItem("Custom…");
    connect(shader_, &QComboBox::currentIndexChanged, this, &CreateVmtDialog::updatePreview);
    form->addRow("Shader:", shader_);

    baseTexture_ = new QLineEdit(this);
    baseTexture_->setPlaceholderText("e.g. custom/my_texture");
    baseTexture_->setText(suggestedBaseTexture);
    connect(baseTexture_, &QLineEdit::textChanged, this, &CreateVmtDialog::updatePreview);
    form->addRow("$basetexture:", baseTexture_);

    bumpMap_ = new QLineEdit(this);
    bumpMap_->setPlaceholderText("e.g. custom/my_texture_normal");
    connect(bumpMap_, &QLineEdit::textChanged, this, &CreateVmtDialog::updatePreview);
    form->addRow("$bumpmap:", bumpMap_);

    baseTexture2_ = new QLineEdit(this);
    baseTexture2_->setPlaceholderText("optional");
    connect(baseTexture2_, &QLineEdit::textChanged, this, &CreateVmtDialog::updatePreview);
    form->addRow("$basetexture2:", baseTexture2_);

    bumpMap2_ = new QLineEdit(this);
    bumpMap2_->setPlaceholderText("optional");
    connect(bumpMap2_, &QLineEdit::textChanged, this, &CreateVmtDialog::updatePreview);
    form->addRow("$bumpmap2:", bumpMap2_);

    envMap_ = new QLineEdit(this);
    envMap_->setPlaceholderText("e.g. env_cubemap");
    connect(envMap_, &QLineEdit::textChanged, this, &CreateVmtDialog::updatePreview);
    form->addRow("$envmap:", envMap_);

    envMapMask_ = new QLineEdit(this);
    envMapMask_->setPlaceholderText("optional");
    connect(envMapMask_, &QLineEdit::textChanged, this, &CreateVmtDialog::updatePreview);
    form->addRow("$envmapmask:", envMapMask_);

    detailTexture_ = new QLineEdit(this);
    detailTexture_->setPlaceholderText("optional");
    connect(detailTexture_, &QLineEdit::textChanged, this, &CreateVmtDialog::updatePreview);
    form->addRow("$detail:", detailTexture_);

    toolTexture_ = new QLineEdit(this);
    toolTexture_->setPlaceholderText("optional");
    connect(toolTexture_, &QLineEdit::textChanged, this, &CreateVmtDialog::updatePreview);
    form->addRow("%tooltexture:", toolTexture_);

    normalMap_ = new QLineEdit(this);
    normalMap_->setPlaceholderText("optional");
    connect(normalMap_, &QLineEdit::textChanged, this, &CreateVmtDialog::updatePreview);
    form->addRow("$normalmap:", normalMap_);

    duDvMap_ = new QLineEdit(this);
    duDvMap_->setPlaceholderText("optional");
    connect(duDvMap_, &QLineEdit::textChanged, this, &CreateVmtDialog::updatePreview);
    form->addRow("$dudvmap:", duDvMap_);

    surfaceProp1_ = new QLineEdit(this);
    surfaceProp1_->setPlaceholderText("optional");
    connect(surfaceProp1_, &QLineEdit::textChanged, this, &CreateVmtDialog::updatePreview);
    form->addRow("$surfaceprop:", surfaceProp1_);

    surfaceProp2_ = new QLineEdit(this);
    surfaceProp2_->setPlaceholderText("optional");
    connect(surfaceProp2_, &QLineEdit::textChanged, this, &CreateVmtDialog::updatePreview);
    form->addRow("$surfaceprop2:", surfaceProp2_);

    keywords_ = new QLineEdit(this);
    keywords_->setPlaceholderText("optional");
    connect(keywords_, &QLineEdit::textChanged, this, &CreateVmtDialog::updatePreview);
    form->addRow("%keywords:", keywords_);

    translucent_ = new QCheckBox(this);
    connect(translucent_, &QCheckBox::toggled, this, &CreateVmtDialog::updatePreview);
    form->addRow("$translucent:", translucent_);

    alphaTest_ = new QCheckBox(this);
    connect(alphaTest_, &QCheckBox::toggled, this, &CreateVmtDialog::updatePreview);
    form->addRow("$alphatest:", alphaTest_);

    additive_ = new QCheckBox(this);
    connect(additive_, &QCheckBox::toggled, this, &CreateVmtDialog::updatePreview);
    form->addRow("$additive:", additive_);

    noCull_ = new QCheckBox(this);
    connect(noCull_, &QCheckBox::toggled, this, &CreateVmtDialog::updatePreview);
    form->addRow("$nocull:", noCull_);

    noDecal_ = new QCheckBox(this);
    connect(noDecal_, &QCheckBox::toggled, this, &CreateVmtDialog::updatePreview);
    form->addRow("$nodecal:", noDecal_);

    noLod_ = new QCheckBox(this);
    connect(noLod_, &QCheckBox::toggled, this, &CreateVmtDialog::updatePreview);
    form->addRow("$nolod:", noLod_);

    vertexColor_ = new QCheckBox(this);
    connect(vertexColor_, &QCheckBox::toggled, this, &CreateVmtDialog::updatePreview);
    form->addRow("$vertexcolor:", vertexColor_);

    vertexAlpha_ = new QCheckBox(this);
    connect(vertexAlpha_, &QCheckBox::toggled, this, &CreateVmtDialog::updatePreview);
    form->addRow("$vertexalpha:", vertexAlpha_);

    model_ = new QCheckBox(this);
    connect(model_, &QCheckBox::toggled, this, &CreateVmtDialog::updatePreview);
    form->addRow("$model:", model_);

    decal_ = new QCheckBox(this);
    connect(decal_, &QCheckBox::toggled, this, &CreateVmtDialog::updatePreview);
    form->addRow("$decal:", decal_);

    noFog_ = new QCheckBox(this);
    connect(noFog_, &QCheckBox::toggled, this, &CreateVmtDialog::updatePreview);
    form->addRow("$nofog:", noFog_);

    ignoreZ_ = new QCheckBox(this);
    connect(ignoreZ_, &QCheckBox::toggled, this, &CreateVmtDialog::updatePreview);
    form->addRow("$ignorez:", ignoreZ_);

    halfLambert_ = new QCheckBox(this);
    connect(halfLambert_, &QCheckBox::toggled, this, &CreateVmtDialog::updatePreview);
    form->addRow("$halflambert:", halfLambert_);

    alphaCoverage_ = new QCheckBox(this);
    connect(alphaCoverage_, &QCheckBox::toggled, this, &CreateVmtDialog::updatePreview);
    form->addRow("$allowalphatocoverage:", alphaCoverage_);

    selfIllum_ = new QCheckBox(this);
    connect(selfIllum_, &QCheckBox::toggled, this, &CreateVmtDialog::updatePreview);
    form->addRow("$selfillum:", selfIllum_);

    root->addLayout(form);

    preview_ = new QPlainTextEdit(this);
    preview_->setReadOnly(true);
    preview_->setLineWrapMode(QPlainTextEdit::NoWrap);
    root->addWidget(preview_, 1);

    buttons_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons_, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons_);

    alphaTest_->setChecked(suggestAlphaTest);
    translucent_->setChecked(suggestTranslucent);

    updatePreview();
}

CreateVmtDialog::~CreateVmtDialog() = default;

QString CreateVmtDialog::buildVmt() const {
    QString shader = shader_->currentText().trimmed();
    if(shader == "Custom…") shader = "VertexLitGeneric";

    QStringList lines;
    lines << quoted(shader);
    lines << "{";

    const QString base = baseTexture_->text().trimmed();
    if(!base.isEmpty()) lines << QString("\t%1 %2").arg(quoted("$basetexture"), quoted(base));

    const QString bump = bumpMap_->text().trimmed();
    if(!bump.isEmpty()) lines << QString("\t%1 %2").arg(quoted("$bumpmap"), quoted(bump));

    const QString base2 = baseTexture2_->text().trimmed();
    if(!base2.isEmpty()) lines << QString("\t%1 %2").arg(quoted("$basetexture2"), quoted(base2));

    const QString bump2 = bumpMap2_->text().trimmed();
    if(!bump2.isEmpty()) lines << QString("\t%1 %2").arg(quoted("$bumpmap2"), quoted(bump2));

    const QString env = envMap_->text().trimmed();
    if(!env.isEmpty()) lines << QString("\t%1 %2").arg(quoted("$envmap"), quoted(env));

    const QString envMask = envMapMask_->text().trimmed();
    if(!envMask.isEmpty()) lines << QString("\t%1 %2").arg(quoted("$envmapmask"), quoted(envMask));

    const QString detail = detailTexture_->text().trimmed();
    if(!detail.isEmpty()) lines << QString("\t%1 %2").arg(quoted("$detail"), quoted(detail));

    const QString tool = toolTexture_->text().trimmed();
    if(!tool.isEmpty()) lines << QString("\t%1 %2").arg(quoted("%tooltexture"), quoted(tool));

    const QString normal = normalMap_->text().trimmed();
    if(!normal.isEmpty()) lines << QString("\t%1 %2").arg(quoted("$normalmap"), quoted(normal));

    const QString dudv = duDvMap_->text().trimmed();
    if(!dudv.isEmpty()) lines << QString("\t%1 %2").arg(quoted("$dudvmap"), quoted(dudv));

    const QString surf1 = surfaceProp1_->text().trimmed();
    if(!surf1.isEmpty()) lines << QString("\t%1 %2").arg(quoted("$surfaceprop"), quoted(surf1));

    const QString surf2 = surfaceProp2_->text().trimmed();
    if(!surf2.isEmpty()) lines << QString("\t%1 %2").arg(quoted("$surfaceprop2"), quoted(surf2));

    const QString kw = keywords_->text().trimmed();
    if(!kw.isEmpty()) lines << QString("\t%1 %2").arg(quoted("%keywords"), quoted(kw));

    auto addBool = [&](const QString &k, bool v) {
        if(!v) return;
        lines << QString("\t%1 %2").arg(quoted(k), quoted("1"));
    };

    addBool("$additive", additive_->isChecked());
    addBool("$alphatest", alphaTest_->isChecked());
    addBool("$nocull", noCull_->isChecked());
    addBool("$nodecal", noDecal_->isChecked());
    addBool("$nolod", noLod_->isChecked());
    addBool("$translucent", translucent_->isChecked());
    addBool("$vertexalpha", vertexAlpha_->isChecked());
    addBool("$vertexcolor", vertexColor_->isChecked());
    addBool("$model", model_->isChecked());
    addBool("$decal", decal_->isChecked());
    addBool("$nofog", noFog_->isChecked());
    addBool("$ignorez", ignoreZ_->isChecked());
    addBool("$halflambert", halfLambert_->isChecked());
    addBool("$allowalphatocoverage", alphaCoverage_->isChecked());
    addBool("$selfillum", selfIllum_->isChecked());

    lines << "}";
    lines << "";
    return lines.join("\n");
}

void CreateVmtDialog::updatePreview() {
    preview_->setPlainText(buildVmt());
}

QString CreateVmtDialog::vmtText() const {
    return buildVmt();
}
