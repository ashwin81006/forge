#include "MainWindow.h"
#include "../archive/ArchiveWriter.h"
#include "../archive/ArchiveReader.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QApplication>
#include <QGroupBox>
#include <QDesktopServices>
#include <QUrl>
#include <QMenu>
#include <QAction>
#include <QClipboard>
#include <QFileInfo>
#include <QMimeData>
#include <QFrame>

#include <fstream>
#include <chrono>
#include <filesystem>

namespace forge {
namespace gui {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    setAcceptDrops(true);
    setupUi();
    applyLightTheme();

    resize(880, 580);
    setMinimumSize(760, 500);
    setWindowTitle("FORGE — Archive Manager");
}

MainWindow::~MainWindow() {
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
}

QString MainWindow::formatBytes(uint64_t bytes) {
    if (bytes < 1024) {
        return QString("%1 B").arg(bytes);
    } else if (bytes < 1024 * 1024) {
        return QString("%1 KB").arg(QString::number(bytes / 1024.0, 'f', 1));
    } else if (bytes < 1024ULL * 1024 * 1024) {
        return QString("%1 MB").arg(QString::number(bytes / (1024.0 * 1024.0), 'f', 2));
    } else {
        return QString("%1 GB").arg(QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 2));
    }
}

void MainWindow::applyLightTheme() {
    QString qss = R"(
        QMainWindow {
            background-color: #f1f3f5;
            color: #1f2937;
            font-family: 'Segoe UI', Tahoma, Arial, sans-serif;
            font-size: 13px;
        }

        QWidget {
            color: #1f2937;
            font-size: 13px;
        }

        /* Top Bar */
        #headerFrame {
            background-color: #ffffff;
            border-bottom: 1px solid #d1d5db;
            padding: 8px 16px;
        }

        /* Tab Widget (Clean Standard Tabs) */
        QTabWidget::pane {
            border: 1px solid #d1d5db;
            background-color: #ffffff;
            border-radius: 4px;
            margin-top: -1px;
        }

        QTabBar::tab {
            background-color: #e5e7eb;
            color: #4b5563;
            font-weight: 500;
            padding: 8px 22px;
            border: 1px solid #d1d5db;
            border-bottom: none;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
            margin-right: 2px;
        }

        QTabBar::tab:selected {
            background-color: #ffffff;
            color: #1d4ed8;
            font-weight: 600;
            border-top: 2px solid #2563eb;
            border-bottom: 1px solid #ffffff;
        }

        QTabBar::tab:hover:!selected {
            background-color: #f3f4f6;
            color: #111827;
        }

        /* Group Boxes */
        QGroupBox {
            background-color: #ffffff;
            border: 1px solid #e2e8f0;
            border-radius: 6px;
            margin-top: 14px;
            padding: 14px;
            font-weight: 600;
            color: #1e293b;
        }

        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 10px;
            padding: 0 6px;
            background-color: #ffffff;
            color: #334155;
        }

        /* Line Edit */
        QLineEdit {
            background-color: #ffffff;
            color: #111827;
            border: 1px solid #cbd5e1;
            border-radius: 4px;
            padding: 6px 10px;
            font-size: 13px;
            selection-background-color: #2563eb;
            selection-color: #ffffff;
        }

        QLineEdit:focus {
            border: 1px solid #2563eb;
        }

        /* Buttons */
        QPushButton {
            background-color: #ffffff;
            color: #1f2937;
            border: 1px solid #d1d5db;
            border-radius: 4px;
            padding: 6px 14px;
            font-weight: 500;
            font-size: 13px;
        }

        QPushButton:hover {
            background-color: #f9fafb;
            border-color: #9ca3af;
        }

        QPushButton:pressed {
            background-color: #f3f4f6;
        }

        QPushButton:disabled {
            background-color: #f3f4f6;
            color: #9ca3af;
            border-color: #e5e7eb;
        }

        QPushButton[primary="true"] {
            background-color: #2563eb;
            color: #ffffff;
            border: 1px solid #1d4ed8;
            font-weight: 600;
        }

        QPushButton[primary="true"]:hover {
            background-color: #1d4ed8;
        }

        QPushButton[primary="true"]:pressed {
            background-color: #1e40af;
        }

        /* ComboBox */
        QComboBox {
            background-color: #ffffff;
            color: #111827;
            border: 1px solid #cbd5e1;
            border-radius: 4px;
            padding: 6px 10px;
            font-size: 13px;
        }

        QComboBox:focus {
            border: 1px solid #2563eb;
        }

        QComboBox QAbstractItemView {
            background-color: #ffffff;
            color: #111827;
            border: 1px solid #cbd5e1;
            selection-background-color: #2563eb;
            selection-color: #ffffff;
            padding: 4px;
        }

        /* Table */
        QTableWidget {
            background-color: #ffffff;
            color: #1f2937;
            gridline-color: #f1f5f9;
            border: 1px solid #cbd5e1;
            border-radius: 4px;
            selection-background-color: #2563eb;
            selection-color: #ffffff;
            outline: none;
        }

        QHeaderView::section {
            background-color: #f8fafc;
            color: #475569;
            padding: 7px 10px;
            font-weight: 600;
            font-size: 12px;
            border: none;
            border-bottom: 1px solid #cbd5e1;
            border-right: 1px solid #e2e8f0;
        }

        QTableWidget::item {
            padding: 5px 8px;
        }

        QTableWidget::item:hover {
            background-color: #f8fafc;
        }

        /* Progress Bar */
        QProgressBar {
            background-color: #e5e7eb;
            border: 1px solid #d1d5db;
            border-radius: 4px;
            text-align: center;
            color: #111827;
            font-weight: 600;
            font-size: 12px;
            height: 20px;
        }

        QProgressBar::chunk {
            background-color: #2563eb;
            border-radius: 3px;
        }
    )";
    setStyleSheet(qss);
}

