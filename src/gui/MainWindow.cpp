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
#include <fstream>
#include <thread>
#include <chrono>

namespace forge {
namespace gui {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    setupUi();
    resize(850, 580);
    setWindowTitle("FORGE — Lossless Archive & Compression Suite");
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

void MainWindow::setupUi() {
    m_tabs = new QTabWidget(this);
    setCentralWidget(m_tabs);

    m_tabs->addTab(createHomeTab(), "Home");
    m_tabs->addTab(createCompressionTab(), "Create Archive");
    m_tabs->addTab(createExplorerTab(), "Archive Explorer");
    m_tabs->addTab(createAboutTab(), "About");
}

QWidget* MainWindow::createHomeTab() {
    QWidget* tab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(30, 30, 30, 30);
    layout->setSpacing(20);

    QLabel* title = new QLabel(
        "<h1 style='color: #1a73e8; margin-bottom: 2px;'>FORGE</h1>"
        "<p style='font-size: 14px; color: #5f6368; margin-top: 0;'><b>File Optimization & Recursive Generation Engine</b></p>",
        tab);
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    QLabel* desc = new QLabel(
        "<div style='font-size: 13px; line-height: 1.6; color: #3c4043; text-align: center;'>"
        "Fast, secure, and intelligent lossless file compression with automatic algorithm selection.<br>"
        "FORGE dynamically evaluates <b>Greedy Huffman Coding</b> and <b>Dictionary LZW</b> per block,<br>"
        "guaranteeing maximum space savings with zero file expansion."
        "</div>",
        tab);
    desc->setAlignment(Qt::AlignCenter);
    layout->addWidget(desc);

    // Feature highlights box
    QGroupBox* featBox = new QGroupBox("Core Capabilities", tab);
    QVBoxLayout* featLayout = new QVBoxLayout(featBox);
    featLayout->setSpacing(8);
    featLayout->addWidget(new QLabel(" • <b>Adaptive Block Selection</b>: Selects the smallest output between RAW, Huffman, and LZW per block.", tab));
    featLayout->addWidget(new QLabel(" • <b>Guaranteed Lossless</b>: Cryptographic SHA-256 and CRC-32 integrity validation on every block and file.", tab));
    featLayout->addWidget(new QLabel(" • <b>Low Memory Footprint</b>: Stream-based chunking processes multi-gigabyte archives within a few megabytes of RAM.", tab));
    featLayout->addWidget(new QLabel(" • <b>Path Traversal Security</b>: Built-in sandbox protects against malicious path escape and traversal exploits.", tab));
    layout->addWidget(featBox);

    layout->addStretch();

    // Quick Action buttons
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(15);

    QPushButton* btnCompress = new QPushButton("Create New Archive (.forge)...", tab);
    btnCompress->setFixedHeight(42);
    btnCompress->setStyleSheet("font-weight: bold; font-size: 13px;");
    connect(btnCompress, &QPushButton::clicked, [this]() { m_tabs->setCurrentIndex(1); });

    QPushButton* btnExplore = new QPushButton("Open & Extract Archive...", tab);
    btnExplore->setFixedHeight(42);
    btnExplore->setStyleSheet("font-weight: bold; font-size: 13px;");
    connect(btnExplore, &QPushButton::clicked, [this]() { m_tabs->setCurrentIndex(2); });

    btnLayout->addWidget(btnCompress);
    btnLayout->addWidget(btnExplore);
    layout->addLayout(btnLayout);

    return tab;
}

QWidget* MainWindow::createCompressionTab() {
    QWidget* tab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(25, 25, 25, 25);
    layout->setSpacing(15);

    // Source folder row
    QGroupBox* srcBox = new QGroupBox("Source Selection", tab);
    QVBoxLayout* srcBoxLayout = new QVBoxLayout(srcBox);
    QHBoxLayout* srcRow = new QHBoxLayout();
    srcRow->addWidget(new QLabel("Source Folder:", tab));
    m_sourcePathEdit = new QLineEdit(tab);
    m_sourcePathEdit->setPlaceholderText("Select the directory you wish to compress...");
    srcRow->addWidget(m_sourcePathEdit);
    QPushButton* btnBrowseSrc = new QPushButton("Browse...", tab);
    connect(btnBrowseSrc, &QPushButton::clicked, this, &MainWindow::onBrowseSourceFolder);
    srcRow->addWidget(btnBrowseSrc);
    srcBoxLayout->addLayout(srcRow);
    layout->addWidget(srcBox);

    // Output archive row
    QGroupBox* dstBox = new QGroupBox("Archive Destination", tab);
    QVBoxLayout* dstBoxLayout = new QVBoxLayout(dstBox);
    QHBoxLayout* dstRow = new QHBoxLayout();
    dstRow->addWidget(new QLabel("Target File:", tab));
    m_destArchiveEdit = new QLineEdit(tab);
    m_destArchiveEdit->setPlaceholderText("Path to save the .forge archive...");
    dstRow->addWidget(m_destArchiveEdit);
    QPushButton* btnBrowseDst = new QPushButton("Save As...", tab);
    connect(btnBrowseDst, &QPushButton::clicked, this, &MainWindow::onBrowseDestinationArchive);
    dstRow->addWidget(btnBrowseDst);
    dstBoxLayout->addLayout(dstRow);
    layout->addWidget(dstBox);

    // Compression options
    QGroupBox* optBox = new QGroupBox("Compression Settings", tab);
    QHBoxLayout* optRow = new QHBoxLayout(optBox);
    optRow->addWidget(new QLabel("Block Chunking:", tab));
    m_blockSizeCombo = new QComboBox(tab);
    m_blockSizeCombo->addItem("64 KiB — Balanced (Recommended)", static_cast<qulonglong>(64 * 1024));
    m_blockSizeCombo->addItem("16 KiB — Low Memory Mode", static_cast<qulonglong>(16 * 1024));
    m_blockSizeCombo->addItem("256 KiB — High Compression", static_cast<qulonglong>(256 * 1024));
    m_blockSizeCombo->addItem("1024 KiB — Maximum Compression", static_cast<qulonglong>(1024 * 1024));
    m_blockSizeCombo->setCurrentIndex(0);
    optRow->addWidget(m_blockSizeCombo);
    optRow->addStretch();
    layout->addWidget(optBox);

    // Action button & progress
    m_compressButton = new QPushButton("Compress into .forge Archive", tab);
    m_compressButton->setFixedHeight(44);
    m_compressButton->setStyleSheet("font-weight: bold; font-size: 14px;");
    connect(m_compressButton, &QPushButton::clicked, this, &MainWindow::onStartCompression);
    layout->addWidget(m_compressButton);

    m_compressProgress = new QProgressBar(tab);
    m_compressProgress->setRange(0, 100);
    m_compressProgress->setValue(0);
    m_compressProgress->setTextVisible(true);
    layout->addWidget(m_compressProgress);

    m_compressStatusLabel = new QLabel("Ready to compress.", tab);
    m_compressStatusLabel->setStyleSheet("color: #5f6368; font-size: 12px;");
    layout->addWidget(m_compressStatusLabel);

    layout->addStretch();
    return tab;
}

QWidget* MainWindow::createExplorerTab() {
    QWidget* tab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);

