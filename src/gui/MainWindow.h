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

#include <thread>
#include <atomic>

namespace forge {
namespace gui {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    // Compression tab
    void onBrowseSourceFolder();
    void onBrowseDestinationArchive();
    void onStartCompression();

    // Explorer tab
    void onOpenArchive();
    void onExtractArchive();
    void onTestArchiveIntegrity();

private:
    void setupUi();
    QWidget* createHomeTab();
    QWidget* createCompressionTab();
    QWidget* createExplorerTab();
    QWidget* createAboutTab();

    // Helper for formatting byte counts to KB, MB, etc.
    static QString formatBytes(uint64_t bytes);

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
    QPushButton* m_btnExtract{nullptr};
    QPushButton* m_btnTestArchive{nullptr};

    // Worker thread management for safe background execution
    std::thread m_workerThread;
    std::atomic<bool> m_isProcessing{false};
};

} // namespace gui
} // namespace forge

#endif // FORGE_MAIN_WINDOW_H