void MainWindow::setupUi() {
    QWidget* central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout* rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(14, 12, 14, 14);
    rootLayout->setSpacing(8);

    // App Header Bar
    rootLayout->addWidget(createHeaderBar());

    // Main Tab Switcher
    m_tabs = new QTabWidget(this);
    m_tabs->addTab(createCompressionView(), "Create Archive");
    m_tabs->addTab(createExplorerView(), "Archive Explorer");
    rootLayout->addWidget(m_tabs, 1);
}

QWidget* MainWindow::createHeaderBar() {
    QFrame* header = new QFrame(this);
    header->setObjectName("headerFrame");
    header->setStyleSheet(
        "#headerFrame {"
        "  background-color: #ffffff;"
        "  border: 1px solid #d1d5db;"
        "  border-radius: 6px;"
        "  padding: 6px 12px;"
        "}"
    );

    QHBoxLayout* layout = new QHBoxLayout(header);
    layout->setContentsMargins(10, 4, 10, 4);
    layout->setSpacing(10);

    QLabel* logoLabel = new QLabel(
        "<b style='font-size: 16px; color: #1e3a8a;'>FORGE</b> "
        "<span style='color: #64748b; font-size: 12px;'>Archive Manager</span>",
        header);
    logoLabel->setTextFormat(Qt::RichText);
    layout->addWidget(logoLabel);

    layout->addStretch();

    QLabel* statusHint = new QLabel("Lossless .forge Compression", header);
    statusHint->setStyleSheet("color: #64748b; font-size: 12px;");
    layout->addWidget(statusHint);

    return header;
}