    // Top open bar
    QHBoxLayout* topRow = new QHBoxLayout();
    topRow->addWidget(new QLabel("Archive:", tab));
    m_archivePathEdit = new QLineEdit(tab);
    m_archivePathEdit->setPlaceholderText("Select a .forge archive to inspect or extract...");
    topRow->addWidget(m_archivePathEdit);
    QPushButton* btnOpen = new QPushButton("Open Archive...", tab);
    connect(btnOpen, &QPushButton::clicked, this, &MainWindow::onOpenArchive);
    topRow->addWidget(btnOpen);
    layout->addLayout(topRow);

    // Files table
    m_filesTable = new QTableWidget(0, 5, tab);
    m_filesTable->setHorizontalHeaderLabels({
        "File Path", "Original Size", "Compressed Size", "Space Saved", "Blocks"
    });
    m_filesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_filesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_filesTable->setAlternatingRowColors(true);
    layout->addWidget(m_filesTable);

    // Bottom action bar
    QHBoxLayout* btmRow = new QHBoxLayout();
    m_btnExtract = new QPushButton("Extract All Files...", tab);
    m_btnExtract->setFixedHeight(36);
    m_btnExtract->setStyleSheet("font-weight: bold;");
    connect(m_btnExtract, &QPushButton::clicked, this, &MainWindow::onExtractArchive);
    btmRow->addWidget(m_btnExtract);

    m_btnTestArchive = new QPushButton("Verify Integrity (SHA-256)", tab);
    m_btnTestArchive->setFixedHeight(36);
    connect(m_btnTestArchive, &QPushButton::clicked, this, &MainWindow::onTestArchiveIntegrity);
    btmRow->addWidget(m_btnTestArchive);

