#include "MainWindow.h"
#include "../archive/ArchiveWriter.h"
#include "../archive/ArchiveReader.h"
#include "../benchmark/Benchmark.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QApplication>
#include <thread>

namespace forge {
namespace gui {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    setupUi();
    resize(900, 600);
    setWindowTitle("FORGE — Compression & Archive Suite");
}

void MainWindow::setupUi() {
    m_tabs = new QTabWidget(this);
    setCentralWidget(m_tabs);

    m_tabs->addTab(createHomeTab(), "Home");
    m_tabs->addTab(createCompressionTab(), "Compression");
    m_tabs->addTab(createExplorerTab(), "Archive Explorer");
    m_tabs->addTab(createBenchmarkTab(), "Benchmark Laboratory");
}

QWidget* MainWindow::createHomeTab() {
    QWidget* tab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QLabel* title = new QLabel("<h2>FORGE</h2><p><b>Full Optimization & Recursive Generation Engine</b></p>", tab);
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    QLabel* desc = new QLabel(
        "A modular, adaptive lossless compression & archiving system featuring:\n\n"
        " • Adaptive Block-Level Selection (RAW, Huffman, LZW)\n"
        " • Custom .forge Archive Format with Little-Endian Binary Layout\n"
        " • Bounded-Memory Stream Processing (16 KiB - 1024 KiB blocks)\n"
        " • SHA-256 File & CRC-32 Block Integrity Verification\n"
        " • Path-Traversal Security & Malformed Archive Protection",
        tab);
    desc->setAlignment(Qt::AlignCenter);
    layout->addWidget(desc);

    layout->addStretch();

    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* btnCompress = new QPushButton("Go to Compression", tab);
    QPushButton* btnExplore = new QPushButton("Go to Archive Explorer", tab);
    QPushButton* btnBench = new QPushButton("Go to Benchmark Lab", tab);

    connect(btnCompress, &QPushButton::clicked, [this]() { m_tabs->setCurrentIndex(1); });
    connect(btnExplore, &QPushButton::clicked, [this]() { m_tabs->setCurrentIndex(2); });
    connect(btnBench, &QPushButton::clicked, [this]() { m_tabs->setCurrentIndex(3); });

    btnLayout->addWidget(btnCompress);
    btnLayout->addWidget(btnExplore);
    btnLayout->addWidget(btnBench);
    layout->addLayout(btnLayout);

    return tab;
}

QWidget* MainWindow::createCompressionTab() {
    QWidget* tab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(tab);

    // Source folder row
    QHBoxLayout* srcRow = new QHBoxLayout();
    srcRow->addWidget(new QLabel("Source Folder:", tab));
    m_sourcePathEdit = new QLineEdit(tab);
    srcRow->addWidget(m_sourcePathEdit);
    QPushButton* btnBrowseSrc = new QPushButton("Browse...", tab);
    connect(btnBrowseSrc, &QPushButton::clicked, this, &MainWindow::onBrowseSourceFolder);
    srcRow->addWidget(btnBrowseSrc);
    layout->addLayout(srcRow);

    // Destination archive row
    QHBoxLayout* dstRow = new QHBoxLayout();
    dstRow->addWidget(new QLabel("Destination .forge:", tab));
    m_destArchiveEdit = new QLineEdit(tab);
    dstRow->addWidget(m_destArchiveEdit);
    QPushButton* btnBrowseDst = new QPushButton("Save As...", tab);
    connect(btnBrowseDst, &QPushButton::clicked, this, &MainWindow::onBrowseDestinationArchive);
    dstRow->addWidget(btnBrowseDst);
    layout->addLayout(dstRow);

    // Block size row
    QHBoxLayout* optRow = new QHBoxLayout();
    optRow->addWidget(new QLabel("Block Size:", tab));
    m_blockSizeCombo = new QComboBox(tab);
    m_blockSizeCombo->addItem("16 KiB", 16 * 1024);
    m_blockSizeCombo->addItem("64 KiB (Default)", 64 * 1024);
    m_blockSizeCombo->addItem("256 KiB", 256 * 1024);
    m_blockSizeCombo->addItem("1024 KiB", 1024 * 1024);
    m_blockSizeCombo->setCurrentIndex(1);
    optRow->addWidget(m_blockSizeCombo);
    optRow->addStretch();
    layout->addLayout(optRow);

    m_compressButton = new QPushButton("Start Compression", tab);
    connect(m_compressButton, &QPushButton::clicked, this, &MainWindow::onStartCompression);
    layout->addWidget(m_compressButton);

    m_compressProgress = new QProgressBar(tab);
    m_compressProgress->setRange(0, 100);
    m_compressProgress->setValue(0);
    layout->addWidget(m_compressProgress);

    m_compressStatusLabel = new QLabel("Ready.", tab);
    layout->addWidget(m_compressStatusLabel);

    layout->addStretch();
    return tab;
}

QWidget* MainWindow::createExplorerTab() {
    QWidget* tab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QHBoxLayout* topRow = new QHBoxLayout();
    topRow->addWidget(new QLabel("Archive Path:", tab));
    m_archivePathEdit = new QLineEdit(tab);
    topRow->addWidget(m_archivePathEdit);
    QPushButton* btnOpen = new QPushButton("Open Archive...", tab);
    connect(btnOpen, &QPushButton::clicked, this, &MainWindow::onOpenArchive);
    topRow->addWidget(btnOpen);
    layout->addLayout(topRow);

    m_filesTable = new QTableWidget(0, 5, tab);
    m_filesTable->setHorizontalHeaderLabels({"Relative Path", "Original Size", "Stored Size", "Ratio", "Blocks"});
    m_filesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    layout->addWidget(m_filesTable);

    QHBoxLayout* btmRow = new QHBoxLayout();
    QPushButton* btnExtract = new QPushButton("Extract Entire Archive...", tab);
    connect(btnExtract, &QPushButton::clicked, this, &MainWindow::onExtractArchive);
    btmRow->addWidget(btnExtract);
    m_explorerStatusLabel = new QLabel("No archive loaded.", tab);
    btmRow->addWidget(m_explorerStatusLabel);
    btmRow->addStretch();
    layout->addLayout(btmRow);

    return tab;
}

QWidget* MainWindow::createBenchmarkTab() {
    QWidget* tab = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QHBoxLayout* topRow = new QHBoxLayout();
    m_benchmarkRunButton = new QPushButton("Run Standard Benchmark Suite", tab);
    connect(m_benchmarkRunButton, &QPushButton::clicked, this, &MainWindow::onRunBenchmark);
    topRow->addWidget(m_benchmarkRunButton);

    m_benchmarkExportButton = new QPushButton("Export to CSV...", tab);
    connect(m_benchmarkExportButton, &QPushButton::clicked, this, &MainWindow::onExportBenchmarkCSV);
    topRow->addWidget(m_benchmarkExportButton);
    topRow->addStretch();
    layout->addLayout(topRow);

    m_benchmarkTable = new QTableWidget(0, 8, tab);
    m_benchmarkTable->setHorizontalHeaderLabels({
        "Dataset", "Algorithm", "Original", "Stored", "Ratio", "Saved %", "Compress (ms)", "Status"
    });
    m_benchmarkTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    layout->addWidget(m_benchmarkTable);

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
        QMessageBox::warning(this, "Input Required", "Please specify both source folder and destination archive.");
        return;
    }