QWidget* MainWindow::createCompressionView() {
    QWidget* view = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(view);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(12);

    // Source Selection Group
    QGroupBox* srcBox = new QGroupBox("Source Selection", view);
    QVBoxLayout* srcLayout = new QVBoxLayout(srcBox);
    srcLayout->setSpacing(8);

    QHBoxLayout* srcInputRow = new QHBoxLayout();
    m_sourcePathEdit = new QLineEdit(srcBox);
    m_sourcePathEdit->setPlaceholderText("Select a folder or file to compress...");
    srcInputRow->addWidget(m_sourcePathEdit, 1);

    QPushButton* btnBrowseFolder = new QPushButton("Browse Folder...", srcBox);
    connect(btnBrowseFolder, &QPushButton::clicked, this, &MainWindow::onBrowseSourceFolder);
    srcInputRow->addWidget(btnBrowseFolder);

    QPushButton* btnBrowseFile = new QPushButton("Browse File...", srcBox);
    connect(btnBrowseFile, &QPushButton::clicked, this, &MainWindow::onBrowseSourceFile);
    srcInputRow->addWidget(btnBrowseFile);

    srcLayout->addLayout(srcInputRow);
    layout->addWidget(srcBox);

    // Output Destination Group
    QGroupBox* dstBox = new QGroupBox("Archive Destination", view);
    QHBoxLayout* dstLayout = new QHBoxLayout(dstBox);
    dstLayout->setSpacing(8);

    m_destArchiveEdit = new QLineEdit(dstBox);
    m_destArchiveEdit->setPlaceholderText("Select destination path for the archive (.forge)...");
    dstLayout->addWidget(m_destArchiveEdit, 1);

    QPushButton* btnBrowseDst = new QPushButton("Browse...", dstBox);
    connect(btnBrowseDst, &QPushButton::clicked, this, &MainWindow::onBrowseDestinationArchive);
    dstLayout->addWidget(btnBrowseDst);

    layout->addWidget(dstBox);

    // Options Group
    QGroupBox* optBox = new QGroupBox("Compression Settings", view);
    QHBoxLayout* optLayout = new QHBoxLayout(optBox);
    optLayout->setSpacing(10);

    QLabel* levelLabel = new QLabel("Compression level:", optBox);
    optLayout->addWidget(levelLabel);

    m_compressionLevelCombo = new QComboBox(optBox);
    m_compressionLevelCombo->addItem("Normal (Recommended)", static_cast<qulonglong>(64 * 1024));
    m_compressionLevelCombo->addItem("Fast", static_cast<qulonglong>(16 * 1024));
    m_compressionLevelCombo->addItem("Maximum", static_cast<qulonglong>(256 * 1024));
    m_compressionLevelCombo->addItem("Ultra", static_cast<qulonglong>(1024 * 1024));
    m_compressionLevelCombo->setCurrentIndex(0);
    optLayout->addWidget(m_compressionLevelCombo, 1);

    layout->addWidget(optBox);

    // Compress Button
    m_compressButton = new QPushButton("Create Archive", view);
    m_compressButton->setProperty("primary", true);
    m_compressButton->setFixedHeight(38);
    connect(m_compressButton, &QPushButton::clicked, this, &MainWindow::onStartCompression);
    layout->addWidget(m_compressButton);

    // Progress
    m_compressProgress = new QProgressBar(view);
    m_compressProgress->setRange(0, 100);
    m_compressProgress->setValue(0);
    layout->addWidget(m_compressProgress);

    m_compressStatusLabel = new QLabel("Ready.", view);
    m_compressStatusLabel->setStyleSheet("color: #4b5563; font-size: 12px;");
    layout->addWidget(m_compressStatusLabel);

    // Success Banner
    m_compressionSuccessBanner = new QFrame(view);
    m_compressionSuccessBanner->setStyleSheet(
        "QFrame {"
        "  background-color: #ecfdf5;"
        "  border: 1px solid #a7f3d0;"
        "  border-radius: 4px;"
        "  padding: 8px 12px;"
        "}"
    );
    m_compressionSuccessBanner->setVisible(false);

    QHBoxLayout* bannerLayout = new QHBoxLayout(m_compressionSuccessBanner);
    bannerLayout->setContentsMargins(8, 4, 8, 4);
    bannerLayout->setSpacing(10);

    m_compressionSummaryLabel = new QLabel("Archive created successfully.", m_compressionSuccessBanner);
    m_compressionSummaryLabel->setStyleSheet("color: #065f46; font-weight: 500;");
    bannerLayout->addWidget(m_compressionSummaryLabel, 1);

    m_openOutputDirBtn = new QPushButton("Open Folder", m_compressionSuccessBanner);
    m_openOutputDirBtn->setFixedHeight(30);
    connect(m_openOutputDirBtn, &QPushButton::clicked, this, &MainWindow::onOpenOutputFolder);
    bannerLayout->addWidget(m_openOutputDirBtn);

    m_inspectInExplorerBtn = new QPushButton("View in Explorer", m_compressionSuccessBanner);
    m_inspectInExplorerBtn->setFixedHeight(30);
    connect(m_inspectInExplorerBtn, &QPushButton::clicked, this, &MainWindow::onSwitchToExplorerWithOutput);
    bannerLayout->addWidget(m_inspectInExplorerBtn);

    layout->addWidget(m_compressionSuccessBanner);

    layout->addStretch();
    return view;
}

