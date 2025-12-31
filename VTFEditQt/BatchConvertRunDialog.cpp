#include "BatchConvertRunDialog.h"

#include "QtUtil.h"

#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QAbstractItemView>
#include <QGridLayout>
#include <QImageReader>
#include <QHeaderView>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>
#include <QTimer>

#include <algorithm>
#include <cstring>
#include <utility>
 
namespace {

bool imageHasTransparencyRgba8888(const QImage &rgba8888) {
    if(rgba8888.isNull()) return false;
    if(rgba8888.format() != QImage::Format_RGBA8888) return false;
    for(int y = 0; y < rgba8888.height(); ++y) {
        const uchar *row = rgba8888.constScanLine(y);
        const int w = rgba8888.width();
        for(int x = 0; x < w; ++x) {
            const uchar a = row[x * 4 + 3];
            if(a != 255) return true;
        }
    }
    return false;
}

QString quoted(QString s) {
    s.replace('\\', "\\\\");
    s.replace('"', "\\\"");
    s.replace('\r', "");
    s.replace('\n', "\\n");
    return QString("\"%1\"").arg(s);
}

QString buildInformationKvd(const BatchConvertRunDialog::Job &job) {
    QStringList lines;
    lines << quoted("Information");
    lines << "{";
    auto add = [&](const QString &k, const QString &v) {
        const QString t = v.trimmed();
        if(t.isEmpty()) return;
        lines << QString("\t%1 %2").arg(quoted(k), quoted(t));
    };
    add("Author", job.infoAuthor);
    add("Contact", job.infoContact);
    add("Version", job.infoVersion);
    add("Modification", job.infoModification);
    add("Description", job.infoDescription);
    add("Comments", job.infoComments);
    lines << "}";
    lines << "";
    return lines.join('\n');
}

bool applyRequestedResourcesToVtf(const BatchConvertRunDialog::Job &job, const QString &outVtf, QString *error) {
    if(!job.createLodResource && !job.createInformationResource) return true;

    vlUInt id = 0;
    if(!vlCreateImage(&id) || !vlBindImage(id)) {
        if(error) *error = QString::fromUtf8(vlGetLastError());
        if(id) vlDeleteImage(id);
        return false;
    }

    const QByteArray encoded = QFile::encodeName(outVtf);
    if(!vlImageLoad(encoded.constData(), vlFalse)) {
        if(error) *error = QString::fromUtf8(vlGetLastError());
        vlDeleteImage(id);
        return false;
    }

    if(job.createLodResource) {
        SVTFTextureLODControlResource lod{};
        lod.ResolutionClampU = static_cast<vlByte>(std::clamp(job.lodClampU, 0, 255));
        lod.ResolutionClampV = static_cast<vlByte>(std::clamp(job.lodClampV, 0, 255));
        vlUInt packed = 0;
        std::memcpy(&packed, &lod, sizeof(lod));
        if(!vlImageSetResourceData(VTF_RSRC_TEXTURE_LOD_SETTINGS, sizeof(vlUInt), &packed)) {
            if(error) *error = QString::fromUtf8(vlGetLastError());
            vlDeleteImage(id);
            return false;
        }
    }

    if(job.createInformationResource) {
        QByteArray bytes = buildInformationKvd(job).toUtf8();
        bytes.push_back('\0');
        if(!vlImageSetResourceData(VTF_RSRC_KEY_VALUE_DATA, static_cast<vlUInt>(bytes.size()), bytes.data())) {
            if(error) *error = QString::fromUtf8(vlGetLastError());
            vlDeleteImage(id);
            return false;
        }
    }

    if(!vlImageSave(encoded.constData())) {
        if(error) *error = QString::fromUtf8(vlGetLastError());
        vlDeleteImage(id);
        return false;
    }

    vlDeleteImage(id);
    return true;
}

} // namespace