    m_explorerStatusLabel = new QLabel("No archive loaded.", tab);
    m_explorerStatusLabel->setStyleSheet("color: #5f6368; margin-left: 10px;");
    btmRow->addWidget(m_explorerStatusLabel);
    btmRow->addStretch();
    layout->addLayout(btmRow);

    return tab;
}

QWidget* MainWindow::createAboutTab() {
    QWidget* tab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(35, 30, 35, 30);
    layout->setSpacing(15);

    QLabel* title = new QLabel("<h2>FORGE Archive Suite</h2><p style='color: #666;'>Version 1.0 — Desktop Edition</p>", tab);
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    QLabel* content = new QLabel(
        "<div style='font-size: 13px; line-height: 1.7; color: #333;'>"
        "<p><b>FORGE (File Optimization & Recursive Generation Engine)</b> is an intelligent, high-density lossless archiver.</p>"
        "<p><b>Core Features:</b></p>"
        "<ul>"
        "<li><b>Adaptive Block Selection:</b> Tests RAW, Huffman, and LZW representations per block and selects the smallest.</li>"
        "<li><b>Zero File Inflation:</b> Guarantees incompressible or pre-compressed files (like JPEG, ZIP, MP4) never expand.</li>"
        "<li><b>Dual-Tier Integrity:</b> Combines block-level CRC-32 checksums with end-to-end cryptographic SHA-256 verification.</li>"
        "<li><b>Secure by Design:</b> Sandboxed extraction prevents path-traversal attacks and malicious archive overwrites.</li>"
        "</ul>"
        "<p>Built using standard modern C++17 and Qt 6.</p>"
        "</div>",
        tab);
    layout->addWidget(content);

    layout->addStretch();
    return tab;
}

void MainWindow::onBrowseSourceFolder() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Folder to Compress");
    if (!dir.isEmpty()) {
        m_sourcePathEdit->setText(dir);
        if (m_destArchiveEdit->text().isEmpty()) {
            m_destArchiveEdit->setText(dir + ".forge");
        }
    }
}

void MainWindow::onBrowseDestinationArchive() {
    QString f = QFileDialog::getSaveFileName(this, "Select Destination .forge Archive", "", "FORGE Archives (*.forge)");
    if (!f.isEmpty()) {
        m_destArchiveEdit->setText(f);
    }
}

void MainWindow::onStartCompression() {
    QString src = m_sourcePathEdit->text();
    QString dst = m_destArchiveEdit->text();
    if (src.isEmpty() || dst.isEmpty()) {
        QMessageBox::warning(this, "Input Required", "Please specify both a source folder and a destination archive file.");
        return;
    }

    if (m_isProcessing.load()) {
        QMessageBox::warning(this, "Busy", "An archive operation is currently in progress.");
        return;
    }

    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }

    size_t blockSize = m_blockSizeCombo->currentData().toULongLong();
    m_compressButton->setEnabled(false);
    m_compressStatusLabel->setText("Preparing compression...");
    m_compressProgress->setValue(0);
    m_isProcessing.store(true);

    std::string sSrc = src.toStdString();
    std::string sDst = dst.toStdString();

    m_workerThread = std::thread([this, sSrc, sDst, blockSize]() {
        auto startTime = std::chrono::high_resolution_clock::now();
        try {
            archive::ArchiveWriter writer;
            writer.compressFolder(sSrc, sDst, blockSize, true, [this](const archive::ArchiveWriter::ProgressInfo& p) {
                int pct = (p.totalBytes > 0) ? static_cast<int>((p.bytesProcessed * 100) / p.totalBytes) : 100;
                QMetaObject::invokeMethod(this, [this, pct, p]() {
                    m_compressProgress->setValue(pct);
                    m_compressStatusLabel->setText(QString("Compressing [%1/%2]: %3 (%4 processed)")
                        .arg(p.fileIndex).arg(p.totalFiles)
                        .arg(QString::fromStdString(p.currentFile))
                        .arg(formatBytes(p.bytesProcessed)));
                });
            });

            auto endTime = std::chrono::high_resolution_clock::now();
            double elapsedSec = std::chrono::duration<double>(endTime - startTime).count();

            QMetaObject::invokeMethod(this, [this, elapsedSec]() {
                m_compressProgress->setValue(100);
                m_compressStatusLabel->setText(QString("Compression complete in %1 seconds!").arg(QString::number(elapsedSec, 'f', 2)));
                m_compressButton->setEnabled(true);
                m_isProcessing.store(false);
                QMessageBox::information(this, "Success", "Archive successfully created and verified!");
            });
        } catch (const std::exception& e) {
            std::string err = e.what();
            QMetaObject::invokeMethod(this, [this, err]() {
                m_compressButton->setEnabled(true);
                m_compressStatusLabel->setText("Compression failed.");
                m_isProcessing.store(false);
                QMessageBox::critical(this, "Compression Error", QString::fromStdString(err));
            });
        }
    });
}