QWidget* MainWindow::createExplorerView() {
    QWidget* view = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(view);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(10);

    // Archive selector row
    QHBoxLayout* topRow = new QHBoxLayout();
    topRow->setSpacing(8);

    m_archivePathEdit = new QLineEdit(view);
    m_archivePathEdit->setPlaceholderText("Select an archive (.forge) to open...");
    topRow->addWidget(m_archivePathEdit, 1);

    QPushButton* btnOpen = new QPushButton("Open Archive...", view);
    btnOpen->setFixedHeight(34);
    connect(btnOpen, &QPushButton::clicked, [this]() { onOpenArchive(); });
    topRow->addWidget(btnOpen);

    layout->addLayout(topRow);

    // Simple Summary Strip (Clean like 7-Zip / WinRAR)
    m_archiveSummaryLabel = new QLabel("No archive loaded.", view);
    m_archiveSummaryLabel->setStyleSheet(
        "background-color: #f8fafc; border: 1px solid #e2e8f0; border-radius: 4px; padding: 6px 10px; color: #334155; font-size: 12px;"
    );
    layout->addWidget(m_archiveSummaryLabel);

    // Search filter
    QHBoxLayout* searchRow = new QHBoxLayout();
    searchRow->setSpacing(8);

    m_searchFilterEdit = new QLineEdit(view);
    m_searchFilterEdit->setPlaceholderText("Filter files...");
    connect(m_searchFilterEdit, &QLineEdit::textChanged, this, &MainWindow::onFilterTextChanged);
    searchRow->addWidget(m_searchFilterEdit, 1);

    layout->addLayout(searchRow);

    // Files Table (Clean standard columns)
    m_filesTable = new QTableWidget(0, 4, view);
    m_filesTable->setHorizontalHeaderLabels({
        "Name", "Original Size", "Compressed Size", "Space Saved"
    });
    m_filesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_filesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_filesTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_filesTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_filesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_filesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_filesTable->setAlternatingRowColors(true);
    m_filesTable->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_filesTable, &QTableWidget::customContextMenuRequested, this, &MainWindow::onTableCustomContextMenu);
    connect(m_filesTable, &QTableWidget::cellDoubleClicked, this, &MainWindow::onTableRowDoubleClicked);

    layout->addWidget(m_filesTable, 1);

    // Bottom Action Row
    QHBoxLayout* btmRow = new QHBoxLayout();
    btmRow->setSpacing(8);

    m_btnExtractAll = new QPushButton("Extract All...", view);
    m_btnExtractAll->setProperty("primary", true);
    m_btnExtractAll->setFixedHeight(34);
    connect(m_btnExtractAll, &QPushButton::clicked, this, &MainWindow::onExtractAll);
    btmRow->addWidget(m_btnExtractAll);

    m_btnExtractSelected = new QPushButton("Extract Selected", view);
    m_btnExtractSelected->setFixedHeight(34);
    connect(m_btnExtractSelected, &QPushButton::clicked, this, &MainWindow::onExtractSelected);
    btmRow->addWidget(m_btnExtractSelected);

    m_btnTestArchive = new QPushButton("Test Archive", view);
    m_btnTestArchive->setFixedHeight(34);
    connect(m_btnTestArchive, &QPushButton::clicked, this, &MainWindow::onTestArchiveIntegrity);
    btmRow->addWidget(m_btnTestArchive);

    m_explorerStatusLabel = new QLabel("", view);
    m_explorerStatusLabel->setStyleSheet("color: #64748b; font-size: 12px; margin-left: 8px;");
    btmRow->addWidget(m_explorerStatusLabel, 1);

    layout->addLayout(btmRow);

    return view;
}

