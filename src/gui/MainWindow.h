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
#include <QDragEnterEvent>
#include <QDropEvent>

#include <thread>
#include <atomic>
#include <vector>
#include "../archive/ArchiveFormat.h"

namespace forge {
namespace gui {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    // Compress tab slots
    void onBrowseSourceFolder();
    void onBrowseSourceFile();
    void onBrowseDestinationArchive();
    void onStartCompression();
    void onOpenOutputFolder();
    void onSwitchToExplorerWithOutput();

    // Explorer tab slots
    void onOpenArchive(const QString& explicitPath = QString());
    void onExtractAll();
    void onExtractSelected();
    void onTestArchiveIntegrity();
    void onFilterTextChanged(const QString& text);
    void onTableCustomContextMenu(const QPoint& pos);
    void onTableRowDoubleClicked(int row, int column);

private:
    void setupUi();
    void applyLightTheme();
    QWidget* createHeaderBar();
    QWidget* createCompressionView();
    QWidget* createExplorerView();

    void updateExplorerStats(const std::vector<archive::FileMetadata>& list);
    static QString formatBytes(uint64_t bytes);

    QTabWidget* m_tabs{nullptr};

    // Compression View Widgets
    QLineEdit* m_sourcePathEdit{nullptr};
    QLineEdit* m_destArchiveEdit{nullptr};
    QComboBox* m_compressionLevelCombo{nullptr};
    QProgressBar* m_compressProgress{nullptr};
    QLabel* m_compressStatusLabel{nullptr};
    QPushButton* m_compressButton{nullptr};
    QPushButton* m_openOutputDirBtn{nullptr};
    QPushButton* m_inspectInExplorerBtn{nullptr};
    QWidget* m_compressionSuccessBanner{nullptr};
    QLabel* m_compressionSummaryLabel{nullptr};

    QString m_lastOutputArchive;
    bool m_isSourceSingleFile{false};

    // Explorer View Widgets
    QLineEdit* m_archivePathEdit{nullptr};
    QLineEdit* m_searchFilterEdit{nullptr};
    QTableWidget* m_filesTable{nullptr};
    QLabel* m_archiveSummaryLabel{nullptr};
    QLabel* m_explorerStatusLabel{nullptr};
    QPushButton* m_btnExtractAll{nullptr};
    QPushButton* m_btnExtractSelected{nullptr};
    QPushButton* m_btnTestArchive{nullptr};

    std::vector<archive::FileMetadata> m_currentArchiveFiles;

    // Worker thread management
    std::thread m_workerThread;
    std::atomic<bool> m_isProcessing{false};
};

} // namespace gui
} // namespace forge

#endif // FORGE_MAIN_WINDOW_H