BatchConvertRunDialog::BatchConvertRunDialog(Job job, QWidget *parent) : QDialog(parent), job_(std::move(job)) {
    setWindowTitle("Batch Convert");
    setModal(true);
    resize(980, 620);

    auto *layout = new QGridLayout(this);

    table_ = new QTableWidget(this);
    table_->setColumnCount(3);
    table_->setHorizontalHeaderLabels({"File", "Status", "Message"});
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setRowCount(job_.inputFiles.size());
    for(int i = 0; i < job_.inputFiles.size(); ++i) {
        const QString path = job_.inputFiles.at(i);
        const QString rel = QDir(job_.inputRoot).relativeFilePath(path).replace('\\', '/');
        table_->setItem(i, 0, new QTableWidgetItem(rel));
        table_->setItem(i, 1, new QTableWidgetItem("Queued"));
        table_->setItem(i, 2, new QTableWidgetItem(""));
    }
    layout->addWidget(table_, 0, 0, 1, 2);

    progress_ = new QProgressBar(this);
    progress_->setRange(0, job_.inputFiles.size());
    progress_->setValue(0);
    layout->addWidget(progress_, 1, 0, 1, 2);

    summary_ = new QLabel(this);
    summary_->setText("Ready");
    layout->addWidget(summary_, 2, 0, 1, 2);

    log_ = new QPlainTextEdit(this);
    log_->setReadOnly(true);
    log_->setMaximumBlockCount(2000);
    layout->addWidget(log_, 3, 0, 1, 2);

    buttons_ = new QDialogButtonBox(this);
    cancelButton_ = buttons_->addButton("Cancel", QDialogButtonBox::RejectRole);
    closeButton_ = buttons_->addButton("Close", QDialogButtonBox::AcceptRole);
    closeButton_->setEnabled(false);
    connect(cancelButton_, &QPushButton::clicked, this, &BatchConvertRunDialog::cancel);
    connect(closeButton_, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(buttons_, 4, 0, 1, 2);

    tick_ = new QTimer(this);
    tick_->setSingleShot(true);
    connect(tick_, &QTimer::timeout, this, &BatchConvertRunDialog::processNext);

    if(!vtflibCanEncode(job_.createOptions.ImageFormat) || (job_.useAlphaFormat && !vtflibCanEncode(job_.alphaFormat))) {
        logLine("Warning: selected output format cannot be encoded by this build of VTFLib (DXT compression unavailable).");
        logLine("Choose an uncompressed format (RGBA8888/BGRA8888) or install Compressonator and rebuild.");
        for(int i = 0; i < table_->rowCount(); ++i) {
            setRowStatus(i, "Failed", "DXT encoding unavailable");
        }
        cancelButton_->setEnabled(false);
        closeButton_->setEnabled(true);
        summary_->setText("Cannot start: DXT encoding unavailable.");
        return;
    }

    if(job_.createOptions.bThumbnail && !vtflibCanEncode(IMAGE_FORMAT_DXT1)) {
        job_.createOptions.bThumbnail = vlFalse;
        logLine("Note: thumbnail generation disabled (DXT1 encoding unavailable without Compressonator).");
    }

    tick_->start(0);
}

BatchConvertRunDialog::~BatchConvertRunDialog() = default;

void BatchConvertRunDialog::cancel() {
    canceled_ = true;
    cancelButton_->setEnabled(false);
    logLine("Cancel requested…");
}

void BatchConvertRunDialog::logLine(const QString &line) {
    log_->appendPlainText(line);
}

void BatchConvertRunDialog::setRowStatus(int row, const QString &status, const QString &message) {
    if(row < 0 || row >= table_->rowCount()) return;
    table_->item(row, 1)->setText(status);
    table_->item(row, 2)->setText(message);
}

QString BatchConvertRunDialog::computeOutFolder(const QString &inputPath) const {
    const QDir inDir(job_.inputRoot);
    const QString rel = inDir.relativeFilePath(inputPath);
    const QString relDir = QFileInfo(rel).path();
    QString outFolder = QDir(job_.outputRoot).absolutePath();
    if(relDir != "." && !relDir.isEmpty()) {
        outFolder = QDir(outFolder).filePath(relDir);
    }
    return QDir::cleanPath(outFolder);
}

QString BatchConvertRunDialog::computeOutVtfPath(const QString &inputPath) const {
    const QDir inDir(job_.inputRoot);
    const QString rel = inDir.relativeFilePath(inputPath);
    const QString base = QFileInfo(rel).completeBaseName();
    const QString outFolder = computeOutFolder(inputPath);
    return QDir(outFolder).absoluteFilePath(base + ".vtf");
}

QString BatchConvertRunDialog::computeBaseTextureFromOutVtf(const QString &outVtf, const QString &fallbackRelNoExt) const {
    const QDir materialsDir(job_.materialsRoot);
    QString relToMaterials = materialsDir.relativeFilePath(outVtf);
    QString baseTexture;
    if(relToMaterials.startsWith("..")) {
        baseTexture = fallbackRelNoExt;
    } else {
        relToMaterials.replace('\\', '/');
        const QFileInfo relInfo(relToMaterials);
        const QString relFolder = (relInfo.path() == "." || relInfo.path().isEmpty()) ? QString{} : (relInfo.path() + "/");
        baseTexture = relFolder + relInfo.completeBaseName();
    }
    baseTexture.replace('\\', '/');

    QString prefix = job_.vmtPrefix;
    prefix.replace('\\', '/');
    if(prefix.endsWith('/')) prefix.chop(1);
    if(!prefix.isEmpty()) baseTexture = prefix + "/" + baseTexture;
    return baseTexture;
}

void BatchConvertRunDialog::processNext() {
    if(index_ >= job_.inputFiles.size()) {
        summary_->setText(QString("Done. OK: %1  Skipped: %2  Failed: %3").arg(ok_).arg(skipped_).arg(failed_));
        cancelButton_->setEnabled(false);
        closeButton_->setEnabled(true);
        return;
    }

    const int row = index_;
    const QString inPath = job_.inputFiles.at(index_);
    const QDir inDir(job_.inputRoot);
    const QString rel = inDir.relativeFilePath(inPath);
    const QString relDir = QFileInfo(rel).path();
    const QString base = QFileInfo(rel).completeBaseName();

    const QString outFolder = computeOutFolder(inPath);
    QDir().mkpath(outFolder);
    const QString outVtf = QDir(outFolder).absoluteFilePath(base + ".vtf");

    if(QFileInfo::exists(outVtf) && !job_.overwrite) {
        ++skipped_;
        setRowStatus(row, "Skipped", "Exists");
        ++index_;
        progress_->setValue(index_);
        tick_->start(0);
        return;
    }

    setRowStatus(row, "Running");
    summary_->setText(QString("Converting %1 (%2/%3)…").arg(QFileInfo(inPath).fileName()).arg(index_ + 1).arg(job_.inputFiles.size()));

    QImageReader reader(inPath);
    reader.setAutoTransform(true);
    QImage image = reader.read();
    if(image.isNull()) {
        ++failed_;
        setRowStatus(row, "Failed", reader.errorString());
        logLine(QString("Failed to read %1: %2").arg(rel, reader.errorString()));
        ++index_;
        progress_->setValue(index_);
        tick_->start(0);
        return;
    }
    image = ensureRgba8888(image);

    QString error;
    SVTFCreateOptions opts = job_.createOptions;
    if(job_.useAlphaFormat && imageHasTransparencyRgba8888(image)) {
        opts.ImageFormat = job_.alphaFormat;
    }
    if(!createVtfFromRgbaAndSave(image, opts, outVtf, &error)) {
        ++failed_;
        setRowStatus(row, "Failed", error);
        logLine(QString("Failed to write %1: %2").arg(QDir(job_.outputRoot).relativeFilePath(outVtf), error));
        ++index_;
        progress_->setValue(index_);
        tick_->start(0);
        return;
    }

    if(job_.createLodResource || job_.createInformationResource) {
        QString resErr;
        if(!applyRequestedResourcesToVtf(job_, outVtf, &resErr)) {
            ++failed_;
            setRowStatus(row, "Failed", resErr);
            logLine(QString("Failed to apply resources to %1: %2").arg(QDir(job_.outputRoot).relativeFilePath(outVtf), resErr));
            ++index_;
            progress_->setValue(index_);
            tick_->start(0);
            return;
        }
    }

    if(job_.createVmts) {
        const QString fallbackRelNoExt = (relDir == "." || relDir.isEmpty()) ? base : (QString("%1/%2").arg(relDir, base));
        const QString baseTexture = computeBaseTextureFromOutVtf(outVtf, fallbackRelNoExt);
        const QString shader = job_.vmtShader.isEmpty() ? "VertexLitGeneric" : job_.vmtShader;
        const QString vmt = QString("\"%1\"\n{\n\t\"$basetexture\" \"%2\"\n}\n").arg(shader, baseTexture);
        const QString outVmt = QDir(outFolder).absoluteFilePath(base + ".vmt");
        QString writeErr;
        if(!writeTextFileUtf8(outVmt, vmt, &writeErr)) {
            logLine(QString("Warning: failed to write VMT %1: %2").arg(QDir(job_.outputRoot).relativeFilePath(outVmt), writeErr));
        }
    }

    ++ok_;
    setRowStatus(row, "OK", QDir(job_.outputRoot).relativeFilePath(outVtf));
    logLine(QString("OK: %1 → %2").arg(rel, QDir(job_.outputRoot).relativeFilePath(outVtf)));

    ++index_;
    progress_->setValue(index_);

    if(canceled_) {
        for(int i = index_; i < job_.inputFiles.size(); ++i) {
            setRowStatus(i, "Canceled");
        }
        summary_->setText(QString("Canceled. OK: %1  Skipped: %2  Failed: %3").arg(ok_).arg(skipped_).arg(failed_));
        cancelButton_->setEnabled(false);
        closeButton_->setEnabled(true);
        return;
    }

    tick_->start(0);
}