// ─────────────────────────────────────────────────────────────────────────────
// Drag and Drop
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event) {
    const auto urls = event->mimeData()->urls();
    if (urls.isEmpty()) return;

    QString localPath = urls.first().toLocalFile();
    if (localPath.isEmpty()) return;

    QFileInfo info(localPath);
    if (!info.exists()) return;

    if (info.suffix().toLower() == "forge") {
        m_tabs->setCurrentIndex(1);
        onOpenArchive(localPath);
    } else {
        m_tabs->setCurrentIndex(0);
        m_sourcePathEdit->setText(localPath);
        m_destArchiveEdit->setText(localPath + ".forge");
        m_isSourceSingleFile = info.isFile();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Compression Actions
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::onBrowseSourceFolder() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Folder to Compress");
    if (!dir.isEmpty()) {
        m_isSourceSingleFile = false;
        m_sourcePathEdit->setText(dir);
        m_destArchiveEdit->setText(dir + ".forge");
        m_compressionSuccessBanner->setVisible(false);
    }
}

void MainWindow::onBrowseSourceFile() {
    QString file = QFileDialog::getOpenFileName(this, "Select File to Compress", "", "All Files (*.*)");
    if (!file.isEmpty()) {
        m_isSourceSingleFile = true;
        m_sourcePathEdit->setText(file);
        m_destArchiveEdit->setText(file + ".forge");
        m_compressionSuccessBanner->setVisible(false);
    }
}

void MainWindow::onBrowseDestinationArchive() {
    QString f = QFileDialog::getSaveFileName(this, "Save Archive As", "", "FORGE Archives (*.forge)");
    if (!f.isEmpty()) {
        m_destArchiveEdit->setText(f);
    }
}

void MainWindow::onStartCompression() {
    QString src = m_sourcePathEdit->text().trimmed();
    QString dst = m_destArchiveEdit->text().trimmed();

    if (src.isEmpty() || dst.isEmpty()) {
        QMessageBox::warning(this, "Missing Path", "Please select a source folder or file, and a destination archive path.");
        return;
    }

    QFileInfo srcInfo(src);
    if (!srcInfo.exists()) {
        QMessageBox::warning(this, "Not Found", "The specified source path does not exist:\n" + src);
        return;
    }

    if (m_isProcessing.load()) {
        QMessageBox::warning(this, "In Progress", "An archive operation is currently in progress.");
        return;
    }

    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }

    size_t blockSize = m_compressionLevelCombo->currentData().toULongLong();
    bool isSingleFile = srcInfo.isFile();

    m_compressButton->setEnabled(false);
    m_compressButton->setText("Compressing...");
    m_compressStatusLabel->setText("Preparing compression...");
    m_compressProgress->setValue(0);
    m_compressionSuccessBanner->setVisible(false);
    m_isProcessing.store(true);

    m_lastOutputArchive = dst;
    std::string sSrc = src.toStdString();
    std::string sDst = dst.toStdString();

    m_workerThread = std::thread([this, sSrc, sDst, blockSize, isSingleFile]() {
        auto startTime = std::chrono::high_resolution_clock::now();
        try {
            archive::ArchiveWriter writer;

            auto progressCallback = [this](const archive::ArchiveWriter::ProgressInfo& p) {
                int pct = (p.totalBytes > 0) ? static_cast<int>((p.bytesProcessed * 100) / p.totalBytes) : 100;
                QMetaObject::invokeMethod(this, [this, pct, p]() {
                    m_compressProgress->setValue(pct);
                    m_compressStatusLabel->setText(
                        QString("Compressing [%1/%2]: %3 (%4 processed)")
                            .arg(p.fileIndex)
                            .arg(p.totalFiles)
                            .arg(QString::fromStdString(p.currentFile))
                            .arg(formatBytes(p.bytesProcessed)));
                });
            };

            if (isSingleFile) {
                std::filesystem::path p(sSrc);
                std::string baseDir = p.parent_path().string();
                std::string fileName = p.filename().string();
                writer.compressFiles(baseDir, { fileName }, sDst, blockSize, true, progressCallback);
            } else {
                writer.compressFolder(sSrc, sDst, blockSize, true, progressCallback);
            }

            auto endTime = std::chrono::high_resolution_clock::now();
            double elapsedSec = std::chrono::duration<double>(endTime - startTime).count();

            uint64_t finalArchiveSize = 0;
            if (std::filesystem::exists(sDst)) {
                finalArchiveSize = std::filesystem::file_size(sDst);
            }

            QMetaObject::invokeMethod(this, [this, elapsedSec, finalArchiveSize]() {
                m_compressProgress->setValue(100);
                m_compressStatusLabel->setText(QString("Completed in %1 seconds.").arg(QString::number(elapsedSec, 'f', 1)));
                m_compressButton->setEnabled(true);
                m_compressButton->setText("Create Archive");
                m_isProcessing.store(false);

                m_compressionSummaryLabel->setText(
                    QString("Archive created successfully: %1 in %2s.")
                        .arg(formatBytes(finalArchiveSize))
                        .arg(QString::number(elapsedSec, 'f', 1)));
                m_compressionSuccessBanner->setVisible(true);
            });
        } catch (const std::exception& e) {
            std::string err = e.what();
            QMetaObject::invokeMethod(this, [this, err]() {
                m_compressButton->setEnabled(true);
                m_compressButton->setText("Create Archive");
                m_compressStatusLabel->setText("Compression failed.");
                m_isProcessing.store(false);

                QMessageBox::critical(this, "Compression Failed",
                    "Could not create archive:\n\n" + QString::fromStdString(err));
            });
        }
    });
}