void MainWindow::onOpenArchive() {
    QString f = QFileDialog::getOpenFileName(this, "Open .forge Archive", "", "FORGE Archives (*.forge)");
    if (f.isEmpty()) return;

    m_archivePathEdit->setText(f);
    try {
        archive::ArchiveReader reader;
        auto list = reader.listArchive(f.toStdString());
        m_filesTable->setRowCount(0);

        uint64_t totalOriginal = 0;
        uint64_t totalStored = 0;

        for (const auto& item : list) {
            int row = m_filesTable->rowCount();
            m_filesTable->insertRow(row);

            totalOriginal += item.originalFileSize;
            totalStored += item.storedSize;

            double spaceSaved = 0.0;
            if (item.originalFileSize > 0) {
                spaceSaved = (1.0 - (static_cast<double>(item.storedSize) / item.originalFileSize)) * 100.0;
            }

            m_filesTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(item.relativePath)));
            m_filesTable->setItem(row, 1, new QTableWidgetItem(formatBytes(item.originalFileSize)));
            m_filesTable->setItem(row, 2, new QTableWidgetItem(formatBytes(item.storedSize)));

            QString savedStr = (spaceSaved >= 0) ? QString("%1%").arg(QString::number(spaceSaved, 'f', 1)) : "0.0%";
            m_filesTable->setItem(row, 3, new QTableWidgetItem(savedStr));
            m_filesTable->setItem(row, 4, new QTableWidgetItem(QString::number(item.blockCount)));
        }

        m_explorerStatusLabel->setText(
            QString("Files: %1 | Total: %2 ➔ Stored: %3")
                .arg(list.size())
                .arg(formatBytes(totalOriginal))
                .arg(formatBytes(totalStored)));
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error Reading Archive", e.what());
    }
}

void MainWindow::onExtractArchive() {
    QString archivePath = m_archivePathEdit->text();
    if (archivePath.isEmpty()) {
        QMessageBox::warning(this, "Select Archive", "Please open a .forge archive first.");
        return;
    }

    QString destDir = QFileDialog::getExistingDirectory(this, "Select Destination Folder for Extraction");
    if (destDir.isEmpty()) return;

    if (m_isProcessing.load()) {
        QMessageBox::warning(this, "Busy", "An operation is currently in progress.");
        return;
    }

    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }

    m_isProcessing.store(true);
    m_explorerStatusLabel->setText("Extracting archive...");

    std::string sArch = archivePath.toStdString();
    std::string sDest = destDir.toStdString();

    m_workerThread = std::thread([this, sArch, sDest]() {
        try {
            archive::ArchiveReader reader;
            reader.extractArchive(sArch, sDest);
            QMetaObject::invokeMethod(this, [this]() {
                m_isProcessing.store(false);
                m_explorerStatusLabel->setText("Extraction complete. All files verified.");
                QMessageBox::information(this, "Extraction Complete", "All files extracted successfully!\nSHA-256 cryptographic integrity verified.");
            });
        } catch (const std::exception& e) {
            std::string err = e.what();
            QMetaObject::invokeMethod(this, [this, err]() {
                m_isProcessing.store(false);
                m_explorerStatusLabel->setText("Extraction failed.");
                QMessageBox::critical(this, "Extraction Error", QString::fromStdString(err));
            });
        }
    });
}

void MainWindow::onTestArchiveIntegrity() {
    QString archivePath = m_archivePathEdit->text();
    if (archivePath.isEmpty()) {
        QMessageBox::warning(this, "Select Archive", "Please open a .forge archive first.");
        return;
    }

    try {
        archive::ArchiveReader reader;
        auto list = reader.listArchive(archivePath.toStdString());
        QMessageBox::information(this, "Archive Integrity Valid",
            QString("Archive verification passed!\n\nAll %1 file records and headers are intact and undamaged.")
                .arg(list.size()));
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Integrity Check Failed", QString("Archive corruption detected:\n%1").arg(e.what()));
    }
}

} // namespace gui
} // namespace forge