    size_t blockSize = m_blockSizeCombo->currentData().toULongLong();
    m_compressButton->setEnabled(false);
    m_compressStatusLabel->setText("Compressing...");
    m_compressProgress->setValue(0);

    std::string sSrc = src.toStdString();
    std::string sDst = dst.toStdString();

    std::thread([this, sSrc, sDst, blockSize]() {
        try {
            archive::ArchiveWriter writer;
            writer.compressFolder(sSrc, sDst, blockSize, true, [this](const archive::ArchiveWriter::ProgressInfo& p) {
                int pct = (p.totalBytes > 0) ? static_cast<int>((p.bytesProcessed * 100) / p.totalBytes) : 100;
                QMetaObject::invokeMethod(this, [this, pct, p]() {
                    m_compressProgress->setValue(pct);
                    m_compressStatusLabel->setText(QString("Compressing [%1/%2]: %3")
                        .arg(p.fileIndex).arg(p.totalFiles).arg(QString::fromStdString(p.currentFile)));
                });
            });

            QMetaObject::invokeMethod(this, [this]() {
                m_compressProgress->setValue(100);
                m_compressStatusLabel->setText("Compression complete!");
                m_compressButton->setEnabled(true);
                QMessageBox::information(this, "Success", "Archive successfully created!");
            });
        } catch (const std::exception& e) {
            std::string err = e.what();
            QMetaObject::invokeMethod(this, [this, err]() {
                m_compressButton->setEnabled(true);
                m_compressStatusLabel->setText("Error occurred.");
                QMessageBox::critical(this, "Compression Error", QString::fromStdString(err));
            });
        }
    }).detach();
}