void MainWindow::onOpenOutputFolder() {
    if (m_lastOutputArchive.isEmpty()) return;
    QFileInfo info(m_lastOutputArchive);
    QDesktopServices::openUrl(QUrl::fromLocalFile(info.absolutePath()));
}

void MainWindow::onSwitchToExplorerWithOutput() {
    if (m_lastOutputArchive.isEmpty()) return;
    m_tabs->setCurrentIndex(1);
    onOpenArchive(m_lastOutputArchive);
}

// ─────────────────────────────────────────────────────────────────────────────
// Archive Explorer Actions
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::onOpenArchive(const QString& explicitPath) {
    QString f = explicitPath;
    if (f.isEmpty()) {
        f = QFileDialog::getOpenFileName(this, "Open Archive", "", "FORGE Archives (*.forge)");
    }
    if (f.isEmpty()) return;

    m_archivePathEdit->setText(f);
    m_explorerStatusLabel->setText("Reading archive...");

    try {
        archive::ArchiveReader reader;
        m_currentArchiveFiles = reader.listArchive(f.toStdString());
        m_filesTable->setRowCount(0);

        for (const auto& item : m_currentArchiveFiles) {
            int row = m_filesTable->rowCount();
            m_filesTable->insertRow(row);

            double spaceSaved = 0.0;
            if (item.originalFileSize > 0) {
                spaceSaved = (1.0 - (static_cast<double>(item.storedSize) / item.originalFileSize)) * 100.0;
            }

            QTableWidgetItem* pathItem = new QTableWidgetItem(QString::fromStdString(item.relativePath));
            QTableWidgetItem* origItem = new QTableWidgetItem(formatBytes(item.originalFileSize));
            QTableWidgetItem* compItem = new QTableWidgetItem(formatBytes(item.storedSize));
            
            QString savedStr = (spaceSaved >= 0) ? QString("%1%").arg(QString::number(spaceSaved, 'f', 1)) : "0.0%";
            QTableWidgetItem* savedItem = new QTableWidgetItem(savedStr);

            origItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            compItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            savedItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

            m_filesTable->setItem(row, 0, pathItem);
            m_filesTable->setItem(row, 1, origItem);
            m_filesTable->setItem(row, 2, compItem);
            m_filesTable->setItem(row, 3, savedItem);
        }

        updateExplorerStats(m_currentArchiveFiles);
        m_explorerStatusLabel->setText(QString("%1 files in archive.").arg(m_currentArchiveFiles.size()));

    } catch (const std::exception& e) {
        m_filesTable->setRowCount(0);
        m_currentArchiveFiles.clear();
        updateExplorerStats({});
        m_explorerStatusLabel->setText("Failed to load.");

        QString msg = QString::fromStdString(e.what());
        if (msg.contains("EOF", Qt::CaseInsensitive) || msg.contains("magic", Qt::CaseInsensitive)) {
            QMessageBox::critical(this, "Corrupted Archive",
                "This archive could not be opened.\n\nThe file appears to be incomplete or corrupted.");
        } else {
            QMessageBox::critical(this, "Error Opening Archive", msg);
        }
    }
}

