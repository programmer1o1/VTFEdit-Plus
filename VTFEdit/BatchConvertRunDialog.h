#pragma once

#include <QDialog>

#include <VTFLib.h>

class QDialogButtonBox;
class QLabel;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QTableWidget;
class QTimer;

class BatchConvertRunDialog final : public QDialog {
    Q_OBJECT

public:
    struct Job {
        QString inputRoot;
        QString outputRoot;
        QString materialsRoot;
        bool overwrite = false;
        bool createVmts = false;
        QString vmtShader;
        QString vmtPrefix;
        SVTFCreateOptions createOptions{};
        bool useAlphaFormat = false;
        VTFImageFormat alphaFormat = IMAGE_FORMAT_DXT5;
        bool createLodResource = false;
        int lodClampU = 0;
        int lodClampV = 0;
        bool createInformationResource = false;
        QString infoAuthor;
        QString infoContact;
        QString infoVersion;
        QString infoModification;
        QString infoDescription;
        QString infoComments;
        QList<QString> inputFiles;
    };

    explicit BatchConvertRunDialog(Job job, QWidget *parent = nullptr);
    ~BatchConvertRunDialog() override;

private slots:
    void processNext();
    void cancel();

private:
    void logLine(const QString &line);
    void setRowStatus(int row, const QString &status, const QString &message = {});
    QString computeOutFolder(const QString &inputPath) const;
    QString computeOutVtfPath(const QString &inputPath) const;
    QString computeBaseTextureFromOutVtf(const QString &outVtf, const QString &fallbackRelNoExt) const;

    Job job_;
    int index_ = 0;
    int ok_ = 0;
    int failed_ = 0;
    int skipped_ = 0;
    bool canceled_ = false;

    QTableWidget *table_ = nullptr;
    QProgressBar *progress_ = nullptr;
    QLabel *summary_ = nullptr;
    QPlainTextEdit *log_ = nullptr;
    QPushButton *cancelButton_ = nullptr;
    QPushButton *closeButton_ = nullptr;
    QDialogButtonBox *buttons_ = nullptr;
    QTimer *tick_ = nullptr;
};
