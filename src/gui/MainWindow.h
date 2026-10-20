#ifndef FORGE_MAIN_WINDOW_H
#define FORGE_MAIN_WINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QProgressBar>
#include <QLabel>
#include <QTableWidget>
#include <QPushButton>

namespace forge {
namespace gui {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    // Compression tab
    void onBrowseSourceFolder();
    void onBrowseDestinationArchive();
    void onStartCompression();

    // Explorer tab
    void onOpenArchive();
    void onExtractArchive();

    // Benchmark tab
    void onRunBenchmark();
    void onExportBenchmarkCSV();

private:
    void setupUi();
    QWidget* createHomeTab();
    QWidget* createCompressionTab();
    QWidget* createExplorerTab();
    QWidget* createBenchmarkTab();

    QTabWidget* m_tabs{nullptr};

    // Compression Tab widgets
    QLineEdit* m_sourcePathEdit{nullptr};
    QLineEdit* m_destArchiveEdit{nullptr};
    QComboBox* m_blockSizeCombo{nullptr};
    QProgressBar* m_compressProgress{nullptr};
    QLabel* m_compressStatusLabel{nullptr};
    QPushButton* m_compressButton{nullptr};

    // Explorer Tab widgets
    QLineEdit* m_archivePathEdit{nullptr};
    QTableWidget* m_filesTable{nullptr};
    QLabel* m_explorerStatusLabel{nullptr};

    // Benchmark Tab widgets
    QTableWidget* m_benchmarkTable{nullptr};
    QPushButton* m_benchmarkRunButton{nullptr};
    QPushButton* m_benchmarkExportButton{nullptr};
};

} // namespace gui
} // namespace forge

#endif // FORGE_MAIN_WINDOW_H