void MainWindow::onOpenArchive() {
    QString f = QFileDialog::getOpenFileName(this, "Open .forge Archive", "", "FORGE Archives (*.forge)");
    if (f.isEmpty()) return;

    m_archivePathEdit->setText(f);
    try {
        archive::ArchiveReader reader;
        auto list = reader.listArchive(f.toStdString());
        m_filesTable->setRowCount(0);

        for (const auto& item : list) {
            int row = m_filesTable->rowCount();
            m_filesTable->insertRow(row);
            m_filesTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(item.relativePath)));
            m_filesTable->setItem(row, 1, new QTableWidgetItem(QString::number(item.originalFileSize)));
            m_filesTable->setItem(row, 2, new QTableWidgetItem(QString::number(item.storedSize)));

            double ratio = (item.storedSize > 0) ? (static_cast<double>(item.originalFileSize) / item.storedSize) : 1.0;
            m_filesTable->setItem(row, 3, new QTableWidgetItem(QString::number(ratio, 'f', 2) + "x"));
            m_filesTable->setItem(row, 4, new QTableWidgetItem(QString::number(item.blockCount)));
        }

        m_explorerStatusLabel->setText(QString("Loaded %1 files.").arg(list.size()));
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error Reading Archive", e.what());
    }
}

void MainWindow::onExtractArchive() {
    QString archivePath = m_archivePathEdit->text();
    if (archivePath.isEmpty()) {
        QMessageBox::warning(this, "Select Archive", "Please open an archive first.");
        return;
    }

    QString destDir = QFileDialog::getExistingDirectory(this, "Select Destination Folder for Extraction");
    if (destDir.isEmpty()) return;

    try {
        archive::ArchiveReader reader;
        reader.extractArchive(archivePath.toStdString(), destDir.toStdString());
        QMessageBox::information(this, "Extraction Complete", "All files extracted and SHA-256 integrity verified!");
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Extraction Error", e.what());
    }
}

void MainWindow::onRunBenchmark() {
    m_benchmarkRunButton->setEnabled(false);
    m_benchmarkTable->setRowCount(0);

    std::thread([this]() {
        auto results = benchmark::BenchmarkSuite::runStandardSuite();
        QMetaObject::invokeMethod(this, [this, results]() {
            for (const auto& r : results) {
                int row = m_benchmarkTable->rowCount();
                m_benchmarkTable->insertRow(row);
                m_benchmarkTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(r.datasetName)));
                m_benchmarkTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(r.algorithmName)));
                m_benchmarkTable->setItem(row, 2, new QTableWidgetItem(QString::number(r.originalSizeBytes)));
                m_benchmarkTable->setItem(row, 3, new QTableWidgetItem(QString::number(r.storedSizeBytes)));
                m_benchmarkTable->setItem(row, 4, new QTableWidgetItem(QString::number(r.compressionRatio, 'f', 2) + "x"));
                m_benchmarkTable->setItem(row, 5, new QTableWidgetItem(QString::number(r.spaceSavedPercent, 'f', 1) + "%"));
                m_benchmarkTable->setItem(row, 6, new QTableWidgetItem(QString::number(r.compressTimeMs, 'f', 2)));
                m_benchmarkTable->setItem(row, 7, new QTableWidgetItem(r.exactRecovery ? "PASS" : "FAIL"));
            }
            m_benchmarkRunButton->setEnabled(true);
        });
    }).detach();
}

void MainWindow::onExportBenchmarkCSV() {
    QString f = QFileDialog::getSaveFileName(this, "Export Benchmark Results", "benchmark_results.csv", "CSV Files (*.csv)");
    if (f.isEmpty()) return;

    auto results = benchmark::BenchmarkSuite::runStandardSuite();
    std::string csv = benchmark::BenchmarkSuite::exportToCSV(results);

    std::ofstream out(f.toStdString());
    if (out) {
        out << csv;
        QMessageBox::information(this, "Exported", "Benchmark results exported to CSV.");
    }
}

} // namespace gui
} // namespace forge