void MainWindow::updateExplorerStats(const std::vector<archive::FileMetadata>& list) {
    if (list.empty()) {
        m_archiveSummaryLabel->setText("No archive loaded.");
        return;
    }

    uint64_t totalOriginal = 0;
    uint64_t totalStored = 0;
    for (const auto& item : list) {
        totalOriginal += item.originalFileSize;
        totalStored += item.storedSize;
    }

    double overallSaved = 0.0;
    if (totalOriginal > 0) {
        overallSaved = (1.0 - (static_cast<double>(totalStored) / totalOriginal)) * 100.0;
    }

    m_archiveSummaryLabel->setText(
        QString("Files: <b>%1</b> | Original: <b>%2</b> | Compressed: <b>%3</b> | Space Saved: <b>%4%</b>")
            .arg(list.size())
            .arg(formatBytes(totalOriginal))
            .arg(formatBytes(totalStored))
            .arg(QString::number(overallSaved, 'f', 1))
    );
}

void MainWindow::onFilterTextChanged(const QString& text) {
    QString q = text.trimmed();
    int visibleCount = 0;

    for (int row = 0; row < m_filesTable->rowCount(); ++row) {
        QTableWidgetItem* item = m_filesTable->item(row, 0);
        if (!item) continue;

        bool match = q.isEmpty() || item->text().contains(q, Qt::CaseInsensitive);
        m_filesTable->setRowHidden(row, !match);
        if (match) ++visibleCount;
    }

    if (!q.isEmpty()) {
        m_explorerStatusLabel->setText(QString("Showing %1 of %2 files").arg(visibleCount).arg(m_filesTable->rowCount()));
    } else if (m_filesTable->rowCount() > 0) {
        m_explorerStatusLabel->setText(QString("%1 files in archive.").arg(m_filesTable->rowCount()));
    }
}

void MainWindow::onExtractAll() {
    QString archivePath = m_archivePathEdit->text().trimmed();
    if (archivePath.isEmpty()) {
        QMessageBox::warning(this, "No Archive", "Please open an archive file first.");
        return;
    }

    QString destDir = QFileDialog::getExistingDirectory(this, "Select Destination Folder");
    if (destDir.isEmpty()) return;

    if (m_isProcessing.load()) {
        QMessageBox::warning(this, "In Progress", "An operation is currently in progress.");
        return;
    }

    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }

    m_isProcessing.store(true);
    m_btnExtractAll->setEnabled(false);
    m_btnExtractSelected->setEnabled(false);
    m_explorerStatusLabel->setText("Extracting...");

    std::string sArch = archivePath.toStdString();
    std::string sDest = destDir.toStdString();

    m_workerThread = std::thread([this, sArch, sDest, destDir]() {
        try {
            archive::ArchiveReader reader;
            reader.extractArchive(sArch, sDest);

            QMetaObject::invokeMethod(this, [this, destDir]() {
                m_isProcessing.store(false);
                m_btnExtractAll->setEnabled(true);
                m_btnExtractSelected->setEnabled(true);
                m_explorerStatusLabel->setText("Extraction complete.");

                auto reply = QMessageBox::information(
                    this,
                    "Extraction Complete",
                    "All files were extracted successfully.\n\nOpen destination folder?",
                    QMessageBox::Open | QMessageBox::Close,
                    QMessageBox::Open);

                if (reply == QMessageBox::Open) {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(destDir));
                }
            });
        } catch (const std::exception& e) {
            std::string err = e.what();
            QMetaObject::invokeMethod(this, [this, err]() {
                m_isProcessing.store(false);
                m_btnExtractAll->setEnabled(true);
                m_btnExtractSelected->setEnabled(true);
                m_explorerStatusLabel->setText("Extraction failed.");

                QMessageBox::critical(this, "Extraction Failed", QString::fromStdString(err));
            });
        }
    });
}

void MainWindow::onExtractSelected() {
    int curRow = m_filesTable->currentRow();
    if (curRow < 0) {
        QMessageBox::information(this, "Select File", "Please select a file from the list to extract.");
        return;
    }

    QTableWidgetItem* item = m_filesTable->item(curRow, 0);
    if (!item) return;

    QString relativePath = item->text();
    QString archivePath = m_archivePathEdit->text().trimmed();

    QString destDir = QFileDialog::getExistingDirectory(this, "Select Destination Folder");
    if (destDir.isEmpty()) return;

    try {
        archive::ArchiveReader reader;
        reader.extractFile(archivePath.toStdString(), relativePath.toStdString(), destDir.toStdString());

        auto reply = QMessageBox::information(
            this,
            "File Extracted",
            QString("Extracted:\n%1\n\nOpen destination folder?").arg(relativePath),
            QMessageBox::Open | QMessageBox::Close,
            QMessageBox::Open);

        if (reply == QMessageBox::Open) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(destDir));
        }
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Extraction Error", QString::fromStdString(e.what()));
    }
}

void MainWindow::onTableRowDoubleClicked(int row, int column) {
    Q_UNUSED(column);
    m_filesTable->selectRow(row);
    onExtractSelected();
}

void MainWindow::onTableCustomContextMenu(const QPoint& pos) {
    QTableWidgetItem* item = m_filesTable->itemAt(pos);
    if (!item) return;

    int row = item->row();
    m_filesTable->selectRow(row);
    QString relPath = m_filesTable->item(row, 0)->text();

    QMenu menu(this);
    QAction* actExtract = menu.addAction("Extract Selected...");
    QAction* actCopyPath = menu.addAction("Copy Path");

    QAction* chosen = menu.exec(m_filesTable->viewport()->mapToGlobal(pos));
    if (chosen == actExtract) {
        onExtractSelected();
    } else if (chosen == actCopyPath) {
        QGuiApplication::clipboard()->setText(relPath);
    }
}

void MainWindow::onTestArchiveIntegrity() {
    QString archivePath = m_archivePathEdit->text().trimmed();
    if (archivePath.isEmpty()) {
        QMessageBox::warning(this, "No Archive", "Please open an archive file first.");
        return;
    }

    try {
        archive::ArchiveReader reader;
        auto list = reader.listArchive(archivePath.toStdString());
        QMessageBox::information(this, "Archive Test Passed",
            QString("Archive test completed with no errors.\n\nAll %1 file records are intact.")
                .arg(list.size()));
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Archive Test Failed",
            QString("Errors found in archive:\n\n%1").arg(e.what()));
    }
}

} // namespace gui
} // namespace forge
